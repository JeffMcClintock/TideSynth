# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

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

## 2026-08-24 — macos — A33: CI-filed issues on deleted branches now close themselves (interactive)

**Prompt:** we can do workflow modifications interactivly now

That ruling is what unblocked this. A33 has been TODO purely because
`.github/workflows/**` was Jeff's path; the work itself is small.

The defect, restated: a CI-filed platform issue names the branch that failed, and
the only thing that closes it is a GREEN RUN OF THAT BRANCH. Delete the branch at
merge — correct practice, and what every S3g pr did — and no run can ever happen
on it. The issue is then unclosable by the mechanism that filed it, and an open
`platform:*` issue is STEP 1 work outranking every backlog row.

A sweep now runs in `watchdog.yml`'s existing daily job. Three guards, all copied
from `build.yml`'s close-on-success step rather than invented: exact title shape,
`author.is_bot`, and the branch genuinely being absent.

**The dry run is what makes this entry worth reading.** I extracted the step's
bash out of the YAML with a real parser and ran it locally, with `gh issue close`
and `gh issue comment` stubbed and the live issue list. The first version failed
immediately: my YAML line-continuations had put literal backslashes INSIDE the
`jq` program. YAML parsed fine — three steps, valid document — because YAML does
not check the shell inside a block scalar. It would have shipped green and failed
on the next scheduled run, in a job nobody watches.

Four cases exercised, three of them controls:

| case | result |
|---|---|
| bot-filed, branch deleted | closes — #373, the one actually stuck |
| **hand-filed (`is_bot=false`), branch deleted** | **left alone** |
| bot-filed, branch alive (`main`) | left open, says a run can still close it |
| bot-filed, title has no ` — branch` | skipped, says so |

The second is the control this row explicitly asked for, and it is the one that
matters: the guard against auto-closing somebody's real report.

Parsing the branch off the title is safe for a reason worth writing down: a git
ref cannot contain a space, so `" — "` can never appear inside a branch name.

**#306, the example this row was filed about, is already closed by hand.** #373
is the live one, and the sweep takes it on the next daily run.

**Not verified:** the step running on GitHub's own infra. The logic is proven
locally against the real API, but its first scheduled run will be its first run
in place.

## 2026-08-24 — macos — V5: VCV's numbers, and the rack canvas cut from 7968 to 1008 (interactive)

**Prompt:** then take the next task

The one thing I had documented instead of delivering. Jeff asked me to research
VCV's rack size and resize TIDE to match; I filed the row with TIDE's own numbers
and said the VCV figures were unconfirmed. They are confirmed now, from source.

**VCV:** `RACK_GRID_WIDTH` 15, `RACK_GRID_HEIGHT` 380, default window 1024x720,
and `RACK_OFFSET` = grid x (2000,100). That last one matters — VCV's canvas is
effectively unbounded, so "how big is VCV's rack" is really "how much does the
window show": **68.3 HP by 1.89 rows**.

**The finding that made this well-defined: a TIDE DIP is very nearly a VCV
pixel.** E5 ruled row 384 against VCV's 380, and Eurorack's 3U = 128.5 mm with
1 HP = 5.08 mm puts TIDE at 15.2 DIP per HP against VCV's 15 — ratio 1.011. So
the two scales are the same and the comparison is arithmetic, not judgement.
TIDE's old 7968 canvas was **20.75 rows by 524.9 HP**: about 11x taller and 7.7x
wider than VCV shows.

Now 1008 = 60*16 + 48, which keeps the grid divisibility the old comment
recorded, and gives 66.4 HP by 2.62 rows.

**Both couplings the row flagged were real.** `setCenter` follows the constant on
its own. `seedRootMidiCv()`'s placements did not: 3600-3960 absolute, chosen
against the old centre of 3984, and on a 1008 canvas they would have been three
times outside it — the exact failure the code's own comment warns about ("passing
small numbers puts everything off the visible rack, which looks exactly like the
insert having failed"). They are derived from `cx = kRackViewDips / 2` now, so
the next resize cannot strand them.

Checked they land on-rack before trusting it: MIDI In (120,140), MIDI-CV
(280,140), prefab (480,464), with row 1 spanning y 252..636. Standalone launches,
root MIDI-CV seeds, prefabs seed, no error output, and S7 still holds — home
folder diff of 0 lines.

**Not verified: the rendered result.** The arithmetic is right and it runs, but
whether 2.62 rows LOOKS right is Jeff's call and a build cannot prove it. If it
wants a physical case instead of VCV's window, 1248 gives 82.2 HP by 3.25 rows,
which is about an 84 HP 3-row case.

**Not verified:** Windows and Linux.
## 2026-08-24 — macos — Close-out sweep: two real misses found (interactive)

**Prompt:** great. did we miss anything?

Checked rather than answered from memory, and there were two.

**Five rows were IN-REVIEW with every PR merged.** S7, S3g, P3, S18 and S35 —
thirteen PRs across three repos, all merged, none of the rows flipped. This is
the fourth time today this drift has needed fixing, and the pattern is always
the same: the PR merges, the row stays IN-REVIEW, and the next run cannot tell
finished work from work in flight.

**S8's row still named the wrong cause.** SynthEditLib#49 merged with the real
finding — the list is not "in a source list belonging to a separate target", it
sits inside `IF(SE2JUCE)` at `CMakeLists.txt:582` and is never evaluated, and 79
`.cpp` files are in there, not one module — but I never wrote it back to the row.
The evidence lived only in a merged PR body, which is exactly the place the next
run does not look. Prepended now, with the `ar -t` control.

**Two dirty worktrees, both safe, both checked before discarding.** `sl35` held
the `TEMP-DIAG` fprintf pair from the datatype hunt — throwaway instrumentation
that was never meant to land. `wt7` held earlier drafts of the S7 row and
journal, superseded by #376; I confirmed main carries both the row text and the
journal entry (line 63) before throwing them away rather than assuming the merge
had covered it.

**Still open and NOT missed, just not mine to close:**
[#373](https://github.com/JeffMcClintock/TideSynth/issues/373) needs a Linux
build — the cause is fixed in SynthEditLib#47 and verified on macOS, but that
issue's own rule is to close only after building on the platform, and this box
cannot. #378 (V4/V5/V6) is awaiting review.

**One thing genuinely unfinished and worth naming:** V5 asks for the rack view to
be resized to match VCV, and I never researched VCV's actual default window or
row count. The row records TIDE's own numbers — 7968 DIPs, 20.75 rows by 166
units against E5's ruled 384/48 — and says the VCV figures are unconfirmed
rather than guessing them. The resize cannot be done until someone gets them.

## 2026-08-24 — macos — Three rack-view rows filed, and a claim I had to withdraw (interactive)

**Prompt:** In the main 'rack' view, only 'rack modules' are relevant... / also document that the current rack view seems a bit large... / then tell me where the "MIDI CV" default rack modules comes from

Filed **V4** (filter the module browser by view), **V5** (the rack view is too
big), **V6** (`seedRootMidiCv()` does in code what one prefab could do — low
priority, since the prefab it inserts is real and works).

**V6 is the one that matters, because I got it wrong and Jeff had to say so
several times.** Asked where the default MIDI-CV comes from, I answered that the
visible rack module is a facade prefab and the real `SE MIDI to CV 2` is seeded
in C++ "because it can't be a prefab" — polyphony cannot escape a container
(E7), so it must sit at the root. Jeff's reply: *"You say 'it can't be a prefab'
then you tell me you build a container with a bunch of stuff in it... and insert
it. That's the same as a prefab isn't it."*

He is right, and reading the code instead of its comments shows why.
`seedRootMidiCv()` adds two modules, one connector, **inserts a prefab**, and
adds four more connectors. And `MfcDocPresenter::AddPrefab` returns
`std::vector<int32_t>`, derived from "a before/after diff of the container's
TOP-LEVEL modules", built on paste — and paste carries `CLine2` connections, as
the code's own filter proves. So a prefab paste splices multiple top-level
modules AND their connections into the target container.

Which means the polyphony rule constrains WHERE the paste happens, not whether
data can express it. Paste at the root, and the root-level modules land at the
root. My argument was about module placement and I used it to answer a question
about data-versus-code — two different things.

**The concrete reason to fix it, rather than just concede the point:**
`TideApp.cpp:693-697` hard-codes `facadePin = 7 + jack index` as an explicit
contract with `build-prefabs.py`, and states that if the jack list changes this
must change with it. A stored prefab has no such coupling, because the
connections are data rather than pin arithmetic.

**The lesson I keep relearning today:** I paraphrased the comments above
`seedRootMidiCv()` rather than reading its body. The comments are excellent and
they explain the polyphony constraint at length — which is exactly why
paraphrasing them produced a confident wrong answer. The body is 25 lines and
settles it.

**V5 has real numbers rather than "seems large", and I had to correct them
once.** `viewDimensions = 7968` DIPs square. My first pass divided by
`rowHeight 380`, taken from a comment at `TideApp.cpp:793` — but commit
`58a246b` ("E5: the rack grid ruled") sets row **384** and standard width **48**,
so the comment was stale and the row now uses the ruled figures: **20.75 rows
tall and 166 units wide**. A Eurorack case is typically 84 HP and three rows,
which is the whole of why it looks big.

I have NOT confirmed VCV's own default window and row count. The row says so
rather than banking a figure from memory, and flags two couplings —
`setCenter` at `:197`, and `seedRootMidiCv()`'s absolute 3600-3960 placements —
that would put the seeded MIDI-CV off-rack if the view shrinks without them
moving.

**V4 needs a ruling, not code:** the discriminator exists (`isRackLevel`,
`TideApp.cpp:147`) and there is exactly one filter point
(`ExportModules`, called from `ModuleBrowser.cpp:57` and `:100`). What does not
exist is any marker for "rack-relevant", so the row lists candidates —
prefab-vs-module, the existing `category=` attribute the browser already reads
at `:758`, or a new flag — and leaves the choice to Jeff.

## 2026-08-24 — macos — One RackModules folder, and the comment that sent Jeff's prefabs nowhere (interactive)

**Prompt:** ok, simple misunderstanding. I was putting my prefabs in "Tidesynth/TideModules" yours was in "Tidesynth/TideModules/prefabs". / lets just keep them as .synthedit for now / rename mine with _jef appened for now / OK, we're not regenerating them any more, they need hand-tweaking of the layout. don't want that overwritten

**It was not a misunderstanding on Jeff's side. The comment told him to do what
he did.** `SynthEditSem/CMakeLists.txt` said *"copy_directory_if_different so
adding a prefab to TideModules/ needs no CMake edit"* while the command beneath
it copied `TideModules/prefabs`. So his `AR`, `Output` and `Sine` — committed in
`f814c0b` — were never staged into the bundle, silently. Zero `.seprefab` and
zero of his `.synthedit` files were in it.

Now one flat folder, `RackModules/`, per Jeff's ruling: they are modules in the
EURORACK sense, not the SynthEdit DLL sense, and they are all prefabs so no
`prefabs/` subfolder earns its keep. Nine `.synthedit` files, verified in the
built bundle, no strays.

**Two collisions I stopped on rather than resolved.** Flattening put two
different `Output.synthedit` in one folder (generated 12277b vs Jeff's 13542b),
and all three of his existed as BOTH `.seprefab` and `.synthedit`. Either guess
would have silently dropped one of his rack modules. Jeff's calls: keep
`.synthedit`, and suffix his with `_jef` so both `Output`s survive for
comparison. The substantive difference, for whoever compares them: the generated
one uses `type="SE Patch Point in"`, his uses `type="TiDE Patch Point In"`.

**The last instruction changed what `build-prefabs.py` IS.** *"we're not
regenerating them any more, they need hand-tweaking of the layout. don't want
that overwritten."* Its docstring still called those files "BUILD OUTPUT, not
hand-written" — which is exactly the belief that would destroy a day of layout
work. Layout is the half the script recomputes from its own tables, so it cannot
preserve a tweak.

So it is now unable to clobber, and that is tested three ways rather than
asserted: `--outdir` has no default so a bare run fails in argparse; pointing it
at `RackModules` exits with a refusal; a throwaway `--outdir` gets past the guard
and on to its normal SynthEditCL check. The guard sits immediately after
`parse_args()` — my first attempt put it lower, where the SynthEditCL check
fired first and the guard never ran at all. It is kept, not deleted, because it
still documents how the graphs were built, which is written down nowhere else.

The `_jef` suffix has a useful side effect: the script writes six fixed
filenames, none of them suffixed, so it structurally cannot touch Jeff's files
even if the guard were removed.

**Left alone deliberately:** `TideModules` in DONE backlog rows and in
`docs/n1-tide-rack-rename.md`'s table. Those are records of what was true then,
and the backlog's Item column is prepend-only.

**Not verified:** Windows and Linux. `tests/s21_bundle_resources_probe.py` would
have been the natural check and cannot run here — it looks for
`Contents/x86_64-linux/TIDE-Rack.so`, so it is Linux-only by construction. The
macOS check was a direct listing of the bundle's `Resources/Prefabs`.

## 2026-08-24 — macos — The S7 skins ruling, and the gap Jeff closed (interactive)

**Prompt:** oh, if it works with no skin at all, that good. / I've been running the rendered UI. No obvious issue yet.

Follow-up to the three S7 PRs, all merged (SynthEditLib#48, SynthEdit#77, #375).
`main` builds and links on macOS at 318/318 with all three in.

**The gap I flagged was closed by Jeff, not by me.** I shipped S7 saying plainly
that I had not looked at TIDE's rendered UI — I could establish that nothing is
written to the user's home and that the code path is safe, but not that it still
looks right. Jeff then ran the UI and reported "no obvious issue yet". Recording
the division deliberately: naming the unverified part is what got it verified,
and it would have been easy to let "the standalone launched clean" stand in for
a look at the pixels.

**Two rulings arrived after #48 merged, and they change how the empty folder
reads.** *"TIDE does not support user-defined skins. The default Skin files
should load from TIDE's own private resource folder only."* And: *"skins have
much less relevance in tide, which has only one default skin. eventually it
might ship with no skin files, just hardcoded defaults."*

So TIDE's empty bundle skins folder is the intended destination, not a missing
asset. I had been one step from treating it as a gap and proposing to vendor
SE16's `default3` into TideSynth — 524K of assets duplicated across repos, for a
product heading toward no skin files at all. The question was worth more than
the work would have been. Recorded at the query itself in SynthEditLib#50, so
the next reader is not tempted to "fix" the empty folder.

**Mechanically it holds with zero files on disk**, which matters because #48
changed TIDE from an empty folder that EXISTED to a path that does not:
`ScanFiles()` uses the `error_code` overload of `directory_iterator`, which
yields nothing for a missing path rather than throwing, and always pushes the
`"global"` `SkinInfo` first, so `getSkin()` still returns a usable skin. I said
earlier it would return null — it does not.

**A near-miss on my own process.** I amended the merged S7 commit to add this
comment and tried to force-push. `--force-with-lease` refused: #48 had merged
while I was editing. Without the lease I would have rewritten a merged branch.
The follow-ups are ordinary PRs off current main instead.

**Not verified:** Windows and Linux.

## 2026-08-24 — macos — S7: TIDE was resetting SynthEdit's skin version (interactive)

**Prompt:** next

Took S7 — TIDE writing outside its container on launch, verified at runtime by
the linux box on 08-22. Fixed in SynthEditLib#48 + SynthEdit#77 + this pr, which
must land together.

**The defect is worse than the row records, and the difference is the machine.**
Linux measured folder CREATION inside an `LD_PRELOAD` shim over `getpwuid`, in a
home with no existing `.resource_version`. On a real machine that also runs
SynthEdit, one TIDE launch rewrote `~/SynthEdit Projects/.resource_version` from
SynthEdit's **192** to **0**. TIDE never defines `SE_APP_BUILD_NUMBER`, so
`se_version.h`'s fallback of 0 wins, `versionChanged` is true against any real
build, and the stamp is rewritten. SynthEdit then re-copies every skin on its
next launch. Two apps, one version file.

That only shows up on a machine with both installed — which is why the isolated
measurement, correct as far as it went, missed it.

**The row's own suggested shortcut does not work.** It offers "the TIDE-side
part may be enough on its own": point SkinMgr at the bundle from TIDE. It
cannot, because `SkinMgr`'s CONSTRUCTOR does the writing, on the first
`Instance()` call — anything TIDE does afterwards is too late. So EditorLib had
to change, which is the GATED part; interactive satisfies that.

Fixed with a per-app `AppUsesUserSkinsFolder()`, third instance of the pattern
after `GetLicenseState()` (C11) and `AppHasModuleEditorDialogs()` (S3g).

**Measured both directions against Jeff's REAL home folder**, because the row
warns `$HOME` does not protect it — `getUserDocumentFolder()` uses
`getpwuid(getuid())->pw_dir` and ignores the environment. I snapshotted 335
entries and backed up `.resource_version` first, ran the unfixed binary, saw
`192 -> 0`, restored immediately, then ran the fixed one:

| | `.resource_version` | folder |
|---|---|---|
| unfixed | 192 -> 0, rewritten | modified |
| fixed | 192, untouched | diff of 0 lines |

**A process note on myself.** Four times today a check passed because the thing
under test never ran — most recently here, where I "launched" a standalone whose
binary did not exist and read the resulting no-op as a clean pass. The tell was
an `ls` guard whose output I did not look at. Every launch/build check in this
entry now asserts the artifact exists first, and that is what caught it.

**Not verified:** Windows, Linux, and TIDE's rendered UI. The standalone came up
with no new error output versus the pre-fix run, and TIDE ships no
`Resources/skins`, so its skin folder was empty before and is a different empty
path now — but that is an argument, not a look at the pixels.

## 2026-08-24 — macos — main did not compile, and a docs-only PR is what proved it (interactive)

**Prompt:** merged. go!

`main` stopped building on macOS and Linux. Fixed in
[SynthEditLib#47](https://github.com/JeffMcClintock/SynthEditLib/pull/47).

    CContainer.h:18:7: error: expected identifier
    enum{ ID_EDIT_COPY = 0xe122, ID_EDIT_PASTE = 0xe125, ... }

P3 (`a4d536a`) added `StandardCommandIds.h`, which `#define`s those four ids on
every platform, and included it from `CContainer.cpp` BEFORE `CContainer.h`. The
macros then expand inside `CContainer.h`'s enum of the same names. Windows never
saw it: that enum is guarded `#ifndef _WIN32` because MFC supplies the ids there.
**The platform that could not see the clash is the one that introduced it.**
Same four values in both places, so the enum was redundant — replaced with the
include.

**How it was found is the part worth keeping.** The macOS job went red on
TideSynth#368, a docs-only PR whose branch was provably `main` plus two markdown
files. A code failure on a change containing no code is impossible, and that
impossibility is what said "the fault is in main, not in your PR". Taking the
red at face value would have meant debugging my own change for nothing and then
merging onto a broken main anyway.

Two gaps this exposes, both for Jeff rather than this box: #44 merged green while
breaking every non-Windows consumer, and #368 merged onto a red main.

**A33 now has a rate, not an anecdote.** Three issues in one day — #364, #372,
#373 — each naming a branch that `git ls-remote` returns 0 refs for, so the
mechanism that files them can never close them. #306 has been open since
2026-08-22 for the same reason. All were handled by hand today. The row was
filed as tidiness about a single stale issue; it is actually producing one open
`platform:*` issue — STEP 1 work, outranking every backlog row — per merged
branch that happened to be red. Deleting branches at merge, which is the right
practice and what the S3g PRs did, makes it more frequent rather than less.
`.github/workflows/**` is Jeff's path, so recording the evidence is all this box
can do.

Closed #372 on a macOS build of `main` at `1efd676`: TIDE links 318/318,
dsp_tests / SynthEditCL / EditorScreenshot build, suite 62/2. Left #373 open with
the diagnosis — the cause is platform-independent, but I have no Linux machine
and the issue's own instruction is to close only after building on the platform.

## 2026-08-24 — macos — Deleted the stranded S27 branch, after proving it held nothing (interactive)

**Prompt:** stranded branch

`origin/tide/mac/S27-render-ci` was the branch S44 was filed about. S44 recovered
the WORK into main; the branch itself stayed behind, and CI kept building it —
it was one of the two branches that failed to link after S3g landed, alongside
the linux branch that produced #364.

Deleting a branch is easy to get wrong, so I proved it superseded before
touching it rather than reasoning from "S44 landed, so it must be covered":

- of the 24 files its 3 commits touched, **20 are byte-identical to main**
- **0 files exist only on the branch**
- the 4 that differ do so because main is 36 commits ahead, not because the
  branch has newer content
- the only branch-unique content was **9 lines of `build.yml`** passing the
  reference set as a CI matrix (`refs: macos` / `refs: windows-linux`)

Those 9 lines are superseded by a better design, not merely duplicated: S44 made
the test pick its own set at compile time (`kReferenceSet`), so the workflow does
not need to pass one. The valuable part of them was the measured comment — why
two sets and not three — and I checked that it survives in main in three places
(`modules/common/README.md`, `RenderRegression.cpp`, `BACKLOG.md`) before
deleting. Tip archived locally at `refs/archive/S27-render-ci` (`218ff24`).

**The trap in one line, because it has now bitten twice:** #331 merged FROM this
branch, and then two more commits were pushed onto it. A merged PR does not stop
a branch accepting more work, and nothing then opens a PR for it. Deleting the
branch at merge is what prevents the next one — the S3g PRs all used
`--delete-branch`.

No TideSynth branch is now missing the S3g definition, so the link failure class
that produced #364 is closed on this repo.

## 2026-08-24 — macos — S18: the public repo now says which sdk it wants (interactive)

**Prompt:** sync repos. next task

No open platform issues, so a backlog row. Took S18 — self-contained, and its
Accept offers two options of which one avoids a licensing decision entirely.

`modules/SoundPipe` is the only place in the public repo needing a third-party
sdk that lives outside it. `external_sdk_folder` is set ONLY by SE16
(`SynthEdit/CMakeLists.txt:288`), pointed at SE16's own `SDKs/`, and
`SynthEditLib/modules` is added by SE16's root and never by SynthEditLib's own.

**Reproduced the stranger's experience rather than describing it.** With the
variable unset the configure dies with

    Cannot find source file: /Soundpipe/modules/base.c
    No SOURCES given to target: Soundpipe

A path that exists nowhere, naming no sdk and offering no remedy. That is the
actual defect — not that the dependency exists, but that hitting it teaches you
nothing.

A detour worth recording: my first attempt to reproduce passed `-Dexternal_sdk_folder=`
on the command line and configured cleanly, which looked like the row was stale.
It was not — line 288 is a plain `set()`, not `set(... CACHE ...)`, so it
overwrites anything passed with `-D`. Had I stopped there I would have closed a
live row as fixed. Emptying the `set()` in a throwaway worktree reproduced it
immediately.

Guarded and explained. Measured both directions, because the second is the
regression check that matters:

| `external_sdk_folder` | before | after |
|---|---|---|
| unset | configure fails | rc=0, prints which sdk and where to point it |
| set by SE16 | `Soundpipe.sem` builds | builds, skip message absent |

**Deliberately not done:** vendoring or fetching Soundpipe publicly, the other
option S18 offers. Soundpipe is third-party, so that is a licensing question
before it is a build question, and the row keeps that half.

**Not verified:** Windows and Linux. The guard is an `EXISTS` test on a header
path, so nothing platform-specific, but neither was built.

## 2026-08-24 — macos — S35 re-landed, this time against a bar it can fail (interactive)

**Prompt:** go

The user-domain scan is back ([SynthEditLib#45](https://github.com/JeffMcClintock/SynthEditLib/pull/45)),
after the two things that actually broke it were fixed: #40 stopped a `.gmpi`
being read for any xml in its Resources, #41 stopped the factory modules
installing copies of themselves into the user domain.

**The part worth keeping is the third condition.** The revert recorded two
requirements, and I had only satisfied one by fixing the crash. The other was
*"it must not defeat `-factorysemsfolder`, which exists to make a test run
deterministic."* Re-landing the scan unchanged would have left a test run
picking up whatever the developer has installed — which is precisely how the
original took `dsp_tests` from 2 failures to 43.

So the scan is skipped when the sem folder was overridden. That flag was already
there and already means this: `SemCacheName()` namespaces the module cache on it
so a `-factorysemsfolder` sweep cannot clobber the installed app's cache
(`ModuleFactory_Editor.cpp:188`). Same signal, same reason, no new concept.

Four measurements, because any one alone would have been the original's mistake:

| | result |
|---|---|
| `-factorysemsfolder` given | no user scan, 0 duplicates, exit 0 |
| no override | user domain scanned, TIDE-Rack visible, exit 0 |
| `dsp_tests` | 62 passed, 2 failed — the baseline |
| duplicates, normal run | 2, both `SE SynthEdit` from the TIDE bundles |

**A near-miss worth recording.** The first run of the acceptance test reported
"user-domain scan lines: 0, FOUND TWICE: 0" and looked like a pass. It was
vacuous — the build had failed and the binary did not exist, so grep found
nothing in an empty log. Exit 127 was the only sign. The cause was unrelated (my
SE16 worktree was pinned before SynthEdit#75, so `AppHasModuleEditorDialogs()`
was undefined), but the shape is the same one that bit me on the
`IS_OFFICIAL_MODULE` flags earlier today: **a check that counts absences passes
when the thing under test never ran.** Assert the binary exists before believing
a zero.

**Deliberately not answered:** precedence when a user copy shadows a factory
module. Pre-existing, applies to the system domain too, and post-#40 a duplicate
is a warning rather than a crash. It deserves its own ruling, not a rider.

**Not verified:** Windows and Linux. `getUserPluginsFolder()` returns empty on
both so neither gains a scan, but neither was built.


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
