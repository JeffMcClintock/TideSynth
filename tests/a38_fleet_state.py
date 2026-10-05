#!/usr/bin/env python3
"""A41: the fleet state an A38 probe measures, as an INPUT rather than an ambient.

Why this exists
---------------
Both A38 probes used to discover what they measured by asking the live fleet,
which made every number they printed a function of the instant they ran. On
2026-10-01 `tests/a38_row_adjacency_probe.py` printed `PROBE FAILED (1)` in the
morning and `PROBE OK` in the afternoon from a byte-identical blob, one merge
commit apart -- so a recorded pass could not be re-checked, and a drifted fleet
was indistinguishable from a regression. That is BACKLOG **A41**.

Three defects, and the fix for each
-----------------------------------
  1. **Two ref sources that can disagree.** Enumeration came from
     `ls-remote --heads origin` (the remote, live) while every read resolved
     `origin/<branch>` (whatever this clone last fetched). Measured for A41 at
     one instant from one `origin/main` sha: Jeff's tree called `BACKLOG.md`
     *MAXIMALLY DIVERGENT, 2 of 2*, and a fresh `--depth 1 --single-branch`
     clone called it *inert on most branches, 0 of 2*. The two most opposite
     verdicts the script can print, from the same code against the same remote.
     **Fix:** one source for both. `ls-remote` already prints the sha beside the
     ref, so live mode uses that and never consults `origin/<branch>`.

  2. **A ref that would not resolve was skipped in silence.** `blob()` passes
     `check=False` and returns `None`, which the divergence loop skipped while
     `len(allb)` still counted the branch in the denominator -- so an unfetched
     branch quietly moved the numerator down. **Fix:** an unresolvable ref is a
     hard error naming the `git fetch` that repairs it, never a skip.

  3. **An empty fixture read as a pass.** With one branch in the lane there are
     no pairs to merge, so the discriminator control C2 held vacuously and the
     probe printed the same `PROBE OK` and `rc=0` as a real pass. **Fix:**
     `vacuous()` below, which the caller reports as `VACUOUS` rather than `OK`.

Pinned by default, live on request
----------------------------------
The default input is `tests/a38_fleet_state.json`, the state #618's controls were
measured against. Its refs are `refs/pull/<N>/head`, which GitHub keeps after a
branch is deleted -- all seven of those branches are gone, so `refs/heads/**`
would not have survived a week. A pinned run reproduces byte-for-byte; `--live`
measures today's fleet and says so in the header, so the two readings can never
be mistaken for each other.
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_MANIFEST = os.path.join(HERE, "a38_fleet_state.json")


def git(repo, *args, check=True, stdin=None, binary=False):
    """Run git. `binary=True` keeps stdout/stdin as bytes.

    Text mode decodes UTF-8 explicitly with `replace`: the default on Windows is
    cp1252, and BACKLOG.md carries em-dashes that abort a read partway through
    with a UnicodeDecodeError from a reader thread -- which surfaces as an
    unrelated NoneType later, not as an encoding error.
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


class FleetState:
    """The refs a probe measures, with every sha already resolved.

    `main` and `branches` are shas, not names. Nothing downstream re-resolves a
    name, which is defect 1 above: a name is only as stable as the clone holding
    it, and a sha is the same everywhere or absent everywhere.
    """

    def __init__(self, source, main_sha, branches, lane):
        self.source = source          # one line, printed in the probe header
        self.main = main_sha
        self.branches = branches      # [(label, branch, sha)], lane-filtered
        self.lane = lane
        self.live = source.startswith("live")

    def header(self):
        out = ["=== fleet state: " + self.source + " ===",
               "  main   " + self.main[:9],
               "  lane   '" + (self.lane or "(all)") + "', "
               + str(len(self.branches)) + " branch(es)"]
        for label, branch, sha in self.branches:
            out.append("  %-6s %s  %s" % (label, sha[:9], branch))
        if self.live:
            out.append("  NOTE: live input -- these numbers are a snapshot of"
                       " the fleet at this instant")
            out.append("        and are NOT expected to reproduce. Record them"
                       " with the shas above.")
        return "\n".join(out)

    def vacuous(self, n_pairs):
        """True when the fixture is too small for a pairwise arm to mean anything.

        A41's third defect: two branches is the minimum that produces one pair,
        and below that every pairwise control holds because nothing was tested.
        The caller must print VACUOUS, not OK.
        """
        return n_pairs == 0


def _require(repo, want, fetch):
    """Resolve every (label, ref, sha) locally, or fail naming the fetch.

    Defect 2: the old code skipped what it could not read. An unresolvable ref
    is a missing input, so it stops the run.
    """
    missing = [(l, r, s) for l, r, s in want
               if git(repo, "rev-parse", "--verify", "--quiet", s + "^{commit}",
                      check=False).returncode != 0]
    if missing and fetch:
        refs = sorted(set(r for _, r, _ in missing if r))
        if refs:
            print("  fetching %d missing ref(s)..." % len(refs))
            git(repo, "fetch", "--quiet", "origin", *refs, check=False)
        missing = [(l, r, s) for l, r, s in missing
                   if git(repo, "rev-parse", "--verify", "--quiet",
                          s + "^{commit}", check=False).returncode != 0]
    if missing:
        print("FLEET STATE UNRESOLVABLE -- %d ref(s) are not in this repo:"
              % len(missing), file=sys.stderr)
        for label, ref, sha in missing:
            print("  %-6s %s  %s" % (label, sha[:9], ref or "(no ref)"),
                  file=sys.stderr)
        print("\nRe-run with --fetch, or fetch them by hand:", file=sys.stderr)
        for ref in sorted(set(r for _, r, _ in missing if r)):
            print("  git -C %s fetch origin %s" % (repo, ref), file=sys.stderr)
        raise SystemExit(2)


def from_manifest(repo, path, lane, fetch=False):
    with open(path, encoding="utf-8") as f:
        m = json.load(f)
    want = [("main", m["main"].get("ref", ""), m["main"]["sha"])]
    picked = []
    for e in m["branches"]:
        if lane and not e["branch"].startswith("tide/%s/" % lane):
            continue
        picked.append(e)
        want.append((e["label"], e.get("ref", ""), e["sha"]))
    _require(repo, want, fetch)
    source = "pinned %s, recorded %s" % (os.path.basename(path), m["recorded"])
    return FleetState(source, m["main"]["sha"],
                      [(e["label"], e["branch"], e["sha"]) for e in picked],
                      lane)


def from_live(repo, lane, fetch=False):
    """Today's fleet, enumerated AND resolved from one `ls-remote` call.

    Defect 1: `ls-remote` prints `<sha>\\tref/...`, so the sha comes from the
    same answer as the name. `origin/<branch>` is never consulted.
    """
    out = git(repo, "ls-remote", "--heads", "origin",
              "refs/heads/tide/*").stdout
    picked = []
    for line in out.splitlines():
        if not line.strip():
            continue
        sha, ref = line.split()[0], line.split()[1]
        branch = ref.replace("refs/heads/", "")
        if lane and not branch.startswith("tide/%s/" % lane):
            continue
        picked.append((branch.split("/")[-1][:18], branch, sha))
    main_line = git(repo, "ls-remote", "origin", "refs/heads/main").stdout
    main_sha = main_line.split()[0] if main_line.strip() else ""
    if not main_sha:
        raise SystemExit("could not read origin's main from ls-remote")
    want = [("main", "refs/heads/main", main_sha)] + \
           [(l, "refs/heads/" + b, s) for l, b, s in picked]
    _require(repo, want, fetch)
    return FleetState("live fleet, read at run time", main_sha, picked, lane)


def add_args(ap):
    """The three flags every A38 probe now shares."""
    ap.add_argument("--fleet-state", default=DEFAULT_MANIFEST,
                    help="pinned fleet-state manifest (default: the recorded one)")
    ap.add_argument("--live", action="store_true",
                    help="measure today's fleet instead; output is a snapshot")
    ap.add_argument("--fetch", action="store_true",
                    help="fetch any ref the input names but this repo lacks")


def resolve(repo, a, lane):
    if a.live:
        return from_live(repo, lane, a.fetch)
    return from_manifest(repo, a.fleet_state, lane, a.fetch)
