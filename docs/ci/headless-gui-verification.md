# Driving TIDE-Rack (the standalone) without risking the developer's desktop

**Linux only.** Written 2026-08-21 for **BACKLOG S32**, after `gnome-shell`
segfaulted four times in two days on the linux box and took the developer's
session to the login screen each time — twice while a scheduled run was working.

## Why this exists

The crash is in **mutter's Wayland client-teardown path**, not in TIDE:

```
wl_display_flush_clients -> wl_client_destroy -> libmutter-14
  -> g_signal_handler_disconnect -> g_type_check_instance   SIGSEGV
```

It is a use-after-free in `gnome-shell 46.0-0ubuntu6~24.04.14` while destroying a
disconnecting client. **Any** client exiting can plausibly trigger it — one of
the four crashes happened with TIDE not running at all — so this is not
something TIDE can fix, and not something a run should keep gambling on.

The fix is to stop being a client of the developer's compositor.

## The recipe

Weston's **headless** backend needs no display, no seat and no GPU, and nothing
it does reaches `gnome-shell`. Install once:

```bash
sudo apt install --no-install-recommends weston
```

Start a compositor on its own socket, and run against it:

```bash
weston --backend=headless --socket=tide-test --width=1400 --height=900 &

XDG_RUNTIME_DIR=/run/user/$(id -u) \
WAYLAND_DISPLAY=tide-test \
XDG_CONFIG_HOME=<scratch> \
  ./TIDE-Rack
```

Stop it with the helper, which cannot kill the shell that runs it:

```bash
scripts/kill-named.sh TIDE-Rack
scripts/kill-named.sh weston
```

**Why not `pkill -f`** (BACKLOG **S31**, hit three times): `-f` matches the full
command line, and the shell running the command has the pattern in *its* command
line, so it matches itself and dies — exit 144.

**This is a Linux-only trap, which is why it kept surprising people.** Measured
2026-08-22: BSD `pkill` (macOS) excludes the calling process *and all of its
ancestors* by default — `man pkill`, the `-a` flag — so on a Mac the same
command is already safe and the bug is invisible. GNU procps (Linux) excludes
only the calling process itself. A mac box cannot reproduce this no matter how
carefully it tries, so "it worked when I tested it" was true and useless.

`scripts/kill-named.sh` walks the ancestor chain itself and spares it explicitly,
so it behaves the same on both. The manual form still works if you prefer it —
note it matches the process *name*, not the command line, which is what makes it
safe:

```bash
for p in $(pgrep -x TIDE-Rack); do kill "$p"; done
for p in $(pgrep -x weston);    do kill "$p"; done
```

## What you keep, and what you give up

**You keep everything that matters for verification.** The MCP command channel
works unchanged — the app prints `command channel: …` as usual, and
`gmpi_screenshot` renders from the app's **own** buffer rather than asking the
compositor, so screenshots, clicks and drags all work with no visible window.
Measured 2026-08-21: the rack drew, the module browser listed every group, and a
1100x626 screenshot came back correct.

**You give up watching it happen.** Nothing appears on screen. If you want to
see it, `weston --width=1400 --height=900` (no `--backend`) opens a nested window
inside the current session instead — but note that is only **partial** isolation:
the nested compositor is itself a mutter client, so when it exits, mutter still
runs the teardown path that crashes.

**Always set `XDG_CONFIG_HOME` to a scratch directory.** The standalone honours
it (`StandaloneSettings.cpp:49`), and it is what keeps a run's experiments out of
`~/.config/TIDE Rack/`, where the developer's own session state lives.

## Known cosmetic noise

Under headless weston the app logs:

```
Gdk-CRITICAL **: gdk_seat_get_keyboard: assertion 'GDK_IS_SEAT (seat)' failed
```

Harmless — the headless backend advertises no keyboard. It does not affect
rendering, audio, or the command channel.
