#!/usr/bin/env python3
"""Check that every rack prefab keeps the TiDE panel/patch-point contract.

The contract is docs/tide-panel-layout.md: a rack module is an inert
`SE TiDE:Panel` painting the faceplate, with functional `TiDE Patch Point
In/Out` and `SE TiDE:knob` modules layered over it, and **the panel's `Layout`
pin and the modules' `panelRect` centres are the same point written twice**.
Nothing in the product checks that they agree (the panel cannot see the modules
above it), and Jeff ruled the duplication acceptable for now -- so agreement is
exactly the kind of quantised, invisible-until-wrong property that needs a
measurement rather than an eyeball. This script is that measurement.

Checks, per RackModules/*.synthedit:

  1. the XML parses and module handles are unique
  2. every <module type=X> has a <Plugin id=X> definition in PluginList
  3. a document with an `SE TiDE:Panel` must not also contain the stock
     `SE Patch Point in`/`out` -- those draw their OWN jack picture on top of
     the panel's painted one (two jacks, one real and one painted)
  4. the panel is RackUnits x 48 DIPs wide and 384 tall (E5's ruled grid),
     and the container's PanelWndPosition/panel_rect agree with it
  5. the Layout pin is set explicitly (the pin's default is the demo layout,
     not this module's), it parses, and its jack/knob entries match the patch
     point / knob module centres EXACTLY, both directions, relative to the
     panel's top-left
  6. every layout coordinate sits on the half-DIP widget grid, and every
     panel-visible child sits inside the panel rect

A file with no `SE TiDE:Panel` at all is reported as `old-style` and FAILS:
after the 2026-08 upgrade every shipped prefab carries a panel, so a new
panel-less file is either unfinished or a regression.

Exit 0 when every file passes; 1 otherwise. No arguments needed; pass file
paths to check a subset (used by the negative-control test in the PR).
"""
import pathlib
import sys
import xml.etree.ElementTree as ET

HERE = pathlib.Path(__file__).resolve().parent
RACK_MODULES = (HERE / ".." / "RackModules").resolve()

RACK_UNIT_DIPS = 48    # TiDEPanelGui kRackUnitDips
RACK_HEIGHT_DIPS = 384 # kRackHeightDips, E5's ruled row height
WIDGET_GRID = 0.5      # kWidgetGridDips

PANEL = "SE TiDE:Panel"
KNOB = "SE TiDE:knob"
TIDE_JACKS = ("TiDE Patch Point In", "TiDE Patch Point Out")
STOCK_JACKS = ("SE Patch Point in", "SE Patch Point out")

# Layout kinds that pair with a functional module vs decoration-only.
PAIRED_KINDS = {"jack", "knob"}
DECOR_KINDS = {"grill", "slots", "switch", "led"}


def rect(el):
    return tuple(float(el.get(k)) for k in ("l", "t", "r", "b"))


def centre(r):
    l, t, rr, b = r
    return ((l + rr) / 2.0, (t + b) / 2.0)


def snap(v):
    return round(v / WIDGET_GRID) * WIDGET_GRID


def parse_layout(text):
    """Mirror parsePanelLayout (TiDEPanelGui.cpp): ';'/newline separated,
    '#' comments -- but STRICT: the module skips a bad statement so live pin
    edits cannot blank the panel; in a committed file a bad statement is a bug.
    Returns (entries, errors); entries are (kind, x, y)."""
    entries, errors = [], []
    for raw in text.replace("\r", "\n").replace(";", "\n").split("\n"):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        tok = line.split()
        kind = tok[0]
        if kind not in PAIRED_KINDS | DECOR_KINDS:
            errors.append(f"unknown layout statement {line!r}")
            continue
        coords = tok[1:]
        if kind == "knob" and coords and coords[0] in ("big", "small"):
            coords = coords[1:]
        try:
            x, y = float(coords[0]), float(coords[1])
        except (IndexError, ValueError):
            errors.append(f"unparseable layout statement {line!r}")
            continue
        if (snap(x), snap(y)) != (x, y):
            errors.append(f"layout {line!r} is off the {WIDGET_GRID}-DIP grid")
        entries.append((kind, x, y))
    return entries, errors


def check_file(path):
    errors = []
    try:
        doc = ET.parse(path).getroot()
    except ET.ParseError as e:
        return [f"XML does not parse: {e}"]

    modules = doc.findall(".//module")
    plugin_ids = {p.get("id") for p in doc.findall("./PluginList/Plugin")}

    handles = [m.get("handle") for m in modules]
    for h in sorted(set(h for h in handles if handles.count(h) > 1)):
        errors.append(f"duplicate module handle {h}")

    for mtype in sorted(set(m.get("type") for m in modules)):
        if mtype not in plugin_ids:
            errors.append(f"module type {mtype!r} has no PluginList definition")

    panels = [m for m in modules if m.get("type") == PANEL]
    if not panels:
        errors.append("old-style: no SE TiDE:Panel (see docs/tide-panel-layout.md)")
        return errors
    if len(panels) > 1:
        errors.append(f"{len(panels)} SE TiDE:Panel modules; expected 1")
    panel = panels[0]

    for stock in STOCK_JACKS:
        n = sum(1 for m in modules if m.get("type") == stock)
        if n:
            errors.append(f"{n}x stock {stock!r} on a TiDE panel -- use the TiDE patch points")

    # -- the panel's own geometry -------------------------------------------
    pl, pt, pr, pb = rect(panel.find("panelRect"))
    units = 1
    for plug in panel.findall("./plugs/plug"):
        if plug.get("idx") == "2":
            units = int(plug.get("default"))
    width, height = pr - pl, pb - pt
    if width != units * RACK_UNIT_DIPS or height != RACK_HEIGHT_DIPS:
        errors.append(
            f"panel is {width:g}x{height:g}, expected "
            f"{units * RACK_UNIT_DIPS}x{RACK_HEIGHT_DIPS} for {units} rack unit(s)")

    # The container the panel sits in: its PanelWndPosition must be NON-EMPTY.
    # That rect is what the rack draws (e2a-prefabs 9.1) -- SynthEditCL saves it
    # 0,0,0,0, and such a prefab drops successfully, selects, reports the right
    # size, and paints nothing. Its exact size is NOT checked: the view rewrites
    # it to the union of the visible children on the first panel pass, so the
    # hand-authored references legitimately carry stale editor values.
    container = next((m for m in modules
                      if m.find("modules") is not None and
                      panel in m.find("modules").findall("module")), None)
    if container is not None:
        cl, ct, cr, cb = rect(container.find("PanelWndPosition"))
        if cr - cl <= 0 or cb - ct <= 0:
            errors.append(
                f"container PanelWndPosition is {cr - cl:g}x{cb - ct:g} -- a "
                f"zero-size rect drops into the rack and draws NOTHING (e2a-prefabs 9.1)")

    # -- the layout pin vs the functional modules ---------------------------
    layout_plug = next((p for p in panel.findall("./plugs/plug")
                        if p.get("idx") == "3"), None)
    if layout_plug is None:
        errors.append("Layout pin not set -- the panel is painting the pin's demo default")
        return errors
    entries, layout_errors = parse_layout(layout_plug.get("default", ""))
    errors += layout_errors

    def centres_of(types):
        out = []
        for m in modules:
            if m.get("type") in types:
                cx, cy = centre(rect(m.find("panelRect")))
                out.append((cx - pl, cy - pt))
        return sorted(out)

    for kind, types in (("jack", TIDE_JACKS), ("knob", (KNOB,))):
        painted = sorted((x, y) for k, x, y in entries if k == kind)
        real = centres_of(types)
        if painted != real:
            def fmt(pts):
                return ", ".join(f"({x:g},{y:g})" for x, y in pts) or "none"
            errors.append(
                f"{kind} mismatch: Layout paints [{fmt(painted)}] but the "
                f"functional modules sit at [{fmt(real)}] (move one, move both)")

    # -- everything visible sits on the plate -------------------------------
    for m in modules:
        if m.get("type") in TIDE_JACKS + (KNOB, "SE Label"):
            r = rect(m.find("panelRect"))
            if r[0] < pl or r[1] < pt or r[2] > pr or r[3] > pb:
                errors.append(
                    f"{m.get('type')} handle {m.get('handle')} at "
                    f"({r[0]:g},{r[1]:g})..({r[2]:g},{r[3]:g}) overhangs the panel "
                    f"({pl:g},{pt:g})..({pr:g},{pb:g}) -- SubView::measure will "
                    f"stretch the container past the plate")
    return errors


def main(argv):
    paths = [pathlib.Path(a) for a in argv] or sorted(RACK_MODULES.glob("*.synthedit"))
    failed = False
    for path in paths:
        errors = check_file(path)
        status = "ok" if not errors else "FAIL"
        print(f"{status:4}  {path.name}")
        for e in errors:
            print(f"      - {e}")
        failed |= bool(errors)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
