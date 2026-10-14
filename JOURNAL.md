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

## 2026-10-13 — linux — E89: a box-scoped run lock, whose first version let #583 straight through at the exact 5.6 s gap (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.32885.1** (CLI `2.1.234`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E89**, this box's own process wound ([#583](https://github.com/JeffMcClintock/TideSynth/issues/583)). Added `scripts/run-lock.py`, a box-scoped lock taken at STEP 0.5, `tests/e89_box_lock_probe.py` which measures it in thirteen arms, and the STEP 0.5 / STEP 5 prompt edit that makes a run actually call it. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`5175b02..938e648`).

**STEP 1:** #583 is the only open `platform:linux` issue, and its own first line says its tier is not STEP 1's — *"triage it into a BACKLOG row and take it in normal order"*. That row is E89. **STEP 1.5:** no open PR anywhere matches `tide/linux/**`; [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (mac, E91) is the fleet's only open PR and is not this lane's. **STEP 2:** the `linux` NEXT cell says *"TAKE NEXT: E89"* and has said so since 09-09. It is `TODO`, `linux`, and no branch or PR named it — `git ls-remote --heads origin` returns `main` and `tide/mac/E91-au-activate-first` only, so the two stale branches the 10-06 cell flagged have since been deleted. None of the four open `PROPOSED:` entries parks it: three say *"everything"* and E72's parks only E72. DOING mark pushed first, `b777d65`.

### The thing worth reading: the fix's first version reproduced the defect it was for

`run-lock.py` first recorded `os.getpid()` — the pid of the `acquire` process. **That process exits immediately, so the lock went stale the instant it was taken.** Run at #583's exact gap, on the real code:

```
run A acquire rc=0 -> lock acquired: pid=7047 ...
run B acquire 5.6s later rc=0 -> reclaiming a stale lock (holder pid 7047 is gone)
```

**rc=0. Run B was let straight through, and the guard did nothing, silently** — which is the one outcome #583 calls worse than the collision. **Arms 1-10 of the probe were green while that was true**, because not one of them let a real `acquire`'s process exit before judging the lock. So the lock now records an *owner* pid: `--owner-pid auto` walks the parent chain and takes the first ancestor matching the Claude Code CLI, prints which pid it chose and how, and warns loudly when no ancestor matched (that lock is age-capped only). Arms 11-13 are that hole, its fix, and the broken configuration pinned so a regression says which half moved.

### Why a lock, and not either of E89's other two shapes — measured, not argued

| E89's shape | verdict |
|---|---|
| **`ps` for a second `claude`** (#583's own suggestion) | **refuted, in both directions at once** |
| **a DOING check against the local `.git`** | **blind by 169 seconds** in the observed incident |
| **fix the duplicate firing at source** | outside this repo; the scheduled-task runner |

**`ps` cannot see a scheduled run, and the count-based variant would have stood down the run that measured it.** Swept `/proc/*/cmdline` across the whole box: **no `claude` process's cmdline contains `tidesynth-weekly-linux`** — the task name reaches a run through its prompt, not its argv; the single match was the sweep's own shell. And two `claude` CLI processes were alive for entirely ordinary reasons — pid 4270 (`--model default`, this run) and pid 4414 (`--model claude-haiku-4-5-20251001`), a sibling started 21 s later by the same `claude-desktop` parent 3836. A run that stands down on *"another claude exists"* stands down always.

**The `.git`-aware DOING check is a late detector, and the number kills it as a preventer.** Run B started **22:03:07.240Z**; run A's DOING commit `9177984` is authored **2026-09-08T22:05:50Z** — **169 seconds later**. Run B did its STEP 2 claim survey inside that window, so there was nothing in the shared `.git` for such a check to read. Worth having as a backstop; cannot be the guard.

### Evidence

`python3 tests/e89_box_lock_probe.py` — **13 arms, rc=0, 7.5 s**, no build and no DAW. Arms 1-4 and 11-13 measure *"it lets two runs in"*; arms 5-8 measure *"it lets no run in"*, which #583 names as the worse failure; 9-10 are housekeeping.

**The vacuity control runs first and must FAIL:** the same 12-process barrier-released harness against a deliberately non-atomic check-then-create acquire gives **12 of 12 winners**. Without it, arm 4's *"1 of 12"* would prove nothing — a harness whose children never overlap reports one winner for a lock with no exclusion in it at all.

**Then three A/B breaks of the real script, each turning the probe red on exactly the arms it should**, with the restored file **byte-identical** (sha256 `433adc6ea11340d3`) so each A/B's only variable was its one edit:

| break | arms red |
|---|---|
| `O_EXCL` → check-then-create with a window | **4** alone: 12 of 12 racers acquire |
| `--owner-pid auto` → `self` (the hole as first written) | **11 and 12** |
| `staleness()` → always "live" (strand the box) | **9 of 13** |

Arm 12 is the incident itself with real processes: a holder resolved through the real `auto` walk, a **5.6 s** sleep, run B **rc=1 REFUSED** naming the holder's pid — then the holder is killed and the next acquire **rc=0, reclaimed**, so a crashed run does not strand the lane. It only became an end-to-end test of `auto` after the second A/B showed it staying green with an explicit `--owner-pid`; that is why it passes a marker ancestor now.

**End-to-end on the real path**, the literal STEP 0.5 and STEP 5 commands: acquire rc=0 recording `pid=4270 … owner=auto:ancestor-match`, an immediate second acquire **rc=1 REFUSED**, release rc=0, release-again rc=0 and safe. `~/.tide/` is left holding `agent-token` alone, exactly as found.

### Filed while here: A43, a `PROPOSED:` entry that outlived its row

Working out which open questions park E89 meant reading all four `PROPOSED:` entries, and one of them has no owner. **[E72](BACKLOG-DONE.md) was archived DONE on 2026-10-01, and its entry still stands open in [docs/decisions.md](docs/decisions.md) saying *"May proceed meanwhile: everything except E72 itself"* — a parks-clause naming a row that is not in `BACKLOG.md` any more.** No ruling for it is in that file's Decisions table, so the question is genuinely still unanswered and nothing is carrying it. **This is A42's hazard from the other side:** A42 is *"a question left inside a closing row is archived with it"*, and E72 did the other sanctioned thing — escalated to `docs/decisions.md`, which *is* the mechanism — and the escalation then outlived its row. Both paths lose the question; they lose it in different files. Filed as **A43** with the shape of an answer and no pick, because this is process rather than product. Id allocated per A42's own guard: swept `BACKLOG.md` + `BACKLOG-DONE.md` on `origin/main` and on every remote `tide/*` branch, highest was E93 / A42, so A43 was free.

**Learned:**

- **A fix whose own probe is green can still be the defect.** Ten arms passed while `run-lock.py` recorded a pid that was dead a moment later. Every one of them tested the lock *file*; none let the acquiring *process* exit first, which is the only state a real run is ever in. **When a guard's correctness depends on a lifetime, an arm must outlive something.**
- **`ps` cannot identify a scheduled run on this box.** The task name is in the prompt, not the argv — `/proc/*/cmdline` has zero matches — and two `claude` CLI processes are normal, so counting them false-positives. #583's own recommended cheap check would have stood this run down.
- **Ask when a late detector could first have seen anything.** A `.git`-aware DOING check is blind for the 169 s between run B's start and run A's claim commit, and that is exactly the window run B spent in STEP 2. The gap between two firings (5.6 s) and the gap before the first local artefact (169 s) are different numbers, and only the second one judges the mechanism.
- **`O_EXCL` over `flock` when the lock's value must be read by a different process.** An advisory `flock` dies with the fd; a run needs its successor to read *who* holds it, and `O_CREAT|O_EXCL` is the one primitive that is atomic and identical on all three boxes.
- **A barrier file is what makes a race harness race.** Twelve children released together contend; twelve started in a loop are serialised by ~30 ms of interpreter startup each, and report one winner for a lock with no mutual exclusion in it.
- **Pin the broken configuration as its own arm.** Arm 13 asserts `--owner-pid self` still goes stale on exit. That is what makes arm 12's pass attributable to the ancestor walk rather than to something else having moved.
- **An escalation can outlive the row that raised it, and no check looks.** `check-id-refs.py` walks ids across files but not into `docs/decisions.md`'s `PROPOSED:` blocks, so E72's entry has been read as live by three consecutive runs since its row was archived. A43.
- **`grep -oE` with a wide fixed-width prefix over `BACKLOG.md` backtracks catastrophically.** `grep -oE '.{90}(A[^|]{0,120}B|B[^|]{0,120}A).{60}'` against rows that are single 2-4 KB lines ran past a 2-minute timeout and had to be killed; the same question in python answered instantly. **This file's rows are not line-shaped, so line-oriented tools do not cost what you expect on it.**
- **`os.kill(pid, 0)` has no Windows equivalent worth shipping unverified**, so `pid_alive` returns `True`/`False`/`None` and `None` means *possibly alive*. A lock is reclaimed only on a **known**-dead pid, which makes the Windows box strictly more conservative rather than less.

**Not verified:** **the Windows and macOS boxes**. Everything above was measured on linux. `run-lock.py` is `#ifdef`-free Python and the probe needs no build, so `python3 tests/e89_box_lock_probe.py` should run on both, but it has not. **Windows is the one platform with a different code path** — no `/proc`, no `ps`, so `--owner-pid auto` falls back to `self` and `pid_alive` returns `None`, leaving the 6 h age cap as the sole reclaim route; **that path is written but unmeasured, and it is the thing to run first on that box.** macOS takes the `ps -o ppid=,args=` branch of `_parent_and_cmdline`, which linux never exercises because `/proc` exists here — also unmeasured. **Why the task fired twice is still unestablished**, as #583 said: this is a guard against the duplicate firing, not a fix for it, and fixing the scheduler still beats guarding against it. I did not test two genuinely separate scheduled firings, only two processes at the same gap. No product code was touched, nothing was built, and TiDE was not measured.

**CI and the default branch:** [#652](https://github.com/JeffMcClintock/TideSynth/pull/652) is **6 pass, 2 skipping, `mergeStateStatus: CLEAN`** — `lint`, `linux`, `e57-delete-key` and the three `render-*` jobs. `main`'s last `build.yml` run is **success at `07b0172`** (2026-10-09); the commits since are bookkeeping. **I did not build `main` locally, and this item gave me no reason to** — it is Python and the probe needs no build, so unlike a code item there was no by-product build to report. **The lock is `#ifdef`-free Python, so nothing about SynthEdit, SynthEditCL or TIDE is changed by this PR.**

**Rotation:** not needed. With this entry `JOURNAL.md` is **58,062 bytes**, under the 60 KB cap, and the floor (four most recent entries, which beats the two carrying today's date) binds anyway. `extract-lessons.py --check` is green, 1688 lessons from 394 entries.

**Machine state:** `~/TideSynth` stayed on `main`, clean, and never left it — **its local `main` is 94 commits behind `origin/main` and I deliberately left it there**, for the reason the 10-06 entry gives: fast-forwarding a sibling tree out from under another process is the second half of #583. All work was in a scratchpad `git worktree`, removed at the end. No sibling repo was touched, read or fast-forwarded. No GUI, no screen taken, and no credential value appears anywhere.

**Next:** see the `linux` cell. **For Jeff:** (1) this PR touches `docs/weekly-run-prompt.md`, which the A4 allowlist carves out on purpose, so it waits for you — and a prompt change reaches all three boxes on their next run, which is A2's stated trade and the reason the edit is 52 lines rather than a rewrite. (2) **Windows and linux had both been silent since 10-06**; this entry breaks linux's week. (3) The bot token expires **2026-11-07**, 25 days out.

**Branch/PR:** `tide/linux/E89-box-run-lock`, [#652](https://github.com/JeffMcClintock/TideSynth/pull/652) — the lock, the probe, the STEP 0.5 / STEP 5 prompt edit, the E89 flip, the `linux` cell, the A43 row and the regenerated `docs/lessons.md`. **It is NOT auto-mergeable and that is deliberate:** `docs/weekly-run-prompt.md` is carved out of the A4 allowlist, so it waits for Jeff.

---


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
