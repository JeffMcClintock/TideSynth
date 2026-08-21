#!/usr/bin/env python3
"""Fleet watchdog: one refreshed-in-place digest issue, generated not hand-maintained.

Written for A6 (2026-08-13). BACKLOG A6's own reasoning: "the hand-maintained
state table was wrong within three days, twice." This script is the
replacement -- it reads BACKLOG.md, docs/decisions.md and git/GitHub state
directly and regenerates the digest body from scratch every run, so it is
never stale in the way a table someone updates by hand always eventually is.

Cross-repo scope, decided 2026-08-13: this runs with the workflow's default
GITHUB_TOKEN, which GitHub scopes to the repo the workflow lives in --
TideSynth -- and cannot read gmpi_ui, GMPI_Wrappers, SynthEditLib or the
private SynthEdit repo. Rather than skip cross-repo checks silently, each one
that needs another repo is attempted and reports "cross-repo: unavailable"
inline in the digest if it 403s/404s, so the gap is visible in the one place
this project treats as the source of truth, not silently absent. Widening
this needs a deliberate secret (a read-only cross-repo PAT) added by Jeff --
not something this script or the workflow adds itself.

    python scripts/watchdog-digest.py [--repo-root DIR] [--dry-run]

--dry-run prints the digest body to stdout and does not touch GitHub at all
(no `gh` calls beyond what data-gathering strictly needs). Without it, the
script finds or creates the digest issue (matched by DIGEST_TITLE) and edits
it in place.

Needs `gh` authenticated in the environment (GH_TOKEN or interactive `gh auth
login`) and a full git history checkout (age-by-git-blame needs it) --
`fetch-depth: 0` in CI.
"""
import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timedelta, timezone

DIGEST_TITLE = "Fleet watchdog digest"
DIGEST_MARKER = "<!-- watchdog-digest: do not edit by hand, regenerated every run -->"
REPO = "JeffMcClintock/TideSynth"


def run(cmd, check=True):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if check and r.returncode != 0:
        raise RuntimeError('%s failed: %s' % (' '.join(cmd), r.stderr.strip()))
    return r.stdout


def gh_json(args):
    """For `gh ... --json ...` calls, which emit real JSON. Do not use this
    for `--jq` scalar extraction -- `--jq '.state'` prints the bare string
    `open`, which is not valid JSON and will fail json.loads."""
    try:
        out = run(['gh'] + args)
        return json.loads(out) if out.strip() else None
    except (RuntimeError, json.JSONDecodeError) as e:
        return {'__error__': str(e)}


def gh_text(args):
    """For `gh api ... --jq '<scalar expr>'` calls. Returns (ok, text)."""
    try:
        return True, run(['gh'] + args).strip()
    except RuntimeError as e:
        return False, str(e)


def age_str(dt):
    delta = datetime.now(timezone.utc) - dt
    hours = delta.total_seconds() / 3600
    if hours < 48:
        return '%.0fh' % hours
    return '%.1fd' % (hours / 24)


def parse_iso(s):
    return datetime.fromisoformat(s.replace('Z', '+00:00'))


def row_first_seen(repo_root, needle):
    """Most recent commit that introduced this exact substring into BACKLOG.md
    -- i.e. when the row most recently took on its current status. Uses
    pickaxe (-S), not -L, so it survives the row's line number moving as
    other rows are added/archived around it."""
    out = run(['git', '-C', repo_root, 'log', '-1', '--format=%aI', '-S', needle, '--', 'BACKLOG.md'],
              check=False)
    return parse_iso(out.strip()) if out.strip() else None


ROW = re.compile(r'^\| ([^\|]+) \| ([^\|]+) \| ([^\|]+) \| (.*) \|\s*$', re.MULTILINE)
PR_URL = re.compile(r'github\.com/([\w.-]+)/([\w.-]+)/pull/(\d+)')


def parse_backlog_rows(text):
    rows = []
    for m in ROW.finditer(text):
        rid, status, plat, item = (g.strip() for g in m.groups())
        if rid == 'ID' or set(rid) <= {'-'}:
            continue
        rows.append((rid, status, plat, item))
    return rows


def check_doing_rows(repo_root, rows, open_pr_branches):
    lines = ['### DOING rows older than 72h with no open PR\n']
    stale = []
    for rid, status, plat, item in rows:
        if status != 'DOING':
            continue
        seen = row_first_seen(repo_root, '| %s | DOING |' % rid)
        if not seen:
            lines.append('- `%s`: DOING, age unknown (no matching commit found)' % rid)
            continue
        hours = (datetime.now(timezone.utc) - seen).total_seconds() / 3600
        # Branch convention: tide/{platform}/{id}-{slug}. Match on the ID
        # appearing in a branch under that platform's prefix, rather than a
        # keyword search of PR title/body text, which the branch name is not.
        prefix = 'tide/%s/' % plat.lower() if plat != 'any' else 'tide/'
        open_pr = any(b.startswith(prefix) and rid.lower() in b.lower() for b in open_pr_branches)
        if hours > 72 and not open_pr:
            stale.append('- `%s` (%s): claimed %s ago, no open PR whose branch matches `%s%s*` -- '
                          'likely a dead claim, reset to TODO' % (rid, plat, age_str(seen), prefix, rid.lower()))
    if stale:
        lines.extend(stale)
    else:
        lines.append('None.')
    return '\n'.join(lines)


def check_needs_jeff(repo_root, rows):
    lines = ['\n### Open NEEDS-JEFF rows, by age\n']
    found = []
    for rid, status, plat, item in rows:
        if status != 'NEEDS-JEFF':
            continue
        seen = row_first_seen(repo_root, '| %s | NEEDS-JEFF |' % rid)
        age = age_str(seen) if seen else 'unknown'
        found.append((seen or datetime.min.replace(tzinfo=timezone.utc), rid, age, item[:100]))
    found.sort(reverse=True)
    if found:
        for _, rid, age, item in found:
            lines.append('- `%s` (%s old): %s...' % (rid, age, item))
    else:
        lines.append('None.')
    return '\n'.join(lines)


def check_proposed(repo_root):
    lines = ['\n### Open PROPOSED questions in docs/decisions.md\n']
    path = os.path.join(repo_root, 'docs', 'decisions.md')
    if not os.path.exists(path):
        lines.append('docs/decisions.md not found.')
        return '\n'.join(lines)
    with open(path, encoding='utf-8') as f:
        text = f.read()
    # Skip fenced code blocks -- the escalation template itself contains a
    # literal "PROPOSED: <one-line question>" example line, not a real one.
    in_fence, matches = False, []
    for line in text.split('\n'):
        if line.lstrip().startswith('```'):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        if re.match(r'^PROPOSED:', line) and '<one-line question>' not in line:
            matches.append(line)
    if matches:
        for m in matches:
            lines.append('- ' + m[:150])
    else:
        lines.append('None.')
    return '\n'.join(lines)


def check_in_review(rows):
    lines = ['\n### IN-REVIEW rows -- PR merge status\n']
    found = []
    for rid, status, plat, item in rows:
        if status != 'IN-REVIEW':
            continue
        prs = PR_URL.findall(item)
        if not prs:
            found.append('- `%s`: IN-REVIEW but no PR link found in the row text' % rid)
            continue
        states, all_merged, any_unreachable = [], True, False
        for owner, repo, num in prs:
            # .state alone is not enough: a PR that was closed WITHOUT merging
            # also has state "closed", so merged-ness needs the separate
            # .merged boolean, not a string match on .state.
            ok, out = gh_text(['api', 'repos/%s/%s/pulls/%s' % (owner, repo, num),
                                '--jq', '.state + "," + (.merged | tostring)'])
            if not ok:
                states.append('%s/%s#%s: cross-repo unavailable' % (owner, repo, num))
                all_merged, any_unreachable = False, True
                continue
            state, merged = out.split(',', 1)
            states.append('%s/%s#%s: %s%s' % (owner, repo, num, state, ' (merged)' if merged == 'true' else ''))
            if merged != 'true':
                all_merged = False
        if all_merged:
            tag = 'ALL MERGED -- ready to flip to DONE'
        elif any_unreachable:
            tag = '; '.join(states) + ' -- cannot fully confirm without cross-repo access'
        else:
            tag = '; '.join(states)
        found.append('- `%s`: %s' % (rid, tag))
    if found:
        lines.extend(found)
    else:
        lines.append('None.')
    return '\n'.join(lines)


def check_orphan_branches():
    lines = ['\n### `tide/**` branches in TideSynth with no open or merged PR\n']
    ok, out = gh_text(['api', 'repos/%s/branches' % REPO, '--paginate', '--jq', '.[].name'])
    if not ok:
        lines.append('Could not list branches: %s' % out)
        return '\n'.join(lines)
    branch_names = [b for b in out.splitlines() if b.startswith('tide/')]
    all_prs = gh_json(['pr', 'list', '--repo', REPO, '--state', 'all', '--json', 'headRefName', '--limit', '200'])
    pr_branches = set(p['headRefName'] for p in all_prs) if isinstance(all_prs, list) else set()
    orphans = [b for b in branch_names if b not in pr_branches]
    if orphans:
        for b in orphans:
            lines.append('- `%s` -- no PR ever opened for it' % b)
    else:
        lines.append('None.')
    return '\n'.join(lines)


def check_open_prs():
    lines = ['\n### Open PRs in TideSynth, by age\n']
    prs = gh_json(['pr', 'list', '--repo', REPO, '--state', 'open', '--json', 'number,title,createdAt'])
    if not isinstance(prs, list):
        lines.append('Could not list PRs.')
        return '\n'.join(lines)
    prs.sort(key=lambda p: p['createdAt'])
    if prs:
        for p in prs:
            lines.append('- #%d (%s old): %s' % (p['number'], age_str(parse_iso(p['createdAt'])), p['title']))
    else:
        lines.append('None.')
    return '\n'.join(lines)


def latest_entry_per_platform(repo_root):
    """Most recent journal entry date for each box, across the live journal AND
    its archives.

    Reading only JOURNAL.md is wrong for this check, and the failure is not
    hypothetical: rotation (A8/A24) moves entries out by age, so a box that is
    running normally but less often than the others has its last entry carried
    into JOURNAL-<YYYY>-<MM>.md by somebody else's busy day. The live file then
    says "no entry found", which is indistinguishable from a box that has never
    run -- the exact confusion this check exists to remove."""
    latest = {}
    names = ['JOURNAL.md'] + sorted(
        (f for f in os.listdir(repo_root) if re.match(r'^JOURNAL-\d{4}-\d{2}\.md$', f)),
        reverse=True)
    for name in names:
        path = os.path.join(repo_root, name)
        if not os.path.exists(path):
            continue
        with open(path, encoding='utf-8') as f:
            text = f.read()
        for m in re.finditer(r'^## (\d{4}-\d{2}-\d{2}) — (windows|macos|linux) ', text, re.MULTILINE):
            date_s, plat = m.groups()
            if plat not in latest or date_s > latest[plat]:
                latest[plat] = date_s
    return latest


# Days of silence before a box is called quiet, and before it is called likely
# halted. The fleet runs roughly daily, so one missed day is ordinary (a machine
# off, a travel day) and three is not. Seven is past any explanation that does
# not need somebody to look.
QUIET_DAYS = 3
HALTED_DAYS = 7

# The bot credential's expiry, from docs/weekly-run-prompt.md's "Becoming the
# agent" section. Hard-coded rather than queried because the digest runs in CI
# under the workflow's GITHUB_TOKEN, not under the bot PAT, so asking the API
# about "this token" would measure the wrong one. Update it when the token is
# rotated -- and note the whole point of the countdown is that this date halts
# ALL THREE boxes on the same day.
TOKEN_EXPIRY = '2026-11-07'
TOKEN_WARN_DAYS = 30


def check_halted_boxes(repo_root):
    """A12: a halted box is invisible to the fleet, and nothing escalates it.

    A run that fails STEP 0.7's identity assertion is told to write a journal
    entry and stop -- but it cannot PUSH that entry, because the thing it just
    failed to obtain is the credential it would need. So a halted box emits
    nothing at all, and silence is the only evidence there is.

    That is why this check is about absence, and why it is careful to separate
    the two ways a box can produce no work:

      - it RAN AND DID NOTHING -- queue blocked, nothing eligible. The run still
        writes and pushes a journal entry saying so. Visible, and fine.
      - it HALTED -- or never woke. No entry, no branch, no PR. Invisible.

    A recent entry means the box is alive whatever the entry says, so the
    presence of an entry is the discriminator, not its content."""
    lines = ['\n### Fleet liveness -- boxes that have gone quiet (A12)\n',
             'A box that ran and found nothing to do still writes a journal entry, so it '
             'appears here as fine. A box that halted at STEP 0.7 cannot push anything at '
             'all -- it has no credential to push WITH -- so silence is the only symptom '
             'it can produce, and this section is the thing that notices.\n']

    latest = latest_entry_per_platform(repo_root)
    today = datetime.now(timezone.utc)
    flagged = []

    for plat in ('windows', 'macos', 'linux'):
        if plat not in latest:
            lines.append('- **%s: NO ENTRY EVER FOUND** -- in the journal or any archive. '
                         'Either this box has never completed a run, or its entries predate '
                         'the archives in this repo.' % plat)
            flagged.append(plat)
            continue
        d = datetime.strptime(latest[plat], '%Y-%m-%d').replace(tzinfo=timezone.utc)
        days = (today - d).days
        if days >= HALTED_DAYS:
            lines.append('- **%s: LIKELY HALTED -- silent %d days** (last entry %s). '
                         'Past any ordinary explanation. Check STEP 0.7 on that box by hand.'
                         % (plat, days, latest[plat]))
            flagged.append(plat)
        elif days >= QUIET_DAYS:
            lines.append('- **%s: QUIET -- silent %d days** (last entry %s). '
                         'Ordinary if the machine was off; worth a look if it was not.'
                         % (plat, days, latest[plat]))
            flagged.append(plat)
        else:
            lines.append('- %s: alive -- last entry %s (%d day%s ago).'
                         % (plat, latest[plat], days, '' if days == 1 else 's'))

    if flagged:
        lines.append('\n**What to do for a flagged box, and why the digest cannot do it for you.** '
                     'Run STEP 0.7\'s two assertions on that machine:\n')
        lines.append('```\ngh api user --jq .login\n'
                     'gh api graphql -f query=\'{ viewer { login databaseId } }\'\n'
                     'git config --global --get url."https://github.com/".insteadOf\n```\n')
        lines.append('**The failing assertion cannot be reported from here.** A halted run holds '
                     'no credential it is permitted to use, so it cannot file an issue, push a '
                     'branch, or comment -- and using the developer\'s own credential to say so '
                     'is precisely what STEP 0.7 forbids. Naming the silent box is therefore the '
                     'most this repo can know; reading the assertion needs someone at that keyboard.')

    return '\n'.join(lines)


def check_credential_expiry():
    """The one fleet-wide halt that is known in advance, so it should never
    arrive as a surprise. When the bot token expires, all three boxes fail
    STEP 0.7 on the same day and all three go silent together -- which the
    check above would report as three simultaneous halts with no cause."""
    lines = ['\n### Bot credential expiry\n']
    expiry = datetime.strptime(TOKEN_EXPIRY, '%Y-%m-%d').replace(tzinfo=timezone.utc)
    days = (expiry - datetime.now(timezone.utc)).days
    if days < 0:
        lines.append('- **EXPIRED %d days ago (%s).** Every box fails STEP 0.7 until the token '
                     'is rotated; expect the section above to show all three as halted.'
                     % (-days, TOKEN_EXPIRY))
    elif days <= TOKEN_WARN_DAYS:
        lines.append('- **Expires in %d days (%s)** -- rotate it before then, or all three boxes '
                     'halt on the same day.' % (days, TOKEN_EXPIRY))
    else:
        lines.append('- Expires %s (%d days away).' % (TOKEN_EXPIRY, days))
    return '\n'.join(lines)


def build_digest(repo_root):
    with open(os.path.join(repo_root, 'BACKLOG.md'), encoding='utf-8') as f:
        backlog_text = f.read()
    rows = parse_backlog_rows(backlog_text)

    open_prs = gh_json(['pr', 'list', '--repo', REPO, '--state', 'open', '--json', 'headRefName'])
    open_pr_branches = [p['headRefName'] for p in open_prs] if isinstance(open_prs, list) else []

    now = datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')
    parts = [
        DIGEST_MARKER,
        '# %s' % DIGEST_TITLE,
        '',
        'Regenerated %s. This is the single "awaiting Jeff / awaiting the fleet" '
        'surface -- generated fresh every run, never hand-maintained. See BACKLOG A6.' % now,
        '',
        check_doing_rows(repo_root, rows, open_pr_branches),
        check_orphan_branches(),
        check_in_review(rows),
        check_needs_jeff(repo_root, rows),
        check_proposed(repo_root),
        check_open_prs(),
        check_halted_boxes(repo_root),
        check_credential_expiry(),
    ]
    return '\n'.join(parts)


def selftest():
    """A12's Accept clause, made re-runnable.

    The clause asks for a demonstration rather than an argument, and the honest
    demonstration available to a script is a fixture: journals written with
    known dates, checked for the classification each should produce. It does
    NOT induce a real STEP 0.7 halt on a real box -- nothing in this repo can
    do that, and the row should not be read as if it had.

    The fourth case is the one worth having. It is the false alarm this check
    shipped with: a box whose last entry has been ROTATED into the archive
    reads as "no entry found" if you only open JOURNAL.md, which is the same
    output as a box that never ran."""
    import tempfile, shutil

    today = datetime.now(timezone.utc)
    def ago(n):
        return (today - timedelta(days=n)).strftime('%Y-%m-%d')

    cases = [
        # (name, days-since-entry, which file it lives in, expected substring)
        ('alive',         0, 'live',    'alive'),
        ('quiet',         QUIET_DAYS,  'live',    'QUIET'),
        ('halted',        HALTED_DAYS, 'live',    'LIKELY HALTED'),
        ('rotated-alive', 1, 'archive', 'alive'),
    ]

    failures = []
    for name, days, where, expect in cases:
        d = tempfile.mkdtemp(prefix='watchdog-selftest-')
        try:
            entry = '## %s — windows — fixture\n\nbody\n\n' % ago(days)
            # macos/linux always fresh, so only the windows line is under test
            fresh = ''.join('## %s — %s — fixture\n\nbody\n\n' % (ago(0), p)
                            for p in ('macos', 'linux'))
            live = '# Journal\n\n' + fresh + (entry if where == 'live' else '')
            with open(os.path.join(d, 'JOURNAL.md'), 'w', encoding='utf-8') as f:
                f.write(live)
            arch = '# Journal — archive\n\n' + (entry if where == 'archive' else '')
            with open(os.path.join(d, 'JOURNAL-2026-08.md'), 'w', encoding='utf-8') as f:
                f.write(arch)

            out = check_halted_boxes(d)
            line = next((l for l in out.split('\n') if l.startswith('- ') and 'windows' in l), '')
            ok = expect in line
            print('%-16s %-8s -> %s' % (name, 'PASS' if ok else 'FAIL', line.strip() or '(no windows line)'))
            if not ok:
                failures.append('%s: expected %r in %r' % (name, expect, line))
        finally:
            shutil.rmtree(d, ignore_errors=True)

    if failures:
        print('\nFAILED:')
        for f in failures:
            print('  ' + f)
        return 1
    print('\nall %d liveness classifications correct' % len(cases))
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', default='.')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--selftest', action='store_true',
                    help="check the fleet-liveness classifications against fixtures (A12's Accept)")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    body = build_digest(args.repo_root)

    if args.dry_run:
        print(body)
        return 0

    existing = gh_json(['issue', 'list', '--repo', REPO, '--state', 'open',
                         '--search', DIGEST_TITLE, '--json', 'number,title'])
    match = None
    if isinstance(existing, list):
        match = next((i for i in existing if i['title'] == DIGEST_TITLE), None)

    if match:
        run(['gh', 'issue', 'edit', str(match['number']), '--repo', REPO, '--body', body])
        print('Refreshed issue #%d' % match['number'])
    else:
        out = run(['gh', 'issue', 'create', '--repo', REPO, '--title', DIGEST_TITLE, '--body', body])
        print('Created: ' + out.strip())
    return 0


if __name__ == '__main__':
    sys.exit(main())
