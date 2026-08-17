#!/usr/bin/env python3
"""Fail when the NEXT block sends a run at work that is not live.

BACKLOG A20. `check-id-refs.py` (A10) already fails when prose names an ID that
does not *exist*, and it draws its known-ID set from the ID column of BOTH
backlog files -- so an archived ID is a perfectly valid reference and passes.
That is correct for a reference and wrong for an instruction.

The NEXT block is not a reference. It is the first thing every run reads and
the one place the backlog tells a specific machine *what to pick up*. An ID
cited there as a take-this target has to still be in the queue.

What went wrong, 2026-08-18 (macos)
-----------------------------------
The `mac` NEXT row said a GUI-less scheduled run "should take **U1b** or **D6**
instead and say so". Both had been archived DONE the previous day. The
documented escape hatch for exactly that situation resolved to nothing, the run
had to derive its own fallback, and no check anywhere could have noticed --
`U1b` and `D6` are real IDs, so id-refs passed them.

Why this is not just id-refs with a smaller ID set
--------------------------------------------------
Because most IDs in the NEXT block are NOT take-targets, and flagging them all
would be noise that buries the one that matters. The `mac` row alone names
S11, S13, S14, A20, E2a, S1b, S5, S7, S8, A12 and B1 -- and for most of those
the sentence is a *warning* ("Do not fall back to E2a", "S13 is GATED and needs
Jeff"). An archived ID in a "do not take this" clause is fine; an archived ID
in a "take this" clause is the bug.

So the trigger set is deliberately narrow: the Take column, which is
definitionally a take-target, plus a short list of imperative phrases. Anything
else in the prose is left alone. A missed take-phrase is a false negative and
costs a run some minutes; a false positive costs trust in the other five
checks, which is the more expensive failure -- the same trade A10 documented.

    python3 scripts/check-next-block.py [--repo-root DIR] [--show] [--selftest]
"""
import argparse
import os
import re
import sys

# One uppercase letter, one or two digits, optional lowercase suffix -- the
# same shape rule A10 derived from the real ID set, for the same reason (it is
# what keeps SE16 and MSVC's C1083 out).
ID = r"[A-Z]\d{1,2}[a-z]?"

# Bold-or-bare ID, so both `take **S14**` and `take S14` are seen.
_ID_TOKEN = r"\*{0,2}(%s)\*{0,2}" % ID

# Phrases that can only introduce something to work on. Each is anchored on a
# verb, not on proximity, so "instead of taking S5" style prose does not match
# by accident. The verb is matched on its own and the IDs are read off after
# it, because a take clause is routinely a list -- "take U1b or D6 instead"
# is the exact sentence that produced this check, and matching one ID would
# have caught U1b and waved D6 through.
TAKE_VERBS = [
    r"should take",
    r"\btakes?\b",
    r"\bpick up\b",
    r"moves? on to",
    r"\bthen\b",
]
RE_TAKE_VERB = re.compile("|".join(TAKE_VERBS), re.IGNORECASE)

# An ID, optionally preceded by a list connector, anchored at the start of what
# is left -- so the run stops at the first non-ID word.
RE_NEXT_IN_LIST = re.compile(
    r"^\s*(?:(?:,|/|or|and)\s*)?\*{0,2}(%s)\*{0,2}" % ID, re.IGNORECASE)

# A negative clause disarms the whole sentence it appears in: an archived ID
# inside "do not fall back to E2a" is not an instruction to take it.
RE_NEGATED = re.compile(
    r"\b(?:do not|don't|never|rather than|instead of|not to|nor to)\b", re.IGNORECASE)

RE_ROW = re.compile(r"^\|(?P<cells>.+)\|\s*$")
RE_FENCE = re.compile(r"^\s*```")


def split_cells(line):
    return [c.strip() for c in line.strip().strip("|").split("|")]


def read_ids(path):
    """IDs from a backlog file's ID column."""
    ids = set()
    if not os.path.exists(path):
        return ids
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            m = RE_ROW.match(line)
            if not m:
                continue
            cells = split_cells(line)
            if cells and re.fullmatch(ID, cells[0]):
                ids.add(cells[0])
    return ids


def next_block_rows(backlog_path):
    """The NEXT block's data rows, as (line_no, platform, text)."""
    rows = []
    in_block = False
    in_fence = False
    with open(backlog_path, encoding="utf-8") as fh:
        for n, line in enumerate(fh, 1):
            if RE_FENCE.match(line):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            if line.startswith("## "):
                # The block runs from the NEXT heading to the next heading.
                in_block = line.startswith("## NEXT")
                continue
            if not in_block:
                continue
            if not RE_ROW.match(line):
                continue
            cells = split_cells(line)
            if len(cells) < 2:
                continue
            if cells[0].lower() in ("platform", "") or set(cells[0]) <= set("-: "):
                continue  # header or separator
            rows.append((n, cells[0], " | ".join(cells[1:])))
    return rows


def sentences(text):
    # Rough, and deliberately so: the point is to scope a negation to its own
    # clause rather than let one "do not" disarm a whole 3,000-character cell.
    return re.split(r"(?<=[.;])\s+|\s+—\s+|\s+--\s+", text)


def take_targets(text):
    """IDs the text tells a run to take, with the phrase that said so."""
    found = {}
    for sentence in sentences(text):
        if RE_NEGATED.search(sentence):
            continue
        for m in RE_TAKE_VERB.finditer(sentence):
            rest = sentence[m.end():]
            ids = []
            while True:
                hit = RE_NEXT_IN_LIST.match(rest)
                if not hit:
                    break
                ids.append(hit.group(1))
                rest = rest[hit.end():]
            phrase = m.group(0) + " " + " ".join(ids)
            for rid in ids:
                found.setdefault(rid, phrase.strip())
    return found


def run(repo_root, show=False):
    backlog = os.path.join(repo_root, "BACKLOG.md")
    done = os.path.join(repo_root, "BACKLOG-DONE.md")
    if not os.path.exists(backlog):
        print("check-next-block: no BACKLOG.md at %s -- nothing to check" % repo_root)
        return 0

    live = read_ids(backlog)
    archived = read_ids(done)

    rows = next_block_rows(backlog)
    if not rows:
        print("check-next-block: no NEXT block found -- nothing to check")
        return 0

    problems = []
    checked = 0
    for line_no, platform, text in rows:
        # Note there is deliberately no "every ID in the Take column counts"
        # rule. Measured against the real block, that flagged seven IDs whose
        # sentences were warnings and precedent ("do not fall back to E2a",
        # "C12c is IN-REVIEW", "A4's auto-merge tier") -- the Take cell is a
        # long editorial paragraph, not a field. Seven false alarms on the
        # first run is exactly the noise A10 warned erodes trust in the other
        # checks, so the imperative phrases are the whole trigger set.
        targets = take_targets(text)
        for rid, phrase in sorted(targets.items()):
            checked += 1
            if rid in live:
                if show:
                    print("  ok    %-5s %-6s (%s)" % (platform, rid, phrase))
                continue
            why = "archived DONE" if rid in archived else "no such row"
            problems.append((line_no, platform, rid, why, phrase))

    print("%d take-target(s) checked across %d NEXT row(s)" % (checked, len(rows)))

    if not problems:
        print("every NEXT take-target is a live BACKLOG.md row")
        return 0

    print("\nNEXT block sends a run at work that is not live:\n")
    for line_no, platform, rid, why, phrase in problems:
        print("  BACKLOG.md:%d  [%s]  %s -- %s" % (line_no, platform, rid, why))
        print("      matched: %r" % phrase)
    print("\nA run reads the NEXT block first and does what it says. Point the row")
    print("at a live row, or drop the clause. An archived ID is fine in a warning")
    print("(\"do not fall back to X\") -- it is only flagged as a take-this target.")
    return 1


SELFTEST_CASES = [
    ("should take, live", "a run should take **S14** instead", {"S14"}),
    ("should take, two", "should take **U1b** or **D6** instead", {"U1b", "D6"}),
    ("bare id", "then take S5 and stop", {"S5"}),
    ("then X", "S11 is done. Then **S14**, which will look like a regression", {"S14"}),
    ("negated fall back", "Do not fall back to E2a without the check", set()),
    ("negated nor", "nor to S1b/S5/S7/S8 (GATED-in-full)", set()),
    ("negated never", "never take B1, it needs workflow scope", set()),
    ("informational", "S13 is GATED and needs Jeff, but it is what stops anyone", set()),
    ("no ids", "measure whether audio returns before writing code", set()),
    ("not an id", "SE16 is the private repo and C1083 is an MSVC code", set()),
]


def selftest():
    failures = 0
    for description, text, expected in SELFTEST_CASES:
        got = set(take_targets(text))
        if got != expected:
            failures += 1
            print("FAIL  %s\n      text:     %r\n      expected: %s\n      got:      %s"
                  % (description, text, expected or "set()", got or "set()"))

    # The end-to-end shape: an archived take-target must fail, a live one must not.
    import tempfile
    for label, take_id, expect_rc in (("archived target", "D6", 1),
                                      ("live target", "S14", 0),
                                      ("absent target", "Z9", 1)):
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "BACKLOG.md"), "w", encoding="utf-8") as fh:
                fh.write("# B\n\n## NEXT\n\n| Platform | Take | Why |\n|---|---|---|\n")
                fh.write("| mac | a GUI-less run should take **%s** instead | x |\n" % take_id)
                fh.write("\n## Ready\n\n| ID | Status | Plat | Item |\n|---|---|---|---|\n")
                fh.write("| S14 | TODO | any | live row |\n")
            with open(os.path.join(d, "BACKLOG-DONE.md"), "w", encoding="utf-8") as fh:
                fh.write("| ID | Done | Plat | Item |\n|---|---|---|---|\n")
                fh.write("| D6 | 2026-08-17 | any | archived row |\n")
            rc = run(d)
            if rc != expect_rc:
                failures += 1
                print("FAIL  %s: expected rc=%d, got %d" % (label, expect_rc, rc))
            print("")

    total = len(SELFTEST_CASES) + 3
    print("%d case(s), %d failed" % (total, failures))
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo-root", default=".",
                        help="repository root (default: current directory)")
    parser.add_argument("--show", action="store_true",
                        help="list every take-target, not just the stale ones")
    parser.add_argument("--selftest", action="store_true",
                        help="run the built-in cases and exit")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    return run(args.repo_root, show=args.show)


if __name__ == "__main__":
    sys.exit(main())
