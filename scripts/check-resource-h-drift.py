#!/usr/bin/env python3
"""Fail when the two resource.h copies disagree about any resource constant.

BACKLOG P9. `SynthEditLib/resource.h` and `SE16/SynthEdit2/resource.h` are two
independently-maintained copies of the same generated header, 361 lines each.
They are **not** a carve-out artifact: the public one goes back to
`SynthEditLib`'s initial commit and the private one arrived with SynthEdit's own
"move source out of the old SynthEdit folder" commit, so they have been
duplicates for a long time and nothing has ever compared them.

Why this matters more than tidiness
-----------------------------------
**C12a delisted `${EDITOR_DIR}/resource.h` from EditorLib's source list on the
strength of the two copies being identical**, and that is the whole safety
argument for the delist: quoted includes resolve against the including file's
own directory first, so public files get the public copy and private files get
the private one, and it is a no-op *only while the constants agree*. Nothing
enforced that. This does.

The counter is already drifting, which is the measurable warning
--------------------------------------------------------------
The two files differ today in exactly two places: a trailing space in a comment,
and `_APS_NEXT_RESOURCE_VALUE` — **210** private against **207** public. That is
a Visual Studio auto-counter inside `#ifdef APSTUDIO_INVOKED`, so it changes no
compiled constant, but it shows the private copy has had three resource slots
allocated that the public one has not. The next allocation is as likely to
collide as not.

So the counter is deliberately **not** compared: it is metadata, it moves for
legitimate reasons, and failing on it would make the check cry wolf until
someone turned it off. Everything under `APSTUDIO_INVOKED` is ignored for the
same reason. What is compared is every `ID*` constant — the things that end up
in compiled code and in `.rc` files.

Usage
-----
    python scripts/check-resource-h-drift.py
    python scripts/check-resource-h-drift.py --public <path> --private <path>
    python scripts/check-resource-h-drift.py --selftest

Note it cannot run in TideSynth's CI: one of the two files lives in the private
repo. It is a tool for a dev box or an agent run, like
`dangling_private_includes.py`. Run it before touching either copy, and as part
of any carve-out stage that moves a `.cpp` out of `SynthEdit2` — such a TU
switches from the private copy to the public one, which is a no-op only if this
check passes.
"""

import argparse
import os
import re
import sys

DEFAULT_PUBLIC = "C:/SE/SynthEditLib/resource.h"
DEFAULT_PRIVATE = "C:/SE/SE16/SynthEdit2/resource.h"

RE_DEFINE = re.compile(r"^\s*#\s*define\s+(ID\w+)\s+(\S+)")
RE_APS_OPEN = re.compile(r"^\s*#\s*ifdef\s+APSTUDIO_INVOKED")
RE_IF = re.compile(r"^\s*#\s*if")
RE_ENDIF = re.compile(r"^\s*#\s*endif")


def constants(path):
    """Map every ID* constant to its value, skipping the APSTUDIO block.

    The APSTUDIO section holds Visual Studio's own bookkeeping -- the
    _APS_NEXT_* counters -- which move for legitimate reasons and compile to
    nothing. Comparing them would make this check fire on every resource edit.
    """
    found = {}
    depth = 0          # nesting depth inside the APSTUDIO block, 0 = outside
    with open(path, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if depth:
                if RE_IF.match(line):
                    depth += 1
                elif RE_ENDIF.match(line):
                    depth -= 1
                continue
            if RE_APS_OPEN.match(line):
                depth = 1
                continue
            match = RE_DEFINE.match(line)
            if match:
                found[match.group(1)] = match.group(2)
    return found


def compare(public, private):
    """Return (only_public, only_private, conflicting) as sorted lists."""
    only_public = sorted(set(public) - set(private))
    only_private = sorted(set(private) - set(public))
    conflicting = sorted(name for name in set(public) & set(private)
                         if public[name] != private[name])
    return only_public, only_private, conflicting


def run(public_path, private_path):
    for label, path in (("public", public_path), ("private", private_path)):
        if not os.path.isfile(path):
            print("error: %s copy not found: %s" % (label, path), file=sys.stderr)
            return 2

    public = constants(public_path)
    private = constants(private_path)
    only_public, only_private, conflicting = compare(public, private)

    print("%d public constant(s), %d private constant(s)" % (len(public), len(private)))

    if not (only_public or only_private or conflicting):
        print("no drift -- every ID* constant agrees")
        return 0

    if conflicting:
        print("\n%d CONFLICTING -- same name, different value:" % len(conflicting))
        for name in conflicting:
            print("  %-40s public=%s  private=%s"
                  % (name, public[name], private[name]))
    if only_public:
        print("\n%d only in the public copy:" % len(only_public))
        for name in only_public:
            print("  %-40s = %s" % (name, public[name]))
    if only_private:
        print("\n%d only in the private copy:" % len(only_private))
        for name in only_private:
            print("  %-40s = %s" % (name, private[name]))

    print("\nThese two files must agree: C12a delisted the private copy from "
          "EditorLib's source list precisely because they were identical, and "
          "public and private TUs each resolve to their own by the "
          "own-directory-first rule. A divergence means the same name compiles "
          "to different values on the two sides. See BACKLOG P9.")
    return 1


# --- Self-test ---------------------------------------------------------------

SELFTEST_BASE = """//{{NO_DEPENDENCIES}}
#define IDD_ABOUTBOX                    100
#define IDR_MAINFRAME                   128
#define ID_AUD_HELP                     3

#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        207
#define _APS_NEXT_COMMAND_VALUE         40000
#endif
#endif
"""


def selftest():
    import tempfile

    def constants_of(text):
        handle = tempfile.NamedTemporaryFile("w", suffix=".h", delete=False,
                                             encoding="utf-8")
        handle.write(text)
        handle.close()
        try:
            return constants(handle.name)
        finally:
            os.unlink(handle.name)

    failures = 0

    def check(description, text_a, text_b, expect):
        nonlocal failures
        got = compare(constants_of(text_a), constants_of(text_b))
        got = tuple(list(x) for x in got)
        if got != expect:
            failures += 1
            print("FAIL  %s\n      expected: %s\n      got:      %s"
                  % (description, expect, got))

    check("identical files", SELFTEST_BASE, SELFTEST_BASE, ([], [], []))

    # The real state of these two files today: the counter differs by 3, and
    # that must NOT be reported.
    drifted_counter = SELFTEST_BASE.replace("207", "210")
    check("APSTUDIO counter drift is ignored -- the real case today",
          SELFTEST_BASE, drifted_counter, ([], [], []))

    check("comment-only difference is ignored",
          SELFTEST_BASE, "// a different comment\n" + SELFTEST_BASE, ([], [], []))

    conflicting = SELFTEST_BASE.replace("IDD_ABOUTBOX                    100",
                                        "IDD_ABOUTBOX                    101")
    check("same name, different value -- the case that breaks compiled code",
          SELFTEST_BASE, conflicting, ([], [], ["IDD_ABOUTBOX"]))

    added = SELFTEST_BASE + "#define IDD_BRAND_NEW                   300\n"
    check("constant only in the private copy",
          SELFTEST_BASE, added, ([], ["IDD_BRAND_NEW"], []))
    check("constant only in the public copy",
          added, SELFTEST_BASE, (["IDD_BRAND_NEW"], [], []))

    # A constant added inside the APSTUDIO block is bookkeeping, not a resource.
    aps_added = SELFTEST_BASE.replace(
        "#define _APS_NEXT_COMMAND_VALUE         40000",
        "#define _APS_NEXT_COMMAND_VALUE         40000\n"
        "#define IDS_INSIDE_APSTUDIO             999")
    check("an ID* inside the APSTUDIO block is still ignored",
          SELFTEST_BASE, aps_added, ([], [], []))

    print("\n7 case(s), %d failed" % failures)
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--public", default=DEFAULT_PUBLIC)
    parser.add_argument("--private", default=DEFAULT_PRIVATE)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    return run(args.public, args.private)


if __name__ == "__main__":
    sys.exit(main())
