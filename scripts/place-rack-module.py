#!/usr/bin/env python3
"""Place, cable or re-option ONE rack module in a TiDE document.

BACKLOG E94. Companion to patch-cables.py and set-view-center.py, and it exists
for the same reason they do: a fixture is a `<Preset>` whose `<Param id="1">`
holds the whole document base64'd, so an edit to it cannot be reviewed in the
committed bytes and any re-encode rewrites the whole blob. The derivation has
to be a command someone can re-run, and the only reviewable form of the change
is the DECODED document's diff.

    ./scripts/place-rack-module.py in.xml -o out.xml \
        --type "VCV: Mixer" --handle 94000094 --panel 1125,276,1170,660 \
        --cable 529566147:5

WHAT A PLACED MODULE IS, in a document, and it is less than you would guess.
Two elements, one per half:

    <DSP>    <Module Id="<handle>" Type="<type>" />
    <Editor> <module type="<type>" handle="<handle>">
                 <structRect l="0" r="0" t="0" b="0" />
                 <panelRect l=".." t=".." r=".." b=".." />
             </module>

**NO `<Parameter>` ENTRIES, and that is measured rather than assumed:** in
`e83-vcv-scope-cabled.xml` the `VCV: Scope` is exactly these two elements and
nothing else -- `grep 987654321` over the decoded document returns two lines --
and E83 measured that Scope receiving audio. A module whose parameters are not
stored gets its declared defaults, which is what a freshly placed one wants.
The `<Plugs>` list is likewise only there to carry non-default plug values.

So this script writes the minimum and lets the plug-in supply the rest. The
check that it worked is the adaptor's own construction trace, not this script's
exit code: build with `-DRACK_ADAPTOR_TRACE=1` and look for
`RackProcessor: '<slug>' constructed`. A type string the factory does not know
raises a `Module not found in factory` dialog instead, so verify the type
against the module's generated XML -- `tests/e19_datatype_census_probe.sh
'' '' '' --xml <Model>` prints it, `<Plugin id=...>` being the type.

PANEL GEOMETRY. `--panel l,t,r,b` in the master container's own panel space,
which is the space every `panelRect` in the document already uses. The width is
the module's SVG width exactly (`modules/<M>/res/<M>.svg`) and the height is
that SVG's height plus 4; the rack snaps to a 15-unit grid, so l and r should
both be multiples of 15. Overlap is not checked -- `--panel` is deliberately
explicit, because the alternative is this script computing a layout, which is
what `build-prefabs.py` does and what Jeff's 2026-08-24 ruling took away from
it.

CABLES. `--cable <fromHandle>:<fromPin>[:<toPin>[:<colour>]]` appends one cable
per flag INTO BOTH HALVES' `HC_PATCH_CABLES` lists, with the new module as the
destination; `--cable-out <toHandle>:<toPin>[:<fromPin>[:<colour>]]` does the
same with the new module as the SOURCE, and `--drop-cable
<fm>:<fp>:<tm>:<tp>` removes an existing one. Always both halves, because a
document whose two lists disagree draws a cable that carries nothing -- E83's
whole finding, and `patch-cables.py --show` is the check for it. The pin
defaults are 0 and the colour 3.

WHY `--drop-cable` EXISTS, since inserting a module into an existing chain is
not the obvious use of a placement script. The adaptor's `first NONZERO` trace
is ONE LATCH PER INSTANCE and it checks inputs before outputs
(`RackAdaptor.h:629-643`), so a module with a connected input NEVER traces its
own output -- which is what E94's row recommended reading. Measured: a Mixer
fed from the LFO printed `first NONZERO INPUT pin 0 (1.000000)` and no OUTPUT
line at all. The readable arrangement is therefore to splice the module INTO
the chain, so a downstream module's input trace reports its output.

OPTIONS. `--set-option <moduleParamId>:<value>` writes an EXISTING parameter's
stored value, which is how a rack module's context-menu option is carried: the
Mixer's `Invert output` is `<param type="4" module="<handle>" moduleParamId="1">`
with a `<patch-list>`, and the adaptor binds it to the pin the menu pick writes
(`RackAdaptor.h`'s `menuBoolPins`). So a document with that value set is what a
user's saved rack looks like after they tick the item, and a fresh instance
receives it with its other pins.

It only edits an entry that is already there, and that bounds when it is
usable: a FRESHLY PLACED module has no parameter entries at all -- measured,
and it is not a defect, because the plug-in creates them on load with the
module's declared defaults. So the sequence is: place the module, load the
fixture once in a host and `--save` it (which is a `<Preset>` again), then
`--set-option` on that save.

Both halves are written when both carry the entry. Current builds wire the DSP
from their own export of `<Editor>` (docs/decisions.md, E81), so the Editor
half is the one that decides; the DSP half is written when present only so an
older build reads the same thing.

`--type` and `--panel` are required only when PLACING a module. Give `--handle`
alone, with `--cable`/`--drop-cable`/`--set-option`, to edit one that is
already in the document.

Exit codes: 0 wrote the file - 2 the document could not be read or an anchor
was not found. It never edits in place.
"""

import argparse
import base64
import re
import sys
import xml.etree.ElementTree as ET

HC_PATCH_CABLES = 49  # SynthEditLib/HostControls.h -- counted, not guessed.

# The DSP half and the Editor half spell the cable parameter differently; the
# same pair patch-cables.py carries.
HALVES = (("DSP", "Parameter", "HostControl"), ("Editor", "param", "hostControl"))


def load_document(path):
    """(preset, param, header, raw, tree) -- patch-cables.py's loader, verbatim."""
    raw = open(path, "rb").read()
    if b"<Preset" in raw[:400]:
        preset = ET.parse(path)
        for param in preset.getroot().iter("Param"):
            if param.get("id") == "1":
                blob = base64.b64decode(param.get("val"))
                start = blob.find(b"<")
                return preset, param, blob[:start], blob[start:], ET.fromstring(blob[start:])
        raise SystemExit(f"{path}: a <Preset> with no <Param id=\"1\">")
    start = raw.find(b"<")
    if start < 0:
        raise SystemExit(f"{path}: no XML in this document")
    return None, None, raw[:start], raw[start:], ET.fromstring(raw[start:])


def rack_handle(document):
    """The master container's handle -- the rack the modules are siblings in."""
    master = document.find("Editor/master_container")
    if master is None or not master.get("handle"):
        raise SystemExit("no <Editor><master_container handle=...>: not a TiDE document")
    return master.get("handle")


def insert_after(raw, anchor, text, what):
    """Insert `text` on its own line after the line holding `anchor`.

    A byte edit on the decoded document, for set-view-center.py's reason:
    re-serialising through ElementTree would rewrite every tag in a 38 KB
    document to add two elements, and the decoded diff is the only form in
    which this edit can be reviewed at all.
    """
    at = raw.find(anchor)
    if at < 0:
        raise SystemExit(f"could not find the {what} anchor: {anchor!r}")
    eol = raw.find(b"\n", at)
    if eol < 0:
        raise SystemExit(f"the {what} anchor is on the last line")
    return raw[:eol + 1] + text + raw[eol + 1:]


def cable_slot(document, tag, attr):
    """The one <s> element holding this half's base64'd <Cables> list."""
    for param in document.iter(tag):
        if param.get(attr) != str(HC_PATCH_CABLES):
            continue
        for s in param.iter("s"):
            if (s.text or "").strip():
                return s
        for s in param.iter("s"):
            return s
    return None


def edit_cables(raw, document, added, dropped, half, tag, attr):
    """Add and remove cables in one half's list, as a byte edit on its base64."""
    slot = cable_slot(document, tag, attr)
    if slot is None:
        raise SystemExit(f"the {half} half has no HostControl {HC_PATCH_CABLES} parameter")
    old_b64 = (slot.text or "").strip()
    blob = base64.b64decode(old_b64)
    trailing = b"\x00" if blob.endswith(b"\x00") else b""
    text = blob.rstrip(b"\x00").decode("utf-8")

    for fm, fp, tm, tp in dropped:
        # The cable's own line, matched on all four endpoints so a fan-out from
        # the same pin to a different module is never the one removed.
        pattern = re.compile(
            rf'[ \t]*<Cable fm="{fm}" tm="{tm}" fp="{fp}" tp="{tp}"[^/]*/>\n')
        text, n = pattern.subn("", text, count=1)
        if n != 1:
            raise SystemExit(
                f"the {half} list has no cable {fm}:{fp} -> {tm}:{tp} to drop")

    if "</Cables>" not in text:
        raise SystemExit(f"the {half} cable list has no </Cables>")
    new = "".join(
        f'    <Cable fm="{fm}" tm="{tm}" fp="{fp}" tp="{tp}" c="{c}"/>\n'
        for fm, fp, tm, tp, c in added)
    text = text.replace("</Cables>", new + "</Cables>", 1)

    new_b64 = base64.b64encode(text.encode("utf-8") + trailing).decode("ascii")
    if old_b64.encode("ascii") not in raw:
        raise SystemExit(f"the {half} base64 cable string is not in the document bytes")
    return raw.replace(old_b64.encode("ascii"), new_b64.encode("ascii"), 1)


def set_option(raw, handle, param_id, value):
    """Write one existing parameter's stored value, in whichever halves hold it.

    A byte edit on the decoded document, as everything else here is. The two
    halves spell the same fact differently -- `<Parameter Module= ModuleParamId=>`
    in <DSP>, `<param module= moduleParamId=>` in <Editor> -- and each wraps
    the value in a `<patch-list>` whose `<s>` may carry a `repeat` count.
    """
    written = []
    for half, tag, m_attr, p_attr in (("DSP", "Parameter", "Module", "ModuleParamId"),
                                      ("Editor", "param", "module", "moduleParamId")):
        # Group 1 is the element's open tag through its first <s ...>, group 2
        # the stored value, group 3 the closing tag. Written as one f-string so
        # the three groups are visible rather than assembled.
        pattern = (rf'(<{tag}\b[^>]*\b{m_attr}="{handle}"[^>]*'
                   rf'\b{p_attr}="{param_id}"[^>]*>\s*<patch-list>\s*<s[^>]*>)'
                   rf'([^<]*)(</s>)')
        raw, n = re.subn(pattern.encode("ascii"),
                         lambda m: m.group(1) + str(value).encode("ascii") + m.group(3),
                         raw, count=1)
        if n:
            written.append(half)
    if not written:
        raise SystemExit(
            f"no parameter entry for module {handle} moduleParamId {param_id} -- "
            f"a freshly placed module has none; see --set-option in the header")
    return raw, written


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("fixture")
    ap.add_argument("-o", "--out", required=True, help="output fixture; never edits in place")
    ap.add_argument("--type", help='the factory type string, e.g. "VCV: Mixer"; placing only')
    ap.add_argument("--handle", required=True, help="the module's handle")
    ap.add_argument("--panel", metavar="L,T,R,B",
                    help="panelRect in the master container's panel space; placing only")
    ap.add_argument("--cable", action="append", default=[], metavar="FROM:FP[:TP[:C]]",
                    help="cable FROM:FP into this module's pin TP (default 0), colour C (default 3)")
    ap.add_argument("--cable-out", action="append", default=[], metavar="TO:TP[:FP[:C]]",
                    help="cable this module's pin FP (default 0) into TO:TP, colour C (default 3)")
    ap.add_argument("--drop-cable", action="append", default=[], metavar="FM:FP:TM:TP",
                    help="remove an existing cable from both halves")
    ap.add_argument("--set-option", action="append", default=[], metavar="PARAMID:VALUE",
                    help="write an existing parameter's stored value on this module")
    args = ap.parse_args()

    placing = bool(args.type or args.panel)
    if placing and not (args.type and args.panel):
        ap.error("placing a module needs both --type and --panel")
    if not placing and not (args.cable or args.cable_out or args.drop_cable or args.set_option):
        ap.error("nothing to do: give --type/--panel to place, or an edit flag")

    preset, param, header, raw, document = load_document(args.fixture)

    present = f'"{args.handle}"'.encode("ascii") in raw
    if placing and present:
        print(f"handle {args.handle} is already used in this document", file=sys.stderr)
        return 2
    if not placing and not present:
        print(f"handle {args.handle} is not in this document", file=sys.stderr)
        return 2

    l = t = r = b = 0
    if placing:
        try:
            l, t, r, b = (int(v) for v in args.panel.split(","))
        except ValueError:
            print(f"--panel wants four integers L,T,R,B, got {args.panel!r}", file=sys.stderr)
            return 2

    cables = []          # (fm, fp, tm, tp, colour), ready to write
    for flag, spec in ([("--cable", s) for s in args.cable]
                       + [("--cable-out", s) for s in args.cable_out]):
        bits = spec.split(":")
        if not 2 <= len(bits) <= 4:
            print(f"{flag} wants HANDLE:PIN[:PIN[:C]], got {spec!r}", file=sys.stderr)
            return 2
        other, other_pin = bits[0], bits[1]
        own_pin = bits[2] if len(bits) > 2 else "0"
        colour = bits[3] if len(bits) > 3 else "3"
        if f'"{other}"'.encode("ascii") not in raw:
            print(f"{flag} handle {other} is not in this document", file=sys.stderr)
            return 2
        if flag == "--cable":
            cables.append((other, other_pin, args.handle, own_pin, colour))
        else:
            cables.append((args.handle, own_pin, other, other_pin, colour))

    dropped = []
    for spec in args.drop_cable:
        bits = spec.split(":")
        if len(bits) != 4:
            print(f"--drop-cable wants FM:FP:TM:TP, got {spec!r}", file=sys.stderr)
            return 2
        dropped.append(tuple(bits))

    options = []
    for spec in args.set_option:
        bits = spec.split(":")
        if len(bits) != 2:
            print(f"--set-option wants PARAMID:VALUE, got {spec!r}", file=sys.stderr)
            return 2
        options.append((bits[0], bits[1]))

    before = len(raw)

    if placing:
        # The DSP half: a bare self-closing <Module>, a sibling of the rack's
        # other modules. Anchored on the rack container's own <Modules> open
        # tag, so the module lands in the rack rather than in a child
        # container. A document with no <DSP> half (every save since
        # 2026-10-09) has nothing to anchor on and nothing that needs one.
        rack = rack_handle(document)
        if document.find("DSP") is not None:
            dsp_anchor = re.search(
                rb'<Module Id="' + rack.encode("ascii") + rb'"[^>]*>.*?(<Modules>)',
                raw, re.DOTALL)
            if not dsp_anchor:
                print("no <Modules> list under the rack container in <DSP>", file=sys.stderr)
                return 2
            raw = (raw[:dsp_anchor.end(1)] + b"\n            "
                   + f'<Module Id="{args.handle}" Type="{args.type}" />'.encode("utf-8")
                   + raw[dsp_anchor.end(1):])

        # The Editor half: the module plus its two rects, in the master
        # container's <modules> list.
        editor = (f'        <module type="{args.type}" handle="{args.handle}">\n'
                  f'          <structRect l="0" r="0" t="0" b="0" />\n'
                  f'          <panelRect l="{l}" r="{r}" t="{t}" b="{b}" />\n'
                  f'        </module>\n').encode("utf-8")
        raw = insert_after(raw, b"      <modules>", editor, "Editor <modules>")

    if cables or dropped:
        for half, tag, attr in HALVES:
            if document.find("DSP") is None and half == "DSP":
                continue
            raw = edit_cables(raw, document, cables, dropped, half, tag, attr)

    halves_written = []
    for param_id, value in options:
        raw, halves = set_option(raw, args.handle, param_id, value)
        halves_written.append(f"{param_id}={value} in {'+'.join(halves)}")

    if preset is None:
        open(args.out, "wb").write(header + raw)
    else:
        param.set("val", base64.b64encode(header + raw).decode("ascii"))
        # Written as BYTES with LF, not through ElementTree's file path: on
        # Windows that one wrote CRLF, so the committed file and the file the
        # derivation produced differed by three bytes and two md5s -- which is
        # exactly the figure a fixture README exists to let a reader check.
        # .gitattributes pins this tree to LF everywhere for the same reason.
        body = ET.tostring(preset.getroot(), encoding="UTF-8", xml_declaration=True)
        open(args.out, "wb").write(body.replace(b"\r\n", b"\n"))

    what = (f"placed {args.type} as {args.handle} at {l},{t},{r},{b}" if placing
            else f"edited module {args.handle}")
    print(f"wrote {args.out}: {what}; {len(cables)} cable(s) added, "
          f"{len(dropped)} dropped"
          + (f"; option(s) {', '.join(halves_written)}" if halves_written else "")
          + f"; decoded document {before} -> {len(raw)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
