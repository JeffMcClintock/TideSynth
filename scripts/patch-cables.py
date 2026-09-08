#!/usr/bin/env python3
"""Read -- and reconcile -- the rack patch cables of a TiDE preset fixture.

BACKLOG E83. Exists because a TiDE document stores its rack cabling TWICE and
nothing checks that the two copies agree. `HC_PATCH_CABLES` (host control 49)
appears once in the `<DSP>` half, as `<Parameter HostControl="49">`, and once in
the `<Editor>` half, as `<param hostControl="49">`. Each holds a base64'd
`<Cables>` list. **The editor draws from its copy and the audio graph is wired
from the DSP's**, so a document whose two lists differ shows the user a cable
that carries nothing -- which is exactly what `e53-vcv-rack-segv.xml` and its
descendant `e75-vcv-visible-rack.xml` do, and what made E83 look like a broken
Scope capture for a week.

  ./scripts/patch-cables.py tests/fixtures/e75-vcv-visible-rack.xml --show
  ./scripts/patch-cables.py in.xml --sync-dsp -o out.xml

WHY IT IS NOT READABLE WITHOUT THIS. A fixture is a `<Preset>` whose
`<Param id="1">` holds the whole document base64'd, and the cable lists are
base64'd a SECOND time inside that. So the field that decides whether a cable
exists in the audio graph is two decodes below the committed bytes: it cannot be
read, diffed or reviewed in the file, and any re-encode rewrites the whole blob.
The derivation therefore has to be a command someone can re-run, which is the
same reason `scripts/set-view-center.py` exists for the view centre.

WHAT `--show` PRINTS, and the only line that matters is the last one: the two
lists side by side with module types resolved from their handles, then whether
they agree. A disagreement is reported as the editor-only and DSP-only cables,
because those are what a reader has to act on.

WHAT `--sync-dsp` DOES. It replaces the DSP list with the editor list, verbatim,
and rewrites the fixture. That direction and no other: the editor list is the
one the user built and the one the panel draws, so it is the intent -- the DSP
list is what failed to keep up. **This does not fix whatever produced the
disagreement**, which is a question about TiDE's save path and is filed
separately; it makes a FIXTURE consistent so that a measurement through it means
something.

Exit codes: 0 when the two lists agree (or after a successful write), 1 when
they disagree under `--show`, 2 when the document cannot be read or repaired.
**A half with no `HC_PATCH_CABLES` parameter has no cables**, which is the normal
state of `DefaultRack.synthedit` and every prefab -- so that is a 0, not an
error. `--show` is therefore usable as a check over every committed fixture.
"""

import argparse
import base64
import re
import sys
import xml.etree.ElementTree as ET

HC_PATCH_CABLES = 49  # SynthEditLib/HostControls.h -- counted, not guessed.

# The write path edits the decoded document as BYTES, the way
# scripts/set-view-center.py does, rather than round-tripping it through
# ElementTree. Re-serialising would rewrite every tag in a 38 KB document to
# change one base64 string, and the decoded-document diff is the only form in
# which this edit can be reviewed at all -- so it has to stay one line.
DSP_CABLES_RE = re.compile(
    rb'(<Parameter[^>]*HostControl="49"[^>]*>.*?<s>)([^<]*)(</s>)', re.DOTALL)

# The DSP half and the Editor half spell every one of these differently.
HALVES = (
    ("DSP", "Parameter", "HostControl"),
    ("Editor", "param", "hostControl"),
)


def load_document(path):
    """Both shapes a TiDE document is stored in, as set-view-center.py takes.

    A `<Preset>` fixture holds the whole document base64'd in `<Param id="1">`;
    a `.synthedit` file (`DefaultRack.synthedit`, and every fixture saved by the
    editor) IS the document. The second form matters here because it is the only
    source of a KNOWN-GOOD control: a rack whose two cable lists agree.

    Returns (preset, param, header, doc, tree). `preset`/`param` are None for the
    bare form; `header` is the legacy chunk prefix, empty for most fixtures.
    """
    raw = open(path, "rb").read()
    if b"<Preset" in raw[:400]:
        preset = ET.parse(path)
        for param in preset.getroot().iter("Param"):
            if param.get("id") == "1":
                header, doc = split_chunk_header(base64.b64decode(param.get("val")))
                return preset, param, header, doc, ET.fromstring(doc)
        raise SystemExit(f"{path}: a <Preset> with no <Param id=\"1\">")
    header, doc = split_chunk_header(raw)
    return None, None, header, doc, ET.fromstring(doc)


def split_chunk_header(blob):
    """Separate a legacy chunk header from the document it prefixes.

    A fixture written by hand or by `set-view-center.py` is the document alone;
    one written by a plug-in's `state->save` carries a short binary header in
    front of it (TiDE logs these as "Legacy chunk"). Splitting on the first `<`
    reads both, and keeping the header lets --sync-dsp write the file back in
    the shape it arrived in rather than silently converting it.
    """
    start = blob.find(b"<")
    if start < 0:
        raise SystemExit("no XML in this document")
    return blob[:start], blob[start:]


def cable_slot(document, tag, attr):
    """The one <s> element holding this half's base64'd <Cables> list."""
    for param in document.iter(tag):
        if param.get(attr) != str(HC_PATCH_CABLES):
            continue
        for s in param.iter("s"):
            if (s.text or "").strip():
                return s
        # A present-but-empty list is a real state: no cables at all.
        for s in param.iter("s"):
            return s
    return None


def read_cables(slot):
    """[(fromModule, fromPin, toModule, toPin, colour)], in document order."""
    if slot is None or not (slot.text or "").strip():
        return []
    # The blob is NUL-terminated -- it is a C string that was stored whole.
    raw = base64.b64decode(slot.text).rstrip(b"\x00")
    return [
        (c.get("fm"), c.get("fp", "0"), c.get("tm"), c.get("tp", "0"), c.get("c"))
        for c in ET.fromstring(raw.decode("utf-8")).iter("Cable")
    ]


def module_types(document):
    return {m.get("Id"): m.get("Type") for m in document.iter("Module")}


def describe(cable, names):
    fm, fp, tm, tp, colour = cable
    return (f"{names.get(fm, '?'):<14} ({fm}) pin {fp:>2}"
            f"  ->  {names.get(tm, '?'):<14} ({tm}) pin {tp:>2}   colour={colour}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("fixture")
    ap.add_argument("--show", action="store_true",
                    help="print both cable lists and whether they agree")
    ap.add_argument("--sync-dsp", action="store_true",
                    help="replace the DSP list with the Editor list and write -o")
    ap.add_argument("-o", "--out", help="output fixture for --sync-dsp")
    args = ap.parse_args()

    if not (args.show or args.sync_dsp):
        ap.error("nothing to do: pass --show or --sync-dsp")
    if args.sync_dsp and not args.out:
        ap.error("--sync-dsp needs -o/--out; this never edits in place")

    preset, param, header, raw, document = load_document(args.fixture)
    names = module_types(document)
    slots = {half: cable_slot(document, tag, attr) for half, tag, attr in HALVES}

    # A HALF WITH NO HC_PATCH_CABLES PARAMETER HAS NO CABLES, which is not an
    # error and must not be reported as one: `DefaultRack.synthedit` and every
    # prefab in RackModules/ are in exactly that state -- the shipped rack has
    # no rack cabling, so its DSP half carries no such parameter at all while
    # its editor half carries an empty one. Treating that as malformed would
    # make this unusable as a check over the fixtures that matter (E84).
    lists = {half: read_cables(slot) for half, slot in slots.items()}

    if args.show:
        print(f"{args.fixture}")
        for half in ("DSP", "Editor"):
            print(f"  {half:<7} {len(lists[half])} cable(s)")
            for cable in lists[half]:
                print(f"      {describe(cable, names)}")
        dsp, editor = set(lists["DSP"]), set(lists["Editor"])
        if dsp == editor:
            print("  AGREE -- the audio graph is wired the way the panel draws it")
            return 0
        print("  DISAGREE -- the panel and the audio graph are wired differently")
        for cable in sorted(editor - dsp):
            print(f"    drawn but NOT in the audio graph: {describe(cable, names)}")
        for cable in sorted(dsp - editor):
            print(f"    in the audio graph but NOT drawn: {describe(cable, names)}")
        return 1

    # --sync-dsp: the editor's base64, verbatim, into the DSP slot -- as a byte
    # edit on the decoded document, so the decoded diff is one line.
    if slots["DSP"] is None:
        print(f"{args.fixture}: no DSP HC_PATCH_CABLES parameter to write into -- "
              f"this repairs a stale list, it does not create one", file=sys.stderr)
        return 2
    editor_b64 = (slots["Editor"].text or "").strip().encode("ascii") if slots["Editor"] is not None else b""
    if not editor_b64:
        print(f"{args.fixture}: the Editor half has no cables to copy", file=sys.stderr)
        return 2

    edited, n = DSP_CABLES_RE.subn(lambda m: m.group(1) + editor_b64 + m.group(3),
                                   raw, count=1)
    if n != 1:
        print(f"{args.fixture}: could not locate the DSP HC_PATCH_CABLES <s> element",
              file=sys.stderr)
        return 2

    if preset is None:
        open(args.out, "wb").write(header + edited)
    else:
        param.set("val", base64.b64encode(header + edited).decode("ascii"))
        preset.write(args.out, encoding="UTF-8", xml_declaration=True)
    print(f"wrote {args.out}: DSP list set to the Editor's "
          f"{len(lists['Editor'])} cable(s); decoded document "
          f"{len(raw)} -> {len(edited)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
