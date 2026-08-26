#!/usr/bin/env python3
"""Render a REAPER project headlessly and report whether it made any sound.

Built 2026-08-18 for the audio half of PLAN's V1 acceptance -- "the patch plays
after reload" -- which had never been measured because everyone assumed it
needed someone sitting in front of REAPER. It does not:

    /Applications/REAPER.app/Contents/MacOS/REAPER -renderproject file.rpp

renders and exits, so an unattended run can measure audio. The same realisation
as S13's repro: needing a GUI *application* is not the same as needing to drive
a GUI.

What this does NOT tell you
---------------------------
Silence is only evidence about the plugin if the project could have made sound
in the first place, and the count that answers that is NOT the one you would
guess.

A TIDE rack's wiring is not in the document's <Line> elements. A patch cable is
stored in the patch manager as the HC_PATCH_CABLES host control (= 49,
SynthEditLib/HostControls.h), a serialised <Cables> list written by
MfcDocPresenter::AddPatchCable, and turned into real DSP connections at load
time by ug_container::ConnectPatchCables (SynthEditLib/ug_container.cpp:433).
The <Line> elements that ARE in the document belong to the insides of prefab
containers, so a rack of three prefabs reports EIGHT of them while being
completely unwired. This script reports both counts and says which one it
judged on.

    Corrected 2026-08-18. The first version of this note said a non-zero
    <Lines> count meant the project could sound, which was true only while
    saved projects held bare modules. E2a's prefabs carry internal cables and
    broke that inference.

Run --control first: it proves the render-and-measure chain detects audio, so a
subsequent -inf is a fact about the patch rather than about this script.

    python3 scripts/render-and-measure.py --control
    python3 scripts/render-and-measure.py path/to/project.rpp
"""
import argparse
import math
import os
import re
import struct
import subprocess
import sys
import tempfile
import wave

REAPER = "/Applications/REAPER.app/Contents/MacOS/REAPER"

# See the timeout's own comment in render() for why this exists at all.
RENDER_TIMEOUT_SECONDS = 300
RENDER_TIMED_OUT = -9999   # distinct from any REAPER exit code

RENDER_SECONDS = 2.0

# HC_PATCH_CABLES, counted off the enum in SynthEditLib/HostControls.h:14. This
# is the patch-manager parameter a rack's patch cables live in, and the only
# thing in a saved project that says whether the rack is wired.
HC_PATCH_CABLES = 49


def analyse(path):
    """(peak_dbfs, rms_dbfs, silent) for a wav file."""
    with wave.open(path) as w:
        frames, chans, sw = w.getnframes(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(frames)
    n = frames * chans
    if n == 0:
        return float("-inf"), float("-inf"), True
    peak, sq = 0, 0.0
    if sw == 3:  # REAPER's default render is 24-bit
        full = 1 << 23
        for i in range(0, n * 3, 3):
            v = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if v & (1 << 23):
                v -= 1 << 24
            peak = max(peak, abs(v))
            sq += v * v
    elif sw == 2:
        full = 1 << 15
        for v in struct.unpack("<%dh" % n, raw):
            peak = max(peak, abs(v))
            sq += v * v
    else:
        raise SystemExit("unhandled sample width: %d bytes" % sw)
    rms = math.sqrt(sq / n)
    db = lambda x: 20 * math.log10(x / full) if x else float("-inf")
    return db(peak), db(rms), peak == 0


def render(rpp_in, wav_out, workdir):
    """Copy the project with deterministic render settings, render, return the wav."""
    src = open(rpp_in, encoding="utf-8", errors="replace").read()
    src = re.sub(r'RENDER_FILE "[^"]*"', 'RENDER_FILE "%s"' % wav_out, src)
    if "RENDER_FILE" not in src:
        src = src.replace("<REAPER_PROJECT", "<REAPER_PROJECT", 1)
    src = re.sub(r"RENDER_RANGE [^\n]*",
                 "RENDER_RANGE 0 0 %g 0 1000" % RENDER_SECONDS, src)
    staged = os.path.join(workdir, "render.rpp")
    open(staged, "w", encoding="utf-8").write(src)

    log = os.path.join(workdir, "reaper.log")
    with open(log, "w") as fh:
        # A TIMEOUT, and it is not belt-and-braces -- it is the difference
        # between a failing test and a hung one.
        #
        # `-renderproject` is headless only while REAPER has nothing to ask.
        # If the project names a plug-in REAPER cannot resolve it raises a
        # MODAL "Project Load Warning ... are not available" and waits for a
        # human who is not there. Measured 2026-08-26 (BACKLOG E29): a fixture
        # carrying the wrong VST3 UID token blocked for over seven minutes with
        # no output and no error, and had to be killed by hand.
        #
        # A healthy render of these fixtures takes about four seconds, so five
        # minutes is far beyond any real render and still bounded. On timeout
        # the child is killed and rc is a sentinel the caller reports, rather
        # than an exception thrown from inside a `with`.
        try:
            rc = subprocess.call([REAPER, "-renderproject", staged],
                                 stdout=fh, stderr=subprocess.STDOUT,
                                 timeout=RENDER_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            fh.write("\n*** REAPER did not exit within %d s -- killed.\n"
                     "*** It is almost certainly waiting on a modal dialog; the usual\n"
                     "*** cause is a plug-in the project names that REAPER cannot\n"
                     "*** resolve (see BACKLOG E29 and tests/hosts/README.md).\n"
                     % RENDER_TIMEOUT_SECONDS)
            subprocess.call(["pkill", "-f", "REAPER -renderproject"])
            rc = RENDER_TIMED_OUT
    return rc, log


def control(workdir):
    """A known -6 dBFS 1 kHz sine, rendered through REAPER, to prove the chain."""
    tone = os.path.join(workdir, "tone.wav")
    with wave.open(tone, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(44100)
        w.writeframes(b"".join(
            struct.pack("<h", int(16384 * math.sin(2 * math.pi * 1000 * t / 44100)))
            for t in range(int(44100 * RENDER_SECONDS))))
    out = os.path.join(workdir, "control-out.wav")
    rpp = os.path.join(workdir, "control.rpp")
    open(rpp, "w").write(
        '<REAPER_PROJECT 0.1 "7.0" 1\n'
        "  SAMPLERATE 44100 0 0\n"
        '  RENDER_FILE "%s"\n'
        '  RENDER_PATTERN ""\n'
        "  RENDER_FMT 0 2 0\n"
        "  RENDER_RANGE 0 0 %g 0 1000\n"
        "  <TRACK\n    NAME control\n    <ITEM\n"
        "      POSITION 0\n      LENGTH %g\n"
        '      <SOURCE WAVE\n        FILE "%s"\n      >\n    >\n  >\n>\n'
        % (out, RENDER_SECONDS, RENDER_SECONDS, tone))
    rc, _ = render(rpp, out, workdir)
    if rc != 0 or not os.path.exists(out):
        print("CONTROL FAILED: REAPER rc=%d, no output" % rc)
        return 1
    pdb, rdb, silent = analyse(out)
    print("  control (known -6 dBFS 1 kHz sine)")
    print("    peak=%7.1f dBFS  rms=%7.1f dBFS  -> %s"
          % (pdb, rdb, "SILENCE" if silent else "AUDIO PRESENT"))
    ok = (not silent) and abs(pdb - (-6.0)) < 0.5
    print("  %s -- the render-and-measure chain %s detect audio"
          % ("PASS" if ok else "FAIL", "does" if ok else "does NOT"))
    return 0 if ok else 1


def graph_summary(rpp):
    """(rack_cables, container_lines) for the embedded TIDE document, or None.

    `rack_cables` is what decides whether the project could sound; see the
    module docstring for why `container_lines` does not.

    FOUR layers of encoding, each of which cost a wrong guess to find:
      1. The <VST block's body is base64, one stream split over many lines --
         but the FIRST line is REAPER's own header block, separately padded, so
         concatenating everything and decoding truncates at its '='.
      2. The decoded state is the preset XML, whose blob attribute is `val=`,
         not `value=`.
      3. That attribute is base64 again, and yields the <Document>.
      4. The cable list is base64 a THIRD time, inside the <patch-list><s> of
         the param whose hostControl is HC_PATCH_CABLES.

    Scoping the cable count to that hostControl matters: a saved document also
    carries a copy of the same <patch-list> in its editor half, under a param
    with no hostControl attribute, and counting both doubles every cable.
    """
    try:
        import base64
        import xml.etree.ElementTree as ET
        lines = open(rpp, encoding="utf-8", errors="replace").read().split("\n")
        start = next(i for i, l in enumerate(lines) if l.strip().startswith("<VST"))
        blk = []
        for l in lines[start + 1:]:
            s = l.strip()
            if s == ">":
                break
            if re.fullmatch(r"[A-Za-z0-9+/=]+", s):
                blk.append(s)
        if len(blk) < 2:
            return None
        stream = "".join(blk[1:])
        raw = base64.b64decode(stream + "=" * (-len(stream) % 4), validate=False)
        m = re.search(rb'<Param\s+id="1"\s+val="([^"]+)"', raw)
        if not m:
            return None
        doc = base64.b64decode(m.group(1))

        cables = 0
        for pm in re.finditer(
                rb'<param[^>]*hostControl="%d"[^>]*>\s*<patch-list>(.*?)</patch-list>'
                % HC_PATCH_CABLES, doc, re.S):
            s = re.search(rb"<s>([^<]+)</s>", pm.group(1))
            if not s:
                continue
            b64 = b"".join(s.group(1).split())
            cables += base64.b64decode(
                b64 + b"=" * (-len(b64) % 4), validate=False).count(b"<Cable ")

        dsp = ET.fromstring(doc).find("DSP")
        if dsp is None:
            return None
        internal = sum(len(l) for mod in dsp.iter("Module")
                       for l in mod.findall("Lines"))
        return cables, internal
    except Exception:
        return None


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("project", nargs="?", help="a .rpp to render and measure")
    ap.add_argument("--control", action="store_true",
                    help="render a known tone instead, to prove the chain works")
    args = ap.parse_args()

    if not os.path.exists(REAPER):
        raise SystemExit("REAPER not found at %s" % REAPER)

    with tempfile.TemporaryDirectory() as workdir:
        if args.control:
            return control(workdir)
        if not args.project:
            ap.error("give a project, or --control")

        out = os.path.join(workdir, "out.wav")
        rc, log = render(args.project, out, workdir)
        if not os.path.exists(out):
            print("no audio file produced (REAPER rc=%d); log tail:" % rc)
            print("".join(open(log).readlines()[-10:]))
            return 1
        pdb, rdb, silent = analyse(out)
        print("  %s" % os.path.basename(args.project))
        print("    peak=%7.1f dBFS  rms=%7.1f dBFS  -> %s"
              % (pdb, rdb, "SILENCE" if silent else "AUDIO PRESENT"))
        summary = graph_summary(args.project)
        if summary is None:
            print("    NOTE: could not read the embedded TIDE document, so it is")
            print("    unknown whether this project could make sound at all.")
            return 0

        cables, internal = summary
        print("    rack: %d patch cable(s) (HC_PATCH_CABLES) joining modules;"
              % cables)
        print("          %d <Line>(s) inside prefab containers" % internal)
        if silent and cables == 0:
            print("    NOTE: NOTHING joins one rack module to another, so silence is")
            print("    expected and says nothing about the plugin. Not a usable audio")
            print("    test. The <Line>s counted above are INSIDE containers and are")
            print("    NOT evidence the rack is wired -- only the cable count is.")
        elif silent:
            print("    NOTE: the rack IS wired, so this silence is a real finding about")
            print("    the plugin rather than about the project. The cables survived the")
            print("    save, so look at ug_container::ConnectPatchCables reconstructing")
            print("    them, not at the prefabs.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
