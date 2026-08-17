#!/usr/bin/env python3
"""Fail if agent commits reached a protected branch without going through a PR.

BACKLOG A17/A18. The third of the shared-checkout guards, after
`check-commit-authorship.py` (A14, *who* wrote a commit) and
`check-commit-completeness.py` (A16, *what is in* a commit). This one asks
*how a commit arrived*.

Why it exists
-------------
A17 relaxes the GATED rule so a run may repair a build break in shared code,
on the reasoning that nothing lands without Jeff reviewing the PR. That premise
is only two-thirds true, measured 2026-08-17:

    SynthEditLib (public)   main protected, "Agent PRs only" ruleset ACTIVE
    SynthEdit    (private)  master UNPROTECTED, no ruleset

Private repos cannot carry rulesets on the current plan, and **Jeff has decided
not to upgrade** (2026-08-18). So in `SE16` the bot holds write access to
`master` with nothing mechanical in the way: every PR a run has opened there was
voluntary compliance with the run prompt.

Prevention is unavailable, so this is the detection half of a best-effort
control. It does not stop a direct push. It makes one impossible to leave
unnoticed -- which is the difference between convention and convention with an
alarm on it.

How it decides
--------------
In a merge-based PR workflow, a branch's commits reach the default branch as
ancestors of a merge commit; only the merge itself sits on the first-parent
chain. A commit pushed straight to the default branch sits on that chain
directly. So:

    walk `git rev-list --first-parent <branch>`
    flag any commit that is not a merge AND whose AUTHOR *and* COMMITTER
    are both the agent

**The committer field is the discriminator, and getting this wrong is the whole
difficulty.** Measured on `SE16` 2026-08-18, a first draft of this check flagged
eight commits as bypassing review. Every one was author `tide-rack-bot`,
committer `Jeff McClintock` -- Jeff landing agent work by rebase, squash or
cherry-pick, which preserves authorship but restamps the committer. That is a
human handling the change, i.e. **exactly the property A17 wants assured**, and
reporting it as a bypass would have been eight false alarms on the first run.
A scheduled run exports all four `GIT_*` variables, so a genuine direct push has
the agent in *both* fields.

**Caveat, stated because it changes the meaning of a pass:** a repository that
squash-merges puts one non-merge commit on the first-parent chain per PR. The
committer test above handles the case where a human does the squashing, which is
the normal one. It would NOT catch an agent that squash-merged its own PR --
but an agent merging its own PR is already forbidden outright by STEP 5, and
this check is not the thing standing in its way.

Usage
-----
    python scripts/check-no-direct-commits.py --repo ../SE16 --branch master
    python scripts/check-no-direct-commits.py --repo ../SynthEditLib
    python scripts/check-no-direct-commits.py --repo ../SE16 --since 2026-08-09
    python scripts/check-no-direct-commits.py --selftest

`--since` exists because the bot identity landed 2026-08-09 (A2); before that a
run pushed as Jeff and this check cannot distinguish its commits from his.
Default is to scan the whole branch and report anything it finds.

Exit codes: 0 clean, 1 direct commits found, 2 could not run the check.
"""

import argparse
import subprocess
import sys

DEFAULT_AGENT = "tide-rack-bot"
SEP = "\x1f"


def git(repo, *args, env_author=None, env_committer=None):
    import os as _os
    env = dict(_os.environ)
    if env_author is not None:
        env["GIT_AUTHOR_NAME"] = env_author
        env["GIT_AUTHOR_EMAIL"] = "%s@example.invalid" % env_author.replace(" ", "_")
    if env_committer is not None:
        env["GIT_COMMITTER_NAME"] = env_committer
        env["GIT_COMMITTER_EMAIL"] = "%s@example.invalid" % env_committer.replace(" ", "_")
    result = subprocess.run(
        ["git", "-C", repo] + list(args), env=env,
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        raise RuntimeError((result.stderr or result.stdout).strip())
    return result.stdout


def default_branch(repo):
    try:
        ref = git(repo, "symbolic-ref", "--short", "refs/remotes/origin/HEAD").strip()
        return ref.split("/", 1)[1] if "/" in ref else ref
    except RuntimeError:
        return "main"


def scan(repo, branch, agent, since):
    """Non-merge commits by `agent` sitting on the first-parent chain."""
    ref = "origin/%s" % branch
    try:
        git(repo, "rev-parse", "--verify", ref)
    except RuntimeError:
        ref = branch  # no remote-tracking ref; fall back to the local branch

    args = ["log", "--first-parent", "--no-merges",
            "--format=" + SEP.join(["%h", "%an", "%ae", "%cn", "%ce", "%ad", "%s"]),
            "--date=short"]
    if since:
        args.append("--since=%s" % since)
    args.append(ref)

    found = []
    for line in git(repo, *args).splitlines():
        if not line.strip():
            continue
        sha, an, ae, cn, ce, ad, subject = line.split(SEP, 6)
        agent_authored = (an == agent or agent in ae)
        agent_committed = (cn == agent or agent in ce)
        # Both, deliberately: author alone catches Jeff's rebases of agent work.
        if agent_authored and agent_committed:
            found.append((sha, an, ad, subject))
    return ref, found


def selftest():
    """Prove it distinguishes a merged PR from a direct push."""
    import tempfile, shutil, os

    failures = []
    root = tempfile.mkdtemp()
    try:
        git(root, "init", "-q", "-b", "main", ".")
        git(root, "config", "user.email", "human@example.invalid")
        git(root, "config", "user.name", "A Human")

        def commit(name, msg, author="A Human", committer=None):
            # Explicit -c overrides, because the caller's shell may export
            # GIT_AUTHOR_NAME etc. -- a run's shell does exactly that, and an
            # earlier draft of this selftest silently tested nothing because of
            # it: every "human" commit came out authored as the bot.
            committer = committer or author
            with open(os.path.join(root, name), "w") as fh:
                fh.write(msg + "\n")
            git(root, "add", name)
            git(root,
                "-c", "user.name=%s" % author,
                "-c", "user.email=%s@example.invalid" % author.replace(" ", "_"),
                "commit", "-q", "-m", msg,
                env_author=author, env_committer=committer)

        commit("base.txt", "base")

        # A properly merged agent PR: agent commits on a branch, merge on main.
        git(root, "checkout", "-q", "-b", "feature")
        commit("f.txt", "agent work on a branch", author=DEFAULT_AGENT,
               committer=DEFAULT_AGENT)
        git(root, "checkout", "-q", "main")
        git(root, "merge", "-q", "--no-ff", "feature", "-m", "Merge pull request #1")

        _, found = scan(root, "main", DEFAULT_AGENT, None)
        if found:
            failures.append("merged PR was flagged: %s" % found)
            print("   FAIL  merged agent PR should be clean")
        else:
            print("   ok    merged agent PR is clean")

        # A direct push: agent commits straight onto main.
        commit("d.txt", "agent commit straight to main", author=DEFAULT_AGENT,
               committer=DEFAULT_AGENT)
        _, found = scan(root, "main", DEFAULT_AGENT, None)
        if len(found) != 1:
            failures.append("direct push not caught: %s" % found)
            print("   FAIL  direct agent commit should be flagged")
        else:
            print("   ok    direct agent commit is flagged (%s)" % found[0][0])

        # A human's direct commit is not this check's business.
        commit("h.txt", "human commit straight to main")
        _, found = scan(root, "main", DEFAULT_AGENT, None)
        if len(found) != 1:
            failures.append("human commit changed the count: %s" % found)
            print("   FAIL  human direct commit should be ignored")
        else:
            print("   ok    human direct commit ignored")

        # Jeff rebasing/squashing agent work: author=agent, committer=human.
        # This is A17's premise being SATISFIED and must not be flagged.
        commit("r.txt", "agent work landed by Jeff (rebase/squash)",
               author=DEFAULT_AGENT, committer="A Human")
        _, found = scan(root, "main", DEFAULT_AGENT, None)
        if len(found) != 1:
            failures.append("human-committed agent work was flagged: %s" % found)
            print("   FAIL  human-committed agent work should be clean")
        else:
            print("   ok    human-committed agent work ignored (the SE16 case)")
    finally:
        shutil.rmtree(root, ignore_errors=True)

    if failures:
        print("\nSELFTEST FAILED:")
        for f in failures:
            print("  %s" % f)
        return 1
    print("\nselftest passed")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Assert agent commits reached a branch via PR, not directly.")
    parser.add_argument("--repo", default=".")
    parser.add_argument("--branch", default=None,
                        help="default: the repo's origin/HEAD")
    parser.add_argument("--agent", default=DEFAULT_AGENT)
    parser.add_argument("--since", default=None,
                        help="e.g. 2026-08-09, when the bot identity landed")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    if args.selftest:
        return selftest()

    try:
        branch = args.branch or default_branch(args.repo)
        ref, found = scan(args.repo, branch, args.agent, args.since)
    except RuntimeError as exc:
        print("cannot run: %s" % exc)
        return 2

    print("%s: scanned %s (first-parent, non-merge) for %s commits"
          % (args.repo, ref, args.agent))

    if found:
        print("\n%d COMMIT(S) REACHED %s WITHOUT A PR:" % (len(found), ref))
        for sha, an, ad, subject in found:
            print("  %s  %s  %s" % (sha, ad, subject))
        print("\nEach of these was pushed straight to the default branch by the "
              "agent, bypassing review -- see BACKLOG A17/A18.")
        print("Do not rewrite them: they are published history, and rewriting a "
              "shared default branch is worse than the problem.")
        print("Report them to Jeff, and note them in the journal. If one is "
              "yours and unpushed elsewhere, the fix going forward is a branch "
              "and a PR, not an amend.")
        return 1

    print("clean -- every %s commit arrived as a merge" % args.agent)
    return 0


if __name__ == "__main__":
    sys.exit(main())
