# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

Every run on three machines reads this file in full, so its size is a cost paid
forever. It hit **192 KB across 37 entries in six days** before the first
rotation (**A8**, 2026-08-12). Nothing is ever deleted or rewritten — old
entries just move to a per-month archive.

**The rule, applied after you append your own entry:**

1. Move the oldest entries out, in order, into `JOURNAL-<YYYY>-<MM>.md` for the
   month each entry belongs to, appending **below** what is already there so the
   archive stays newest-first. Copy the template from
   [JOURNAL-2026-08.md](JOURNAL-2026-08.md) if that month has no file yet.
2. Stop when this file is **under 30 KB**, or when the **four most recent
   entries** remain — whichever comes first. **The floor of four wins**: the run
   prompt tells every run to read the last four entries, so a verbose month
   pushing this file over 30 KB is correct, not a rotation failure.
3. Never edit an entry while archiving it. The archive is the record.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

Template:

```
## YYYY-MM-DD — <machine> — <BACKLOG id>

**Did:** what actually changed.
**Result:** built / tested / failed, with the real output.
**Learned:** anything the next run would otherwise rediscover the hard way.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---

## 2026-08-17 — macos — MIDI notes verified from pure upstream; the patch workaround is retired (interactive session, then unsupervised)

**Prompt:** n/a — interactive session; Jeff granted the bot write access to
GMPI, merged [GMPI#2](https://github.com/JeffMcClintock/GMPI/pull/2), then
said "clean up the patches and verify from upstream" and left ("do as many
tasks as you can unsupervised"). Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** **the bot can now open PRs on GMPI** (invitation accepted,
`push=true` confirmed), and with both GMPI fixes merged I **cleared every
local override and re-verified MIDI from pure upstream: peak 0.5103 (−5.8 dB)
with a note, 0.0001 with the transport stopped** — the identical numbers to
the local-override build, so a fresh clone now hears MIDI. This closes S12(a)
properly rather than on a machine-specific build.

**The access fix, for the record:** the bot had `push=true` on all five other
repos and **`push=false` on GMPI alone** — it had simply never been granted.
Jeff added it as a collaborator; the bot accepted invitation `329366224`
itself via `gh api -X PATCH user/repository_invitations/<id>`. **Both GMPI
fixes are now normal PRs (#1 and #2), and the patch-file route is retired.**

**A near-miss worth recording, because it would have been a false pass.**
After clearing the overrides the build **succeeded** — and grepping the
fetched sources showed **the fixes were absent**: `_deps` still held the old
checkouts, because **FetchContent does not re-pull an already-populated
dependency just because `origin/main` moved.** I had "verified upstream"
against stale code for one build. Fixed by `git fetch && git reset --hard
origin/main` in each `_deps/*-src`, after which all four greps matched and
the numbers reproduced. **Grep the fetched source for the change you are
verifying — a green build proves nothing about which code was compiled.**

**Cleanup, and why the patch files stay.** `docs/patches/` now carries a
**README marking both patches superseded**, with the PR each landed as, and
an instruction not to use that route again. **The `.patch` files themselves
are kept deliberately**: JOURNAL entries link to them and the journal is an
immutable record, so deleting the files would break history to tidy a folder.
The README is the honest way to say "obsolete" without rewriting the past.

**Also cleared:** all three `*_FOLDER_OVERRIDE` cache entries are empty, the
local GMPI branches are deleted, and the installed plug-in is byte-identical
(`cmp`) to the pure-upstream build. `SE_LOCAL_BUILD=TRUE` had to be restored
again after the earlier `--fresh` — the trap already documented in
[docs/building.md](docs/building.md), hit for the second time today, which is
why it is written down.

**Learned — a clean instance beats a debugged one.** The first upstream
measurement read silence, and the reason was not the build: earlier failed
UI batches had dropped modules into that instance's rack while the structure
view was not open, leaving a polluted document. Rebuilding the patch in a
**fresh tab** reproduced the expected numbers immediately. **When a test rig
has been poked at by failed automation, rebuild the rig before debugging the
product.**

**Next:** S12's remainder — the save/reopen re-check first, since the chunk
now carries real documents *and* is seeded into fresh processors, so S11's
restore half may already work; then the faded swap and preset retention.

**Side effects on this box:** several TIDE_VST3 builds; `_deps` for GMPI,
GMPI_Wrappers and SynthEditLib re-cloned/reset to upstream main; overrides
cleared; installed plug-in current. REAPER restarted twice and several
throwaway tabs were created; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (docs + bookkeeping only; no code).

---

## 2026-08-17 — macos — MIDI notes play: the blob pin was never seeded on a new processor (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged choice (ii) and said "keep
going till you are blocked". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** found and fixed the last blocker. **A MIDI note now plays a TIDE
rack.** Measured: **peak 0.5103 (−5.8 dB) while the note sounds, 0.0001 with
the transport stopped** — so the audio genuinely follows the notes rather than
droning, which is the test Jeff's VCA correction made possible. The fix is
generic and lives in GMPI; **the bot cannot push that repo (403 again), so the
patch is filed at
[docs/patches/gmpi-seed-blob-pins.patch](docs/patches/gmpi-seed-blob-pins.patch)**
(72 lines) for Jeff to apply, exactly like this morning's transport patch.

**The bug, in one sentence:** `gmpi_processor`'s pin-initialisation loop seeded
parameter-backed pins for `Float32`, `Int32` and `Bool`, and **`Blob` fell
through to `default: assert(false)`** — so a newly created processor started
with an empty blob pin and was never told otherwise, because blobs only reach
the processor when they **change**.

**Why that mattered so much here.** A host creates processors whenever it
likes — after `restartComponent`, for offline or anticipative processing, on
state restore. REAPER was running **two** TIDE processors: the editor's held
the rack document and ran audio, while a second one received all the MIDI with
`prepared=0` for its entire life. It had never been handed the document, so it
had no graph to play. **The two-instance behaviour was never the bug; the
un-seeded blob pin was.**

**Learned — a `default: assert(false)` is a to-do list.** The same switch
statement had already bitten me this morning: `sendParameterToProcessor` was
missing its Blob case, and I added it to fix the *change* path. **I fixed one
arm of the pattern and did not check the other**, so the initialise path kept
the hole for another six hours. When a datatype is missing from one switch
over `PinDatatype`, grep every switch over `PinDatatype` in the same file
before moving on — there were exactly two, and they needed the same case.

**Verification note:** the gate test is what makes this claim safe. A patch
whose oscillator reaches Sound Out will drone at its default pitch and read a
healthy peak whether or not MIDI works — Jeff caught me making exactly that
mistake earlier. Comparing **note-playing (0.5103) against transport-stopped
(0.0001)** is the measurement that cannot be faked by a drone.

**Next:** with notes audible, S12(a) is done. The remainder of S12 is the
save/reopen re-check (the chunk now carries real documents **and** is seeded
into fresh processors, so S11's restore half may work already), then the faded
swap and preset retention.

**Side effects on this box:** three TIDE_VST3 builds; all probes reverted
earlier and both code trees verified clean before this change. **The build now
points `GMPI_SDK_FOLDER_OVERRIDE` at the local GMPI checkout** (needed to test
the unmerged patch) — it should be cleared once Jeff applies it, or fresh
clones will build without the fix. REAPER restarted twice; **"Optimus HP"
untouched**.

**Branch/PR:** this TideSynth PR + local GMPI branch `tide/mac/seed-blob-pins`
(patch filed for Jeff).

---

## 2026-08-17 — macos — the MIDI mystery solved: a second processor instance, and it never gets the document (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged choice (ii) and said "keep
going till you are blocked". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** chased the "MIDI stops after a rebuild" defect and **solved it — the
cause is not the rebuild at all.** REAPER runs **two** TIDE processor
instances, and **MIDI is delivered to the one that has never received a
document**. This also retracts yesterday's conclusion, which was based on a
transport that had quietly stopped. No code change; the evidence and the fix
direction are the deliverable.

**The evidence, from one instrumented run** (probe in TIDE's `onMidiMessage`,
`subProcess` and `SynthRuntime::MidiIn`, all logging `this`):

```
onMidiMessage  proc[0x12b0b0800] prepared=1   x31      <- editor's instance
onMidiMessage  proc[0x12ba36800] prepared=0   x77      <- the one REAPER feeds MIDI
PREPARE        proc[0x12b0b0800]  (every document)
subProcess     proc[0x12b0b0800]  (only this one)
```

**So:** the instance the editor is attached to receives every document push,
builds its graph and runs audio. A **second** processor instance receives the
host's MIDI, has `prepared=0` for its whole life, and never runs `subProcess`.
The two never meet.

**Why it happens, and it is a flaw in my S12 design rather than a host quirk.**
TIDE pushes the document **only when the XML changes** — `serviceDocumentSync`
dedupes against `lastPushedDspXml`. **Any processor that appears afterwards
therefore starts empty and stays empty**, because nothing ever re-sends. A
plug-in's processor can be created at any time — the host may re-instantiate
after a `restartComponent`, add an instance for offline/anticipative
processing, or restore state into a fresh one — so "push once on change" was
never going to be sufficient.

**Fix direction, and the right one is not the obvious one.** The hacky answer
is to re-push periodically. **The correct answer is that a newly created
processor should be seeded with the current value of every parameter**,
including blob parameters — which is what the chunk parameter is for. The
document already persists in the DAW state (that half now works), so the same
delivery that restores a saved project should seed a mid-session instance.
Worth checking whether `gmpi_processor` seeds pins from
`patchManager` at construction and simply skips blobs: if so, this is a small
generic fix in the same place as this morning's transport work, not a
TIDE-specific patch.

**Learned — retract cleanly when the evidence changes.** Yesterday I recorded
"MIDI delivery stops across a document rebuild", with instance pointers to
back it. Today's run shows MIDI never stopped: **the transport had stopped**
because clicking in the editor to place modules had halted playback, and my
"no deliveries after the rebuild" was that, not a defect. **The pointer
evidence was real and the conclusion drawn from it was wrong** — the missing
control was "is the transport actually running while I measure?", which the
`reaper.defer` sampler answers in its first line and which I did not check
before concluding.

**Also settled: there is exactly one processor per editor.** The earlier
worry that registration and delivery hit different `SeAudioMaster`s has the
same explanation — different *processors*, each with its own runtime and
generator, not a stale pointer inside one.

**Next:** seed a new processor with the current chunk-parameter value (check
`gmpi_processor`'s construction path for blob handling first, since a generic
fix there beats a TIDE-specific one). That is the last thing between the
current build and an audible MIDI note.

**Side effects on this box:** four TIDE_VST3 builds; **all probes reverted
byte-safely and both trees verified clean** (`git status` empty in SynthEdit
and SynthEditLib), installed plug-in rebuilt from the committed state. REAPER
restarted twice; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — choice (ii) built: the runtime feeds MIDI In; a second defect surfaced (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff ruled **choice (ii)**: "MIDI-in will
eventually be a rack module with a patch-point of its own, perhaps more like a
MIDI-CV". (He also briefly pasted Optimus dialog screenshots from another
session and said to carry on with MIDI — no Optimus work was done.) Committed
and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** implemented (ii) —
[SynthEditLib#16](https://github.com/JeffMcClintock/SynthEditLib/pull/16), 29
lines. **`SeAudioMaster` now remembers a registered `MIDI In` module and
`MidiIn()` feeds it with the same `AddMidiEvent` call
`UIoManager::OnMidiData` makes**, so the module behaves identically in the
standalone and in a plug-in. **Half-verified, and the other half turned up a
second, separate defect** — both stated below rather than blurred together.

**Why the gap existed, in one line:** a MIDI In module registers itself with
the audio master exactly as Sound Out does, the standalone's `UIoManager`
pairs that registration with a MIDI device, and the plug-in's
`SynthRuntime::RegisterIoModule` is a documented no-op — *"nothing special to
do in plugin"*. **So a MIDI In module in a plug-in patch was silent by
construction**, and the fix is to make the plug-in the device.

**Verified:** the module **does** register in a plug-in — the probe printed
`RegisterIoModule[0x11e39a370]: midiIn=1` and the pointer was stored.

**Not verified, and the reason is a NEW finding:** after the document rebuild
that adds the module, **no further MIDI reached `SynthRuntime::MidiIn` at
all**. Instance pointers made it unambiguous: every delivery went to
`SeAudioMaster[0x11ce0e2a0]` — the graph that existed *before* the rebuild —
and the registration landed on `[0x11e39a370]`, the graph built *after* it,
which then received nothing despite the transport running for 240 sampled
frames. **So MIDI delivery stops across a document rebuild.** That is
independent of this fix and is the next thing to chase.

**Learned — log the instance pointer when two objects can wear the same
name.** "Registered" and "not receiving" looked contradictory until `%p`
showed they were different `SeAudioMaster`s. **And read the log's ORDER before
concluding:** I nearly filed "registration never happens" when the
registration line was simply the *last* line in the file — everything before
it predated placing the module. One `tail` corrected a wrong conclusion.

**Repeated a mistake I had already recorded, which is worth admitting.** My
first cleanup of the probes used a line-based Python rewrite and normalised
`SeAudioMaster.cpp`/`.h` from CRLF to LF — a **5754-line diff** for a 29-line
change, exactly the trap I hit on the GMPI patch earlier today and wrote down.
Reverted and redone byte-safely (`b'\r\n'`-aware), giving the honest 29-line
diff. **A lesson recorded is not a lesson learned until the tool that caused
it is fixed** — the byte-safe `edit()` helper now used should be the default
for every repo that stores CRLF.

**Next:** chase the rebuild defect — MIDI stops reaching the processor after a
graph rebuild. Cheapest probe is the one already proven: log in TIDE's
`onMidiMessage` and in `SynthRuntime::MidiIn` across a document change, and
compare instance pointers on both sides.

**Side effects on this box:** six TIDE_VST3 builds; all probes reverted and
the installed plug-in rebuilt from the committed state. REAPER restarted three
times; **"Optimus HP" untouched** (the guard aborted one script when REAPER
reopened that project, after which every script created its own tab).
`SYNTHEDITLIB_FOLDER_OVERRIDE` remains pointed at the local checkout.

**Branch/PR:** this TideSynth PR +
[SynthEditLib#16](https://github.com/JeffMcClintock/SynthEditLib/pull/16).

---

## 2026-08-17 — macos — container-IO contract reverse-engineered; the MIDI In module is standalone-only (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff ruled "synthesise real container IO
for MIDI, I think that is the least disruptive to SynthEdit". Committed and
pushed as `tide-rack-bot` (claude-fable-5).

**Did:** worked out **exactly** what `exportDspXml` has to emit, by reading the
importer rather than guessing — and found one thing that changes the shape of
the fix: **the patch's "MIDI In" module cannot be the endpoint, because in a
plug-in nothing ever feeds it.** No code change; the contract and that
constraint are the deliverable, and together they make the next session a
single implementation pass.

**The XML contract, from `ug_base::Setup` and `SeAudioMaster::BuildModules`:**

- A **container IO plug** is any `<Plug>` carrying a `Direction` attribute —
  that is literally how the importer distinguishes it ("IO Plug on Container
  or I/O Mod. Identified by 'Direction' element"). It becomes
  `new UPlug(this, (EDirection)direction, (EPlugDataType)datatype)`, so:
  `<Plug Direction="0" Datatype="2"/>` is a MIDI **input** — `DT_MIDI2` is
  **2** in `EPlugDataType{DT_ENUM=0, DT_TEXT, DT_MIDI2, DT_DOUBLE, DT_BOOL,
  DT_FSAMPLE=5}`.
- An **IO Mod**'s plug ties to its container's plug **by handle**, and only
  when the module carries `UGF_IO_MOD`:
  `<Plug Direction="1" Datatype="2" TiedTo="<containerHandle>"
  TiedToPinIdx="<n>"/>` → `up->TiedTo = p2; p2->TiedTo = up;`
- **Connections** are `<Line From="<handle>" To="<handle>" FromPin="i"
  ToPin="j"/>`; `FromPin`/`ToPin` default to 0, and the handles are resolved
  through `HandleToObject`.

**The constraint that changes the design.** TIDE's browser offers a **MIDI In**
module, and it looks like the obvious MIDI source — but
`modules_internal/MidiIn.h` is `class MidiIn final : public MpBase2, public
ISpecialIoModule`, and it obtains MIDI by calling
`AudioMaster()->RegisterIoModule(this)` in `open()`. In the **standalone** that
registration lands in `UIoManager`, which feeds it from a MIDI device. In the
**plug-in** it lands in `SynthRuntime::RegisterIoModule`, whose entire body is
`{ return 1; } // nothing special to do in plugin`. **So a "MIDI In" module in
a plug-in registers itself and is then never fed by anyone** — it is a
standalone-app module, and its Audio pins confirm it (`MIDI Data` out,
`Activity` out, `MPE Mode` in — **no MIDI input pin at all**, so nothing can be
routed into it either).

**Which means the classic plug-in MIDI path is the only one available**, and
it is exactly what Jeff's ruling describes: host → `vst_in` → **the synth
container's DT_MIDI2 plug** → an **IO Mod** inside → the user's MIDI-consuming
modules (MIDI-CV 2 and friends). That is how an exported SE plug-in has always
worked; TIDE's flat rack simply never grew the container plug.

**So the open question is a UX one, not a mechanical one, and it is Jeff's:**
what does the user patch *from* in the rack? Either **(i)** TIDE synthesises a
container MIDI plug plus a tied IO Mod at export, and the IO Mod is the thing
users drag from — it is already in TIDE's module list, so this needs no new
module and no SynthEdit change; or **(ii)** TIDE keeps "MIDI In" as the
user-facing source and `SynthRuntime` learns to feed registered MIDI modules
the way `UIoManager` does — nicer for users, but it is the SynthEdit change
Jeff's ruling was steering away from.

**Learned — read the importer, not the exporter, when synthesising a format.**
Every attribute that matters here (`Direction` as the IO-plug marker,
`Datatype`'s enum ordering, `TiedTo`/`TiedToPinIdx`, the defaulting of
`FromPin`/`ToPin`) came from the ~40 lines that *parse* the XML. The exporter
would have shown only what a normal project happens to contain, which is
exactly the case that does not apply to TIDE's synthesised document.

**Next:** Jeff picks (i) or (ii); the row holds the full contract so the
implementation is one pass either way.

**Side effects on this box:** read-only investigation — nothing built, REAPER
not driven, no probes left anywhere. All six repos clean.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — suspect (a) refuted; the real cause found: the rack exposes no plugs (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "do suspect (a)". Committed
and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** chased suspect (a) — the UMP-vs-MIDI-1.0 format theory — and **it is
wrong**. Instrumenting the chain instead found the actual break, one layer
lower and much more consequential: **TIDE's rack container exposes no MIDI
plug and no audio plugs, so `SetupVstIO` never connects the synthetic VST
Input's MIDI Out to anything.** No code change; the diagnosis is the
deliverable.

**Suspect (a) is refuted at the source.** `SeAudioMaster::MidiIn` hands the
bytes to `ug_vst_in::sendMidi`, which calls `MpeConverter::processMidi` — and
that function opens with `if (gmpi::midi_2_0::isMidi2Message(msg)) { sink(msg,
timestamp); return; }`. **MIDI 2.0 passes through by design**, comment and
all. The UMP packets TIDE forwards are exactly what it accepts. Nothing needs
translating.

**What the chain probe showed, in order:**

```
SetupVstIO: synthModule=1 plugs=3
  synth plug: type=0 dir=0        <- DT_ENUM
  synth plug: type=4 dir=0        <- DT_BOOL
  synth plug: type=0 dir=0        <- DT_ENUM
AudioMaster::MidiIn len=8 [40 90 3c] audioIn=1
  vst_in sink: size=8 inUse=0 mo=1
```

`EPlugDataType` is `DT_ENUM=0, DT_TEXT=1, DT_MIDI2=2, DT_DOUBLE=3, DT_BOOL=4,
DT_FSAMPLE=5`. **So the container's three plugs are ENUM, BOOL, ENUM — there
is no DT_MIDI2 plug and no DT_FSAMPLE plug.** `SetupVstIO` loops over exactly
those looking for MIDI and audio, finds neither, and connects nothing —
which is precisely the measured `inUse=0` on `vst_in`'s MIDI Out. **The MIDI
arrives at the audio master, is converted correctly, and is then sent into a
plug with no connections.**

**And this explains the asymmetry that made the bug confusing.** Audio works
(the tone clipped at +10 dB) **not** through the container's plugs but because
**Sound Out is a special IO module**: `ug_soundcard_out` registers itself via
`RegisterIoModule` at `Open()` and receives the host's buffers directly. MIDI
has no equivalent registration, so it depends on the container plumbing that
does not exist. **A rack that makes sound while ignoring MIDI is exactly what
those two different mechanisms predict.**

**The fix direction, for the next session to design rather than guess:** the
inner rack container needs real plugin IO — a `DT_MIDI2` input plug wired to
the patch's MIDI In module (and, if audio should ever leave via the container
rather than via Sound Out's registration, `DT_FSAMPLE` outputs too). In SE
terms that is what an **IO Mod** provides, and `exportDspXml`'s synthetic
outer container is the natural place to synthesise it. **Whether TIDE should
instead treat the patch's "MIDI In" module as a registering special IO module
— the symmetric counterpart of Sound Out — is the design question, and it is
Jeff's call.**

**Learned — probe the chain, not the theory.** Suspect (a) was a reasonable
hypothesis and it cost one build to disprove by *reading the consumer*
(`processMidi`'s first three lines). The chain probe then located the break
in a single run because it logged **at four points**, so the last successful
step and the first failing one were adjacent in the output. **Instrumenting
several points at once beats bisecting one hypothesis at a time.**

**Caught a build-configuration trap of my own making:** the first probe run
produced *no log at all*, because `cmake --fresh` had also cleared
`SYNTHEDITLIB_FOLDER_OVERRIDE`, so the build was compiling **SynthEditLib
fetched from GitHub** (`_deps/syntheditlib-src`) and my local edits were
invisible. Verified by `strings`-ing the installed binary for the probe's log
path — zero hits — before believing the silence. **`strings` the artefact when
a probe does not fire; the code you edited may not be the code that ran.** The
override is now restored to the local checkout, which is how this box was set
up before the `--fresh`.

**Next:** design the container-IO fix (S12(a) continues). Everything else in
S12's remainder is unchanged.

**Side effects on this box:** four TIDE_VST3 builds; all probes reverted and
the installed plug-in rebuilt clean (verified probe-free with `strings`).
`SYNTHEDITLIB_FOLDER_OVERRIDE` now points at the local checkout again; GMPI
and GMPI_Wrappers remain upstream. REAPER restarted twice. **"Optimus HP" was
never modified — and the guard proved itself:** REAPER reopened that project
as the active tab and the rig script aborted rather than touch it, after which
every subsequent script created its own new tab first.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — S12(a): MIDI reaches the processor but not the graph (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "sync and clean up, then
continue", which meant S12's next item: verify the MIDI note path. Committed
and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** synced all six repos and deleted three merged branches, then took
**S12(a)**. **Result: MIDI arrives at TIDE's processor and is forwarded to the
runtime, but produces no sound in a correctly-built instrument — the MIDI does
not manifest inside the DSP graph.** No code change; the measurement and the
two named suspects are the deliverable.

**What is proven, each by direct measurement:**

1. **MIDI reaches the processor.** A temporary probe in `onMidiMessage` logged
   **42 messages with `prepared=1`** during a looping C4: note-ons (`0x90`),
   note-offs (`0x80`), and the note number `0x3c` = 60. They are forwarded to
   `rack.MidiIn`. **They arrive as 8-byte MIDI 2.0 UMP packets** (leading
   `0x40` = MIDI2 channel-voice), not 3-byte MIDI 1.0 — which is the first
   suspect below.
2. **The audio path works.** With the oscillator wired straight to Sound Out
   the master meter clipped at **+10 dB** — loud, obviously alive.
3. **A complete instrument is silent.** Jeff's correction was the key
   methodological point: an oscillator wired directly to Sound Out **drones at
   its default pitch whether or not MIDI arrives**, so it proves nothing about
   MIDI. Rebuilt as a real instrument — **MIDI In → MIDI-CV 2; Pitch → Phase
   Dist Osc; Gate → VCA Volume; Osc → VCA Signal; VCA → Sound Out** — and the
   measured peak is **−156.7 dB, i.e. digital silence**, throughout the note.

**Learned — a tight Lua polling loop measures nothing.** My first two
"measurements" reported 9.3M and 34M samples of `Track_GetPeakInfo`, all
identical, because a busy-wait blocks REAPER's main thread and those values
only update on it: **the loop was re-reading one frozen snapshot millions of
times and reporting it as a result.** The give-away was the transport position
never advancing across 6 seconds of wall clock. Re-done with `reaper.defer`
(one sample per main-thread cycle), position advanced normally and the reading
was trustworthy. **A high sample count is not evidence; a changing input is.**

**Two suspects for the next session, in order:**

1. **MIDI format.** GMPI hands the processor **UMP**; `SeAudioMaster::MidiIn`
   may expect MIDI 1.0 bytes. `se_vst3`'s `SeProcessor` does explicit
   translation around its `MidiIn` calls (`midi2data`/`midi2size`, and it
   advertises `kMIDIProtocol_2_0`), which TIDE's one-line forward does not.
   Compare those call sites first.
2. **Container plumbing.** `SeAudioMaster::SetupVstIO` connects the synthetic
   **VST Input**'s "MIDI Out" to *the synth container's* MIDI plug — and S12
   wraps TIDE's flat rack in a **synthetic outer container**, so MIDI may be
   delivered to the outer container and never forwarded to the inner rack
   where the MIDI In module lives. That wrapper was introduced for
   `SetupVstIO`'s benefit, so it is exactly the code to re-read.

**Also worth knowing:** TIDE's module set has **no MIDI Monitor** (Diagnostic
holds only 1 kHz Tone and DAW Sample Rate), which is why the verification had
to be built from a VCA instead of read off a monitor — Jeff suggested a
monitor first and it simply is not in the fixed set.

**Next:** S12(a) continues with suspect 1 (cheap: log what
`SeAudioMaster::MidiIn` receives, and compare with `se_vst3`'s translation),
then suspect 2. The other S12 remainder items are unchanged.

**Side effects on this box:** two probe builds plus two clean rebuilds; the
probe branch was deleted and the tree reverted, so **the installed plug-in is
built from `master` with no diagnostics**. REAPER restarted twice; throwaway
projects only, **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (row + entry only; no code).
