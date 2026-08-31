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

---

# macOS — a portable REAPER isolates completely, and the lock is the real boundary

**Measured 2026-08-29 (macos, scheduled run) for BACKLOG E19's mac AU3 cell.**
This document was Linux-only; the macOS answer is different in every respect
worth knowing, and one half of it is better than Linux's while the other half
is worse.

## Portable mode works, and it is total

Copy `REAPER.app` into a directory and `touch reaper.ini` beside it. REAPER
then keeps its ENTIRE resource tree there — `ColorThemes/`, `Data/`,
`Effects/`, every `reaper-*.ini`, the plug-in caches — and never writes
`~/Library/Application Support/REAPER`.

Verified rather than assumed, both directions: after four REAPER launches and
four renders, the developer's config compared **identical, mtimes and sizes
included**, and his `~/Library/Audio/Plug-Ins` plus `~/Applications` compared
identical across **1,019 files**.

**This is the opposite of the Windows result, which matters for reading the
fleet's per-platform notes.** Windows was measured TWICE as un-isolatable —
`reaper.ini` beside `reaper.exe` does not engage portable mode there, deleting
`reaper-install-rev.txt` does not either, and REAPER resolves `%APPDATA%` via
`SHGetKnownFolderPath`, which ignores the environment. On macOS the same idea
simply works. So "we cannot isolate the host" is a Windows fact, not a fleet
fact, and a mac run has no excuse for touching the developer's REAPER.

## A FRESH portable config hangs, and it looks exactly like a plug-in fault

The trap costs a session if you meet it cold: a newly-created portable config
**hangs on a first-run modal** that an unattended run cannot dismiss. Every
symptom points at the plug-in — `-renderproject` never returns, no output file
appears, and the timeout looks like TIDE wedging the render.

It is not TIDE, and the control that proves it is the repo's own:

```
python3 scripts/render-and-measure.py --control     # no plug-in involved at all
```

With a fresh config this **times out**. Seed the portable directory with the
developer's configured settings — copying `reaper.ini` is the load-bearing
one, and the plug-in caches and `reaper-reginfo2.ini` save a rescan — and the
same control returns **rc=0 in 14 seconds**, `peak -6.0 / rms -9.0 dBFS`.

Copy those files; never quote `reaper-reginfo2.ini`'s contents anywhere, since
it carries the developer's registration.

## The boundary a SCHEDULED run actually hits: the screen is locked

A scheduled run on this box finds the login window up — the console is owned by
the developer and the session is locked. Two behaviours follow, and they are
NOT the same, which is why this section exists rather than a flat "GUI does not
work":

| path | locked session |
|---|---|
| `REAPER -renderproject project.rpp` (offline) | **works** — 3–4 s per fixture |
| full GUI launch driven by `Scripts/__startup.lua` | **hangs**, no script output |

So audio measurement through a real host is available to an unattended mac run,
and anything needing the plug-in's editor — animation counters, pixel diffs,
menu toggles — is not. `screencapture` on a locked session returns an
**all-black frame of the full screen size**, which is worth naming because a
black PNG reads as "the window drew nothing" rather than "the display is off";
that is E58's lesson in a new place.

The separation was measured, not assumed: the offline render succeeds and the
GUI launch fails **in the same session, minutes apart**, so neither result is
about the machine being busy.

## Measured through this harness, 2026-08-29

Both renders through the isolated portable REAPER, on a locked session:

```
tests/hosts/v1-rack.rpp            peak -6.3 dBFS  rms -17.0 dBFS   AUDIO PRESENT
tests/hosts/v1-rack-uncabled.rpp   peak -inf       rms -inf         SILENCE
```

The first reproduces the 2026-08-18 macOS reference to the decimal and confirms
**E59's fix holds on macOS `main`**; the second is the negative control, and the
pair is what makes either number mean anything.

## Which build answered? Take the plug-in away and re-render

**Added 2026-08-31 (macos, scheduled run).** The section above isolates the
*host*. It does not, on its own, tell you which *plug-in* the host loaded, and
E19's windows leg already paid for that distinction: a local build does not
shadow an installed one, and REAPER will silently pick either — it alternated
between them across runs on that box.

Narrowing the scan path is the arrangement:

```bash
# stage ONLY the bundle under test
mkdir -p "$PORTABLE/plugins"
cp -R build-<tree>/SynthEditSem/TIDE-Rack.vst3 "$PORTABLE/plugins/"

# point the portable config at that folder alone, then force a rescan
sed -i '' "s|^vstpath_arm64=.*|vstpath_arm64=$PORTABLE/plugins|" "$PORTABLE/reaper.ini"
rm -f "$PORTABLE"/reaper-vstplugins*.ini
```

**The measurement is removing it.** Move the staged bundle aside, clear the
cache again and re-render the same fixture. REAPER hangs on an unresolvable
plug-in modal and writes **no TIDE entry** to `reaper-vstplugins_arm64.ini`;
restore the bundle and the fixture's numbers come back. That round trip is what
makes "this number came from the build I just made" a fact rather than a hope,
and it costs one 300-second timeout.

Two things it settles that are otherwise easy to hand-wave:

- **REAPER sanitises `-` to `_` in the ini key.** The cache reads
  `TIDE_Rack.vst3=...` while the staged bundle is `TIDE-Rack.vst3`. No
  underscore-named bundle exists on the box; the key is not evidence that some
  other artifact was scanned. Check with `find` before believing either reading.
- The developer's installed plug-ins stay untouched throughout — verify rather
  than assume, with `shasum -a 256` on the installed binary and a
  `find … -exec stat` snapshot of `~/Library/Application Support/REAPER`
  compared before and after. Measured identical across 2052 files, mtimes
  included, on 2026-08-31.

## Measured through this harness, 2026-08-31

macOS `main` at `4994c32`, with E64, E66 and E67 merged — a fresh Release/arm64
tree, all six repos clean and equal to `origin/main`:

```
--control (no plug-in at all)      peak -6.0 dBFS  rms  -9.0 dBFS   chain detects audio
tests/hosts/v1-rack.rpp            peak -6.3 dBFS  rms -17.0 dBFS   AUDIO PRESENT
tests/hosts/v1-rack-uncabled.rpp   peak -inf       rms  -inf        SILENCE
```

Unchanged from 2026-08-29, which is the point: the three merges that landed in
between did not move it.

---

# No host at all — drive the plug-in's own state contract over the C ABI

**Added 2026-08-31 (macos, scheduled run) for BACKLOG E69.** Everything above
this line is about isolating a *host*. This section is about not needing one.

A save/restore defect is a question about **bytes**, not about audio, and every
route this document had for reading those bytes went through a DAW: REAPER's
GUI, a `__startup.lua`, a project file to decode afterwards. On macOS that route
is closed to a scheduled run whenever the screen is locked, and for CLAP it is
closed anyway — REAPER's CLAP state path does not restore (**E60**).

The CLAP C ABI needs none of it. `tests/e69_clap_state_probe.c` is about 200
lines and links nothing but `dlfcn` and the CLAP headers:

```bash
cc -std=c11 -I <build>/_deps/clap-src/include tests/e69_clap_state_probe.c -o probe
./probe <build>/SynthEditSem/TIDE-Rack.clap /tmp/out tests/…/preset.xml
```

It calls `dlopen` → `clap_entry` → `get_factory` → `create_plugin` →
`plugin->init` → `get_extension(CLAP_EXT_STATE)`, then `save`, `load`, `save`,
writing each save to a file. The whole run takes about a second.

**Why this is a real test and not a stub of one.** `Processor_CLAP`'s
constructor creates and `initialize()`s the plug-in's own `<Controller/>`
unconditionally — `Processor_CLAP.cpp:88`, added for S43(ii) — and it asks the
host for no extension to do it. So a bare host with `get_extension` returning
`NULL` for everything still exercises the same controller/processor pair a DAW
would, which is precisely where save/restore bugs live.

**What it found on its first run.** GMPI_Wrappers#36 moved the CLAP save to the
controller's store; this wrapper's `stateLoad` writes only the processor's. The
probe measured the consequence in one command, and the A/B is one commit wide:

| CLAP built at | save after loading an 18,893-byte 4-cable rack |
|---|---|
| `bb155b1` (#35) | 18,933 bytes, **4 cables** |
| `379d5c1` (#36) | **85 bytes, 0 cables** — `<Param id="1" val=""/>` |
| GMPI_Wrappers#37 | 18,661 bytes, **4 cables** |

## Three habits this makes cheap, and each one cost a session elsewhere

- **Build the two ends of a bisect as separate trees, not by moving a shared
  one.** `-DFETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS=<clone>` points a build at a
  private clone of a dependency at whatever commit you like, so the A/B never
  touches the developer's working tree and cannot be spoiled by his next
  rebuild. Two clones and two build dirs; nothing to restore afterwards.
- **Round-trip twice.** Feeding a minted document back in and getting
  **byte-identical** output is what separates "the save re-serialises" from "the
  save rewrites the patch a little more every time". Without it, the one-time
  14,136 → 13,930 shrink reads as loss.
- **Count the thing, not the bytes.** `scripts/dump_preset.py`-style cable and
  module censuses (22 modules, 8 types, 4 `<Cable>` endpoints) survive a
  re-serialisation that any size comparison fails. **A size DIFFERENCE proves a
  save did not echo; a size MATCH proves nothing** — the mint is a fixed point,
  so a re-saved document matches exactly.

**What it deliberately does not answer:** what the patch sounds like, and
anything needing the editor. Those are the sections above, and this one does not
replace them — it removes the host from the questions that never needed one.

---

# macOS AUv3 — registering one, hosting it, and the instrument that does not survive

**Measured 2026-08-31 (macos, interactive, Jeff present) for BACKLOG E19's mac
AU3 cell.** The 2026-08-29 run measured *five* ways to register a current AUv3
beside the developer's and all five failed, concluding the cell needs a human.
It does, and this is what the human unlocks — plus one wall that a human does
not remove.

## Registering: displace, having first taken a copy

There is no second slot. `pluginkit -a`, a distinct `CFBundleIdentifier`, a
distinct AU subtype, an inside-out ad-hoc re-sign and `lsregister -f` were all
measured to leave `pluginkit -m -i <id> -v` answering `(no matches)`. The only
thing that works is replacing the installed app — which is why it needs
somebody who can say yes:

```bash
ditto ~/Applications/TIDE-Rack-AUv3.app <scratch>/backup/   # FIRST. it is 5 MB
rm -rf ~/Applications/TIDE-Rack-AUv3.app
ditto build-<tree>/SynthEditSem/TIDE-Rack-AUv3.app ~/Applications/
open -g ~/Applications/TIDE-Rack-AUv3.app                   # LAUNCH registers it
pluginkit -mv | grep -i tide                                # UUID and date must CHANGE
```

**The backup is what makes this safe to do at all**, and it answers the
2026-08-29 objection directly: the risk was a run dying mid-way and leaving the
registration pointing at a build tree that later gets deleted. A copy taken
first makes that recoverable in one command.

Read the result by the **UUID and timestamp**, not by presence — the old
registration is present too, and looks identical apart from those two fields.

## Then `auval`, before any DAW

```bash
auval -a | grep -i tide          # aumu Drck Dsyh  -  TiDE Synth:TiDE Rack
auval -v aumu Drck Dsyh
```

Apple's own validator is stricter than our probes and cheaper than a DAW.
2026-08-31: **AU VALIDATION SUCCEEDED**, rc=0.

## REAPER hosts it, and two traps cost a launch each

REAPER 7.45 scans the registered extension into `reaper-auplugins_arm64.ini` as
`TiDE Synth:TiDE Rack` and loads it under the name **`AUi: TiDE Rack (TiDE
Synth)`** — the `AUi: <name> (<manufacturer>)` form. Get that spelling from
REAPER itself rather than guessing; it is printed in any "Project Load Warning"
about a missing plug-in.

- **A seeded portable config reloads the developer's last project**, which
  raises a modal naming plug-ins it cannot find, and the modal blocks
  `Scripts/__startup.lua` from ever running. Set `loadlastproj=0` in the
  portable `reaper.ini` and pass an explicit empty `.rpp`. The symptom is a
  startup script that produces *no log at all*, which reads as "the script is
  wrong".
- **Delete `reaper-auplugins_arm64*.ini` from the PORTABLE copy** to force an AU
  rescan. A cache seeded from the developer's has no TIDE entry — his has never
  held one — so without this REAPER never looks.

## The wall a human does NOT remove: an appex has no stderr

**`RACK_ADAPTOR_TRACE` is unusable for a hosted AUv3 on macOS.** The counters
this project relies on — `RackProcessor: '<slug>' display-state capture #N`,
`first nonzero light`, the `apply expect=/sum=` pair, and TIDE's own
`syncState`/`building rack from` lines — all go to **stderr**, and an audio-unit
extension runs out-of-process under the system, so **none of them reach the
host's stderr**. Verified: the strings are in the appex binary, the plug-in
loads and runs, and `grep -i "TIDE:|RackProcessor" reaper-stderr.log` returns
nothing.

That is the same problem E65 already solved one layer up, and the same shape of
answer applies: a file-path log switch (`TIDE_PANEL_LOG_PATH` +
`-DTIDE_PANEL_TRACE_LOG`). Filed as **E73**.

So on macOS AU3 the reachable evidence is: registration, `auval`, host
instantiation, parameter count, and **screenshots**. The counter-based clauses
of E19 are not reachable until the trace can write to a file.

## What a screenshot settles that a symbol check cannot

The floated editor drew, and its module browser listed `LFO`, `LFO2`, `Scope`,
`SEQ3`, `SHASR`, `Quantizer` and the rest under a **"Rack-VCV Fundamental"**
heading. That is a picture of VCV Fundamental linked and *enumerated inside the
hosted extension* — the question the 2026-08-29 run got wrong twice by reading
`strings` for `VCV: Scope`, which never appears because ids are composed at
runtime. **When a symbol check is ambiguous and the thing is on screen, screenshot it.**
