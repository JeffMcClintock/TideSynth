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

## Driving a real DAW the same way — REAPER, both plug-in formats

Added 2026-08-28 for **BACKLOG E19**'s linux cell. The section above drives the
*standalone*, which has a command channel; a plug-in inside a host has none, so
this is the other half: how to open a prepared rack in REAPER, unattended, on
the same headless compositor, and read the plug-in's own stderr while it runs.

REAPER's Linux build is portable — untar it and run `REAPER/reaper`; nothing is
installed. Point `HOME` at a scratch directory and it keeps its config, its
plug-in scan and its cache entirely inside it, so the developer's `~/.vst3`,
`~/.clap` and `~/.config/REAPER` are never written. Install the plug-ins into
that scratch `HOME` exactly as `scripts/package-linux.sh` would.

```bash
weston --backend=headless --socket=tide-test --width=1600 --height=1000 --xwayland &
# weston prints "xserver listening on display :N" — that N is what DISPLAY wants

env -u WAYLAND_DISPLAY HOME=<scratch> DISPLAY=:2 GDK_BACKEND=x11 \
    XDG_RUNTIME_DIR=/run/user/$(id -u) \
    <scratch>/REAPER/reaper -nosplash
```

**Two traps, and each one cost a measurement before it was found.**

**`GDK_BACKEND=x11` with `WAYLAND_DISPLAY` UNSET, or REAPER segfaults.** Both
halves are load-bearing. A scheduled run inherits `WAYLAND_DISPLAY=wayland-0`
from the developer's session, so GDK picks the Wayland backend — connecting to
*his* compositor, which is exactly what this whole document exists to avoid —
while REAPER's own SWELL code takes the X11 path. The result is a chain of
`gdk_screen_get_root_window` / `gdk_x11_window_get_xid` assertion failures and
then `segfault … in libX11`. It looks like a plug-in crash and is not one: it
happens with the plug-in's editor being opened, so the plug-in is the obvious
suspect and the innocent one.

**Audio needs `linux_audio_mode=2` in `reaper.ini`, under `[reaper]`.** That is
Dummy Audio: no JACK, no device, no sound anywhere near the developer's
speakers, and the audio thread still runs in real time — `GetPlayPosition()`
advances, and the plug-in's `process()` is called. Without it REAPER tries JACK,
fails, and raises a modal *"Error opening devices"* that an unattended run
cannot dismiss. **`prefs_audiodev` is NOT the key** — it is a string in the
binary next to the device names, which is what makes it look like one; values
0-3 were each tried and each left the dialog up. The way the key was found is
worth reusing: set the preference once through the GUI, quit, and diff
`reaper.ini`.

Drive the rest from `<scratch>/.config/REAPER/Scripts/__startup.lua`, which
REAPER runs on startup. `reaper.Main_openProject("noprompt:" .. path)`,
`reaper.TrackFX_Show(tr, 0, 3)` and `reaper.CSurf_OnPlay()` are enough to open a
project, float the plug-in's editor and roll the transport with nobody present.
A `reaper.defer` loop logging `GetPlayState()` and `GetPlayPosition()` is the
control that separates "the plug-in is frozen" from "nothing is being processed"
— do not skip it, because those two look identical from the plug-in's side.

**Getting a prepared rack into a hosted instance.** The rack itself has to be
built in the standalone (that is what the command channel above is for) and then
carried across. Its `session.xml` **is** a `<Preset>` element, the same one
`decode_rpp.py --preset-out` extracts, so no conversion is needed:

- **VST3** — `TrackFX_SetNamedConfigParm(tr, fx, "vst_chunk", b64)`, where the
  bytes are `int32 xmlLen+4`, `int32 1`, `int32 xmlLen`, the XML, then eight zero
  bytes. Read the parm back from a default instance first; that is where the
  framing came from and it is a one-line check that it has not changed.
- **CLAP** — `vst_chunk` returns nothing, and the state lives in a `<STATE>`
  base64 sub-block of the track chunk with no framing at all. **Neither route
  actually restores** as of REAPER 7.43 — see **BACKLOG E60**, which records
  what was measured and what is still unknown.

**Screenshots.** `xwininfo -root -tree` finds the floating FX window by title
(`VST3i: …` / `CLAPi: …`), and `XGetImage` on that window id works; a *root*
grab does not, under XWayland. `_NET_CLIENT_LIST` is absent under weston, so any
helper that looks windows up by WM_CLASS through that property finds nothing —
go by window id.

**Menus need the keyboard, not the mouse.** XTest clicks land correctly on
REAPER's ordinary controls, but a click on an item in an open SWELL dropdown
selects nothing. Open the dropdown with a click, then `Down`/`Up` and `Return`.
