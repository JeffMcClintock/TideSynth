#!/usr/bin/env python3
"""BACKLOG E5 -- does a module on the rack occupy a whole number of HP, and
does it keep out of its neighbours' way?

E5's Accept is a geometric claim about a SAVED RACK, so this reads one and
answers it in numbers rather than by looking at a screenshot:

    a stock GUI module (a scope is the example Jeff gave) dropped on the rack
    occupies a whole number of HP and does not overlap its neighbours

WHY A PROBE AND NOT AN EYEBALL. The two clauses fail by amounts a screenshot
cannot report -- 6.667 HP looks like "about seven" and a 36-DIP overlap looks
like "they touch". Both were measured on macOS 2026-08-21 and both are off by
figures worth carrying in a diff. It also means whoever eventually fixes this
(the mechanism is in GATED `SynthEditLib` -- see the E5 row) can prove it
rather than assert it.

THE GRID IS THE READER'S, NOT OURS. Every constant below is quoted from
`SynthEditLib/modules/se_sdk3_hosting/Presenter.h:38-45`, the `RackLayout`
struct that `ViewBase::renderRack` actually draws the rails from:

    hpWidth    = 15.0   one HP (0.2in at 75dpi) -- the VISUAL unit: rail hole
                        pitch and hole radius. NOT the placement pitch.
    snapWidth  =  3.0   the placement pitch = gcd(12, 15), the coarsest snap on
                        which SynthEdit's 12-DIP multiples AND every VCV module
                        (all multiples of 15) both land exactly
    rowHeight  = 384.0  one rack row = 32x12 = 8x48, clearing a 380 VCV panel
    railHeight = 15.0   drawn along each row's top and bottom -- BACKGROUND ONLY

**A module's usable height is the WHOLE ROW, 384.** The rails are painted as
background by `ViewBase::renderRack` and the module draws over them, exactly as
a real Eurorack panel covers the rails it is screwed to; TIDE draws no mounting
hardware on the panel. An earlier draft of this probe subtracted `2*railHeight`
and reported TIDE's own prefab as TALLER THAN ROW -- that verdict was measuring
against a constant that does not govern. Corrected 2026-08-21 on Jeff's ruling.

Pass --hp / --snap / --row / --rail if those ever move; the defaults are not
guesses -- they are `RackLayout`'s.

WHY A GUI-LESS MODULE NEEDS NO EXCEPTION HERE, measured 2026-08-21 after this
probe reported TIDE's root plumbing and a row (S28) was filed for it in error.
A module with no GUI class does not render on the rack at all -- only in the
structure view -- and it self-excludes from this check by mechanism, because
its saved panel rect is 0,0,0,0 and the zero-rect skip below already drops it.
`SE MIDI to CV 2` is exactly that case: no `*Gui*` registration anywhere in
SynthEditLib, zero rect, skipped, correct.

So do NOT add a name-based exception list. If something appears here that you
think should not, check whether it has a GUI class before concluding anything:
`MIDI In` DOES (`modules_internal/MidiInGui.cpp`), which is why it measures
8x14 and is reported -- it renders because it is meant to. Under the
2026-08-21 I/O-modules ruling it is a rack module that simply has not been
authored to rack proportions yet, which is E2's work and not a probe defect.

WHAT IS DELIBERATELY NOT CHECKED: whether a module's LEFT EDGE sits on a
column boundary. That needs `RackLayout::origin`, which is the master
container's panel rect and is not in the saved document -- and it is not in
E5's Accept either. Width, height and overlap are all origin-independent, and
they are exactly the two clauses.

Usage:
    python3 tests/e5_rack_footprint_probe.py <session.xml | document.xml>
    python3 tests/e5_rack_footprint_probe.py --selftest

Exit 0 when every rack module is a whole number of HP, fits inside one row,
and overlaps nothing. Exit 1 otherwise -- which is the state of `main` today.
"""
import argparse
import base64
import sys
import xml.etree.ElementTree as ET

HP_DEFAULT = 15.0
SNAP_DEFAULT = 3.0
ROW_DEFAULT = 384.0
RAIL_DEFAULT = 15.0

# Floating point slop. Rects are authored and serialised as integers, so this
# only absorbs the /15 division, never a real disagreement.
EPS = 0.01


def load_document(path):
    """Return the document XML text, from either shape of input file.

    TIDE's standalone keeps its patch in `session.xml` with the whole document
    base64'd into a <Param>; a chunk pulled out of a DAW project is the plain
    document. Accepting both is what lets the same probe run against a saved
    host session and against the standalone.
    """
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    root = ET.fromstring(text)
    if root.tag == "Preset":
        param = root.find(".//Param")
        if param is None or not param.get("val"):
            raise SystemExit(f"{path}: <Preset> with no <Param val=...> to decode")
        raw = base64.b64decode(param.get("val"))

        # THE STANDALONE'S CHUNK IS TAGGED, AND THE TAG IS NOT XML. A session
        # written by TIDE Rack begins with a four-byte ASCII magic before the
        # declaration -- `TDs1` on 2026-08-26 -- so feeding the decoded bytes
        # straight to the parser dies with "not well-formed (invalid token):
        # line 1, column 4", which reads like a corrupt file and is not one.
        #
        # Skipping to the first '<' rather than to a fixed offset of 4: the
        # magic is a version tag, so a `TDs2` is expected to be the same shape
        # and a longer header is not ruled out. A document that legitimately
        # starts with '<' is unaffected -- the slice is a no-op there.
        #
        # BACKLOG E36. Before this, verifying an insert meant hand-stripping
        # the prefix in a throwaway snippet, which is why the run prompt has to
        # explain the four bytes to every new session.
        start = raw.find(b"<")
        if start > 0:
            raw = raw[start:]
        return raw.decode("utf-8", "replace")
    return text


def rack_modules(doc_xml):
    """Top-level modules of the rack, with their panel rect.

    Two element names, and picking the wrong one is E2a's documented trap: a
    Container serialises its rect in the parent as `PanelWndPosition`
    (`CContainer.h:214`) while a plain control uses `panelRect`. A Container has
    BOTH -- `panelRect` is then its own internal canvas, not its footprint on
    the rack -- so PanelWndPosition wins wherever it exists.

    A [0,0,0,0] rect means "no panel presence", not "at the origin" (se_dump
    documents the same rule), so those modules are reported and skipped rather
    than counted as a zero-width violation.
    """
    root = ET.fromstring(doc_xml)
    editor = root.find("Editor")
    if editor is None:
        raise SystemExit("no <Editor> section -- is this a TIDE document?")
    master = editor.find("master_container")
    if master is None:
        raise SystemExit("no <master_container> -- is this a TIDE document?")
    mods = master.find("modules")
    if mods is None:
        return []

    out = []
    for mod in mods.findall("module"):
        rect = mod.find("PanelWndPosition")
        source = "PanelWndPosition"
        if rect is None:
            rect = mod.find("panelRect")
            source = "panelRect"
        if rect is None:
            out.append((label(mod), None, "no panel rect"))
            continue
        l, r, t, b = (float(rect.get(k, 0)) for k in ("l", "r", "t", "b"))
        if (r - l) == 0 and (b - t) == 0:
            out.append((label(mod), None, "zero rect (no panel presence)"))
            continue
        out.append((label(mod), (l, t, r, b), source))
    return out


def label(mod):
    name = mod.get("name")
    return mod.get("type", "?") + (f' "{name}"' if name else "")


def overlaps(a, b):
    """Rect intersection area, 0 when they merely touch."""
    l = max(a[0], b[0])
    t = max(a[1], b[1])
    r = min(a[2], b[2])
    bo = min(a[3], b[3])
    if r <= l or bo <= t:
        return 0.0
    return (r - l) * (bo - t)


def check(entries, hp, snap, row, rail, out=sys.stdout):
    # The whole row: rails are background the module covers. See the docstring.
    usable = row
    print(f"rack grid: hpWidth={hp:g} snapWidth={snap:g} rowHeight={row:g} "
          f"railHeight={rail:g} (background)"
          f"  -> usable module height {usable:g}", file=out)
    print(file=out)

    failures = []
    placed = []
    for name, rect, source in entries:
        if rect is None:
            print(f"  --  {name:34s} {source}", file=out)
            continue
        l, t, r, b = rect
        w, h = r - l, b - t
        in_hp = w / hp
        in_snap = w / snap
        whole = abs(in_snap - round(in_snap)) < EPS
        fits = h <= usable + EPS
        flag = "ok" if (whole and fits) else "FAIL"
        if not whole:
            failures.append(f"{name}: {w:g} wide = {in_snap:.3f} snap units "
                            f"({in_hp:.3f} HP), not on the {snap:g}-DIP grid")
        if not fits:
            failures.append(f"{name}: {h:g} tall, exceeds the row's {usable:g}")
        print(f"  {flag:4s} {name:34s} {source:16s} "
              f"w={w:<6g} h={h:<6g} {in_hp:7.3f} HP  "
              f"{'on-grid' if whole else 'OFF-GRID':9s} "
              f"{'fits row' if fits else 'TALLER THAN ROW'}", file=out)
        placed.append((name, rect))

    print(file=out)
    pairs = 0
    for i in range(len(placed)):
        for j in range(i + 1, len(placed)):
            area = overlaps(placed[i][1], placed[j][1])
            if area > 0:
                pairs += 1
                print(f"  FAIL overlap {area:g} sq: {placed[i][0]}  x  {placed[j][0]}", file=out)
                failures.append(f"overlap {area:g} sq between {placed[i][0]} and {placed[j][0]}")
    if not pairs:
        print(f"  ok   no overlaps among {len(placed)} placed module(s)", file=out)

    print(file=out)
    if failures:
        print(f"RESULT: FAIL -- {len(failures)} violation(s)", file=out)
        for f in failures:
            print(f"  - {f}", file=out)
        return 1
    print(f"RESULT: PASS -- {len(placed)} rack module(s) are whole-HP, "
          f"fit one row, and do not overlap", file=out)
    return 0


# --- controls ---------------------------------------------------------------
#
# A probe that can only ever fail proves as little as one that can only ever
# pass -- the S21 lesson, in the other direction. So the selftest runs BOTH
# arms over real document shapes.

def _doc(modules):
    body = "".join(modules)
    return ('<?xml version="1.0" ?><Document><Editor>'
            '<master_container type="Container" handle="1" name="Main">'
            f'<modules>{body}</modules>'
            '</master_container></Editor></Document>')


def _mod(kind, l, t, w, h, container=False):
    tag = "PanelWndPosition" if container else "panelRect"
    return (f'<module type="{kind}" handle="9">'
            f'<{tag} l="{l}" r="{l + w}" t="{t}" b="{t + h}" />'
            f'</module>')


def selftest():
    cases = [
        # name, document, expected rc
        ("on-grid, side by side",
         _doc([_mod("Container", 0, 0, 90, 350, container=True),
               _mod("Container", 90, 0, 105, 350, container=True)]), 0),
        ("off-grid width",
         _doc([_mod("List Entry", 0, 0, 100, 38)]), 1),
        # The ruled model's whole point: a panel is the FULL row. 380 is a VCV
        # panel dropped in as-is and 384 is a TIDE one filling the row -- both
        # legal. Only past 384 is it too tall.
        ("380 tall -- a VCV panel, fits the 384 row",
         _doc([_mod("Container", 0, 0, 90, 380, container=True)]), 0),
        ("384 tall -- a TIDE panel filling the row",
         _doc([_mod("Container", 0, 0, 48, 384, container=True)]), 0),
        ("taller than a row",
         _doc([_mod("Container", 0, 0, 90, 400, container=True)]), 1),
        # The two grids the snap exists to reconcile, each landing exactly.
        ("48 -- TIDE's standard width, 16 snap units",
         _doc([_mod("Container", 0, 0, 48, 384, container=True)]), 0),
        ("30 -- a 2 HP VCV module, 10 snap units",
         _doc([_mod("Container", 0, 0, 30, 380, container=True)]), 0),
        ("36 -- a SynthEdit 12-DIP multiple, 12 snap units",
         _doc([_mod("Container", 0, 0, 36, 384, container=True)]), 0),
        ("100 -- on neither grid",
         _doc([_mod("Container", 0, 0, 100, 384, container=True)]), 1),
        ("overlapping pair, both on-grid",
         _doc([_mod("Container", 0, 0, 90, 350, container=True),
               _mod("Container", 45, 0, 90, 350, container=True)]), 1),
        ("zero rect is skipped, not failed",
         _doc([_mod("SE MIDI to CV 2", 0, 0, 0, 0)]), 0),
        # A Container carrying BOTH rects must be read by PanelWndPosition.
        # Written as the real serialiser writes it: the inner panelRect starts
        # at 0,0 and is a different size from the footprint.
        ("container prefers PanelWndPosition",
         _doc(['<module type="Container" handle="9">'
               '<panelRect l="0" r="100" t="0" b="38" />'
               '<PanelWndPosition l="0" r="90" t="0" b="350" />'
               '</module>']), 0),
    ]
    import io
    failed = 0
    for name, doc, want in cases:
        sink = io.StringIO()
        got = check(rack_modules(doc), HP_DEFAULT, SNAP_DEFAULT, ROW_DEFAULT, RAIL_DEFAULT, out=sink)
        ok = got == want
        failed += 0 if ok else 1
        print(f"{'PASS' if ok else 'FAIL'} {name:38s} rc={got} want={want}")
    print(f"\n{len(cases)} case(s), {failed} failed")
    return 1 if failed else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("document", nargs="?", help="session.xml or a TIDE document XML")
    ap.add_argument("--hp", type=float, default=HP_DEFAULT)
    ap.add_argument("--snap", type=float, default=SNAP_DEFAULT)
    ap.add_argument("--row", type=float, default=ROW_DEFAULT)
    ap.add_argument("--rail", type=float, default=RAIL_DEFAULT)
    ap.add_argument("--selftest", action="store_true",
                    help="run the built-in pass AND fail controls")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.document:
        ap.error("a document is required (or --selftest)")
    return check(rack_modules(load_document(args.document)),
                 args.hp, args.snap, args.row, args.rail)


if __name__ == "__main__":
    sys.exit(main())
