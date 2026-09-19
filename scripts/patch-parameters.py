#!/usr/bin/env python3
"""Read the per-module patch-parameter census of a TiDE document.

BACKLOG E80, 2026-09-16 (windows). Companion to patch-cables.py, and written
for the same reason: a TiDE document is base64 inside a preset, so a fact that
is one grep away in the decoded form is invisible in the committed file, and
three rows spent six runs measuring through a document nobody had read.

WHAT THIS ANSWERS, and it is one question: which modules in this document have
patch parameters, and which have NONE?

That matters because a rack module's parameters are not decoration. Every
DSP->GUI value a rack module produces -- its lights, and the display-state blob
that carries a custom display's picture -- is declared as a private,
non-persistent PARAMETER (SynthEdit_Rack_Adaptor/RackAdaptor.h), with a
direction="out" pin bound to it. If the document has no parameter for that
module, ug_patch_param_setter::ConnectParameter leaves the pin unconnected,
UPlug::Transmit finds an empty connection list, and every value the module
sends goes nowhere. Silently: the module still builds, still processes, still
captures its display state, and still reports success.

    tests/fixtures/e75-vcv-visible-rack.xml, measured 2026-09-16:
      VCV: LFO    12 parameters      VCV: Pulses  20
      VCV: LFO2   12                 VCV: SHASR    6
      VCV: Scope   0   <-- and E80 is the consequence

WHAT THIS CANNOT DO, said plainly, because the obvious stronger check does not
work here. A TiDE document stores its PatchManager TWICE -- once in <DSP> as
<Parameter Module=...> and once in <Editor> as <param module=...> -- which is
exactly the shape E83 found the patch CABLES disagreeing in. So the natural
sound check is "do the two copies agree", and this defect DEFEATS it: measured
on the same fixture, both halves agree, and both are missing the same eleven
parameters. --halves prints that comparison anyway, because a disagreement is
still worth catching; it is simply not what finds THIS.

So the zero-parameter flag below is a SCREEN, not a proof. A module may
legitimately have no parameters, and this script cannot know which from the
document alone. MEASURED on the repo's committed corpus, 2026-09-16: the screen
raises 6 flags on e75 of which 1 is the defect -- `IO Mod`, `VCA` and
`SE MIDI to CV 2` are parameterless by design. The allowlist is deliberately
NOT extended to cover them, because a long allowlist is how a screen stops
screening; the false-alarm rate is stated instead.

TWO THINGS DO PROVE IT, and both are cheap.

  --compare <other>   Diff this document's per-module counts against another's.
                      A module that goes 0 -> N is provably missing parameters
                      the product itself writes, with no allowlist and no
                      judgement anywhere in it. Get the other document by
                      loading this one and saving it back:

                          e80vst3probe.exe <TIDE-Rack.vst3> <fixture> \
                              --blocks 1 --save roundtrip.xml
                          patch-parameters.py <fixture> --compare roundtrip.xml

                      That is how E80 was settled: e75's Scope went 0 -> 11.

  The plug-in's own diagnostic, which already exists and fires at graph build
  (SynthEditLib/ug_patch_param_setter.cpp, TIDE E49):

      SynthEdit: no patch parameter for module <handle> parameter id <n>
                 -- pin left unconnected rather than dereferenced.

  Load the document in any bare host and grep stderr for that line. Nine of
  them is how E80 was finally found.

Usage:
    patch-parameters.py <fixture-or-document> [--halves] [--compare <other>]
    patch-parameters.py --survey <path> [<path> ...]

Exit: 0 clean, 1 a module carries no parameters (or lost some under --compare),
2 the file could not be read as a TiDE document.
"""

import argparse
import base64
import collections
import glob
import re
import sys
import xml.etree.ElementTree as ET

# Types that legitimately carry no parameters of their own. Containers hold
# other modules; the IO modules are plumbing. Kept deliberately short -- a long
# allowlist is how a screen stops screening.
NO_PARAMETERS_EXPECTED = {
    "Container",
    "TiDE Patch Point In",
    "TiDE Patch Point Out",
    "Patch Point In",
    "Patch Point Out",
}


def documents(path):
    """Yield (label, decoded-document-text) for a fixture or a bare document."""
    raw = open(path, "rb").read()
    text = raw.decode("utf-8", errors="replace")

    if "<PatchManager" in text and "<Param " not in text.split("\n", 3)[-1][:400]:
        # Already a decoded document.
        if "<PatchManager" in text:
            yield ("document", text)
            return

    try:
        root = ET.fromstring(text)
    except ET.ParseError:
        if "<PatchManager" in text:
            yield ("document", text)
        return

    for el in root.findall("Param"):
        val = el.get("val") or ""
        if len(val) < 200:
            continue
        try:
            blob = base64.b64decode(val)
        except Exception:
            continue
        # A saved chunk carries a 4-byte ChunkPrefix before the XML.
        start = blob.find(b"<?xml")
        if start < 0:
            continue
        body = blob[start:].decode("utf-8", errors="replace")
        if "<PatchManager" in body:
            yield ("Param id=%s" % el.get("id"), body)


def split_halves(doc):
    cut = doc.find("<Editor")
    return (doc, "") if cut < 0 else (doc[:cut], doc[cut:])


def count_params(block, dsp_schema):
    """Parameters per module handle in one half's <PatchManager>.

    The two halves use different spellings of the same fact -- <Parameter
    Module="..."> in <DSP>, <param module="..."> in <Editor> -- so the schema
    is a parameter rather than something to guess.
    """
    pm = re.search(r"<PatchManager.*?</PatchManager>", block, re.S)
    counts = collections.Counter()
    if not pm:
        return counts
    tag = r"<Parameter\b([^>]*)>" if dsp_schema else r"<param\b([^>]*?)/?>"
    attr = r'\bModule="(\d+)"' if dsp_schema else r'\bmodule="(\d+)"'
    for m in re.finditer(tag, pm.group(0)):
        handle = re.search(attr, m.group(1))
        counts[handle.group(1) if handle else "(no module attr)"] += 1
    return counts


def modules_of(dsp_half):
    return {
        m.group(1): m.group(2)
        for m in re.finditer(r'<Module\s+Id="(\d+)"\s+Type="([^"]+)"', dsp_half)
    }


def counts_by_module(path):
    """Per-module DSP parameter counts across every document in a file."""
    out = collections.Counter()
    types = {}
    for _, doc in documents(path):
        dsp, _ = split_halves(doc)
        types.update(modules_of(dsp))
        out.update(count_params(dsp, True))
    return out, types


def compare(path, other):
    """Which modules carry FEWER parameters here than in `other`?

    No allowlist and no heuristic: `other` is the same rack as the product
    itself writes it, so a shortfall is a fact about this document rather than
    a guess about what a module ought to declare.
    """
    mine, types = counts_by_module(path)
    theirs, other_types = counts_by_module(other)
    types.update({h: t for h, t in other_types.items() if h not in types})

    print("%s  vs  %s" % (path, other))
    worst = 0
    for handle in sorted(set(mine) | set(theirs), key=lambda h: (types.get(h, "~"), h)):
        a, b = mine.get(handle, 0), theirs.get(handle, 0)
        if a == b:
            continue
        note = "   <-- MISSING %d" % (b - a) if a < b else "   (extra %d here)" % (a - b)
        print("    %-24s %-12s %d -> %d%s" % (types.get(handle, "?"), handle, a, b, note))
        if a < b:
            worst = 1
    if worst == 0:
        print("    every module carries at least as many parameters as in %s" % other)
    return worst


def report(path, show_halves):
    found_any = False
    worst = 0

    for label, doc in documents(path):
        found_any = True
        dsp, editor = split_halves(doc)
        mods = modules_of(dsp)
        dsp_counts = count_params(dsp, True)
        editor_counts = count_params(editor, False)

        print("%s [%s]  %d module(s), %d DSP parameter(s)"
              % (path, label, len(mods), sum(dsp_counts.values())))

        for handle, mtype in sorted(mods.items(), key=lambda kv: (kv[1], kv[0])):
            n = dsp_counts.get(handle, 0)
            suspect = n == 0 and mtype not in NO_PARAMETERS_EXPECTED
            flag = "   <-- NO PARAMETERS" if suspect else ""
            if suspect or show_halves or n == 0:
                extra = ""
                if show_halves:
                    e = editor_counts.get(handle, 0)
                    extra = "  editor=%-4d%s" % (e, "  <-- HALVES DISAGREE" if e != n else "")
                print("    %-24s %-12s dsp=%-4d%s%s" % (mtype, handle, n, extra, flag))
            if suspect:
                worst = 1

        if show_halves:
            d, e = sum(dsp_counts.values()), sum(editor_counts.values())
            print("    totals: dsp=%d editor=%d%s"
                  % (d, e, "" if d == e else "   <-- HALVES DISAGREE"))

    if not found_any:
        print("%s: no TiDE document with a <PatchManager> found" % path, file=sys.stderr)
        return 2
    return worst


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--halves", action="store_true",
                    help="also print the <Editor> half's copy and whether it agrees")
    ap.add_argument("--survey", action="store_true",
                    help="accept several paths and keep going past an unreadable one")
    ap.add_argument("--compare", metavar="OTHER",
                    help="diff per-module parameter counts against OTHER (the proof, not the screen)")
    args = ap.parse_args()

    if args.compare:
        if len(args.paths) != 1:
            ap.error("--compare takes exactly one input document")
        return compare(args.paths[0], args.compare)

    paths = []
    for p in args.paths:
        expanded = sorted(glob.glob(p)) if any(c in p for c in "*?[") else [p]
        paths.extend(expanded or [p])

    worst = 0
    for p in paths:
        try:
            rc = report(p, args.halves)
        except Exception as exc:                       # noqa: BLE001 - a survey keeps going
            if not args.survey:
                raise
            print("%s: %s" % (p, exc), file=sys.stderr)
            rc = 2
        worst = max(worst, rc)
    return worst


if __name__ == "__main__":
    sys.exit(main())
