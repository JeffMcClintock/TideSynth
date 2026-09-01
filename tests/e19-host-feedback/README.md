# E19's linux VST3 cell — the harness, and what it measured

Drives REAPER 7.43 on headless weston, gets a **prepared** rack into a hosted
TIDE VST3, rolls the transport for 60+ s, and reads the plug-in's own
`RACK_ADAPTOR_TRACE` counters on both sides of the DSP↔editor feedback path.

Built 2026-08-31 (linux) for **BACKLOG E19**. The 2026-08-28 run built the
REAPER-on-weston recipe and it lives in
[docs/ci/headless-gui-verification.md](../../docs/ci/headless-gui-verification.md);
what is here is the part that was in that run's scratch directory and therefore
did not survive it — *"a reproduction that lives in `_scratch/` is not a
reproduction"*, which this repo has now paid for twice.

## Run it

```bash
export E19_SCRATCH=/some/scratch
mkdir -p "$E19_SCRATCH"/{home/.config/REAPER/Scripts,home/.vst3,proj,shots}

# 1. a trace-armed build
cmake -S . -B build-e19 -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DTIDE_VCV_FUNDAMENTAL=ON -DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1
ninja -C build-e19
cp -a build-e19/SynthEditSem/TIDE-Rack.vst3 "$E19_SCRATCH/home/.vst3/"

# 2. REAPER, portable, into the scratch HOME
curl -sSLo r.tar.xz https://www.reaper.fm/files/7.x/reaper743_linux_x86_64.tar.xz
tar -C "$E19_SCRATCH" -xf r.tar.xz
printf '[REAPER]\nlinux_audio_mode=2\nloadlastproj=0\nautosaveint=0\n' \
    > "$E19_SCRATCH/home/.config/REAPER/reaper.ini"

# 3. learn REAPER's own vst_chunk framing off a DEFAULT instance
bash run-host.sh prepare.lua dump          # writes default_chunk.b64

# 4. frame a prepared rack and mint a project carrying it
python3 frame_chunk.py ../fixtures/e53-vcv-rack-segv.xml "$E19_SCRATCH/prepared_chunk.b64"
bash run-host.sh prepare.lua mint

# 5. open it, float the editor, roll 75 s
bash run-host.sh measure.lua vst3 75

# 6. read the two sides
grep -oE "display-state capture #[0-9]+"                    "$E19_SCRATCH/reaper-vst3.err" | tail -1
grep -oE "display-state update #[0-9]+ arrived \([0-9]+ bytes\)" "$E19_SCRATCH/reaper-vst3.err" | tail -1
grep -oE "RackEditor: light [0-9]+ update #[0-9]+ value [-0-9.]+"  "$E19_SCRATCH/reaper-vst3.err" | tail -1
```

The **standalone is the control**, and it is not optional — see below.

```bash
mkdir -p "$E19_SCRATCH/sacfg/TiDE Rack"
cp ../fixtures/e53-vcv-rack-segv.xml "$E19_SCRATCH/sacfg/TiDE Rack/session.xml"
XDG_RUNTIME_DIR=/run/user/$(id -u) WAYLAND_DISPLAY=tide-e19 \
XDG_CONFIG_HOME="$E19_SCRATCH/sacfg" build-e19/SynthEditSem/TIDE-Rack 2> sa.err
```

## What it measured, 2026-08-31

Same build, same document, same box, same compositor. The only variable is the
host.

| | hosted VST3, REAPER 7.43 | STANDALONE (control) |
|---|---|---|
| rack the DSP built | **43,191 bytes — the prepared one** | 43,389 bytes |
| `Scope display-state capture` | **#2100**, all 65,548 bytes, still climbing | #1800 |
| `display-state update … arrived` | **frozen at #1, 0 bytes** | **#1820, 65,548 bytes** |
| `light … update` | **frozen at #2, value 0.000** | **#18100**, values varying |
| transport | `playstate=1`, pos 0 → 74.919 throughout | — |

**The mechanism is in the log ORDER, not in any single line:**

```
TIDE: instance #3 building rack from 43191 byte document
RackProcessor: 'Scope' display-state capture #0 (65548 bytes)
RackEditor: light 0 update #0 value 0.000        <- the editors' initial defaults
RackEditor: display-state update #1 arrived (0 bytes)
TIDE: instance #4 building rack from 43191 byte document   <- a SECOND processor
RackProcessor: 'Scope' display-state capture #0 … #2100     <- and it runs alone
                                                  (no RackEditor line ever again)
```

The standalone builds twice as well — but as **`instance #1` both times**
(`Legacy chunk`, then `Build chunk, rack already prepared`), so the editor's
pins stay attached to the object that is running. In the host the instance
NUMBER changes and the editor never hears another byte.

## Three traps, each of which cost a measurement

**The `vst_chunk` parm is the state and NOTHING else.** Measured off a default
instance: 140 base64 chars, 105 bytes, exactly
`int32 len+4 | int32 1 | int32 len | XML | 8 zero bytes`. The 44-byte header and
the `AAAQAAAA` trailer that `scripts/make-host-fixture.py` also writes belong to
a `.rpp`'s `<VST>` block, not to this parm. Minting through
`TrackFX_SetNamedConfigParm` is also how the platform TOKEN question disappears:
REAPER wrote `1013510754{506C7567696E474D50492050A2A07287}` — the Linux one —
by itself, so no fixture can carry the wrong one (**BACKLOG E29**).

**The standalone's config folder is `TiDE Rack`, lower-case `i`.**
`tests/fixtures/e53-vcv-rack-segv.README.md` says `TIDE Rack`, and a run that
follows it gets the DEFAULT rack with the fixture sitting one folder away and
nothing saying so — measured here as `building rack from 17961 byte document`
where the fixture would have given 38,658.

**A fresh portable REAPER raises a "New Version Notification" modal** on its
first launch and rewrites `loadlastproj` to `19`. Neither is fatal, both are
confusing; the second launch is clean.

## What this fixture CANNOT measure, and it is the fixture rather than the host

E19's pixel-diff and int/bool/enum clauses both need a VCV panel **on screen**.
All five of the fixture's VCV module editors construct with their panel art in
both arms — `RackEditor: 'Scope' model=yes art=yes(res/Scope.svg)
art-size=195x380` — and **none of them is on the visible rack page**, in the
host or in the standalone. Vertical and horizontal scrolling did not reach them.

So the hosted pixel diff of **0 of 690,800** says nothing on its own: the
standalone control, whose counters are at #18100, gives **byte-identical
screenshots** over the same interval. The negative control that makes this a
statement about the fixture is that the **DEFAULT** rack, in the same build,
draws its `Out` panel on the rails perfectly well.

## Windows — same drivers, a different runner, and it needs no `__startup.lua`

Added 2026-09-02, when E19's **windows** VST3 cell was measured through this
harness. The two `.lua` files are shared; only the runner differs, and it is
[run-host-win.sh](run-host-win.sh) rather than [run-host.sh](run-host.sh).

```bash
export E19_SCRATCH='C:\path\to\scratch'          # WINDOWS path form: the lua and REAPER get this
mkdir -p "$(cygpath -u "$E19_SCRATCH")"/{proj,stage}

# 1. a trace-armed build. Windows produces FLAT FILES -- assemble the bundle,
#    or the plug-in starts with an empty rack and every content measurement is void.
cmake -S . -B build-e19win -G "Visual Studio 18 2026" \
  -DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community" \
  -DTIDE_VCV_FUNDAMENTAL=ON -DSE_LOCAL_BUILD=OFF -DCMAKE_CXX_FLAGS="-DRACK_ADAPTOR_TRACE=1"
cmake --build build-e19win --config Release
#    bundle: <stage>/TIDE-Rack.vst3/Contents/{x86_64-win/TIDE-Rack.vst3,Resources/*}

# 2. BACK UP %APPDATA%\REAPER FIRST -- see below -- then point vstpath64 at
#    <stage> alone and move reaper-vstplugins64.ini aside to force a rescan.

# 3. mint, then measure
python3 tests/e19-host-feedback/frame_chunk.py tests/fixtures/e53-vcv-rack-segv.xml \
        "$(cygpath -u "$E19_SCRATCH")/prepared_chunk.b64"
bash tests/e19-host-feedback/run-host-win.sh tests/e19-host-feedback/prepare.lua mint \
     "$(cygpath -u "$E19_SCRATCH")/prepare.log" 150
bash tests/e19-host-feedback/run-host-win.sh tests/e19-host-feedback/measure.lua vst3 \
     "$(cygpath -u "$E19_SCRATCH")/measure.log" 170 75

# 4. RESTORE %APPDATA%\REAPER from the backup, and check the md5s match.
```

**`fx_ident` is the answer to "which binary did I just measure?", and it is not
optional here.** A staged build and the developer's installed one carry the same
VST3 UID; REAPER keeps ONE cache entry for the pair and `TrackFX_AddByName`
picks whichever it likes. On the first run of this harness it picked
`C:\Program Files\Common Files\VST3\TIDE-Rack.vst3` — the installed one — and
said nothing. Both drivers now log the parm. This **supersedes** the 2026-08-28
remedy of compiling a distinguishing string into the build and reading it back:
that works, but it answers the question a whole build later.

**The host cannot be isolated on Windows.** REAPER resolves its resource path
with `SHGetKnownFolderPath`, which ignores `%APPDATA%`, and a `reaper.ini` beside
`reaper.exe` does not engage portable mode. So there is no scratch `HOME` trick
here as there is on linux: back up `%APPDATA%\REAPER` **before the first launch**
and restore it afterwards. The 2026-09-02 run did, and both `REAPER.ini` and
`reaper-vstplugins64.ini` came back md5-identical. The STANDALONE *is* isolatable
— `GMPI_STANDALONE_CONFIG_DIR=<root>` (E55), with the fixture at
`<root>/TiDE Rack/session.xml`; `%APPDATA%\TiDE Rack\` was md5-identical after.

**Two Windows-only traps, each of which cost a measurement.**

**REAPER's File:Quit raises "Save project … before closing?" on a dirty project,
and adding an FX dirties it.** An unattended run parks on it forever.
`Main_SaveProjectEx` does **not** clear the flag — after a successful save-as the
prompt still named the *original* project. So `bail()` writes a `done` sentinel
and quits only off-Windows; the runner waits for the sentinel and kills the
process. That is strictly better here anyway: a killed REAPER never rewrites the
developer's ini on the way out.

**An EMPTY project has length 0 and REAPER stops the transport the instant it
reaches the end** — `playstate` 1 → 0 inside one second, `pos` never leaving
0.000, which reads exactly like a wedged plug-in. `measure.lua` now creates a
silent MIDI item to give the project a length, and re-issues play (saying so on
the line) if the transport ever drops.

**And one that is not REAPER's at all:** `read -r -t N < /dev/zero` is a working
sleep on linux and returns **instantly** in Git Bash, because `/dev/zero` always
has a byte to read. A 180-iteration wait finished in milliseconds and killed
REAPER about a second after launch — three runs read as "the host will not start
on this box", one of them wrongly blamed on REAPER's evaluation nag, before the
harness was suspected. Use `sleep`.

## What Windows measured, 2026-09-02

Same fixture, same drivers, REAPER 7.78. The transport rolled the whole window.

| | hosted VST3, REAPER 7.78 | STANDALONE (control) |
|---|---|---|
| rack the DSP built | **43,187 bytes — the prepared one** | 43,391 |
| `feedback send` / `editor received` | **3,200 / 3,200** — one-for-one | 4,700 / 4,700 |
| `display-state update … arrived` | **#2180, 65,548 bytes** | #2260, 65,548 bytes |
| `light … update` | **#6800, value 0.824**, 106 distinct values | #9300, value 0.305 |
| transport | `playstate=1`, pos 0 → **74.671** | — |

**Still advancing at the end of the window, by line position rather than by
inference:** the last `building rack` line is 145 of 620, and **216
`display-state update`, 146 `light update` and 32 `editor received feedback`
lines follow it**.

**The pixel-diff clause is unmeasurable from this fixture on Windows too**, for
the reason the section above gives — 0 of 760,950 on the rack canvas. The
controls that make that a statement about the fixture: REAPER's own transport
area in the **same screenshot pair** changed **9,333 of 232,200**, so the capture
is live; the standalone changed **0 of 921,600** while its counters ran; and the
rack is drawn as **bare rails with no VCV panel on it**. **E75**, on a second
platform.
