#!/usr/bin/env python3
"""BACKLOG E82 -- a rack module's own context-menu option reaches the DSP.

E82's Accept: a right-click on a VCV panel offers at least one option the
module owns, and toggling it changes DSP behaviour. This probe measures both
halves on the standalone, with no screen, no host and no DAW, using the
command channel's --context-menu verb (E38) as the menu readout and the rack
adaptor's RACK_ADAPTOR_TRACE lines as the DSP readout.

THE MODULE IS WT LFO (VCV Fundamental's `LFO2`), whose `Wave points` submenu
is an IndexPtr option with ten labels, 32 .. 16384, default 1024. Picking a
label calls `Wavetable::setWaveLen` in the processing instance, which re-slices
the same 4096 stored samples into waves of the new length -- so the LFO plays
a 32-sample sliver of its sine instead of a full cycle, and the first output
sample a fresh instance produces is a different number.

THE DSP READOUT IS A FRESH INSTANCE, BY DESIGN. The adaptor prints
`first NONZERO OUTPUT pin 0 (<value>)` once per RackProcessor instance, and
a menu pick does not rebuild the rack -- it lands in the live instance through
onSetPins, which traces nothing. A cable edit DOES rebuild, and the new
instance receives the option value with its other pins. So each arm picks a
label, then adds or removes a cable to force a rebuild, and reads the value the
new instance prints. A/B/A/B: the value must alternate with the pick and repeat
within it. Measured 2026-10-25 (macos): 0.500143 at 1024, 0.500004 at 32, four
rebuilds, no exceptions.

WHY NOT --render-audio: TIDE's whole patch is one blob parameter and the render
verb primes scalars only (its own comment, BACKLOG E6), so it renders an empty
graph -- peak 0 whatever the rack holds. Measured here before switching readouts.

GEOMETRY. The coordinates below are window DIPs at the standalone's default
1100x626 window on the shipped default rack (zoom 0.745), which is what a
launch with an empty GMPI_STANDALONE_CONFIG_DIR gives. The probe checks --info
reports that size and exits 2 otherwise rather than clicking blind.

  browser item "LFO2"      130,294   double-click places it (a channel --drag
                                     from the browser only selects the row)
  scroll point             700,400   three notches down brings its jacks on screen
  WT LFO body              565,352   select + context-menu point, after the scroll
  WT LFO `WT` output jack  590,540
  Out module L jack        592,220   (after the scroll)
  Out module R jack        592,250
  empty rack               700,330   dropping a cable here removes it

Controls, each of which must hold or the probe fails:
  * before the module is selected, the same point returns the rack's own menu
    (the E38 fact: the menu is built for the SELECTION);
  * the background returns the rack's own menu, with no `Wave points`;
  * an unknown label is an error naming the offer list;
  * the A/B/A/B must both DIFFER across picks and REPEAT within a pick -- a
    value that drifts between two rebuilds of the same pick would make the
    difference meaningless, and a value that never changes is the row's defect.

Exit codes: 0 pass - 1 a clause or a control failed - 2 setup.
"""

import argparse, glob, json, os, pathlib, re, shutil, socket, subprocess, sys, tempfile, time

SUN_PATH_MAX = 103
BROWSER_LFO2 = "130,294"
SCROLL_AT = "700,400"
WT_BODY = "565,352"
WT_JACK = "590,540"
OUT_L = "592,220"
OUT_R = "592,250"
EMPTY = "700,330"
OPTION = "Wave points"
LABELS = ["32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"]
TRACE_OUT = re.compile(r"RackProcessor: 'LFO2' first NONZERO OUTPUT pin \d+ \(([-0-9.]+)\)")


def talk(sock_path, verbs, timeout=30):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout); s.connect(sock_path)
    s.sendall("".join(v + "\n" for v in verbs).encode()); s.shutdown(socket.SHUT_WR)
    buf = b""
    try:
        while True:
            c = s.recv(65536)
            if not c: break
            buf += c
    except socket.timeout:
        pass
    s.close()
    out = []
    for line in buf.decode(errors="replace").splitlines():
        line = line.strip()
        if line.startswith("{"):
            try: out.append(json.loads(line))
            except json.JSONDecodeError: pass
    return out


class Trace:
    """The app's stderr, read incrementally: RACK_ADAPTOR_TRACE is the DSP readout."""
    def __init__(self, path):
        self.path = path; self.pos = 0
    def new_lines(self):
        with open(self.path, "rb") as f:
            f.seek(self.pos); data = f.read(); self.pos = f.tell()
        return data.decode("utf-8", "replace").splitlines()
    def wait_for(self, pattern, seconds):
        deadline = time.monotonic() + seconds; seen = []
        while time.monotonic() < deadline:
            for l in self.new_lines():
                seen.append(l)
                m = pattern.search(l)
                if m: return m, seen
            time.sleep(0.25)
        return None, seen


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--standalone", required=True, help="the TIDE-Rack executable, built with -DRACK_ADAPTOR_TRACE=1")
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("--settle", type=float, default=2.5, help="seconds to allow a rebuild after a cable edit")
    ap.add_argument("--keep", action="store_true", help="keep the temp dirs and screenshots")
    args = ap.parse_args()
    binary = pathlib.Path(args.standalone)
    if not binary.is_file() or not os.access(binary, os.X_OK):
        print(f"SETUP FAILED -- not executable: {binary}"); return 2

    ipc = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e82-"))
    cfg = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e82cfg-"))
    if len(str(ipc)) + 24 > SUN_PATH_MAX:
        print("SETUP FAILED -- IPC path too long (sun_path)"); return 2
    env = dict(os.environ, GMPI_STANDALONE_IPC_DIR=str(ipc), GMPI_STANDALONE_CONFIG_DIR=str(cfg))
    stderr_path = cfg / "stderr.log"
    proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                            stderr=open(stderr_path, "wb"), env=env)
    trace = Trace(stderr_path)
    failures = []

    def check(ok, name, detail):
        print(f"  {'PASS' if ok else 'FAIL'}  {name}\n        {detail}")
        if not ok: failures.append(name)

    try:
        sock = None
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            hits = glob.glob(str(ipc / "gmpi-standalone.*"))
            if hits: sock = hits[0]; break
            if proc.poll() is not None:
                print(f"SETUP FAILED -- app exited rc={proc.returncode}"); return 2
            time.sleep(0.5)
        if not sock:
            print("SETUP FAILED -- no socket"); return 2
        time.sleep(3.0)

        info = next((x for x in talk(sock, ["--info"]) if x.get("cmd") == "info"), {})
        if (info.get("windowWidth"), info.get("windowHeight")) != (1100, 626):
            print(f"SETUP FAILED -- window is {info.get('windowWidth')}x{info.get('windowHeight')}, "
                  "the coordinates here assume 1100x626"); return 2
        def menu(pt, pick=None):
            v = f"--context-menu {pt}" + (f" {pick}" if pick else "")
            r = [x for x in talk(sock, [v]) if x.get("cmd") == "context-menu"]
            return r[0] if r else {}
        def labels(r): return [i.get("label", "") for i in r.get("items", [])]
        def ticked(r): return [i.get("label", "") for i in r.get("items", []) if i.get("ticked")]
        def click(pt, double=False):
            talk(sock, [f"--pointer-down {pt}" + (" --double" if double else ""), f"--pointer-up {pt}"])
            time.sleep(0.8)
        def rebuild(drag):
            trace.new_lines()  # discard what came before the gesture
            talk(sock, [f"--drag {drag} --steps 30"])
            m, seen = trace.wait_for(TRACE_OUT, args.settle + 8.0)
            constructed = any("RackProcessor: 'LFO2' constructed" in l for l in seen)
            return (m.group(1) if m else None), constructed

        print("E82 -- a VCV module's own context-menu option, observed and driven, reaching the DSP\n")

        # 1. place the module from the browser, and prove the DSP built it
        trace.new_lines()
        click(BROWSER_LFO2, double=True)
        m, seen = trace.wait_for(re.compile(r"RackProcessor: 'LFO2' constructed"), 10.0)
        check(m is not None, "1a double-clicking LFO2 in the browser places a WT LFO the DSP constructs",
              "RackProcessor: 'LFO2' constructed" if m else f"no construction line in {len(seen)} lines")
        if m is None:
            return 1
        talk(sock, [f"--scroll {SCROLL_AT} --notches -3"]); time.sleep(1.0)
        talk(sock, [f"--screenshot {cfg}/placed.png"])

        # 2. cable WT -> Out L; the rebuild gives the baseline value at the default 1024
        v0, built = rebuild(f"{WT_JACK} {OUT_L}")
        check(built and v0 is not None, "1b cabling the WT jack to Out L rebuilds the rack and the new LFO2 outputs",
              f"first NONZERO OUTPUT = {v0}" if v0 else "no rebuild / no output line")
        if v0 is None:
            return 1
        talk(sock, [f"--screenshot {cfg}/cabled.png"])

        # 3. menu readouts, with the E38 selection fact as the first control
        click(EMPTY)                      # deselect: the module is selected by its own placement
        r = menu(WT_BODY)
        check(OPTION not in labels(r), "2a control: over the module but NOT selected -> the rack's own menu, no Wave points",
              f"count={r.get('count')} items={labels(r)}")
        click(WT_BODY)
        r = menu(WT_BODY)
        ls = labels(r)
        have = [l for l in LABELS if l in ls]
        begins = [i for i in r.get("items", []) if i.get("subMenuBegin") and i.get("label") == OPTION]
        check(bool(begins) and have == LABELS,
              "2b selected -> the module's own `Wave points` submenu with all ten labels",
              f"count={r.get('count')} submenu={'yes' if begins else 'NO'} labels={have}")
        check(ticked(r) == ["1024"], "2c the default label is ticked", f"ticked={ticked(r)}")
        # The menu follows the SELECTION, not the point: asked over the background
        # while the module is still selected, it still carries Wave points
        # (measured 2026-10-25 -- the first version of this control failed on
        # exactly that). So deselect first, then the background must be bare.
        r_sel = menu(EMPTY)
        click(EMPTY)
        r = menu(EMPTY)
        check(OPTION in labels(r_sel) and OPTION not in labels(r),
              "2d control: the background offers Wave points only while the module is selected",
              f"selected: count={r_sel.get('count')}   deselected: count={r.get('count')} items={labels(r)}")
        click(WT_BODY)
        r = menu(WT_BODY, "NoSuchLabelXYZ")
        check(r.get("ok") is False and "menu offers" in json.dumps(r),
              "2e control: an unknown label is an error naming the offer", json.dumps(r)[:140])

        # 4. A/B/A/B on the DSP: pick, force a rebuild, read the fresh instance's first output
        arms = [("B", "32", f"{OUT_R} {WT_JACK}"),    # add a second cable R -> WT: rebuild
                ("A", "1024", f"{OUT_R} {EMPTY}"),    # drag the R cable into space: removed, rebuild
                ("B", "32", f"{OUT_R} {WT_JACK}"),
                ("A", "1024", f"{OUT_R} {EMPTY}")]
        values = {"A": [v0], "B": []}
        for arm, pick, drag in arms:
            click(WT_BODY)
            r = menu(WT_BODY, pick)
            ok = r.get("ok") is True and r.get("invoked") == pick
            time.sleep(0.8)
            now = ticked(menu(WT_BODY))
            check(ok and now == [pick], f"3 pick {pick}: invoked and ticked", f"invoked={r.get('invoked')} ticked={now}")
            v, built = rebuild(drag)
            check(built and v is not None, f"3 pick {pick}: a cable edit rebuilds and the new LFO2 outputs",
                  f"first NONZERO OUTPUT = {v}")
            if v is not None:
                values[arm].append(v)
        a, b = values["A"], values["B"]
        check(len(set(a)) == 1 and len(a) >= 2, "4a the 1024 value REPEATS across rebuilds (A, A, A)", f"A={a}")
        check(len(set(b)) == 1 and len(b) >= 2, "4b the 32 value REPEATS across rebuilds (B, B)", f"B={b}")
        check(bool(a) and bool(b) and a[0] != b[0],
              "4c the two picks give DIFFERENT first output samples -- the DSP obeyed the menu",
              f"1024 -> {a[0] if a else None}   32 -> {b[0] if b else None}")
        talk(sock, [f"--screenshot {cfg}/end.png"])

    finally:
        if proc.poll() is None:
            proc.terminate()
            try: proc.wait(timeout=10)
            except subprocess.TimeoutExpired: proc.kill()
        if args.keep:
            print(f"\nkept: {cfg} (stderr.log, placed.png, cabled.png, end.png)")
        else:
            shutil.rmtree(cfg, ignore_errors=True)
        shutil.rmtree(ipc, ignore_errors=True)

    print()
    if failures:
        print(f"FAILED -- {len(failures)}: {', '.join(failures)}"); return 1
    print("OK -- the module's own option is offered, the pick lands in the DSP, and the DSP's output changes with it.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
