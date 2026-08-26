#!/usr/bin/env python3
"""Render the flat Windows/Linux icons from SynthEditSem/TiDE.icon.

The .icon document (Apple Icon Composer format) is the single source of truth
for the app icon: macOS/iOS get it compiled by actool at build time (see
SynthEditSem/CMakeLists.txt, tide_apply_app_icon). Windows and Linux have no
actool, and their CI runners have no rsvg either -- so their renders are
COMMITTED, and this script is how they are (re)generated when the art changes.

Reads:   SynthEditSem/TiDE.icon/icon.json          (background colour)
         SynthEditSem/TiDE.icon/Assets/TiDE-text.svg (text layer, outlined)
Writes:  SynthEditSem/icons/TiDE.svg               flat scalable render
         SynthEditSem/icons/png/tide-<N>.png       16..512
         SynthEditSem/icons/TiDE.ico               multi-size, PNG entries

Needs rsvg-convert on PATH (brew install librsvg). Run from anywhere.

The flat render is a rounded square (corner radius 22.4% of the side, close
to Apple's squircle) so all three platforms show one identity. The .ico uses
PNG-format entries, valid on Windows Vista+ at every size.
"""
import json
import pathlib
import re
import shutil
import struct
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
ICON_DOC = (HERE / ".." / "SynthEditSem" / "TiDE.icon").resolve()
OUT = (HERE / ".." / "SynthEditSem" / "icons").resolve()

PNG_SIZES = [16, 24, 32, 48, 64, 128, 256, 512]
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]   # 512 is wasteful inside an .ico
CORNER_FRACTION = 0.224                       # ~ Apple squircle radius


def bg_hex():
    fill = json.load(open(ICON_DOC / "icon.json"))["fill"]["solid"]
    kind, comps = fill.split(":")
    r, g, b = [round(float(c) * 255) for c in comps.split(",")[:3]]
    return f"#{r:02X}{g:02X}{b:02X}"


def text_layer():
    svg = (ICON_DOC / "Assets" / "TiDE-text.svg").read_text()
    m = re.search(r"(<g\b.*</g>)", svg, re.DOTALL)
    if not m:
        sys.exit("TiDE-text.svg: no <g> layer found")
    return m.group(1)


def main():
    if not shutil.which("rsvg-convert"):
        sys.exit("rsvg-convert not found -- brew install librsvg")

    bg = bg_hex()
    radius = round(1024 * CORNER_FRACTION)
    flat = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024">
<rect width="1024" height="1024" rx="{radius}" fill="{bg}"/>
{text_layer()}
</svg>
"""
    (OUT / "png").mkdir(parents=True, exist_ok=True)
    (OUT / "TiDE.svg").write_text(flat)
    print(f"TiDE.svg  (bg {bg}, radius {radius})")

    for n in PNG_SIZES:
        png = OUT / "png" / f"tide-{n}.png"
        subprocess.run(["rsvg-convert", "-w", str(n), "-h", str(n),
                        "-o", str(png), str(OUT / "TiDE.svg")], check=True)
        print(f"tide-{n}.png")

    # .ico: ICONDIR + one ICONDIRENTRY per size + raw PNG blobs.
    blobs = [(n, (OUT / "png" / f"tide-{n}.png").read_bytes()) for n in ICO_SIZES]
    ico = struct.pack("<HHH", 0, 1, len(blobs))
    offset = 6 + 16 * len(blobs)
    for n, data in blobs:
        wh = 0 if n >= 256 else n            # 0 means 256 in ICO
        ico += struct.pack("<BBBBHHII", wh, wh, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    for _, data in blobs:
        ico += data
    (OUT / "TiDE.ico").write_bytes(ico)
    print(f"TiDE.ico  ({len(blobs)} sizes, {len(ico)} bytes)")


if __name__ == "__main__":
    main()
