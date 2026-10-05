#!/usr/bin/env python3
"""A38, the depth-2 question: does resolving every branch against `main` make a
BATCH of them landable?

Why this exists beside the other two A38 probes
-----------------------------------------------
`tests/a38_sweep_probe.py` (2026-09-22) and `tests/a38_fleet_sweep_probe.py`
(2026-09-23) both search for the deepest sweep. This one exists to pin a
specific WRONG INFERENCE, because the fleet made it on 2026-09-25 and then
acted on it for a day.

That run measured `git merge-tree --write-tree --name-only origin/<branch>
origin/main`, once per branch, found `docs/lessons.md` was the only conflicting
path, neutralised it by taking `main`'s copy verbatim, re-measured, and got a
clean result on all four. It then wrote down: "merging one does not re-conflict
the others."

**That conclusion does not follow from that measurement, and it is false.**
`merge-tree <branch> <main>` asks whether a branch is individually mergeable.
Whether TWO branches can both land is a different question, about branch-vs-
branch content, and no number of branch-vs-main measurements can answer it.

What this probe measures
------------------------
Two things the branch-vs-main check cannot see, for one lane -- small enough to
enumerate exhaustively, so there is no search heuristic to doubt:

  1. **Every branch alone into `main`** -- the 09-25 measurement, reproduced.
  2. **Every ORDERING of the lane, exhaustively** (5! = 120 for five branches),
     replaying real in-memory merges. The depth histogram is the answer: all
     120 orders reaching depth 1 means no two of them can both land.

It also prints the depth-2 matrix -- for each branch that lands first, which
others then conflict and on what paths. That is where the hot bookkeeping files
show themselves, and it is how `BACKLOG-DONE.md` was found to be a fifth one
that A38's row does not list.

No worktree and no checkout; no network at all in pinned mode, and one
`ls-remote` in `--live`. `merge-tree --write-tree` plus `commit-tree` do every
merge in memory, so this is safe to run against a tree the developer is working
in.

The control
-----------
`--control` replaces each branch with its merge base against `main` -- branches
that provably contain nothing new. Every ordering must then reach full depth.
If it does not, the merge machinery here is broken and nothing else this probe
prints means anything. That arm is what makes a depth of 1 a measurement rather
than a bug.

The input is PINNED, not ambient (A41, 2026-10-01)
--------------------------------------------------
This probe used to ask `gh pr list` what to measure, which made every number it
printed a function of the instant it ran -- and a recorded result could not be
re-checked once the fleet moved. It now reads `tests/a38_fleet_state.json` by
default, so a recorded run reproduces byte-for-byte; `--live` measures today's
queue and labels the output a snapshot. See `tests/a38_fleet_state.py` for the
three defects A41 measured and what each fix is.

Usage
-----
    python3 tests/a38_lane_sweep_probe.py [--repo PATH] [--lane win] [--control]
    python3 tests/a38_lane_sweep_probe.py --live          # today's queue
    python3 tests/a38_lane_sweep_probe.py --live --fetch   # and fetch what it names

Four exit codes, matching `a38_row_adjacency_probe.py`: **0** the measurement
completed (and, with --control, swept fully), **1** the control failed, **2** the
input names a ref this repo cannot resolve, **3** VACUOUS -- fewer than two
branches in the lane, so there was no ordering to compare and rc=0 would have
read as a pass. The depth is REPORTED, not asserted: it describes a moving
queue, so a threshold here would be a tripwire on someone else's merge habits.
"""
import argparse
import itertools
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import a38_fleet_state  # noqa: E402

BOOKKEEPING = {"BACKLOG.md", "BACKLOG-DONE.md", "JOURNAL.md",
               "docs/lessons.md", "docs/decisions.md"}


def git(repo, *args, check=True):
    p = subprocess.run(["git", "-C", repo] + list(args), capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    if check and p.returncode != 0:
        raise RuntimeError("git %s failed:\n%s%s" % (" ".join(args), p.stdout, p.stderr))
    return p


def try_merge(repo, cur, ref):
    """Merge `ref` into commit `cur` in memory -> (commit, None) or (None, paths)."""
    p = git(repo, "merge-tree", "--write-tree", "--name-only", cur, ref, check=False)
    lines = p.stdout.split("\n")
    tree = lines[0].strip() if lines else ""
    if p.returncode == 0:
        c = git(repo, "commit-tree", tree, "-p", cur, "-p", ref, "-m", "sweep")
        return c.stdout.strip(), None
    if p.returncode != 1:
        raise RuntimeError("merge-tree failed:\n%s%s" % (p.stdout, p.stderr))
    paths = [l for l in lines[1:] if l.strip()
             and not l.startswith(("Auto-merging", "CONFLICT", "Removing", "Adding"))]
    return None, sorted(set(paths))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap.add_argument("--lane", default="win",
                    help="branch-name lane: win, mac, linux, or empty for all")
    ap.add_argument("--control", action="store_true",
                    help="replace each branch by its merge base -- every order must sweep fully")
    a38_fleet_state.add_args(ap)
    a = ap.parse_args()

    repo = a.repo
    # A41: one resolved input. `base` is a sha from the same answer as the
    # branch shas, so nothing here re-resolves a name against the local clone.
    fleet = a38_fleet_state.resolve(repo, a, a.lane)
    base = fleet.main

    refs = []
    for label, branch, sha in fleet.branches:
        if a.control:
            sha = git(repo, "merge-base", sha, base).stdout.strip()
        refs.append((label, sha, branch))

    print("=== %s ===" %
          ("CONTROL (merge bases -- every order must sweep fully)" if a.control
           else "TREATMENT"))
    print(fleet.header())
    if not refs:
        print("\n  VACUOUS: no branches in this lane -- nothing was measured.")
        return 3
    vacuous = fleet.vacuous(len(refs) * (len(refs) - 1) // 2)
    if vacuous:
        print("\n  VACUOUS: one branch is zero pairs -- every ordering below is")
        print("  the single branch, so there is nothing to compare it against.")
    print()

    # 1. the 09-25 measurement, reproduced: each branch alone into main.
    print("--- each branch alone into main (what the 09-25 cell measured) ---")
    alone_clean = 0
    for label, sha, _ in refs:
        c, conf = try_merge(repo, base, sha)
        if c:
            alone_clean += 1
            print("  %-6s CLEAN" % label)
        else:
            print("  %-6s CONFLICT: %s" % (label, ", ".join(conf)))
    print("  %d of %d merge into main individually\n" % (alone_clean, len(refs)))

    # 2. every ordering, exhaustively -- the question that one cannot answer.
    n = len(refs)
    if n > 8:
        print("--- %d branches: %d! orderings is too many to enumerate; use "
              "a38_fleet_sweep_probe.py ---" % (n, n))
        return 0
    total_orders = 1
    for k in range(2, n + 1):
        total_orders *= k
    print("--- every ordering, exhaustive (%d! = %d) ---" % (n, total_orders))
    hist = {}
    best = []
    for perm in itertools.permutations(refs):
        cur, depth, order = base, 0, []
        for label, sha, _ in perm:
            nxt, _conf = try_merge(repo, cur, sha)
            if nxt is None:
                break
            cur, depth = nxt, depth + 1
            order.append(label)
        hist[depth] = hist.get(depth, 0) + 1
        if depth > len(best):
            best = list(order)
    print("  depth histogram: %s" % dict(sorted(hist.items())))
    print("  deepest order: %s" % (" -> ".join(best) or "(none)"))
    print("  orderings landing ALL %d: %d of %d\n"
          % (n, hist.get(n, 0), sum(hist.values())))

    # 3. the depth-2 matrix -- who blocks whom, and on which paths.
    print("--- depth-2 matrix: after X lands, which others conflict, and where ---")
    for la, sa, _ in refs:
        m, _c = try_merge(repo, base, sa)
        if m is None:
            print("  %-6s does not merge into main at all" % la)
            continue
        blocked, paths = [], set()
        for lb, sb, _ in refs:
            if la == lb:
                continue
            c, conf = try_merge(repo, m, sb)
            if c is None:
                blocked.append(lb)
                paths.update(conf)
        book = sorted(p for p in paths if p in BOOKKEEPING)
        code = sorted(p for p in paths if p not in BOOKKEEPING)
        print("  after %-6s blocks %-28s bookkeeping: %-58s other: %s"
              % (la, ", ".join(blocked) or "nothing",
                 ", ".join(book) or "none", ", ".join(code) or "none"))

    if a.control:
        full = hist.get(n, 0)
        if full != sum(hist.values()):
            print("\nCONTROL FAILED: merge bases must sweep fully in EVERY order; "
                  "%d of %d did" % (full, sum(hist.values())))
            return 1
        print("\ncontrol OK: %d/%d orderings swept fully -- a shallow depth on the "
              "real branches is a measurement, not a broken merge"
              % (full, sum(hist.values())))
    if vacuous:
        print("\nSWEEP VACUOUS -- fewer than two branches in the lane, so no"
              " ordering was compared;\nrc=3 rather than 0, because this is not"
              " a pass")
        return 3
    return 0


if __name__ == "__main__":
    sys.exit(main())
