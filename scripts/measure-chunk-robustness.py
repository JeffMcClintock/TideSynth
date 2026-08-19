#!/usr/bin/env python3
"""Render TIDE in REAPER with deliberately BAD saved chunks, and report what happens.

    python3 scripts/measure-chunk-robustness.py [--keep]

TIDE's whole patch is one blob parameter, restored by the host from the project
file. Nothing between the host's bytes and SeAudioMaster::BuildDspGraph
validated its shape, and BuildDspGraph trusts it completely -- so a corrupt,
truncated or foreign chunk was a host CRASH, not a refusal (E10, measured
2026-08-19: `<Patch/>` segfaulted a REAPER render, EXC_BAD_ACCESS at 0x28, in
TiXmlNode::FirstChildElement <- BuildDspGraph <- prepareToPlay <- onSetPins <-
Processor_VST3::process, on a REAPER worker thread).

This is the harness that measured it, kept so the fix stays fixed. Every case
must now print a `TIDE: REFUSED ...` line and render without dying. A crash
shows up as `rc=-11`.

Why the projects are synthesised rather than committed: REAPER stores the chunk
inside a base64 VST3 state block whose length fields must agree with the payload
(header[8] = len(body), body = u32(len(xml)+4), u32(1), u32(len(xml)), xml,
8 zero bytes), so a fixture cannot be hand-edited to carry a different chunk --
it has to be built. The audio item is a generated tone, so nothing binary lands
in git.

The positive control is the REAL chunk lifted out of tests/hosts/v3-midi-pitch.rpp,
because a hand-written skeleton is not good enough: `<Document><DSP><Module/>`
satisfies everything TIDE can check and STILL crashes the engine (a <Module> with
no <PatchManager> reaches ug_container.cpp:1469 unguarded). That case is included
and reported as a KNOWN LIMIT rather than a failure -- it is what E10 has to fix
in SynthEditLib, and it is the honest boundary of the TIDE-side guard: TIDE can
refuse chunks that are the wrong SHAPE, it cannot validate the engine's whole
schema.

Writes only to a temp dir. Needs the TIDE VST3 installed where REAPER can see it
(the build's post-build step does that).
"""
import argparse
import base64
import math
import os
import re
import struct
import subprocess
import sys
import tempfile
import textwrap
import wave

REAPER = "/Applications/REAPER.app/Contents/MacOS/REAPER"
SECONDS = 2.0
RATE = 48000

# The VST3 state block REAPER writes for TIDE. Only header[8] varies with the
# payload; the rest is copied from a real saved project.
HDR = (1386065673, 0xFEED5EEE, 0, 2, 1, 0, 2, 0)
HDR_TAIL = (1, 0x10FFFF)
BLOCK_TAIL = bytes.fromhex("000010000000")

FIXTURE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "tests", "hosts", "v3-midi-pitch.rpp")


def real_chunk():
    """The document bytes out of a known-good saved project."""
    src = open(FIXTURE, errors="replace").read().splitlines()
    i = [n for n, l in enumerate(src) if l.strip().startswith("<VST ")][0]
    raw, n = [], i + 1
    while src[n].strip() != ">":
        raw.append(src[n].strip())
        n += 1
    body = base64.b64decode("".join(raw[1:-1]))
    xml = body[12:].decode("utf-8", "replace")
    val = re.search(r'<Param id="1" val="([^"]*)"', xml).group(1)
    return base64.b64decode(val)


# (name, chunk, why, expectation) where expectation is one of:
#   "render"      must load and play -- refusing it would be a false positive
#   "refuse"      must be refused, and must not crash
#   "quiet"       nothing to refuse, nothing to build
#   "known-limit" passes TIDE's guard and still crashes: E10's job, reported not failed
CASES = [
    ("valid",     None,
     "the real chunk from tests/hosts/v3-midi-pitch.rpp -- the positive control",
     "render"),
    ("skeleton",  b'<?xml version="1.0" ?>\n<Document>\n  <DSP>\n'
                  b'    <Module Id="1" Type="Container"/>\n  </DSP>\n</Document>\n',
     "<Module> with no <PatchManager>: passes TIDE's guard, dies in the engine",
     "known-limit"),
    ("empty",     b"", "no chunk at all: the instance never prepares", "quiet"),
    ("notxml",    b"this is not xml at all", "bytes that do not parse", "refuse"),
    ("badroot",   b"<Patch/>", "parses cleanly, but the root is not <Document> (crashed pre-fix)",
     "refuse"),
    ("nodsp",     b"<Document/>", "<Document> with no <DSP> (crashed one frame up, in Open())",
     "refuse"),
    ("emptydsp",  b"<Document><DSP/></Document>", "<DSP> with no <Module> (crashed at :420)",
     "refuse"),
]


def state_lines(chunk):
    val = base64.b64encode(chunk).decode() if chunk else ""
    xml = ('<?xml version="1.0" encoding="UTF-8"?>\n<Preset>\n'
           '    <Param id="1" val="%s"/>\n</Preset>\n' % val).encode()
    body = struct.pack("<3I", len(xml) + 4, 1, len(xml)) + xml + b"\x00" * 8
    header = struct.pack("<11I", *HDR, len(body), *HDR_TAIL)
    return ([base64.b64encode(header).decode()]
            + textwrap.wrap(base64.b64encode(body).decode(), 128)
            + [base64.b64encode(BLOCK_TAIL).decode()])


def write_tone(path):
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(
            struct.pack("<h", int(16384 * math.sin(2 * math.pi * 440 * t / RATE)))
            for t in range(int(RATE * SECONDS))))


def write_project(path, out_wav, tone, chunk):
    state = "\n".join("        " + l for l in state_lines(chunk))
    open(path, "w").write(
        '<REAPER_PROJECT 0.1 "7.45/macOS-arm64" 1\n'
        '  SAMPLERATE %d 0 0\n'
        '  RENDER_FILE "%s"\n  RENDER_PATTERN ""\n  RENDER_FMT 0 2 0\n'
        '  RENDER_RANGE 0 0 %g 0 1000\n'
        '  <TRACK\n    NAME chunk-robustness\n    NCHAN 2\n    FX 1\n'
        '    <FXCHAIN\n      SHOW 0\n      LASTSEL 0\n      DOCKED 0\n      BYPASS 0 0 0\n'
        '      <VST "VST3i: TIDE Rack (TIDE Synth)" TIDE_VST3.vst3 0 "" '
        '1386065673{506C7567696E474D504920501951ED43} ""\n%s\n      >\n'
        '      FLOATPOS 0 0 0 0\n      WAK 0 0\n    >\n'
        # An audio item so the render has something to write even when the rack
        # is silent, and a note so the processor is exercised with real events.
        '    <ITEM\n      POSITION 0\n      LENGTH %g\n      NAME tone440\n'
        '      <SOURCE WAVE\n        FILE "%s"\n      >\n    >\n'
        '    <ITEM\n      POSITION 0\n      LENGTH 1\n      NAME midi-c\n'
        '      <SOURCE MIDI\n        HASDATA 1 960 QN\n        E 0 90 3c 60\n'
        '        E 960 80 3c 00\n        E 0 b0 7b 00\n        >\n      >\n'
        '  >\n>\n' % (RATE, out_wav, SECONDS, state, SECONDS, tone))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--keep", action="store_true",
                    help="keep the generated projects, renders and logs")
    args = ap.parse_args()

    if not os.path.exists(REAPER):
        raise SystemExit("REAPER not found at %s" % REAPER)

    workdir = tempfile.mkdtemp(prefix="tide-chunk-")
    tone = os.path.join(workdir, "tone440.wav")
    write_tone(tone)

    failures = []
    limits = []
    for name, chunk, why, expect in CASES:
        if chunk is None:
            chunk = real_chunk()
        rpp = os.path.join(workdir, name + ".rpp")
        out = os.path.join(workdir, name + ".wav")
        log = os.path.join(workdir, name + ".log")
        write_project(rpp, out, tone, chunk)
        with open(log, "w") as fh:
            rc = subprocess.call([REAPER, "-renderproject", rpp],
                                 stdout=fh, stderr=subprocess.STDOUT)
        tide = [l.rstrip() for l in open(log, errors="replace") if l.startswith("TIDE:")]
        refused = any("REFUSED" in l for l in tide)
        crashed = rc < 0 or not os.path.exists(out)

        print("  %-9s %s" % (name, why))
        print("      rc=%-4d %s%s" % (
            rc,
            "CRASHED" if crashed else "rendered",
            "" if not crashed else "  <-- signal %d" % -rc if rc < 0 else ""))
        for l in dict.fromkeys(tide):        # de-duplicated: two instances log
            print("      " + l)

        if expect == "known-limit":
            limits.append("%s: %s (%s)" % (
                name, "still crashes, as expected until E10" if crashed
                      else "NO LONGER CRASHES -- has E10 landed?",
                "rc=%d" % rc))
        elif crashed:
            failures.append("%s crashed the host (rc=%d)" % (name, rc))
        elif expect == "render" and refused:
            failures.append("the valid document was REFUSED -- the guard is too strict")
        elif expect == "refuse" and not refused:
            failures.append("%s was accepted -- the guard did not catch it" % name)

    print()
    for l in limits:
        print("KNOWN LIMIT (E10, GATED engine fix): " + l)
    if limits:
        print()
    if failures:
        print("FAIL")
        for f in failures:
            print("  " + f)
    else:
        print("PASS -- every malformed chunk was refused, the real document was not,")
        print("        and no case TIDE can guard took the host down.")
    if args.keep:
        print("\n  artefacts kept in %s" % workdir)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
