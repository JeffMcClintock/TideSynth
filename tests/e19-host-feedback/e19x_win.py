#!/usr/bin/env python3
"""E19's Windows screen driver -- the twin of e19x.c, which is X11-only.

    e19x_win.py tree   [--match SUBSTR]
    e19x_win.py geom   --hwnd H
    e19x_win.py shot   (--hwnd H | --rect X,Y,W,H) --out f.ppm [--printwindow]
    e19x_win.py menu
    e19x_win.py click  X,Y [--hwnd H] [--right] [--double]
    e19x_win.py drag   X1,Y1 X2,Y2 [--hwnd H] [--steps N]
    e19x_win.py key    NAME...

Coordinates are PHYSICAL PIXELS. Without --hwnd they are screen coordinates;
with it they are client-relative to that window, which is what the linux cell's
`e19x click -w <win> x y` meant and what the geometry in this directory's README
is written in.

WHY A SEPARATE FILE RATHER THAN A PORT OF e19x.c. Three of e19x's five verbs are
X11 calls with no Win32 equivalent worth writing in C here (XQueryTree for the
override-redirect menu windows, XTestFakeButtonEvent, XGetImage), and the
Windows versions are ~20 lines of ctypes each. Nothing in the harness compiles a
C driver on this platform, so a script keeps the Windows leg buildless.

TWO WINDOWS FACTS THIS ENCODES, both of which cost a measurement before they
were understood:

  * DPI. Python is not DPI-aware by default, so GetWindowRect and the screen DC
    both come back in VIRTUALISED coordinates on a scaled monitor -- a 1100x600
    editor reads 733x400 at 150%, and a click lands a third of the way up the
    window. SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2) first, before
    any other user32 call, and everything is physical pixels. (Memory-of-box:
    pwsh is only system-aware, so it cannot do this at all.)
  * CAPTURE. The default is a screen BitBlt over the window's rect, which is the
    faithful analogue of X11 XGetImage and is what makes the pixel diff a
    statement about what is ON SCREEN. PrintWindow(PW_RENDERFULLCONTENT) is
    offered as --printwindow because it survives occlusion, but gmpi_ui draws
    through DirectComposition and a DComp/swap-chain child commonly prints
    BLACK -- so a --printwindow capture that comes back uniform is the tool
    failing, not the plug-in freezing. The verb prints the distinct-colour count
    for exactly that reason; trust a diff only from a capture with many colours.

PPM is P6, which is what ppmdiff.py in this directory reads.
"""
import argparse
import ctypes
import sys
import time
from ctypes import wintypes

user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)

# Per-monitor-v2. Must happen before any geometry or DC call.
try:
    user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))
except Exception:
    try:
        ctypes.WinDLL("shcore").SetProcessDpiAwareness(2)
    except Exception:
        user32.SetProcessDPIAware()

SRCCOPY = 0x00CC0020
DIB_RGB_COLORS = 0
PW_RENDERFULLCONTENT = 0x00000002

MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_ABSOLUTE = 0x8000
# MOUSEEVENTF_VIRTUALDESK is NOT optional on a multi-monitor box, and leaving it
# out fails in a way that looks like the application misbehaving rather than the
# driver. ABSOLUTE alone maps 0..65535 onto the PRIMARY monitor, so a coordinate
# normalised against the virtual desktop is rescaled by primary_w/virtual_w on
# arrival. Measured here 2026-11-02: a right-click meant for client 860,470 of
# the editor (screen 892,603) landed at 1874,603 -- 892 * 3840/5760 + the
# virtual origin -- which is REAPER's ruler, and what came up was REAPER's own
# timeline context menu. The Y was correct throughout, because the virtual
# desktop is the same height as the primary monitor here; that is what makes
# this fail as "the plug-in offered the wrong menu" rather than as a miss.
MOUSEEVENTF_VIRTUALDESK = 0x4000
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_RIGHTDOWN = 0x0008
MOUSEEVENTF_RIGHTUP = 0x0010

SM_XVIRTUALSCREEN, SM_YVIRTUALSCREEN = 76, 77
SM_CXVIRTUALSCREEN, SM_CYVIRTUALSCREEN = 78, 79

VK = {
    "return": 0x0D, "enter": 0x0D, "escape": 0x1B, "esc": 0x1B, "tab": 0x09,
    "space": 0x20, "down": 0x28, "up": 0x26, "left": 0x25, "right": 0x27,
    "home": 0x24, "end": 0x23, "delete": 0x2E, "back": 0x08,
}


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG), ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG), ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]


def win_text(hwnd):
    n = user32.GetWindowTextLengthW(hwnd)
    buf = ctypes.create_unicode_buffer(n + 1)
    user32.GetWindowTextW(hwnd, buf, n + 1)
    return buf.value


def win_class(hwnd):
    buf = ctypes.create_unicode_buffer(256)
    user32.GetClassNameW(hwnd, buf, 256)
    return buf.value


def rect(hwnd):
    r = wintypes.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    return r.left, r.top, r.right - r.left, r.bottom - r.top


def client_origin(hwnd):
    p = wintypes.POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(p))
    return p.x, p.y


def client_size(hwnd):
    r = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    return r.right - r.left, r.bottom - r.top


ENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def children(hwnd):
    out = []

    def cb(h, _):
        out.append(h)
        return True

    user32.EnumChildWindows(hwnd, ENUMPROC(cb), 0)
    return out


def toplevels():
    out = []

    def cb(h, _):
        if user32.IsWindowVisible(h):
            out.append(h)
        return True

    user32.EnumWindows(ENUMPROC(cb), 0)
    return out


def describe(hwnd, depth=0):
    x, y, w, h = rect(hwnd)
    return "%s%d %dx%d+%d+%d [%s] %r" % (
        "  " * depth, hwnd, w, h, x, y, win_class(hwnd), win_text(hwnd)[:70])


def cmd_tree(a):
    """Every visible top-level and its children. The editor is a CHILD of the
    host's floating FX window on Windows, as it is under X11."""
    for t in toplevels():
        txt = win_text(t)
        if a.match and a.match.lower() not in txt.lower():
            continue
        print(describe(t))
        for c in children(t):
            if user32.IsWindowVisible(c):
                print(describe(c, 1))
    return 0


def cmd_geom(a):
    x, y, w, h = rect(a.hwnd)
    cx, cy = client_origin(a.hwnd)
    cw, ch = client_size(a.hwnd)
    print("hwnd %d window %dx%d+%d+%d client %dx%d at %d,%d class %s text %r"
          % (a.hwnd, w, h, x, y, cw, ch, cx, cy, win_class(a.hwnd),
             win_text(a.hwnd)))
    return 0


def grab(x, y, w, h, hwnd=None, printwindow=False):
    """BGRA bytes, top-down, for a screen rect -- or for a window via PrintWindow."""
    if printwindow:
        src = user32.GetWindowDC(hwnd)
    else:
        src = user32.GetDC(0)
    mem = gdi32.CreateCompatibleDC(src)
    bmp = gdi32.CreateCompatibleBitmap(src, w, h)
    gdi32.SelectObject(mem, bmp)
    if printwindow:
        ok = user32.PrintWindow(hwnd, mem, PW_RENDERFULLCONTENT)
    else:
        ok = gdi32.BitBlt(mem, 0, 0, w, h, src, x, y, SRCCOPY)
    if not ok:
        raise SystemExit("capture failed: %d" % ctypes.get_last_error())
    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(bi)
    bi.biWidth = w
    bi.biHeight = -h          # negative: top-down
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = 0
    buf = ctypes.create_string_buffer(w * h * 4)
    got = gdi32.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), DIB_RGB_COLORS)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(mem)
    if printwindow:
        user32.ReleaseDC(hwnd, src)
    else:
        user32.ReleaseDC(0, src)
    if got != h:
        raise SystemExit("GetDIBits returned %d of %d rows" % (got, h))
    return buf.raw


def cmd_shot(a):
    if a.hwnd:
        if a.printwindow:
            x, y = 0, 0
            w, h = client_size(a.hwnd)
        else:
            x, y = client_origin(a.hwnd)
            w, h = client_size(a.hwnd)
    else:
        x, y, w, h = [int(v) for v in a.rect.split(",")]
    bgra = grab(x, y, w, h, a.hwnd, a.printwindow)
    # BGRA -> RGB, and count distinct colours: a uniform frame means the
    # capture failed, which on this platform is the likely failure (see module
    # docstring) rather than a frozen plug-in.
    rgb = bytearray(w * h * 3)
    seen = set()
    for i in range(w * h):
        b, g, r = bgra[i * 4], bgra[i * 4 + 1], bgra[i * 4 + 2]
        rgb[i * 3] = r
        rgb[i * 3 + 1] = g
        rgb[i * 3 + 2] = b
        if len(seen) < 4096:
            seen.add((r, g, b))
    with open(a.out, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (w, h))
        f.write(bytes(rgb))
    print("wrote %s %dx%d from %d,%d distinct_colours=%s%d"
          % (a.out, w, h, x, y, ">=" if len(seen) >= 4096 else "", len(seen)))
    return 0


def to_screen(a, pt):
    x, y = [int(v) for v in pt.split(",")]
    if a.hwnd:
        ox, oy = client_origin(a.hwnd)
        return ox + x, oy + y
    return x, y


def move_abs(x, y):
    vx = user32.GetSystemMetrics(SM_XVIRTUALSCREEN)
    vy = user32.GetSystemMetrics(SM_YVIRTUALSCREEN)
    vw = user32.GetSystemMetrics(SM_CXVIRTUALSCREEN)
    vh = user32.GetSystemMetrics(SM_CYVIRTUALSCREEN)
    ax = int((x - vx) * 65535 / max(1, vw - 1))
    ay = int((y - vy) * 65535 / max(1, vh - 1))
    user32.mouse_event(
        MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK,
        ax, ay, 0, 0)
    # Verify rather than assume: a silent remap is the failure this whole
    # function exists to avoid, and the cursor is the cheapest witness.
    p = wintypes.POINT()
    user32.GetCursorPos(ctypes.byref(p))
    if abs(p.x - x) > 2 or abs(p.y - y) > 2:
        raise SystemExit("cursor landed at %d,%d, not %d,%d -- coordinate "
                         "mapping is wrong, refusing to click blind"
                         % (p.x, p.y, x, y))


MN_GETHMENU = 0x01E1
MF_GRAYED, MF_DISABLED, MF_CHECKED = 0x0001, 0x0002, 0x0008
MF_SEPARATOR, MF_POPUP = 0x0800, 0x0010


MIIM_STATE, MIIM_ID, MIIM_SUBMENU = 0x01, 0x02, 0x04
MIIM_DATA, MIIM_STRING, MIIM_FTYPE = 0x20, 0x40, 0x100
MFT_SEPARATOR, MFT_OWNERDRAW, MFT_RADIOCHECK = 0x800, 0x100, 0x200


class MENUITEMINFOW(ctypes.Structure):
    _fields_ = [
        ("cbSize", wintypes.UINT), ("fMask", wintypes.UINT),
        ("fType", wintypes.UINT), ("fState", wintypes.UINT),
        ("wID", wintypes.UINT), ("hSubMenu", wintypes.HMENU),
        ("hbmpChecked", wintypes.HBITMAP), ("hbmpUnchecked", wintypes.HBITMAP),
        ("dwItemData", ctypes.c_void_p), ("dwTypeData", wintypes.LPWSTR),
        ("cch", wintypes.UINT), ("hbmpItem", wintypes.HBITMAP),
    ]


def item_info(hmenu, i):
    """fType, fState, hSubMenu and the real label for item `i`.

    GetMenuStringW is NOT enough on this menu and the gap is specific: the
    SDK3/gmpi_ui side builds the submenu's PARENT as an OWNER-DRAW item, so
    there is no string in the menu for it and GetMenuStringW returns a length
    of 0 -- while the submenu's own leaves are ordinary string items and come
    back fine. Reading it as a separator (its old behaviour here) printed the
    `Wave points` row as `---`, which is why a measurement that had the right
    menu on screen still reported the option as not offered. 2026-11-02.
    """
    mi = MENUITEMINFOW()
    mi.cbSize = ctypes.sizeof(mi)
    mi.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_FTYPE
    user32.GetMenuItemInfoW(hmenu, i, True, ctypes.byref(mi))
    ftype, fstate, sub = mi.fType, mi.fState, mi.hSubMenu
    ln = user32.GetMenuStringW(hmenu, i, None, 0, 0x0400)
    label = ""
    if ln:
        buf = ctypes.create_unicode_buffer(ln + 1)
        user32.GetMenuStringW(hmenu, i, buf, ln + 1, 0x0400)
        label = buf.value
    return ftype, fstate, sub, label


def menu_items(hmenu, depth=0, out=None):
    """Every item of a live HMENU, with its checked state, recursively.

    This is the Windows leg's menu READOUT, and it is textual where the linux
    leg's is pixels: there, a menu is an override-redirect X window that can
    only be screenshotted and clicked by coordinate. Here a popup is a real
    `#32768` window, it answers MN_GETHMENU with its HMENU, and user32 then
    hands over the item strings and MF_CHECKED directly -- so "`Wave points`
    is offered, with 1024 ticked" is read as text rather than inferred from
    pixels, and the submenu does not even have to be opened to read it.
    """
    out = [] if out is None else out
    n = user32.GetMenuItemCount(hmenu)
    for i in range(n):
        ftype, fstate, sub, text = item_info(hmenu, i)
        flags = []
        if fstate & MF_CHECKED:
            flags.append("CHECKED")
        if fstate & (MF_GRAYED | MF_DISABLED):
            flags.append("disabled")
        if ftype & MFT_SEPARATOR:
            text = "---"
        elif ftype & MFT_OWNERDRAW and not text:
            # No string to read. Name it by what it CONTAINS rather than
            # printing an empty row -- a submenu is identified by its leaves.
            kids = []
            if sub:
                for j in range(min(3, user32.GetMenuItemCount(sub))):
                    kids.append(item_info(sub, j)[3])
            text = "<owner-draw, no label>" + (
                " containing %s..." % ",".join(k for k in kids if k) if kids else "")
        if sub:
            flags.append("submenu")
        out.append((depth, text, flags))
        if sub and depth < 4:
            menu_items(sub, depth + 1, out)
    return out


def find_submenu_with(hmenu, label, depth=0):
    """(hmenu, index) of the item whose SUBMENU directly contains `label`.

    This is how an owner-draw parent is reached: it has no string of its own,
    so it can only be named by its children -- and its rect is what has to be
    hovered before those children are displayed and clickable.
    """
    n = user32.GetMenuItemCount(hmenu)
    for i in range(n):
        sub = user32.GetSubMenu(hmenu, i)
        if not sub:
            continue
        for j in range(user32.GetMenuItemCount(sub)):
            if item_info(sub, j)[3].strip() == label:
                return hmenu, i
        if depth < 4:
            got = find_submenu_with(sub, label, depth + 1)
            if got:
                return got
    return None


def cmd_menu(a):
    popups = [h for h in toplevels() if win_class(h) == "#32768"]
    if not popups:
        print("no #32768 popup on screen")
        return 1
    for h in popups:
        x, y, w, ht = rect(h)
        hmenu = user32.SendMessageW(h, MN_GETHMENU, 0, 0)
        print("popup hwnd %d %dx%d+%d+%d hmenu %s" % (h, w, ht, x, y, hmenu))
        if not hmenu:
            print("  (MN_GETHMENU returned 0 -- not a standard menu)")
            continue
        for depth, text, flags in menu_items(hmenu):
            print("  %s%s%s" % ("    " * depth, text,
                                (" [" + ",".join(flags) + "]") if flags else ""))
    return 0


def find_item(hmenu, want, depth=0):
    """(hmenu, index) of the item whose text starts with `want`, searching
    submenus. Exact-ish by prefix, because an item may carry a trailing
    accelerator or ellipsis."""
    n = user32.GetMenuItemCount(hmenu)
    for i in range(n):
        text = item_info(hmenu, i)[3].strip()
        if text == want or text.startswith(want + "\t"):
            return hmenu, i
        sub = user32.GetSubMenu(hmenu, i)
        if sub and depth < 4:
            got = find_item(sub, want, depth + 1)
            if got:
                return got
    return None


def cmd_menupick(a):
    """Click the item named --item, by the rect the menu itself reports.

    GetMenuItemRect returns SCREEN coordinates for a live popup, so this needs
    no per-platform row-height arithmetic -- which is what the linux leg had to
    do (`row n of the submenu at (35, 17 + 24.8*n)`) because X11 gives it
    nothing better. Hovering the parent item first is what makes the submenu
    exist to be found on the second call.
    """
    popups = [h for h in toplevels() if win_class(h) == "#32768"]
    if not popups:
        print("no #32768 popup on screen")
        return 1
    for h in popups:
        hmenu = user32.SendMessageW(h, MN_GETHMENU, 0, 0)
        if not hmenu:
            continue
        got = find_item(hmenu, a.item)
        if not got:
            continue
        owner, idx = got
        r = wintypes.RECT()
        if not user32.GetMenuItemRect(0, owner, idx, ctypes.byref(r)):
            print("GetMenuItemRect failed for %r" % a.item)
            return 1
        cx = (r.left + r.right) // 2
        cy = (r.top + r.bottom) // 2
        move_abs(cx, cy)
        time.sleep(a.hover)
        if not a.hover_only:
            user32.mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
            time.sleep(0.05)
            user32.mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        print("%s %r at screen %d,%d" %
              ("hovered" if a.hover_only else "picked", a.item, cx, cy))
        return 0
    print("item %r not found in any open popup" % a.item)
    return 1


def cmd_click(a):
    x, y = to_screen(a, a.point)
    down = MOUSEEVENTF_RIGHTDOWN if a.right else MOUSEEVENTF_LEFTDOWN
    up = MOUSEEVENTF_RIGHTUP if a.right else MOUSEEVENTF_LEFTUP
    move_abs(x, y)
    time.sleep(0.08)
    for _ in range(2 if a.double else 1):
        user32.mouse_event(down, 0, 0, 0, 0)
        time.sleep(0.04)
        user32.mouse_event(up, 0, 0, 0, 0)
        time.sleep(0.04)
    print("click %s at screen %d,%d" % ("right" if a.right else "left", x, y))
    return 0


def cmd_drag(a):
    x1, y1 = to_screen(a, a.start)
    x2, y2 = to_screen(a, a.end)
    move_abs(x1, y1)
    time.sleep(0.15)
    user32.mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
    time.sleep(0.1)
    for i in range(1, a.steps + 1):
        move_abs(x1 + (x2 - x1) * i // a.steps, y1 + (y2 - y1) * i // a.steps)
        time.sleep(0.02)
    time.sleep(0.1)
    user32.mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
    print("drag %d,%d -> %d,%d in %d steps" % (x1, y1, x2, y2, a.steps))
    return 0


def cmd_key(a):
    for name in a.names:
        vk = VK.get(name.lower())
        if vk is None:
            if len(name) == 1:
                vk = user32.VkKeyScanW(ord(name)) & 0xFF
            else:
                raise SystemExit("unknown key %r" % name)
        user32.keybd_event(vk, 0, 0, 0)
        time.sleep(0.03)
        user32.keybd_event(vk, 0, 2, 0)
        time.sleep(0.05)
    print("keys %s" % " ".join(a.names))
    return 0


def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="verb", required=True)

    t = sub.add_parser("tree")
    t.add_argument("--match")
    t.set_defaults(fn=cmd_tree)

    g = sub.add_parser("geom")
    g.add_argument("--hwnd", type=int, required=True)
    g.set_defaults(fn=cmd_geom)

    s = sub.add_parser("shot")
    s.add_argument("--hwnd", type=int)
    s.add_argument("--rect")
    s.add_argument("--out", required=True)
    s.add_argument("--printwindow", action="store_true")
    s.set_defaults(fn=cmd_shot)

    m = sub.add_parser("menu")
    m.set_defaults(fn=cmd_menu)

    mp = sub.add_parser("menupick")
    mp.add_argument("--item", required=True)
    mp.add_argument("--hover", type=float, default=0.4)
    mp.add_argument("--hover-only", action="store_true")
    mp.set_defaults(fn=cmd_menupick)

    c = sub.add_parser("click")
    c.add_argument("point")
    c.add_argument("--hwnd", type=int)
    c.add_argument("--right", action="store_true")
    c.add_argument("--double", action="store_true")
    c.set_defaults(fn=cmd_click)

    d = sub.add_parser("drag")
    d.add_argument("start")
    d.add_argument("end")
    d.add_argument("--hwnd", type=int)
    d.add_argument("--steps", type=int, default=30)
    d.set_defaults(fn=cmd_drag)

    k = sub.add_parser("key")
    k.add_argument("names", nargs="+")
    k.set_defaults(fn=cmd_key)

    a = p.parse_args()
    if a.verb == "shot" and not a.hwnd and not a.rect:
        p.error("shot needs --hwnd or --rect")
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
