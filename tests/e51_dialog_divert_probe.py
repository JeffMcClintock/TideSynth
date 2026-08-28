#!/usr/bin/env python3
"""BACKLOG E51 -- prompts divert, are kept, and --dialogs drains them.

WHAT THIS GUARDS. The E51 chain, end to end, in the shipped macOS standalone:

    -quiet on the command line
      -> parsed before anything can prompt (argc/argv forwarding, both halves
         of which have silently broken before: GMPI_Wrappers#29)
      -> ApplicationBase::SeMessageBox[Async] routes through divertPrompt
         instead of a native modal (the chokepoint)
      -> the prompt is printed to stderr AND kept
      -> the --dialogs verb drains the kept list over the command channel
      -> a second --dialogs finds it empty (take semantics).

A break anywhere -- argv forwarding regressing, the chokepoint bypassed, the
keep-list dropped, the verb detached -- shows up as a changed count or a
missing line here.

THE SENTINEL IS THE QUIET BANNER, AND USING IT IS A MEASURED CHOICE, NOT A
SHORTCUT. SetQuiet()'s announcement ("Logging dialogs to stderr, and keeping
them for --dialogs.") is itself routed through the chokepoint, so it exercises
every stage above. The obvious richer fixture -- a session whose document names
a module that does not exist, which on Windows 2026-08-27 produced three
"Module not found in factory" prompts -- CANNOT be built for the rack any
more, and that is a finding, not a limitation of this probe:

    Measured 2026-08-28 (macos): corrupting Type="Multiply" (x2, inside a
    prefab) and then Type="TiDE Patch Point Out" (x10, everywhere) in a saved
    session produced ZERO prompts. The rack restore rebuilds every prefab
    container from the compiled-in bundle (E48), so the corrupted content is
    discarded wholesale and the re-saved document comes back byte-identical
    to the uncorrupted original -- the factory lookup that would prompt is
    never reached. The E48-era restore prompts were possible precisely
    BECAUSE prefab modules were not compiled in; E48 fixed that, and this
    class of prompt died with it on the rack path.

So a persistent fixture for a *restore-time* prompt does not exist on macOS
today. If one becomes constructible again, add it here as a second arm -- the
scaffolding (isolated config, corrupt-session splicing) is in the E51 journal
entry of 2026-08-28.

TWO ARMS:
  1. -quiet        : stderr carries the banner; --dialogs returns it with a
                     valid response constant recorded; second call drains to 0.
  2. no -quiet     : --dialogs is still a working verb and reports count 0 --
                     nothing was diverted, and the verb does not error.

Exit codes: 0 all checks pass - 1 a check failed - 2 the probe could not set
up (no socket, app died early). The distinction matters: a red 1 means the
E51 chain broke; a red 2 means the harness did, and blaming the chain for it
would be a false alarm in the exact shape of the real defect.

USAGE
    python3 tests/e51_dialog_divert_probe.py --standalone <TIDE-Rack binary>
"""

import argparse
import glob
import json
import os
import pathlib
import shutil
import socket
import subprocess
import sys
import tempfile
import time

BANNER = "Logging dialogs to stderr, and keeping them for --dialogs."
# Valid response constants: SafeMessageBox.h's shim says IDOK=0 (mac/linux);
# <winuser.h> says IDOK=1. IDCANCEL=2, IDYES=6, IDNO=7 agree across both.
VALID_ANSWERS = {0, 1, 2, 6, 7}
SUN_PATH_MAX = 103


def talk(sock_path, verbs, timeout=30):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect(sock_path)
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


class Arm:
    """One launch of the standalone, isolated, with or without -quiet."""

    def __init__(self, binary, quiet, timeout):
        self.binary, self.quiet, self.timeout = binary, quiet, timeout
        self.ipc = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e51-"))
        self.cfg = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e51cfg-"))
        self.stderr_path = self.cfg / "stderr.log"
        self.proc = None
        self.sock = None

    def __enter__(self):
        projected = len(str(self.ipc)) + len("/gmpi-standalone.") + 7
        if projected > SUN_PATH_MAX:
            raise RuntimeError(f"IPC path projects to ~{projected} bytes, over "
                               f"the {SUN_PATH_MAX}-byte sun_path limit")
        env = dict(os.environ,
                   GMPI_STANDALONE_IPC_DIR=str(self.ipc),
                   GMPI_STANDALONE_CONFIG_DIR=str(self.cfg))
        argv = [str(self.binary)] + (["-quiet"] if self.quiet else [])
        self.errf = open(self.stderr_path, "wb")
        self.proc = subprocess.Popen(argv, stdout=subprocess.DEVNULL,
                                     stderr=self.errf, env=env)
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            hits = glob.glob(str(self.ipc / "gmpi-standalone.*"))
            if hits:
                self.sock = hits[0]
                break
            if self.proc.poll() is not None:
                raise RuntimeError(f"app exited early rc={self.proc.returncode}; "
                                   f"stderr:\n{self.read_stderr()[-1200:]}")
            time.sleep(0.5)
        if not self.sock:
            raise RuntimeError(f"no command socket within {self.timeout}s; "
                               f"stderr:\n{self.read_stderr()[-1200:]}")
        time.sleep(2.0)  # let restore finish so late prompts are in the list
        return self

    def read_stderr(self):
        self.errf.flush()
        try:
            return self.stderr_path.read_text(errors="replace")
        except OSError:
            return ""

    def __exit__(self, *exc):
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self.errf.close()
        shutil.rmtree(self.ipc, ignore_errors=True)
        shutil.rmtree(self.cfg, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--standalone", required=True)
    ap.add_argument("--timeout", type=float, default=60)
    args = ap.parse_args()

    binary = pathlib.Path(args.standalone)
    if not binary.is_file() or not os.access(binary, os.X_OK):
        print(f"SETUP FAILED -- not an executable: {binary}")
        return 2

    failures = []

    def check(ok, name, detail):
        print(f"  {'PASS' if ok else 'FAIL'}  {name}\n        {detail}")
        if not ok:
            failures.append(name)

    print("E51 -- prompts divert, are kept, and --dialogs drains them\n")

    # ---- arm 1: -quiet -----------------------------------------------------
    print("arm 1: launched WITH -quiet")
    try:
        with Arm(binary, quiet=True, timeout=args.timeout) as arm:
            err = arm.read_stderr()
            check("parsed quiet=1" in err, "1a argv reaches the app",
                  "stderr says 'parsed quiet=1'" if "parsed quiet=1" in err
                  else "stderr never said 'parsed quiet=1' -- argv forwarding "
                       "has regressed before (GMPI_Wrappers#29); this is that")
            check(BANNER in err, "1b banner on stderr",
                  "the chokepoint's announcement is present" if BANNER in err
                  else "banner missing -- SetQuiet never ran or stderr lost")

            first = talk(arm.sock, ["--dialogs"])
            d = next((r for r in first if r.get("cmd") == "dialogs"), {})
            n = d.get("count", -1)
            texts = [x.get("text", "") for x in d.get("dialogs", [])]
            check(d.get("ok") is True and n >= 1, "1c --dialogs returns the kept list",
                  f"count={n}")
            check(any(BANNER in t for t in texts), "1d banner is IN the kept list",
                  "kept and drained intact" if any(BANNER in t for t in texts)
                  else f"texts={texts[:3]}")
            bad = [x for x in d.get("dialogs", []) if x.get("answered") not in VALID_ANSWERS]
            check(not bad, "1e every recorded answer is a response constant",
                  "all answers in {IDOK, IDCANCEL, IDYES, IDNO}" if not bad
                  else f"invalid answers: {[x.get('answered') for x in bad]} -- "
                       f"MB_OK-as-answer was E51's latent trap; it is back")

            second = talk(arm.sock, ["--dialogs"])
            d2 = next((r for r in second if r.get("cmd") == "dialogs"), {})
            check(d2.get("count") == 0, "1f second --dialogs drains to zero",
                  f"count={d2.get('count')} (take semantics)")
    except RuntimeError as e:
        print(f"\nSETUP FAILED (arm 1) -- {e}")
        return 2

    # ---- arm 2: no -quiet --------------------------------------------------
    print("arm 2: launched WITHOUT -quiet")
    try:
        with Arm(binary, quiet=False, timeout=args.timeout) as arm:
            r = talk(arm.sock, ["--dialogs"])
            d = next((x for x in r if x.get("cmd") == "dialogs"), {})
            check(d.get("ok") is True and d.get("count") == 0,
                  "2a verb works and nothing was diverted",
                  f"count={d.get('count')}")
            check(BANNER not in arm.read_stderr(), "2b no banner without -quiet",
                  "quiet is opt-in, as ruled")
    except RuntimeError as e:
        print(f"\nSETUP FAILED (arm 2) -- {e}")
        return 2

    print()
    if failures:
        print(f"FAILED -- {len(failures)} check(s): {', '.join(failures)}")
        return 1
    print("OK -- the E51 chain is intact end to end.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
