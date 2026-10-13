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

## 2026-10-13 — macos — no item: STEP 1 and 1.5 empty, and since `40ff8e8` only the 10-12 `mac` cell moved, so the walk stands (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.31226.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`40ff8e8..b0b0a01`).

**STEP 1:** there is no open `platform:mac` issue. The only open issues are #583 (linux, already triaged as E89) and #44 (the digest).

**STEP 1.5:** [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91) is still the only open PR in the fleet. Merge state `CLEAN`, 13 checks pass and 2 skip, no reviews and no comments. Jeff has not handed it back, so I left it alone. The 10-10 entry's point stands: its evidence measures the pre-E81 route.

**STEP 2:** the only commit on `main` since the last walk is the 10-12 run's own bookkeeping (`b0b0a01`, #650). `git diff --stat 40ff8e8 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md | 2 +-`, and that one line is the `mac` cell. So the 10-08 table still holds row for row, and nothing is eligible for `mac`. The digest's four `PROPOSED:` entries are unchanged.

**Default branch:** `main`'s last code commit, `07b0172`, is green in CI. The runs since then are bookkeeping, and their `watchdog`, `verify` and `auto-merge` runs all passed. I did not build `main` locally.

**Fleet liveness:** the 10-12 digest on #44 still flags **windows and linux as QUIET**. Their last entry is 2026-10-06, seven days ago as of today.

**Learned:** nothing new. This is the sixth no-item mac run in a row.

**Not verified:** nothing was built or run, because there was no item. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half). The prompt gives it as **2026-11-07**, 25 days from today.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. No GUI was used and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) #643 predates E81: merge it as a record of the old route, or hand it back for a re-measure. (2) E93 needs your scope call. (3) Windows and linux have been silent since 10-06, a full week. (4) The bot token expires 2026-11-07.

**Branch/PR:** `tide/mac/2026-10-13-queue-blocked`. This entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-12 — macos — no item: STEP 1 and 1.5 empty, and since `d3fa83e` only the 10-11 `mac` cell moved, so the walk stands (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.31226.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`d3fa83e..40ff8e8`).

**STEP 1:** there is no open `platform:mac` issue. The only open issues are #583 (linux, already triaged as E89) and #44 (the digest).

**STEP 1.5:** [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91) is still the only open PR in the fleet. Merge state `CLEAN`, checks `SUCCESS`/`SKIPPED` only, no reviews and no comments. Jeff has not handed it back, so I left it alone. The 10-10 entry's point stands: its evidence measures the pre-E81 route.

**STEP 2:** the only commit on `main` since the last walk is the 10-11 run's own bookkeeping (`40ff8e8`, #649). `git diff --stat d3fa83e origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md | 2 +-`, and that one line is the `mac` cell. So the 10-08 table still holds row for row, and nothing is eligible for `mac`. The digest's four `PROPOSED:` entries are unchanged.

**Default branch:** `main`'s last code commit, `07b0172`, is green in CI (`build`, `verify` and `auto-merge` all `success`). `d3fa83e` and `40ff8e8` are bookkeeping only, and their `watchdog` runs passed. I did not build `main` locally.

**Fleet liveness:** the 10-11 digest on #44 still flags **windows and linux as QUIET** (last entry 2026-10-06, six days ago as of today). This box cannot tell why.

**Learned:** nothing new. This is the fifth no-item mac run in a row. The open questions are all Jeff's, so mac will stay idle until he answers one of them.

**Not verified:** nothing was built or run, because there was no item. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half). The prompt gives it as **2026-11-07**, 26 days from today.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. No GUI was used and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) #643 predates E81: merge it as a record of the old route, or hand it back for a re-measure. (2) E93 needs your scope call. (3) Windows and linux have been silent since 10-06. (4) The bot token expires 2026-11-07.

**Branch/PR:** `tide/mac/2026-10-12-queue-blocked`. This entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-11 — macos — no item: STEP 1 and 1.5 empty, and since `07b0172` only the 10-10 `mac` cell moved, so the walk stands (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.31226.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`07b0172..d3fa83e`).

**STEP 1:** there is no open `platform:mac` issue. The only open issues are #583 (linux, already triaged as E89) and #44 (the digest).

**STEP 1.5:** [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91) is still this lane's only open PR and the only open PR in the fleet. It has 13 checks passing and 2 skipped, no reviews and no comments. Jeff has not handed it back, so it is waiting for merge and I left it alone. The 10-10 entry's point stands: its evidence measures the pre-E81 route.

**STEP 2:** the only commit on `main` since the 10-10 walk is that run's own bookkeeping (`d3fa83e`, #648). `git diff --stat 07b0172 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md` with one changed line, the `mac` cell, and nothing else. I re-listed every active row's Status and Plat cells as a check. The eligible set and each row's reason for being parked are the 10-08 table's: A35, A37 and A42 are parked on their own open questions, S8 is `NEEDS-SPEC`, E19 and E82 need a screen, E2 is an umbrella, E84 is a workflow edit, E92 is Linux-only by its CMake guard, E93 waits on Jeff's AU2 scope call, and X2 and E89 are `linux`. The four `PROPOSED:` entries the digest lists are unchanged.

**Default branch:** `main`'s last code commit, `07b0172`, is green in CI (`build`, `verify` and `auto-merge` all `success`). `d3fa83e` is bookkeeping only, and its `watchdog` run passed. I did not build `main` locally.

**Fleet liveness:** the 10-10 digest on #44 flags **windows and linux as QUIET**, with no journal entry from either since 10-06. Today that is five days. The digest cannot say why, and nor can this box. If those machines were on, someone at each keyboard needs to run STEP 0.7's assertions.

**Learned:** nothing that an earlier entry does not already say. This is the fourth no-item mac run in a row. The 10-09 `git diff --stat` check settled STEP 2 again, and the Status re-list agreed with it.

**Not verified:** nothing was built or run, because there was no item. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half), which the prompt gives as **2026-11-07**, 27 days from today.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean, and never left it. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. No GUI was used, no screenshot was taken, and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) #643 predates E81. Either merge it as a record of the old route or hand it back, and a mac run will re-measure it. (2) E93 needs your scope call. (3) Windows and linux have been silent since 10-06. (4) The bot token expires in under four weeks.

**Branch/PR:** `tide/mac/2026-10-11-queue-blocked`. This entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-10 — macos — no item: STEP 1 and 1.5 empty, and nothing eligible for `mac`; E81's ruling leaves #643 measuring a route `main` no longer uses (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.31226.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`3d58174..07b0172`).

**STEP 1:** there is no open `platform:mac` issue. The only open issues are #583 (linux, already triaged as E89) and #44 (the digest).

**STEP 1.5:** [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91) is still this lane's only open PR and the only open PR in the fleet. It has 13 checks passing and 2 skipped, no reviews and no comments, so it is waiting for merge and I left it alone. **Its evidence is now out of date, though. See below.**

**STEP 2:** `git diff 3d58174 origin/main -- BACKLOG.md PLAN.md` shows only the 10-09 `mac` cell. `docs/decisions.md` changed in one way: **Jeff answered E81's `PROPOSED:` entry on 10-09** (*"SynthEdit's handle system stays as it is, and TIDE stops SAVING the DSP"*), which leaves four open. Every walk since 10-07 records that E81's entry parked no row, so removing it frees nothing. The other four are unchanged. I re-listed the Status and Plat cells of every active row. The eligible set and the reasons are the 10-08 table's, row for row: A35, A37 and A42 are parked on their own open questions, S8 is `NEEDS-SPEC`, E19 and E82 need a screen, E2 is an umbrella, E84 is a workflow edit, E92 is Linux-only by its CMake guard, E93 waits on Jeff's AU2 scope call, and X2 and E89 are `linux`.

### What E81's change does to #643

Jeff's `13d4720` changes how the DSP is delivered. The processor now builds only the DSP that the controller pushes on a new non-persistent **parameter 4**, and parameter 1 saves `<Editor>` only. His own notes in `tests/e80_vst3_feedback_probe.cpp` say the processor *"has no pin for the restored parameter 1"*. He marked the e79 and e80 probes *"Not re-measured"*.

#643's probe (`tests/e91_au3_restore_order_probe.mm`, on the branch only) measured the old route: `-setFullState:` framing parameter 1's blob onto the ui→dsp queue. **Its control arm was what made the result mean something.** With that queue send deleted, both orders went silent. On `main` after `13d4720`, parameter 1's blob should not build a rack whatever that line does. The rack should come from the controller's parameter-4 push. So the control would probably not go silent any more, and the 3/3 result no longer says anything about `main`'s restore path. **This is a reading of Jeff's commit message and his probe comments. I did not rebuild or run anything to confirm it.** The PR still asks the right question (does an AU3 restore after `allocateRenderResources` reach the DSP?), but its answer is for `5175b02`.

**Default branch:** `main` at `07b0172` is green in CI (`build` and `verify` both `success`). One earlier `main` build, at `ecc73e4`, failed on **macos only**, at configure. The log shows `CMakeLists.txt:105 file DOWNLOAD … status: [56;"Failure when receiving data from the peer"]` three times, then `Could not download CPM.cmake v0.38.6 after 3 attempts`. That is a network failure on the runner, not a code break, and `4df8dbe` and `07b0172` both built after it. I filed no issue. I did not build `main` locally.

**Learned:**

- **A ruling can make an open PR's evidence stale without touching the PR.** #643 still shows green, and nothing on it changed. What changed is the code its control arm depended on. Since STEP 1.5 only checks a PR's checks and comments, read the commits that landed on `main` since the PR's base whenever they touch the PR's subject.
- **One red `main` run is not a platform break until you read its log.** The `ecc73e4` macOS failure was a CPM.cmake download that failed with curl error 56. The next two commits were green.

**Not verified:** nothing was built or run, because there was no item. The #643 finding above is a reading, not a measurement. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half), which the prompt gives as **2026-11-07**, 28 days from today.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean, and never left it. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. No GUI was used, no screenshot was taken, and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) **#643 predates E81.** Either merge it as a record of the old route, or hand it back with changes requested and a mac run will re-measure it against `07b0172` or later. (2) E93 needs your scope call. (3) A ruling on the `Plat`-correction `PROPOSED:` entry would let a run re-label E92. (4) The bot token expires in four weeks.

**Branch/PR:** `tide/mac/2026-10-10-queue-blocked`. This entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-09 — macos — no item: STEP 1 and 1.5 empty, and since the 10-08 walk BACKLOG, decisions and PLAN have changed only in the `mac` cell (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.26454.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`7e33c2e..3d58174`).

**STEP 1:** there is no open `platform:mac` issue. The only open issues are #583 (linux, already triaged as E89) and #44 (the digest).

**STEP 1.5:** [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91) is still this lane's only open PR and the only open PR in the fleet. It has no failing checks, no reviews and no comments, so it is waiting for merge and I left it alone.

**STEP 2:** `git diff --stat 7e33c2e origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows one changed line, the 10-08 `mac` cell. The ten commits since are that run's bookkeeping (#645) and nine of Jeff's own: Default Rack MIDI-CV layout, TiDEPanel and PatchPoint painting, `build-prefabs.py` dropping the MidiCv recipe, and File > New Project loading the Default Rack. None of them touches a row. I re-listed every active row's Status and Plat cells rather than trusting the 10-08 table. The eligible set is the same ten rows, and each is parked for the same reason the 10-08 entry gives: A35, A37 and A42 on their own open questions, S8 `NEEDS-SPEC`, E19 and E82 needing a screen, E2 an umbrella, E84 a workflow edit, E92 Linux-only by its CMake guard, and E93 waiting on Jeff's AU2 scope call. X2 and E89 are `linux`. The five `PROPOSED:` entries in `docs/decisions.md` are byte-unchanged.

**Default branch:** `main` at `3d58174` is green in CI on all three platforms (`build`, `verify` and `watchdog` all `success`). I did not build it locally.

**Learned:**

- **When nothing in BACKLOG, decisions or PLAN has moved, one `git diff --stat` against the last walked sha settles STEP 2.** I re-listed the Status column anyway as a check, and it confirmed that result.

**Not verified:** nothing was built or run, because there was no item. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half), which the prompt states as **2026-11-07**, 29 days from today.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean, and never left it. The work was in a scratchpad `git worktree`, which I removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. No GUI was used, no screenshot was taken, and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) #643 is waiting on you by design. (2) E93 needs your scope call. (3) A ruling on the `Plat`-correction `PROPOSED:` entry would let a run re-label E92. (4) The bot token expires in four weeks. Until one of these moves, mac runs will keep producing no-item entries like this one.

**Branch/PR:** `tide/mac/2026-10-09-queue-blocked`. This entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-08 — macos — no item: STEP 1 and 1.5 empty, STEP 2 re-walked and nothing is eligible for `mac`; E92 is Linux-only by its own guard (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.26454.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. The queue is blocked for this lane, and this entry says why, item by item, so the next run can check the reasons instead of inheriting them. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`5175b02..7e33c2e`).

**STEP 1:** there is no open `platform:mac` issue. The open issues are #583 (linux, `tide-rack-bot`, already triaged as E89) and #44 (the digest).

**STEP 1.5:** this lane has one open PR, [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91's probe). All its checks pass, and it has no reviews and no comments. Under STEP 1.5 it is waiting for merge, so I left it alone. It is `tests/` only and so allowlist-blocked by design. It is also the only open PR in the fleet.

**STEP 2, re-derived rather than inherited.** Since the 10-07 walk, `main` gained only that run's bookkeeping (#644) and Jeff's `7e33c2e` (path-tracer core cap, `modules/common/TidePathTracer.*`). `git diff 5175b02 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only the E91 flip, the new E93 row and the `mac` cell. So every parking reason below rests on unchanged text, which I read again:

| row | why not this run |
|---|---|
| A35, A37, A42 | parked on their own open `PROPOSED:` entries or, for A42, by its own words. The five `PROPOSED:` entries in `docs/decisions.md` are unchanged. |
| S8 | `NEEDS-SPEC` is the first thing in its row |
| E19 | Accept needs a real host with ≥60 s of watched animation and a VCV context-menu toggle. That is a screen, and a scheduled run cannot drive one. |
| E82 | Accept is a right-click on a VCV panel, which also needs a screen |
| E2 | an umbrella; it says of itself it is not takeable |
| E84 | a `.github/workflows/**` edit, which this credential cannot make |
| X2, E89 | `linux` |
| **E92** | **`any` on paper, Linux-only in fact; see below** |
| E93 | `mac`, but its first Accept branch is Jeff's scope ruling (AU2 is out of TIDE since S40). Fixing it is not identical under the WONTFIX answer. The 10-07 cell also says not to build an AU2 consumer unasked. |

**E92 cannot be taken or measured on macOS, and no previous cell said so.** The 10-07 cell's walk did not mention it. `se_gmpi/vst3/CMakeLists.txt` opens with `if(NOT UNIX OR APPLE) return()`, ahead of the `gmpi_wrappers_POPULATED` guard the row is about. On this box, `SynthEditTemplate_VST3` therefore does not exist under **either** variable. E92's Accept (`ninja -t targets all | grep -c SynthEditTemplate_VST3` non-zero in both configures) is unsatisfiable by construction on mac and win. I read this from SE16 `master`. I did not configure it to measure it. I did not edit E92's `Plat` cell, because whether a run may correct a `Plat` cell after filing is itself an open `PROPOSED:` entry.

**Learned:**

- **Check a row's platform guards before you trust its `Plat` cell.** E92 says `any`, but the target it measures sits behind `if(NOT UNIX OR APPLE) return()`, three lines above the guard the row describes. Reading the first twenty lines of the file the Accept cites is enough to settle it.
- **Neither `claude` nor its CLI version is on this box's `PATH` for a scheduled run.** The app version came from `/Applications/Claude.app`'s `Info.plist` (`CFBundleShortVersionString`).

**Not verified:** nothing was built or run, because there was no item. I did not configure SE16 to measure E92's target count on mac. The claim above is a reading of the guard. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half).

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean, and never left it. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was read only. No GUI, no screen taken, and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) #643 waits on you by design. (2) E93 wants a scope call. (3) E92 is a linux job in practice. A ruling on the `Plat`-correction `PROPOSED:` entry would let a run re-label it. Until something moves, mac runs will keep producing entries like this one.

**Branch/PR:** `tide/mac/2026-10-08-queue-blocked`. This entry, the `mac` cell, the rotation and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.
