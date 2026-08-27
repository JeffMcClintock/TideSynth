#!/usr/bin/env python3
"""Check that BACKLOG.md holds no finished rows, and that every row is visible.

BACKLOG E45. Two failures that look nothing alike and have the same cause -- a
rule written down but enforced by nobody.

1. A ROW THAT IS `DONE` AND STILL IN BACKLOG.md
-----------------------------------------------
BACKLOG.md's own Done section says an `IN-REVIEW` row whose PRs have merged
"becomes `DONE` and MOVES to that file, with the merge date in the `Done`
column". Runs have been doing the flip and skipping the move.

Nothing caught it, because `check-backlog-diff.py` PERMITS an archive move (its
case 3) and never requires one -- so a `DONE` row left in place is silently
legal. By 2026-08-27 nothing had been archived since 2026-08-20 and BACKLOG.md
had reached 754 KB, which is A8 recurring at nine times the size that triggered
A8: "every run on three machines was reading all of it to find the handful of
rows it could act on".

Statuses that are NOT flagged, deliberately: `IN-REVIEW` (PRs open, work not
landed), `DONE-PENDING-CI` and `DONE-PENDING-ACCEPT` (landed but not yet
provable), `WONTFIX` and `RESOLVED` (closed without landing, and their reasoning
is what a reader of the live queue most often wants). Only a bare `DONE` means
"this has merged and belongs in the archive".

2. A ROW THE OTHER LINTS CANNOT SEE
-----------------------------------
`check-backlog-diff.py` matches a row with

    ^\\| ([^\\|]+) \\| ([^\\|]+) \\| ([^\\|]+) \\| (.*) \\|\\s*$

-- note the trailing `\\|`. A row missing its closing pipe does not match, so
that row DOES NOT EXIST as far as that lint is concerned: a run could rewrite or
delete it and the diff check would report clean. E43 was in exactly that state
and nobody noticed until E45 went looking.

That is worth its own assertion rather than a comment, because it fails SILENTLY
and in the direction that removes protection.

WHY THIS IS A SEPARATE SCRIPT and not a clause in check-backlog-diff.py: that
one compares a base against a head and answers "is this change allowed". This
one asks "is the file in a good state at all", needs no base, and should fail on
a tree nobody has edited. Different question, different inputs.

    python3 scripts/check-backlog-archived.py [--repo-root DIR] [--selftest]

Exit 0 when clean, 1 when not.
"""

import argparse
import os
import re
import sys

# The row shape check-backlog-diff.py uses, kept deliberately identical -- the
# whole point of test 2 is to agree with THAT regex rather than one that looks
# equivalent. E45 records getting this wrong: a detector that rejected a
# trailing space reported two invisible rows where the real lint saw one.
ROW = re.compile(r'^\| ([^\|]+) \| ([^\|]+) \| ([^\|]+) \| (.*) \|\s*$')

# A row's opening, without requiring the close. Anything matching this and NOT
# matching ROW is invisible to the other lints.
ROW_OPEN = re.compile(r'^\| ([A-Z]+[0-9]+[a-z]?) \| ')

ARCHIVE_ME = "DONE"


def scan(text):
    """Return (finished, invisible) as lists of (line_number, id, line)."""
    finished, invisible = [], []

    for n, line in enumerate(text.split("\n"), 1):
        opening = ROW_OPEN.match(line)
        if not opening:
            continue

        row = ROW.match(line)
        if not row:
            invisible.append((n, opening.group(1), line))
            continue

        if row.group(2).strip() == ARCHIVE_ME:
            finished.append((n, row.group(1).strip(), line))

    return finished, invisible


def run(repo_root):
    path = os.path.join(repo_root, "BACKLOG.md")
    with open(path, encoding="utf-8") as f:
        text = f.read()

    finished, invisible = scan(text)

    for n, rid, line in invisible:
        print("BACKLOG.md:%d: row %s has no closing '|' -- it is INVISIBLE to "
              "check-backlog-diff.py, which cannot protect it" % (n, rid))

    if finished:
        print("%d row(s) marked DONE and still in BACKLOG.md:" % len(finished))
        for _, rid, _ in finished:
            print("    %s" % rid)
        print("\nMove them to BACKLOG-DONE.md VERBATIM, replacing the Status cell")
        print("with the merge date. Do not rewrite or distil the Item text --")
        print("check-backlog-diff.py treats that as a change needing a human,")
        print("and archiving is supposed to be bookkeeping.")

    if finished or invisible:
        return 1

    rows = len([l for l in text.split("\n") if ROW_OPEN.match(l)])
    print("%d row(s) in BACKLOG.md, none DONE, all terminated, OK"
          " (%d KB)" % (rows, len(text.encode("utf-8")) // 1024))
    return 0


CASES = [
    # (name, text, expect_finished, expect_invisible)
    ("clean",
     "| E1 | TODO | any | doing |\n| E2 | IN-REVIEW | mac | pending |\n", 0, 0),
    ("a DONE row is flagged",
     "| E1 | TODO | any | doing |\n| E2 | DONE | any | landed |\n", 1, 0),
    ("DONE-PENDING-* is NOT a DONE row",
     "| E1 | DONE-PENDING-CI | win | landed |\n"
     "| E2 | DONE-PENDING-ACCEPT | any | landed |\n", 0, 0),
    ("WONTFIX and RESOLVED stay in the live queue",
     "| E1 | WONTFIX | any | no |\n| E2 | RESOLVED | any | fixed elsewhere |\n", 0, 0),
    ("a missing closing pipe is invisible",
     "| E1 | TODO | any | no close\n", 0, 1),
    ("a trailing space still counts as terminated",
     "| E1 | TODO | any | closed | \n", 0, 0),
    ("BLOCKED(id) is not DONE",
     "| E1 | BLOCKED(E2) | any | waiting |\n", 0, 0),
    ("an invisible row is reported even when it is DONE",
     "| E1 | DONE | any | landed but unterminated\n", 0, 1),
]


def selftest():
    failures = 0
    for name, text, want_fin, want_inv in CASES:
        fin, inv = scan(text)
        ok = (len(fin) == want_fin and len(inv) == want_inv)
        if not ok:
            failures += 1
        print("%-4s %-46s finished=%d/%d invisible=%d/%d"
              % ("ok" if ok else "FAIL", name, len(fin), want_fin, len(inv), want_inv))
    print("%d case(s), %d failed" % (len(CASES), failures))
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo-root", default=".",
                        help="repository root (default: current directory)")
    parser.add_argument("--selftest", action="store_true",
                        help="run the built-in cases and exit")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    return run(args.repo_root)


if __name__ == "__main__":
    sys.exit(main())
