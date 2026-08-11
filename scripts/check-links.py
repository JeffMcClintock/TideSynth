#!/usr/bin/env python3
"""Check that every relative markdown link in the repo resolves to a real file.

Written for A8 (2026-08-12), whose acceptance is "JOURNAL.md under 30 KB after
rotation, no broken links". Rotation moves prose between files, so a link
checker is the only cheap proof it did not strand a reference.

A3 is the row that turns checks like this into CI. This is deliberately a
standalone script with no dependencies so A3 can adopt or replace it freely —
it does not touch .github/workflows/, which agents may not edit.

    python scripts/check-links.py          # exit 1 if anything is broken
    python scripts/check-links.py --list   # also print every link checked

Only *relative* links are checked. http(s) links are counted and skipped: this
must run offline and must never be the thing that fails a run because a website
was down.
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
    """GitHub's anchor rule, near enough: lowercase, drop punctuation, spaces to dashes."""
    s = heading.strip().lower()
    s = re.sub(r'[^\w\s-]', '', s)
    return re.sub(r'\s+', '-', s)


def anchors_of(path):
    out = set()
    try:
        with open(path, encoding='utf-8') as fh:
            for line in fh:
                if line.startswith('#'):
                    out.add(slugify(line.lstrip('#')))
    except OSError:
        pass
    return out


def main():
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
