# State management in a SynthEdit VST3 plugin: a survey

Written 2026-10-15 as groundwork for simplifying and combining
`ProcessorStateMgrVst3` with the VST3 controller's preset handling. It
describes what exists today, where each copy of the state lives, how the
copies are kept in step, and where the same logic is written more than once.
Nothing here changes code.

Files surveyed (all under `C:\SE` unless noted):

| Layer | File | Classes |
|---|---|---|
| DSP (audio thread) | `SynthEditLib/dsp_patch_parameter_base.h`, `dsp_patch_parameter.h`, `PatchStorage.h`, `dsp_patch_manager.h` | `dsp_patch_parameter_base`, `dsp_patch_parameter<T>`, `PatchStorageFixedSize/VariableSize`, `DspPatchManager` |
| Processor shadow | `SynthEditLib/modules/se_sdk3_hosting/ProcessorStateManager.h/.cpp` | `paramInfo`, `paramValue`, `DawPreset`, `ProcessorStateMgr`, `ProcessorStateMgrVst3` |
| Controller (UI thread) | `SynthEditLib/modules/se_sdk3_hosting/MpParameter.h/.cpp`, `Controller.h/.cpp` | `MpParameter`, `MpParameter_base`, `MpParameter_private`, `MpParameter_native`, `SeParameter_vst3_hostControl`, `MpController`, `UndoManager` |
| Editor document | `SynthEditLib/EditorLib/PatchParameter.h` | `PatchParameter_base`, `PatchParameter<T>` (owned by `CPatchManager`) |
| Legacy VST3 wrapper | `SE16/se_vst3/source/adelaycontroller.h/.cpp`, `adelayprocessor.h/.cpp` | `VST3Controller`, `MpParameterVst3`, `SeProcessor` |
| GMPI VST3 wrapper (TIDE) | `GMPI_Wrappers/wrapper/VST3/Controller_VST3.h/.cpp`, `Processor_VST3.h/.cpp`, `GMPI/Hosting/controller_holder.h`, `processor_holder.h` | `Controller_VST3`, `Processor_VST3`, `gmpi_controller_holder`, `ControllerPatchManager`, `gmpi_processor`, `PatchManager`, `GmpiParameter` |

## 1. The short version

A stateful parameter in a SynthEdit-exported VST3 plugin has its value held
in four places at once, each with its own container type, its own raw-bytes
encoding, and its own copy of the parameter's metadata:

1. **DSP**: `dsp_patch_parameter<T>::patchMemory[voice]` (a `PatchStorageBase`
   per voice, 128 patch slots each, the processor uses slot 0). The engine's
   actual value. Audio thread only.
2. **Processor shadow**: `ProcessorStateMgrVst3::presetMutable`, a `DawPreset`
   whose `params` map holds `paramValue::rawValues_[voice]`. Exists so the
   host's `getState()` can be answered from any thread without touching the
   DSP. Guarded by `presetMutex` and fed by two lock-free FIFOs.
3. **Controller**: `MpParameter_base::rawValues_[voice]` (a `std::string` of
   raw bytes per voice) inside `MpController::parameters_`. What the GUI and
   the DAW's `setParamNormalized` write to. Also carries the undo history
   (`UndoManager`, a vector of `DawPreset` snapshots), the preset-browser list
   and `session_preset`.
4. **DAW**: the host's own normalized value for each native (non-private)
   parameter, kept current by `performEdit` and read back via
   `setParamNormalized`.

A fifth store is the SynthEdit **editor document**, `PatchParameter<T>` owned
by `CPatchManager`, with 128 presets per voice. In SynthEdit.exe it is the
source the other four are exported from. In TIDE the editor runs inside the
plugin controller, so this store is live in the plugin too and is the state
of record (see section 6).

The metadata is parsed from `parameters.se.xml` three separate times in the
plugin (once into `ProcessorStateMgr::parametersInfo` on the processor, once
into `MpController::parametersInfo` on the controller, once more by
`MpController::Initialize` into the individual `MpParameter` fields), and
a fourth time on the DSP side from `dsp.se.xml` with a different XML library
(tinyxml 1 vs tinyxml2).

## 2. Where the state lives

### 2.1 DSP side: `dsp_patch_parameter_base` and `PatchStorage`

- `patchMemory_t patchMemory` is a vector indexed by voice; each element is a
  `PatchStorageBase*` holding `getPatchCount()` patch slots plus a per-slot
  dirty flag. `PatchStorageFixedSize` is one flat `char*` block,
  `PatchStorageVariableSize` an array of `(size, char*)` pairs for strings and
  blobs.
- `EffectivePatch()` is hard-coded to 0 on the DSP side (see
  `dsp_patch_parameter_base.h`); the 128-slot capacity is inherited from the
  editor and unused in a plugin.
- Values enter via `SetValueRaw2` (from `DspPatchManager::setPreset`),
  `SetValueFromXml` (from `setPresetState`), `vst_automate2` (DAW automation
  and MIDI-mapped CCs), or `SerialiseValue` from the UI queue (`OnUiMsg`
  "ppc"). Every path ends in `OnValueChanged` → `SendValue` to the connected
  pins.
- Values leave two ways:
  - `UpdateUI()` marks the patch dirty and asks the shell for the GUI queue;
    `getQueMessage()` later writes a "ppc" frame to the dsp→ui queue.
    Automation from the DAW does *not* echo unless flagged
    `kIsMidiMappedAutomation` or `kMustUpdateUi`, because the host already
    told the controller.
  - `shellDsp_->onSetParameter(handle, field, raw, voice)` is called from
    `SendValuePt2` and `setAutomation` so the processor shell can keep its
    shadow current. In the legacy wrapper this lands in
    `ProcessorStateMgrVst3::SetParameterFromProcessor`.
- `DspPatchManager::getPresetState` and `setPresetState` are a complete
  XML preset writer and reader living on the DSP side, with per-voice
  `patch-list` support. They are what `SeProcessor::getState` used before the
  state manager (the `#if 0` is still there) and what `SynthRuntime` uses to
  carry presets across an async restart (`getPresetsState` /
  `setPresetsState`).

### 2.2 Processor shadow: `ProcessorStateMgr` and `ProcessorStateMgrVst3`

`ProcessorStateMgr` (base):

- `parametersInfo`: `map<handle, paramInfo>` built by the free function
  `::init()` from `parameters.se.xml`. Non-stateful parameters are skipped.
- `presets`: `vector<unique_ptr<const DawPreset>>`. `retainPreset()` pushes
  every preset that is ever applied or minted, and nothing ever pops. This
  is deliberate, so the `const DawPreset*` handed to the audio thread via
  `interrupt_preset_` stays valid, but it means memory grows with every
  preset load and every dirty `getState()` for the life of the processor.
- `callback`: how a new preset reaches the DSP. The legacy processor sets it
  to `synthEditProject.setPresetUnsafe(preset)`, which stores the pointer in
  `SeAudioMaster::interrupt_preset_` and raises an interrupt; the audio
  thread picks it up in `HandleInterrupt()` and calls
  `DspPatchManager::setPreset`.

`ProcessorStateMgrVst3` adds the thread-safe shadow:

- `presetMutable`: the current preset, every stateful parameter present.
- `messageQueFromProcessor`: "ppc" frames from the audio thread
  (`SetParameterFromProcessor`).
- `messageQueFromController`: "ppc" frames from the controller
  (`VST3Controller::ParamToStateMgr` → "StateM" IMessage →
  `ControllerToStateMgrQue()`). Exists for when the processor is inactive and
  so cannot echo; `ProcessorWatchdog()` clears it while audio is running.
- `OnTimer()` every 177 ms drains the processor FIFO into `presetMutable`;
  `getPreset()` drains both on demand, then copies `presetMutable` into a new
  immutable `DawPreset` only if `presetDirty`, and publishes it through the
  atomic `currentPreset`.
- `setPresetRespectingIpc()`: the merge used for both `setState` and the
  "Preset" IMessage. It drains the FIFOs, merges ignore-program-change
  parameters from the old preset when `ignoreProgramChangeActive`, fills
  missing parameters with defaults, replaces `presetMutable`, clears the
  FIFOs, retains, and fires `callback`.
- `replacePresetMutable()`: keeps a MIDI-learn assignment when the incoming
  preset has none.

`DawPreset` itself is the interchange object: `map<handle, paramValue>` plus
name, category, a hash of its own XML (`calcHash`, used to recognise factory
presets in `MpController::syncPresetControls`), and three `mutable` flags that
callers set to steer the receiver (`ignoreProgramChangeActive`, `resetUndo`,
`reloadProcessor`). Its constructor from text, `initFromXML`, recognises six
XML layouts. `toString()` writes voice 0 only, and a debug assert in
`initFromXML` requires exactly one value per parameter, so a `DawPreset`
is monophonic by construction; per-voice state only round-trips through
`DspPatchManager::getPresetState`, which no wrapper calls for a save any
more.

**Decision (Jeff, 2026-10-15): polyphonic parameter values are not saved by
any plug-in format, and this is accepted, not fixed.** The se_vst3 chunk,
the GMPI engine-preset blob (same writer) and the undo history all carry one
value per parameter: a polyphonic parameter's voice-0 value. A session
restore therefore gives every voice that value, and whatever the engine then
writes per voice (voice pitch, gate, per-voice modulation) is runtime state,
not patch state. The per-voice `patch-list` form that
`DspPatchManager::getPresetState` writes exists for the engine's own
restart-and-rebuild (`SynthRuntime::getPresetsState`), where it stays.

### 2.3 Controller: `MpParameter` and `MpController`

`MpParameter` hierarchy (`MpParameter.h`):

- `MpParameter`: identity (`parameterHandle_`, `moduleHandle_`,
  `moduleParamId_`), metadata (`name_`, `enumList_`, `datatype_`,
  `stateful_`, `ignorePc_`), MIDI learn (`MidiAutomation`,
  `MidiAutomationSysex`), grab state, and the pure virtuals that separate
  the three transports (`updateProcessor`, `updateDaw`, `updateDawUnsafe`,
  `upDateImmediateValue`).
- `MpParameter_base`: adds the value store `rawValues_[voice]`, `minimum`,
  `maximum`, `hostControl_`, and the normalised↔real conversions.
  `setParameterRaw(MP_FT_NORMALIZED)` converts to raw bytes by datatype and
  recurses into `MP_FT_VALUE`; a change fans out to all registered GUIs via
  `controller_->updateGuis`.
- `MpParameter_private`: not exported to the DAW; `updateProcessor` sends a
  "ppc" frame straight to the DSP queue (`MpController::ParamToDsp`).
- `MpParameter_native`: exported; `onGrabbedChanged` drives
  `beginEdit`/`endEdit`. The wrapper subclass (`MpParameterVst3`) routes
  `updateProcessor` through the host (`performEdit`), so the value reaches
  the DSP as automation in `process()`, not through the queue.
- `SeParameter_vst3_hostControl`: controller-only host controls (program
  index, patch commands, etc.). A change calls
  `MpController::OnSetHostControl`, which is where the preset browser and
  undo commands are dispatched.

`MpController` owns:

- `parameters_` plus two indexes (`ParameterHandleIndex`,
  `moduleParameterIndex`).
- `parametersInfo`: the *second* `map<handle, paramInfo>` built by the same
  `::init()` the processor uses, kept only so the controller can construct
  `DawPreset` objects from XML.
- `undoManager`: `vector<pair<description, unique_ptr<const DawPreset>>>`
  plus an A/B slot. Every knob release and every `setParameterValue` on a
  stateful parameter takes a snapshot via `getPreset()`.
- `presets` (factory and user `presetInfo` records with hashes) and
  `session_preset` (a `DawPreset` copy of whatever the DAW restored that did
  not match a known preset).
- `message_que_dsp_to_ui` and a 35 ms timer that drains it
  (`onQueMessageReady` → `MpParameter::updateFromDsp`).

Key entry points:

- `getPreset()`: builds a fresh `DawPreset` by walking `parameters_`. Name and
  category come from the `HC_PROGRAM_NAME`/`HC_PROGRAM_CATEGORY` host
  controls. This is functionally the same thing as
  `ProcessorStateMgrVst3::getPreset()` but computed from the controller's
  copy.
- `setPreset(DawPreset const*)`: writes every value into `MpParameter`s,
  updates GUIs and the DAW, applies the MIDI-learn-keep rule and the
  ignore-program-change rule, resets undo, then `syncPresetControls` to pick
  the matching browser entry or add a session preset. The comment says it
  plainly: "set preset UI only. Processor is updated in parallel."
- `setPresetXmlFromSelf` / `setPresetFromSelf` (pure virtual; wrapper
  implements): the only route by which the controller tells the processor
  about a whole preset.

### 2.4 Editor document: `PatchParameter_base`

The editor's parameter is the richest of the five: 128 presets per voice,
a `CPatchManager` that knows the current program, watchers for the WPF/SE2
GUI, `Serialize2` for the document file, `Export()`/`Import()` for the
exported `parameters.se.xml`, and `UpdateDspValue` which sends "ppc" to the
editor's own in-process DSP. Its `EffectivePatch()` returns
`m_patch_mgr->GetProgram()` unless the parameter ignores program change,
which is where the 128-slot `PatchStorage` design comes from. It shares
`PatchStorage.h` and `MyTypeTraits` with `dsp_patch_parameter`, but nothing
else: a parallel class hierarchy with the same template shape
(`PatchParameter<T, MetaDataPolicy>` vs `dsp_patch_parameter<T, MetaDataPolicy>`).

## 3. How state moves (legacy se_vst3 wrapper)

### 3.1 A single knob turn in the plugin GUI

```
GUI control
 └─ MpController::setParameterValue(raw, handle)
     ├─ MpParameter::setParameterRaw            → store 3 (controller), updateGuis
     ├─ MpParameter::updateProcessor
     │   ├─ native:  VST3Controller::ParamToProcessorViaHost
     │   │           ├─ performEdit(tag, normalised)     → store 4 (DAW)
     │   │           │    └─ host → SeProcessor::process parameter changes
     │   │           │         └─ SynthRuntime::setParameterNormalizedDsp
     │   │           │              └─ dsp_patch_parameter::vst_automate → store 1 (DSP)
     │   │           └─ ParamToStateMgr → "StateM" IMessage → messageQueFromController → store 2
     │   └─ private: MpController::ParamToDsp → "ppc" into queueToDsp_
     │                └─ VST3Controller::OnTimer → "BinaryMessage"/MyData IMessage
     │                     └─ SeProcessor::notify → m_message_que_ui_to_dsp
     │                          └─ SynthRuntime::ServiceDspRingBuffers → OnUiMsg("ppc") → store 1
     │                               └─ OnValueChangedFromGUI → SendValue → onSetParameter → FIFO → store 2
     └─ undoManager.snapshot(getPreset())      → undo history (store 3 copies)
```

Two different transports (host automation vs. binary queue) depending on
whether the parameter is private, and the shadow is written twice for private
parameters (once by the controller's "StateM", once by the DSP echo) with the
watchdog deciding which copy counts.

### 3.2 A change originating in the DSP (MIDI CC, output parameter, MIDI learn)

```
dsp_patch_parameter::vst_automate2 / setAutomation
 ├─ UpdateUI → getQueMessage("ppc") → dsp→ui queue → background thread → IMessage
 │    └─ VST3Controller::notify → message_que_dsp_to_ui → OnTimer
 │         └─ MpParameter_base::updateFromDsp → store 3, updateGuis, emulateMouseDown, notifyDawStateChanged
 └─ shellDsp_->onSetParameter → ProcessorStateMgrVst3::SetParameterFromProcessor → FIFO → store 2
```

The DAW is *not* told (no `performEdit`), by design: the DSP already has the
value and a round trip would fight automation. The controller's copy and the
processor's shadow are both updated from the DSP, independently.

### 3.3 Whole-preset changes: three entry points, two XML parses each

**A. The DAW restores a project.** The host calls both halves:

```
SeProcessor::setState(chunk)                 VST3Controller::setComponentState(chunk)
 └─ DawPreset(parametersInfo, chunk)   [1]    └─ DawPreset(parametersInfo, chunk)   [2]
     └─ stateMgr.setPresetRespectingIpc           └─ MpController::setPreset
         ├─ replacePresetMutable → store 2            ├─ each MpParameter → store 3
         └─ callback → setPresetUnsafe                ├─ updateDaw → store 4
              └─ interrupt_preset_ (atomic)           ├─ undoManager.initial
                   └─ HandleInterrupt (audio)         └─ syncPresetControls
                        └─ DspPatchManager::setPreset → store 1
                             └─ OnValueChangedFromGUI → UpdateUI → "ppc" echo → store 3 again
```

The same chunk is read twice, once per half. That is correct, not a defect:
text is the only interchange that works when the halves are in different
processes, and the host itself delivers the same chunk to both. The risk is
not the second parse but that the two readers are driven by two separately
built `parametersInfo` maps, so a change to how one side builds its map (the
`persistant` filter, enum-to-int32 widening, host-control handling) can make
the halves read the same bytes differently.

**B. The user picks a preset in the plugin's browser.**
`HC_PROGRAM` host control → `OnSetHostControl` → load XML from disk or the
bundle → `setPresetXmlFromSelf(xml)`:

```
VST3Controller::setPresetXmlFromSelf(xml)
 ├─ "BinaryMessage"/Preset IMessage → SeProcessor::notify → stateMgr.setPresetFromXml
 │    └─ DawPreset(parametersInfo, xml)  [1]  → setPresetRespectingIpc → ... → store 1, 2
 ├─ DawPreset(parametersInfo, xml)       [2]  → MpController::setPreset → store 3, 4
 └─ notifyDawStateChanged
```

**C. Undo, redo, A/B compare.** `UndoManager` already holds a `DawPreset`
object, but delivers it by serialising to XML and taking route B:

```
UndoManager::undo → setPreset(controller, preset)
 └─ controller->setPresetFromSelf(preset)
     └─ preset->toString(pluginId)            → XML text
         └─ setPresetXmlFromSelf(xml)         → parsed back into two new DawPresets
```

So an undo step is: object → XML → IMessage → parse → object (processor) and
object → XML → parse → object (controller), with the hash recomputed on each.
Serialising for the processor is right. Parsing the text back to update the
controller's own `MpParameter`s is the avoidable half: `MpController::setPreset`
already takes the object.

### 3.4 Save

`SeProcessor::getState` → `stateMgr.getPreset()` → drain both FIFOs → if
dirty, copy `presetMutable` to a new retained `DawPreset` and hash it →
`toString(pluginId)` → write. `VST3Controller::getState` writes nothing;
the controller half of the VST3 state is empty and restore relies solely
on `setComponentState`.

## 4. The same logic, written several times

| Concern | Copies |
|---|---|
| Parameter metadata parsed from `parameters.se.xml` | `::init()` into processor `parametersInfo`; `::init()` into controller `parametersInfo`; `MpController::Initialize` loop into `MpParameter` fields; DSP side from `dsp.se.xml` via `dsp_patch_parameter_base::Initialize` (tinyxml 1) |
| Normalised ↔ real ↔ raw conversion | `MetaDataPolicy::ValueFromNormalised` (DSP); `MpParameter_base::setParameterRaw(MP_FT_NORMALIZED)` and `normalisedToReal`/`RealToNormalized`; free function `normalizedToRaw()` in `ProcessorStateManager.cpp` (a near-verbatim copy of the `MpParameter_base` switch, currently unreferenced); `PatchParameter` `MetaData()` (editor); `GmpiParameter::normalized2Real` (GMPI) |
| Preset XML writer | `DawPreset::toString` (mono, `Param@val`); `DspPatchManager::getPresetState` (adds `patch-list`, per voice); `PatchParameter_base::ExportXmlPreset` (editor); `writePresetXml` (GMPI) |
| Preset XML reader | `DawPreset::initFromXML` (six layouts); `DspPatchManager::setPresetState`; `MpController::ImportPresetXml`/`ImportBankXml`; `gmpi_processor::readPreset` / `setPresetXmlFromDaw` (GMPI) |
| "Keep MIDI-learn assignment if preset has none" | `ProcessorStateMgrVst3::replacePresetMutable`; `MpController::setPreset`; `DspPatchManager::setPreset`; `DspPatchManager::setPresetState` |
| Ignore-program-change filtering | `ProcessorStateMgrVst3::setPresetRespectingIpc` (merge old values in); `MpController::setPreset` (skip); `DspPatchManager::setPreset` (skip); `DspPatchManager::setPresetState` (`mEmulateIgnoreProgramChange`); `DawPreset::initFromXML` (skip in the old-style branch); plus the `ignoreProgramChangeActive` flag on `DawPreset` and the 2 s startup timer in `MpController` |
| "Fill missing parameters with defaults" | `DawPreset::initFromXML`; `setPresetRespectingIpc`; `ProcessorStateMgrVst3::init`; `readPreset` (GMPI). `DspPatchManager::setPresetState` has a TODO saying it does not |
| Raw value container | `PatchStorageBase` (DSP), `paramValue::rawValues_` (shadow), `MpParameter_base::rawValues_` (controller), `PatchStorageBase` again (editor), `std::variant<double, vector<uint8_t>>` (GMPI) |
| Current-preset snapshot | `ProcessorStateMgrVst3::getPreset()` and `MpController::getPreset()` build the same `DawPreset` from different stores |
| HC_PROGRAM_NAME / CATEGORY special-casing | `getPresetState`, `setPresetState`, `serviceQueue`, `MpController::getPreset`, `MpController::setPreset`, `ProcessorStateMgrVst3::init` |

## 5. Thread model

| Object | Thread | Protection |
|---|---|---|
| `dsp_patch_parameter`, `DspPatchManager`, `HandleInterrupt` | audio | none needed; owns store 1 |
| `onSetParameter` → `SetParameterFromProcessor` | audio → any | lock-free FIFO |
| `ProcessorStateMgrVst3::presetMutable`, `presets` | any (host main thread for state calls, 177 ms timer, audio via FIFO only) | `presetMutex`; `currentPreset` atomic |
| `interrupt_preset_` | host main → audio | atomic pointer, lifetime guaranteed by `retainPreset` |
| `MpController`, `MpParameter`, `UndoManager` | UI/controller | none; 35 ms timer drains dsp→ui queue |
| `SeProcessor::notify` | whichever thread the host uses (GUI thread in Live) | pushes into queues only |
| dsp→ui forwarding | background thread in `SeProcessor` | condition variable |

A stale note: `serviceQueue` is commented "must be called under presetMutex
lock" but takes the lock itself, and `getPreset()` reads `presetDirty`
outside the lock.

## 6. The GMPI wrapper and TIDE

`Controller_VST3` and `Processor_VST3` do **not** use `ProcessorStateMgr` at
all. They carry two stores of the identical type instead:
`gmpi_controller_holder::patchManager` (`ControllerPatchManager`) and
`gmpi_processor::patchManager` (`PatchManager`), both
`unordered_map<int, GmpiParameter>` with a single shared writer
`writePresetXml`. `Controller_VST3` *also* inherits `MpController`, so the
SynthEdit panel's `parameters.se.xml` set lives in the same object as the
GMPI native set; the class comment admits the two are "genuinely separate
systems sharing this one object" and the panel's tags are not registered with
the host. For TIDE Rack there is no `parameters.se.xml`, so that
`MpController` half holds zero parameters and the preset virtuals are stubs.

The save path is the opposite of the legacy design (E68): `Processor_VST3::getState`
pulls from the paired controller in-process (`mintFreshPresetXml` →
`sePluginController->syncState()` → `gmpiController.getPresetXml()`), and
falls back to its own store only when no controller is connected. TIDE's
real state is the editor document: `TideApp::exportChunkXmlForSave` serialises
the `CPatchManager`/`PatchParameter` document into parameter 1 (saved) and
`exportDspXml` into parameter 4 (never saved); the processor's `SynthRuntime`
and its `dsp_patch_parameter`s are rebuilt from parameter 4 and treated as
disposable. Knob edits from TIDE's panel reach the running rack through the
"SeBinaryMessage" channel as "ppc" frames (`TideApp::takeDspMessages`).

`ProcessorStateMgrVst3` itself is only reached through `se_vst3` and
`SE2JUCE`. **Decision (Jeff, 2026-10-15): the production SynthEdit VST3
wrapper (`se_vst3`) and SE2JUCE stay untouched for now.** The GMPI VST3
wrapper, new and still untested on Linux, is the test bed for the ideas in
section 7; its users are not yet expecting perfection. The same-process
shortcuts to isolate there are the "GmpiCtlPtr" message (E68) and the
app-object pointer TIDE publishes through `controllerPtrParamId`.

## 7. Observations for the consolidation

Ground rules settled in discussion on 2026-10-15:

- Text (XML) stays the interchange between controller and processor. The two
  halves may be on different threads or in different processes, so no
  in-memory structure can be shared across that boundary. Sharing *code* is
  fine and is the lever: one `DawPreset`, one reader, one metadata builder
  compiled into both halves.
- Metadata may live in a per-process singleton (a function-local static built
  from the bundled XML, immutable, metadata only, no per-side pointers). In one
  process the halves share it; in two processes each builds an identical
  copy. Both sides hold a `const` reference, never a copy. Precedents already
  in the tree: `BundleInfo::instance()` and the GMPI factory's `pluginInfo`.
- Any same-process shortcut (a pointer carried through the host) must sit
  behind an explicit same-process check with the text path as the fallback.
- Work happens in the GMPI VST3 wrapper first (section 6); `se_vst3` is
  production and off limits for now.

1. **The shadow duplicates the controller.** `ProcessorStateMgrVst3` exists
   to answer `getState` without the audio thread. But the controller already
   receives every DSP-originated change through the "ppc"/"lern" echo and
   already builds the identical `DawPreset` in `MpController::getPreset()`.
   E68 in the GMPI wrapper made exactly this move: processor `getState` pulls
   from the connected controller. The residual cases the shadow still covers
   are a processor with no controller (validator, some offline renderers) and
   out-of-process hosting (AUv3), where `DspPatchManager::getPresetState` on
   a stopped processor or a much smaller audio-thread-safe snapshot would be
   the fallback.
2. **Keep XML as the interchange; stop the controller re-reading its own
   output.** Undo, redo and A/B should apply their `DawPreset` object to the
   controller's parameters directly and serialise only for the processor.
   On the processor side, a `shared_ptr<const DawPreset>` handed to the audio
   thread (a same-process, cross-thread hand-off, which is fine) replaces
   `retainPreset`'s unbounded vector.
3. **One `parametersInfo`.** Both halves build it from the same XML with the
   same function; a per-process singleton as described above removes one
   place for the two to drift. The saving is consistency, not CPU: the parse
   is microseconds.
4. **One conversion and one merge.** `normalizedToRaw()`, the
   `MpParameter_base` switch and the `GmpiParameter` arithmetic should be one
   `paramInfo`-driven function; the MIDI-learn-keep and
   ignore-program-change rules should be one operation on `DawPreset`
   (`DawPreset::mergeInto(current, info)`) that the controller, the shadow and
   `DspPatchManager::setPreset` all call.
5. **Decide what `DawPreset` is for voices.** Today it is monophonic by
   assertion while every store around it is per-voice. Either make it carry
   voices (and have `toString` write `patch-list` like `getPresetState`) or
   document that per-voice state is not saved by VST3.
6. **`VST3Controller::getState` is empty.** If the controller becomes the
   source of truth for saves, that is the natural place to write the chunk,
   with `setComponentState` as the restore; the processor would then only
   need to serialise when it has no controller.
