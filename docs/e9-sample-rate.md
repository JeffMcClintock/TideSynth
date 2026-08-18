# E9 — how a host sample-rate change actually reaches the rack

**Researched 2026-08-18 (macos, interactive, Jeff directing).** Jeff's question
was narrow: *"for E9 research how SynthEdit's AU target and VST3 target handle
it. Will involve rebuilding the DSP graph, it can't handle sample-rate changes
after audio starts (without rebuilding)."*

The premise is right — a rate change **is** handled by rebuilding the DSP graph,
and the rebuild already exists. What the research changes is **E9's diagnosis**.
TIDE latching its rate in `onSetPins` is not the silent-detune bug the row
describes, because **the object holding the latch does not survive a rate
change**. Every wrapper absorbs a rate change by destroying the `IProcessor` and
constructing a new one.

## The mechanism: the rate is pulled, and the plugin is replaced

`gmpi::api::IProcessor` has exactly three methods —
`open`, `setBuffer`, `process` (`GMPI/Core/GmpiApiAudio.h:50`).
There is no `setSampleRate`, no `setActive`, no `suspend`/`resume`, no `reset`.
**There is nowhere to put a rate-change handler.** The rate is pulled from the
host side, `IProcessorHost::getSampleRate()`, which returns a value cached in the
holder (`GMPI/Hosting/processor_holder.cpp:757`).

The one function that writes that cache is `gmpi_processor::start_processor()`
(`GMPI/Hosting/processor_holder.cpp:48`), and it is not a "prepare" — it is a
construction:

```cpp
blockSize  = pblockSize;
sampleRate = psampleRate;   // :52
events.clear();
processor  = {};            // :55  the OLD plugin object is released here
...
factory->createInstance(info->id.c_str(), gmpi::api::PluginSubtype::Audio, ...);  // :69
processor = pluginUnknown.as<gmpi::api::IProcessor>();
processor->open(host);      // :82
```

The piece that makes re-instantiation re-latch TIDE's rate is deliberate, and its
own comment says why (`processor_holder.cpp:215`):

> Seed the pin with the parameter's CURRENT bytes, not a default. A processor can
> be created at any time — after restartComponent, for offline rendering, or on
> state restore — and without this it would start with an empty blob and never be
> told otherwise, since blobs only reach it when they CHANGE.

TIDE's whole patch is that blob (`pinChunk`, `parameterId 1`,
`SynthEditSem/SynthEdit.cpp:29`). So the fresh instance is re-fed the document,
`onSetPins` fires, and `prepareToPlay` runs at the **new** rate.

## Per-format trigger — one mechanism, four doorbells

| target | what the host calls | where | consequence |
|---|---|---|---|
| **VST3** | `setActive(true)` | `GMPI_Wrappers/wrapper/VST3/Processor_VST3.cpp:369` → `reInitialise()` `:243` → `start_processor` `:247` | rebuild on **every** activation |
| **AU2** | `Initialize()` | `GMPI_Wrappers/wrapper/AU2/AU2_Wrapper.cpp:483` | one rebuild per initialize cycle (`AUBase::DoInitialize` is guarded by `if (!mInitialized)`) |
| **CLAP** | `activate(rate, …)` | `GMPI_Wrappers/wrapper/CLAP/Processor_CLAP.cpp:366` | rebuild per activation |
| **Standalone** | `onAudioFormatChanged(rate, blockSize)` | `GMPI_Wrappers/wrapper/Standalone/StandaloneHost.cpp:405` → `restartProcessor()` `:258` | the only *explicit* rate-change entry point in the tree |

**VST3 never sees `setupProcessing`.** `grep -rn setupProcessing GMPI_Wrappers/
GMPI/ SynthEditSem/` returns nothing — the wrapper does not override it, so
Steinberg's base class merely caches the new rate
(`VST_SDK/public.sdk/source/vst/vstaudioeffect.cpp:171`). That is safe because
the VST3 spec *mandates* the bracket: "Called in disable state (setActive not
called with true) before setProcessing is called and processing will begin"
(`VST_SDK/pluginterfaces/vst/ivstaudioprocessor.h:307`). So a rate change always
arrives as **`setActive(false)` → `setupProcessing(newRate)` → `setActive(true)`**,
and the wrapper hangs the rebuild on the trailing `setActive(true)`.

**AU enforces the same bracket, passively.** `AU2_Wrapper::StreamFormatWritable`
returns false while initialised (`AU2_Wrapper.cpp:978`), which the SDK consults
before accepting either `kAudioUnitProperty_StreamFormat` or
`kAudioUnitProperty_SampleRate`, so a rate change on an initialised unit is
rejected with `kAudioUnitErr_PropertyNotWritable` and the host must
`Uninitialize` first. Note **`Uninitialize` tears down nothing** — the wrapper
overrides neither `Cleanup()` nor `Uninitialize()`, so the old graph stays alive
until the next `start_processor` drops it.

**TIDE builds no AU target today** — `SynthEditSem/CMakeLists.txt:59` is
`set(FORMATS_LIST GMPI VST3 STANDALONE)`. `gmpi_plugin.cmake` accepts `AU`, so
this is generic wrapper code TIDE would inherit, not code it exercises now.

## The engine side already rebuilds on a rate change

`SynthRuntime::prepareToPlay` needs no new machinery
(`SynthEditLib/SynthRuntime.cpp:48`):

```cpp
const bool mustReinitilize =
    documentPendingNow ||
    generator == nullptr ||
    generator->SampleRate() != sampleRate ||          // <-- rate change, already handled
    generator->BlockSize() != generator->CalcBlockSize(maxBlockSize) ||
    (!runsRealtime && runsRealtimeCurrent);
```

and the rebuild is the real thing: `ServiceDspRingBuffers()` → `HandleInterrupt()`
→ `getPresetsState()` → `generator->Close()` → `ResetMessageQues()` →
`generator = {}` → `new SeAudioMaster(sampleRate, …)` → `BuildDspGraph()` →
`OpenGenerator()` (`SynthRuntime.cpp:116-159`), synchronously on the calling
thread.

**Precedent for where the call belongs.** SynthEdit's own format glue does not
wait for a parameter:

- `SynthEdit/se_gmpi/source/SeGmpiProcessor.cpp:151` — overrides `open()` and
  calls `prepareToPlay(this, host->getSampleRate(), host->getBlockSize(), true)`.
- `SynthEdit/se_vst3/source/adelayprocessor.cpp:253` — `reInitialise()` calls
  `prepareToPlay(this, processSetup.sampleRate, …)`, from both `initialize()`
  (`:328`) and **`setActive(true)`** (`:494`).

TIDE overrides neither `open()` nor `onGraphStart()`
(`GMPI/Core/Processor.h:302`, `:307`); its only `prepareToPlay` call site is the
chunk arriving in `onSetPins`.

## Measured in REAPER, 2026-08-18

REAPER launched from a shell on `tests/hosts/v3-midi-pitch.rpp`, stderr captured,
then **Preferences → Audio → Device → Request sample rate** driven by hand
48000 → 44100 → 48000 on the loaded project (the GUI route E9's row said this
needs — REAPER ignores a project's `SAMPLERATE` when rendering, and a
hand-written `RENDER_SRATE` stops it on a dialog):

```
TIDE: 5 rack prefab(s) seeded from the bundle
TIDE: root MIDI-CV seeded (MIDI In … -> MIDI-CV 2 … -> facade …)
TIDE: rack built for 48000 Hz, block 512
TIDE: rack built for 44100 Hz, block 512
TIDE: rack built for 44100 Hz, block 512
TIDE: rack built for 44100 Hz, block 512
TIDE: host MIDI reaching the rack - first message 8 byte(s), status 0x40
TIDE: rack built for 44100 Hz, block 512
TIDE: host MIDI reaching the rack - first message 8 byte(s), status 0x40
TIDE: rack built for 44100 Hz, block 512
TIDE: rack built for 48000 Hz, block 512
TIDE: rack built for 48000 Hz, block 512
```

Two things to read off this:

1. **The rack does rebuild at the new rate** — and playing the project afterwards
   metered **−6.2 dBFS peak / −13.4 RMS**, the same level the fixture renders at
   48 kHz, so the rebuilt graph works.
2. **Not one line carries the `(rate CHANGED)` suffix.** That branch fires only
   when `preparedSampleRate != 0` (`SynthEditSem/SynthEdit.cpp:99-107`), and
   `preparedSampleRate` is a *member* of the object `start_processor` destroys.
   **The guard cannot fire under any of these wrappers, by construction** — every
   line above is a fresh instance latching once. The repeated identical `44100`
   lines are the proof: an instance that survived with an unchanged rate would
   print nothing at all.

Also note REAPER re-instantiates on far more than the rate change itself — the
`host MIDI reaching the rack` line is one-shot per instance too, and it printed
twice for one playback.

## What this means for E9

**Two comments in `SynthEditSem/SynthEdit.cpp` were wrong and are fixed in the
same PR as this file.**

1. `preparedSampleRate`'s comment inferred *"on this evidence it would not be
   [handled]: the rack would keep the stale rate and everything would be detuned
   by the ratio, silently."* The evidence (no `(rate CHANGED)` line) is real; the
   inference does not follow, because the wrappers replace the object.
2. *"The AsyncRestart path is unreachable in the plugin runtime — nothing enters
   `eRuntimeState::resetting`"* is **false**. `resetting` has one writer,
   `SeShellDsp::OnFadeOutComplete()` (`SynthEditLib/iseshelldsp.h:124`), reached
   in a plugin via `ug_vst_out.h:65` → `SeAudioMaster::onFadeOutComplete()`
   (`:1509`) → `getShell()->OnFadeOutComplete()`. `ug_vst_out` **is**
   `audioOutModule` in a plugin (`SetupVstIO()` runs under
   `if (!getShell()->isEditor())`, `SeAudioMaster.cpp:502`, `:864`). And
   `DoAsyncRestart()` is reached from engine code that runs in a plugin:
   `dsp_patch_parameter.cpp:773` when a host control with
   `requiresAsyncRestart()` changes — a set that **includes `HC_PATCH_CABLES`**
   (`dsp_patch_parameter_base.h:187`), i.e. every rack re-cabling — plus a
   mid-stream latency change (`SeAudioMaster.cpp:1432`) and
   `ug_delay.cpp:162`. What is true is only that *nothing in TIDE calls it yet*.

**The real gaps, none of which is "the rack goes out of tune":**

- **A fresh instance with no chunk stored never prepares at all.**
  `processor_holder.cpp:225` `continue`s on an empty blob — "nothing stored yet;
  a later change will deliver it" — and TIDE's only `prepareToPlay` call site is
  that blob arriving. Fixing this is the one clearly worthwhile code change, and
  it has a precedent to copy exactly: override `open()` like
  `SeGmpiProcessor.cpp:151` does, and let the blob arrival be a pure document
  swap.
- **`DoAsyncRestart()` alone cannot absorb a rate change.** The `resetting`
  branch rebuilds from the *member* `sampleRate` (`SynthRuntime.cpp:388`), which
  is only ever written by `prepareToPlay` (`:33`). A faded rate change would need
  the new rate installed first. Those members are also plain `int32_t`, written
  outside `generatorLock` and read on the audio thread — a formally racy hand-off.
- **`prepareToPlay` never joins `dspBuilderThread`.** Calling it while a
  background build is in flight destroys the `SeAudioMaster` the worker is
  writing into. Its precondition is the wrappers' own: no concurrent `process()`.
  TIDE currently calls it *from* the audio thread, which is race-free but parses
  XML and builds the graph there.
- **A host that changes rate without the deactivate/activate bracket gets
  nothing.** In CLAP that is a host bug clap-helpers actively reports
  (`plugin.hxx:349`). The plugin cannot force its own reactivation on the CLAP or
  AU path; `restartComponent` appears only in the VST3 controller, for latency
  (`Controller_VST3.cpp:547`).

## Not established

- **AU is unmeasured and currently unmeasurable** — TIDE has no AU target, so
  the AU chain above is read, not run. Whether Logic uninitialises on a device
  rate change (rather than destroying the AU) is host behaviour; both routes end
  at a fresh `IProcessor`, so the conclusion holds either way.
- **CLAP is unmeasured** for the same reason.
- **Thread-safety of parsing the document on the audio thread** was not audited,
  only noted. `AU2_Wrapper.cpp:445` says `Initialize` runs on a background thread
  in Logic, while `onSetPins`/`prepareToPlay` run later on the audio thread.
- The AU2 wrapper has two vestigial paths worth knowing before anyone adds an AU
  target: `reInitialize()` does not update the `AU2_Wrapper::sampleRate` member
  that `getSampleRate()` returns, and `offLineRenderMode`'s only consumer
  (`prepareToPlay(…, 0 == offLineRenderMode)`) is inside `#if 0`
  (`AU2_Wrapper.cpp:462`), so the offline-render property triggers a rebuild
  whose purpose has been amputated. Tempo/PPQ reporting is `#if 0` too (`:740`).
