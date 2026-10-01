#!/usr/bin/env python3
"""E72 -- measure which editor edits can set `dspDirty`, and whether a rack
patch-cable edit is one of them.

BACKLOG E72 asserts, from a code reading, that
`MfcDocPresenter::AddPatchCable` and `RemovePatchCable` "do a bare
`patchManager->setParameterValue(...)` and construct no `SuspendDSP` guard, so
they never reach `invalidateDsp()` and never set `dspDirty`".  E72 says so in as
many words: "the finding, read rather than measured".

This probe turns that reading into a measurement.  It enumerates, from
SynthEditLib's own source, every site that can set the flag, and then asks of
each structural-edit entry point whether it is guarded -- with a POSITIVE
CONTROL in the same table, which is the part that makes an unguarded answer a
fact about the cable path rather than a fact about this parser.

Why the control matters.  "AddPatchCable has no SuspendDSP in it" is also what
you would print if the pattern were simply wrong, or if the body extraction
stopped at the first brace.  `ConnectPlugs` (plug4.cpp) is the structure view's
equivalent operation -- the user drawing a wire between two module pins -- and it
IS guarded.  Same parser, same file set, opposite answer, so the zero is a
measurement.  The fleet has paid for this lesson twice (2026-09-09 E83: "put the
invariant in the same trace as the variable").

What this probe does NOT establish, stated rather than glossed: that the flag's
absence changes any observable behaviour.  That is E72's own Accept -- recreate
the processor after a cable edit with no save in between -- and it wants a GUI to
draw the cable with.  This measures the mechanism, not the consequence.

Why it is safe to run from a scheduled run: it READS SynthEditLib and writes
nothing.  SynthEditLib is GATED by STEP 5 and reading it has never needed
permission.  Source is taken from `git show <rev>:<path>` rather than the
working tree, so a developer's uncommitted work cannot change the answer.

Run:   python3 tests/e72_dsp_dirty_probe.py [--repo PATH] [--rev REV] [--verbose]
Exit:  0 if every site behaves as E72's reading records
       1 if the guard placement has MOVED -- someone has guarded the cable path,
         or unguarded a control, and E72 wants re-reading before anyone acts
       2 if the SynthEditLib source could not be read at all (a skip, not a
         pass: say so rather than reporting a green table nobody measured)
"""
import argparse
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_REPO = os.path.join(os.path.dirname(os.path.dirname(HERE)), 'SynthEditLib')

# Every file that mentions the flag or the guard, as of 2026-09-15.  Listed
# explicitly rather than globbed so that a file appearing or vanishing is
# visible in a diff of THIS file instead of silently changing the answer.
SOURCES = [
    'EditorLib/Application.h',
    'EditorLib/CContainer.cpp',
    'EditorLib/CUG.cpp',
    'EditorLib/DocOb.cpp',
    'EditorLib/MfcDocPresenter.cpp',
    'EditorLib/PatchParameter.cpp',
    'EditorLib/SuspendDSP.cpp',
    'EditorLib/SynthEditAppBase.h',
    'EditorLib/UG2.cpp',
    'EditorLib/plug4.cpp',
]

# A SuspendDSP construction -- `SuspendDSP x(...)`.  Deliberately NOT matching
# `SuspendDSP::` (the definition) or the `#include`.
RE_GUARD = re.compile(r'\bSuspendDSP\s+[A-Za-z_]\w*\s*\(')
# A direct call.  `->invalidateDsp()` or `.invalidateDsp()`, never the
# declaration (`virtual void invalidateDsp`) or the override definition.
RE_DIRECT = re.compile(r'(?:->|\.)\s*invalidateDsp\s*\(\s*\)')

# The entry points this probe asks about.  `guarded` is what E72's reading
# says the answer is today; a mismatch is what exits 1.
#
# The first two are the subject.  The rest are controls, and each is a
# structural edit a user performs from the editor, so "the cable path is the
# odd one out" is a comparison rather than an assertion.
CASES = [
    ('MfcDocPresenter.cpp', 'MfcDocPresenter::AddPatchCable',    False, 'subject -- rack patch cable added'),
    ('MfcDocPresenter.cpp', 'MfcDocPresenter::RemovePatchCable', False, 'subject -- rack patch cable removed'),
    ('plug4.cpp',           'ConnectPlugs',                      True,  'CONTROL -- structure-view wire drawn'),
]


def read_source(repo, rev, path):
    """Read one file at `rev`.  Returns text, or None if unavailable."""
    try:
        out = subprocess.run(
            ['git', '-C', repo, 'show', '%s:%s' % (rev, path)],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except OSError:
        return None
    if out.returncode != 0:
        return None
    return out.stdout.decode('utf-8', 'replace')


def strip_dead_code(text):
    """Remove `#if 0` ... `#endif` blocks.

    This is not pedantry.  E72 cites "the RAII guard that does [set the flag] is
    `SuspendDSP.cpp:27`" -- and line 27 is `m_app->dspDirty = true;`, which sits
    inside an `#if 0` block and has not compiled for as long as it has been
    there.  The live line is `SuspendDSP.cpp:7`, `p_app->invalidateDsp()`.  A
    probe that counted the dead line would confirm E72's conclusion by way of a
    line that does not exist, which is the shape of every false green the fleet
    has recorded.
    """
    out, depth = [], 0
    for line in text.split('\n'):
        s = line.strip()
        if depth:
            if re.match(r'#\s*if', s):
                depth += 1
            elif re.match(r'#\s*endif', s):
                depth -= 1
            out.append('')          # keep line numbering intact
            continue
        if re.match(r'#\s*if\s+0\b', s):
            depth = 1
            out.append('')
            continue
        out.append(line)
    return '\n'.join(out)


def function_body(text, name):
    """Return (start_line, body) for the DEFINITION of `name`, else (None, None).

    Finds the name, skips to the `{` that opens its body, and brace-matches to
    the close.  Declarations (`;` before any `{`) are skipped, so a header-style
    forward declaration in the same file cannot be mistaken for the definition.
    """
    for m in re.finditer(re.escape(name) + r'\s*\(', text):
        i = text.find('(', m.start())
        depth, j = 1, i + 1
        while j < len(text) and depth:                 # step over the arg list
            depth += (text[j] == '(') - (text[j] == ')')
            j += 1
        k = j
        while k < len(text) and text[k] not in '{;':   # what follows it?
            k += 1
        if k >= len(text) or text[k] == ';':
            continue                                   # a declaration, not a body
        depth, e = 1, k + 1
        while e < len(text) and depth:
            depth += (text[e] == '{') - (text[e] == '}')
            e += 1
        return text[:m.start()].count('\n') + 1, text[k:e]
    return None, None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--repo', default=DEFAULT_REPO,
                    help='SynthEditLib checkout (default: %(default)s)')
    ap.add_argument('--rev', default='origin/main',
                    help='revision to read, NOT the working tree (default: %(default)s)')
    ap.add_argument('--verbose', action='store_true')
    args = ap.parse_args()

    sources = {}
    for path in SOURCES:
        text = read_source(args.repo, args.rev, path)
        if text is None:
            print('SKIP: cannot read %s:%s from %s' % (args.rev, path, args.repo))
            print('This is a skip, not a pass. E72 is unmeasured on this box.')
            return 2
        sources[path] = strip_dead_code(text)

    # 1. Every site that can set the flag.
    sites = []
    for path, text in sources.items():
        for n, line in enumerate(text.split('\n'), 1):
            if RE_GUARD.search(line):
                sites.append((path, n, 'SuspendDSP guard', line.strip()))
            elif RE_DIRECT.search(line) and 'SuspendDSP.cpp' not in path:
                sites.append((path, n, 'invalidateDsp() direct', line.strip()))
    sites.sort()

    print('## Every site that can set `dspDirty`, %s at %s\n' % (
        os.path.basename(args.repo.rstrip('/')), args.rev))
    print('| file | line | kind |')
    print('|---|---|---|')
    for path, n, kind, _ in sites:
        print('| `%s` | %d | %s |' % (path, n, kind))
    guards = sum(1 for s in sites if s[2] == 'SuspendDSP guard')
    direct = len(sites) - guards
    print('\n**%d sites total** -- %d `SuspendDSP` constructions and %d direct '
          '`invalidateDsp()` calls.\n' % (len(sites), guards, direct))

    # 2. Is the cable path one of them?
    print('## Is a given editor edit guarded?\n')
    print('| entry point | role | guarded | expected |')
    print('|---|---|---|---|')
    failures = []
    for filename, name, expected, role in CASES:
        path = next(p for p in sources if p.endswith(filename))
        start, body = function_body(sources[path], name)
        if body is None:
            failures.append('%s: definition not found in %s' % (name, path))
            print('| `%s` | %s | **NOT FOUND** | %s |' % (name, role, expected))
            continue
        found = bool(RE_GUARD.search(body) or RE_DIRECT.search(body))
        ok = (found == expected)
        if not ok:
            failures.append('%s: guarded=%s, E72 records %s' % (name, found, expected))
        print('| `%s` (%s:%d) | %s | %s | %s |' % (
            name, path.split('/')[-1], start, role,
            'yes' if found else '**no**', 'yes' if expected else 'no'))
        if args.verbose:
            print('\n```\n%s\n```\n' % body.strip()[:1200])

    if failures:
        print('\n%d case(s) did NOT behave as E72 recorded:' % len(failures))
        for f in failures:
            print('  - %s' % f)
        print("\nThe guard placement has MOVED. Re-read E72 before acting on it.")
        return 1

    print("\nEvery case behaves as E72's reading says. The rack patch-cable path "
          "sets no dirty flag,\nand the structure-view control in the same table "
          "does -- so the omission is specific to\npatch cables, not an artifact "
          "of this parser.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
