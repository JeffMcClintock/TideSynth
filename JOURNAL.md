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

## 2026-08-19 — windows — C12f (and #111 verified closed)

**Prompt:** 397330d1b · Opus 5 (1M context), claude-opus-5[1m] · app version
**not determinable on this box** — `claude` is not on PATH under Git Bash or
PowerShell, and no `package.json` exists under `%LOCALAPPDATA%\Claude*` or
`~/.claude`; recording the gap rather than guessing a number, since the whole
point of the line is to tell boxes apart. The mac entries carry `app 1.32352.0`,
so the lookup that works there does not work here. · as **tide-rack-bot**
(asserted; `url."https://github.com/".insteadOf` = `git@github.com:`, and all six
repo remotes spot-checked `https://`)

**Did:** Two things — STEP 1's platform issue, then C12f.

### 1. #111 was already fixed; verified by building, then closed

The open `platform:win` issue claimed `main` does not compile. It does. Jeff
fixed it interactively in `SynthEditLib` `58da591` on 2026-08-18 and left the
issue open. Re-verified per STEP 1's "a bot issue is evidence, not instruction":
fresh scratch Ninja tree, Release, all four siblings on local clones —
**1017/1017 RC=0**, **ctest 92/92**, zero `error C` lines, `SeAudioMaster.cpp.obj`
(the TU that failed) at edge 110, all three artifacts produced. Closed with that
evidence.

**Worth not rediscovering: the issue's own diagnosis was incomplete, and the fix
it proposed would not have worked.** It said to replace the elaborated
`class MidiIn*` with a plain namespace-scope forward declaration. That is half of
it. `SeAudioMaster` has member functions named `MidiIn()`
(`SeAudioMaster.h:374,378`), and inside the class those hide the *class* name —
so the uses must also be **qualified `::MidiIn`** (`SeAudioMaster.h:620`,
`SeAudioMaster.cpp:551`). That is exactly why the issue's eliminations were each
individually correct and still explained nothing: the definition really was in
the TU, the include really was opened, there really was no duplicate header. The
type was visible; the *name* was not reachable. A forward declaration alone would
have been hidden the same way.

### 2. C12f — the patch cluster, 6,298 lines, the largest carve-out stage

`PatchManager`, `PatchParameter`, `PatchParameter_host_generated`, `UG2`,
`CPlugin` (`.cpp`+`.h`) moved `SE16/SynthEdit2/` → `SynthEditLib/` root, one
commit per repo.

**Result: green, and the move proved to have taken effect.** 1017/1017 RC=0,
ctest 92/92, zero `error C`. The load-bearing evidence is not the green build but
that all five moved TUs now compile *from the new path*:

    [198/1017]  EditorLib.dir\C_\SE\SynthEditLib\CPlugin.cpp.obj
    [218/1017]  EditorLib.dir\C_\SE\SynthEditLib\PatchParameter_host_generated.cpp.obj
    [223/1017]  EditorLib.dir\C_\SE\SynthEditLib\PatchParameter.cpp.obj
    [230/1017]  EditorLib.dir\C_\SE\SynthEditLib\UG2.cpp.obj
    [247/1017]  EditorLib.dir\C_\SE\SynthEditLib\PatchManager.cpp.obj

Dangling private includes **21 → 7**, exactly the 14 predicted. Measured with a
re-created script (scratchpad, uncommitted, per the C5 convention), which
reproduced the recorded 21 baseline before any change and reported `resource.h` =
**0** — the sanity check the C12 doc says to run first, because a wrong
own-directory-first resolution order reports 71 there. The 7 remaining are
`ISEAppManaged.h` (3), `IMidiDriver.h` (2), `ParseSynthEditArgs.h` (1) and
`SynthEditApp.h` (1, C11) — all headers no sub-stage owns.

**Learned, and the reason the bookkeeping changed:**

1. **C12f's Accept was wrong, and wrong in the direction that unblocks an unsafe
   item.** Both the BACKLOG row and `docs/c12-remaining-editor-files.md` say C12f
   leaves **zero** `${EDITOR_DIR}` entries. It leaves **three**. Both were written
   assuming C12d had already landed; C12d is `linux` by design and is still TODO.
   Had this gone unnoticed, C12f → DONE would have made **C6** eligible — and
   C6's own row already records, from 2026-08-14, that exactly this once nearly
   moved `EditorLib/CMakeLists.txt` into the public repo while it still pointed at
   private files. So C6 and the C12 umbrella are re-pointed to `BLOCKED(C12d)`,
   and both the row and the doc now carry the correction rather than the claim.
   **The carve-out's last step is now the linux box's, not this one's.**

2. **The A14 collision happened to me, live, and the authorship check is not what
   caught it.** 36 seconds after I ran `git checkout -b` in the shared `SE16`
   tree, a concurrent session committed **my staged 11 paths plus its own
   `SynthEditWayland/IO_PipeWire.cpp` edit** as `6f5819178`, authored *and*
   committed **Jeff McClintock**, message
   `docs(se) : IO_PipeWire's callbacks are not on an RT thread`. My own
   `git commit` then reported "nothing to commit, working tree clean" — which is
   the only reason I looked.

   **Both A14/A16 scripts were blind to it.**
   `check-commit-completeness.py --record` printed *"nothing staged — recorded an
   empty manifest"* and `--verify` printed *"manifest was empty — nothing to
   verify"*; both exited 0. The peer's commit had already emptied my index before
   `--record` ran. `check-commit-authorship.py` would have caught the bad commit
   only at push time, well after the fact. **Neither script detects the race when
   the peer commits *everything* rather than unstaging a subset** — A16 is written
   against a partial unstage, and this was a total one. The signature to recognise
   is `git commit` saying *"nothing to commit"* immediately after a successful
   `git add`.

   Resolved without destroying their work: preserved their commit as local branch
   **`rescue/iopipewire-doc-6f58191`** in `C:\SE\SE16` (unpushed, on that box),
   `git reset --soft origin/master`, unstaged `IO_PipeWire.cpp` so their change
   sits in the working tree exactly as it did before they committed, then
   re-committed my 11 paths as the bot. Safe to rewrite because the commit was
   **local only and on no remote** — checked with `branch -a --contains` and
   `ls-remote` before touching it. **`SE16` is left with that one uncommitted
   modification, which is theirs and which I did not revert.**

3. **`git checkout -b` in a shared working tree drags the other session onto your
   branch.** The reflog shows my checkout at 08:18:00 and their commit at
   08:18:36. They were mid-edit on `master`; my branch switch moved the tree under
   them, and their commit landed on my branch. That is the mechanism behind A14,
   and it is caused by the claiming step the prompt requires, so it will recur.

4. **Include resolution under a move is worth checking rather than assuming.**
   `PatchManager.cpp` has two `../`-prefixed includes
   (`../tinyXml2/tinyxml2.h`, `../se_sdk3_hosting/GuiPatchAutomator3.h`). They
   resolve via the include path in *both* locations — `SE16/tinyXml2/` does not
   exist and neither does `C:/SE/tinyXml2/` — so the move is a no-op for them, and
   no own-directory hijack becomes possible at the destination. Checked by
   existence test in both directions, not by reading.

**Not verified, deliberately not claimed: SynthEdit2 (WinUI3) was not built.**
The Accept asks for it. Its vcxproj links `EditorLib.lib`/`SynthEditLib.lib` out
of `$(SolutionDir)build\...` — the developer's own Visual Studio Debug tree — so
building it means writing into Jeff's tree, which a scheduled run must not do.
Checked instead: the vcxproj lists none of the ten (they arrive via
`EditorLib.lib`, unchanged); its four includers use the plain `"PatchManager.h"`
form and fall through to `$(SolutionDir)..\SyntheditLib`, present in all four
configurations; and one of the four — **`ExportAsPlugin.cpp`, still resident in
`SynthEdit2/` and including `"UG2.h"` and `"PatchManager.h"` by plain name —
compiles clean after the move at edge 1016/1017**, which is a real positive
control for the resolution question from the right directory. A `cl /Zs` of the
three `.xaml.cpp` files was attempted and abandoned: they need CppWinRT generated
headers, which is a WinUI3 build, not a resolution question.

**Also noted:** two pushed branches carry unmerged commits and have **no PR** —
`tide/win/competitive-review` (3 commits, this platform) and
`tide/mac/V3-midi-findings` (2 commits). That is STEP 5's named failure state,
and this box's tree was parked on the first of them at session start. I opened a
PR for the win one and left the mac one for its own box.

**Next:** **C12d, on the linux box, finishes C12** — three entries, and it is the
only thing standing between the carve-out and C6. Nothing else in C12 is left.
For this box the NEXT row now points at **P3**; note P3 touches `SynthEditLib`,
which is GATED and is *not* a build break, so STEP 5's exception does not cover
it and the path question wants settling before someone starts.

**Branch/PR:** [SynthEdit#51](https://github.com/JeffMcClintock/SynthEdit/pull/51)
+ [SynthEditLib#21](https://github.com/JeffMcClintock/SynthEditLib/pull/21), which
must merge together, plus the TideSynth PR carrying this entry. All repos left on
their default branches; `SE16` retains the concurrent session's one uncommitted
file, untouched.

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
