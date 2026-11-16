#!/usr/bin/env python3
"""BACKLOG E19 -- the STANDALONE format target, measured with no DAW.

E19's Accept names three format targets for `win`: **VST3 + GMPI + STANDALONE**.
VST3 passed on 2026-11-02 in REAPER. This is the standalone leg, and until
2026-11-16 the Windows box had no way to run it at all: every standalone probe
opened an `AF_UNIX` socket, and this platform's server publishes a NAMED PIPE
(`tests/standalone_channel.py` is the port).

WHAT IS MEASURED, and the controls that make each clause mean something:

  1  the channel      the app publishes a channel and answers `--info` at the
                      1100x626 the rack coordinates assume
  2  place a module   a `--double` click in the browser places a WT LFO and the
                      DSP constructs it -- read from RACK_ADAPTOR_TRACE, so it
                      is the PROCESSOR saying so, not the editor
  3  animation        E19's own words: both counters still advancing at the end
                      of the window. Measured by line position -- `light update`
                      and `render` lines that FOLLOW the last `building rack`.
                      The in-frame control is that the counters' values differ,
                      so a stuck-but-printing editor would not pass
  4  a widget drag    POSITIVE CONTROL for the two clauses below: the FREQ knob
                      must visibly rotate under `--drag`, compared only inside
                      the knob's own box so the animating display cannot answer
                      for it
  5  move a module    drag the panel by its title strip -- the rack must move it
  6  cable two jacks  drag the WT output to the Out module's L input, swept over
                      9 points +/- 4 DIP so a hit-test near-miss is excluded

5 and 6 FAIL on this platform as of 2026-11-16, and 4 is why that is a finding
rather than a broken probe: pointer events reach the editor, a widget inside the
module panel obeys them, and the same gesture aimed at the rack does nothing at
all -- no document change, no movement, at any of the 9 points. Both pass on
macOS, where `tests/e82_wtlfo_wave_points_probe.py` cables these exact jacks.

GEOMETRY IS DERIVED, NOT ASSUMED (`tests/rack_geometry.py`). The window is
1100x626 everywhere and the channel takes DIPs, so DPI is already divided out --
but the RACK's zoom is not the same: 0.713 here against mac's 0.745, which moves
a jack about 11 DIP, and a jack is 7 DIP across. Hard-coded coordinates miss.

Exit 0 every clause passed - 1 a clause or a control failed - 2 setup.
"""

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import rack_geometry as geo
import standalone_channel as chan

BROWSER_LFO2 = "130,294"
SCROLL_AT = "700,400"
WINDOW = (1100, 626)
CONSTRUCTED = re.compile(r"RackProcessor: 'LFO2' constructed")
LIGHT = re.compile(r"RackEditor: light (\d+) update #(\d+) value ([0-9.]+)")
RENDER = re.compile(r"RackEditor: render #(\d+)")
BUILDING = "building rack"
CHANGED = "document changed"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--standalone", required=True,
                    help="the TIDE-Rack executable, built with -DRACK_ADAPTOR_TRACE=1")
    ap.add_argument("--animate", type=float, default=60.0,
                    help="the animation window in seconds (E19 measured the freeze horizon at 60)")
    ap.add_argument("--timeout", type=float, default=60.0)
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()

    binary = pathlib.Path(args.standalone)
    if not binary.is_file():
        print(f"SETUP FAILED -- not a file: {binary}")
        return 2

    ipc = chan.tempdir("e19win-")
    cfg = chan.tempdir("e19wincfg-")
    ok, detail = chan.path_is_usable(ipc)
    if not ok:
        print(f"SETUP FAILED -- {detail}")
        return 2
    stderr_path = cfg / "stderr.log"
    proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                            stderr=open(stderr_path, "wb"),
                            env=chan.launch_env(ipc, cfg))
    failures = []

    def check(ok, name, detail):
        print(f"  {'PASS' if ok else 'FAIL'}  {name}\n        {detail}")
        if not ok:
            failures.append(name)

    def mark():
        with open(stderr_path, "rb") as f:
            f.seek(0, 2)
            return f.tell()

    def since(pos):
        with open(stderr_path, "rb") as f:
            f.seek(pos)
            return f.read().decode("utf-8", "replace")

    def saw(pos, needle, seconds):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            if needle in since(pos):
                return True
            time.sleep(0.3)
        return False

    try:
        print("E19 -- the STANDALONE format target on this platform\n")

        # 1 ---------------------------------------------------------------
        try:
            addr = chan.wait_for_channel(ipc, proc.pid, timeout=args.timeout,
                                         is_alive=lambda: proc.poll() is None)
        except chan.ChannelError as exc:
            print(f"SETUP FAILED -- {exc}")
            return 2
        time.sleep(3.0)
        info = next((x for x in chan.talk(addr, ["--info"]) if x.get("cmd") == "info"), {})
        size = (info.get("windowWidth"), info.get("windowHeight"))
        check(size == WINDOW, "1 the app publishes a command channel and answers --info",
              f"{addr}  window {size[0]}x{size[1]}")
        if size != WINDOW:
            return 2

        # 2 ---------------------------------------------------------------
        p = mark()
        chan.talk(addr, [f"--pointer-down {BROWSER_LFO2} --double", f"--pointer-up {BROWSER_LFO2}"])
        built = saw(p, "RackProcessor: 'LFO2' constructed", 12)
        check(built, "2 a --double click in the browser places a WT LFO the DSP constructs",
              "RackProcessor: 'LFO2' constructed" if built else "no construction line in 12s")
        if not built:
            return 1
        chan.talk(addr, [f"--scroll {SCROLL_AT} --notches -3"])
        time.sleep(1.2)
        shot = cfg / "placed.png"
        chan.talk(addr, [f"--screenshot {shot}"])
        time.sleep(1.0)

        # 3 ---------------------------------------------------------------
        p = mark()
        print(f"  .. letting it animate for {args.animate:g}s")
        time.sleep(args.animate)
        text = since(p).splitlines()
        last_build = max((i for i, l in enumerate(text) if BUILDING in l), default=-1)
        after = text[last_build + 1:]
        lights = [LIGHT.search(l) for l in after]
        lights = [m for m in lights if m]
        renders = [m for m in after if RENDER.search(m)]
        values = {m.group(3) for m in lights}
        check(len(lights) > 10 and len(renders) > 10 and len(values) > 1,
              f"3 both counters still advancing after {args.animate:g}s",
              f"{len(lights)} light update line(s), {len(renders)} render line(s) after the last "
              f"`{BUILDING}`; {len(values)} distinct light values (a frozen editor would print one)")

        # geometry, derived from the app's own screenshot
        panel, _ = geo.find_panel(shot, window=WINDOW)
        jacks = geo.find_jacks(shot, panel=geo.find_panel(shot, window=WINDOW))
        if len(jacks) < 3:
            print(f"SETUP FAILED -- {len(jacks)} jack(s) found on the panel")
            return 2
        bottom_y = max(j[1] for j in jacks)
        wt = max([j for j in jacks if abs(j[1] - bottom_y) < 3], key=lambda j: j[0])
        pw, ph = panel[2] - panel[0], panel[3] - panel[1]
        freq = (panel[0] + pw * 0.25, panel[1] + ph * 0.44)
        title = ((panel[0] + panel[2]) / 2, panel[1] + 8)
        print(f"\n  geometry: panel {panel[0]:.1f},{panel[1]:.1f} .. {panel[2]:.1f},{panel[3]:.1f} "
              f"(zoom ~{ph / 384:.3f})   WT jack {geo.dip(wt)}   FREQ knob {geo.dip(freq)}\n")

        # 4 -- the positive control -----------------------------------------
        from PIL import Image, ImageChops
        a_path, b_path = cfg / "knob_a.png", cfg / "knob_b.png"
        chan.talk(addr, [f"--screenshot {a_path}"]); time.sleep(0.6)
        chan.talk(addr, [f"--drag {geo.dip(freq)} {geo.dip((freq[0], freq[1] - 60))} --steps 30"])
        time.sleep(1.2)
        chan.talk(addr, [f"--screenshot {b_path}"]); time.sleep(0.6)
        a, b = Image.open(a_path).convert("RGB"), Image.open(b_path).convert("RGB")
        sx, sy = a.size[0] / WINDOW[0], a.size[1] / WINDOW[1]
        box = (int((freq[0] - 14) * sx), int((freq[1] - 14) * sy),
               int((freq[0] + 14) * sx), int((freq[1] + 14) * sy))
        diff = ImageChops.difference(a.crop(box), b.crop(box))
        moved_px = sum(1 for px in diff.getdata() if sum(px) > 30)
        total_px = (box[2] - box[0]) * (box[3] - box[1])
        check(moved_px > 20,
              "4 POSITIVE CONTROL: a --drag on the FREQ knob rotates it",
              f"{moved_px} of {total_px} px changed inside the knob's own box")

        # 5 -- move the module ----------------------------------------------
        p = mark()
        chan.talk(addr, [f"--drag {geo.dip(title)} {geo.dip((title[0] + 90, title[1]))} --steps 30"])
        changed = saw(p, CHANGED, 8)
        time.sleep(1.5)
        after_shot = cfg / "moved.png"
        chan.talk(addr, [f"--screenshot {after_shot}"]); time.sleep(0.8)
        panel2, _ = geo.find_panel(after_shot, window=WINDOW)
        check(changed and abs(panel2[0] - panel[0]) > 10,
              "5 dragging the panel by its title strip MOVES the module",
              f"`{CHANGED}` = {changed}; panel x {panel[0]:.1f} -> {panel2[0]:.1f}")

        # 6 -- cable ----------------------------------------------------------
        out_l = "590,224"
        hits = []
        for dx in (-4, 0, 4):
            for dy in (-4, 0, 4):
                src = (wt[0] + dx, wt[1] + dy)
                p = mark()
                chan.talk(addr, [f"--drag {geo.dip(src)} {out_l} --steps 30"])
                if saw(p, CHANGED, 6):
                    hits.append(geo.dip(src))
        check(bool(hits),
              "6 dragging the WT output jack to the Out module's L input MAKES A CABLE",
              f"{len(hits)} of 9 sweep points (+/-4 DIP) produced an edit"
              + (f": {hits}" if hits else " -- so this is not a hit-test near-miss"))

        chan.talk(addr, [f"--screenshot {cfg / 'end.png'}"])
        time.sleep(0.6)
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
        if args.keep:
            print(f"\nkept: {cfg}")
        else:
            shutil.rmtree(cfg, ignore_errors=True)
        shutil.rmtree(ipc, ignore_errors=True)

    print()
    if failures:
        print(f"FAILED -- {len(failures)}: {', '.join(failures)}")
        return 1
    print("OK -- the standalone target answers, animates, and accepts every rack gesture.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
