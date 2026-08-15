#!/usr/bin/env python3
"""Fail when prose names a backlog ID that does not exist.

BACKLOG A10, the half of A3 that was deliberately deferred. A3 shipped link
checking; this checks the *other* kind of cross-reference -- the bare prose
mention. `blocked on A6`, `see C9`, `BLOCKED(C12f)`, `**P8**`. Those are not
links, so `check-links.py` cannot see them, and they rot the same way: a row
gets renamed, split, or archived and the mentions elsewhere quietly stop
naming anything.

Why a naive scan does not work
------------------------------
A10's own row says it: `\\b[A-Z]\\d+[a-z]?\\b` false-positives constantly, and
a noisy lint erodes trust in the other four checks fast. Measured against this
tree, the tokens that would have been wrongly flagged are:

    SE16   331 occurrences    the private repo's name
    SE15     4                the dormant repo
    SE14     1                the older dormant repo
    C1083    8                MSVC "cannot open include file"
    C2664    1                MSVC "cannot convert argument"
    C4834    1                MSVC "discarding return value"

Two shape rules remove every one of them, and both are derived from the real
IDs rather than guessed:

1. **A single uppercase letter**, never two. All 96 IDs in BACKLOG.md and
   BACKLOG-DONE.md use one -- A, B, C, D, E, G, H, L, M, N, P, R, S, U, V, W,
   X. That kills `SE16` outright.
2. **One or two digits.** The longest real ID is `C12f`/`S10`/`A13`. MSVC
   diagnostics are four digits. That kills `C1083` and friends.

The remaining protection is context. An ID-shaped token is only treated as a
reference when it is **bold** (`**C9**`), inside `BLOCKED(...)`, or follows a
phrase that can only introduce a cross-reference (`see`, `blocked on`, `filed
as`, `unblocks`, `supersedes`, ...). Everything else is left alone, so a
sentence that happens to contain `V1` or `P7a` is not the lint's business.

Known IDs come from the **ID column of BACKLOG.md and BACKLOG-DONE.md** and
nowhere else, so the check can never invent an ID that looks real: if a row
does not exist in the table, a reference to it is stale by definition.

What it deliberately does not do
--------------------------------
- Fenced code blocks, inline code spans, link targets and bare URLs are all
  skipped. Commit messages, CMake variables and shell snippets live there.
- It does not check that a reference is *apt*, only that its target exists.
- **Stacked PRs are a known limitation.** If PR A adds row `A14` and PR B
  references it, B fails until A lands, because B's tree has no such row. That
  is the check working -- the reference really is dangling on B's branch -- but
  the fix is `--allow-id A14` on the trailing PR rather than weakening the rule.

Usage
-----
    python scripts/check-id-refs.py                  # lint the tree, exit 1 on stale
    python scripts/check-id-refs.py --show           # also list every valid reference
    python scripts/check-id-refs.py --allow-id A14   # tolerate one not-yet-landed row
    python scripts/check-id-refs.py --selftest       # unit cases, no repo needed
"""

import argparse
import glob
import os
import re
import sys

# --- Shape -------------------------------------------------------------------
# Single uppercase letter, one or two digits, optional lowercase stage suffix.
# Both bounds are measured from the real ID column, not guessed. See the module
# docstring for what each one excludes.
ID = r"[A-Z]\d{1,2}[a-z]?"

# Phrases that can only be introducing a cross-reference. Kept deliberately
# short: every addition is a new false-positive surface, and the bold rule
# already catches most real mentions.
TRIGGERS = (
    r"see", r"per", r"blocked on", r"blocker(?: is)?", r"filed as", r"files as",
    r"unblocks?", r"unblocked by", r"supersedes?", r"superseded by",
    r"depends on", r"pairs with", r"part of", r"absorbed into", r"BACKLOG",
    r"rides? along with", r"same wall as", r"tracked as",
)

RE_BLOCKED = re.compile(r"BLOCKED\((" + ID + r")\)")
RE_BOLD = re.compile(r"\*\*(" + ID + r")\*\*")
RE_TRIGGER = re.compile(
    r"(?:\b(?:" + "|".join(TRIGGERS) + r"))\s+\**(" + ID + r")\**", re.IGNORECASE
)

RE_FENCE = re.compile(r"^\s*```")
RE_CODE_SPAN = re.compile(r"`[^`]*`")
RE_LINK_TARGET = re.compile(r"\]\([^)]*\)|https?://\S+")

# The ID column of a table row, tolerating the ~~strikethrough~~ and *italic*
# markers the backlog uses for superseded rows.
RE_ID_CELL = re.compile(r"^[~*\s]*([A-Z]\d{1,2}[a-z]?)[~*\s]*$")

DEFAULT_GLOBS = ("*.md", "docs/*.md")
BACKLOG_FILES = ("BACKLOG.md", "BACKLOG-DONE.md")


def known_ids(repo_root, backlog_files=BACKLOG_FILES):
    """Every ID that has a row of its own, read from the tables' ID column."""
    found = set()
    for name in backlog_files:
        path = os.path.join(repo_root, name)
        if not os.path.isfile(path):
            continue
        with open(path, encoding="utf-8") as handle:
            for line in handle:
                if not line.startswith("|"):
                    continue
                cells = line.split("|")
                if len(cells) < 3:
                    continue
                match = RE_ID_CELL.match(cells[1])
                if match:
                    found.add(match.group(1))
    return found


def strip_noncontent(line):
    """Blank out code spans and link targets, keeping offsets roughly intact."""
    return RE_LINK_TARGET.sub(" ", RE_CODE_SPAN.sub(" ", line))


def references(path):
    """Yield (lineno, kind, id, context) for every cross-reference in a file."""
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    in_fence = False
    for lineno, raw in enumerate(text.split("\n"), 1):
        if RE_FENCE.match(raw):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        line = strip_noncontent(raw)
        for regex, kind in ((RE_BLOCKED, "blocked"),
                            (RE_BOLD, "bold"),
                            (RE_TRIGGER, "trigger")):
            for match in regex.finditer(line):
                start = max(0, match.start() - 60)
                context = line[start:match.end() + 30].strip()
                yield lineno, kind, match.group(1), context


def collect_files(repo_root, patterns):
    paths = []
    for pattern in patterns:
        paths.extend(glob.glob(os.path.join(repo_root, pattern)))
    return sorted(set(p for p in paths if os.path.isfile(p)))


def run(repo_root, allow=(), show=False, patterns=DEFAULT_GLOBS):
    ids = known_ids(repo_root)
    if not ids:
        print("error: no IDs found -- is %s a TideSynth checkout?" % repo_root,
              file=sys.stderr)
        return 2
    permitted = ids | set(allow)

    stale, examined, distinct = [], 0, set()
    for path in collect_files(repo_root, patterns):
        rel = os.path.relpath(path, repo_root).replace(os.sep, "/")
        for lineno, kind, ref, context in references(path):
            examined += 1
            distinct.add(ref)
            if ref not in permitted:
                stale.append((rel, lineno, kind, ref, context))
            elif show:
                print("  ok  %s:%d  [%s] %s" % (rel, lineno, kind, ref))

    print("%d ID reference(s) checked against %d row(s), %d distinct ID(s) named"
          % (examined, len(ids), len(distinct)))

    if stale:
        print("\n%d STALE -- named but no such row in %s:"
              % (len(stale), " or ".join(BACKLOG_FILES)))
        for rel, lineno, kind, ref, context in stale:
            print("  %s:%d  %s  (%s reference)" % (rel, lineno, ref, kind))
            print("        ...%s" % context)
        print("\nIf the row is being added by another open PR, re-run with "
              "--allow-id <ID> rather than removing the reference.")
        return 1

    print("no stale ID references")
    return 0


# --- Self-test ---------------------------------------------------------------
# Every trap in here is real: the false positives are tokens measured in this
# repo's own docs, and the true positives are the shapes A10 was filed to catch.

SELFTEST_IDS = {"A6", "C9", "C12f", "P8", "S1b", "V1", "A13", "S10"}

SELFTEST_CASES = [
    # (text, expected stale ids, description)
    ("blocked on A6 for now", [], "valid trigger reference"),
    ("**C9** blocks C4 and C5", [], "valid bold reference"),
    ("| C6 | BLOCKED(C12f) | any |", [], "valid BLOCKED()"),
    ("see S1b and **A13**", [], "two valid references, mixed kinds"),

    ("blocked on A99 for now", ["A99"], "stale trigger reference"),
    ("**Z3** is the next item", ["Z3"], "stale bold reference"),
    ("| C6 | BLOCKED(C99) | any |", ["C99"], "stale BLOCKED() -- the highest-value case"),
    ("superseded by P42 on Tuesday", ["P42"], "stale, two-digit"),

    # False positives A10's row predicted, each measured in this repo's docs.
    ("the **SE16** repo holds it", [], "SE16: two-letter prefix is never an ID"),
    ("fails with **C1083** cannot open include", [], "MSVC code: four digits"),
    ("error **C2664** on that line", [], "MSVC code: four digits"),
    ("see SE15 for the dormant tree", [], "SE15: two-letter prefix"),
    ("built for **x64** and ARM64", [], "lowercase and three-letter prefixes"),
    ("checkSizeConstraint(0,0,2178,32672) in P7a's note", [],
     "bare ID-shaped token in prose, no reference context"),
    ("the V1 acceptance test", [], "bare mention without a trigger is not checked"),
    ("shipped in v0.1 and V1.6", [], "version strings"),

    # Structural skips.
    ("`blocked on A99`", [], "inline code span is skipped"),
    ("see [A99](docs/a99.md)", [], "link target is skipped"),
    ("see https://example.com/A99", [], "bare URL is skipped"),
]


def selftest():
    failures = 0
    for text, expected, description in SELFTEST_CASES:
        in_fence = False
        got = []
        for regex in (RE_BLOCKED, RE_BOLD, RE_TRIGGER):
            for match in regex.finditer(strip_noncontent(text)):
                if match.group(1) not in SELFTEST_IDS:
                    got.append(match.group(1))
        if sorted(got) != sorted(expected):
            failures += 1
            print("FAIL  %s\n      text:     %r\n      expected: %s\n      got:      %s"
                  % (description, text, expected or "[]", got or "[]"))

    # Fenced blocks are a line-level concern, so exercise them on a real file body.
    fenced = "```\nblocked on A99\n```\nblocked on C9\n"
    in_fence, leaked = False, []
    for line in fenced.split("\n"):
        if RE_FENCE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        for match in RE_TRIGGER.finditer(strip_noncontent(line)):
            if match.group(1) not in SELFTEST_IDS:
                leaked.append(match.group(1))
    if leaked:
        failures += 1
        print("FAIL  fenced code block leaked: %s" % leaked)

    total = len(SELFTEST_CASES) + 1
    print("\n%d case(s), %d failed" % (total, failures))
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo-root", default=".",
                        help="repository root (default: current directory)")
    parser.add_argument("--allow-id", action="append", default=[], metavar="ID",
                        help="treat ID as valid even without a row; for a PR that "
                             "references a row another open PR is adding. Repeatable.")
    parser.add_argument("--show", action="store_true",
                        help="list every valid reference, not just the stale ones")
    parser.add_argument("--selftest", action="store_true",
                        help="run the built-in cases and exit")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    return run(args.repo_root, allow=args.allow_id, show=args.show)


if __name__ == "__main__":
    sys.exit(main())
