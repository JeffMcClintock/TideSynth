#!/usr/bin/env python3
"""A38, from the merge-sweep side: how many of the fleet's OPEN PRs can one
human sweep actually land, and in what order?

Why this exists, and why it is not `a38_sweep_probe.py`
-------------------------------------------------------
`tests/a38_sweep_probe.py` (2026-09-22) replays all 24 orderings of the four
open `tide/win/**` branches and reports how deep the sweep gets. It answered a
question about ONE LANE and it answered it well: depth 0 before that run's
resolution, depth 1 after, full sweep 0/24 either way.

But Jeff does not sweep one lane. On 2026-09-23 there are NINE open PRs across
three lanes plus a build fix, and the number that actually decides how much of
the queue drains is the maximum over ALL of them -- which no probe has ever
measured, because 9! = 362,880 orderings is not a thing you enumerate.

So this probe searches instead of enumerating, and reports the ORDER, because an
order is the part a human can act on.

How it searches
---------------
Depth-first over subsets of the branch set, memoised on the frozenset of
branches already merged, so each reachable subset is expanded once: at most
2**N states rather than N! paths. For N=9 that is 512 states, not 362,880.

The memo is an APPROXIMATION and this is the one thing to hold against the
result: two different orders reaching the same subset can leave different
merged trees, and this keeps the first. It can therefore UNDERSTATE the true
maximum. It cannot overstate it -- every reported order is replayed and
verified before it is printed -- so the headline number is a floor, which is
the safe direction for a number someone is going to act on.

No worktree, no checkout, no network. `git merge-tree --write-tree` computes
each merge in memory and `git commit-tree` chains the result, so a 9-deep sweep
costs nine object writes and nothing else. That is what makes the search
affordable, and it is why this probe can run against a repo the developer is
working in without touching the working tree at all.

The control
-----------
`--control` replays the same search with each branch's MERGE BASE with main
substituted for the branch itself -- i.e. a set of branches that provably
contain nothing new. It must sweep to full depth. If it does not, the search is
broken and nothing this probe says about the real branches means anything.

Usage
-----
    python3 tests/a38_fleet_sweep_probe.py [--repo PATH] [--control]
                                           [--first BRANCH] [--verbose]

`--first` pins one branch to the front of every order, which is how you ask
"does landing the build fix first unjam anything?" rather than guessing.

Exit code 0 if the search completed (and, with --control, the control swept
fully). The depth is REPORTED, not asserted: this probe measures a moving
queue, so a threshold here would be a tripwire on someone else's merge habits.
"""
import argparse
import os
import subprocess
import sys

BOOKKEEPING = {"BACKLOG.md", "JOURNAL.md", "docs/lessons.md", "docs/decisions.md"}


def git(repo, *args, check=True):
    p = subprocess.run(["git", "-C", repo] + list(args),
                       capture_output=True, text=True, encoding="utf-8",
                       errors="replace")
    if check and p.returncode != 0:
        raise RuntimeError("git %s failed:\n%s%s" % (" ".join(args), p.stdout, p.stderr))
    return p


def open_prs(repo):
    """The fleet's open PRs, as (label, branch) -- from gh if available."""
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
        out.append(("#" + num, branch))
    return out


def try_merge(repo, cur, ref):
    """Merge `ref` into commit `cur` in memory.

    Returns (new_commit, None) on a clean merge, or (None, [conflicting paths]).
    """
    p = git(repo, "merge-tree", "--write-tree", "--name-only", cur, ref, check=False)
    lines = p.stdout.split("\n")
    tree = lines[0].strip() if lines else ""
    if p.returncode == 0:
        c = git(repo, "commit-tree", tree, "-p", cur, "-p", ref, "-m", "sweep")
        return c.stdout.strip(), None
    if p.returncode != 1:
        raise RuntimeError("merge-tree failed:\n%s%s" % (p.stdout, p.stderr))
    paths = [l for l in lines[1:] if l.strip() and not l.startswith(("Auto-merging",
                                                                    "CONFLICT", "Removing",
                                                                    "Adding"))]
    return None, sorted(set(paths))


def search(repo, base, refs, verbose):
    """Deepest sweep reachable from `base`. Returns (order, blocked_at)."""
    best = {"order": [], "blocked": {}}
    seen = set()
    pair_conflicts = {}

    def walk(cur, merged, order):
        # Memoise on the SET of branches merged, not the path that got here.
        # Without this the control -- where everything merges -- explores N!
        # paths instead of 2**N states, and 9! does not finish.
        key = frozenset(merged)
        if key in seen:
            return
        seen.add(key)
        if len(order) > len(best["order"]):
            best["order"] = list(order)
        for label, ref in refs:
            if label in merged:
                continue
            new, conflicts = try_merge(repo, cur, ref)
            if new is None:
                pair_conflicts.setdefault(label, set()).update(conflicts)
                best["blocked"].setdefault(key, {})[label] = conflicts
                continue
            if verbose:
                print("    %s+ %s" % ("  " * len(order), label))
            walk(new, merged | {label}, order + [label])

    walk(base, frozenset(), [])
    return best, pair_conflicts


def replay(repo, base, refs, order):
    """Re-merge `order` from scratch, so a printed order is a verified one."""
    byname = dict(refs)
    cur = base
    for label in order:
        cur, conflicts = try_merge(repo, cur, byname[label])
        if cur is None:
            return False, label, conflicts
    return True, None, []


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap.add_argument("--control", action="store_true",
                    help="replay with each branch replaced by its merge base -- must sweep fully")
    ap.add_argument("--first", help="pin this branch (or PR number) to the front of every order")
    ap.add_argument("--verbose", action="store_true")
    a = ap.parse_args()

    repo = a.repo
    base = git(repo, "rev-parse", "origin/main").stdout.strip()
    prs = open_prs(repo)

    refs = []
    for label, branch in prs:
        r = git(repo, "rev-parse", "--verify", "--quiet", "refs/remotes/origin/" + branch, check=False)
        if r.returncode != 0:
            print("  (skipping %s -- no local ref for %s; fetch first)" % (label, branch))
            continue
        sha = r.stdout.strip()
        if a.control:
            sha = git(repo, "merge-base", sha, base).stdout.strip()
        refs.append((label, sha))

    print("=== %s: fleet sweep from origin/main %s ===" %
          ("CONTROL (merge bases -- must sweep fully)" if a.control else "TREATMENT",
           base[:9]))
    for (label, branch), (_, sha) in zip(prs, refs):
        print("  %-6s %s  %s" % (label, sha[:9], branch))
    print()

    # One label -> sha map for the whole run. The search may drop a pinned
    # branch from `refs`, but `replay` must still be able to look it up, and a
    # replay that silently skipped it would verify the wrong order.
    allrefs = list(refs)

    start = base
    pinned = []
    if a.first:
        want = [r for r in refs if r[0] == a.first or r[0] == "#" + a.first.lstrip("#")]
        if not want:
            raise SystemExit("--first %s is not one of the open PRs" % a.first)
        label, sha = want[0]
        start, conflicts = try_merge(repo, base, sha)
        if start is None:
            raise SystemExit("--first %s does not merge cleanly into main: %s"
                             % (label, ", ".join(conflicts)))
        refs = [r for r in refs if r[0] != label]
        pinned = [label]
        print("pinned first: %s merges cleanly into main\n" % label)

    best, pair_conflicts = search(repo, start, refs, a.verbose)
    order = pinned + best["order"]

    ok, stalled, conflicts = replay(repo, base, allrefs, order)

    total = len(prs)
    print("DEEPEST VERIFIED SWEEP: %d of %d open PRs" % (len(order), total))
    print("  order: " + " -> ".join(order))
    print("  replay: %s" % ("clean" if ok else "STALLED at %s on %s" % (stalled, ", ".join(conflicts))))
    print("  left behind: " + (", ".join(l for l, _ in prs if l not in order) or "none"))

    if pair_conflicts:
        print("\nWhat each unmergeable PR conflicted on, somewhere in the search:")
        for label in sorted(pair_conflicts):
            paths = sorted(pair_conflicts[label])
            book = [p for p in paths if p in BOOKKEEPING]
            code = [p for p in paths if p not in BOOKKEEPING]
            print("  %-6s bookkeeping: %-52s other: %s"
                  % (label, ", ".join(book) or "none", ", ".join(code) or "none"))

    if a.control and len(order) != total:
        print("\nCONTROL FAILED: merge bases should sweep fully, got %d/%d" % (len(order), total))
        return 1
    if a.control:
        print("\ncontrol OK: %d/%d, the search finds a full sweep when one exists" % (len(order), total))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
