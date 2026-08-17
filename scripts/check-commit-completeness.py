#!/usr/bin/env python3
"""Fail if a commit contains fewer files than were staged for it.

BACKLOG A16, and the sibling of `check-commit-authorship.py` (A14). The two
catch different halves of the same hazard -- a concurrent git operation in a
shared working copy -- and neither catches the other's:

  A14 asserts WHO authored a commit.  A16 asserts WHAT is in it.

On 2026-08-15 a run staged five paths with one `git add`, ran `git commit`
immediately after, and got a commit containing **one** of the five. The working
tree held all five correctly throughout, nothing was lost, the commit exited 0,
and it was correctly authored as `tide-rack-bot` -- so A14 passed cleanly and
would pass again. It was caught by eyeballing `git show --stat`, by habit.

REPRODUCED 2026-08-17 (macos), which the original filing could not do. The
signature is matched exactly by a concurrent **partial** index reset landing
between the add and the commit:

    git add f1 f2 f3 f4 f5          # 5 staged
    git reset -q HEAD -- f2 f3 f4 f5    # <-- the other session, any git op
                                        #     that unstages a subset
    git commit -m ...               # exits 0, commits f1 alone

That is silent. Two neighbouring shapes are NOT silent and are not this bug:
a *full* `git reset` makes the commit fail loudly with "nothing added to
commit", and a peer running `git commit` first is the A14 shape (wrong author,
right content). Only the partial unstage both succeeds and lies.

Because the staged list is destroyed by the very race being detected, this is a
two-phase check -- there is no way to recover "what was staged" after the fact:

    python scripts/check-commit-completeness.py --record   # BEFORE git commit
    git commit -m "..."
    python scripts/check-commit-completeness.py --verify   # AFTER  git commit

`--record` writes the staged path list to `.git/tide-staged-manifest`, which is
inside `.git/` and so is never itself committable. `--verify` compares it to the
files actually in HEAD and fails on anything missing.

Usage
-----
    python scripts/check-commit-completeness.py --record
    python scripts/check-commit-completeness.py --verify
    python scripts/check-commit-completeness.py --verify --repo ../SE16
    python scripts/check-commit-completeness.py --selftest

`--verify` with no recorded manifest is a *skip*, not a failure (exit 0 with a
note), so adding this to a workflow cannot break a commit made before the guard
existed. Everything else that goes wrong is a failure.

Exit codes: 0 pass or skip, 1 files went missing, 2 could not run the check
(not a repository, no commits, unreadable manifest).
"""

import argparse
import os
import subprocess
import sys

MANIFEST = "tide-staged-manifest"


def git(repo, *args):
    """Run git in repo, returning stdout. Raises on failure."""
    result = subprocess.run(
        ["git", "-C", repo] + list(args),
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        raise RuntimeError((result.stderr or result.stdout).strip())
    return result.stdout


def git_dir(repo):
    """Absolute path to the .git directory (handles worktrees and submodules)."""
    return git(repo, "rev-parse", "--absolute-git-dir").strip()


def manifest_path(repo):
    return os.path.join(git_dir(repo), MANIFEST)


def staged_files(repo):
    """Paths currently staged, relative to the repo root."""
    out = git(repo, "diff", "--cached", "--name-only")
    return [line for line in out.splitlines() if line.strip()]


def head_files(repo):
    """Paths touched by HEAD.

    A merge commit legitimately reports nothing under --name-only against its
    first parent, so it is reported as such and left to the caller.
    """
    out = git(repo, "show", "--name-only", "--format=", "HEAD")
    return [line for line in out.splitlines() if line.strip()]


def is_merge(repo):
    return len(git(repo, "rev-list", "--parents", "-n", "1", "HEAD").split()) > 2


def do_record(repo):
    files = staged_files(repo)
    path = manifest_path(repo)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(files))
        if files:
            fh.write("\n")
    if not files:
        print("nothing staged -- recorded an empty manifest")
        return 0
    print("recorded %d staged path(s) for the next commit" % len(files))
    for f in files:
        print("   %s" % f)
    return 0


def do_verify(repo):
    path = manifest_path(repo)
    if not os.path.exists(path):
        print("no staged manifest recorded -- skipping completeness check")
        print("(run with --record immediately before `git commit` to enable it)")
        return 0

    with open(path, encoding="utf-8") as fh:
        recorded = [line for line in fh.read().splitlines() if line.strip()]

    # Consume it either way: a stale manifest silently checked against an
    # unrelated later commit would be worse than no check at all.
    os.remove(path)

    if not recorded:
        print("manifest was empty -- nothing to verify")
        return 0

    if is_merge(repo):
        print("HEAD is a merge commit -- skipping (--name-only is not "
              "meaningful against a merge's first parent)")
        return 0

    landed = set(head_files(repo))
    missing = [f for f in recorded if f not in landed]

    print("%d path(s) staged, %d in HEAD" % (len(recorded), len(landed)))

    if missing:
        print("\n%d STAGED PATH(S) MISSING FROM THE COMMIT:" % len(missing))
        for f in missing:
            print("   %s" % f)
        print("\nDo not push. The commit is short -- see BACKLOG A16. The most "
              "likely cause is a concurrent git operation in this shared "
              "checkout unstaging a subset between your `git add` and your "
              "`git commit`.")
        print("The content is almost certainly still in your working tree. "
              "Check, then re-stage and amend:")
        print("  git status")
        print("  git add <the missing paths>")
        print("  git commit --amend --no-edit")
        print("Never amend a commit that is already pushed, or one a "
              "concurrent session may be building on.")
        return 1

    print("all staged paths are present in the commit")
    return 0


def selftest():
    """Prove the check catches the real race, and does not cry wolf.

    Builds throwaway repositories in a temp dir and simulates the concurrent
    partial unstage that reproduces A16.
    """
    import tempfile
    import shutil

    failures = []

    def scenario(name, sabotage, expect_rc):
        root = tempfile.mkdtemp()
        try:
            git(root, "init", "-q", ".")
            git(root, "config", "user.email", "selftest@example.invalid")
            git(root, "config", "user.name", "selftest")
            with open(os.path.join(root, "base.txt"), "w") as fh:
                fh.write("base\n")
            git(root, "add", "base.txt")
            git(root, "commit", "-q", "-m", "base")

            names = ["f%d.txt" % i for i in range(1, 6)]
            for n in names:
                with open(os.path.join(root, n), "w") as fh:
                    fh.write(n + "\n")
            git(root, "add", *names)

            do_record(root)
            sabotage(root, names)
            try:
                git(root, "commit", "-q", "-m", "mine")
            except RuntimeError:
                pass  # a loud failure is a different bug; verify still runs
            rc = do_verify(root)

            if rc != expect_rc:
                failures.append("%s: expected rc=%d, got %d" % (name, expect_rc, rc))
                print("   FAIL %s" % name)
            else:
                print("   ok   %s (rc=%d)" % (name, rc))
        finally:
            shutil.rmtree(root, ignore_errors=True)

    print("--- selftest: clean commit should PASS")
    scenario("clean", lambda r, n: None, 0)

    print("\n--- selftest: concurrent PARTIAL unstage should FAIL (the A16 race)")
    scenario("partial-unstage",
             lambda r, n: git(r, "reset", "-q", "HEAD", "--", *n[1:]), 1)

    print("\n--- selftest: concurrent FULL unstage should FAIL")
    scenario("full-unstage", lambda r, n: git(r, "reset", "-q"), 1)

    if failures:
        print("\nSELFTEST FAILED:")
        for f in failures:
            print("  %s" % f)
        return 1
    print("\nselftest passed")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Assert a commit contains everything that was staged for it.")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--record", action="store_true",
                      help="capture the staged path list; run BEFORE git commit")
    mode.add_argument("--verify", action="store_true",
                      help="compare HEAD against the captured list; run AFTER")
    mode.add_argument("--selftest", action="store_true",
                      help="prove the check works, in throwaway repositories")
    parser.add_argument("--repo", default=".", help="repository to check")
    args = parser.parse_args()

    if args.selftest:
        return selftest()

    try:
        git(args.repo, "rev-parse", "--git-dir")
    except RuntimeError as exc:
        print("cannot run: %s" % exc)
        return 2

    try:
        if args.record:
            return do_record(args.repo)
        return do_verify(args.repo)
    except RuntimeError as exc:
        print("cannot run: %s" % exc)
        return 2


if __name__ == "__main__":
    sys.exit(main())
