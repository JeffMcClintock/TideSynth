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
