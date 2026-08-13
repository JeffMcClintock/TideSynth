#!/usr/bin/env python3
"""Decide whether a PR's file list qualifies for the A4 auto-merge tier.

BACKLOG A4. The rule is a STRICT INCLUSION list: a PR auto-merges only if every
file it touches is on the allowlist. Anything unrecognised -- a new top-level
file, a new directory, a code change -- fails closed and waits for a human.
That direction matters more than the list's contents: the failure mode of an
exclusion list is that tomorrow's file is merged by default.

Usage:
    python3 scripts/automerge_eligible.py <file-with-one-path-per-line>
    git diff --name-only main...HEAD | python3 scripts/automerge_eligible.py -
    python3 scripts/automerge_eligible.py --selftest

Exit code 0 = eligible, 1 = not eligible (with the reason on stdout), 2 = misuse.
The workflow reads the exit code; the reason is for the human reading the log.
"""

import re
import sys

# --- the allowlist -------------------------------------------------------
#
# Coordination files: the handoff (JOURNAL), the queue (BACKLOG), their
# archives, and prose docs. These are what a run rewrites every time, and what
# A4 exists to stop bouncing off a human.

ALLOWED_EXACT = {
    "JOURNAL.md",
    "BACKLOG.md",
    "BACKLOG-DONE.md",     # added 2026-08-14: A8 created it 2026-08-12, three
                           # days after A4's allowlist was written, and every
                           # IN-REVIEW -> DONE chore touches it.
}

# JOURNAL-2026-08.md and its successors. Same omission as BACKLOG-DONE.md: A8
# introduced the monthly archives and STEP 4's rotation rule makes almost every
# run touch one, so leaving them off meant A4 would fire close to never.
ALLOWED_PATTERNS = [
    re.compile(r"^JOURNAL-\d{4}-\d{2}\.md$"),
]

ALLOWED_PREFIXES = ("docs/",)

# --- the carve-outs inside the allowlist ---------------------------------
#
# Files that live in an allowed location but must never merge without a human,
# because merging them IS the decision rather than recording one.

DENIED_EXACT = {
    # Steers every machine on its next run -- A4's original wording.
    "docs/weekly-run-prompt.md",

    # Added 2026-08-14, and NOT in A4's original list. docs/decisions.md is the
    # PROPOSED mechanism: "an agent that hits a genuine design fork opens a PR
    # adding a PROPOSED: entry here ... **Jeff's merge of that PR is the
    # decision.**" Auto-merging it would let a run answer its own escalation --
    # the one file where merging is not bookkeeping but an act of authority.
    "docs/decisions.md",
}

# PLAN.md, website/**, .github/**, scripts/**, tools/**, tests/** and every code
# repo are absent from the allowlist entirely, so they fail closed with no
# special case needed. Listed here only so a reader does not go looking.


def classify(path):
    """Return None if the path is allowed, else the reason it is not."""
    # Reject anything that is not a plain relative path before any prefix match.
    # `docs/../PLAN.md` starts with "docs/" and would otherwise sail through the
    # prefix test. `git diff --name-only` normalises paths so this should be
    # unreachable in practice -- which is exactly why it is worth a guard rather
    # than an assumption about an upstream tool's output.
    if path.startswith("/") or "\\" in path or ".." in path.split("/"):
        return f"{path} is not a plain relative path -- failing closed"
    if path in DENIED_EXACT:
        return f"{path} is allowlisted by location but carved out (merging it is a decision, not bookkeeping)"
    if path in ALLOWED_EXACT:
        return None
    if any(p.match(path) for p in ALLOWED_PATTERNS):
        return None
    if path.startswith(ALLOWED_PREFIXES):
        return None
    return f"{path} is not on the auto-merge allowlist"


def evaluate(paths):
    """(eligible, reasons). Fails closed on an empty list."""
    paths = [p.strip() for p in paths if p.strip()]
    if not paths:
        return False, ["no changed files detected -- failing closed"]
    reasons = [r for r in (classify(p) for p in paths) if r]
    return (not reasons), reasons


# --- selftest ------------------------------------------------------------
#
# Cases 1-7 are the real file lists of merged PRs #41-#55, so the expectations
# are measurements rather than guesses. The rest are the edges.

SELFTEST = [
    # (name, files, expected_eligible)
    ("#55 backlog+journal+archives", [
        "BACKLOG-DONE.md", "BACKLOG.md", "JOURNAL-2026-08.md", "JOURNAL.md"], True),
    ("#54 S2 audit (carries tools/)", [
        "BACKLOG-DONE.md", "BACKLOG.md", "JOURNAL-2026-08.md", "JOURNAL.md",
        "docs/sandbox-audit.md", "tools/sandbox_audit.py"], False),
    ("#50 journal + archive", ["JOURNAL.md", "JOURNAL-2026-08.md"], True),
    ("#51 journal + a script", [
        "JOURNAL.md", "JOURNAL-2026-08.md", "scripts/check-links.py"], False),
    ("#41 A3, workflow + scripts", [
        ".github/workflows/lint.yml", "scripts/check-backlog-diff.py"], False),
    ("docs prose only", ["docs/sandbox-audit.md", "docs/building.md"], True),
    ("archive rotation only", ["JOURNAL.md", "JOURNAL-2026-09.md"], True),

    # Carve-outs: allowed location, denied file.
    ("run prompt", ["docs/weekly-run-prompt.md"], False),
    ("run prompt smuggled in with a legit journal edit",
     ["JOURNAL.md", "docs/weekly-run-prompt.md"], False),
    ("decisions.md -- merging it IS the ruling", ["docs/decisions.md"], False),
    ("decisions.md alongside a journal entry",
     ["JOURNAL.md", "docs/decisions.md"], False),

    # Fails closed.
    ("PLAN.md", ["PLAN.md"], False),
    ("website is a production deploy", ["website/index.html"], False),
    ("the auto-merge workflow itself", [".github/workflows/auto-merge.yml"], False),
    ("this very script", ["scripts/automerge_eligible.py"], False),
    ("empty", [], False),
    ("a new top-level file nobody has seen", ["NOTES.md"], False),
    ("near-miss on the archive pattern", ["JOURNAL-2026-8.md"], False),
    ("path traversal shape", ["docs/../PLAN.md"], False),
]


def selftest():
    failures = 0
    for name, files, expected in SELFTEST:
        got, reasons = evaluate(files)
        ok = got == expected
        failures += 0 if ok else 1
        mark = "ok  " if ok else "FAIL"
        verdict = "eligible" if got else "blocked"
        print(f"  {mark} {name:52s} -> {verdict}")
        if not ok:
            print(f"       expected {'eligible' if expected else 'blocked'}; reasons: {reasons}")
    print(f"selftest: {failures} failed")
    return 1 if failures else 0


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    if sys.argv[1] == "--selftest":
        return selftest()

    if sys.argv[1] == "-":
        paths = sys.stdin.read().splitlines()
    else:
        with open(sys.argv[1], encoding="utf-8") as fh:
            paths = fh.read().splitlines()

    eligible, reasons = evaluate(paths)
    if eligible:
        print(f"eligible: all {len([p for p in paths if p.strip()])} changed file(s) are on the auto-merge allowlist")
        return 0
    for r in reasons:
        print(f"not eligible: {r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
