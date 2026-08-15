#!/usr/bin/env python3
"""Fail if any commit about to be pushed was not authored by the expected identity.

BACKLOG A14. STEP 0.7 of the run prompt asserts, at the top of every run, that
`gh api user` answers `tide-rack-bot`. That proves the **running process** is
the bot. It says nothing about the **repository**, and the difference is not
academic:

On 2026-08-15 a run had ten files staged in `SE16` and `SynthEditLib` and had
gone off to build. A concurrent Claude session on the same box committed those
staged changes -- onto the run's own branches -- authored and committed as
`Jeff McClintock`. The content was correct. The authorship was not, and
authorship is the entire audit trail: Jeff sits on every branch ruleset's bypass
list, so a commit stamped with his name is indistinguishable from one he made.
STEP 0.7 had passed, and would pass again, because it had already run and was
never wrong about the thing it actually checks.

This check closes that by asserting the property that matters -- who authored
the commits -- at the moment it matters, immediately before the push. It catches
a foreign commit regardless of how it arrived: a concurrent agent, a stray
`git commit` in another terminal, a rebase that picked up someone else's work,
or the four `GIT_*` variables simply not being exported.

Usage
-----
    python scripts/check-commit-authorship.py                    # vs origin/HEAD
    python scripts/check-commit-authorship.py --range main..HEAD
    python scripts/check-commit-authorship.py --repo ../SE16     # any checkout
    python scripts/check-commit-authorship.py --expect "Jeff McClintock"

Default expectation is `tide-rack-bot`, because that is what a scheduled run
must be. An interactive session that legitimately commits as Jeff should pass
`--expect` rather than skip the check.

Exit codes: 0 all commits match, 1 a mismatch, 2 the range could not be worked
out (unknown remote, detached HEAD, not a repository).
"""

import argparse
import subprocess
import sys

DEFAULT_EXPECT = "tide-rack-bot"

# The separator has to be something no name or address contains.
SEP = "\x1f"
FORMAT = SEP.join(["%h", "%an", "%ae", "%cn", "%ce", "%s"])


def git(repo, *args):
    """Run git in repo, returning stdout. Raises on failure."""
    result = subprocess.run(
        ["git", "-C", repo] + list(args),
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        raise RuntimeError((result.stderr or result.stdout).strip())
    return result.stdout


def default_range(repo):
    """`<upstream-or-default>..HEAD`, preferring the branch's own upstream.

    A branch pushed once already has an upstream, and comparing against it would
    only show *new* commits -- which is wrong here, because a foreign commit may
    have been pushed by an earlier attempt. So the default branch wins: the
    question is always "who wrote everything on this branch".
    """
    head = git(repo, "rev-parse", "--abbrev-ref", "HEAD").strip()
    if head == "HEAD":
        raise RuntimeError("detached HEAD -- pass --range explicitly")
    try:
        base = git(repo, "symbolic-ref", "--short", "refs/remotes/origin/HEAD").strip()
    except RuntimeError:
        raise RuntimeError(
            "cannot determine origin/HEAD -- run "
            "`git remote set-head origin -a`, or pass --range explicitly")
    if base.endswith("/" + head):
        return None  # on the default branch itself; nothing to compare
    return "%s..HEAD" % base


def commits(repo, rev_range):
    out = git(repo, "log", "--format=" + FORMAT, rev_range)
    for line in out.splitlines():
        if not line.strip():
            continue
        sha, an, ae, cn, ce, subject = line.split(SEP, 5)
        yield sha, an, ae, cn, ce, subject


def matches(expect, name, email):
    """True if either the name or the email identifies the expected party.

    Deliberately loose on the email: the bot's address is
    `314850083+tide-rack-bot@users.noreply.github.com`, and pinning the numeric
    prefix would make the check break when nobody meant it to.
    """
    expect_low = expect.lower()
    return expect_low == name.lower() or expect_low in email.lower()


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", default=".", help="repository (default: cwd)")
    parser.add_argument("--range", dest="rev_range",
                        help="commit range (default: origin/<default>..HEAD)")
    parser.add_argument("--expect", default=DEFAULT_EXPECT,
                        help="expected author (default: %s)" % DEFAULT_EXPECT)
    args = parser.parse_args()

    try:
        rev_range = args.rev_range or default_range(args.repo)
    except RuntimeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2

    if rev_range is None:
        print("on the default branch, nothing to check")
        return 0

    try:
        found = list(commits(args.repo, rev_range))
    except RuntimeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2

    bad = [c for c in found
           if not matches(args.expect, c[1], c[2])
           or not matches(args.expect, c[3], c[4])]

    print("%d commit(s) in %s, expecting %s" % (len(found), rev_range, args.expect))

    if bad:
        print("\n%d NOT authored by %s:" % (len(bad), args.expect))
        for sha, an, ae, cn, ce, subject in bad:
            print("  %s  %s" % (sha, subject))
            print("        author:    %s <%s>" % (an, ae))
            print("        committer: %s <%s>" % (cn, ce))
        print("\nDo not push. Either another process committed on this branch "
              "(see BACKLOG A14), or the four GIT_* variables were not exported.")
        print("If the content is yours, re-author it:")
        print("  git commit --amend --reset-author      # last commit only")
        print("  git rebase --exec 'git commit --amend --no-edit --reset-author' "
              "<base>   # a range")
        print("Do not rewrite commits another session may be building on, and "
              "never rewrite anything already pushed.")
        return 1

    print("all commits authored by %s" % args.expect)
    return 0


if __name__ == "__main__":
    sys.exit(main())
