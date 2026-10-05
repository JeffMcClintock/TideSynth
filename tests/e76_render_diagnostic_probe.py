#!/usr/bin/env python3
"""BACKLOG E76: an unfinished render must report REAPER, not a corrupt fixture.

E76's stated downstream symptom is "an `EOFError` out of Python's `wave` module
on a zero-length render, which reads as a corrupt fixture and is not one", and
it cost a linux run a wrong provisional conclusion about E29. The zero-length
case is the sharp one: `wave` raises EOFError whose message is the EMPTY STRING,
so the operator is handed a traceback with nothing in it.

Five arms and a vacuity control. The controls are the point:

  arm 1  zero-length wav      -> a diagnostic naming REAPER, not a bare EOFError
  arm 2  truncated wav        -> the same
  arm 3  valid -6 dBFS sine   -> still measures -6.0; the guard swallows nothing
  arm 4  valid DIGITAL SILENCE-> still reports SILENCE, not "unusable"
  arm 5  end to end: a REAPER stub that exits 1 leaving a zero-length file
         -> the script prints the cause and returns 1, with no traceback
  arm 6  VACUITY CONTROL: arms 1, 2 and 5 run against the PRE-FIX copy of the
         script and MUST fail there. Without this, arms 1-5 passing says
         nothing about whether anything was fixed.

The baseline is a PINNED COMMIT, not `origin/main`. A41's finding was that a
probe reading a moving ref measures a different thing on every run -- and here
it would be worse than that: pointed at `origin/main`, arm 6 goes RED the moment
this fix merges, because then `origin/main` diagnoses too. `--base <rev>`
overrides the pin.

Arm 4 is the discriminator. A genuinely silent render is the finding this script
exists to report; a guard that called silence "unusable" would destroy the
script's whole purpose while making arms 1 and 2 pass.

    python3 tests/e76_render_diagnostic_probe.py

Exit: 0 measured, 1 a control failed, 2 input unresolvable, 3 vacuous --
the four codes tests/a38_fleet_state.py settled on, for the same reason.
"""
import argparse
import importlib.util
import math
import os
import struct
import subprocess
import sys
import tempfile
import wave

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SCRIPT = os.path.join(REPO, "scripts", "render-and-measure.py")

# The commit this fix was written against -- `main` immediately before it. Arm 6
# needs a copy of the script that does NOT diagnose, so this must stay pinned:
# see the module docstring.
BASE_REV = "e7fba108d"

fails = []


def check(ok, label, detail=""):
    print("  %-4s %s%s" % ("OK" if ok else "FAIL", label,
                           ("  -- " + detail) if detail else ""))
    if not ok:
        fails.append(label)
    return ok


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def zero_length(d):
    p = os.path.join(d, "zero.wav")
    open(p, "wb").close()
    return p


def truncated(d):
    p = os.path.join(d, "trunc.wav")
    open(p, "wb").write(b"RIFF\x24\x00\x00\x00WAVE")
    return p


def tone(d, amplitude):
    """A 1 kHz sine at `amplitude` counts, or digital silence at 0."""
    p = os.path.join(d, "tone-%d.wav" % amplitude)
    with wave.open(p, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(44100)
        w.writeframes(b"".join(
            struct.pack("<h", int(amplitude * math.sin(2 * math.pi * 1000 * t / 44100)))
            for t in range(44100)))
    return p


def diagnosed(mod, path):
    """(was_diagnosed, what_came_back) for mod.analyse(path)."""
    try:
        return False, "returned %r" % (mod.analyse(path),)
    except SystemExit as exc:
        return True, str(exc)
    except BaseException as exc:
        return False, "raised %s(%r)" % (type(exc).__name__, str(exc))


def names_the_cause(text):
    return "REAPER" in text and "corrupt fixture" in text


def reaper_stub(d):
    """An executable that exits 1 after creating a zero-length render file.

    It honours RENDER_FILE out of the staged project exactly as REAPER would, so
    the script's own plumbing is exercised rather than mocked.
    """
    body = os.path.join(d, "stub.py")
    with open(body, "w") as fh:
        fh.write("import re, sys\n")
        fh.write("src = open(sys.argv[-1], encoding='utf-8', errors='replace').read()\n")
        fh.write("m = re.search(r'RENDER_FILE .([^\"]*).', src)\n")
        fh.write("open(m.group(1), 'wb').close()\n")
        fh.write("sys.stderr.write('gdk_screen_get_root_window: assertion failed\\n')\n")
        fh.write("sys.exit(1)\n")
    if sys.platform.startswith("win"):
        shim = os.path.join(d, "reaper-stub.cmd")
        with open(shim, "w") as fh:
            fh.write('@"%s" "%s" %%*\n' % (sys.executable, body))
    else:
        shim = os.path.join(d, "reaper-stub.sh")
        with open(shim, "w") as fh:
            fh.write("#!/bin/sh\n")
            fh.write('exec "%s" "%s" "$@"\n' % (sys.executable, body))
        os.chmod(shim, 0o755)
    return shim


def end_to_end(script, d, tag):
    """(rc, combined output) from running `script` on a fixture via the stub."""
    rpp = os.path.join(d, "e2e-%s.rpp" % tag)
    out = os.path.join(d, "e2e-%s-out.wav" % tag)
    with open(rpp, "w") as fh:
        fh.write('<REAPER_PROJECT 0.1 "7.0" 1\n')
        fh.write("  SAMPLERATE 44100 0 0\n")
        fh.write('  RENDER_FILE "%s"\n' % out)
        fh.write("  RENDER_RANGE 0 0 2 0 1000\n")
        fh.write("  <TRACK\n    NAME e2e\n  >\n>\n")
    env = dict(os.environ, REAPER=reaper_stub(d))
    p = subprocess.run([sys.executable, script, rpp], env=env,
                       capture_output=True, text=True)
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base", default=BASE_REV, metavar="REV",
                    help="the pre-fix revision arm 6 reads (default %s)" % BASE_REV)
    args = ap.parse_args()

    if not os.path.exists(SCRIPT):
        print("INPUT UNRESOLVABLE: %s is missing" % SCRIPT)
        return 2
    head = load(SCRIPT, "ram_head")

    print("--- arms 1-5: this working tree's script ---")
    with tempfile.TemporaryDirectory() as d:
        ok, text = diagnosed(head, zero_length(d))
        check(ok and names_the_cause(text),
              "arm 1  zero-length render is diagnosed",
              text.replace("\n", " | ")[:110])

        ok, text = diagnosed(head, truncated(d))
        check(ok and names_the_cause(text),
              "arm 2  truncated render is diagnosed",
              text.replace("\n", " | ")[:110])

        try:
            pdb, rdb, silent = head.analyse(tone(d, 16384))
            check(abs(pdb - (-6.0)) < 0.5 and not silent,
                  "arm 3  a real -6 dBFS render still measures",
                  "peak=%.2f rms=%.2f silent=%s" % (pdb, rdb, silent))
        except BaseException as exc:
            check(False, "arm 3  a real -6 dBFS render still measures",
                  "raised %s" % type(exc).__name__)

        try:
            pdb, rdb, silent = head.analyse(tone(d, 0))
            check(silent and pdb == float("-inf"),
                  "arm 4  digital silence still reports SILENCE",
                  "peak=%s silent=%s" % (pdb, silent))
        except BaseException as exc:
            check(False, "arm 4  digital silence still reports SILENCE",
                  "raised %s -- a silent render MUST NOT be called unusable"
                  % type(exc).__name__)

        rc, text = end_to_end(SCRIPT, d, "head")
        check(rc == 1 and names_the_cause(text) and "Traceback" not in text,
              "arm 5  end to end: cause reported, rc=1, no traceback",
              "rc=%d traceback=%s" % (rc, "Traceback" in text))

    print("--- arm 6: VACUITY CONTROL, the same arms at %s ---" % args.base)
    with tempfile.TemporaryDirectory() as d:
        base = os.path.join(d, "render-and-measure-base.py")
        ref = "%s:scripts/render-and-measure.py" % args.base
        try:
            blob = subprocess.run(["git", "show", ref], cwd=REPO,
                                  capture_output=True, check=True).stdout
        except (OSError, subprocess.CalledProcessError) as exc:
            print("  INPUT UNRESOLVABLE: cannot read %s (%s)."
                  % (ref, type(exc).__name__))
            print("  Fetch it:  git -C %s fetch origin %s" % (REPO, args.base))
            return 2
        with open(base, "wb") as fh:
            fh.write(blob)
        old = load(base, "ram_base")

        ok, text = diagnosed(old, zero_length(d))
        check(not ok, "arm 6a the baseline does NOT diagnose zero-length",
              text.replace("\n", " | ")[:90])
        ok, text = diagnosed(old, truncated(d))
        check(not ok, "arm 6b the baseline does NOT diagnose a truncated file",
              text.replace("\n", " | ")[:90])
        rc, text = end_to_end(base, d, "base")
        check(rc != 1 or not names_the_cause(text),
              "arm 6c the baseline's end-to-end run does NOT name the cause",
              "rc=%d traceback=%s" % (rc, "Traceback" in text))

    print()
    if fails:
        print("PROBE FAILED (%d): %s" % (len(fails), "; ".join(fails)))
        return 1
    print("PROBE OK -- 5 arms measured, 3 vacuity controls held")
    return 0


if __name__ == "__main__":
    sys.exit(main())
