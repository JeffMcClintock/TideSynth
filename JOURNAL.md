# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

**WHY THIS SECTION IS ABOVE THE ENTRIES AND MUST STAY THERE (A36, 2026-09-10).**
It used to sit *below* the oldest entry, and on 2026-09-01 a rotation swept it
into the archive along with the entries it was standing behind — because a
rotation removes the oldest entries and the oldest entries are at the bottom.
**The instruction that says "do this every run" stopped being read every run,
and the file grew from 53 KB to 229 KB across the fourteen entries that
followed, written by runs that could no longer see this rule.** Nothing caught it: `check-journal-prepend.py` deliberately does
not gate the header block, and the rotation was legal in every other respect.
Above the first dated entry a rotation cannot reach this — which is also where
the run prompt ("the template at the top of that file"), that script's own
docstring ("the header/template block above the first real entry") and A8's own
row ("the live file holding the template + current month") have all said it was.

**One more thing to check before you rotate**, learned the same day: rotation
opens a NEW archive file whenever it crosses a month, and
`scripts/extract-lessons.py` must be able to SEE that file or every lesson you
move into it drops out of [docs/lessons.md](docs/lessons.md) silently — the
exact failure A30 exists to prevent. It discovers `JOURNAL-YYYY-MM.md` by glob
now, so this should stay true on its own; `python3 scripts/extract-lessons.py
--check` before and after a rotation is the one command that proves it did.

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

**Archives:** [JOURNAL-2026-09.md](JOURNAL-2026-09.md), [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-09-10 — macos — A36: the rotation rule had rotated itself out, and the lessons digest was about to lose 106 lessons (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.49585.0** · as **tide-rack-bot** (both paths) · scheduled run

**Did:** walked every backlog row, found all of them ineligible, and did the housekeeping four consecutive `mac` cells have named — `JOURNAL.md` rotation. **Filed and fixed A36: the reason it never happens is that the rule telling runs to do it had itself been rotated into the archive.** Also filed **A37** (not taken), and flipped **E83** to DONE and moved it verbatim to [BACKLOG-DONE.md](BACKLOG-DONE.md) after [#581](https://github.com/JeffMcClintock/TideSynth/pull/581) merged.

### The finding: a remedy carried off the rule that governed it

`JOURNAL.md` kept two undated trailer sections **below its oldest entry** — `## Rotation — do this as part of STEP 4, every run`, and the entry template. A rotation removes the **oldest** entries, and the oldest entries are at the **bottom**, so the trailer went out with them.

| commit | date | `## Rotation` | template | `JOURNAL.md` |
|---|---|---|---|---|
| `14c3aaa` | 2026-08-31 | present | present | **67,105 B** |
| `b824422` ([#565](https://github.com/JeffMcClintock/TideSynth/pull/565)) | 2026-09-01 | **gone** | **gone** | 52,942 B |
| 15 commits since, 14 entries | to 2026-09-09 | absent | absent | **228,834 B** |

`b824422`'s diffstat is the mechanism, not an inference: `JOURNAL.md -453 lines`, `JOURNAL-2026-08.md +308`. The rotation was **legal in every respect** — every moved entry reappears verbatim in the archive, which is all A8 asks — and `check-journal-prepend.py` deliberately does not gate the header block, so nothing could have caught it.

**Four cells blamed the wrong thing.** 09-06, 09-07, 09-08 and 09-09 each called the rotation "housekeeping nobody owns" and each explained the delay by an open PR touching `JOURNAL.md`. The open PR was never the reason; the instruction was missing.

### The fix is *where* the trailer lives, not what it says

It now sits **above the first dated entry**, where a rotation cannot reach it. That is not a new invention — it is where three independent descriptions already said it was:

- the run prompt: *"Append a JOURNAL.md entry using the template **at the top of that file**"*
- `check-journal-prepend.py`'s docstring: *"The **header/template block above the first real entry** is not checked"*
- **A8's own archived row**: *"the **live file holding the template** + current month"*

The trailer had been in the one place all three exclude.

### The half worth more than the rotation: the lessons digest was about to lose 106 lessons

`scripts/extract-lessons.py` hard-coded `SOURCES = ["JOURNAL.md", "JOURNAL-2026-08.md"]`. This rotation opens `JOURNAL-2026-09.md` — so **every lesson moved there would have vanished from `docs/lessons.md`**, the one file A30 created to stop lessons ageing out. Silently, and caused by the A8 remedy.

**A/B on the identical rotated tree**, which is the control that makes this a measurement rather than a code reading:

| `SOURCES` | entries | lessons |
|---|---|---|
| old hard-coded pair | 325 | 1354 |
| discovered by glob | **340** | **1460** |

**15 entries and 106 lessons.** `SOURCES` now globs `JOURNAL-YYYY-MM.md`, newest month first, and the preamble's "read the working" pointer is generated from that list instead of naming August.

### Verification

| clause | evidence |
|---|---|
| nothing lost in the rotation | `extract-lessons.py --check` → **1460 lessons from 340 entries**, identical to the pre-rotation baseline taken before any file was touched |
| the 17 moved entries are verbatim | `check-journal-prepend.py` → *17 entries rotated out, verified verbatim elsewhere in the diff* · `prepend-only, OK` |
| the 3 kept entries are untouched | entries 1 and 2 **byte-identical** to `origin/main`; entry 3 differs by its final newline alone, being newly last — the case `canonical()` was written for |
| size | `JOURNAL.md` **228,834 → 49,092 B**, under A24's 60 KB with its floor (four most recent entries) intact |
| E83 archived verbatim | `check-backlog-diff.py` → *1 row(s) archived, verified verbatim* |
| lint | all six `lint.yml` checks rc=0 locally, plus `check-backlog-archived.py` |

**Not verified:** nothing was built and nothing needed to be — this run changed no code. `main`'s macOS build state is therefore carried, not re-measured; the 2026-09-09 mac entry recorded `TIDE_Rack_CLAP` at 61/61, 0 errors, and nothing here touches it.

**One edit declared rather than slipped in.** The swept template block was **corrupt**: a linux entry's `### Correction: Ardour IS a host here` section and a stray `0.` Learned bullet had been spliced inside its fence since before 2026-08-28. The restored copy is the last clean one, `32c7028:JOURNAL.md` (2026-08-22), verbatim, and the corrupt copy is removed from the August archive rather than left to be found twice.

**Learned:**

- **A remedy that MOVES data can carry off the rule that governs it, and every check can stay green while it does.** Rotation deleted the rotation rule; the lessons glob would have deleted the lessons. Both halves of this run are the same shape, and neither is visible from any single commit — only from the file's size curve across nine of them.
- **When an instruction stops being followed, suspect that it stopped being READABLE before suspecting the readers.** Fourteen entries went in across three machines without one rotation, and four cells wrote down a reason that was wrong. That many careful runs agreeing is evidence about the input, not about them.
- **A rule's position is part of its content when a process moves things.** "Above the first entry" is not formatting; it is what makes the rule survive the operation it describes.
- **Three descriptions can all say where something is while it is somewhere else.** The prompt, the check's docstring and A8's row all said "at the top". Nobody had compared them to the file, and comparing them cost one `grep`.
- **Take the baseline BEFORE you touch anything, or your after-number proves nothing.** `extract-lessons.py --check` on the untouched tree is the only reason `1460/340` afterwards means "lost nothing" rather than "ran successfully".
- **Match the file's own separator convention, and check it rather than assuming.** The first rebuild inserted `---` between entries because the archive uses them; `JOURNAL.md` does not, and that turned three untouched entries into three modified ones. The canonical-comparison check still passed — a green check is not the same as a clean diff.

**Machine state.** Screen **locked** (`ioreg -n Root -d1 -a` → `CGSSessionScreenIsLocked` present), which is why every GUI row was out. All six repos were clean and on their default branches at the start; only `TideSynth` was touched. Nothing built, nothing running, no developer working tree disturbed.

**Next:** **A37** is the same unbounded-growth shape in `BACKLOG.md`'s NEXT block — the `mac` cell this run replaced was 34,988 bytes with a ten-deep chain, against `any` at 4,661; it is filed and deliberately not taken. **E84** still cannot be closed by any scheduled run — it is a `.github/workflows/**` edit and the bot's token has no `workflow` scope, so it wants Jeff or an interactive session. The mac lane's GUI rows (**E80**, **E82**, **E19**'s cells) still want an unlocked screen; **E80**'s specific next step is the two counters measured **with an editor present**.

**Branch/PR:** `tide/mac/A36-journal-rotation-rule` — the rotation, the trailer's move to the top, the `extract-lessons.py` glob, A36, A37, E83's flip and archive, the `mac` NEXT cell, and this entry.

## 2026-09-09 — windows — E80's second opinion: the blob does not travel on Windows either, and it is not the timer (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.46388.4.0** · as **tide-rack-bot** (both paths) · scheduled run

**Did:** took **E80** and answered its step one — *"a second opinion, not a fix"* — with a **new bare CLAP host that builds on Windows**, [tests/e80_clap_feedback_probe.c](tests/e80_clap_feedback_probe.c). E80's symptom reproduces here with **no GUI, no DAW, no GTK and no compositor**. Row back to TODO; no product behaviour changed. **No host was driven, because the developer was working at the machine** — see below, which is the part of this entry worth more than the measurement.

### The measurement

`tests/fixtures/e75-vcv-visible-rack.xml`, `TIDE_VCV_FUNDAMENTAL=ON -DRACK_ADAPTOR_TRACE=1`, Release, 800 blocks of 512 at 44.1 kHz (9.288 s), editor **never created**:

| arm | feedback sends | **largest send** | `display-state capture` | audio |
|---|---|---|---|---|
| `--pump` | 569 | **337 bytes** | `#200 (65548 bytes)` | −inf dBFS |
| `--no-pump` | 569 | **337 bytes** | `#200 (65548 bytes)` | −inf dBFS |
| `--no-preset` (negative control) | **0** | — | **none** | `TIDE: unprepared - writing silence` |
| `--pump` on **`e83-vcv-scope-cabled.xml`** (added after #581 merged) | 569 | **337 bytes** | `#200 (65548 bytes)` | −inf dBFS |

The DSP captured a 65,548-byte picture two hundred times and the queue carried at most 337 bytes. `0 held back` on every send, so nothing is stuck in `drainRackFeedback`'s reassembly scratch either — the blob is not entering `queDspToUi` at all, which is exactly what E80 says happens on linux.

**Three things each arm settles, and none of them is the headline:**

- **`--no-preset` is what makes the other two mean anything.** No `state->load` ⇒ no rack, no capture, no send. So every line in the other arms came from the document under test rather than from something the plug-in does anyway.
- **`--pump` and `--no-pump` are identical to the byte.** A bare host's first suspect is that it starved the timer the plug-in's controller ticks on — that is the trap `e79`'s two arms were built for. It is not that.
- **−inf dBFS is correct here and would be alarming elsewhere.** `e75-vcv-visible-rack.xml` is an LFO, a Scope and two CV utilities with no path to an audio output. The evidence the document arrived is `TIDE: instance #1 building rack from 38661 byte document`, not the peak.

### The old number could not have been right, and the new one can

**`TIDE: instance #N feedback send #M` prints sends #0, #1, #2 and every 100th.** Three runs across two platforms have quoted *"never more than 200 bytes"* off that cadence. A blob sent **once** among ~570 sends has about a **2% chance** of landing on a sample — so none of those runs could distinguish *"it never crossed"* from *"it crossed while the trace was looking away"*, and that is the whole question the row asks.

`SynthEditSem/SynthEdit.cpp` now reads **`TIDE_FEEDBACK_TRACE_EVERY`**; `=1` prints every send. Unset, unparseable or `< 1` keeps the original cadence **exactly**, because the linux and macOS cells quote figures read off it. With it armed, all 569 sends are printed and the largest really is 337 bytes.

**The general habit: before quoting a counter, check its print cadence.** A sampled trace answers *"is this healthy"* and cannot answer *"did this ever happen"*, and the two look identical in a log.

### The confound, stated plainly, because it is why this is not a diagnosis

**This probe creates no editor. E80's linux measurement had one** — its `RackEditor: light #3300 value 0.754` lines say so. So the symptom is shown under a **weaker** condition than the row was filed under, and **a Windows CLAP with an editor open is still unmeasured.**

What makes the weaker condition worth having anyway is where the counter sits: `drainRackFeedback` reads TIDE's own inner `queDspToUi`, written by the inner rack's `SynthRuntime` and read by TIDE itself — **no editor on either end of it.** The editor only appears downstream, on `pinFeedback`. So the quantity E80 names is genuinely observable without one; what is not observable is the far end, `RackEditor: display-state update #N arrived`.

**The obvious lead is refuted, and E83's own fixture is what refuted it.** `ControlPin::setValue` (GMPI `Core/Processor.h`) still dedups by value — `if(value != value_)` — and **E83 measured the Scope's display-state payload to be constant**, because in `e75-vcv-visible-rack.xml` the Scope's input is not connected in the DSP half of the document at all. A constant picture would ship once and never again, which would explain everything above without any defect in the channel.

**It does not survive the control.** [#581](https://github.com/JeffMcClintock/TideSynth/pull/581) landed [`tests/fixtures/e83-vcv-scope-cabled.xml`](tests/fixtures/e83-vcv-scope-cabled.xml) — the same rack with the DSP cable list synced to the editor's — and re-running this probe against it while resolving this PR's merge conflict gives `RackProcessor: 'Scope' connections ins=100` and `'Scope' first NONZERO INPUT pin 0 (1.000000)`, so the payload genuinely varies. **569 sends, largest 337 bytes, capture `#200 (65548 bytes)` — identical to the uncabled arm in every figure.**

So **the blob fails to cross whether or not its contents change**, and E83 is not this row's cause. That also disposes of the objection E83 would otherwise raise against the first three arms, which all used the stale `e75` fixture: the cabled control says the fixture was never the variable. GMPI [`09f0221`](https://github.com/JeffMcClintock/GMPI/commit/09f0221) removed the same dedup one layer up, in `gmpi_processor::setPin`'s blob arm — *"a blob output parameter is a STREAM, not a value"* — and the pin-level compare was not changed with it; that remains a real inconsistency and is simply not what bites here.

**And E83's run is a second, independent measurement of the same cap.** It re-measured it on macOS through `tests/e79_clap_headless_probe.c` — *"identical feedback traffic in both arms, max send 200 bytes"* — so the CLAP cap now stands on **three platforms and two separate probes**, and the one thing none of them has is an editor.

### The developer was at the machine, and nothing in the process looks

This is the finding this lane should keep.

`Get-Process | Where-Object { $_.MainWindowTitle }` returned **two Visual Studio instances open on `modules/TiDEknob/TiDEknobGui.cpp`**, `SynthEdit2` running a document, `cmake-gui` on `TideSynth/modules/build`, Chrome, Outlook and Slack. **That file changed underneath this run twice**, at 08:06 and 08:11 — and the first build failed with

```
C:\SE\GMPI\Core\Common.h(80,42): error C2259: 'TiDEknobGui': cannot instantiate abstract class
```

on a half-saved state where `: public gmpi::api::IDrawingLayer` had been typed and its `addRef`/`release` overrides had not. **The identical command succeeded eight minutes later with zero errors.** A run that had not looked would have filed a `platform:win` build break against `main` that does not exist — CI is green at `0ed6ca1db` on all three platforms.

**So this run did not launch REAPER.** The `%APPDATA%\REAPER` backup was taken first, per the standing rule that the host is not isolatable on this platform, and then **restored unused and verified md5-identical across all 2,385 files**. `clappath` had been set and `reaper-clap-win64.ini` moved aside in preparation; both were undone before anything ran.

**`Get-Process | Where-Object { $_.MainWindowTitle }` is this platform's answer to the mac lane's `CGSSessionScreenIsLocked`, and it is strictly more informative.** The mac check says whether a human *could* be there; this one says which applications they have open and on which file. **A locked screen is not the only reason for a scheduled run to stay off the GUI — an unlocked one with the developer mid-edit is a stronger one**, and this box had no check for it at all until now.

**The corollary, and it is uncomfortable:** the binary these numbers came from was built from a tree carrying one uncommitted developer edit. It is a knob's drawing-layer override and cannot touch the rack-feedback channel, so the measurement stands — but the honest statement is *"stands despite it"*, not *"was unaffected"*, and a run that needs a pristine tree on this box should build from a clean export rather than from `C:\SE\TideSynth`.

### What is in the tree now

| file | what |
|---|---|
| [tests/e80_clap_feedback_probe.c](tests/e80_clap_feedback_probe.c) | **new** — the first bare CLAP host in this repo that builds on Windows. `e69`, `e78` and `e79`'s probes are all `#include <dlfcn.h>`; this one has `LoadLibrary`/`GetProcAddress` behind a two-line macro and builds on all three platforms. Three arms, ~20 s, no DAW and no window. |
| `SynthEditSem/SynthEdit.cpp` | `TIDE_FEEDBACK_TRACE_EVERY`, default-preserving |
| `tests/e19-host-feedback/{prepare,measure}-clap.lua` | log `fx_ident` — the 2026-09-02 wrong-bundle trap applies to `clappath` exactly as it does to `vstpath64`, and the CLAP drivers were the only ones without the line |
| [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md) | the recipe, the three-arm table, and the staging note |

**The CLAP REAPER harness that was already in the tree is ready and was not used.** `prepare-clap.lua`, `measure-clap.lua` and `frame_clap_chunk.py` were written on linux for E78, where `TrackFX_Show` killed REAPER inside its own GTK before `guiSetParent`. Nothing about them is linux-specific; whoever gets an idle Windows box can run them as they stand.

**Learned:**

- **Check whether a human is using the machine before taking the GUI, and on Windows that is one command.** `Get-Process | Where-Object { $_.MainWindowTitle }`. The mac lane has checked `CGSSessionScreenIsLocked` since 2026-08-29 and this box has never checked anything; it drove REAPER on 2026-09-02 and got away with it.
- **A sampled counter cannot answer an existence question.** Every-100th is right for *"is the channel healthy"* and useless for *"did the blob ever cross"* — and the two questions read the same in a log. Make the cadence settable and default it to what the existing quotes were read off.
- **The first incremental build after the source tree has moved can fail spuriously with the VS generator**, because `ZERO_CHECK` re-runs CMake while other projects are already compiling. Re-run the identical command before believing a compile error — but read the error first, because here it was a *real* error about a *transient* file state, and both facts mattered.
- **A row written off as another platform's may not be.** E80 was filed from linux and three consecutive `win` cells classified it as *"linux in substance"*. Its `Plat` is `any`, its own text says the blocker is that REAPER 7.43 dies in GTK, and its step one is a second opinion — which is the one thing only another platform can give. This is the mac lane's Accept/question split, arriving on this lane for the first time.

**Not verified:** **a Windows CLAP with an editor open** — the one arm that would make this a diagnosis rather than a reproduction, and the next thing to do on this row. **Anything at the far end of the channel** — no `RackEditor: display-state update` line exists in any arm here, because no editor exists; this entry's numbers are the DSP side only. **Whether the blob crosses on the Windows VST3 with no editor** — the 65,673-byte VST3 figure quoted anywhere in this repo was measured with an editor open, in REAPER, on 2026-09-02, so it differs from these arms in *two* variables and isolates nothing on its own. **macOS and Linux** — nothing here was re-measured there; the probe builds on both and was compiled only on Windows. **Which layer drops the blob** — four arms say it does not arrive and none of them says where. **`SynthEditCL` and SynthEdit proper** — not built; nothing this run changed is outside TideSynth, so neither should be affected, but neither was compiled to say so.

**Machine state.** All six repos were on their default branches at the start; `TideSynth`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI` and `SynthEdit_Rack_Adaptor` all clean, `SE16` already carrying the developer's untracked `UnitTest/Manual Tests/project_specific_resources.resources/samples/` folder, which was not touched. **`TideSynth` did NOT stay clean, and not because of this run:** `modules/TiDEknob/TiDEknobGui.cpp` was edited by the developer at 08:06 and again at 08:11 while this run was building. **It is left exactly as found and is deliberately not on the branch** — `git add` named four paths, never `-A`. Dependency shas the measurement was built against, recorded because the build uses the developer's local checkouts via `*_FOLDER_OVERRIDE`: `SynthEditLib` `72a4227`, `gmpi_ui` `73919ab`, `GMPI` `99eeb85`, `GMPI_Wrappers` `bcb0d3a` (one commit behind `origin/main`'s `4c11d6d`, which is AU3-only), `SynthEdit_Rack_Adaptor` `04d1296`, `VCV_Fundamental_gmpi` `93a27f9`. **No host was launched and no plug-in was installed.** `%APPDATA%\REAPER` was backed up, had `clappath` set and `reaper-clap-win64.ini` moved aside, and was then **restored and verified md5-identical across all 2,385 files** when the developer was found to be at the machine; `reaper.exe` never ran. The staged CLAP, its resources, the probe and every log live in the session scratchpad, outside all repos.

**Next:** **E80 again, and it is one arm** — the same two counters with an editor present. The Windows standalone does it with no DAW and no host isolation, which makes it the cheapest arm anybody has; it decides whether E80 is about CLAP at all or is a rack-feedback question every format shares. **The `ControlPin::setValue` lead is closed** — E83's cabled fixture refutes it, as above. What still wants instrumenting is one trace line at `displayStatePin->setValue` in `SynthEdit_Rack_Adaptor`, to say whether `sendPinUpdate` is called at all or is called and then dropped; that repo is on neither of STEP 5's lists and so is GATED by default, making it a filing rather than an edit. **E82 is the only other row this box could take**, and its own text says to read it first because the answer may be a product ruling. **`JOURNAL.md` is 201 KB against A24's 60 KB ceiling and rotation is still deferred** — [#581](https://github.com/JeffMcClintock/TideSynth/pull/581) is open and macOS's, and rotating from this lane would make it conflict on the hardest file; whoever merges it should rotate. **`build.yml`'s `matrix.platform != 'win'` exclusion (`:523`) still means STEP 1 cannot fire on this platform** — a workflow edit, which the bot's token deliberately cannot make, so it is Jeff's or nobody's, and the `win` cell has now restated it five times.

**Branch/PR:** `tide/win/E80-clap-gui-second-opinion`, [#582](https://github.com/JeffMcClintock/TideSynth/pull/582) — E80 back to TODO with the three-arm measurement, the refreshed `win` NEXT cell, the new `tests/e80_clap_feedback_probe.c`, `TIDE_FEEDBACK_TRACE_EVERY` in `SynthEditSem/SynthEdit.cpp`, `fx_ident` logging in the two CLAP `.lua` drivers, the recipe in `docs/ci/headless-gui-verification.md`, regenerated `docs/lessons.md`, and this entry.

## 2026-09-09 — macos — E83: the Scope was never wired; a TiDE document stores its cabling twice and the two copies disagreed (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.46388.4** (no `claude` CLI on this box's PATH; A13 records the app's `CFBundleShortVersionString` as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required

**Did:** took **E83** and **answered it with no GUI at all**, on a locked screen, which is the fourth row running this lane has recovered by separating what a row ASKS from what its Accept asks. New [scripts/patch-cables.py](scripts/patch-cables.py), new fixture [tests/fixtures/e83-vcv-scope-cabled.xml](tests/fixtures/e83-vcv-scope-cabled.xml) and [its README](tests/fixtures/e83-vcv-scope-cabled.README.md), one new row (**E84**). **No product code changed, in this repo or any sibling.** Branch `tide/mac/E83-scope-input`.

### The answer, and it is the clause the row thought less likely

E83 asked *"whether the Scope's INPUT is constant or its capture is"*. **The input is not merely constant. It is not connected at all**, and the capture has been correct the entire time.

**A TiDE document stores its rack cabling TWICE.** `HC_PATCH_CABLES` — host control **49**, counted in `SynthEditLib/HostControls.h` rather than guessed — appears once in the `<DSP>` half as `<Parameter HostControl="49">` and once in the `<Editor>` half as `<param hostControl="49">`, each holding its own base64'd `<Cables>` list. **The panel draws from the editor's copy; the audio graph is built from the DSP's.**

| half | `e75-vcv-visible-rack.xml` |
|---|---|
| Editor | `Pulses->SHASR`, **`LFO->Scope`** |
| DSP | `Pulses->SHASR` |

So the orange cable that three E19 runs and E75 all saw on screen was **drawn and carried nothing**.

### Measured twice, and the control is inside the same trace

The document reading is one command ([scripts/patch-cables.py](scripts/patch-cables.py) `--show`). The one that settles it is the DSP's own trace, through `tests/e79_clap_headless_probe.c` against a `TIDE_VCV_FUNDAMENTAL=ON -DRACK_ADAPTOR_TRACE=1` CLAP — **one document line of 798 apart:**

| | `e75-vcv-visible-rack` | `e83-vcv-scope-cabled` |
|---|---|---|
| `'LFO' connections` | `outs=0000` | **`outs=1000`** |
| `'Scope' connections` | `ins=000` | **`ins=100`** |
| `'Scope' first NONZERO INPUT` | *never printed* | **`pin 0 (1.000000)`** |
| `'Pulses' outs` / `'SHASR' ins` (**the control**) | `…1000` / `…1…` | **identical** |

**The last row is what makes the other three a fact about the cable rather than about the harness.** `Pulses` pin 16 → `SHASR` pin 4 is the cable that IS in both halves, and it connects in both arms. Without it, "I changed the document and the connections changed" is also consistent with a probe that connects whatever it is given.

### Why the payload was a perfect constant — explained, not inferred

From `vcv/Scope.cpp`, with `params: 0 0 0 0 0 0 0 0` read off the same trace:

- `X_INPUT` unconnected → `getChannels()` is **0** → the per-channel loops never execute → `currentPoint` stays at its `{INFINITY, -INFINITY}` default, and all 8,192 `pointBuffer` entries are written with it.
- `TRIG_PARAM = 0` means **trigger ENABLED**, and the trigger loop runs over `trigChannels = 0`, so nothing ever re-triggers and **`bufferIndex` freezes at `BUFFER_SIZE`**.

All three captured members constant, so the 65,548-byte payload is constant — which is the previous run's **324 of 327 applies carrying one checksum, `sum=19140`**, arrived at from the other end.

### The fixture is stale, the product is not, and that is measured too

`e53-vcv-rack-segv.xml` — which `e75` is two view fields away from — is a **session file TiDE itself wrote on 2026-08-26**, before E68's 2026-08-31 ruling made a cable edit push the document. So this is not hand-authored damage.

**Loading the disagreeing `e75` into a build of current `main` and saving produces halves that AGREE**, the LFO→Scope cable present in both (`tests/e69_clap_state_probe.c`, 51,694 in / 57,697 out). So no run can still provoke this through that path — **and every document committed before 2026-08-31 is suspect.** Surveyed: of twelve committed documents, exactly **two** disagree, and they are `e53` and its descendant `e75`. The other ten pass, including `DefaultRack.synthedit` and all five prefabs.

### Verification

| check | result |
|---|---|
| build, `TIDE_Rack_CLAP`, Release/arm64 | rc=**0**, `[61/61]`, **0** `error:` |
| A/B, e75 vs e83, same binary | `Scope ins=000` → `ins=100`; `first NONZERO INPUT` absent → `pin 0 (1.000000)` |
| the A/B's own control | `Pulses`/`SHASR` connection strings **identical** in both arms |
| decoded-document diff, e75 vs e83 | **1 hunk, 1 line** of 798 |
| fixture regenerates from the command | `--sync-dsp` output **byte-identical** to the committed file (md5 `80d7b858…`) |
| `--show` over all 12 committed documents | 10× rc=0, 2× rc=1 (`e53`, `e75`), 0× rc=2 |
| `--sync-dsp` refusals, both seen to fire | no DSP slot → rc=2 and **no file written**; no `-o` → argparse error |
| E80's CLAP cap, re-measured | max feedback send **200 bytes**, identical in both arms |
| all seven lints | **rc=0 each**, reproduced locally with [lint.yml](.github/workflows/lint.yml)'s own arguments -- base from `git show origin/main:`, `--changed-file` from the diff. `check-backlog-diff`: `E83: TODO -> IN-REVIEW`, `1 new row(s): E84`, status/date cells and new rows only |
| `check-commit-authorship --repo .` | rc=0 -- every unpushed commit `tide-rack-bot` |
| `check-commit-completeness --record/--verify` | 7 staged, 7 in HEAD, all present |
| `check-no-direct-commits --repo .` | rc=0 -- every `tide-rack-bot` commit on `main` arrived as a merge |
| CI on the pushed head | **6 pass, 0 fail** -- `lint`, `linux`, `e57-delete-key`, `render-linux`, `render-macos`, `render-windows`; `guard`/`matrix.name` **skipped**, correctly, because this branch touches no compiled source. `mergeStateStatus: CLEAN` |
| NEXT-cell chain before/after | 8 → **9** generations; pipe count 4 |

**NO macOS COMPILE RAN IN CI, and that is correct rather than a gap:** `guard` skips the build matrix because this branch changes no compiled source at all. The build evidence is local -- `TIDE_Rack_CLAP`, `[61/61]`, rc=0. **No product code was built into anything shipped** — this branch touches `scripts/`, `tests/` and the three bookkeeping files only. **`SynthEditCL` is discharged by SCOPE, stated rather than glossed:** no sibling repo was edited, and `SE16` is not on this box.

**Learned:**

- **When a picture never changes, ask whether the thing feeding it is connected before you ask whether the capture works.** E83 was filed as a display-state question and spent its whole life there; the input was the free variable and nobody had read it. One decode of the document answered it.
- **A document that stores the same fact twice will eventually store it two ways, and nothing here was checking.** The editor half and the DSP half of `HC_PATCH_CABLES` are written by different code and read by different code; the panel and the audio graph are the two things a user compares, and they were the two things that disagreed.
- **A constructor default in a saved file is indistinguishable from a decision — and so is a MISSING list entry.** E75 landed exactly this lesson about `PanelLocationCenter` two days ago. The same fixture was carrying a second instance of it, one field along, and the run that found the first did not think to look for the second.
- **The Accept/question split has now paid four times running on this lane** (E77, E71, E75, E83). E83's Accept named a display measurement; its question was a property of a file. **It should be the first thing tried, not the last.**
- **Put the invariant in the same trace as the variable.** The Pulses→SHASR cable was in both halves and connected in both arms, so it is a control that costs nothing and was already being printed. A 0→1 with no invariant beside it is a claim about the instrument.
- **A probe that cannot see the thing you are measuring should be said so, not worked around.** CLAP caps the feedback payload at 200 bytes (E80), so no CLAP run can ever show a display-state blob changing. That is a limit of the instrument and belongs in the write-up, not in a hedge.
- **`rc` from a pipeline is the last command's.** My first fixture survey printed `rc=0` for every file because `$?` was `tail`'s. The fleet already has this lesson twice from `grep -c`; it is the same mistake with a different last command, and it made a discriminating check look useless.
- **A check that fails on the normal case is not a check.** `--show` first reported "no HC_PATCH_CABLES parameter" as an error, which is the state of `DefaultRack.synthedit` and all five prefabs — i.e. it failed on the shipped rack. A half with no cable parameter has no cables; fixing that is what made it usable as E84.

**Not verified:** **that the Scope's picture ANIMATES with the new fixture** — the 65,548-byte payload reaches the editor on VST3 and the standalone but not on CLAP (**E80**, re-measured here), and the standalone wants a window this locked-screen run did not have. That is **E19**'s clause, and it is now GUI-blocked only, not fixture-blocked. **That the DSP half governs the graph in a build WITH an editor** — measured on a headless CLAP; the rack-building path is the same source, and "same source" is a reading. **Why the 2026-08-26 save produced disagreeing halves** — the round-trip shows current `main` does not, and the mechanism that did is not established. **Whether any document outside this repo is affected.** **Windows and Linux**, where nothing was built or run. **`e82`**, untouched.

**Machine state.** All six local repos were clean and on their default branches at the start; **`SE16` is not on this box**. **No sibling repo was committed to, modified or fast-forwarded** — `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI` and `SynthEdit` were read and used as build overrides, never written, and `SynthEdit_Rack_Adaptor` (GATED by default, on neither STEP 5 list) was **read only**. TideSynth's `main` was fast-forwarded to `9851b0d` before the branch was cut; TideSynth is on `tide/mac/E83-scope-input` until STEP 5 returns it. **Nothing was installed, registered or launched with a window**: every build ran `SE_LOCAL_BUILD=OFF`, `~/Library/Audio/Plug-Ins` was not touched, no AUv3 was registered, and **no DAW, standalone or appex was launched** — the two probes are bare C hosts that `dlopen` the CLAP out of a scratch build tree. **0 TIDE processes running**, checked. The screen was **locked throughout and no GUI was attempted**. `build-e75/` is the 2026-09-07 run's gitignored scratch tree, reused warm and left with `TIDE_Rack_CLAP` added; every probe binary and artefact is in the session scratchpad, outside every repo.

**Next:** **`JOURNAL.md` is ~200 KB against A24's 60 KB target with 18 entries and a floor of 4, and NOTHING IS OPEN IN ANY REPO** — three previous cells deferred the rotation saying it would be cheap once the queue was clear. **The window is AFTER [#581](https://github.com/JeffMcClintock/TideSynth/pull/581) merges, not now** -- rotating on a second branch while this PR is open recreates the hard `JOURNAL.md` conflict they were avoiding. Merge #581, then rotate, while the queue is still empty. **E19's pixel-diff clause is no longer fixture-blocked** — `e83-vcv-scope-cabled.xml` is the fixture it wanted; it needs one standalone launch or one hosted VST3 session on an unlocked screen, which would also close E83's own unverified half. **E84 needs Jeff or an interactive session** — it is a `lint.yml` edit and the bot's token deliberately has no `workflow` scope. **Read `./scripts/patch-cables.py <fixture> --show` before trusting any committed fixture**, and note **E82** is the remaining E19 clause and may be a product ruling rather than a defect.

**Branch/PR:** `tide/mac/E83-scope-input` — [scripts/patch-cables.py](scripts/patch-cables.py), [tests/fixtures/e83-vcv-scope-cabled.xml](tests/fixtures/e83-vcv-scope-cabled.xml) and [its README](tests/fixtures/e83-vcv-scope-cabled.README.md), E83 → IN-REVIEW with its answer, E84 filed, the refreshed `mac` NEXT cell, and this entry.

## 2026-09-08 — windows — the merge sweep: three PRs, and the fleet is empty again (interactive continuation, Jeff directing)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.46388.4.0** · as **tide-rack-bot** (both paths) · interactive continuation of the scheduled run below, Jeff directing (*"merge any PRs"*)

**Did:** merged every open PR in the fleet — **three, all in TideSynth** — resolving two conflicts on the way, then flipped **E75** DONE and archived it, **A35** back to TODO and **E7** to DONE-PENDING-ACCEPT. No product code was written by this entry; the code it landed belongs to the two entries below it and to the macOS box.

### What landed, in order

| PR | what | merged as |
|---|---|---|
| [#577](https://github.com/JeffMcClintock/TideSynth/pull/577) | **E75** — the visible-rack fixture; **E82** and **E83** filed | `7aebd640e` |
| [#578](https://github.com/JeffMcClintock/TideSynth/pull/578) | **A35** — the `Plat` column measured, two `PROPOSED:` entries filed | `3f08ea66b` |
| [#579](https://github.com/JeffMcClintock/TideSynth/pull/579) | **E7** — answered already; **P8** DONE and archived; the `extract-lessons.py` CRLF fix | `0ed6ca1db` |

**Oldest first, and it cost the minimum.** Two conflict resolutions for three PRs, both in the bookkeeping files and neither in product code — against the 2026-09-07 sweep's four PRs and *five* resolutions. That is the O(N²) the previous entry described, seen from the cheap end: the fix really is not letting them accumulate.

**Squash merges, which is this repo's convention** — every commit on `main`'s first-parent chain has one parent and a `(#N)` suffix. Worth stating because `check-no-direct-commits.py` passes on it: the squash commit's *committer* is `GitHub <noreply@github.com>`, not the bot, so an agent-authored commit still arrives as something a human merge button produced.

### The two resolutions, and both were the predicted shape

Set arithmetic over entry headings first, every time — which of the branch's headings appear in **neither** of `main`'s two journal files:

| PR | which side had rotated | `JOURNAL.md` resolved by |
|---|---|---|
| #578 | **neither** (419 archived entries on both sides) | main whole; the branch's **one** unique entry (09-08 macos) inserted at the top |
| #579 | **neither** (419 both) | main whole; the branch's **one** unique entry (09-08 windows) inserted at the top |

**Two entries share the date 2026-09-08 and the tie was broken by commit time, not by guessing.** The macOS A35 run committed 02:07–02:14 NZ; the windows E7 run committed 10:16–10:34. So windows sits above macos, which is "newest at the top" meaning what it says.

`docs/lessons.md` was **regenerated** on both, never merged. `BACKLOG.md` was resolved by ownership, and the NEXT block was the only hunk that conflicted in either:

- **#578** — splice: the branch's 09-08 `mac` head onto main's 09-07 `mac` cell entire. Chain **7 → 8** generations, `09-08 → 09-07 → 09-06 → 09-05 → 09-01 → 08-31 → 08-31 → 08-28`. #578's own body had predicted this in writing and named the remedy; it was right.
- **#579** — no splice needed: `win` from the branch, `mac` from main, because each side had re-pointed a different platform's cell. Verified by chain length rather than by eye — `re.findall(r'RE-POINTED (\d{4}-\d{2}-\d{2})')` on both cells before and after.

**One thing was NOT a conflict and still had to be edited: a cell that had explained why it could not cite a row.** #579's `win` cell said E19's two open clauses *"are two rows filed on #577… their IDs are deliberately NOT written here, because `check-id-refs.py` draws its known-ID set from the two backlog files."* True when written, false after #577 merged — and no lint fires on a citation that is merely *missing*. Replaced with **E82** and **E83** by name, plus what each says. **A merge can make a correct sentence wrong without making any file conflict**, and nothing looks for that.

### The `extract-lessons.py` CRLF fix, seen from both sides in one session

#579 carries `newline=""` on the one `write_text` call. The evidence that it was the right fix is in this sweep rather than in that PR: resolving **#578**, whose branch predates the change, `--write` produced **2,550 CRLFs** and had to be normalised by hand; resolving **#579**, which carries it, the same command produced **0**. Same repo, same machine, twenty minutes apart.

### The three flips, and only one of them is DONE

| row | to | why not something else |
|---|---|---|
| **E75** | **DONE**, archived | its Accept is met as written — the fixture is on `main` and opens on its VCV modules |
| **A35** | **TODO** | its PR merged, so `IN-REVIEW` is false; its Accept (a working narrowing exception plus tests) was **deliberately not shipped**, so `DONE` is false too. The **X2 shape**, and it now parks itself on its own two open `PROPOSED:` entries |
| **E7** | **DONE-PENDING-ACCEPT** | its Accept was **retired, not met** — the fixture records a ruled-out construction. Whether that closes a row is Jeff's call, not a run's, and it is the same status E44 and E49 carry for the same reason |

**Learned:**

- **A merge can falsify a sentence without touching a line of it.** #579's `win` cell explained why it could not name two rows; #577 landed them ten minutes later and the explanation became wrong in a file that merged cleanly. Conflicts are found by git; **claims about what does not exist yet are not**, and a cross-PR sweep is exactly when they rot. Grep your own outgoing prose for "not on `main` yet" and "does not exist" before merging past it.
- **Same-date journal entries need a clock, and the commits carry one.** Two 2026-09-08 entries from two boxes; `git log --format=%ad` on each branch settles the order in one command, and eyeballing the dates cannot.
- **A fix's evidence can come from the merge that follows it.** The CRLF change produced 2,550 → 0 across two branches in one session, one with the fix and one without, on the same machine — a better control than the PR that made it could construct for itself.
- **Three PRs cost two resolutions; four cost five.** The previous sweep's O(N²) claim now has a second data point at the small end, and it points the same way: merge sooner rather than order better.
- **`IN-REVIEW` has two exits, not one.** A merged PR does not mean a met Accept. A35 went back to TODO and E7 to DONE-PENDING-ACCEPT, and both would have been a lie as `DONE` — which is a status the backlog cannot walk back once the row is archived.

**Not verified:** **anything about the merged code's behaviour** — this entry ran no build, no probe and no host, and every measurement it cites belongs to the entry that made it. **`main`'s own `build` run** for `0ed6ca1db`, not read. **That E82 and E83 reproduce on Windows** — they are macOS measurements taken on #577 and are quoted, not re-run here. **A35's `PROPOSED:` entries** — filed, unread by Jeff, and unanswered.

**Machine state.** All six repos on their default branches, clean, and `TideSynth` fast-forwarded to `0ed6ca1db` before this branch was cut. **No `tide/*` branch remains in any of the six repos and there are no open PRs in any of them** — the third time the fleet has been in that state. `SE16` keeps the two dirty entries it had at the start of the scheduled run; nothing was built, launched or installed by this continuation, and no REAPER or TIDE process was started.

**Next:** **E19's win VST3 cell is the obvious next windows pick and it is no longer fixture-blocked** — `tests/fixtures/e75-vcv-visible-rack.xml` is on `main`, and the two clauses now sit on **E82** (no context-menu producer on any platform) and **E83** (the display-state payload never changes, so the pixel diff is 0). Read both before re-taking it: neither is obviously a win-lane job. **`JOURNAL.md` is 159 KB against A24's 60 KB ceiling and NOTHING IS OPEN, so the rotation this lane deferred twice is now free** — that is the single cheapest thing the next run can do. **A35, E72, E81 and S8 all want a ruling rather than a session**, which is four of the eleven remaining `TODO` rows.

**Branch/PR:** `tide/win/post-merge-sweep` — E75 flipped DONE and archived, A35 back to TODO, E7 to DONE-PENDING-ACCEPT, and this entry.
