#!/usr/bin/env bash
# E19 linux VST3 cell -- drive REAPER headlessly and capture everything.
#
#   run-host.sh <lua> <tag> [window-seconds]
#
# Two traps, both from docs/ci/headless-gui-verification.md, both load-bearing:
#   * GDK_BACKEND=x11 with WAYLAND_DISPLAY UNSET, or REAPER segfaults in libX11
#     while the plug-in's editor opens -- which looks exactly like a plug-in bug.
#   * linux_audio_mode=2 (Dummy Audio) in reaper.ini, or an undismissable
#     "Error opening devices" modal stops everything.
set -uo pipefail

S="${E19_SCRATCH:?set E19_SCRATCH to a scratch directory holding reaper_linux_x86_64/ and home/}"
LUA="$1"; TAG="$2"; WINDOW="${3:-75}"
REAPER_BIN="$S/reaper_linux_x86_64/REAPER/reaper"

export E19_SCRATCH="$S"
export E19_WINDOW="$WINDOW"

cp "$LUA" "$S/home/.config/REAPER/Scripts/__startup.lua"

# The compositor. Its own socket, so nothing reaches the developer's session.
if ! pgrep -x weston >/dev/null; then
  weston --backend=headless --socket=tide-e19 --width=1600 --height=1000 --xwayland \
      > "$S/weston-$TAG.log" 2>&1 &
  for i in $(seq 1 40); do
    grep -q "xserver listening on display" "$S/weston-$TAG.log" && break
    read -r -t 0.5 < /dev/zero 2>/dev/null || true
  done
fi
DISP=$(grep -o "xserver listening on display :[0-9]*" "$S"/weston-*.log | tail -1 | grep -o ":[0-9]*")
echo "weston X display $DISP"

env -u WAYLAND_DISPLAY \
    HOME="$S/home" \
    DISPLAY="$DISP" \
    GDK_BACKEND=x11 \
    XDG_RUNTIME_DIR="/run/user/$(id -u)" \
    E19_SCRATCH="$S" \
    E19_WINDOW="$WINDOW" \
    E19_PROJ="${E19_PROJ:-$S/proj/e19.rpp}" \
    "$REAPER_BIN" -nosplash \
    > "$S/reaper-$TAG.out" 2> "$S/reaper-$TAG.err" &
RPID=$!
echo "reaper pid $RPID display $DISP"
echo "$DISP" > "$S/display"
echo "$RPID" > "$S/reaper.pid"
