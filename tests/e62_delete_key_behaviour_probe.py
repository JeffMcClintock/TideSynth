#!/usr/bin/env python3
"""BACKLOG E62 -- the delete key actually deletes, driven end-to-end.

WHAT THIS IS FOR. E57's Accept asked for two things and only one shipped:

    "--type \\x7f removes it from the document -- shown by the document byte
     size changing and the module absent from a --screenshot -- AND that check
     runs unattended in CI"

`e57_delete_key_probe.py` is the STRUCTURAL half: it proves the binding exists
and is unconditional. It never presses a key, so it would pass on a build where
`DeleteSelection()` is wired correctly and declines, or deletes the wrong
object. THIS probe is the behavioural half. Both are worth having: the
structural one runs in seconds on every PR with no build, this one needs a
built app and answers a question the other cannot.

HOW IT WORKS. It launches the standalone, finds a module on the rack by looking
at the pixels, clicks it, presses Delete over the command channel, and requires
BOTH pieces of evidence E57 named:

  * the document SHRINKS -- the app prints "pushing <N> byte document" to stderr
    whenever the DSP structure changes, so this is a number, not an impression.
    Measured 2026-08-28 on a fixed build: 17960 -> 14613.
  * the module's PIXELS go -- its panel column is no longer light against the
    dark case.

WHY IT ASSERTS THE SELECTION FIRST, AND WHY THAT IS THE IMPORTANT PART. If the
click misses, nothing is selected, Delete correctly does nothing, and a naive
probe reports "the delete key is broken". That failure mode is worse than no
probe: it is a false alarm that looks exactly like the real bug. So a click that
does not visibly change the screen is a SETUP failure (exit 2), never a verdict
about the key.

THE TRAPS, ALL THREE MEASURED RATHER THAN GUESSED (2026-08-28, macOS):

 1. THE SOCKET PATH HAS A 103-BYTE LIMIT. A unix socket's sun_path is 104 bytes
    including the terminator. The first attempt here put the IPC directory under
    a session scratchpad whose path was 150 bytes, and the app declined to open
    a channel at all -- it says so plainly on stderr, but a caller that only
    watched for the socket file would just time out. The default (--ipc-dir
    unset) is a short /tmp name, and an over-long one is rejected up front with
    the arithmetic, not left to fail as a timeout.

 2. IT MUST NOT TOUCH THE DEVELOPER'S SESSION. The standalone always restores
    the last session, so a probe that deleted a module from the real one would
    hand the user a damaged rack next launch. GMPI_STANDALONE_CONFIG_DIR is
    pointed at a temp directory, which also makes the run deterministic: a fresh
    config gives the default rack every time.

 3. `--type` IS NOT `--key`, AND ONLY ONE OF THEM REACHES THIS CODE. The
    EditorScreenshot dispatcher that SynthEditCL uses requires an active key
    LISTENER -- the opt-in text-widget path -- and refuses when none is open.
    That path never reaches ViewBase::onKey, which is where E57's binding lives.
    The STANDALONE's --type calls `client->onKeyPress(c)` directly, the same
    entry the real macOS keyDown: uses. So this probe drives the standalone; it
    cannot be rewritten against SynthEditCL without testing nothing.

USAGE

    python3 tests/e62_delete_key_behaviour_probe.py --standalone <binary>
    python3 tests/e62_delete_key_behaviour_probe.py --standalone <bin> --expect-fail

`--expect-fail` inverts the verdict, for demonstrating the control: run it
against a build with the binding removed and it should SUCCEED at finding the
key dead. A guard nobody has watched fail is not a guard.

Exit codes: 0 verdict as expected - 1 the delete key did not work - 2 the probe
could not set up (no socket, no module found, click did not register).
"""

import argparse
import glob
import json
import os
import pathlib
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

DEL_KEY = r'\x7f'          # forward delete; 0x08 and 0x2E are the other two
SUN_PATH_MAX = 103         # sizeof(sockaddr_un.sun_path) - 1


# --- talking to the app ---------------------------------------------------

class Channel:
    def __init__(self, path):
        self.path = path

    def send(self, *verbs, timeout=30):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect(self.path)
        s.sendall("".join(v + "\n" for v in verbs).encode())
        s.shutdown(socket.SHUT_WR)
        buf = b""
        try:
            while True:
                chunk = s.recv(65536)
                if not chunk:
                    break
                buf += chunk
        except socket.timeout:
            pass
        s.close()
        out = []
        for line in buf.decode(errors="replace").splitlines():
            line = line.strip()
            if line.startswith("{"):
                try:
                    out.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
        return out


# --- looking at the screen ------------------------------------------------

def load_png(path):
    """Read a PNG into (width, height, getpixel). Pillow if present, else pure
    stdlib -- CI should not need an install step to run a gate."""
    try:
        from PIL import Image
        im = Image.open(path).convert("RGB")
        px = im.load()
        return im.size[0], im.size[1], lambda x, y: px[x, y]
    except ImportError:
        pass
    import struct, zlib
    data = pathlib.Path(path).read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"not a PNG: {path}")
    pos, idat, w = 8, b"", None
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]
        typ = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + ln]
        if typ == b"IHDR":
            w, h, depth, colour = struct.unpack(">IIBB", body[:10])
            if depth != 8 or colour not in (2, 6):
                raise SystemExit("probe expects an 8-bit RGB/RGBA PNG")
            nch = 3 if colour == 2 else 4
        elif typ == b"IDAT":
            idat += body
        elif typ == b"IEND":
            break
        pos += 12 + ln
    raw = zlib.decompress(idat)
    stride = w * nch
    rows, prev, p = [], bytearray(stride), 0
    for _ in range(h):
        f = raw[p]; p += 1
        line = bytearray(raw[p:p + stride]); p += stride
        for i in range(stride):
            a = line[i - nch] if i >= nch else 0
            b = prev[i]
            c = prev[i - nch] if i >= nch else 0
            if f == 1:   line[i] = (line[i] + a) & 0xFF
            elif f == 2: line[i] = (line[i] + b) & 0xFF
            elif f == 3: line[i] = (line[i] + (a + b) // 2) & 0xFF
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        rows.append(bytes(line)); prev = line
    return w, h, lambda x, y: tuple(rows[y][x * nch:x * nch + 3])


def pixels_differing(a, b):
    wa, ha, pa = load_png(a)
    wb, hb, pb = load_png(b)
    if (wa, ha) != (wb, hb):
        return wa * ha
    return sum(1 for y in range(ha) for x in range(wa) if pa(x, y) != pb(x, y))


def find_module(png, canvas_x0=520):
    """Locate a module panel: a run of columns that is LIGHT where the empty
    case is near-black. Returns (x0, x1, y0, y1) in image pixels, or None.

    canvas_x0 skips the module browser down the left, whose panels are also
    light and are not on the rack."""
    w, h, px = load_png(png)

    def light(c):
        r, g, b = c
        return r > 110 and g > 110 and b > 110

    band = range(int(h * 0.18), int(h * 0.55))
    cols = [x for x in range(canvas_x0, w)
            if sum(1 for y in band if light(px(x, y))) > len(band) * 0.5]
    if not cols:
        return None
    runs = []
    for x in cols:
        if runs and x == runs[-1][1] + 1:
            runs[-1][1] = x
        else:
            runs.append([x, x])
    runs = [r for r in runs if r[1] - r[0] >= 8]
    if not runs:
        return None
    x0, x1 = max(runs, key=lambda r: r[1] - r[0])
    mid = (x0 + x1) // 2
    ys = [y for y in range(h) if light(px(mid, y))]
    return x0, x1, min(ys), max(ys)


def column_is_light(png, x0, x1, y0, y1):
    w, h, px = load_png(png)
    mid = (x0 + x1) // 2
    band = range(max(0, y0), min(h, y1))
    if not band:
        return False
    lit = sum(1 for y in band
              if px(mid, y)[0] > 110 and px(mid, y)[1] > 110 and px(mid, y)[2] > 110)
    return lit > len(band) * 0.5


# --- the run --------------------------------------------------------------

DOC_RX = re.compile(r"pushing (\d+) byte document")


def doc_sizes(stderr_path):
    try:
        return [int(m) for m in DOC_RX.findall(pathlib.Path(stderr_path).read_text(errors="replace"))]
    except OSError:
        return []


def setup_fail(msg):
    print(f"\nSETUP FAILED -- {msg}")
    print("This is NOT a verdict about the delete key. Exit 2 so a red build "
          "means 'the probe could not run', which is a different thing to fix.")
    return 2


def run(args):
    binary = pathlib.Path(args.standalone)
    if not binary.is_file() or not os.access(binary, os.X_OK):
        return setup_fail(f"not an executable: {binary}")

    ipc = pathlib.Path(args.ipc_dir) if args.ipc_dir else pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e62-"))
    # Trap 1: sun_path. The socket leaf is "gmpi-standalone.<pid>", ~24 bytes.
    projected = len(str(ipc)) + len("/gmpi-standalone.") + 7
    if projected > SUN_PATH_MAX:
        return setup_fail(
            f"IPC directory path is too long: '{ipc}' projects to ~{projected} bytes, "
            f"over the {SUN_PATH_MAX}-byte sun_path limit. The app will refuse to open a "
            f"channel and this would otherwise look like a timeout. Pass a shorter "
            f"--ipc-dir under /tmp.")
    ipc.mkdir(parents=True, exist_ok=True)

    # Trap 2: never touch the developer's saved session.
    cfg = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e62cfg-"))
    workdir = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e62run-"))
    stderr_path = workdir / "stderr.log"

    env = dict(os.environ)
    env["GMPI_STANDALONE_IPC_DIR"] = str(ipc)
    env["GMPI_STANDALONE_CONFIG_DIR"] = str(cfg)

    print(f"E62 -- the delete key must actually delete")
    print(f"  standalone {binary}")
    print(f"  ipc        {ipc}")
    print(f"  config     {cfg}  (isolated: the real session is untouched)\n")

    proc = None
    try:
        with open(stderr_path, "wb") as errf:
            proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                                    stderr=errf, env=env)
            sock_path = None
            deadline = time.monotonic() + args.timeout
            while time.monotonic() < deadline:
                hits = glob.glob(str(ipc / "gmpi-standalone.*"))
                if hits:
                    sock_path = hits[0]
                    break
                if proc.poll() is not None:
                    return setup_fail(f"the app exited early (rc={proc.returncode}); "
                                      f"stderr:\n{stderr_path.read_text(errors='replace')[-1500:]}")
                time.sleep(0.5)
            if not sock_path:
                tail = stderr_path.read_text(errors="replace")
                hint = ""
                for line in tail.splitlines():
                    if "command channel" in line.lower():
                        hint = f"\nThe app said: {line.strip()}"
                return setup_fail(f"no command socket appeared in {ipc} within "
                                  f"{args.timeout}s.{hint}")

            ch = Channel(sock_path)
            time.sleep(args.settle)

            shots = {k: str(workdir / f"{k}.png") for k in ("start", "selected", "after")}
            ch.send(f"--screenshot {shots['start']}")
            if not pathlib.Path(shots["start"]).is_file():
                return setup_fail("the app accepted --screenshot but wrote no file")

            box = find_module(shots["start"])
            if not box:
                return setup_fail("no module found on the rack to delete. The default "
                                  "session should seed one; check the rack-content gate (M6).")
            x0, x1, y0, y1 = box
            print(f"  module panel at px x{x0}..{x1} y{y0}..{y1}")

            # Screenshot pixels are 2x DIP on a retina backing store; the channel
            # takes DIP. Click high on the panel, clear of the jacks -- a click on
            # a jack starts a cable drag instead of selecting.
            cx = (x0 + x1) // 4
            cy = (y0 + (y1 - y0) // 6) // 2
            print(f"  clicking DIP ({cx},{cy}) to select it")
            ch.send(f"--pointer-down {cx},{cy}", f"--pointer-up {cx},{cy}")
            time.sleep(args.settle)
            ch.send(f"--screenshot {shots['selected']}")

            moved = pixels_differing(shots["start"], shots["selected"])
            print(f"  selection changed {moved} px")
            if moved == 0:
                return setup_fail(
                    "the click changed nothing on screen, so no module is selected. "
                    "Delete would correctly do nothing and this probe would blame the "
                    "key. Fix the click, not the binding.")

            before = doc_sizes(stderr_path)
            doc_before = before[-1] if before else None
            print(f"  document before: {doc_before if doc_before else 'unknown'} bytes")

            print(f"  sending --type {DEL_KEY}")
            replies = ch.send(f"--type {DEL_KEY}")
            if not any(r.get("cmd") == "type" and r.get("ok") for r in replies):
                return setup_fail(f"the channel refused --type: {replies}")
            time.sleep(args.settle * 2)
            ch.send(f"--screenshot {shots['after']}")

            after = doc_sizes(stderr_path)
            doc_after = after[-1] if after else None
            changed = pixels_differing(shots["selected"], shots["after"])
            still_there = column_is_light(shots["after"], x0, x1, y0, y1)

            print(f"\n  document after : {doc_after if doc_after else 'unknown'} bytes")
            print(f"  pixels changed : {changed}")
            print(f"  module column  : {'STILL PRESENT' if still_there else 'gone'}")

            doc_shrank = (doc_before is not None and doc_after is not None
                          and doc_after < doc_before)
            deleted = doc_shrank and changed > 0 and not still_there

            print()
            if deleted:
                print(f"DELETED -- document {doc_before} -> {doc_after} bytes "
                      f"({doc_before - doc_after} smaller), the panel is gone, and "
                      f"{changed} px changed.")
            else:
                why = []
                if not doc_shrank:
                    why.append(f"the document did not shrink ({doc_before} -> {doc_after})")
                if changed == 0:
                    why.append("no pixel changed")
                if still_there:
                    why.append("the module's panel is still on the rack")
                print("NOT DELETED -- " + "; ".join(why) + ".")
                print("A module was selected and Delete was delivered, so the key "
                      "reached the app and nothing happened. See E57 in "
                      "BACKLOG-DONE.md for the three defects this looked like.")

            if args.expect_fail:
                if deleted:
                    print("\nCONTROL FAILED: --expect-fail was given, but the key WORKED.")
                    return 1
                print("\nCONTROL OK: --expect-fail was given and the key is dead, "
                      "which is what this build was supposed to demonstrate.")
                return 0
            return 0 if deleted else 1
    finally:
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
        if not args.keep:
            for d in (cfg, workdir):
                shutil.rmtree(d, ignore_errors=True)
            if not args.ipc_dir:
                shutil.rmtree(ipc, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--standalone", required=True, help="the TIDE-Rack executable")
    ap.add_argument("--timeout", type=float, default=60, help="seconds to wait for the socket")
    ap.add_argument("--settle", type=float, default=1.0, help="seconds to wait after each gesture")
    ap.add_argument("--ipc-dir", help="override the IPC directory (must be short -- see sun_path)")
    ap.add_argument("--expect-fail", action="store_true",
                    help="invert the verdict, to demonstrate the control")
    ap.add_argument("--keep", action="store_true", help="keep the temp dirs for inspection")
    args = ap.parse_args()
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
