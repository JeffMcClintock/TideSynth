# Plan: one preset model for SynthEdit plugins on the GMPI wrappers

Companion to [state-management-survey.md](state-management-survey.md).
Written 2026-10-15. Plan only; no code has changed.

## Scope and constraints

- **Test bed:** the GMPI wrappers (`C:\SE\GMPI_Wrappers\wrapper\*`) and the
  SynthEdit GMPI export (`C:\SE\SE16\se_gmpi`), starting with VST3 on Linux,
  which is new and whose users do not yet expect perfection.
- **Off limits:** the production SynthEdit VST3 wrapper (`SE16\se_vst3`),
  SE2JUCE, and `ProcessorStateManager.h/.cpp`. They are read for reference and
  not modified.
- **Must also serve AU2, AU3 and CLAP.** Anything format-specific stays in
  the thin adapter for that format. The preset model, the reader and writer,
  the metadata, and the controller↔processor protocol live in format-agnostic
  code: `GMPI/Hosting` (`gmpi_controller_holder`, `gmpi_processor`) and the SE
  plugin's own `<Controller/>` subtype.
- **Ground rules from the survey** (section 7): text is the interchange across
  the controller/processor boundary; shared code yes, shared memory no;
  metadata may be a per-process immutable singleton; any same-process
  shortcut needs a text fallback.
- **Shared SynthEditLib code** (`MpController`, `MpParameter`, `DawPreset`) is
  also compiled into `se_vst3`. Changes there must be additive and
  behaviour-preserving for the existing callers, or they wait. Each stage
  below says which files it touches.

## Where the GMPI path stands today

Measured by reading, not yet by running (Stage 0 runs it):

1. An SE export's `plugin.gmpi.xml` (`ExportAsPlugin.cpp` ~1207) declares the
   DAW-automatable float parameters, MIDI pins and seven host-connect pins.
   Nothing else. So `gmpi_processor::patchManager` and
   `gmpi_controller_holder::patchManager` hold only those floats, and
   `writePresetXml` saves only those floats.
2. The engine's full parameter set (`parameters.se.xml`) is loaded by
   `MpController::Initialize` inside `Controller_VST3`, and the panel edits it
   through the "SeBinaryMessage" queue. But `Controller_VST3`'s preset
   virtuals are stubs, `SeGmpiProcessor::onSetParameter` is a TODO, and
   nothing writes those values into any saved chunk. **An SE plugin on the
   GMPI wrapper loses every non-DAW parameter on save/reload.** MIDI learn,
   text and list settings, private knobs, all of it.
3. AU2, AU3, CLAP and the Standalone all already do the two calls that matter
   on the format-agnostic side: `pluginController->syncState()` before
   reading `gmpiController.getPresetXml()`, and
   `notifyControllerOfPreset(pluginController)` after
   `setPresetXmlFromDaw()`. VST3 does the same through `mintFreshPresetXml`.
   TIDE's `SynthEditController` is the working example of a plugin
   `<Controller/>` that keeps its whole state in one blob parameter (parameter
   1) and pushes DSP messages through another (`dspMessagesParamId`).
4. The two GMPI preset readers (`setPresetXmlFromDaw` and `readPreset`) are
   one loop copied twice with matching comments saying so. The writer is
   already shared.

The consolidation therefore has a concrete first customer: give SE plugins on
GMPI a saved engine preset at all, and do it in the shape that removes the
processor-side shadow rather than adding one.

## Target shape

```
                 host (VST3 / AU / CLAP / standalone)
                    │ save: syncState() + getPresetXml()
                    │ restore: setPresetXmlFromDaw() + notifyControllerOfPreset()
                    ▼
   gmpi_controller_holder.patchManager  ──── one stateful Blob parameter "SE engine preset"
                    ▲                        one non-stateful Blob "SE dsp messages" (ppc frames)
                    │ setParameter(blob)
   SeGmpiController : gmpi::api::IController          ← NEW, owns the MpController
        ├─ syncState():   MpController::getPreset()->toString() → blob
        ├─ setParameter(blob): DawPreset(sharedInfo, xml) → MpController::setPreset
        │                       then per-parameter "ppc" to the DSP (existing path)
        └─ MpController: MpParameter values, undo, preset browser (unchanged)

   SeGmpiProcessor : IProcessor
        ├─ blob pin "SE engine preset": applied only at graph build (seed), never mid-stream
        ├─ blob pin "SE dsp messages": bytes pushed into ControllerToProcessorQue
        └─ engine echo (MIDI learn, CCs) → MessageQueToGui → controller as today
```

Properties:

- **One source of truth for a save:** the controller's `MpController`, which
  already receives every engine-originated change through the "ppc"/"lern"
  echo. No `ProcessorStateMgrVst3`, no FIFOs, no retained-preset vector.
- **Headless fallback stays correct enough:** with no controller connected,
  `getPresetUnsafe()` serialises the processor's store, whose engine-preset
  blob is whatever was last restored. A validator or an offline renderer
  restores and renders; it does not MIDI-learn.
- **No XML on the audio thread:** the blob is consumed at build
  (`SynthRuntime::prepareToPlay` already takes `pendingPresets` XML); live
  preset changes travel as per-parameter "ppc" frames, which is how panel
  knobs reach the engine today.
- **Format-agnostic by construction:** the wrappers call two methods they
  already call. VST3's "SeBinaryMessage" side channel and the
  `g_pendingEditorGuiHost` back-channel become unnecessary once the
  `<Controller/>` owns `MpController` (Stage 3).

## Stages

Each stage leaves every build green and the plugin working; each has a
measurement that proves it did what it claims and a negative control that
shows the measurement can fail.

### Stage 0: measure the baseline (no code change)

- Build an SE-exported GMPI plugin for Linux VST3 from a fixture project with
  one DAW parameter, one private float, one text parameter and one
  MIDI-learned parameter.
- Write `tests/se_gmpi_state_probe.cpp` in `GMPI_Wrappers/tests` (same shape
  as TideSynth's `e79_clap_headless_probe.c` and `e69_clap_state_probe.c`):
  load the module, `setComponentState`/`setState` from a preset XML fixture,
  `getState`, diff the two documents, and report which parameter ids survived.
- Expected today: only the DAW float survives. That number is the baseline
  the later stages move. Also record chunk size and the Steinberg validator
  result on Linux.
- Touches: tests only.

**Stage 0 result, measured 2026-10-15 (windows + WSL Ubuntu 24.04).**

Fixture: [tests/fixtures/stage0-state.synthedit](../tests/fixtures/stage0-state.synthedit),
built headlessly (Slider → Oscillator pitch, List Entry → waveform, an
unconnected Text Entry, Sound Out outside the container). Exported once from
the same document as a GMPI bundle and as the production Windows VST3. Both
exports share one `parameters.se.xml` with three module-owned parameters:
pitch (float, DAW-facing), waveform (enum, DAW-facing), text (string). Probe:
`C:\SE\GMPI_Wrappers\tests\se_gmpi_state_probe.cpp`, a bare VST3 host that
restores a chunk, runs 8 blocks, saves, and lists which `parameters.se.xml`
handles the saved chunk carries.

| arm | wrapper | engine params saved | restore round trip | audio |
|---|---|---|---|---|
| default | GMPI VST3, Linux | 0 of 3 | n/a | -6.0 dBFS |
| restore `stage0-gmpi-preset.xml` | GMPI VST3, Linux | 0 of 3 | 2 of 2 GMPI floats same | -6.0 dBFS |
| restore, `--no-controller` | GMPI VST3, Linux | 0 of 3 | 2 of 2 | -6.0 dBFS |
| default | se_vst3, Windows (control) | 3 of 3 | n/a | -6.0 dBFS |
| restore `stage0-se-preset.xml` | se_vst3, Windows (control) | 3 of 3 | 3 of 3, `MIDI="7"` kept | -6.0 dBFS |

The GMPI chunk (291 bytes) is `<Preset>` with `Param id="0"` and `id="1"` (the
two `plugin.gmpi.xml` floats) plus `id="-2"`..`"-7"`, the six host-connect
pins written with `val="0"`. No SE handle appears. The se_vst3 chunk (257
bytes) is `<Preset pluginId=.. name=..>` with the three SE handles, the
MIDI-learn attribute, and `id="0"` (program name). The controller half's
`getState` writes 0 bytes on both wrappers. The Steinberg validator (SDK
3.8.0, built at `/root/build/vst3sdk`) passes the Linux bundle: 47 tests,
0 failures. That is the bar every later stage must keep.

Two things learned on the way that later stages depend on:

- A Linux SE export needs its modules as `<bundle>/Contents/PlugIns/<name>.sem/Contents/x86_64-linux/<name>.sem`
  (`Module_Info3.cpp:83`); a flat `.sem` beside the Resources is not found and
  the engine renders silence with no message on stderr.
- `se_gmpi/vst3/CMakeLists.txt` skipped the template target whenever
  `GMPI_WRAPPER_FOLDER_OVERRIDE` was set, because it tested the FetchContent
  population flag that an override never sets. Fixed in SE16 (uncommitted) to
  honour the override.

Reproduce: WSL `Ubuntu-24.04`, build dir `/root/build/se16-stage0`
(`/opt/cmake/bin/cmake -S /mnt/c/SE/SE16 -G Ninja -DCMAKE_BUILD_TYPE=Release`
with the five `*_FOLDER_OVERRIDE`s pointing at `/mnt/c/SE/...`), targets
`SynthEditTemplate_VST3 OscillatorNaive`; the assembled bundle and probe
outputs are under `TideSynth/build-stage0/` (ignored by git).

### Stage 1: one metadata description per process

- Add `SeParameterInfo::instance()` (function-local static, `const
  std::map<int32_t, paramInfo>` built by the existing `::init()` from the
  bundle's `parameters.se.xml`) beside `ProcessorStateManager.h`, or in a
  new header so the production file is untouched.
- `MpController` holds `const std::map<...>&` instead of its own copy;
  `DawPreset` constructors are unchanged (they already take a `const` ref).
- `se_vst3` keeps calling `::init()` into its own `ProcessorStateMgr`; it is
  not changed. Confirm by building `se_vst3` and diffing its binary's
  behaviour with the existing Windows state test (`vst3_state_read_test`).
- Measurement: a unit test that builds the singleton and the legacy map from
  the same XML and asserts the maps compare equal. Negative control: feed it
  two different XMLs.
- Touches: `SynthEditLib/modules/se_sdk3_hosting` (additive), `Controller.h/.cpp`
  (member type only).

**Stage 1 result, implemented and measured 2026-10-15 (uncommitted).**

- New `SynthEditLib/modules/se_sdk3_hosting/SeParameterInfo.h` declares
  `seParameterInfo()`, defined in `Controller.cpp` as a function-local static
  `const std::map<int32_t, paramInfo>` built once per process from
  `BundleInfo::getResource("parameters.se.xml")` with the existing `::init()`;
  empty when the resource is absent (TIDE Rack).
- `MpController::parametersInfo` is now `const std::map<int32_t, paramInfo>&`,
  bound in the constructor. `Initialize()` no longer builds its own copy.
  Every existing use passes it to a `DawPreset` constructor, which already
  took a `const` reference, so `se_vst3` and `Controller_VST3` compile
  unchanged. The one downstream user that indexed it with `operator[]`
  (Optimus `SubPresetManager.cpp`, both the SSG and Optimus_1_5 copies) now
  uses `.at()`.
- Debug builds cross-check: `Initialize()` still parses the element the old
  way into a scratch map and asserts it equals the shared one field by field.
  Measured by running the Stage 0 probe against a Debug template on Linux:
  no assertion fired, 0 of 3 saved, audio -6.0 dBFS (so the engine ran).
- Behaviour unchanged on the Release template: all three Stage 0 arms re-run,
  saved chunks byte-identical to the Stage 0 outputs (`cmp`), validator 47
  passed, 0 failed.
- `ProcessorStateMgr::init` (production) still builds its own table; it is
  untouched by decision.
- `se_vst3` compiles and links with the change (Linux target
  `SynthEdit_VST3`, after fast-forwarding SynthEditLib to `9199681`, which
  SE16 commit `ad236e24a` needed for `getDefaultNormalized`; that mismatch
  predated this work). Its only use of `parametersInfo` is as a `const`
  argument to `DawPreset`, so no source change was needed there.

### Stage 2: carry the engine preset as a blob parameter

- Declare two Blob parameters in the SE export's `plugin.gmpi.xml`:
  `SE/EnginePreset` (stateful, not DAW-facing) and `SE/DspMessages`
  (non-stateful), each with a processor input pin. Preferred: the exporter
  (`ExportAsPlugin.cpp`) writes them. Fallback if the exporter must not
  change yet: `gmpi_controller_holder::init` appends them when
  `parameters.se.xml` is present in the bundle.
- `SeGmpiProcessor`: on `open()`, read the `SE/EnginePreset` pin's seeded
  bytes and pass them to `SynthRuntime` as the pending preset for the build;
  on `SE/DspMessages` `PinSet`, push the bytes into
  `ControllerToProcessorQue()`. Ignore `SE/EnginePreset` events after build.
- Interim controller side (until Stage 3): `Controller_VST3` implements
  `syncState`-time writing of `MpController::getPreset()->toString()` into
  `SE/EnginePreset`, and restores from it in `setComponentState` via
  `DawPreset(sharedInfo, xml)` → `MpController::setPreset` → per-parameter
  `ParamToDsp`. `MpController::getQueueToDsp()` writes into `SE/DspMessages`
  instead of the "SeBinaryMessage" staging buffer.
- Measurement: Stage 0's probe now reports all four parameters surviving;
  chunk size grows by the engine preset; validator still passes. Negative
  control: build with the blob declaration removed and watch the probe fall
  back to the baseline.
- Touches: `SE16/se_gmpi`, `SE16/SynthEdit2/ExportAsPlugin.cpp` (or
  `GMPI/Hosting/controller_holder.cpp`), `GMPI_Wrappers/wrapper/VST3`.

**Stage 2 result, implemented and measured 2026-10-15 (uncommitted).** Scope
was narrowed to the engine-preset blob alone; the DSP-messages blob and the
retirement of "SeBinaryMessage" move to Stage 3, where the controller moves.

What was built:

- `ExportAsPlugin.cpp` declares one more GMPI parameter,
  `SE/EnginePreset` (`datatype="blob"`, id = highest DAW index + 1), and an
  input pin for it. A blob never gets a DAW tag (`xml_spec_reader.cpp:784`),
  so hosts never see it. Found by name everywhere else.
- `SeGmpiProcessor` maps that pin to a new `EnginePreset` role. On `PinSet`
  it builds a `DawPreset` from the shared table (`seParameterInfo()`, Stage
  1) and hands it to the engine through `SynthRuntime::setPresetUnsafe`, the
  same interrupt se_vst3 uses; the engine applies it at the top of that
  block's `DoProcess`. The holder seeds pins on the first block after
  `open()`, which is how a restore made before activation reaches an engine
  that was built in `open()`. An empty payload (fresh instance) is ignored.
  This parses XML on the audio thread; a preset is a few hundred bytes, and
  TIDE builds its whole rack there for the same reason. Noted, not fixed.
- `Controller_VST3` (interim home, see Stage 3): `refreshEnginePresetBlob()`
  runs inside `mintFreshPresetXml()` and writes `MpController::getPreset()`
  minus the DAW-owned parameters into the blob with `setBlob` (store only, no
  message, so a save never disturbs the engine); `restoreEnginePresetBlob()`
  runs after `setPresetXmlFromDaw` in `setComponentState` and applies the
  blob to `MpController::setPreset`, keeping the controller's current values
  for the DAW-owned parameters that `DawPreset` would otherwise default.

Ownership rule that fell out: a parameter is owned either by the DAW (it has
a GMPI parameter, travels as a normalised float, the host's automation lane is
the truth) or by the engine preset blob, never both. The blob therefore
excludes every parameter with a native tag.

Measured with the Stage 0 probe (now blob-aware: it base64-decodes a blob
Param and lists the handles inside, and estimates the output's fundamental
from zero crossings so a restore can be seen reaching the engine):

| fixture | arm | engine params saved | round trip | audio |
|---|---|---|---|---|
| stage0 (pitch, enum DAW; text) | default | 3 of 3 (2 as DAW ids, text in blob) | n/a | -6.0 dBFS |
| stage0 | restore `stage2-gmpi-preset.xml` | 3 of 3 | 3 of 3 | -6.0 dBFS |
| stage0 | restore, `--no-controller` | 3 of 3 (blob from the processor's own store) | 3 of 3 | -6.0 dBFS |
| stage0 | restore the Stage 0 chunk (no blob) | 3 of 3, text at its default | 2 of 2 | -6.0 dBFS |
| stage2 (pitch **private**; enum DAW; text) | default | 3 of 3 (pitch and text in blob) | n/a | **1757.8 Hz** (pitch 7 V) |
| stage2 | restore `stage2b-gmpi-preset.xml` (pitch 3 V) | 3 of 3 | 3 of 3 | **105.5 Hz** (pitch 3 V) |
| stage2 | restore, `--no-controller` | 3 of 3 | 3 of 3 | **105.5 Hz** |

The frequency pair is the engine-side proof: with the pitch slider private it
has no DAW parameter, so the only route from the chunk to the oscillator is
the blob. 7 V is A4 + 2 octaves = 1760 Hz, 3 V = 110 Hz; the zero-crossing
estimate over 4096 samples resolves to about 12 Hz. Validator: 47 passed, 0
failed on both bundles. Stage 0 was 0 of 3 on every arm.

Fixtures: `tests/fixtures/stage2-state.synthedit` (Stage 0's project with
`private="true"` on the pitch parameter), `stage2-gmpi-preset.xml`,
`stage2b-gmpi-preset.xml`. The first Linux bundles under `build-stage0/` had
their `plugin.gmpi.xml` edited by hand to what the exporter now writes; the
Debug SynthEditCL was then rebuilt with the exporter change, the stage2
project re-exported through it, and the three stage2 arms re-run on that
untouched export: same 3 of 3, same 1757.8 Hz / 105.5 Hz, validator 47/0.
The exporter writes `<Parameter id="1" name="SE/EnginePreset"
datatype="blob"/>` and `<Pin id="9" parameterId="1"/>` for that project.

Known gaps carried to Stage 3:

- `Controller_VST3::setParamNormalized` updates `gmpiController` only, so
  `MpController`'s copy of a DAW-owned parameter goes stale under host
  automation (se_vst3 forwards it via `getDawParameter(tag)`). Harmless for
  the blob (it excludes those parameters) but the panel knob does not follow.
- Headless saves (`--no-controller`) write the blob as last restored; a MIDI
  learn made by the engine in that configuration is not captured. There is no
  UI to make one there.
- `writePresetXml` writes the six host-connect pins as `Param id="-2"..
  "-7" val="0"`; readers skip them, so it is noise rather than a defect.

### Stage 3: the SE plugin owns its controller (AU and CLAP for free)

- New `SeGmpiController : gmpi::api::IController` in `SE16/se_gmpi`,
  registered by `SeGmpiFactory` as the plugin's `<Controller/>` subtype. It
  owns an `MpController` subclass whose transport is the GMPI host
  (`IControllerHost::setParameter` on `SE/DspMessages`, `notifyDaw` for
  native parameters) rather than a VST3 object.
- `syncState()` and `setParameter(SE/EnginePreset)` move here from
  `Controller_VST3`. The editor obtains its `IGuiHost2` from this controller
  (the `initUi` route), retiring `g_pendingEditorGuiHost` and
  `MinimalGuiHost`.
- `Controller_VST3` stops inheriting `MpController`; its remaining SE-specific
  code ("SeBinaryMessage", `seMsgQues_`) is deleted once Stage 2's pins carry
  the traffic. AU2, AU3, CLAP and Standalone need no change: they already
  call `syncState()` and `notifyControllerOfPreset()`.
- Measurement: Stage 0's probe, re-run on CLAP (Linux) and AU3 (mac, via the
  in-process AUv3 host method recorded in E91), reports the same survivors as
  VST3. The old `SeBinaryMessage` string no longer appears in any wrapper
  binary (`strings | grep`).
- Touches: `SE16/se_gmpi`, `GMPI_Wrappers/wrapper/VST3` (deletions),
  `SynthEditLib` (a transport seam on `MpController`: `getQueueToDsp()` is
  already virtual; `ParamToProcessorViaHost` needs a virtual).

**Stage 3 result, implemented and measured 2026-10-15 (uncommitted).**
`SynthEditLib` needed no change: `MpController`'s existing virtuals
(`getQueueToDsp`, `ParamToDsp`, `ParamGrabbed`, `makeNativeParameter`,
`notifyDawStateChanged`, the preset hooks) were enough of a seam.

What was built:

- `SE16/se_gmpi/source/SeGmpiController.h/.cpp`: the export's `<Controller/>`
  subtype, `MpController` + `gmpi::api::IController` (which is also the
  `IParameterObserver` the wrappers notify) + `gmpi::TimerClient`.
  `initialize()` runs `MpController::Initialize()` and publishes its
  `IGuiHost2*` as `SE/ControllerPtr`; `syncState()` writes the engine preset
  (minus DAW parameters) to `SE/EnginePreset`; `setParameter()` restores from
  that blob (applied to `MpController`, forwarded to the engine as
  `SE/EngineApply`), feeds `SE/Feedback` bytes into `MpController`'s dsp→ui
  queue, and keeps the panel's copy of a DAW-owned parameter current when the
  host moves it. Panel edits of DAW-owned parameters go
  `host->setParameter(N, Normalized)`, which the holder already routes to the
  DAW; private ones go as "ppc" frames in `SE/DspMessages`, flushed after each
  edit and on the timer. `SE/StateCounter` is bumped for changes the DAW
  cannot see (TIDE's idiom). Preset browser and undo apply through
  `setPreset` plus `SE/EngineApply`.
- `SeGmpiDescriptor.h`: the ids of those parameters and pins, by name, parsed
  once from `plugin.gmpi.xml`. `-1` means an older export; every user treats
  that as absent.
- `SeGmpiProcessor`: three new pin roles. `SE/EnginePreset` is applied once
  per `open()` as the seed (so a host with no controller still restores);
  `SE/EngineApply` is applied on every arrival; `SE/DspMessages` bytes go into
  the engine's controller→processor queue; the engine's dsp→ui FIFO is
  drained at the end of every `process()` onto the `SE/Feedback` output pin.
- `SeGmpiEditor`: receives the controller pointer through the GUI pin on
  `initUi` (`setPin`) and builds its document then; an export without that pin
  builds at `setHost` with the static fallback host as before.
- `SeGmpiFactory` serves the `Controller` subtype. The exporter declares the
  six parameters, their pins, a `<GUI>` pin for the pointer and
  `<Controller/>`.
- `gmpi_controller_holder` (format-agnostic): a `pluginController` observer
  the wrapper sets, told of processor-originated values (`ppc2`/`ppc3`) by
  `notifyPluginController`; `Controller_VST3::setParamNormalized` also calls
  it for DAW-originated values. `Processor_CLAP` and `Controller_VST3` set it;
  AU2, AU3 and the Standalone do not yet (one line each, mac/standalone
  builds not available here), so there the engine's feedback does not reach
  the panel.
- `Controller_VST3` no longer inherits `MpController`: `MpParameterVst3`, the
  staging buffer, `ParamToProcessorViaHost`, the "SeBinaryMessage" channel,
  `g_pendingEditorGuiHost` and Stage 2's interim blob code are gone.
  `Processor_VST3` lost `seMsgQues_` and its bridging. `se_gmpi/vst3`
  builds a CLAP template too (`FORMATS_LIST VST3 CLAP`, one CMake loop).

Measured, both on the rebuilt exporter's own output (stage2 project, pitch
private), same probe reader (`se_gmpi_state_analysis.h`) for both wrappers:

| wrapper | arm | engine params saved | round trip | audio |
|---|---|---|---|---|
| GMPI VST3, Linux | default | 3 of 3 | n/a | 1757.8 Hz |
| GMPI VST3, Linux | restore `stage2b` | 3 of 3 | 3 of 3 | 105.5 Hz |
| GMPI VST3, Linux | restore, `--no-controller` | 3 of 3 (seed) | 3 of 3 | 105.5 Hz |
| GMPI CLAP, Linux | default | 3 of 3 | n/a | 1757.8 Hz |
| GMPI CLAP, Linux | restore `stage2b` | 3 of 3 | 3 of 3 | 105.5 Hz |

Validator 47 passed, 0 failed. `strings` on the VST3 wrapper binary finds no
"SeBinaryMessage". The production `se_vst3` still compiles on Linux. The
Debug SynthEditCL (and with it the Windows `SynthEdit_GMPI` template) compiles
the new controller under MSVC. CLAP probe:
`GMPI_Wrappers/tests/se_gmpi_clap_state_probe.cpp`.

Not measured here, because a headless probe has no panel: the editor
receiving the pointer and building against the live `MpController`, and a
panel knob reaching the DAW and the engine. The code paths are the ones the
design names, but they have not been exercised.

**Measured later the same day: the engine → controller → save path, which
is what the old processor shadow existed for.** Fixture
`tests/fixtures/stage3-state.synthedit`: pitch private and MIDI-learned to CC 7
(`MIDI="33554439"`, the engine's `(ControllerType::CC << 24) | 7`, not `7`),
a `MIDI In` outside the container feeding a `MIDI Filter` and an
`SE Patch Automator` inside, and the container's `MIDI Automation` plug on.
The probe's new `--cc 7 0.5` arm delivers the CC as the VST3 parameter the
wrapper maps MIDI onto (id 10007) on block 4 of 64.

| arm | audio | pitch in saved blob |
|---|---|---|
| learned, no CC | 1757.8 Hz | 7 |
| learned, CC 7 = 0.5 | **439.5 Hz** | **5** |
| unlearned, CC 7 = 0.5 (control) | 1757.8 Hz | 7 |
| learned, CC 7 = 0.5, `--no-controller` | 439.5 Hz | 7 (stale: the documented headless caveat) |

Two findings on the way, both recorded in code:

- The engine moved (439.5 Hz) but the first save still said 7. `SE_GMPI_TRACE=1`
  (an env-gated stderr trace now in `SeGmpiProcessor` and `SeGmpiController`)
  showed the processor shipping the feedback and the wrapper allocating the
  messages, but the controller never receiving them: on a Linux host with no
  editor open nothing ticks `Controller_VST3::onTimer`, so the wrapper's
  dsp→ui queue is never polled (E74). Fix: `Controller_VST3::drainProcessorFeedback()`
  polls that queue at save time, before `syncState`, in `mintFreshPresetXml`
  and `getState`; `Processor_CLAP::stateSave` does the same. This is the one
  behaviour of `ProcessorStateMgrVst3` worth keeping (its `getPreset` drained
  its FIFO on demand), now on the controller side.
- Two fixture lessons: an SE export gets a MIDI input only when a MIDI cable
  crosses the container boundary (`ExportAsPlugin.cpp:791`), and MIDI learn
  needs an `SE Patch Automator` the project places and feeds, since the one
  the engine supplies automatically has an unconnected MIDI input.

Also done: `StandaloneHost.cpp` and `AU3_Wrapper.mm` now set the holder's
`pluginController` (the Standalone compiled on Linux; AU3 is mac-only and
unverified here).

AU2 (`AU2_Wrapper.cpp`) was the one wrapper with no `<Controller/>` at all,
and it also had no controller-to-processor transport for a blob and a
`RestoreState` that wrote only the controller's store. It now mirrors CLAP
and AU3: the constructor creates, initialises and holds the plug-in
controller and sets the holder's `pluginController`; a
`sendNonNativeParameterToProcessor` lambda frames blobs into the ui->dsp
queue (`sendParameterToProcessorQueue`, drained at the top of `Render`);
`ParameterListener` calls `notifyPluginController` for DAW-originated values;
`SaveState` drains `message_que_dsp_to_ui` and calls `syncState` before
`getPreset`; `RestoreState` calls `notifyControllerOfPreset` and delivers the
restored stateful parameters to the processor queue and the editors; and a
`hostStateTransfer_` guard keeps a restore from marking the project modified.
Mac-only, so uncompiled here: it is a line-by-line mirror of the CLAP code
that is compiled and measured.

### Stage 4: one reader, one merge, one conversion

- Fold `gmpi_processor::readPreset` and
  `gmpi_controller_holder::setPresetXmlFromDaw` into one function over
  `unordered_map<int, GmpiParameter>&`, mirroring `writePresetXml`.
- Add `DawPreset::merge(const DawPreset& current, const paramInfo map&)`
  implementing the two rules (keep MIDI-learn when absent; honour
  ignore-program-change) and call it from `MpController::setPreset`. Leave
  `DspPatchManager` and `ProcessorStateMgrVst3` calling their own copies.
- Delete the unreferenced `normalizedToRaw()` in `ProcessorStateManager.cpp`
  only if Jeff lifts the production freeze; otherwise leave it and note it.
- Measurement: round-trip tests on `DawPreset` (XML → object → XML equal
  hash), merge rule tests with and without MIDI assignments. Negative
  control: a preset carrying a MIDI assignment must change the hash.
- Touches: `GMPI/Hosting`, `SynthEditLib/modules/se_sdk3_hosting`
  (additive).

**Stage 4 result, 2026-10-15: the reader is done; the merge and the
conversion are deferred, deliberately.**

- `readPresetXml(parameters, xml)` in `GMPI/Hosting/controller_holder.cpp`
  is now the one reader beside the one writer, `writePresetXml`.
  `gmpi_controller_holder::setPresetXmlFromDaw` and
  `gmpi_processor::readPreset` both call it; the two loops they used to carry
  (one had learned to dispatch on datatype, the other had not, and a blob once
  aborted the host through `std::stod`) are gone. 307 lines removed, 49 added.
- Measured: both templates rebuilt, all five arms (VST3 default / restore /
  no-controller, CLAP default / restore) re-run; every saved chunk is
  byte-identical to its Stage 3 counterpart (`cmp`); validator 47 passed, 0
  failed. Negative control: restoring a document with no `<Preset>` element
  is accepted by both halves and leaves the saved chunk byte-identical to the
  default save, which is the "false, nothing changed" contract.
- Deferred, not forgotten: the MIDI-learn-keep and ignore-program-change
  rules exist in four places, three of them in production files
  (`ProcessorStateMgrVst3::replacePresetMutable`, `DspPatchManager::setPreset`,
  `DspPatchManager::setPresetState`). Moving only `MpController::setPreset`'s
  copy onto a `DawPreset::merge` would relocate one of four, not consolidate
  them, so it waits for the production freeze to lift. The same holds for the
  unreferenced `normalizedToRaw()` in `ProcessorStateManager.cpp` and for
  unifying the three normalised-to-real arithmetics, two of which are in
  `MpParameter_base`, shared with `se_vst3`.

### Stage 5: decide voices

- Choose: `DawPreset` carries per-voice values (and `toString` writes a
  `patch-list` like `DspPatchManager::getPresetState`), or it stays
  monophonic and the survey documents that polyphonic parameter state is not
  saved by any wrapper.
- Measurement: a polyphonic fixture parameter round-trips (or is documented
  not to).

**Stage 5 decided, 2026-10-15 (Jeff): document it, no code change.**
Polyphonic parameter values are not saved by any plug-in format: the se_vst3
chunk, the GMPI engine-preset blob and the undo history each carry one value
per parameter (voice 0), because `DawPreset::toString` writes voice 0 only and
`initFromXML` asserts one value per parameter. Per-voice state is runtime
state; the per-voice `patch-list` form that `DspPatchManager::getPresetState`
writes serves the engine's own restart-and-rebuild and stays there. Recorded
in the survey (section 2.2). `DawPreset`'s one-value assert and
`paramValue::rawValues_`'s "voice is 0 - 127" comment now disagree on
purpose; the assert is the truth.

## What this removes, in the GMPI world

| Today (before this work) | After Stage 3 (done 2026-10-15) |
|---|---|
| Processor shadow + 2 FIFOs + retained-preset vector (never instantiated on GMPI, so nothing to delete; the design never needs it) | controller is the source of truth |
| `Controller_VST3 : MpController` dual-system object | `Controller_VST3` knows nothing about SynthEdit |
| "SeBinaryMessage" side channel, `seMsgQues_`, `g_pendingEditorGuiHost` | two declared blob parameters, standard `setParameter` |
| engine preset not saved on GMPI | saved on VST3, AU2, AU3, CLAP, Standalone with one implementation |
| two GMPI readers | one |
| `MpController::parametersInfo` copy | shared `const` singleton |

## Open questions for Jeff

1. Exporter: may `ExportAsPlugin.cpp` gain the two blob parameters, or should
   `controller_holder` inject them for now?
2. Additive changes to `MpController`/`MpParameter` (shared with `se_vst3`)
   are assumed acceptable when behaviour-preserving. Confirm, or the plan
   stops at Stage 2's interim shape.
3. Stage 3 moves `MpController` into the SE plugin's own `<Controller/>`.
   That is the step that pays for AU and CLAP; it is also the largest.
4. Voices (Stage 5).
