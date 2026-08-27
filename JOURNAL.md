# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-28 — macos — E52: a shipping build option that did not compile, and the control that proves the fix is not a deletion (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** (there is no `claude` CLI on this box's PATH, so this is the desktop app's `CFBundleShortVersionString`, the version A13 recorded as the discoverable one on a mac) · as **tide-rack-bot** (both paths)

**Did:** took **E52**. Both Accept clauses met.
[GMPI_Wrappers#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28) off branch
`tide/mac/E52-geometry-seam-outside-channel` carries the code, PR-gated so proposed and
not merged; TideSynth's `tide/mac/E52-standalone-channel-off-build` carries the row, the
E51 annotation, E50's archive and this entry. Row is **IN-REVIEW**.

### Why E52 and not one of the six TODO rows above it

The next run will walk the same list, so here is the walk rather than the conclusion. The
`mac` NEXT cell is dated **2026-08-25** and says nothing is takeable; it predates E32–E54
and I read it as history. In file order: **S1b** and **S8** are wholly GATED
(`EditorLib/CMakeLists.txt`, `SynthEditLib/CMakeLists.txt`) and S8 is additionally MOOT
since the Oscillator HD ruling; **E38** already carries `NEEDS-SPEC`; **E19**'s mac cell
wants AU3 in a real host, which needs a human at a DAW; **E7** is an engine fact Jeff has
ruled is not a blocker; **E2**'s own row says it is not takeable; **E25** is done and
waiting on [#513](https://github.com/JeffMcClintock/TideSynth/pull/513); **E39**'s fix is
GATED and its own row says re-write the Accept before taking it; **E48**'s remaining step
is a PLAN constraint 7 product decision, and it is the `win` cell's pick.

**E51 is the one I annotated rather than skipped silently**, because it is the first row
where the reason is the row's own text: its Accept requires *"grep finds no direct
`MessageBox`/`NSAlert`"* and the row then records two such calls that are **correct as they
are** (`MainWin32.cpp:218`, `mac/MainMac.mm`'s `showFatalAlert`), concluding *"re-state it
before using it"*. Of the two things left, the `--dialogs` verb is design work the row
itself defers, and identifying the one answer-consuming call site is a grep that does not
close the row. `NEEDS-SPEC` added, naming what is missing, per the E38 precedent.

### The break, reproduced before anything was changed

`cmake -DGMPI_STANDALONE_COMMAND_CHANNEL=OFF` on **unmodified `origin/main`**, building
TIDE's standalone:

```
StandaloneApp.cpp:240:19: error: no member named 'setWindowPosition' in 'gmpi::standalone::PlatformShell'
StandaloneApp.cpp:598:15: error: no member named 'logicalSize'       in 'gmpi::standalone::PlatformShell'
StandaloneApp.cpp:621:19: error: no member named 'windowPosition'    in 'gmpi::standalone::PlatformShell'
3 errors generated.
```

Three errors, exactly those three calls, nothing else — which is the row's own account.
**One correction: the row's `:201`/`:555`/`:578` are stale line numbers.** They moved when
E32's position half landed. The calls did not.

### The fix, and the one that would have passed while deleting the feature

`logicalSize`, `windowPosition` and `setWindowPosition` move **out** of
`#if GMPI_STANDALONE_COMMAND_CHANNEL`, with all three shells' overrides. `framePixels` and
`canvasSize` stay **in**: with the channel off nothing calls them and `mcp/` is not
compiled at all.

**Guarding the three CALL SITES was the alternative and E52 called it right.** It compiles,
and silently removes window restoration from every OFF build — the same class of mistake as
the break. The row is worth quoting to itself here: *"these are window GEOMETRY, and
reopening where the user left the window is a SHIPPING FEATURE, not a test affordance."*

**For an ON build this is a pure move.** Every line was already compiled, because the guard
it sat in was true. That is the whole reason it was safe to move all three shells at once
from a box that can build one of them.

### Verification, and the control is the part worth keeping

| build (Release, Ninja, macOS, all four siblings local) | result |
|---|---|
| **OFF**, all targets | **rc=0** — GMPI, VST3, CLAP, AU3, STANDALONE all link (was 3 errors) |
| **ON**, all targets | **rc=0**, 314/314 |
| the OFF binary really is OFF | `gmpi-standalone` occurs **0** times in it, **3** in the ON one |

Clause 2 — prepare `standalone.conf`, launch, `SIGTERM` (the normal teardown, which saves),
read the file back. E32's technique, because `--info` reports no position and there is no
move verb — and on an OFF build there is no channel at all, so it is the only technique:

| saved | OFF reads back | ON reads back |
|---|---|---|
| `x=300 y=200 900x700` | **300, 200, 900x700** | **300, 200, 900x700** |
| `x=740 y=415 1020x760` | **740, 386, 1020x760** | **740, 386, 1020x760** |
| *(empty config)* | **570, 153, 1100x626** | — |

**The empty-config row is the whole reason the other two mean anything.** A build with the
feature deleted also produces a `standalone.conf` full of plausible numbers — the ones the
window happened to open at. Knowing that an unconfigured launch lands at `570,153
1100x626` is what turns "it wrote a position" into "it read mine". Two saved positions
rather than one, for the same reason: one value can be a coincidence.

**`y=415 → 386` is not a defect and is not new.** AppKit's `constrainFrameRect:toScreen:`
pulling the window fully onto a 2240x1260-point display, `386 + 760 = 1146`. Both arms show
it identically, and it is the platform behaviour Jeff ruled on for E32 on 2026-08-27.

### What was NOT verified, stated rather than implied

**Windows and Linux were not compiled.** This box builds neither, which E52 predicted
(*"it touches all three shells and wants a box that can build each — or three runs"*). What
was done instead is a read: every member the moved bodies touch is declared outside the
guard in both shells — `ToplevelWindow window_` and `window_.frame()` in `MainWin32.cpp`,
`WaylandToplevel frame_` in `MainWayland.cpp`. `FrameCapture capture_` is the only guarded
member in either, and no moved body names it.

**The bound on that risk is structural, and it is why one box was enough.** Their ON builds
cannot change, because the moved text was already inside a TRUE guard; their OFF builds
cannot regress, because they do not compile today. So the worst case is that an OFF build
stays broken somewhere, which is the state before this change.

`SE16` does not compile `wrapper/Standalone/**` at all — checked, not assumed — so
SynthEditCL and SynthEdit are not consumers of this and did not need rebuilding.

### E50 archived, and the lint that caught my own note

STEP 4 bookkeeping: **E50** was IN-REVIEW with
[#508](https://github.com/JeffMcClintock/TideSynth/pull/508) merged. Flipped **DONE** and
moved to [BACKLOG-DONE.md](BACKLOG-DONE.md) — **on the Accept, not on the merge**, which is
yesterday's E49/E46/E47 lesson: its Accept is an either/or (*"either the Compare is
accounted for … or it stops being constructed"*) and the row's own first line records the
second limb as met by measurement.

**E45 and R6 were NOT flipped, deliberately.** E45's PRs both merged and its row says the
check *"exists and enforces nothing"* until one line lands in `lint.yml`, which the bot
token cannot write. R6's row states no Accept at all, so there is nothing to check it
against; both left alone and named here instead.

**Archiving E50 turned the `win` NEXT cell red**, and the failure is worth writing down.
`check-next-block.py` reads a take-phrase inside its own SENTENCE, and the cell preserves
its previous re-pointing verbatim — including a `TAKE` clause naming E50 from two days ago.
It was true when written and is now an instruction to take archived work. Defused in place
with a visible marker, because a correction appended afterwards is a different sentence and
the lint cannot see it. **Then my own explanatory note re-armed it**, by quoting the
defused phrase: the quote is itself a take-phrase in a fresh sentence with no negation in
it. Reworded to describe the phrase rather than reproduce it.

**Learned:**

- **An absent control makes a passing round-trip worthless.** A build with window
  restoration deleted still writes a full `standalone.conf`, because it saves whatever the
  window opened at. Only the empty-config launch — `570,153 1100x626` — separates "it
  restored mine" from "it reported its default", and it costs one extra launch.
- **A "pure move" is the strongest argument available for editing code you cannot build.**
  The Windows and Wayland edits are unverifiable from here, and they are still safe,
  because moving text out of a guard that was TRUE cannot change what that build compiles.
  Say the invariant, not "it should be fine".
- **Guarding the call site is the fix that passes and deletes the feature.** Three `#if`s
  would have turned this red build green in five minutes, with window restoration silently
  gone from every OFF build and nothing to notice it.
- **Quoting a lint's trigger re-arms it.** I defused a stale `TAKE` phrase and then
  reproduced it verbatim in the note explaining the defusal, which is a fresh armed
  sentence. Describe the pattern; do not paste it.
- **Check a lint by its exit code, not by the tail of its output.** I very nearly recorded
  `check-next-block.py` as green because I piped it through `tail` and read `$?` from the
  pipe. The archive already carries this lesson from 2026-08-27 (windows) and I repeated it
  the same day I read it.
- **A NEXT cell three days old is history, not a queue.** The `mac` cell said nothing was
  takeable and predated eighteen filed rows. Reading it as current would have ended the
  session with no work done.

**Next:** **E51**'s two remaining pieces need the spec named on its row before a run can
take it — that is one decision, not a task. **E52's Windows and Linux OFF builds are
unverified** and are one command each on the boxes that can run them: if either box has a
spare moment, `cmake -DGMPI_STANDALONE_COMMAND_CHANNEL=OFF` against
[#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28) is the whole check.
**E53** still wants a faulting address; **E48** still wants a product decision.

**Machine state.** All six repos on their default branches at the start and clean;
`TideSynth` and `GMPI_Wrappers` now on the branches above, returned to their defaults at
the end. Two scratch build trees (`e52-off`, `e52-on`) in the session scratchpad, outside
every repo. `~/Library/Application Support/TIDE Rack/` was copied out before the first
launch and **restored byte-for-byte, md5-verified** — the three files are Jeff's, not this
run's. No TIDE process left running.

**Branch/PR:** `tide/mac/E52-geometry-seam-outside-channel` in GMPI_Wrappers
([#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28), the code) and
`tide/mac/E52-standalone-channel-off-build` in TideSynth
([#515](https://github.com/JeffMcClintock/TideSynth/pull/515)) (E52's row, E51's `NEEDS-SPEC`,
E50's archive, the `win` NEXT cell's defused phrase, the `mac` NEXT cell, and this entry).
**Merging TideSynth's side alone changes no behaviour**; merging GMPI_Wrappers' alone
leaves the backlog saying the work is open.

## 2026-08-27 — windows — E48: a shipped prefab uses a module TIDE does not ship, and that one fact explains both dialogs and the 3,577 bytes (interactive, Jeff directing)

**Prompt:** *"take next windows task"* · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** · as **tide-rack-bot**

**Did:** took **E48** (the `win` NEXT pick). Diagnosed it end to end with a stack.
Landed `scripts/check-prefab-modules.py`, which fails on the defect. **Row stays
TODO**: the remaining step is a product decision, not a task. Annotated **E51**
with a measured instance of its own gap.

### The cause, in one sentence

`RackModules/AR_jef.synthedit` — a **shipped prefab** — contains
`<module type="SynthEdit ADSR">`, and TIDE neither compiles that module nor
stages its XML. Measured: the string occurs **0 times** in `TIDE-Rack.exe` in
either UTF-8 or UTF-16, while all 32 other module types across the five shipped
prefabs are present.

### The chain, each step measured

1. Inserting the prefab **appears to work** and the rack looks right.
2. The saved document therefore carries `type="SynthEdit ADSR"`.
3. On reload `CContainer::ImportChildren` cannot resolve it and raises
   `SeMessageBoxAsync("Module not found in factory: …", L"", MB_OK)`
   (`CContainer.cpp:1089`).
4. That is a **blocking `MessageBoxW`** running a nested `SoftModalMessageBox`
   pump on the main thread, so the restore never finishes — no further stderr,
   no `building rack from`, no command channel, **~0.08 s of CPU**.
5. The module is then `continue`d past, so its connectors are dropped and the
   *"Connectors lost while loading"* dialog this row was filed on follows at
   `:1143`.

**One cause, both dialogs, and the 3,577 bytes.**

### The stack, because reading the source would not have settled it

```
wWinMain → runStandaloneApp → SessionState::restore → StandaloneHost::restoreState
 → notifyControllerOfPreset → SynthEditController::setParameter
  → TideApp::importChunkXml → CSynthEditDocBase::ImportModules
   → CContainer::Import → CContainer::ImportChildren
    → ApplicationBase::SeMessageBoxAsync → USER32!MessageBoxW
     → SoftModalMessageBox → IsDialogMessageA → PeekMessageW
```

**Attach, do not launch.** An earlier attempt launched the app *under* `cdb` and
hung for seven minutes with nothing to show, because the modal pumps inside the
debuggee before the debugger has anything to report. Attaching to an
already-blocked process answers in seconds.

### Two corrections to the row, and the second cost real time

- **The blocking dialog is not the one the row names.** *"Connectors lost while
  loading"* is the **sync** `SeMessageBox` at `:1143`; the blocker is the
  **async** one at `:1089`, which fires first.
- **`MainWindowTitle` is not a usable handle on it.** E48 and E51 both lean on
  the title as *"the only evidence available from outside"*. `:1089` passes
  **`L""`** as the caption, so the title reads **empty** for the entire block and
  `EnumWindows` lists no visible window — while a dialog is demonstrably on
  screen, because Jeff saw it twice and told me.

That second one is the expensive part of this session, and it was my error
rather than the row's. I built a detector keyed to the window title, **told Jeff
it made driving the app safe, and it could not see this dialog at all** — so the
early-bail never fired and each probe left a modal up for its full timeout. The
control I needed was one I already had: enumerate windows *while blocked* and
notice that the count is zero when a dialog is plainly there.

### The control is what makes the reproduction mean anything

The **default** rack round-trips **byte-identical** — 18,169 → 18,169, no dialog,
no unresolved parameters. So the loss genuinely needs the inserted modules and is
not a property of save-and-reload as such. Fixture kept:
`_scratch/e48-rack-session.xml`, 65,878 B holding a 49,295 B document.

Also worth writing down: **`session.xml` is written only on quit.** It is absent
through the whole editing session, which is why an earlier measurement read
49,297 bytes and then found 18,106 on disk moments later — two different writes,
not one file changing under me.

### What landed, and what it replaces

`scripts/check-prefab-modules.py` fails when a shipped prefab names a module
absent from the built binary. It catches this defect (1 of 32 types) and
**skips rather than passing when it cannot find a binary**.

**`check-prefab-layout.py` could never have caught this**, and the reason is
worth keeping: its check #2 is *every `<module type=X>` has a `<Plugin id=X>`* —
but a prefab carries its **own** `PluginList`, so that asks whether the FILE
describes its modules, not whether the PRODUCT has them. `AR_jef.synthedit`
passed it for weeks.

The test is **absence**, deliberately: a registered id must exist as a literal in
the binary, so a type string appearing nowhere cannot be registered — while a
string that *is* present proves nothing. Only the direction it can be sure of is
reported. **Not wired into CI**: the bot token has no `workflow` scope.

### What is left, and it is not mine

Re-author `AR_jef.synthedit` to use a module TIDE ships, **or** add `Adsr.cpp` +
`EnvelopeAdsr.xml` to the compiled-in set — a **PLAN constraint 7** decision
about the fixed module set. A run must not pick, so the row stays TODO.

**Learned:**

- **A dialog with an empty caption is invisible to every handle we have.**
  `MainWindowTitle` empty, `EnumWindows` listing nothing, stderr silent, CPU
  idle — four instruments agreeing on "nothing here" while a modal is on screen.
- **Do not promise a mitigation you have not seen fire.** I claimed a bounded,
  title-detecting harness made GUI driving safe, on the strength of a detector
  that had never been tested against the dialog it existed for.
- **Attach to a hung process; do not launch it under the debugger.** Seven
  minutes versus seconds, for the same question.
- **A checker that validates a file against itself is not validating the
  product.** The prefab's own `PluginList` made it self-consistent and
  unloadable at the same time.
- **`session.xml` is written on quit only.** Reading it mid-session tells you
  about the *previous* run.

**Next:** the product decision above. Once it is made, E48's Accept is one
re-run of the fixture. **E53** still wants a faulting address and now has a
working technique for it — attach, do not launch.

**Machine state.** All eight repos on their default branches, clean.
`%APPDATA%\TIDE Rack\` holds a scratch session from this work, not Jeff's — his
was restored earlier and this run replaced it again; **restored once more at the
end and md5-verified.** No TIDE-Rack or cdb process running, checked by
enumerating every visible top-level window as well as by name.

**Branch/PR:** `tide/win/E48-connectors-lost` — the check, E48's row, E51's
annotation, and this entry.

## 2026-08-27 — windows — E32, E34, E42 archived; E42's row cited an issue as its PR; E25 marked as mac's (state update, interactive, Jeff directing)

**Prompt:** *"what gated stuff remains?"* then *"yes, flip them and fix the link. note mac agent is working on E25"*

**Did:** flipped **E32**, **E34** and **E42** to DONE and moved all three to
[BACKLOG-DONE.md](BACKLOG-DONE.md) — `BACKLOG.md` **259 KB → 237 KB**, 51 rows →
48. Corrected E42's PR link. Marked **E25** as taken by the macOS agent. No code.

### All three were stale in the same way

Every linked PR merged, nothing open, all three still `IN-REVIEW`:

- **E32** — its own words were *"E32 closes when #23 lands"*.
  [GMPI_Wrappers#23](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/23)
  landed.
- **E34** — [SynthEditLib#60](https://github.com/JeffMcClintock/SynthEditLib/pull/60)
  merged, and Jeff had already run the drag himself and passed it.
- **E42** — [SynthEditLib#63](https://github.com/JeffMcClintock/SynthEditLib/pull/63)
  merged, and its TideSynth half merged as
  [#499](https://github.com/JeffMcClintock/TideSynth/pull/499).

That makes **seven rows in two days** found saying something their own merged PRs
had made false (E32, X2, R5, E25, E34, E42, and E49's clause). E45 shipped a
check for the state *after* a flip — `check-backlog-archived.py`, which correctly
refused to let these three be DONE and stay in `BACKLOG.md` — but **nothing
anywhere notices a row that should have been flipped and was not.** That gap is
the one still open.

### E42's row cited an issue as its PR

The row says its TideSynth half is `#497`. **#497 is an issue** — *"Build failure
on macos — tide/mac/install-dialog-drain"* — not a pull request. The real one is
**#499**.

**Corrected in the prepended note, not in place**, because
`check-backlog-diff.py` requires a row's base text to survive verbatim; the wrong
number is left below deliberately. The underlying cause is the one **A22** exists
for: a PR number written into a row before the PR exists is a guess, which is why
A22 says name the **branch** and treat the number as a best-effort extra.

### E25 is mac's, and nothing mechanical would have said so

Marked at Jeff's instruction. Worth recording *why* the note is load-bearing:
**STEP 2's collision check reads remote branches and open PRs, and work in
progress on another box is invisible to both until something is pushed.** E25
carries no DOING mark, so a windows or linux run walking the queue would have read
it as free and taken it. The row now says otherwise.

**Learned:**

- **A check on the end state does not catch a transition that never happened.**
  E45's archive check makes a *flipped* row behave; nothing raises a hand for a row
  whose PRs all merged a day ago and which still says IN-REVIEW.
- **A row can cite a number that resolves to the wrong kind of object.**
  `#497` is a real, live, closed thing in the same repo — it just is not a PR.
  Every link check passed it, because the link works.
- **The queue cannot see another machine's uncommitted work.** If a box is
  mid-row with nothing pushed, only a human can tell the other two.

**Next:** unchanged — **E53** wants a faulting address, **E48** has E49's
diagnostic as a lead. The remaining gated work is **S1b**, **S8**, E51's
SynthEditLib half, **E23** (repo on neither STEP 5 list), and X2's 1,547 gated
warnings, which is Jeff's decision rather than a task.

**Branch/PR:** `tide/win/flip-E32-E34-E42` — three rows moved verbatim, E25
annotated, and this entry. Bookkeeping only.

## 2026-08-27 — windows — The gated guards are on SynthEditLib's main, and none of the three rows can honestly say DONE (state update, interactive, Jeff directing)

**Prompt:** *"merge 64"*

**Did:** merged
[SynthEditLib#64](https://github.com/JeffMcClintock/SynthEditLib/pull/64) —
`796bbc2`, squashed, branch deleted. **E49**, **E46** and **E47** go
**DONE-PENDING-ACCEPT**, not DONE. No code changed; this is the flip and this
entry.

### Why not DONE, for each of the three

A merged PR says the code landed. It does not say the row's Accept was met, and
for all three of these it was not:

- **E49** has two clauses and exactly one is met. *"A pair the patch manager does
  not have is refused with a diagnostic rather than dereferenced"* — met and
  measured. *"The 38,658-byte document loads without faulting"* — **not**: it
  still segfaults 3/3, after the graph is live instead of during the build.
  **That clause is now E53's**, and E53 is the row to take.
- **E46** and **E47** are guards written from the source and the call graph, with
  **no reproduction of either defect**. Their own rows say do not quote them as
  measured, and merging the guard does not change that. E47's warning is the one
  to keep in view: *"it did not crash this time" is what a dangling pointer also
  looks like.*

`DONE-PENDING-ACCEPT` is the status this backlog already has for exactly this,
and `check-backlog-archived.py` deliberately does not treat it as archivable —
so all three stay visible in `BACKLOG.md` instead of moving to the archive on the
strength of a merge.

**Learned:**

- **A merge is evidence about the code, not about the row.** Three PRs landed
  together and none of the three Accepts was met; flipping all three to DONE
  would have retired two unreproduced defects and one half-fixed one in a single
  edit that every lint would have passed.
- **When a fix and its verification land apart, say which one you have.** The
  distinction between "the guard is in" and "the bug is gone" is the entire
  content of these three rows now.

**Next:** **E53** — the fixture still faults, and it wants a faulting address
first. **E48** has E49's diagnostic as a lead. Nobody has run SynthEdit's
`ctest` against these changes.

**Branch/PR:** `tide/win/E49-E46-E47-landed` — the three rows and this entry.
Bookkeeping only.

## 2026-08-27 — windows — The gated null guards: one is measured, two are not, and the fixture still crashes one layer further in (interactive, Jeff directing)

**Prompt:** *"do the gated fixes"*, after the E50 run above · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** · as **tide-rack-bot**

**Did:** wrote the three outstanding gated guards —
[SynthEditLib#64](https://github.com/JeffMcClintock/SynthEditLib/pull/64), branch
`tide/win/E49-E46-E47-null-guards`. **E49**, **E46** and **E47** go IN-REVIEW.
Found **E25's** two guards already on `main` and corrected that row. Filed
**E53** for a second crash the first fix exposed, and annotated **E48** with the
lead it handed over.

### One family, and only one of the three has a measurement

All three are the same defect: **a lookup that returns `nullptr` (or `end()`) by
design, dereferenced with an `assert` as the only guard, in a product that ships
`-DNDEBUG`.** The evidence behind them is not the same, and the rows say so
rather than letting the PR imply otherwise:

| | site | evidence |
|---|---|---|
| **E49** | `ug_patch_param_setter.cpp` | **A/B on a reproducing document** |
| **E46** | `EditorLib/PatchManager.cpp` | read off the source, no repro |
| **E47** | `EditorLib/PropertiesBrowser.cpp` | read off the call graph, no repro |

Writing a guard is not evidence a crash was real. E46 and E47 are guards worth
having and their Accepts are **not met**; both rows still say do not quote them
as measured.

### E49's A/B, and the thing it printed that was worth more than the fix

Same build tree, the commit the only difference, on the 38,658-byte fixture:

| | before | after |
|---|---|---|
| rack modules constructed | **3 of 5** | **5 of 5** |
| connections / DSP | never reached | made; `processing (block 96)`, `first NONZERO OUTPUT` |
| command channel | never opened | opens |

Then the diagnostic named the cause outright:

```
SynthEdit: no patch parameter for module 987654321 parameter id 0..7
```

**`987654321` is the `VCV: Scope`, and the document defines it** —
`<Module Id="987654321" Type="VCV: Scope">`. So the pair misses because that
module reached the graph with **no patch-manager parameters at all**, not because
the handle dangles. E49's own refinement — *"the miss must come from the
PARAMETER ID, on a module that does exist"* — was right that the module exists
and wrong that one id is at fault; all eight are absent. **A module whose
parameters went missing is a module whose connectors are lost**, which is E48's
symptom seen from the DSP end instead of the editor end. Annotated there as a
lead, untested as a cause.

### And the fix moved the crash rather than removing it

**The fixture still segfaults, 3/3 — later, and somewhere else.** Before, the
process died *during* graph build. Now the graph completes, all five modules
produce audio, the channel opens, and it faults after
`LFO2 first nonzero light`. E49's Accept has two clauses and exactly one is met.

Filed as **E53** rather than buried in E49, because it is a different fault with
its own trivial repro. A guard that turns "dies at step 3" into "dies at step 9"
is progress and is not a fix, and a row that says "fixed" when the document still
crashes is the kind of row this journal keeps having to correct.

### E25 was stale, and reading the file beat reading the row

E25 asks for two gated guards and says twice that they are unwritten because
`SynthEditLib` is GATED. **Both have been on `main` since earlier the same day** —
Jeff landed them himself as
[SynthEditLib#58](https://github.com/JeffMcClintock/SynthEditLib/pull/58), and
both carry comments naming E25. I found it by opening
`PatchParameter.cpp:1302`, which I was about to edit.

Status left **TODO** deliberately: its Accept is a repro *and* a fix *and* a
before/after crash-report count, and only the fix exists.

### What made this session cost more than it should have

**Seven minutes lost to `cdb`, and the reason is a finding.** Trying to get a
faulting address for E53, the app under the debugger never crashed and never
returned — and its log shows it built the **default** rack (17,960 bytes) rather
than restoring the fixture. Jeff, watching the screen: *"Assign Controller dialog
up for a long time"*, then *"gone now"*.

**That launch was made with `-quiet`.** E51's divert covers `SeMessageBox` and
`SeMessageBoxAsync`; *Assign Controller* is a different dialog class and goes
straight to the screen. So a headless run can still be silently blocked by a
modal, which is the exact failure E51 exists to prevent — recorded on E53 and
pointed at E51.

**I did not see it and could not have.** Nothing in the app's stderr mentions it,
and by the time I enumerated every visible top-level window the owning process
was gone. The only signal available from inside the run was a debugger that
produced no output for seven minutes, which reads identically to a slow build.

I then went on to propose running SynthEdit's `ctest` suite, which launches GUI
tests — the same hazard again. Jeff stopped it. **Not run, and the PR says so.**

**Learned:**

- **Writing the guard is not the same as verifying the bug.** Three fixes, one
  measurement. Marking all three the same way would have made two unmeasured
  claims look settled, and both rows explicitly warn against exactly that.
- **A guard that moves a crash has not fixed it.** "Dies at step 3" becoming
  "dies at step 9" is real progress and a separate row, not a closed one.
- **The diagnostic is often worth more than the guard.** `return` on null would
  have stopped the crash silently; printing the pair identified the culprit
  module, corrected the row's own hypothesis, and handed E48 a lead — for one
  extra line.
- **Read the file you are about to edit before believing the row that sent you.**
  E25 was two commits out of date and would have had me re-apply a fix already on
  `main`.
- **A modal can block a headless run with no trace in its own output.** Seven
  minutes of debugger silence and a slow build are indistinguishable from inside;
  only a person looking at the screen could tell. `-quiet` is not proof against
  it.
- **`SynthEditLib` ships in SynthEdit too, so build SynthEdit.** 1088/1088 on the
  `SE16` tree is what makes "TIDE builds" mean anything about the commercial
  product, and it is one command on this box.

**Next:** **E53** wants a faulting address, and getting one needs a debugger that
does not deadlock on that modal, or a build with the dialog suppressed. **E48**
now has a concrete lead. Nobody has run SynthEdit's `ctest` against these
changes.

**Machine state.** `SynthEditLib` on `tide/win/E49-E46-E47-null-guards` with
[#64](https://github.com/JeffMcClintock/SynthEditLib/pull/64) open; every other
repo on its default branch and clean. Two scratch build trees
(`_scratch/e50-off`, `_scratch/e50-se16`) outside every repo. `%APPDATA%\TIDE
Rack\` restored byte-for-byte, md5-verified; no TIDE, SynthEdit or `cdb` process
left running, checked by enumerating every visible top-level window.

**Branch/PR:** `tide/win/E49-E46-E47-gated-guards` in TideSynth (rows and this
entry) and `tide/win/E49-E46-E47-null-guards` in SynthEditLib (the code).
**Merging TideSynth's side alone changes no behaviour**; merging SynthEditLib's
alone leaves the backlog saying the work is open.

## 2026-08-27 — windows — E50: the test the row asked for, with the controls it did not — the Compare does not reproduce, and the same wrong name is in E49's row (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** · as **tide-rack-bot** (both paths)

**Did:** took **E50**, the NEXT block's `win` pick. Ran its one-build test, added
three controls it did not ask for, and **could not reproduce its premise**. The
Accept's second limb is met by measurement rather than by a fix, so the row is
**IN-REVIEW** with its content rewritten from a hypothesis into a result.
Annotated **E48** and **E49** — they lose the common cause this row offered them
— and re-pointed the `win` NEXT cell at E48. **No product code changed in any
repo.**

### The test, and what it takes for ON-vs-OFF to mean anything

Five launches, three builds, two documents:

| default rack staged | `main` `=ON` (39 modules, exe 5,346,816 B) | `main` `=OFF` (exe 3,780,096 B) | `c6697b80c` `=ON` |
|---|---|---|---|
| today's **25,110 B** | **17,958** | **17,962** | **17,958** (its own 25,109 B) |
| the row's **25,257 B** (`bed03b0a0`) | **17,969** | **17,969** | **17,973** |

**On the row's own document ON and OFF agree to the byte.** No `RackEditor:` and
no `RackProcessor:` line appears in any of the five — not a `Compare`, not
anything.

**Know the noise floor before reading a difference.** Module handles are random
and their decimal length varies, so the pushed document jitters a few bytes per
launch: these five span **17,958–17,973**. The row's **18,183** is 210+ bytes
outside that band, so it is real and cannot be the same rack measured twice —
which is exactly why "it does not reproduce" is worth writing down rather than
shrugging at.

**The third build is the one that closes it.** A cold build of **`c6697b80c`** —
the E19 run's own merge commit — with the pack ON and the trace live, on the
exact 25,257-byte document, is clean too. So this is not "the tree has moved on";
it does not reproduce at the commit that reported it.

### The positive control is what makes an absence worth anything

An absence and a trace that was never compiled in print the same thing. The same
ON binary, given `_scratch/e19-fixture-doc.xml` — E49's own 38,658-byte fixture —
prints five `RackEditor:` lines and three `RackProcessor:` lines, and **every one
resolves to its own slug and its own art**:

```
RackEditor: 'LFO2'   art=yes(res/WTLFO.svg)     RackProcessor: 'Pulses' constructed
RackEditor: 'Scope'  art=yes(res/Scope.svg)     RackProcessor: 'SHASR'  constructed
RackEditor: 'LFO'    art=yes(res/LFO.svg)       RackProcessor: 'Scope'  constructed
RackEditor: 'SHASR'  art=yes(res/SHASR.svg)
RackEditor: 'Pulses' art=yes(res/Pulses.svg)
```

So the instrument fires, and **no module resolves to the wrong class** — the S46
shape E50 rests on. The other half of that mechanism is ruled out by reading:
`ModelRegistry::find` returns **`nullptr`** on a miss
(`SynthEdit_Rack_Adaptor/rack/rack.hpp:2901`), with no first-entry fallback, so
an unknown or empty slug cannot silently become the first registered model.

### E49 reproduces, and its row names a module its own document does not contain

That fixture launch **crashed**: exit `0xC0000005` after exactly
`Pulses / SHASR / Scope constructed` and no further. E49 says the same, so that
row is reproducible on demand and its diagnosis stands.

But E49 records `LFO, LFO2, VCA x3, Compare` as never reached, and
`e19-fixture-doc.xml` **has no Compare in it** — `grep -ci compare` is 0. Its
module types are `VCV: LFO`, `VCV: LFO2`, `VCV: Pulses`, `VCV: SHASR`,
`VCV: Scope`, three bare `VCA`, and TIDE's own parts.

**So one session named a Compare twice, in two unrelated observations, and both
documents are provably without one.** The economical reading is a mis-attributed
module name rather than a defect that has since vanished — but that is a reading,
not a measurement, and both rows now say so rather than closing on it.

**What would reopen it, in one sentence:** any `RackEditor:`/`RackProcessor:`
line naming a module the launched document does not contain. One
`RACK_ADAPTOR_TRACE=1` build and one launch.

### E48 and E49 lose their common cause

"One cause, three symptoms" was E50's own framing and it was explicitly untested.
With the pack ruled out, **E49's measured diagnosis stands alone** — a null
`parameter` at `SynthEditLib/ug_patch_param_setter.cpp:172`, reached on the
WASAPI render thread — and waits on nothing here. **E48 was not re-tested**, and
it has not been re-run since `-quiet` landed (E51), which changes how its modal
behaves headlessly. It is where the `win` cell now points.

### A thing I got wrong on the way, and the command that fixed it

`c6697b80c` changes `DefaultRack.synthedit` while its own journal entry says *"No
product code changed in any repo"*, and I spent a while building a story about a
run sweeping Jeff's working-tree edit into its branch — the exact hazard STEP 5's
third kind of dirt exists for. **It is nothing of the sort.** One
`git log -1` on the branch head named in issue #491:

```
0876c3ace  Jeff McClintock  13:06  switched Default Rack to MIDI-In (no LED). else LED appears on the rack
```

Jeff committed it onto the run's branch himself, two minutes before the squash
merged. The run's claim was true of the run's own commits. **Nothing to file** —
recorded because I was one command away from filing a row against a colleague's
commit, and the accusation would have been in the permanent record.

**Learned:**

- **An absence is worth nothing until the instrument has been seen to fire.**
  "No `RackProcessor: 'Compare'`" and "no trace compiled in" print the same
  thing. Five named modules on a second document is what separated them, and it
  cost one launch.
- **Rule a hypothesis out from both ends.** ON-vs-OFF says the pack changes
  nothing; the fixture says every module resolves to its own class. Either alone
  leaves the other reading open.
- **Measure the noise floor before reading the difference**, especially when the
  row's whole argument is a size comparison. Random handles move this document a
  few bytes a launch: 4 bytes is nothing, 214 is everything.
- **Reproduce at the reporting commit, not just at `main`.** "It is fixed" and
  "it never happened here" are different findings, and only a build of the
  reporter's own commit tells them apart. It cost one worktree and six minutes.
- **When a row's premise will not reproduce, check its siblings for the same
  claim.** E49's never-reached list carried the identical absent module name;
  finding it there turned "I cannot reproduce this" into a reading of what
  happened.
- **`strings` on a Windows PE found none of the format strings that were
  demonstrably in the binary**; a plain `grep -c` on the file found all three. I
  briefly concluded the trace was not compiled in. Do not read a silent tool as
  evidence of absence — the archive already has this lesson from
  `-ErrorAction SilentlyContinue`, and this is the same mistake with a different
  tool.
- **Check the author before writing a blame.** A commit that contradicts a run's
  own journal entry is more likely to be someone else's than a run lying about
  itself.

**Next:** **E48** — the modal `Connectors lost while loading` and its 3,577
dropped bytes — now that it has no candidate cause, on the box where it was
seen. **E19's win VST3 cell stays blocked behind it.**

**Machine state.** `main` green. No open `platform:win` issue — and the `win`
cell's standing warning still holds, that `build.yml:409` excludes
`matrix.platform == 'win'` from filing them, so an empty list has verified
nothing. Two mac branches sit on the remote with no PR
(`tide/mac/E36-renumber-duplicate-e34`, `tide/mac/icon-tide-app`), which is the
one end state STEP 5 forbids; not mine to unwind, noted for whoever owns them.
All six repos on their default branches and clean. `%APPDATA%\TIDE Rack\` was
copied to the session scratchpad before the first launch and restored
byte-for-byte after, verified by md5; no TIDE process left running. Build trees
`_scratch/e50-off` and `_scratch/e50-base` are outside every repo; the
`_scratch/e50-wt-c6697b8` worktree is removed.

**Branch/PR:** `tide/win/E50-vcv-compare-default-rack` — TideSynth only: E50's
row, annotations on E48 and E49, the `win` NEXT cell, and this entry.

## 2026-08-27 — macos — correction: V7 was tested on BOTH build arms, and the caveat I wrote was mine, not the record's (state update, interactive)

**Prompt:** *"re right-click, i tested both builds."*

**Did:** prepended a correction to V7's archived row. No code; the previous
entry stands as written, because this file is prepend-only and a log you edit is
not a log.

**What was wrong.** Closing V7 an hour ago I recorded that the verdict *"right-click
menu passed manual test"* could not itemise which rules were exercised, and
added that **a Debug build cannot demonstrate the four Release-only removals**
(`Pa&nel Edit...` / `Panel Edit...`, `Goto Parent Container` /
`Goto Parent...`), since they are deliberately still present there. That is a
true statement about one build and a false statement about what was done. Both
arms were tested. V7 has no on-screen gap at all — the probe covers both
(`-D_DEBUG` re-run separately, 0 failures) and so does the human.

**The mistake is not the hedge, it is where I put it.** "Which build did you
run?" is one line in a message and would have cost nothing; instead I wrote the
uncertainty into the permanent record as a limitation of the work. A caveat
costs the next reader real time — they have to decide whether to go and close
it — so an unasked question that becomes a written limit is more expensive than
the question would have been.

**And it is the second time today in the same shape.** Earlier I put a choice to
Jeff about two menu items that are in mutually exclusive branches — a question
that should not have been asked, because reading four lines of `if/else` settled
it. Now the reverse: a question that should have been asked, resolved into a
caveat instead. **Both are the same failure to check the cost of asking against
the cost of guessing**, and both were caught by Jeff rather than by me.

**Learned:**

- **An unasked question written into the record as a caveat is more expensive
  than asking.** A caveat outlives the conversation and obliges every later
  reader to re-evaluate it; a message costs one line and ends there.
- **Check the cost of asking against the cost of assuming, in both directions.**
  Twice today I got it wrong in opposite directions on the same row — a question
  the code already answered, and an assumption a question would have answered.

**Next:** nothing on V7; it is closed with no remaining gap.

**Branch/PR:** `tide/mac/V7-both-builds-tested` — the archived row's correction
and this entry.

## 2026-08-27 — macos — V7 closed by a human right-click, which is the only instrument that exists for it (state update, interactive)

**Prompt:** *"right-click menu passed manual test"*.

**Did:** flipped **V7** to DONE and archived it. No code changed;
[#500](https://github.com/JeffMcClintock/TideSynth/pull/500) merged at 05:58Z
and this was the row's one remaining gap.

**The gap was structural, not an oversight, and that is why it took a person.**
V7 shipped verified by probe — 23 checks across both build arms, asserting on
the far side of the filtering sink. What a probe there can never assert is that
a menu APPEARED. Three separate measured reasons close off every automated
route:

- **E38** — the command channel cannot raise a context menu. The menu is raised
  by `DrawingFrameCommon::doContextMenu` on the FRAME; `cmdPointer` calls the
  INPUT CLIENT and never touches the frame.
- **E43** — trying it on macOS *wedges* the app: a native `NSMenu` runs a nested
  modal run loop inside the command's job.
- `--screenshot` reads `context.framePixels`, the app's own render buffer, and a
  macOS popup is a separate window — so it could not have seen one even if one
  opened.

So the honest state of that clause was never "not done yet"; it was "not
reachable from here", and it stayed that way through three rows trying.

**WHAT THE VERDICT DOES NOT ITEMISE, recorded so nobody reads more into it than
was said.** It is a human verdict on the menu as a whole, not a per-rule
checklist — and **which rules a run of the app exercises depends on the build**.
The four Release-only strings (`Pa&nel Edit...` / `Panel Edit...`,
`Goto Parent Container` / `Goto Parent...`) are deliberately still PRESENT in a
Debug build, so a Debug test cannot have observed their removal. The probe
covers both arms (`-D_DEBUG` re-run separately, 0 failures), so what remains is
on-screen coverage of one arm, which is the narrowest this row has ever been.
A single right-click on a Release build would close it outright.

**E45's check earned its keep on its first real customer.** Flipping V7 to DONE
and leaving it in `BACKLOG.md` now FAILS `check-backlog-archived.py`, so the
flip and the archive are one action instead of two — and the second one can no
longer be the step somebody forgets. That is the exact failure the row was filed
for, caught the same day the check landed:

```
50 row(s) in BACKLOG.md, none DONE, all terminated, OK (242 KB)
```

**Learned:**

- **"Not verified" and "not verifiable from here" are different claims, and only
  one of them is a to-do.** V7 carried the second for two days while reading like
  the first. Naming which one it is tells the next reader whether to try again or
  to go and find a human.
- **A human verdict closes a clause; it does not itemise one.** Record what was
  actually said and what it cannot cover — here, that a Debug build cannot
  demonstrate a Release-only removal — rather than promoting "passed" into
  "every rule observed".

**Next:** nothing on V7. The same instrument gap is what **E44** was filed to
fix for the app's own menu bar, and that has since merged — a context menu still
has no equivalent.

**Branch/PR:** `tide/mac/V7-manual-verified` — the row, its archive move, and
this entry. Bookkeeping only.

## 2026-08-27 — macos — E45: the check found the row the sweep missed (interactive session, Jeff directing)

**Prompt:** standing backlog-loop instruction in the session · Opus 5, `claude-opus-5` · Claude Code · commits authored `Jeff McClintock` per the interactive convention

**Did:** took **E45**. Both halves built, as two PRs the row explicitly asked to
keep apart — the sweep is
[#503](https://github.com/JeffMcClintock/TideSynth/pull/503), the check is
[#504](https://github.com/JeffMcClintock/TideSynth/pull/504).

**`BACKLOG.md` 754,322 → 241,344 bytes. 89 rows moved. A 68% cut** to the file
every run on three machines reads first.

### The order was the whole trick, and it was the row's idea

E45 says to land the sweep and the check separately, because *"a move-only diff
is reviewable by size, and a check plus a 600-row move is not."* That is a
reviewability argument. It turned out to be a **correctness** one too.

I wrote the sweep first, ran it, verified it four ways, and it looked clean: 88
rows moved, 0 Item texts differing, 0 lines added. Then I wrote the check and ran
it against my own output. It reported one row still `DONE`:

```
| E6 |  DONE | any | ...
```

**Two spaces before the status.** My sweep's regex required one; the lint's
`\| ([^\|]+) \|` accepts either. So E6 would have stayed behind — `DONE`,
invisible to me, and visible to every lint that mattered.

**E45's own text warns about this**, in the other direction: a previous run's
detector rejected a trailing space and reported two invisible rows where the real
lint saw one. The row's conclusion is *"checking with the regex that matters,
rather than one that looks equivalent, is the whole lesson."* I made the mirror
image of that mistake inside the row that records it.

**The transferable bit: when a bulk edit and its validator are both in scope,
write the validator first and point it at your own output.** Four hand-rolled
verifications agreed with each other and were all wrong the same way, because
they shared my regex. The check disagreed because it borrowed the lint's.

### What the check asserts

Two things, and the second fails silently in the direction that removes
protection:

- **No row is `DONE` and still in `BACKLOG.md`.** `DONE-PENDING-CI`,
  `DONE-PENDING-ACCEPT`, `IN-REVIEW`, `WONTFIX` and `RESOLVED` are deliberately
  not flagged — only a bare `DONE` means "merged, belongs in the archive".
- **Every row is terminated.** A row missing its closing `|` does not match
  `check-backlog-diff.py`'s regex, so that row **does not exist** as far as that
  lint is concerned — a run could rewrite or delete it and the diff check would
  report clean. E43 sat in that state until E45 went looking. Zero invisible in
  either file today; nothing stopped it recurring until now.

### Still needs Jeff

**One line in `.github/workflows/lint.yml`.** The check is not wired in, because
the bot token deliberately has no `workflow` scope. **Until it is, the check
exists and enforces nothing** — so E45 should not be flipped to DONE on the two
PRs alone.

## 2026-08-27 — macos — E44: the menu verb, and an Accept that names an item TIDE does not have (interactive session, Jeff directing)

**Prompt:** standing backlog-loop instruction in the session · Opus 5, `claude-opus-5` · Claude Code · commits authored `Jeff McClintock` per the interactive convention

**Did:** took **E44** — a verb that drives a menu action without raising a menu.
Built, measured and merged as
[GMPI_Wrappers#27](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/27).
Row is DONE-PENDING-ACCEPT, because its Accept cannot run.

### The row's first instruction was the whole job

> Check first whether the action can be invoked without raising the menu at all —
> if `MenuBarView` dispatches through a table of named commands, this is small;
> if the action only exists as a closure hung off a drawn item, it is medium and
> wants designing.

It is the small case. `MenuBarView::Item` is `{ label, std::function<void()>
action, enabled, checked }` — a model, not a drawing. So the verb looks the item
up by name and calls the function; the drawn bar and its native `NSMenu` are
never involved.

**Worth generalising: the row spent one sentence telling the next run where to
look before committing to a size, and that sentence saved the session.** A row
that says "check X first, and the answer changes the shape" is worth more than
one that guesses the shape.

### The clause that separates this from E43

E43 made a wedged command *bounded and self-describing*; E44 has to make it not
wedge at all. The Accept encodes that as *"`--info` answers immediately
afterwards on the SAME connection"* — a bounded error satisfies E43 and must not
satisfy this. Three commands, one connection:

```
--menu Revert to Plugin Defaults   0.002 s   ok
--info                             0.018 s   ok
--ping                             0.014 s   ok
```

Ordinary latency, not a deadline. And the actions really run: invoking
`Audio/MIDI Settings...` visibly switched the window to the settings page, and
`--menu File/Quit` **exits the process cleanly** — no `kill -9`, which is exactly
what a pointer click could never manage.

### The Accept names an item TIDE does not have

It says **`--menu save`**. Measured: `File` holds *Revert to Plugin Defaults*, a
separator, *Quit*; `Options` holds *Audio/MIDI Settings...*, a separator, *Quit*.
**There is no Save and no Open.** The row was written assuming `File > Save`
existed, so its instrument cannot run and *"the session file on disk changes"*
has nothing to trigger it.

**And that matters beyond bookkeeping, because it undercuts the row's own
motivation.** E44 argues that every Accept phrased *"save and reload"* is
unreachable headlessly on macOS. The wedge is now gone — but there is still **no
Save item to invoke**, so those Accepts are no closer. They need either a Save
menu item or E43's `kill -TERM` route, which saves unconditionally on the normal
teardown.

### Two design choices the menus forced

- **TIDE has two items called *Quit***, one per menu. A bare ambiguous label is
  an error naming both candidates rather than a guess — picking the first would
  be a coin toss the caller cannot see.
- **A slash is only a menu/item split when the left side names a menu.**
  Otherwise `Audio/MIDI Settings...` reads as menu *Audio*, item *MIDI
  Settings...*, and the one real item with a slash in its label becomes
  unreachable.

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
