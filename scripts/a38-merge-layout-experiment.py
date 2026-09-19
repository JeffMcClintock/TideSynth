#!/usr/bin/env python3
"""Measure BACKLOG A38: is the fleet's merge jam adjacency, or disagreement?

A38's Accept clause asks for exactly this and says how it wants it proved:

    two runs on different platforms can each append a journal entry and
    re-point their own NEXT cell, on branches cut from the same `main`, and
    both merge with no conflict -- demonstrated by a scripted two-branch
    test, not by waiting for it to happen.

This is that script. It builds throwaway repositories in a temp directory,
never touches the real one, and needs no toolchain -- which is the point: the
jam it measures has never once been in code, so nothing has to compile to
reproduce it.

Four arms, two layouts x two edit sets. Each arm cuts branches `W` and `M` from
one base, has each make the edits ONE platform's run would make, and merges:

  1. today's layout, NEXT block only  -- `win` row vs `mac` row, adjacent lines
  2. option (b),     NEXT block only  -- docs/next/win.md vs docs/next/mac.md
  3. today's layout, a full STEP 4    -- (1) plus a JOURNAL.md entry each
  4. option (c),     a full STEP 4    -- (2) plus docs/journal/<run>.md each

The edits are strictly disjoint in every arm. Nothing in any arm is a genuine
disagreement -- no two branches ever change the same fact -- so any conflict
reported is an artifact of where the bytes live, and that is what is being
measured.

Result as of 2026-09-19, against `origin/main` at 792330672:

    arm 1  today  NEXT only   CONFLICT   BACKLOG.md
    arm 2  (b)    NEXT only   clean
    arm 3  today  full STEP 4 CONFLICT   BACKLOG.md, JOURNAL.md
    arm 4  (c)    full STEP 4 clean

Arms 1 and 3 are the negative control the proposal needs: they say the present
layout conflicts on edits that do not disagree about anything. Arms 2 and 4 say
the proposed layout does not. Run it before believing either.

    python3 scripts/a38-merge-layout-experiment.py [--repo DIR] [--keep]
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

PLATFORMS = ("win", "mac", "linux", "any")


def git(repo, *args, check=True):
    return subprocess.run(
        ("git", "-C", repo) + args,
        capture_output=True, text=True, encoding="utf-8", errors="replace",
        check=check,
    )


def show(repo, rev_path):
    r = git(repo, "show", rev_path)
    return r.stdout


def write(path, text):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    # newline="" so we never rewrite line endings underneath the experiment
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def init(path):
    os.makedirs(path, exist_ok=True)
    git(path, "init", "-q", ".")
    git(path, "config", "user.email", "a38@example.invalid")
    git(path, "config", "user.name", "a38-experiment")
    return path


def repoint_table_row(path, platform, marker):
    """Re-point one NEXT row, the way STEP 4 does -- prepend to that cell only."""
    prefix = "| %s |" % platform
    lines = read(path).split("\n")
    out = []
    for line in lines:
        if line.startswith(prefix):
            out.append("%s %s%s" % (prefix, marker, line[len(prefix):]))
        else:
            out.append(line)
    write(path, "\n".join(out))


def split_next_block(repo, backlog_text):
    """Option (b): each NEXT cell becomes its own file; the table becomes links."""
    out = []
    for line in backlog_text.split("\n"):
        hit = next((p for p in PLATFORMS if line.startswith("| %s |" % p)), None)
        if hit is None:
            out.append(line)
            continue
        cell = line[len("| %s |" % hit):].rstrip()
        if cell.endswith("|"):
            cell = cell[:-1]
        write(os.path.join(repo, "docs", "next", "%s.md" % hit), cell.strip() + "\n")
        out.append("| %s | see [docs/next/%s.md](docs/next/%s.md) |" % (hit, hit, hit))
    return "\n".join(out)


def split_journal(repo, journal_text):
    """Option (c): one file per entry, JOURNAL.md becoming a generated index."""
    parts = re.split(r"(?m)^(?=## )", journal_text)
    header, entries = parts[0], parts[1:]
    write(os.path.join(repo, "docs", "journal", "_header.md"), header)
    for n, entry in enumerate(entries):
        name = "%03d.md" % (len(entries) - n)
        write(os.path.join(repo, "docs", "journal", name), entry)
    return len(entries)


def prepend_journal_entry(path, headline):
    text = read(path)
    i = text.index("## ")
    entry = "## 2026-09-19 — %s\n\n**Did:** simulated STEP 4 entry.\n\n" % headline
    write(path, text[:i] + entry + text[i:])


def merge_arm(repo, edit_w, edit_m):
    """Cut W and M from HEAD, apply one edit to each, merge M into W."""
    git(repo, "add", "-A")
    git(repo, "commit", "-qm", "base")
    base = git(repo, "rev-parse", "HEAD").stdout.strip()

    git(repo, "checkout", "-qb", "W", base)
    edit_w(repo)
    git(repo, "add", "-A")
    git(repo, "commit", "-qm", "W: this platform's STEP 4")

    git(repo, "checkout", "-qb", "M", base)
    edit_m(repo)
    git(repo, "add", "-A")
    git(repo, "commit", "-qm", "M: the other platform's STEP 4")

    git(repo, "checkout", "-q", "W")
    git(repo, "merge", "M", "--no-commit", check=False)
    conflicted = [
        f for f in git(repo, "diff", "--name-only", "--diff-filter=U").stdout.split("\n") if f
    ]
    return conflicted


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                    help="the real TideSynth checkout to read origin/main's files from")
    ap.add_argument("--rev", default="origin/main")
    ap.add_argument("--keep", action="store_true", help="do not delete the scratch repos")
    args = ap.parse_args()

    backlog = show(args.repo, "%s:BACKLOG.md" % args.rev)
    journal = show(args.repo, "%s:JOURNAL.md" % args.rev)
    if not backlog or not journal:
        print("could not read BACKLOG.md/JOURNAL.md from %s" % args.rev, file=sys.stderr)
        return 2
    head = git(args.repo, "rev-parse", "--short", args.rev).stdout.strip()

    root = tempfile.mkdtemp(prefix="a38-")
    WIN = "**RE-POINTED 2026-09-19 (windows).** "
    MAC = "**RE-POINTED 2026-09-19 (macos).** "
    results = []

    def build_today(repo, with_journal):
        init(repo)
        write(os.path.join(repo, "BACKLOG.md"), backlog)
        if with_journal:
            write(os.path.join(repo, "JOURNAL.md"), journal)

    def build_split(repo, with_journal):
        init(repo)
        write(os.path.join(repo, "BACKLOG.md"), split_next_block(repo, backlog))
        if with_journal:
            split_journal(repo, journal)

    # --- arm 1: today's layout, NEXT block only -------------------------------
    r = os.path.join(root, "arm1")
    build_today(r, with_journal=False)
    results.append(("1", "today", "NEXT only", merge_arm(
        r,
        lambda d: repoint_table_row(os.path.join(d, "BACKLOG.md"), "win", WIN),
        lambda d: repoint_table_row(os.path.join(d, "BACKLOG.md"), "mac", MAC))))

    # --- arm 2: option (b), NEXT block only -----------------------------------
    r = os.path.join(root, "arm2")
    build_split(r, with_journal=False)
    results.append(("2", "(b)", "NEXT only", merge_arm(
        r,
        lambda d: write(os.path.join(d, "docs/next/win.md"),
                        WIN + read(os.path.join(d, "docs/next/win.md"))),
        lambda d: write(os.path.join(d, "docs/next/mac.md"),
                        MAC + read(os.path.join(d, "docs/next/mac.md"))))))

    # --- arm 3: today's layout, a full STEP 4 ---------------------------------
    def today_step4(plat, marker, headline):
        def go(d):
            repoint_table_row(os.path.join(d, "BACKLOG.md"), plat, marker)
            prepend_journal_entry(os.path.join(d, "JOURNAL.md"), headline)
        return go

    r = os.path.join(root, "arm3")
    build_today(r, with_journal=True)
    results.append(("3", "today", "full STEP 4", merge_arm(
        r,
        today_step4("win", WIN, "windows — a run that did something"),
        today_step4("mac", MAC, "macos — a run that did something else"))))

    # --- arm 4: option (c), a full STEP 4 -------------------------------------
    def split_step4(plat, marker, n, headline):
        def go(d):
            p = os.path.join(d, "docs/next/%s.md" % plat)
            write(p, marker + read(p))
            write(os.path.join(d, "docs/journal/%d.md" % n),
                  "## 2026-09-19 — %s\n\n**Did:** simulated.\n" % headline)
        return go

    r = os.path.join(root, "arm4")
    build_split(r, with_journal=True)
    results.append(("4", "(c)", "full STEP 4", merge_arm(
        r,
        split_step4("win", WIN, 900, "windows — a run that did something"),
        split_step4("mac", MAC, 901, "macos — a run that did something else"))))

    print("A38 merge-layout experiment — %s at %s" % (args.rev, head))
    print("every arm's two edits are strictly disjoint; no arm contains a disagreement\n")
    print("  arm  layout  edits        result")
    bad = 0
    for arm, layout, edits, conflicted in results:
        verdict = "CONFLICT  " + ", ".join(conflicted) if conflicted else "clean"
        print("  %-4s %-7s %-12s %s" % (arm, layout, edits, verdict))
        expect_conflict = layout == "today"
        if bool(conflicted) != expect_conflict:
            bad += 1
    print()
    if bad:
        print("UNEXPECTED: %d arm(s) did not behave as 2026-09-19 measured them." % bad)
        print("That is a result, not a failure of this script -- write down which.")
    else:
        print("As measured 2026-09-19: today's layout conflicts on edits that do not")
        print("disagree; the proposed layout does not. The jam is adjacency.")

    if args.keep:
        print("\nscratch repos kept at %s" % root)
    else:
        shutil.rmtree(root, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
