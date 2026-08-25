# TIDE_VCV_FUNDAMENTAL — bundling the VCV/Cardinal Fundamental ports

Written 2026-08-25 (windows, interactive, Jeff directing). The CMake option
that statically bundles the GPL VCV Fundamental module ports into TIDE Rack —
**OFF by default, and the default is a licensing statement**.

## The licensing wall, first

[VCV_Fundamental_gmpi](https://github.com/JeffMcClintock/VCV_Fundamental_gmpi)
carries VCV's module sources byte-for-byte, and the
[SynthEdit_Rack_Adaptor](https://github.com/JeffMcClintock/SynthEdit_Rack_Adaptor)
that hosts them is GPL-3.0-or-later itself. A TIDE binary built with
`TIDE_VCV_FUNDAMENTAL=ON` is therefore **GPL-3.0-or-later** and cannot ship
under TIDE's ordinary terms.

That is why OFF means **completely absent**, not disabled: with the option at
its default, neither repo is fetched, no GPL source is compiled or linked, and
the one call site in TIDE's own code (`TideApp::InitInstance`) compiles out —
the `TIDE_VCV_FUNDAMENTAL` compile definition exists only inside the option's
`if()` in the root CMakeLists. The negative control is
`cmake -B build 2>&1 | grep -i vcv` printing nothing on a default configure.

## Building with it

```
cmake -B build -S . -DTIDE_VCV_FUNDAMENTAL=ON
```

Two more overrides join the standing four, same pattern, for working against
local clones:

| variable | repo |
|---|---|
| `RACK_ADAPTOR_FOLDER_OVERRIDE` | SynthEdit_Rack_Adaptor |
| `VCV_FUNDAMENTAL_FOLDER_OVERRIDE` | VCV_Fundamental_gmpi |

## How it works, in one pass

TIDE cannot load a `.gmpi` bundle (PLAN constraint 7: fixed statically
registered set; S1a deleted the scan), so the modules come in the same way as
every other module TIDE ships — compiled into the binary:

1. **Object libraries.** `VCV_Fundamental_gmpi/static_library/` builds each
   ported module as an OBJECT library and aggregates them as
   `VCV_Fundamental_static` (one target for TIDE to link). Objects, not an
   archive, because registration is a static initializer per module TU and
   archive members that nothing references get dead-stripped — the same rule
   SynthEditSem/CMakeLists.txt's source list documents. The panel SVGs are
   embedded in the binary by `rack_module_resources()`, so nothing stages
   into the bundle.
2. **Deferred registration.** Each module's own `createModel()` line registers
   at static init, but its XML cannot be generated there — a module's
   `RACK_DISPLAY_STATE` is declared after upstream's `.cpp` registers. The
   `.gmpi` build defers via the adaptor's own factory; a static host has no
   factory of its own (RackFactory.cpp's entry points would collide with
   SynthEditLib's), so the adaptor's `RackFactoryStatic.cpp` **queues** the
   registrations instead.
3. **The flush.** `TideApp::InitInstance` calls
   `rack_adaptor::registerDeferredModules()`, which builds every queued XML
   and registers it through `gmpi::RegisterPluginWithXml` — SynthEditLib's
   implementation (UgDatabase.cpp), landing in `ModuleFactory()` exactly like
   `SE TiDE:Panel`. The XML carries the pins, so the both-halves rule is
   satisfied in one step and the enrichment loop needs no entry.
4. **The category is the rack-compatibility switch.** The browser's rack
   scope (`ModuleScope::RackOnly`, EditorLib/SynthEditAppBase.cpp, BACKLOG V4)
   lists prefabs plus modules whose category **starts with "Rack"** — nothing
   else. The ports register as `Rack/VCV Fundamental`
   (cmake/RackModuleMetadata.cmake in their repo), so they appear in the rack
   browser under Rack → VCV Fundamental.

## Lights and displays — FIXED 2026-08-25

The section below described a real gap and is kept for its measurements; the
gap is now closed. The processor drains its rack's feedback queue and
forwards the bytes to the editor as a blob parameter (parameter 2,
`SynthEdit::drainRackFeedback` → `TideApp::receiveRackFeedback` →
`SynthRuntime_editor::receiveDspMessages`), so DSP→GUI parameter updates
reach the GUI for every module, not just these ports. Verified on a running
rack: the VCV LFO's light receives changing values (`0.750`, against a frozen
`0.000` before) and visibly blinks.

Scope's 64 KB display-state frames are now measured too: sustained ~30 Hz for
60 s on the Windows standalone, trace visibly sweeping (reviewed 2026-08-25,
same session, after Jeff caught it freezing). The review found and fixed two
transport defects, one of which is a RULE for anyone touching
`drainRackFeedback`:

- **A GMPI parameter is last-writer-wins, so every pin update must carry
  WHOLE `ppc` messages.** The first cut forwarded arbitrary contiguous fifo
  runs; under display-state load an update was overwritten unsent, the torn
  message desynchronised the editor-side reader (it waits forever on a
  length field read from mid-payload), and the display froze permanently
  after a handful of updates. Whole-message forwarding makes a lost update
  lose only those messages — the stream can never garble.
- **Blob output parameters must not dedup** (`gmpi_processor::setPin` used
  to skip `AddWaiter` when the bytes matched the previous frame — a blob
  here is a stream, not a value; GMPI branch `tide/win/blob-params-are-streams`).

Per-target coverage (VST3/AU3/CLAP/GMPI on all three platforms, plus the
int/bool/enum datatypes via context-menu options) is BACKLOG **E19**.

## The original finding (2026-08-25, before the fix)

The ports' LEDs, VU-style lights and DSP-fed displays (Scope's trace)
do not animate in TIDE, and the modules are not at fault — measured
2026-08-25 with the adaptor's `RACK_ADAPTOR_TRACE` (windows, interactive):
the LFO's processor runs, computes light brightness 0.975 and queues an
update every block; SynthEditLib's patch-parameter machinery forwards every
one (`UpdateOutputParameter` → `UpdateUI`, 1054 in 12 s) — into the
processor's GUI-bound queue, **which TIDE deliberately does not drain**:
`SynthEditSem/SynthEdit.cpp`'s own comment says *"Nothing drains the
GUI-bound queue yet (parameters don't flow in the thin slice)"*. The editor's
light pins receive exactly one initial 0.000 and nothing ever after.

So this is TIDE's standing DSP→GUI parameter-feedback gap made visible for
the first time, not a regression and not the adaptor's plumbing — that was
verified end to end. Closing it means carrying the processor's `ppc`
messages to the controller side in a way that survives the process split
(AUv3), then routing them into the editor's patch manager. Until then,
knobs, jacks, cables and audio all work; nothing that the DSP animates does.

Dead end, so nobody re-walks it: restoring the app timer the wholesale
InitInstance override dropped (`timerhelper`, base
CSynthEditAppBase::InitInstance's last line — the same U2e class of loss)
makes `CSynthEditAppBase::OnTimer` tick and `serviceQueues()` run, and
changes nothing here: that circuit drains the EDITOR-runtime's queue pair,
which TIDE's processor/controller split never uses. Both measured, both
reverted.

## Traps

- **The two module lists must move together.** `modules/CMakeLists.txt` (the
  `.gmpi` build) and `static_library/CMakeLists.txt` in VCV_Fundamental_gmpi
  both name every module; one added to only one entry point ships in one
  world and silently not the other.
- **Never link `SynthEditRackAdaptor` (non-Static) into TIDE.** It carries
  `RackFactory.cpp` as an INTERFACE source, whose `gmpi::RegisterPlugin` /
  `RegisterPluginWithXml` / `MP_GetFactory` definitions collide with
  SynthEditLib's and the wrapper's.
- **tinyxml2 comes from SynthEditLib.** The static build compiles the
  adaptor's SVG parsing against SynthEditLib's vendored copy
  (`RACK_STATIC_TINYXML2_INCLUDE`) and links its symbols; adding the 10.x
  copy the `.gmpi` build uses would be an ODR/version-skew hazard.
