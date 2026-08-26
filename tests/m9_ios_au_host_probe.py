#!/usr/bin/env python3
"""BACKLOG M9 -- drive the iOS container app as an AUv3 HOST and measure what
comes out of it.

WHY THIS EXISTS
---------------
There is NO AUv3 host in the iOS simulator. GarageBand and AUM are device-only
App Store builds, and the simulator runs simulator-slice binaries only. So
until M9 the iOS AU could be installed, launched and REGISTERED -- all three
verified -- and never once INSTANTIATED. That is the same content-blindness M6
records for `auval`: a plugin that loads empty passes every gate that does not
ask what it contains.

M9's answer is that the container app hosts its own extension, which it has to
ship anyway. This drives that host from a test run.

WHAT IT ASSERTS, AND WHAT THE NUMBER MEANS
------------------------------------------
M9's Accept: "the iOS container app instantiates and plays TIDE, and a rendered
buffer from it shows the same fundamental macOS measures (440.0 Hz, M7)."

440 Hz is not an arbitrary target. The fixture rack is an Oscillator -> Envelope
-> Output chain whose oscillator free-runs at its default -- 5 V at 1 V/oct is
middle A -- and it has been measured at 440.0 Hz through the VST3 (E2a), through
the AU3 on macOS in GarageBand (M7), and by tests/e9_au_rate_probe.mm. So the
same preset through the same wrapper on a different platform is a real
comparison rather than a self-fulfilling one.

THE CONTROL, because "440 Hz" is also what a broken analyser reports
--------------------------------------------------------------------
--selftest runs the analyser against synthetic tones whose answers are known,
including silence (which must report NO frequency, not 0-and-shrug) and 404.25
Hz -- the pitch a rack that believed it was at the wrong sample rate would
produce. If the analyser cannot tell those apart, this probe cannot conclude
anything and says so.

USAGE
  python3 tests/m9_ios_au_host_probe.py --selftest
  python3 tests/m9_ios_au_host_probe.py --app <path/to/TIDE-Rack-AUv3.app> \
                                        --preset <preset.xml>

Exit 0 when the hosted AU rendered audio at the expected fundamental.
"""
import argparse
import math
import os
import plistlib
import re
import shutil
import struct
import subprocess
import sys
import time
import wave

EXPECTED_HZ = 440.0
TOLERANCE_CENTS = 5.0          # the same bar e9_au_rate_probe uses
DEFAULT_SECONDS = 2.0


# --------------------------------------------------------------------------
# analysis
# --------------------------------------------------------------------------
def measure_fundamental(samples, rate):
    """Zero-crossing fundamental, or None when there is nothing to measure.

    Zero-crossing rather than an FFT, deliberately: it is what M7 and
    e9_au_rate_probe use, so the numbers are directly comparable, and for a
    single sustained tone it is both exact enough and impossible to get subtly
    wrong in a way that still looks plausible.

    DC is removed first. A rack with any offset would otherwise cross zero
    twice per cycle at the wrong places, or not at all.
    """
    if not samples:
        return None

    mean = sum(samples) / len(samples)
    x = [s - mean for s in samples]

    peak = max(abs(v) for v in x)
    if peak < 1e-4:
        return None                      # silence: no frequency, not "0 Hz"

    # Hysteresis at a tenth of peak so ripple near the axis cannot manufacture
    # crossings -- the classic way to measure a harmonic as the fundamental.
    hi, lo = 0.1 * peak, -0.1 * peak
    crossings, armed = 0, False
    first = last = None
    for i, v in enumerate(x):
        if not armed and v > hi:
            armed = True
            if first is None:
                first = i
            last = i
            crossings += 1
        elif armed and v < lo:
            armed = False

    if crossings < 2 or first is None or last is None or last == first:
        return None

    cycles = crossings - 1
    seconds = (last - first) / float(rate)
    if seconds <= 0:
        return None
    return cycles / seconds


def cents(a, b):
    if not a or not b:
        return float("inf")
    return 1200.0 * math.log2(a / b)


def read_wav(path):
    with wave.open(path, "rb") as w:
        rate = w.getframerate()
        chans = w.getnchannels()
        width = w.getsampwidth()
        frames = w.readframes(w.getnframes())
    if width != 2:
        raise SystemExit("%s: expected 16-bit PCM, got %d-bit" % (path, width * 8))
    count = len(frames) // 2
    ints = struct.unpack("<%dh" % count, frames[:count * 2])
    left = [ints[i] / 32768.0 for i in range(0, count, chans)]
    return left, rate


# --------------------------------------------------------------------------
# selftest -- the analyser, against answers that are known
# --------------------------------------------------------------------------
def selftest():
    failures = 0

    def check(what, ok):
        nonlocal failures
        print("  %-4s %s" % ("ok" if ok else "FAIL", what))
        if not ok:
            failures += 1

    rate = 48000
    def tone(hz, seconds=1.0, amp=0.5):
        return [amp * math.sin(2 * math.pi * hz * i / rate)
                for i in range(int(seconds * rate))]

    for hz in (440.0, 404.2586, 100.0, 1000.0):
        got = measure_fundamental(tone(hz), rate)
        check("%.4f Hz tone measures %.4f Hz" % (hz, got or -1),
              got is not None and abs(cents(got, hz)) < 1.0)

    check("440 and 404.2586 are distinguishable (the stale-rate hypothesis)",
          abs(cents(measure_fundamental(tone(440.0), rate),
                    measure_fundamental(tone(404.2586), rate))) > 100.0)

    check("digital silence reports NO frequency, not 0",
          measure_fundamental([0.0] * rate, rate) is None)

    check("DC offset alone reports no frequency",
          measure_fundamental([0.4] * rate, rate) is None)

    # A tone riding a large DC offset must still measure -- this is the case
    # the mean-removal above exists for.
    got = measure_fundamental([v + 0.4 for v in tone(440.0)], rate)
    check("440 Hz on a 0.4 DC offset still measures %.2f Hz" % (got or -1),
          got is not None and abs(cents(got, 440.0)) < 1.0)

    print("\n%s: %d check(s) failed" %
          ("SELFTEST FAILED" if failures else "SELFTEST OK", failures))
    return 1 if failures else 0


# --------------------------------------------------------------------------
# driving the simulator
# --------------------------------------------------------------------------
def run(cmd, **kw):
    """subprocess.run with the return code CAPTURED BEFORE anything else runs.

    Spelled out because the alternative bites regularly in this repo: a
    `cmd | tail` pipeline reports tail's status, and a command that failed
    reads as rc=0.
    """
    return subprocess.run(cmd, capture_output=True, text=True, errors="replace", **kw)


def bundle_id_of(app):
    plist = os.path.join(app, "Info.plist")
    if not os.path.exists(plist):
        raise SystemExit("no Info.plist in %s" % app)
    with open(plist, "rb") as f:
        return plistlib.load(f)["CFBundleIdentifier"]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--app", help="the built TIDE-Rack-AUv3.app")
    ap.add_argument("--preset", help="GMPIPRESET xml (scripts/decode_rpp.py --preset-out)")
    ap.add_argument("--device", default="booted")
    ap.add_argument("--seconds", type=float, default=DEFAULT_SECONDS)
    ap.add_argument("--expect-hz", type=float, default=EXPECTED_HZ)
    ap.add_argument("--in-process", action="store_true",
                    help="developer toggle; the default is out-of-process, which "
                         "is what real hosts do and what M9 rules")
    ap.add_argument("--keep", metavar="DIR", help="copy render.wav here")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.app or not args.preset:
        ap.error("--app and --preset are required (or use --selftest)")

    bid = bundle_id_of(args.app)
    print("app       : %s" % args.app)
    print("bundle id : %s" % bid)

    run(["xcrun", "simctl", "uninstall", args.device, bid])
    r = run(["xcrun", "simctl", "install", args.device, args.app])
    if r.returncode != 0:
        print(r.stdout + r.stderr)
        raise SystemExit("simctl install failed rc=%d" % r.returncode)
    print("install   : rc=0")

    # The app has to run once before its data container exists.
    run(["xcrun", "simctl", "launch", args.device, bid])
    time.sleep(3)
    run(["xcrun", "simctl", "terminate", args.device, bid])

    r = run(["xcrun", "simctl", "get_app_container", args.device, bid, "data"])
    if r.returncode != 0:
        raise SystemExit("get_app_container failed: " + r.stderr.strip())
    docs = os.path.join(r.stdout.strip(), "Documents")
    os.makedirs(docs, exist_ok=True)
    shutil.copy(args.preset, os.path.join(docs, "preset.xml"))
    wav = os.path.join(docs, "render.wav")
    if os.path.exists(wav):
        os.remove(wav)
    print("preset    : copied into the app's Documents/")

    # NOT `simctl launch --console`. That attaches to the app's stdio and does
    # not return until the app EXITS -- and this is a GUI app, which never
    # does. The first version of this probe hung for the full timeout on a
    # render that had in fact succeeded in four seconds, which is the most
    # misleading failure available: the artifact was on disk the whole time.
    #
    # So: launch detached, poll for the file, and read the diagnostics back out
    # of the unified log afterwards. NSLog on the simulator goes there, and
    # `log show` is exact and re-runnable after the fact rather than a stream
    # that has to be started before the thing it watches.
    started = time.time()
    launch = ["xcrun", "simctl", "launch", args.device, bid,
              "--gmpi-preset", "preset.xml",
              "--gmpi-render", "%g" % args.seconds]
    if args.in_process:
        launch.append("--gmpi-in-process")

    print("launch    : %s" % " ".join(launch[4:]))
    r = run(launch)
    if r.returncode != 0:
        print(r.stdout + r.stderr)
        raise SystemExit("simctl launch failed rc=%d" % r.returncode)

    # A render of N seconds takes appreciably less than N in wall clock, plus
    # instantiation -- out-of-process means launching another process, which is
    # the slow part. Poll rather than sleep a guessed constant.
    deadline = time.time() + max(60.0, args.seconds * 10 + 45)
    while time.time() < deadline and not os.path.exists(wav):
        time.sleep(0.5)
    # The file appears at fopen, before it is complete; let the write finish.
    if os.path.exists(wav):
        time.sleep(1.0)

    run(["xcrun", "simctl", "terminate", args.device, bid])

    since = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(started - 5))
    log = run(["xcrun", "simctl", "spawn", args.device, "log", "show",
               "--start", since,
               "--predicate", 'eventMessage CONTAINS "GMPI-HOST"',
               "--style", "compact"])
    for line in (log.stdout or "").splitlines():
        if "GMPI-HOST:" in line and "log show" not in line:
            print("   | " + line.split("GMPI-HOST:", 1)[-1].strip())

    if not os.path.exists(wav):
        print("\nNo render.wav was written.")
        print("This is the informative failure: the app ran and the host did not "
              "reach the end of a render, so read the GMPI-HOST lines above.")
        return 1

    if args.keep:
        os.makedirs(args.keep, exist_ok=True)
        shutil.copy(wav, os.path.join(args.keep, "render.wav"))

    samples, rate = read_wav(wav)
    peak = max(abs(s) for s in samples) if samples else 0.0
    hz = measure_fundamental(samples, rate)

    print("\nrendered  : %d frames @ %d Hz, peak %.4f" % (len(samples), rate, peak))
    if hz is None:
        print("fundamental: NONE -- the render is silent.")
        print("\nFAIL: the AU instantiated and rendered, but produced no signal.")
        return 1

    off = cents(hz, args.expect_hz)
    print("fundamental: %.4f Hz  (%.2f cents from %.1f)" % (hz, off, args.expect_hz))

    if abs(off) > TOLERANCE_CENTS:
        print("\nFAIL: %.4f Hz is not %.1f Hz." % (hz, args.expect_hz))
        return 1

    print("\nPASS: the iOS container app hosted its own AUv3 and it played at "
          "%.1f Hz." % args.expect_hz)
    return 0


if __name__ == "__main__":
    sys.exit(main())
