#!/usr/bin/env python3
"""Check that every relative markdown link in the repo resolves to a real file.

Written for A8 (2026-08-12), whose acceptance is "JOURNAL.md under 30 KB after
rotation, no broken links". Rotation moves prose between files, so a link
checker is the only cheap proof it did not strand a reference.

A3 is the row that turns checks like this into CI. This is deliberately a
standalone script with no dependencies so A3 can adopt or replace it freely —
it does not touch .github/workflows/, which agents may not edit.

    python scripts/check-links.py            # exit 1 if anything is broken
    python scripts/check-links.py --list     # also print every link checked
    python scripts/check-links.py --selftest # check slugify() against GitHub

Only *relative* links are checked. http(s) links are counted and skipped: this
must run offline and must never be the thing that fails a run because a website
was down. --selftest is offline too — its expectations are baked in, not fetched.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# [text](target) — target may carry an #anchor. Skips image syntax's leading !
# by not caring: a broken image path is worth reporting too.
LINK = re.compile(r'\[[^\]]*\]\(([^)\s]+)(?:\s+"[^"]*")?\)')

SKIP_DIRS = {'.git', 'node_modules', 'build', '_deps'}


def md_files():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if fn.endswith('.md'):
                yield os.path.join(dirpath, fn)


def slugify(heading):
    """GitHub's anchor rule, as `github-slugger` implements it.

    "Near enough" was not near enough (A13). The rule is:

      1. lowercase;
      2. delete punctuation and control characters — every ASCII punctuation
         mark goes, except `-` and `_`, and so does non-ASCII punctuation such
         as the em-dashes this repo's headings are full of;
      3. replace each remaining space with ONE hyphen — one per *space*, not
         one per *run*.

    Step 3 is the bug this replaced. `re.sub(r'\\s+', '-', s)` collapsed a run,
    so `## 2026-08-13 — macos — S6` — where step 2 leaves two spaces behind
    each deleted em-dash — produced `2026-08-13-macos-s6` while GitHub produces
    `2026-08-13--macos--s6`. Wrong in both directions: correct links were
    reported broken, and a link written in the collapsed form passed here and
    404'd on GitHub.
    """
    s = heading.strip().lower()
    s = re.sub(r'[^\w\s-]', '', s)   # punctuation, em-dashes included
    s = re.sub(r'[^\S ]', '', s)     # tabs and friends: GitHub deletes, never hyphenates
    return s.replace(' ', '-')


def anchors_of(path):
    """Every anchor GitHub emits for this file, duplicate suffixes included.

    Two things beyond slugify(): a repeated heading gets `-1`, `-2`, … in
    document order, and fenced code is skipped — `#include "it_empty.h"` inside
    a ``` block is not a heading, and registering it as one made a link to a
    nonexistent anchor pass.
    """
    out, seen, in_fence = set(), {}, False
    try:
        with open(path, encoding='utf-8') as fh:
            for line in fh:
                if line.lstrip().startswith('```'):
                    in_fence = not in_fence
                    continue
                if in_fence or not line.startswith('#'):
                    continue
                base = slugify(line.lstrip('#'))
                n = seen.get(base, 0)
                seen[base] = n + 1
                out.add(base if n == 0 else '%s-%d' % (base, n))
    except OSError:
        pass
    return out


# Golden cases for --selftest. Every expectation here was READ OFF GitHub's own
# renderer, not derived from the code below — fetched via the contents API with
# `Accept: application/vnd.github.html`, which returns the file rendered by
# GitHub's real markdown pipeline with its anchors intact
# (`<a id="user-content-…" class="anchor">`). Do not "correct" one of these to
# match the code; re-run that fetch and correct the code.
#
# Note for anyone repeating the measurement: the /markdown API is NOT an oracle
# — it renders headings with no ids at all.
SLUG_CASES = [
    # em-dash between spaces: step 2 deletes the dash and leaves BOTH spaces,
    # and GitHub then emits one hyphen for each. This is the A13 bug.
    ('2026-08-13 — macos — S6 (part 2 of 2)', '2026-08-13--macos--s6-part-2-of-2'),
    ('Rotation — do this as part of STEP 4, every run',
     'rotation--do-this-as-part-of-step-4-every-run'),
    # '&' is deleted like any other punctuation, again leaving two spaces
    ('Release & distribution — blocked on V1 (nothing to ship yet)',
     'release--distribution--blocked-on-v1-nothing-to-ship-yet'),
    # punctuation glued to a word leaves no space, so a single hyphen is right
    ('What "done" looks like for v0.1', 'what-done-looks-like-for-v01'),
    # '_' and '-' survive; '/', '.' and backticks do not
    ('C8 — `SynthEditLib/it_empty.h`: why it exists',
     'c8--syntheditlibit_emptyh-why-it-exists'),
    ('STEP 0.7 — Become the agent', 'step-07--become-the-agent'),
]


def selftest():
    """Prove slugify() still agrees with GitHub, and that duplicates suffix."""
    bad = 0
    for heading, expect in SLUG_CASES:
        got = slugify(heading)
        flag = 'ok  ' if got == expect else 'FAIL'
        if got != expect:
            bad += 1
        print('  %s %-52s -> %s' % (flag, heading[:52], got))
        if got != expect:
            print('       expected %r' % expect)

    # Repeated headings, and headings inside fenced code, both via anchors_of().
    import tempfile
    doc = ('# Dup\n\n```\n# not a heading\n```\n\n# Dup\n\n# Dup\n')
    fd, tmp = tempfile.mkstemp(suffix='.md')
    with os.fdopen(fd, 'w', encoding='utf-8') as fh:
        fh.write(doc)
    try:
        got = anchors_of(tmp)
    finally:
        os.unlink(tmp)
    expect = {'dup', 'dup-1', 'dup-2'}
    if got != expect:
        bad += 1
        print('  FAIL duplicate/fence handling: %r != %r' % (sorted(got), sorted(expect)))
    else:
        print('  ok   duplicates suffix -1/-2, fenced "# not a heading" ignored')

    print('selftest: %d failed' % bad)
    return 1 if bad else 0


def main():
    if '--selftest' in sys.argv:
        return selftest()
    show = '--list' in sys.argv
    broken, checked, external = [], 0, 0

    for path in sorted(md_files()):
        rel = os.path.relpath(path, ROOT).replace('\\', '/')
        with open(path, encoding='utf-8') as fh:
            text = fh.read()
        in_fence = False
        for lineno, line in enumerate(text.split('\n'), 1):
            if line.lstrip().startswith('```'):
                in_fence = not in_fence
                continue
            # Code blocks hold things that look like links and are not — a
            # linker error such as libVST3_Wrapper.a[x86_64][17](Foo.mm.o) is
            # the case that prompted this.
            if in_fence or line.startswith('    ') or line.startswith('\t'):
                continue
            for target in LINK.findall(line):
                if target.startswith(('http://', 'https://', 'mailto:')):
                    external += 1
                    continue
                if target == '#':
                    # The repo's placeholder for "a real file:line, but no URL
                    # worth pointing at" — e.g. [DrawingFrameMac.mm:434](#).
                    # Deliberate, and not a broken link.
                    continue
                if target.startswith('#'):
                    # same-file anchor
                    checked += 1
                    if slugify(target[1:]) not in anchors_of(path):
                        broken.append((rel, lineno, target, 'no such heading'))
                    continue
                filepart, _, anchor = target.partition('#')
                if not filepart:
                    continue
                checked += 1
                dest = os.path.normpath(os.path.join(os.path.dirname(path), filepart))
                if not os.path.exists(dest):
                    broken.append((rel, lineno, target, 'no such file'))
                elif anchor and dest.endswith('.md') and slugify(anchor) not in anchors_of(dest):
                    broken.append((rel, lineno, target, 'no such heading in ' + filepart))
                elif show:
                    print('  ok  %s:%d  %s' % (rel, lineno, target))

    print('%d relative links checked, %d external skipped' % (checked, external))
    if broken:
        print('\n%d BROKEN:' % len(broken))
        for rel, lineno, target, why in broken:
            print('  %s:%d  %s  (%s)' % (rel, lineno, target, why))
        return 1
    print('no broken links')
    return 0


if __name__ == '__main__':
    sys.exit(main())
