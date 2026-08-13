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
from datetime import datetime, timezone

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


def check_journal_freshness(repo_root):
    lines = ['\n### Days since each platform\'s last journal entry\n',
             '(Not "missed its run window" -- this repo has no record of each box\'s '
             'actual cron schedule, which lives in `~/.claude/scheduled-tasks/` on that '
             'box, not here. This is the closest honest proxy.)\n']
    path = os.path.join(repo_root, 'JOURNAL.md')
    if not os.path.exists(path):
        lines.append('JOURNAL.md not found.')
        return '\n'.join(lines)
    with open(path, encoding='utf-8') as f:
        text = f.read()
    latest = {}
    for m in re.finditer(r'^## (\d{4}-\d{2}-\d{2}) — (windows|macos|linux) ', text, re.MULTILINE):
        date_s, plat = m.groups()
        if plat not in latest:
            latest[plat] = date_s
    for plat in ('windows', 'macos', 'linux'):
        if plat in latest:
            d = datetime.strptime(latest[plat], '%Y-%m-%d').replace(tzinfo=timezone.utc)
            days = (datetime.now(timezone.utc) - d).days
            lines.append('- %s: last entry %s (%d day%s ago)' % (plat, latest[plat], days, '' if days == 1 else 's'))
        else:
            lines.append('- %s: no entry found' % plat)
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
        check_journal_freshness(repo_root),
    ]
    return '\n'.join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', default='.')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

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
