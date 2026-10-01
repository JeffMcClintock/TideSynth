#!/usr/bin/env python3
"""A40: prove the watchdog's credential countdown is DERIVED, not recited.

A39's trap, one file over: a check that reads a constant passes its own test
for the wrong reason. So every arm below is paired with a control that must
move the output -- a test that cannot fail when the derivation is removed
proves nothing.

    python3 tests/a40_token_expiry_probe.py          # all arms
    python3 tests/a40_token_expiry_probe.py -v       # with each arm's output

Exit 0 = every arm as expected. No network, no credential: headers are
injected, which is the whole point of `check_credential_expiry(headers=...)`.
"""
import argparse
import importlib.util
import os
import re
import sys
from datetime import datetime, timedelta, timezone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = 'github-authentication-token-expiration'


def load():
    path = os.path.join(ROOT, 'scripts', 'watchdog-digest.py')
    spec = importlib.util.spec_from_file_location('watchdog_digest', path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def day(offset):
    return (datetime.now(timezone.utc) + timedelta(days=offset)).strftime('%Y-%m-%d')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-v', '--verbose', action='store_true')
    args = ap.parse_args()
    wd = load()

    arms = []

    def arm(name, ok, detail=''):
        arms.append((name, ok, detail))
        print('  %-4s %s%s' % ('ok' if ok else 'FAIL', name, ('  -- ' + detail) if detail else ''))

    # --- expiry_from_headers: the parse itself -----------------------------
    arm('header parsed, date extracted from a full timestamp',
        wd.expiry_from_headers({HEADER: '2027-03-04 15:04:05 UTC'}) == '2027-03-04')
    arm('header matched case-insensitively',
        wd.expiry_from_headers({'GitHub-Authentication-Token-Expiration': '2027-03-04'}) == '2027-03-04')
    arm('absent header -> None', wd.expiry_from_headers({'x-ratelimit-limit': '5000'}) is None)
    arm('empty headers -> None', wd.expiry_from_headers({}) is None)
    arm('None headers -> None', wd.expiry_from_headers(None) is None)
    arm('unparseable value -> None', wd.expiry_from_headers({HEADER: 'never'}) is None)

    # --- THE NEGATIVE CONTROL ---------------------------------------------
    # Two fabricated headers 100 days apart must produce two different reported
    # dates. If the function recited a constant, both would read the same and
    # this arm is the only thing in the file that would notice.
    near, far = day(10), day(110)
    out_near = wd.check_credential_expiry(headers={HEADER: near})
    out_far = wd.check_credential_expiry(headers={HEADER: far})
    arm('negative control: fabricated header MOVES the reported date',
        near in out_near and far in out_far and out_near != out_far,
        '%s vs %s' % (near, far))
    arm('negative control: neither output recites the recorded constant as the countdown',
        wd.TOKEN_EXPIRY_RECORDED not in out_near.split('- Derived')[0]
        or wd.TOKEN_EXPIRY_RECORDED in (near, far))

    # --- VACUITY CONTROL ---------------------------------------------------
    # Prove the arms above can fail: a stub that ignores its argument and
    # returns the recorded date must break the negative control. Without this,
    # "the test passed" is compatible with the test asserting nothing.
    real = wd.expiry_from_headers
    try:
        wd.expiry_from_headers = lambda h: wd.TOKEN_EXPIRY_RECORDED
        s_near = wd.check_credential_expiry(headers={HEADER: near})
        s_far = wd.check_credential_expiry(headers={HEADER: far})
        arm('vacuity control: a constant-reciting stub FAILS the negative control',
            s_near == s_far and near not in s_near)
    finally:
        wd.expiry_from_headers = real

    # --- the absent-header path -------------------------------------------
    out_none = wd.check_credential_expiry(headers={})
    arm('absent header -> says unknown, not a countdown',
        'unknown -- no expiry header on this credential' in out_none)
    arm('absent header -> prints NO "days away" countdown',
        'days away' not in out_none and 'Expires in' not in out_none)
    arm('absent header -> still surfaces the recorded date, labelled unverified',
        wd.TOKEN_EXPIRY_RECORDED in out_none and 'unverified' in out_none)

    # --- the three countdown bands, each derived --------------------------
    expired = wd.check_credential_expiry(headers={HEADER: day(-5)})
    arm('expired band', 'EXPIRED' in expired and day(-5) in expired)
    warn = wd.check_credential_expiry(headers={HEADER: day(10)})
    arm('warn band', 'Expires in' in warn and day(10) in warn)
    calm = wd.check_credential_expiry(headers={HEADER: day(200)})
    arm('calm band', 'days away' in calm and day(200) in calm)

    # --- disagreement with the doc is reported, not hidden -----------------
    dis = wd.check_credential_expiry(headers={HEADER: day(200)}, recorded='2001-01-01')
    arm('doc/header disagreement is flagged', 'disagrees' in dis and '2001-01-01' in dis)
    agree = wd.check_credential_expiry(headers={HEADER: day(200)}, recorded=day(200))
    arm('no disagreement flagged when they agree', 'disagrees' not in agree)

    # --- header-block parsing stops at the blank line ---------------------
    arm('fetch_response_headers exists and is callable',
        callable(getattr(wd, 'fetch_response_headers', None)))

    if args.verbose:
        print('\n--- absent-header output ---\n' + out_none)
        print('\n--- derived output ---\n' + calm)

    failed = [n for n, ok, _ in arms if not ok]
    print('\n%d arm(s), %d failed' % (len(arms), len(failed)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
