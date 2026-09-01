#!/usr/bin/env bash
# E19 windows VST3 cell -- drive REAPER 7.78 on Windows and capture everything.
#
#   run-host-win.sh <lua> <tag> <sentinel-log> <timeout-s> [window-s]
#
# The Windows twin of run-host.sh, and it is a DIFFERENT shape rather than the
# same script with a path swapped. Three reasons, each measured 2026-09-02:
#
#   * REAPER runs a Lua file named on the COMMAND LINE, so nothing has to be
#     installed as Scripts/__startup.lua. That matters more here than on linux:
#     the resource path is %APPDATA%\REAPER and cannot be redirected (REAPER
#     resolves it with SHGetKnownFolderPath, which ignores the environment), so
#     every file this harness does NOT write is one it cannot get wrong.
#   * Passing an explicit empty .rpp stops REAPER reopening the developer's last
#     project, whose missing plug-ins raise a modal that blocks the script before
#     it runs. Same trap the macOS AU3 leg hit through loadlastproj.
#   * The script never quits REAPER; this kills it. See prepare.lua's bail().
#
# THE HOST IS NOT ISOLATABLE ON THIS PLATFORM and that is a measured Windows
# fact, not a fleet one -- portable mode does not engage and %APPDATA% cannot be
# redirected. So back up the resource directory BEFORE the first launch and
# restore it afterwards; this script deliberately does not do that for you,
# because a backup taken by the thing that also breaks it is not a backup.

set -uo pipefail

S="${E19_SCRATCH:?set E19_SCRATCH to a scratch directory (Windows path form)}"
LUA="$1"; TAG="$2"; SENT="$3"; TIMEOUT="$4"; WINDOW="${5:-75}"
REAPER_BIN="${E19_REAPER:-/c/Program Files/REAPER (x64)/reaper.exe}"

# A unix-form copy of the scratch dir, for this script's own file handling. The
# Lua and REAPER get the Windows form in $S, which is what they understand.
SU=$(cygpath -u "$S" 2>/dev/null || echo "$S")

[ -f "$SU/empty.rpp" ] || printf '<REAPER_PROJECT 0.1 "7.78" 0\n>\n' > "$SU/empty.rpp"
rm -f "$SENT"

# Copy the script in, so the caller passes a repo path and REAPER is handed a
# Windows one -- the same split run-host.sh makes when it installs __startup.lua.
cp "$LUA" "$SU/_run.lua"

E19_SCRATCH="$S" \
E19_PROJ="${E19_PROJ:-$S\\proj\\e19.rpp}" \
E19_WINDOW="$WINDOW" \
  "$REAPER_BIN" -nosplash "$S\\empty.rpp" "$S\\_run.lua" \
  > "$SU/reaper-$TAG.out" 2> "$SU/reaper-$TAG.err" &

# sleep, NOT `read -t N < /dev/zero`. That idiom is a working sleep on linux and
# returns INSTANTLY in Git Bash, because /dev/zero always has a byte to read --
# so a 180-iteration wait completed in milliseconds and killed REAPER about a
# second after launch, before it could write anything. Three runs were read as
# "the host never started" before the harness was suspected. 2026-09-02.
for i in $(seq 1 "$TIMEOUT"); do
  if grep -q '^done$' "$SENT" 2>/dev/null; then echo "sentinel after ${i}s"; break; fi
  sleep 1
done
grep -q '^done$' "$SENT" 2>/dev/null || echo "NO SENTINEL after ${TIMEOUT}s -- check $SU/reaper-$TAG.err"

powershell -NoProfile -Command "Get-Process reaper -ErrorAction SilentlyContinue | Stop-Process -Force" 2>/dev/null
sleep 2
echo "reaper left running: $(powershell -NoProfile -Command '(@(Get-Process reaper -ErrorAction SilentlyContinue)).Count' 2>&1 | tr -d '\r')"
echo "stderr bytes: $(wc -c < "$SU/reaper-$TAG.err")"
