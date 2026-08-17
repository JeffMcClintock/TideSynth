# S11 — does the rack come back on reload?

Measured 2026-08-17 on the macOS box by the weekly scheduled run, against the
mac NEXT row's instruction: *"measure `/tmp/tide-persist3.rpp` reopening before
writing any code."*

**Answer: no, and it cannot today — the editor has no inbound path for the
chunk.** The save side is finished and provably correct; the restore side was
never wired at all. The base64 work ([GMPI#3](https://github.com/JeffMcClintock/GMPI/pull/3)
+ [SynthEditLib#17](https://github.com/JeffMcClintock/SynthEditLib/pull/17)) was
necessary but not sufficient.

No code was changed to establish this, and none needed to be.

---

## 1. The save side works — proven from the file alone

`/tmp/tide-persist3.rpp` (4426 bytes, written 19:15) was left in place by the
previous macOS session for exactly this test. Decoding its `<VST>` block without
running anything:

| Measurement | Value |
|---|---|
| Decoded VST state | **1119 bytes** |
| Outer preset | `<Preset>` with `Param id="1"` carrying a 964-char base64 value |
| Inner payload | **723-byte `<Document>`** |
| Document contents | `<DSP><Module Id="1" Type="Container">` containing `<Module Id="2079404292" Type="Container">` — the placed module |

That is the rack, inside the project file. The writer half of S11 is done.

The decoded document is checked in beside this note as
[s11-restored-document.xml](s11-restored-document.xml) so a later run can diff
against it rather than re-deriving it.

## 2. The restore side cannot work — four one-way facts

Every path that could carry parameter 1 (`chunk`) back into the editor is either
absent or stubbed. These are independent; fixing any one alone is not enough.

**(a) TIDE's inbound parameter callback is an unconditional stub.**
`SynthEditSem/SynthEditController.cpp:94`:

```cpp
ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice,
                        int32_t size, const uint8_t* data) override
{
    return ReturnCode::NoSupport;
}
```

TIDE registers as an `IParameterObserver` (`queryInterface`, same file) and then
ignores every parameter it is handed, the restored chunk included.

**(b) Parameter 1 has no `<GUI>` pin, so no controller→editor route can carry
it.** In `SynthEditSem/SynthEdit.cpp` the chunk pin exists only in `<Audio>`:

```
<Audio>  … <Pin name="chunk" datatype="blob" parameterId="1"/>        (:202)
<GUI>        <Pin name="controllerPtr" datatype="blob" parameterId="0" …/>  (:205)
```

`xml_spec_reader.cpp:676` puts `<GUI>` pins — and only those — into
`info.guiPins`. **Every** controller→editor delivery site in GMPI iterates that
list (`controller_holder.cpp:51, 166, 229, 289, 365, 458, 504, 586`), so
parameter 1 is unreachable from the editor by construction, whatever the preset
holds.

**(c) `onPushChunk` is push-only.** Installed at `SynthEditController.cpp:78`,
called from `TideApp::serviceDocumentSync` (`TideApp.cpp:274`). There is no
inbound counterpart: `grep -rE "ImportXml|importDspXml|OnOpenDocument" SynthEditSem/`
returns nothing.

**(d) The document is blank before any preset is applied.**
`TideApp::InitInstance` (`TideApp.cpp:377-378`) unconditionally runs
`createNewDocument(); Document()->OnNewDocument();`, and it runs from
`SynthEditController::initialize()` — i.e. at construction, ahead of state
restore.

## 3. The processor, by contrast, *does* get it

This is the asymmetry, and it is deliberate on the DSP side.
`processor_holder.cpp:215` seeds a new processor's blob pin from the parameter's
current bytes, with a comment that names this exact case:

> Seed the pin with the parameter's CURRENT bytes, not a default. A processor
> can be created at any time — after restartComponent, for offline rendering, or
> **on state restore** — and without this it would start with an empty blob and
> never be told otherwise, since blobs only reach it when they CHANGE.

So the restored document reaches `Processor::onSetPins` →
`rack.setDocumentXml(...)` (`SynthEdit.cpp:54-58`) and the DSP graph rebuilds.

**The prediction this suggested — "empty rack, audio from an invisible patch" —
was NOT confirmed, and cannot be observed as stated.** See section 4: the host
dies during load before any of it is visible. The trace above stands on its own
(it is a statement about which code paths exist), but it is not the whole story
of what happens on reload.

## 4. Reopening a saved project crashes REAPER — deterministic, 3/3

Established by the concurrent interactive session that owned REAPER
(`open -a REAPER /tmp/tide-restore-test.rpp` → dead within ~30s), and
**independently verified here from the crash reports on this box** rather than
taken on report.

Four reports, not three — `REAPER-2026-08-17-191525`, `-193800`, `-200213`,
`-200926` — all with an identical signature:

| Field | Value |
|---|---|
| Exception | `EXC_CRASH` / `SIGABRT`, `Abort trap: 6` |
| Faulting thread | **0 — the main thread** |
| Top frames | `abort` ← `__abort_message` ← `demangling_terminate_handler()` ← `_objc_terminate()` ← `std::__terminate` ← **`__cxa_rethrow`** ← `objc_exception_rethrow` ← **`-[NSApplication run]`** |

So an uncaught C++ exception escapes through AppKit's event loop and aborts the
process.

**Two things the reports add that the repro alone did not:**

1. **`TIDE_VST3` is loaded in all four crashes**, so TIDE is implicated rather
   than merely present at the time.
2. **TIDE's own frames appear only on `wrapper::Processor_VST3::CommunicationProc()`
   worker threads — never on the faulting main thread.** (Three such threads in
   the 19:15 report, two at 19:38, one each at 20:02 and 20:09 — consistent with
   the previously-journalled second processor instance.)

**This explains the guard's negative result rather than leaving it a puzzle.**
The interactive session wrapped `rack.prepareToPlay(...)` in the processor's
`onSetPins` and it **caught nothing while REAPER still died**. That is exactly
what the reports predict: `onSetPins` runs on the audio/communication thread,
and the throw is on the main thread. The guard was not wrong about its own path
— **it was in the wrong thread.** The DSP-side graph build is thereby largely
exonerated.

That guard has since been **reverted and its branch deleted** — correctly, on
two counts: it wrote `/tmp/tide-load-error.log`, i.e. outside the plugin bundle
(PLAN constraint 4, the same fault [#87](https://github.com/JeffMcClintock/TideSynth/issues/87)
pinned on SynthEditLib), and it was not the fix. `SynthEdit` is back on `master`
at `28907334e`, clean. Recorded here so the attempt is not silently repeated.

**Where this points.** The crash is on the *main/UI* thread — the same side this
trace shows is unwired for the chunk. The editor-side restore is therefore both
missing *and* implicated in a throw, which makes section 5's part 1 the place to
look, not a separate bug.

**Two open questions, deliberately not answered here:**

- **Does the base64 change cause the crash?** *Unknown.* The A/B against a
  pre-base64 binary was never run. Nobody should assume either way — that A/B is
  the cheapest next measurement and the same control that settled the blank-editor
  question earlier in the week.
- **Does `/tmp/tide-persist3.rpp` crash too, or only the larger
  `/tmp/tide-restore-test.rpp`?** Untested, and the difference was thought to be
  a clue (persist3: 4426 bytes, near-empty rack; restore-test: 4568 bytes, with a
  placed module). **One datum against the clue:** there is a crash report
  timestamped `19:15:25`, the same minute persist3 was written, so persist3's own
  session also ended in an abort. Whether that abort was on save or on a reopen
  attempt cannot be told from the report, so this weakens the "only the bigger
  file crashes" reading without settling it.

## 5. What the fix has to be

Two parts, because (a)/(b) and (d) are separate failures:

1. **Give the editor a route.** Either add a `chunk` pin to `<GUI>` so the
   existing `guiPins` machinery delivers parameter 1, or handle it in
   `SynthEditController::setParameter` instead of returning `NoSupport`. The pin
   route reuses code that already works for blobs (`controller_holder.cpp:606`);
   the callback route keeps the GUI spec unchanged.
2. **Import rather than always create.** `InitInstance` must not leave a blank
   document standing when a preset is about to supply one, or the restore will
   race the blank and lose.

Ordering matters: part 2 without part 1 has nothing to import, and part 1
without part 2 delivers a document to an app that has already replaced it.

3. **The load path must fail safe.** An unusable, truncated or foreign document
   must yield an empty rack, never take the DAW down. Section 4 shows the
   current behaviour is the opposite, and this requirement is now the item's
   most important acceptance criterion — a synthesiser that loses a patch is
   unshippable, but one that kills the host on File > Open is worse. Note the
   guard belongs on the **main thread** path, where the throw actually is; the
   audio-thread placement has been tried and does not catch it.

Note also that per PLAN's own ruling on restore — "a new document implies a
rebuild of the DSP graph, and SynthEdit already handles that: fade-out →
teardown → reconstruction → fade-up" — the editor-side import should drive that
existing mechanism rather than invent one.

## 5. Method note

Sections 1-3 came from the saved artefact plus `git`-committed sources; section
4's crash characterisation came from this box's own `.ips` reports. This run
could not drive the GUI — computer-use approval is suppressed for scheduled runs,
and a concurrent interactive session held REAPER — so the repro itself is
another session's, re-verified here from the reports rather than relayed.

Stated plainly, because the three have different strengths:

- **Proven:** the save side works (from the file), and the editor has no inbound
  path for the chunk (from the sources).
- **Proven, second-hand then independently corroborated:** reopening a saved
  project aborts REAPER on the main thread via an uncaught exception.
- **Not established:** that the base64 change causes the crash; that audio would
  return if the crash were fixed; that the two `.rpp` files behave differently.

**Standing trap, unrelated but easy to re-trigger:** `SynthEdit` and
`SynthEditLib` must stay at their paired C12c tips (`28907334e` / `f0e3c92`).
Mismatching them reproduces `redefinition of 'ui_msg_target'` — the public half
adds twelve files to SynthEditLib that the private half deletes from
`SynthEdit2/`, so a mismatch yields two copies through two include roots, and it
reports at the innocent copy. Anyone reverting or bisecting either repo while
chasing the crash will hit this.
