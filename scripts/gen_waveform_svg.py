#!/usr/bin/env python3
"""Waveform icons for the Oscillator's slider switch, one per detent.

The SVG is cropped to the icons' own bounding box -- no padding -- and the
generator prints where its top-left belongs, in panel DIPs, to stderr. Place it
there; the file carries no position of its own.

ORIGIN IS THE CENTRE. SynthEdit's Svg Geometry / Render pair draws path
coordinates 1:1 in DIPs about the CENTRE of the Render module's panelRect
(measured 2026-10-28; the viewBox is ignored), so the viewBox here is centred on
(0,0). A browser then shows the file right too, and the Render module's
panelRect is simply the box printed below.

The slider lever is half the widget's height, so its centre travels H/2:
value 0 (first menu item, Sine) at the bottom, value 1 (last, Pink) at the top.
Detent i of N sits at v = i/(N-1), lever centre y = top + H*(0.25 + 0.5*(1-v)).

Usage: gen_waveform_svg.py [CX TOP HEIGHT [PANEL_UNITS]] > Resources/Oscillator-waveforms.svg

Resources/ is staged wholesale into the plugin's resource folder, and only files the
product uses ship -- so this script lives in scripts/, not beside its output.
"""
import math, sys

WAVES = ["sine", "saw", "ramp", "triangle", "pulse", "white", "pink"]  # menu order
ICON_W, ICON_H, GAP = 10.0, 6.0, 4.0   # DIPs; icon box and gap from the widget
STROKE = "#A6A6A6"

def wave(name, w, h):
    """Polyline points in a w x h box, y down, centred on (0,0)."""
    n = 48
    pts = []
    for k in range(n + 1):
        t = k / n
        if name == "sine":      y = math.sin(2 * math.pi * t)
        elif name == "triangle": y = 4 * t - 1 if t < .5 else 3 - 4 * t
        else: y = 0
        pts.append((t, y))
    if name == "pulse":   # vertical edges, not slopes
        pts = [(0, -1), (0, 1), (.25, 1), (.25, -1), (1, -1)]
    if name == "saw":
        pts = [(0, 1), (.5, -1), (.5, 1), (1, -1)]
    if name == "ramp":
        pts = [(0, -1), (.5, 1), (.5, -1), (1, 1)]
    if name in ("white", "pink"):
        rnd = 12345 if name == "white" else 777
        steps = 20 if name == "white" else 12
        pts = []
        for k in range(steps + 1):
            rnd = (rnd * 1103515245 + 12345) & 0x7fffffff
            a = ((rnd >> 8) & 0xff) / 127.5 - 1
            pts.append((k / steps, a if name == "white" else 0.6 * a))
    return [((t - 0.5) * w, -y * h / 2) for t, y in pts]

def main():
    cx, top, height = 24.0, 140.0, 120.0
    a = sys.argv[1:]
    if len(a) >= 3: cx, top, height = map(float, a[:3])
    widget_w = 8.0
    n = len(WAVES)
    x0 = cx + widget_w / 2 + GAP + ICON_W / 2
    ox = x0 - ICON_W / 2
    oy = top + height * 0.25 - ICON_H / 2
    bw = ICON_W
    bh = height * 0.5 + ICON_H
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{bw:g}" height="{bh:g}" viewBox="{-bw / 2:g} {-bh / 2:g} {bw:g} {bh:g}">']
    print(f"place at x={ox:g} y={oy:g} (panel DIPs), size {bw:g} x {bh:g}", file=sys.stderr)
    for i, name in enumerate(WAVES):
        v = i / (n - 1)
        y0 = top + height * (0.25 + 0.5 * (1 - v))
        pts = " ".join(f"{x0 + x - ox - bw / 2:.2f},{y0 + y - oy - bh / 2:.2f}" for x, y in wave(name, ICON_W, ICON_H))
        out.append(f'<polyline id="{name}" points="{pts}" fill="none" stroke="{STROKE}" '
                   f'stroke-width="1" stroke-linecap="butt" stroke-linejoin="miter"/>')
    out.append("</svg>")
    print("\n".join(out))

main()
