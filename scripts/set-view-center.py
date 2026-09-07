#!/usr/bin/env python3
"""Set the saved view centre of a TiDE preset fixture, and say what it was.

BACKLOG E75. Exists because the field this edits is invisible: a TiDE fixture is
a `<Preset>` whose `<Param id="1">` holds the whole document base64'd, so the
one attribute that decides WHERE THE RACK VIEW OPENS cannot be read, diffed or
reviewed in the committed file. Any re-encoding rewrites the whole blob, so the
committed diff of a one-field change is a total rewrite no matter how the change
was made -- which is exactly why the derivation has to be a command someone can
re-run rather than a hand edit someone has to trust.

  ./scripts/set-view-center.py in.xml --show
  ./scripts/set-view-center.py in.xml --center 988.5,468 -o out.xml

WHAT THE FIELD IS. `PanelLocationCenter` on the master container is the ground
truth for the panel view's pan position -- `SynthEditLib/EditorLib/CContainer.h`
says so in as many words ("Ground truth: view center (document coords) and zoom
factor"), and the `PanelScroll` / `PanelLocation` pair beside it are legacy,
synthesised on save by `CContainer::preSaveState` and read back only when
`PanelLocationZoom` is 0. TiDE restores it on open: BACKLOG E33, at
`SynthEditSem/TideApp.cpp`'s `setPanZoom(targetContainer->GetViewCenter(...))`.

WHY A FIXTURE NEEDS IT SET AT ALL, which is the part worth reading. `CContainer`'s
constructor initialises the field to (3984, 3984) -- `viewDimensions / 2`, the
canvas midpoint. So a document that was never panned before it was saved carries
a value that LOOKS deliberate and means "nobody ever looked at this". TiDE's
rack rows are laid out from the master container's panel rect, which ships at
(3732, 3732), and its modules are conventionally placed in the first row at
y 276..660 -- so such a document opens onto bare rails some 3,400 DIPs from
where its own modules sit.

That is not a rendering fault and scrolling is not the fix. The modules are
inside the canvas and inside the scrollbars' range the whole time; the view
simply does not OPEN on them, and a harness that nudges the wheel a few notches
will not cross 3,400 DIPs. Measured on `tests/fixtures/e53-vcv-rack-segv.xml`,
whose modules sit at x 480..1380, y 276..660 under an untouched (3984, 3984) --
and against `DefaultRack.synthedit`, which puts its modules in the same band and
IS visible, because it carries a centre somebody actually saved.

THE EDIT IS TEXTUAL, NOT AN XML ROUND TRIP, and deliberately so: re-serialising
would reflow attributes and whitespace across the whole document, and then
"these two fixtures are one field apart" could not be checked by diffing their
decoded documents. It rewrites the LAST `<PanelLocationCenter .../>` in the
file, which is the master container's -- every nested container's copy is
serialised inside `<modules>`, and the master's own trailing block comes after
it. The script refuses rather than guesses if that assumption stops holding.

Exit 0 on success, 1 on anything it cannot do safely.
"""
import argparse
import base64
import re
import sys
import xml.etree.ElementTree as ET

CENTRE_RE = re.compile(rb'<PanelLocationCenter\s+x="([^"]*)"\s+y="([^"]*)"\s*/>')
ZOOM_RE = re.compile(rb'(<master_container\b[^>]*?\bPanelLocationZoom=")([^"]*)(")')


def module_bounds(master):
    """Bounding box of every top-level module that has a panel position.

    Two elements carry one and both count: a plain module writes `panelRect`,
    a Container writes `PanelWndPosition`. The rack modules ARE Containers, so
    reading only `panelRect` finds none of them -- which is the shape of mistake
    that makes a rack look empty when it is not. An all-zero rect is skipped:
    that is how a module with no panel presence is written, not one at the
    origin.
    """
    box = None
    for mod in master.find('modules') or []:
        for tag in ('panelRect', 'PanelWndPosition'):
            el = mod.find(tag)
            if el is None:
                continue
            r = tuple(float(el.get(k, 0)) for k in ('l', 't', 'r', 'b'))
            if r == (0.0, 0.0, 0.0, 0.0):
                continue
            box = r if box is None else (
                min(box[0], r[0]), min(box[1], r[1]),
                max(box[2], r[2]), max(box[3], r[3]))
            break
    return box


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('preset', help='a <Preset> fixture with the document in <Param id="1">')
    ap.add_argument('--center', '--centre', dest='center', help='X,Y in document coordinates')
    ap.add_argument('--zoom', help='PanelLocationZoom, e.g. 0.65 -- a centre alone cannot '
                                   'fit content wider than the view')
    ap.add_argument('--show', action='store_true', help='report and change nothing')
    ap.add_argument('-o', '--out', help='write here instead of over the input')
    args = ap.parse_args()

    if not args.show and not (args.center or args.zoom):
        print('nothing to do: pass --center, --zoom or --show', file=sys.stderr)
        return 1

    # Two shapes, because the two files worth comparing are one of each: a
    # fixture is a <Preset> carrying the document base64'd, and the shipped
    # DefaultRack.synthedit is a bare <Document>. The default rack is the
    # control for every question this script answers, so refusing to read it
    # would leave the interesting comparison out of reach of the instrument.
    preset = ET.parse(args.preset)
    if preset.getroot().tag == 'Document':
        param = None
        doc = open(args.preset, 'rb').read()
    else:
        param = next((p for p in preset.getroot().iter('Param') if p.get('id') == '1'), None)
        if param is None:
            print(f'{args.preset}: root is <{preset.getroot().tag}> with no '
                  f'<Param id="1"> -- not a TiDE preset or document', file=sys.stderr)
            return 1
        doc = base64.b64decode(param.get('val'))
    hits = list(CENTRE_RE.finditer(doc))
    if not hits:
        print(f'{args.preset}: document has no <PanelLocationCenter/>', file=sys.stderr)
        return 1

    # `Editor/master_container` in a saved preset, but the shipped default rack
    # has it at the root -- the same element, one level up, because that file is
    # written by the editor rather than round-tripped through a host's state.
    root = ET.fromstring(doc)
    master = root.find('Editor/master_container') or root.find('master_container')
    if master is None:
        print(f'{args.preset}: document has no master_container', file=sys.stderr)
        return 1

    # The parse and the regex must agree about which value is the master's, or
    # the textual edit would silently move a nested container's view instead.
    parsed = master.find('PanelLocationCenter')
    last = hits[-1]
    if parsed is None or (parsed.get('x'), parsed.get('y')) != tuple(
            g.decode() for g in last.groups()):
        print(f'{args.preset}: the last <PanelLocationCenter/> is not the master '
              f"container's -- refusing to guess", file=sys.stderr)
        return 1

    box = module_bounds(master)
    print(args.preset)
    print(f'  PanelLocationCenter  ({parsed.get("x")}, {parsed.get("y")})'
          f'{"   <- CContainer default: never panned" if (parsed.get("x"), parsed.get("y")) == ("3984", "3984") else ""}')
    print(f'  PanelLocationZoom    {master.get("PanelLocationZoom")}')
    if box:
        print(f'  module panel bounds  l={box[0]:g} t={box[1]:g} r={box[2]:g} b={box[3]:g}'
              f'   centre ({(box[0]+box[2])/2:g}, {(box[1]+box[3])/2:g})')
    else:
        print('  module panel bounds  none -- no top-level module has a panel position')

    if args.show:
        return 0

    if args.center:
        try:
            x, y = (float(v) for v in args.center.split(','))
        except ValueError:
            print(f'--center wants X,Y, got {args.center!r}', file=sys.stderr)
            return 1

        replacement = f'<PanelLocationCenter x="{x:g}" y="{y:g}" />'.encode()
        doc = doc[:last.start()] + replacement + doc[last.end():]
        print(f'  -> PanelLocationCenter ({x:g}, {y:g})')

    if args.zoom:
        try:
            z = float(args.zoom)
        except ValueError:
            print(f'--zoom wants a number, got {args.zoom!r}', file=sys.stderr)
            return 1
        # Zero would collapse calcViewTransform, and TideApp substitutes 1.0 for
        # it rather than dividing -- so a fixture asking for 0 would silently get
        # a different view from the one it names. Refuse instead.
        if z <= 0.0:
            print(f'--zoom must be > 0; TiDE substitutes 1.0 for a non-positive '
                  f'stored zoom, so {z:g} would not mean what it says', file=sys.stderr)
            return 1
        doc, n = ZOOM_RE.subn(lambda m: m.group(1) + f'{z:g}'.encode() + m.group(3), doc, count=1)
        if n != 1:
            print(f'{args.preset}: master_container has no PanelLocationZoom attribute',
                  file=sys.stderr)
            return 1
        print(f'  -> PanelLocationZoom  {z:g}')

    out = args.out or args.preset
    if param is None:
        open(out, 'wb').write(doc)
    else:
        param.set('val', base64.b64encode(doc).decode('ascii'))
        preset.write(out, encoding='UTF-8', xml_declaration=True)
    print(f'  wrote {out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
