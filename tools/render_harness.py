#!/usr/bin/env python3
"""
TIDE Rack audio verification harness.

Drives SynthEditCL headlessly over its --script interface, renders each test
case to a WAV, and checks the result against a checked-in golden reference.
Emits one JSON report describing every case, and exits nonzero if any case
fails. This is the program CI runs; it is also the program the autonomous
coordinator reads to decide what to do next.

Stdlib only -- no pip install step in CI.

Usage:
    render_harness.py --cli <path-to-SynthEditCL> --modules <factory sem dir>
                      [--cases tests/cases] [--refs tests/references]
                      [--out report.json] [--artifacts artifacts/]
                      [--update-refs] [--filter NAME]

Exit codes:
    0  every case passed
    1  at least one case failed (mismatch, silence, or render error)
    2  harness/configuration error (bad paths, no cases found)
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
import sys
import tempfile
import wave
from dataclasses import dataclass, asdict, field
from pathlib import Path

# --------------------------------------------------------------------------
# Defaults. Deliberately conservative: a case may override any of these.
# --------------------------------------------------------------------------

DEFAULT_RATE = 48000
DEFAULT_DURATION = 2.0

# A rendered case whose peak sits below this is treated as a FAILURE, not a
# pass, even if it happens to match a reference. Rationale: on 2026-08-07 a
# relative --factorysemsfolder produced a perfectly-formed but digitally
# silent WAV while every SynthEditCL command still reported "ok":true. Green
# CI on silence is the exact failure this harness exists to prevent.
SILENCE_FLOOR_DBFS = -90.0

# Cross-platform float drift means byte-equality is not a portable contract.
# Same-platform renders have been observed bit-exact, so the hash is reported
# for information, but the pass/fail gates are the two null-test metrics below.
#
# TWO gates, because they catch different failures:
#
#   RMS  -- whole-file residual energy. Catches broad changes: wrong level,
#           wrong waveform, detuning, filter regressions.
#   PEAK -- largest single-sample deviation. Catches LOCALIZED damage: a click,
#           a dropout, one bad block. Whole-file RMS is nearly blind to these.
#           Measured: a 3-LSB nudge across 200 of 96000 samples yields an RMS
#           residual of only -107.6 dBFS -- comfortably inside any sane RMS
#           tolerance, while being an obvious audible-class defect. Without the
#           peak gate that regression ships green.
#
# Peak tolerance is set just above the ~1 LSB (-90.3 dBFS at 16-bit) that
# cross-platform float rounding can legitimately produce.
NULL_TOLERANCE_DBFS = -100.0
PEAK_DIFF_TOLERANCE_DBFS = -86.0

# --------------------------------------------------------------------------
# Module provenance. Added 2026-08-11 (TIDE E1) after --modules was measured
# NOT to be authoritative on a machine that has run SynthEdit before.
#
# The engine reaches modules through persistent state in its own directory --
# an absolute ModulePath in SynthEdit16.settings.xml, and per-override
# Plugin-Cache-16-*.xml files carrying a previously scanned folder's contents.
# Measured on the Linux box: `-factorysemsfolder /nonexistent` still rendered
# both cases at full level and BYTE-IDENTICAL to the goldens. Redirecting
# XDG_DATA_HOME did not help; isolating HOME did not either.
#
# CI on a clean runner is unaffected -- none of that state exists there, which
# is exactly why the -factorysemsfolder trap below was discoverable in CI and
# is invisible locally. The danger is the reverse direction: reproducing a CI
# failure on a dev box can render green from a module set you did not name.
#
# So the report records where the engine SAID it scanned, and flags any source
# outside --modules. This is reported, never fatal: on a dev box the extra
# source is normal, and failing on it would make the harness unusable exactly
# where a human is trying to debug.
SCAN_LINE_PREFIXES = ("Scanning for factory SEMs in:", "Scanning for 3rd-party SEMs in:")


# --------------------------------------------------------------------------
# WAV helpers
# --------------------------------------------------------------------------

_FMT_FOR_WIDTH = {1: "b", 2: "h", 4: "i"}


@dataclass
class Audio:
    rate: int
    channels: int
    width: int
    samples: tuple
    raw: bytes

    @property
    def full_scale(self) -> float:
        return float(2 ** (8 * self.width - 1))

    @property
    def sha256(self) -> str:
        return hashlib.sha256(self.raw).hexdigest()

    @property
    def peak_dbfs(self) -> float:
        peak = max((abs(s) for s in self.samples), default=0)
        return _dbfs(peak, self.full_scale)


def _dbfs(value: float, full_scale: float) -> float:
    if value <= 0:
        return -math.inf
    return 20.0 * math.log10(value / full_scale)


def read_wav(path: Path) -> Audio:
    with wave.open(str(path), "rb") as w:
        width = w.getsampwidth()
        if width not in _FMT_FOR_WIDTH:
            raise ValueError(f"unsupported sample width {width} in {path}")
        frames = w.getnframes()
        raw = w.readframes(frames)
        count = frames * w.getnchannels()
        samples = struct.unpack(f"<{count}{_FMT_FOR_WIDTH[width]}", raw)
        return Audio(w.getframerate(), w.getnchannels(), width, samples, raw)


def null_test(a: Audio, b: Audio) -> tuple[bool, float, float, str | None]:
    """Subtract b from a.

    Returns (within_tolerance, rms_dBFS, peak_diff_dBFS, reason).
    Both metrics must pass: RMS catches broad regressions, peak catches
    localized ones that RMS averages away.
    """
    if (a.rate, a.channels, a.width) != (b.rate, b.channels, b.width):
        return False, math.inf, math.inf, (
            f"format mismatch: {a.rate}Hz/{a.channels}ch/{a.width*8}bit "
            f"vs {b.rate}Hz/{b.channels}ch/{b.width*8}bit"
        )
    if len(a.samples) != len(b.samples):
        return False, math.inf, math.inf, (
            f"length mismatch: {len(a.samples)} vs {len(b.samples)} samples"
        )
    if not a.samples:
        return False, math.inf, math.inf, "empty render"

    diffs = [x - y for x, y in zip(a.samples, b.samples)]
    rms = math.sqrt(sum(d * d for d in diffs) / len(diffs))
    rms_db = _dbfs(rms, a.full_scale)
    peak_db = _dbfs(max(abs(d) for d in diffs), a.full_scale)

    ok = rms_db <= NULL_TOLERANCE_DBFS and peak_db <= PEAK_DIFF_TOLERANCE_DBFS
    return ok, rms_db, peak_db, None


# --------------------------------------------------------------------------
# Case model
# --------------------------------------------------------------------------


@dataclass
class Case:
    name: str
    script: list           # SynthEditCL verbs, minus the render verb
    source: str            # "$alias:pin" feeding the recorder
    duration: float = DEFAULT_DURATION
    rate: int = DEFAULT_RATE
    source_right: str | None = None
    description: str = ""

    @staticmethod
    def load(path: Path) -> "Case":
        data = json.loads(path.read_text(encoding="utf-8"))
        known = {f for f in Case.__dataclass_fields__}
        unknown = set(data) - known
        if unknown:
            raise ValueError(f"{path.name}: unknown keys {sorted(unknown)}")
        data.setdefault("name", path.stem)
        return Case(**data)


@dataclass
class Result:
    name: str
    passed: bool
    reason: str = ""
    peak_dbfs: float | None = None
    null_dbfs: float | None = None
    peak_diff_dbfs: float | None = None
    sha256: str | None = None
    reference_sha256: str | None = None
    rendered_path: str | None = None
    cli_commands: list = field(default_factory=list)
    module_sources: list = field(default_factory=list)
    foreign_module_sources: list = field(default_factory=list)


# --------------------------------------------------------------------------
# Rendering
# --------------------------------------------------------------------------


def engine_version(cli: Path, modules: Path) -> str:
    """Best-effort engine build string, e.g. 'SynthEditCL V1.6.174'.

    Recorded in every report. Engine dependencies are deliberately unpinned
    while the project is pre-1.0, so a reference can fail because the engine
    moved rather than because TIDE Rack regressed. Stamping the version into
    the report makes that distinguishable at a glance -- compare the version
    in the failing report against the last green one before assuming the
    regression is yours.
    """
    try:
        proc = subprocess.run(
            [str(cli.resolve()), "-factorysemsfolder", str(modules.resolve()),
             "--script", "-"],
            input="", capture_output=True, text=True, timeout=180,
        )
    except Exception as exc:            # noqa: BLE001 - diagnostics only
        return f"unknown ({exc.__class__.__name__})"

    for line in (proc.stderr + proc.stdout).splitlines():
        line = line.strip()
        if line.startswith("SynthEditCL") and any(c.isdigit() for c in line):
            return line
    return "unknown"


def scan_sources(blob: str) -> list:
    """Folders the engine reported scanning, in the order it reported them."""
    found = []
    for line in blob.splitlines():
        line = line.strip()
        for prefix in SCAN_LINE_PREFIXES:
            if line.startswith(prefix):
                path = line[len(prefix):].strip()
                if path and path not in found:
                    found.append(path)
    return found


def foreign_sources(sources: list, modules: Path) -> list:
    """Scanned folders that are not under --modules.

    A non-empty result means the render could have drawn modules from
    somewhere other than the folder under test. Informational, not fatal --
    see the SCAN_LINE_PREFIXES note above for why.
    """
    root = modules.resolve()
    outside = []
    for s in sources:
        try:
            candidate = Path(s).resolve()
        except OSError:
            outside.append(s)
            continue
        if candidate != root and root not in candidate.parents:
            outside.append(s)
    return outside


def render(cli: Path, modules: Path, case: Case, out_wav: Path) -> tuple[bool, str, list, list]:
    """Run one case through SynthEditCL. Returns (ok, reason, parsed_json, scanned)."""
    render_verb = (
        f"--render-audio {out_wav} --from {case.source} "
        f"--duration {case.duration} --rate {case.rate}"
    )
    if case.source_right:
        render_verb += f" --from-r {case.source_right}"

    script = "\n".join([*case.script, render_verb]) + "\n"

    # -factorysemsfolder MUST be absolute. A relative path lets the editor
    # resolve modules while the DSP loader silently fails, yielding a
    # well-formed but silent WAV with every command still reporting ok.
    argv = [
        str(cli.resolve()),
        "-rescan",
        "-factorysemsfolder", str(modules.resolve()),
        "--script", "-",
    ]

    proc = subprocess.run(
        argv, input=script, capture_output=True, text=True, timeout=600
    )

    scanned = scan_sources(proc.stdout + proc.stderr)

    commands = []
    for line in proc.stdout.splitlines():
        line = line.strip()
        if line.startswith("{"):
            try:
                commands.append(json.loads(line))
            except json.JSONDecodeError:
                pass

    failed = [c for c in commands if c.get("ok") is False]
    if failed:
        first = failed[0]
        return False, f"CLI error on '{first.get('cmd')}': {first.get('error')}", commands, scanned

    # SynthEditCL reports missing DSP modules on stdout/stderr but still
    # returns ok:true and writes a silent file -- catch it explicitly.
    blob = proc.stdout + proc.stderr
    if "MISSING MODULES" in blob:
        missing = [
            ln.strip() for ln in blob.splitlines()
            if ln.strip() and ln.startswith("    ") and "MISSING" not in ln
        ]
        return False, f"DSP modules missing: {', '.join(missing[:5]) or 'unknown'}", commands, scanned

    if not out_wav.exists():
        return False, "no WAV produced", commands, scanned

    return True, "", commands, scanned


def run_case(cli: Path, modules: Path, case: Case, refs: Path,
             artifacts: Path | None, update: bool) -> Result:
    target = (artifacts / f"{case.name}.wav") if artifacts else None
    with tempfile.TemporaryDirectory() as tmp:
        wav = Path(tmp) / f"{case.name}.wav"
        ok, reason, commands, scanned = render(cli, modules, case, wav)
        foreign = foreign_sources(scanned, modules)
        prov = dict(module_sources=scanned, foreign_module_sources=foreign)
        if not ok:
            return Result(case.name, False, reason, cli_commands=commands, **prov)

        rendered = read_wav(wav)

        if target:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(wav.read_bytes())

        # Gate 1: the render must contain signal.
        if rendered.peak_dbfs <= SILENCE_FLOOR_DBFS:
            return Result(
                case.name, False,
                f"render is silent (peak {rendered.peak_dbfs:.1f} dBFS <= "
                f"floor {SILENCE_FLOOR_DBFS} dBFS)",
                peak_dbfs=rendered.peak_dbfs, sha256=rendered.sha256,
                rendered_path=str(target) if target else None,
                cli_commands=commands, **prov,
            )

        ref_path = refs / f"{case.name}.wav"

        if update:
            ref_path.parent.mkdir(parents=True, exist_ok=True)
            ref_path.write_bytes(wav.read_bytes())
            return Result(case.name, True, "reference updated",
                          peak_dbfs=rendered.peak_dbfs, sha256=rendered.sha256,
                          rendered_path=str(target) if target else None,
                          cli_commands=commands, **prov)

        if not ref_path.exists():
            return Result(
                case.name, False,
                f"no reference at {ref_path} (run with --update-refs to create)",
                peak_dbfs=rendered.peak_dbfs, sha256=rendered.sha256,
                rendered_path=str(target) if target else None,
                cli_commands=commands, **prov,
            )

        # Gate 2: null-test against the golden reference.
        reference = read_wav(ref_path)
        within, rms_db, peak_db, why = null_test(rendered, reference)

        if within:
            reason = ""
        elif why:
            reason = why
        else:
            breached = []
            if rms_db > NULL_TOLERANCE_DBFS:
                breached.append(
                    f"RMS residual {rms_db:.1f} dBFS > {NULL_TOLERANCE_DBFS} dBFS")
            if peak_db > PEAK_DIFF_TOLERANCE_DBFS:
                breached.append(
                    f"peak sample diff {peak_db:.1f} dBFS > "
                    f"{PEAK_DIFF_TOLERANCE_DBFS} dBFS (localized glitch)")
            reason = "; ".join(breached)

        return Result(
            case.name, within, reason,
            peak_dbfs=rendered.peak_dbfs,
            null_dbfs=None if rms_db == math.inf else rms_db,
            peak_diff_dbfs=None if peak_db == math.inf else peak_db,
            sha256=rendered.sha256, reference_sha256=reference.sha256,
            rendered_path=str(target) if target else None,
            cli_commands=commands, **prov,
        )


# --------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="TIDE Rack audio verification harness")
    p.add_argument("--cli", required=True, type=Path, help="path to SynthEditCL")
    p.add_argument("--modules", required=True, type=Path, help="factory .sem folder")
    p.add_argument("--cases", type=Path, default=Path("tests/cases"))
    p.add_argument("--refs", type=Path, default=Path("tests/references"))
    p.add_argument("--out", type=Path, default=Path("report.json"))
    p.add_argument("--artifacts", type=Path, default=None,
                   help="keep rendered WAVs here for CI upload")
    p.add_argument("--update-refs", action="store_true",
                   help="overwrite references with this run (explicit, never automatic)")
    p.add_argument("--filter", default=None, help="only run cases whose name contains this")
    args = p.parse_args(argv)

    if not args.cli.exists():
        print(f"error: SynthEditCL not found at {args.cli}", file=sys.stderr)
        return 2
    if not args.modules.is_dir():
        print(f"error: modules folder not found at {args.modules}", file=sys.stderr)
        return 2

    case_files = sorted(args.cases.glob("*.json"))
    if args.filter:
        case_files = [f for f in case_files if args.filter in f.stem]
    if not case_files:
        print(f"error: no cases found in {args.cases}", file=sys.stderr)
        return 2

    engine = engine_version(args.cli, args.modules)
    print(f"engine: {engine}\n")

    results = []
    for f in case_files:
        try:
            case = Case.load(f)
        except Exception as exc:
            results.append(Result(f.stem, False, f"bad case file: {exc}"))
            continue
        results.append(run_case(args.cli, args.modules, case, args.refs,
                                args.artifacts, args.update_refs))

    passed = [r for r in results if r.passed]
    failed = [r for r in results if not r.passed]

    report = {
        "schema": "tide-rack.audio-verify/2",
        # Engine deps are unpinned pre-1.0 by design. If a case starts failing,
        # diff this against the last green report before assuming TIDE Rack
        # broke -- the engine may simply have moved.
        "engine": engine,
        "totals": {"cases": len(results), "passed": len(passed), "failed": len(failed)},
        "config": {
            "silence_floor_dbfs": SILENCE_FLOOR_DBFS,
            "null_tolerance_dbfs": NULL_TOLERANCE_DBFS,
            "peak_diff_tolerance_dbfs": PEAK_DIFF_TOLERANCE_DBFS,
            "cli": str(args.cli),
            "modules": str(args.modules),
            "updated_references": bool(args.update_refs),
        },
        # Where the engine said it scanned, unioned over all cases. Compare
        # against "modules" above: anything in foreign_module_sources means
        # this run could have drawn modules from a folder you did not name.
        "module_sources": sorted({s for r in results for s in r.module_sources}),
        "foreign_module_sources": sorted({s for r in results for s in r.foreign_module_sources}),
        "cases": [asdict(r) for r in results],
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2), encoding="utf-8")

    for r in results:
        mark = "PASS" if r.passed else "FAIL"
        detail = ""
        if r.peak_dbfs is not None:
            detail += f" peak={r.peak_dbfs:.1f}dBFS"
        if r.null_dbfs is not None:
            detail += f" null={r.null_dbfs:.1f}dBFS"
        if r.peak_diff_dbfs is not None:
            detail += f" peakdiff={r.peak_diff_dbfs:.1f}dBFS"
        print(f"{mark}  {r.name}{detail}" + (f"  -- {r.reason}" if r.reason else ""))

    foreign = sorted({s for r in results for s in r.foreign_module_sources})
    if foreign:
        print("\nwarning: engine scanned folders outside --modules; this run does")
        print("         not prove the named module set is what rendered:")
        for s in foreign:
            print(f"           {s}")
        print("         (normal on a developer box, never on a clean CI runner)")

    print(f"\n{len(passed)}/{len(results)} passed. Report: {args.out}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
