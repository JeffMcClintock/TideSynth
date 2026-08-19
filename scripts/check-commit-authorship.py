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

Pushed commits are reported, not failed -- BACKLOG A26
------------------------------------------------------
This check used to fail on ANY misattributed commit on the branch, and that
made it unusable on exactly the branches STEP 2 tells a run to continue.

An interactive session starts `tide/mac/foo` and its commits are authored
`Jeff McClintock`, by that session's own convention. A later scheduled run
finds the open PR, and STEP 2 says resume on it -- "do not start a fresh
branch". The run adds its own commits, runs this check before pushing, and is
told **"Do not push"** over commits that were pushed before it began and that
STEP 4 separately forbids it from rewriting ("never rewrite anything already
pushed"). The run is left holding a check it can neither satisfy nor act on.
Tripped for real on 2026-08-19 (macos).

The fix is not a narrower range -- the range should stay wide, or a foreign
commit pushed by an earlier attempt would go unseen. The fix is that severity
depends on whether the run can still DO anything:

  * **Not yet pushed** -> BLOCKING, exit 1, "do not push". This is A14's case
    exactly: a concurrent session committed locally onto this branch, and
    `--amend --reset-author` is available. Unchanged.
  * **Already pushed** -> ADVISORY, exit 0, printed in full. The run cannot
    rewrite it, so failing would demand the one action the rules forbid. It is
    still shown, because "who else committed here" is worth knowing -- and on
    a continued branch the answer is usually "the developer, legitimately".

Nothing is hidden either way; only the verdict changes. `--strict` restores
the old fail-on-everything behaviour.

Usage
-----
    python scripts/check-commit-authorship.py                    # vs origin/HEAD
    python scripts/check-commit-authorship.py --range main..HEAD
    python scripts/check-commit-authorship.py --repo ../SE16     # any checkout
    python scripts/check-commit-authorship.py --expect "Jeff McClintock"
    python scripts/check-commit-authorship.py --strict           # pre-A26
    python scripts/check-commit-authorship.py --selftest

Default expectation is `tide-rack-bot`, because that is what a scheduled run
must be. An interactive session that legitimately commits as Jeff should pass
`--expect` rather than skip the check.

Exit codes: 0 all commits match, or the only mismatches are already pushed;
1 an unpushed mismatch; 2 the range could not be worked out (unknown remote,
detached HEAD, not a repository).
"""

import argparse
import subprocess
import sys

DEFAULT_EXPECT = "tide-rack-bot"

# The separator has to be something no name or address contains.
SEP = "\x1f"
# %H, not %h: these SHAs are set-matched against `git rev-list` output in
# unpushed(), which always prints full ones. Abbreviated SHAs never match,
# and the failure is silent and in the dangerous direction -- every commit
# reads as "already pushed", so nothing ever blocks. Caught by --selftest.
FORMAT = SEP.join(["%H", "%an", "%ae", "%cn", "%ce", "%s"])


def git(repo, *args):
    """Run git in repo, returning stdout. Raises on failure."""
    result = subprocess.run(
        ["git", "-C", repo] + list(args),
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        raise RuntimeError((result.stderr or result.stdout).strip())
    return result.stdout


def default_range(repo):
    """`origin/<default>..HEAD` -- everything on this branch.

    Deliberately NOT the branch's own upstream. A branch pushed once already has
    one, and comparing against it would hide a foreign commit that an earlier
    attempt had already pushed. The question here is always "who wrote
    everything on this branch".

    BACKLOG A26: that breadth is right, but it used to make the *verdict* wrong.
    See classify() -- the range stays wide and the severity is decided per
    commit instead.
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


def unpushed(repo):
    """SHAs reachable from HEAD but from no remote-tracking ref.

    The A26 distinction. A commit that is NOT here is already published, so
    STEP 4 forbids rewriting it -- telling a run "do not push" over one asks
    for the single thing it is not allowed to do.
    """
    out = git(repo, "rev-list", "HEAD", "--not", "--remotes")
    return {line.strip() for line in out.splitlines() if line.strip()}


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



def _selftest():
    """Build throwaway repos and assert the three A26 cases.

    This check gates every push the fleet makes, and A26 changed its verdict
    logic, so the cases are worth pinning rather than re-reasoning: the danger
    of getting this wrong is a check that stays green when it should not.
    """
    import os
    import tempfile

    BOT = ("tide-rack-bot", "314850083+tide-rack-bot@users.noreply.github.com")
    DEV = ("Jeff McClintock", "jef@synthedit.com")

    def run(cwd, *args, **kw):
        env = dict(os.environ)
        env.update(kw.pop("env", {}))
        r = subprocess.run(["git", "-C", cwd] + list(args), capture_output=True,
                           text=True, env=env)
        if r.returncode != 0:
            raise AssertionError("git %s failed: %s" % (args, r.stderr))
        return r.stdout

    def commit(cwd, who, msg):
        name, email = who
        open(os.path.join(cwd, "f.txt"), "a").write(msg + "\n")
        run(cwd, "add", "f.txt")
        run(cwd, "commit", "-m", msg, env={
            "GIT_AUTHOR_NAME": name, "GIT_AUTHOR_EMAIL": email,
            "GIT_COMMITTER_NAME": name, "GIT_COMMITTER_EMAIL": email})

    failures = []

    def check(label, repo, expect_rc, strict=False):
        argv = ["--repo", repo]
        if strict:
            argv.append("--strict")
        rc = main(argv)
        ok = rc == expect_rc
        print("  %-52s rc=%d expected=%d  %s"
              % (label, rc, expect_rc, "OK" if ok else "*** FAIL ***"))
        if not ok:
            failures.append(label)

    tmp = tempfile.mkdtemp(prefix="authorship-selftest-")
    remote = os.path.join(tmp, "remote.git")
    work = os.path.join(tmp, "work")
    subprocess.run(["git", "init", "-q", "--bare", "-b", "main", remote], check=True)
    subprocess.run(["git", "clone", "-q", remote, work], check=True)
    run(work, "config", "user.name", BOT[0])
    run(work, "config", "user.email", BOT[1])

    commit(work, BOT, "base")
    run(work, "push", "-q", "origin", "main")
    run(work, "remote", "set-head", "origin", "-a")

    print("A26 selftest:")

    # 1. bot-only work on a feature branch -> clean
    run(work, "checkout", "-q", "-b", "feature")
    commit(work, BOT, "bot work")
    check("bot-only, unpushed", work, 0)

    # 2. a foreign commit that is NOT pushed -> blocking (A14, unchanged)
    commit(work, DEV, "concurrent session commit")
    check("foreign + UNPUSHED -> blocks", work, 1)

    # 3. the same foreign commit, now pushed, plus new bot work -> advisory.
    #    This is A26: STEP 2 says continue the branch, STEP 4 forbids rewriting
    #    what is pushed, so this must not fail.
    run(work, "push", "-q", "-u", "origin", "feature")
    commit(work, BOT, "the run's own work")
    check("foreign + PUSHED, bot work on top -> advisory", work, 0)

    # 4. --strict still fails on it, so nothing is lost
    check("same, --strict -> blocks", work, 1, strict=True)

    # 5. an unpushed foreign commit is still caught even when a pushed one exists
    commit(work, DEV, "second concurrent commit")
    check("pushed foreign + UNPUSHED foreign -> blocks", work, 1)

    print("\n%s" % ("all cases passed" if not failures
                     else "FAILED: " + ", ".join(failures)))
    return 1 if failures else 0


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", default=".", help="repository (default: cwd)")
    parser.add_argument("--range", dest="rev_range",
                        help="commit range (default: origin/<default>..HEAD)")
    parser.add_argument("--expect", default=DEFAULT_EXPECT,
                        help="expected author (default: %s)" % DEFAULT_EXPECT)
    parser.add_argument("--strict", action="store_true",
                        help="fail on ANY misattributed commit, including ones "
                             "already pushed (pre-A26 behaviour)")
    parser.add_argument("--selftest", action="store_true",
                        help="build throwaway repos and assert the A26 cases")
    args = parser.parse_args(argv)

    if args.selftest:
        return _selftest()

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

    if not bad:
        print("all commits authored by %s" % args.expect)
        return 0

    # BACKLOG A26. Severity is decided by whether the run can still act on the
    # commit, not by who wrote it -- see describe_split() in the module docstring.
    if args.strict:
        blocking, advisory = bad, []
    else:
        try:
            live = unpushed(args.repo)
        except RuntimeError as exc:
            # Fail closed: if we cannot tell what is pushed, treat everything as
            # blocking, which is exactly the pre-A26 behaviour.
            print("warning: could not determine which commits are pushed (%s);"
                  " treating all as blocking" % exc, file=sys.stderr)
            live = {c[0] for c in bad}
        blocking = [c for c in bad if c[0] in live]
        advisory = [c for c in bad if c[0] not in live]

    if advisory:
        print("\n%d NOT authored by %s, but ALREADY PUSHED -- not blocking:"
              % (len(advisory), args.expect))
        for sha, an, ae, cn, ce, subject in advisory:
            print("  %s  %s" % (sha[:9], subject))
            print("        author:    %s <%s>" % (an, ae))
        print("\nThese are reported, not failed, because STEP 4 forbids "
              "rewriting anything already pushed -- failing here would demand "
              "the one action that is not allowed.")
        print("Check whose they are before continuing. On a branch you are "
              "CONTINUING per STEP 2, commits authored by the developer are "
              "normal: an interactive session started the branch, and that is "
              "what resuming it means. If you cannot account for them, stop and "
              "say so in the journal rather than pushing on top.")
        print("Re-run with --strict to fail on these too.")

    if blocking:
        print("\n%d NOT authored by %s and NOT yet pushed:"
              % (len(blocking), args.expect))
        for sha, an, ae, cn, ce, subject in blocking:
            print("  %s  %s" % (sha[:9], subject))
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

    print("\nno unpushed commit is misattributed -- OK to push")
    return 0


if __name__ == "__main__":
    sys.exit(main())
