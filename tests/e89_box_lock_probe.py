#!/usr/bin/env python3
"""E89 -- measure that scripts/run-lock.py is a box-scoped mutex that cannot wedge the box.

BACKLOG E89 / issue #583. The defect: the `tidesynth-weekly-linux` scheduled task
fired twice 5.6 s apart and both runs worked in the same trees. The guard is a
lock file taken at STEP 0.5. **A lock has two failure modes and they pull in
opposite directions**, so both have to be measured or the fix is a coin-flip:

  1. it lets two runs in  -- the collision #583 reports, unfixed; and
  2. it lets NO run in    -- a stale lock silently stranding the lane, which
                             #583 says in as many words is the WORSE outcome.

Arms 1-4 and 11-13 measure (1). Arms 5-8 measure (2). Arms 9-10 are housekeeping.
The vacuity control runs first and must FAIL.

**Arms 11-13 exist because the first version of the fix did not work and this
probe did not notice.** `run-lock.py acquire` is a short-lived process, so
recording its OWN pid made the lock stale the moment it exited: measured at
#583's exact 5.6 s gap, run B was told `reclaiming a stale lock (holder pid 7047
is gone)` and let straight through. Arms 1-10 were all green while that was
true, because none of them let a real acquire's process exit before testing the
lock. Arm 12 is the incident itself, with real processes and the real gap; arm 11
is the ancestor walk that makes it pass; arm 13 pins the broken configuration so
a regression says which of the two it is.

**Arm 9 is the load-bearing arm and it is a control, not a test.** It races the
same N-process harness against a deliberately non-atomic acquire -- read, pause,
then create -- and REQUIRES more than one winner. Without it, arm 4's "exactly
one winner out of 12" proves nothing about atomicity: a harness whose processes
never actually overlap reports one winner for a lock that has no mutual exclusion
in it at all. The fleet has paid for this once already (2026-10-07 macos: *"a
control that will not fail is a finding about the probe, not about the code"*),
so the control runs first and the probe fails if it PASSES.

Every arm runs against a lock path in a fresh temporary directory, never against
`~/.tide/run.lock`, so running this probe cannot disturb a real run.

Run:  python3 tests/e89_box_lock_probe.py [--verbose] [--racers N]
Exit: 0 if every arm behaves as recorded here, 1 if any does not.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import textwrap
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
LOCK_SCRIPT = os.path.join(REPO, 'scripts', 'run-lock.py')

# A pid that is certainly not running. Linux caps at /proc/sys/kernel/pid_max
# (4194304 by default) and macOS lower still, so this cannot be allocated.
DEAD_PID = 4194303


def run_lock(lock, *argv):
    """Invoke the real script as a subprocess. Returns (rc, combined output)."""
    cmd = [sys.executable, LOCK_SCRIPT] + list(argv) + ['--lock', lock]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    return proc.returncode, (proc.stdout or '') + (proc.stderr or '')


def write_raw(lock, payload):
    with open(lock, 'w', encoding='utf-8') as fh:
        fh.write(payload if isinstance(payload, str) else json.dumps(payload))


# --- the race harness ----------------------------------------------------
#
# N children all block on the same barrier file and then attempt an acquire, so
# their attempts overlap rather than merely following each other. The barrier is
# what makes this a race: without it, process startup cost (~30 ms of python
# interpreter) serialises them and nothing is ever contended.

RACER = r'''
import json, os, sys, time
sys.path.insert(0, %(scripts)r)
barrier, lock, mode, out = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
while not os.path.exists(barrier):
    time.sleep(0.001)
if mode == 'atomic':
    import importlib.util
    spec = importlib.util.spec_from_file_location('runlock', %(script)r)
    mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
    rc = mod.main(['acquire', '--lock', lock, '--label', 'racer %%d' %% os.getpid()])
else:
    # The CONTROL: check-then-create, with the window held open. This is the
    # shape an obvious implementation has, and it is what arm 9 must catch.
    if os.path.exists(lock):
        rc = 1
    else:
        time.sleep(0.05)
        with open(lock, 'a', encoding='utf-8') as fh:
            fh.write(json.dumps({'pid': os.getpid()}) + '\n')
        rc = 0
with open(out, 'w', encoding='utf-8') as fh:
    fh.write('%%d %%d' %% (rc, os.getpid()))
'''


def race(tmp, mode, racers):
    """Start `racers` children, release them together, return the winner count."""
    lock = os.path.join(tmp, 'race-%s.lock' % mode)
    barrier = os.path.join(tmp, 'barrier-%s' % mode)
    src = os.path.join(tmp, 'racer-%s.py' % mode)
    with open(src, 'w', encoding='utf-8') as fh:
        fh.write(RACER % {'scripts': os.path.join(REPO, 'scripts'), 'script': LOCK_SCRIPT})

    procs = []
    for i in range(racers):
        out = os.path.join(tmp, 'out-%s-%d' % (mode, i))
        procs.append((subprocess.Popen([sys.executable, src, barrier, lock, mode, out],
                                       stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL), out))
    # Let every child reach the barrier spin before any of them can proceed.
    time.sleep(0.6)
    open(barrier, 'w').close()
    for proc, _ in procs:
        proc.wait(timeout=60)

    winners = []
    for _, out in procs:
        with open(out, encoding='utf-8') as fh:
            rc, pid = fh.read().split()
        if int(rc) == 0:
            winners.append(int(pid))
    return lock, winners


# --- long-lived holders, for the arms that need a real run's lifetime ----
#
# Arms 1-10 all use short-lived acquires, which is exactly why they could not
# see the owner-pid defect. These two helpers give the probe a process that is
# still alive while the lock is being judged.

HOLDER = r"""
import os, subprocess, sys, time
lock, script, stamp, marker = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
# This process is the long-lived one, standing in for the `claude` CLI that a
# real run's `--owner-pid auto` finds. It acquires from a CHILD and lets the
# DEFAULT auto walk find it by `--owner-pattern`, so this arm exercises the real
# resolution path end to end. Measured 2026-10-13: with an explicit
# `--owner-pid` here instead, breaking the walk left this arm green.
subprocess.run([sys.executable, script, 'acquire', '--lock', lock,
                '--owner-pattern', marker, '--label', 'holder run A'],
               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
open(stamp, 'w').write(str(os.getpid()))
time.sleep(600)
"""

# The ancestor the walk is supposed to find. Its MARKER is in the filename, so
# it appears in the process cmdline and `--owner-pattern` can match it -- no
# `claude` process is needed, which matters because CI has none.
OWNER_MARKER = 'e89_owner_marker_ancestor'

ANCESTOR = r"""
import os, subprocess, sys
lock, script, out = sys.argv[1], sys.argv[2], sys.argv[3]
# Acquire from a CHILD of this process, so the walk has somewhere to walk to.
proc = subprocess.run([sys.executable, script, 'acquire', '--lock', lock,
                       '--owner-pattern', sys.argv[4], '--label', 'via ancestor'],
                      capture_output=True, text=True)
open(out, 'w').write('%d\n%d\n%s' % (os.getpid(), proc.returncode, proc.stdout))
"""


def _write(path, text):
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(text)
    return path


# --- the arms ------------------------------------------------------------

def arms(tmp, racers):
    """Yield (label, callable) where the callable returns (ok, detail)."""

    def a1_free():
        lock = os.path.join(tmp, 'a1.lock')
        rc, out = run_lock(lock, 'acquire', '--label', 'arm1')
        if rc != 0:
            return False, 'acquire on a free path returned %d: %s' % (rc, out.strip())
        holder = json.load(open(lock, encoding='utf-8'))
        if holder['pid'] <= 0:
            return False, 'lock file carries no pid'
        return True, 'rc=0, lock carries pid %d and %s' % (holder['pid'], holder['acquired_utc'])

    def a2_held_by_live():
        lock = os.path.join(tmp, 'a2.lock')
        # Our OWN pid is the live holder: guaranteed alive, and os.kill(_, 0)
        # on it is guaranteed permitted.
        write_raw(lock, {'pid': os.getpid(), 'acquired': time.time(),
                         'host': 'probe', 'platform': 'linux', 'label': 'live peer'})
        rc, out = run_lock(lock, 'acquire', '--label', 'arm2')
        ok = rc == 1 and 'REFUSED' in out and str(os.getpid()) in out
        return ok, 'rc=%d; %s' % (rc, out.strip().splitlines()[0] if out.strip() else '(no output)')

    def a3_status_agrees():
        lock = os.path.join(tmp, 'a3.lock')
        write_raw(lock, {'pid': os.getpid(), 'acquired': time.time(), 'label': 'live peer'})
        held_rc, _ = run_lock(lock, 'status')
        os.unlink(lock)
        free_rc, _ = run_lock(lock, 'status')
        return (held_rc == 1 and free_rc == 0), 'status held rc=%d, free rc=%d' % (held_rc, free_rc)

    def a4_race_atomic():
        lock, winners = race(tmp, 'atomic', racers)
        return len(winners) == 1, '%d of %d racers acquired (pids %s)' % (
            len(winners), racers, winners)

    def a5_stale_dead_pid():
        lock = os.path.join(tmp, 'a5.lock')
        write_raw(lock, {'pid': DEAD_PID, 'acquired': time.time(), 'label': 'crashed run'})
        rc, out = run_lock(lock, 'acquire', '--label', 'arm5')
        ok = rc == 0 and 'reclaiming a stale lock' in out and 'is gone' in out
        return ok, 'rc=%d; %s' % (rc, out.strip().splitlines()[0] if out.strip() else '')

    def a6_stale_by_age():
        lock = os.path.join(tmp, 'a6.lock')
        # A LIVE pid -- our own -- deliberately paired with an old timestamp, so
        # the only thing that can reclaim this is the age cap. This is the arm
        # that stands in for the Windows box, where pid liveness is unknown.
        write_raw(lock, {'pid': os.getpid(), 'acquired': time.time() - 7200,
                         'label': 'live but ancient'})
        rc, out = run_lock(lock, 'acquire', '--label', 'arm6', '--max-age', '3600')
        ok = rc == 0 and 'past the 3600 s cap' in out
        return ok, 'rc=%d; %s' % (rc, out.strip().splitlines()[0] if out.strip() else '')

    def a7_live_within_age():
        # The CONTROL for arm 6: same live pid, same cap, a FRESH timestamp.
        # Without this pairing, arm 6 could be passing because the lock is
        # always reclaimable rather than because the age cap works.
        lock = os.path.join(tmp, 'a7.lock')
        write_raw(lock, {'pid': os.getpid(), 'acquired': time.time(),
                         'label': 'live and fresh'})
        rc, out = run_lock(lock, 'acquire', '--label', 'arm7', '--max-age', '3600')
        return rc == 1 and 'REFUSED' in out, 'rc=%d (same pid and cap as arm 6, fresh stamp)' % rc

    def a8_corrupt_does_not_wedge():
        lock = os.path.join(tmp, 'a8.lock')
        write_raw(lock, 'this is not json\n')
        rc, out = run_lock(lock, 'acquire', '--label', 'arm8')
        ok = rc == 0 and 'not valid JSON' in out
        return ok, 'rc=%d; %s' % (rc, out.strip().splitlines()[0] if out.strip() else '')

    def a9_release_rules():
        lock = os.path.join(tmp, 'a9.lock')
        write_raw(lock, {'pid': os.getpid(), 'acquired': time.time(), 'label': 'someone else'})
        # A subprocess has a DIFFERENT pid from the file's, so this is the
        # "releasing a lock that is not yours" case.
        refused_rc, _ = run_lock(lock, 'release')
        forced_rc, _ = run_lock(lock, 'release', '--force')
        gone = not os.path.exists(lock)
        absent_rc, _ = run_lock(lock, 'release')
        ok = refused_rc == 1 and forced_rc == 0 and gone and absent_rc == 0
        return ok, ('foreign release rc=%d, --force rc=%d, removed=%s, '
                    'release-when-absent rc=%d' % (refused_rc, forced_rc, gone, absent_rc))

    def a10_acquire_release_cycle():
        lock = os.path.join(tmp, 'a10.lock')
        first, _ = run_lock(lock, 'acquire', '--label', 'arm10a')
        # Each run_lock is its own pid, so release it by force -- the point of
        # this arm is that the PATH is reusable, not that one pid owns it.
        rel, _ = run_lock(lock, 'release', '--force')
        second, _ = run_lock(lock, 'acquire', '--label', 'arm10b')
        return (first == 0 and rel == 0 and second == 0), \
            'acquire rc=%d, release rc=%d, re-acquire rc=%d' % (first, rel, second)

    def a11_owner_is_the_ancestor():
        """--owner-pid auto must record a pid that OUTLIVES the acquire."""
        lock = os.path.join(tmp, 'a11.lock')
        out = os.path.join(tmp, 'a11.out')
        src = _write(os.path.join(tmp, OWNER_MARKER + '.py'), ANCESTOR)
        proc = subprocess.run([sys.executable, src, lock, LOCK_SCRIPT, out, OWNER_MARKER],
                              capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            return False, 'ancestor helper failed: %s' % (proc.stderr or '').strip()
        ancestor_pid, rc, acquire_out = open(out, encoding='utf-8').read().split('\n', 2)
        holder = json.load(open(lock, encoding='utf-8'))
        ok = (int(rc) == 0
              and holder['pid'] == int(ancestor_pid)
              and holder['recorded_by'] != holder['pid']
              and holder['owner_how'] == 'auto:ancestor-match')
        return ok, ('recorded pid=%s (the ancestor), recorded_by=%s (the acquire, '
                    'now exited), how=%s' % (holder['pid'], holder['recorded_by'],
                                             holder['owner_how']))

    def a12_the_583_gap():
        """#583 itself: a second run starting 5.6 s later must be refused."""
        lock = os.path.join(tmp, 'a12.lock')
        stamp = os.path.join(tmp, 'a12.stamp')
        # The marker is in the holder's FILENAME, so it is in its cmdline and
        # the acquire's ancestor walk can match it without a `claude` process.
        src = _write(os.path.join(tmp, 'e89_holder_' + OWNER_MARKER + '.py'), HOLDER)
        holder = subprocess.Popen([sys.executable, src, lock, LOCK_SCRIPT, stamp,
                                   'e89_holder_' + OWNER_MARKER],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            deadline = time.time() + 30
            while not os.path.exists(stamp) and time.time() < deadline:
                time.sleep(0.05)
            if not os.path.exists(stamp):
                return False, 'holder never acquired the lock'
            resolved = json.load(open(lock, encoding='utf-8'))
            time.sleep(5.6)                      # #583's measured gap, exactly
            refused_rc, refused_out = run_lock(lock, 'acquire', '--label', 'run B')
            held_pid = int(open(stamp, encoding='utf-8').read())
        finally:
            holder.kill()
            holder.wait(timeout=30)
        # The holder is now gone: a crashed run must NOT leave the box stranded.
        after_rc, after_out = run_lock(lock, 'acquire', '--label', 'next day')
        ok = (resolved['owner_how'] == 'auto:ancestor-match'
              and resolved['pid'] == held_pid
              and refused_rc == 1 and 'REFUSED' in refused_out
              and str(held_pid) in refused_out
              and after_rc == 0 and 'is gone' in after_out)
        return ok, ('owner resolved to the holder (%s via %s); run B at +5.6s '
                    'rc=%d; after the holder is killed rc=%d (reclaimed)'
                    % (resolved['pid'], resolved['owner_how'], refused_rc, after_rc))

    def a13_self_owner_is_the_known_hole():
        """Pin the broken configuration, so a regression says WHICH half broke.

        `--owner-pid self` records the acquire's own pid, which is dead a
        moment later -- the defect measured on 2026-10-13. Asserting it still
        behaves that way is what makes arm 12's pass attributable to the
        ancestor walk rather than to something else having changed.
        """
        lock = os.path.join(tmp, 'a13.lock')
        rc, _ = run_lock(lock, 'acquire', '--owner-pid', 'self', '--label', 'self-owned')
        holder = json.load(open(lock, encoding='utf-8'))
        status_rc, status_out = run_lock(lock, 'status')
        ok = (rc == 0 and holder['pid'] == holder['recorded_by']
              and status_rc == 0 and 'is gone' in status_out)
        return ok, ('acquire rc=%d with pid==recorded_by==%s; status now reads '
                    'stale ("%s") -- the hole, as recorded'
                    % (rc, holder['pid'],
                       status_out.strip().split(':')[0] if status_out else ''))

    return [
        ('1  free path -> acquired', a1_free),
        ('2  live holder -> REFUSED', a2_held_by_live),
        ('3  status matches acquire', a3_status_agrees),
        ('4  %d-way race -> ONE winner' % racers, a4_race_atomic),
        ('5  dead pid -> reclaimed', a5_stale_dead_pid),
        ('6  live pid past age cap -> reclaimed', a6_stale_by_age),
        ('7  live pid inside age cap -> REFUSED', a7_live_within_age),
        ('8  corrupt lock -> reclaimed, not wedged', a8_corrupt_does_not_wedge),
        ('9  release: foreign refused, --force works', a9_release_rules),
        ('10 acquire/release/re-acquire', a10_acquire_release_cycle),
        ('11 --owner-pid auto -> the ANCESTOR, not the acquire', a11_owner_is_the_ancestor),
        ('12 #583: live holder, +5.6s -> REFUSED; killed -> reclaimed', a12_the_583_gap),
        ('13 --owner-pid self -> stale on exit (the pinned hole)', a13_self_owner_is_the_known_hole),
    ]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--verbose', action='store_true')
    ap.add_argument('--racers', type=int, default=12,
                    help='processes in the race arms (default 12)')
    args = ap.parse_args(argv)

    if not os.path.exists(LOCK_SCRIPT):
        print('cannot find %s' % LOCK_SCRIPT)
        return 1

    failures = 0
    with tempfile.TemporaryDirectory(prefix='e89-lock-') as tmp:
        # --- the vacuity control, FIRST -------------------------------------
        print('CONTROL (must FAIL to be worth anything): the same %d-way harness '
              'against a non-atomic check-then-create acquire' % args.racers)
        _, ctl_winners = race(tmp, 'unsafe', args.racers)
        ctl_ok = len(ctl_winners) > 1
        print('  %d of %d racers "acquired" the non-atomic lock -- %s\n'
              % (len(ctl_winners), args.racers,
                 'the harness can see a broken lock, so arm 4 means something'
                 if ctl_ok else
                 'THE HARNESS CANNOT SEE A BROKEN LOCK. Arm 4 proves nothing.'))
        if not ctl_ok:
            failures += 1

        rows = []
        for label, fn in arms(tmp, args.racers):
            ok, detail = fn()
            if not ok:
                failures += 1
            rows.append((label, 'ok' if ok else 'UNEXPECTED', detail))
            if args.verbose or not ok:
                print('--- %s\n    %s' % (label, detail))

    width = max(len(r[0]) for r in rows)
    print('| arm | verdict | measured |')
    print('|%s|---|---|' % ('-' * (width + 2)))
    for label, verdict, detail in rows:
        print('| %-*s | %s | %s |' % (width, label, verdict, detail))
    print('\ncontrol: %d winners (want >1) | arms: %d, failures: %d'
          % (len(ctl_winners), len(rows), failures - (0 if ctl_ok else 1)))

    if failures:
        print(textwrap.dedent('''
            %d arm(s) did not behave as E89 records. Do NOT adopt the STEP 0.5
            lock on this result -- a lock that is wrong in direction (2) strands
            the box, which #583 calls the worse outcome.''' % failures).strip())
        return 1
    print('\nEvery arm holds. The lock excludes a concurrent run and cannot be '
          'wedged by a dead pid, an ancient stamp or a corrupt file.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
