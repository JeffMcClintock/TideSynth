#!/usr/bin/env python3
"""A38 -- measure whether two runs' STEP 4 bookkeeping edits can merge.

BACKLOG A38 asserts, from six consecutive STEP 1.5 runs that did nothing but
re-resolve the same three files, that the fleet's bookkeeping is a LIVELOCK, and
that the mechanism is ADJACENT LINES rather than disagreement: two runs editing
strictly disjoint content still collide, because git needs at least one unchanged
line of context between two sides' insertions to merge them as separate hunks.

A38's own Accept clause asks for exactly this: *"two runs on different platforms
can each append a journal entry and re-point their own NEXT cell, on branches cut
from the same `main`, and both merge with no conflict -- demonstrated by a
scripted two-branch test, not by waiting for it to happen."*  This is that test.

It is published under an OPEN question (the `PROPOSED:` entry A38 filed in
docs/decisions.md), so it is deliberately NEUTRAL between the options: it asserts
today's behaviour and each candidate layout's behaviour, and picks nothing.  That
is the A35 precedent -- "a probe that asserts today's behaviour is neutral
between the options, and that is what makes it publishable under an open
question" (2026-09-08 macos).

WHAT IT DOES.  For each layout it builds a throwaway git repo seeded with THIS
repo's REAL `BACKLOG.md` and `JOURNAL.md` (not a synthetic stand-in -- the whole
claim is about these two files' real shape), then replays the fleet's actual
sequence:

    main @ base
      |-- branch win : this platform's STEP 4 edits, committed
      |-- branch mac : the other platform's STEP 4 edits, committed
    merge mac -> main          (the other box's PR merges first)
    merge main -> win          (<- the re-resolution the fleet keeps paying)

and records whether that last merge conflicts, and in which files.

The edits are deliberately DISJOINT in content: `win` re-points only the `win`
NEXT cell and prepends only its own journal entry; `mac` touches only `mac`'s.
Any conflict is therefore structural, never disagreement -- which is A38's claim.

LAYOUTS MEASURED

  today        what `main` has now: one NEXT markdown table whose platform rows
               are adjacent lines, one JOURNAL.md every run prepends to.
  next-split   NEXT cells move to `docs/next/<platform>.md`, one file per lane;
               JOURNAL.md unchanged.
  next-sections NEXT stays in BACKLOG.md but becomes blank-line-separated
               per-platform sections instead of table rows -- the cheap option,
               which tests whether CONTEXT alone is enough without new files.
  full-split   next-split plus one journal file per run (`journal/<date>-<plat>.md`);
               JOURNAL.md is not touched by a run at all.

CONTROLS.  Two, because a conflict only means something once you have shown the
same harness merges cleanly when it should:

  ctl-context  today's layout, but the two sides' journal insertions are
               separated by one unchanged entry.  This is the positive control
               for the mechanism claim, and it is a WITHIN-CASE one: JOURNAL.md
               must drop out of the conflict list while BACKLOG.md -- whose
               adjacent table rows this control does not touch -- stays in it.
               If JOURNAL.md still conflicts, the "adjacent lines" model is wrong
               and A38 wants re-reading before anyone acts on it.
  ctl-code     both sides edit an ordinary source file in different places.
               The fleet has never once conflicted in code; this says the
               harness agrees.

Run:  python3 tests/a38_bookkeeping_merge_probe.py [--verbose]
Exit: 0 if every case behaves as recorded in EXPECTED below, 1 if any has moved.
      A moved result means A38's premise has changed and the row -- and the
      PROPOSED entry resting on it -- want re-reading before anyone acts.

Needs git on PATH and nothing else.  No network, no build, ~5 s.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)

# Two runs, same day, cut from the same base -- the fleet's normal Sunday.
DATE = '2026-09-21'
MINE, THEIRS = 'win', 'mac'


def git(repo, *args, check=True):
    """Run git in `repo`; return (rc, stdout). Never raises on a merge conflict."""
    env = dict(os.environ)
    # Authorship here is irrelevant (throwaway repo) but must be SET, or git
    # refuses to commit on a box with no user.name configured.
    env.update({
        'GIT_AUTHOR_NAME': 'probe', 'GIT_AUTHOR_EMAIL': 'probe@example.invalid',
        'GIT_COMMITTER_NAME': 'probe', 'GIT_COMMITTER_EMAIL': 'probe@example.invalid',
        'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_SYSTEM': os.devnull,
    })
    p = subprocess.run(['git', '-C', repo] + list(args), capture_output=True,
                       text=True, env=env)
    if check and p.returncode != 0:
        raise RuntimeError('git %s failed: %s%s' % (' '.join(args), p.stdout, p.stderr))
    return p.returncode, p.stdout


def read(path):
    with open(path, 'rb') as f:
        return f.read()


def write(path, data):
    d = os.path.dirname(path)
    if d and not os.path.isdir(d):
        os.makedirs(d)
    with open(path, 'wb') as f:
        f.write(data)


# ---------------------------------------------------------------- edits

def journal_entry(plat, marker=''):
    """One run's STEP 4 journal entry, as bytes, newest-at-top shaped."""
    return (
        '## %s — %s — %s (scheduled run)\n\n'
        '**Prompt:** probe · **Did:** the work.\n\n'
        '**Learned:** nothing, this is a probe.\n\n' % (DATE, plat, marker or 'an item')
    ).encode()


def prepend_journal(repo, plat, path='JOURNAL.md'):
    """Insert a new entry above the newest one -- what STEP 4 tells every run."""
    p = os.path.join(repo, path)
    b = read(p)
    i = b.index(b'\n## ') + 1          # start of the first dated entry
    write(p, b[:i] + journal_entry(plat) + b[i:])


def insert_journal_after_first(repo, plat):
    """ctl-context only: insert BELOW the newest entry, so one unchanged entry
    sits between this insertion and a prepend by the other side."""
    p = os.path.join(repo, 'JOURNAL.md')
    b = read(p)
    first = b.index(b'\n## ') + 1
    second = b.index(b'\n## ', first) + 1
    write(p, b[:second] + journal_entry(plat) + b[second:])


def repoint_table_cell(repo, plat):
    """Re-point one row of the NEXT markdown table -- today's layout."""
    p = os.path.join(repo, 'BACKLOG.md')
    b = read(p)
    tag = b'| ' + plat.encode() + b' |'
    lines = b.split(b'\n')
    for n, line in enumerate(lines):
        if line.startswith(tag):
            lines[n] = (tag + b' **RE-POINTED %s (%s, scheduled run).** New cell text. '
                        b'**Previous cell follows.** ' % (DATE.encode(), plat.encode())
                        + line[len(tag):].lstrip())
            write(p, b'\n'.join(lines))
            return
    raise AssertionError('no NEXT row for ' + plat)


def repoint_section(repo, plat):
    """Re-point one blank-line-separated section -- the `next-sections` layout."""
    p = os.path.join(repo, 'BACKLOG.md')
    b = read(p)
    head = b'### ' + plat.encode() + b'\n\n'
    i = b.index(head) + len(head)
    write(p, b[:i] + b'**RE-POINTED %s.** New cell text.\n\n' % DATE.encode() + b[i:])


def repoint_lane_file(repo, plat):
    """Re-point a per-lane file -- the `next-split` / `full-split` layouts."""
    p = os.path.join(repo, 'docs', 'next', plat + '.md')
    write(p, b'# NEXT -- ' + plat.encode() + b'\n\n**RE-POINTED %s.** New cell text.\n\n'
          % DATE.encode() + read(p).split(b'\n\n', 2)[-1])


def new_journal_file(repo, plat):
    """One journal file per run -- the `full-split` layout."""
    write(os.path.join(repo, 'journal', '%s-%s.md' % (DATE, plat)), journal_entry(plat))


def edit_code(repo, plat):
    """ctl-code: two different functions in one ordinary source file."""
    p = os.path.join(repo, 'src.cpp')
    b = read(p).replace(b'// %s-here' % plat.encode(), b'int %s_added() { return 1; }' % plat.encode())
    write(p, b)


# ---------------------------------------------------------------- layouts

def seed_common(repo):
    """Files every layout starts from: the repo's REAL bookkeeping files."""
    shutil.copyfile(os.path.join(REPO, 'BACKLOG.md'), os.path.join(repo, 'BACKLOG.md'))
    shutil.copyfile(os.path.join(REPO, 'JOURNAL.md'), os.path.join(repo, 'JOURNAL.md'))
    # A small ordinary source file, for ctl-code.  Two edit points, far apart.
    write(os.path.join(repo, 'src.cpp'),
          b'// win-here\n' + b'int filler() { return 0; }\n' * 40 + b'// mac-here\n')


def to_sections(repo):
    """Rewrite the NEXT table as blank-line-separated per-platform sections."""
    p = os.path.join(repo, 'BACKLOG.md')
    b = read(p)
    lines = b.split(b'\n')
    out, cells = [], []
    for line in lines:
        m = [t for t in (b'win', b'mac', b'linux', b'any') if line.startswith(b'| ' + t + b' |')]
        if m:
            cells.append((m[0], line.split(b'|', 3)[3].rstrip(b' |')))
            continue
        if line.startswith(b'| Platform |') or line.startswith(b'|---|---|---|'):
            continue
        out.append(line)
    block = []
    for name, text in cells:
        block += [b'### ' + name, b'', text.strip(), b'']
    # Put the sections where the table was: right after the NEXT heading.
    i = out.index(b'## NEXT \xe2\x80\x94 per-platform priority, obeyed before file order') + 1
    write(p, b'\n'.join(out[:i] + [b''] + block + out[i:]))


def to_lane_files(repo):
    """Move each NEXT cell into docs/next/<platform>.md, leaving a link table."""
    p = os.path.join(repo, 'BACKLOG.md')
    b = read(p)
    lines, out = b.split(b'\n'), []
    for line in lines:
        m = [t for t in (b'win', b'mac', b'linux', b'any') if line.startswith(b'| ' + t + b' |')]
        if m:
            name = m[0].decode()
            text = line.split(b'|', 3)[3].rstrip(b' |').strip()
            write(os.path.join(repo, 'docs', 'next', name + '.md'),
                  b'# NEXT -- ' + m[0] + b'\n\n' + text + b'\n')
            out.append(b'| ' + m[0] + b' | [docs/next/' + m[0] + b'.md](docs/next/' + m[0] + b'.md) |')
            continue
        out.append(line)
    write(p, b'\n'.join(out))


LAYOUTS = {
    # name: (transform-after-seed, edits-each-side-applies)
    'today':         (None,          [repoint_table_cell, prepend_journal]),
    'next-sections': (to_sections,   [repoint_section, prepend_journal]),
    'next-split':    (to_lane_files, [repoint_lane_file, prepend_journal]),
    'full-split':    (to_lane_files, [repoint_lane_file, new_journal_file]),
    'ctl-code':      (None,          [edit_code]),
}

# ctl-context is `today` with ONE asymmetry: the other side inserts its journal
# entry below the newest one instead of above it, so an unchanged entry sits
# between the two insertions.  Everything else is identical.
CTL_CONTEXT = 'ctl-context'


def run_case(name, verbose=False):
    tmp = tempfile.mkdtemp(prefix='a38-%s-' % name)
    try:
        repo = tmp
        git(repo, 'init', '-q', '-b', 'main')
        seed_common(repo)
        transform, edits = LAYOUTS['today' if name == CTL_CONTEXT else name]
        if transform:
            transform(repo)
        git(repo, 'add', '-A')
        git(repo, 'commit', '-q', '-m', 'base')

        for side in (MINE, THEIRS):
            git(repo, 'checkout', '-q', '-b', side, 'main')
            for edit in edits:
                if name == CTL_CONTEXT and side == THEIRS and edit is prepend_journal:
                    insert_journal_after_first(repo, side)
                else:
                    edit(repo, side)
            git(repo, 'add', '-A')
            git(repo, 'commit', '-q', '-m', '%s STEP 4' % side)

        # Their PR merges to main first.
        git(repo, 'checkout', '-q', 'main')
        git(repo, 'merge', '-q', '--no-ff', '-m', 'merge %s' % THEIRS, THEIRS)

        # Now MY open PR has to take main -- the cost A38 is about.
        git(repo, 'checkout', '-q', MINE)
        rc, _ = git(repo, 'merge', '--no-edit', 'main', check=False)
        _, unmerged = git(repo, 'diff', '--name-only', '--diff-filter=U')
        files = tuple(sorted(f for f in unmerged.split('\n') if f))
        if verbose:
            print('  %-14s rc=%d  conflicts: %s' % (name, rc, files or '(none)'))
        return files
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# What this probe measured when it was written.  A change here is a finding,
# not a failure of the probe -- see the module docstring.
EXPECTED = {
    'today':         ('BACKLOG.md', 'JOURNAL.md'),
    'next-sections': ('JOURNAL.md',),
    'next-split':    ('JOURNAL.md',),
    'full-split':    (),
    # ctl-context is `today` with ONE thing changed -- the journal insertions are
    # separated by an unchanged entry -- so JOURNAL.md drops OUT of the conflict
    # list while BACKLOG.md, whose adjacent rows are untouched by the control,
    # stays in it.  That is the control doing its job: one file moved, for the one
    # reason the model names, in the same run and the same layout.
    'ctl-context':   ('BACKLOG.md',),
    'ctl-code':      (),
}

NOTE = {
    'today':         "both hot spots conflict, on disjoint content",
    'next-sections': "context alone fixes the NEXT block; JOURNAL.md is untouched by it",
    'next-split':    "same as sections for the NEXT block -- new files are not what fixed it",
    'full-split':    "nothing shared, nothing to conflict",
    'ctl-context':   "POSITIVE CONTROL: JOURNAL.md drops out once one entry sits between",
    'ctl-code':      "CONTROL: code has never conflicted in the fleet, and does not here",
}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--verbose', action='store_true')
    args = ap.parse_args()

    for f in ('BACKLOG.md', 'JOURNAL.md'):
        if not os.path.isfile(os.path.join(REPO, f)):
            print('FAIL: %s not found -- run this from a TideSynth checkout' % f)
            return 1

    order = ['today', 'next-sections', 'next-split', 'full-split', 'ctl-context', 'ctl-code']
    rows, moved = [], []
    for name in order:
        got = run_case(name, args.verbose)
        want = EXPECTED[name]
        ok = got == want
        if not ok:
            moved.append((name, want, got))
        rows.append((name, got, ok))

    w = max(len(n) for n in order)
    print('| %-*s | conflicts in                | what it says |' % (w, 'layout'))
    print('|-%s-|-----------------------------|--------------|' % ('-' * w))
    for name, got, ok in rows:
        shown = ', '.join(got) if got else '(clean)'
        print('| %-*s | %-27s | %s%s' % (w, name, shown, NOTE[name], '' if ok else '  <-- MOVED'))

    if moved:
        print('\n%d case(s) no longer behave as recorded:' % len(moved))
        for name, want, got in moved:
            print('  %s: expected %s, got %s' % (name, want or '(clean)', got or '(clean)'))
        print("A38's premise has changed -- re-read the row and the PROPOSED entry.")
        return 1

    print('\nAll six cases as recorded.  The conflict in `today` is structural: the two '
          'sides\nedit different platforms and different journal entries, and share no '
          'claim at all.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
