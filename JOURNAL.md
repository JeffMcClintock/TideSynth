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

## 2026-10-29 — macos — no item: #671 merged and E19 is free again, but its only mac clause is AU3 in a real host and that still needs Jeff's AUv3 registration displaced (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **4.29.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. I re-pointed the `mac` NEXT cell and rotated one entry out. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`fd8fd5b..f4e3a61`). `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: there are no open `platform:mac` issues. STEP 1.5: there are no open PRs at all. Remote heads are `main` and Jeff's `gpu-backend`. STEP 2: [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) merged (`44e6949`), so E19 is `TODO`, `any`, and no longer claimed by an open PR. `git diff --stat fd8fd5b origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md` only: the `mac` and `linux` cells, the E19 row's new linux text, and E82 leaving for `BACKLOG-DONE.md`. `decisions.md` and `PLAN.md` are unchanged, so the rest of the 10-27 walk stands.

- **E19:** the row's Accept gives mac one cell: *"AU3 in a real host, not just auval"*. `pluginkit -m -v` still shows only the 2026-08-31 `com.tidesynth.tiderack.au3app.extension` 0.1.1 at `~/Applications/TIDE-Rack-AUv3.app`. Measuring a current build means registering a new appex over Jeff's, and every mac cell since 10-25 has left that for when he is present. The row's other open clauses (bool, linux/CLAP, win/VST3) are not mac's.
- **E93:** not built unasked. **The other `any` rows** stay parked for the reasons the 10-26 windows entry lists.

Since 10-28 Jeff also landed product work on `main` directly (the Oscillator prefab's waveform slider switch and its SVG, `SynthEditSem/CMakeLists.txt` staging it into the bundles). No row names it, so nothing here acts on it.

**Learned:**

- **A merged claim does not make a row reachable.** E19 stopped being taken today, but the mac cell was never blocked by the claim. It is blocked by the AUv3 registration, which is a separate thing.

**Not verified:** nothing was built or run, so "`main` builds on mac" after Jeff's `SynthEditSem/CMakeLists.txt` change is CI's claim, not mine. The box was idle (HID idle 213,409 s, about 59 h). I read `BACKLOG.md`, `docs/lessons.md` and `docs/decisions.md` only where they bear on this walk. The bot token expires **2026-11-07**, 9 days from today. I did not re-check that date against the credential.

**Rotation:** done, one entry. With this entry `JOURNAL.md` was over 60 KB. I moved the oldest entry (10-20 windows) to the end of [JOURNAL-2026-10.md](JOURNAL-2026-10.md), verbatim, which brought the file back under 60 KB (57,327 bytes) with nine entries. `extract-lessons.py --check` was green on the pristine worktree before any edit (1,736 lessons from 406 entries). I regenerated `docs/lessons.md` afterwards.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) E19's mac AU3 cell needs you present, or a word that a run may register a fresh AUv3 appex over the 08-31 one. (2) The bot token expires 2026-11-07, 9 days out.

**Branch/PR:** `tide/mac/noitem-2026-10-29`. It changes the journal, the rotation, the `mac` NEXT cell and `docs/lessons.md`.

## 2026-10-28 — macos — no item: nothing moved since the 10-27 walk except this lane's own cell, and #671 is now CONFLICTING (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **3.6556.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. I re-pointed the `mac` NEXT cell. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`3b63f57..fd8fd5b`). `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: there are no open `platform:mac` issues. STEP 1.5: there is no open PR on `tide/mac/**`. STEP 2: the only commit since `3b63f57` is the 10-27 mac bookkeeping merge (#673), and `git diff --stat 3b63f57 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows one line, the `mac` cell. So the 10-27 walk stands row for row. E19 is taken by #671, E93 stays unbuilt unasked, and the `any` rows are parked for the reasons the 10-26 windows entry gives. Nothing is eligible.

**One new fact:** [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) (`tide/linux/E19-vst3-linux-recheck`) still passes its checks (13 pass, 2 skipping), but GitHub now reports it `mergeable: CONFLICTING`. The 10-26 and 10-27 bookkeeping merges both said they would conflict with it at the top of `JOURNAL.md`, and that has now happened. It is the linux lane's PR, so STEP 1.5 hands it to that lane, not this one. I did not touch it.

**Learned:**

- **Each no-item bookkeeping PR that merges ahead of a waiting PR adds to that PR's conflict.** Three lanes prepending to `JOURNAL.md` while #671 waits on Jeff is what turned it from green to `CONFLICTING`.

**Not verified:** nothing was built or run. The box was idle (HID idle 127,010 s, about 35 h), but `pluginkit -m -v` still shows only the 2026-08-31 `com.tidesynth.tiderack.au3app.extension` 0.1.1, so E19's mac AU3 cell is still out of reach even once #671 merges. I did not read `docs/lessons.md` or `BACKLOG.md` in full. The bot token expires **2026-11-07**, 10 days from today. I did not re-check that date.

**Rotation:** not needed. `JOURNAL.md` is 59,824 bytes with this entry, just under 60 KB. The next entry will have to rotate.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) #671 now needs a rebase by the linux lane before it can merge. (2) E19's mac AU3 cell needs you present. (3) The bot token expires 2026-11-07, 10 days out.

**Branch/PR:** `tide/mac/noitem-2026-10-28`. It changes the journal, the `mac` NEXT cell and `docs/lessons.md`. It will conflict with #671 at the top of `JOURNAL.md`.

## 2026-10-27 — macos — no item: E82 merged, E19 claimed by the linux lane on #671, and nothing else is reachable by `mac` (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **3.6556.2** (CLI version not readable, no `claude` on PATH) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. I re-pointed the `mac` NEXT cell, which still said E82 was in review. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`09206a2..3b63f57`). `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: there are no open `platform:mac` issues. STEP 1.5: there is no open PR on `tide/mac/**`. The only open PR is [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) (`tide/linux/E19-vst3-linux-recheck`), and it is green, so I left it alone. STEP 2: `git diff --stat 5a24534 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only `BACKLOG.md`, three lines: the `win` and `mac` NEXT cells and E82 moving `TODO`→`IN-REVIEW`. So the 10-26 walk stands row for row:

- **E82:** merged ([#669](https://github.com/JeffMcClintock/TideSynth/pull/669), `09206a242`). Its flip and archive are already on #671 (it edits the E82 row and `BACKLOG-DONE.md`). Doing them here would only add a conflict, so I left them to #671.
- **E19:** taken. #671 is an open PR from a different platform that names the id. Its mac AU3 cell needs Jeff's AUv3 registration displaced anyway. `pluginkit -m -v` still shows only the 2026-08-31 `com.tidesynth.tiderack.au3app.extension` 0.1.1. The box was idle (HID idle 40,616 s), but that does not unlock this.
- **E93:** not built unasked. Its Accept is either a WONTFIX ruling or a non-TIDE AU2 consumer, and neither is a run's call.
- **The other `any` rows** (A35, A37, A42, A43, A45, S8, E2, E84) are parked for the reasons the 10-26 windows entry lists. `decisions.md` and `PLAN.md` are unchanged.

**Learned:**

- **When a merged PR's bookkeeping flip already rides on another platform's open PR, leave it there.** Doing the flip twice only adds a conflict on the most contended rows.

**Not verified:** nothing was built or run, so "`main` builds on mac" is CI's claim, not mine. I read `BACKLOG.md`, `docs/lessons.md` and `docs/decisions.md` only where they bear on this walk, not in full. I did not verify #671's measurements. The bot token expires **2026-11-07**, 11 days from today. I did not re-check that date.

**Rotation:** not needed. `JOURNAL.md` is still under 60 KB with this entry.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) #671 is green and waiting on you. Merging it also flips E82 to DONE. (2) E19's mac AU3 cell needs you present. (3) The bot token expires 2026-11-07.

**Branch/PR:** `tide/mac/noitem-2026-10-27`. It changes the journal, the `mac` NEXT cell and `docs/lessons.md`. It will conflict with #671 at the top of `JOURNAL.md`.

## 2026-10-26 — windows — no item: this lane finally got the idle evening box it has been blocked on for five runs, and the queue had nothing left for it — E19 was claimed by the linux lane nine hours earlier. What delivered the idle box was a **13-hour clock error** (scheduled run)

**Prompt:** 18210767f · Opus 5, `claude-opus-5` · app Claude desktop **3.6556.2** (CLI version not readable — no `claude` on PATH and `~/.claude/versions` is empty on this box) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item — nothing is eligible for `win`, and the walk below is derived this run rather than inherited. Nothing was built and no product code ran. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`20a60580f..09206a242`). `run-lock.py acquire` rc=0, pid 25272, `owner=self (NO agent ancestor matched -- this lock is age-capped only)` — the documented Windows path, printing the warning it is supposed to print.

**STEP 1:** no open `platform:win` issue, and on this platform that verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing, as six previous cells record. The only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, triaged as E89, DONE and archived) and #44 (the digest). **STEP 1.5:** no open PR on `tide/win/**`. One PR is open in the fleet and it is not mine: [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) (`tide/linux/E19-vst3-linux-recheck`), every check green, `tests/**` so off the auto-merge allowlist and waiting for Jeff by design. Green with nothing unresolved is not mine to touch, so I left it alone.

### The finding: the box was unattended for the first time in this lane's history, and the queue was empty anyway

Five windows runs in a row (09-14, 09-16, 10-13, 10-20 and this one) have ended with no item, and every one of the first four gave the same cause: **E19 and E82 were the only two rows this lane could take, both need an idle box, and the box was never idle.** The 10-20 entry measured why — the Monday 20:06 local slot is missed and the catch-up fires at Claude app launch, which is by construction when the developer sits down.

**This run had the idle box.** `GetLastInputInfo` read **18,371 s — 5 h 06 m 11 s** at 22:59:04 local, so the last input was about **17:52:53**. One `devenv` and three `VirtualBoxVM` processes are up, both since early afternoon, and neither has been touched since. Nobody is at this machine.

**And there was nothing to do with it, because the other two boxes finished both rows first.** E82's Accept was met by the mac lane on 10-25 ([#669](https://github.com/JeffMcClintock/TideSynth/pull/669), merged 2026-10-25T22:55Z as `09206a242`), and E19 was claimed by the linux lane at **2026-10-26T00:20Z**, **9 h 36 m before this run started**. So the lane's blocker has changed shape rather than cleared: it is no longer *"the box is never idle"*, it is *"this lane is awake so rarely that the two rows reserved for an idle box were completed by the boxes that run daily."* That is a scheduling consequence and it belongs to **A45**, which is already filed and already waiting on Jeff; I filed nothing, per the 10-23 mac precedent of not re-filing A45's own evidence as a new row.

### What delivered the idle box: a 13-hour system clock error, corrected 2 m 21 s before this run fired

This run started **09:56:34Z = 22:56:34 Monday local**. Its slot was `nextRunAt 2026-10-26T07:06:36Z` = **20:06 Mon local** (recorded by the 10-20 entry; `nextRunAt` is now `2026-11-02T07:06:36.000Z`, the same seconds, which is what confirms the slot was not re-pointed). So the run is **2 h 49 m 58 s late**, and `jitterSeconds 396` — 6 m 36 s at most — cannot cover that.

**The mechanism is in the system event log, and it is not app launch this time.** `Microsoft-Windows-Kernel-General` **event 1** (system time change) fired twice at 22:54:13 local, and the material one says:

| field | value |
|---|---|
| `OldTime` | `2026-10-25T20:54:12.586Z` |
| `NewTime` | `2026-10-26T09:54:13.792Z` |
| `TimeDeltaInMs` | **46801206** = **13 h 00 m 01.2 s forward** |
| `ProcessName` | `svchost.exe` (w32time), `Reason 1` |
| `TimeZoneBias` | `-780` (UTC+13, `New Zealand Standard Time` + DST) |
| `RealTimeIsUniversal` | `false` |

`w32tm /query /status` agrees and dates it: **`Last Successful Sync Time: 26/10/2026 9:54:12 am`** — that stamp is in the *pre-jump* clock, i.e. real 22:54:12 — with `Leap Indicator: 3(not synchronized)` and `Root Dispersion: 46808.97s`, which is the size of the jump.

So **the clock came up about 13 h slow at boot and stayed that way for roughly nine hours.** Boot is `Kernel-General` event 12 stamped `00:47:29`, and `Win32_OperatingSystem.LastBootUpTime` — computed from the *corrected* clock — reads **13:47:29**: the same instant, 13 h apart, which is the cleanest proof of the skew. Why it came up slow, and why w32time's first successful sync took nine hours, are **not established** here. A `RealTimeIsUniversal=false` machine whose RTC holds UTC is the obvious candidate and the three running VirtualBox VMs are another; I measured neither, and changing a system clock setting is not a run's to do.

**The slot and the correction are the same event.** Under the slow clock, local time read 07:06 Monday at the real 20:06 slot, so the cron's `0 20 * * 1` had not come round; it would next have matched at real **09:06 Tuesday local** — another catch-up into the developer's working morning, exactly the 10-20 pattern. The 22:54:13 correction made the Monday 20:06 slot past-due instead, and the scheduler fired the catch-up **2 m 21 s later**. A clock error is what bought this lane its first evening run.

### Three instrument notes, because this box's timestamps are now known to lie

- **Tick-based readings survived the jump and wall-clock stamps did not.** `GetLastInputInfo`/`GetTickCount` measure since boot, so the 5 h 06 m idle above is unaffected. Every absolute stamp this box recorded between 13:47 and 22:54 real time — process `StartTime`, the event log, anything a run or the digest wrote — is **13 h behind**. Relative orderings inside that window are intact, because the offset is uniform.
- **The 10-20 entry's conclusions survive this, and it is worth saying why rather than leaving it hanging.** Its evidence was relative: app launch 09:10:39, the run 09:10:45 six seconds later, every other process 09:10:26–09:10:59, `GetLastInputInfo` 1 s. A uniform skew changes none of that. What a skew *can* corrupt is the one comparison that mixes sources — server-side `lastRunAt`/`nextRunAt` against a locally stamped event — which is precisely the comparison A45 rests on. Use the server-side pair on both sides where you can.
- **`quser` printed `IDLE TIME: none` for the console session while `GetLastInputInfo` said five hours.** Do not use `quser` for this question on this box.

### STEP 2, walked rather than inherited

`git diff --stat 20a60580f origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md` only, four lines: the `win` and `mac` NEXT cells, A45 filed, and E82 moved `TODO`→`IN-REVIEW`. `decisions.md` and `PLAN.md` have not changed since the 10-20 walk. Eleven rows are `TODO` and reachable by `win` or `any`, and every one is parked:

- **E19** is **taken**. [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) is an open PR from a different platform naming the id, which STEP 2 settles in one sentence — and the collision is real rather than nominal: that PR rewrites the whole E19 row and the `linux` cell, so a second branch editing it would conflict in the row it is reporting on. **This is the row this run would otherwise have taken**, and it is the only one the idle box above would have unlocked.
- **E82** is `IN-REVIEW` on `origin/main` with [#669](https://github.com/JeffMcClintock/TideSynth/pull/669) merged, so STEP 4's mechanical flip reaches it — **but #671 already carries that flip and archive.** Doing it here would be a second edit to `BACKLOG.md`'s and `BACKLOG-DONE.md`'s most contended rows for no gain. Left to #671.
- **A35** says in terms it is *"PARKED ON ITS OWN TWO `PROPOSED:` ENTRIES … NOT AVAILABLE TO TAKE"*; both are still open.
- **A37** is parked on its own Accept, which asks for *"a stated ceiling"* and an archive convention that does not exist yet — and its own Size line says the convention question *"may want Jeff rather than a run"*. Under-specified by STEP 2's test, as the 10-13 and 10-20 runs read it.
- **A42** is ineligible by its own words: *"(b) without (a) is not identical under every answer"*.
- **A43** is the same shape and says *"a run should not pick"*.
- **A45** is this lane's own scheduling row and picks nothing deliberately; every option is about Jeff's hours and his box.
- **S8** carries `NEEDS-SPEC` first in its row.
- **E2** is an umbrella that says of itself it is not takeable.
- **E84** is a `.github/workflows/**` edit this credential deliberately cannot make.
- **X2** is `linux`, **E93** is `mac`.

All four open `PROPOSED:` entries in `docs/decisions.md` were read: they say *"May proceed meanwhile: everything"*, *"ALL ordinary work, without exception"*, *"everything except E72 itself"* (archived) and *"EVERYTHING, without exception"*. **None parks a row by its own clause**, so A35 and A37 are parked by their rows' own Accepts, as before. The three `DONE-PENDING-ACCEPT` rows (E7, E44, E49) await an acceptance judgement rather than a merge, so STEP 4's rule does not reach them.

**Default branch:** not built here — there was no item. CI's reading is green: `build` **success** at `09206a242`, the current `main` head, and #671's own `windows` job passed in 11 m 19 s against the same content.

**Learned:**

- **A workstation's clock can be hours wrong for most of a day, and then a routine NTP sync becomes a scheduler event.** The correction made a missed slot past-due and the catch-up fired 2 m 21 s later. Any conclusion of the form *"the task fired at X"* on this box should be checked against `Kernel-General` event 1 over the same window before it is believed.
- **`Win32_OperatingSystem.LastBootUpTime` and the boot entry in the event log disagreeing by exactly a timezone offset is the cheapest possible detector for this.** One is computed from the current clock and the other was stamped by the old one; the gap between them *is* the skew, with no NTP query needed.
- **Prefer tick-based instruments to wall-clock ones for "is anybody there".** `GetLastInputInfo` is immune to a clock jump by construction, and it was the only reading this run did not have to re-derive. `quser`'s `IDLE TIME` column is useless here — it said `none` against a measured five hours.
- **A row reserved for a condition one lane almost never meets will be finished by a lane that meets it often.** E19 and E82 were this lane's only two rows for five runs; by the time the idle box arrived, the mac box had closed E82 and the linux box had claimed E19 nine hours earlier. Reserving work for a rare condition is not the same as reserving it for the box that has the condition.
- **A full stop in STEP 2 can be the correct outcome of a *successful* run.** Every guard passed, the one structural blocker cleared for the first time, and the answer was still "nothing is eligible" — because availability and eligibility are different properties and only one of them improved.

**Not verified:** **nothing was built, compiled or run as product code**, so *"`main` builds on Windows"* is CI's claim and not mine, and I did not read the build logs — only the run conclusions. **No GUI was taken, no window created and no screenshot made**, even though the box was idle: the one row that wanted a window is taken by #671, and preparing a taken item's measurement is still working on a taken item. **The cause of the 13 h clock skew is not established** — RTC-as-UTC and the VirtualBox VMs are candidates I did not test, and I changed no system setting. **Why w32time's first successful sync took about nine hours is likewise unestablished.** I did not re-read `docs/lessons.md` in full (176 KB), only the parts bearing on this walk. **I did not verify #671's measurements**, only its state and its diff. The bot token expires **2026-11-07**, **12 days** from today; I did not re-check that date against the credential.

**Rotation:** done, and it took **two** entries rather than one. With this entry `JOURNAL.md` was 71,733 bytes; moving the oldest (10-14 windows, E89/box-lock) left 63,605, still over the 60 KB stop, so the next oldest (10-16 linux, E92) went too, leaving **52,936 bytes and six entries** — above the four-entry floor, which is what the rule stops on. Both were appended to [JOURNAL-2026-10.md](JOURNAL-2026-10.md) below what was there, verbatim and in date order (that file is ascending in practice, whatever the header's *"newest-first"* says — its tail was already the other 10-14 entry). `extract-lessons.py --check` was green on a pristine `origin/main` worktree before any edit (1,725 lessons from 402 entries) and is green again after the regeneration (1,730 from 403) — measured on a separate checkout, because running it after the prepend only tells you your own edit is unwritten. **#671 rotates the 10-14 entry too and stops there**, so our first archive hunk is identical and the second is mine alone; expect to re-resolve the tail of the archive as well as the top of `JOURNAL.md`.

**Machine state:** `C:\SE\TideSynth` **stayed on `main` and was never checked out** — all work was in one scratchpad `git worktree`, removed and pruned at the end. The tree was clean on arrival. No other repo was touched, read or fetched: `C:\SE\SE16`, `C:\SE\SynthEditLib`, `C:\SE\gmpi_ui`, `C:\SE\GMPI_Wrappers` and `C:\SE\GMPI` were not opened, because no item needed them — and the developer has a Visual Studio instance open, so not touching them was also the right call independent of the queue. No credential value appears anywhere. Box lock released as the last act.

**Next:** see the `win` cell. **For Jeff, four things:** (1) **This box's clock was 13 hours slow from boot at 13:47 until w32time corrected it at 22:54** — worth a look, because it will recur on every boot if the cause is the RTC/UTC setting, and while it lasts every timestamp this box writes into the fleet's own records is 13 h out. (2) **A45 is still the one row waiting on you**, and this run sharpens it rather than answering it: the idle box arrived and the queue was empty, because the two rows held for it were finished by the two boxes that run daily. (3) [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) is green and waiting on you, and **E19's win/VST3 cell is the next win item the moment that PR merges** — its two owed clauses (the rack-canvas pixel diff and `int/bool/enum`) now have a producer and a recipe from #669's `WT LFO` probe. (4) The bot token expires **2026-11-07**, 12 days out.

**Branch/PR:** `tide/win/2026-10-26-queue-blocked`. This entry, the `win` cell, the rotation and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge. It will conflict with #671 at the top of `JOURNAL.md`, and at the tail of `JOURNAL-2026-10.md` because this rotation moved one entry more than #671's.

## 2026-10-26 — linux — E19: the linux VST3 cell passes animation, pixel diff and the int/enum menu clause in REAPER (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop (version not read; CLI `2.1.220`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E19's linux/VST3 cell** and re-measured it for the first time since 2026-08-31, when it failed because the editor was bound to a retired processor. Every blocker that cell named has closed since: E74 (binding), E75 (visible fixture), E83 (cabled Scope), and E82 gave the menu clause an instrument. No product code changed. `tests/e19-host-feedback/e19x.c` gained a `drag` verb and the harness README gained the recipe and the numbers. **E82 flipped to DONE and was archived:** #669 merged 2026-10-25 22:55Z, and `gh pr view` confirms it. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`6538c2b..09206a2`). `run-lock.py acquire` returned rc=0, `owner=auto:ancestor-match`, and reclaimed a stale lock from pid 12021, dead and 547,071 s old. That was the 10-16 run's lock. Its pid was gone, so this is the documented reclaim and not a collision.

**STEP 1:** the only open `platform:linux` issue is [#583](https://github.com/JeffMcClintock/TideSynth/issues/583). Its row, E89, is DONE, and the issue says of itself that it is not STEP 1's tier, so I walked past it. **STEP 1.5:** there are zero open PRs. Remote heads are `main` and Jeff's `gpu-backend`, which I did not touch. **STEP 2:** `git diff --stat 20a6058 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only NEXT-cell churn, E82 and A45. The 10-16 linux cell listed E19 as wanting a GUI arm via the weston recipe. That is a description, not a status: E19 is `TODO`, `any`, not blocked, and none of the open `PROPOSED:` entries parks it. I pushed the DOING mark first (`1a6b924`).

**Result:** build from `origin/main` at `09206a2`: Ninja, Release, `TIDE_VCV_FUNDAMENTAL=ON`, `-DRACK_ADAPTOR_TRACE=1`, with every dependency fetched at its own `origin/main`. **611/611, 0 errors**, 176 warnings, so `main` builds on linux. The host was REAPER **7.82** (`/opt/REAPER`, run with a scratch `HOME`) on `weston --backend=headless --xwayland`. The fixture was `e83-vcv-scope-cabled.xml`, minted through `TrackFX_SetNamedConfigParm`. `fx_ident` named the staged bundle.

| clause | readout |
|---|---|
| animation | `instance #4 feedback send #13500` / `editor received feedback #13600`; `display-state update #6350 arrived (65548 bytes)`; `light 0 update #26900 value 0.758`; the transport at `pos 220.9` at `t=221` |
| pixel diff, 65 s apart | Scope screen **1,559 / 14,300 px**, LFO panel **178**, module browser **0 / 138,000** in the same pair |
| menu, WT LFO selected | 6 rack entries plus **`Wave points >`**, with `32..16384` and **1024 ticked** |
| menu, empty canvas, deselected | the 6 entries alone |
| A/B, the fresh `LFO2` after a pick and a cable edit | **0.500142 (1024) / 0.500004 (32) / 0.500142 / 0.500004**; an edit with no pick after that gave **0.500004** |

The A/B values match E82's mac standalone (0.500143 / 0.500004), so the standalone result carries over to a hosted VST3 on another OS. The re-opened submenus show the tick on 32 and then on 1024, following each pick.

**Learned:**

- **A cold VM can make REAPER run ~44 s behind wall time, and nothing in the plug-in's trace shows it.** The first measure run logged `t=43.7 pos=0.685`, then caught up to real time, so its "75 s" window held ~32 s of transport. The relaunch was real time from t=3 s. Read `pos` against `t` before counting a window.
- **E82's rebuild trick works in a host, and its trace does not need a cable.** `first NONZERO OUTPUT` printed on arms where `LFO2 outs=0`. So any cable edit anywhere in the rack is enough to read a fresh instance.
- **Dragging from a jack that already carries a cable picks that cable up** rather than adding a second one. My edits alternated between adding and removing. Any edit rebuilds, so this cost nothing, but a run that expects cables to pile up will misread `connections`.
- **Under weston, a GMPI context menu and its submenu are separate override-redirect X windows**, 141x185 and 120x248 here. `xwininfo -root -tree` finds them, and `e19x shot`/`click` drive them by id.

**Not verified:** **the bool arm**: no bool-bearing module is in `e83`. **linux/CLAP** was not attempted. **string** still has no producer, per the 09-16 census. The pixel diff counts changed pixels. It does not judge whether the trace is right. Only REAPER 7.82 was used, not the 7.43 the harness was written against. I did not run the drag verb's change through any compile except this box's `gcc -Wall`, which was clean. Nothing was built or measured for Windows or macOS.

**Rotation:** done. With this entry `JOURNAL.md` was 63,060 bytes, so the oldest entry (10-14 windows, E89) moved to `JOURNAL-2026-10.md`, appended below what was there, verbatim. Seven entries remain, above the four-entry floor. `extract-lessons.py --write` regenerated `docs/lessons.md`, and `--check` is green after. (Before the write it reported stale, because this entry adds lessons.)

**Machine state:** `~/TideSynth` stayed on `main`. I never checked it out, and its local `main` is behind `origin` and left that way. All work was in a scratchpad `git worktree` and a scratchpad build tree, both removed at the end. REAPER ran with `HOME` in the scratchpad, and the developer's `~/.config/REAPER` is unchanged (mtime Oct 20). `~/.vst3` was not touched: the staged bundle lived in the scratch `HOME`. The weston instance on socket `tide-e19` was started by this run and stopped at the end, and no REAPER or weston process was left. No other repo was touched, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `linux` cell. The CLAP cell is the cheapest remaining linux debt. **For Jeff:** (1) E19's linux VST3 cell is measured green. Whether E19 now waits only on the mac AU3 cell, the bool arm and CLAP is your reading to make. (2) #583 is still open with E89 DONE. (3) The bot token expires **2026-11-07**, 12 days out.

**Branch/PR:** `tide/linux/E19-vst3-linux-recheck`, [#671](https://github.com/JeffMcClintock/TideSynth/pull/671). The harness `drag` verb, the README section, the E19 cell, the E82 archive, this entry, the `linux` cell and the regenerated `docs/lessons.md` are on that branch. `tests/**` is touched, so it waits for Jeff.

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

## 2026-10-25 — macos — E82: the WT LFO's `Wave points` menu is offered and the DSP obeys it — both clauses measured on the standalone with the screen locked, A/B/A/B through the adaptor's own trace (scheduled run)

**Prompt:** 1821076 · Fable 5.1, `claude-fable-5-1` · app Claude desktop **3.6556.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E82** and met its Accept. New [tests/e82_wtlfo_wave_points_probe.py](tests/e82_wtlfo_wave_points_probe.py) -- **18 checks, 0 failures, 27 s** -- plus the three bookkeeping files. **No product code changed, in this repo or any sibling.** `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`dc80d88..85a3147`, plus a new `gpu-backend` branch of Jeff's). `run-lock.py acquire` rc=0, `owner=auto:ancestor-match`, holder pid 69485.

**STEP 1:** no open `platform:mac` issue; the only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, E89 DONE) and #44 (the digest). **STEP 1.5:** zero open PRs in the fleet; `git ls-remote --heads origin` returned `main` and `gpu-backend` only. **STEP 2:** `git diff --stat dc80d88 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` showed one line in `BACKLOG.md` -- the 10-24 `mac` cell -- so the 10-23 walk stood, and the walk's one condition for E82 and E19 was an idle box. It was idle: `ioreg` HID idle **122,023 s (33.9 h)**, `pmset -g log` says the display went off **2026-10-23 17:13** and never came back, console user `jeffmcclintock`, no Xcode, SynthEdit, DAW or IDE process. E82 is `TODO`, `any`, not blocked, and none of the four open `PROPOSED:` entries parks it (three say *everything*, E72's parks E72). DOING mark pushed first, `5fc223b`.

### The row's two clauses, measured on the standalone built from `main` at `85a3147`

| step | gesture (window DIPs) | readout |
|---|---|---|
| place a WT LFO | `--pointer-down 130,294 --double` on the browser's `LFO2` | `RackProcessor: 'LFO2' constructed` -- the DSP built it |
| cable it | `--drag 590,540 592,220` (its `WT` jack to Out L) | rebuild, `connections ins=0000 outs=1`, `first NONZERO OUTPUT pin 0 (0.500143)` |
| menu, module NOT selected | `--context-menu 565,352` | **7** -- the rack's own (E38's selection fact) |
| menu, module selected | select, then the same | **20** -- the 7 plus **`Wave points`** submenu: `32 64 128 256 512 1024 2048 4096 8192 16384`, **1024 ticked** |
| control: background | deselect, `--context-menu 700,330` | **7** |
| control: unknown label | `--context-menu 565,352 NoSuchLabelXYZ` | `ok:false`, *"menu offers: Goto Rack, About TIDE..., Cut, Copy, Paste, Delete, 32, 64, ..."* |
| pick 32 | `--context-menu 565,352 32` | `invoked: "32"`, re-list shows **32 ticked** |

That is clause one: a right-click on a VCV panel offers an option the module owns. E82's own 09-14 finding said `WT LFO` would be the producer and that the five e75 probe points had all been on a module with nothing to offer; both halves hold.

### Clause two -- "toggling it changes DSP behaviour" -- and the instrument that did NOT work first

`--render-audio` was the obvious instrument and it measured nothing: **`peak 0`, `parametersUnprimed 5`** with the cable in place. The verb's own comment explains it -- *"TIDE's entire patch is one blob parameter, so this loop primes NOTHING for it and the render is built from an empty graph"* -- and names **E6** as the row it is filed on. So nothing new was filed; the lesson is to read `parametersUnprimed` before reading `peak`.

What did work is the adaptor's own trace. A menu pick writes the chosen index into the live instance in `onSetPins` (`RackAdaptor.h:594`, `m.write(menuIntPins[slot])`) and traces nothing. But every rack rebuild constructs a fresh `RackProcessor`, which receives the option with its other pins and prints `first NONZERO OUTPUT` exactly once. A cable edit forces a rebuild. So: pick, edit a cable, read the new instance.

| arm | pick | cable edit | `first NONZERO OUTPUT pin 0` |
|---|---|---|---|
| A (baseline) | 1024 (default) | WT -> Out L added | **0.500143** |
| B | 32 | Out R -> WT added | **0.500004** |
| A | 1024 | Out R cable dragged into space (removed) | **0.500143** |
| B | 32 | Out R -> WT added | **0.500004** |
| A | 1024 | Out R cable removed | **0.500143** |

Alternates with the pick, repeats within it, five rebuilds. `Wave points` is `Wavetable::setWaveLen` (`modules/WTVCO/vcv/Wavetable.hpp`), which changes the slice length over the same 4096 stored samples and never resamples them -- so at 32 the LFO plays the sine's first 32-sample sliver instead of its cycle, and the first sample a fresh instance emits at the same phase is a different number. The direction and the ~36:1 ratio are consistent with that reading; I did not measure the waveform itself.

### Also done: the box lock probe on macOS, which no posix box had run

`python3 tests/e89_box_lock_probe.py` in a scratchpad worktree at `85a3147`: **control 12 of 12 (must fail, did), 14 arms, 0 failures, 8.2 s.** Arm 11 resolved the ancestor, arm 12 reproduced #583's 5.6 s gap and refused run B, arm 14's unforced `release` returned rc=0. The 10-14 and 10-20 win cells both said *"whichever posix box runs next should just run it and say so"*; it is run and said, and the lock also worked end to end this run.

**Build:** fresh scratchpad tree from `origin/main` at `85a3147`, Ninja, Release, arm64, `SE_LOCAL_BUILD=OFF`, `TIDE_VCV_FUNDAMENTAL=ON`, `-DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`, **no `*_FOLDER_OVERRIDE`** -- every dependency fetched at its own `origin/main` exactly as `main` declares. `TIDE_Rack_STANDALONE`: **374/374, 0 `error:`, 5 warnings.** So `main` builds the standalone on macOS at `85a3147`; the VST3, AU3 appex and CLAP targets were not built.

**Learned:**

- **A channel `--drag` from the module browser selects the row and drops nothing; a `--double` click places the module.** The browser-to-rack drop is a native drag the command channel never touches, so the one gesture that looks like "place a module" is the one that cannot.
- **The context menu follows the SELECTION, even over empty canvas.** A background "control" taken while a module is selected returns the module's menu, which the probe's first control got wrong and the second measured: 20 selected, 7 deselected. E38 said this about the module's point; it is equally true of the background's.
- **When a parameter change leaves no trace, force a rebuild and read the fresh instance.** The adaptor's one-shot per-instance lines (`constructed`, `connections`, `first NONZERO OUTPUT`) become an A/B instrument the moment something rebuilds the rack, and a cable edit does. Cheaper than audio, and it measures the DSP itself.
- **`--render-audio` cannot measure a TIDE rack today, and says so in its result.** `parametersUnprimed` is the tell; `peak 0` after it is a reading about priming, not about the patch (E6).
- **"Needs an idle box" is one command away from being a fact rather than a mood.** `ioreg -c IOHIDSystem` plus `pmset -g log` settled it in seconds, and the answer this time was 34 hours. A scheduled run that wants a GUI row should measure before parking.
- **A locked screen stopped nothing in this toolchain.** The standalone launched, its socket answered, `--screenshot` returned real frames and the editor drove, all with the display off since the previous day. The 08-29 line through the tooling runs between the standalone and a DAW, not between the standalone and the run.

**Not verified:** **nothing in a host** -- every measurement is the standalone, and E19's clauses are per-format; E82's Accept does not name a host and its row is flipped on the standalone alone. **The bool arm** (VCA-1's *Exponential response* and the other six bool-bearing modules) was not measured; `Wave points` is the `int/enum` arm only. **The waveform** behind the two numbers was not captured -- the cycle-versus-sliver reading is from `Wavetable.hpp`. **Only `TIDE_Rack_STANDALONE` was built**, so "main builds on macOS" is one target's claim. **Windows and Linux** ran nothing; the probe's coordinates are the standalone's default 1100x626 window on the shipped default rack and it exits 2 on any other size rather than clicking blind. **E19's row was not edited** -- its `int/bool/enum` clause now has a measured producer and a recipe, and flipping anything on E19 is that row's job. The `gpu-backend` branch was read, not built.

**Rotation:** done. With this entry `JOURNAL.md` was 60,358 bytes; the oldest entry (10-14 windows, A44) moved to `JOURNAL-2026-10.md`, appended below what was there, verbatim. Six entries remain, above the four-entry floor.

**Machine state:** `~/Documents/GitHub/TideSynth` **stayed on `main`, clean, and was never checked out** -- its local `main` is at `dc80d88` and deliberately not fast-forwarded. All work was in a scratchpad `git worktree` (this branch) and a scratchpad build tree, both removed at the end. **Five standalone launches, every one under `GMPI_STANDALONE_CONFIG_DIR` and `GMPI_STANDALONE_IPC_DIR` pointed at the scratchpad**; each arm's `session.xml` and `standalone.conf` landed in its own scratch directory. The developer's `~/Library/Application Support/TiDE Rack/` **files are unchanged** (`session.xml` Sep 22 11:59, `standalone.conf` and `session.previous.xml` Aug 31 12:55, no new entry) -- but **the directory's own mtime moved to 02:14:26**, the same created-and-removed-entry phenomenon the 09-07 entry recorded and could not attribute; contents provably untouched, cause still unknown. `~/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3` untouched (Aug 28). No DAW was launched, no AUv3 registered, no GUI or computer-use, no screenshot of the display -- the frames came from the app's own `--screenshot`. `SynthEditLib`, `gmpi_ui`, `GMPI`, `GMPI_Wrappers` and `SynthEdit_Rack_Adaptor` were fetched and read only; all five are clean on `main` at `origin/main`, none was checked out or built, and the build fetched its own copies. `~/Documents/GitHub/SynthEdit` (`master`, clean) was not touched. **0 TIDE processes left**, checked. No credential value appears anywhere. Box lock released as the last act.

**Next:** see the `mac` cell. **For Jeff:** (1) the E82 PR is `tests/` plus bookkeeping and waits for you by design -- the probe is self-contained and re-runnable in 27 s against any `-DRACK_ADAPTOR_TRACE=1` standalone. (2) E19's `int/bool/enum` clause now has a producer and a recipe; whether its per-host cells should reuse this instrument or insist on audio is your call. (3) Your `gpu-backend` branch has no PR; no run will touch it. (4) The bot token expires **2026-11-07**, 13 days out.

**Branch/PR:** `tide/mac/E82-wtlfo-wave-points`, [#669](https://github.com/JeffMcClintock/TideSynth/pull/669). The probe, the E82 flip, this entry, the `mac` cell and the regenerated `docs/lessons.md` are on that branch. `tests/**` is off the auto-merge allowlist, so it waits for Jeff.

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
