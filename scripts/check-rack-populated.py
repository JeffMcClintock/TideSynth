#!/usr/bin/env python3
"""Assert that a TiDE build comes up with its rack actually POPULATED.

BACKLOG M6, filed out of M5. `auval` is not a sufficient shipping gate, and the
way we learned that is the whole reason this script exists:

    Release, main, 2026-08-23 to 2026-08-25 --
        `auval -v aumu Drck Dsyh` -> exit 0, AU VALIDATION SUCCEEDED

    ...on a plugin that, in its own words, had
        "ControlsXp.xml missing from bundle resources - those controls will
         have no pins"                                            (x4 files)
        "no Prefabs folder in bundle resources - the rack module browser will
         be empty"
        "MidiCv.synthedit did not insert a container - the rack will have no
         MIDI jacks"

`auval` validates the AU *interface* -- parameters, render, MIDI, bad-input
handling -- and never asks whether the plugin CONTAINS anything. A resource
resolution failure that empties the entire rack is invisible to it. So the
plugin shipped, validated and ran with no control pins, an empty module browser
and no MIDI jacks, for two days, with every gate green.

TiDE already emits everything needed to catch that; nothing read it. This does.

WHY THIS ASSERTS POSITIVES RATHER THAN GREPPING FOR THE NEGATIVE LINES.
The tempting cheap version is "fail if any of those three messages appears".
That version is blind to the failure it most needs to catch. When this script
was written, `seedPrefabsFromBundle` opened with:

    const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
    if (resourceFolder.empty())
        return;                       // <-- no message of any kind

An empty resource folder -- which is one plausible shape of exactly the M5
BundleInfo defect -- produced a rack with no prefabs and SAID NOTHING. A
negative-line scan passes it. Requiring the positive line ("N rack prefab(s)
seeded from the bundle") fails it, because the line is simply absent. The rule
is: a silent plugin is a failing plugin. We check for both, but the positive
assertions are the load-bearing half.

BACKLOG M8 has since given that branch a voice -- "bundle resource folder did
not resolve" is now in FATAL_LINES below, so the case names itself instead of
being inferred from an absence. THAT DOES NOT DEMOTE THE POSITIVE ASSERTIONS,
and reading it that way would undo the whole argument above. A message can only
catch the silence someone already thought of; the positive line catches the
next one too. The two fixtures in tests/rack-content/ keep both halves honest:
`silent-empty-rack.log` is still a pure absence and must still fail.

THE CAPTURE PROBLEM, AND WHY THE AU3 ARM NEEDS os_log.
An app extension's stderr reaches neither the terminal nor the unified log, and
its /tmp is not /tmp -- both measured in M5, which read the diagnostics by
`freopen`-ing stderr to a file inside the sandbox container and removing the
hack before redeploying. That is not a gate; it is a one-off. TideApp.cpp now
mirrors these same diagnostics to os_log under the subsystem below, which is
the platform's own facility rather than a file the plugin creates -- nothing is
written to the user's disk and no sandbox exception is needed (PLAN constraints
3 and 4). This script subscribes to that subsystem.

    --standalone <binary>   run it, read its stderr        (all platforms)
    --au3                   run auval, read the unified log (macOS only)
    --log-file <path>       assert against already-captured text, for CI and
                            for testing this script

Exit 0 when the rack came up populated; 1 otherwise, naming what was missing.
"""
import argparse
import datetime
import pathlib
import re
import shutil
import subprocess
import sys
import threading
import time

# The four module-description XMLs TideApp::InitInstance merges. Keep in step
# with the list in TideApp.cpp -- a file added there and not here is simply not
# gated, which is silent, so the count is asserted too.
EXPECTED_XMLS = ("ControlsXp.xml", "MidiPlayer2.xml", "Converters.xml", "VaFilters.xml")

# What a healthy `main` seeds today. M5 measured exactly these nine in
# GarageBand: AR jef, Envelope, Filter, Midi, MidiCv, Oscillator, Output,
# Output jef, Sine jef. Overridable, because adding a prefab is normal work and
# this script should be updated deliberately rather than being a tripwire that
# every prefab author has to guess at.
# 7 since 2026-08-26: V6 made the root assembly a default DOCUMENT, which made
# MidiCv.synthedit redundant, and Output_jef.synthedit went with it as the
# other duplicate. Keeping MidiCv browsable would have been actively unsafe --
# a second root `SE MIDI to CV 2` breaks voice allocation.
EXPECTED_PREFABS = 7

# The AudioComponent this project registers -- SynthEditSem/CMakeLists.txt:162.
AU_TYPE, AU_SUBTYPE, AU_MANUFACTURER = "aumu", "Drck", "Dsyh"

# Must match kTideDiagSubsystem in SynthEditSem/TideApp.cpp.
DIAG_SUBSYSTEM = "com.tidesynth.tiderack"

ENRICHED = re.compile(r"TIDE: (\S+) enriched (\d+) of (\d+) described class\(es\)")
PREFABS = re.compile(r"TIDE: (\d+) rack prefab\(s\) seeded from the bundle")
# V6 replaced seedRootMidiCv() with a default DOCUMENT, so the old
# "root MIDI-CV seeded (...)" line no longer exists. Assert the OUTCOME the
# gate always meant -- the rack came up populated -- rather than which code
# path produced it. The byte count is part of the line on purpose: a rack
# that loaded nothing cannot print one.
DEFAULT_RACK = re.compile(r"TIDE: default rack loaded, (\d+) byte document")

# Lines that are themselves the verdict. Each is a real message in TideApp.cpp;
# a typo here would silently disarm one check, so the negative-control test in
# the PR exercises them.
FATAL_LINES = (
    ("missing from bundle resources", "a module-description XML did not resolve"),
    ("-- ZERO: is the .cpp in CMakeLists' source list?", "an XML enriched zero classes"),
    ("bundle resource folder did not resolve", "BundleInfo found no Resources folder "
     "-- this is the M5 defect's own shape, and until M8 it was completely silent"),
    ("no Prefabs folder in bundle resources", "the rack module browser will be empty"),
    ("Prefabs folder present but empty", "the POST_BUILD staging step did not run"),
    ("starting with an empty rack", "the default document is missing, unreadable or did not import"),
    ("refused", "a root MIDI-CV connection was refused"),
)

# The subset of FATAL_LINES that are the KNOWN reasons the "N rack prefab(s)
# seeded" line can be absent -- i.e. every early return in
# seedPrefabsFromBundle(), plus the found==0 path. When one of these has already
# fired, the absence below is explained and says so; when none has, the absence
# is unexplained, which is the M5 shape and is worth flagging as such. Both are
# still failures. Keep in step with the tideDiag calls in that function.
PREFAB_ABSENCE_CAUSES = (
    "bundle resource folder did not resolve",
    "no Prefabs folder in bundle resources",
    "Prefabs folder present but empty",
)


def check(text, expect_prefabs=EXPECTED_PREFABS):
    """Return (failures, notes) for a blob of captured diagnostic output."""
    failures, notes = [], []

    if not text.strip():
        return (["captured NOTHING. The plugin either never started, or its "
                 "diagnostics are not reaching this channel -- both are "
                 "failures, and an empty capture must never read as a pass."],
                notes)

    fired = set()
    for needle, why in FATAL_LINES:
        for line in text.splitlines():
            if needle in line and "TIDE:" in line:
                failures.append("%s -- %s\n      %s" % (needle, why, line.strip()))
                fired.add(needle)

    # --- positive assertion 1: every module-description XML enriched something
    seen = {}
    for name, enriched, described in ENRICHED.findall(text):
        seen[name] = (int(enriched), int(described))
    for name in EXPECTED_XMLS:
        if name not in seen:
            failures.append("no 'enriched' line for %s -- it was never merged, "
                            "and nothing said so" % name)
        elif seen[name][0] == 0:
            failures.append("%s enriched 0 of %d classes -- every class it "
                            "describes is missing from the source list"
                            % (name, seen[name][1]))
        else:
            notes.append("%s enriched %d of %d" % (name, seen[name][0], seen[name][1]))
    for name in sorted(set(seen) - set(EXPECTED_XMLS)):
        notes.append("%s enriched %d of %d (not in this script's expected list -- "
                     "add it)" % (name, seen[name][0], seen[name][1]))

    # --- positive assertion 2: the prefabs seeded, and the count is right
    found = PREFABS.findall(text)
    if not found:
        cause = [n for n in PREFAB_ABSENCE_CAUSES if n in fired]
        if cause:
            failures.append("no 'rack prefab(s) seeded' line -- the rack module "
                            "browser is EMPTY. Cause already named above: %s."
                            % "; ".join(cause))
        else:
            failures.append("no 'rack prefab(s) seeded' line at all, AND NOTHING "
                            "SAID WHY. seedPrefabsFromBundle() left by a path "
                            "that reports nothing, so the browser is empty and "
                            "the only evidence is this absence -- the M5 shape. "
                            "Since M8 every known way out of that function "
                            "prints a line, so this means either a new one, or "
                            "diagnostics that never reached this channel.")
    else:
        count = int(found[-1])
        if count != expect_prefabs:
            failures.append("%d rack prefab(s) seeded, expected %d. Either a "
                            "prefab failed to stage, or one was added and this "
                            "script's EXPECTED_PREFABS was not updated."
                            % (count, expect_prefabs))
        else:
            notes.append("%d rack prefab(s) seeded" % count)

    # --- positive assertion 3: the default rack loaded, so the rack has content
    m = DEFAULT_RACK.search(text)
    if not m:
        failures.append("no 'default rack loaded' line -- the rack came up EMPTY, "
                        "so it has no MIDI jacks and no output, which is what M5 "
                        "shipped and M6 exists to stop shipping again.")
    else:
        notes.append("default rack loaded, %s byte document" % m.group(1))

    return failures, notes


def capture_standalone(binary, timeout, expect_prefabs=EXPECTED_PREFABS):
    """Run the standalone, read its stderr, and stop the moment the evidence is
    complete.

    THE TIMEOUT IS A CEILING, NOT A DURATION, and the difference is the whole
    point of this function. The standalone is a GUI app: it never exits on its
    own, so the first version here called communicate(timeout=...) and always
    ran the FULL timeout before killing it -- 90 seconds by default. Everything
    this script asserts is printed within about two seconds of launch, so those
    remaining 88 seconds bought nothing and cost plenty: mac CI runs on a
    SELF-HOSTED runner on the developer's own desktop (build.yml's `guard` job
    routes there for every same-repo push), so each run parked a TIDE window on
    a real person's screen for a minute and a half. Reported as exactly that.

    So: poll the accumulated output and stop as soon as check() is satisfied.
    A healthy build now takes a couple of seconds.

    A FAILING build still waits out the ceiling, deliberately. The failure this
    whole script exists for is an ABSENT line (see the module docstring), and
    absence cannot be concluded early -- you can only wait long enough to be
    sure it is not coming. Slow on failure is the right trade when the
    alternative is a false pass.

    A reader thread rather than select(): stderr here is a pipe, and select on
    pipes does not work on Windows. The `--standalone` arm is the portable one,
    so it should not acquire a POSIX-only dependency.
    """
    binary = pathlib.Path(binary)
    if not binary.exists():
        sys.exit("no such binary: %s" % binary)

    proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                            stderr=subprocess.PIPE, text=True, errors="replace")

    lines = []
    def reader():
        # readline() returns "" only at EOF, i.e. when the process has gone.
        for line in iter(proc.stderr.readline, ""):
            lines.append(line)
    pump = threading.Thread(target=reader, daemon=True)
    pump.start()

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        # Snapshot before testing: the reader thread appends concurrently, and
        # list slicing is atomic under the GIL while "".join(lines) mid-append
        # is not guaranteed to be.
        failures, _ = check("".join(lines[:]), expect_prefabs)
        if not failures:
            break
        if proc.poll() is not None:      # it exited or crashed; no more output
            break
        time.sleep(0.1)

    if proc.poll() is None:
        proc.kill()
    proc.wait()
    pump.join(timeout=1.0)
    return "".join(lines)


def capture_au3(timeout):
    """Run auval, then read back what the extension logged.

    `log show --start` rather than a concurrent `log stream`: the stream would
    have to be started first and raced against auval, while --start is exact
    and re-runnable after the fact.
    """
    if sys.platform != "darwin":
        sys.exit("--au3 is macOS-only (the AUv3 does not exist elsewhere)")
    if not shutil.which("auval"):
        sys.exit("auval not found")

    start = datetime.datetime.now() - datetime.timedelta(seconds=2)

    au = subprocess.run(["auval", "-v", AU_TYPE, AU_SUBTYPE, AU_MANUFACTURER],
                        capture_output=True, text=True, errors="replace",
                        timeout=timeout)
    print("auval exit %d%s" % (au.returncode,
          "" if au.returncode == 0 else "  <-- auval itself failed"))

    # The unified log is written asynchronously; give it a moment to settle
    # before asking for the window we just created.
    subprocess.run(["sleep", "3"])

    log = subprocess.run(
        ["log", "show", "--predicate", 'subsystem == "%s"' % DIAG_SUBSYSTEM,
         "--start", start.strftime("%Y-%m-%d %H:%M:%S"), "--style", "compact"],
        capture_output=True, text=True, errors="replace")

    text = log.stdout
    if au.returncode != 0:
        text += "\n(auval exit %d)\n" % au.returncode
    return text, au.returncode


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--standalone", metavar="BINARY",
                     help="run this standalone binary and read its stderr")
    src.add_argument("--au3", action="store_true",
                     help="run auval against the installed AUv3 and read the unified log")
    src.add_argument("--log-file", metavar="PATH",
                     help="assert against text captured earlier")
    ap.add_argument("--expect-prefabs", type=int, default=EXPECTED_PREFABS,
                    help="prefab count to require (default %d)" % EXPECTED_PREFABS)
    ap.add_argument("--timeout", type=int, default=90,
                    help="seconds to let the subject run (default 90)")
    ap.add_argument("--show-capture", action="store_true",
                    help="print everything captured, not just the verdict")
    args = ap.parse_args()

    auval_rc = 0
    if args.standalone:
        subject = "standalone %s" % args.standalone
        text = capture_standalone(args.standalone, args.timeout, args.expect_prefabs)
    elif args.au3:
        subject = "AUv3 %s %s %s" % (AU_TYPE, AU_SUBTYPE, AU_MANUFACTURER)
        text, auval_rc = capture_au3(args.timeout)
    else:
        subject = "log file %s" % args.log_file
        text = pathlib.Path(args.log_file).read_text(errors="replace")

    if args.show_capture:
        print("---- captured ----")
        print(text)
        print("------------------")

    failures, notes = check(text, args.expect_prefabs)

    print("subject: %s" % subject)
    for n in notes:
        print("  ok   %s" % n)
    for f in failures:
        print("  FAIL %s" % f)

    if failures:
        print("\n%d assertion(s) failed -- the rack did NOT come up populated."
              % len(failures))
        print("This is the state that passed `auval` for two days in 2026-08.")
        return 1

    if auval_rc != 0:
        print("\ncontent assertions passed, but auval itself exited %d." % auval_rc)
        return 1

    print("\nrack is populated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
