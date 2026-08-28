#!/usr/bin/env python3
"""BACKLOG E38 -- the editor's context menu, observed and driven headlessly.

WHAT THIS GUARDS. The --context-menu verb (GMPI_Wrappers), and with it the
whole V7 ruling, on-screen, which shipped 2026-08-27 with its menu unverified
because nothing could raise one headlessly:

  rule 1 (rack background): no "Arrange", "Skin", "Locked", "Goto Structure...";
  rule 2 (module, selected): "Show Circuit" PRESENT, "Delete (keep wires)" absent;
  rule 3 (structure view):   no "Arrange", "Screenshot"; and in Release no
                             "Panel Edit...", "Goto Parent...".

THE READOUT IS THE MENU MODEL, NOT PIXELS -- E38's own history is why. A
--right pointer flag was measured raising nothing (the menu belongs to the
frame / the Cocoa view, which the channel bypasses), and --screenshot could
never show a native popup (separate window, not in the app's render buffer).
The verb runs the same population path the frame runs
(inputClient->populateContextMenu) into a recording sink, and invokes a chosen
item exactly as the native menu would (IPopupMenuCallback::onComplete).

TWO BEHAVIOURAL FACTS THE PROBE DEPENDS ON, both measured 2026-08-28:
  * the menu is built for the current SELECTION, so the module must be clicked
    before rule 2 is asked -- same two steps a hand performs;
  * invoking "Show Circuit" swaps the canvas to the structure view, which this
    proves by pixels: >100k changed AND the canvas centre turns light (the
    rack case is near-black, the structure canvas is not).

Exit codes: 0 pass - 1 the verb or the V7 filtering regressed - 2 setup.
"""

import argparse, glob, json, os, pathlib, shutil, socket, subprocess, sys, tempfile, time

RULE1_FORBIDDEN = ["Arrange", "Skin", "Locked", "Goto Structure"]
RULE1_EXPECTED  = ["Goto Rack", "About TIDE"]
RULE2_REQUIRED  = ["Show Circuit"]
RULE2_FORBIDDEN = ["Delete (keep wires)"]
RULE3_FORBIDDEN = ["Arrange", "Screenshot", "Panel Edit", "Goto Parent"]
SUN_PATH_MAX = 103
MODULE_DIP = "592,130"     # the seeded Out module; e62's probe clicks the same point
BACKGROUND_DIP = "800,300"


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


def load_png(path):
    try:
        from PIL import Image
        im = Image.open(path).convert("RGB"); px = im.load()
        return im.size[0], im.size[1], lambda x, y: px[x, y]
    except ImportError:
        raise SystemExit("this probe needs Pillow OR the stdlib decoder from "
                         "e62_delete_key_behaviour_probe -- import it")


def labels(reply):
    return [i.get("label", "") for i in reply.get("items", [])]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--standalone", required=True)
    ap.add_argument("--timeout", type=float, default=60)
    args = ap.parse_args()
    binary = pathlib.Path(args.standalone)
    if not binary.is_file() or not os.access(binary, os.X_OK):
        print(f"SETUP FAILED -- not executable: {binary}"); return 2

    ipc = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e38-"))
    cfg = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e38cfg-"))
    if len(str(ipc)) + 24 > SUN_PATH_MAX:
        print("SETUP FAILED -- IPC path too long (sun_path)"); return 2
    env = dict(os.environ, GMPI_STANDALONE_IPC_DIR=str(ipc), GMPI_STANDALONE_CONFIG_DIR=str(cfg))
    proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, env=env)
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
        time.sleep(2.0)

        print("E38 -- context menu observed and driven headlessly\n")

        # rule 1: rack background
        r = next((x for x in talk(sock, [f"--context-menu {BACKGROUND_DIP}"])
                  if x.get("cmd") == "context-menu"), {})
        ls = labels(r)
        check(r.get("ok") is True and r.get("count", 0) >= 2, "1a background menu lists",
              f"count={r.get('count')} items={ls}")
        bad = [f for f in RULE1_FORBIDDEN if any(f in l for l in ls)]
        check(not bad, "1b V7 rule 1: forbidden items absent",
              "none of Arrange/Skin/Locked/Goto Structure" if not bad else f"present: {bad}")
        missing = [e for e in RULE1_EXPECTED if not any(e in l for l in ls)]
        check(not missing, "1c TIDE's own items present",
              "Goto Rack + About TIDE" if not missing else f"missing: {missing}")

        # rule 2: module, selected first (the measured behavioural fact)
        talk(sock, [f"--pointer-down {MODULE_DIP}", f"--pointer-up {MODULE_DIP}"])
        time.sleep(0.8)
        r = next((x for x in talk(sock, [f"--context-menu {MODULE_DIP}"])
                  if x.get("cmd") == "context-menu"), {})
        ls = labels(r)
        missing = [e for e in RULE2_REQUIRED if not any(e in l for l in ls)]
        check(not missing, "2a V7 rule 2: Show Circuit present (module selected)",
              f"items={ls}" if missing else "present")
        bad = [f for f in RULE2_FORBIDDEN if any(f in l for l in ls)]
        check(not bad, "2b V7 rule 2: Delete (keep wires) absent",
              "absent" if not bad else f"present: {bad}")

        # control: a label that is not there must error with the offer list
        r = next((x for x in talk(sock, [f"--context-menu {MODULE_DIP} NoSuchItemXYZ"])
                  if x.get("cmd") == "context-menu"), {})
        check(r.get("ok") is False and "menu offers" in json.dumps(r),
              "2c control: unknown label is an error naming the offer",
              json.dumps(r)[:120])

        # rule 3 + invocation: Show Circuit -> structure view
        shot_a = str(cfg / "a.png"); shot_b = str(cfg / "b.png")
        talk(sock, [f"--screenshot {shot_a}"])
        r = next((x for x in talk(sock, [f"--context-menu {MODULE_DIP} Show Circuit"])
                  if x.get("cmd") == "context-menu"), {})
        check(r.get("ok") is True and r.get("invoked") == "Show Circuit",
              "3a Show Circuit invoked", json.dumps(r)[:100])
        time.sleep(1.5)
        talk(sock, [f"--screenshot {shot_b}"])
        wa, ha, pa = load_png(shot_a); wb, hb, pb = load_png(shot_b)
        changed = sum(1 for y in range(0, min(ha, hb), 4) for x in range(0, min(wa, wb), 4)
                      if pa(x, y) != pb(x, y))
        light = sum(1 for y in range(int(hb*0.25), int(hb*0.75), 10)
                    for x in range(int(wb*0.3), int(wb*0.65), 10)
                    if sum(pb(x, y)) > 380)
        check(changed > 6000 and light > 500, "3b the view swapped to the structure canvas",
              f"~{changed*16} px changed, {light} light samples in the canvas centre")

        r = next((x for x in talk(sock, [f"--context-menu {BACKGROUND_DIP}"])
                  if x.get("cmd") == "context-menu"), {})
        ls = labels(r)
        bad = [f for f in RULE3_FORBIDDEN if any(f in l for l in ls)]
        check(not bad, "3c V7 rule 3: structure menu clean (Release)",
              f"items={ls}" if bad else "no Arrange/Screenshot/Panel Edit/Goto Parent")

    finally:
        if proc.poll() is None:
            proc.terminate()
            try: proc.wait(timeout=10)
            except subprocess.TimeoutExpired: proc.kill()
        shutil.rmtree(ipc, ignore_errors=True)
        shutil.rmtree(cfg, ignore_errors=True)

    print()
    if failures:
        print(f"FAILED -- {len(failures)}: {', '.join(failures)}"); return 1
    print("OK -- the verb works and every V7 rule holds on the real menus.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
