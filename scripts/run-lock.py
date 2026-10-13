#!/usr/bin/env python3
"""A BOX-scoped mutual-exclusion lock for scheduled runs.

BACKLOG E89, triaging issue #583. On 2026-09-08 the `tidesynth-weekly-linux`
scheduled task **fired twice, 5.6 seconds apart**, and both runs went to work in
the same set of working trees. Every existing guard passed and would pass again:
STEP 0.7 asserted the bot's identity correctly in both processes,
`check-commit-authorship.py` was clean because the commit really was the bot's,
and STEP 2's DOING mark did exactly what it was written to do. **The DOING mark
is a claim against the other MACHINES; it is not a claim against yourself.** Two
processes sharing one `.git` are not three machines that cannot see each other,
and a pushed claim there is just a checkout of the other process's HEAD.

Why a lock file and not `ps`
----------------------------
#583 suggested the cheap check: *"`ps` for a second `claude` with the same
task"*. Measured on the linux box 2026-10-13, that does not work, in both
directions at once:

  - **There is no "same task" to match on.** No `claude` process's cmdline
    contains `tidesynth-weekly-linux`; the task name reaches the run through its
    prompt, not its argv. Swept `/proc/*/cmdline` across the whole system: one
    match, and it was the sweep's own shell.
  - **The count-based variant FALSE-POSITIVES, and it would have stood down the
    run that measured it.** Two `claude` CLI processes were alive -- pid 4270
    (`--model default`, the run) and pid 4414 (`--model claude-haiku-4-5`), a
    sibling helper started 21 s later by the same `claude-desktop` parent. A run
    that stands down on "another claude exists" stands down always.

An explicit lock file says what `ps` can only guess at, and it says it about
runs rather than about processes.

Why not a `.git`-aware DOING check
----------------------------------
E89's second candidate shape was *"making the DOING check compare against the
local `.git` as well as the remote"*. That is a late detector, not a preventer,
and in the observed incident it is **blind by construction**: run B started at
22:03:07.240Z and run A's DOING commit (`9177984`) is authored
2026-09-08T22:05:50Z -- **169 seconds later.** Run B did its STEP 2
claim-collision survey inside that window, so there was nothing in the shared
`.git` for such a check to see. It is worth having as a backstop; it cannot be
the guard.

The lock closes the window at STEP 0.5 instead, before either run has created a
branch, fetched a sibling repo, or written a byte.

What it must never do
---------------------
**Strand the box.** A stale lock that silently stops the lane is worse than the
collision it prevents -- #583 says so in as many words. So:

  - the lock carries the holder's pid and a UTC timestamp, and
  - a lock whose pid is **known** dead is not a lock, and
  - a lock older than `--max-age` (default 6 h) is not a lock either, whatever
    its pid says, because pids are recycled and a run is not meant to last that
    long.

Pid liveness is `os.kill(pid, 0)` on POSIX. **On Windows this script does not
determine liveness at all** -- it reports it as unknown, treats unknown as
*possibly alive* (so it never steals a lock it cannot prove is dead), and falls
back to the age cap alone. That makes the Windows box strictly more
conservative, never less: the worst case there is a crashed run blocking the
lane until the age cap expires, where linux and macOS reclaim in seconds.

Whose pid goes in the lock, and why it is not this script's
-----------------------------------------------------------
**This was measured wrong first, and the measurement is the reason the script
has an owner-pid concept at all.** `acquire` runs as a short-lived python
process: record its own pid and the lock is stale the instant the process exits,
so the reclaim rule eats the lock a moment after it is taken. Reproduced on the
linux box 2026-10-13 at #583's exact gap -- run A acquired, and run B 5.6 s later
was told `reclaiming a stale lock (holder pid 7047 is gone)` and let straight
through. **The guard would have done nothing, silently**, which is the one
outcome worse than not having it.

So `acquire` records an OWNER pid, which must be a process that lives as long as
the run does. `--owner-pid auto` (the default) walks the parent chain from this
process outward and takes the first ancestor whose cmdline matches
`--owner-pattern` (default: the Claude Code CLI). It prints which pid it chose
and how it resolved it, every time, because a silent fallback here is the whole
failure mode:

    lock acquired: ... owner=4270 via auto:ancestor-match
    lock acquired: ... owner=7312 via self (NO agent ancestor matched -- this
                                      lock is age-capped only)

The walk uses `/proc` where it exists and `ps -o ppid=,args=` otherwise, so it
works on linux and macOS. **On Windows neither is available and resolution
always falls back to `self`**, which combines with the unknown pid liveness
described above to make the Windows lock purely age-capped. That is conservative
in the safe direction -- it never releases early -- and it is the one platform
difference in this script.

Usage
-----
    python3 scripts/run-lock.py acquire --label "linux scheduled run"
    python3 scripts/run-lock.py status
    python3 scripts/run-lock.py release

Exit codes, which are the whole interface:

    acquire   0 = the lock is yours      1 = another live run holds it
    release   0 = released (or was not held by you and --force was given)
              1 = held by another pid and --force was not given
    status    0 = free or stale          1 = held by a live run
    any       2 = misuse (bad arguments, unwritable lock directory)

`acquire` prints one line on success naming any stale lock it reclaimed, and on
refusal prints the holder's pid, age, label and machine. A refused run has
nothing to say and should say nothing: writing a journal entry costs exactly the
merge conflict the lock exists to prevent.
"""

import argparse
import errno
import json
import os
import re
import socket
import subprocess
import sys
import time

DEFAULT_LOCK = os.path.join(os.path.expanduser('~'), '.tide', 'run.lock')

# Six hours. A scheduled run is minutes to a couple of hours; nothing in the
# journal records one past three. Short enough that a crash on the Windows box
# (no pid liveness, see the module docstring) costs at most one run, long enough
# that a slow build is never mistaken for a corpse.
DEFAULT_MAX_AGE = 6 * 3600

# The Claude Code CLI, which is the process that lives for exactly as long as a
# run. Matched against the whole cmdline, so it catches both the versioned
# install path and a bare `claude` on PATH.
DEFAULT_OWNER_PATTERN = r'claude-code[/\\].*[/\\]claude|[/\\]claude(\s|$)'

MAX_WALK = 40          # a process tree deeper than this is a loop, not a tree


def _parent_and_cmdline(pid):
    """(ppid, cmdline) for pid, or (None, None) if it cannot be determined.

    /proc first because it needs no subprocess and cannot be confused by a
    cmdline containing newlines; `ps` second for macOS. Windows has neither, and
    the caller treats (None, None) as "stop walking".
    """
    stat = '/proc/%d/stat' % pid
    if os.path.exists(stat):
        try:
            with open(stat, 'r', encoding='utf-8', errors='replace') as fh:
                data = fh.read()
            # comm can contain spaces and parentheses, so split on the LAST ')'.
            after = data[data.rindex(')') + 1:].split()
            ppid = int(after[1])
            with open('/proc/%d/cmdline' % pid, 'rb') as fh:
                cmdline = fh.read().replace(b'\0', b' ').decode('utf-8', 'replace').strip()
            return ppid, cmdline
        except (OSError, ValueError, IndexError):
            return None, None
    if os.name != 'posix':
        return None, None
    try:
        out = subprocess.run(['ps', '-o', 'ppid=,args=', '-p', str(pid)],
                             capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        return None, None
    line = (out.stdout or '').strip()
    if not line:
        return None, None
    parts = line.split(None, 1)
    try:
        return int(parts[0]), (parts[1] if len(parts) > 1 else '')
    except ValueError:
        return None, None


def resolve_owner(spec, pattern):
    """Return (pid, how). `spec` is 'auto', 'self', or a pid as a string."""
    if spec == 'self':
        return os.getpid(), 'self (requested)'
    if spec != 'auto':
        try:
            pid = int(spec)
        except ValueError:
            return None, 'not a pid: %r' % spec
        if pid <= 0:
            return None, 'not a pid: %r' % spec
        return pid, 'explicit'
    try:
        rx = re.compile(pattern)
    except re.error as exc:
        return None, 'bad --owner-pattern (%s)' % exc
    pid = os.getpid()
    for _ in range(MAX_WALK):
        ppid, cmdline = _parent_and_cmdline(pid)
        if ppid is None or ppid <= 1 or ppid == pid:
            break
        pid = ppid
        if cmdline is None:
            continue
        _, parent_cmdline = _parent_and_cmdline(pid)
        if parent_cmdline and rx.search(parent_cmdline):
            return pid, 'auto:ancestor-match'
    return os.getpid(), ('self (NO agent ancestor matched -- this lock is '
                         'age-capped only)')


def pid_ownership_available():
    """Can a lock's owner be identified by PID on this platform?

    `resolve_owner('auto')` needs a parent chain to find the long-lived agent
    process. Without one it falls back to the short-lived `acquire`'s own pid,
    which exits on return -- so no later `release` process can ever match it.
    Windows has neither /proc nor `ps` (see `_parent_and_cmdline`, which
    returns (None, None) there without trying), so the answer is no.

    Measured on the windows box 2026-10-13: acquire recorded
    pid == recorded_by == 33336, and `release` resolved to 33532 and refused,
    leaving EVERY run holding its lock until the age cap -- not just crashed
    ones, which is what the prompt had claimed.
    """
    return os.name == 'posix'


def pid_alive(pid):
    """True / False / None, where None means "this platform cannot tell".

    None is deliberately NOT False. A lock is only ever reclaimed on a KNOWN
    dead pid, so an unknown answer leaves the age cap as the sole reclaim route
    and the lock is never stolen from a process that might be running.
    """
    if pid is None or pid <= 0:
        return False
    if os.name != 'posix':
        return None
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        # Alive, owned by somebody else. Not ours to reclaim.
        return True
    except OSError as exc:                                   # pragma: no cover
        if exc.errno == errno.ESRCH:
            return False
        return True
    return True


def read_holder(path):
    """Parse the lock file, or None if it is absent.

    A lock file that exists but cannot be parsed is reported as a holder with no
    pid, which `staleness` then treats as dead -- an unreadable lock must not be
    able to wedge the lane either.
    """
    try:
        with open(path, 'r', encoding='utf-8') as fh:
            raw = fh.read()
    except FileNotFoundError:
        return None
    except OSError as exc:                                   # pragma: no cover
        return {'pid': None, 'unreadable': str(exc), 'acquired': 0.0}
    try:
        holder = json.loads(raw)
        if not isinstance(holder, dict):
            raise ValueError('not an object')
    except (ValueError, TypeError):
        return {'pid': None, 'corrupt': raw[:200], 'acquired': 0.0}
    holder.setdefault('pid', None)
    holder.setdefault('acquired', 0.0)
    return holder


def staleness(holder, max_age, now=None):
    """Return None if the holder is live, else the reason it is reclaimable."""
    now = time.time() if now is None else now
    pid = holder.get('pid')
    if 'corrupt' in holder:
        return 'lock file is not valid JSON'
    if 'unreadable' in holder:
        return 'lock file could not be read (%s)' % holder['unreadable']
    alive = pid_alive(pid)
    if alive is False:
        return 'holder pid %s is gone' % pid
    try:
        age = now - float(holder.get('acquired') or 0.0)
    except (TypeError, ValueError):
        age = max_age + 1
    if age > max_age:
        return 'lock is %.0f s old, past the %d s cap' % (age, max_age)
    if alive is None:
        return None     # unknown pid liveness, inside the age cap: assume live
    return None


def describe(holder, max_age, now=None):
    now = time.time() if now is None else now
    try:
        age = now - float(holder.get('acquired') or 0.0)
    except (TypeError, ValueError):
        age = -1.0
    return ('pid=%s host=%s platform=%s label=%r age=%.0fs repo=%s owner=%s'
            % (holder.get('pid'), holder.get('host'), holder.get('platform'),
               holder.get('label'), age, holder.get('repo'),
               holder.get('owner_how') or 'unrecorded'))


def write_lock(path, label, platform, repo, owner_pid=None, owner_how='self'):
    """Create the lock atomically, or raise FileExistsError.

    O_CREAT|O_EXCL rather than flock: it is the one primitive that is atomic and
    identical on all three boxes, and the lock's VALUE has to survive being read
    by a different process anyway, which an advisory flock does not give us.
    """
    payload = {
        'pid': os.getpid() if owner_pid is None else owner_pid,
        'recorded_by': os.getpid(),
        'owner_how': owner_how,
        'acquired': time.time(),
        'acquired_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'host': socket.gethostname(),
        'platform': platform,
        'label': label,
        'repo': repo,
    }
    fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        os.write(fd, (json.dumps(payload, indent=2) + '\n').encode('utf-8'))
    finally:
        os.close(fd)
    return payload


def ensure_dir(path):
    parent = os.path.dirname(os.path.abspath(path))
    try:
        os.makedirs(parent, exist_ok=True)
    except OSError as exc:
        print('cannot create %s for the lock: %s' % (parent, exc))
        return False
    return True


def cmd_acquire(args):
    if not ensure_dir(args.lock):
        return 2
    owner_pid, owner_how = resolve_owner(args.owner_pid, args.owner_pattern)
    if owner_pid is None:
        print('cannot resolve an owner pid: %s' % owner_how)
        return 2
    if owner_how.startswith('self (NO agent'):
        print('WARNING: %s' % owner_how)
    # Two passes at most: if the first O_EXCL loses to an existing file we judge
    # that file, and only a KNOWN-stale one earns a second attempt. A third pass
    # would be a retry loop, and racing a live peer is the one thing this must
    # not do.
    for attempt in (1, 2):
        try:
            payload = write_lock(args.lock, args.label, args.platform, args.repo,
                                 owner_pid, owner_how)
        except FileExistsError:
            pass
        else:
            print('lock acquired: %s' % describe(payload, args.max_age))
            if owner_pid != os.getpid():
                print('owner pid %d resolved via %s; this process (%d) exits now '
                      'and the lock outlives it.' % (owner_pid, owner_how, os.getpid()))
            return 0
        holder = read_holder(args.lock)
        if holder is None:
            continue            # released between our O_EXCL and our read
        reason = staleness(holder, args.max_age)
        if reason is None:
            print('REFUSED: another run holds the box lock -- %s'
                  % describe(holder, args.max_age))
            print('reason: live holder; this run must stop now and write nothing.')
            return 1
        if attempt == 2:
            print('REFUSED: lock at %s keeps reappearing; a peer is racing us.'
                  % args.lock)
            return 1
        print('reclaiming a stale lock (%s) -- previous holder %s'
              % (reason, describe(holder, args.max_age)))
        try:
            os.unlink(args.lock)
        except FileNotFoundError:
            pass
        except OSError as exc:                               # pragma: no cover
            print('cannot remove the stale lock at %s: %s' % (args.lock, exc))
            return 2
    return 1                                                 # pragma: no cover


def cmd_release(args):
    holder = read_holder(args.lock)
    if holder is None:
        print('no lock at %s -- nothing to release' % args.lock)
        return 0
    owner_pid, owner_how = resolve_owner(args.owner_pid, args.owner_pattern)
    mine = owner_pid is not None and holder.get('pid') == owner_pid
    how = 'ours'
    if not mine and args.owner_pid == 'auto' and not pid_ownership_available():
        # The recorded pid belongs to the `acquire` process, which exited on
        # return, so no release can match it here. Fall back to the identity
        # the lock already carries and that this lock is SCOPED to anyway --
        # the box. Safe in the prescribed flow because only a run that
        # ACQUIRED ever reaches STEP 5: a run refused at STEP 0.5 is told to
        # stop and write nothing, and never releases.
        #
        # What this gives up, stated plainly: on such a platform any run on
        # this box could release another's lock without --force. Arm 14 pins
        # both halves so the trade stays deliberate.
        if holder.get('host') and holder.get('host') == socket.gethostname():
            mine = True
            how = 'same box (pid ownership unavailable on this platform)'
    if not mine and not args.force:
        print('REFUSED: the lock at %s is held by %s, not by this run '
              '(owner resolves to %s via %s). Pass --force only if you know '
              'that run is gone.'
              % (args.lock, describe(holder, args.max_age), owner_pid, owner_how))
        return 1
    try:
        os.unlink(args.lock)
    except FileNotFoundError:
        pass
    print('lock released (%s)' % (how if mine else 'forced'))
    return 0


def cmd_status(args):
    holder = read_holder(args.lock)
    if holder is None:
        print('free: no lock at %s' % args.lock)
        return 0
    reason = staleness(holder, args.max_age)
    if reason is None:
        print('HELD: %s' % describe(holder, args.max_age))
        return 1
    print('stale (%s): %s' % (reason, describe(holder, args.max_age)))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('action', choices=('acquire', 'release', 'status'))
    ap.add_argument('--lock', default=os.environ.get('TIDE_RUN_LOCK', DEFAULT_LOCK),
                    help='lock path (default %s, or $TIDE_RUN_LOCK)' % DEFAULT_LOCK)
    ap.add_argument('--label', default='', help='what is holding it, for the next reader')
    ap.add_argument('--platform', default='', help='win / mac / linux')
    ap.add_argument('--repo', default=os.getcwd(), help='the repo this run is working in')
    ap.add_argument('--max-age', type=int, default=DEFAULT_MAX_AGE,
                    help='seconds after which a lock is stale whatever its pid (default %d)'
                         % DEFAULT_MAX_AGE)
    ap.add_argument('--owner-pid', default='auto',
                    help="whose pid the lock records: 'auto' (walk out to the "
                         "agent process, the default), 'self', or a pid")
    ap.add_argument('--owner-pattern', default=DEFAULT_OWNER_PATTERN,
                    help='cmdline regex identifying the agent process for --owner-pid auto')
    ap.add_argument('--force', action='store_true',
                    help='release: remove a lock held by another pid')
    args = ap.parse_args(argv)
    if args.max_age <= 0:
        print('--max-age must be positive')
        return 2
    return {'acquire': cmd_acquire, 'release': cmd_release, 'status': cmd_status}[args.action](args)


if __name__ == '__main__':
    sys.exit(main())
