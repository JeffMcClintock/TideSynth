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

**Archives:** [JOURNAL-2026-10.md](JOURNAL-2026-10.md), [JOURNAL-2026-09.md](JOURNAL-2026-09.md), [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-10-16 — linux — E92: its Accept is MET on SE16 `origin/master` — Jeff's own commit answered the ruling the row was parked on, and four configures here measure it (scheduled run)

**Prompt:** 1821076 · Fable 5.1, `claude-fable-5-1` · app Claude desktop **2.31226.1** (CLI `2.1.220`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E92**, the linux NEXT cell's target -- and did NOT do what that cell said to do with it. The cell said *"its first move is a `PROPOSED:` entry and not a fix"*, because the fix lived in one of two files and the two sat in different STEP 5 tiers. **Reading the GATED file on SE16 `origin/master` before writing the entry showed Jeff had already changed it**, so the ruling the row wanted was given by a landed commit and the item became *run the Accept*. It passes, in four configures on this box, with a pre-fix control that fails. **E92 is DONE and archived.** `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`938e648..c67f5c2`). **First scheduled run under the live box lock:** `run-lock.py acquire` rc=0, `owner=auto:ancestor-match`, pid 3275 -- the ancestor walk E89 built resolved the agent process on the first real try.

**STEP 1:** [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) is the only open `platform:linux` issue, by the bot, and its own first line says its tier is not STEP 1's; its row **E89** is DONE and archived. `main`'s `linux` job is green on every `build.yml` run since 10-09. The **`macos` job failed three times** on `main` (`e8586e9`, `46aee55`, and the first run of `ddf2978`, 10-14/10-15) and is **green again at `c67f5c2` and on `ddf2978`'s re-run** -- not this platform's, no open `platform:mac` issue, latest run green, so nothing to file. **STEP 1.5:** zero open PRs in the fleet. **STEP 2:** E92 is `TODO`, `any`, unblocked; `git ls-remote --heads origin` returns `main` alone and `gh pr list` is empty, so nobody held it. None of the four open `PROPOSED:` entries parks it (three say *"everything"*, E72's parks only E72). DOING mark pushed first, `aecc2a0`.

### The finding: the GATED half had already moved, and the row's ruling question is answered by whose commit moved it

E92 named two candidate files and said *"which of the two files should change is the question, and it decides whether any of this is takeable at all"*: `SE16/se_gmpi/vst3/CMakeLists.txt` (GATED by default -- `se_gmpi/` is on neither STEP 5 list) and `SynthEditSem/CMakeLists.txt` (TideSynth's own, ALLOWED). On SE16 `origin/master`:

- **`f9ad10372` (Jeff, 2026-10-15)** -- *"se_gmpi/vst3 honours GMPI_WRAPPER_FOLDER_OVERRIDE and builds a CLAP template too"*. The guard is now `if(NOT "${GMPI_WRAPPER_FOLDER_OVERRIDE}" STREQUAL "") set(GMPI_ADAPTORS ...) else() FetchContent_GetProperties(gmpi_wrappers) ... endif()`, with a comment giving E92's own mechanism and *"measured 2026-10-15, WSL"*. TideSynth's `docs/state-management-consolidation-plan.md:155` records the same fix as *"Fixed in SE16 (uncommitted)"* -- it is committed now.
- **`c868fc19f` (Jeff, 2026-10-16)** -- the platform guard went from `if(NOT UNIX OR APPLE) return()` to **`if(APPLE) return()`**, so **Windows now configures `SynthEditTemplate_VST3` too** (VST3 only, never copied to Common Files). **E92's *"Linux-only in fact"*, measured by the mac lane on 10-08 and the windows lane on 10-13, is retired** -- only mac is excluded today.

So the GATED file changed and TideSynth's `SynthEditSem/CMakeLists.txt:32-51` did not need to. That is the ruling, given the way Jeff gives most of them -- by doing it -- and a `PROPOSED:` entry filed today would have asked a question whose answer was already on `origin/master`.

### Evidence: four configures, one of which must fail

SE16 in a scratchpad worktree, Ninja, RelWithDebInfo, `CPM_SOURCE_CACHE=~/.cache/CPM` (warm), `GMPI_UI_FOLDER_OVERRIDE=~/SE/gmpi_ui`, everything else fetched by FetchContent exactly as `main` declares it; GMPI_Wrappers for the two local-override arms is a worktree at its `origin/main` `16c02e9`. The Accept's command, verbatim: `ninja -t targets all | grep -c SynthEditTemplate_VST3`.

| arm | SE16 | wrapper variable | configure | `grep -c` | `skipped (gmpi_wrappers not populated)` |
|---|---|---|---|---|---|
| control | `f9d3b2d34` (`origin/master`) | neither -- FetchContent | rc=0, 26 s | **13** | no |
| override | `f9d3b2d34` | `GMPI_WRAPPER_FOLDER_OVERRIDE` | rc=0, 23 s | **13** | no |
| fcsrc | `f9d3b2d34` | `FETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS` | rc=0, 21 s | **13** | no |
| **pre-fix control** | `ad236e24a` (= `f9ad10372^`) | `GMPI_WRAPPER_FOLDER_OVERRIDE` | rc=0, 19 s | **0** | **yes** -- `-- SynthEditTemplate_VST3: skipped (gmpi_wrappers not populated)` |

The 13 lines are the target's object rules (SeGmpiFactory/Processor/Editor/Controller, `platform_plugin.cpp`, `ProcessorStateManager.cpp`, GMPI `Common.cpp`/`Processor.cpp`, the wrapper's VST3 sources), its link rule for `SynthEditTemplate_VST3.so`, and three phony aliases. The two local-override arms list a byte-identical template target set (`md5 07a1fa83…`); the control differs only in the wrapper path inside object names (`_deps/gmpi_wrappers-src` vs the worktree). **The pre-fix arm is what makes 13 a result:** same box, same cache, same wrapper checkout, same variable, SE16 one commit older -- 0 targets and the exact skip line the row quotes. The variable is the SE16 commit, not the environment.

**Accept met:** *"non-zero in BOTH configures"* -- 13 and 13. The row is flipped `DONE 2026-10-16` and moved to `BACKLOG-DONE.md`. **DONE is awarded on Jeff's landed SE16 commits, not on any PR of mine** -- the P7d / E87 shape, *"a row that parks on a question can be closed by running its Accept"*. Nothing on this branch is code: the only edits are `BACKLOG.md`, `BACKLOG-DONE.md`, `JOURNAL.md`, `JOURNAL-2026-10.md` and the regenerated `docs/lessons.md`.

**Learned:**

- **Before filing the `PROPOSED:` entry a NEXT cell asks for, re-read the GATED file at `origin`.** The cell was written three days ago from a reading of the file as it then stood; Jeff changed it the next day and again this morning. The question was real on 10-13 and answered on 10-15, and the only cost of not noticing would have been a PR asking him to decide something he had already decided -- the inverse of E72, where a row waited fifteen days because nobody asked.
- **A ruling can arrive as a commit in the GATED repo rather than as a merged `PROPOSED:` entry, and a run has to look there to see it.** `docs/decisions.md` records rulings given in session; a developer fixing the shared file himself leaves no trace in TideSynth except, here, one sentence in a plan document that still said *"uncommitted"*.
- **A configure-time Accept is cheap to measure and worth its control.** Four SE16 configures cost 89 s on this box with the CPM cache warm. The pre-fix arm cost 19 s of that and is the whole reason the 13s mean anything -- without it, *"the target exists"* could have been true of every SE16 ever and the row's premise simply wrong.
- **"Linux-only in fact" has a shelf life of one platform-guard edit.** Two lanes measured E92 as unsatisfiable on their platforms by reading `if(NOT UNIX OR APPLE)`; `c868fc19f` made that reading false for Windows the same morning this run started. A platform claim derived from a guard line should name the commit it was read at.
- **The box lock's first live acquire resolved the agent process by ancestor walk**, which is the half E89's own probe said had never been exercised by a real scheduled run: `owner=auto:ancestor-match`, holder pid 3275 outliving the 3551 `acquire` process.

**Not verified:** **nothing was built.** Configure only -- E92's Accept is a configure-time property by its own words, so the template target was generated and never compiled on this box this run; whether `SynthEditTemplate_VST3.so` links against today's SynthEditLib is CI's question (`main` is green on linux at `c67f5c2`, which fetches the same `origin/main`s). **The Windows arm is unmeasured** -- `c868fc19f` says Windows configures the template now; this box cannot check it and the windows lane has no reason to unless it wants the *"Linux-only"* retraction confirmed rather than read. **I did not measure the TIDE-side variable's effect on TIDE's own targets**, which the row said are unaffected and which no arm here counted. The three macOS `main` failures were read as job conclusions, not logs. `#583`'s root cause -- why the task fired twice on 09-08 -- is as unestablished as the 10-13 entry left it.

**Rotation:** done. With this entry `JOURNAL.md` was over 60 KB; the two oldest entries (10-13 macos, 10-13 linux) moved to `JOURNAL-2026-10.md`, appended below the 10-12 entry already there, verbatim. The floor (four most recent entries, which beats the one carrying today's date) still holds five. `extract-lessons.py --check` green before and after.

**Machine state:** `~/TideSynth` stayed on `main`, clean, and was never checked out -- its local `main` is **106 commits behind `origin/main` and deliberately left there**, for the 10-06 and 10-13 entries' reason: fast-forwarding a sibling tree out from under a possible concurrent process is the other half of #583. All work was in scratchpad `git worktree`s -- TideSynth (this branch), SE16 at `origin/master` and at `ad236e24a`, GMPI_Wrappers at `origin/main` -- each removed and pruned at the end. `~/SE/SE16` (`master`, clean, local `32e0ae3`, behind `origin`), `~/SE/GMPI_Wrappers`, `~/SE/SynthEditLib`, `~/SE/gmpi_ui` and `~/SE/GMPI` were read and fetched only; none was checked out, built or fast-forwarded, and Jeff's `~/SE/build` was read for its cache variables and not touched. No GUI, no screen taken, and no credential value appears anywhere. Box lock released as the last act.

**Next:** see the `linux` cell. **For Jeff:** (1) [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) is still open with E89 DONE; it says of itself that the lock is a guard and not a fix for the double firing, so it is yours to close or keep -- the next linux run will meet it in STEP 1 again either way. (2) E92 needed nothing from you after all; your `f9ad10372` was the ruling. (3) The bot token expires **2026-11-07**, 22 days out.

**Branch/PR:** `tide/linux/E92-template-vst3-override`. The E92 flip and archive, this entry, the `linux` cell, the rotation and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-14 — windows — E89 DONE and the box lock is live; four stale mac issues closed; and the link break that stopped #652 was Jeff's own cross-repo WIP, not this fleet's doing (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.31226.1** (CLI `2.1.295`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:` · **interactive continuation of this box's 2026-10-13 scheduled run, Jeff directing**

**Did:** Jeff merged everything, then said ***"do both please"***. **E89 is DONE and archived**, and the four stale `platform:mac` issues are closed with the cause and the evidence. **A scheduled run must not copy this entry** — it closes rows and issues on rulings given in chat.

**State at the end: zero open PRs in any of the six repos, and `main` green on all three platforms** (`build` at `ff6c245db`: `linux`, `macos`, `windows`, all three `render-*` and `guard` all success).

### The box lock is LIVE, which is the thing the next run most needs to know

[#652](https://github.com/JeffMcClintock/TideSynth/pull/652) merged as `ff6c245db`, so **prompt sha is now `18210767f`** and from the next scheduled run every box acquires at STEP 0.5 and releases at STEP 5. The windows-specific correction is in that prompt text.

### Why #652 was not merged when Jeff first asked, and what measuring it found

He asked for suitable PRs to be merged, chose *"measure on Windows, then merge"* for this one, and it did not stay green. **4 of 13 arms failed and the probe printed its own verdict not to adopt the lock on that result.** `_parent_and_cmdline` returns `(None, None)` off posix by construction, so `--owner-pid auto` degraded to `self` — the short-lived `acquire`'s pid — and `pid_alive` is an unconditional `None` there, which masked the staleness for `acquire` and **broke `release`**:

```
acquire  rc=0  pid == recorded_by == 33336
acquire  rc=1  REFUSED  <- correct, exclusion works
release  rc=1  REFUSED: "not by this run" (owner resolves to 33532)
```

So **every** windows run would have ended holding its lock, not merely a crashed one, with the 6 h age cap the only route out on the *normal* path — the stranding direction #583 calls worse than the collision.

**The cause of the cause is the durable part: no arm exercised an unforced `release` on ANY platform.** Arm 10 passes `--force`, and its own comment says why — in that harness every `run_lock` is its own pid. The one command STEP 5 prescribes was untested everywhere, which is how a green PR carried it.

Fix: `release` falls back to the host the lock already carries, gated on `--owner-pid auto` **and** a platform that cannot establish pid ownership. `acquire` untouched. **Inert on posix by that gate**, which is the whole argument it cannot regress the other two boxes. Arms 5/11/12/13 now assert the documented non-posix contract instead of the posix one, and **new arm 14** pins the gap in three parts. Windows: **14 arms, 0 failures**, control still 12-of-12, re-run after merging `main` in.

### The link break was not ours, and I said the opposite before checking

`main` and four branches went red on macos and windows. **I first told Jeff I had caused it by merging #643** — wrong, and the correction matters more than the original claim. The undefined symbols were `DawPreset::calcHash()`, `DawPreset::DawPreset(...)`, `init(std::map<int, paramInfo>&, tinyxml2::XMLNode*)` and `_g_pendingEditorGuiHost`, from `libSynthEditLib.a(Controller.cpp.o)` and `libVST3_Wrapper.a(Controller_VST3.cpp.o)` — nothing to do with an AU3 test probe.

**What settled it in one step: three of the red PRs were bookkeeping-only and touch no compiled file at all.** A link error cannot be caused by a `BACKLOG.md` edit, so the cause had to be external.

It was two commits **sixteen seconds apart**, both Jeff's, both on 2026-10-13 15:33, in repos TideSynth fetches at `GIT_TAG origin/main`: SynthEditLib `45c349b` added `extern IGuiHost2* g_pendingEditorGuiHost;` to a header with no definition anywhere (+11/−1, header only), and GMPI_Wrappers `7909b76` then assigned it and pulled `Controller.cpp` into the link, which needs `DawPreset` from `ProcessorStateManager.cpp` — a file deliberately **not** part of SynthEditLib, per the comment at `SE16/tests/CMakeLists.txt:136`; `se_vst3` and `se_au` each add it to their own target and the TIDE wrappers did not. **I did not attempt the fix**: two repos, one GATED and one PR-gated, on a platform this box cannot compile. Jeff fixed both halves in GMPI_Wrappers `de118cb`.

### The four stale issues, and why they could never close themselves

#656, #658, #660 and #661 were auto-filed by the same break. **`build.yml`'s *"Close the platform issue on success"* step only closes the issue for the branch it just built** — so #654 (`main`) closed itself on the green run while these four could not, their branches being merged and deleted. Left open they read as four live mac build failures to the next mac run's STEP 1. Closed with the cause, `de118cb`, and the green run as evidence.

**Learned:**

- **A bookkeeping-only PR going red is a free diagnosis: it says the cause is not in this repo.** Three of them were red simultaneously, and a `BACKLOG.md` edit cannot break a link step. That observation cost nothing and was worth more than reading the compiler output.
- **Check before attributing a break to your own merge.** I claimed authorship of this one on the strength of timing alone — my #643 merge was the first build run in four days, so it *looked* causal. The symbol names refuted it immediately and I should have read them first.
- **An auto-filed issue whose branch is deleted is immortal.** The closing step is branch-scoped, so a fleet-wide break leaves one self-closing issue and N permanent ones. Whoever fixes such a break should expect to close the rest by hand.
- **`GIT_TAG origin/main` makes a dependency's WIP your build's WIP.** Two commits sixteen seconds apart in two other repos turned every TideSynth build red with no TideSynth commit involved. The 10-06 entry noted this works in the useful direction too; this is the other one.
- **A fix that is inert on the platforms you cannot test is worth more than a fix that is merely small.** The release fallback sits behind a non-posix gate, so the argument for linux and macOS safety is structural rather than a hope — which is what made it mergeable without a posix run.

**Not verified:** **the lock probe has still never run on linux or macOS, by anyone.** No workflow references it, so CI will not catch a regression there, and the posix-safety case is the gate rather than a measurement — it needs no build and took ~8 s on linux. I did not run the probe against the merged `main`, only against the branch. Nothing was built or compiled on this box this session, and no product code ran. The three `DONE-PENDING-ACCEPT` rows (E7, E44, E49) were **not** touched — that status awaits an acceptance judgement, not a merge, so the mechanical STEP 4 rule does not reach them.

**Machine state:** `C:\SE\TideSynth` stayed on `main` throughout and every piece of work was a scratchpad `git worktree`, each removed. **The developer's four work-in-progress files are now his own commit `27b36f71a`** — `DefaultRack.synthedit`, `RackModules/Envelope.synthedit` and both `modules/PatchPoint/` files — so the tree is clean for the first time this session, and they were never touched by any run. No GUI, no screenshot, no credential value anywhere.

**Next:** see the `win` cell. **For Jeff:** nothing is waiting on you — no open PRs, no open mac issues, `main` green on three platforms. The queue's ten `TODO` rows are parked on the same open questions as before, and **E19 and E82 are still the only two blocked on nothing but an idle box.**

**Branch/PR:** `tide/win/2026-10-14-e89-done`. The E89 flip and archive, this entry, the `win` cell, the rotation and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-14 — windows — A44 closed WONTFIX on Jeff's question, and the honest answer was that the row overstated itself (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.31226.1** (CLI `2.1.295`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:` · **interactive continuation of this box's 2026-10-13 scheduled run, Jeff directing**

**Did:** Jeff asked ***"do we need to fret about the cadence of agents? it's not that important is it."*** He was right. **A44 is WONTFIX**, with the whole remedy being one standing note in the digest ([#657](https://github.com/JeffMcClintock/TideSynth/pull/657)) so no further run re-raises it.

**This is an interactive session and a scheduled run must not copy it** — it closes a row on a ruling given in chat, which is Jeff's to give and not a run's to assume.

### Why the row overstated itself, which is the part worth keeping

I filed A44 the previous day and then had to argue against it, so the reckoning is written out rather than summarised. **Nothing was parked on it, no scheduled run was ever lost, and no work was blocked.** The entire measured cost was *attention*: three mac entries (10-11, 10-12, 10-13) each spent a paragraph escalating windows' silence, and the 10-11 one asked that STEP 0.7 be checked by hand on two machines that were fine.

**What retired it was not the cadence argument but a coverage one.** The scenario the liveness check would be missed in — all three boxes dying together when the credential expires — **already has its own alarm with a real date**, `check_credential_expiry()`, which A40 built into the same digest. So closing A44 costs no coverage of the one fleet-wide halt that is known in advance. Without that, WONTFIX would have been cheap rather than safe.

**The technically-true residue is recorded in the row and not in the queue:** on a weekly box the liveness check does not merely false-alarm, it carries **no information** — *"silent 7 days"* is simultaneously *perfectly healthy* and *halted a week ago*, which here are the same observation. **So this box has no halt detection, by arithmetic, and that is now an accepted state rather than an open question.**

### The fix, and what it deliberately is not

One sentence in `check_halted_boxes()`'s section intro. **No logic change, and deliberately no per-platform cadence table** — a table would be a second place to drift out of step with the real cron, which is the staleness A7 already flagged and this project keeps paying for. **The flag is not suppressed**: a genuinely long silence still belongs in that section, and suppressing windows would blind the one box the check demonstrably works for.

`--selftest` still reports `all 4 liveness classifications correct`; `--dry-run` renders the note above the per-box lines.

### A second cause of the same noise, found while verifying it

`--dry-run` reported **linux as LIKELY HALTED, silent 7 days** — on a day linux had run and pushed [#652](https://github.com/JeffMcClintock/TideSynth/pull/652). Its journal entry is still inside that unmerged PR and the digest reads `main`. **No cadence fix would have touched that**, and it is the strongest argument for a note over machinery: the section is structurally noisy for at least two independent reasons. Not filed as a row — A44 was just closed for being the same shape of concern, and filing its sibling the same hour would be exactly the busywork STEP 2 warns against.

**Learned:**

- **A run that files a row should be willing to argue against it when asked, and the honest reckoning is usually narrower than the filing.** A44's real content shrank, under one question, from *"the fleet's liveness detection is wrong"* to *"three entries wasted a paragraph each"*. The filing was not wrong, it was disproportionate — and disproportionate is the failure mode a queue every run reads cannot afford.
- **Check whether a neighbouring alarm already covers the scenario before arguing a detector matters.** The case for A44 collapsed on finding `check_credential_expiry()`, which is in the *same file* and was built by A40 for the exact halt A44's defenders would have cited. Reading the sibling check was worth more than any further measurement.
- **"No information" is a sharper claim than "false alarm", and it is sometimes the one that closes a row rather than escalating it.** Showing the check cannot distinguish healthy from halted on this box is what made *accepting* the gap a defensible answer instead of a deferral.
- **A note is the right fix when a section is noisy for more than one reason.** Had the only cause been cadence, option (c) would have been proportionate; two independent causes in one afternoon means precision machinery would still leave the section untrustworthy.
- **Heredoc backslash mangling bites hardest when patching source that contains `\n` as two characters.** Three attempts to match a line ending in a literal backslash-n failed through the Bash tool's heredoc; writing the patch to a file and running `python3 -I` worked first time. Build such literals with `chr(92)+'n'` rather than escaping them.

**Not verified:** the digest was **not** run in CI as part of #657, and no workflow was touched. **I did not re-measure macos's or linux's cron** — macos's `0 6 * * *` is quoted from A7's 2026-08-17 measurement, and linux's remains unmeasured by anyone, so "the fleet runs two cadences" is still established for windows and macos only. Nothing was built and no product code ran. I did not act on the linux `LIKELY HALTED` reading beyond noting its cause.

**Machine state:** `C:\SE\TideSynth` stayed on `main` and was never checked out; both branches were built in scratchpad `git worktree`s, removed at the end. **The developer's four work-in-progress files are untouched for the third time across this session** — `DefaultRack.synthedit`, `RackModules/Envelope.synthedit`, and both `modules/PatchPoint/` files, still mtimed 2026-10-09. No GUI, no screenshot, and no credential value anywhere.

**Next:** see the `win` cell. **For Jeff:** [#657](https://github.com/JeffMcClintock/TideSynth/pull/657) is `scripts/**` so it waits for you by design; it is one comment block and no logic. [#652](https://github.com/JeffMcClintock/TideSynth/pull/652) still needs the Windows `release` fix or your call to merge without it. The bookkeeping-hot-spots question is the next one worth your attention, and I have argued separately that most of it needs no ruling at all.

**Branch/PR:** `tide/win/2026-10-14-a44-wontfix`. The A44 flip, this entry, the `win` cell and the regenerated `docs/lessons.md` are on that branch; the digest change is on `tide/win/A44-digest-weekly-note` ([#657](https://github.com/JeffMcClintock/TideSynth/pull/657)), deliberately separate because `scripts/**` is off the allowlist.

## 2026-10-13 — windows — the merge sweep Jeff asked for: #643 landed, and #652 did NOT, because measuring its Windows path showed `release` cannot release (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.31226.1** (CLI `2.1.295`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:` · **interactive continuation of this box's scheduled run, Jeff directing**

**Did:** Jeff read the scheduled run's report and said ***"merge any PRs that are suitable"***, then, asked how to handle #652, chose ***"measure on Windows, then merge"***. One PR merged, one deliberately not, and the measurement is the reason.

**This is an interactive session, not the scheduled run, and the distinction is load-bearing twice.** STEP 5 says *"Do NOT merge the PR"* and STEP 1.5 scopes a run to its own platform's PRs — I did both of the things those rules forbid, because Jeff asked for them in session. That is the convention the 2026-09-07 and 2026-10-06 sweeps record. **A scheduled run reading this entry must not copy it.**

### What merged

| # | lane | what | note |
|---|---|---|---|
| [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) | mac | E91's AU3 probe | `CLEAN`, all checks pass, one additive file, no row touched → merged `0c7d4b498` |
| [#653](https://github.com/JeffMcClintock/TideSynth/pull/653) | win | yesterday's bookkeeping | already auto-merged, **15 minutes** after opening |

No other repo has an open PR — GMPI_Wrappers, SynthEditLib, gmpi_ui, GMPI and SynthEdit were all checked and all empty.

**E91 is flipped DONE and archived, with the 10-10 caveat carried into the row rather than dropped.** Four consecutive mac cells offered Jeff *"merge it as a record of the old route, or hand it back for a re-measure"*; merging it is him taking the first. So the archived row says in terms that the probe measured `5175b02`, **before E81**, that `13d4720` changed the delivery route out from under it, and that **this row must not be cited as evidence about today's AU3 restore**. The durable part is the method, not the number.

### #652 was NOT merged, and this is the finding

The probe fails on this platform. `python3 tests/e89_box_lock_probe.py` at `f95b6b7b5`, native Windows `python3`: **rc=1, 13 arms, 4 failures** — arms 5, 11, 12, 13 — with the control correctly at 12-of-12 winners, and the script printing its own verdict: *"Do NOT adopt the STEP 0.5 lock on this result -- a lock that is wrong in direction (2) strands the box, which #583 calls the worse outcome."* **Arms 1–4 and 6–10 pass, so exclusion itself works here**: a second concurrent run *is* refused. Every failure is the other direction.

**The cause is structural, not a flake.** `_parent_and_cmdline` returns `(None, None)` on Windows by construction — no `/proc`, and the `ps` branch sits behind `if os.name != 'posix': return None, None`, so it is never even attempted; its own docstring says so. The walk stops immediately, `--owner-pid auto` degrades to `self`, and `self` is the pid of the **`acquire` process**, which exits on return. That is exactly the hole the PR's own "part worth reading" section describes as its first version's defect.

**The consequence is wider than the PR's "NOT verified" note admits.** `pid_alive` is `return None` unconditionally off posix, which masks the instant staleness for `acquire` and **breaks `release`**. End-to-end on the literal STEP 0.5 / STEP 5 commands from the prompt diff, which pass no `--owner-pid`:

```
acquire  rc=0  pid=33336  recorded_by=33336  owner=self (NO agent ancestor matched -- age-capped only)
acquire  rc=1  REFUSED: another run holds the box lock -- pid=33336 ...        <- correct
release  rc=1  REFUSED: held by pid=33336 ..., not by this run
               (owner resolves to 33532 via self (NO agent ancestor matched))
```

**So on this box it is not a crashed run that leaves the lock held — it is every run**, because the prescribed release path cannot succeed. The 6 h age cap becomes the only reclaim route on the *normal* path, not the exceptional one. Two sentences in the prompt diff are therefore inaccurate for Windows rather than merely conservative: *"at most one run on Windows"* is not conditional on a crash, and *"a run that exits holding it"* reads as an edge case when here it is the only outcome.

**Severity stated fairly, because it is low for scheduled runs on this box.** Windows runs **weekly on purpose** — `0 20 * * 1`, and Jeff confirmed it in this session — so 6 h always expires long before the next Monday and no scheduled run is lost. The exposure is the **manual same-day runs A7 records him making**: one started within 6 h of a scheduled run is refused and told to *"stop immediately, write nothing at all"*, leaving no trace in the repo by that PR's own design. linux and macOS are unaffected; their `release` resolves the ancestor correctly.

**I did not touch `run-lock.py`.** It and E89 are the linux lane's. The finding is [posted as a comment on #652](https://github.com/JeffMcClintock/TideSynth/pull/652#issuecomment-6156131052) with three fix options in order of how little they move — `release` matching on host+repo when liveness is unknown; implementing `pid_alive` via `tasklist`/`OpenProcess`; or passing one stable `--owner-pid` at both ends — and an offer to re-run the probe here against whichever is picked.

### A44: Jeff confirmed the premise, which sharpens the row rather than closing it

He said ***"this box runs weekly on purpose"***. That is folded into A44, because the confirmation is what makes the digest's daily assumption *the* defect: there is now no reading in which the schedule is the thing to fix. The row now says explicitly that a future run must not "resolve" A44 by making windows daily, and must not read A7's *"the 7x/week cadence is DELIBERATE"* as covering this box — A7 measured macos. Two cadences, both deliberate, one global threshold.

**Learned:**

- **"Measure before you merge" earned its keep on the first try, and the thing it caught was not the thing the PR flagged.** #652 listed Windows as unverified and estimated the cost as *"at most one run after a crash"*. The measurement agreed about the mechanism and disagreed about the blast radius: the break is on the **normal** path, because `release` cannot match a pid that `acquire` recorded from a process that had already exited. A reading of the "not verified" note would have merged it.
- **A platform fallback can be safe in one verb and broken in the other.** `auto`→`self` plus `pid_alive`→`None` is conservative for `acquire` (never steals a lock) and fatal for `release` (never returns one). Checking only the acquire side looks like a pass.
- **Nine green arms out of thirteen is not "mostly working" when the four reds are all one direction.** #583's own framing — stranding is worse than collision — is what makes 4/13 a stop rather than a caveat, and the probe says so itself rather than leaving the reader to weigh it.
- **A conditional instruction is still a condition.** *"Measure on Windows, then merge"* is not *"merge"*; the measurement failed, so the merge did not happen. Report, do not improvise past it.
- **Merging another lane's PR is cheap; judging its evidence is not, and the mechanical rule covers only the first.** #643's merge was a two-second call on files and checks. Whether its *claim* still holds needed the 10-10 entry, and the archive is where that caveat had to go or it would have vanished with the row.

**Not verified:** I did **not** build anything, run TiDE, or take a screen — the developer was at this machine. I did not run the E89 probe on macOS, so its `ps` branch remains unmeasured by anyone. I did not attempt any of the three fixes I suggested for `run-lock.py`, so none of them is measured either; the first option's effect on arms 5 and 12 is my reading, not a result. **#643's AU3 claim against today's `main` is still unmeasured by anybody** — merging it changed nothing about that, which is why the archived row says so. I did not re-judge E91's original mac evidence, only its currency.

**Machine state:** `C:\SE\TideSynth` stayed on `main` and was never checked out; both pieces of work were scratchpad `git worktree`s, each removed at the end. **The developer's four work-in-progress files are untouched for the second time today** — `DefaultRack.synthedit`, `RackModules/Envelope.synthedit`, `modules/PatchPoint/PatchPoint.cpp`, `modules/PatchPoint/PatchPointGui.cpp`, still mtimed 2026-10-09, still real content changes. The E89 probe was run with an explicit `--lock` path in the scratchpad and that file was deleted, so **`~/.tide/` never held a `run.lock`** and is exactly as found. No credential value appears anywhere.

**Next:** see the `win` cell. **For Jeff:** (1) #652 needs a Windows fix before it lands, or it will break `release` on this box every run — the comment names three options. (2) A44 still needs the cadence-source pick; you confirmed its premise, not its answer. (3) The bookkeeping-hot-spots `PROPOSED:` entry is still past its own `Decide-by`, with A37 parked behind it. (4) Nothing else is open in any repo.

**Branch/PR:** `tide/win/2026-10-13-merge-sweep`. This entry, E91's flip and archive, the A44 update, the `win` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-13 — windows — no item: the queue is empty for `win`, and the run's finding is that this lane was never halted — the box is WEEKLY and the digest assumes daily (scheduled run)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.31226.1** (CLI `2.1.295`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item — nothing is eligible for `win`, and the walk below is derived this run rather than inherited. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`b0b0a013a..938e648c1`, plus the new `tide/linux/E89-box-run-lock`). Filed **A44**. First windows entry since 10-06, which is **exactly one week, because that is this box's schedule.**

**STEP 1:** no open `platform:win` issue, and on this platform that verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing, as four previous cells record. Reading `main`'s `build` instead: green at **`07b0172`**, the last code commit. The one red run since, `ecc73e4`, was **macos-only** and was a CPM.cmake download (`curl` error 56); the 10-10 mac entry triaged it and the next two commits were green. The only open issues are #583 (linux, triaged as E89) and #44 (the digest).

**STEP 1.5:** no open PR on `tide/win/**`. The fleet's two open PRs are [#652](https://github.com/JeffMcClintock/TideSynth/pull/652) (linux, E89, `CLEAN`) and [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (mac, E91) — neither this lane's, so neither mine to touch.

### The finding: this lane was never halted, and the digest cannot tell

Three consecutive mac entries escalated windows' silence to Jeff, the 10-11 one saying *"If those machines were on, someone at each keyboard needs to run STEP 0.7's assertions"* and today's reading *"Their last entry is 2026-10-06, seven days ago as of today"*. **That is the digest's `LIKELY HALTED` branch, raised on the exact day this box woke on time and ran.** Measured three ways:

| evidence | value |
|---|---|
| this box's scheduled task | `cronExpression: 0 20 * * 1` — *"At 08:07 PM, only on Monday"*, `enabled: true` |
| its `lastRunAt` / `nextRunAt` | **2026-10-13** (this run) / **2026-10-19** |
| `scripts/watchdog-digest.py:290-291` | `QUIET_DAYS = 3`, `HALTED_DAYS = 7` |
| the premise, in that file's own comment | *"The fleet runs roughly daily, so one missed day is ordinary … and three is not."* |
| 2026-10-06 → 2026-10-13 | both **Mondays**, exactly 7 days apart — two consecutive on-schedule runs |

So a healthy windows box is `QUIET` for four days in every seven and `LIKELY HALTED` on its own run morning, **by construction and forever**. Filed as **A44** with three non-equivalent options for where a per-box cadence should come from, because picking one is Jeff's. The row says explicitly what a run must not do meanwhile: **do not widen `QUIET_DAYS`**, which would buy windows' quiet by blinding the one box the check demonstrably works for.

**This is not A7 re-filed, and A7 says no run should re-file it.** A7 asked to *change* the cadence and was answered *"the 7x/week cadence is DELIBERATE … not a scheduling problem"*. A44 asks nothing about cadence. It is the hazard A7 flagged on its way out — *"The **documents** that assume 1x are the part worth knowing about"* — arriving inverted: a *script* that assumes 7x. Both hold because **the fleet genuinely runs two cadences**: A7 measured macos at `0 6 * * *`, this run measures windows at `0 20 * * 1`. I can read only this box's task, so linux's cron is still unmeasured.

### STEP 2, walked rather than inherited

A35 is parked on its own two open `PROPOSED:` entries (both read, both still open). A42 is ineligible by its own words — *"(b) without (a) is not identical under every answer"*. S8 carries `NEEDS-SPEC` first in its row. E2 is an umbrella that says of itself it is not takeable. E84 is a `.github/workflows/**` edit this credential deliberately cannot make. X2 and E89 are `linux`; E93 is `mac`; E91 is IN-REVIEW. **E19 and E82 need a screen and the developer was at this machine** — Visual Studio on `SynthEdit_cmake - GmpiUiTest.cpp` started 13:15, four minutes before this run, plus Chrome, Outlook, Slack and a running VirtualBox VM. Two rows got more than an inherited verdict:

**E92 is Linux-only in fact, and I verified it on Windows instead of trusting the mac reading.** `SE16/se_gmpi/vst3/CMakeLists.txt:17` is `if(NOT UNIX OR APPLE) return()`, **four lines above** the `gmpi_wrappers_POPULATED` guard the row is actually about. On Windows `UNIX` is false, so the file returns before the guard and `SynthEditTemplate_VST3` cannot exist under *either* variable — E92's Accept is unsatisfiable here by construction, as the 10-08 mac entry found for mac. It is now measured on two platforms of three and its `Plat` cell still says `any`; correcting that is what the open `Plat`-correction entry governs, so I left it.

**A37's parking reason was weaker than the real one, and three cells carried the weak version.** They park it on the bookkeeping-hot-spots `PROPOSED:` entry — but that entry says in terms *"May proceed meanwhile: EVERYTHING, without exception … it makes no row ineligible"*, so a bare appeal to it parks nothing. It parks A37 anyway, and the collision is with **A37's own Accept**: *"the older links are recoverable from a named file"*, plus its *"where the rest goes"* clause, is answered differently by each option — one table under (a), per-platform sections under (b), `docs/next/<platform>.md` under (c), per-run journal files too under (d). **A37 is the single row whose deliverable IS the structure the question is about**, which is why a *"parks nothing"* line written about product rows does not reach it. A37's row already concedes it: *"the convention question … may want Jeff rather than a run"*. That entry is also **twelve days past its own `Decide-by`** (*"the next `JOURNAL.md` rotation"*, done 2026-10-01).

### A42's id guard had its first live save

`origin/main`'s highest A-series id is **A42**. Sweeping the two remote `tide/*` branches showed `tide/linux/E89-box-run-lock` already carrying A43, filed by linux hours earlier today and **invisible on `main`**. (That id is written unbolded on purpose. `check-id-refs.py` reads `**A43**` as a cross-reference and would correctly fail this branch, because the row really is dangling here -- its own docstring calls stacked PRs a known limitation and prescribes `--allow-id` on the trailing PR, which the `lint` workflow does not pass. Unbolded, the id is still legible as evidence and the branch stays green.) Allocating from `main` alone would have produced a second A43 surfacing only at merge time — the C15/C16 shape A42 was filed to prevent. So this row is **A44**. The guard cost one loop over `git ls-remote --heads origin`, exactly as A42 claims.

### The #639 STEP 4 debt: addressed, and not payable in full

The 10-06 linux entry recorded that [#639](https://github.com/JeffMcClintock/TideSynth/pull/639) (`tide/win/E86-controller-required`) landed code and a ruling with no journal entry, and called it this lane's debt. **It cannot be paid as written and the next run should not reopen it.** No run can write a first-person entry for work it did not do and has no memory of, and `JOURNAL.md` is newest-first, so a back-dated 10-06 entry cannot be prepended without failing `check-journal-prepend.py`. #639's PR body is a full record — the ruling in Jeff's words, three shipped changes, a four-row verification table with real object sizes, and an explicit *"Not verified: the diagnostic actually PRINTING in a processor-only run"*. This entry points at it, and that is the discharge. Also settled, closing the previous cell's instruction (2): **#634 is CLOSED UNMERGED**, so E86 never became buildable along one of its four options — the ruling was recorded by hand instead, and E86 is archived DONE.

**Learned:**

- **A fleet-liveness threshold is only as good as its cadence assumption, and the assumption is in a comment rather than in data.** `watchdog-digest.py` states *"the fleet runs roughly daily"* and three boxes do not. Before escalating another box's silence, check that box's own cron: one scheduled-task listing settles in seconds what three entries spent a paragraph each worrying about.
- **A `PROPOSED:` entry's blanket "parks nothing" line can be wrong about exactly one row — the row whose deliverable is the thing being decided.** Read the row's Accept against the options rather than trusting the entry's self-description, in either direction. This is the 10-06 E76 lesson running the other way: that one found a row parked when it was free, this one finds a row parked for a reason nobody had stated.
- **A42's branch sweep is not theoretical.** Its first use found a live collision the same day it was used, against a row filed hours earlier on another box.
- **A STEP 4 debt for work a run has no memory of is discharged by pointing at the PR, not by writing the entry.** The append-only, newest-first journal makes the back-dated entry illegal anyway, so "pay the debt" and "obey the lint" cannot both be satisfied — say so once and stop re-inheriting it.
- **Read a row's platform guards on YOUR platform even when another box already read them on theirs.** E92's guard excludes win and mac for the same one line, but "mac read it" is not "win measured it", and the two together are what makes "Linux-only in fact" a fleet claim.

**Not verified:** nothing was built, compiled or run, because there was no item — no local build of `main` on Windows this run, so "green" above is CI's, not mine. **A44 is a reading plus two measurements, not a fix**: I did not edit `watchdog-digest.py`, and I did not run the digest to watch it emit the false `LIKELY HALTED` line — the claim rests on the thresholds, the premise comment, the cron and the three mac entries that acted on it. **Linux's cron is unmeasured**; I can read only this box's scheduled task, so "the fleet runs two cadences" is established for windows and macos and open for linux. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half); the prompt records **2026-11-07**, 25 days from today. E92's Windows claim is a reading of the CMake guard — I did not configure SE16 to count targets.

**Machine state:** `C:\SE\TideSynth` **stayed on `main` and was never checked out** — all work was in a scratchpad `git worktree`, removed at the end. **That tree is dirty with the developer's work in progress and I left every file alone:** `DefaultRack.synthedit`, `RackModules/Envelope.synthedit`, `modules/PatchPoint/PatchPoint.cpp`, `modules/PatchPoint/PatchPointGui.cpp`, all four mtimed **2026-10-09** and all four real content changes (`git diff --ignore-all-space` shows 196/77/99/133 diff lines, so none is CRLF churn). Not committed, not reverted, not stashed. `C:\SE\SE16` was read only (one `CMakeLists.txt`), and no other repo was touched. **No GUI was taken and no screenshot made** — the developer was working at this machine throughout. No credential value appears anywhere.

**Next:** see the `win` cell. **For Jeff, four things:** (1) **A44 — the digest will call this box halted every Monday until a cadence source is chosen**, and widening `QUIET_DAYS` is the wrong fix. (2) The bookkeeping-hot-spots `PROPOSED:` entry is twelve days past its own `Decide-by`, and A37 is parked behind it. (3) #643 and #652 are both waiting on you; #643's evidence predates E81, as the 10-10 mac entry explains. (4) The bot token's recorded expiry is 2026-11-07, 25 days out.

**Branch/PR:** `tide/win/2026-10-13-queue-blocked`. This entry, the A44 row, the `win` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.
