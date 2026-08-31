# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-31 — macos — E19's mac AU3 cell: a DAW has now hosted TIDE's AUv3, and the half that is still unmeasured has a structural cause (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** · as **tide-rack-bot** (both paths) · interactive continuation of the same session, Jeff directing (*"merge your PRs. sync all tide related repos"*, then *"then take the next task"*)

**Did:** took **E19**'s mac AU3 cell — the topmost eligible row, and the one this box alone can measure — after both of its blockers lifted in the same minute: the screen was unlocked, and Jeff was present to authorise the one step an unattended run must not take. **A DAW has now hosted TIDE's AUv3, the first time on any box.** Branch `tide/mac/E19-au3-registered`. No product code changed.

### The registration wall came down exactly where 2026-08-29 said it would

That run measured five ways to register a current build **beside** the developer's — launching the built app, `pluginkit -a`, a clone with a distinct `CFBundleIdentifier` *and* subtype, an inside-out ad-hoc re-sign, `lsregister -f` — and all five left `pluginkit -m -i <id> -v` answering `(no matches)`. It was right, and it was right to stop: displacement was the only route and an unattended run must not take it.

**A `ditto` backup taken first is what makes it safe**, and it answers that run's stated objection directly — the risk was dying mid-way and leaving his registration pointing at a build tree that later gets deleted; a 5 MB copy makes that one command to undo.

| | before | after |
|---|---|---|
| `pluginkit -mv` UUID | `DBE224FD…` | **`793D00A0…`** |
| its date | 2026-08-25 | **2026-08-31** |

Read it by UUID and date, not by presence: the stale registration is present too and differs in nothing else.

### Then Apple's validator, before any DAW

```
auval -a          ->  aumu Drck Dsyh  -  TiDE Synth:TiDE Rack
auval -v aumu Drck Dsyh   ->  AU VALIDATION SUCCEEDED   rc=0
```

**The first Apple-validated AU result this project has.** M2 and E9 both record that TIDE's AU evidence was *our own probe, never a DAW*; `auval` is neither ours nor a DAW, and it is stricter than the first and cheaper than the second.

### REAPER hosts it — and two traps cost a launch each

REAPER 7.45 scanned the registered extension into its AU cache as `TiDE Synth:TiDE Rack` (his own cache had **never** held a TIDE entry — 0 matches, checked before starting), instantiated it as **`AUi: TiDE Rack (TiDE Synth)`**, floated the editor, and rolled the transport **43 s at `playstate=1`** with the position advancing to 43.14 — so `process()` ran and nothing wedged.

- **A seeded portable config reloads the developer's last project**, whose missing plug-ins raise a modal, and the modal blocks `Scripts/__startup.lua` from ever running. The symptom is a startup script that writes **no log at all**, which reads as "my script is wrong" — I spent a launch there. `loadlastproj=0` plus an explicit empty `.rpp`.
- **The AU cache must be deleted from the PORTABLE copy** to force a rescan; seeded from his, it has no TIDE entry, so REAPER never looks.

Useful by-product: the blocking modal is where REAPER's own naming convention is printed — `AUi: <name> (<manufacturer>)`. Take the spelling from REAPER rather than guessing it.

### The screenshot settles what a symbol check could not

The floated editor **drew**, and its module browser lists `LFO`, `LFO2`, `Scope`, `SEQ3`, `SHASR`, `Quantizer`, `RandomValues` and the rest under a **`Rack-VCV Fundamental`** heading, with the five prefabs above them.

That is a picture of VCV Fundamental linked and **enumerated inside the hosted extension**. The 2026-08-29 run reached for `strings … "VCV: Scope"`, got 0, read it as "VCV did not link", and then confirmed its own error with a second bad reading — the ids are composed at runtime so the literal never appears. No symbol check could have answered this; one screenshot did.

### The wall a human does NOT remove, and it is the reason the rest is unmeasured

**An audio-unit extension runs out-of-process, so everything this project traces to `stderr` is invisible when the plug-in is hosted.** `RACK_ADAPTOR_TRACE`'s counters and TIDE's own `syncState`/`building rack from` lines are all `fprintf(stderr, …)`. Measured, not assumed: the strings are in the appex binary, the plug-in loads and runs under the host, and grepping REAPER's stderr for `TIDE:` or `RackProcessor` returns **nothing**.

So the linux box's whole instrument set is unavailable here, and E19's animation, int/bool/enum and pixel-diff clauses cannot be read on macOS AU3 however long anybody watches. **Filed as E73**, whose fix already exists one layer up: E65's `TIDE_PANEL_LOG_PATH` + `-DTIDE_PANEL_TRACE_LOG`, which routes a trace to a file and defaults into `TMPDIR` so it survives the sandbox.

### One measurement that belongs to V2, recorded in passing

REAPER sees **3** parameters on the instance: `Bypass`, `Wet`, `Delta` — all REAPER's own AU wrapper params. **None of TIDE's parameters are visible to the host**, so there is nothing for a DAW to automate today. That is V2's problem and this is a datum for it, not a new row.

**Learned:**

- **"Needs a human" is a claim with an expiry, and it expired the minute one showed up.** Two of E19's blockers were properties of an *unattended* run — a locked screen and a registration nobody may displace — not of the platform. The row had said so since 2026-08-29; what changed was availability, and a run should check that before re-inheriting a blocker.
- **Take the backup and the objection disappears with it.** The 2026-08-29 refusal was reasoned from irreversibility ("if the run died in between"). A `ditto` first converts the whole argument into a one-command undo — the blocker was recoverability, not permission.
- **`auval` before any DAW.** It is Apple's, stricter than our probes, needs no host config, and had never been run against this plug-in. A DAW failure after `auval` passes means something about the DAW; before it, you do not know what it means.
- **A no-output startup script is more often a modal than a bug.** REAPER wrote nothing at all, and the cause was a dialog about a *different* project's missing plug-ins. Screenshot before debugging the script.
- **When a symbol check is ambiguous and the thing is on screen, screenshot it.** Third time this project has been misled by `strings` on runtime-composed ids; the picture cost one command and is unarguable.
- **Out-of-process changes what an instrument IS, not just where it prints.** Every counter this fleet added for the linux box is a `stderr` write, and that design choice silently excludes the AUv3 target entirely. Worth knowing before adding the next one.

**Not verified:** E19's animation window, int/bool/enum toggle and pixel diff — blocked on E73 and on getting a PREPARED rack into a hosted AUv3, which is the same shape as E60's CLAP blocker; audio out of the hosted AU (the default rack with no MIDI is silence, so the test would have proved nothing); whether the same holds in Logic or Live, neither of which was opened.

**Machine state.** **One deliberate change to the developer's machine, and it is the point of the exercise:** `~/Applications/TIDE-Rack-AUv3.app` is now the current build (Release/arm64, `TIDE_VCV_FUNDAMENTAL=ON`, `RACK_ADAPTOR_TRACE=1`, 395/395 0 errors) and is the registered AUv3. **The 2026-08-26 app it replaced is backed up** in the session scratchpad; restoring it is `rm -rf` + `ditto` + one `open -g`. Everything else was isolated and verified afterwards: his `~/Library/Application Support/REAPER` has **0 files** modified in the last two hours across 2052, and his installed `VST3/TIDE-Rack.vst3` (Aug 28) and `CLAP/TIDE-Rack.clap` (Aug 22) are untouched — every build ran `SE_LOCAL_BUILD=OFF`. The portable REAPER, its config and all captures are in the scratchpad. No REAPER, appex or TIDE process left running by this run; a `e38_context_menu_probe.py` and a standalone TIDE belonging to Jeff's own live session were running throughout and were left alone. **A macOS permission dialog is on his screen** — *"Claude is requesting to bypass the system private window picker"*, raised by `screencapture`; I did not answer it, because system security settings are his, and screen capture worked without it.

**Next:** **E73 unblocks three of E19's clauses** and is one session. **E19's remaining mac clauses also want a prepared rack in a hosted AUv3** — worth solving once, since E60 needs the same thing for CLAP. **E72** wants a ruling, not a session. And the AUv3 is registered *now*, so any further AU3 measurement is cheap until somebody rebuilds over it.

**Branch/PR:** `tide/mac/E19-au3-registered` — the E19 row, E73, the macOS AUv3 section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry.

## 2026-08-31 — macos — E69: the CLAP save was fixed into an EMPTY save, and a 200-line bare host found it in one command (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** (no `claude` CLI on this box's PATH; the app's version, which A13 records as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** took **E69**, filed this morning from E68's leftovers, and found that its part (1) had **already been implemented and merged 20 minutes after the row was filed** — and that the implementation is a **regression that loses the entire patch on a CLAP save**. Built the instrument that shows it, measured a three-way A/B, and fixed it: [GMPI_Wrappers#37](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/37). **E69 → IN-REVIEW; E70 and E71 filed.**

### Why E69 and not "the queue is blocked"

The `mac` NEXT cell was written twice today: Jeff's correction about E65/#559, over yesterday's *"THE mac/any QUEUE IS BLOCKED FOR A SCHEDULED RUN"*. Both predate **E69**, which was filed at 10:27 NZST in the same commit that archived E68. Re-walking in file order: **S8** GATED, **E19**'s mac AU3 cell needs a human, **E7** turns on an unruled question, **E2** not takeable by its own row, **E60** is linux's on [#550](https://github.com/JeffMcClintock/TideSynth/pull/550), **E63** is `win`, **X1**/**X2** are `linux` — and **E69** is `any`, TODO, unblocked and unclaimed (no remote branch, no open PR). The blocked-queue cell was **19 hours old and already wrong**.

### The thing worth carrying: a row's work can be consumed before the row is read

[GMPI_Wrappers#36](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/36) — *"CLAP + AU3: saves serialise the controller's store"* — merged at **22:47Z**, and E69 was filed at **22:27Z**. Twenty minutes. Nothing in the row said so, and nothing could: the row is in this repo and the code is in another. **One command changes what the item IS:**

```bash
gh pr list --repo JeffMcClintock/GMPI_Wrappers --state merged --limit 5
```

And #36 shipped with a line this box has now met twice in three days: *"CLAP compiled and installed on Windows; **AU3 is mac-only — CI will say**."* CI says a thing compiles. **This defect compiles perfectly.**

### The instrument, and why it needed no DAW at all

Every route this fleet had for reading a plug-in's saved bytes went through REAPER: a GUI launch, a `__startup.lua`, a project file to decode. That route is shut for CLAP anyway — REAPER's CLAP state path does not restore (**E60**).

`tests/e69_clap_state_probe.c` is ~200 lines against `dlfcn` and the CLAP headers: `dlopen` → `clap_entry` → `create_plugin` → `plugin->init` → `get_extension(CLAP_EXT_STATE)` → `save`, `load`, `save`. It runs in about a second.

**It is a real test and not a stub because of one line in the wrapper:** `Processor_CLAP`'s constructor creates and `initialize()`s the plug-in's own `<Controller/>` unconditionally (`Processor_CLAP.cpp:88`, added for S43(ii)), asking the host for no extension to do it. So a bare host whose `get_extension` returns `NULL` for everything still drives the same controller/processor pair a DAW drives — which is exactly where save/restore bugs live.

### Measured — three builds, one commit apart, load an 18,893-byte four-cable rack and save

| CLAP built at | save after load | `<Cable>` | modules |
|---|---|---|---|
| `bb155b1` (#35 — save echoes the processor) | 18,933 bytes | **4** | 22 |
| `379d5c1` (#36 — save reads the controller) | **85 bytes** | **0** | — |
| [#37](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/37) | 18,661 bytes | **4** | 22 |

#36's 85 bytes are the *whole document*: `<Preset><Param id="1" val=""/></Preset>`.

The plug-in's own log is the mechanism, printed by the probe:

```
#36          syncState declined to publish the startup default (17959 bytes)
#37          syncState exporting 13926 byte document (host asked for state)
```

### The cause: a census, not a hypothesis

#36 was right about *which* store a save must read. What it could not see is that **CLAP's `stateLoad` writes only the PROCESSOR's store** (`plugin.setPresetUnsafe`), so after #36 load wrote one store and save read another, and the other was never written. One grep across the four wrappers settles it:

| wrapper | load feeds the controller's store? | save reads |
|---|---|---|
| VST3 | ✔ `Controller_VST3.cpp:514,526` | controller |
| Standalone | ✔ `StandaloneHost.cpp:311,318` | controller |
| AU3 | ✔ `AU3_Wrapper.mm:539` | controller |
| **CLAP** | **✘ — processor only** | controller (after #36) |

**No async hop explains the 85 bytes**, and that distinction is what made this safe to call a defect rather than a probe artifact: `setPresetXmlFromDaw` is never called on that path, so nothing can arrive later and no run loop would rescue it. TIDE's controller then *correctly* declines to publish — E59's refusal, doing its job — and the save writes nothing. #37 adds the standalone's two calls verbatim.

### The controls, because "4 cables" alone would not have been enough

- **Structural, not just counted:** the minted document holds the **same 22 modules and same 8 module types** as the input, with all four `<Cable>` endpoints (`fm`/`tm`/`fp`/`tp`/`c`) byte-identical. `scripts/dump_preset.py` (new) is the census.
- **Round-tripped twice:** feeding the minted document back in gives **byte-identical** output. That is what separates "the save re-serialises once" from "the save rewrites the patch a little more every time", and without it the 14,136 → 13,930 shrink reads as loss.
- **Fresh-instance save is 85 bytes on all three builds** — unchanged, and E59's refusal still fires there, which is correct.
- **The A/B never touched the developer's trees:** `-DFETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS=<private clone>` points each build at its own clone at its own commit. Two clones, three build dirs, nothing to restore.

### Consumers built

Fresh Release/arm64 Ninja over `main`, `SE_LOCAL_BUILD=OFF`, `TIDE_VCV_FUNDAMENTAL=OFF`: **TIDE 598/598, 0 errors** — VST3, AU, AUv3 appex, AU3 app, CLAP, standalone. **`SynthEditCL` 779/779, 0 errors** against #37's branch. The change is confined to `wrapper/CLAP/`, and SynthEdit consumes only the **VST3** wrapper from this repo (`se_gmpi/vst3`, the export template) — so the 779 is the rule discharged, not the risk.

**The 598 also discharges #36's own "CI will say" for AU3**: `AU3_Wrapper.mm`'s added `syncState()` compiles on this platform, and AU3's load already feeds the controller, so its one-line half of part (1) is correct by construction and does **not** share CLAP's asymmetry.

### Filed rather than fixed

- **E70** — E69's part (2). `MfcDocPresenter::AddPatchCable`/`RemovePatchCable` (`SynthEditLib/EditorLib/MfcDocPresenter.cpp:280,363`) set no dirty flag; confirmed by reading — a bare `setParameterValue` with no `SuspendDSP` guard, so `invalidateDsp()` is never reached. **`SynthEditLib/EditorLib/` is GATED and this is not a build break**, so the STEP 5 exception does not reach it. It is also shared with SynthEdit proper, which is the ruling the row asks for.
- **E71** — found in passing: AU3's `setFullState` never calls `notifyControllerOfPreset`, which both other wrappers do and which is the only call that tells a plug-in's own `<Controller/>` its state was restored. A code reading, deliberately not fixed by a run that cannot drive an AUv3 host.

**Learned:**

- **A row filed this morning may have had its code landed by an interactive session before any scheduled run reads it** — twenty minutes, here. The row still said TODO, and it had to: the code was in a sibling repo. One `gh pr list --state merged` on the repo the row's Scope names, before starting, and the item's whole shape changes.
- **"CI will say" about a platform you cannot test is an assignment with no addressee, and CI answers a different question.** This is the same lesson this box wrote two days ago, arriving as a *stronger* case: three merges deferred to CI and CI had already run; here CI ran, passed, and was never asked anything that could have caught an empty save.
- **A fix that corrects one half of a pair is a regression until you check the other half.** #36 moved the save to the right store without asking who writes it. The check is a four-row table — for each wrapper, which store does load write and which does save read — and it fits in one grep.
- **A "no host at all" harness is cheaper than isolating a host, and stricter.** Days of this document's macOS section are about isolating REAPER; the defect that mattered needed no host, ran in a second, and produced files you can diff. Reach for the plug-in's own C ABI before the DAW whenever the question is about bytes.
- **A size MATCH proves nothing and a size DIFFERENCE proves one thing.** E68's `14,136 → 14,494` discriminator works in one direction only: the mint is a **fixed point**, so a re-saved document matches exactly. My own probe printed "consistent with an echo" over a correct mint before I fixed its wording.
- **Distinguish "the store is stale" from "nothing ever writes the store" before calling a probe result a race.** A missing call cannot be won by waiting, and the grep that shows it is absent is faster than any run-loop pump would have been to write.
- **Reading the machine costs one command and I nearly skipped it.** `CGSSessionScreenIsLocked` is *absent* from `IOConsoleUsers` when the screen is unlocked — cheaper and less ambiguous than `screencapture`, whose all-black frame is the trap this document already names.

**Not verified:** audio through a CLAP host — E60 has REAPER's CLAP state route open, and this probe measures bytes deliberately, not sound; the AU3 save through a real AUv3 host — the registration wall E19's mac cell measured on 2026-08-29 is unchanged, and the developer was working at the machine; **whether a cable edit made LAST is captured** — E69's Accept says "cables as the last edit" and that needs a driven editor gesture, so what is proven here is the round trip, not the ordering; linux and Windows builds of #37.

**Machine state.** All six repos were clean and on their default branches at the start; `GMPI_Wrappers` was 2 commits behind and was fast-forwarded. TideSynth is on this run's branch and GMPI_Wrappers on its own until STEP 5 returns them. **An accidental `REAPER --help` launched the developer's REAPER for a few seconds early in the session** — recorded because it should not have happened, and because the check afterwards is the point: **nothing under `~/Library/Application Support/REAPER` was modified** (0 files newer than 30 minutes, 2052 files total as before, `reaper.ini` mtime still Sep 2025), and no REAPER was launched deliberately at any point. His installed plug-ins were never touched, measured rather than assumed: `~/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3` is sha256 `f3b09c3c…` with mtime **Aug 28 17:45:54** — the same two values the earlier run today recorded — and his installed `CLAP/TIDE-Rack.clap` is still dated **Aug 22 09:40:08**, nine days old, despite three CLAP builds this session. All three ran `SE_LOCAL_BUILD=OFF` in gitignored trees, and `SynthEditCL`'s tree is under the session scratchpad, outside every repo. A `tests/e51_dialog_divert_probe.py` and a TIDE standalone belonging to the developer's own live session were running throughout and were left alone. `decode_rpp.py` wrote `tests/hosts/v1-rack.rpp.block0.param1.xml` as a side effect; removed. Build trees `build-e69-after/`, `build-e69-before/`, `build-e69-fixed/` are gitignored; the two GMPI_Wrappers clones, the probe binary and all presets are in the scratchpad. No REAPER or TIDE process left running by this run.

**Next:** **#37 is the one that matters** — without it every CLAP save is empty, and it is one file. **E71 and E19's mac AU3 cell want the same single unlocked interactive session**, which is now the second row waiting on it. **E70 wants a ruling, not a session.** And the probe generalises: an AU3 and a VST3 equivalent would each be an afternoon, and would let a scheduled run answer "did the patch survive" on every format without a DAW.

**Branch/PR:** [GMPI_Wrappers#37](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/37) (the fix — the substance) + `tide/mac/E69-clap-au3-pull-state` (the probe, `scripts/dump_preset.py`, the E69/E70/E71 rows, the mac NEXT cell, the C-ABI section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry). The TideSynth side lands alone harmlessly; #37 is what stops the data loss.

## 2026-08-31 — windows — E68: the save was an echo of the wrong store, and Jeff's three questions redesigned it into a pull (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"use the computer to figure out why no sound"*, *"what is serviceDocumentSync about?"*, *"we have doubled-up… skip the complex document comparison yeah?"*, *"won't the parameter path handle the patch cables already?"*, *"we should be able to save the state without races?"*, *"why not save/load the Controller's state instead?"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** diagnosed Jeff's silent-patch report down to the byte, then built the design his questions converged on: **delete the document-shape machinery** (this repo) and **make the VST3 save pull fresh state from the controller** ([GMPI_Wrappers#35](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/35)). Proven by a scripted save round-trip. **E68 filed and IN-REVIEW.**

### The diagnosis: the .als itself was the witness

Driving Ableton from a shell with his MRU project: restore healthy (29,475 bytes imported, rack built, feedback flowing), MIDI reaching the rack, silence. Decoding TIDE's chunk out of the saved `.als`: **every module present, `HC_PATCH_CABLES` an empty patch-list** — a rack with no wiring. When Jeff re-cabled live, instant sound. The saved state, not the engine, was the defect.

Two mechanisms stacked, both already confessed in comments:

1. `serviceDocumentSync` pushed only on document-**shape** change, and `documentShape()` strips every `<patch-list>` — cables live inside HC49's patch-list, i.e. inside the one thing the comparison was built to ignore. The 2026-08-25 ruling's premise — *"the DSP rebuilds from the document it ALREADY HAS"* — is true of the running rack and **false of the store a save reads**.
2. The save serialised the **processor's** store, fresh only up to the last async IMessage the audio thread applied — and `Controller_VST3::getState`'s own "HONEST CAVEAT" documents the race, including the case no flush can fix: a host reading the component's state before ever calling the controller's `getState`.

### Jeff's questions did the design work, in order

- *"We have doubled-up — skip the comparison?"* → Almost: the flag (`dspDirty`) is already structural-only (SuspendDSP's sites; knobs never touch it), and the compare's one surviving job was suppressing exactly the push that would have saved his patch. **Deleted** — `documentShape()` + `lastPushedShape`, −37 lines. Verified in the standalone: 1 push at startup, **0 in idle** (the "holy fuck" economics intact), 1 per module insert, 0 per layout nudge.
- *"Won't the parameter path handle cables?"* → For the running DSP, yes — but the wrapper serialises only its four parameters, and cables live *inside* parameter 1's document; a `ppc` updates the live rack and touches nothing a save reads.
- *"We should be able to save without races?"* / *"Why not save the Controller's state — are we expecting the Processor to know the state when it simply does not?"* → Exactly, and the API's frame allows it: the component stream is canonical (hosts hand it back to *both* sides on load — restore was always controller-authoritative via `setComponentState`), so the fix is that **the processor stops answering from its own knowledge**: `getState` pulls from the paired controller, which mints the preset synchronously from its live store (`syncState()` + the shared `getPresetXml()` — the drift-unified `writePresetXml` meant no new serialiser). No hop, no ordering, nothing to race. Pairing is a one-time pointer over the connection-point **message** channel ("GmpiCtlPtr") — a message, not a cast of the peer, because hosts interpose connection proxies; cleared on `disconnect` so a dead controller cannot be pulled through (E66 one layer up, avoided).

### Proven

Scripted REAPER load→save→quit of `v1-rack-win.rpp` (a temporary `__startup.lua`, backed up and restored):

| | chunk in the project file |
|---|---|
| before | 14,136 bytes — the seeded store copy |
| after the save | **14,494 bytes — the freshly minted document**, `syncState exporting … (host asked for state)` in the log, **both cables present** in HC49 |

The size mismatch *is* the discriminator: an echo of the store would have written 14,136 back.

**Learned:**

- **The user's design questions were the diagnosis.** Four questions in sequence — each one eliminated a layer I would have patched — and the end state deletes code net. "Are we expecting the Processor to know the state when it simply does not?" is the whole fix in one sentence.
- **A save path that echoes a store is only as correct as the store's freshest writer** — and every writer between editor and store was asynchronous. Mint at the moment of asking, from the side that cannot be stale.
- **A comment's premise can be measured.** *"The DSP rebuilds from the document it already has"* was written about the running rack and silently extended to the saved one; one decoded `.als` separated the two claims.
- **Send pointers as messages, not casts, across a host-owned connection** — proxies forward the one and defeat the other.
- **The fixture's saved chunk size is a free discriminator** between "echoed the store" and "minted fresh" — no instrumentation needed, the number differs by construction.

**Not verified:** mac/linux builds; Ableton itself (REAPER proved the mechanism — Jeff's own `TIDE Test.als` re-cable→save→reload is the live Accept once merged); CLAP/AU3 still echo their processor store (named in the row); `AddPatchCable` still sets no dirty flag (harmless for saves now; named in the row for the live-restore path).

**Machine state.** The E68 stderr probe in `SynthEdit.cpp` reverted — diagnosis complete, the permanent E59-era lines suffice. `modules/TiDEPanel/TiDEPanelGui.cpp` briefly carried a foreign `TIDE_PANEL_TRACE_LOG 1` edit (Jeff's E65 diagnosis, marked REVERT) — left alone and since reverted by its owner. TideSynth on `tide/win/E68-push-on-dirty`, GMPI_Wrappers on `tide/win/E68-pull-state-at-save`, both with PRs. REAPER's `__startup.lua` restored. Jeff's Ableton closed by him; the installed VST3 carries both halves.

**Branch/PR:** `tide/win/E68-push-on-dirty` (this repo: the shape deletion, this row and entry) + [GMPI_Wrappers#35](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/35) (the pull). Either lands alone; together a patch can no longer be saved incompletely from either direction.
## 2026-08-31 — macos — the queue is blocked, so this run closed three "not verified: mac builds" lines and flipped the rows they sat on (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** (no `claude` CLI on this box's PATH; the app's `CFBundleShortVersionString`, which A13 records as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** re-walked the queue in file order and found nothing takeable, then did the thing that was actually owed instead: **E64, E66 and E67 → DONE and archived**, with the macOS half of each row's own *"not verified"* line measured rather than inherited. Branch `tide/mac/step4-e64-e66-e67-mac-verified`. TideSynth only; no product code changed, no sibling repo committed to.

### Why nothing was takeable, walked rather than inherited

The `mac` NEXT cell was 2026-08-28 and predates all four of E64–E67, so I re-walked. **S8** GATED — its live half is `SynthEditLib/UgDatabase.cpp`, and the row itself says changing the CMake gating *"needs a ruling this row does not ask for"*. **E19**'s mac AU3 cell needs a human, both reasons re-confirmed today rather than assumed: `ioreg -n Root -d1 -a` reports `CGSSessionScreenIsLocked true`, and the 2026-08-29 run measured five separate ways to register a current AUv3 beside the developer's, all failing. **E7** turns on Jeff's unruled *"where do the jacks live"*. **E2** is not takeable by its own row. **E60** is linux's. **E63**/**X1**/**X2** are other platforms'.

**E65 is this platform's and is already finished.** [#559](https://github.com/JeffMcClintock/TideSynth/pull/559) is green, unreviewed, nothing unresolved — STEP 1.5 says explicitly that is waiting on Jeff and not mine to touch. Its branch already flips the row to IN-REVIEW, **so I deliberately did NOT flip E65 on `main`**: doing so would hand #559 a conflict in the one file this fleet conflicts in most.

### The work that was owed, and nothing pointed at it

E64, E66 and E67 merged from the windows box on 2026-08-28 and **every one shipped saying "Not verified: mac/linux builds (no platform code; CI will say)"**. CI did not say. `gh run list --branch main` shows the last `build` on `main` at **9af5ea5a, 21:41Z** — E64's own merge — while `SynthEditLib#75` and `GMPI_Wrappers#34` landed at **23:57Z**. So no macOS build has ever compiled E67's change to `ViewBase.cpp`, and the row that says CI will answer named a run that had already finished.

### Measured, on a fresh tree with the trees themselves as the control

All six repos were clean (`git status --porcelain` empty) and byte-equal to `origin/main` before configuring, recorded per repo, because a `FETCHCONTENT_SOURCE_DIR_*` build reads a live working tree and another session's uncommitted work would land in the result. Fresh Release/arm64 Ninja tree, `SE_LOCAL_BUILD=OFF`, `TIDE_VCV_FUNDAMENTAL=OFF` — the shipped configuration, and OFF is also what stops POST_BUILD replacing the developer's installed plug-ins.

| | result |
|---|---|
| TIDE, every target | **598/598, 0 errors**, 97 s — standalone, VST3, AU, AUv3 appex, AU3 app, CLAP |
| `SynthEditCL` | **779/779, 0 errors** against the same merged `SynthEditLib` |
| E64's drain diagnostic in fresh artifacts | **1** each in VST3, CLAP, standalone |
| same string in the two INSTALLED pre-merge bundles | **0** and **0** |

`SynthEditCL` is the row that mattered: `ViewBase.cpp` ships in SynthEdit as well as TIDE, so *"TIDE builds"* is not evidence the commercial product is safe, and E67's own verification built SynthEditCL on Windows only.

**`nm` finds no `canvasCenter` and that proves nothing** — it is an inline member declared at `ViewBase.h:316`. The evidence that E67 is in this build is that both corrected call sites are in the compiled `ViewBase.cpp` (`:1237`, `:1388`) and the tree was fresh, so no stale object could have been reused. This is the 2026-08-29 `VCV: Scope` lesson arriving in a second shape, and I checked the declaration before reading the miss as a result this time.

### The audio half, and the control that makes the number mean anything

Through an isolated portable REAPER (copy `REAPER.app`, `touch reaper.ini`, seed the developer's `reaper.ini`):

| fixture | measured |
|---|---|
| `--control` (no plug-in at all) | **peak −6.0 / rms −9.0 dBFS** — the chain detects audio |
| `tests/hosts/v1-rack.rpp` | **peak −6.3 / rms −17.0 dBFS** — the 2026-08-18 macOS reference to the decimal |
| `tests/hosts/v1-rack-uncabled.rpp` | **−inf** — negative control, 0 patch cables |
| `tests/hosts/v3-midi-pitch.rpp` | −6.2 / −21.1 dBFS |
| `tests/hosts/v1-rack-midi.rpp` | −6.3 / −17.0 dBFS |

**The last row is NOT an E7 measurement and must not be read as one.** E7's Accept is about envelope TIMING, and peak/rms is precisely the number the 2026-08-28 run showed can make E7's failure look like a pass — identical figures to the no-MIDI fixture are the failing signature, not a passing one. Recorded as a by-product only.

### The provenance control, which is the reusable part

E19's windows leg cost a measurement to *"a local build does not shadow the installed plug-in and REAPER will silently load either one."* So this run did not merely narrow `vstpath_arm64` to a staging folder holding one bundle and delete `reaper-vstplugins*.ini` to force a rescan — **it then removed the bundle and re-rendered.** REAPER hung on an unresolvable-plug-in modal and wrote no TIDE cache entry; restoring the bundle reproduced −6.3 / −17.0 exactly. That round trip is what turns "the number came from my build" from a hope into a fact, and it cost one 300-second timeout.

It also settled a discrepancy I would otherwise have hand-waved: the rescanned cache keys the entry `TIDE_Rack.vst3` with an **underscore** while the staged bundle is `TIDE-Rack.vst3` with a **dash**. No underscore-named bundle exists anywhere on this box — REAPER sanitises `-` to `_` in an ini key.

**Learned:**

- **A "not verified on your platform" line is an assignment with no addressee, and "CI will say" can be false at the moment it is written.** All three rows deferred to CI; the last `build` on `main` predated two of the three merges, so the deferral pointed at a run that had already finished. One `gh run list` compared against the merge timestamps is the whole check.
- **Prove which binary answered, by taking it away.** Narrowing the scan path and clearing the cache are *arrangements*; removing the bundle and watching the render fail is a *measurement*, and only the second survives someone asking whether the installed copy got loaded.
- **A `strings` hit is worth what its miss is worth.** Two pre-merge bundles were already sitting on this box and gave the negative control for free — 0 occurrences against 1 in every fresh artifact.
- **Check how a symbol is declared before reading its absence from `nm`.** Inline members never appear; this is the runtime-composed-id lesson from two days ago wearing different clothes, and the same box wrote both.
- **"Which consumers did you build" is a different question from "does it compile".** The shared file compiled into TIDE at 598/598 and that says nothing about SynthEdit; `SynthEditCL` 779/779 is the claim worth having, and it is one target.
- **Do not flip a row on `main` when another platform's open PR already flips it.** E65 was one edit away from handing #559 a conflict in `BACKLOG.md`, which is exactly the file this fleet conflicts in.
- **A blocked queue is not the same as an idle run.** Three rows were sitting IN-REVIEW with every PR merged, and the mac verification each of them asked for was cheap once somebody read the sentence.

**Not verified:** E67's zoom BEHAVIOUR on mac — its drift figure needs a driven `--scroll --ctrl` gesture with the editor on screen, and the screen is locked; E66's 5/5 close/reopen reproduction — a GUI gesture whose assert half is `_DEBUG`-only and this is Release, and it was never reproduced on this box; E64's hosted processor-recreate lifecycle on mac — the row's reproduction was Ableton on Windows and nobody has re-run it here; **linux builds of all three, which are still unverified by anybody** and are one build on that box.

**Machine state.** All six repos were clean and on their default branches at the start, and were fast-forwarded to `origin/main` (TideSynth 8 commits, SynthEditLib 1, gmpi_ui 1, GMPI_Wrappers 2, GMPI 1; SynthEdit already current). TideSynth is on this run's branch until STEP 5 returns it; nothing else was committed to. **The developer's `~/Library/Application Support/REAPER` compares identical to its pre-run snapshot across 2052 files including sizes and mtimes**, and his installed `~/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3` is sha256 `f3b09c3c…`, mtime still Aug 28 17:45:54 — the isolation held, as the 2026-08-29 run measured it would. Build trees `build-macverify/` (gitignored) and a scratch `build-secl/` for SynthEditCL, the latter outside every repo so Jeff's Xcode `build/` tree was never touched. Portable REAPER, its seeded config and all renders are under the session scratchpad. No REAPER or TIDE process left running.

**Next:** **the same three rows want a linux build**, which is the one leg nobody has run and is a single command on that box. **E65 wants Jeff's merge** — [#559](https://github.com/JeffMcClintock/TideSynth/pull/559) is green and has been since 2026-08-29. **E19's mac AU3 cell still wants one unlocked interactive session**, unchanged: build `TIDE_Rack_AU3_assemble` with `-DTIDE_VCV_FUNDAMENTAL=ON -DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`, copy the app over `~/Applications`, launch it once.

**Branch/PR:** `tide/mac/step4-e64-e66-e67-mac-verified` — the E64/E66/E67 flips and their archive rows, the mac NEXT cell, the macOS plug-in-provenance recipe in [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry.

## 2026-08-29 — windows — E67: ctrl+wheel translated the document under the cursor — E42's defect, one function from where E42 fixed it (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"new bug: ctrl mouse wheel is meant to zoom in/out while keeping same point of document under the mouse"*, then *"the zoom works, but the document moves under the mouse"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** diagnosed by reading, fixed, made the gesture drivable, and measured the A/B. [SynthEditLib#75](https://github.com/JeffMcClintock/SynthEditLib/pull/75) (GATED — proposed for review, never merged by a run) + [GMPI_Wrappers#34](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/34) (`--scroll --ctrl`). **E67 → IN-REVIEW.**

### Jeff's clarification did the triage

The first report read as "broken"; I was half-way into the delivery path (E39's old note that `--scroll` "reports ok and moves nothing" pointed that way) when the clarification landed: **the zoom works, the document translates**. That eliminated delivery entirely — a zoom that works proves the event arrives with the ctrl flag intact — and reduced the search to the anchor arithmetic in one function.

### The defect, and where the answer was already written

`TopView::onMouseWheel` keeps the doc point under the cursor by recomputing the view centre — against `viewWidth * 0.5f`, the pane's half-SIZE. `calcViewTransform`, thirty lines below, anchors the actual transform on the pane's MIDPOINT, `(left+right)/2`, under a long E42 comment explaining **precisely this distinction**, measured to +240 DIP of browser strips. `(left+right)/2 − (right−left)/2 = left`, so every zoom step translated the view by `left/zoom` (and `top/zoom`) while the zoom factor itself was right. Origin-rooted panes hide it — midpoint equals half-size there — which is every other view in the repo, and why only TIDE showed it.

The fix is the same substitution E42 made, term for term against `calcViewTransform`.

### Making the gesture drivable was half the work, and it pays forever

The command channel could not express ctrl+wheel — `--scroll` built its flags from `kHoverFlags` only. [GMPI_Wrappers#34](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/34) adds `--ctrl`, the `--double`/`--right` precedent for the third time: each was one flag, and each turned a verification that "needs a human at the window" into a script. Also resolves E39's dangling note — the verb was never broken; the *gesture* was inexpressible.

### Measured — one variable, self-calibrating

`measure_zoom.py`: anchor one ctrl+wheel notch on the Out module's edge, find the module's two panel edges along the anchor row before and after, derive the ACTUAL zoom ratio from the edge spread (no assumption about snap), and compare each edge's landing spot with the doc-anchored prediction `anchor + (edge − anchor) × ratio`.

| build | result |
|---|---|
| origin/main | module **clean out of the viewport** after one notch; rails jumped rows |
| SynthEditLib#75 | left edge drift **+0.3 px**, right edge drift **+0.3 px**, ratio 1.302 |

The BEFORE build was produced by checking SynthEditLib back to `main` in the second build tree, so both binaries share the `--ctrl` flag and differ by exactly one commit's worth of view math.

**Consumers built:** TIDE standalone Debug and **SynthEditCL 90/90** — `ViewBase.cpp` is shared, so SynthEdit's own ctrl+wheel gets the same correction; origin-rooted panes are unchanged by construction.

**Learned:**

- **"The zoom works but it translates" is a complete triage in one sentence.** It rules out delivery, flags, and the zoom path, and leaves only the anchor arithmetic — the user's second sentence saved the session the delivery investigation the first sentence had started.
- **When a bug is fixed in one function, grep for the same expression in its callers.** E42 fixed midpoint-vs-half-size in `calcViewTransform` and documented it loudly; the identical expression sat in `onMouseWheel` computing the input to that very function. A fix that renames or wraps the corrected quantity (a `canvasCenter()` helper) would have fixed both sites at once.
- **An inexpressible gesture is a class of unverifiable rows.** Third time one flag on the command channel converted "needs a human" into a script — `--double` (E36), `--right` (E38), now `--ctrl`.
- **Self-calibrate the measurement against the artifact, not the spec.** Deriving the zoom ratio from the edge spread made the drift number independent of the snap formula — the measurement cannot be fooled by the very quantity under test.

**Not verified:** mac/linux builds (no platform code; CI on #75 will say); SynthEdit's interactive feel beyond compiling — the correction is mathematically the E42 fix, but nobody has wheel-zoomed the full editor against this branch.

**Machine state.** `SynthEditLib` on `tide/win/E67-zoom-anchor-drift` (PR #75), `GMPI_Wrappers` on `tide/win/E67-scroll-ctrl-flag` (PR #34), TideSynth on `tide/win/E67-filed` — all with open PRs. gmpi_ui still parked on its #16 branch awaiting Jeff's review click. Measurement artifacts under `C:\SE\_scratch\e64\zoom-*\`, script at `measure_zoom.py`.

**Branch/PR:** [SynthEditLib#75](https://github.com/JeffMcClintock/SynthEditLib/pull/75) + [GMPI_Wrappers#34](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/34) + `tide/win/E67-filed` (row and entry). The lib PR is the substance; #34 lands alone harmlessly.

## 2026-08-29 — windows — E66 fixed both halves: reload releases the visuals first, and State's death is now loud at the cause (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"drive the computer... should crash"*, then *"yes, fix E66"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** reproduced Jeff's settings-pane crash under a resident debugger, diagnosed it to a named invariant, and fixed both halves: [GMPI_Wrappers#33](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/33) (the root cause) + [gmpi_ui#16](https://github.com/JeffMcClintock/gmpi_ui/pull/16) (the tripwire). **E66 → IN-REVIEW.** Also filed **E65** earlier in the session ([#555](https://github.com/JeffMcClintock/TideSynth/pull/555), the panel's missing draft render — report only, deliberately undiagnosed).

### The reproduction, driven blind over the command channel

The pane was **already open at launch** — the audio device is held exclusively (Jeff's Ableton), and the pane auto-opens on that failure, which made the repro cheap. Clicking Close survived; **re-opening via `--menu Audio/MIDI Settings...` crashed 2/2**, AV reading `0xdddddddd` — MSVC's freed-heap fill. A resident `cdb -p <pid> -c "g; ~#k; qd"` caught the full stack on the second run:

```
State<bool>::release            <- walking the subscriber vector of a FREED State
~StateRef<bool> <- ~ToggleSwitch <- unique_ptr<View> dtor <- ViewParent::clear
Form::renderVisuals <- SettingsPane::render <- AppLayout <- paintLoop
```

### The invariant was already written down, twice

`ViewParent::clear()`'s own comment: *"release anything pointing to states before releasing states (else crash)"* — and `SettingsPane`'s destructor obeys it, with a comment saying why. **`reload()` is the same teardown happening mid-life and it skipped the first half:** `midiInputs_.clear()` destroys the MIDI tick-boxes' `State<bool>` objects while the old widget tree — only torn down at the *next* `renderVisuals` — still holds `StateRef`s into them. The other `State` members survive reload (they are assigned, not destroyed), which is why only the tick boxes could kill it.

### The fixes, and the E64 ruling applied one layer down

**Root cause** (GMPI_Wrappers#33): `reload()` begins with `clear()` and marks the form dirty. All three call sites (menu action, startup failure, device-death notification) verified to run outside the form's own widget dispatch, so clearing there cannot destroy a widget that is currently on the stack.

**Tripwire** (gmpi_ui#16): `~State()` now **asserts** no watcher remains — Debug stays loud **at the destruction site**, which is the cause, instead of a UAF two frames later in STL iterator machinery — and then **detaches** every survivor by nulling its back-pointer, so a Release build loses a notification instead of corrupting the heap. Drain-and-assert, the exact shape Jeff ruled for E64's queue. `StateRef` grants `State<T>` friendship for the detach.

### Verified

- Scripted repro: **5/5 close/reopen cycles alive** (was 2/2 crash on the first reopen), the pane fully rendered afterwards (screenshot — device combo, rate, buffer, tick boxes, status line all present), zero asserts on stderr.
- TIDE standalone + VST3 Debug rebuilt; the binary contains the new assert string (checked before running, installed-copy as negative control).
- **SynthEditCL 130/130** against the gmpi_ui branch — `observable.h` is shared with SynthEdit, per G3.
- The VST3 POST_BUILD install step failed once mid-session: **Ableton holds the installed bundle's DLL** — benign, left alone, and worth knowing: `SE_LOCAL_BUILD=ON` cannot replace the installed plugin while any host has it loaded.

**Learned:**

- **A settings pane that auto-opens on failure is a free reproduction rig.** The audio device being held exclusively looked like an obstacle and was the enabler: the pane was on screen at launch, every launch.
- **When a class's destructor documents a teardown order, grep for every other place the same members die.** `reload()` was the destructor's own sequence run mid-life, minus the half that made it safe — the invariant was stated in two places and enforced in neither.
- **Put the tripwire at the destruction site, not the use site.** The UAF surfaced in `_Adopt_unlocked` two frames after the cause; `~State()`'s assert fires at the exact line that breaks the contract, which is the difference between a session and a glance.
- **`0xdddddddd` in a Debug AV is a diagnosis in itself** — MSVC's freed-heap fill means use-after-free before any stack is read.
- **A resident `cdb -p <pid> -c "g; ~#k; qd"` costs nothing and catches what a post-mortem cannot** — the first attach without `g` lost the process; the second run's stack was the whole diagnosis.

**Not verified:** mac/linux builds of both changes (no platform code; CI will say); whether any *other* `gmpi_forms` consumer relies on destroying a watched `State` (the new assert will now say so loudly, which is the point).

**Machine state.** `gmpi_ui` on `tide/win/E66-state-outlives-ref-guard` (PR #16), `GMPI_Wrappers` on `tide/win/E66-settingspane-reload-order` (PR #33), TideSynth on `tide/win/E66-fixed` — all with open PRs, returned to defaults once merged. Jeff's Ableton untouched. Repro artifacts under `C:\SE\_scratch\e64\s3\` and `s4\`.

**Branch/PR:** [GMPI_Wrappers#33](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/33) + [gmpi_ui#16](https://github.com/JeffMcClintock/gmpi_ui/pull/16) (the fixes, either lands alone) + `tide/win/E66-fixed` (this row and entry).

## 2026-08-29 — windows — E64 root cause fixed Jeff's way: the wrapper's handle is registered, so the namespace defends itself (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"how about the obvious. register the root containers handle so everyone knows about it?"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** implemented Jeff's design for E64's root cause, and it turned out to be **TIDE-side only** — the `Id="1"` wrapper is minted by `TideApp::exportChunkXml` (`SynthEditSem/TideApp.cpp`), not by SynthEditLib, and SynthEdit's own exporter writes no such literal, so the collision never touched the commercial product. Branch `tide/win/E64-reserve-wrapper-handle`. GMPI#20 (the queue containment) stands unchanged as defence in depth.

### The design, and why it beats all four options I had listed

The wrapper's handle becomes `TideApp::kDspWrapperContainerHandle` (= 1, so every existing saved session and host chunk restores unchanged), and a `UniqueSnowflake dspWrapperReservation` member is **registered in the document's `uniqueIdDatabase`** at the top of `InitInstance` — before anything else in the document allocates. From there the namespace defends itself by mechanisms that already exist: the sequential parameter allocator's `while(find(key)) ++key` skips 1 like any other taken handle, and a latecomer claiming 1 (a hand-edited document) is renumbered by `Register`'s existing collision path. No reserved-base magic, no export-shape change, no new rule for anyone to remember — the reservation is a fact in the same map every allocator already consults.

Two loud checks, per Jeff's plastering-over ruling: `InitInstance` asserts if the reservation itself was beaten to the handle, and `importChunkXml` asserts if it did not survive a document rebuild (`DeleteContents` only unregisters objects in the document tree, so it does — verified, not assumed: the map has no bulk-clear on that path and `swap()` has no callers).

### Measured

- **Ordering verified in the artifact:** the standalone's pushed DSP doc now shows the wrapper still at `Id="1"` and the first host-control parameter at **`Handle="0"`, with nothing allocated 1** — the allocator skipped the reservation exactly as designed. No reservation-failure lines, no asserts.
- **The trigger path, end to end:** a hosted Debug render of `tests/hosts/v1-rack.rpp` — REAPER, `setActive` processor recreation, hc59's `ppc` and all — produced **no assert, no drain diagnostic**, a correct restore (both instances build 14,136), and **peak −6.3 dBFS / rms −17.0**, the reference figures. Before this fix the same path desynced the queue on the first parameter update.
- The E56 property survives: handles are still deterministic per load, just numbered around the reservation.

**Learned:**

- **"Register it so everyone knows" beats every clever alternative when a namespace already has an authority.** I had offered a reserved base, an export change, and a send-side filter; Jeff's version needs no new knowledge anywhere because the map IS the knowledge, and both existing allocators already consult it.
- **Find out whose literal it is before deciding whose fix it is.** Three sessions discussed this as a SynthEditLib/GMPI question; one grep found the `Id="1"` in TIDE's own ALLOWED file, which collapsed the gating question entirely.
- **A reservation is only a reservation if it is registered before the first allocation** — and the verification of that ordering is in the exported artifact (parameter handles 0, 2, …), not in the code review.
- **The heredoc backslash trap got me again**, one session after writing it into lessons: `\\n` collapsed and put a real newline inside a C string literal. The rule that sticks: escape-bearing code goes through a Write-tool file, never a heredoc — no exceptions for "just two lines".

**Not verified:** mac/linux builds (no platform code; CI will say); Ableton itself — REAPER exercises the same wrapper lifecycle, but the original reproduction machine is one insert-and-cable session away from closing this for good.

**Machine state.** TideSynth on `tide/win/E64-reserve-wrapper-handle` until this lands; `GMPI` parked on `tide/win/E64-que-selfheal` (open PR #20); all other repos clean on defaults. The installed Debug VST3 carries both fixes.

**Branch/PR:** `tide/win/E64-reserve-wrapper-handle` — TideApp.h/.cpp, this row and entry. Pairs with [GMPI#20](https://github.com/JeffMcClintock/GMPI/pull/20); either lands without the other, but together the collision is impossible AND any future misreader is contained loudly.

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
