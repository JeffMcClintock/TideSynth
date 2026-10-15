# State management on the GMPI wrappers, as it works now

The design that `state-management-consolidation-plan.md` set out is merged
(SynthEditLib #76, GMPI #21, GMPI_Wrappers #43, SynthEdit #79, 2026-10-16).
This is the description of the result: where state lives, how each flow
moves it, which thread touches what, and what keeps the threads apart. The
production `se_vst3` and SE2JUCE paths are unchanged and are not described
here; `state-management-survey.md` still covers them.

## One truth per side

| store | lives in | thread | what it holds |
|---|---|---|---|
| `MpController` (inside `SeGmpiController`, the export's own `<Controller/>`) | plug-in, controller process | UI | every parameter the export declares, with undo, the preset browser, MIDI learn, A/B |
| the engine's `DspPatchManager` | plug-in, processor process | audio | the running values, per voice |
| `gmpi_controller_holder::patchManager` | wrapper, controller side | UI | the GMPI parameters: the DAW-facing ones plus six SE/ carriers |
| `gmpi_processor::patchManager` | wrapper, processor side | audio (while active) | the same GMPI parameters, as the processor last applied them |
| the editor | plug-in, same process as the controller | UI | nothing of its own: it draws from `MpController` through a pointer |

There is no processor-side shadow of the controller: `ProcessorStateMgrVst3`,
its two FIFOs and the retained-preset list are not instantiated on this path.
The wrappers know nothing about SynthEdit; they host one `IController`, one
`IProcessor` and one `IEditor` and move bytes between them.

**Ownership rule.** A parameter is owned either by the DAW or by the engine
preset, never both. A DAW-owned parameter has a GMPI parameter (`Index` N in
`parameters.se.xml` is GMPI parameter N), travels normalised, and the host's
automation lane is the truth for it. Everything else (private parameters,
MIDI-learn assignments, the preset name) travels inside the `SE/EnginePreset`
blob and the DAW never sees it individually. `SeGmpiController::eraseDawParameters`
and `keepDawParametersCurrent` enforce the rule in both directions.

**The six SE/ parameters and one GUI pin** (written by `ExportAsPlugin.cpp`,
found by name through `SeGmpiDescriptor`):

| parameter | kind | direction | carries |
|---|---|---|---|
| `SE/EnginePreset` | blob, saved | controller → processor | the engine-only preset XML, minted at save time |
| `SE/EngineApply` | blob, not saved | controller → processor | a preset to apply now (restore, browser, undo) |
| `SE/DspMessages` | blob, not saved | controller → processor | `MpController`'s "ppc" frames: private edits, MIDI learn, restart |
| `SE/Feedback` | blob, not saved | processor → controller | the engine's dsp→ui frames: CCs, learned assignments, output parameters |
| `SE/ControllerPtr` | blob, not saved | controller → editor (GUI pin 0) | the controller's `IGuiHost2*`, in-process only |
| `SE/StateCounter` | int, saved | controller → host | bumped so the host sees a change it has no parameter for |

XML text is the only cross-process interchange. The one pointer that
travels (`SE/ControllerPtr`) is meaningful only when editor and controller
share a process, which is the only arrangement in which an editor can share
`MpController` at all; an editor in another process gets no panel rather
than a wrong one (`SeGmpiEditor` builds nothing until an 8-byte pointer
arrives).

## The flows

**Save.** The host asks for state on the UI thread. VST3: the processor's
`getState` pulls the chunk from its paired controller (`mintFreshPresetXml`);
CLAP's `stateSave` and AU2's `SaveState` are on the controller side already.
In every case the wrapper first polls its dsp→ui queue
(`drainProcessorFeedback`), then calls `syncState`, then writes one XML with
`writePresetXml`. `syncState` mints `SE/EnginePreset` from `MpController`
minus the DAW-owned parameters. So a save contains the DAW parameters as the
host last set them and the engine's parameters as the controller holds them,
which is never stale: panel edits land in `MpController` as they happen and
the engine's own changes arrived on `SE/Feedback`.

**Restore.** The host hands the chunk to the controller on the UI thread.
`readPresetXml` fills the holder's store, `notifyControllerOfPreset` hands
every value to `SeGmpiController::setParameter`. For `SE/EnginePreset` that
means: fill the DAW-owned slots from the controller's current values
(`keepDawParametersCurrent`, so a restore cannot move a knob the DAW owns),
`MpController::setPreset` (values, GUIs, undo baseline, with `restoring_`
set so nothing is marked dirty), then the same bytes go out as
`SE/EngineApply`. The processor applies them at a block boundary. The
processor also receives its own copy of the chunk from the host (VST3
`setState`, CLAP `stateLoad`): while inactive it writes its store and the
next activation seeds every pin from it; while active it reads the preset
into a scratch store and sends each value through the ui→dsp queue, all of
it or none, so the audio thread's store is never written from the UI thread.
On its first block after `open()` the processor applies `SE/EnginePreset`
once (`enginePresetSeeded_`), which is how a host that runs it with no
controller at all (a validator, an offline renderer) still gets the saved
engine state; later arrivals of that parameter are save-time refreshes and
are ignored, so minting a save never disturbs a running engine.

**DAW automation.** To the processor: a normalised value in the block's
input parameter changes, applied by `setParameterNormalizedDsp` at its
sample offset. To the controller: the host's `setParamNormalized` (or AU2's
parameter listener, or a CLAP in-event) reaches the holder, which calls
`notifyPluginController`; `SeGmpiController` writes the value into the
`MpParameterGmpi` without echoing it back, and leaves it alone if the user
is holding that control (`isGrabbed`), so a late echo cannot fight a drag.

**A panel edit of a DAW-owned parameter.** `MpParameterGmpi::updateProcessor`
→ `nativeToHost` → the holder's `setParameter` → `notifyDaw` → VST3
`performEdit`, a CLAP `CLAP_EVENT_PARAM_VALUE` out-event, or an AU
`AUParameterSet`. The host records it and sends it to the processor the
normal way. The panel never writes a DAW-owned value to the engine directly.
Measured: 8 host edits per drag on VST3 and CLAP, engine pitch following.

**A panel edit of anything else.** `MpController` frames it as it always has
("ppc"); `SeGmpiController::ParamToDsp` flushes the frame immediately as one
`SE/DspMessages` blob (the timer flushes frames written outside `ParamToDsp`,
such as MIDI-learn commands). The holder forwards a changed blob to the
processor through the ui→dsp queue; `SeGmpiProcessor` pushes the bytes into
the engine's own controller→processor queue, which `SynthRuntime` polls every
block. `notifyDawStateChanged` bumps `SE/StateCounter`, and the wrapper marks
the project modified (`setDirty`, `stateMarkDirty`, `kAudioUnitProperty_ClassInfo`).
Measured: a slider drag on a private pitch reaches the engine and the saved
blob (7 → 5.4).

**Engine → controller.** Whatever `dsp_patch_parameter_base::UpdateUI` queues
on the audio thread (a learned CC moving a parameter, a MIDI-learn
assignment, an output parameter) sits in the processor's dsp→ui FIFO.
`drainFeedback` siphons it at the end of the same block and sets the
`SE/Feedback` pin with the bytes. The holder puts the output parameter on
its pending list and, still in `process()`, frames it into the wrapper's
dsp→ui `interThreadQue`. From there: VST3's `CommunicationProc` thread packs
the bytes into `IMessage`s; `Controller_VST3::notify` (which Ableton calls
on the processor's thread) pushes them into the controller holder's
`interThreadQue`; the UI thread drains that in `onTimer`, or at save time.
CLAP and AU2 skip the message hop: controller and processor share a process,
so the holder's queue is read directly. The holder then calls
`notifyPluginController`, and `SeGmpiController` feeds `MpController`'s
dsp→ui byte queue and services it at once: MIDI-learn state, GUIs, and the
value that the next save will carry. Measured: CC 7 moving a learned
parameter, blob 7 → 5.

## Threads, and what keeps them apart

Three threads matter, and on VST3 a fourth:

- **UI thread.** `MpController`, `SeGmpiController`, the editor, every
  wrapper controller call (`setParamNormalized`, `getState`, `setState`
  on the controller, `syncState`), the holders' `pollMessage`, the gmpi
  timers. `MpController` is single-threaded by design and nothing in the new
  code calls it from anywhere else.
- **Audio thread.** The engine, `gmpi_processor::patchManager` while active,
  every `PinSet` the processor receives, `applyEnginePreset`, `drainFeedback`,
  and on CLAP `paramsFlush` while active.
- **The host's state thread** is the UI thread for every host seen, but the
  VST3 processor's `setState` assumes nothing about the audio thread being
  stopped (see Restore).
- **VST3's background thread** (`Processor_VST3::CommunicationProc`) only
  reads the processor's dsp→ui FIFO and sends `IMessage`s; it never touches
  a store.

The mitigations, each with the hazard it answers:

| hazard | mitigation |
|---|---|
| two threads touching one store | each store has one owning thread (table above); the other side only ever sends it bytes. The processor's store is the audio thread's while active, so VST3 `setState` while active goes through the queue instead of writing it, and refuses the whole preset if the queue cannot hold all of it rather than apply half. |
| cross-thread hand-off | every hand-off is a single-producer, single-consumer lock-free FIFO (`interThreadQue` / `lock_free_fifo`: atomic read and write pointers on their own cache lines, no locks, no allocation on the audio side). Frames are self-delimiting, so a byte FIFO can be siphoned in any chunking. |
| a host calling the controller off the UI thread | `Controller_VST3::notify` does nothing but push into a lock-free queue; the UI thread consumes. A latency change is a flag (`std::atomic<bool>`, acquire/release) taken by `onTimer`, because `restartComponent` is UI-thread only. |
| the GUI blocking on a full queue | CLAP `reportToHost` drops an edit when the host is not draining rather than block the GUI; a blob that does not fit the ui→dsp queue waits on the holder's pending list and the latest value is sent when there is room, so no value overtakes an older one. |
| a save-time refresh moving a running engine | `SE/EnginePreset` is applied once per `open()`; everything the controller wants applied now is `SE/EngineApply`. |
| a restore moving a DAW-owned knob | `keepDawParametersCurrent` fills those slots from the controller's live values before `setPreset`. |
| a restore or save marking the project dirty | `hostStateTransfer_` in every wrapper, `restoring_` in the controller. |
| a late host echo fighting a drag | the controller ignores a host-originated value for a parameter the user holds (`isGrabbed`). |
| a headless host that never ticks a timer | the wrapper polls its dsp→ui queue at save time (`drainProcessorFeedback`), so the chunk is as fresh as the last block, not the last tick. On Linux the gmpi timers are pumped by the open editor; a plug-in with no editor open gets its feedback at save. |
| shared memory across processes | none. XML text between controller and processor; the metadata (`seParameterInfo`) is a per-process immutable singleton, shared when in one process and identical copies otherwise. The one pointer (`SE/ControllerPtr`) is only read by an editor in the controller's process. |

Accepted, and written at the place it happens:

- `applyEnginePreset` parses XML and allocates on the audio thread. A preset
  is a few hundred bytes to a few KB, and it happens on open and on an
  explicit apply, not per block.
- `MpController::setPreset` and `DspPatchManager` keep their own copies of
  the MIDI-learn-keep and ignore-program-change rules; folding them into one
  `DawPreset::merge` waits for the production freeze to lift.
- Polyphonic (per-voice) parameter values are not saved by any format; the
  blob carries voice 0, as the se_vst3 chunk always has.
- AU2 mirrors CLAP line for line but has not been compiled.

## What a wrapper has to do

A format wrapper serves this design with five things, and nothing
SynthEdit-specific: create and hold the plug-in's `IController` and set the
holder's `pluginController`; forward host-originated parameter values to it
(`notifyPluginController`); frame non-DAW blobs into the ui→dsp queue
(`sendNonNativeParameterToProcessor`); before a save, poll the dsp→ui queue
then call `syncState`; after a restore, call `notifyControllerOfPreset`.
VST3, CLAP, AU2, AU3 and the Standalone do these.

## Measured

Every path above was run on the Linux VST3 and CLAP wrappers with a negative
control, the numbers are in the plan's Stage 3 sections: DAW → controller →
panel, engine → controller → save (CC 7: blob 7 → 5), panel → engine
(private pitch: blob 7 → 5.4, 1760.7 → 580.1 Hz) and panel → host → DAW →
processor → engine (8 edits, 0.25 → 0.09, 79.1 → 26.4 Hz), with the same
numbers on both formats. Steinberg validator 47 passed, 0 failed throughout.
