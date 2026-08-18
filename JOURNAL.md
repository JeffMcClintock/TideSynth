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

## 2026-08-19 — macos — E9's sliver was a silence writer, and next door to it was a live host crash (interactive session, Jeff directing)

**Did:** verified the scheduled run's E9 correction independently (it holds, and is
stronger than it claimed), then measured the two things nobody had measured, and
shipped TIDE's half of both.

**Result 1 — a malformed saved chunk was a LIVE HOST CRASH.** The previous run filed
the `SeAudioMaster.cpp:410` deref as latent, reachable only if something prepared
before a document existed. It is reachable now. A REAPER project whose saved TIDE
chunk is `<Patch/>` — well-formed XML, wrong root — **segfaulted the render**:
`EXC_BAD_ACCESS at 0x28`, `TiXmlNode::FirstChildElement` ← `BuildDspGraph` ←
`prepareToPlay` ← `onSetPins` ← `Processor_VST3::process`, on a REAPER worker thread.
The trigger is "no `<Document>` root", **not** "empty": `<Patch/>` parses with no
error, so `RootElement()` is non-null and `SynthRuntime.cpp:76`'s bundle fallback
never runs — the absent `dsp.se.xml` is irrelevant on that route. Nothing validated
the bytes anywhere: `gmpi::base64Decode` silently skips anything outside its
alphabet, so a truncated or hand-edited project file is enough.

**Result 2 — an unprepared TIDE never wrote its output buffers.** `subProcess` was
installed only at the tail of `onSetPins`, which never runs for a no-chunk instance
(no audio inputs → no `PinStreamingStart`; outputs skipped; MIDI has no default; the
empty blob `continue`s at `processor_holder.cpp:226`). A/B measured in REAPER, same
build except two lines: without the `open()` install the log shows `host MIDI arrived
BEFORE the rack was prepared` and **no** silence line; with it, `TIDE: unprepared -
writing silence to the host's output buffers`. **REAPER is not exposed** — its render
measured identical either way — so this is a contract fix, not an audible bug on this
host. Other hosts are untested, and VST3 does not guarantee zeroed buffers.

**Shipped, both in `SynthEditSem/SynthEdit.cpp` (TIDE's own, ungated):** a
`documentIsBuildable` check that refuses a chunk which is not
`<Document>`/`<DSP>`/`<Module>`-shaped and logs why, and an `open()` override that
installs the silence writer immediately. `TIDE_VST3` Release builds clean;
`tests/hosts/v1-rack.rpp` still renders −6.3 dBFS / −17.0 rms / 2 cables, so no
regression.

**Learned — the honest boundary of TIDE's guard, measured rather than assumed.** A
`<Document><DSP><Module/></DSP></Document>` skeleton passes everything TIDE can check
and **still crashes** (a `<Module>` with no `<PatchManager>` reaches
`ug_container.cpp:1469`). TIDE can refuse the wrong *shape*; it cannot validate the
engine's schema. My first harness used that skeleton as its positive control and the
run correctly failed — the real chunk from `tests/hosts/v3-midi-pitch.rpp` is the
positive control now, and the skeleton is reported as a KNOWN LIMIT so nobody reads it
as a pass.

**Learned — E10's Accept clause would have passed while the process still crashed.**
`SynthRuntime.cpp:157` calls `OpenGenerator()` unconditionally after `BuildDspGraph`,
and `SeAudioMaster::Open()` dereferences `main_container` at `:2330`, which is null
after ANY early return from the build — including the `:413` return already there. So
"move the guard one line, rerun the probe, see the return" is a fix that does not fix.
The row now says so, and E10's real scope is at least three deref sites plus a way for
`SynthRuntime` to learn the build failed.

**Learned — one trap for the next person measuring this.** A fixture cannot be
hand-edited to carry a different chunk: REAPER's VST3 state block embeds length fields
(`header[8] = len(body)`, `body = u32(len(xml)+4), u32(1), u32(len(xml)), xml, 8 zero
bytes`), so the block has to be *synthesised*. And a hand-written `<VST …>` line with
no state does not instantiate at all — REAPER says "the following effects … are not
available", which looks exactly like a passing test if you only read the rendered
audio. Both are handled in
[scripts/measure-chunk-robustness.py](scripts/measure-chunk-robustness.py).

**Also corrected in [docs/e9-sample-rate.md](docs/e9-sample-rate.md):** the probe's
build command (`../TideSynth/…` → `../../TideSynth/…`, since TideSynth is a sibling of
SynthEditLib, so the command as shipped could not compile) and the
`BundleInfo.cpp:542` citation, which is the `_WIN32` branch — the mac path these
measurements ran on is `:581-637`.

**Next:** **E10** (GATED; rewrite its Accept clause first). **E11** filed but wants a
measurement, not a patch: `processor_holder.cpp:231` publishes a raw pointer into a
vector another thread may reallocate, flagged unverified by an agent and not chased.

**Branch/PR:** `tide/mac/e10-chunk-guard` in both TideSynth and SynthEdit.

---

## 2026-08-19 — macos — E9 (re-specced; E10 and A26 filed)

**Prompt:** 397330d · Opus 5 (1M context), claude-opus-5[1m] · app 1.32352.0 · as tide-rack-bot

**Did:** Continued E9 on this branch per STEP 2 (open PR #149 from my own
platform names it). Before writing the `open()` override the row and the
research doc both recommended, I checked the one thing neither had: whether the
precedent actually transfers. **It does not, and implementing it as written
would have null-dereferenced.** Wrote that up, added the probe that proves it,
re-specced E9, and filed the two things it exposed.

**Result: the recommended fix would crash, measured with positive controls.**
`SeGmpiProcessor::open()` may call `prepareToPlay` immediately because an
exported SynthEdit plugin bakes its graph into the bundle as `dsp.se.xml`. TIDE
has no such resource — the document arrives at runtime as the blob — confirmed
against the installed bundle:

    $ ls ~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3/Contents/Resources
    ControlsXp.xml  Converters.xml  MidiPlayer2.xml  Prefabs  SubControlsXp.xml

So preparing from `open()` walks: `mustReinitilize` is forced by
`generator == nullptr` (`SynthRuntime.cpp:48-53`) -> no root, so it falls back
to the bundle resource (`:76-79`) -> `BundleInfo::getResource` finds no file and
returns `{}` (`BundleInfo.cpp:542-546`) -> `Parse("")` errors, `RootElement()`
is null -> `BuildDspGraph` runs anyway (`:147`), and:

    document_xml = hDoc.FirstChildElement("Document").Element();   // :409 -> nullptr
    pElem = document_xml->FirstChildElement("DSP");                // :410 -> DEREFERENCES IT
    if (!pElem)                                                    // :413 -> one line too late
        return;

The verification artifact is `tests/e9_buildgraph_null_probe.cpp`, which
reproduces `SeAudioMaster.cpp:403-413` verbatim against the real
`SynthEditLib/tinyxml` sources. It ran clean from the committed copy:

    --- EMPTY document  (TIDE with no chunk pushed) ---
      RootElement()       : NULL
      document_xml (:409) : NULL
      -> SeAudioMaster.cpp:410 would dereference this NULL pointer.
    --- POSITIVE CONTROL: <Document> with no <DSP> ---
      document_xml (:409) : non-null
      pElem   (:410)      : NULL  -> guard at :413 returns cleanly
    --- POSITIVE CONTROL: <Document><DSP/> ---
      pElem   (:410)      : non-null  -> guard at :413 passes

**The two controls are the point.** The middle case is exactly what the existing
`if (!pElem)` guard was written for, and it passes — so the NULL in the first
case is the code's behaviour, not the probe failing to run.

**Learned, and worth not rediscovering:**

1. **"It has an exact precedent to copy" is a claim about TWO call sites, and
   the 2026-08-18 research only checked one.** The asymmetry that kills it —
   `SeGmpiProcessor` always has a document at `open()`, TIDE never does — is
   invisible from the precedent's own source. This is the second time an E9
   conclusion has been confidently wrong in the same direction: the row's
   original "silent detune" diagnosis was also an inference nobody had run.
2. **The guard at `SeAudioMaster.cpp:409-413` is one line short of its own
   intent.** Its comment ("should always have a valid root but handle gracefully
   if it does" — garbled in the original) shows defensiveness was meant. Filed
   as **E10**, GATED, NOT fixed: `SynthEditLib` is gated and this is a latent
   crash, not a build break, so STEP 5's build-break exception does not apply.
   It is not live today because every current caller has a document.
3. **STEP 2's continue-a-branch rule trips STEP 4's authorship check, and I hit
   it.** This branch was started by an interactive session, so its two commits
   are authored `Jeff McClintock`; `check-commit-authorship.py` defaults to
   `origin/main..HEAD`, sees them, and prints **"Do not push"** — for commits
   already pushed before this run began, which STEP 4 separately forbids
   rewriting. **My own two commits are clean** (`--range e01bb72..HEAD` ->
   "all commits authored by tide-rack-bot"). Filed as **A26** with the
   `--range` workaround. **Being honest about the order I did this in:** the
   push and the check ran as separate statements, so the push went out before I
   had read the check's verdict. It happened to be the right outcome, but I did
   not decide it first. A run that gets used to pushing past "Do not push" on
   continued branches is exactly the failure A14 wrote that check to catch.

**Not verified, deliberately not claimed:** I did **not** build TIDE or
SynthEdit this run, so I cannot say whether `main` builds on this box today.
Nothing I changed is compiled into either — the commits are docs, a standalone
probe, and BACKLOG rows. The probe itself compiled and ran clean under
`clang++ -std=c++17` against SynthEditLib's tinyxml, which says the toolchain
works and nothing more. Whoever takes E10 must build **SynthEdit as well as
TIDE**: `SeAudioMaster.cpp` ships in both.

**Next:** E9 is `NEEDS-SPEC` and should stay there until someone answers what
TIDE prepares with before a document exists — a no-op guard restores today's
behaviour and buys nothing, and a minimal stand-up document is a design call
(`SeAudioMaster.cpp:421-422` asserts the first `<DSP>` child is a
`Module`/`Container`, so "empty" is not free). E10 unblocks the safety half and
is one line, but it is GATED. The mac NEXT block now points at **E2** or the
per-prefab **E1** cases instead — coverage work with stated acceptance checks.

**Branch/PR:** [#149](https://github.com/JeffMcClintock/TideSynth/pull/149) —
continued rather than branched fresh, per STEP 2; a fresh branch would have
conflicted with it on `BACKLOG.md`, `JOURNAL.md` and `docs/e9-sample-rate.md`.
All four working copies (TideSynth, SynthEdit, SynthEditLib, GMPI) were clean at
start and are left on their default branches.

## 2026-08-18 — macos — E9 researched: a rate change is absorbed by REPLACING the plugin, not by re-reading the rate (interactive session, Jeff directing)

**Did:** answered Jeff's question — *how do SynthEdit's AU and VST3 targets handle
a host sample-rate change, given it requires rebuilding the DSP graph* — by
reading all four wrappers and then **measuring a live rate change in REAPER**.
Wrote [docs/e9-sample-rate.md](docs/e9-sample-rate.md), corrected E9's row, and
fixed two wrong comments in `SynthEditSem/SynthEdit.cpp`.

**Result: the premise is right, E9's diagnosis was wrong, and the correction is
the finding.** The rebuild Jeff expected already exists and is already
rate-triggered — `SynthRuntime::prepareToPlay` rebuilds when
`generator->SampleRate() != sampleRate` (`SynthRuntime.cpp:51`). What no wrapper
does is *tell a running plugin* about a new rate. `gmpi::api::IProcessor` has
three methods — `open`, `setBuffer`, `process` (`GMPI/Core/GmpiApiAudio.h:50`) —
so there is nowhere to put such a callback. Instead
`gmpi_processor::start_processor` (`GMPI/Hosting/processor_holder.cpp:48`)
**destroys the IProcessor** (`:55`), **creates a new one** (`:69`), calls `open()`
(`:82`), and re-seeds the blob parameter from its retained bytes (`:215`) — so
TIDE's chunk arrives again, `onSetPins` runs again, and the rack is built at the
new rate. Doorbells: VST3 `setActive(true)`, AU `Initialize()`, CLAP `activate()`,
standalone `onAudioFormatChanged`.

**The measurement, since this row had never had one.** REAPER launched from a
shell on `tests/hosts/v3-midi-pitch.rpp`, then **Preferences → Audio → Device →
Request sample rate** driven by hand 48000 → 44100 → 48000 on the loaded project
(the GUI route the row said this needs). Eight `TIDE: rack built for N Hz` lines,
the rate following the device every time, and playback afterwards metering
**−6.2 dBFS peak / −13.4 RMS** — the level the fixture gives at 48 kHz. Device
and preference left exactly as found; REAPER quit cleanly.

**Learned — the thing that reframes the row.** **Not one of those eight lines
carried the `(rate CHANGED)` suffix, and it never can.** `preparedSampleRate` is
a *member* of the object `start_processor` destroys, so it is re-zeroed with each
new instance. The guard cannot outlive the rebuild that handles the change. The
repeated identical `44100` lines are the proof — an instance that survived with an
unchanged rate would print nothing at all. **My earlier comment drew the wrong
conclusion from correct evidence** (it inferred "the rack would keep the stale
rate and everything would be detuned, silently"); the evidence was the absence of
a line that is structurally impossible.

**Second wrong comment, also fixed:** *"the AsyncRestart path is unreachable in
the plugin runtime — nothing enters `eRuntimeState::resetting`"*. `resetting` is
entered via `ug_vst_out.h:65` → `SeAudioMaster::onFadeOutComplete()` (`:1509`) →
`OnFadeOutComplete()` (`iseshelldsp.h:124`), and `ug_vst_out` **is**
`audioOutModule` in a plugin (`SetupVstIO()` runs under `!isEditor()`,
`SeAudioMaster.cpp:502`). `DoAsyncRestart` is reached from
`dsp_patch_parameter.cpp:773` for any host control with `requiresAsyncRestart()`
— a set that **includes `HC_PATCH_CABLES`**, i.e. every rack re-cabling. Nothing
in TIDE calls it *yet*; that is TIDE's wiring, not the runtime's limits.

**What is actually left of E9, and it is smaller:** a fresh instance with **no
chunk stored never prepares at all** — `processor_holder.cpp:225` `continue`s on
an empty blob, and TIDE's only `prepareToPlay` call site is that blob arriving.
The fix has an exact precedent in SynthEdit's own glue: override `open()` like
`se_gmpi/source/SeGmpiProcessor.cpp:151`, and let the blob be a pure document
swap. Two caveats for whoever does it: `DoAsyncRestart()` alone cannot absorb a
rate change (the `resetting` branch rebuilds from the member `sampleRate`,
`SynthRuntime.cpp:388`, which only `prepareToPlay` writes), and `prepareToPlay`
never joins `dspBuilderThread`, so its precondition is no concurrent `process()`.

**Not claimed:** AU and CLAP are read, not run — TIDE builds neither
(`SynthEditSem/CMakeLists.txt:59` is `GMPI VST3 STANDALONE`). Whoever adds AU
should know `reInitialize()` does not update the `AU2_Wrapper::sampleRate` that
`getSampleRate()` returns, and that `offLineRenderMode`'s only consumer is inside
`#if 0`.

**Next:** either take the `open()` latch above, or E2 / the per-prefab E1 cases.

**Branch/PR:** `tide/mac/e9-research` (TideSynth), `tide/mac/e9-comment-fix`
(SynthEdit).

---

## 2026-08-18 — macos — PLAN's v0.1 acceptance test is COMPLETE (interactive session, Jeff directing)

**Did:** merged the last three PRs, synced the fleet, rebuilt against updated
dependencies and re-measured everything. **Every clause of PLAN's v0.1 acceptance
test now passes, measured.** V3 and E8 are DONE and archived.

**The acceptance test, clause by clause, all measured rather than argued:**

| clause | evidence |
|---|---|
| loads in a DAW, shows the rack | `TIDE: 5 rack prefab(s) seeded from the bundle`, editor opens |
| drop in an oscillator and an envelope as prefabs | E2a, DONE |
| cable them to an output | 4 patch cables in `HC_PATCH_CABLES` |
| **play it from the DAW's MIDI** | **261.6257 Hz for a middle C — +0.001 cents** |
| patch survives save-and-reload | −6.3 dBFS, 440.0 Hz, cables intact |

Five host fixtures in `tests/hosts/`, E1 **4/4**, all re-run from merged `main`.

**Where this session started:** nobody had ever heard TIDE make a sound after a
host reload, and V1 had been blocked for weeks behind a circular dependency with
E2a. It ends with the whole v0.1 bar cleared and four checked-in fixtures anyone
can re-run in one command. That last part is the real change — this stopped being
something the project reasons about and became something it measures.

**Rows closed today:** V1, E2a, V3, E8 — all archived. **A25** landed too (the
NEXT-block check now actually gates `lint`, proven by a two-commit probe).

**Still open, and none of them blocking:** **E9** (TIDE latches its sample rate at
document-push time with no rate-change path — the nearest thing to a live defect
left, and the new `mac` NEXT target), **E7** (polyphony cannot escape a container —
V3 side-stepped it by keeping the MIDI-CV at the root, so it is an
engine-limitation row now rather than a blocker), **E6**, **S8**, **E2**, and
**E5**/**A25**-style items needing Jeff.

**Dependency churn checked rather than assumed.** The sync pulled GMPI_Wrappers
`ea2e357 → ebf8cfe`, GMPI-plugins `5c1c6e5 → 79e3f92` and synthedit-website. TIDE
builds against local overrides of GMPI and GMPI_Wrappers, so those land in the
plugin on the next build — which is exactly the kind of thing that silently
invalidates a measurement. So TIDE was rebuilt against them and every fixture
re-measured **unchanged**, including the pitch at +0.001 cents. Both deltas were also audited
and both came back `affects_tide: no` at high confidence, which explains the
unchanged numbers rather than just corroborating them:

* **GMPI-plugins** deletes one stale unused `FreqAnalyser.xml` that was never a
  build input (its CMakeLists never passed `HAS_XML`; the plugin registers inline
  in code under a different id), and GMPI-plugins is not in TIDE's build at all.
* **GMPI_Wrappers** is 20 files, all under `wrapper/Standalone/` or `mcp/` —
  nothing under `wrapper/VST3/`, `wrapper/common/` or the shared
  `GMPI_HOSTING_SRCS`. It teaches the standalone to notice when its audio device
  dies (`isStreamRunning`/`stoppedReason`) and hardens the Windows named-pipe IPC.
  `Processor_VST3.cpp` is not in the changed-file list at all, and no added or
  removed line mentions ump/noteon/pitch/bend — so the class of bug fixed today is
  not in scope. The one CMake risk was real and is clear: `add_subdirectory(Standalone)`
  is unconditional, so a configure error there would break TIDE's configure even
  though the VST3 never links the target; the sole change is one header added to
  `standalone_mcp_srcs`, and the file exists.

**Two caveats from that audit worth keeping, both confined to `TIDE_STANDALONE` —
the developer target this project uses for screenshot/click/render work.** (1) The
MCP `info` reply now gates `sampleRate`/`bufferFrames` on a live driver poll and
adds an `audioStopped` field, **so a harness that reads `sampleRate` out of `info`
will find it ABSENT rather than stale once a stream has died** — a JSON-shape
change in the measurement tooling, not in what TIDE renders. (2) Windows only:
WASAPI's `Start()` moved inside `open()`, so a device that refuses to start now
fails the open instead of returning success and playing nothing. Strictly better,
but a real behaviour change if anyone drives the Windows standalone.

**Learned, and it is the pattern of the whole session.** Six of my own hypotheses
died today, and every single one failed the same way: **I attributed a silence or
an error measured at the END of a chain to a component inside it.** The tool (E6),
the wire (the missing converters), the module (MidiToGate2), the sample rate, the
convention, the JUCE block. Each time the fix was to measure at the suspected link
instead — a `Sound Out` inside the container, a trace in the module, the authors'
own `DMIDI_LOG`. And twice the *instrument* was the thing at fault: the render tap
cannot see inside a container, and a 0.25 s Goertzel window cannot separate two
candidates 1.2 Hz apart. **Validate the instrument on a case whose answer you
already know, before believing what it says about the case you care about.**

**Side effects on this box:** no REAPER this round — every measurement was a
headless render. TIDE_VST3 rebuilt Release from merged `master`. All fourteen repos
on their default branches and clean; the three merged feature branches deleted and
pruned. `AlphaBlender` remains parked on `DrawOnImage`, `VST_SDK` detached at its
SDK tag, both deliberately.

**Next:** **E9**.

**Branch/PR:** `tide/mac/v3-e8-done`.

