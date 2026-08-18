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
in the first place. Every saved TIDE project as of 2026-08-18 has an EMPTY
<Lines> element in its DSP graph -- no connections -- so it renders silence
whatever the plugin does. Run --control first: it proves the render-and-measure
chain detects audio, so a subsequent -inf is a fact about the patch rather than
about this script.

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
RENDER_SECONDS = 2.0


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
        rc = subprocess.call([REAPER, "-renderproject", staged],
                             stdout=fh, stderr=subprocess.STDOUT)
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


def lines_in_chunk(rpp):
    """How many DSP connections the embedded TIDE document has, if we can tell.

    Three layers, each of which cost a wrong guess to find:
      1. The <VST block's body is base64, one stream split over many lines --
         but the FIRST line is REAPER's own header block, separately padded, so
         concatenating everything and decoding truncates at its '='.
      2. The decoded state is the preset XML, whose blob attribute is `val=`,
         not `value=`.
      3. That attribute is base64 again, and yields the <Document>.
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
        dsp = ET.fromstring(base64.b64decode(m.group(1))).find("DSP")
        if dsp is None:
            return None
        return sum(len(l) for mod in dsp.iter("Module") for l in mod.findall("Lines"))
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
        if silent:
            n = lines_in_chunk(args.project)
            if n == 0:
                print("    NOTE: this project's DSP graph has ZERO connections")
                print("    (<Lines> is empty), so silence is expected and says")
                print("    nothing about the plugin. Not a usable audio test.")
            elif n is None:
                print("    NOTE: could not read the embedded TIDE document, so it")
                print("    is unknown whether this project could make sound at all.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
