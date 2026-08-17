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

---

## 2026-08-17 — macos — the sound reproduces from upstream alone (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff applied the GMPI patch, merged it,
and said "done". Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** verified the thing the local overrides had been hiding: **with no
local overrides and nothing unmerged anywhere, a from-scratch configure that
fetches GMPI and GMPI_Wrappers from GitHub main builds TIDE and it still makes
sound** — 1 kHz Tone → Sound Out, −6.0 dB, master green. The four-repo S12
stack is now genuinely self-consistent on main, and this box builds exactly
what a fresh clone would. Also appended a build-trap note to
[docs/building.md](docs/building.md).

**Confirmed on main before testing:** `ppc3` in `processor_holder.cpp` and the
`sendNonNativeParameterToProcessor` hook in `controller_holder.h` (GMPI,
merged as PR #1), and the VST3 installer of that hook in GMPI_Wrappers. Then
cleared both `GMPI_*_FOLDER_OVERRIDE` cache entries and deleted the local
`tide/mac/blob-param-transport` branch — **nothing on this box is now needed
to build TIDE that is not on GitHub.**

**Made a mess and cleaned it, which is the entry's real content.** Clearing
the overrides was not enough: FetchContent had already populated `_deps`, so
I deleted those directories — which left the build tree inconsistent and
configure failing (`gmpi_plugin.cmake` not found; the fetch step silently
declined to re-run). The right tool was **`cmake --fresh`**, and it worked
first time. But `--fresh` wipes the *whole* cache, and two of those cached
values mattered:

1. **The generator.** The tree was an **Xcode** project; a bare `--fresh`
   re-generated it as Unix Makefiles. Restored with
   `cmake --fresh -G Xcode .` — worth knowing before anyone runs `--fresh` on
   a tree they did not create.
2. **`SE_LOCAL_BUILD`**, and this one is genuinely nasty. The POST_BUILD step
   that copies the bundle to `~/Library/Audio/Plug-Ins/VST3` lives inside
   `if(SE_LOCAL_BUILD)` in GMPI's `gmpi_plugin.cmake`, and the option is
   **declared FALSE by default** — a developer machine auto-installs only
   because the value is sitting in `CMakeCache.txt`. After `--fresh` it was
   gone, so **the build succeeded, the bundle in the tree was current, and
   REAPER kept loading the previous binary.** No error anywhere.

**Learned — "it built" and "the host is running it" are different claims, and
a cache reset can split them silently.** I caught it only because I compared
the installed binary's timestamp and size against the build tree's before
trusting a host test, which turned a plausible false pass into a two-command
fix (`cmake -DSE_LOCAL_BUILD=TRUE .`). **This is the same discipline as the
clipboard sentinel and the thumbnail change-test: make the artefact prove it
is the one under test.** Both traps are now written into
[docs/building.md](docs/building.md), since the next person to run `--fresh`
here will hit them in the same order.

**Left as found:** Xcode generator restored, `SE_LOCAL_BUILD=TRUE` restored,
overrides empty, installed plug-in byte-identical to the current build tree
(checked with `cmp`).

**Next:** unchanged — S12's remainder in its row (MIDI-note verify first, then
the save/reopen re-check, faded swap, preset retention).

**Side effects on this box:** two full fresh configures and three TIDE_VST3
builds (~15 min); `_deps` for GMPI and GMPI_Wrappers re-cloned from GitHub;
REAPER restarted once, throwaway project only, **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (doc + entry only; no code).

---

## 2026-08-17 — macos — FIRST SOUND (interactive session, Jeff present — "i can hear it!")

**Prompt:** n/a — interactive session. Jeff merged the S12 stack, pointed out
that Sound Out is the device sink and "VST Output" the plugin path, and then
— after three more fixes — heard TIDE Rack's first sound. Committed and
pushed as `tide-rack-bot` (claude-fable-5).

**Did:** finished the S12 thin slice. **Place 1 kHz Tone, place Sound Out,
drag the cable: track meter −6.0 dB, master green, and Jeff heard it.** The
whole product loop is live for the first time: edit the rack → document
exports → chunk parameter → processor → graph rebuilds → audio.
[SynthEditLib#15](https://github.com/JeffMcClintock/SynthEditLib/pull/15) +
[SynthEdit#40](https://github.com/JeffMcClintock/SynthEdit/pull/40), 
diagnostics stripped.

**The three finds between "merged" and the meter moving, in order:**

1. **The export pruner was a design decision, not a bug** — with any target
   except `SAT_SYNTHEDIT_DSP`, `ExportXml_Pt2` on a top-level container
   serialises ONLY its first child container and disregards loose modules
   ("save XML for use in a plugin. Excludes 'Sound Out'..." — the comment says
   it plainly). The probe that proved both per-module gates PASSED
   (`doExport=1 hasDsp=1`) is what forced reading past them. **TIDE now
   exports with the editor's own runtime format, and the modules appear.**
2. **The AsyncRestart swap path is unreachable in the plugin runtime** —
   `eRuntimeState::resetting` exists only as a case label; nothing enters it.
   So the first (empty) graph played silence forever while every later
   document push was stored and never consumed. **Fix: `documentPending_`
   forces `prepareToPlay` to rebuild, and the processor re-calls it on every
   document arrival** — synchronous, in place, fine at rack scale; the faded
   swap can come later without changing the transport.
3. **No browser unhide was needed.** Jeff's pointer resolved cleanly:
   "VST Output" is what `SetupVstIO` builds internally; the user-facing sink
   is plain **Sound Out**, whose `ug_soundcard_out` is a real
   `ISpecialIoModuleAudioOut` — it registers with the audio master at `Open`
   and receives whatever buffers the shell provides: device buffers in the
   standalone, **the host's output buffers in TIDE**. (IO Mod, the other
   candidate, refuses to instantiate in a master container — it needs a
   parent to expose plugs to.)

**Learned — when both gates pass and the output is still empty, the skip is
between the gates.** The probe pattern (log every candidate's name plus each
gate's verdict) took one build and turned "modules missing, cause unknown"
into "read `ExportXml_Pt2`". And its own first run crashed REAPER on an
uninitialised iterator (`it_doc_ob` needs `First()`), which is the day's
smallest lesson: MFC-style iterators do not position themselves.

**Also caught on the way:** my line-ending-blind Python edit turned the GMPI
patch into a 2212-line diff; redone byte-safe (CRLF preserved) it is 58
lines. **Check the patch size before filing a patch.**

**Where S12 stands:** thin-slice Accept met and heard. Remaining, named in
the row: the MIDI-note path (wired via `onMidiMessage` → `rack.MidiIn`, but
untested — no MIDI module was in the patch), the faded document swap, preset
retention across edits, and re-verifying a REAPER save/reopen now that the
chunk carries real documents (S11's other half). **The GMPI blob-transport
patch is still unapplied** — main lacks "ppc3" and GMPI_Wrappers main now
requires it, so builders without the local branch break; the patch sits in
[docs/patches/](docs/patches/gmpi-blob-param-transport.patch) and Jeff's queue.

**Side effects on this box:** ~6 more TIDE_VST3 rebuilds, one REAPER crash
(my probe's iterator — report read, fixed), several restarts; throwaway
projects only, **"Optimus HP" untouched**. `/tmp/tide-s12.log` and
`/tmp/tide-dsp-doc.xml` left behind by the now-removed diagnostics.

**Branch/PR:** this TideSynth PR +
[SynthEditLib#15](https://github.com/JeffMcClintock/SynthEditLib/pull/15) +
[SynthEdit#40](https://github.com/JeffMcClintock/SynthEdit/pull/40).

---

## 2026-08-17 — macos — S12 built to its last step: the rack has an engine; the tone is one gate away (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff ruled Option A with the boundary
sharpened — "keep the wrappers pure and simple, put the DSP xml stuff in TIDE
itself" — and the implementation began. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** the S12 thin slice, end to end except its final step, across four
repos: GMPI branch `tide/mac/blob-param-transport` (**the bot cannot push to
that repo — 403 — so the commit is local and the patch is filed at
[docs/patches/gmpi-blob-param-transport.patch](docs/patches/gmpi-blob-param-transport.patch)
for Jeff to apply**),
[GMPI_Wrappers#4](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/4),
[SynthEditLib#14](https://github.com/JeffMcClintock/SynthEditLib/pull/14),
[SynthEdit#39](https://github.com/JeffMcClintock/SynthEdit/pull/39) (WIP).

**Found and fixed on the way in — the wrapper had no blob transport at all.**
The three-point log bisected it in one run: the editor pushed 570 bytes, the
controller's `setParameter` returned Ok, and the processor's pin never fired.
`notifyDaw`/`performEdit` speak normalized doubles; `onQueMessageReady`
consumed only `"ppc2"`. **The fix is generic, not TIDE-specific** — any
wrapped plug-in with a blob parameter needs it: the holder hands changed blobs
to a wrapper-installed hook (the `"ppc3"` framing already existed unused!),
the VST3 wrapper moves the framed bytes with its existing binary message, and
the processor holder consumes them into a Blob PinSet event. **Verified: the
next run's log shows the same push arriving as `onSetPins size=570`.**

**Then the graph — three crashes, each teaching the document's required
shape:** (1) `SetupVstIO` derefs the *nested synth container* — exported
projects have main→synth-child, TIDE's flat rack IS the master → wrap the
export in a synthetic outer container. (2) `BuildPatchManager` runs on main
unconditionally → move the PatchManager to the outer; the inner inherits,
ordinary SE containment. (3) After both: **the empty document builds and RUNS
(`prepared SR=48000 BS=512`) and a wired-document push while running takes
the AsyncRestart fade/teardown/rebuild path without crashing.** The
`#if 0 // editor only` `pendingDspXml` hook is enabled and working.

**The frontier, precisely: modules are pruned from the export.** The placed
1 kHz Tone and Sound Out never reach the XML — `<Modules />` stays empty while
their connecting `<Line>` survives. First cause found and fixed:
`CDocOb::exportFlags = EXP_PLUGIN` makes `doExport()` drop every
`excludeFromVst` module — **Sound Out and the whole Diagnostic group are
exactly those** — because a baked export replaces them; TIDE self-hosts
editor semantics, so flags are now 0. **But modules are still pruned with
flags 0.** Prime suspect: the other gate in `CUG::ExportXml` —
`hasDspModule()` false because the module set's **DSP-side registrations never
ran in TIDE**. That is the exact U2d pattern that hit the GUI half. **Next
probe (one build): log `name / doExport() / hasDspModule()` per master-child
inside `exportDspXml`.**

**Learned — a three-point log turns "it doesn't work" into a one-run
bisect.** sync-push / setParameter-rc / onSetPins-size located a missing
wrapper subsystem in a single REAPER launch, then re-verified the fix the
same way. The temp diagnostics (tagged `TEMP S12 diag`) are deliberately
left in the WIP branch so the next session continues without re-instrumenting;
they come out before merge.

**Build note until the stack lands:** TIDE's build cache now sets
`GMPI_SDK_FOLDER_OVERRIDE` and `GMPI_WRAPPER_FOLDER_OVERRIDE` to the local
checkouts (CPM otherwise fetches GitHub main, which lacks the transport).
Drop the overrides once the GMPI patch and GMPI_Wrappers#4 are on main.

**Next:** the row's next-probe, then whichever registration wiring it names —
the Accept (1 kHz Tone → Sound Out → host meter moves) is plausibly one or
two builds away. After the tone: remove the diagnostics, then S11's restore
path rides the same wire (the chunk already persists in the DAW state).

**Side effects on this box:** ~8 TIDE_VST3 rebuilds; REAPER crashed twice
(both crash reports read and acted on) and was restarted ~6 times; throwaway
projects only, **"Optimus HP" never opened, saved or modified**. Temp files:
`/tmp/tide-s12.log`, `/tmp/tide-dsp-doc.xml`. GMPI repo has a local branch
the bot could not push.

**Branch/PR:** this TideSynth PR + the four-repo stack above.
