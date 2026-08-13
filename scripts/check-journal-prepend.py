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


def split_entries(text):
    """Return the list of entry blocks (heading through the char before the
    next heading, or EOF), each including its own trailing blank lines."""
    starts = [m.start() for m in ENTRY.finditer(text)]
    if not starts:
        return []
    bounds = starts + [len(text)]
    return [text[bounds[i]:bounds[i + 1]] for i in range(len(starts))]


def find_verbatim(entry, search_texts):
    stripped = entry.rstrip('\n')
    return any(stripped in t for t in search_texts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('base')
    ap.add_argument('head')
    ap.add_argument('--repo-root', default='.')
    ap.add_argument('--changed-file', action='append', default=[])
    args = ap.parse_args()

    with open(args.base, encoding='utf-8') as f:
        base_text = f.read()
    with open(args.head, encoding='utf-8') as f:
        head_text = f.read()

    base_entries = split_entries(base_text)
    head_entries = split_entries(head_text)

    if not base_entries:
        print('base JOURNAL.md has no dated entries — nothing to check against')
        return 0

    head_set = set(e.rstrip('\n') for e in head_entries)

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

    missing = []
    rotated = []
    for entry in base_entries:
        key = entry.rstrip('\n')
        if key in head_set:
            continue
        if find_verbatim(entry, search_texts):
            rotated.append(entry.split('\n', 1)[0])
        else:
            missing.append(entry.split('\n', 1)[0])

    # New entries: present in head, absent from base. Must be well-formed --
    # already guaranteed by ENTRY's own regex, since split_entries only ever
    # produces well-formed blocks. Flag if any are dated *before* an entry
    # that remains above them (newest-first broken), a cheap and real
    # corruption signal distinct from content tampering.
    base_set = set(e.rstrip('\n') for e in base_entries)
    new_entries = [e for e in head_entries if e.rstrip('\n') not in base_set]

    dates = [m.group(1) for e in head_entries for m in [ENTRY.match(e)] if m]
    out_of_order = dates != sorted(dates, reverse=True)

    if rotated:
        print('%d entr%s rotated out, verified verbatim elsewhere in the diff:' %
              (len(rotated), 'y' if len(rotated) == 1 else 'ies'))
        for title in rotated:
            print('  ' + title)

    if new_entries:
        print('%d new entr%s prepended' % (len(new_entries), 'y' if len(new_entries) == 1 else 'ies'))

    ok = True
    if missing:
        ok = False
        print('\n%d entr%s from the base version MISSING from head, with no verbatim '
              'copy in any other file in the diff:' % (len(missing), 'y' if len(missing) == 1 else 'ies'))
        for title in missing:
            print('  ' + title)
        print('An entry may move to an archive file verbatim (rotation), or stay. '
              'It may not be edited or silently dropped.')

    if out_of_order:
        ok = False
        print('\nJOURNAL.md entries are not newest-first: ' + ' > '.join(dates))

    if ok:
        print('\nJOURNAL.md: prepend-only, OK')
        return 0
    return 1


if __name__ == '__main__':
    sys.exit(main())
