#!/usr/bin/env python3
"""Check that new scheduled-run JOURNAL.md entries carry the Prompt: line.

Written for A3 (2026-08-13). STEP 4 of the run prompt requires every
scheduled run's entry to carry one line: "**Prompt:** <sha> · <model> ·
app <version> · as <login>" — the only way to tell, from outside the box,
which instructions actually executed (see the "the prompt is fetched, not
copied" journal entry, 2026-08-09). Interactive sessions with Jeff are
explicitly exempt — the run prompt says their conventions "never apply to
scheduled runs", and symmetrically this provenance line is a scheduled-run
convention that never applied to them either.

Checked ONLY against entries new in this diff (present in head, absent from
base), not retroactively: the convention did not exist before 2026-08-09, so
a blanket check against the whole file's history would fail ~20 real, still-
correct entries that simply predate it.

An entry heading is treated as interactive (exempt) if it contains the word
"interactive" or "manual", case-insensitively, anywhere in the heading line —
the actual, inconsistent-but-always-present convention in every real
interactive entry so far (see JOURNAL.md / JOURNAL-2026-08.md for the range
of phrasings this has to match: "(interactive session, Jeff directing)",
"(manual, not a scheduled run)", "(state update, interactive)", etc.).

    python scripts/check-prompt-provenance.py <base-journal> <head-journal>
"""
import argparse
import re
import sys

ENTRY = re.compile(r'^## (\d{4}-\d{2}-\d{2}) — (.+)$', re.MULTILINE)
EXEMPT = re.compile(r'interactive|manual', re.IGNORECASE)


def split_entries(text):
    starts = [(m.start(), m.group(2)) for m in ENTRY.finditer(text)]
    if not starts:
        return []
    bounds = [s for s, _ in starts] + [len(text)]
    return [(heading, text[bounds[i]:bounds[i + 1]]) for i, (_, heading) in enumerate(starts)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('base')
    ap.add_argument('head')
    args = ap.parse_args()

    with open(args.base, encoding='utf-8') as f:
        base_bodies = set(b.rstrip('\n') for _, b in split_entries(f.read()))
    with open(args.head, encoding='utf-8') as f:
        head_entries = split_entries(f.read())

    new_entries = [(h, b) for h, b in head_entries if b.rstrip('\n') not in base_bodies]

    if not new_entries:
        print('no new JOURNAL.md entries in this diff — nothing to check')
        return 0

    missing = []
    for heading, body in new_entries:
        exempt = EXEMPT.search(heading)
        has_prompt = '**Prompt:**' in body
        tag = 'interactive (exempt)' if exempt else 'scheduled'
        print('%-22s %s' % (tag, heading))
        if not exempt and not has_prompt:
            missing.append(heading)

    if missing:
        print('\n%d new scheduled-run entr%s missing the **Prompt:** provenance line:' %
              (len(missing), 'y' if len(missing) == 1 else 'ies'))
        for heading in missing:
            print('  ' + heading)
        print('Template: **Prompt:** <sha> · <model> · app <version> · as <login>')
        return 1

    print('\nall new scheduled-run entries carry **Prompt:**, OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
