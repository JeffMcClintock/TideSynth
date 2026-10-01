#!/usr/bin/env python3
"""A38, the CAUSE question: is the bookkeeping livelock line ADJACENCY, or is it
two runs genuinely writing different things in the same place?

Why this exists beside the other three A38 probes
------------------------------------------------
`tests/a38_sweep_probe.py`, `tests/a38_fleet_sweep_probe.py` and
`tests/a38_lane_sweep_probe.py` all measure HOW DEEP a merge sweep gets. None of
them measures WHY it stops, and A38's own recommendation turns on that: option
(b) "blank-line sections in BACKLOG.md" only helps if the conflicts are between
edits on ADJACENT lines, and option (d) "per-run journal files" only helps if
`JOURNAL.md` is genuinely divergent rather than merely named in a merge.

Two lanes have now argued this from sweep depth alone and reached opposite
conclusions -- 2026-09-25 "the journal half is conditional on run density",
2026-09-26 "`BACKLOG-DONE.md` is a fifth hot file". Sweep depth cannot settle
either claim. Content can.

What this measures
------------------
Three arms, none of which needs a checkout or a worktree -- every merge happens
in memory via `merge-tree --write-tree` plus `commit-tree`, so this is safe to
run against a tree the developer is working in.

  1. **Blob divergence.** For each bookkeeping file, how many DISTINCT blobs
     exist across `main` and every `tide/**` branch. A file that is
     byte-identical on two branches cannot conflict between them, however often
     a merge names it. This separates "hot" from "merely mentioned".

  2. **Touched-line map.** For one lane, which `BACKLOG.md` line numbers each
     branch's diff against `main` actually touches, and which row id sits there.
     Two branches editing the SAME line disagree; two editing ADJACENT lines do
     not, and git conflicts on both.

  3. **The adjacency experiment.** For every pair in the lane, re-merge after
     inserting ONE blank line between every pair of adjacent table rows in
     `BACKLOG.md`, applied to the merge base and BOTH branches. If a conflict
     disappears, adjacency was the cause and A38's option (b) addresses it. If
     it survives, the two branches disagree about one line and no amount of
     spacing helps.

     The transformation is a PROBE, not a proposal. A blank line between rows
     ends a markdown table, so shipping option (b) means BACKLOG.md stops being
     a table -- a cost for A38's ruling to weigh, not something this script
     recommends.

The controls, which are what make arm 3 a measurement
----------------------------------------------------
  C1  Inference check, both directions: a branch whose blob for file F equals
      `main`'s must produce an EMPTY `git diff` in F, and a branch whose blob
      differs must produce a non-empty one. Arm 1's whole reading rests on this.

  C2  **The discriminator.** A pair that conflicts because both edit the SAME
      line must STILL conflict after the transformation. If blank lines "fixed"
      those too, the transformation is corrupting the merge rather than
      isolating a mechanism, and arm 3 means nothing.

  C3  A pair that already merges cleanly must not be made to conflict by the
      transformation.

Usage
-----
    python3 tests/a38_row_adjacency_probe.py [--repo PATH] [--lane win]

Exit 0 if the measurement completed and every control held; 1 if a control
failed. The divergence numbers and conflict verdicts are REPORTED, not asserted
-- they describe a moving queue, so a threshold here would be a tripwire on
someone else's merge habits.
"""
import argparse
import itertools
import subprocess
import sys

BOOKKEEPING = ["BACKLOG.md", "BACKLOG-DONE.md", "JOURNAL.md",
               "docs/decisions.md", "docs/lessons.md"]


def git(repo, *args, check=True, stdin=None, binary=False):
    """Run git. `binary=True` keeps stdout/stdin as bytes.

    Text mode decodes UTF-8 explicitly with `replace`: the default on Windows is
    cp1252, and BACKLOG.md carries em-dashes and curly quotes that abort a read
    partway through with a UnicodeDecodeError from a reader thread -- which
    surfaces as an unrelated NoneType later, not as an encoding error.
    """
    kw = {"input": stdin}
    if not binary:
        kw.update(encoding="utf-8", errors="replace")
    r = subprocess.run(["git", "-C", repo, *args], capture_output=True, **kw)
    if check and r.returncode != 0:
        err = r.stderr if not binary else r.stderr.decode("utf-8", "replace")
        raise RuntimeError("git " + " ".join(args) + " -> "
                           + str(r.returncode) + "\n" + err)
    return r


def blob(repo, ref, path):
    r = git(repo, "rev-parse", ref + ":" + path, check=False)
    return r.stdout.strip() if r.returncode == 0 else None


def branches(repo):
    out = git(repo, "ls-remote", "--heads", "origin", "refs/heads/tide/*").stdout
    return [l.split()[1].replace("refs/heads/", "")
            for l in out.splitlines() if l.strip()]


def merge(repo, base, a, b):
    """Three-way merge in memory. Returns (ok, conflicting_paths)."""
    r = git(repo, "merge-tree", "--write-tree", "--name-only",
            "--merge-base", base, a, b, check=False)
    if r.returncode == 0:
        return True, []
    lines = r.stdout.splitlines()
    # merge-tree quotes a path containing anything unusual. Unquote, so a
    # corrupted entry name shows up as itself rather than hiding behind quotes.
    out = []
    for l in lines[1:]:
        l = l.strip()
        if not l:
            continue
        if l.startswith('"') and l.endswith('"'):
            l = l[1:-1]
        out.append(l)
    return False, out


def respace(data):
    """Insert one blank line between every pair of adjacent table rows.

    Operates on BYTES, so a file with non-ASCII prose round-trips untouched
    outside the inserted newlines.
    """
    lines = data.split(b"\n")
    out = []
    for i, ln in enumerate(lines):
        out.append(ln)
        nxt = lines[i + 1] if i + 1 < len(lines) else b""
        if ln.startswith(b"|") and nxt.startswith(b"|"):
            out.append(b"")
    return b"\n".join(out)


def rewrite_commit(repo, commit, path):
    """Commit whose tree is `commit`'s with `path` respaced. Root-level only."""
    content = git(repo, "show", commit + ":" + path, binary=True).stdout
    oid = git(repo, "hash-object", "-w", "--stdin", binary=True,
              stdin=respace(content)).stdout.decode().strip()
    fixed = []
    for e in git(repo, "ls-tree", commit).stdout.splitlines():
        meta, name = e.split("\t", 1)
        if name == path:
            mode, typ, _ = meta.split()
            fixed.append(mode + " " + typ + " " + oid + "\t" + name)
        else:
            fixed.append(e)
    # mktree MUST be fed as bytes. In text mode Python translates each "\n" to
    # os.linesep on write, so on Windows git reads the entry name as
    # "BACKLOG.md\r" -- a DIFFERENT path, which then conflicts with everything
    # and reads as a genuine merge result. Control C2 is what caught this.
    payload = ("\n".join(fixed) + "\n").encode()
    tree = git(repo, "mktree", binary=True, stdin=payload).stdout.decode().strip()
    return git(repo, "commit-tree", tree, "-m", "respaced probe").stdout.strip()


def touched_lines(repo, base_ref, branch, path):
    """[(old_line, row_id)] for each hunk of branch's diff against base_ref."""
    r = git(repo, "diff", base_ref + "..." + branch, "-U0", "--", path,
            check=False)
    hunks, cur = [], None
    for ln in r.stdout.splitlines():
        if ln.startswith("@@"):
            cur = ln.split()[1].lstrip("-").split(",")[0]
        elif ln.startswith("+") and not ln.startswith("+++") and cur:
            cells = ln[1:].split("|")
            rid = cells[1].strip() if len(cells) > 1 else "?"
            hunks.append((int(cur), rid[:12]))
            cur = None
    return hunks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=".")
    ap.add_argument("--lane", default="win")
    a = ap.parse_args()
    repo, lane = a.repo, a.lane
    main_ref = "origin/main"
    main_sha = git(repo, "rev-parse", "--short", main_ref).stdout.strip()
    allb = branches(repo)
    laneb = [b for b in allb if "/" + lane + "/" in b]
    fails = []

    print("=== A38 adjacency probe: lane '" + lane + "' from "
          + main_ref + " " + main_sha + " ===\n")

    print("--- arm 1: blob divergence across main + every tide/** branch ---")
    print("  %-22s %8s %17s   %s" % ("file", "distinct", "differ from main",
                                     "verdict"))
    for f in BOOKKEEPING:
        m = blob(repo, main_ref, f)
        got = set([m]) if m else set()
        differ = 0
        for b in allb:
            x = blob(repo, "origin/" + b, f)
            if x:
                got.add(x)
                if x != m:
                    differ += 1
        if differ == len(allb):
            verdict = "MAXIMALLY DIVERGENT"
        elif differ * 2 <= len(allb):
            verdict = "inert on most branches"
        else:
            verdict = "partly divergent"
        print("  %-22s %8d %17s   %s"
              % (f, len(got), "%d of %d" % (differ, len(allb)), verdict))

    print("\n--- control C1: blob identity <-> empty diff, both directions ---")
    c1 = 0
    for f in BOOKKEEPING:
        m = blob(repo, main_ref, f)
        for b in allb:
            x = blob(repo, "origin/" + b, f)
            if x is None:
                continue
            empty = git(repo, "diff", "--quiet", main_ref, "origin/" + b, "--",
                        f, check=False).returncode == 0
            if (x == m) != empty:
                fails.append("C1 " + b + ":" + f + " blob_eq="
                             + str(x == m) + " diff_empty=" + str(empty))
            c1 += 1
    print("  %d (branch, file) pairs checked, %d mismatches"
          % (c1, len([x for x in fails if x.startswith("C1")])))

    print("\n--- arm 2: which BACKLOG.md lines each '" + lane
          + "' branch touches ---")
    touch = {}
    for b in laneb:
        t = touched_lines(repo, main_ref, "origin/" + b, "BACKLOG.md")
        touch[b] = t
        pretty = ", ".join("L%d(%s)" % (n, r) for n, r in t) or "(none)"
        print("  %-44s %s" % (b.split("/")[-1][:44], pretty))
    hits = {}
    for b, t in touch.items():
        for n, r in t:
            hits.setdefault(n, []).append(b.split("/")[-1][:28])
    print("\n  lines edited by MORE THAN ONE branch (genuine disagreement):")
    any_same = False
    for n in sorted(hits):
        if len(hits[n]) > 1:
            any_same = True
            print("    L%d: %d branches -> %s"
                  % (n, len(hits[n]), ", ".join(hits[n])))
    if not any_same:
        print("    (none)")
    print("  ADJACENT-line pairs (no disagreement; git conflicts anyway):")
    any_adj = False
    for x, y in itertools.combinations(sorted(hits), 2):
        if y - x == 1:
            any_adj = True
            print("    L%d vs L%d: %s  ||  %s"
                  % (x, y, ", ".join(hits[x]), ", ".join(hits[y])))
    if not any_adj:
        print("    (none)")

    print("\n--- arm 3: does respacing BACKLOG.md unblock each pair? ---")
    print("  %-40s %22s %22s  %s" % ("pair", "before", "after", "reading"))
    same_line, adjacent = [], []
    for x, y in itertools.combinations(sorted(hits), 2):
        if y - x == 1:
            adjacent += [(bx, by) for bx in hits[x] for by in hits[y]]
    for n in hits:
        if len(hits[n]) > 1:
            same_line += list(itertools.combinations(hits[n], 2))

    for b1, b2 in itertools.combinations(laneb, 2):
        base = git(repo, "merge-base", "origin/" + b1,
                   "origin/" + b2).stdout.strip()
        ok_b, paths_b = merge(repo, base, "origin/" + b1, "origin/" + b2)
        rb = rewrite_commit(repo, base, "BACKLOG.md")
        r1 = rewrite_commit(repo, "origin/" + b1, "BACKLOG.md")
        r2 = rewrite_commit(repo, "origin/" + b2, "BACKLOG.md")
        ok_a, paths_a = merge(repo, rb, r1, r2)
        s1, s2 = b1.split("/")[-1][:18], b2.split("/")[-1][:18]
        bl = "CLEAN" if ok_b else ",".join(
            p.split("/")[-1] for p in paths_b)[:21]
        al = "CLEAN" if ok_a else ",".join(
            p.split("/")[-1] for p in paths_a)[:21]
        bl_bk = "BACKLOG.md" in paths_b
        al_bk = "BACKLOG.md" in paths_a
        if bl_bk and not al_bk:
            reading = "ADJACENCY -> (b) helps"
        elif bl_bk and al_bk:
            reading = "DISAGREEMENT -> (b) no help"
        elif not bl_bk and al_bk:
            reading = "!! transform CREATED one"
        else:
            reading = "-"
        print("  %-40s %22s %22s  %s" % (s1 + " + " + s2, bl, al, reading))
        is_same = any(set([a1[:18], a2[:18]]) == set([s1, s2])
                      for a1, a2 in same_line)
        # C2 applies only to a same-line pair that ACTUALLY conflicted in
        # BACKLOG.md. Two branches can touch one line and write the SAME text
        # there -- identical content is a no-op, never a conflict -- and that is
        # arm 1's finding, not a failure of this control. Requiring `bl_bk` is
        # what keeps C2 a discriminator instead of a restatement of `same_line`.
        if is_same and bl_bk and not al_bk:
            fails.append("C2 " + s1 + "+" + s2
                         + ": same-line pair stopped conflicting after respace")
        if not bl_bk and al_bk:
            fails.append("C3 " + s1 + "+" + s2
                         + ": respace created a BACKLOG.md conflict")

    print("\n--- controls ---")
    for tag, label in (("C1", "blob-identity <-> empty-diff"),
                       ("C2", "same-line pairs still conflict after respace"),
                       ("C3", "respace creates no new conflict")):
        bad = any(f.startswith(tag) for f in fails)
        print("  %s %-48s %s" % (tag, label, "FAIL" if bad else "OK"))
    for f in fails:
        print("    " + f)
    print("\n" + ("PROBE OK" if not fails
                  else "PROBE FAILED (%d)" % len(fails)))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
