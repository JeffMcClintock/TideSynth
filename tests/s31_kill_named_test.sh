#!/bin/sh
# BACKLOG S31 -- prove scripts/kill-named.sh does the two things its row asks
# for, and prove the ancestor filter itself works even on a platform where the
# OS makes it redundant.
#
#     sh tests/s31_kill_named_test.sh
#
# Exit 0 if every case passes, 1 otherwise. No arguments, no network, no build.

set -u
# Job control prints "Terminated: 15" when a backgrounded probe is killed --
# which is the expected outcome of every case here. Silence it so a real
# failure is the only thing that stands out.
# Probes are started in a detached subshell rather than with a bare `&`, so
# this shell is not their job-control parent and does not print "Terminated: 15"
# every time a case succeeds. Noisy output that always appears is output nobody
# reads.
start_probe() {
    ( "$probe" >/dev/null 2>&1 & )
    sleep 0.4
    pgrep -f "$pat" | head -1
}
here=$(cd "$(dirname "$0")/.." && pwd)
kn="$here/scripts/kill-named.sh"
pass=0
fail=0

ok()   { echo "PASS  $1"; pass=$((pass + 1)); }
bad()  { echo "FAIL  $1"; fail=$((fail + 1)); }

pat="s31test-$$"
probe="/tmp/$pat.sh"
printf '#!/bin/sh\nwhile :; do sleep 1; done\n' > "$probe"
chmod +x "$probe"

cleanup() { rm -f "$probe"; }
trap cleanup EXIT

# ---------------------------------------------------------------- case 1
# It kills what it was asked to kill. Without this the rest is meaningless --
# a script that kills nothing trivially "spares the shell".
target=$(start_probe)
"$kn" "$pat" > /dev/null 2>&1
sleep 0.4
if kill -0 "$target" 2>/dev/null; then
    bad "kills the named process"
    kill "$target" 2>/dev/null
else
    ok "kills the named process"
fi

# ---------------------------------------------------------------- case 2
# The row's actual Accept: run it from a shell whose OWN command line contains
# the pattern, and that shell must live. A self-kill shows up as exit 143/144.
target=$(start_probe)
out=$(sh -c "$kn $pat >/dev/null 2>&1; echo ALIVE" 2>/dev/null)
rc=$?
sleep 0.4
kill "$target" 2>/dev/null
if [ "$out" = "ALIVE" ] && [ "$rc" -eq 0 ]; then
    ok "the calling shell survives (rc=$rc)"
else
    bad "the calling shell survives -- got out='$out' rc=$rc"
fi

# ---------------------------------------------------------------- case 3
# The filter itself, asserted directly rather than via the OS.
#
# This is the case that matters on macOS, where BSD pkill already excludes
# ancestors so cases 1 and 2 pass without the filter ever running. Here we ask
# the script what it considers an ancestor and check the answer against pids we
# know the truth about.
ancestors=$("$kn" --print-ancestors)

# This shell IS an ancestor of the script it just ran.
if echo "$ancestors" | grep -qx "$$"; then
    ok "ancestor list contains the calling shell ($$)"
else
    bad "ancestor list is missing the calling shell ($$) -- got: $(echo $ancestors | tr '\n' ' ')"
fi

# So is this shell's own parent.
myparent=$(ps -o ppid= -p $$ | tr -d ' ')
if [ -n "$myparent" ] && [ "$myparent" -gt 1 ] 2>/dev/null; then
    if echo "$ancestors" | grep -qx "$myparent"; then
        ok "ancestor list contains the grandparent ($myparent)"
    else
        bad "ancestor list is missing the grandparent ($myparent)"
    fi
fi

# An unrelated process must NOT be in the list, or the filter would spare
# everything and the script would kill nothing.
unrelated=$(start_probe)
if echo "$ancestors" | grep -qx "$unrelated"; then
    bad "ancestor list wrongly contains an unrelated pid ($unrelated)"
else
    ok "ancestor list excludes an unrelated pid ($unrelated)"
fi
kill "$unrelated" 2>/dev/null

# ---------------------------------------------------------------- case 4
# Nothing to kill is success, not failure -- a tidy-up step runs whether or not
# the thing it tidies is there.
"$kn" "s31test-nothing-matches-this-$$" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    ok "no match is exit 0"
else
    bad "no match should be exit 0"
fi

# ---------------------------------------------------------------- case 5
"$kn" > /dev/null 2>&1
if [ $? -eq 2 ]; then
    ok "no argument is a usage error (exit 2)"
else
    bad "no argument should exit 2"
fi

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
