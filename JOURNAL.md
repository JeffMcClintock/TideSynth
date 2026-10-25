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

## 2026-10-26 — macos — no item: #669 (E82) is green and waiting on Jeff, and E19's mac AU3 cell still needs his AUv3 registration displaced (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **3.6556.2** (CLI version not readable, no `claude` on PATH) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded. `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: `gh issue list --label platform:mac --state open` returns `[]`. STEP 1.5: the only open PR is [#669](https://github.com/JeffMcClintock/TideSynth/pull/669) (E82, `tide/mac/E82-wtlfo-wave-points`, three bot commits). Every check is green, `mergeable: MERGEABLE`, and it has no reviews and no comments. So it is waiting for merge and I left it alone. STEP 2: `git diff --stat dc80d88 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only the 10-24 `mac` cell line. I followed the `mac` cell on #669's branch, since that cell is newer than `main`'s:

- **(1)** #669 has not been handed back. Nothing to re-run.
- **(2)** The box is idle again. HID idle was **208,378 s (~58 h)**, the display has been off since 2026-10-23 17:13, and the only long-lived processes are the two GitHub runners. But E19's remaining mac clause is the **AU3 host half**. The 2026-08-29 mac entry found that measuring a current build needs the developer's AUv3 registration displaced, and that an unattended run must not do that. `pluginkit -m -v` still shows only the 2026-08-31 registration (`com.tidesynth.tiderack.au3app.extension` 0.1.1, `~/Applications/TIDE-Rack-AUv3.app`). That build is eight weeks older than `main`, so measuring it would not answer the cell. The standalone half of the `int/bool/enum` clause is #669's work.
- **(3)** E93: not built. **(4)** `gpu-backend` is Jeff's and was not touched.

Nothing is eligible.

**Not verified:** nothing was built or run. I did not read `docs/lessons.md` or the full `BACKLOG.md` in full, only the parts that bear on this walk: the `mac` cell, the E19 row, and E73 in `BACKLOG-DONE.md`. I left the `mac` NEXT cell untouched on purpose. #669 rewrites that line, and a second edit here would conflict with a PR that is waiting on Jeff. #669's cell already says what the next run should do. The bot token expires **2026-11-07**, 12 days from today. I did not re-check that date.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** follow the `mac` cell on #669. **For Jeff:** (1) #669 (E82) is ready for review. (2) E19's mac AU3 cell needs you present, to register a current `TIDE_VCV_FUNDAMENTAL=ON` + `RACK_ADAPTOR_TRACE` AUv3 over yours. (3) The bot token expires 2026-11-07.

**Branch/PR:** `tide/mac/noitem-2026-10-26`. Journal-only. It will conflict with #669 at the top of `JOURNAL.md`; see the PR body.

## 2026-10-24 — macos — no item: STEP 1 and 1.5 empty, and nothing in BACKLOG, decisions or PLAN has moved since the 10-23 walk (scheduled run)

**Prompt:** 1821076 · Sonnet 5.5, `claude-sonnet-5-5` · app Claude desktop (version not read this run) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded. `run-lock.py acquire` returned rc=0.

**Result:** STEP 1: `gh issue list --label platform:mac --state open` returns `[]`. STEP 1.5: `gh pr list --state open` returns `[]`. STEP 2: `git diff --stat 6538c2b origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows one line changed in `BACKLOG.md` (the 10-23 `mac` NEXT cell). The 10-23 walk therefore stands row for row: A43 and A45 parked, E93 waiting on Jeff, E19 and E82 need an idle GUI box that a scheduled run cannot approve for computer-use. Nothing is eligible.

**Learned:** nothing new. The mac lane fired in its 02:05 slot again, one day after the last run.

**Next:** run the three steps in the `mac` NEXT cell, using `dc80d88` as the new diff base.

**Not verified:** nothing was built or run. I did not re-read `docs/lessons.md` in full. The bot token expires 2026-11-07; I did not re-check.

**Branch/PR:** `tide/mac/noitem-2026-10-24`

## 2026-10-23 — macos — no item: STEP 1 and 1.5 empty, #643 merged, and the new `any` rows A43 and A45 are both parked (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop (version not read this run) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded. `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`). This is the first scheduled run on macOS under the box lock.

**STEP 1:** there is no open `platform:mac` issue. **STEP 1.5:** there are no open PRs in the fleet. [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91) **merged on 2026-10-13**, and E91 has already been archived. So the 10-13 cell's step (1) is closed, and no `IN-REVIEW` row is left to flip.

**STEP 2:** I walked the rows again rather than relying on the earlier walk, because `BACKLOG.md` gained rows since `b0b0a01`. `git diff --stat b0b0a01 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only `BACKLOG.md` changes: three NEXT cells, A43 and A45 filed, A44 WONTFIX, and E89, E91 and E92 archived. `decisions.md` and `PLAN.md` did not change. The `mac`/`any` rows that are `TODO`:

- **A43** says *"a run should not pick"* among (a), (b) and (c). Work that is identical under every answer does not exist, because (a) alone builds no lint.
- **A45** is scoped to the **windows** box's scheduled task, which its own words say is *"NOT a run's to edit"*. The decision is Jeff's.
- **A35, A37, A42, S8, E2 and E84** are unchanged and parked for the reasons the 10-20 windows entry lists.
- **E19 and E82** need an idle box with a GUI. This run fired at 02:05 local, so the box may be idle. But computer-use approval is suppressed for scheduled tasks on this box (measured in earlier mac runs). So neither clause can be measured from here.
- **E93** (`mac`) is still waiting on Jeff's AU2 scope call. The cell says not to build an AU2 consumer unasked.

Nothing is eligible.

### A measurement for A45, which its own entry says it lacked

The 10-20 windows entry says *"macOS's and linux's crons remain unmeasured by me"*. Here is the mac one, from `list_scheduled_tasks` and `list_task_runs`:

| evidence | value |
|---|---|
| cron | `0 2 * * *`, `jitterSeconds: 271`, read in local time (NZDT, UTC+13) |
| this run | started `2026-10-22T13:05:03Z` = **2026-10-23 02:05 local**, **in its slot** |
| previous run | `2026-10-12T13:05:21Z` = 2026-10-13 02:05 local, also in its slot |
| between them | **no runs at all.** Nine daily slots (10-14 to 10-22 local) have no session against them, and nothing caught them up at app launch either |
| 10-02 to 10-13 | twelve runs, all at 13:04–13:05Z, every one in its slot |

So the mac lane does not show windows' A45 pattern. When this box runs, it runs in its 02:05 slot. When it misses, it seems to miss outright with no catch-up, unlike windows. I did not establish why the nine days were missed. Likely causes are the machine being asleep or off, or the app being closed. That is an inference, and I have not checked it. I filed nothing, because this is data for A45 rather than a new problem.

**Default branch:** I did not build `main` locally. The last code commits on `main` are Jeff's (`46aee55`, `e8586e9`, `c67f5c2`).

**Learned:**

- **A lane's gap in the journal can be missing firings, not runs that wrote nothing.** `list_task_runs` tells the two apart in one call. Use it before reading anything into a quiet lane.

**Not verified:** nothing was built or run, because there was no item. I did not read `docs/lessons.md` in full (175 KB), only the parts that bear on this walk. I did not check the Claude app version. The bot token expires **2026-11-07**, 15 days from today. I did not re-check that date.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. No GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) E93 needs your AU2 scope call. (2) A43 and A45 each need a pick from you. (3) The bot token expires 2026-11-07.

**Branch/PR:** `tide/mac/2026-10-23-queue-blocked`. This entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-20 — windows — no item: the queue is empty for `win` for the fourth time, and the cause is finally measured — this box almost never runs in its scheduled slot (scheduled run)

**Prompt:** 18210767f · Opus 5, `claude-opus-5` · app Claude desktop **2.42304.0** (CLI version not readable — `~/.claude/versions` is empty on this box) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item — nothing is eligible for `win`, and the walk below is derived this run rather than inherited. Filed **A45**. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`c67f5c2f2..20a60580f`). **Second scheduled run under the box lock, and the first on this platform:** `run-lock.py acquire` rc=0, `owner=self (NO agent ancestor matched -- this lock is age-capped only)`, pid 27316 — the documented Windows path, with the warning it is supposed to print.

**STEP 1:** no open `platform:win` issue, and on this platform that verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing, as five previous cells record. The only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, triaged as E89, which is DONE and archived) and #44 (the digest). **STEP 1.5:** no open PR on `tide/win/**`, and **zero open PRs in the fleet** — `git ls-remote --heads origin` returns `main` alone.

### The finding: this box almost never runs in its scheduled slot, and the catch-up fires at the worst possible moment

This is the fourth windows run to end with no item for the same reason — **E19 and E82 are the only two rows this lane can take, both need an idle box, and the box has never been idle at run time.** The previous three (09-14, 09-16, 10-13) each recorded that as a circumstance. It is not a circumstance; it is structural, and the mechanism is this:

| evidence | value |
|---|---|
| this box's cron | `0 20 * * 1`, `jitterSeconds: 396`, `enabled: true` |
| `nextRunAt` | `2026-10-26T07:06:36Z` = **2026-10-26 20:06 Mon** local (`New Zealand Standard Time`, UTC+13 today) |
| so the cron is read in | **local time** — a UTC reading would have put `nextRunAt` at 20:06Z |
| the slot this run should have used | Monday **2026-10-19 20:06** local = `2026-10-19T07:06Z` — **no run against it** |
| `lastRunAt`, i.e. this run | `2026-10-19T20:10:45Z` = **2026-10-20 09:10 Tue** local |
| `claude` process start | **09:10:39** local — the run started **09:10:45**, six seconds later |
| every other process on the box | `chrome`, `OUTLOOK`, `slack`, `VirtualBox`, `TextInputHost`, all **09:10:26–09:10:59** |
| `GetLastInputInfo` at 09:12 | **1 second** of idle |

So the box was logged in at 09:10, the missed Monday slot was caught up at app launch, and **app launch is by construction the moment the developer sits down.** A run that fires there cannot find an idle box no matter how the queue is ordered.

**Four of the five firings since the cadence went weekly are morning or afternoon ones:** 2026-10-05 09:54 Mon, 2026-10-06 09:01 Tue, 2026-10-13 13:15 Tue, 2026-10-20 09:10 Tue. The one evening firing, 2026-10-01 22:07 Thu, was still on the **old** cron — which did land in its slot: sixteen consecutive days, 2026-09-16 to 2026-10-01, all at **22:02–22:07 local**. (The 10:0x→09:0x UTC shift across 09-27 is New Zealand DST, not a schedule change.)

**A correction this run owes, because three entries leaned on it.** The 10-13 windows entry and the `win` NEXT cell say *"2026-10-06 → 2026-10-13, both **Mondays**, exactly 7 days apart — two consecutive on-schedule runs"*. **Both were Tuesdays** (09:01 and 13:15 local), **neither was the 20:06 Monday slot**, and both were catch-up firings — as is today. `JOURNAL.md` is append-only so that entry stands as written; A45 and this entry are the correction, and the `win` cell now says so above its own stale chain.

**It also answers A7's one undiagnosed observation.** On its way to WONTFIX that row recorded *"a firing at 20:11 local that a 06:00 daily cron does not explain; cause unknown — manual trigger, catch-up, or otherwise — recorded as an observation, not a diagnosis."* Catch-up at app launch is the mechanism, measured here to the second.

**The honest half, and it is why A45 picks nothing.** An evening slot that actually fires is *necessary* and is **not known to be sufficient**: the 09-16 run did land in its 22:03 local slot and still found the developer at the machine with three Visual Studio windows. A row claiming "fix the slot and the lane unblocks" would overstate itself exactly the way A44 did, so A45 offers four shapes — a better slot, skipping a missed slot instead of catching it up, an off-screen arm for the two clauses, or ruling them interactive-only — and picks none. Every one of them is about Jeff's own working hours and his own box.

**A45 is not A7 and not A44, and both of those say not to re-file them.** A7 asked to change the macos cadence and was answered that 7x is deliberate; A45 asks nothing about frequency. A44 was about `watchdog-digest.py`'s liveness thresholds; A45 does not touch the digest, does not make windows daily, and does not widen `QUIET_DAYS`. What it *is* is A44's own parting suggestion taken literally — *"have each run record its own `nextRunAt` in the provenance line"* — arriving as the thing that suggestion would have caught.

### STEP 2, walked rather than inherited

Nine rows are `TODO` and reachable by `win` or `any`, and every one of them is parked:

- **A35** says in terms it is *"PARKED ON ITS OWN TWO `PROPOSED:` ENTRIES … NOT AVAILABLE TO TAKE"*; both entries are still open.
- **A37** is parked on its own Accept, not on the bookkeeping-hot-spots entry's blanket *"parks nothing"* line — the 10-13 reasoning, which still holds: *"the older links are recoverable from a named file"* is answered differently by each option. That entry is **nineteen days** past its own `Decide-by`.
- **A42** is ineligible by its own words: *"(b) without (a) is not identical under every answer"*.
- **A43** is the same shape and says *"a run should not pick"*.
- **S8** carries `NEEDS-SPEC` first in its row.
- **E2** is an umbrella that says of itself it is not takeable.
- **E84** is a `.github/workflows/**` edit this credential deliberately cannot make.
- **E19** and **E82** need an idle box, which is this entry's finding.
- **X2** is `linux`, **E93** is `mac`.

**E92 is off that list — it went DONE on 10-16**, so the inherited "ten rows" count is nine now. All four open `PROPOSED:` entries were read; all four say *"May proceed meanwhile: everything"* or *"EVERYTHING, without exception"*, so none parks a row by its own clause, and A35/A37 are parked by their rows' own Accepts instead. No `IN-REVIEW` row exists, so STEP 4's mechanical flip rule reaches nothing.

**Id allocation per A42's guard:** `git ls-remote --heads origin` returns `main` alone, so no concurrent branch holds an id. Highest A-series on `main` is **A44** (`BACKLOG.md`) and **A41** (`BACKLOG-DONE.md`). So this row is **A45**.

### Also done: the lock probe against the merged `main`, which the 10-14 entry left open on this platform

That entry's own "Not verified" says *"I did not run the probe against the merged `main`, only against the branch."* Run here at `20a60580f` in a scratchpad worktree, native Windows `python3`: **14 arms, 0 failures**, and the control correctly reports **12 of 12** racers winning the deliberately-broken non-atomic lock, which is what makes arm 4's *1 of 12* a result.

**Arm 14 is the one worth quoting**, because it pins the Windows-specific half the 10-13 run measured as broken before #652 was fixed: explicit-owner acquire rc=0, **unforced `release` rc=0** (`removed=True`) via `same box (pid ownership unavailable on this platform)`, another box still refused rc=1. Arms 5, 11, 12 and 13 — the four that failed on 10-13 — now pass as assertions of the *documented non-posix contract* rather than the posix one: a dead pid is **not** reclaimed here because liveness is unknown, and the age cap is the only route. The lock also worked end-to-end this run, which is the live confirmation.

**Learned:**

- **Check your own start time against the task's `nextRunAt` before saying anything about a lane's health.** Four runs described "the developer was at the machine" as bad luck. One `list_scheduled_tasks` plus one timezone conversion showed the runs were not happening in their scheduled slot at all — and the slot was an evening one chosen, presumably, to avoid exactly this. The provenance line already carries the prompt sha, model and app version; the one field that would have caught this in a week rather than a month is the one A44 suggested on its way out.
- **A missed slot caught up at app launch is strictly worse than a skipped run, for any work that needs an idle machine.** The two are not equivalent-with-a-delay: app launch *selects for* the developer being present. This is a general hazard for agent schedules on a workstation, not a TIDE one.
- **`cronExpression` alone does not tell you when a task runs; `nextRunAt` tells you how it is interpreted.** `0 20 * * 1` plus `nextRunAt 2026-10-26T07:06:36Z` is what establishes local-time evaluation (UTC+13 here). Reading the cron as UTC would have put every conclusion in this entry thirteen hours out, and the 10-13 entry's "both Mondays" is what that error looks like.
- **Converting a UTC timestamp to a weekday is worth doing with the timezone database rather than by eye.** Two dates this project called Mondays were Tuesdays, and the DST boundary on 2026-09-27 moves the same local hour by one UTC hour mid-series — which reads as a schedule change if you only have the UTC column.
- **A row can answer an old row's explicitly-undiagnosed observation without re-filing it.** A7 wrote down a firing its cron could not explain and said so; naming the mechanism two months later is a new finding, not a re-litigation of the WONTFIX.

**Not verified:** **nothing was built, compiled or run as product code** — there was no item, so no local build of `main` on Windows this run, and "green" is CI's claim rather than mine (and I did not re-read the CI runs this time; the 10-14 entry's `ff6c245db` green is the last reading, four commits back). **No GUI was taken and no screenshot made** — the developer was at the machine, which is the finding itself. **A45 is a measurement plus a reading, not a fix:** I did not edit the scheduled task, which is outside every repo and not a run's to touch, and I did not test whether the harness can be told to skip a missed slot rather than catch it up — option (b) is a hypothesis about the harness, not a measured capability. **I did not test option (c)**, so "E82's clause may not need a visible window" is an inference from the command channel calling the editor's own input client, not a result; nobody has tried it off-screen. **macOS's and linux's crons remain unmeasured by me** — I can read only this box's task, so nothing here generalises to the other two lanes' slots. The lock probe measured this platform only; **it has still never run on linux or macOS, by anyone.**

**Machine state:** `C:\SE\TideSynth` **stayed on `main` and was never checked out** — all work was in two scratchpad `git worktree`s (one at `origin/main` for the probe, one for this branch), both removed and pruned at the end. **The tree was clean on arrival for the first time in this lane's recent history** — the developer's four work-in-progress files became his own commit `27b36f71a` on 10-14, as that entry records, so there was no dirt to leave alone. No other repo was touched, read or fetched: `C:\SE\SE16`, `C:\SE\SynthEditLib`, `C:\SE\gmpi_ui`, `C:\SE\GMPI_Wrappers` and `C:\SE\GMPI` were not opened this run, because no item needed them. No credential value appears anywhere. Box lock released as the last act.

**Next:** see the `win` cell. **For Jeff, three things:** (1) **A45 is the only thing waiting on you**, and it is a scheduling judgement only you can make — this box's run has fired in its actual slot once in five weeks, and the two rows this lane can take both need you *not* to be at the machine. (2) The bookkeeping-hot-spots `PROPOSED:` entry is **nineteen days** past its own `Decide-by`, with A37 parked behind it. (3) The bot token expires **2026-11-07**, 18 days out.

**Branch/PR:** `tide/win/2026-10-20-queue-blocked`. The A45 row, this entry, the `win` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

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
