#!/usr/bin/env python3
"""BACKLOG E57 -- the delete key works, and a second regression fails a build.

WHY THIS EXISTS. Jeff, 2026-08-28: *"note the delete key is meant to work. so
bug may recur in future"*. E57's Accept was therefore written to run unattended
rather than to wait for someone to press a key. This is that check.

WHAT E57 WAS. Three defects between the key and the deletion, and fixing any two
left the key dead:

  A  GMPI_VIEW_CLASS had neither -acceptsFirstResponder nor -keyDown:, so macOS
     delivered NO key to the client at all. Windows/X11/Wayland all push keys
     from the window proc; macOS requires the opt-in.
  B  ViewBase::onKey had no delete case on ANY platform. SynthEdit binds it in
     its own HostedView, which TIDE does not build, so Windows would have failed
     too.
  C  AppKit reports arrows and forward-delete as Unicode private-use values
     (NSUpArrowFunctionKey = 0xF700..), while the shared layer switches on
     Windows VK codes. ESC and Apple-delete worked BY LUCK -- 0x1B and 0x7F/0x08
     collide with their VK values -- so a two-thirds fix looked total.

WHY IT IS NOT A GREP, AND WHY THAT MATTERS MORE THAN THE CHECKS.

The costliest hour of E57 went to a patch anchored inside `#if !USE_BACKING_BUFFER`.
It COMPILED OUT ENTIRELY. The build was rc=0, the file compiled from the right
path into two targets, and the app beeped -- which is indistinguishable from a
handler that ran and declined. A plain `grep keyDown:` would have PASSED on that
broken tree.

So this probe does two things a grep cannot:

  1. IT STRIPS COMMENTS FIRST. E57's fix is heavily commented and the comments
     name every constant the code uses. Matching raw text would keep passing
     after someone deleted the code and left the comment behind.
  2. IT COMPUTES PREPROCESSOR NESTING DEPTH and requires each guarded construct
     to sit at the SAME #if depth as its enclosing function or @implementation
     -- that is, to be unconditional relative to the thing that contains it.
     Moving the fix inside any #if raises its depth and fails here.

That second rule is a PROXY, stated plainly because a proxy sold as a proof is
how E57 got expensive: this script does not evaluate macros and cannot know
whether a given #if is active. It demands the code be unconditional instead,
which is what the fix deliberately made it. If a future change needs it
conditional, this check should fail and a human should decide -- that is the
intended behaviour, not a false positive.

WHAT IS DELIBERATELY NOT HERE. A tier that inspects a BUILT binary's Objective-C
metadata would prove compilation directly rather than by proxy, and is the
natural companion to the lesson above. It is absent because there was no macOS
build on the box when this was written, and a check whose passing has never been
observed -- nor its failing -- is not evidence. Adding it is worth doing when a
build is at hand; see E57's row.

USAGE

    python3 tests/e57_delete_key_probe.py --deps <build-dir>/_deps
    python3 tests/e57_delete_key_probe.py --gmpi-ui <dir> --syntheditlib <dir>
    python3 tests/e57_delete_key_probe.py --selftest

`--deps` is the convenience form: TideSynth pulls both repos with FetchContent,
so they land as `_deps/gmpi_ui-src` and `_deps/syntheditlib-src`.

Exit codes: 0 all checks passed - 1 a check failed - 2 bad configuration.
"""

import argparse
import pathlib
import re
import sys

# --- source hygiene -------------------------------------------------------

def strip_comments(text):
    """Blank out comments, PRESERVING line count and column positions.

    Comments are replaced with spaces rather than removed so that every line
    number this script reports still matches the file a human will open.
    String and char literals are honoured so a "//" inside one is not mistaken
    for a comment -- DrawingFrameMac.mm has none today, but a check that reads
    source has to survive the source changing.
    """
    out = []
    i, n = 0, len(text)
    state = None   # None | 'line' | 'block' | 'str' | 'chr'
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if state is None:
            if c == '/' and nxt == '/':
                state = 'line'; out.append('  '); i += 2; continue
            if c == '/' and nxt == '*':
                state = 'block'; out.append('  '); i += 2; continue
            if c == '"':
                state = 'str'
            elif c == "'":
                state = 'chr'
            out.append(c); i += 1; continue
        if state == 'line':
            if c == '\n':
                state = None; out.append('\n')
            else:
                out.append(' ')
            i += 1; continue
        if state == 'block':
            if c == '*' and nxt == '/':
                state = None; out.append('  '); i += 2; continue
            out.append('\n' if c == '\n' else ' '); i += 1; continue
        # inside a literal
        if c == '\\':
            out.append(c); i += 1
            if i < n:
                out.append(text[i]); i += 1
            continue
        if (state == 'str' and c == '"') or (state == 'chr' and c == "'"):
            state = None
        out.append(c); i += 1
    return ''.join(out)


COND_OPEN = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
COND_MID = re.compile(r'^\s*#\s*(elif|else)\b')
COND_END = re.compile(r'^\s*#\s*endif\b')


def conditional_depths(lines):
    """Depth of #if nesting for each line. A line inside one #if has depth 1.

    The #if/#endif lines themselves report the depth OUTSIDE their own block,
    so `#if X` on a file's top level is depth 0 and its contents are depth 1.
    """
    depths, depth = [], 0
    for ln in lines:
        if COND_OPEN.match(ln):
            depths.append(depth); depth += 1
        elif COND_END.match(ln):
            depth = max(0, depth - 1); depths.append(depth)
        elif COND_MID.match(ln):
            depths.append(max(0, depth - 1))
        else:
            depths.append(depth)
    return depths


class Source:
    def __init__(self, path):
        self.path = path
        raw = path.read_text(encoding='utf-8', errors='replace')
        self.text = strip_comments(raw)
        self.lines = self.text.split('\n')
        self.depths = conditional_depths(self.lines)

    def find(self, pattern, start=0, end=None):
        """First line index matching `pattern`, or None."""
        rx = re.compile(pattern)
        stop = len(self.lines) if end is None else end
        for i in range(start, stop):
            if rx.search(self.lines[i]):
                return i
        return None

    def find_all(self, pattern, start=0, end=None):
        rx = re.compile(pattern)
        stop = len(self.lines) if end is None else end
        return [i for i in range(start, stop) if rx.search(self.lines[i])]

    def brace_block(self, start):
        """Range of the {...} block opening at or after line `start`."""
        depth, began, i = 0, False, start
        while i < len(self.lines):
            for ch in self.lines[i]:
                if ch == '{':
                    depth += 1; began = True
                elif ch == '}':
                    depth -= 1
            if began and depth <= 0:
                return start, i
            i += 1
        return start, len(self.lines) - 1


# --- reporting ------------------------------------------------------------

class Report:
    def __init__(self):
        self.rows = []

    def add(self, ok, name, detail):
        self.rows.append((ok, name, detail))
        print(f"  {'PASS' if ok else 'FAIL'}  {name}\n        {detail}")

    def failed(self):
        return [r for r in self.rows if not r[0]]


def check_depth(rep, src, idx, enclosing_depth, name, what):
    """`what` must be no more deeply #if-nested than the thing containing it."""
    d = src.depths[idx]
    if d == enclosing_depth:
        rep.add(True, name, f"{src.path.name}:{idx + 1} unconditional (#if depth {d})")
        return True
    rep.add(False, name,
            f"{src.path.name}:{idx + 1} {what} sits at #if depth {d}, enclosing "
            f"construct is at {enclosing_depth}. Conditional code can compile out "
            f"silently -- that is the E57 trap. Make it unconditional, or change "
            f"this probe deliberately.")
    return False


# --- the checks -----------------------------------------------------------

def check_mac_backend(rep, root):
    path = root / 'backends' / 'DrawingFrameMac.mm'
    if not path.is_file():
        rep.add(False, 'A/C  mac backend present', f"missing: {path}")
        return
    src = Source(path)

    impl = src.find(r'@implementation\s+GMPI_VIEW_CLASS')
    if impl is None:
        rep.add(False, 'A  @implementation GMPI_VIEW_CLASS',
                f"not found in {path.name} -- the class the fix hangs on is gone or renamed")
        return
    end = src.find(r'^\s*@end\b', impl) or len(src.lines)
    base = src.depths[impl]
    rep.add(True, 'A  @implementation GMPI_VIEW_CLASS',
            f"{path.name}:{impl + 1} (#if depth {base})")

    afr = src.find(r'-\s*\(BOOL\)\s*acceptsFirstResponder', impl, end)
    if afr is None:
        rep.add(False, 'A1 -acceptsFirstResponder',
                "absent. Without it the view never becomes first responder and macOS "
                "delivers no key to the client at all (E57 defect A).")
    else:
        check_depth(rep, src, afr, base, 'A1 -acceptsFirstResponder', '-acceptsFirstResponder')

    kd = src.find(r'-\s*\(void\)\s*keyDown:', impl, end)
    if kd is None:
        rep.add(False, 'A2 -keyDown:',
                "absent. Windows/X11/Wayland push keys from the window proc; macOS "
                "needs this override (E57 defect A).")
        return
    if not check_depth(rep, src, kd, base, 'A2 -keyDown:', '-keyDown:'):
        return

    # C -- the AppKit function-key translation.
    kd_start, kd_end = src.brace_block(kd)
    wanted = {
        'NSUpArrowFunctionKey': '0x26',
        'NSDownArrowFunctionKey': '0x28',
        'NSLeftArrowFunctionKey': '0x25',
        'NSRightArrowFunctionKey': '0x27',
        'NSDeleteFunctionKey': '0x2E',
    }
    missing, miscoded = [], []
    for key, vk in wanted.items():
        hit = src.find(rf'case\s+{key}\s*:', kd_start, kd_end + 1)
        if hit is None:
            missing.append(key)
            continue
        if not re.search(rf'=\s*{vk}\b', src.lines[hit], re.IGNORECASE):
            miscoded.append(f"{key} -> expected {vk}, line reads: {src.lines[hit].strip()}")
    if missing or miscoded:
        detail = []
        if missing:
            detail.append("not mapped: " + ", ".join(missing))
        if miscoded:
            detail.extend(miscoded)
        detail.append("AppKit sends these as private-use values (0xF700..); unmapped "
                      "they never match the VK codes ViewBase::onKey switches on "
                      "(E57 defect C).")
        rep.add(False, 'C  AppKit function-key translation', " | ".join(detail))
    else:
        rep.add(True, 'C  AppKit function-key translation',
                f"{path.name}:{kd + 1} all 5 keys map to their VK codes")


def check_shared_view(rep, root):
    path = root / 'modules' / 'se_sdk3_hosting' / 'ViewBase.cpp'
    if not path.is_file():
        rep.add(False, 'B  shared view present', f"missing: {path}")
        return
    src = Source(path)

    fn = src.find(r'ViewBase::onKey\s*\(')
    if fn is None:
        rep.add(False, 'B  ViewBase::onKey', f"not found in {path.name}")
        return
    base = src.depths[fn]
    fn_start, fn_end = src.brace_block(fn)

    call = src.find(r'DeleteSelection\s*\(', fn_start, fn_end + 1)
    if call is None:
        rep.add(False, 'B  delete binding',
                f"ViewBase::onKey ({path.name}:{fn + 1}) never calls DeleteSelection. "
                "The key arrives and nothing happens (E57 defect B).")
        return
    if not check_depth(rep, src, call, base, 'B1 DeleteSelection call', 'the DeleteSelection call'):
        return

    # Walk back from the call over the run of case labels that fall into it.
    labels, i = set(), call - 1
    case_rx = re.compile(r'case\s+(0x[0-9A-Fa-f]+)\s*:')
    while i > fn_start:
        line = src.lines[i].strip()
        if not line:
            i -= 1; continue
        m = case_rx.search(line)
        if not m:
            break
        if src.depths[i] != base:
            rep.add(False, 'B2 delete key codes',
                    f"{path.name}:{i + 1} case label is #if-nested at depth "
                    f"{src.depths[i]} (enclosing {base})")
            return
        labels.add(m.group(1).lower())
        i -= 1

    required = {'0x08': 'backspace, the macOS "delete" key',
                '0x2e': 'VK_DELETE, from the mac backend\'s NSDeleteFunctionKey mapping',
                '0x7f': 'forward delete, and what SynthEdit\'s HostedView sends'}
    missing = {k: v for k, v in required.items() if k not in labels}
    if missing:
        rep.add(False, 'B2 delete key codes',
                "not bound to DeleteSelection: "
                + "; ".join(f"{k} ({v})" for k, v in missing.items())
                + f". Found: {sorted(labels) or 'none'}. All three producers were "
                  "OBSERVED arriving during E57 -- dropping one silently breaks a "
                  "keyboard or a platform.")
    else:
        rep.add(True, 'B2 delete key codes',
                f"{path.name}:{call + 1} 0x08, 0x2E and 0x7F all reach DeleteSelection")


def check_presenter(rep, root):
    path = root / 'modules' / 'se_sdk3_hosting' / 'Presenter.h'
    if not path.is_file():
        rep.add(False, 'D  Presenter.h present', f"missing: {path}")
        return
    src = Source(path)
    hit = src.find(r'\bDeleteSelection\s*\(')
    if hit is None:
        rep.add(False, 'D  Presenter::DeleteSelection',
                f"not declared in {path.name}. The view's entry point into the "
                "presenter is gone; ViewBase::onKey cannot compile against it.")
    else:
        rep.add(True, 'D  Presenter::DeleteSelection', f"{path.name}:{hit + 1} declared")


# --- selftest -------------------------------------------------------------

SELFTEST_MM = """
#define GMPI_VIEW_CLASS Foo
@implementation GMPI_VIEW_CLASS
// - (BOOL)acceptsFirstResponder { return YES; }   <- a comment must not count
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent*)e
{
    switch (c)
    {
    case NSUpArrowFunctionKey:    c = 0x26; break;
    case NSDownArrowFunctionKey:  c = 0x28; break;
    case NSLeftArrowFunctionKey:  c = 0x25; break;
    case NSRightArrowFunctionKey: c = 0x27; break;
    case NSDeleteFunctionKey:     c = 0x2E; break;
    }
}
@end
"""


def selftest():
    """Prove the two mechanisms this probe rests on, since both are subtle."""
    ok = True

    stripped = strip_comments(SELFTEST_MM)
    if '<- a comment must not count' in stripped:
        print("  FAIL  strip_comments left comment text behind"); ok = False
    elif stripped.count('\n') != SELFTEST_MM.count('\n'):
        print("  FAIL  strip_comments changed the line count"); ok = False
    else:
        print("  PASS  strip_comments blanks comments and preserves line numbers")

    lines = ['int a;', '#if FOO', 'int b;', '#if BAR', 'int c;', '#endif', '#endif', 'int d;']
    got = conditional_depths(lines)
    want = [0, 0, 1, 1, 2, 1, 0, 0]
    if got != want:
        print(f"  FAIL  conditional_depths -> {got}, want {want}"); ok = False
    else:
        print("  PASS  conditional_depths tracks nesting (the E57 compiled-out trap)")

    return 0 if ok else 1


# --- entry point ----------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--deps', help='a CMake _deps dir holding gmpi_ui-src and syntheditlib-src')
    ap.add_argument('--gmpi-ui', help='gmpi_ui checkout')
    ap.add_argument('--syntheditlib', help='SynthEditLib checkout')
    ap.add_argument('--selftest', action='store_true', help='test this script, not the tree')
    args = ap.parse_args()

    if args.selftest:
        print("E57 delete-key probe -- selftest")
        return selftest()

    gmpi_ui, selib = args.gmpi_ui, args.syntheditlib
    if args.deps:
        deps = pathlib.Path(args.deps)
        gmpi_ui = gmpi_ui or str(deps / 'gmpi_ui-src')
        selib = selib or str(deps / 'syntheditlib-src')
    if not gmpi_ui or not selib:
        print("need --deps, or both --gmpi-ui and --syntheditlib", file=sys.stderr)
        return 2
    for label, p in (('gmpi_ui', gmpi_ui), ('SynthEditLib', selib)):
        if not pathlib.Path(p).is_dir():
            print(f"{label} path is not a directory: {p}", file=sys.stderr)
            return 2

    print("E57 -- the delete key must keep working (BACKLOG E57)")
    print(f"  gmpi_ui      {gmpi_ui}")
    print(f"  SynthEditLib {selib}\n")

    rep = Report()
    check_mac_backend(rep, pathlib.Path(gmpi_ui))
    check_shared_view(rep, pathlib.Path(selib))
    check_presenter(rep, pathlib.Path(selib))

    bad = rep.failed()
    print()
    if bad:
        print(f"FAILED -- {len(bad)} of {len(rep.rows)} checks. The delete key is "
              "very likely broken again; see E57 in BACKLOG-DONE.md for what each "
              "defect looked like from the user's side.")
        return 1
    print(f"OK -- {len(rep.rows)} checks. Delete reaches the presenter on macOS "
          "and on the shared path.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
