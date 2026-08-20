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

Also here (A23): one ID owning more than one row fails the lint. And (A31):
two LIVE rows citing the same `file:line` fails it too -- the tell for two IDs
describing one job, which no id-based check can see. And (A32): a live
umbrella row whose split rows have all landed is reported as ADVISORY only,
never an exit code. Each carries its measured false-alarm analysis at its own
definition below.

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


# A row whose ID cell is struck through -- `| ~~P8~~ | *(was)* | ...` -- is a
# superseded entry kept for the record beside its replacement, not a second
# owner of the ID. RE_ID_CELL deliberately tolerates the tildes so such rows
# still count as KNOWN (references to them must resolve); the duplicate check
# has to exclude them or every one is a false alarm. Real instances today: P8
# in BACKLOG.md and G3 in BACKLOG-DONE.md.
RE_SUPERSEDED_CELL = re.compile(r"^\s*~~")

# --- A31: two live rows citing the same file:line --------------------------
# A23 detects one ID owning two rows; it is blind by construction to two IDs
# describing one job (C15 and C16, 2026-08-20 -- filed by different boxes from
# branches where each other's row was invisible, for the identical file, the
# identical three symbols). No id-based check can see that. What both rows DID
# share, verbatim, was the citation `SynthEditSem/TideAppStubs.cpp:31`.
#
# The granularity is measured, not guessed (A31's working, 2026-08-20):
#
#   * same FILE, any line, live rows only:  14 collision groups on the live
#     tree, every one legitimate (`CMakeLists.txt` alone is cited by 14 rows).
#     Unusable as a gate.
#   * same FILE:LINE, live rows only:  ZERO collisions on the live tree, and
#     the C15/C16 pair is caught (both cite TideAppStubs.cpp:31).
#   * same FILE:LINE, live row vs DONE/archived row:  6 hits, 6 of them false
#     alarms -- an umbrella (C7) sharing citations with its own landed splits,
#     a filed remainder (S3g) citing the sites its parent S3 fixed, follow-ups
#     (E6, E7) citing lines that finished work touched. 100% false-positive
#     today, so that tier is deliberately NOT checked; the filing-time habit in
#     the run prompt (grep freshly-fetched origin/main before naming a file in
#     a new row) covers it instead.
#
# So: only rows whose status is live, only citations carrying a line number,
# keyed on basename:line so different path spellings of one location still
# collide -- C15 wrote `SE16/SynthEditSem/TideAppStubs.cpp` and C16 wrote
# `SynthEditSem/TideAppStubs.cpp:31`; basename-keying is what makes those meet.
# Like A23, this fires once both rows are on one branch: the first place the
# collision is visible, and the last moment merging the rows is cheap.
RE_FILE_LINE_CITE = re.compile(
    r"`([A-Za-z0-9_./\\-]+\.(?:cpp|h|hpp|mm|c|cc|py|cmake|yml|yaml|xml|md|txt"
    r"|pbxproj|vcxproj|iss|lua|sln)):(\d+)`")

# Statuses that mean a row is finished business. Matched as prefixes so
# variants like DONE-PENDING-CI count as closed. Everything else -- TODO,
# DOING(...), IN-REVIEW, BLOCKED(...), NEEDS-JEFF, NEEDS-SPEC -- is live.
CLOSED_STATUS_PREFIXES = ("DONE", "WONTFIX", "RESOLVED", "MOOT")


def live_row_citations(repo_root, live_file=BACKLOG_FILES[0]):
    """(basename, line) -> [(id, lineno), ...] for live rows in BACKLOG.md.

    Only the live backlog: new jobs are only ever filed there, and the
    archive is history. Superseded (~~struck~~) rows are skipped, closed rows
    are skipped, and one row citing a location twice counts once.
    """
    path = os.path.join(repo_root, live_file)
    cited = {}
    if not os.path.isfile(path):
        return cited
    with open(path, encoding="utf-8") as handle:
        for lineno, line in enumerate(handle, 1):
            if not line.startswith("|"):
                continue
            cells = line.split("|")
            if len(cells) < 4:
                continue
            if RE_SUPERSEDED_CELL.match(cells[1]):
                continue
            match = RE_ID_CELL.match(cells[1])
            if not match:
                continue
            status = cells[2].strip().upper()
            if status.startswith(CLOSED_STATUS_PREFIXES):
                continue
            rid = match.group(1)
            row_cites = set()
            for cite in RE_FILE_LINE_CITE.finditer(line):
                base = cite.group(1).replace("\\", "/").rsplit("/", 1)[-1]
                row_cites.add((base, cite.group(2)))
            for key in row_cites:
                cited.setdefault(key, []).append((rid, lineno))
    return cited


def shared_locations(citations):
    """The subset of citations owned by more than one distinct live row."""
    out = {}
    for key, owners in citations.items():
        if len({rid for rid, _ in owners}) > 1:
            out[key] = owners
    return out


# --- A32: umbrella rows whose splits have all landed -------------------------
# U2's Accept was met on 2026-08-16 -- the triage doc shipped and all five
# splits (U2a-U2e) were archived with merged PRs -- and the row read TODO for
# four more days. It was the only mac-marked row left, so a run looking for
# mac work took it and found nothing inside. The failure is silent and
# self-concealing: from outside, the queue looks like it has work in it.
#
# This is ADVISORY BY CONSTRUCTION, never an exit code, and that is measured
# rather than cautious (A32's working, 2026-08-20): the obvious rule -- flag
# any live row all of whose X[a-z] splits are closed -- fired on exactly two
# rows of the real tree, U2 (correctly: nothing left in it) and E2 (wrongly:
# a/b/c are done but its remaining module stages are simply not filed yet).
# One real, one false. A 50% false-positive rate is the same shape A23, A24
# and A27 were each nearly shipped with, so the report only names candidates;
# a human closes the umbrella or files its next child. An umbrella with all
# children landed and more intended is indistinguishable FROM OUTSIDE from one
# that is finished -- only the row's author knows, which is why this cannot be
# a gate.


def stale_umbrellas(repo_root, live_file=BACKLOG_FILES[0]):
    """Live rows whose child rows (id + one lowercase letter) all closed.

    Returns {umbrella_id: sorted child ids}. A row with no child rows anywhere
    is not an umbrella and is never reported. A child row that is itself live
    keeps its umbrella alive (C7 today: C7e is NEEDS-JEFF, so C7 is not
    reported however many siblings landed).
    """
    statuses = {}          # id -> (is_live, owning file) for non-superseded rows
    for name in BACKLOG_FILES:
        path = os.path.join(repo_root, name)
        if not os.path.isfile(path):
            continue
        with open(path, encoding="utf-8") as handle:
            for line in handle:
                if not line.startswith("|"):
                    continue
                cells = line.split("|")
                if len(cells) < 4 or RE_SUPERSEDED_CELL.match(cells[1]):
                    continue
                match = RE_ID_CELL.match(cells[1])
                if not match:
                    continue
                rid = match.group(1)
                if name == live_file:
                    status = cells[2].strip().upper()
                    live = not status.startswith(CLOSED_STATUS_PREFIXES)
                else:
                    live = False   # the archive is finished business by definition
                # A row in both files keeps the live file's verdict; that
                # conflict is A23's copied-not-moved case and is its to flag.
                if rid not in statuses or name == live_file:
                    statuses[rid] = live

    out = {}
    for rid, live in statuses.items():
        if not live or not re.fullmatch(r"[A-Z]\d{1,2}", rid):
            continue   # children (trailing lowercase) cannot themselves umbrella
        children = [c for c in statuses if c[:-1] == rid and c[-1].islower()]
        if children and not any(statuses[c] for c in children):
            out[rid] = sorted(children)
    return out


def id_locations(repo_root, backlog_files=BACKLOG_FILES, include_superseded=True):
    """Every row ID -> the [(file, lineno), ...] where it has a row of its own.

    A23 needs the locations, not just the set: a duplicate is only actionable if
    the report names BOTH lines, since renumbering means editing one of them.
    """
    found = {}
    for name in backlog_files:
        path = os.path.join(repo_root, name)
        if not os.path.isfile(path):
            continue
        with open(path, encoding="utf-8") as handle:
            for lineno, line in enumerate(handle, 1):
                if not line.startswith("|"):
                    continue
                cells = line.split("|")
                if len(cells) < 3:
                    continue
                if not include_superseded and RE_SUPERSEDED_CELL.match(cells[1]):
                    continue
                match = RE_ID_CELL.match(cells[1])
                if match:
                    found.setdefault(match.group(1), []).append((name, lineno))
    return found


def known_ids(repo_root, backlog_files=BACKLOG_FILES):
    """Every ID that has a row of its own, read from the tables' ID column."""
    return set(id_locations(repo_root, backlog_files))


def duplicate_ids(locations, live_file=BACKLOG_FILES[0]):
    """IDs owning more than one row, in the cases that are actionable. A23.

    Two runs can allocate the same ID from branches cut off the same `main`,
    where each other's row is unmerged and so invisible -- and if the rows land
    at different points in the file, git merges them CLEANLY rather than
    conflicting. Nothing detected that until a human noticed (2026-08-17, the
    two A17s). Lint runs against the merge result, which is the first place the
    collision is visible and the last moment renumbering is cheap.

    TWO cases are flagged, and one deliberately is not:

      * two rows in BACKLOG.md -- the collision this row was filed for. New IDs
        are only ever allocated here, so this is where a race lands.
      * one row in each file -- an archive move that COPIED instead of moving.
        Same defect, different hat, and it makes the row's status ambiguous.
      * two rows in BACKLOG-DONE.md and nowhere else -- NOT flagged. The archive
        is history, "archiving never rewrites a row", and it already contains a
        deliberate duplicate: S1 was taken by the linux AND macOS boxes on
        2026-08-06 before the cron stagger took effect, and both rows are kept
        on purpose with the second saying so. Flagging that would demand an edit
        the rules forbid, every run, forever.
    """
    out = {}
    for rid, locs in locations.items():
        if len(locs) < 2:
            continue
        if any(name == live_file for name, _ in locs):
            out[rid] = locs
    return out


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
    locations = id_locations(repo_root)
    ids = set(locations)
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

    # A23 -- reported before the stale-reference block, because a duplicated ID
    # makes every reference to it ambiguous and so is the more fundamental
    # failure. Both are reported in one run rather than short-circuiting.
    duplicates = duplicate_ids(
        id_locations(repo_root, include_superseded=False))
    if duplicates:
        print("\n%d DUPLICATE ID(s) -- one ID, more than one row:" % len(duplicates))
        for rid in sorted(duplicates):
            print("  %s" % rid)
            for name, lineno in duplicates[rid]:
                print("      %s:%d" % (name, lineno))
        print("\nTwo runs can allocate the same ID from branches cut off the same")
        print("main, and git merges the rows cleanly when they land at different")
        print("points in the file. Renumber the newer row -- it is cheap now and")
        print("expensive once anything references it.")

    # A31 -- two live rows citing the same file:line is the tell for two IDs
    # describing one job, which the duplicate-ID check above is blind to.
    shared = shared_locations(live_row_citations(repo_root))
    if shared:
        print("\n%d SHARED LOCATION(s) -- one file:line cited by more than one "
              "live row:" % len(shared))
        for key in sorted(shared):
            print("  %s:%s" % key)
            for rid, lineno in shared[key]:
                print("      %s  (%s:%d)" % (rid, BACKLOG_FILES[0], lineno))
        print("\nTwo live rows naming the same file:line usually means two runs")
        print("filed the same job under different ids (C15/C16, 2026-08-20).")
        print("Read both rows; if they are one job, fold the newer into the")
        print("older -- and if they genuinely differ, make each row say so and")
        print("cite different lines, or drop the citation from one.")

    # A32 -- advisory only, by measurement: the naive gate had a 50% false-
    # positive rate on the real tree (U2 real, E2 false). Candidates for a
    # human, never an exit code.
    umbrellas = stale_umbrellas(repo_root)
    if umbrellas:
        print("\nADVISORY -- %d live umbrella row(s) whose split rows have all "
              "landed:" % len(umbrellas))
        for rid in sorted(umbrellas):
            print("  %s  (children all closed: %s)"
                  % (rid, ", ".join(umbrellas[rid])))
        print("Advisory, not a failure: an umbrella with unfiled future children")
        print("(E2's shape) is indistinguishable from a finished one (U2's shape).")
        print("If nothing is left in the row, close it; if more children are")
        print("intended, file the next one or say so in the row.")

    if stale:
        print("\n%d STALE -- named but no such row in %s:"
              % (len(stale), " or ".join(BACKLOG_FILES)))
        for rel, lineno, kind, ref, context in stale:
            print("  %s:%d  %s  (%s reference)" % (rel, lineno, ref, kind))
            print("        ...%s" % context)
        print("\nIf the row is being added by another open PR, re-run with "
              "--allow-id <ID> rather than removing the reference.")
        return 1

    if duplicates or shared:
        return 1

    print("no stale ID references, no duplicate IDs, no shared live citations")
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

    # A23 -- the duplicate rule, on real file bodies rather than regex snippets,
    # because every subtlety in it is about WHICH FILE a row is in.
    import tempfile
    LIVE = "| ID | Status | Plat | Item |\n|---|---|---|---|\n"
    DONE = "| ID | Done | Plat | Item |\n|---|---|---|---|\n"
    dup_cases = [
        ("two rows in BACKLOG.md",
         LIVE + "| A24 | TODO | any | x |\n| A24 | TODO | any | y |\n", DONE, ["A24"]),
        ("one row in each file -- archived but not removed",
         LIVE + "| A24 | TODO | any | x |\n", DONE + "| A24 | 2026-01-01 | any | x |\n", ["A24"]),
        ("archive-only duplicate -- deliberate, see S1",
         LIVE + "| A24 | TODO | any | x |\n",
         DONE + "| S1 | 2026-08-06 | linux | x |\n| S1 | 2026-08-06 | mac | duplicate run |\n", []),
        ("superseded row beside its replacement -- P8/G3's shape",
         LIVE + "| P8 | DONE | win | new |\n| ~~P8~~ | *(was)* | win | old |\n", DONE, []),
        ("clean",
         LIVE + "| A24 | TODO | any | x |\n| A25 | TODO | any | y |\n", DONE, []),
    ]
    for description, live, done, expected in dup_cases:
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "BACKLOG.md"), "w", encoding="utf-8") as fh:
                fh.write(live)
            with open(os.path.join(d, "BACKLOG-DONE.md"), "w", encoding="utf-8") as fh:
                fh.write(done)
            got = sorted(duplicate_ids(id_locations(d, include_superseded=False)))
            if got != sorted(expected):
                failures += 1
                print("FAIL  duplicate/%s\n      expected: %s\n      got:      %s"
                      % (description, expected or "[]", got or "[]"))

    # A31 -- shared live citations, on real file bodies again. The negative
    # cases are the measured false-alarm tiers, so a regression toward any of
    # them turns a case red here before it turns the live tree red.
    shared_cases = [
        # (description, BACKLOG.md body, expected shared keys)
        ("two live rows, same file:line -- the C15/C16 shape",
         LIVE + "| C15 | TODO | win | fix `SynthEditSem/TideAppStubs.cpp:31` |\n"
                "| C16 | TODO | any | also `TideAppStubs.cpp:31` |\n",
         [("TideAppStubs.cpp", "31")]),
        ("different path spellings of one location still collide",
         LIVE + "| C15 | TODO | win | `SE16/SynthEditSem/TideAppStubs.cpp:31` |\n"
                "| C16 | TODO | any | `SynthEditSem/TideAppStubs.cpp:31` |\n",
         [("TideAppStubs.cpp", "31")]),
        ("live row vs DONE row -- measured 100%% false-alarm tier, not checked",
         LIVE + "| C7 | BLOCKED(C7e) | any | umbrella cites `TideAppStubs.cpp:31` |\n"
                "| C16 | DONE | any | landed `TideAppStubs.cpp:31` |\n",
         []),
        ("same file, different lines -- the 14-group file-level tier, not checked",
         LIVE + "| E9 | TODO | any | `SeAudioMaster.cpp:410` |\n"
                "| E10 | TODO | any | `SeAudioMaster.cpp:413` |\n",
         []),
        ("citation without a line number is not a location",
         LIVE + "| B1 | TODO | any | `build.yml` matrix |\n"
                "| R7 | TODO | any | `build.yml` secrets |\n",
         []),
        ("one row citing a location twice is not a collision",
         LIVE + "| S5 | TODO | any | `Application.cpp:167` and again `Application.cpp:167` |\n",
         []),
        ("superseded row does not own its citations",
         LIVE + "| P8 | TODO | win | new row, no citation |\n"
                "| ~~P8~~ | *(was)* | win | old row cites `foo.cpp:12` |\n"
                "| S5 | TODO | any | live row cites `foo.cpp:12` |\n",
         []),
    ]
    for description, live, expected in shared_cases:
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "BACKLOG.md"), "w", encoding="utf-8") as fh:
                fh.write(live)
            got = sorted(shared_locations(live_row_citations(d)))
            if got != sorted(expected):
                failures += 1
                print("FAIL  shared/%s\n      expected: %s\n      got:      %s"
                      % (description, sorted(expected) or "[]", got or "[]"))

    # A32 -- stale umbrellas, every case a shape measured on the real tree.
    umb_cases = [
        ("U2's shape: live umbrella, every child archived",
         LIVE + "| U2 | TODO | mac | triage |\n",
         DONE + "| U2a | 2026-08-16 | mac | x |\n| U2b | 2026-08-16 | mac | x |\n",
         ["U2"]),
        ("C7's shape: one child still live keeps the umbrella alive",
         LIVE + "| C7 | BLOCKED(C7e) | any | umbrella |\n"
                "| C7e | NEEDS-JEFF | any | ci clause |\n",
         DONE + "| C7a | 2026-08-19 | any | x |\n| C7b | 2026-08-20 | any | x |\n",
         []),
        ("children closed in the LIVE file count as closed",
         LIVE + "| E2 | TODO | any | umbrella |\n"
                "| E2a | DONE | any | x |\n| E2b | WONTFIX | any | x |\n",
         DONE, ["E2"]),
        ("a row with no children is not an umbrella",
         LIVE + "| S5 | TODO | any | no splits exist |\n", DONE, []),
        ("a closed umbrella is nobody's business",
         LIVE + "| U2 | DONE | mac | flipped |\n",
         DONE + "| U2a | 2026-08-16 | mac | x |\n", []),
        ("a child row cannot itself be an umbrella",
         LIVE + "| C7e | NEEDS-JEFF | any | leaf |\n",
         DONE + "| C7a | 2026-08-19 | any | x |\n", []),
    ]
    for description, live, done, expected in umb_cases:
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "BACKLOG.md"), "w", encoding="utf-8") as fh:
                fh.write(live)
            with open(os.path.join(d, "BACKLOG-DONE.md"), "w", encoding="utf-8") as fh:
                fh.write(done)
            got = sorted(stale_umbrellas(d))
            if got != sorted(expected):
                failures += 1
                print("FAIL  umbrella/%s\n      expected: %s\n      got:      %s"
                      % (description, expected or "[]", got or "[]"))

    total = (len(SELFTEST_CASES) + 1 + len(dup_cases) + len(shared_cases)
             + len(umb_cases))
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
