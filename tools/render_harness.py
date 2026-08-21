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
#
# BOTH DEFAULTS CONFIRMED BY MEASUREMENT, 2026-08-14 (TIDE E1a), on the first
# cross-platform render the harness has ever done: macOS against the Linux
# goldens, three independent engines (local Release V1.6.175, local Debug
# V1.6.182, published V1.6.183), all agreeing.
#
# voice_midi_note -- the case whose residual is PURE ROUNDING -- came back at
# RMS -123.1 dBFS and peak -90.3 dBFS, i.e. exactly 1 LSB on 51 of 95,999
# samples (0.053%). So:
#
#   * The RMS gate keeps 22.9 dB of headroom. E1 finding (e) feared the
#     opposite: worst-case arithmetic (1 LSB on EVERY sample = -90.3 dBFS RMS)
#     says this gate tolerates 1-LSB error on only ~10.7% of samples, and
#     warned the first cross-platform render would fail spuriously. It did not.
#     Real cross-platform rounding touched 0.053% of samples -- 200x inside
#     that budget -- because the two builds agree on almost every sample and
#     disagree only where a value sits on a quantisation boundary. The gate was
#     NOT widened: the number was never the problem, the missing measurement
#     was. Finding (e) is closed, not deferred.
#   * The peak gate keeps 4.3 dB of headroom, and that is structurally safe
#     rather than lucky: 1 LSB is a HARD per-sample ceiling for rounding-class
#     drift. Two builds that agree on the underlying float value can only
#     disagree about which way it rounds. Drift of this class can affect more
#     samples; it cannot make any one sample wrong by more than 1 LSB.
#
# WHAT DOES NOT FIT THIS MODEL, and why per-case overrides now exist.
#
# osc_naive_sine failed, at RMS -73.5 dBFS / peak -68.7 dBFS (12 LSB) -- five
# orders of magnitude outside the rounding budget, and identical on all three
# macOS engines, so not an engine-version or build-config artifact. Analysed:
# the residual GROWS MONOTONICALLY through the render (-96 dBFS in the first
# 0.1 s block, -69 dBFS in the last) with zero best-fit time lag. That is not
# rounding, it is a frequency offset integrating into phase error: fitting
# dphi = k*t gives 0.15 ppm, i.e. ~2.5 ULP at SINGLE precision (2^-24).
#
# The pitch table and the phase increment are both double, but the path from
# pitch to increment is not: OscillatorNaive.h:66 computes the table index as
# a float from a float pitch, so the interpolation carries single-precision
# resolution. A few ULP there is the right order for what was measured. (Named
# as the plausible locus, not proven -- the measured quantity is the 2.5 ULP.)
#
# The structural consequence is what matters here: a free-running oscillator's
# residual is UNBOUNDED IN RENDER DURATION. It grows linearly with time, so no
# fixed dBFS number is a duration-independent statement about it. Widening the
# global gates to pass a 2 s sine would have to reach -67 dBFS, which sails
# straight past finding (b)'s reference defect (a 3-LSB nudge over 200 samples,
# caught at peak -80.8 dBFS) -- and would fail again the moment a case renders
# for 4 s. So the global gates stay where they are, and a case that legitimately
# accumulates drift declares its own budget with a written reason. See
# tests/cases/osc_naive_sine.json.
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


def null_test(a: Audio, b: Audio,
              rms_tolerance: float = NULL_TOLERANCE_DBFS,
              peak_tolerance: float = PEAK_DIFF_TOLERANCE_DBFS,
              ) -> tuple[bool, float, float, str | None]:
    """Subtract b from a.

    Returns (within_tolerance, rms_dBFS, peak_diff_dBFS, reason).
    Both metrics must pass: RMS catches broad regressions, peak catches
    localized ones that RMS averages away.

    The tolerances default to the module-level rounding-class budget; a case
    may widen them for a residual that is a different class of thing (see the
    NULL_TOLERANCE_DBFS comment block).
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

    ok = rms_db <= rms_tolerance and peak_db <= peak_tolerance
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

    # Per-case null-test budget. Omitted means the module-level default, which
    # is the rounding-class budget every case should be held to. Set these ONLY
    # for a case whose residual is a different class of thing -- E1a's
    # oscillator phase drift is the founding example -- and say why in
    # tolerance_reason, which the harness prints and puts in the report so a
    # widened gate can never be invisible.
    null_tolerance_dbfs: float | None = None
    peak_diff_tolerance_dbfs: float | None = None
    tolerance_reason: str = ""

    @property
    def null_tolerance(self) -> float:
        return (NULL_TOLERANCE_DBFS if self.null_tolerance_dbfs is None
                else self.null_tolerance_dbfs)

    @property
    def peak_tolerance(self) -> float:
        return (PEAK_DIFF_TOLERANCE_DBFS if self.peak_diff_tolerance_dbfs is None
                else self.peak_diff_tolerance_dbfs)

    @property
    def overridden(self) -> bool:
        return (self.null_tolerance_dbfs is not None
                or self.peak_diff_tolerance_dbfs is not None)

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
    # The tolerances this case was actually judged against, and why they are
    # not the defaults. Recorded per case, not just globally, so a report can
    # never look stricter than the run that produced it.
    null_tolerance_dbfs: float = NULL_TOLERANCE_DBFS
    peak_diff_tolerance_dbfs: float = PEAK_DIFF_TOLERANCE_DBFS
    tolerance_overridden: bool = False
    tolerance_reason: str = ""


# --------------------------------------------------------------------------
# Rendering
# --------------------------------------------------------------------------


def write_reference_provenance(ref_path: Path, case, rendered,
                               cli: Path, modules: Path) -> Path:
    """Record WHICH PLATFORM produced a reference, beside the reference.

    BACKLOG E1c. A null-test number is uninterpretable without this. The same
    residual means "rounding, ignore it" if both sides ran on one platform and
    "cross-platform drift, size your gates for it" if they did not -- and
    nothing in tests/references/ recorded which had happened. Establishing that
    for E1c's four cases took reading a journal entry from nine days earlier;
    the numbers were only comparable because one run happened to produce them
    all, which is luck rather than method.

    Written only by --update-refs, i.e. exactly when a reference is created, by
    the run that created it. A sidecar rather than a field inside the WAV so
    that the reference stays a plain file any tool can read, and so that
    backfilled records can be marked as reconstructed (see `recorded`).
    """
    import platform as _platform

    meta = {
        "case": case.name,
        "reference_sha256": rendered.sha256,
        "seeded_on": {
            "system": _platform.system(),        # Darwin / Linux / Windows
            "release": _platform.release(),
            "machine": _platform.machine(),      # arm64 / x86_64
        },
        "engine": engine_version(cli, modules),
        "duration_s": case.duration,
        "rate": case.rate,
        # How this record came to exist -- one of:
        #   measured       written by the run that seeded the reference. The
        #                  only first-hand record; this function only ever
        #                  writes this value.
        #   reconstructed  backfilled afterwards from journal or case-file
        #                  evidence, with that evidence quoted in `evidence`.
        #                  Weaker: it says what someone wrote down later.
        #   unknown        nobody recorded it and it could not be established.
        #                  `seeded_on` is null. A cross-platform claim about
        #                  such a case is not supported by anything.
        "recorded": "measured",
    }
    out = ref_path.with_suffix(".provenance.json")
    out.write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n")
    return out


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
        prov = dict(module_sources=scanned, foreign_module_sources=foreign,
                    null_tolerance_dbfs=case.null_tolerance,
                    peak_diff_tolerance_dbfs=case.peak_tolerance,
                    tolerance_overridden=case.overridden,
                    tolerance_reason=case.tolerance_reason)
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
            write_reference_provenance(ref_path, case, rendered, cli, modules)
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
        within, rms_db, peak_db, why = null_test(
            rendered, reference, case.null_tolerance, case.peak_tolerance)

        if within:
            reason = ""
        elif why:
            reason = why
        else:
            breached = []
            if rms_db > case.null_tolerance:
                breached.append(
                    f"RMS residual {rms_db:.1f} dBFS > {case.null_tolerance} dBFS")
            if peak_db > case.peak_tolerance:
                breached.append(
                    f"peak sample diff {peak_db:.1f} dBFS > "
                    f"{case.peak_tolerance} dBFS (localized glitch)")
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
# Self-test. Added 2026-08-14 (E1a) alongside the per-case tolerance override.
#
# Two things it makes permanent. First, the override plumbing: a widened gate
# is exactly the kind of mechanism that quietly becomes a blanket pass, so the
# check that it does NOT leak to other cases has to outlive the run that added
# it. Second, the dBFS arithmetic that E1's findings (b) and (e) are argued
# from -- those numbers are quoted as settled fact in three documents, and
# nothing executed them until now.
#
# Synthesises its audio in memory: no fixture WAVs, no engine, runs anywhere.
# --------------------------------------------------------------------------


def _synth(samples: list) -> Audio:
    raw = struct.pack(f"<{len(samples)}h", *samples)
    return Audio(48000, 1, 2, tuple(samples), raw)


def selftest() -> int:
    failures = []

    def check(label, got, want, tol=0.05):
        ok = (abs(got - want) <= tol) if isinstance(want, float) else (got == want)
        print(f"  {'ok ' if ok else 'XX '}{label}: got {got}, want {want}")
        if not ok:
            failures.append(label)

    n = 96000
    base = [int(16000 * math.sin(2 * math.pi * 440 * i / 48000)) for i in range(n)]
    ref = _synth(base)

    # E1 finding (e)'s premise: 1 LSB on EVERY sample is -90.3 dBFS RMS, and
    # that is also the per-sample ceiling for rounding-class drift.
    _, rms, peak, _ = null_test(_synth([s + 1 for s in base]), ref)
    check("1 LSB on every sample, RMS dBFS", round(rms, 1), -90.3)
    check("1 LSB on every sample, peak dBFS", round(peak, 1), -90.3)

    # E1 finding (b)'s reference defect: 3 LSB across 200 of 96,000 samples.
    # RMS is nearly blind to it; the peak gate is what catches it.
    glitched = list(base)
    for i in range(40000, 40200):
        glitched[i] += 3
    ok, rms, peak, _ = null_test(_synth(glitched), ref)
    check("finding (b) defect, RMS dBFS", round(rms, 1), -107.6)
    check("finding (b) defect, peak dBFS", round(peak, 1), -80.8)
    check("finding (b) defect is caught at the default gates", ok, False)

    # The RMS gate's actual budget, which finding (e) computed as ~10.7% of
    # samples carrying 1 LSB. Straddle it: 10% must pass, 12% must fail.
    for pct, want_pass in ((10, True), (12, False)):
        nudged = [s + (1 if i % 100 < pct else 0) for i, s in enumerate(base)]
        ok, rms, _, _ = null_test(_synth(nudged), ref)
        check(f"1 LSB on {pct}% of samples passes RMS gate ({rms:.1f} dBFS)",
              ok or rms <= NULL_TOLERANCE_DBFS, want_pass)

    # Per-case override plumbing.
    plain = Case(name="plain", script=[], source="$x:0")
    wide = Case(name="wide", script=[], source="$x:0",
                null_tolerance_dbfs=-67.0, peak_diff_tolerance_dbfs=-62.0)
    check("a case with no override uses the default RMS gate",
          plain.null_tolerance, NULL_TOLERANCE_DBFS)
    check("a case with no override uses the default peak gate",
          plain.peak_tolerance, PEAK_DIFF_TOLERANCE_DBFS)
    check("a case with no override reports overridden=False", plain.overridden, False)
    check("an overriding case reports overridden=True", wide.overridden, True)
    check("an override does not mutate the module defaults",
          (NULL_TOLERANCE_DBFS, PEAK_DIFF_TOLERANCE_DBFS), (-100.0, -86.0))

    # A residual that the wide gates admit must still be rejected by the
    # defaults -- i.e. widening is per-case, not global.
    loud = [int(s * 1.0002) for s in base]
    ok_wide, rms, _, _ = null_test(_synth(loud), ref, wide.null_tolerance, wide.peak_tolerance)
    ok_default, _, _, _ = null_test(_synth(loud), ref, plain.null_tolerance, plain.peak_tolerance)
    check(f"wide gates admit a {rms:.1f} dBFS residual", ok_wide, True)
    check("default gates reject the same residual", ok_default, False)

    # E1c -- reference provenance. Covered here rather than only in a real
    # --update-refs run, because --update-refs needs an engine and this file's
    # whole point is that it runs without one. engine_version() degrades to
    # "unknown (...)" against a nonexistent CLI, which is the behaviour we want
    # anyway: a missing engine must not stop the platform being recorded.
    import platform as _platform
    with tempfile.TemporaryDirectory() as _tmp:
        _ref = Path(_tmp) / "probe.wav"
        _ref.write_bytes(b"not really a wav")

        class _R:                       # only .sha256 is read
            sha256 = "deadbeef"

        _case = Case(name="probe", script=[], source="$x:0",
                     duration=2.0, rate=48000)
        _out = write_reference_provenance(_ref, _case, _R(),
                                          Path("/nonexistent/SynthEditCL"),
                                          Path("/nonexistent/modules"))
        check("provenance lands beside the reference",
              _out.name, "probe.provenance.json")
        _meta = json.loads(_out.read_text())
        check("provenance records the seeding platform",
              _meta["seeded_on"]["system"], _platform.system())
        check("provenance records the architecture",
              _meta["seeded_on"]["machine"], _platform.machine())
        check("provenance records the reference hash",
              _meta["reference_sha256"], "deadbeef")
        check("a first-hand record is marked measured",
              _meta["recorded"], "measured")
        check("a missing engine does not block the record",
              _meta["engine"].startswith("unknown"), True)

    print(f"\nselftest: {'PASSED' if not failures else str(len(failures)) + ' FAILED'}")
    return 1 if failures else 0


# --------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="TIDE Rack audio verification harness")
    p.add_argument("--selftest", action="store_true",
                   help="check the gate arithmetic and tolerance plumbing; no engine needed")
    p.add_argument("--cli", type=Path, help="path to SynthEditCL")
    p.add_argument("--modules", type=Path, help="factory .sem folder")
    p.add_argument("--cases", type=Path, default=Path("tests/cases"))
    p.add_argument("--refs", type=Path, default=Path("tests/references"))
    p.add_argument("--out", type=Path, default=Path("report.json"))
    p.add_argument("--artifacts", type=Path, default=None,
                   help="keep rendered WAVs here for CI upload")
    p.add_argument("--update-refs", action="store_true",
                   help="overwrite references with this run (explicit, never automatic)")
    p.add_argument("--filter", default=None, help="only run cases whose name contains this")
    args = p.parse_args(argv)

    if args.selftest:
        return selftest()

    if args.cli is None or args.modules is None:
        print("error: --cli and --modules are required (or use --selftest)", file=sys.stderr)
        return 2
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
        # /3 adds the per-case tolerance fields (E1a, 2026-08-14). The two
        # numbers under "config" are the DEFAULTS; a case that overrode them
        # says so in its own entry, so read the case, not the config, when
        # asking what a given result was actually judged against.
        "schema": "tide-rack.audio-verify/3",
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
        # A widened gate must never be invisible in the output that a human
        # actually reads -- that is the whole risk of allowing overrides.
        if r.tolerance_overridden:
            print(f"      relaxed gates: rms<={r.null_tolerance_dbfs} dBFS "
                  f"peak<={r.peak_diff_tolerance_dbfs} dBFS "
                  f"(defaults {NULL_TOLERANCE_DBFS}/{PEAK_DIFF_TOLERANCE_DBFS})")
            if r.tolerance_reason:
                print(f"      reason: {r.tolerance_reason}")

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
