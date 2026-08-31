# Host-project fixtures

Saved DAW projects that exercise TIDE **through a real host**, which is the only
way to measure the thing PLAN's v0.1 acceptance test actually asks about: does a
patch still play after the host saves and reloads it?

Measure one with [`scripts/render-and-measure.py`](../../scripts/render-and-measure.py):

```bash
python3 scripts/render-and-measure.py --control
python3 scripts/render-and-measure.py tests/hosts/v1-rack.rpp
```

Run `--control` first. It renders a known −6 dBFS 1 kHz sine and must report
exactly −6.0 peak / −9.0 rms; that proves the render-and-measure chain detects
audio at all, so a subsequent silence is a fact about the patch rather than
about the script.

| Fixture | What it holds | Measured 2026-08-18 |
|---|---|---|
| `v1-rack.rpp` | Oscillator → Envelope → Output, cabled jack-to-jack | **peak −6.3 dBFS, rms −17.0 dBFS** — 440.0 Hz, left channel only |
| `v1-rack-uncabled.rpp` | the same three prefabs, **no patch cables** | **−inf, silent** — the negative control |
| `v1-rack-midi.rpp` | MIDI → Oscillator → Envelope → Output, four cables, **plus a middle-C note** | **−6.3 dBFS, 440.0 Hz, unchanged by the note** — a FAILING fixture, on purpose (BACKLOG **E7**) |
| `v3-midi-gate.rpp` | the same rack but gated from the MONOPHONIC `SE MIDItoGate2`, plus the note | **silent · 440.0 Hz for the note · silent** — V3's Accept, met |
| `v3-midi-pitch.rpp` | the auto-seeded **root** MIDI-CV → Oscillator/Envelope/Output, plus the note | **silent · 261.6257 Hz for the note · silent** — middle C to +0.001 cents. Gate, pitch and tuning all correct |

## Measured on Linux for the first time, 2026-08-31

Every figure is the macOS 2026-08-18 reference to the decimal, on REAPER
7.43/Linux against a `TIDE_VCV_FUNDAMENTAL=ON` build — and `v1-rack.rpp` was
**digital silence** on this box on 2026-08-28, before **E59** merged.

| fixture | linux |
|---|---|
| `--control` (no plug-in at all) | peak −6.0 / rms −9.0 dBFS |
| `v1-rack.rpp` | **peak −6.3 / rms −17.0 dBFS** — 2 patch cables |
| `v1-rack-uncabled.rpp` | **−inf** — 0 patch cables, the negative control |
| `v3-midi-pitch.rpp` | −6.2 / −21.1 dBFS |
| `v3-midi-gate.rpp` | −6.3 / −21.2 dBFS |

**Two things this settles.** The committed fixtures carry the macOS 7.45
plug-in token and REAPER 7.43/Linux **loads them anyway**, so E29's divergence
did not make them unreadable here. And the reason nobody had these numbers is
not the fixtures: run from a scheduled run's own shell the script segfaults
REAPER, because it inherits `WAYLAND_DISPLAY` from the developer's session. The
one-line wrapper is in
[docs/ci/headless-gui-verification.md](../../docs/ci/headless-gui-verification.md).


The pair matters more than either one alone. `v1-rack-uncabled.rpp` is what a
saved rack looks like when nothing joins one module to another, and it reports
**eight `<Line>` elements** — the cables *inside* the three prefab containers.
So a non-zero `<Line>` count is not evidence that a rack is wired, and the
uncabled fixture is what keeps that from being re-learned the hard way. The
count that decides it is the patch-cable list in `HC_PATCH_CABLES`; see the
docstring of `render-and-measure.py`.

## Regenerating one

These are GUI artifacts — there is no script that writes them, because placing
and cabling rack prefabs is an editor operation. To rebuild one:

1. Launch REAPER **from a shell**, not `open -a`, so an uncaught C++ exception
   in the plugin names itself on stderr. TIDE also prints
   `TIDE: 3 rack prefab(s) seeded from the bundle` there, which confirms the
   installed bundle's `Resources/Prefabs/` is what supplied them.
2. Insert **TIDE Rack** on a track. It is listed under its product name, not
   its filename — filtering the FX browser for "TIDE" finds it, and REAPER's
   plugin cache may still hold an older name from a previous build, in which
   case re-scan (Preferences → Plug-ins → VST → Re-scan).
3. Open its editor, pick **Prefabs** in the browser, and click a prefab then
   click the rack to place it. Drag jack to jack to cable. **Cable each jack
   once** — dragging a second cable from a jack that already has one grabs the
   existing cable instead of making a new one.
4. Save. The rack needs no MIDI: the oscillator's PITCH jack defaults to 5 V
   (440 Hz) and the envelope's GATE jack defaults open, so three cabled modules
   emit a continuous tone with nothing sequenced.

**Cable each jack by its exact centre.** The jack hit-area is only a few pixels;
press even 3 px off and you grab the module BODY and *move* it instead, and TIDE
has no undo (PLAN excludes it from v0.1). Cable in an order that grabs each jack
**before** any cable is drawn near it, or a later drag picks up the cable rather
than the jack.

## `v1-rack-midi.rpp` is a fixture for a failure

It is checked in **because** it fails, and because the numbers say precisely
where. A MIDI item is plain text inside a `.rpp` — `E <delta-ticks> <status>
<d1> <d2>`, hex, 960 ticks per quarter note, so at TEMPO 120 one quarter note is
0.5 s — which is why this one was hand-written rather than drawn in the MIDI
editor.

The note is **middle C, not A4, deliberately**: the Oscillator's own unpatched
PITCH default is 5 V = 440 Hz, so a fixture using A4 could not distinguish "MIDI
set the pitch" from "the default did". It renders 440.0 Hz either way, which is
how we know the MIDI cables contribute nothing.

What is NOT wrong: the fixture (REAPER draws the note), and MIDI delivery — TIDE
prints `TIDE: host MIDI reaching the rack` when launched from a shell, and
MIDI-CV 2's gate tracks the note exactly when read from inside its own container.
See **E7**, and `build-prefabs.py --diagnostics` for the two probes that
established it.

## The VST3 UID token, and why a fixture may refuse to load — BACKLOG E29

Every `.rpp` here names the plugin by a REAPER-specific token. **Different
REAPER versions write and expect DIFFERENT byte orders for the SAME UID**, and a
fixture carrying the wrong one loads with no plugin at all:

> **Project Load Warning** — The following effects were in the project file and
> are not available. `Track 1: VST3i: TIDE Rack (TIDE Synth)`

| token | written/expected by | measured |
|---|---|---|
| `1386065673{506C7567696E474D50492050A2A07287}` | REAPER **7.45**, macOS — the raw TUID, and what is COMMITTED here | loads, renders `-6.3 / -17.0 dBFS` |
| `1558955188{67756C506E694D4750492050A2A07287}` | REAPER **7.78**, Windows — COM little-endian | **refuses to load on 7.45** |

**The plugin's identity has not changed.** The TUID is literally
`"PluginGMPI     "` with byte 11 `'P'` plus a 4-byte id hash
(`GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:200`); the two tokens are the
same sixteen bytes in opposite order.

**THE TWO ARE MUTUALLY EXCLUSIVE — measured 2026-08-26, both directions.** The
committed token fails on 7.78 (the Windows box), and substituting the 7.78 token
makes the same fixture fail on 7.45 (verified interactively on macOS: the dialog
above, and the FX slot reading *"could not be loaded"*). **So re-saving these
fixtures from a newer REAPER would fix one box and break the other.** Do not do
it as a "fix" without deciding a fleet-wide REAPER version first.

### If a fixture will not load on your box

Swap the token in a LOCAL copy — do not commit it:

```bash
sed 's/1386065673{506C7567696E474D50492050A2A07287}/1558955188{67756C506E694D4750492050A2A07287}/g' \
    tests/hosts/v1-rack.rpp > /tmp/v1-rack-local.rpp
```

To find what YOUR REAPER writes: load the plugin into an empty project, save it,
and read the `<VST ...>` line.

## Measuring YOUR build on Windows, not the one that happens to be installed

**A local build does not shadow the installed plug-in, and REAPER picks silently.**
Both live on the scan path, both answer to the same VST3 UID, and a measurement
taken against the wrong one looks completely normal. This cost a measurement on
2026-08-28 and again on the run that fixed **E59**.

Two things make a run attributable, and you want both:

1. **A distinguishing string in the build, read back off the log.** Every
   `TIDE:` diagnostic added for a row is one; check the *binary* for it before
   running anything, with the installed plug-in as the negative control:

   ```bash
   python3 -c "
   for p in ['build-e59/SynthEditSem/Release/TIDE-Rack.vst3',
             'C:/Program Files/Common Files/VST3/TIDE-Rack.vst3/Contents/x86_64-win/TIDE-Rack.vst3']:
       print(p, open(p,'rb').read().count(b'syncState exporting'))"
   ```

   `strings` is not a substitute — it found none of these format strings in a
   Windows PE that demonstrably contained them (**E50**).

2. **Narrow REAPER's scan path to one folder**, so there is only one candidate.
   Back the whole resource directory up first, then:

   ```powershell
   Copy-Item -Recurse "$env:APPDATA\REAPER\*" C:\SE\_scratch\reaper-backup
   (Get-Content "$env:APPDATA\REAPER\REAPER.ini") -replace '^vstpath64=.*$', `
     'vstpath64=C:\SE\_scratch\e59\vst3' | Set-Content "$env:APPDATA\REAPER\REAPER.ini"
   Move-Item "$env:APPDATA\REAPER\reaper-vstplugins64.ini" C:\SE\_scratch\   # force a rescan
   ```

   Restore by copying the backup back afterwards, and **verify it** — sizes and
   mtimes should compare identical to a snapshot taken before the run.

**Assemble a bundle; do not copy the bare DLL.** A `.vst3` lifted straight out
of `build-*/SynthEditSem/Release/` has no `Contents/Resources`, so it starts with
an empty rack and prints `no DefaultRack.synthedit in bundle resources` — every
measurement of rack CONTENT against it is void while looking entirely normal.
The shape is the one `SynthEditSem/CMakeLists.txt` builds under `SE_LOCAL_BUILD`:

```
TIDE-Rack.vst3/Contents/x86_64-win/TIDE-Rack.vst3   <- the built DLL
TIDE-Rack.vst3/Contents/Resources/                  <- the six XMLs,
                                                       DefaultRack.synthedit,
                                                       and Prefabs/
```

**Configure with `-DSE_LOCAL_BUILD=OFF`** (the default) so the build's POST_BUILD
step does not replace the developer's installed plug-in at
`C:\Program Files\Common Files\VST3\TIDE-Rack.vst3`.

### A portable REAPER does NOT isolate the config on Windows — measured

Copying the whole 152 MB install to a scratch directory, adding a `reaper.ini`
beside `reaper.exe` and deleting `reaper-install.ini` **does not engage portable
mode**: REAPER 7.78 still read and wrote `%APPDATA%\REAPER`, which was proven by
snapshotting that directory and diffing it after a render (`REAPER.ini` grew, three
other `.ini`s were re-stamped). This is the second run to try it. **Back the
directory up and restore it instead** — a `-renderproject` run only adds a recent-project
entry and re-stamps timestamps, so a restore-and-verify comes back byte-identical.

### It fails as a HANG, not an error

The warning above is **modal**, so `REAPER -renderproject` blocks on it forever
rather than exiting. `scripts/render-and-measure.py` now caps the render at
`RENDER_TIMEOUT_SECONDS` (300) and kills REAPER, because the first measurement of
this sat for over seven minutes producing nothing. A healthy render of these
fixtures takes about four seconds.
