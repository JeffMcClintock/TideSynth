#!/usr/bin/env python3
"""BACKLOG E19 -- the WINDOWS VST3 cell: pixel diff and the int/enum clause.

The win/VST3 cell's ANIMATION clause passed 2026-09-02 through this harness.
Its other two clauses did not, and the reason was the fixture rather than the
platform: `e53-vcv-rack-segv.xml` puts no VCV panel on the visible rack page,
so the rack canvas was bare rails and the diff was 0 of 760,950 with the
counters climbing. `e83-vcv-scope-cabled.xml` fixes that -- the linux leg
proved so 2026-10-26 -- and this probe is the Windows twin of what that leg
drove with X11.

WHAT IT MEASURES, all inside ONE hosted window so the arms cannot disagree
about which instance they are looking at:

  1. PIXEL DIFF -- two captures of the floated editor 65 s apart, diffed whole,
     over the Scope screen, and over the module browser. The browser is the
     IN-FRAME ZERO CONTROL: it is in the same capture pair, so a diff there
     would mean the whole frame moved rather than the rack animating.
     REAPER's own transport is captured in the same pair as the LIVE-CAPTURE
     control, which is what says a zero is the plug-in being still rather than
     the capture being dead.
  2. MENU -- right-click the selected WT LFO and read the popup AS TEXT (the
     `menu` verb; see e19x_win.py for why Windows can do that and X11 cannot),
     then right-click the empty canvas DESELECTED and read it again. The option
     must be offered in the first and absent in the second, which is E38's fact
     that the menu is built for the selection.
  3. DSP OBEYS -- E82's instrument, in a host. A pick lands in the live module
     and traces nothing, so each arm picks a label and then forces a rack
     rebuild with a cable edit; the fresh RackProcessor prints
     `first NONZERO OUTPUT pin 0 (<v>)` once. A/B/A/B must DIFFER across picks
     and REPEAT within a pick, and a final edit with NO pick must repeat the
     last value -- which is what makes the difference follow the pick rather
     than the edit.

Exit codes: 0 pass - 1 a clause or a control failed - 2 setup.
"""
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "e19-host-feedback"))
import e19x_win as X  # noqa: E402

# Client coordinates in the 1650x900 editor, read off a capture of this fixture
# at this window size. The probe asserts the window size before using them.
EDITOR_W, EDITOR_H = 1650, 900
WT_BODY = (860, 470)        # the WT LFO's dark display area: select + right-click
EMPTY_CANVAS = (500, 745)   # bare rack, below the rails and left of the WT LFO.
                            # It must be INSIDE the rack view: a right-click on
                            # the editor's grey margin is not handled by the
                            # plug-in and propagates to REAPER, which answers
                            # with its own ruler menu.
WT_JACK = (892, 732)        # WT LFO's `WT` output
SCOPE_IN2 = (1090, 588)     # Scope `IN 2`
SCOPE_TILE = (1000, 295, 200, 110)   # the Scope's screen, inside its bezel
BROWSER_TILE = (170, 10, 170, 870)   # the module browser -- in-frame zero control

OPTION = "Wave points"
# The option is identified by its LABEL SET rather than by its name, because on
# this platform the parent menu item is owner-draw and has no readable string
# (e19x_win.item_info). The ten labels are the WT LFO's own, from E82.
LABELS = ["32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"]
TRACE_OUT = re.compile(r"RackProcessor: 'LFO2' first NONZERO OUTPUT pin \d+ \(([-0-9.]+)\)")


class Trace:
    def __init__(self, path):
        self.path = path
        self.pos = 0

    def new(self):
        if not os.path.exists(self.path):
            return []
        with open(self.path, "rb") as f:
            f.seek(self.pos)
            data = f.read()
            self.pos = f.tell()
        return data.decode(errors="replace").splitlines()

    def new_outputs(self):
        return [m.group(1) for ln in self.new() for m in [TRACE_OUT.search(ln)] if m]


def shot(path, hwnd=None, rect=None):
    if hwnd is not None:
        x, y = X.client_origin(hwnd)
        w, h = X.client_size(hwnd)
    else:
        x, y, w, h = rect
    bgra = X.grab(x, y, w, h)
    rgb = bytearray(w * h * 3)
    for i in range(w * h):
        rgb[i * 3] = bgra[i * 4 + 2]
        rgb[i * 3 + 1] = bgra[i * 4 + 1]
        rgb[i * 3 + 2] = bgra[i * 4]
    with open(path, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (w, h))
        f.write(bytes(rgb))
    return w, h


def read_ppm(p):
    with open(p, "rb") as f:
        assert f.readline().strip() == b"P6"
        w, h = map(int, f.readline().split())
        f.readline()
        return w, h, f.read()


def diff(a, b, tile=None, thr=8):
    w, h, da = read_ppm(a)
    w2, h2, db = read_ppm(b)
    assert (w, h) == (w2, h2), "size mismatch %s vs %s" % ((w, h), (w2, h2))
    if tile:
        tx, ty, tw, th = tile
    else:
        tx, ty, tw, th = 0, 0, w, h
    n = 0
    total = 0
    for y in range(ty, min(ty + th, h)):
        row = y * w
        for x in range(tx, min(tx + tw, w)):
            i = (row + x) * 3
            total += 1
            if (abs(da[i] - db[i]) > thr or abs(da[i + 1] - db[i + 1]) > thr
                    or abs(da[i + 2] - db[i + 2]) > thr):
                n += 1
    return n, total


def find_editor():
    """The gmpi_ui child, found by CLASS rather than by its host's title.

    REAPER does not always give the editor a floating window titled
    `VST3i: TiDE Rack ...`: when its ini remembers the FX chain docked, the
    editor is a descendant of `REAPER_dock` and no ancestor carries the
    plug-in's name. Keying on the title read the docked case as "the host
    never opened an editor" while measure.log plainly said `editor floated`.
    `GMPIUI*` is gmpi_ui's own window class and cannot belong to anything else
    on this screen. EnumChildWindows is already recursive over descendants.
    """
    for t in X.toplevels():
        for c in X.children(t):
            if X.win_class(c).startswith("GMPIUI") and X.user32.IsWindowVisible(c):
                w, h = X.client_size(c)
                if w > 400 and h > 300:
                    # Return the owning TOP-LEVEL as well. GetAncestor(c,
                    # GA_ROOT) answers 0 for this child here, so the window to
                    # raise has to come from the enumeration that found it.
                    return c, t
    return None, None


def menu_dump():
    popups = [h for h in X.toplevels() if X.win_class(h) == "#32768"]
    items = []
    for h in popups:
        hm = X.user32.SendMessageW(h, X.MN_GETHMENU, 0, 0)
        if hm:
            items += X.menu_items(hm)
    return items


GA_ROOT = 2
ED_ROOT = [None]   # the editor's owning top-level, set once it is found
PLUGIN_MENU_MARK = "About TIDE"


def is_plugin_menu(items):
    """True iff this popup is TIDE's own.

    Without this the harness cannot tell the plug-in's menu from the host's,
    and REAPER's is what you get whenever a right-click misses the editor or
    the editor is not foreground -- a 60-item ruler menu that contains no
    `Wave points`, which reads exactly like "the option is not offered". Two
    measurements were lost to that before the check existed: one where the
    click was mapped to the wrong monitor, one where focus had moved after the
    previous clause. The plug-in's menu always carries its About item.
    """
    return any(t.strip().startswith(PLUGIN_MENU_MARK) for _, t, _ in items)


SWP_NOSIZE, SWP_NOMOVE, SWP_SHOWWINDOW = 0x0001, 0x0002, 0x0040
HWND_TOP = 0


def focus_editor(ed, log=None):
    """Raise the editor's floating window and make it foreground. Verified.

    A bare SetForegroundWindow is NOT enough and this is the single thing that
    cost the most here. Windows refuses a foreground change requested by a
    process that does not own the current foreground window: the call returns
    without raising anything, REAPER's MAXIMIZED main window stays on top of
    the 1672x995 FX float, and every subsequent click lands on REAPER instead
    of the plug-in. What came back was REAPER's own menus -- the TCP menu from
    a click meant for bare rack, its 39-item track menu from one meant for the
    WT LFO -- each of which contains no `Wave points` and so reads exactly like
    the plug-in failing to offer the option.

    AttachThreadInput to the owning thread lifts that restriction, which is the
    documented way round it. The function then CHECKS GetForegroundWindow and
    says so, because a focus call that silently did nothing is what produced
    three wrong measurements.
    """
    root = ED_ROOT[0]
    cur = X.user32.GetWindowThreadProcessId(X.user32.GetForegroundWindow(), None)
    tgt = X.user32.GetWindowThreadProcessId(root, None)
    me = X.ctypes.windll.kernel32.GetCurrentThreadId()
    for t in {cur, tgt} - {0, me}:
        X.user32.AttachThreadInput(me, t, True)
    X.user32.SetWindowPos(root, HWND_TOP, 0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    X.user32.BringWindowToTop(root)
    X.user32.SetForegroundWindow(root)
    X.user32.SetActiveWindow(root)
    for t in {cur, tgt} - {0, me}:
        X.user32.AttachThreadInput(me, t, False)
    time.sleep(0.4)
    fg = X.user32.GetForegroundWindow()
    if fg != root:
        # Windows still refused. Do it the way a person would: CLICK the FX
        # window's own caption. A real click activates a window with no
        # foreground-rights check at all, which is why this is the thing that
        # works rather than the documented API. The caption is safe to hit --
        # it is not the rack, so it cannot change the selection -- and a click
        # with no movement between down and up cannot drag the window.
        x, y, w, h = X.rect(root)
        cx, cy = x + w // 2, y + 14
        if window_at(cx, cy)[0] != root:
            if log:
                log("  NOTE: the FX window's caption is covered; not clicking")
            return root
        X.move_abs(cx, cy)
        time.sleep(0.1)
        X.user32.mouse_event(X.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        time.sleep(0.05)
        X.user32.mouse_event(X.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        time.sleep(0.4)
        fg = X.user32.GetForegroundWindow()
    if log and fg != root:
        log("  NOTE: foreground is %s, wanted the editor's %s" % (fg, root))
    return root


class POINT(X.wintypes.POINT):
    pass


def window_at(x, y):
    """Which window owns this screen point -- the only thing that settles
    'did my click reach the plug-in'. Everything else is inference."""
    p = X.wintypes.POINT(x, y)
    h = X.user32.WindowFromPoint(p)
    return h, X.win_class(h), X.win_text(h)[:40]


class MisdirectedInput(Exception):
    """Raised instead of clicking somewhere that is not the plug-in's editor."""


def check_target(hwnd, x, y, what):
    """Refuse to deliver input unless the point really belongs to the editor.

    THIS GATE IS NOT A TIDINESS CHECK, and it is here because the alternative
    happened. These are the DEVELOPER's machine and his running applications,
    and synthetic mouse input goes wherever the pointer is -- so a stale target
    window does not make a click fail, it makes it land in somebody else's
    program. When the editor HWND was destroyed by a stray Escape (see
    close_popups), ClientToScreen left the offset at 0,0 and this probe's
    clicks and drags were delivered, in raw screen coordinates, to REAPER's
    track view and then to the developer's open Outlook window -- one of them
    into its folder pane, which changed the folder on display. Nothing was
    deleted and no menu item was ever activated, because the lookups that
    follow a click all failed to find their item; that was luck, not design.

    WindowFromPoint is the whole check, and it costs one call.
    """
    h, cls, txt = window_at(x, y)
    if h != hwnd:
        raise MisdirectedInput(
            "%s meant for the editor (%s) would land on %s [%s] %r at %d,%d "
            "-- refusing" % (what, hwnd, h, cls, txt, x, y))
    return h


def click(hwnd, pt, right=False, double=False, log=None):
    if not X.user32.IsWindow(hwnd):
        raise MisdirectedInput("the editor window %s no longer exists" % hwnd)
    ox, oy = X.client_origin(hwnd)
    x, y = ox + pt[0], oy + pt[1]
    check_target(hwnd, x, y, "a %s-click" % ("right" if right else "left"))
    if log:
        log("    click %s at client %s = screen %d,%d (editor confirmed)"
            % ("R" if right else "L", pt, x, y))
    X.move_abs(x, y)
    time.sleep(0.1)
    down = X.MOUSEEVENTF_RIGHTDOWN if right else X.MOUSEEVENTF_LEFTDOWN
    up = X.MOUSEEVENTF_RIGHTUP if right else X.MOUSEEVENTF_LEFTUP
    for _ in range(2 if double else 1):
        X.user32.mouse_event(down, 0, 0, 0, 0)
        time.sleep(0.05)
        X.user32.mouse_event(up, 0, 0, 0, 0)
        time.sleep(0.05)


def drag(hwnd, a, b, steps=25):
    if not X.user32.IsWindow(hwnd):
        raise MisdirectedInput("the editor window %s no longer exists" % hwnd)
    ox, oy = X.client_origin(hwnd)
    x1, y1 = ox + a[0], oy + a[1]
    x2, y2 = ox + b[0], oy + b[1]
    # BOTH ends: a drag that starts on the editor and ends elsewhere is a
    # drag-and-drop INTO another application, which is worse than a stray click.
    check_target(hwnd, x1, y1, "a drag start")
    check_target(hwnd, x2, y2, "a drag end")
    X.move_abs(x1, y1)
    time.sleep(0.2)
    X.user32.mouse_event(X.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
    time.sleep(0.15)
    for i in range(1, steps + 1):
        X.move_abs(x1 + (x2 - x1) * i // steps, y1 + (y2 - y1) * i // steps)
        time.sleep(0.025)
    time.sleep(0.15)
    X.user32.mouse_event(X.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
    time.sleep(0.5)


def esc():
    X.user32.keybd_event(0x1B, 0, 0, 0)
    time.sleep(0.03)
    X.user32.keybd_event(0x1B, 0, 2, 0)
    time.sleep(0.4)


def popups_open():
    return [h for h in X.toplevels() if X.win_class(h) == "#32768"]


def close_popups(log=None):
    """Escape ONLY while a popup is actually up.

    An unconditional `esc(); esc()` is what broke every clause after the first:
    the first Escape closes the menu, and the second reaches REAPER's FLOATING
    FX WINDOW and CLOSES IT. The editor HWND is then destroyed, so
    ClientToScreen on it fails and silently leaves the point at 0,0 -- every
    later click is delivered in raw screen coordinates to whatever is beneath,
    which here was REAPER's track view and then Outlook. Nothing errored; the
    clicks simply went somewhere else, and the menus that came back were the
    hosts' own. Measured 2026-11-02 with WindowFromPoint logging.
    """
    for _ in range(4):
        if not popups_open():
            return True
        esc()
    if log and popups_open():
        log("  NOTE: a popup is still open after 4 escapes")
    return not popups_open()


def editor_alive(ed):
    return bool(X.user32.IsWindow(ed)) and X.client_size(ed) == (EDITOR_W, EDITOR_H)


def menu_hit(item, click_it, by_child=False):
    """Hover or click a live menu item, by its own label or by a child's.

    GetMenuItemRect gives SCREEN coordinates for a DISPLAYED item, so the
    submenu must be open before its rows can be found -- which is why a pick is
    hover-then-click rather than one call. `by_child` reaches the OWNER-DRAW
    parent, which has no label of its own (see e19x_win.item_info) and can only
    be named by what it contains.
    """
    for h in [h for h in X.toplevels() if X.win_class(h) == "#32768"]:
        hm = X.user32.SendMessageW(h, X.MN_GETHMENU, 0, 0)
        if not hm:
            continue
        got = X.find_submenu_with(hm, item) if by_child else X.find_item(hm, item)
        if not got:
            continue
        owner, idx = got
        r = X.wintypes.RECT()
        if not X.user32.GetMenuItemRect(0, owner, idx, X.ctypes.byref(r)):
            return "GetMenuItemRect failed for %r" % item
        cx, cy = (r.left + r.right) // 2, (r.top + r.bottom) // 2
        X.move_abs(cx, cy)
        time.sleep(0.45)
        if click_it:
            X.user32.mouse_event(X.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
            time.sleep(0.05)
            X.user32.mouse_event(X.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        return "%s %s%r at %d,%d" % ("clicked" if click_it else "hovered",
                                     "parent-of-" if by_child else "", item, cx, cy)
    seen = [(h, X.user32.SendMessageW(h, X.MN_GETHMENU, 0, 0))
            for h in X.toplevels() if X.win_class(h) == "#32768"]
    detail = []
    for h, hm in seen:
        n = X.user32.GetMenuItemCount(hm) if hm else -1
        detail.append("hwnd=%s hmenu=%s items=%s" % (h, hm, n))
    return "NOT FOUND: %r%s -- popups: %s" % (
        item, " (as a submenu parent)" if by_child else "",
        "; ".join(detail) or "NONE OPEN")


def pick(label, log):
    """Open the option submenu and click `label`. The popup must already be up.

    The parent is reached BY CHILD because it is owner-draw and carries no
    label -- `32` is a row of the submenu, so the item whose submenu holds `32`
    is the option's parent.
    """
    log("  %s" % menu_hit(LABELS[0], False, by_child=True))
    time.sleep(0.8)
    log("  %s" % menu_hit(label, True))
    time.sleep(0.6)


def main():
    scratch = os.environ.get("E19_SCRATCH_U")
    if not scratch:
        print("set E19_SCRATCH_U to the scratch dir in unix-ish form")
        return 2
    shots = os.path.join(scratch, "shots")
    os.makedirs(shots, exist_ok=True)
    logf = open(os.path.join(scratch, "probe.log"), "w")

    def log(s):
        print(s)
        logf.write(s + "\n")
        logf.flush()

    err = os.path.join(scratch, "reaper-run.err")
    tr = Trace(err)
    mlog = os.path.join(scratch, "measure.log")

    # wait for the editor
    ed = root = None
    for _ in range(90):
        ed, root = find_editor()
        if ed:
            break
        time.sleep(1)
    ED_ROOT[0] = root
    if not ed:
        log("SETUP: no editor window after 90 s")
        return 2
    w, h = X.client_size(ed)
    log("editor hwnd %d client %dx%d at %s ; root %s rect %s"
        % (ed, w, h, X.client_origin(ed), root, X.rect(root)))
    if (w, h) != (EDITOR_W, EDITOR_H):
        log("SETUP: editor is %dx%d, the coordinates are for %dx%d -- refusing "
            "to click blind" % (w, h, EDITOR_W, EDITOR_H))
        return 2
    tr.new()
    time.sleep(3)

    fails = []

    # E19_ONLY lets a failing clause be re-run on its own while the harness is
    # being shaken out; a reported PASS always comes from a run with it unset.
    only = os.environ.get("E19_ONLY", "123")

    # ---------------- 1. pixel diff ----------------
    rx, ry, rw, rh = 520, 1647, 1200, 54      # REAPER's transport strip
    if "1" in only:
        log("\n== clause 1: pixel diff, 65 s apart ==")
        shot(os.path.join(shots, "a.ppm"), hwnd=ed)
        shot(os.path.join(shots, "a_tp.ppm"), rect=(rx, ry, rw, rh))
        time.sleep(65)
        shot(os.path.join(shots, "b.ppm"), hwnd=ed)
        shot(os.path.join(shots, "b_tp.ppm"), rect=(rx, ry, rw, rh))
        a, b = os.path.join(shots, "a.ppm"), os.path.join(shots, "b.ppm")
        whole = diff(a, b)
        scope = diff(a, b, SCOPE_TILE)
        browser = diff(a, b, BROWSER_TILE)
        transport = diff(os.path.join(shots, "a_tp.ppm"), os.path.join(shots, "b_tp.ppm"))
        log("  whole editor   %d of %d" % whole)
        log("  Scope screen   %d of %d" % scope)
        log("  module browser %d of %d   (in-frame ZERO control)" % browser)
        log("  REAPER transport %d of %d (LIVE-CAPTURE control)" % transport)
        if scope[0] == 0:
            fails.append("Scope screen did not change over 65 s")
        if browser[0] != 0:
            fails.append("module browser changed (%d) -- the whole frame moved" % browser[0])
        if transport[0] == 0:
            fails.append("REAPER transport did not change -- the capture is dead")

    # ---------------- 2. menu ----------------
    log("\n== clause 2: the menu follows the selection ==")
    focus_editor(ed, log)
    click(ed, WT_BODY)                 # select
    time.sleep(0.6)
    click(ed, WT_BODY, right=True)     # context menu
    time.sleep(0.9)
    sel = menu_dump()
    log("  selected WT LFO -- %d items" % len(sel))
    for d, t, f in sel:
        log("    %s%s%s" % ("    " * d, t, (" [" + ",".join(f) + "]") if f else ""))
    if not is_plugin_menu(sel):
        fails.append("the menu on the selected WT LFO is NOT the plug-in's "
                     "(no %r item) -- the right-click missed the editor"
                     % PLUGIN_MENU_MARK)
    close_popups(log)

    focus_editor(ed, log)
    click(ed, EMPTY_CANVAS)            # deselect
    time.sleep(0.6)
    click(ed, EMPTY_CANVAS, right=True, log=log)
    time.sleep(0.9)
    desel = menu_dump()
    log("  empty canvas, deselected -- %d items" % len(desel))
    for d, t, f in desel:
        log("    %s%s%s" % ("    " * d, t, (" [" + ",".join(f) + "]") if f else ""))
    close_popups(log)

    def offers_option(items):
        """The option is present iff ALL ten of its labels are, as menu rows."""
        texts = {t.strip() for _, t, _ in items}
        return all(lb in texts for lb in LABELS)

    if not offers_option(sel):
        missing = [lb for lb in LABELS if lb not in {t.strip() for _, t, _ in sel}]
        fails.append("the %s option is not offered on the selected WT LFO "
                     "(missing labels %s)" % (OPTION, missing))
    if offers_option(desel):
        fails.append("the %s option is offered on the empty canvas too -- "
                     "not selection-scoped" % OPTION)
    log("  deselected menu is the plug-in's: %s" % is_plugin_menu(desel))
    if not is_plugin_menu(desel):
        fails.append("the deselected right-click produced the HOST's menu, not "
                     "the plug-in's -- that is a miss, not a control")
    ticked = [t.strip() for _, t, f in sel if "CHECKED" in f]
    log("  ticked in the selected menu: %s" % ticked)
    if ticked != ["1024"]:
        fails.append("expected exactly 1024 ticked (the option's default), got %s"
                     % ticked)

    # ---------------- 3. the DSP obeys ----------------
    log("\n== clause 3: a pick reaches the DSP ==")
    arms = [("1024", True), ("32", True), ("1024", True), ("32", True), (None, True)]
    seen = []
    jack_from, jack_to = WT_JACK, SCOPE_IN2
    for label, _ in arms:
        if label is not None:
            if not editor_alive(ed):
                fails.append("the editor window was destroyed before arm %r" % label)
                log("  the editor HWND is gone -- stopping clause 3")
                break
            focus_editor(ed, log)
            click(ed, WT_BODY, log=log)
            time.sleep(0.5)
            click(ed, WT_BODY, right=True, log=log)
            time.sleep(1.0)
            opened = menu_dump()
            if not is_plugin_menu(opened):
                log("  WARNING: popup is not the plug-in's (%d items) -- "
                    "retrying once after re-focusing" % len(opened))
                close_popups(log)
                focus_editor(ed, log)
                click(ed, WT_BODY, log=log)
                time.sleep(0.5)
                click(ed, WT_BODY, right=True, log=log)
                time.sleep(1.0)
                opened = menu_dump()
            if not is_plugin_menu(opened):
                fails.append("arm %r could not open the plug-in's menu" % label)
            pick(label, log)
            close_popups(log)
        tr.new()
        drag(ed, jack_from, jack_to)        # a cable edit rebuilds the rack
        # POLL rather than sleep a fixed time: REAPER's stderr is block-buffered
        # through the runner's redirect, so a rebuild's trace line can surface
        # many seconds after the edit that caused it. A fixed 3 s wait read
        # every arm as "(none)" while the rebuilds were plainly happening.
        outs = []
        for _ in range(40):
            outs += tr.new_outputs()
            if outs:
                break
            time.sleep(1.0)
        log("  pick=%-5s cable edit -> first NONZERO OUTPUT %s"
            % (label, outs if outs else "(none)"))
        seen.append((label, outs[-1] if outs else None))

    vals = dict()
    for label, v in seen:
        vals.setdefault(label, []).append(v)
    log("  by pick: %s" % vals)

    got = [v for _, v in seen]
    if any(v is None for v in got):
        fails.append("a cable edit produced no fresh RackProcessor output: %s" % got)
    else:
        if got[0] == got[1]:
            fails.append("1024 and 32 gave the same value %s" % got[0])
        if got[0] != got[2] or got[1] != got[3]:
            fails.append("a pick did not repeat: %s" % got)
        if got[4] != got[3]:
            fails.append("an edit with NO pick changed the value: %s -> %s"
                         % (got[3], got[4]))

    log("\n== transport over the window ==")
    if os.path.exists(mlog):
        for ln in open(mlog).read().splitlines():
            if ln.startswith("t=") and ("playstate" in ln):
                last = ln
        log("  last: %s" % last)
        log("  fx_ident: %s" % next((l for l in open(mlog) if l.startswith("fx_ident")), "?").strip())

    log("\n== RESULT ==")
    if fails:
        for f in fails:
            log("  FAIL: %s" % f)
        return 1
    log("  all clauses and controls passed")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except MisdirectedInput as e:
        # Stopping with a clause unmeasured is the CORRECT outcome here: the
        # alternative is delivering mouse input into whatever program is under
        # the pointer on the developer's desktop.
        print("ABORTED before delivering input -- %s" % e)
        sys.exit(1)
