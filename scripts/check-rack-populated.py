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
That version is blind to the failure it most needs to catch. `seedPrefabsFromBundle`
opens with:

    const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
    if (resourceFolder.empty())
        return;                       // <-- no message of any kind

An empty resource folder -- which is one plausible shape of exactly the M5
BundleInfo defect -- produces a rack with no prefabs and SAYS NOTHING. A
negative-line scan passes it. Requiring the positive line ("N rack prefab(s)
seeded from the bundle") fails it, because the line is simply absent. The rule
is: a silent plugin is a failing plugin. We check for both, but the positive
assertions are the load-bearing half.

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

# The four module-description XMLs TideApp::InitInstance merges. Keep in step
# with the list in TideApp.cpp -- a file added there and not here is simply not
# gated, which is silent, so the count is asserted too.
EXPECTED_XMLS = ("ControlsXp.xml", "MidiPlayer2.xml", "Converters.xml", "VaFilters.xml")

# What a healthy `main` seeds today. M5 measured exactly these nine in
# GarageBand: AR jef, Envelope, Filter, Midi, MidiCv, Oscillator, Output,
# Output jef, Sine jef. Overridable, because adding a prefab is normal work and
# this script should be updated deliberately rather than being a tripwire that
# every prefab author has to guess at.
EXPECTED_PREFABS = 9

# The AudioComponent this project registers -- SynthEditSem/CMakeLists.txt:162.
AU_TYPE, AU_SUBTYPE, AU_MANUFACTURER = "aumu", "Drck", "Dsyh"

# Must match kTideDiagSubsystem in SynthEditSem/TideApp.cpp.
DIAG_SUBSYSTEM = "com.tidesynth.tiderack"

ENRICHED = re.compile(r"TIDE: (\S+) enriched (\d+) of (\d+) described class\(es\)")
PREFABS = re.compile(r"TIDE: (\d+) rack prefab\(s\) seeded from the bundle")
MIDI_CV = re.compile(r"TIDE: root MIDI-CV seeded \(")

# Lines that are themselves the verdict. Each is a real message in TideApp.cpp;
# a typo here would silently disarm one check, so the negative-control test in
# the PR exercises them.
FATAL_LINES = (
    ("missing from bundle resources", "a module-description XML did not resolve"),
    ("-- ZERO: is the .cpp in CMakeLists' source list?", "an XML enriched zero classes"),
    ("no Prefabs folder in bundle resources", "the rack module browser will be empty"),
    ("Prefabs folder present but empty", "the POST_BUILD staging step did not run"),
    ("did not insert a container", "the rack will have no MIDI jacks"),
    ("could not create the root MIDI-CV", "the MIDI-CV module is not registered"),
    ("refused", "a root MIDI-CV connection was refused"),
)


def check(text, expect_prefabs=EXPECTED_PREFABS):
    """Return (failures, notes) for a blob of captured diagnostic output."""
    failures, notes = [], []

    if not text.strip():
        return (["captured NOTHING. The plugin either never started, or its "
                 "diagnostics are not reaching this channel -- both are "
                 "failures, and an empty capture must never read as a pass."],
                notes)

    for needle, why in FATAL_LINES:
        for line in text.splitlines():
            if needle in line and "TIDE:" in line:
                failures.append("%s -- %s\n      %s" % (needle, why, line.strip()))

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
        failures.append("no 'rack prefab(s) seeded' line at all. This is the "
                        "silent case: seedPrefabsFromBundle() returns without a "
                        "message when the resource folder is empty, so the "
                        "browser is empty and nothing complained.")
    else:
        count = int(found[-1])
        if count != expect_prefabs:
            failures.append("%d rack prefab(s) seeded, expected %d. Either a "
                            "prefab failed to stage, or one was added and this "
                            "script's EXPECTED_PREFABS was not updated."
                            % (count, expect_prefabs))
        else:
            notes.append("%d rack prefab(s) seeded" % count)

    # --- positive assertion 3: the root MIDI-CV is in and wired
    if not MIDI_CV.search(text):
        failures.append("no 'root MIDI-CV seeded' line -- the rack has no MIDI "
                        "jacks, which is one of the three things M5 shipped.")
    else:
        notes.append("root MIDI-CV seeded")

    return failures, notes


def capture_standalone(binary, timeout):
    """Run the standalone and read its stderr. It opens a window, so it is
    killed once it has said its piece rather than waited on."""
    binary = pathlib.Path(binary)
    if not binary.exists():
        sys.exit("no such binary: %s" % binary)

    proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                            stderr=subprocess.PIPE, text=True, errors="replace")
    try:
        _, err = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        _, err = proc.communicate()
    return err or ""


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
        text = capture_standalone(args.standalone, args.timeout)
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
