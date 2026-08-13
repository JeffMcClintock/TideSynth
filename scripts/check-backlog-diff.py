#!/usr/bin/env python3
"""Check that a BACKLOG.md change only edits the Status column, or adds rows.

Written for A3 (2026-08-13). The invariant is BACKLOG-DONE.md's own stated
rule for archiving: "Rows here are verbatim — archiving never rewrites one."
In practice, checked against real history (A8's rotation, and every status
flip a run has made), "verbatim" means the ORIGINAL text is never lost, not
that the cell is byte-frozen: a row is routinely flipped and elaborated in
the same edit ("Original item, kept for the record: <the exact old text>").
Three edits are legitimate:

  1. Status change: an existing row's Status/date cell changes (TODO -> DOING,
     DOING -> IN-REVIEW, IN-REVIEW -> a done-date). Plat must not change.
     Item may grow, as long as the base version's Item text is still present
     verbatim somewhere inside the new Item text — shrinking or replacing it
     is not a status change, it is a rewrite.
  2. New row: an ID that did not exist in the base version.
  3. Archive move: a row present in the base version is gone from the head
     version, but a row with the same ID and Plat, whose Item text contains
     the base version's Item text verbatim, exists in another file this PR
     touched (BACKLOG-DONE.md, or a docs/<id>*.md distillation).

Anything else fails: Plat changing, Item text on a still-present row no
longer containing the original wording (a genuine rewrite, not a flip —
A8's own distillation of N1/A2/R1/S1b/X4 down to a docs/<id>.md pointer is
exactly this case, and correctly needs a human, not an auto-merge), or a row
vanishing with no trace anywhere in the diff.

    python scripts/check-backlog-diff.py <base-backlog> <head-backlog> \
        [--repo-root DIR] [--changed-file PATH ...]
"""
import argparse
import os
import re
import sys

ROW = re.compile(r'^\| ([^\|]+) \| ([^\|]+) \| ([^\|]+) \| (.*) \|\s*$')
HEADER_IDS = {'ID'}  # the table header row itself: "| ID | Status | Plat | Item |"


def parse_rows(text):
    """Return {id: (raw_line, status, plat, item)} for every real row."""
    rows = {}
    for line in text.split('\n'):
        m = ROW.match(line)
        if not m:
            continue
        rid, status, plat, item = m.groups()
        rid = rid.strip()
        if rid in HEADER_IDS or set(rid) <= {'-'}:
            continue
        rows[rid] = (line, status.strip(), plat.strip(), item.strip())
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('base')
    ap.add_argument('head')
    ap.add_argument('--repo-root', default='.')
    ap.add_argument('--changed-file', action='append', default=[])
    args = ap.parse_args()

    with open(args.base, encoding='utf-8') as f:
        base_rows = parse_rows(f.read())
    with open(args.head, encoding='utf-8') as f:
        head_rows = parse_rows(f.read())

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

    # Rows in the *other* changed files (the archive), keyed by ID, so an
    # archived row can be matched on (Plat, Item) even though its Status
    # usually also changes in the same move -- IN-REVIEW becomes a done-date,
    # exactly like an ordinary status flip, just landing in a different file.
    archive_rows = {}
    for p in search_paths:
        try:
            with open(p, encoding='utf-8') as f:
                for rid, row in parse_rows(f.read()).items():
                    archive_rows.setdefault(rid, []).append(row)
        except OSError:
            pass

    status_changes, new_rows, archived, rewrites, dropped = [], [], [], [], []

    for rid, (line, status, plat, item) in base_rows.items():
        if rid not in head_rows:
            candidates = archive_rows.get(rid, [])
            if any(plat == c_plat and item in c_item for _, _, c_plat, c_item in candidates):
                archived.append(rid)
            else:
                dropped.append(rid)
            continue
        h_line, h_status, h_plat, h_item = head_rows[rid]
        # Item may GROW -- a status flip is allowed to prepend/append prose
        # (verification detail, links, "Original item, kept for the record: ...")
        # as long as the base text is still present somewhere in it. Plat must
        # never change; Item shrinking or being replaced outright is a rewrite.
        if plat != h_plat or item not in h_item:
            rewrites.append((rid, 'Plat' if plat != h_plat else 'Item'))
        elif status != h_status:
            status_changes.append((rid, status, h_status))

    for rid in head_rows:
        if rid not in base_rows:
            new_rows.append(rid)

    if status_changes:
        print('%d status change(s):' % len(status_changes))
        for rid, old, new in status_changes:
            print('  %s: %s -> %s' % (rid, old, new))
    if new_rows:
        print('%d new row(s): %s' % (len(new_rows), ', '.join(sorted(new_rows))))
    if archived:
        print('%d row(s) archived, verified verbatim elsewhere in the diff: %s' %
              (len(archived), ', '.join(sorted(archived))))

    ok = True
    if rewrites:
        ok = False
        print('\n%d row(s) with Plat or Item CHANGED in place (only Status may change '
              'on an existing row):' % len(rewrites))
        for rid, col in rewrites:
            print('  %s: %s column differs' % (rid, col))
    if dropped:
        ok = False
        print('\n%d row(s) MISSING from head, with no verbatim copy in any other file '
              'in the diff:' % len(dropped))
        for rid in dropped:
            print('  ' + rid)
        print('A row may move to an archive file verbatim, or stay. It may not be '
              'edited or silently dropped.')

    if ok:
        print('\nBACKLOG.md: status/date cells and new rows only, OK')
        return 0
    return 1


if __name__ == '__main__':
    sys.exit(main())
