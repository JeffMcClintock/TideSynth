"""Generate the TIDE UI design-language example SVGs into docs/images/.

Source of truth for the four ui-language-*.svg files referenced by
docs/ui-design-language.md. Edit here and re-run; never hand-edit the SVGs.

    python scripts/gen-ui-language-svgs.py

Everything is drawn to the language's own rules: 1px butt-capped strokes,
flat fills, 2px max corner radius, mono caps labels, acid reserved for
live state. Fonts are generic stacks so the SVGs render anywhere.
"""
import math
import os
import re

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "docs", "images")

# ---- palette (from the doc) ----
BLACK   = "#000000"
PANEL   = "#1C1C1C"
PANEL2  = "#262626"
SEAM    = "#3A3A3A"
GREY    = "#A6A6A6"
DIMGREY = "#555555"
WHITE   = "#FFFFFF"
ACID    = "#C0FE04"
VIOLET  = "#5200FF"
CYAN    = "#01FFFF"
AMBER   = "#FF9E00"
RED     = "#FF0D1A"

MONO = "Consolas, 'Cascadia Mono', 'DejaVu Sans Mono', monospace"
DISP = "'Arial Black', 'Archivo Black', Arial, sans-serif"


def pt(cx, cy, r, deg):
    a = math.radians(deg)
    return (cx + r * math.cos(a), cy + r * math.sin(a))


def arc_path(cx, cy, r, d0, d1):
    x0, y0 = pt(cx, cy, r, d0)
    x1, y1 = pt(cx, cy, r, d1)
    large = 1 if (d1 - d0) > 180 else 0
    return (f"M{x0:.2f} {y0:.2f} A{r} {r} 0 {large} 1 {x1:.2f} {y1:.2f}")


A0, A1 = -225.0, 45.0  # 270-degree knob sweep


def knob(cx, cy, val, label, value_text, accent=None, r=14,
         label_col=GREY, tick_col=DIMGREY, track_col=SEAM):
    """A knob drawn as an open arc: track, active arc, square indicator."""
    s = []
    # tick rail: 5 majors
    for i in range(5):
        deg = A0 + (A1 - A0) * i / 4
        x0, y0 = pt(cx, cy, r + 3, deg)
        x1, y1 = pt(cx, cy, r + 6, deg)
        s.append(f'<line x1="{x0:.2f}" y1="{y0:.2f}" x2="{x1:.2f}" y2="{y1:.2f}" '
                 f'stroke="{tick_col}" stroke-width="1" stroke-linecap="butt"/>')
    # track
    s.append(f'<path d="{arc_path(cx, cy, r, A0, A1)}" fill="none" '
             f'stroke="{track_col}" stroke-width="2" stroke-linecap="butt"/>')
    # active arc
    if accent and val > 0.002:
        deg = A0 + (A1 - A0) * val
        s.append(f'<path d="{arc_path(cx, cy, r, A0, deg)}" fill="none" '
                 f'stroke="{accent}" stroke-width="2" stroke-linecap="butt"/>')
    # indicator
    deg = A0 + (A1 - A0) * val
    x0, y0 = pt(cx, cy, r - 7, deg)
    x1, y1 = pt(cx, cy, r, deg)
    s.append(f'<line x1="{x0:.2f}" y1="{y0:.2f}" x2="{x1:.2f}" y2="{y1:.2f}" '
             f'stroke="{WHITE}" stroke-width="2" stroke-linecap="butt"/>')
    # label above, value below
    s.append(text(cx, cy - r - 12, label, 8, label_col, anchor="middle", ls=1.2))
    num, unit = split_value(value_text)
    s.append(value(cx, cy + r + 16, num, unit, 10, anchor="middle", ls=0.5))
    return "\n".join(s)


def text(x, y, t, size, col, anchor="start", ls=None, font=MONO, weight=None):
    extra = f' letter-spacing="{ls}"' if ls else ""
    w = f' font-weight="{weight}"' if weight else ""
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{font}" font-size="{size}" '
            f'fill="{col}" text-anchor="{anchor}"{extra}{w}>{t}</text>')


UNIT_SCALE = 0.70  # units render at 70% of the numeral's size
_VAL_RE = re.compile(r"^([-+]?[0-9][0-9.,]*)\s*(.*)$")


def split_value(t):
    """'1.24 KHZ' -> ('1.24', 'KHZ');  '35%' -> ('35', '%')."""
    m = _VAL_RE.match(t)
    return (m.group(1), m.group(2)) if m else (t, "")


def value(x, y, num, unit, size, anchor="start", num_col=WHITE,
          unit_col=GREY, ls=None):
    """Read-out: numeral at full size/ink, unit at 70% and one tier down."""
    extra = f' letter-spacing="{ls}"' if ls else ""
    us = round(size * UNIT_SCALE, 1)
    tail = (f'<tspan font-size="{us}" fill="{unit_col}" dx="1.5">{unit}</tspan>'
            if unit else "")
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{MONO}" '
            f'font-size="{size}" fill="{num_col}" text-anchor="{anchor}"'
            f'{extra}>{num}{tail}</text>')


def stepped_corner(x, y, unit, col, flip_x=False, flip_y=False):
    """The Marathon stepped-edge device: halving squares 2u -> u -> u/2."""
    sx = -1 if flip_x else 1
    sy = -1 if flip_y else 1
    s = []
    cx_, cy_ = x, y
    for u in (unit * 2, unit, unit / 2):
        px = cx_ if sx > 0 else cx_ - u
        py = cy_ if sy > 0 else cy_ - u
        s.append(f'<rect x="{px:.1f}" y="{py:.1f}" width="{u:.1f}" height="{u:.1f}" fill="{col}"/>')
        cx_ += sx * u
        cy_ += sy * (u / 2) * 0  # steps move horizontally then drop
        cy_ += sy * u / 2 if False else 0
    return "\n".join(s)


def stepped_steps(x, y, u, col, direction=1):
    """Three halving squares stepping diagonally: (2u), (u), (u/2)."""
    s = []
    sizes = [2 * u, u, u / 2]
    cx_, cy_ = x, y
    for sz in sizes:
        s.append(f'<rect x="{cx_:.1f}" y="{cy_:.1f}" width="{sz:.1f}" height="{sz:.1f}" fill="{col}"/>')
        cx_ += sz
        cy_ += direction * sz / 2 * 0
        cy_ -= sz / 2 if direction < 0 else -0.0
        if direction > 0:
            cy_ += 0  # horizontal run handled by caller offset
    return "\n".join(s)


def corner_marks(x, y, w, h, col=GREY, arm=6):
    """Registration crosshairs where a group's corners would be.

    Substitutes for a drawn border: four small '+' marks, and the eye
    completes the rectangle. Quieter than a box and it never boxes in
    content that overflows.
    """
    s = []
    for (cx, cy) in ((x, y), (x + w, y), (x, y + h), (x + w, y + h)):
        s.append(f'<line x1="{cx-arm:.1f}" y1="{cy:.1f}" x2="{cx+arm:.1f}" y2="{cy:.1f}" '
                 f'stroke="{col}" stroke-width="1" stroke-linecap="butt"/>')
        s.append(f'<line x1="{cx:.1f}" y1="{cy-arm:.1f}" x2="{cx:.1f}" y2="{cy+arm:.1f}" '
                 f'stroke="{col}" stroke-width="1" stroke-linecap="butt"/>')
    return "\n".join(s)


def reg_square(cx, cy, col, size=12, tick=18, gap=7):
    """Heavier corner mark: solid square with detached orthogonal ticks."""
    h = size / 2
    return "\n".join([
        f'<rect x="{cx-h:.1f}" y="{cy-h:.1f}" width="{size}" height="{size}" fill="{col}"/>',
        f'<line x1="{cx-h:.1f}" y1="{cy-h-gap:.1f}" x2="{cx-h:.1f}" y2="{cy-h-gap-tick:.1f}" '
        f'stroke="{col}" stroke-width="1" stroke-linecap="butt"/>',
        f'<line x1="{cx+h+gap:.1f}" y1="{cy+h:.1f}" x2="{cx+h+gap+tick:.1f}" y2="{cy+h:.1f}" '
        f'stroke="{col}" stroke-width="1" stroke-linecap="butt"/>',
    ])


def jack(cx, cy, label, state="empty", accent=ACID):
    """Patch point: 1px ring; filled concentric core when connected."""
    s = []
    s.append(f'<circle cx="{cx}" cy="{cy}" r="7.5" fill="{BLACK}" stroke="{GREY}" stroke-width="1"/>')
    if state == "connected":
        s.append(f'<circle cx="{cx}" cy="{cy}" r="7.5" fill="{BLACK}" stroke="{accent}" stroke-width="1"/>')
        s.append(f'<circle cx="{cx}" cy="{cy}" r="3.5" fill="{accent}"/>')
    elif state == "target":
        s.append(f'<rect x="{cx-11}" y="{cy-11}" width="22" height="22" fill="none" stroke="{WHITE}" stroke-width="1"/>')
    s.append(text(cx, cy + 20, label, 8, GREY, anchor="middle", ls=1.0))
    return "\n".join(s)


def dot_defs(pid, cell=21.75, r=1.5, col="rgba(255,255,255,0.16)"):
    """The verified Marathon dot screen: one dot per 21.75px cell, as a pattern."""
    return (f'<defs><pattern id="{pid}" patternUnits="userSpaceOnUse" '
            f'width="{cell}" height="{cell}">'
            f'<circle cx="{cell/2}" cy="{cell/2}" r="{r}" fill="{col}"/>'
            f'</pattern></defs>')


def dot_rect(pid, x, y, w, h):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="url(#{pid})"/>'


# =====================================================================
# 1. THE RACK — three modules showing the livery system
# =====================================================================
def gen_rack():
    W, H = 720, 470
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'viewBox="0 0 {W} {H}" font-family="{MONO}">']
    s.append(f'<rect width="{W}" height="{H}" fill="{BLACK}"/>')
    # rack ground dot screen
    s.append(dot_defs("rackdots", col="rgba(255,255,255,0.07)"))
    s.append(dot_rect("rackdots", 0, 0, W, H))

    modules = [
        # (x, w, code, name, livery, family)
        (30,  200, "SR-01", "PULSE", AMBER, "SOURCE"),
        (250, 200, "FL-03", "LADDER", VIOLET, "FILTER"),
        (470, 200, "MD-02", "DRIFT", CYAN, "MOD"),
    ]

    PX, PY, PH = 0, 40, 390  # panel y/top and height

    for (x, w, code, name, livery, family) in modules:
        # panel
        s.append(f'<rect x="{x}" y="{PY}" width="{w}" height="{PH}" fill="{PANEL}"/>')
        s.append(f'<rect x="{x}" y="{PY}" width="{w}" height="{PH}" fill="none" stroke="{SEAM}" stroke-width="1"/>')

        # header: livery PLATE (code knocked out black) + stepped edge + name
        hy = PY
        s.append(f'<rect x="{x}" y="{hy}" width="{w}" height="28" fill="{BLACK}"/>')
        s.append(f'<line x1="{x}" y1="{hy+28}" x2="{x+w}" y2="{hy+28}" stroke="{SEAM}" stroke-width="1"/>')
        # plate with stepped trailing edge (10 -> 5 -> 2.5)
        s.append(f'<rect x="{x}" y="{hy}" width="46" height="28" fill="{livery}"/>')
        s.append(f'<rect x="{x+46}" y="{hy}" width="10" height="10" fill="{livery}"/>')
        s.append(f'<rect x="{x+56}" y="{hy}" width="5" height="5" fill="{livery}"/>')
        s.append(f'<rect x="{x+61}" y="{hy}" width="2.5" height="2.5" fill="{livery}"/>')
        s.append(text(x + 23, hy + 18, code, 9, BLACK, anchor="middle", ls=0.8, weight="700"))
        s.append(text(x + w - 10, hy + 19, name, 13, WHITE, anchor="end", font=DISP, weight="900"))

        # family tag: quarter-turn, reading up the left edge
        s.append(f'<text transform="rotate(-90 {x+14} {PY+PH-32})" x="{x+14}" y="{PY+PH-32}" '
                 f'font-family="{MONO}" font-size="8" fill="{GREY}" letter-spacing="1.5">{family}</text>')

        # bottom footer
        fy = PY + PH - 22
        s.append(f'<line x1="{x}" y1="{fy}" x2="{x+w}" y2="{fy}" stroke="{SEAM}" stroke-width="1"/>')
        s.append(text(x + 10, fy + 15, "12 HP", 8, GREY, ls=1.5))
        s.append(text(x + w - 10, fy + 15, "TIDE", 8, GREY, anchor="end", ls=1.5))

    # ---- module 1: PULSE (source) — two knobs + wave segmented ----
    x = 30
    s.append(knob(x + 60, 140, 0.62, "FREQ", "261.6 Hz"))
    s.append(knob(x + 145, 140, 0.35, "SHAPE", "35%"))
    # segmented wave select
    seg_y = 205
    opts = ["SAW", "PLS", "SIN"]
    for i, o in enumerate(opts):
        bx = x + 25 + i * 52
        on = (i == 1)
        fill = ACID if on else "none"
        col = BLACK if on else GREY
        s.append(f'<rect x="{bx}" y="{seg_y}" width="52" height="20" fill="{fill}" stroke="{SEAM}" stroke-width="1"/>')
        s.append(text(bx + 26, seg_y + 14, o, 9, col, anchor="middle", ls=1.0))
    s.append(knob(x + 60, 290, 0.5, "FINE", "+0 C"))
    s.append(knob(x + 145, 290, 0.8, "LEVEL", "-2.0 dB"))
    # jacks
    s.append(jack(x + 45, 375, "V/OCT", "connected"))
    s.append(jack(x + 100, 375, "FM", "empty"))
    s.append(jack(x + 155, 375, "OUT", "connected"))

    # ---- module 2: LADDER (filter) — hero knob + modulated cutoff ----
    x = 250
    s.append(knob(x + 100, 160, 0.62, "CUTOFF", "1.24 kHz", accent=ACID, r=26))
    s.append(knob(x + 55, 265, 0.34, "RESO", "34%"))
    s.append(knob(x + 145, 265, 0.18, "DRIVE", "4.3 dB"))
    # slope segmented
    seg_y = 315
    for i, o in enumerate(["12", "24", "36"]):
        bx = x + 25 + i * 52
        on = (i == 1)
        fill = ACID if on else "none"
        col = BLACK if on else GREY
        s.append(f'<rect x="{bx}" y="{seg_y}" width="52" height="20" fill="{fill}" stroke="{SEAM}" stroke-width="1"/>')
        s.append(text(bx + 26, seg_y + 14, o, 9, col, anchor="middle", ls=1.0))
    s.append(jack(x + 45, 375, "IN", "connected"))
    s.append(jack(x + 100, 375, "CV", "connected"))
    s.append(jack(x + 155, 375, "OUT", "connected"))

    # ---- module 3: TIDES (mod) — LFO with live readout bar ----
    x = 470
    # LFO core: grouped by corner marks, not a border
    s.append(corner_marks(x + 20, 98, 165, 78, col=SEAM))
    s.append(f'<rect x="{x+36}" y="93" width="58" height="10" fill="{PANEL}"/>')
    s.append(text(x + 40, 101, "LFO CORE", 8, GREY, ls=1.2))
    s.append(knob(x + 60, 140, 0.28, "RATE", "0.8 Hz"))
    s.append(knob(x + 145, 140, 0.75, "DEPTH", "75%"))
    # phase readout: tick rail + position
    ry = 210
    s.append(f'<line x1="{x+25}" y1="{ry}" x2="{x+175}" y2="{ry}" stroke="{SEAM}" stroke-width="1"/>')
    for i in range(7):
        tx = x + 25 + i * 25
        th = 6 if i % 3 == 0 else 3
        s.append(f'<line x1="{tx}" y1="{ry}" x2="{tx}" y2="{ry-th}" stroke="{GREY}" stroke-width="1"/>')
    s.append(f'<rect x="{x+25}" y="{ry-1}" width="62" height="2" fill="{ACID}"/>')
    s.append(text(x + 25, ry + 14, "PHASE", 8, GREY, ls=1.2))
    s.append(text(x + 175, ry + 14, "112&#176;", 9, WHITE, anchor="end"))
    s.append(knob(x + 60, 290, 0.5, "SKEW", "0%"))
    s.append(knob(x + 145, 290, 0.0, "OFFSET", "0 V"))
    s.append(jack(x + 45, 375, "SYNC", "empty"))
    s.append(jack(x + 100, 375, "UNI", "empty"))
    s.append(jack(x + 155, 375, "OUT", "connected"))

    # patch cables: fat catenary droop, flat colour, square plug heads
    # DRIFT OUT -> LADDER CV (modulation path: acid)
    s.append(f'<path d="M625 375 C625 458, 350 458, 350 375" fill="none" '
             f'stroke="{ACID}" stroke-width="5"/>')
    # PULSE OUT -> LADDER IN (audio: white)
    s.append(f'<path d="M185 375 C185 446, 295 446, 295 375" fill="none" '
             f'stroke="{WHITE}" stroke-width="5" opacity="0.85"/>')
    for (px, py, col) in [(625, 375, ACID), (350, 375, ACID),
                          (185, 375, WHITE), (295, 375, WHITE)]:
        s.append(f'<circle cx="{px}" cy="{py}" r="5" fill="{col}"/>')

    # caption row
    s.append(text(30, 24, "TIDE RACK // MODULE LIVERY SYSTEM", 9, GREY, ls=1.5))
    s.append(text(690, 24, "SOURCE / FILTER / MOD", 9, GREY, anchor="end", ls=1.5))

    s.append("</svg>")
    return "\n".join(s)


# =====================================================================
# 2. WIDGET STATES — the spec sheet
# =====================================================================
def gen_widgets():
    W, H = 720, 320
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'viewBox="0 0 {W} {H}" font-family="{MONO}">']
    s.append(f'<rect width="{W}" height="{H}" fill="{PANEL}"/>')

    s.append(text(30, 30, "KNOB STATES", 9, GREY, ls=1.5))
    # idle / hover / modulated / clipping
    states = [
        (80,  "IDLE",      0.62, None,   SEAM),
        (190, "HOVER",     0.62, None,   GREY),   # track lightens
        (300, "MODULATED", 0.62, ACID,   SEAM),
        (410, "CLIP",      0.97, RED,    SEAM),
    ]
    for (cx, lab, val, accent, track) in states:
        s.append(knob(cx, 95, val, lab, "", accent=accent, track_col=track))
    # annotation
    s.append(text(490, 70, "IDLE: grey track, white indicator.", 9, GREY))
    s.append(text(490, 86, "HOVER: track to #A6A6A6. No glow.", 9, GREY))
    s.append(text(490, 102, "MODULATED: acid arc = live value.", 9, GREY))
    s.append(text(490, 118, "CLIP: red. The only red there is.", 9, GREY))

    # ---- fader + meter ----
    s.append(text(30, 175, "FADER", 9, GREY, ls=1.5))
    fx, fy, fw, fh = 45, 190, 18, 100
    s.append(f'<rect x="{fx}" y="{fy}" width="{fw}" height="{fh}" fill="{BLACK}" stroke="{SEAM}" stroke-width="1"/>')
    lvl = 0.72
    s.append(f'<rect x="{fx}" y="{fy + fh*(1-lvl):.1f}" width="{fw}" height="{fh*lvl:.1f}" fill="{ACID}"/>')
    s.append(f'<rect x="{fx-3}" y="{fy + fh*(1-lvl)-1:.1f}" width="{fw+6}" height="3" fill="{WHITE}"/>')
    for i, t in enumerate(["+6", "0", "-12", "-24"]):
        s.append(text(fx + fw + 8, fy + 4 + i * (fh - 8) / 3 + 3, t, 8, GREY))

    s.append(text(130, 175, "METER", 9, GREY, ls=1.5))
    mx = 140
    for ch, (l, p, clip) in enumerate([(0.74, 0.82, False), (0.62, 0.97, True)]):
        cx = mx + ch * 16
        s.append(f'<rect x="{cx}" y="{fy}" width="11" height="{fh}" fill="{BLACK}" stroke="{SEAM}" stroke-width="1"/>')
        s.append(f'<rect x="{cx}" y="{fy + fh*(1-l):.1f}" width="11" height="{fh*l:.1f}" fill="{ACID}"/>')
        s.append(f'<rect x="{cx}" y="{fy + fh*(1-p)-1:.1f}" width="11" height="2" fill="{WHITE}"/>')
        if clip:
            s.append(f'<rect x="{cx}" y="{fy}" width="11" height="5" fill="{RED}"/>')

    # ---- toggle / segmented ----
    s.append(text(230, 175, "TOGGLE + SEGMENTED", 9, GREY, ls=1.5))
    ty = 190
    s.append(f'<rect x="230" y="{ty}" width="64" height="22" fill="{ACID}"/>')
    s.append(text(262, ty + 15, "ACTIVE", 9, BLACK, anchor="middle", ls=1.0))
    s.append(f'<rect x="302" y="{ty}" width="64" height="22" fill="none" stroke="{SEAM}" stroke-width="1"/>')
    s.append(text(334, ty + 15, "BYPASS", 9, GREY, anchor="middle", ls=1.0))
    sy = ty + 40
    for i, o in enumerate(["A", "B", "C", "D"]):
        bx = 230 + i * 34
        on = (i == 0)
        fill = ACID if on else "none"
        col = BLACK if on else GREY
        s.append(f'<rect x="{bx}" y="{sy}" width="34" height="20" fill="{fill}" stroke="{SEAM}" stroke-width="1"/>')
        s.append(text(bx + 17, sy + 14, o, 9, col, anchor="middle"))

    # ---- jacks ----
    s.append(text(230, 268, "PATCH POINTS", 9, GREY, ls=1.5))
    s.append(jack(245, 292, "", "empty"))
    s.append(text(265, 296, "EMPTY", 8, GREY, ls=1.0))
    s.append(jack(330, 292, "", "connected"))
    s.append(text(350, 296, "PATCHED", 8, GREY, ls=1.0))
    s.append(jack(425, 292, "", "target"))
    s.append(text(447, 296, "TARGET", 8, GREY, ls=1.0))

    # ---- value edit field ----
    s.append(text(490, 175, "VALUE EDIT", 9, GREY, ls=1.5))
    s.append(f'<rect x="490" y="188" width="110" height="24" fill="{BLACK}" stroke="{WHITE}" stroke-width="1"/>')
    s.append(value(500, 205, "1240", "Hz", 12))
    # caret sits after the numeral, before the unit — the numeral is what edits
    s.append(f'<rect x="528" y="192" width="1" height="16" fill="{ACID}"/>')
    s.append(text(490, 232, "Click value to type. Caret is", 9, GREY))
    s.append(text(490, 246, "the one blinking acid element.", 9, GREY))

    # two type sizes rule
    s.append(text(490, 278, "UNITS AT 70%, ONE TIER DOWN:", 9, GREY, ls=1.5))
    s.append(value(490, 300, "24", "dB", 20))
    s.append(value(560, 300, "1.24", "kHz", 20))

    s.append("</svg>")
    return "\n".join(s)


# =====================================================================
# CARD — a reading surface: eyebrow plate / display / stub / mono body
# =====================================================================
def gen_card():
    W, H = 720, 300
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'viewBox="0 0 {W} {H}" font-family="{MONO}">']
    s.append(f'<rect width="{W}" height="{H}" fill="{BLACK}"/>')
    s.append(dot_defs("carddots", col="rgba(255,255,255,0.07)"))
    s.append(dot_rect("carddots", 0, 0, W, H))
    s.append(text(30, 20, "READING SURFACE // MODULE CARD", 9, GREY, ls=1.5))

    cx0, cy0, cw, ch = 30, 34, 460, 240
    s.append(f'<rect x="{cx0}" y="{cy0}" width="{cw}" height="{ch}" fill="{PANEL}"/>')
    s.append(f'<rect x="{cx0}" y="{cy0}" width="{cw}" height="{ch}" fill="none" stroke="{SEAM}" stroke-width="1"/>')

    # eyebrow: livery plate, two levels of caps; white text on the dark plate
    s.append(f'<rect x="{cx0}" y="{cy0}" width="{cw}" height="26" fill="{VIOLET}"/>')
    s.append(text(cx0 + 14, cy0 + 17, "FL-03 FILTER", 10, WHITE, ls=2.0, weight="700"))
    s.append(f'<text x="{cx0+cw-14}" y="{cy0+17}" font-family="{MONO}" font-size="8" '
             f'fill="{WHITE}" text-anchor="end" letter-spacing="1.2" opacity="0.75">12 HP // 3 PORTS</text>')

    # display headline: the size cliff (ratio ~4:1 to body)
    s.append(text(cx0 + 16, cy0 + 86, "LADDER", 42, WHITE, font=DISP, weight="900"))

    # stub rule: short, never full-width
    s.append(f'<rect x="{cx0+18}" y="{cy0+106}" width="26" height="1" fill="{DIMGREY}"/>')

    # body: mono, sentence case, loose leading
    body = ["Four-pole transistor ladder with drive.",
            "Cutoff tracks V/OCT when patched;",
            "resonance self-oscillates past 80%."]
    for i, line in enumerate(body):
        s.append(text(cx0 + 18, cy0 + 136 + i * 18, line, 12, WHITE))

    # annotations, right column
    ann = [(cy0 + 17, "EYEBROW: LIVERY PLATE, 2 CAP LEVELS"),
           (cy0 + 80, "DISPLAY CAPS, RATIO ~4:1 TO BODY"),
           (cy0 + 109, "STUB RULE, 1px, NEVER FULL-WIDTH"),
           (cy0 + 154, "BODY: MONO, SENTENCE CASE, 150%")]
    for (ay, atext) in ann:
        s.append(f'<line x1="{cx0+cw+8}" y1="{ay-3}" x2="{cx0+cw+20}" y2="{ay-3}" stroke="{DIMGREY}" stroke-width="1"/>')
        s.append(text(cx0 + cw + 26, ay, atext, 8, GREY, ls=1.0))

    s.append("</svg>")
    return "\n".join(s)


# =====================================================================
# 3. DEVICES + PALETTE
# =====================================================================
def gen_devices():
    W, H = 720, 570
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'viewBox="0 0 {W} {H}" font-family="{MONO}">']
    s.append(f'<rect width="{W}" height="{H}" fill="{BLACK}"/>')

    # ---- palette strip ----
    s.append(text(30, 30, "PALETTE", 9, GREY, ls=1.5))
    sw = [
        (ACID,   "C0FE04", "STATE"),
        (BLACK,  "000000", "GROUND"),
        (PANEL,  "1C1C1C", "PANEL"),
        (SEAM,   "3A3A3A", "SEAM"),
        (GREY,   "8E8E8E", "LABEL"),
        (WHITE,  "FFFFFF", "VALUE"),
        (RED,    "FF0D1A", "CLIP"),
    ]
    for i, (col, hexs, role) in enumerate(sw):
        x = 30 + i * 96
        stroke = f' stroke="{SEAM}" stroke-width="1"' if col == BLACK else ""
        s.append(f'<rect x="{x}" y="44" width="84" height="40" fill="{col}"{stroke}/>')
        s.append(text(x, 100, "#" + hexs, 9, WHITE))
        s.append(text(x, 114, role, 8, GREY, ls=1.2))

    # livery row
    s.append(text(30, 148, "FAMILY LIVERIES (header block only)", 9, GREY, ls=1.5))
    liv = [(AMBER, "FF9E00", "SOURCE"), (VIOLET, "5200FF", "FILTER"),
           (CYAN, "01FFFF", "MOD"), (WHITE, "FFFFFF", "I/O"), (GREY, "8E8E8E", "UTILITY")]
    for i, (col, hexs, role) in enumerate(liv):
        x = 30 + i * 96
        s.append(f'<rect x="{x}" y="160" width="26" height="26" fill="{col}"/>')
        s.append(f'<rect x="{x+26}" y="160" width="10" height="10" fill="{col}"/>')
        s.append(f'<rect x="{x+36}" y="160" width="5" height="5" fill="{col}"/>')
        s.append(text(x, 200, "#" + hexs, 9, WHITE))
        s.append(text(x, 214, role, 8, GREY, ls=1.2))

    # ---- devices ----
    dy = 246
    s.append(text(30, dy, "DEVICES", 9, GREY, ls=1.5))

    # stepped edge
    s.append(f'<rect x="30" y="{dy+12}" width="50" height="20" fill="{ACID}"/>')
    s.append(f'<rect x="80" y="{dy+12}" width="10" height="10" fill="{ACID}"/>')
    s.append(f'<rect x="90" y="{dy+12}" width="5" height="5" fill="{ACID}"/>')
    s.append(f'<rect x="95" y="{dy+12}" width="2.5" height="2.5" fill="{ACID}"/>')
    s.append(text(30, dy + 48, "STEPPED EDGE 10/5/2.5", 8, GREY, ls=1.0))

    # brackets
    s.append(f'<path d="M240 {dy+8} h-10 v28 h10" fill="none" stroke="{WHITE}" stroke-width="1"/>')
    s.append(f'<path d="M330 {dy+8} h10 v28 h-10" fill="none" stroke="{WHITE}" stroke-width="1"/>')
    s.append(text(255, dy + 26, "FL-03", 10, WHITE, ls=1.0))
    s.append(text(230, dy + 52, "BRACKETS, NOT BOXES", 8, GREY, ls=1.0))

    # tick rail
    s.append(f'<line x1="420" y1="{dy+32}" x2="560" y2="{dy+32}" stroke="{GREY}" stroke-width="1"/>')
    for i in range(8):
        tx = 420 + i * 20
        th = 8 if i % 3 == 0 else 4
        s.append(f'<line x1="{tx}" y1="{dy+32}" x2="{tx}" y2="{dy+32-th}" stroke="{GREY}" stroke-width="1"/>')
    s.append(text(420, dy + 48, "TICK RAILS EVERYWHERE", 8, GREY, ls=1.0))

    # dot screen
    s.append(dot_defs("devdots", cell=10.9, r=0.75, col="rgba(255,255,255,0.25)"))
    s.append(f'<rect x="600" y="{dy+8}" width="90" height="28" fill="{PANEL}"/>')
    s.append(dot_rect("devdots", 600, dy + 8, 90, 28))
    s.append(text(600, dy + 52, "DOT SCREEN 1.5/21.75", 8, GREY, ls=1.0))

    # ---- devices, row 2 ----
    d2 = 330
    # plate text with end cells (the CONSCIOUSNESS device)
    s.append(f'<rect x="30" y="{d2}" width="96" height="24" fill="{ACID}"/>')
    s.append(text(78, d2 + 18, "SIGNAL", 15, BLACK, anchor="middle", font=DISP, weight="900"))
    s.append(f'<rect x="130" y="{d2}" width="24" height="24" fill="{WHITE}"/>')
    s.append(f'<rect x="136" y="{d2+6}" width="12" height="12" fill="{BLACK}"/>')
    s.append(f'<rect x="158" y="{d2+9}" width="22" height="6" fill="{WHITE}"/>')
    s.append(text(30, d2 + 42, "PLATE TEXT + END CELLS", 8, GREY, ls=1.0))

    # quarter-turn text along a seam
    s.append(f'<line x1="268" y1="{d2-4}" x2="268" y2="{d2+28}" stroke="{SEAM}" stroke-width="1"/>')
    s.append(f'<text transform="rotate(-90 282 {d2+26})" x="282" y="{d2+26}" '
             f'font-family="{MONO}" font-size="8" fill="{GREY}" letter-spacing="1.5">FILTER 08</text>')
    s.append(text(230, d2 + 42, "QUARTER-TURN TEXT", 8, GREY, ls=1.0))

    # cables: fat catenary droop, flat colour, square plug heads
    s.append(f'<path d="M430 {d2-4} C430 {d2+36}, 545 {d2+36}, 545 {d2-4}" fill="none" '
             f'stroke="{ACID}" stroke-width="5"/>')
    s.append(f'<path d="M455 {d2-4} C455 {d2+26}, 570 {d2+26}, 570 {d2-4}" fill="none" '
             f'stroke="{WHITE}" stroke-width="5" opacity="0.85"/>')
    for px in (430, 545, 455, 570):
        col = ACID if px in (430, 545) else WHITE
        s.append(f'<circle cx="{px}" cy="{d2-4}" r="5" fill="{col}"/>')
    s.append(text(420, d2 + 42, "CABLES: FAT + CURVED, FLAT COLOUR", 8, GREY, ls=1.0))

    # ---- devices, row 3: corner marks in place of a section border ----
    d3 = 424
    s.append(text(30, d3 - 12, "GROUPING: CORNER MARKS REPLACE THE BOX", 9, GREY, ls=1.5))

    def mini_group(gx, gy):
        """Two knobs + a readout — the content both variants enclose."""
        out = [knob(gx + 34, gy + 34, 0.62, "RATE", "0.8 Hz"),
               knob(gx + 104, gy + 34, 0.4, "DEPTH", "40%")]
        return "\n".join(out)

    # LEFT: the box (what we are replacing)
    bx, by, bw, bh = 40, d3 + 8, 152, 76
    s.append(f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" fill="none" '
             f'stroke="{SEAM}" stroke-width="1"/>')
    s.append(mini_group(bx + 4, by + 10))
    s.append(text(bx, by + bh + 18, "BORDERED: 4 EDGES OF NOISE", 8, DIMGREY, ls=1.0))

    # RIGHT: corner marks only, with the group label riding the top edge
    cx0, cy0, cw, ch = 280, d3 + 8, 152, 76
    s.append(corner_marks(cx0, cy0, cw, ch, col=GREY))
    s.append(f'<rect x="{cx0+16}" y="{cy0-5}" width="60" height="10" fill="{BLACK}"/>')
    s.append(text(cx0 + 20, cy0 + 3, "LFO CORE", 8, GREY, ls=1.2))
    s.append(mini_group(cx0 + 4, cy0 + 10))
    s.append(text(cx0, cy0 + ch + 18, "MARKED: 4 CORNERS, LABEL IN THE GAP", 8, GREY, ls=1.0))

    # FAR RIGHT: the heavier registration-square variant, and active state
    s.append(reg_square(540, d3 + 26, ACID))
    s.append(reg_square(600, d3 + 66, ACID))
    s.append(text(510, d3 + 102, "REGISTRATION SQUARES:", 8, GREY, ls=1.0))
    s.append(text(510, d3 + 114, "HEAVIER, FOR ACTIVE ZONES", 8, GREY, ls=1.0))

    s.append("</svg>")
    return "\n".join(s)


os.makedirs(OUT, exist_ok=True)
for name, gen in [("ui-language-rack.svg", gen_rack),
                  ("ui-language-widgets.svg", gen_widgets),
                  ("ui-language-devices.svg", gen_devices),
                  ("ui-language-card.svg", gen_card)]:
    path = os.path.join(OUT, name)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(gen())
    print("wrote", path)
