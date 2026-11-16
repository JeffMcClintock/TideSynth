#!/usr/bin/env python3
"""Where a rack module's jacks are, read from a screenshot rather than assumed.

BACKLOG E19 (win/STANDALONE). The standalone probes address the rack in window
DIPs, with the coordinates written down as constants. **Those constants are
not portable, and the reason is not DPI.** The window is the same 1100x626 on
every platform and the command channel takes DIPs, so DPI is already divided
out. What differs is the RACK's OWN ZOOM: measured on the windows box the WT
LFO's panel is 274 DIP tall for a 384-unit art, a zoom of **0.713**, where the
constants in `e82_wtlfo_wave_points_probe.py` were written against mac's
**0.745**. 4% of a 380-unit panel is 11 DIP, and a jack is about 7 DIP across
-- so the drag lands just outside the jack, no cable is made, and the probe
fails at a clause that has nothing to do with what it is testing. Measured
2026-11-16: `WT_JACK = "590,540"` against an actual jack centre of
**586.3, 529.7**.

So the geometry is derived, once, from a screenshot the probe already takes.
What is scale-invariant is not the coordinate but its FRACTION of the panel:
the WT jack sits at 0.81 of the panel's width and 0.88 of its height whatever
the zoom. Find the panel, and the jacks follow.

HOW THE PANEL IS FOUND. A VCV module panel is a large bright rectangle on a
near-black canvas, and nothing else in the rack canvas is. The browser and the
properties pane are bright too, which is why the search is clipped to the
canvas between them. Columns that are mostly bright give the panel's x range,
rows within those columns give its y.

HOW THE JACKS ARE FOUND. They are small dark filled circles on that panel:
connected components of near-black pixels, round, and an order of magnitude
smaller than the knobs above them. They are reported in reading order, so the
caller picks by position rather than by guessing a coordinate.

This reads a PNG the app wrote through its own `--screenshot` verb, so it needs
Pillow and nothing else. A probe that cannot import Pillow should say so and
exit 2 rather than fall back to constants, which is the failure this exists to
remove.
"""

try:
    from PIL import Image
except ImportError:                                   # pragma: no cover
    Image = None


class GeometryError(RuntimeError):
    """The screenshot did not show what the caller expected."""


def _bright(p):
    return p[0] > 200 and p[1] > 200 and p[2] > 200


def _dark(p):
    return p[0] < 70 and p[1] < 70 and p[2] < 70


def find_panel(png_path, window=(1100, 626), canvas_dip=(245, 800)):
    """The brightest large rectangle in the rack canvas: `(x0, y0, x1, y1)` DIP."""
    if Image is None:
        raise GeometryError("Pillow is not installed; rack geometry cannot be measured")
    im = Image.open(png_path).convert("RGB")
    W, H = im.size
    sx, sy = W / window[0], H / window[1]
    px = im.load()

    cx0, cx1 = int(canvas_dip[0] * sx), int(canvas_dip[1] * sx)
    cols = [0] * W
    for x in range(cx0, min(cx1, W)):
        cols[x] = sum(1 for y in range(0, H, 3) if _bright(px[x, y]))
    best = max(cols)
    if best == 0:
        raise GeometryError(f"{png_path}: no module panel in the rack canvas")
    xs = [x for x, n in enumerate(cols) if n > best * 0.6]
    x0, x1 = min(xs), max(xs)

    wide = x1 - x0 + 1
    ys = [y for y in range(H)
          if sum(1 for x in range(x0, x1 + 1) if _bright(px[x, y])) > wide * 0.5]
    if not ys:
        raise GeometryError(f"{png_path}: found a panel's columns but no rows")
    y0, y1 = min(ys), max(ys)
    return (x0 / sx, y0 / sy, x1 / sx, y1 / sy), (im, sx, sy)


def find_jacks(png_path, panel=None, min_area=40, max_area=200):
    """Jack centres on the panel, in DIP, in reading order.

    `max_area` is what separates a jack from a knob: on this rack a jack is
    ~70 screen pixels of black and the smallest knob is ~200, so the gap is
    wide enough not to need shape alone. Both bounds are in SCREEN pixels,
    because that is what the blobs are measured in.
    """
    (x0, y0, x1, y1), (im, sx, sy) = panel or find_panel(png_path)
    px = im.load()
    px0, py0, px1, py1 = int(x0 * sx), int(y0 * sy), int(x1 * sx), int(y1 * sy)

    dark = set()
    for y in range(py0, py1 + 1):
        for x in range(px0, px1 + 1):
            if _dark(px[x, y]):
                dark.add((x, y))

    seen, out = set(), []
    for seed in dark:
        if seed in seen:
            continue
        stack, comp = [seed], []
        seen.add(seed)
        while stack:
            cx, cy = stack.pop()
            comp.append((cx, cy))
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (cx + dx, cy + dy)
                if n in dark and n not in seen:
                    seen.add(n)
                    stack.append(n)
        if not (min_area <= len(comp) <= max_area):
            continue
        xs = [c[0] for c in comp]
        ys = [c[1] for c in comp]
        w, h = max(xs) - min(xs) + 1, max(ys) - min(ys) + 1
        if not 0.7 <= w / h <= 1.4:
            continue
        out.append((sum(xs) / len(xs) / sx, sum(ys) / len(ys) / sy))
    out.sort(key=lambda p: (round(p[1], 1), p[0]))
    return out


def dip(point):
    """A DIP pair as the command channel wants it: `"x,y"`, integers."""
    return "%d,%d" % (round(point[0]), round(point[1]))
