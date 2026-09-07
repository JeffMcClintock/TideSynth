#!/usr/bin/env python3
"""A35 -- measure which BACKLOG `Plat` edits scripts/check-backlog-diff.py allows.

BACKLOG A35 asserts, from a code reading, that "a BACKLOG row's `Plat` column is
frozen at filing time and no legal edit can correct it", and that all four of the
check's legitimate edits pin `Plat`.  This probe turns that reading into a
measurement: it builds synthetic base/head BACKLOG pairs that differ in exactly
one way each, runs the REAL check as a subprocess, and prints a truth table of
what it accepts.

Why a probe rather than a reading.  The fleet's own repeated lesson is that a
filed row is one run's reading (2026-09-01 linux, E74: "a filed row is one run's
reading, and STEP 1's re-verify deserves to apply to BACKLOG rows too").  A35 is
asking Jeff for a process ruling; a ruling deserves a truth table under it rather
than an assertion, and this costs one file and no build.

It also answers a question A35 did NOT ask, and the answer changes the shape of
the request: what a Plat-changing RENUMBER does.  A35 says there is "no route,
not even filing a fresh id".  That is right, and the failure mode is worse than
a refusal -- see CASE_RENUMBER_NARROW below.

Run:  python3 tests/a35_plat_edit_probe.py [--verbose]
Exit: 0 if every case behaves as recorded here, 1 if the check's behaviour has
      moved (which would mean A35's premise has changed and the row wants
      re-reading before anyone acts on it).
"""
import argparse
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
CHECK = os.path.join(REPO, 'scripts', 'check-backlog-diff.py')

HEADER = '| ID | Status | Plat | Item |\n|---|---|---|---|\n'

# One row, used as the base version everywhere.  Item text is deliberately
# ordinary: every branch of the check matches on Item containment, so the
# wording must not be what makes a case pass or fail.
BASE_ITEM = 'A hosted CLAP with no editor never receives its document.'
OTHER = '| Z1 | TODO | win | An unrelated row that never changes. |\n'


def backlog(rows):
    return '# Backlog\n\n' + HEADER + ''.join(rows) + '\n'


def row(rid, status, plat, item):
    return '| %s | %s | %s | %s |\n' % (rid, status, plat, item)


def run_case(base_text, head_text, extra_files=None):
    """Run the real check on one base/head pair.  Returns (rc, stdout)."""
    with tempfile.TemporaryDirectory() as d:
        base = os.path.join(d, 'base.md')
        head = os.path.join(d, 'BACKLOG.md')
        with open(base, 'w', encoding='utf-8') as f:
            f.write(base_text)
        with open(head, 'w', encoding='utf-8') as f:
            f.write(head_text)
        changed = []
        for name, text in (extra_files or {}).items():
            p = os.path.join(d, name)
            with open(p, 'w', encoding='utf-8') as f:
                f.write(text)
            changed.append(p)
        cmd = [sys.executable, CHECK, base, head, '--repo-root', d]
        for p in changed:
            cmd += ['--changed-file', p]
        # --changed-file is what CI passes; with none, the check walks the tree.
        # Pass the head itself too, mirroring lint.yml, so the no-archive cases
        # get an explicit empty search set rather than a tree walk.
        if not changed:
            cmd += ['--changed-file', head]
        r = subprocess.run(cmd, capture_output=True, text=True)
        return r.returncode, r.stdout + r.stderr


# ---------------------------------------------------------------------------
# The cases.  Each is (label, base, head, extra_files, expected_rc, expected_substr)
# ---------------------------------------------------------------------------
def cases():
    base_one = backlog([row('E79', 'TODO', 'any', BASE_ITEM), OTHER])

    # 1. The control: an ordinary status flip, Plat untouched.  Must PASS, or
    #    every FAIL below is about the harness rather than about Plat.
    yield ('status flip, Plat unchanged',
           base_one,
           backlog([row('E79', 'IN-REVIEW', 'any', BASE_ITEM), OTHER]),
           None, 0, 'status change')

    # 2. The edit A35 wants: narrowing, alongside a legal status flip.
    yield ('status flip + Plat NARROW (any -> linux)',
           base_one,
           backlog([row('E79', 'IN-REVIEW', 'linux', BASE_ITEM), OTHER]),
           None, 1, 'E79: Plat column differs')

    # 3/4. The directions A35 proposes keeping closed.
    base_linux = backlog([row('E79', 'TODO', 'linux', BASE_ITEM), OTHER])
    yield ('status flip + Plat WIDEN (linux -> any)',
           base_linux,
           backlog([row('E79', 'IN-REVIEW', 'any', BASE_ITEM), OTHER]),
           None, 1, 'E79: Plat column differs')
    yield ('status flip + Plat SWAP (linux -> mac)',
           base_linux,
           backlog([row('E79', 'IN-REVIEW', 'mac', BASE_ITEM), OTHER]),
           None, 1, 'E79: Plat column differs')

    # 5. Narrowing with nothing else changed -- the minimal correcting edit.
    yield ('Plat NARROW alone, no status change',
           base_one,
           backlog([row('E79', 'TODO', 'linux', BASE_ITEM), OTHER]),
           None, 1, 'E79: Plat column differs')

    # 6. A new row is unconstrained -- A35 concedes this and it is the control
    #    for the workaround case below.
    yield ('new row at any Plat',
           base_one,
           backlog([row('E79', 'TODO', 'any', BASE_ITEM),
                    row('E84', 'TODO', 'linux', 'A brand new finding.'), OTHER]),
           None, 0, '1 new row(s): E84')

    # 7/8. Archive move, with and without a Plat change in the archived copy.
    done_same = backlog([row('E79', 'DONE 2026-09-01', 'any', BASE_ITEM + ' Landed.')])
    done_narrowed = backlog([row('E79', 'DONE 2026-09-01', 'linux', BASE_ITEM + ' Landed.')])
    yield ('archive move, Plat unchanged',
           base_one,
           backlog([OTHER]),
           {'BACKLOG-DONE.md': done_same}, 0, 'archived, verified verbatim')
    yield ('archive move + Plat NARROWED in the archive copy',
           base_one,
           backlog([OTHER]),
           {'BACKLOG-DONE.md': done_narrowed}, 1, 'MISSING from head')

    # 9/10. Renumber -- the escape hatch added 2026-08-31 -- with and without a
    #       Plat change.  Case 10 is the one A35 does not describe: it does not
    #       merely refuse, it reports a DROP and a spurious NEW ROW, so the
    #       operator is told two wrong things rather than one right one.
    yield ('renumber, Plat unchanged',
           base_one,
           backlog([row('E84', 'TODO', 'any', 'RENUMBERED FROM E79. ' + BASE_ITEM), OTHER]),
           None, 0, 'renumbered, Item text verbatim')
    yield ('renumber + Plat NARROW (the "fresh id" route)',
           base_one,
           backlog([row('E84', 'TODO', 'linux', 'RENUMBERED FROM E79. ' + BASE_ITEM), OTHER]),
           None, 1, 'MISSING from head')

    # 11. The route that DOES pass, and it is the reason this is worth ruling on
    #     rather than living with: duplicate the row at the right Plat and
    #     status-flip the original out of the way.  Legal, and it is exactly the
    #     two-ids-for-one-job defect C15/C16 and A31 exist to prevent.
    yield ('WORKAROUND: duplicate at new Plat + flip original to WONTFIX',
           base_one,
           backlog([row('E79', 'WONTFIX', 'any', BASE_ITEM + ' Superseded by E84, which carries the measured platform.'),
                    row('E84', 'TODO', 'linux', BASE_ITEM), OTHER]),
           None, 0, '1 new row(s): E84')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--verbose', action='store_true', help='print each case\'s full output')
    args = ap.parse_args()

    if not os.path.exists(CHECK):
        print('cannot find %s' % CHECK)
        return 1

    rows, failures = [], 0
    for label, base, head, extra, want_rc, want_sub in cases():
        rc, out = run_case(base, head, extra)
        ok = (rc == want_rc) and (want_sub in out)
        if not ok:
            failures += 1
        rows.append((label, want_rc, rc, 'ok' if ok else 'UNEXPECTED'))
        if args.verbose or not ok:
            print('--- %s (want rc=%d, got rc=%d)\n%s' % (label, want_rc, rc, out))

    width = max(len(r[0]) for r in rows)
    print('\n| case | expected rc | actual rc | |')
    print('|%s|---|---|---|' % ('-' * (width + 2)))
    for label, want, got, verdict in rows:
        print('| %-*s | %d | %d | %s |' % (width, label, want, got, verdict))

    passing = [r[0] for r in rows if r[2] == 0]
    print('\n%d of %d cases accepted by the check: %s'
          % (len(passing), len(rows), ', '.join(passing)))
    if failures:
        print('\n%d case(s) did NOT behave as A35 recorded. The check has moved; '
              're-read A35 before acting on it.' % failures)
        return 1
    print("\nEvery case behaves as A35's reading says. No Plat-changing edit is "
          "accepted by any of the four branches.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
