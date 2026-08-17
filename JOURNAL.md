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

---

## 2026-08-17 — macos — S12 mapped: the machinery exists, in the sibling VST3 target (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff's pointer, in substance: SynthEdit
also builds the graph for its own use and switches out the DSP smoothly; the
SynthEdit VST3 target is similar and can rebuild the graph under certain
conditions. Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** followed the pointer through the code and rewrote **S12** from "scope
unknown, probably large" into a **start-ready implementation map with file:line
references**. No product code this entry; the map is the deliverable, and it
changes S12's size class from "unknown" to "one focused session".

**What the pointer found, part by part:**

1. **The plugin-side graph host exists:** `SynthRuntime`
   (`SynthEditLib/SynthRuntime.cpp` — sibling of the standalone's
   `SynthRuntime_editor`) owns `SeAudioMaster` and builds the whole DSP graph
   from an XML document. Today it reads that XML from the **`dsp.se.xml`
   bundle resource** (line 59) — the thing SynthEdit's *exporter* bakes in.
   **The cache check is the injection point:** `if
   (!currentDspXml.RootElement())` means a pre-seeded document skips the
   bundle read entirely. TIDE never needs a baked resource; it needs to hand
   the runtime its XML.
2. **The smooth swap Jeff described is real and complete**
   (`SynthRuntime.cpp:330-395`): `audioMasterState::AsyncRestart` → retain
   presets (`getPresetsState`) → `Close()` → new `SeAudioMaster` → background
   `rebuildDsp` thread → fade-up. **And the document-swap hook already exists
   in skeletal form:** a `pendingDspXml` branch sits inside that path behind
   `#if 0 // editor only` — disabled because an exported plug-in's document
   never changes. TIDE is exactly the product that flag was sketched for.
3. **A complete working reference exists:** `se_vst3`'s `SeProcessor`
   (`adelayprocessor.cpp`, 1502 lines) does everything TIDE's stub does not —
   `prepareToPlay`/`reInitialise`, MIDI translation, queue servicing,
   `ProcessorStateMgrVst3`, and `setState`/`getState` whose chunk is a
   `DawPreset` string. **Jeff's "all state lives in the preset" is that
   target's existing design**, not a new invention.
4. **The editor can emit the DSP XML at runtime:** `dsp.se.xml` is nothing but
   `<Document><DSP>` wrapping `MasterContainer->ExportXml(element, target)` —
   fifteen copyable lines in `ExportAsPlugin.cpp:1204-1220` (skip the Release
   `Scramble`). So the live document TIDE's editor already edits can produce
   exactly what `SynthRuntime` eats, with the same serialiser the exporter
   uses.

**Why S11 and S12 turn out to be one mechanism.** Document XML rides in the
preset (S11's rulings); a preset that carries a new document sets
`pendingDspXml` and triggers `AsyncRestart`; the rebuild thread constructs the
new graph while audio fades (S12). Save, restore, and preset-change-modifies-
the-rack are the same wire.

**The one fork, flagged for a one-word ruling.** **Option A (recommended):**
keep TIDE as a GMPI plug-in and grow its processor a `SynthRuntime` — all of
this session's controller/editor wiring survives, and the document travels
through the already-declared `chunk` blob parameter bound to a DSP pin
(`BlobInPin` exists in sdk3; the GMPI-Core equivalent is a named unknown).
**Option B:** rebase TIDE onto `se_vst3`'s `SeProcessor`/`SeController` —
the processor comes ready-made, but the `controllerPtr` trick, TideApp
attachment and editor hosting all get redone against Steinberg classes.
**Named unknowns for either:** `BundleInfo` calls inside `SynthRuntime`
(`latencyConstraint`, resource folder) against a bundle that lacks the
exporter's resources; in-process queue wiring TideApp ↔ runtime.

**Thin-slice accept, unchanged:** place **1 kHz Tone**, wire it to **Sound
Out**, play, and see the host meter move.

**Learned — "scope it before costing it" can cost one hour and change the
answer.** Yesterday's S12 said "unknown and probably large: building a DSP
graph at runtime is what the exporter does at build time". The pointer plus an
hour of reading found the runtime builder, the swap machinery, the skeletal
document-swap hook, and the serialiser — all existing, none speculative. The
row now names them by file and line, which is the difference between a next
session that implements and one that re-discovers.

**Next:** S12, Option A unless Jeff says B — starting with the fresh-context
implementation session the row is now written to launch.

**Side effects on this box:** none — read-only exploration; nothing built,
REAPER not driven. Only TideSynth committed.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — S11's design answered by Jeff; S12 filed: TIDE makes no sound (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff answered S11's three open questions
and said to keep building. Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** recorded Jeff's three rulings into **S11** so they cannot evaporate,
and filed **S12** — **TIDE's audio processor is a stub that writes silence, so
the rack makes no sound at all.** Found while reading the DSP path his answers
pointed at. **No code this entry**, and the reason is the entry's whole point:
his answers describe rebuilding a DSP graph, and **there is no DSP graph to
rebuild.**

**Jeff's rulings, verbatim in substance, now in S11's row:**

1. **A new document implies a rebuild of the DSP graph** — and SynthEdit
   already handles that: **fade-out → teardown → reconstruction → fade-up.**
   So restore does not need a new mechanism invented; it needs the existing one
   driven.
2. **A host preset change may modify the rack.** It is treated as loading a
   brand-new document — anything can change.
3. **All state lives in the preset.** That settles the third question (rack in
   plug-in state vs a referenced user file) in favour of the preset, which is
   also what makes (2) coherent.

**S12, and why it stops S11 rather than merely accompanying it.**
`SynthEditSem/SynthEdit.cpp`'s `class SynthEdit final : public Processor` is
the only DSP class TIDE has, and its `subProcess` is:

```cpp
// TODO: Signal processing goes here.
*left = 0.0f;  *right = 0.0f;
```

`TideApp` never starts a synth runtime — no `prepareToPlay`, no
`StartBackgroundProcessing`, no generator. **Nothing anywhere instantiates the
user's placed modules as DSP.** So the rack is an editor with a silent audio
stub bolted on: the modules exist as documents and views, and their DSP
counterparts (which ARE registered — `ug_oscillator2` and the rest) are never
built into a running graph.

**Why this reframes everything above it.** Ruling 1 says restore must rebuild
the DSP graph; a rebuild of nothing is a no-op, so **S11's restore path can be
built today and will be correct, but its DSP half cannot be exercised or
verified until S12 lands.** And for the release question the two are not equal:
a rack that forgets your patch is a bad synthesiser, but **a rack that makes no
sound is not a synthesiser at all.** S12 therefore blocks the R-series ahead of
S11.

**What this does NOT mean, stated so nobody re-derives it in alarm.** This is
not a regression and nothing broke: the DSP stub has been a `// TODO` since the
prototype, and every session since has been building the editor — the thing
constraint 1 is about. Six sessions of host verification never caught it for
the same reason they never caught S11: **every test drove the UI, and no test
ever played a note and looked at a meter.** That is now two findings from one
missing habit.

**Learned — when a ruling arrives, check its premise before building to it.**
Jeff's answers are exactly right for the system he is describing; they were
answers about a DSP rebuild, and the honest response was to look at the DSP
path before writing a line. Two greps did it. **Building S11's restore first
and discovering the silence afterwards would have produced code whose central
claim — "the graph rebuilds" — nobody could test.**

**Next:** **S12** is the item, and it is the real v0.1 gate. S11 is fully
specified now (Jeff's three rulings + the mechanism already in its row) and can
follow, or land alongside, once there is a graph for its restore path to
rebuild. Both rows say which comes first and why.

**Side effects on this box:** none — no code changed, nothing rebuilt, REAPER
not driven. Only TideSynth was committed in.

**Branch/PR:** this TideSynth PR (rows + entry only; no code).

---

## 2026-08-17 — macos — TIDE does not save the user's rack (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "keep working, no mercy" after
merging the thumbnails. I went after the smallest remaining follow-up
(`rackMode` on project load) and found something much larger on the way in.
Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** filed **S11** — **TIDE never persists or restores its document, so the
user's rack is lost the moment a project is reloaded.** No code change this
entry: the finding, its evidence, and the mechanism are the deliverable, and
the fix is a real feature that should be scoped deliberately rather than
started at the end of a long session.

**How it surfaced, which is the useful part.** U1c's follow-up asks what
happens to `rackMode` when a project is loaded, since the flag is serialised
(`s("rack_mode", rackMode)` in `SynthEditDocBase.h`). Following D4's lesson I
went to measure rather than reason — and the measurement kept coming back
wrong in a way that only made sense if **nothing loads a document at all.**

**The evidence, in three steps, each one cheap:**

1. **The saved state is 250 bytes of base64** for a project containing a placed
   List Entry in a rack (`GetTrackStateChunk` via ReaScript). A document with a
   module in it cannot fit in 250 bytes.
2. **Decoded, it is two parameters and nothing else** — the `.rpp`'s VST block
   reads `<Preset><Param id="1" val="0"/><Param id="0" val="0"/></Preset>`.
   Those are `controllerPtr` and `chunk`, both zero.
3. **Save → close → reopen → the module is gone.** The reloaded plug-in draws
   an empty rack: rails present (rack mode is set at document creation), no
   List Entry. Verified visually.

**The mechanism, so the row is actionable rather than alarming.** TIDE's XML
already declares the parameter this needs —
`<Parameter id="1" name="chunk" ignorePatchChange="true" datatype="blob"/>` —
and **nothing in the codebase ever writes it or reads it**;
`TideApp::InitInstance` unconditionally does `createNewDocument()` +
`OnNewDocument()`, so every instance starts empty by construction. The
controller's preset system (`MpController` / `DawPreset`) serialises
*parameter values*, which is exactly the two-param XML observed. The document
has its own serialisers already — `CSynthEditDocBase::ExportXml` /
`ImportXml` — so the shape of the fix is: export the document into that blob
parameter on save, import it back and rebuild the view on load.

**Why this is an architecture difference and not an oversight to be ashamed
of.** In a normal SynthEdit-exported plug-in the document IS the product: it
is baked in at export time and the chunk only has to carry knob values. TIDE
inverts that — **the document is what the user edits at runtime** — so it must
ride in the state. Nobody wrote that because nothing before TIDE needed it.
That framing belongs in the row so the next reader does not go looking for a
regression.

**What it means for the release, stated plainly:** the mac NEXT row said this
morning that the board was finished and the remaining question was v0.1. **It
still is, and this is now the answer**: a synthesiser that cannot save its
patch is not shippable, so **S11 blocks the R-series** more concretely than
"there is nothing to ship" did. That is a better problem than it sounds —
the question moved from "what should we build?" to "build this one thing".

**Also settled, and it retires a follow-up:** U1c's `rackMode`-on-load worry is
**moot in the form it was written**. Nothing loads a document, so nothing can
override the flag; the rack survives *because* the document is always fresh.
When S11 lands, the question becomes live again and S11's own work has to
answer it — noted in both rows so the retirement is not silently forgotten.

**Learned — chase the follow-up, find the feature.** The smallest item on the
list was the one that exposed the largest gap, because verifying it required
exercising a path (state round-trip) that no previous session had reason to
touch. **Six sessions of host verification never caught this**: every test
opened a fresh plug-in, and a fresh plug-in looks identical whether or not
persistence exists. The failure is only visible across a save/reload boundary,
which is a class of test worth adding deliberately rather than stumbling into.

**Next:** **S11** is the item, and it is Jeff's call how far to take it — the
row proposes the minimum honest version (round-trip the document through the
existing blob parameter) and lists the questions that need his answer, chiefly
what happens to the DSP graph on restore and whether patch-change should
reload the rack.

**Side effects on this box:** no code changed, nothing rebuilt. REAPER was
driven and a throwaway project was written to `/tmp/tide-persist-test.rpp` as
part of the test; **"Optimus HP" was never opened, saved or modified** — the
test script aborts if it sees that project active, which it checked and
reported.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — crumb thumbnails, and what they cost (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff picked the U1b crumb-thumbnail
follow-up off the list the previous entry left. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** wired the breadcrumb bar's thumbnails —
[SynthEdit#38](https://github.com/JeffMcClintock/SynthEdit/pull/38). **Verified
in REAPER: crumbs render the container's real content, and switching the master
from rack to structure view swaps the tile from the dark case to the light
structure grid with the placed Container visible inside it** — so it is
genuinely rendering the container, not drawing a placeholder.

**There was nothing to invent.** `BreadcrumbBar::renderThumbnail` has always
been the way in, `se_cl::renderContainerThumbnail` has been shared since it was
lifted out of SynthEdit2 ("which made thumbnails a Windows-only feature by
accident rather than by design"), and both the Wayland and WinUI editors
already install the callback. TIDE left it unset and silently got name-only
crumbs. **This is the fourth item in a row where the answer was wiring, not
building** — U1a, U1c, D6's content, and now this.

**The one interface change, and why it is two methods rather than one.**
`ISeApp` grows `setQuiet(bool)` returning the **previous** value. The offscreen
render walks the module factory, whose duplicate-module dialogs must be
suppressed around it; the Wayland version scopes `app_.quiet` directly, but
`ISeApp` exists to firewall SE SDK3 off from the GMPI side, so exposing the
application object to get at one bool would have been the wrong shape.
Returning the previous value means callers restore rather than assume `false`.

**Measured the cost rather than waving at it, because a plug-in pays for every
byte.** TIDE_VST3 went **10,149,744 → 10,414,832 bytes (+265,088, +2.6%)**.
Static-archive extraction did most of what C12e's rule predicts —
`EditorCommandDispatcher` is **not** linked (0 symbols) — **but
`SamplingProfiler` IS pulled in (8 symbols)** through `ScreenshotRenderer`.
That is the finding worth keeping: **the screenshot library is not free of its
tooling, and "only the members you reference" is true transitively, which is
not the same as "only the members you wanted".** If the cost is unwanted the
revert is two lines, and the PR says so.

**Learned — the strongest visual test is a CHANGE, not a picture.** A dark
thumbnail of a dark rack is indistinguishable from a black rectangle, and I
nearly recorded "it renders" on that basis. Switching the same container to its
structure view and watching the tile change to a light grid **containing the
module I had just placed** is proof that content is being rendered per
container and per view flag. Same discipline as yesterday's clipboard sentinel:
make the thing prove it changed, do not photograph it once.

**Next:** three small follow-ups remain from the finished-board list —
`rackMode` on project load (**U1c**), Windows/Linux clipboard for Copy link
(**D6**), and the win box's two **U2e** items — plus the **R-series**, which is
Jeff's call. The mac NEXT row's "do not invent scope" still stands.

**Side effects on this box:** two `TIDE_VST3` builds; the installed plug-in now
draws thumbnails. REAPER restarted once; **"Optimus HP" untouched** (it
reloaded on its own and was left alone). Only `SynthEdit` was committed in.

**Branch/PR:** this TideSynth PR +
[SynthEdit#38](https://github.com/JeffMcClintock/SynthEdit/pull/38).

---

## 2026-08-17 — macos — P10: the dead XML is gone (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged the D3/D6 stack, then
"sync repos, clean up branches, continue". Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** synced all five repos and deleted every merged local branch (only
`main`/`master` and Jeff's own release branches remain; **zero open PRs** in
all four repos at the start), then took **P10** — the item the NEXT row names
for exactly this situation — [SynthEdit#37](https://github.com/JeffMcClintock/SynthEdit/pull/37).
`SynthEditSem/SynthEdit.xml` and its `SynthEdit.rc` resource line are deleted.

**Why the file had to go, restated because it is the trap P5 nearly fell
into:** the file looks like the source of truth — 12 lines, named after the
plug-in, holding `id` and `name` — and **is not what ships**. The live identity
is the embedded XML string literal in `SynthEdit.cpp`'s
`getPluginInformation()`. Editing the `.xml` alone changes nothing at runtime
on any platform: a no-op PR that reviews as correct.

**Re-verified before deleting rather than trusting the row, and the discipline
mattered twice this week.** D4's central measurement turned out false
yesterday, so P10 got the same treatment: only two references exist (the `.rc`
line and an explanatory comment in `SynthEdit.cpp`), and the sole loader sits
inside `#if 0`. **The near-miss worth recording: the loader's first visible
guard is `#if _WIN32` at `MyVstPluginFactory.cpp:472`, which reads as live —
the `#if 0` that kills it is the *enclosing* one at `:462`.** I read the inner
guard first and briefly concluded the row was wrong, exactly as I had concluded
about D4. Checking the enclosing guard settled it in one command. **When a
"this code is dead" claim rests on a preprocessor guard, find the outermost
one, not the nearest.**

**Accept met, both halves:** TIDE_VST3 and SynthEdit_VST3 build on macOS, and
the built binary's identity is byte-identical — `id="SE SynthEdit"
name="TIDE Rack" vendor="TIDE Synth"`, the strings P5 put there.

**The limit this box cannot close, stated rather than glossed:** `.rc` files
are Windows-only, so the deletion is verified *consistent* here but the Windows
resource compile is unexercised. It should be trivially fine — the only line
naming the file goes with the file — but the Windows box is the real check, and
the PR says so.

**Next:** with P10 done, the mac backlog has **no remaining item a scheduled
run should take on its own initiative**. What is left is either Jeff's call
(the R-series, all blocked on there being something to ship) or small
follow-ups already recorded in their rows: crumb thumbnails (U1b), `rackMode`
on project load (U1c), Windows/Linux clipboard for Copy link (D6), and the win
box's two U2e items. **That is a genuinely finished board rather than a tired
one**, and the NEXT row now says so in those words so the next run does not
invent scope to fill the gap.

**Side effects on this box:** two `TIDE_VST3` builds and one `SynthEdit_VST3`
build; the installed plug-in is current. REAPER was not driven this entry.
Only `SynthEdit` was committed in; the other four repos were read only and are
clean on their default branches.

**Branch/PR:** this TideSynth PR +
[SynthEdit#37](https://github.com/JeffMcClintock/SynthEdit/pull/37).
