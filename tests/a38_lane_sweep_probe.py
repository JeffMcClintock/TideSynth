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

No worktree, no checkout, no network beyond `gh pr list`. `merge-tree
--write-tree` plus `commit-tree` do every merge in memory, so this is safe to
run against a tree the developer is working in.

The control
-----------
`--control` replaces each branch with its merge base against `main` -- branches
that provably contain nothing new. Every ordering must then reach full depth.
If it does not, the merge machinery here is broken and nothing else this probe
prints means anything. That arm is what makes a depth of 1 a measurement rather
than a bug.

Usage
-----
    python3 tests/a38_lane_sweep_probe.py [--repo PATH] [--lane win] [--control]

Exit 0 if the measurement completed (and, with --control, swept fully). The
depth is REPORTED, not asserted: it describes a moving queue, so a threshold
here would be a tripwire on someone else's merge habits.
"""
import argparse
import itertools
import os
import subprocess
import sys

BOOKKEEPING = {"BACKLOG.md", "BACKLOG-DONE.md", "JOURNAL.md",
               "docs/lessons.md", "docs/decisions.md"}


def git(repo, *args, check=True):
    p = subprocess.run(["git", "-C", repo] + list(args), capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    if check and p.returncode != 0:
        raise RuntimeError("git %s failed:\n%s%s" % (" ".join(args), p.stdout, p.stderr))
    return p


def lane_prs(repo, lane):
    """Open PRs whose head branch is in this lane, newest first."""
    p = subprocess.run(
        ["gh", "pr", "list", "--repo", "JeffMcClintock/TideSynth", "--state", "open",
         "--limit", "50", "--json", "number,headRefName",
         "--jq", '.[] | "\\(.number)\\t\\(.headRefName)"'],
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("gh pr list failed -- this probe needs the PR list:\n" + p.stderr)
    out = []
    for line in p.stdout.split("\n"):
        if not line.strip():
            continue
        num, branch = line.split("\t")
        if lane and not branch.startswith("tide/%s/" % lane):
            continue
        out.append(("#" + num, branch))
    return out


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
    a = ap.parse_args()

    repo = a.repo
    base = git(repo, "rev-parse", "origin/main").stdout.strip()
    prs = lane_prs(repo, a.lane)

    refs = []
    for label, branch in prs:
        r = git(repo, "rev-parse", "--verify", "--quiet",
                "refs/remotes/origin/" + branch, check=False)
        if r.returncode != 0:
            print("  (skipping %s -- no local ref for %s; fetch first)" % (label, branch))
            continue
        sha = r.stdout.strip()
        if a.control:
            sha = git(repo, "merge-base", sha, base).stdout.strip()
        refs.append((label, sha, branch))

    print("=== %s: lane '%s' from origin/main %s ===" %
          ("CONTROL (merge bases -- every order must sweep fully)" if a.control
           else "TREATMENT", a.lane, base[:9]))
    for label, sha, branch in refs:
        print("  %-6s %s  %s" % (label, sha[:9], branch))
    if not refs:
        print("  (no open PRs in this lane -- nothing to measure)")
        return 0
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
    return 0


if __name__ == "__main__":
    sys.exit(main())
