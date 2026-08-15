#!/usr/bin/env python3
"""Count the public repo's #includes that only resolve inside the private repo.

The carve-out (BACKLOG C1-C12) moves editor sources out of the private
`SynthEdit` repo into the public `SynthEditLib`. C7's acceptance test is a clean
clone of the public repo with **no access** to the private one, so every quoted
`#include` in a public file that can only be satisfied from the private repo is
a C7 blocker. This counts them.

Every stage from C4 onward has measured this by hand and thrown the script away
afterwards -- C4, C5 and the C12 scoping run each re-implemented it, and the C12
doc says outright that "the next stage should re-create it". This is that script,
committed once so they stop.

Why a naive `grep -c` is wrong, and by a factor of nearly 3
----------------------------------------------------------
MSVC and GCC both resolve a quoted include against **the including file's own
directory first**, before any -I path. `resource.h` exists in *both* repos, so
the ~71 public files that include it resolve to their own copy and are not
dangling at all. A scan that checks the include path first reports 114 edges
where the truth is 43, and turns C12a from a four-line delete into the largest
item in the stage. The C12 scoping run hit exactly this.

So: own-directory first, then the public include roots, and only then is an
include considered dangling -- and only if the private repo can actually satisfy
it, otherwise it is a missing header, which is a different bug.

Usage
-----
    python scripts/dangling_private_includes.py                # summary
    python scripts/dangling_private_includes.py --by-header    # group by target
    python scripts/dangling_private_includes.py --by-includer  # group by source
    python scripts/dangling_private_includes.py --header Control.h   # one target

Run it before and after a stage and diff the totals; that number is the
verification artifact the run prompt asks every code item to name.
"""

import argparse
import os
import re
import sys

# --- Layout ------------------------------------------------------------------
# Defaults match the dev boxes (see docs/building.md). Override with the flags
# if your clones live elsewhere; nothing here assumes a drive letter beyond the
# default values themselves.

DEFAULT_PUBLIC = "C:/SE/SynthEditLib"
DEFAULT_PRIVATE = "C:/SE/SE16"

# Directories inside the private repo that the public code can currently reach,
# because EditorLib's include path still carries them. These are what the
# carve-out is emptying.
PRIVATE_INCLUDE_DIRS = ["SynthEdit2", "EditorLib"]

# Trees that are not source: build output, vendored SDKs, and the stray agent
# worktrees under SynthEdit2/.claude. Every one of these has bitten a previous
# measurement -- build/_deps holds a whole second copy of the public repo, and
# .claude/worktrees holds copies of the private one.
SKIP_DIR_PARTS = {
    ".git", ".claude", "build", "build-profile", "out", "cmake-build-debug",
    "_deps", "node_modules", "SDKs", "third_party",
}

SOURCE_SUFFIXES = (".cpp", ".h", ".hpp", ".c", ".cc", ".mm", ".m", ".inl")

# `#include "foo/bar.h"` -- quoted form only. Angle-bracket includes are system
# or SDK headers and are never carve-out edges.
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.MULTILINE)


def walk_sources(root):
    """Yield every source file under root, skipping non-source trees."""
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_PARTS]
        for name in filenames:
            if name.endswith(SOURCE_SUFFIXES):
                yield os.path.join(dirpath, name)


def index_by_relpath(root):
    """Map every source file to its path relative to root, forward-slashed.

    Keyed both by the full relative path and by bare basename, because quoted
    includes appear in both forms ("modules/foo/bar.h" and "bar.h").
    """
    index = set()
    for path in walk_sources(root):
        rel = os.path.relpath(path, root).replace(os.sep, "/")
        index.add(rel)
        index.add(os.path.basename(rel))
    return index


def public_include_roots(public_root):
    """Directories that are on the include path for public consumers.

    The public repo's root is an include dir in all three build systems -- that
    is precisely why C2 and C3 could move files to the root for zero include-path
    edits -- and the hosting modules dir is added by the CMake targets.
    """
    roots = [public_root]
    modules = os.path.join(public_root, "modules")
    if os.path.isdir(modules):
        roots.append(modules)
        hosting = os.path.join(modules, "se_sdk3_hosting")
        if os.path.isdir(hosting):
            roots.append(hosting)
    return roots


def resolve(include, includer_dir, roots):
    """True if `include` resolves without leaving the given roots.

    Own directory first, then the roots, mirroring the compilers.
    """
    candidate = os.path.normpath(os.path.join(includer_dir, include))
    if os.path.isfile(candidate):
        return True
    for root in roots:
        if os.path.isfile(os.path.normpath(os.path.join(root, include))):
            return True
    return False


def find_private(include, private_root):
    """Return the private path satisfying `include`, or None."""
    for sub in PRIVATE_INCLUDE_DIRS:
        candidate = os.path.normpath(os.path.join(private_root, sub, include))
        if os.path.isfile(candidate):
            return os.path.relpath(candidate, private_root).replace(os.sep, "/")
    return None


def scan(public_root, private_root):
    """Return the list of dangling edges.

    An edge is (public file, included header, private file that satisfies it).
    """
    roots = public_include_roots(public_root)
    edges = []
    for path in walk_sources(public_root):
        try:
            text = open(path, encoding="utf-8", errors="replace").read()
        except OSError as exc:
            print(f"warning: cannot read {path}: {exc}", file=sys.stderr)
            continue
        includer_dir = os.path.dirname(path)
        rel_includer = os.path.relpath(path, public_root).replace(os.sep, "/")
        for include in INCLUDE_RE.findall(text):
            include = include.replace("\\", "/")
            if resolve(include, includer_dir, roots):
                continue  # satisfied publicly -- the common case, and the
                          # own-directory rule is what makes resource.h zero
            private = find_private(include, private_root)
            if private is not None:
                edges.append((rel_includer, include, private))
    return edges


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--public", default=DEFAULT_PUBLIC,
                    help=f"public repo root (default {DEFAULT_PUBLIC})")
    ap.add_argument("--private", default=DEFAULT_PRIVATE,
                    help=f"private repo root (default {DEFAULT_PRIVATE})")
    ap.add_argument("--by-header", action="store_true",
                    help="group the edges by the header they point at")
    ap.add_argument("--by-includer", action="store_true",
                    help="group the edges by the public file that has them")
    ap.add_argument("--header", metavar="NAME", action="append",
                    help="report only edges pointing at NAME; repeatable. "
                         "Use this to check one stage's own Accept line.")
    args = ap.parse_args()

    for label, root in (("public", args.public), ("private", args.private)):
        if not os.path.isdir(root):
            print(f"error: {label} repo not found: {root}", file=sys.stderr)
            return 2

    edges = scan(args.public, args.private)

    if args.header:
        wanted = {h.lower() for h in args.header}
        edges = [e for e in edges
                 if os.path.basename(e[1]).lower() in wanted]
        print(f"filtered to headers: {', '.join(sorted(wanted))}")

    if args.by_header:
        by = {}
        for includer, include, private in edges:
            by.setdefault(os.path.basename(include), []).append(includer)
        for header in sorted(by, key=lambda h: (-len(by[h]), h)):
            print(f"{len(by[header]):4}  {header}")
            for includer in sorted(set(by[header])):
                print(f"        {includer}")
    elif args.by_includer:
        by = {}
        for includer, include, private in edges:
            by.setdefault(includer, []).append(include)
        for includer in sorted(by, key=lambda f: (-len(by[f]), f)):
            print(f"{len(by[includer]):4}  {includer}")
            for include in sorted(by[includer]):
                print(f"        {include}")
    else:
        for includer, include, private in sorted(edges):
            print(f"{includer} -> {include}  (private: {private})")

    distinct = len({os.path.basename(e[1]) for e in edges})
    print(f"\n{len(edges)} dangling private include(s), "
          f"{distinct} distinct header(s), "
          f"from {len({e[0] for e in edges})} public file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
