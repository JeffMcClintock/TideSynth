#!/bin/sh
# BACKLOG S31 -- terminate a named child process WITHOUT killing the shell that
# is asking for it.
#
# The trap this exists to remove:
#
#     pkill -f 'drive.py 3000'
#
# `pkill -f` matches against the FULL command line of every process. The shell
# running that command has the pattern in its own command line, verbatim, so it
# matches itself and dies. The fleet has hit this three times -- 2026-08-20
# (linux, twice in one session) and 2026-08-21 (linux, exit 144) -- despite the
# lesson being written down in two journals and in
# docs/ci/headless-gui-verification.md. A rule that costs a session every time
# it is forgotten wants a script, not a fourth retelling.
#
# Usage:
#     scripts/kill-named.sh <pattern> [signal]
#
#     scripts/kill-named.sh 'drive.py 3000'      # TERM, the default
#     scripts/kill-named.sh westond KILL
#
# Exit status: 0 if anything was signalled OR there was nothing to kill (both
# are success for a tidy-up step), 2 on a usage error. Never returns the
# 143/144 that a self-kill produces, because it cannot kill itself.
#
# Portable between macOS and Linux on purpose: the fleet has three boxes and
# /proc is Linux-only, so the ancestor walk uses `ps`, which both have.

set -u

pattern=${1:-}
signal=${2:-TERM}

if [ -z "$pattern" ]; then
    echo "usage: $0 <pattern> [signal]" >&2
    echo "  kills processes whose command line contains <pattern>," >&2
    echo "  except this script, its shell, and every ancestor." >&2
    exit 2
fi

# Every pid from here up to init. These are the ones `pkill -f` would have
# killed along with the target: this script, the shell that invoked it, that
# shell's parent, and so on -- each of which carries the pattern on its own
# command line because it is somewhere in the invocation.
ancestors=""
p=$$
while [ -n "$p" ] && [ "$p" -gt 1 ] 2>/dev/null; do
    ancestors="$ancestors $p"
    p=$(ps -o ppid= -p "$p" 2>/dev/null | tr -d ' ')
done

is_ancestor() {
    for a in $ancestors; do
        [ "$a" = "$1" ] && return 0
    done
    return 1
}

# The ancestor list is the load-bearing part of this script, and on macOS it
# never runs: BSD pkill/pgrep already exclude ancestors by default (`man pkill`,
# the -a flag), so the filter below finds nothing to spare. That is exactly the
# condition under which a bug in it would go unnoticed until Linux -- where GNU
# procps excludes only the calling process -- hits it for real. So expose the
# list and let tests/s31_kill_named_test.sh assert it directly, on every
# platform, rather than only where the OS happens to exercise it.
if [ "$pattern" = "--print-ancestors" ]; then
    for a in $ancestors; do echo "$a"; done
    exit 0
fi

killed=0
spared=0

# `pgrep -f` does the matching; the filtering is ours. Sorted numerically so the
# output is stable and diffable between runs.
for pid in $(pgrep -f -- "$pattern" 2>/dev/null | sort -n); do
    if is_ancestor "$pid"; then
        # This is the whole point of the script. Say so out loud -- a silent
        # skip here looks identical to "there was nothing to kill", and the
        # next person debugging a tidy-up needs to see which is which.
        cmd=$(ps -o command= -p "$pid" 2>/dev/null | cut -c1-70)
        echo "  spared (self/ancestor) pid $pid  $cmd"
        spared=$((spared + 1))
        continue
    fi
    if kill "-$signal" "$pid" 2>/dev/null; then
        echo "  killed pid $pid with SIG$signal"
        killed=$((killed + 1))
    else
        # Already gone, or not ours. Not an error for a tidy-up step.
        echo "  could not signal pid $pid (already exited, or not ours)"
    fi
done

echo "kill-named: $killed killed, $spared spared, pattern '$pattern'"
exit 0
