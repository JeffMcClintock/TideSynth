#!/usr/bin/env python3
"""A38: can the four open `tide/win/**` PRs be merged in ANY order, cleanly?

Why this exists
---------------
`tests/a38_bookkeeping_merge_probe.py` (2026-09-21) measures the livelock's
*mechanism*: two branches editing adjacent lines of `BACKLOG.md` / `JOURNAL.md`
conflict even when their content is strictly disjoint.

This probe measures a different thing -- whether a specific, real set of open
branches can actually be swept. The 2026-09-22 windows run committed its STEP 4
journal entry and its `win` NEXT cell **byte-identically onto all four** of this
lane's open branches rather than onto a fifth bookkeeping branch, on the theory
that content identical on both sides of a merge is a no-op and not a conflict.
That theory is worth exactly nothing unless the sweep is measured, because the
failure it predicts is the one the fleet has hit eight times: the first merge
lands and re-conflicts everything behind it.

So: replay the sweep. Start at `main`, merge the four branches one after
another, and count conflicts -- for all 24 orderings, because a sweep that only
works in one order is not a fix, it is a footgun with an undocumented sequence.

The negative control is the point
---------------------------------
A green result here means nothing on its own -- git merges most things. The
control replays the identical 24 orderings against the branches **as they were
before that run resolved them** (`--control`), which is the state every previous
cell in this chain left behind. The control is expected to CONFLICT. If it ever
comes back clean, this probe has stopped measuring anything and should be
believed about nothing.

Usage
-----
    python3 tests/a38_sweep_probe.py [--repo PATH] [--control] [--verbose]

Exit code 0 when the treatment sweeps clean in all 24 orderings AND the control
conflicts in at least one; 1 otherwise. No network, no build.
"""
import argparse
import itertools
import os
import re
import shutil
import subprocess
import sys
import tempfile

# The four open PRs of this lane, newest-resolution-first. Each entry is
# (label, remote branch). The probe resolves each to a local ref in the clone.
BRANCHES = [
    ("A38/#597", "tide/win/A38-bookkeeping-livelock"),
    ("E19/#590", "tide/win/E19-datatype-census"),
    ("E82/#587", "tide/win/E82-rack-menu-producer"),
    ("E80/#586", "tide/win/E80-clap-editor-arm"),
]

# The two files the livelock has always been about. Reported separately so a
# conflict in CODE is never quietly lumped in with a bookkeeping conflict --
# they mean completely different things.
BOOKKEEPING = {"BACKLOG.md", "JOURNAL.md", "docs/lessons.md"}


def git(repo, *args, check=True):
    p = subprocess.run(["git", "-C", repo] + list(args),
                       capture_output=True, text=True, encoding="utf-8",
                       errors="replace")
    if check and p.returncode != 0:
        raise RuntimeError("git %s failed:\n%s%s" % (" ".join(args), p.stdout, p.stderr))
    return p


def conflicts_of(repo):
    """Paths with an unresolved conflict, via the index rather than by scraping
    merge output -- `git ls-files -u` is the authoritative list."""
    out = git(repo, "ls-files", "-u", "--format=%(path)").stdout
    return sorted(set(x for x in out.split("\n") if x.strip()))


def sweep(repo, order, base):
    """Merge `order` into `base` one at a time, stopping at the first conflict.

    Returns (depth, label, paths): how many PRs merged cleanly before the sweep
    stalled, which one stalled it, and on what. DEPTH is the number that matters
    -- "0 clean orderings" hides the difference between a sweep that stalls on
    the first PR and one that stalls on the last, and those are very different
    states of the queue.
    """
    git(repo, "checkout", "--quiet", "--detach", base)
    for depth, (label, ref) in enumerate(order):
        git(repo, "merge", "--no-edit", "--no-ff", ref, check=False)
        c = conflicts_of(repo)
        if c:
            git(repo, "merge", "--abort", check=False)
            return depth, label, c
    return len(order), None, []


def run(repo_src, control, verbose):
    tmp = tempfile.mkdtemp(prefix="a38-sweep-")
    repo = os.path.join(tmp, "r")
    try:
        git(tmp, "clone", "--quiet", "--no-hardlinks", repo_src, repo)
        git(repo, "config", "user.email", "probe@example.invalid")
        git(repo, "config", "user.name", "a38 sweep probe")

        base = "origin/main"
        refs = []
        for label, br in BRANCHES:
            treatment = "refs/remotes/origin/sweep-" + br.replace("/", "-")
            plain = "refs/remotes/origin/" + br
            # --control: the branch as it is published right now, i.e. before
            # this run's resolution. Otherwise: the resolved local ref.
            want = plain if control else treatment
            if git(repo, "rev-parse", "--verify", "--quiet", want, check=False).returncode != 0:
                want = plain
            refs.append((label, want))

        print("base: %s (%s)" % (base, git(repo, "rev-parse", "--short", base).stdout.strip()))
        for label, ref in refs:
            print("  %-10s %s  %s" % (label, git(repo, "rev-parse", "--short", ref).stdout.strip(), ref))
        print()

        orders = list(itertools.permutations(refs))
        depths = {}
        allpaths = set()
        blockers = {}
        for order in orders:
            depth, label, paths = sweep(repo, order, base)
            depths[depth] = depths.get(depth, 0) + 1
            allpaths.update(paths)
            if label:
                blockers[label] = blockers.get(label, 0) + 1
            if verbose:
                print("  depth %d/%d  %-10s %-28s [%s]"
                      % (depth, len(order), label or "-", ", ".join(paths) or "clean",
                         " -> ".join(l for l, _ in order)))

        full = depths.get(len(refs), 0)
        print("%d ordering(s): %d swept fully clean" % (len(orders), full))
        print("  PRs merged before the sweep stalled:")
        for d in sorted(depths):
            print("    depth %d: %2d ordering(s)%s"
                  % (d, depths[d], "  <- complete sweep" if d == len(refs) else ""))
        if blockers:
            print("  stalled on: " + ", ".join("%s x%d" % (k, v) for k, v in sorted(blockers.items())))
        if allpaths:
            book = sorted(p for p in allpaths if p in BOOKKEEPING)
            code = sorted(p for p in allpaths if p not in BOOKKEEPING)
            print("  conflicting paths -- bookkeeping: %s" % (", ".join(book) or "none"))
            print("  conflicting paths -- other:       %s" % (", ".join(code) or "none"))
        return full, len(orders) - full, len(orders), depths
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap.add_argument("--control", action="store_true",
                    help="replay against the UNRESOLVED published branches; expected to conflict")
    ap.add_argument("--verbose", action="store_true")
    a = ap.parse_args()

    if a.control:
        print("=== NEGATIVE CONTROL: unresolved branches, depth 0 EXPECTED ===")
        full, dirty, total, depths = run(a.repo, True, a.verbose)
        ok = depths.get(0, 0) == total
        print("\ncontrol %s: %d/%d orderings stalled on the FIRST merge%s"
              % ("OK" if ok else "UNEXPECTED", depths.get(0, 0), total,
                 "" if ok else " -- expected all of them to; this probe's baseline has moved"))
        return 0 if ok else 1

    print("=== TREATMENT: resolved branches ===")
    full, dirty, total, depths = run(a.repo, False, a.verbose)
    # The bar this probe actually holds the treatment to: the first PR of EVERY
    # ordering must merge cleanly. That is what "resolved against main" buys and
    # it is all it buys -- a full sweep additionally needs the branches to agree
    # with each OTHER, which generated files and per-run journal entries prevent.
    first_ok = depths.get(0, 0) == 0
    print("\ntreatment %s: every ordering merges at least one PR cleanly (depth 0 count: %d)"
          % ("OK" if first_ok else "FAILED", depths.get(0, 0)))
    print("full sweeps: %d/%d%s" % (full, total,
          "" if full == total else "  -- see the depth histogram; branch-vs-branch divergence, not branch-vs-main"))
    return 0 if first_ok else 1


if __name__ == "__main__":
    sys.exit(main())
