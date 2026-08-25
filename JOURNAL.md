# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-25 — windows — the DSP→GUI return path exists now: the VCV LFO's LED blinks, and so can every meter, scope and light TIDE ever ships (interactive, Jeff directing)

**Prompt:** back to LFOs. Get it working for VST3, "C:\SE\SE16\se_vst3" might
provide inspiration.

**Did:** Built the return path this morning's entry proved was missing. TIDE's
inner rack was queueing parameter feedback into `queDspToUi` and nothing read
it — for ANY module, not just the VCV ports. It flows now, end to end, in the
VST3 (and every other format, for free — the mechanism is wrapper-agnostic).

**The design, and why it is this shape rather than a direct call.** The
processor and the editor are separate objects, and under AUv3 separate
PROCESSES, so nothing crosses as a pointer. The one channel that already
works between them is a GMPI **blob parameter** — which is exactly what the
chunk (parameter 1) uses to carry the document DOWN. So the return path is
the same route in reverse, on a new **parameter 2**:

1. `SynthEdit::drainRackFeedback()` (SynthEditSem/SynthEdit.cpp), called at
   the end of `subProcess` — `rack.process()` ends in
   `SeAudioMaster::PostProcess → ServiceDspWaiters2`, which serialises every
   pending update into `MessageQueToGui()` = `queDspToUi`. Drain it and send
   the bytes out `pinFeedback`.
2. `gmpi_processor::setPin` sees an output blob pin with a parameterId,
   queues it; the wrapper ships it to the controller;
   `gmpi_controller_holder` delivers it to each editor's matching GUI pin as
   a `"ppc3"` message. **All of this already existed** — nothing in GMPI or
   the wrappers was touched.
3. `SynthEditGui::notifyPin` (pin 1) hands the bytes to `ISeApp`.
4. `TideApp::receiveRackFeedback` → `SynthRuntime_editor::receiveDspMessages`
   (new, SynthEditLib branch `tide/win/dsp-to-gui-feedback`) → the SAME queue
   and reader `serviceQueues()` uses → `onQueMessageReady` → routes by handle
   to the PatchParameter → `UpdateGui` → the editor's light pins.

**THE BYTES ARE FORWARDED VERBATIM, framing included** — that is the load-bearing
decision. `queDspToUi` changed from `interThreadQue` to `lock_free_fifo` for
one reason: `siphon2()` hands out a pointer to the queued bytes without
decoding them, so this is a copy rather than a translation and the editor
side reuses the identical reader. Re-framing would mean implementing the
protocol twice and keeping the two in step forever. This is precisely what
`Processor_VST3::CommunicationProc` does with its own DSP→controller queue —
the "inspiration" prompt pointed at `SE16/se_vst3`, and the pattern worth
copying turned out to be in its GMPI successor.

**Verified on the running rack, three ways.** Trace: `light 0 update #100
value 0.750` — live CHANGING values arriving, against the frozen `#0 value
0.000` this morning. Pixels: six settle-spaced screenshots diffed, ~1000
changed pixels per frame, all in one tile at the LFO's light — against
**zero** across the whole rack before. Eye: the FREQ LED lit amber, OFST lit,
and S&H's IN indicator live. Then the VST3 target itself built and installed
(as a real bundle folder, per the fix earlier today) with the same code.

**A trap that cost two wrong measurements, and it is the same one as this
morning:** `cmake --build` returns **exit 0 having linked nothing** when the
running app holds the .exe — the failure is `LNK1104`, which `grep ': error'`
does not match. Both times, the "after" reading was really the "before"
binary. **Check the exe's mtime changed; do not trust the exit code alone.**
Kill the app before every rebuild.

**Scope, honestly:** this makes the CHANNEL work and the lights prove it.
Whether every kind of feedback (Scope's 64KB captures, which are multi-part
messages) survives the trip is not yet measured — the machinery handles
oversize sends and the framing is preserved, so it should, but "should" is
not "did".

## 2026-08-25 — windows — the installed VST3 had no resources: local dev builds now assemble a real bundle folder in Common Files\VST3 (interactive, Jeff directing)

**Prompt:** a screenshot of Ableton Live showing "Can't open file
C:\Program Files\Common Files\VST3\MidiCv.synthedit, error =
XML_ERROR_FILE_NOT_FOUND", captioned "TIDE VST3 in Ableton Live". Then,
mid-diagnosis, when offered a one-off fix vs a systemic one: "Developer local
builds need to produce a working plugin."

**Root cause, traced through the actual resolution code, not guessed.** The
file sitting at `Common Files\VST3\TIDE-Rack.vst3` was a LOOSE FILE
(16,381,440 bytes — byte-identical to that morning's
`build\SynthEditSem\Debug\TIDE-Rack.vst3`), dropped there automatically by
`gmpi_plugin.cmake`'s `copy_plugin()` under `SE_LOCAL_BUILD=ON` (confirmed
`ON` in Jeff's actual build-tree cache). A loose file's own path has no
`Contents` component, so `BundleInfo::pluginIsBundle` reads false,
`getResourceFolder()` falls back to `getImbeddedFileFolder()` — the DLL's
bare containing directory, no subfolder — and `TideApp::ResolveFilename`'s
candidate (`resourceFolder/Prefabs/MidiCv.synthedit`) is absent there, so it
falls through to the base resolver's `GetHomeDir()`, which computes the same
bare directory again. Final path: `C:\Program Files\Common
Files\VST3\MidiCv.synthedit` — the exact string in the dialog.

This was never a one-off mistake. `SE_LOCAL_BUILD`'s Windows VST3 copy has
always been binary-only (`copy_plugin()`'s WIN32 arm is one `copy /Y` of the
`.vst3`), so it has been dropping a resource-less loose file into the SHARED
`Common Files\VST3\` folder on every local build since prefabs existed —
BACKLOG S36 (`SynthEditSem/CMakeLists.txt:617`) already documents that a
non-bundled Windows VST3's resource folder IS its own bare directory, but
nobody had connected that to the SE_LOCAL_BUILD auto-install path landing
the binary somewhere with no resources beside it at all.
`scripts/package-windows.ps1` already solves this correctly for the SHIPPED
installer — it assembles a real `Contents\x86_64-win\...` bundle folder and
its own comments warn *"Copy the FOLDER, not just the file inside it"* — but
that script is a manual, occasional step, not part of the everyday
build-and-run loop.

**The fix, in two repos.**

- **GMPI** (`gmpi_plugin.cmake`, branch `tide/win/no-local-vst3-copy-flag`):
  a new `NO_LOCAL_VST3_COPY` option on `gmpi_plugin()`. Unset by default —
  every other Windows VST3 plugin using this shared macro is untouched.
  A plugin that needs more beside its binary than a flat copy can opt out
  and own its whole local-install step instead of fighting a collision
  between a loose file and a same-named folder.
- **TideSynth** (`SynthEditSem/CMakeLists.txt`): passes the new flag, and
  the per-format POST_BUILD loop grows a `WIN32 AND SE_LOCAL_BUILD AND
  _fmt STREQUAL "VST3"` branch — the exact Windows sibling of the
  `_tide_installed` block already there for macOS (added for the same
  POST_BUILD-ordering reason: `copy_plugin()`/this step both run as
  POST_BUILD, in call order). It `rm -rf`s whatever is at the destination
  first (a stale loose file or a stale folder from a prior build must not
  collide with or hide inside the new one), then assembles
  `TIDE-Rack.vst3\Contents\x86_64-win\TIDE-Rack.vst3` +
  `Contents\Resources\{Prefabs,*.xml}` — the identical shape
  `package-windows.ps1` produces for release, now automatic on every local
  build.

**Verified by tracing the resolution chain against the real output, not by
assuming the shape is right.** Built `TIDE_Rack_VST3` with
`SE_LOCAL_BUILD=ON`: the POST_BUILD step ran clean, and
`Common Files\VST3\TIDE-Rack.vst3\Contents\Resources\Prefabs\MidiCv.synthedit`
now exists, byte-identical to `RackModules\MidiCv.synthedit` (`diff -q`).
Walked `BundleInfo::pluginIsBundle`'s component scan by hand against the new
nested path (`...\TIDE-Rack.vst3\Contents\x86_64-win\TIDE-Rack.vst3`): the
`Contents` component now follows a component with `.vst3`'s extension, so
`pluginIsBundle` reads true and `getResourceFolder()` computes exactly
`...\TIDE-Rack.vst3\Contents\Resources\` — the folder that now holds the
file. The stale pre-rename `TIDE_VST3.vst3` loose file is still sitting in
`Common Files\VST3\` too, untouched by this fix; flagged for Jeff rather
than deleted from a shared system folder unasked.

**Not yet confirmed:** Jeff reopening Ableton Live and reloading TIDE — the
only check this session could not run itself.

## 2026-08-25 — windows — "should that VCV LFO LED be blinking?" — yes, and the answer is TIDE's thin-slice gap, measured hop by hop (interactive, Jeff directing)

**Prompt:** hey, should that VCV LFO LED be blinking? (and, mid-turn:
remember to add logging/instrumentation if needed)

**The answer:** it should (VCV's LFO phase light), it does not, and the
module is innocent. Six settle-spaced screenshots diffed to ZERO changed
pixels across the whole rack — nothing animates. Instrumenting BOTH ends of
the light pipeline (adaptor branch `light-liveness`, gated
RACK_ADAPTOR_TRACE) and then walking SynthEditLib hop by hop with throwaway
fprintfs (all reverted) pinned the break precisely:

1. The LFO's processor RUNS — the culling hypothesis (no Output module in
   the rack, SE builds pull-from-sink) was REFUTED by trace: nine voice
   clones all construct and process.
2. It computes light brightness 0.975 and `sendLights` queues an update
   every block. The generated XML is correct (direction="out" pins,
   private non-persistent parameters).
3. SynthEditLib forwards every one: `ug_gmpi::setPin` → param watcher →
   `UpdateOutputParameter` → `UpdateUI` — 1054 queued in 12 s...
4. ...into the processor's `queDspToUi`, **whose reader does not exist**.
   SynthEditSem/SynthEdit.cpp:84 says so in its own words: *"Nothing drains
   the GUI-bound queue yet (parameters don't flow in the thin slice)"*.
   Editor-side `ppc` arrivals: ZERO. The editor's light pins got exactly
   the initial 0.000 each and never another value.

So DSP→GUI parameter feedback has NEVER flowed in TIDE — for any module.
Lights, meters, and Scope's display-state are all behind this one gap; the
VCV ports are merely the first thing on the rack that visibly animates from
the DSP. Closing it is design work: the `ppc` stream has to cross the
processor/controller split in a way that survives AUv3's process boundary
(the chunk parameter's route), then land in the editor's patch manager.
Filed in docs/vcv-fundamental.md as a known limitation with the measured
chain.

**A dead end walked so nobody re-walks it:** TIDE also never creates the
base InitInstance's LAST line — `timerhelper = new AppTimerHelper(this)` —
the wholesale-override loss class U2e documents, third instance. Restoring
it makes `CSynthEditAppBase::OnTimer` tick (measured 0 → ~40/s) and
`serviceQueues()` run… and changes nothing here, because that circuit
drains the EDITOR-runtime queue pair, which TIDE's processor/controller
split never uses. Reverted rather than shipped as a dead 24 ms timer;
whoever closes the real gap should decide whether the app timer comes back
as part of the actual delivery path.

**Traps for the next instrumented session, both self-inflicted today:**
`': error'` does NOT match MSBuild's `': fatal error LNK1104'` (locked exe
while the app runs), so three "successful" rebuilds shipped a stale binary
and one measurement (OnTimer=0 "after" the fix) was measuring the
instrumentation's ABSENCE — always verify the exe mtime changed. And this
harness's bash transport eats one backslash level: `\n` inside a heredoc'd
C string arrives as a real newline; build the escape as `chr(92)+'n'`.
## 2026-08-25 - macos - It makes noise: audio measured through BOTH the VST3 and the AUv3

**Prompt:** interactive

**Did:** Closed the oldest open item of the day - *"make noise"* - for both
formats, with numbers rather than listening.

**VST3, via E2a's REAPER harness, on current `main`:**

| fixture | cables | result |
|---|---|---|
| `v1-rack.rpp` | 2 | **-6.3 / -17.0 dBFS** - audio present |
| `v1-rack-uncabled.rpp` | 0 | **-inf** - silence (negative control) |
| `v1-rack-midi.rpp` | 4 | -6.3 / -17.0 |
| `v3-midi-pitch.rpp` | 4 | -6.2 / -21.1 |
| `v3-midi-gate.rpp` | 3 | -6.3 / -21.2 |

`-6.3 / -17.0` **matches E2a's August figures exactly**, so nothing regressed
through the day's five merges. No GUI driving needed - `REAPER -renderproject`
renders headlessly and exits, as the harness's own docstring says.

**AUv3, in GarageBand - the one that had never been done.** Jeff: *"why not AU3
in Garageband"*, which was the right call; my instinct to build a sixth REAPER
fixture was over-engineering when the host was already open. Built the patch by
hand in a **fresh project** - deliberately not a restored session, the trap that
produced this run's earlier false negative - then exported and measured:

```
2ch  16-bit  44100 Hz  124.8 s
peak=  -0.1 dBFS   rms=  -5.2 dBFS  -> AUDIO PRESENT
approx fundamental (zero-crossing): 440.0 Hz
```

**440.0 Hz is the same pitch E2a measured through the VST3** - 5 V at 1 V/oct is
middle A - so the oscillator free-runs at its default and the AUv3 carries it to
the host output. Human confirms: *"yep. human hears it"*.

**Patch cabling in AU3 is now confirmed twice**: by Jeff manually, and by this
run pulling both cables itself - **Osc out -> Output in** and
**MIDI-CV PITCH -> Osc Pitch**.

**Correction: I wrote that the two cable colours encode signal type - red for
audio, orange for CV. That is WRONG.** Jeff: *"the colors differ randomly
btw"*. The colours are arbitrary per cable and mean nothing. I saw two cables,
two colours, and two signal types and invented a rule from a coincidence -
the same reach-past-the-evidence habit that cost this run an hour earlier.
**Do not read a connection's type off its colour.**

**Why this matters beyond "it works": it is the M6 gap closed by CONTENT.**
`auval` passed a completely empty plugin for days (M5) because it validates the
interface and never asks whether the rack contains anything. A rendered file
with a measurable fundamental cannot be fooled that way. Export artifact left at
`~/Desktop/tide-au3-test.wav`.

**Not verified:** AU2, CLAP, iOS.

## 2026-08-25 — windows — the VCV Fundamental ports run inside TIDE, behind an option that is OFF by default (interactive, Jeff directing)

**Prompt:** enhance TIDE Rack so it can include the Cardinal fundamental
modules ("C:\SE\VCV_Fundamental_gmpi"). cmake will need to fetch them plus the
GMPI Rack Adaptor ("C:\SE\SynthEdit_Rack_Adaptor"). But this all needs to be
enabled or disabled in the cmake (disabled by default) and to completely omit
this stuff when disabled (for licensing reasons). The rack adaptor may need
updating in order to put the Cardinal modules in the correct synthedit
category (to show up as rack-compatible).

**Did — three repos, one option.** `TIDE_VCV_FUNDAMENTAL` (root CMakeLists,
OFF) fetches SynthEdit_Rack_Adaptor and VCV_Fundamental_gmpi (with
`*_FOLDER_OVERRIDE` escape hatches matching the standing four) and statically
links all 38 ported modules into every TIDE format target. OFF means ABSENT:
nothing fetched, compiled or linked, the TideApp call site compiles out, and
`grep -i vcv` on a default configure prints nothing — measured, both before
and after the feature build. ON means GPL-3.0-or-later binaries, which is the
whole reason for the default. docs/vcv-fundamental.md is the write-up.

**The mechanism, because none of the .gmpi machinery could be reused.** TIDE
has no scan (S1a) and no factory of its own — SynthEditLib defines
`gmpi::RegisterPlugin/RegisterPluginWithXml` straight into `ModuleFactory()`
(UgDatabase.cpp:198), and TIDE compiles with GMPI_DISABLE_FACTORY already. So
the adaptor's RackFactory.cpp (which supplies those very symbols plus
MP_GetFactory for a .gmpi) can never link into TIDE. New in the adaptor:
`RackFactoryStatic.cpp` behind `RACK_ADAPTOR_STATIC_HOST=ON` — QUEUES each
module's deferred-XML registration at static init, and
`rack_adaptor::registerDeferredModules()` (called from TideApp::InitInstance,
once per process) generates the XML and registers it, landing both halves at
once. The XML cannot be generated at static-init time because a module's
`RACK_DISPLAY_STATE` is declared after upstream's .cpp registers — the same
reason the .gmpi factory serves XML lazily. New in VCV_Fundamental_gmpi:
`static_library/` builds every module as an OBJECT library (registration is a
static initializer; archive members dead-strip, objects cannot), delivered to
each plugin target as `$<TARGET_OBJECTS>` INTERFACE sources — an OBJECT
library's objects do NOT travel through an INTERFACE `target_link_libraries`
link; the first build measured exactly that by dropping all 39 registrations.

**The category ruling asked for:** the rack browser scope
(`ModuleScope::RackOnly`, EditorLib/SynthEditAppBase.cpp, V4) lists prefabs
plus modules whose category STARTS "Rack", case-insensitive, nothing else. The
adaptor default is now "Rack/VCV" and the Fundamental repo registers
"Rack/VCV Fundamental" (one shared cmake/RackModuleMetadata.cmake so the two
entry points cannot drift). Verified in the running standalone: the browser
shows Rack → VCV Fundamental with all modules listed.

**Two bugs found by running it, both with Jeff driving the rack live:**

1. **Every VCV module dropped as an artless 100x100 square.** The generated
   panel registrations were `inline const bool panel0` — the SAME symbol in
   every module TU. One binary per module hid it; 38 TUs in one binary
   COLLAPSED to a single copy at link, so one panel registered and 37 modules
   fell back to panelMetrics' 100x100 default. Fix in
   cmake/RackPanelResources.cmake: `static` (internal linkage) + per-target
   names. The adaptor also gained a rack-size floor (a 3U panel is never
   shorter than RACK_GRID_HEIGHT — Jeff's ruling "always at least one rack
   height"), stderr warnings on the previously-Release-silent failure paths,
   and a RACK_ADAPTOR_TRACE per-editor line (slug/model/art/size) — which is
   what verified the fix: `'LFO' … size=135x380`, `'SHASR' … 105x380`,
   `'Pulses' … 120x380`, art=yes on all three, restored from Jeff's own
   session.

2. **#404 left TIDE's own patch points phantom — [#406](https://github.com/JeffMcClintock/TideSynth/pull/406), split out as its own PR.**
   Session restore blocked on "don't have some required SEM Modules: TiDE
   Patch Point Out" (read by window-text enumeration, since the dialog is
   modal and the app hangs under it). The pair existed only as the modules/
   tree's loadable .gmpi — the editor scans it, TIDE cannot — so every
   upgraded prefab's jacks were phantoms in TIDE while all of #404's
   editor-side verification passed. e2a-prefabs §9.2 names this exact trap;
   its "authoritative check is placing the prefab in TIDE and looking" is the
   step #404 skipped. Fix: PatchPoint.cpp/PatchPointGui.cpp join the source
   list (the SE Label one-line case).

**Session-restore side effect worth knowing:** the startup watchdog treats a
dialog-blocked run as "did not finish starting" and quarantines session.xml
to session.previous.xml. Jeff's three-module rack survived as the quarantine
copy and was restored by copying it back before relaunch.

**Merge order:** the two side-repo branches (SynthEdit_Rack_Adaptor
`static-host`, VCV_Fundamental_gmpi `static-library`) must merge before
TIDE's option can build in fetch mode — TIDE fetches both at `origin/main`.
Local-override builds work off the branches today.
## 2026-08-25 — macos — Patch cables were dead because the module was never compiled in; verified fixed in AU3

**Prompt:** interactive

**Did:** Chased why patch cables would not pull in the AUv3, was wrong three
times, and the windows box found it: **`TiDE Patch Point In/Out` was not
compiled into the binary at all.** #404 switched every rack prefab to the TiDE
patch points, but `modules/PatchPoint/PatchPoint.cpp` and `PatchPointGui.cpp`
were never in `SynthEditSem/CMakeLists.txt`'s source list — **E2a's both-halves
rule**, XML present and `.cpp` absent, an insertable phantom. Fixed on `main` by
`479faff2`. **Jeff verified by hand: *"manual test passed. Patch cables working
in AU3"*.**

**Measured at the binary, before and after.** The appex installed in
`~/Applications` carried **0** `PatchPointGui` symbols; after the fix, **13** —
with all four static initialisers present (`PatchPoint.cpp`, `PatchPointGui.cpp`
and the older plural `PatchPoints*` pair) and **identical counts in the
standalone and the appex**, so nothing was dead-stripped and the fix reached
every format.

**One missing module, three symptoms that looked unrelated:**

| observation | cause |
|---|---|
| module faces drew bare, no jacks | no `PatchPointGui::render` |
| drag moved the module instead of pulling a cable | no `PatchPointGui::hitTest` — click fell through to the container |
| knobs still worked | different module, linked fine |

Jeff's *"turning the knobs works in AU3"* is what separated "input is broken"
from "one module is missing", and it was the turning point.

**THE MISTAKE WORTH KEEPING: a false negative sent this run down an
hour-long wrong path.** The standalone appeared to cable fine, so the bug was
written up as **AU3-specific**. It was not. The standalone had **restored a
saved session**, so the modules being cabled were old `SE Patch Point` instances
persisted in the document — not the new TiDE ones at all. **A UI test that
reuses restored state is not a negative control.** Insert fresh, or start a new
document, before concluding any format difference.

**Three further theories, all eliminated by measurement, recorded so nobody
re-runs them:** #404's prefab **data** (the coordinates line up exactly —
`jack 24 80`/`24 337` match the patch-point `panelRect` centres, and #404
touched no C++); **`SE TiDE:Panel` z-order** (the Panel is listed last in all
three prefabs, but that is not the cause); and **this box's own `s_xmlMerged`
guard** from M5 — the hit-test probe printed `patchPoints=PRESENT count=0` on
the container in the **working** case, so that list was never the mechanism.

**The probe that finally aimed things correctly was Jeff's suggestion** — "add a
bright orange circle to the patch-point render method to see if it even
exists". Instrumenting the hit-test rather than the renderer gave the number
that killed my last theory.

**Not verified:** VST3/CLAP/AU2, and whether a cabled rack actually **sounds** —
audio has still never been measured through the AUv3.

## 2026-08-25 — macos — The Release question, measured: auval passes a completely empty plugin

**Prompt:** interactive

**Did:** M5 left one thing reasoned rather than measured — whether a **Release**
build of the AUv3 would crash (as Debug did) or degrade silently, since
`-DNDEBUG` compiles both failing asserts out. Jeff: *"let's do it"*. Cold
Release configure at the **pre-fix** commit, `-O3 -DNDEBUG` confirmed in the
real compile flags, both fixes reverted surgically.

**The answer is the worse one: it was a live shipping defect.**

```
auval -v aumu Drck Dsyh   ->  exit 0,  AU VALIDATION SUCCEEDED,  no crash
```

...while the extension reported, in its own words:

```
TIDE: ControlsXp.xml missing from bundle resources - those controls will have no pins
TIDE: MidiPlayer2.xml missing from bundle resources - those controls will have no pins
TIDE: Converters.xml missing from bundle resources - those controls will have no pins
TIDE: VaFilters.xml missing from bundle resources - those controls will have no pins
TIDE: no Prefabs folder in bundle resources - the rack module browser will be empty
TIDE: MidiCv.synthedit did not insert a container - the rack will have no MIDI jacks
```

**The AUv3 has been loading, validating and running as an EMPTY RACK.** No
pins, no browser contents, no MIDI jacks — and `auval` never said a word,
because it validates the AU *interface* and never asks whether the plugin
contains anything. **That reconciles M4's green of 2026-08-23: it was masking
this, not contradicting it.**

**How to read an appex's stderr, because the next run will need it.** It
reaches neither the terminal nor the unified log (`log stream` carries os_log
only, not stderr), and the extension is **sandboxed**, so `/tmp` is not `/tmp`.
`freopen(getenv("HOME") + "/tide-au3-diag.log")` inside `InitInstance` lands in
`~/Library/Containers/<ext-id>/Data/` and is readable from outside. Logic
untouched; removed before the clean redeploy.

**Bug 2's premise is now measured, not inferred:** the diagnostic prints
`=== InitInstance ===` **twice per extension process**. That is the double-scan
the `s_xmlMerged` guard exists to stop.

**The full matrix, six runs:**

| build | BundleInfo | guard | auval | what the plug-in actually contained |
|---|---|---|---|---|
| Debug | — | — | `FATAL 4099` | abort in `LoadPrefab` |
| Debug | yes | — | `FATAL 4097` | abort in `RegisterParameters` |
| Debug | yes | yes | SUCCEEDED | working |
| Release | — | — | **SUCCEEDED** | **EMPTY — no pins, no prefabs, no MIDI jacks** |
| Release | yes | — | SUCCEEDED | working |
| Release | yes | yes | SUCCEEDED | working — 4 XMLs enriched, **9 prefabs**, root MIDI-CV |

**So the two fixes are not equally load-bearing.** `BundleInfo` is the one that
matters in Release. The `s_xmlMerged` guard is what stops the **Debug** abort;
in Release the re-scan is harmless at runtime, so it is defensive rather than
load-bearing. Worth knowing before anyone "simplifies" either one away.

**The process finding is the bigger one, filed as M6:** `auval` passed an empty
rack for days. It is not a sufficient shipping gate for TIDE, and TIDE already
prints everything a real gate would need — it just prints it where nothing
reads.

**AND IT WORKS IN GARAGEBAND.** Jeff: *"try it in garageband"*. The clean
`main` Release AUv3 loads in GarageBand and the editor draws **populated**: the
browser shows the Prefabs group with **exactly 9 entries**, and the rack holds
the **root MIDI-CV with PITCH / GATE / VEL / TRIG**. That is the precise inverse
of the pre-fix measurement above — and it is also **V4's rack filter running in
a real host** rather than only the standalone.

Closing the plug-in window and reopening redraws correctly, the extension
process stays alive, and the crash-report count is **unchanged at 10** across
the session — so M4's teardown fix still holds.

**Audio was NOT tested.** The seeded rack contains only the root MIDI-CV;
scrolling shows bare rails either side, so there is no oscillator or output and
no sound path. Testing that means building and cabling a patch inside Jeff's own
GarageBand project, which this run did not do.

**Verified after:** a clean `main` Release AUv3, no diagnostics, is installed at
`~/Applications` and passes `auval`. **Not verified:** any real host — still
`auval` only; AU2; iOS.

## 2026-08-25 — windows — every rack prefab is on the TiDE panel pattern, and a script now enforces it (interactive, Jeff directing)

**Prompt:** we need to clean up the TIDE prefabs, look at the new style
(favoring modules from the "TiDE" category). Understand how the Panel module
works to draw backgrounds behind patch-points and knobs. then upgrade the
stragglers.

**Did:** Upgraded the six stragglers docs/tide-panel-layout.md surveyed on
2026-08-24 — `Envelope`, `Filter`, `Midi`, `Oscillator`, `Output` (no panel,
stock patch points) and `MidiCv` (panel, but stock patch points and the pin's
untouched demo `Layout`) — to the pattern the hand-tweaked references
(`AR_jef`, `Output_jef`, `Sine_jef`) establish: an inert `SE TiDE:Panel`
painting the faceplate, `TiDE Patch Point In/Out` layered over its painted
jacks (they draw nothing — the stock `SE Patch Point` modules paint their own
jack ON TOP of the panel's, which is why they had to go), `SE Label` for text,
`rack_module="true"` on the container, and the container resized from the old
generator's 100x160 slot to E5's ruled 48x384. Hand-edited the XML rather than
regenerating: `build-prefabs.py` is HISTORICAL by Jeff's 2026-08-24 ruling and
recomputes placement from its own tables, which would discard the reference
files' hand-tweaked layouts.

**Geometry, taken from the references rather than invented:** `grill 24 20`;
title label centred (24,50); input jacks from y=80 at 40-DIP pitch, each label
20 below its jack centre; output jacks at 337 (plus 297 for Output's stereo
pair, matching `Output_jef` exactly); `slots 24 371`. Outputs stay unlabelled
(position says what they are — `AR_jef`/`Output_jef` precedent) except
`Midi`'s Gate/Trig pair, which are indistinguishable without text. `MidiCv`
kept its 96-wide two-column arrangement; its `Layout` now paints rings under
the four real jack centres (80, 54/124/194/264) instead of the demo default's
two phantoms at 24,297/337. Pin defaults the E1 cases lock (`Envelope` gate
10, `Oscillator` pitch 5) carried over — the TiDE patch points share the stock
ones' pin order, so the `<lines>` and defaults port unchanged.

**Verified by measurement, not eyeball.** New `scripts/check-prefab-layout.py`
parses every prefab and asserts the tide-panel-layout contract: Layout
jack/knob entries == functional module centres (both directions, relative to
the panel's top-left), panel == RackUnits x 48 by 384, no stock patch points
beside a TiDE panel, PluginList complete, half-DIP grid, nothing overhanging
the plate. All nine pass. Negative controls: the pre-upgrade `Envelope` fails
old-style, pre-upgrade `MidiCv` fails on stock jacks + unset Layout, and a
sabotaged copy with one jack moved 1 DIP is caught as a mismatch. Then each
file was loaded in SynthEditCL via the MCP: `se_dump` (panelRectsFinalized)
reports every container measuring exactly 48x384 (96 for MidiCv) and every
jack/label at its authored rect — the SubView-measure union trap
(e2a-prefabs §9.1/build-prefabs.py) did not fire — and settle-checked panel
screenshots (two consecutive byte-identical shots, per the render-verification
lesson) show plate, grill, pocketed jacks, labels and slots where authored.

**One check deliberately looser than first written:** the checker originally
required the container's `PanelWndPosition`/`panel_rect` to equal the panel
size and flagged all three hand-authored references, which carry stale editor
values and demonstrably work — the view rewrites the rect to the union of
visible children on the first panel pass. The real §9.1 trap is a ZERO-size
`PanelWndPosition`, so that is what it checks now.

**Not done, on purpose:** `lint.yml` is untouched — its own header says
workflow edits are deliberate; adding a one-line step running
`check-prefab-layout.py` is a cheap follow-up if Jeff wants it enforced.
`Midi`'s `MIDI In` keeps its odd far-canvas panelRect (normalised 8x14 →
0-size; the editor still measures and parks it at the canvas centre, off the
48-wide window, exactly as before the edit — harmless and unchanged in
behaviour). The `_jef` files are untouched per the hand-maintained ruling.

## 2026-08-25 — windows — the unpushed half was pushed, and `main` is green on all three platforms (interactive, Jeff directing)

**Prompt:** push to main branches

**Did:** The entry below this one is the scheduled run that found `main` red on
all three platforms, measured the cause, and **deliberately did not fix it**
because the fix was Jeff's uncommitted working-tree change and STEP 5 forbids a
run from committing that. Jeff read the report and answered *"push to main
branches"*, which is the authorisation the scheduled run did not have. So the
two files went in as
[SynthEditLib `af42bd6`](https://github.com/JeffMcClintock/SynthEditLib/commit/af42bd6),
authored as Jeff — it is his change — with the pairing to TideSynth
`c0c79818c` spelled out in the commit message, because a commit that only makes
sense next to a commit in another repo should say so.

**Result — green, verified two ways rather than declared.**

1. **CI on the real runners.** Re-ran the failed build
   ([32776912943](https://github.com/JeffMcClintock/TideSynth/actions/runs/32776912943))
   against the **same TideSynth tree**, changing nothing but what
   `GIT_TAG origin/main` resolves to in SynthEditLib:
   `macos=success`, `linux=success`, `windows=success`. All three legs that
   failed. Re-running rather than pushing a new commit is what makes this an
   isolation of the one variable instead of a fresh unrelated build.
2. **Locally, the same A/B that diagnosed it.** `cl /Zs` on `TideApp.cpp`
   against a fresh `git archive origin/main` of SynthEditLib: **`C3668` before
   the push, 0 errors after**, same command, same flags.

**[#392](https://github.com/JeffMcClintock/TideSynth/issues/392) and
[#393](https://github.com/JeffMcClintock/TideSynth/issues/393) CLOSED
THEMSELVES on that green run.** That is `build.yml`'s close-on-success step
firing in place, which is worth recording as evidence in its own right — S41's
row and A33's both turn on whether these mechanisms actually run, and most of
the prior cases were closed by hand and so proved nothing.
[#394](https://github.com/JeffMcClintock/TideSynth/issues/394) had to be closed
by hand, because the windows leg is excluded from that automation by the same
`matrix.platform != 'win'` condition that stopped it filing the issue in the
first place — **the exclusion cuts both ways, and this is the second half of it
showing up within one session.**

**WHAT DID NOT CHANGE, AND SHOULD NOT BE READ AS SOFTENED.** A human being
present is exactly what made this legitimate, and the scheduled run was right to
stop. The rule it obeyed — *never commit the developer's work in progress* —
did not bend; it was **discharged by the developer authorising the push**. A
future run that finds an uncommitted fix in a dirty tree still may not commit
it, and the fact that this one turned out to be wanted is not evidence the rule
is too strict. The strongest argument for that is what the run bought by
stopping: it handed Jeff a two-line answer with an A/B behind it instead of a
merge conflict with a change he was holding.

**The amplification was real and is the argument for fixing this class fast.**
While `main` was red, every branch cut from it inherited the error and filed up
to two more CI issues of its own — #396/#397, #398/#399 and #402 in about forty
minutes, so **six of nine open issues on the repo were one missing push**. That
is also why the scheduled run stopped at two PRs: each push it made created two
more issues. A red default branch is not a static cost.

**Left open on purpose, and this is unchanged by the fix:**
[#398](https://github.com/JeffMcClintock/TideSynth/issues/398),
[#399](https://github.com/JeffMcClintock/TideSynth/issues/399) and
[#402](https://github.com/JeffMcClintock/TideSynth/issues/402) sit on branches
that have merged and been deleted, which is exactly the shape A33's watchdog
sweep targets. A33's row asks a run to find out whether that sweep actually
fires, and records that every previous case was hand-closed and so is not
evidence. Closing them now would destroy the test a second time for the sake of
tidying three issues. **The 2026-08-25 06:00 UTC firing is the check.**

**Learned:**

1. **A rule that stops a run short is not thereby a rule that cost the fleet
   anything.** Stopping produced a measured diagnosis and a two-line fix ready
   for a human; committing would have produced a conflict with a change the
   developer was holding. Judge the rule by what the run handed over, not by
   whether the work was ultimately wanted.
2. **Re-run the failed run rather than pushing a new commit when the variable
   you changed lives in another repo.** `GIT_TAG origin/main` re-resolves on a
   re-run, so the same tree builds against the new sibling and the result
   isolates one variable. A fresh commit would have proved "some newer state
   builds", which is a much weaker claim.
3. **`build.yml`'s close-on-success step works, observed in place** — #392 and
   #393 closed themselves on the green re-run. That is real evidence for the
   family of questions S41 and A33 keep raising about whether the automation
   fires; most prior cases were hand-closed and could not answer it.
4. **The `matrix.platform != 'win'` exclusion cuts both ways.** It stops the
   windows leg filing a platform issue *and* stops it closing one. Both halves
   showed up in a single session. Any future look at that condition should
   weigh the close side, not just the file side.
5. **A commit whose meaning depends on a commit in another repo has to say so
   in its message.** `af42bd6` names TideSynth `c0c79818c`, the compiler errors
   both spellings, and the issues — otherwise the next person to read it sees
   six lines adding a virtual nobody calls in this repo.

**Next:** nothing outstanding on this break. The `win`, `mac` and `any` NEXT
cells are prefixed with the resolution so no run re-opens it. The scheduled
lane's real state is unchanged and is what the entry below describes: **nothing
takeable, blocked on Jeff.** Check the A33 sweep after 06:00 UTC.

**Branch/PR:** none — pushed straight to `main` in both repos at Jeff's
direction, per his interactive convention.

## 2026-08-25 — macos — "launch the au3": it aborted on every instantiation, two bugs deep

**Prompt:** interactive

**Did:** Jeff: *"let's launch the au3"*. It builds, assembles, ad-hoc signs,
registers with `pluginkit` and its host app runs and shows its "registered with
macOS" window. **It cannot be instantiated by any host.** `auval -v aumu Drck
Dsyh` → `FATAL ERROR: OpenAComponent: result: 4099`. One crash report per run:
**SIGABRT in the extension process**, every time.

**Bug 1 — a nested bundle could not find itself.** `CreatePluginBundleRef()`
(`BundleInfo.cpp:66`) `dladdr`s its own binary and walks up to the **first**
`Contents` component. An AUv3 is a bundle INSIDE a bundle:

```
<host>.app/Contents/PlugIns/<plugin>.appex/Contents/MacOS/<exe>
            ^^^^^^^^ first -- the HOST APP
```

Two `Contents`; it took the first, so `getResourceFolder()` returned the host
app — **whose `Contents/Resources` does not exist at all**. Every
`getResource()` missed. TIDE's four module XMLs never merged (no pins for those
controls) and no prefab resolved, so `seedRootMidiCv()` → `AddPrefab` →
`CContainer::LoadPrefab` hit its `assert(false)` on `doc.Error()` and aborted.
**Fix: take the LAST `Contents`.** A `.vst3`/`.component`/`.app` has exactly
one, so last == first and it is a no-op for every non-nested bundle.

**Bug 2, which Bug 1 had been HIDING.** With resources resolving, the crash
**moved** — to `Module_Info::RegisterParameters` via `Module_Info3_base::ScanXml`.
`TideApp::InitInstance()` merges those XMLs into `Module_Info`, which is
**process-global**, and had no guard because **a standalone made the distinction
invisible: `InitInstance` runs exactly once**. A plug-in host creates several
instances in ONE process, and an AUv3 extension process is shared across
instantiations — so instance two re-scanned every module and tripped
`assert("Already scanned parameters")`. Fix: merge once per process.

**Measured, each step separately:**

| state | auval |
|---|---|
| as found | `FATAL OpenAComponent 4099` — abort in `LoadPrefab` |
| + BundleInfo fix | `FATAL OpenAComponent 4097` — abort **moved** to `RegisterParameters` |
| + both | **AU VALIDATION SUCCEEDED**, 10 passes, **0** `retrievedValue` warnings |

**Negative control:** reverting only the BundleInfo half puts it back to `FATAL
4097`. **Both are required.** Standalone re-checked after the shared change:
runs, all four XMLs enrich, nine prefabs seed.

**M4 SAYS auval SUCCEEDED ON 2026-08-23 AND THIS DOES NOT CONTRADICT IT —
but the reconciliation is REASONED, NOT MEASURED, and should be checked.**
`seedRootMidiCv` (2026-08-20) and the XML loop (2026-08-21) both predate that,
so it is not a code regression. The likely answer is **build type**: this box
built **Debug**, and `CMAKE_CXX_FLAGS_RELEASE` is `-O3 -DNDEBUG`, so in Release
both asserts compile out. The same two bugs would then not crash — they would
**degrade silently**: no pins for the four XMLs' controls, no root MIDI-CV, and
`auval` does not check that a rack has its modules. **That makes these fixes
matter in Release too, just not as a crash.** I did not build Release to
confirm it.

**Lesson.** Bug 1 was not new — it was *harmless* until something needed a
bundle resource during init in an appex, and then it presented as a crash three
call levels away in unrelated code. And **fixing it did not make the AU work; it
made the NEXT bug visible.** A crash that moves is the signal that the first fix
landed — worth more than a pass/fail.

**Not verified:** any real host (GarageBand, Logic, Live) — `auval` only; the
AU2 wrapper, which shares `BundleInfo` and is predicted to benefit but was not
tested; iOS; and Release, as above. **One unexplained observation, recorded
rather than explained away:** a single standalone launch asserted
`(mi), ImportChildren, CContainer.cpp:1078` and died; two immediate re-runs were
clean and both my changes are provable no-ops there (one `Contents`;
`InitInstance` runs once, which its own "enriched" output confirms). I could not
reproduce it and cannot attribute it.
## 2026-08-25 — windows — `main` is red on all three platforms from ONE unpushed half, and the fix is uncommitted in Jeff's tree

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1 work, not a backlog item. The queue really is blocked — see
"The queue" below — but `main` broke **eleven minutes before this run started**
and that outranks everything. Diagnosed it, measured it A/B, filed
[#394](https://github.com/JeffMcClintock/TideSynth/issues/394), commented the
cause onto [#392](https://github.com/JeffMcClintock/TideSynth/issues/392) and
[#393](https://github.com/JeffMcClintock/TideSynth/issues/393), and
**deliberately did not fix it.**

**THE BREAK IS ONE CAUSE ON THREE PLATFORMS, AND IT IS A TWO-REPO CHANGE OF
WHICH ONLY ONE REPO WAS PUSHED.** `c0c79818c` ("x", Jeff, 2026-08-25 08:55
+1200, interactive) added five lines to `SynthEditSem/TideApp.h:49`:

```cpp
bool rackModeIsFixed() override { return true; }   // BACKLOG U1c
```

The base virtual it overrides is **not on `SynthEditLib`'s `main`**. It exists
only as an **uncommitted working-tree change on this box** —
`EditorLib/Application.h` (+6) and its call site in
`EditorLib/MfcDocPresenter.cpp` (+1/-1). TIDE's root `CMakeLists.txt:184`
fetches SynthEditLib at `GIT_TAG origin/main`, so every CI machine compiles the
override against a base class that does not declare it.

| job | error |
|---|---|
| `windows` | `TideApp.h(49,28): error C3668: 'TideApp::rackModeIsFixed': method with override specifier 'override' did not override any base class methods` |
| `macos`, `linux` | `TideApp.h:49:62: error: only virtual member functions can be marked 'override'` |

**Result — verified A/B with a control, on this box.** `cl /Zs` (syntax-only,
no codegen) on `SynthEditSem/TideApp.cpp`, MSVC **14.51.36231**, using the exact
`/I` and `/D` set lifted from `build/SynthEditSem/TIDE_Rack.vcxproj`. The
**only** variable changed between arms is which SynthEditLib the include path
points at; the GMPI, gmpi_ui, GMPI_Wrappers and VST3 include paths are
byte-identical in both.

| arm | SynthEditLib source | result |
|---|---|---|
| **A** | `git archive origin/main` into a scratch dir — reproduces CI | **`error C3668` at `TideApp.h(49)`, and nothing else** |
| **B** | this box's working tree = A + the uncommitted diff | **compiles clean, 0 errors** |

The two trees are otherwise identical: local `main` is level with `origin/main`
(`git rev-list --left-right --count HEAD...origin/main` → `0 0`) and
`git diff origin/main --stat` is exactly `2 files changed, 8 insertions(+), 1
deletion(-)`. **So that uncommitted diff is the COMPLETE fix — measured, not
inferred.** Nothing else is missing, which is the part worth having: "push your
change" is otherwise a guess, and a wrong guess costs three boxes a run each.

Arm A is the reusable half of the technique. A **quoted** include
(`SynthEditAppBase.h:4` is `#include "Application.h"`) resolves to the sibling
file first, so **you cannot shadow it with `/I`** — the obvious cheap trick does
not work. `git archive origin/main | tar -x` into a scratch dir costs one
command, touches nothing, and hands you CI's exact tree to point `/I` at.

**WHY THIS RUN DID NOT FIX IT, stated so the next run does not re-litigate it.**
The fix is the developer's uncommitted work in progress, in a **GATED** repo,
made eleven minutes before the run started — the exact scenario STEP 5's
preamble names (*"a late-firing run often starts at app launch — exactly when he
may be mid-work"*). Three routes were considered and all three are wrong:

  1. **Commit his two files.** Forbidden unconditionally by STEP 5's third kind
     of dirt: *"Never commit it, never revert it, never stash it."* No exception
     reaches it, A17's included.
  2. **Revert the breaking commit** — A17 bound 2's stated preference. That
     commit is an eleven-minute-old interactive change that **adds working
     functionality** (U1c: TIDE pins `rackMode` on, so the panel's "Rack Mode"
     toggle has nothing to offer but a way to turn the product off). Bound 2's
     own wording stops here: forward-fix where a revert would remove working
     functionality. And note what the cheap version of this would do — without
     `override` the method silently becomes a non-virtual shadow, so it would
     compile and the feature would just stop working. **A green build that has
     quietly deleted the feature is worse than a red one.**
  3. **Re-author the same two hunks in a clean worktree**, which touches nothing
     of his. Technically permitted, and still wrong: it races a change he is
     holding, and collides with his own commit when he pushes it.

So: file precisely, hand him the exact diff (it is his own), and leave it. That
is bound 6 — fall back to filing — arrived at from a different direction.

**THIS IS NOT S41, AND THE DIFFERENCE MATTERS FOR WHOEVER READS THE RED BUILD.**
S41 measured a two-repo change whose halves merged **26 seconds apart**, with CI
straddling the gap. That one **self-heals** — the next push is green with no fix
in between, and S41's lesson is not to spend a session on it. This one does
**not** self-heal: the second half was never committed at all, so `main` stays
red until somebody pushes it. Same symptom, opposite handling, and telling them
apart takes one command — `git show origin/main:<file> | grep <symbol>` in the
sibling repo.

**THE WINDOWS LEG NEVER FILES A PLATFORM ISSUE, SO STEP 1's FEED IS
STRUCTURALLY EMPTY ON THIS BOX.** `build.yml:409` is
`if: failure() && matrix.platform != 'win' && github.event_name == 'push'`, and
the step reported **skipped** on the failing `windows` job while `macos` and
`linux` each filed one. The exclusion is deliberate — the comment above it says
a failing build *"on mac or linux"* is only useful if the machine that owns that
platform finds out — but the consequence is that **a Windows-only break files
nothing and no run is told.** This box has to read CI itself. Filed #394 by hand
under STEP 3's *"if you discover your platform's default branch is broken and no
platform issue exists, file the platform-labelled issue yourself"*. Left as a
note rather than a row, because the change would be to `.github/workflows/**`,
which the bot token deliberately cannot push.

**The queue: nothing takeable, and the NEXT block was one day stale in both
cells that matter.** Both the `win` and `any` cells named **S47** as the one
ungated row; S47 merged as
[#387](https://github.com/JeffMcClintock/TideSynth/pull/387) at 20:52Z
yesterday, hours after the cell was written. **Third consecutive day of the same
one-day staleness** (A20/A27), this time on the mac cell's own filing. Verified
row by row against the Status column rather than trusting the prose: the whole
non-DONE set is **S1b** (`EditorLib/CMakeLists.txt` plus a split of
`ModuleFactory_Editor.cpp`, GATED), **S8** (`SynthEditLib/CMakeLists.txt:582`,
GATED, and MOOT since the Oscillator HD ruling), **E7** (an engine fact Jeff has
ruled is not a blocker), **E2** (an umbrella whose own row says it is not
takeable because its Accept cannot be stated), **R5** (`.github/workflows/**`
plus a certificate only Jeff can export), **V6/V7** (NEEDS-JEFF), and
**X1/X2/V2/E3/E4/R6** (BLOCKED). There are zero `platform:win` TODO rows;
**P3**, this box's only own-boxed row, is GATED in both its files.

**Bookkeeping, on verified PR state rather than from memory.** **V4** and
**S47** were both IN-REVIEW with every linked PR merged, so both are now
**DONE**. V4 got the extra check its own text demanded: the row said the "Goto
Rack" greying fix *"stands"*, which reads like pending work, so I looked —
`SynthEditGui.cpp:1256` now computes
`atRack = (!rack || (rack == currentContainer && currentViewFlag == CF_PANEL_VIEW))`,
exactly the fix described. It landed. `SynthEditLib` has zero open PRs and its
#51 is merged, so V4's cross-repo half is closed too.

**Learned:**

1. **A red build across all three platforms is evidence of ONE cause, not
   three, and the shared cause is nearly always a sibling repo.** Three
   different compilers agreeing on a `file:line` is not three findings. Check
   `git show origin/main:<file>` in the sibling before reading a second
   compiler log.
2. **"The other half was never committed" and "the other half merged 26 seconds
   late" look identical from CI and need opposite responses** — one is a lost
   session if you ignore it, the other is a lost session if you chase it. S41
   is the self-healing one. One command tells them apart; run it before
   deciding which you are holding.
3. **You cannot `/I`-shadow a quoted include.** `#include "X.h"` resolves
   against the including file's own directory first, so a scratch dir at the
   front of the include path is simply ignored. `git archive <ref> | tar -x`
   gives you the whole tree at that ref for one command and no worktree, which
   is what makes an A/B against `origin/main` cheap.
4. **A missing `override` fails loudly; a missing `virtual` fails silently.**
   Deleting the `override` keyword to go green would have compiled and quietly
   turned U1c off. When a build break's cheapest fix is to delete a keyword,
   find out what the keyword was load-bearing for first.
5. **The Windows box has no automatic STEP 1 feed.** `build.yml` excludes
   `matrix.platform == 'win'` from issue-filing on purpose. A run here that
   trusts an empty `gh issue list --label platform:win` has verified nothing;
   read the branch's latest run instead.
6. **A run that starts minutes after an interactive commit should expect to be
   looking at half a change, not at a defect.** The timestamps are the tell —
   `git log -1 --format=%ad` on the breaking commit cost nothing and reframed
   the whole item from "fix this" to "do not touch this".

**Next:** #394 closes the moment Jeff commits and pushes those two
`SynthEditLib` files; #392 and #393 close with it, and no TideSynth change is
needed. A later run should verify `main` green on all three legs and close all
three. If it is still red in a day or two, that is worth a second look — not at
the diagnosis, which is measured, but at whether the change was abandoned rather
than forgotten, in which case reverting `c0c79818c` becomes the right call
rather than the wrong one.

**Branch/PR:** `tide/win/issue-394`

## 2026-08-25 — macos — V4 verified by driving the UI, which found two bugs a build could not

**Prompt:** interactive

**Did:** Finished V4 by **testing it in the running app with computer
control**, which is the only way the untested half could be reached. The row
said *"NOT verified: the browser's rendered list, and the `Everything`
branch"*. Both are now verified, and **both were broken.**

**Bug 1 — the `Everything` branch never fired.** In the rack the browser
correctly showed **9 of 174** entries. Drilling in with "Goto Structure..."
opened the structure view but **left the browser filtered to the same nine**.
Temporary `fprintf` tracing showed why:

```
TEMP-DIAG V4 OpenViewForContainer isRackLevel=1     <-- on the DRILL-IN
```

`isRackLevel` is `targetContainer == MasterContainer`, and **"Goto
Structure..." on the master shows the RACK'S OWN structure** — same
container, different view. So `isRackLevel` stayed true, `setBrowserScope()`
early-returned on the unchanged value, and the filter never lifted. The
predicate had to be `view_flag == CF_PANEL_VIEW`. `view_flag` was already
computed on the line above and already carried the answer.

**Bug 2 — the same error, already shipped, one screen away.** "Goto Rack" is
greyed in the master's structure view, because U3 tests
`rack == currentContainer` — true there, since "Goto Structure..." does not
change the container. `onViewOpened` was **already being handed the resolved
`view_flag` and discarding it** (`int /*flag*/`); keeping it in
`currentViewFlag` greys the item only when the panel is actually on screen.

**I FIRST WROTE THIS UP AS A DEAD END WITH NO WAY BACK TO THE RACK. THAT WAS
WRONG, AND JEFF CAUGHT IT: *"Goto Panel gets back to the rack"*.** The item is
called **"Panel Edit..."** in the menu, it sits four lines below "Goto Rack",
and it is the exact counterpart of "Goto Structure...":
`POPUP_MENU_CONTROLS -> Document()->OpenView(this, CF_PANEL_VIEW)`
(`CContainer.cpp:1677`). **I had it on screen in my own screenshot and reasoned
past it instead of clicking it.** Measured since: "Panel Edit..." returns to
the rack, rails and all, with the browser back to 9.

**So Bug 2 is a much smaller thing than I claimed:** a greyed item that should
not be greyed, next to a working affordance that is simply named badly for this
purpose. Worth fixing — "Goto Rack" is the discoverable name and "Panel Edit..."
reads like an editor, not like navigation — but it is discoverability, **not a
trapped user**. Bug 1 is unaffected: the structure view really did keep the
rack's 9-entry list, whichever door you used to get there.

**Measured, on the stripped build, both directions:**

| step | browser |
|---|---|
| rack view | 9 entries, one "Prefabs" group |
| after "Goto Structure..." | **20 categories**, `Controls`…`Waveform`, incl. `TiDE` |
| after "Goto Rack" | **back to 9** |

The return trip matters on its own: it proves the scope **restores** rather
than latching.

**Lesson — the class, not the line.** Both bugs are *container identity used
where view identity is meant*. Bug 2 predates V4 and was found only because
the fix for Bug 1 sent me through the same door in the other direction.
**A green build cannot see either one**; both compile, both are type-correct,
and both are wrong only in the running app. The V4 row's honest *"NOT
verified"* is what made this worth doing — it named the gap precisely enough
to aim at.

**Second lesson, the expensive one: I asserted a dead end I never tested.** I
drove the UI to find Bug 1, then wrote up Bug 2 from *reading the enable
condition* — and shipped "no way back short of restarting" into a PR body, this
journal and the V4 row. One click on an item already in my screenshot would
have refuted it. **The rule I broke is the one at the top of the board:
MEASURE BEFORE YOU ASSERT.** It applies hardest right after a real measurement
succeeds, when the next claim feels like it came from the same evidence and did
not. A greyed menu item proves an item is greyed; it proves nothing about
whether another item does the job.

**Filed V7 — the menus need better names, and this run is the bug report.**
Jeff, after the correction: *"these menu need better names"*. Measured in
`MfcDocPresenter.cpp`: **five names for three actions**, and which one you get
depends on where you right-click.

| action | background menu | on a container | TIDE adds |
|---|---|---|---|
| -> panel view | `Panel Edit...` (:1359) | `Pa&nel Edit...` (:1245) | `Goto Rack` |
| -> structure view | `Goto Structure...` (:1226) | `&Structure...` (:1246) | — |
| -> parent | `Goto Parent...` (:1361) | `Goto Parent Container` (:1333) | — |

Both halves of each pair call the same command. **`Panel Edit...` is the outlier
that does not say "goto" at all** — which is precisely why I missed it and
invented a dead end. Proposed: `Goto Panel` / `Goto Structure` / `Goto Parent` /
`Goto Rack`. **Not done, by Jeff's ruling:** the names live in EditorLib, shared
with SynthEdit proper, so a rename changes SE16's menus for every existing user,
and TIDE has no hook to override them. That is a SynthEdit product decision, not
a TIDE cleanup. Jeff also ruled the `currentViewFlag` ungreying **stays** in
#391 — belt-and-braces once the rename lands, an improvement on its own until then.

Also noted in V7: `Pa&nel Edit...`, `&Structure...` and `D&ebug` still carry MFC
accelerator ampersands, and **P3 removed MFC**. Grepping `se_sdk3_hosting/` found
nothing that strips `&` from menu text, so they most likely render literally.
**Not visually confirmed** — reaching them needs a right-click on a container in
the structure view and the modules were off-canvas. The row says to confirm before
fixing, which is the same discipline this entry is otherwise about.

**Not verified:** behaviour with a module actually categorised `Rack` (none
exists yet — the category half is still exercised only by the temporary
recategorisation recorded in the row), and any view other than the master's.
The diagnostic `fprintf`s were removed before committing; the round trip
above was re-run on the clean build.

## 2026-08-25 — macos — The queue is blocked, so the run proved the platform instead; B1 closed on a green matrix

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1 clear (no open `platform:mac` issues — in fact **zero** open
`platform:*` issues on the repo). STEP 1.5 clear (**zero** open PRs across all
six repos; the TideSynth remote carries only `main`). **STEP 2 found nothing
takeable**, which both NEXT cells predicted. Rather than invent work, this run
did the two things that were available: **closed B1 by measurement**, and
**proved macOS `main` is healthy** — the one measurement no other box can make.
No product code changed in any repo.

### The queue really is blocked, row by row

Written out because three consecutive runs have now re-derived it:

| row | why a scheduled run cannot take it |
|---|---|
| **S1b** | `EditorLib/CMakeLists.txt` + a split of `ModuleFactory_Editor.cpp` — **GATED** |
| **S8** | gate is `SynthEditLib/CMakeLists.txt:582` — **GATED**, and the row says the change *"needs a ruling this row does not ask for"* |
| **E7** | remainder is an engine fact Jeff ruled is **not a blocker** |
| **E2** | umbrella; its own row calls it *"not takeable"* because its Accept cannot be stated |
| **R5** | `.github/workflows/**` — the bot token deliberately lacks `workflow` scope |
| **V4, V6** | `NEEDS-JEFF`; STEP 2 forbids starting them |

**V4 looks like an exception and is not, which is worth knowing before someone
else checks.** Its row says in bold *"THE PLUMBING IS IDENTICAL UNDER EVERY
OPTION AND MAY PROCEED"*, and `docs/decisions.md`'s `PROPOSED:` entry repeats it
in the `May proceed meanwhile:` field. But that plumbing is `ModuleBrowser.cpp`
and `SynthEditAppBase.cpp`, and **both live in `SynthEditLib/EditorLib/`** —
checked, not assumed. So it is blocked twice over, by status *and* by path.

**A process gap I decided NOT to file, and the reason is the interesting part.**
The escalation template's `May proceed meanwhile:` field exists precisely to keep
work flowing while a question is open, and STEP 2's blanket *"NEVER start an item
marked NEEDS-JEFF"* makes it unreachable to a scheduled run — because a
`PROPOSED:` entry parks its row at exactly that status. That looked like a real
deadlock worth an A-row. **It has no live instance:** V4's named work is GATED
anyway, and V6's row explicitly says its three shapes *"are not identical under
every answer, so per STEP 2 this run stopped"* — so V6 has no such work at all.
A row whose premise has zero instances is the kind this backlog keeps having to
retract, so it is recorded here instead. If a NEEDS-JEFF row ever names
`May proceed meanwhile:` work on an **ALLOWED** path, that is the moment to file it.

### B1: closed by measurement, no commit needed

B1's remaining half was *"exactly one thing: TIDE's root `CMakeLists.txt`, which
is C7d's job"*. **C7d is DONE and the file exists** — this run configured TIDE
from the TideSynth root to build, which is the same fact from the other side. So
`guard`'s `buildable` test passes and **the matrix runs instead of skipping**.

Run [32710164461](https://github.com/JeffMcClintock/TideSynth/actions/runs/32710164461),
`main` at `c466084`: **seven jobs, all green.** And they *ran* — `linux`, `macos`
and `windows` each show `Configure` and `Build` at `success`, where a
guard-skipped job reports `skipped`. Windows took 7m20s.

**The original Accept is retired rather than met**, and saying which matters: it
asked for a build failing *"for exactly one honest reason (missing private
dependency)"*, and C7 removed that dependency. The deeper clause — *"when this
workflow goes green from a clean clone with no private access, the project is
actually open source"* — **holds, and the two EPHEMERAL legs are what prove it**:
`linux` and `windows` are GitHub-hosted, hold no private access, and are green.

### The near-miss: I nearly filed a row on a refuted premise

macOS finished Configure+Build in **38 seconds** against Windows' **7m20s**, and
macOS routes to the self-hosted `tidesynth-m1` — **which is this very box**
(`/Users/jeffmcclintock/actions-runners/tidesynth/_work/...`), whose `_work` tree
persists between runs. That reads as an obvious correctness gap: an *incremental*
green is a weaker claim than an ephemeral one, and this project has a documented
stale-artifact failure class. I had the row half-drafted as S47.

**It is false, and two measurements killed it:**

- `actions/checkout@v7` runs **`git clean -ffdx`** every job — `clean: true` and
  the `git clean` command are both in the log. The tree is wiped, not reused.
- ccache reported **536 / 536 hits, 0 misses**. A ccache hit is keyed on
  preprocessed source, so it is byte-equivalent to a compile.

So macOS CI is a genuine clean build served by a sound cache. **The 38 seconds
had a mundane, correct explanation, and the workflow's own comment predicts it**
(*"the Build step takes ~23s because ccache hits almost everything"*) — I had read
past it. The 100% hit rate is also an **independent confirmation** of something I
had established separately by diff: nothing compilable has landed since #380.

### macOS `main` is healthy — measured, not inherited

Nothing compilable had changed on TideSynth since #380 (the last mac-verified
build): the four merges after it touch only workflows, markdown and a test
fixture. But **this exact five-repo combination had never been built together**,
and the S45 entry's lesson is that a dependency merge changes what `main` builds
against with no TideSynth event at all. So it was measured.

Fresh worktrees at `origin/main` of all five repos, fresh build tree, all four
overrides confirmed `[local override]` in the configure log:

| | result |
|---|---|
| configure | **rc=0** |
| build | **rc=0**, 0 `error:`, 18 targets |
| artifacts | all five present, all **arm64** Mach-O — `.vst3` `.gmpi` `.clap` `TIDE-Rack.app` `TIDE-Rack-AUv3.app` |
| runtime | 9 rack prefabs seeded, root MIDI-CV seeded, `rack built for 44100 Hz, block 512` |
| S7 regression | home-folder diff **0 lines**, `.resource_version` still **193** |

**The zeros are only worth something because the log had content.** The run
printed its enrichment counts and seed lines, which is what says the instrument
was live — the failure shape this journal records four separate times is a check
that counts absences passing because the thing under test never ran. The binary
was asserted to exist before launching, for the same reason.

**So there is no `platform:mac` issue to file**, and my own cold build is the
control CI's macOS leg cannot be, since CI's macOS leg *is* this machine.

**One advisory the tooling raised, passed through rather than swallowed.**
`check-id-refs.py` reports E2 as *"1 live umbrella row whose split rows have all
landed"* — E2a, E2b and E2c are all closed and E2 is still TODO. It is advisory by
design (A32: an umbrella with unfiled future children is indistinguishable from a
finished one), and **E2 is the second case, not the first**: its row says what is
left of it is MODULES, one stage each, and which modules is the product decision
that makes it un-takeable. So it stays open, and the advisory is correct to be an
advisory. Recorded because it fires on every run and the next reader should not
have to work out whether it is new.

**Verified:** the seven-job CI matrix read step by step, not by its rollup;
`git clean -ffdx` and 536/536 ccache hits from the job log; cold clean build
rc=0 with all five arm64 artifacts; runtime diagnostics; S7 A/B against Jeff's
real home with `.resource_version` backed up first; PR state for #380 and #382
via `gh pr list` returning zero open PRs; A35/S47 duplicate-id checks against
freshly-fetched `origin/main`.

**Not verified:**

- **The A33 sweep has still never run in place**, and I did not discharge that.
  The last `watchdog.yml` run was **06:18 UTC 2026-08-24** and #382 merged at
  **09:02 UTC**, so its first firing is 06:00 UTC on 2026-08-25. **All four
  issues it named (#306, #364, #372, #373) are closed, but every one was closed
  BY HAND** — none is evidence about the mechanism. The next run should check
  whether that sweep fired and what it said.
- **V5's rendered result.** The 1008 canvas is right arithmetically and it runs;
  whether 2.62 rows *looks* right is Jeff's call.
- **Windows and Linux.** Nothing was built on either.
- **No real DAW, and no iOS host.** The standalone only.

**Learned:**

- **A suspicious performance number deserves its mundane explanation checked
  first.** 38s against 7m20s on a self-hosted box with a persistent `_work` tree
  is a compelling story about stale artifacts, and it was wrong. `clean: true`
  and a 100% ccache hit rate are each one line of log.
- **"Persistent workspace" and "incremental build" are different claims.** The
  directory does survive; `actions/checkout` empties it. I reasoned from the
  first to the second without looking.
- **A process gap with zero live instances is a lesson, not a row.** The
  `May proceed meanwhile:` / NEEDS-JEFF deadlock is real in principle and had
  nothing to point at; filing it would have added a row whose premise the next
  reader has to re-check.
- **When the queue is blocked, building your own platform's default branch is
  the one thing left that is not busywork** — it is what STEP 1 needs an answer
  to, and on the mac box CI cannot answer it independently, because CI's macOS
  runner is the same machine.
- **Two independent routes to the same fact is worth noticing.** "No compilable
  change since #380" came from a diff; ccache's 536/536 said it again from CI.

### Filed on the way: S47

Regenerating `docs/lessons.md` (rotation step 4) surfaced a defect in the file the
prompt tells every run to read. `scripts/extract-lessons.py` **hardcodes its own
provenance figures** into the preamble it writes — *"223 KB across 167 entries"*,
*"4.1x smaller"*, *"152 of them, none dropped"* — measured 2026-08-20. The same
script now prints **988 lessons from 248 entries**, so the header understates its
own coverage by 96 entries. Filed as **S47**, not fixed: STEP 3 says file what you
find outside your item, and this run had no item.

**It is a label bug, not data loss, and I checked which.** The lesson SET before
and after regeneration differs by exactly the 5 bullets of this entry — 0 removed,
983 -> 988 — compared as sorted sets rather than by diff line count, because the
rotation reorders the file and 58 added / 53 removed lines looks identical to loss.

**Next:**

1. **Check whether the 2026-08-25 06:00 UTC watchdog sweep fired**, and what it
   reported. That is A33's outstanding verification and it needs no new work.
2. **S47 is takeable by any box, and both NEXT cells now point at it.** I filed
   it rather than taking it, because this run already had an item — but it is
   ALLOWED-path with a stateable Accept, so it is the exception the cells had
   been saying did not exist. **Note the cells would have been stale the day I
   wrote them if I had not gone back and corrected them after filing** — the
   A20/A27 failure, committed by the run complaining about it.
3. **Everything else in the scheduled lane is blocked on Jeff** — S1b/S8 want
   rulings or GATED access, R5 wants a `workflow`-scoped push, V4 and V6 want a
   ruling.
4. **The mac-only work that has no row needs a human at the keyboard:** M2's own
   text records the iOS app installed but never launched and its Audio Unit never
   opened in an iOS host, and E9's AU result is from our own probe, never a DAW.

**Machine left clean.** All work in throwaway worktrees and one scratch build
tree under the session scratchpad; **nothing was built in `~/Documents/GitHub/TideSynth`**
or any of Jeff's checkouts, and nothing was installed. The standalone was launched
from the scratch build tree and terminated (exit 0 on `SIGTERM`). **Jeff's
`~/SynthEdit Projects` was snapshotted before the run and is byte-unchanged after
— 335 entries, `.resource_version` still 193**, backed up first. The CI runner
directory was read only. All repos on their default branches.

**Branch/PR:** `tide/mac/B1-close-and-verify-main` — TideSynth only: the B1, A33
and V5 rows, all four NEXT cells, and this entry. **No product code change in any
repo.**

## 2026-08-24 — linux — V6's risk discharged: a root paste does carry connections, and the row's fork is now live (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** STEP 1 first — closed [#373](https://github.com/JeffMcClintock/TideSynth/issues/373),
a CI-filed linux build failure, by building `main`. Then took **V6** and did the
one thing its row asks for before anything else: **verified the root-level paste.
Nothing has been deleted.**

### #373, closed on a build

CI filed it against `tide/mac/S44-delete-stranded-branch`, which had since merged
and been auto-deleted — so the head it named no longer existed. The break was
never branch-specific:

```
StandardCommandIds.h:54:41: error: expected identifier before numeric constant
CContainer.h:19:1:          error: expected declaration before '}' token
```

`CContainer.h` declared its own enum for four command ids under `#ifndef _WIN32`;
**P3** added `StandardCommandIds.h`, which `#define`s the same four on *every*
platform, and the macros then expanded inside the enum. **Windows never saw it
because the enum was compiled out there** — the platform that would have caught
it is the one the guard excluded. Fixed by `c0bc053` before I got there; verified
by building the full tree from current `main`: **483/483, rc=0, all four Linux
artifacts, zero occurrences of the error.**

### V6: why the risk was real, not ceremonial

The row says *"the risk is the root-level paste, which should be verified before
the C++ is deleted"*. It is easy to read that as diligence. It was not:

**All nine shipped prefabs in `RackModules/` have exactly one top-level module
and zero top-level lines.** The case V6 depends on — several top-level modules
plus connections *between* them, pasted at the root — has never been exercised
anywhere in this product.

### The experiment, and the control that makes it a measurement

A deliberately minimal prefab: two top-level modules (`MIDI In`,
`SE MIDI to CV 2`) and one top-level line between them. Nothing else, so a
failure would have exactly one possible cause. Armed from the browser and
click-placed at the root (arm-then-click, per the 2026-08-20 finding).

**The control is that `seedRootMidiCv()`'s own pair is in the same saved
document.** So the pasted result is compared against the C++'s output, in the
same run, rather than against my judgement:

| | editor half | DSP half |
|---|---|---|
| **seeded (C++)** | `fMod="1521837852" tMod="1620974935" fPlg="1"` | `<Line From="1521837852" To="1620974935"/>` |
| **pasted (prefab)** | `fMod="811000001" tMod="811000002" fPlg="1"` | `<Line From="811000001" To="811000002"/>` |

Byte-equivalent. **`fPlg="1"` surviving is the load-bearing detail** — pin 1 is
`MIDI Data`, pin 0 is the GUI `Activity` input, so a paste that dropped the pin
index would have wired the wrong plug **and still looked structurally correct**.

Fixture and recipe committed at `tests/fixtures/v6-multi-module-paste.synthedit`,
so this is re-runnable rather than a claim.

### Why I stopped there rather than finishing V6

The row offers *"one prefab or default document"*, and those are not the same
build. `seedPrefabsFromBundle()` scans `Resources/Prefabs` **recursively** and
puts everything it finds in the module browser — so shipping the root assembly as
a prefab there makes it **user-insertable**, which reopens precisely the question
this row records as closed: *"there is exactly one, TIDE owns it, and 'what if the
user adds a second' stops being a question."*

Three shapes, none dominant:

- **(a) prefab in `Prefabs/`** — simplest; browsable and duplicable.
- **(b) prefab outside `Prefabs/`** — not browsable, but `ResolveFilename` only
  searches `kPrefabFolder`, so it needs a resolve path or an absolute one.
- **(c) default document** — closest to the row's intent, largest change.

STEP 2 says a run may only do work that is identical under every open answer. The
verification is; the implementation is not. **Row set to NEEDS-JEFF with the fork
and its cost written down**, rather than picking and calling it a decision.

**Verified:** `main` full tree 483/483 rc=0; the paste experiment with its
in-document positive control; nine-prefab survey by XML parse, not by eye.

**Not verified:**

- **The full five-connection assembly** — only the two-module, one-line case was
  built. The facade wiring (`fPlg 4/3/5/2 → tPlg 7/8/9/10`) is extracted and on
  the row, but not exercised.
- **That a pasted `SE MIDI to CV 2` at root still clones per voice** — E7's
  polyphony requirement is the reason the module must be at root at all, and a
  paste is a different code path from `AddModule`. **This is the thing I would
  test first** if the ruling is (a) or (b).
- **Any audio.** Structure only.

**Learned:**

- **"Verify X before deleting Y" earns its place when X has never happened.** The
  nine-prefab survey took one script and turned a procedural-sounding instruction
  into a real precondition.
- **Put the control in the same artifact as the subject.** The seeded pair and the
  pasted pair are in one saved document, so "did it wire correctly" became a diff
  rather than an interpretation.
- **A minimal repro is worth more than a faithful one here.** Two modules and one
  line means a failure has one cause; building the whole five-connection assembly
  first would have conflated the paste with the pin arithmetic.
- **A CI issue can name a head that no longer exists.** #373 pointed at a merged,
  auto-deleted branch; the break was on `main` all along, and building `main` is
  what settled it.
- **Check what a folder scan actually enumerates before shipping a file into it.**
  `seedPrefabsFromBundle` recursing is the whole reason (a) is not free.

**Machine left clean.** Headless weston stopped, standalone stopped, scratch
`HOME`s throughout; the test prefab was copied into a scratch build tree, never
into Jeff's. All six repos on their default branches and clean.

**Branch/PR:** `tide/linux/V6-root-midicv-prefab` — TideSynth only: the fixture,
its README, the V6 row and this entry. **No product code change** — deliberately,
since V6's implementation is what the ruling decides.
## 2026-08-24 — windows — V4: the three candidate markers, measured — one of them selects nothing (interactive, Jeff directing)

**Did:** synced all 23 repos, then took **V4**. STEP 1 clear (no open
`platform:win` issues), STEP 1.5 clear. **There are no `platform: win` rows left
at all** — P3 was the last one and it is DONE, so this box's queue is now the
`any` queue, the same place the mac box reached two days ago.

### What the row asked for, and why it could not just be built

V4 wants the rack view's module browser to offer only rack-relevant modules. It
names the discriminator as an open question and says it *"should be ruled rather
than invented"*, offering (a) prefab-vs-module, (b) the existing `category=`
attribute, (c) a new explicit marker.

That is a correct instruction and it is also the whole cost of the row — the
plumbing is one filter. So the useful work was not to pick one, but to **measure
what each would actually select**, and escalate with numbers instead of opinion.

### The measurement

| option | selects **today** | mechanism |
|---|---:|---|
| **(a) prefab-vs-module** | **9 of 9** | already exists — `ExportModules(list, includePrefabs)` + the `*P=` unique-id prefix |
| **(b) `category=`** | **0 of 9** | cannot see prefabs at all |
| **(c) new marker** | 0 until authored | new field, new plumbing |

**(b) fails structurally, not by degree, and this is the finding.** A regular
module's group comes from its XML: `mm.group = GetGroupName(u)`, reading the
`category=` attribute — **273 modules across 32 distinct categories**. **A prefab
has no module XML.** Its group is derived from its *file path*
(`ModuleFactory_Editor.cpp:2387-2395`), and **#377 flattened `RackModules/`**, so
the nine rack prefabs carry no group at all.

And the rack's entire content today **is** those nine prefabs. So the option that
reads categories selects none of the things the rack is made of.

There is also **no rack- or TiDE-named category anywhere in the tree** — checked
across both repos' module XML — so (b) is not merely empty by accident, it has
nothing to read even in principle until someone adds the field.

### The recommendation is a fourth option, and only because the row's own objection is right

V4 says (a) is *"nearly right but excludes any future non-prefab rack module"*.
True. But the answer to that is not to adopt an option that is empty today — it
is to write the predicate as a **union**: *is it a prefab from the rack folder,
**or** is it marked rack-relevant?* The second half selects nothing until
something claims it, costs one clause, and removes the migration later.

Filed as **(d)** in the `PROPOSED:` entry, with (a) and (b) left on the table
because the ruling is Jeff's, not mine.

**The plumbing may proceed under any option and is stated as such in the entry:**
`ModuleBrowser.cpp:56` and `:99` hard-code `includePrefabs = true`, and
`TideApp.cpp:147` already computes `isRackLevel` for exactly this
rack-vs-structure distinction. Getting that value down to the browser is the same
work whichever predicate wins.

**Verified:** the counts are greps over the tree, re-runnable — `273` and `32`
from `category="…"` across `SynthEditLib/modules/*/*.xml`, `9` from
`RackModules/*.synthedit`, and the `*P=`/path-derived group claims read out of
`ModuleFactory_Editor.cpp` and `SynthEditAppBase.cpp:1334` rather than inferred.

**Not verified:** nothing was built or run this item — it is a measurement and an
escalation, and no code changed. The claim that the plumbing is option-independent
is read from the two call sites, not demonstrated by building it.

**Learned:**

- **"Ruled rather than invented" does not mean "stop" — it means measure, then
  escalate with numbers.** Two greps turned a three-way design argument into one
  option that works, one that is empty, and one that is future work. The ruling
  is still Jeff's, but it is now a much shorter question.
- **An option can fail because the data it reads does not exist for the thing
  being selected.** (b) sounded like the tidy answer — reuse the field the
  browser already reads — and prefabs simply have no XML for it to read. Worth
  checking that a proposed discriminator can *see* its subjects before comparing
  it on elegance.
- **A flattening commit changed what a proposed option would select.** #377
  removed the `RackModules/` subfolder, which is where a prefab's group comes
  from, so the path-derived answer went to empty as a side effect of an unrelated
  tidy-up. Options that read incidental structure are fragile in ways the row
  cannot anticipate.

**Next:**

1. **Jeff rules (a), (b), (c) or (d)** by merging or editing the `PROPOSED:`
   entry. The default in effect meanwhile is today's behaviour — the rack offers
   everything, which is noisy rather than broken.
2. **The plumbing is takeable now** by anyone, under any outcome.
3. **This box has no `platform: win` rows left.** The `any` queue is what it has,
   and most of what is on it needs a ruling or a workflow-scoped token.

**Machine left clean.** One throwaway worktree under the session scratchpad, no
build trees, nothing installed. All 23 repos on their default branches; the four
dormant product repos with large uncommitted trees (`SE15` 407 files, `SSG` 194,
`Waves` 102, `Optimus_1_5` 47) were fetched and left untouched, as were the two
active repos' own working changes.

**Branch/PR:** `tide/win/V4-rack-filter-ruling` — TideSynth only, no code change.
## 2026-08-24 — macos — R7: half was already done, and the other half is deferred (interactive)

**Prompt:** lets do the ones that need admin interactivly / we're already sucessfully codesigning with azure, why do anything / lets say "forget it till it breaks", for now.

Jeff opened admin-requiring rows to interactive sessions, so I took R7. Two
findings, and the second is a correction to my own approach.

**Part (1) was already done.** The row describes an ungated exposure — a workflow
edit on any `tide/**` agent branch executing with read access to all 8
credentials. Measured on the live repo, it cannot happen: a `release` environment
exists with Jeff as a REQUIRED REVIEWER, all 8 credentials are in it, repo-level
secret count is **0**, `release.yml` declares `environment: release`, and
`build.yml` / `auto-merge.yml` touch only `GITHUB_TOKEN`. The row was stale and I
would not have known without checking the API rather than reading the row.

**On part (2) I was working the row instead of the situation.** I had researched
the OIDC migration, confirmed the action supports it, found the missing
`azure-subscription-id`, drafted the Apple API-key swap, and written Jeff a list
of portal steps — before asking whether any of it was worth doing. His reply:
*"we're already successfully codesigning with azure, why do anything"*. Correct.
With (1) in place the remaining benefit is an expiry that has not arrived and a
credential that is already gated behind his approval, against the cost of
changing a working release path that CANNOT BE TESTED without cutting a real tag.

Ruled: *"forget it till it breaks, for now."* Marked WONTFIX rather than left
TODO, so it stops being re-picked off the queue.

**The research is on the row rather than thrown away**, because the next person
to want this should not re-derive it: the action does support OIDC and
`azure-client-secret` is optional, but OIDC also wants `azure-subscription-id`
(configured nowhere), the job needs `id-token: write` (it has only
`contents: read`), and the Apple half is `notarytool --key/--key-id/--issuer`
behind a new App Store Connect key.

**The one cheap thing that would pre-empt the trigger:** both secrets were
created 2026-08-09, and an Entra client secret's expiry is visible only in the
portal. A 30-second look there is worth more than the migration.

**Not verified:** nothing was changed, so there is nothing to verify. The
measurements are live API reads, re-runnable.

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
2. Stop when this file is **under 60 KB**, or when the floor is reached —
   whichever comes first. **The floor is the LATER of: the four most recent
   entries, or every entry carrying the most recent date.** The floor always
   wins; a busy day pushing this file over 60 KB is correct, not a rotation
   failure.
3. Never edit an entry while archiving it. The archive is the record.

**Why a date and not a duration (A24, 2026-08-20).** A24 asked for a time-based
floor — *"retain everything from the last 7 days"* — and measuring what that
costs is what killed it. Entries per day, counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it, so 7 days is **3.4× the 192 KB that
triggered A8 in the first place** — the remedy would have been twenty times more
expensive than the problem. Even two days is worse than the state A8 was created
to fix.

So the floor is **one date**, which bounds the cost at roughly a day's work while
guaranteeing a run can always see everything that happened most recently — the
failure A24 correctly identified, where a 4-entry floor at ten entries a day
bought under half a day. On a quiet week the four-entry floor still binds and
nothing changes.

**What this does NOT fix, filed as A30:** the durable lessons still age out.
Rotation moves an entry's *"Learned"* bullets into the archive with it, and no
run reads the archive. The cheap answer is a standing digest that never rotates;
the expensive one is reading 651 KB.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

Template:

```
## YYYY-MM-DD — <machine> — <BACKLOG id>

**Did:** what actually changed.
**Result:** built / tested / failed, with the real output.
### Correction: Ardour IS a host here, and it settles the question

**Jeff asked "don't we have Ardour host?" — yes, and that makes three separate
claims of mine wrong.** I wrote in the row, both PR bodies and the issue that
closing this needed REAPER on a win/mac box. **Ardour 8.4 is installed on this
box**, `ardour-vst3-scanner` answers precisely this question, and **my own memory
note from 2026-08-19 records using it**, including the
`LD_LIBRARY_PATH=/usr/lib/ardour8` quirk it needs.

```
BROKEN (main):  VST3 not a valid bundle:
                  '.../TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE_Rack_VST3.so'
FIXED  (both):  [Info]: Found Plugin: TIDE Rack
                  uid=506C7567696E474D504920501951ED43 category="Instrument|Synth"
                  n_outputs=2 n_midi_inputs=1
```

Ardour derives the payload name from the bundle name — exactly the rule GMPI's
own comment states — so **the Linux VST3 is unloadable today, not merely oddly
named**, and the fix is host-verified on the platform that has the bug. The
scanned UID also matches the one in all five `.rpp` fixtures.

**The lesson is not "use Ardour".** It is that I asserted an environment limit
three times without testing it, while holding a note that contradicted it.
"Not verifiable here" is a claim about the machine, and it deserves one command
before it goes into a row, two PR bodies and an issue.

Ardour's cache entry from the scan pointed into a scratch tree and was removed;
Jeff's other nine cached plugins were left alone.


**Learned:** anything the next run would otherwise rediscover the hard way.

0. **"Not verifiable on this box" is a measurable claim, and I shipped it three
   times unmeasured.** Ardour was installed the whole time and my own memory note
   named the command. Check the machine before writing a limit into a row.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---
