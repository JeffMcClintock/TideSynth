#!/usr/bin/env python3
"""BACKLOG E65 -- every panel on the rack gets its draft render, not just one.

WHAT THIS GUARDS. TiDEPanel's progressive contract, which is stated in
TiDEPanelGui.cpp's own settle-timer comment: on any size or layout change the
PREVIEW (tide_render RenderMode::Fast, full pixel size, tens of milliseconds)
is requested IMMEDIATELY and is what puts a correct image on screen fast, while
the FULL trace (seconds, uncancellable once begun) waits for 250 ms of quiet.

THE DEFECT IT WAS WRITTEN FOR, measured 2026-08-30 on a seven-panel rack:

    7 REQUEST lines inside 1.2 ms
    5 DROPPED   -- wanted and never started
    2 PREVIEW   -- so five panels sat on flat grey for the whole 60 s run

`FaceRenderer` is a process-wide singleton with ONE worker, and it held ONE
`wantedKey`. Every panel calls request() in the same millisecond of a load, so
each asker overwrote the last and the worker only ever learned about the final
one. Nothing re-declared the others, because request() is reached only when a
panel's SIZE changes -- so on a still rack they stayed grey indefinitely. The
fix keys the want by PANEL and drains every pending preview before starting any
full trace.

WHY A MULTI-PANEL FIXTURE, AND WHY IT IS BUILT HERE. The shipped
DefaultRack.synthedit puts exactly ONE panel in view, and with one panel the
bug is invisible -- the single asker is also the last asker, so its preview
lands at ~70 ms and everything looks correct. That is why this reproduces only
on a populated rack, and why check 0 below refuses to pass on a fixture that
did not actually put several DIFFERENT panels on screen. Panels must differ:
identical panels share one cache entry by design (same width, height and config
hash), so N copies of one module would be one trace and would prove nothing.

REQUIRES A TRACE-ARMED BUILD:

    cmake -S . -B build-e65 -G Ninja -DCMAKE_BUILD_TYPE=Release \\
          -DTIDE_PANEL_TRACE_LOG=ON
    ninja -C build-e65 TIDE_Rack_STANDALONE
    python3 tests/e65_panel_preview_probe.py --standalone \\
        build-e65/SynthEditSem/TIDE-Rack.app/Contents/MacOS/TIDE-Rack

Without the option the log is never written and this exits 2 (setup), never 0 --
a silent pass on an unarmed binary is the one outcome that would make the probe
worse than nothing.

Exit codes: 0 pass - 1 the progressive contract regressed - 2 setup.
"""

import argparse, glob, os, pathlib, re, shutil, socket, subprocess, sys, tempfile, time

SUN_PATH_MAX = 103

# The worker is serial and one preview costs ~70 ms, so N panels need about
# N * 70 ms before the last one has an image. Generous, because a loaded box
# traces slower and this must not go flaky; the regression it catches is
# "never", not "late".
def preview_budget_ms(panels):
    return 400 + 250 * panels


def build_fixture(src_rack: pathlib.Path, dst_rack: pathlib.Path, extra: int):
    """Duplicate the rack's first panel-bearing container `extra` times.

    Each copy gets fresh handles, its own rack slot, and -- the part that
    matters -- a DIFFERENT panel layout string, so it hashes to its own
    FaceRenderer config and needs its own trace.
    """
    lines = src_rack.read_text(encoding="utf-8").splitlines(keepends=True)
    start = next((i for i, l in enumerate(lines) if 'rack_module="true"' in l), None)
    if start is None:
        raise SystemExit("SETUP FAILED -- no rack_module container in " + str(src_rack))
    indent = len(lines[start]) - len(lines[start].lstrip())
    end = next(i for i in range(start + 1, len(lines))
               if lines[i].strip() == "</module>"
               and (len(lines[i]) - len(lines[i].lstrip())) == indent)
    block = "".join(lines[start:end + 1])
    if "SE TiDE:Panel" not in block:
        raise SystemExit("SETUP FAILED -- that container carries no SE TiDE:Panel")

    copies = []
    for k in range(1, extra + 1):
        b = re.sub(r'handle="(\d+)"',
                   lambda m: 'handle="%d"' % (int(m.group(1)) % 100000 + 800000000 + k * 10007),
                   block)
        b = re.sub(r'name="([^"]*)"', lambda m: 'name="%s %d"' % (m.group(1), k), b, count=1)
        # the layout pin -- a different grill row per copy is enough to change
        # the config hash without changing the panel's size
        b = re.sub(r'grill (\d+) (\d+)',
                   lambda m: "grill %s %d" % (m.group(1), int(m.group(2)) + k), b)
        b = re.sub(r'<PanelWndPosition l="(\d+)" r="(\d+)"',
                   lambda m: '<PanelWndPosition l="%d" r="%d"'
                             % (int(m.group(1)) + 48 * k, int(m.group(2)) + 48 * k), b)
        copies.append(b)

    dst_rack.write_text("".join(lines[:end + 1]) + "".join(copies) + "".join(lines[end + 1:]),
                        encoding="utf-8")


def run_app(binary: pathlib.Path, log: pathlib.Path, settle: float, timeout: float):
    ipc = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e65-"))
    cfg = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e65cfg-"))
    if len(str(ipc)) + 24 > SUN_PATH_MAX:
        raise SystemExit("SETUP FAILED -- IPC path too long (sun_path)")
    env = dict(os.environ, GMPI_STANDALONE_IPC_DIR=str(ipc),
               GMPI_STANDALONE_CONFIG_DIR=str(cfg), TIDE_PANEL_LOG_PATH=str(log))
    proc = subprocess.Popen([str(binary)], stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, env=env)
    try:
        sock, deadline = None, time.monotonic() + timeout
        while time.monotonic() < deadline:
            hits = glob.glob(str(ipc / "gmpi-standalone.*"))
            if hits:
                sock = hits[0]
                break
            if proc.poll() is not None:
                raise SystemExit("SETUP FAILED -- app exited rc=%s" % proc.returncode)
            time.sleep(0.5)
        if not sock:
            raise SystemExit("SETUP FAILED -- no command socket")
        time.sleep(settle)
        shot = str(log) + ".png"
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(30); s.connect(sock)
        s.sendall(("--screenshot %s\n" % shot).encode()); s.shutdown(socket.SHUT_WR)
        while s.recv(65536):
            pass
        s.close()
        return shot
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(10)
            except subprocess.TimeoutExpired:
                proc.kill()


# The recorded behaviour this probe was written to catch, kept so the checks
# themselves can be shown to fail. See --selftest.
PREFIX_LOG = pathlib.Path(__file__).with_name("fixtures") / "e65-prefix-7panel.log"


def selftest():
    """The control: run the checks against the log the DEFECT produced.

    A guard nobody has watched fail is not a guard -- this repo has paid for
    that lesson more than once. The fixture is the real trace captured on
    2026-08-30 from the pre-fix binary on the same seven-panel rack, so this
    asserts the checks catch the actual regression rather than a synthetic one.
    """
    if not PREFIX_LOG.is_file():
        print("SETUP FAILED -- missing control fixture %s" % PREFIX_LOG); return 2
    print("E65 SELFTEST -- the checks, run against the PRE-FIX trace\n")
    rc = run_checks(PREFIX_LOG.read_text(encoding="utf-8"), 7, PREFIX_LOG, "(none)")
    print()
    if rc == 1:
        print("SELFTEST PASS -- the checks fail on the defect, as they must.")
        return 0
    print("SELFTEST FAIL -- the pre-fix trace was accepted (rc=%d). These checks "
          "would not have caught E65." % rc)
    return 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--standalone",
                    help="a TIDE-Rack binary built with -DTIDE_PANEL_TRACE_LOG=ON")
    ap.add_argument("--log", help="analyse an existing trace log instead of running the app")
    ap.add_argument("--selftest", action="store_true",
                    help="run the checks against the recorded pre-fix trace; they must FAIL")
    ap.add_argument("--panels", type=int, default=6, help="EXTRA panels to add (default 6)")
    ap.add_argument("--timeout", type=float, default=90)
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if args.log:
        log = pathlib.Path(args.log)
        if not log.is_file():
            print("SETUP FAILED -- no such log: %s" % log); return 2
        return run_checks(log.read_text(encoding="utf-8"), args.panels + 1, log, "(none)")
    if not args.standalone:
        print("SETUP FAILED -- pass --standalone, --log or --selftest"); return 2

    binary = pathlib.Path(args.standalone).resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        print("SETUP FAILED -- not executable: %s" % binary); return 2
    app = binary.parent.parent.parent            # .../TIDE-Rack.app
    if app.suffix != ".app":
        print("SETUP FAILED -- expected a .app bundle, got %s" % app); return 2

    work = pathlib.Path(tempfile.mkdtemp(prefix="/tmp/e65work-"))
    copy = work / app.name
    shutil.copytree(app, copy, symlinks=True)
    rack = copy / "Contents" / "Resources" / "DefaultRack.synthedit"
    if not rack.is_file():
        print("SETUP FAILED -- no DefaultRack.synthedit in the bundle"); return 2
    build_fixture(rack, rack, args.panels)

    log = work / "TiDEPanel.log"
    expect = args.panels + 1
    settle = preview_budget_ms(expect) / 1000.0 + 1.0
    shot = run_app(copy / "Contents" / "MacOS" / binary.name, log, settle, args.timeout)

    if not log.is_file() or not log.stat().st_size:
        print("SETUP FAILED -- no trace log. Build with -DTIDE_PANEL_TRACE_LOG=ON."); return 2
    return run_checks(log.read_text(encoding="utf-8", errors="replace"), expect, log, shot)


def run_checks(text, expect, log, shot):
    def stamped(kind):
        return re.findall(r"^\s*([\d.]+) ms\s+" + kind + r".*?cfg=([0-9a-f]+)", text, re.M)

    requests = stamped(r"REQUEST")
    previews = stamped(r"PREVIEW  ready")
    dropped = re.findall(r"^\s*[\d.]+ ms\s+DROPPED", text, re.M)
    req_cfgs = {c for _, c in requests}
    prev_cfgs = {c for _, c in previews}

    print("E65 -- the panel draft render, on a %d-panel rack\n" % expect)
    print("  log: %s\n  shot: %s\n" % (log, shot))
    failures = []

    def check(ok, name, detail):
        print("  %s  %s\n        %s" % ("PASS" if ok else "FAIL", name, detail))
        if not ok:
            failures.append(name)

    # 0 -- the vacuity guard. With one panel this bug cannot appear, so a run
    #      that only exercised one panel must not be allowed to report PASS.
    if len(req_cfgs) < 2:
        print("SETUP FAILED -- only %d distinct panel(s) asked for a render; the "
              "fixture did not populate the rack, so nothing was tested."
              % len(req_cfgs))
        return 2
    check(len(req_cfgs) == expect, "0 the fixture put every panel on screen",
          "%d distinct panels requested a render, expected %d" % (len(req_cfgs), expect))

    # 1 -- the regression itself.
    missing = sorted(req_cfgs - prev_cfgs)
    check(not missing, "1 every panel that asked got a draft render",
          "%d/%d previewed" % (len(prev_cfgs), len(req_cfgs))
          + ("" if not missing else " -- NEVER RENDERED: " + ", ".join(missing)))

    # 2 -- the mechanism, named directly, so a future single-slot regression is
    #      reported as its cause rather than as its symptom.
    check(not dropped, "2 no panel's request was silently discarded",
          "0 DROPPED" if not dropped else "%d DROPPED lines -- the worker's want "
          "slot is being overwritten again" % len(dropped))

    # 3 -- the contract's timing. Per panel a preview is tens of ms; the worker
    #      is serial, so the LAST of N waits for the ones ahead of it.
    if previews and requests:
        t0 = min(float(t) for t, _ in requests)
        last = max(float(t) for t, _ in previews)
        first = min(float(t) for t, _ in previews)
        budget = preview_budget_ms(expect)
        check(last - t0 <= budget, "3 every draft landed promptly",
              "first at %.0f ms, last at %.0f ms after the first request "
              "(budget %d ms for %d panels)" % (first - t0, last - t0, budget, expect))
    else:
        check(False, "3 every draft landed promptly", "no PREVIEW lines at all")

    # 4 -- what actually reached the screen. A trace that renders and is never
    #      painted is the row's third candidate and fails differently.
    grey = len(re.findall(r"^\s*[\d.]+ ms\s+DRAW\s+flat grey", text, re.M))
    painted = len(re.findall(r"^\s*[\d.]+ ms\s+DRAW\s+(?:PREVIEW|FULL)", text, re.M))
    check(painted >= expect, "4 every panel painted something better than grey",
          "%d grey draws, %d preview/full draws, %d panels" % (grey, painted, expect))

    print("\nRESULT: %s" % ("PASS" if not failures else
                            "FAIL -- " + ", ".join(failures)))
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
