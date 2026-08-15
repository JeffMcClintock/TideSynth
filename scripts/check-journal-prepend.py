#!/usr/bin/env python3
"""Check that a JOURNAL.md change is a pure prepend, or a verbatim rotation.

Written for A3 (2026-08-13). The invariant this enforces is JOURNAL.md's own
rule: "Nothing is ever deleted or rewritten — old entries just move to a
per-month archive." Two edits satisfy that:

  1. Prepend: one or more new "## YYYY-MM-DD — ..." entries added at the top.
     Every entry that existed in the base version must still be present in
     the head version, byte-for-byte.
  2. Rotation (A8): entries move OUT of JOURNAL.md, but only if each moved
     entry reappears byte-for-byte somewhere in another file this same PR
     touched (the month's JOURNAL-YYYY-MM.md archive).

Anything else — an existing entry edited in place, reordered, or dropped with
no trace anywhere in the diff — fails. The header/template block above the
first real entry is not checked: A8 itself edited it (to add the rotation
rule), which is a rare, deliberate, human-reviewed change this script is not
trying to gate.

    python scripts/check-journal-prepend.py <base-journal> <head-journal> \
        [--repo-root DIR] [--changed-file PATH ...]

<base-journal> and <head-journal> are file paths (the CI step extracts the
base version with `git show`). --repo-root defaults to the current directory
and is where a rotated-out entry is searched for; pass --changed-file to
restrict that search to files this PR actually touched, so a rotation isn't
credited to an archive file that already happened to contain matching text
for an unrelated reason.
"""
import argparse
import os
import re
import sys

# A real entry: "## " + an actual date, not the template's "## YYYY-MM-DD".
ENTRY = re.compile(r'^## (\d{4}-\d{2}-\d{2}) — .+$', re.MULTILINE)

# The "\n\n---\n\n" that separates one entry from the next. Only meaningful
# between two entries -- it is not part of either entry's own content.
TRAILING_SEPARATOR = re.compile(r'\n+-{3,}\s*$')


def split_entries(text):
    """Return the list of entry blocks (heading through the char before the
    next heading, or EOF), each including its own trailing blank lines."""
    starts = [m.start() for m in ENTRY.finditer(text)]
    if not starts:
        return []
    bounds = starts + [len(text)]
    return [text[bounds[i]:bounds[i + 1]] for i in range(len(starts))]


def canonical(entry):
    """An entry's content, independent of whether it happens to be last.

    A block runs from its own heading to the next heading (or EOF) -- see
    split_entries. That means the SAME entry's raw text differs depending on
    whether another entry still follows it: with a follower, the block ends
    in the "\\n\\n---\\n\\n" that separates it from that follower; at EOF, it
    does not. Comparing raw blocks therefore fails for any entry that USED TO
    have a follower and, after a rotation, no longer does -- which is every
    entry immediately above the one just rotated out, i.e. it would fail on
    the very next rotation after any rotation. Found 2026-08-15, first hit by
    the C12b run archiving A4: the entry rotated out (A4, already last in
    both versions) verified fine; C5, staying put but newly promoted to last
    by A4's departure, did not, though nothing about C5 had changed.

    Stripping the trailing separator (if any) and trailing whitespace makes
    the two shapes comparable, so this is applied uniformly before any
    equality or containment check -- not just at file boundaries.
    """
    return TRAILING_SEPARATOR.sub('', entry).rstrip()


def find_verbatim(entry, search_texts):
    # canonical(), not a raw rstrip: an archived entry's own trailing
    # separator (if it is not the last thing in the archive either) must not
    # block the match, for the same reason canonical() exists at all.
    stripped = canonical(entry)
    return any(stripped in t for t in search_texts)


def check(base_text, head_text, search_texts=()):
    """Return (ok, output_lines) for a base/head JOURNAL.md pair. No I/O."""
    base_entries = split_entries(base_text)
    head_entries = split_entries(head_text)

    if not base_entries:
        return True, ['base JOURNAL.md has no dated entries — nothing to check against']

    head_set = set(canonical(e) for e in head_entries)

    missing, rotated = [], []
    for entry in base_entries:
        key = canonical(entry)
        if key in head_set:
            continue
        if find_verbatim(entry, search_texts):
            rotated.append(entry.split('\n', 1)[0])
        else:
            missing.append(entry.split('\n', 1)[0])

    base_set = set(canonical(e) for e in base_entries)
    new_entries = [e for e in head_entries if canonical(e) not in base_set]

    dates = [m.group(1) for e in head_entries for m in [ENTRY.match(e)] if m]
    out_of_order = dates != sorted(dates, reverse=True)

    lines = []
    if rotated:
        lines.append('%d entr%s rotated out, verified verbatim elsewhere in the diff:' %
                     (len(rotated), 'y' if len(rotated) == 1 else 'ies'))
        lines += ['  ' + t for t in rotated]
    if new_entries:
        lines.append('%d new entr%s prepended' % (len(new_entries), 'y' if len(new_entries) == 1 else 'ies'))

    ok = True
    if missing:
        ok = False
        lines.append('\n%d entr%s from the base version MISSING from head, with no verbatim '
                     'copy in any other file in the diff:' % (len(missing), 'y' if len(missing) == 1 else 'ies'))
        lines += ['  ' + t for t in missing]
        lines.append('An entry may move to an archive file verbatim (rotation), or stay. '
                     'It may not be edited or silently dropped.')
    if out_of_order:
        ok = False
        lines.append('\nJOURNAL.md entries are not newest-first: ' + ' > '.join(dates))

    lines.append('\nJOURNAL.md: prepend-only, OK' if ok else '')
    return ok, lines


# --- Self-test -----------------------------------------------------------
# Each case is a (base, head, search_texts, expect_ok, description) tuple.
# Entries are built from a template so every case shares the same real
# heading/separator shape rather than a simplified stand-in.

def _entry(date, title, body):
    return '## %s — %s\n\n%s\n' % (date, title, body)


E1 = _entry('2026-08-15', 'windows — newest', 'first line.\nsecond line.')
E2 = _entry('2026-08-14', 'windows — middle', 'the middle entry, unedited.')
E3 = _entry('2026-08-13', 'linux — oldest', 'the oldest entry, about to rotate.')
SEP = '\n---\n\n'

SELFTEST_CASES = [
    (E2 + SEP + E3, E1 + SEP + E2 + SEP + E3, [],
     True, 'plain prepend, nothing rotated'),

    (E2 + SEP + E3, E1 + SEP + E2, [E3],
     True, 'oldest (already-last) entry rotated to an archive, verified'),

    # The real bug, reproduced exactly: E2 has a follower (E3) in base, and
    # loses it in head because E3 rotated out. E2 itself is byte-identical
    # content in both -- only its trailing separator differs, structurally,
    # because it is no longer followed by anything.
    (E1 + SEP + E2 + SEP + E3, E1 + SEP + E2, [E3],
     True, 'entry promoted to last by a sibling rotating out -- the 2026-08-15 bug'),

    (E2 + SEP + E3, E1 + SEP + E2.replace('unedited', 'quietly edited') + SEP + E3, [],
     False, 'an existing entry edited in place must fail'),

    (E2 + SEP + E3, E1 + SEP + E2, [],
     False, 'oldest entry dropped with no archive copy must fail'),

    (E3 + SEP + E2, E1 + SEP + E3 + SEP + E2, [],
     False, 'new entry prepended out of date order must fail'),

    ('', E1, [], True, 'no dated entries in base -- nothing to check'),
]


def selftest():
    failures = 0
    for base, head, search_texts, expect_ok, description in SELFTEST_CASES:
        ok, _ = check(base, head, search_texts)
        if ok != expect_ok:
            failures += 1
            print('FAIL  %s (expected ok=%s, got ok=%s)' % (description, expect_ok, ok))
    print('%d case(s), %d failed' % (len(SELFTEST_CASES), failures))
    return 1 if failures else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('base', nargs='?')
    ap.add_argument('head', nargs='?')
    ap.add_argument('--repo-root', default='.')
    ap.add_argument('--changed-file', action='append', default=[])
    ap.add_argument('--selftest', action='store_true')
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.base or not args.head:
        ap.error('base and head are required unless --selftest is given')

    with open(args.base, encoding='utf-8') as f:
        base_text = f.read()
    with open(args.head, encoding='utf-8') as f:
        head_text = f.read()

    # Other files this PR touched, to credit a rotation. Falls back to every
    # .md file under repo-root if the caller did not say which files changed.
    if args.changed_file:
        search_paths = [p for p in args.changed_file
                         if os.path.abspath(p) != os.path.abspath(args.head)]
    else:
        search_paths = []
        for dirpath, dirnames, filenames in os.walk(args.repo_root):
            dirnames[:] = [d for d in dirnames if d not in ('.git', 'node_modules', 'build', '_deps')]
            for fn in filenames:
                if fn.endswith('.md'):
                    p = os.path.join(dirpath, fn)
                    if os.path.abspath(p) != os.path.abspath(args.head):
                        search_paths.append(p)

    search_texts = []
    for p in search_paths:
        try:
            with open(p, encoding='utf-8') as f:
                search_texts.append(f.read())
        except OSError:
            pass

    ok, lines = check(base_text, head_text, search_texts)
    for line in lines:
        print(line)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
