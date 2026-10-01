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
## 2026-10-01 — windows — correction: the lane-sweep probe DID finish, and it plus #627's own merge confirm #618's zero-diff resting state (scheduled run, continuation)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.16120.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** corrected one false claim in the entry below and recorded two measurements that arrived after it had already merged. No code changed, no row changed.

### The correction

The entry below says, under **Not verified**: *"`tests/a38_lane_sweep_probe.py` did not finish inside this run's time budget, so I have its verdict from before the push only, not an A/B."* **Both halves are wrong.** It finished, `rc=0`, and the run was **after** the push, not before.

It was still running when I checked it, and it completed between that check and the commit. I wrote the conservative-sounding sentence from the last thing I had looked at instead of re-reading the output file — and put it in the one section a reader trusts *because* it is conservative.

**What it actually says**, post-push, over this lane's seven open PRs:

```
--- every ordering, exhaustive (7! = 5040) ---
  depth histogram: {1: 2400, 2: 1920, 3: 720}
  deepest order: #622 -> #618 -> #614
  orderings landing ALL 7: 0 of 5040

--- depth-2 matrix: after X lands, which others conflict, and where ---
  after #622   blocks nothing                bookkeeping: none
  after #618   blocks nothing                bookkeeping: none
  after #614   blocks #597, #590, #587, #586  bookkeeping: BACKLOG.md
  after #597   blocks #614, #590, #587, #586  bookkeeping: BACKLOG.md, JOURNAL.md, docs/decisions.md
  after #590   blocks #614, #597, #587, #586  bookkeeping: BACKLOG.md, JOURNAL.md, docs/decisions.md
  after #587   blocks #614, #597, #590, #586  bookkeeping: BACKLOG.md, JOURNAL.md, docs/decisions.md
  after #586   blocks #614, #597, #590, #587  bookkeeping: BACKLOG.md, JOURNAL.md, docs/decisions.md
```

**`after #618 blocks nothing` is independent confirmation, from the lane's own probe, of exactly what the STEP 1.5 merge claimed** — and #622 sits beside it for the same reason, being the other PR with no bookkeeping diff. The five that block each other are the five carrying code *and* a `BACKLOG.md` edit. A38's livelock is untouched by any of this: **0 of 5040 orderings land all seven**, unchanged in character from the 0-of-120 measured on 09-26.

I still do not have an A/B for the lane sweep — one side only, after the push. A41's A/B rests on the row-adjacency probe, where both sides exist, and that is unaffected.

### The resting state then survived a real merge, which is the part no measurement had yet

[#627](https://github.com/JeffMcClintock/TideSynth/pull/627) **auto-merged roughly two minutes after it opened** (`7c92c3a8e`), touching `BACKLOG.md`, `JOURNAL.md` **and** `docs/lessons.md` — all three contended files at once. Afterwards:

| PR | before #627 | after #627 |
|---|---|---|
| [#618](https://github.com/JeffMcClintock/TideSynth/pull/618) | `MERGEABLE`/`CLEAN` | **`MERGEABLE`/`CLEAN`** |
| #622, #614, #597, #590, #587, #586 | `MERGEABLE`/`CLEAN` | **`MERGEABLE`/`CLEAN`** |
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (mac, A36) | `CONFLICTING` after #626 | `CONFLICTING` — unchanged, mac's lane |
| #604 | superseded | superseded |

**#627 re-conflicted nothing.** Before today, a `main` commit touching those files re-conflicted #618 every time — that is the whole content of the four preceding cells. This is the first time the zero-diff resting state has been *tested* rather than merely constructed, and it held.

### The #120 trap fired again, and the rule was right

#627 merged before I could add any of the above to it, so the entry below is a **merged** entry and not editable: `check-journal-prepend.py` compares head against `main` and would correctly read an in-place edit as a rewrite. That is precisely what the 2026-09-06 mac cell and the 2026-09-29 windows cell each hit. The append-only route is a separate entry, which is this one.

**Learned:**

- **Re-read a backgrounded command's output file immediately before you commit the entry that describes it.** Mine finished in the gap between my last check and the commit, and the entry shipped a false *"did not finish"* — in **Not verified**, the section whose whole value is that a reader can trust it to understate. A stale "still running" reads as caution and is a factual error.
- **An allowlist-eligible PR merges faster than a run can revise it — about two minutes here.** Anything you might want to correct has to be correct *before* the push, because afterwards the only route is a second entry and a second PR.
- **The zero-diff resting state is now measured, not just argued.** `after #618 blocks nothing` from the lane's own probe, plus #618 surviving a `main` merge that touched all three contended files. The construction was reasoning; these two are evidence.

**Not verified:** no build, no host, no code touched. I have the lane sweep from one side of the push only. I did not investigate why #585 is still `CONFLICTING` — it is mac's lane and was already so before this run, from #626 rather than from #627.

**Machine state:** `C:\SE\TideSynth` on `main`, clean, never left it; all work in scratchpad `git worktree`s, removed at the end. `SE16`, `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` untouched and clean. The developer was at the machine throughout (Visual Studio on `ToneMaster`) — no GUI work, no screen taken. `git fetch` warned *"too many unreachable loose objects"* in `C:\SE\TideSynth`; that is a local housekeeping note for Jeff (`git gc`), not a repository problem, and I did not run it on his tree.

**Next:** see the `win` NEXT cell, which this entry does not change — **A41** is still what the next win run should take, and #622 still needs Jeff's one answer about the fleet PAT's expiry.

**Branch/PR:** `tide/win/2026-10-01-lane-sweep-correction`.

## 2026-10-01 — windows — STEP 1.5: three content re-syncs never moved #618's merge-base, and a real merge did; the A38 probes' verdict flipped twice on fleet movement alone (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.16120.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on [#618](https://github.com/JeffMcClintock/TideSynth/pull/618), which was this lane's only conflicting PR. STEP 2 found nothing eligible — thirteenth consecutive cell. I also cleared the `docs/lessons.md` regeneration the 09-25 cell recorded as owed, and filed **A41**. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`25bf45e61..57a1bf593`).

### STEP 1: empty, and still structurally so

`gh issue list --label platform:win --state open` is empty, which on this platform verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing. Reading `main` instead: the latest `build` and `verify` runs are at **`25bf45e61`**, Jeff's slider-switch merge, both `completed/success`. `57a1bf593` (today's mac bookkeeping merge) has no check-runs because it is documentation only. The two open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (`platform:linux`) and [#44](https://github.com/JeffMcClintock/TideSynth/issues/44), the watchdog digest.

### STEP 1.5 — and the cause was a distinction this lane had not drawn

Of this lane's seven open PRs, six were `MERGEABLE`/`CLEAN` with 13 SUCCESS + 2 SKIPPED and no reviews or review comments — #622 (A40), #614 (A39), #597 (A38), #590 (E19), #587 (E82), #586 (E80). STEP 1.5 says a green PR with nothing unresolved is waiting for merge and not a run's to fix, so I left all six alone. **#618 was `CONFLICTING`/`DIRTY` in `BACKLOG.md` and `JOURNAL.md`.**

**The finding is why it conflicted for the fourth cell running.** The three commits before mine are `0c5bab71a`, `60a5ea753` and `70e3506fd`, all titled *"re-sync the bookkeeping pair to main after #NNN merged"* — and all three are **ordinary commits, not merges**. Copying `main`'s content into the branch makes the *diff* empty at that instant, but it leaves the **merge-base** where it was: `git merge-base` was still **`bf23fc1ce` (2026-09-28)**. A three-way merge then sees both sides having changed both files since that base, so the next commit on `main` re-conflicts them, and the cell pays the same resolution again.

| | before | after |
|---|---|---|
| merge-base with `origin/main` | `bf23fc1ce` (09-28) | **`57a1bf593`** (`main`'s head) |
| `git diff origin/main HEAD` | `BACKLOG.md`, `JOURNAL.md`, `modules/TiDEknob/*` (3), 2 probes | **2 probes, 550 insertions, nothing else** |
| `merge-tree --name-only origin/main HEAD` | `BACKLOG.md` + `JOURNAL.md` CONFLICT | **clean, no paths** |
| PR state | `CONFLICTING`/`DIRTY` | **`MERGEABLE`** |

So the fix was a **real merge** (`cc025a2be`), resolving both files by taking `origin/main` verbatim. That advances the base, and because the branch now introduces *no diff at all* in either file, future movement on `main` cannot conflict it: one side is unchanged. It is also lint-safe rather than lint-hostile, which is the trap the previous cell correctly warned about — `lint.yml` sets `BASE="origin/${{ github.base_ref }}"`, i.e. `main`'s **current tip**, and `actions/checkout` on `pull_request` checks out the merge commit, so a branch with a zero diff in the pair is compared against `main`'s own copy and passes trivially. Carrying a *stale* copy is what fails; carrying *no change* is what does not.

**Nothing was lost.** All eight `## 20…` entries the branch had been carrying are already on `origin/main` — checked one at a time with `grep -Fqx` — and its `win` NEXT cell is superseded by `main`'s newer A40 cell. Heading counts: branch **44**, main **44**. Both resolutions were verified byte-identical to `origin/main` with `git hash-object` (`BACKLOG.md` `aa8a22a98…`, `JOURNAL.md` `f4bd99397…`).

All seven lint checks exited 0, run the way `lint.yml` runs them: `check-links` (675 relative links, no broken), `check-journal-prepend` (`prepend-only, OK`), `check-backlog-diff` (`status/date cells and new rows only, OK`), `check-prompt-provenance`, `check-id-refs`, `check-next-block` and `check-backlog-archived`. `check-commit-completeness --record`/`--verify` bracketed the commit (`--verify` skips merges). `check-commit-authorship --repo` printed `11 commit(s) in origin/main..HEAD` and `all commits authored by tide-rack-bot`; `ls-remote --get-url origin` returned `https://`.

### The probes on #618 flip on fleet movement alone — filed as A41

I ran the branch's own probe before pushing, because it is the PR's verification artifact:

```
$ python3 tests/a38_row_adjacency_probe.py --repo <worktree>     # before the push
  C2 2026-09-27-adjacen+A39-prefab-count-d: same-line pair stopped conflicting after respace
PROBE FAILED (1)

$ python3 tests/a38_row_adjacency_probe.py --repo <worktree>     # after the push
  C1 OK   C2 OK   C3 OK
PROBE OK      rc=0
```

**Same probe blob both times, one merge commit apart.** It read red this morning and green this afternoon because `origin/main` and the branch set moved — the probes enumerate branches with `ls-remote --heads origin refs/heads/tide/*` and diff each against `origin/main`, so their verdict is a function of fleet state. Neither probe is run by any workflow (`grep -rn a38_ .github/workflows/` is empty), so neither flip announced itself and no PR went red. I saw both only because I ran the probe while doing STEP 1.5 on the PR carrying it.

**#618's PR body records three controls OK. That is true again today and was false this morning**, and nothing in the repository records which fleet state the claim was measured against. Filed as **A41** with both halves of the A/B. I did not change either probe — that is A41's job, not this cell's.

### STEP 2: nothing eligible, thirteenth cell

`#622` (A40) is still **OPEN**, `MERGEABLE`/`CLEAN`, so A40 stays `IN-REVIEW` and its `NEEDS-JEFF` half — the credential's real expiry — is unanswered; there was nothing to fold into the row. Every other TODO row is ineligible for a reason I checked rather than inherited: A38 (#597), A39 (#614), E19 (#590), E80 (#586) and E82 (#587) are this lane's own open PRs and green; E72 (#588) and E81 (#589) are mac's; E79 (#584) is linux's; A35 waits on its own two open `PROPOSED:` entries, which are the question itself, so it is not identical under every answer; S8 is `NEEDS-SPEC`; E2 is an umbrella with no statable Accept; X2 is `linux`; E84 is a `.github/workflows/**` edit this credential deliberately cannot make. **I read E76 rather than taking the cell's word** — its `Plat` says `any`, but its Accept is `render-and-measure.py … returns -6.3/-17.0` **on linux**, and its other half turns on a ruling about whether a measurement script may edit the caller's environment. Not mine, and not identical under every answer.

### The owed `lessons.md` regeneration, now done

The 09-25 cell recorded *"ONE `python3 scripts/extract-lessons.py --write` IS OWED once these land, and that is the only debt this leaves."* The newest date header in `docs/lessons.md` was **2026-09-25** while `JOURNAL.md` ran to 2026-10-01, so the fleet's standing digest — item 5 on every run's reading list, the file that exists so runs stop re-deriving what has already been paid for — was six days and sixteen entries stale.

**The condition it was waiting on is moot, which is why now is the moment.** It said "once these land" to avoid re-conflicting #586/#587/#590/#597 — and none of those four touches `docs/lessons.md` any more. The only open PR that does is [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (linux, E79, open 23 days), and it is **already `CONFLICTING` in `docs/lessons.md` itself** along with `BACKLOG.md` and `JOURNAL.md`, so this adds nothing to its burden and is not mine to resolve.

`python3 scripts/extract-lessons.py --write` → `1534 lessons from 364 entries, 152,787 bytes`, rc=0. The diff is **92 insertions, 2 deletions**. Six new date sections (2026-09-26 … 2026-10-01), **no date header removed**, verified by diffing the `^## 20` sets. The two deletions are the file's own self-describing statistics, nothing else:

```
-Learned sections are **449 KB** across **351** entries, so copying them
-A8. This is **142 KB / 1495 lessons — 3.2x smaller**, and represents
```

**Learned:**

- **A content re-sync is not a merge, and only the merge moves the merge-base.** Copying `main`'s bytes onto a branch makes the diff empty *now*; it leaves the base where it was, so the very next commit on `main` re-conflicts the same files. This lane paid that toll three commits running on one branch. The one-command tell is `git merge-base HEAD origin/main` — if it is not `main`'s head, the branch still carries the old base's liability no matter how current its file contents look.
- **"Zero diff in the contended file" is a durable resting state for a long-lived branch; "current copy of the contended file" is not.** Both look identical the day you do them. The first cannot conflict because one side is unchanged; the second re-conflicts on `main`'s next move.
- **A probe whose fixture is the live fleet has a shelf life, and its pass is not evidence a reader can re-check.** Watching one verdict go red→green across a single unrelated merge, with the probe blob byte-identical, is worth more than either reading alone.
- **Run a PR's own verification artifact during STEP 1.5, not just the lints.** Every lint passed on #618 and said nothing; the probe it carries was failing. No workflow runs it, so the only way that surfaces is a run choosing to execute it.
- **A recorded debt can outlive its stated precondition.** The `lessons.md` regeneration was gated on four PRs landing; the reason for the gate disappeared when those PRs stopped touching the file, and the gate then held for six days on a condition that no longer meant anything. Re-read *why* a chore was deferred, not just *whether* its trigger fired.

**Not verified:** I built nothing and launched no host — nothing in this cell touches compiled code. #618's CI was still running (`UNSTABLE`) when this was written; its pre-push rollup was 13 SUCCESS + 2 SKIPPED. `tests/a38_lane_sweep_probe.py` did not finish inside this run's time budget, so I have its verdict from before the push only, not an A/B; A41 rests on the row-adjacency probe, where I have both sides. I did not verify the bot PAT's real expiry, which is not verifiable from here, and I did not inspect Jeff's slider-switch commit beyond its check-runs.

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and never left it — all work in `git worktree`s under the scratchpad, removed at the end. **All five repos were clean at the start**, which is the first time this lane has recorded that: `SE16` (`master`, behind 1), `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` (all `main`) had no dirty files, and I touched none of them. Per the 09-29 lesson I read `status -sb` rather than `status --short`: `TideSynth` was `behind 1`, not ahead, so Jeff's `910b7b0d5` has landed. The developer was at the machine throughout (Visual Studio on `ToneMaster`, the ToneMaster app running, Outlook, Slack, Chrome) — a different project, not TIDE — so I did no GUI work and took no screen. No credential value appears in any commit, PR, journal entry or row.

**Next:** see the `win` NEXT cell. For Jeff: **#622 (A40) is green and waiting**, and its `NEEDS-JEFF` half is one question — does the fleet PAT expire at all, and if so when? Only the GitHub UI can answer it.

**Branch/PR:** this entry, the A41 row, the regenerated `docs/lessons.md` and the refreshed `win` NEXT cell are on `tide/win/2026-10-01-step15-and-lessons` (`BACKLOG.md` + `JOURNAL.md` + `docs/` only, so it should auto-merge). The STEP 1.5 merge is `cc025a2be` on `tide/win/2026-09-27-adjacency-measurement`, pushed to the existing branch with no new PR.

## 2026-10-01 — macos — STEP 1.5: #585 (A36) re-conflicted in `JOURNAL.md` alone, fifth cell running; `main` green after Jeff's slider switch; STEP 2 walked, nothing eligible (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.16120.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36). I took no backlog item. STEP 1 was empty: there is no open `platform:mac` issue. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`85e4a30..25bf45e`).

### What changed on `main`

Three commits since the 09-30 cell: that cell's bookkeeping ([#625](https://github.com/JeffMcClintock/TideSynth/pull/625), `dba7cab`), Jeff's **`910b7b0` *"added tiDE slider switch"*** (`modules/TiDEknob/CMakeLists.txt`, `TiDESliderSwitch.cpp`, `TiDESliderSwitchGui.cpp`, +269 lines), and his merge `25bf45e`. That is the commit the 09-29 windows cell saw sitting unpushed on the Windows box's local `main`, so it has now landed.

**It is the first product-code change on `main` since 09-25, so I checked it before STEP 1.5 rather than assuming.** The check-runs on `25bf45e` are all `success`: `macos`, `windows`, `linux`, `render-macos`, `render-windows`, `render-linux`, `guard` and `digest`, plus the `build`, `verify` and `watchdog` workflow runs. The slider switch did not break this lane, so STEP 1 stays empty. I did not build it locally.

### STEP 1.5

`git merge-tree --write-tree --name-only origin/main origin/<branch>`:

| PR | conflicting paths | checks before push | reviews |
|---|---|---|---|
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) A36 | **`JOURNAL.md` only** (Rotation header vs `main`'s 09-30 mac entry at the prepend point) | 13 SUCCESS + 2 SKIPPED, `CONFLICTING` | none, 0 review comments |
| [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) E72 | none | 13 SUCCESS + 2 SKIPPED, `MERGEABLE` | none |
| [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) E81 | none | 13 SUCCESS + 2 SKIPPED, `MERGEABLE` | none |
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) | superseded; left for Jeff to close unmerged | | |

**Resolution:** the same as the last four cells. In a scratchpad worktree I merged `origin/main` into `tide/mac/A36-journal-rotation-rule`, kept A36's Rotation header block (HEAD side), then `main`'s side, and removed the three markers. `BACKLOG.md` and Jeff's three module files auto-merged. The merge commit is `6ea4db1`, pushed to the existing branch, with no new PR.

**Verification:** I compared `## 20…` heading sets over `JOURNAL.md` + `JOURNAL-2026-08.md` + `JOURNAL-2026-09.md`:

| branch (pre-merge) | main | union | merged | missing | extra | dup |
|---|---|---|---|---|---|---|
| 465 | 465 | 466 | 466 | 0 | 0 | 0 |

All seven lint checks exited 0, run the way `lint.yml` runs them: `check-links`, `check-journal-prepend` (`prepend-only, OK`), `check-backlog-diff`, `check-prompt-provenance`, `check-id-refs`, `check-next-block` and `check-backlog-archived`. `check-commit-completeness --record`/`--verify` bracketed the commit (`--verify` skips merges). `check-commit-authorship --repo .` printed `all commits authored by tide-rack-bot`, and `ls-remote --get-url origin` returned `https://`.

### STEP 2: nothing eligible

`git diff dba7cab origin/main -- BACKLOG.md docs PLAN.md` is empty, so every row is byte-identical to what the 09-30 cell walked. The claiming PRs are all still open: A38 (#597), A39 (#614), E19 (#590), E80 (#586), E82 (#587) and A40 (#622) for win, E79 (#584) for linux, and E72 (#588) and E81 (#589) for this lane. The other TODO rows are ineligible for the recorded reasons. A35 waits on an open `PROPOSED:`. S8 is NEEDS-SPEC. X2 is linux in substance. E2 is an umbrella. E76 wants a ruling. E84 is a workflow edit. E83 is `IN-REVIEW` with [#581](https://github.com/JeffMcClintock/TideSynth/pull/581) merged, but its flip already rides on #585 and #588, so I did not add a third copy. No other IN-REVIEW row's PRs have all merged.

**Learned:**

- **The app version moved from 2.9939.4 to 2.16120.0 between the 09-30 and 10-01 cells**, with the same model and prompt sha. Nothing behaved differently, but the provenance line is where that would show if it ever did.
- **Nothing new about the livelock.** This is the fifth consecutive mac cell whose whole output is the same one-hunk `JOURNAL.md` resolution on #585, and this cell's own PR will re-conflict it again. It ends only when Jeff merges #585 or rules on A38/[#597](https://github.com/JeffMcClintock/TideSynth/pull/597).

**Not verified:** I built nothing locally. The claim that `main` builds on macOS rests on CI's `macos` and `render-macos` checks at `25bf45e`. The only edit to a PR branch was the `JOURNAL.md` conflict resolution.

**Machine state:** `~/Documents/GitHub/TideSynth` is on `main`, clean, and 19 behind `origin/main`. I left it alone, as the 09-30 cell did, and did all work in scratchpad `git worktree`s, which I removed. `~/Documents/GitHub/SynthEdit` is on `master`, clean, and `ahead 1, behind 1` of `origin/master`. That predates this run and is the developer's, so I did not touch it. I launched no host and did no GUI work.

**Next:** see the `mac` NEXT cell. For Jeff: **merge #585 first**, then #588/#589, and close #604 unmerged.

**Branch/PR:** `tide/mac/2026-10-01-step15` holds this entry and the refreshed `mac` NEXT cell (`BACKLOG.md` + `JOURNAL.md` only, so it should auto-merge). The merge is on `tide/mac/A36-journal-rotation-rule`.

## 2026-09-30 — macos — STEP 1.5: #585 (A36) re-conflicted in `JOURNAL.md` alone, for the fourth cell running; STEP 2 walked, nothing eligible (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.9939.4** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36). I took no backlog item. STEP 1 was empty: there is no open `platform:mac` issue. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded.

### STEP 1.5

Since the 09-29 mac cell, `main` gained five bookkeeping commits: [#620](https://github.com/JeffMcClintock/TideSynth/pull/620) (that cell) and the windows lane's #617, #621, #623 and #624. The only non-journal change was the new A40 row and the `win` NEXT cell. `git merge-tree --write-tree --name-only origin/main origin/<branch>` gave this:

| PR | conflicting paths | checks before push | reviews |
|---|---|---|---|
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) A36 | **`JOURNAL.md` only** (Rotation header vs `main`'s new entries at the prepend point) | was `DIRTY` | none |
| [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) E72 | none | `CLEAN`, 13 SUCCESS + 2 SKIPPED | none |
| [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) E81 | none | `CLEAN`, 13 SUCCESS + 2 SKIPPED | none |
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) | superseded; left for Jeff to close unmerged | | |

**Resolution:** the same as 09-28 and 09-29. I merged `origin/main` into `tide/mac/A36-journal-rotation-rule`, kept A36's Rotation header block (HEAD side), then `main`'s side, and removed the three markers. `BACKLOG.md` auto-merged. The merge commit is `1b07bbf`, pushed to the existing branch, with no new PR.

**Verification:** I compared the `## 20…` heading sets over `JOURNAL.md` + `JOURNAL-2026-08.md` + `JOURNAL-2026-09.md`:

| branch (pre-merge) | main | union | merged | missing | extra | dup |
|---|---|---|---|---|---|---|
| 457 | 464 | 465 | 465 | 0 | 0 | 0 |

All seven lint checks exited 0, run the way `lint.yml` runs them: `check-links`, `check-id-refs`, `check-next-block`, `check-backlog-archived`, `check-journal-prepend` (`prepend-only, OK`; 17 entries rotated out, all verified verbatim elsewhere), `check-backlog-diff` (`status/date cells and new rows only, OK`) and `check-prompt-provenance`. `check-commit-completeness --record`/`--verify` bracketed the commit (`--verify` skips merges). `check-commit-authorship --repo .` printed `all commits authored by tide-rack-bot`, and `ls-remote --get-url origin` returned `https://`. Right after the push, #585's rollup showed 3 SUCCESS, 2 SKIPPED and 10 still pending, with none failed.

### STEP 2: nothing eligible

`git diff 9ccd09b origin/main -- BACKLOG.md` changes only the `win` NEXT cell and adds **A40**, which windows filed and took as IN-REVIEW ([#622](https://github.com/JeffMcClintock/TideSynth/pull/622)). `docs/decisions.md`, `docs/lessons.md` and `PLAN.md` did not change. Every other row is byte-identical to what the 09-26, 09-28 and 09-29 cells walked. The claiming PRs are all still open: A38 (#597), A39 (#614), E19 (#590), E80 (#586), E82 (#587) and A40 (#622) for win, E79 (#584) for linux, and E72 (#588) and E81 (#589) for this lane. The other TODO rows are ineligible for the same recorded reasons. A35 waits on an open `PROPOSED:`. S8 is NEEDS-SPEC. X2 is linux in substance. E2 is an umbrella. E76 wants a ruling. E84 is a workflow edit. No IN-REVIEW row's PRs have all merged, so there was nothing to flip.

**Learned:**

- **Nothing new about the mechanism.** This is the fourth consecutive mac cell whose whole output is the same one-hunk `JOURNAL.md` resolution on #585, and this cell's own PR will re-conflict it again when it lands. It ends only when Jeff merges #585 or rules on A38/[#597](https://github.com/JeffMcClintock/TideSynth/pull/597).
- **The local `main` in `~/Documents/GitHub/TideSynth` is 16 commits behind `origin/main`** (at `0a8a87c`). That is harmless, because every run reads from `origin/main` and branches from it, but I left it alone rather than fast-forwarding a tree the developer may be using.

**Not verified:** I built nothing, and this run changed no code. The only edit to a PR branch was the `JOURNAL.md` conflict resolution.

**Machine state:** `~/Documents/GitHub/TideSynth` is back on `main` and clean. The A36 merge was done in the main tree and this entry in a scratchpad `git worktree`, which I removed. I did not touch `SynthEdit` (`master`, clean). I launched no host and did no GUI work.

**Next:** see the `mac` NEXT cell. For Jeff: **merge #585 first**, then #588/#589, and close #604 unmerged.

**Branch/PR:** `tide/mac/2026-09-30-step15` holds this entry and the refreshed `mac` NEXT cell. The merge is on `tide/mac/A36-journal-rotation-rule`.

## 2026-09-29 — windows — the developer committed to local `main` mid-run, and "the tree is clean" would not have caught it (scheduled run, continuation)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.4** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** recorded one machine-state fact from this run's STEP 5 that its three merged entries do not carry, because it was only observable after they merged.

### What happened

At **17:14**, mid-run, Jeff committed **`910b7b0d5` *"added tiDE slider switch"*** to the **local** `main` of `C:\SE\TideSynth` — matching the `TiDEModules - TiDESliderSwitchGui.cpp` window that was open all afternoon. The box therefore ended the run with `main` reading **`ahead 1, behind 2`** of `origin/main`.

**`git status --short` printed nothing.** The tree *is* clean; a committed commit is not dirt, and none of STEP 5's three kinds of uncommitted change names it. A run that checked cleanliness alone would have been told everything was fine, and everything was — but not the thing that matters here.

It is **the developer's unpushed commit on his own default branch**: category 3 by intent if not by wording, so not mine to push, rebase, reset or tidy. I left it exactly as found.

### Verified rather than assumed

`git merge-base --is-ancestor 910b7b0d5 origin/<branch>` is **false for all six** branches this run pushed — `2026-09-26-step15-and-sweep-measurement`, `2026-09-27-adjacency-measurement`, `A39-prefab-count-derived`, `2026-09-29-automerge-result`, `A40-token-expiry-derived`, `2026-09-29-a40-bookkeeping` — and `git branch -r --contains 910b7b0d5` finds it on **no remote ref at all**.

That holds for a structural reason and not by luck: **every branch was cut from `origin/<default>`, per STEP 2's *"never base a branch on the working tree's state"*.** This is that rule doing precisely the job it was written for, and it is the first time this journal has a positive measurement of it rather than a statement of intent.

**Learned:**

- **"The tree is clean" and "the tree is where `origin` is" are different claims, and only the second one tells you whether a branch cut from local `main` would ship somebody else's commit.** `git status -sb` prints both in one line; `git status --short`, which this lane's cells have been quoting, prints only the first.
- **The developer-at-the-machine check should look at ahead/behind, not just dirt.** An open editor window predicts a commit as much as it predicts an unsaved buffer, and a mid-run commit to local `main` is invisible to every dirt rule the prompt states.

**Not verified:** I did not inspect the contents of Jeff's commit beyond its subject line and author, and did not build anything.

**Machine state:** `C:\SE\TideSynth` on `main`, clean, `ahead 1` (Jeff's commit) / `behind 2`, left exactly so. All worktrees removed. `SE16` (`master`) and `SynthEditLib` (`main`) carry 1 and 8 dirty files respectively, all predating this run and untouched; `gmpi_ui` and `GMPI_Wrappers` are clean and were not touched. No host, no build, no screen taken.

**Next:** see the `win` NEXT cell, which this entry does not change.

**Branch/PR:** `tide/win/2026-09-29-machine-state`. An earlier draft of this note tried to edit the already-merged [#623](https://github.com/JeffMcClintock/TideSynth/pull/623) entry in place; `check-journal-prepend` rejected it, correctly — a merged entry is not editable — so it was dropped unpushed and re-filed as this separate entry, which is the append-only route.

## 2026-09-29 — windows — A40: the watchdog's credential countdown was counting down to a date nothing could read (scheduled run, continuation)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.4** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **A40** — the first backlog item this lane has taken since 09-25, and the queue was only open because the two entries above cleared this lane's PRs out of it. Claimed it with a pushed DOING mark (`5e84e89de`) before any work, per STEP 2; it was unclaimed by that step's test, with no branch and no PR naming it. [#622](https://github.com/JeffMcClintock/TideSynth/pull/622), branch `tide/win/A40-token-expiry-derived`.

### The defect, and it is worse than the row guessed

`scripts/watchdog-digest.py` held `TOKEN_EXPIRY = '2026-11-07'` as a literal and every *"Expires … (N days away)"* line in the [digest](https://github.com/JeffMcClintock/TideSynth/issues/44) was arithmetic on it. The row filed it as A39's shape — a hand-maintained constant that goes stale — and it is that, but the live measurement makes it sharper.

**The header is genuinely absent on the fleet credential.** `gh api -i rate_limit` under the bot PAT returns 24 headers and `github-authentication-token-expiration` is not among them. The 09-27 cell measured `user` and `rate_limit`; this is a third check through the script's own fetch path, and it agrees.

So the digest was not merely at risk of going stale. **It was printing a countdown to a date that nothing in the system could read**, in the one place this project treats as its source of truth, about one of exactly two bounds on a standing plaintext write credential. `- Expires 2026-11-07 (38 days away).` looked like a measurement and was a recital.

### The change

- `expiry_from_headers(headers)` — pure, so the controls can fabricate input. Case-insensitive key match; GitHub's value is `2026-11-07 15:04:05 UTC`, so only the date is taken.
- `fetch_response_headers(endpoint='rate_limit')` — one `gh api -i`. `rate_limit` needs no scope and consumes no quota, so asking costs nothing when the answer is "no such header".
- `check_credential_expiry(headers=None, recorded=...)` — derives the countdown. **Header absent ⇒ no countdown at all**: it prints `unknown -- no expiry header on this credential`, states which credential it measured (in CI the workflow's `GITHUB_TOKEN`, not the bot PAT, which is the objection the old hard-coding comment raised and the reason this is not simply "query it"), and reports the recorded date as *unverified*. Header present but disagreeing with the doc ⇒ it says the doc is stale.

**The old comment's objection was right and is preserved rather than overruled.** It argued against querying because CI runs under the wrong credential. The answer is not to query and pretend, nor to recite and pretend, but to **name the credential being measured and refuse to count down from a date it did not supply.**

### Verification artifact

A/B of the full `--dry-run` digest, `origin/main`'s script versus this branch, same repo-root, same credential: **one line replaced by two, nothing else changed** — 3 changed lines total out of 62, all inside the credential section.

```
- - Expires 2026-11-07 (38 days away).
+ - **unknown -- no expiry header on this credential.** ...
+ - Recorded in `docs/weekly-run-prompt.md`: **2026-11-07** -- unverified, and not counted down from.
```

`python3 tests/a40_token_expiry_probe.py` — **18 arms, 0 failed, rc=0**, no network, no credential. Two of the arms are the ones that matter:

- **negative control** — two fabricated headers 100 days apart must move the reported date (`2026-10-09` vs `2027-01-17`);
- **vacuity control** — `expiry_from_headers` monkeypatched to a stub that ignores its argument and returns the recorded constant must **fail** the negative control. Without this arm, "the test passed" is compatible with the test asserting nothing, which is A39's trap exactly.

The full digest `--dry-run` exits 0 with all 8 sections intact.

### What this does not settle, and it is the half that cannot be coded

**Whether the credential expires at all.** Absent cannot be distinguished from *"this environment never shows it"* without a credential known to expire, and there is none on this box — Jeff's keyring token is OAuth and would not carry the header either. Only the owner can read the real date, in the GitHub UI. The row already said this; the digest now says it too, which is the whole improvement. **`NEEDS-JEFF`: the real expiry, and whether a non-expiring fleet credential is intended.** If it is non-expiring, the run prompt's *"it expires 2026-11-07"* is one of two stated bounds on that credential and is decorative.

### The PR is code-only, deliberately

`#622` touches `scripts/` and `tests/` and **neither `BACKLOG.md` nor `JOURNAL.md`**. It is allowlist-blocked and will wait for a human, correctly — it is code. Bundling the row flip into it would have bought nothing and cost it a re-conflict on every merge into `main`, which is this run's first finding applied to its own work. The DOING mark was pushed first and removed at the end (`e6571f54e`); the claim was visible for the whole of the work, which is what STEP 2 wants it for.

**Learned:**

- **A constant that no test can move is indistinguishable from a measurement, in the output.** The digest line read `Expires 2026-11-07 (38 days away)` either way. The vacuity control is the cheapest thing that tells them apart, and A39 had to learn it one file over.
- **"Query it instead" was the wrong fix and the original comment knew why.** The digest runs under a different credential in CI than the one the number is about. Deriving without naming *which* credential you derived from would have replaced a confident wrong number with a confident irrelevant one.
- **An absent signal is a finding only if you can say what its presence would have looked like.** This box has no positive control, so the honest report is "unknown", not "does not expire" — and the code says the former.

**Not verified:** no build, no host, no GUI — this item touches no compiled code. `#622`'s CI checks were still running when this was written. I did not verify the bot PAT's real expiry, which is not verifiable from here.

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and never left it — all work in `git worktree`s under the scratchpad, removed at the end. The developer was at the machine throughout (Visual Studio on `TiDEModules - TiDESliderSwitchGui.cpp` and `SynthEditStore - ResizeAdorner.cpp`, Outlook, Slack); no build, no host, no screen taken. I did not touch `SE16`, `SynthEditLib`, `gmpi_ui` or `GMPI_Wrappers`. No credential value appears in any commit, PR, journal entry or test.

**Next:** see the `win` NEXT cell.

**Branch/PR:** [#622](https://github.com/JeffMcClintock/TideSynth/pull/622), `tide/win/A40-token-expiry-derived` (code). This entry and the row flip are on `tide/win/2026-09-29-a40-bookkeeping`.

## 2026-09-29 — windows — the result: #617 auto-merged two minutes after the probes came off it, having sat three days (scheduled run, continuation)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.4** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** recorded the outcome of the split made earlier in this same run, which landed while the run was still going. The entry above predicted it and left it for the next cell; it did not have to wait.

### The experiment resolved inside the run

[#617](https://github.com/JeffMcClintock/TideSynth/pull/617) was opened 09-26 and sat `CONFLICTING` through four cells. I removed two files from it — `tests/a38_lane_sweep_probe.py` and `tests/a38_row_adjacency_probe.py` — changing no prose except one relative link, pushed at **09:15**, and it **auto-merged at 09:17:07 as `be41dbdbf`**. The `auto-merge` workflow run against head `9ccd09b32` concluded `success`.

| | before | after |
|---|---|---|
| files touched | `BACKLOG.md`, `JOURNAL.md`, 2 × `tests/**` | `BACKLOG.md`, `JOURNAL.md` |
| `automerge_eligible.py` | `not eligible`, rc=1 | **`eligible`, rc=0** |
| time open | **3 days**, 4 cells of resolution | **~2 minutes** |

**Nothing else about the PR changed.** Same branch, same entries, same NEXT cell, same lint. The only variable was the file list, which is the variable the A4 gate reads.

### What the merge then did, which is A38 exactly as documented

`be41dbdbf` re-conflicted two PRs in `BACKLOG.md`, both by adjacency rather than disagreement:

- **[#614](https://github.com/JeffMcClintock/TideSynth/pull/614) (A39)** — the branch holds its own row as `IN-REVIEW`; `main` now holds it as `TODO` with the new **A40** row on the next line. Resolved by taking the branch's A39 and `main`'s A40. Lossless by heading-set arithmetic, **461/461, 0 missing, 0 extra**; all seven lint checks rc=0; pushed as `40d4d7287`.
- **[#618](https://github.com/JeffMcClintock/TideSynth/pull/618)** — re-synced its `BACKLOG.md`/`JOURNAL.md` to `main` (`0c5bab71a`). It now differs from `main` by the two probe files alone and measures `CLEAN`.

**I caused both of these and fixed both.** A merge into `main` costs the other open PRs a resolution; that is the A38 tax and it is unchanged by any of today's work. What changed is that the thing paying it is now a two-minute merge rather than a three-day wait.

### One thing I got wrong and is worth stating

I first reset #618's bookkeeping files to the **merge base** rather than to `main`, reasoning that a branch introducing no change to a file can never conflict in it. That is true of the merge, and it **fails `check-journal-prepend`**: relative to `main`, the head was missing `main`'s newest entry, which the lint correctly reads as an entry being dropped. So the conflict-proof choice is rejected by the lint, and the lint is right — the two goals genuinely pull in opposite directions here. Re-syncing to `main`'s current content passes, at the cost of needing a re-sync each time `main` moves. That re-sync is cheap and mechanical; it is `git checkout origin/main -- BACKLOG.md JOURNAL.md` and nothing else.

**Learned:**

- **The allowlist verdict is the single best predictor of whether a fleet PR is about to be stuck, and it costs one command.** Measured 0 of 66 eligible PRs open and 12 of 12 open PRs blocked; the causal test ran today and took two minutes.
- **A PR that only records something should contain only records.** This entry and the one above it are on a PR touching two files, and that is now this lane's standing shape.
- **A branch that deliberately carries no change to a contended file still cannot carry an *older* copy of it**, because `check-journal-prepend` compares head against `main`, not against the merge base.

**Not verified:** I built nothing and ran no host. #618's and #614's own checks were still running when this was written; both were green before the merges and neither changed a line of code.

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and never left it — all work in `git worktree`s under the scratchpad, removed at the end. The developer was at the machine (Visual Studio on `TiDEModules - TiDESliderSwitchGui.cpp` and `SynthEditStore - ResizeAdorner.cpp`, Outlook, Slack); no build, no host, no screen taken.

**Next:** see the `win` NEXT cell.

**Branch/PR:** `tide/win/2026-09-29-automerge-result`, a two-file PR by construction.

## 2026-09-29 — windows — the bookkeeping PRs were never eligible for the auto-merge tier, and two probe files are the whole reason (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.4** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1 empty. STEP 1.5 on this lane's two `DIRTY` bookkeeping PRs, [#617](https://github.com/JeffMcClintock/TideSynth/pull/617) and [#618](https://github.com/JeffMcClintock/TideSynth/pull/618). STEP 2 found nothing eligible, twelfth cell. I took no backlog item and pushed to none of the five green product PRs.

### The livelock has a cause this lane controls, and twelve cells have missed it

Exactly one commit landed on `main` since the 09-28 cell — `9ccd09b32`, macOS's bookkeeping PR [#620](https://github.com/JeffMcClintock/TideSynth/pull/620) — and it re-conflicted both of this lane's bookkeeping branches in `BACKLOG.md` and `JOURNAL.md`, as every previous cell has recorded. What no cell has asked is **why the mac bookkeeping PR merged the same day and this lane's have sat since 09-26.**

The answer is the A4 allowlist, and it is not subtle. `scripts/automerge_eligible.py` is a **strict inclusion** list: a PR auto-merges only if *every* file it touches is allowlisted. `tests/**` is not on it and never was.

| PR | lane | files | verdict |
|---|---|---|---|
| [#620](https://github.com/JeffMcClintock/TideSynth/pull/620), [#619](https://github.com/JeffMcClintock/TideSynth/pull/619), [#616](https://github.com/JeffMcClintock/TideSynth/pull/616) | mac | `BACKLOG.md`, `JOURNAL.md` | **eligible** → merged same day |
| [#617](https://github.com/JeffMcClintock/TideSynth/pull/617), [#618](https://github.com/JeffMcClintock/TideSynth/pull/618) | win | `BACKLOG.md`, `JOURNAL.md`, `tests/a38_lane_sweep_probe.py`, `tests/a38_row_adjacency_probe.py` | **blocked** → open 3 days |

Run against the gate's own script: the mac file list gives `eligible: all 2 changed file(s) are on the auto-merge allowlist`, rc=0; this lane's gives `not eligible: tests/a38_lane_sweep_probe.py is not on the auto-merge allowlist`, rc=1. The workflow `.github/workflows/auto-merge.yml` then logs *"left for a human"* and stops. **This lane opted its own bookkeeping out of the fast lane and then spent twelve cells resolving the conflicts that follow from staying in the queue.**

### Measured across all 120 PRs, not inferred from the five in front of me

Eligibility computed by importing `automerge_eligible.classify` and applying it to each PR's file list from the API:

| cohort | n | still open | merged | median age | p90 | max |
|---|---|---|---|---|---|---|
| **allowlist-eligible** | 66 | **0** | 65 | **0.0 h** | 0.0 h | 3.1 h |
| **allowlist-blocked** | 54 | **12** | 41 | 0.6 h | 21.5 h | 129.6 h |

**Not one eligible PR in the repository's history is stuck, and all twelve that are stuck are blocked.** Eligibility is sufficient for a same-day merge; every open PR, in all three lanes, fails it. Per lane, over PRs touching `JOURNAL.md`: win 15 pure / 19 impure (6 of the impure still open), mac 44 / 22 (3 open), linux 3 / 3 (1 open). The win lane made the majority of its journal PRs impure; mac made a third.

**This does not contradict A38, it bounds it.** The branch-versus-branch divergence the 09-27 and 09-28 cells measured is real and unchanged. But it only ever gets to matter to a PR that is *waiting*, and a pure bookkeeping PR does not wait long enough to be caught by it. The 09-23 cell's *"the fleet's automation is systematically faster at recording problems than at fixing them"* is right, and the half it missed is that **recording is only fast when the recording PR is pure** — which is a property of what a run puts in it, not of the process.

### What I changed

I split the two PRs along the line the allowlist already draws, rather than proposing any change to it:

- **#617 is now documentary only** — `BACKLOG.md` + `JOURNAL.md`, nothing else. It carries this lane's 09-26, 09-27, 09-28 and 09-29 entries, the refreshed `win` NEXT cell, and the **A40** row that previously existed only on #618. `automerge_eligible.py` on its changed-file list: **`eligible`, rc=0**.
- **#618 now carries the two probes and nothing else** — `tests/a38_lane_sweep_probe.py` and `tests/a38_row_adjacency_probe.py`, byte-identical to what it already held. It touches **no bookkeeping file**, which is the [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) property the 09-23 cell identified as the only one that lets a PR merge alongside any other. It stays blocked, correctly: it is code, and A4 sends code to a human by design.

**One trap, and `check-links` is what catches it.** #617's prose linked `tests/a38_lane_sweep_probe.py` by relative path, and stripping the file would have left a broken link and a red lint — which would have kept the PR from auto-merging just as effectively as the file did. There was exactly one such link and zero to the row-adjacency probe; the link is now inline code, per the 09-22 convention (*"identical relative links are only safe for files already on `main`"*). This is the same hazard the 09-28 cell hit from the opposite direction, where the fix was to carry the file rather than drop the link.

### STEP 1.5 resolution

`git merge-tree --write-tree --name-only origin/<branch> origin/main` first, on all seven `tide/win/**` PRs: five `CLEAN` ([#586](https://github.com/JeffMcClintock/TideSynth/pull/586) E80, [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) E82, [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) E19, [#597](https://github.com/JeffMcClintock/TideSynth/pull/597) A38, [#614](https://github.com/JeffMcClintock/TideSynth/pull/614) A39), two conflicting in `BACKLOG.md` and `JOURNAL.md`. All seven carry 13 SUCCESS / 2 SKIPPED, no review decision, 0 unresolved threads. Per STEP 1.5 the five green ones are waiting for merge and not mine to touch; I did not touch them.

Resolution was the standing recipe: `JOURNAL.md` by date order (`main`'s 09-29 macOS entry above the branch's 09-28 windows entry), `BACKLOG.md` by row ownership (`win` from the branch, `mac` from `main`). Verified lossless by `## 20…` heading-set arithmetic over `JOURNAL.md` + `JOURNAL-2026-08.md` + `JOURNAL-2026-09.md`, with the branch side from `ORIG_HEAD`:

| branch | main | union | merged | missing | extra | dup |
|---|---|---|---|---|---|---|
| 459 | 457 | 460 | 460 | **0** | **0** | **0** |

**Learned:**

- **Before resolving a bookkeeping conflict for the Nth time, run the changed-file list through `automerge_eligible.py`.** One command. If it says `not eligible`, the conflict is a consequence of the PR's file list and re-resolving it does not address anything — the PR will still be sitting there when the next merge lands.
- **A measurement on the PRs in front of you cannot see a gate that filters which PRs are still in front of you.** Twelve cells measured conflicts among the stuck PRs and concluded the queue was slow. The eligible PRs were invisible precisely because they merged: 66 of them, 0 still open. **Survivorship, in a queue whose survivors are the failures.**
- **Keep evidence out of the PR that records it.** A probe, fixture or script in a bookkeeping PR converts a zero-hour merge into an indefinite wait, and the cost is paid by every other open PR through the conflicts that follow. Two PRs is the cheap shape: documents auto-merge, code waits for a human.

**Not verified:** I built nothing and ran no host — this run changed no code, and the only non-document files touched were moved between two branches with identical blobs. The 65-of-65 and 0-of-66 figures are from the GitHub API's own `files` lists scored by the repository's own allowlist script; I did not re-run any historical merge. Whether #617 actually auto-merges is CI's to demonstrate and is the next cell's first check.

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and never left it — all work was in `git worktree`s under the scratchpad, removed at the end. **The developer was at the machine and one of the trees is TIDE-adjacent:** Visual Studio on `TiDEModules - TiDESliderSwitchGui.cpp` and on `SynthEditStore - ResizeAdorner.cpp`, plus Outlook and Slack. No build, no host, no screen taken, and I did not touch `SE16`, `SynthEditLib`, `gmpi_ui` or `GMPI_Wrappers`.

**Next:** see the `win` NEXT cell. For Jeff: **#617 should merge itself** — if it has, the experiment held. #618 is two probe files and conflicts with nothing, so it can merge at any time in any order.

**Branch/PR:** no new branch. This entry and the refreshed `win` NEXT cell are on [#617](https://github.com/JeffMcClintock/TideSynth/pull/617) (`tide/win/2026-09-26-step15-and-sweep-measurement`); [#618](https://github.com/JeffMcClintock/TideSynth/pull/618) (`tide/win/2026-09-27-adjacency-measurement`) now carries only the two probes.

## 2026-09-29 — macos — STEP 1.5: #585 (A36) re-conflicted in `JOURNAL.md` alone after #619, as predicted; STEP 2 walked, nothing eligible (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36). I took no backlog item. STEP 1 was empty: there is no open `platform:mac` issue in TideSynth, SynthEdit, SynthEditLib, gmpi_ui or GMPI_Wrappers. `FLEET-PAUSED` is absent on `origin/main`.

### STEP 1.5

The only commit on `main` since the 09-28 cell was that cell's own bookkeeping, [#619](https://github.com/JeffMcClintock/TideSynth/pull/619) (`bf23fc1`). It touched `JOURNAL.md` and the `mac` NEXT cell only. `git merge-tree --write-tree --name-only origin/<branch> origin/main` gave this:

| PR | state before | conflicting paths | checks | reviews / unresolved threads |
|---|---|---|---|---|
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) A36 | `DIRTY` | **`JOURNAL.md` only** | 13 SUCCESS, 2 SKIPPED | none / 0 |
| [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) E72 | `CLEAN` | none | 13 SUCCESS, 2 SKIPPED | none / 0 |
| [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) E81 | `CLEAN` | none | 13 SUCCESS, 2 SKIPPED | none / 0 |
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) | `DIRTY` | superseded; left for Jeff to close unmerged | | |

**Resolution:** the same one as 09-28. I kept A36's Rotation header (HEAD lines 12–97), then `main`'s entries, and removed the three line-start markers. Merge commit `92f3fa2` is pushed to `tide/mac/A36-journal-rotation-rule`.

**Verification:** I compared `## 20…` heading sets over `JOURNAL.md` + `JOURNAL-2026-08.md` + `JOURNAL-2026-09.md`. The branch side was taken from `ORIG_HEAD`.

| branch | main | union | merged | missing | extra | dup |
|---|---|---|---|---|---|---|
| 456 | 34 | 457 | 457 | 0 | 0 | 0 |

These lint checks all exited 0, run the way `lint.yml` runs them: `check-links`, `check-id-refs`, `check-next-block`, `check-backlog-archived`, `check-journal-prepend` (`prepend-only, OK`), `check-backlog-diff` and `check-prompt-provenance`. The last three got `git show origin/main:` base files and `--changed-file` from `git diff --name-only origin/main -- '*.md'`. `check-commit-completeness --record`/`--verify` bracketed the commit (`--verify` skips merges). `check-commit-authorship --range origin/<branch>..HEAD` reported `no unpushed commit is misattributed`, and `ls-remote --get-url origin` returned `https://`.

### STEP 2: nothing eligible

`git diff a7dec9a origin/main -- BACKLOG.md` is one changed line, the `mac` NEXT cell, and `docs/decisions.md`/`docs/lessons.md`/`PLAN.md` did not change at all. **Every row is therefore byte-identical to what the 09-26 and 09-28 cells walked.** Every PR that held a row is still open: A38 (#597), A39 (#614), E19 (#590), E80 (#586) and E82 (#587) for win, E79 (#584) for linux, and E72 (#588) and E81 (#589) for this lane. The remaining TODO rows stay ineligible for the reasons already on record: A35 waits on an open `PROPOSED:`, S8 is NEEDS-SPEC, X2 is linux in substance, E2 is an umbrella with no stated module set, E76 wants a ruling, and E84 is a workflow edit this token cannot make. No IN-REVIEW row's PRs merged, so there was nothing to flip.

**Learned:**

- **The 09-28 prediction held exactly.** A single bookkeeping PR on `main` re-conflicted #585 and nothing else. This is now three consecutive mac cells whose whole output is the same one-hunk `JOURNAL.md` resolution. The loop can't end from inside this lane, because this cell's own PR re-conflicts #585 when it lands. Only merging #585 (or a ruling on A38/[#597](https://github.com/JeffMcClintock/TideSynth/pull/597)) ends it.

**Not verified:** I built nothing, and this run changed no code. The only edit to a PR branch was the `JOURNAL.md` conflict resolution.

**Machine state:** `~/Documents/GitHub/TideSynth` started and ended on `main`, clean. All work was done in `git worktree`s under the scratchpad, and I removed them at the end. I did not touch `SynthEdit` (`master`, clean). I launched no host and did no GUI work.

**Next:** see the `mac` NEXT cell. For Jeff: **merge #585 first**, then #588/#589, and close #604 unmerged.

**Branch/PR:** `tide/mac/2026-09-29-step15` holds this entry and the refreshed `mac` NEXT cell. The merge is on `tide/mac/A36-journal-rotation-rule`.
## 2026-09-28 — windows — a merge order published in this cell has a shelf life of one merge; #617 and #618 made no-ops against each other, and #617 is now a free merge (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1 empty. STEP 1.5 on this lane's two bookkeeping PRs, [#617](https://github.com/JeffMcClintock/TideSynth/pull/617) and [#618](https://github.com/JeffMcClintock/TideSynth/pull/618), which had both gone `DIRTY`. STEP 2 found nothing eligible, eleventh cell. I took no backlog item, opened no new PR, and pushed to none of the five green product PRs.

### STEP 1 and STEP 1.5

No open `platform:win` issue — and that still verifies nothing, because `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing at all. `main`'s build is green on all seven jobs at `2e9235a97`.

Of the seven open `tide/win/**` PRs, five were `MERGEABLE`/`CLEAN` with every check green, no review decision and nothing unresolved — [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) (E80), [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) (E82), [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) (E19), [#597](https://github.com/JeffMcClintock/TideSynth/pull/597) (A38), [#614](https://github.com/JeffMcClintock/TideSynth/pull/614) (A39). Per STEP 1.5 those are waiting for merge and not mine to touch, and I did not touch them. The two that were `CONFLICTING`/`DIRTY` were #617 and #618, this lane's own bookkeeping branches from 09-26 and 09-27.

### The 09-27 cell's merge order was already dead, and one merge killed it

The `win` cell told the next run to merge **#617 first, #614 second**, on a measurement of `{1: 480, 2: 240}` over 720 orderings, because #617 *"blocks nothing and is blocked by nothing"*. Between that run and this one `main` advanced by **exactly one commit** — `bf23fc1ce`, macOS's own bookkeeping PR [#619](https://github.com/JeffMcClintock/TideSynth/pull/619).

I re-ran the same probe, `tests/a38_lane_sweep_probe.py`, on the same lane, changing nothing first:

| | 09-27 (6 branches) | today, before I touched anything (7 branches) |
|---|---|---|
| alone into `main` | 6 of 6 CLEAN | **5 of 7** — #617 and #618 both `CONFLICT: BACKLOG.md, JOURNAL.md` |
| depth histogram | `{1: 480, 2: 240}` | **`{0: 1440, 1: 3600}`** over all 5040 orderings |
| landing all of them | 0 of 720 | **0 of 5040** |
| depth-2 matrix | #617 free | **after ANY of the five, all six others conflict** |

So the recommendation did not survive to be used. **A merge order published in the NEXT cell has a shelf life of about one merge, and four consecutive cells have published one anyway.** That is not a criticism of the 09-27 measurement, which was correct when taken and which I reproduced with its own tool — it is a fact about what kind of claim is worth writing down. A depth number describes a queue that moves; the invariant does not, and the invariant here has held since 09-22: **this lane lands one PR per sweep and re-conflicts the rest, whatever order anyone writes down.**

### What I changed, and the one thing that made it work

#617 and #618 conflicted with **each other**, not only with `main` — which the 09-27 cell had itself predicted as structural: *"under the current layout every windows run necessarily conflicts with the previous windows run's bookkeeping PR."*

So I did not resolve them branch-by-branch against `main`, which is what six previous cells did and what leaves them still mutually blocking. I resolved both **and made their contended regions byte-identical**, so they are no-ops against each other:

- **`JOURNAL.md` is now byte-identical on the two branches** — equal `git hash-object`, checked after every commit including the one adding this entry — carrying the 09-26, 09-27 and 09-28 windows entries plus `main`'s 09-28 macOS entry, in date order. Each branch therefore carries the other's entry deliberately.
- **Both `win` NEXT cells are the same string** (row `md5 2335c323…`).
- **Both branches carry both A38 probes**, with identical blobs.

That last one was not planned — `check-links` caught it. The cross-carried 09-26 entry links `tests/a38_lane_sweep_probe.py` by relative path, and that file existed only on #617, so whichever of the two merged first would have put a broken link on `main` under one order and not the other. **This is the 09-22 cell's warning arriving in practice:** *"identical relative links are only safe for files already on `main`."* Carrying both files on both branches fixes it and conflicts with nothing, because the blobs are equal.

### Measured after, and it is better than the fix I was aiming at

Same method, `git merge-tree --write-tree` plus `commit-tree`, no checkout:

| | before | after |
|---|---|---|
| alone into `main` | 5 of 7 | **7 of 7** |
| ordered pairs blocked | 30 of 30 measurable | **22 of 42** |
| `#617` then `#618` | conflict | **both land** |
| `#618` then `#617` | conflict | **both land** |
| after `#617` lands | (did not merge at all) | **blocks NOTHING — all six others still merge** |
| after `#618` lands | (did not merge at all) | blocks `#614` only, in `BACKLOG.md` |

**#617 is now a free merge: it lands and costs no other PR its mergeability.** #618 costs only #614, and that is the A40-row-at-line-70 against A39-row-at-line-69 adjacency the 09-27 cell measured and chose not to dodge.

**Read that as valid at `bf23fc1ce` and expiring on the next merge into `main`** — which is the whole point of the section above, and applies to this paragraph exactly as much as to the one it corrects.

### The rule and the measurement disagree, and I did not resolve it

The five product PRs are individually `CLEAN` and collectively land one: after any of them merges, the other four conflict, always in `BACKLOG.md` and `JOURNAL.md`, with `docs/decisions.md` on four of five. The same byte-identical treatment would take the lane from depth 1 toward depth 7 — but applying it means **pushing to five green PRs**, and STEP 1.5 says a green PR with nothing unresolved is waiting for merge and not a run's to fix.

I followed the rule. **But the rule is written for a PR that is finished, and it reads "clean" as "done", when the measurement says these five are clean and stuck.** That is worth a ruling rather than a run's unilateral decision, and it belongs to A38. I have not implemented any of A38's four options and this is not a fifth.

### STEP 2: nothing eligible, eleventh cell

Fourteen `TODO` rows walked on `origin/main`. A38/A39/E19/E80/E82 are this lane's own open PRs; E72/E81 are mac's ([#588](https://github.com/JeffMcClintock/TideSynth/pull/588), [#589](https://github.com/JeffMcClintock/TideSynth/pull/589)); E79 is linux's ([#584](https://github.com/JeffMcClintock/TideSynth/pull/584)); A35 is parked on its own two open `PROPOSED:` entries; S8 is `NEEDS-SPEC`; E2 is an umbrella with no statable Accept; E76 is linux plus a ruling; X2's `Plat` cell is `linux`, which STEP 2 test (b) excludes; and E84 is a `.github/workflows/**` edit this credential deliberately cannot make.

**Learned:**

- **Re-measure before you act on any depth number in a NEXT cell, including one you wrote.** One unrelated merge on `main` took this lane from *"merge #617 first, #614 second"* to *"#617 does not merge at all"*. The cost of checking is one command; the cost of not checking is acting on a plan that no longer describes the repository.
- **Resolving N branches against `main` is not the same work as making them agree with each other**, and only the second one raises sweep depth. Six cells did the first. The difference is visible only in a branch-versus-branch merge, which no `mergeStateStatus` and no branch-versus-`main` check reports.
- **Byte-identical content is the cheap fix, and its one real cost is relative links.** A link is only safe in cross-carried text if its target is already on `main`, or is carried alongside it. `check-links` catches this; nothing else in the lint does.

**Not verified:** I built nothing and ran no host. This run changed no code — the only non-document files touched are two A38 probes copied verbatim between two branches. The after-state numbers above are from direct `merge-tree` measurement at each branch's final commit; the `{0: 1440, 1: 3600}` before-histogram is `tests/a38_lane_sweep_probe.py` run unmodified against the branches exactly as found.

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and never left it — all work was done in `git worktree`s under the scratchpad, which I removed at the end. **The developer was at the machine and not in a TIDE tree:** Visual Studio on `SimulatorGmpi - SimulatorForm.h`, plus Chrome and GitHub Desktop. No build, no host, no screen taken. I did not touch `SE16`, `SynthEditLib`, `gmpi_ui` or `GMPI_Wrappers`.

**Next:** see the `win` NEXT cell. For Jeff, measured today and expiring on the next merge: **#617 is the one free merge in this lane** — it lands and re-conflicts nothing. #618 second costs only #614.

**Branch/PR:** no new branch. This entry and the refreshed `win` NEXT cell are committed byte-identically onto `tide/win/2026-09-26-step15-and-sweep-measurement` (#617) and `tide/win/2026-09-27-adjacency-measurement` (#618).

## 2026-09-28 — macos — STEP 1.5: only A36 re-conflicted after #616, in `JOURNAL.md` alone; STEP 2 walked, nothing eligible (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on one of this lane's PRs, [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36). I took no backlog item. STEP 1 was empty: there is no open `platform:mac` issue in TideSynth, SynthEdit, SynthEditLib, gmpi_ui or GMPI_Wrappers.

### STEP 1.5

The only thing that landed on `main` since the 09-26 cell was that cell's own bookkeeping, [#616](https://github.com/JeffMcClintock/TideSynth/pull/616) (`a7dec9a`). It changed `JOURNAL.md` (one prepended entry) and the `mac` NEXT cell in `BACKLOG.md`.

I measured with `git merge-tree --write-tree --name-only origin/<branch> origin/main` first:

| PR | state before | conflicting paths |
|---|---|---|
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) A36 | `DIRTY`/`CONFLICTING` | **`JOURNAL.md` only** |
| [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) E72 | `CLEAN` | none |
| [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) E81 | `CLEAN` | none |
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) | `DIRTY` | superseded; left for Jeff to close unmerged |

All four have every check green, no review decision and 0 unresolved review threads.

**`docs/lessons.md` did not conflict on any branch this time.** The 09-26 fix, which took `main`'s copy verbatim, held across one `main` merge. A36's copy is still hash-identical to `origin/main`'s (`fbc56f0`). The remaining conflict is the one [#618](https://github.com/JeffMcClintock/TideSynth/pull/618) (windows, 09-27) predicts for this lane. A36 rewrites the region between the file header and the first entry, where every run's new entry is prepended, so any journal PR landing on `main` re-conflicts A36. E72 and E81 carry only an ordinary prepended entry further down, and `git` merges that cleanly.

**Resolution:** I kept A36's Rotation header, then `main`'s entries, and removed the three markers. Merge commit is `c6c5e3b`, pushed to `tide/mac/A36-journal-rotation-rule`, after which the PR showed `MERGEABLE` (`UNSTABLE` while checks ran).

**Verification.** I compared `## 20…` heading sets over `JOURNAL.md` + `JOURNAL-2026-08.md` + `JOURNAL-2026-09.md`, with the branch taken from `ORIG_HEAD`:

| branch | main | union | merged | missing | extra | dup |
|---|---|---|---|---|---|---|
| 455 | 455 | 456 | 456 | 0 | 0 | 0 |

These lint checks all exited 0, run as `lint.yml` runs them: `check-links`, `check-id-refs`, `check-next-block`, `check-backlog-archived`, `check-journal-prepend` (`1 new entry prepended … OK`, with `--changed-file` from `git diff --name-only origin/main -- '*.md'`), `check-backlog-diff` and `check-prompt-provenance`, each against `git show origin/main:` bases. `check-commit-completeness --record`/`--verify` ran around the commit (`--verify` skips merges). `check-commit-authorship --repo .` reported `all commits authored by tide-rack-bot`. `ls-remote --get-url origin` returned `https://`.

### STEP 2: nothing eligible

`git diff 1db622c origin/main -- BACKLOG.md` is a single changed line, the `mac` NEXT cell. **Every row is therefore byte-identical to the one the 09-26 cell walked**, and every PR that held a row then is still open. That covers A38 (#597), A39 (#614), E19 (#590), E80 (#586) and E82 (#587) for win, E79 (#584) for linux, and E72 (#588) and E81 (#589) for this lane. The remaining TODO rows are ineligible for the reasons already on record: A35 waits on an open `PROPOSED:`, S8 is NEEDS-SPEC, X2 is linux in substance, E2 is an umbrella with no stated module set, E76 wants a ruling, and E84 is a workflow edit this token cannot make.

**Learned:**

- **The take-main's-`lessons.md` recipe survived one `main` merge in this lane.** Two of three mac PRs stayed `CLEAN`, and the third conflicted in `JOURNAL.md` alone. That is a single data point, and it agrees with #618's measurement that `JOURNAL.md` is what blocks mac pairs.
- **A36 will re-conflict on every journal PR until it merges**, this one included, because the region it rewrites is the prepend point. No per-branch recipe can fix that. Merging #585 first is the only fix.

**Not verified:** I built nothing. This run changed no code. The only edit to a PR branch was a `JOURNAL.md` conflict resolution.

**Machine state:** `~/Documents/GitHub/TideSynth` started and ended on `main`, clean. All work was done in `git worktree`s under the scratchpad, and I removed them at the end. I did not touch `SynthEdit` (`master`, clean). I launched no host and did no GUI work.

**Next:** see the `mac` NEXT cell. For Jeff: **merge #585 first**, because any other journal PR landing before it re-conflicts it. Then merge #588/#589, and close #604 unmerged.

**Branch/PR:** `tide/mac/2026-09-28-step15` holds this entry and the refreshed `mac` NEXT cell. The merge is on `tide/mac/A36-journal-rotation-rule`.

## 2026-09-27 — windows — the livelock's CAUSE measured: A38's option (b) buys 4 of 21 pairs, and `JOURNAL.md` is the file that actually blocks (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1 empty. STEP 1.5 needed nothing, second cell running. STEP 2 found nothing eligible, tenth cell. That left the run free to answer the question three consecutive cells have argued from the wrong evidence, and it produced a result that corrects two of my own lane's cells.

### STEP 1 and STEP 1.5

No open `platform:win` issue — and that verifies nothing on this platform, because `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing at all. `main`'s build is green on all seven jobs at `2e9235a97`. The only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, another box's) and [#44](https://github.com/JeffMcClintock/TideSynth/issues/44), the watchdog digest.

All six `tide/win/**` PRs — [#617](https://github.com/JeffMcClintock/TideSynth/pull/617), [#614](https://github.com/JeffMcClintock/TideSynth/pull/614), [#597](https://github.com/JeffMcClintock/TideSynth/pull/597), [#590](https://github.com/JeffMcClintock/TideSynth/pull/590), [#587](https://github.com/JeffMcClintock/TideSynth/pull/587), [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) — were `MERGEABLE`/`CLEAN`, 13 of 13 checks green, 2 skipped, **zero reviews and zero unresolved review threads** (GraphQL `reviewThreads`, checked per PR). Nothing was pushed to any of them. Per STEP 1.5 a green PR with nothing unresolved is waiting for merge and not mine to touch.

### The depth moved, and #617 is why

The `win` cell told me to run `tests/a38_lane_sweep_probe.py` and record the depth rather than re-resolve. Done, both arms:

| arm | alone into `main` | depth histogram, all 720 orderings | landing all 6 |
|---|---|---|---|
| **CONTROL** (branches replaced by their merge bases) | 6 of 6 CLEAN | `{6: 720}` | 720 of 720 |
| **TREATMENT** (the real branches) | 6 of 6 CLEAN | **`{1: 480, 2: 240}`** | 0 of 720 |

So the 09-26 cell's `{1: 120}` is out of date: **240 of 720 orderings now land two.** The cause is #617 itself, which **blocks nothing and is blocked by nothing** — the only PR in the lane with that property. Every ordering with #617 first or second reaches depth 2, and 120 + 120 = 240 accounts for the histogram exactly, which is what makes that the explanation rather than a guess. **Merge #617 first, #614 second.**

### The cause, which no previous cell measured

Three cells running have argued A38 from **sweep depth alone** and reached three different answers. Sweep depth cannot settle it. New probe `tests/a38_row_adjacency_probe.py`, three arms and three controls, no checkout — in-memory `merge-tree --write-tree` plus `commit-tree`, so it was safe to run against a tree the developer was working in.

**Arm 1 — blob divergence across `main` and all eleven `tide/**` branches.** A file byte-identical on two branches cannot conflict between them however often a merge names it, so this separates *hot* from *merely named*:

| file | distinct blobs (12 refs) | differ from `main` | reading |
|---|---|---|---|
| `BACKLOG.md` | **12** | **11 of 11** | maximally divergent |
| `JOURNAL.md` | **12** | **11 of 11** | maximally divergent |
| `docs/decisions.md` | 5 | 6 of 11 | partly |
| `BACKLOG-DONE.md` | 4 | 6 of 11 | partly |
| `docs/lessons.md` | 4 | **3 of 11** | inert on eight branches |

Control **C1** checks the inference both ways — blob equal to `main`'s must give an empty `git diff` in that file, blob different must give a non-empty one — over 55 branch-file pairs, **0 mismatches**.

**This corrects my own 09-26 cell.** `BACKLOG-DONE.md` is not a fifth hot file in any sense comparable to the first two: four win branches touch it and **all four share one identical blob**, so it conflicts nowhere in this lane. It is a real blocker in the mac lane only (A36 versus E72). *"Named in a merge"* is not *"divergent"*, and the fleet sweep probe reports paths, not divergence.

`docs/lessons.md` confirms the 09-25 recipe spread: only `linux/E79`, `mac/E81` and `mac/issue-599` still carry a regenerated copy, and two of those three are stuck for other reasons.

**Arm 2 — the touched-line map.** Which `BACKLOG.md` lines each win branch's diff against `main` actually touches:

```
2026-09-26-step15-and-sweep-measurement      L11(win)
A38-bookkeeping-livelock                     L68(A38)
A39-prefab-count-derived                     L69(A39)
E19-datatype-census                          L68(A38), L92(E19)
E80-clap-editor-arm                          L68(A38), L115(E80), L119(E85)
E82-rack-menu-producer                       L68(A38), L117(E82)
```

**The single most contended line in the fleet is `BACKLOG.md` line 68 — the A38 row itself — rewritten by four separate runs.** A39 sits at line 69, adjacent and in disagreement with nobody. #617 sits at line 11 and collides with no one.

**Arm 3 — the causal test.** Re-merge every pair after inserting one blank line between adjacent table rows, applied to the merge base **and both branches**:

- **Adjacency-only pairs** — A39 against each of the four line-68 branches — go `BACKLOG.md`-conflicting → **CLEAN, 4 of 4.**
- **Same-line pairs** — the four that all rewrite line 68 — **still conflict.** That is control **C2**, and it is what makes the result a measurement rather than a corrupted merge.
- Control **C3**: the transform creates no new conflict anywhere.

**So option (b) fixes 4 of 15 win pairs.**

**The mac lane is the replication, and it is starker.** No mac branch pair touches the same or an adjacent `BACKLOG.md` line; `BACKLOG.md` appears in **zero of its six pair conflicts**; and **five of six still conflict — in `JOURNAL.md`.** Respacing `BACKLOG.md` does nothing at all for that lane. Fleet total for (b): **4 of 21 pairs.**

**So A38's option (d), per-run journal files, is aimed at the file that actually blocks — and the 09-25 cell's *"the journal half is conditional on run density"* is wrong as a general claim.** It measured branch-versus-`main`. Branch-versus-branch, `JOURNAL.md` is maximally divergent (12 of 12 blobs) and is the sole blocker for five of six mac pairs. That is the same inference error the 09-26 cell caught in the 09-25 cell, one file over — and this is the third cell in a row to make a version of it, which is the part worth noticing.

**The cost of (b), measured rather than assumed, because it is not what you would guess.** The repo's own lint **does not object**: `check-next-block.py` and `check-id-refs.py` both exit 0 on a fully respaced `BACKLOG.md`, A/B against the original, byte-identical output on both arms. What breaks is **rendering**. GitHub's own renderer, `POST /markdown` mode `gfm`, drops the row after a blank line out of the `<table>` and emits it as a paragraph of literal pipes:

```
arm a (no blank line)          <tbody><tr><td>A38</td>…<tr><td>A39</td>…</tbody>
arm b (blank line between)     <tbody><tr><td>A38</td>…</tbody></table>…<p>| A39 | TODO |</p>
```

So (b) as worded means `BACKLOG.md` stops being a table on github.com. That is a real cost for the ruling to weigh, and it is not a lint problem.

**A fifth option A38 does not list, offered as evidence and not implemented here: stop appending run narrative to an existing row.** Five of the nine win `BACKLOG.md` pair conflicts are four runs rewriting the A38 row. A convention of *"your narrative goes in the journal and the NEXT cell, never into another row"* removes them, costs nothing structural, and needs no layout change. **A38 remains Jeff's ruling** and nothing here implements any of its four options.

### This branch confirms the finding at its own expense

I measured my own branch against all eleven others before pushing, because a run
that names a mechanism should be able to predict its own cost:

| against | result |
|---|---|
| `origin/main` | **CLEAN** |
| 6 of 11 branches (win A38/E19/E80/E82, mac E72/E81) | **CLEAN** |
| **[#614](https://github.com/JeffMcClintock/TideSynth/pull/614)** (win, A39) | **`BACKLOG.md` alone** |
| [#617](https://github.com/JeffMcClintock/TideSynth/pull/617) (win) | `BACKLOG.md` + `JOURNAL.md` |
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (mac, A36) | `JOURNAL.md` |
| [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (linux, E79) | `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md` |
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) (mac, superseded) | `scripts/check-rack-populated.py` — inherited, not mine; #604 predates `53a23a03e` |

**Both win-lane conflicts were predicted by arm 2 before they were measured.**
A40 is a pure insertion at line 70 and #614 edits line 69 — the same adjacency
that blocks A39 against the four line-68 branches. #617 and I both rewrite line
11, which is same-line disagreement. Nothing here is a surprise, and that is the
point: the probe's output was usable as a prediction.

**I kept A40 in ID order at the end of the Process-hardening table rather than
dodging the collision.** Moving it would have cost the section its ID ordering
permanently for a transient merge benefit, and it could not have made this branch
free anyway — the line-11 collision with #617 is unavoidable for any run that
refreshes its own NEXT cell. That last clause is worth reading twice: **under the
current layout every windows run necessarily conflicts with the previous windows
run's bookkeeping PR**, which is a structural fact about the NEXT block that
A38's four options do not address either.

### STEP 2: nothing eligible, tenth cell

Fourteen `TODO` rows walked. A38/A39/E19/E80/E82 are this lane's own open PRs; E72/E81 mac's; E79 linux's; A35 parked on its own two `PROPOSED:` entries; S8 `NEEDS-SPEC`; E2 an umbrella with no statable Accept; E76 linux plus a ruling; E84 a `.github/workflows/**` edit the credential deliberately cannot make.

**X2 deserves its reason written down rather than inherited.** Three cells have called it *"linux in substance"* without saying why, and **its own prose says the opposite**: *"the Windows half belongs to this box and I did not do it."* That is a windows run inviting this lane in. But its `Plat` cell is `linux`, so STEP 2 test (b) excludes it — **and reading the prose over the column is precisely the question A35's open `PROPOSED:` entry asks**, which makes X2 ineligible twice over rather than once. Writing that down is worth more than repeating the conclusion.

**E83 was left alone deliberately.** It is `IN-REVIEW` with its PR merged and the watchdog lists it as ready to flip, but the flip is already on both #585 and #588 dated `2026-09-09`, and a third copy would be the `BACKLOG-DONE.md` collision the 09-26 cell walked into. Not repeated.

### Filed A40

`scripts/watchdog-digest.py:299` holds `TOKEN_EXPIRY = '2026-11-07'` as a literal, and every *"Expires … (N days away)"* line in the digest is arithmetic on it. **Nothing reads the credential.** And the header that would confirm it is absent: GitHub returns `github-authentication-token-expiration` for a classic PAT with an expiry set, the fleet credential is a classic PAT (`ghp_` prefix, 40 characters — measured by class and length only, never its value), and **`user` and `rate_limit` both return no such header** under `gh api -i`. A39's shape one file over.

**I checked before filing, per STEP 3 and the C15/C16 lesson:** `2026-11-07`, `agent-token` and `token expir` appear nowhere in `BACKLOG.md`, `JOURNAL.md`, `docs/decisions.md` or `docs/lessons.md` on `main`, **nor in `BACKLOG.md` on any of the eleven `tide/**` branches.** The row is genuinely unfiled.

**Not verified:** I could not establish the real expiry, and I have no positive control. The only other credential on this box is Jeff's `gh` keyring token, which is an OAuth token and would not carry the header either, so *"absent"* cannot be distinguished from *"this environment never shows it"* without a credential known to expire. Only the owner can read the date, in the GitHub UI. That half is on the row as the NEEDS-JEFF part; the coded half is correct under either answer, so A40 is takeable as written.

**Learned:**

- **Sweep depth cannot answer a question about a mechanism.** Three cells argued A38's options from the depth number and got three answers. Blob divergence answers *which file*, the touched-line map answers *which line*, and a re-merge after a targeted transform answers *why*. All three are one command each and need no checkout.
- **A control that fails is worth more than one that passes, and C2 caught a bug in its own probe.** `subprocess` in text mode translates each `\n` to `os.linesep` on **stdin**, so on Windows `git mktree` read the entry name as `BACKLOG.md\r` — a different path, which then conflicted with everything and looked exactly like a genuine merge result. Feed `mktree` and `hash-object` **bytes**. Without C2 asserting that same-line pairs must *still* conflict, the probe would have reported that blank lines fix everything.
- **A control can also be over-broad, and that is not a licence to loosen it.** C2 first flagged E19+E80, which touch line 68 but write the *same* text there and so never conflicted in `BACKLOG.md` at all. The fix is to require *"conflicted before"* as a precondition, not to drop the control — otherwise C2 is a restatement of the same-line set rather than a discriminator.
- **Text mode is the wrong default for git plumbing on this box in both directions.** Reading `BACKLOG.md` through `subprocess` text mode dies on cp1252 at the first em-dash, and the `UnicodeDecodeError` surfaces from a reader thread as an unrelated `NoneType` several frames later. Decode UTF-8 explicitly with `errors="replace"`, or use bytes.
- **A14's authorship check caught a real misattribution on this run, and the cause is the harness, not the box.** Each Bash call in this agent runs a **fresh shell** -- working directory persists, exported variables do not. I exported the four `GIT_*` variables in the call that made the commit, then ran `git commit --amend` in a *later* call that had only re-exported `MSYS_NO_PATHCONV`, so the amend took the committer from the box's git config: `author: tide-rack-bot`, **`committer: Jeff McClintock`**. STEP 0.7 had passed and would have passed again; the content was correct; every exit code was 0. **Re-export all four `GIT_*` variables in EVERY call that commits, amends or rebases** -- not once per run -- and run `check-commit-authorship.py` before every push, because it is the only thing between that mistake and a commit carrying the name that sits on every ruleset's bypass list. Fixed with `--amend --reset-author` while unpushed, which is the sanctioned route.
- **Check whether the fleet's own automation already covers a finding before filing it.** I had a second row drafted about the credential expiry going unnoticed; the digest already has an A12 liveness section that names a silent box, and it explains in its own text why a halted run cannot report itself. The remaining defect was one line narrower than the one I nearly filed.
- **`gh api` needs `MSYS_NO_PATHCONV=1` for a leading-slash endpoint in Git Bash here**, otherwise `/markdown` is rewritten to `C:/Program Files/Git/markdown` and the error blames the endpoint.

**Machine state.** `C:\SE\TideSynth` started and ended on `main`, clean, and **was never checked out to a branch** — all work happened in a `git worktree` under the session scratchpad, removed at the end. No other repo touched. **Nothing built, no host launched, no screen taken, no GUI driven.** The developer was at the machine throughout (Chrome, Outlook, GitHub Desktop, Steam; **no Visual Studio and no SynthEdit**, so no build was in flight and the 09-09 hazard of a source file changing under a build did not apply). A pure-measurement run is the right thing to take on a busy box — fourth cell to say so.

**Not verified:** no TIDE build was run on this box, so whether `main` compiles on windows locally is unknown to me; CI says green at `2e9235a97`. The `linux` box has been silent 26 days and the digest flags it `LIKELY HALTED`; that needs someone at that keyboard and I could not act on it.

**Next:** see the `win` NEXT cell. For Jeff: **merge #617, then #614** — measured as the only free pair in this lane. When you rule on A38, option (b) buys 4 of 21 pairs and costs `BACKLOG.md` its table, while (d) is aimed at the file that actually blocks.

**Branch/PR:** `tide/win/2026-09-27-adjacency-measurement` / [#618](https://github.com/JeffMcClintock/TideSynth/pull/618) holds this entry, the refreshed `win` NEXT cell, A40 and the new probe.

## 2026-09-26 — windows — the 09-25 "merging one does not re-conflict the others" is FALSE, measured 0 of 120 orderings; the livelock survived, only `docs/lessons.md` left it (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths agreed: REST `tide-rack-bot`, GraphQL `tide-rack-bot` / `314850083`, which is the number in `GIT_AUTHOR_EMAIL`)

**Did:** STEP 1 empty, STEP 1.5 needed **nothing for the first time in nine cells**, STEP 2 found nothing eligible — and then measured the claim this lane shipped on 09-25, because STEP 1.5 having nothing to do was itself the evidence that claim wanted testing. **It does not hold.** I took no backlog item and pushed to none of the five open PRs.

### The finding: a branch-vs-`main` measurement cannot answer a batch question, and the 09-25 cell answered one with it

The 09-25 cell (mine, this lane) resolved four PRs by taking `main`'s `docs/lessons.md` verbatim instead of regenerating it, re-measured with `git merge-tree --write-tree --name-only origin/<branch> origin/main`, got a clean result on all four, and wrote: *"ALL FOUR ARE NOW `MERGEABLE` AND, UNLIKE EVERY PREVIOUS SWEEP, MERGING ONE DOES NOT RE-CONFLICT THE OTHERS."*

**The first half is true and the second half is false, and no amount of the first measurement could have established the second.** `merge-tree <branch> <main>` asks whether a branch is *individually* mergeable. Whether **two** branches can both land is a question about branch-vs-branch content, which that command never looks at.

Measured exhaustively — 5 branches, so all 120 orderings enumerated, no search heuristic involved, every merge a real in-memory `merge-tree --write-tree` + `commit-tree`:

| arm | branches alone into `main` | depth histogram over all 120 orderings | orderings landing all 5 |
|---|---|---|---|
| **CONTROL** (each branch replaced by its merge base with `main`) | 5 of 5 CLEAN | `{5: 120}` | **120 of 120** |
| **TREATMENT** (the real branches) | **5 of 5 CLEAN** | **`{1: 120}`** | **0 of 120** |

**Every one of the 120 orderings stalls after the first merge.** The control is what makes that a measurement rather than a broken merge: substitute branches that provably contain nothing new and every ordering sweeps to depth 5.

The depth-2 matrix says who blocks whom and on what:

| after this lands | it blocks | conflicting bookkeeping paths | other |
|---|---|---|---|
| **#614** (A39) | #597, #590, #587, #586 | `BACKLOG.md` **only** | none |
| #597 (A38) | the other four | `BACKLOG.md`, `JOURNAL.md`, `docs/decisions.md` | none |
| #590 (E19) | the other four | `BACKLOG.md`, `JOURNAL.md`, `docs/decisions.md` | none |
| #587 (E82) | the other four | `BACKLOG.md`, `JOURNAL.md`, `docs/decisions.md` | none |
| #586 (E80) | the other four | `BACKLOG.md`, `JOURNAL.md`, `docs/decisions.md` | none |

**`docs/lessons.md` appears in NONE of them.** So the 09-25 fix did exactly what it was measured to do and nothing more: it removed one file from the conflict set. Confirmed independently — `git diff origin/main...origin/tide/win/<branch> --name-only` lists `docs/lessons.md` for **zero of the five** branches, and across the whole fleet it now survives on only **#584** (linux), the one lane that never adopted the recipe. The livelock was never *only* that file, so removing it moved the sweep depth by **zero**.

**Fleet-wide, the number is unchanged from the day A38 first measured it.** Re-ran the existing `tests/a38_fleet_sweep_probe.py` (from #597's branch) against all ten open PRs: **DEEPEST VERIFIED SWEEP: 2 of 10**, order `#614 -> #589`, control **10 of 10**. A38 measured **2 of 9** on 09-23. Six re-resolutions and one recipe change later, a full sweep still lands two.

**`BACKLOG-DONE.md` is a FIFTH hot bookkeeping file, and A38's row lists four.** The fleet probe reports it as a conflicting path on **6 of the 10** open PRs (#585, #586, #587, #588, #590, #597) — it is not in `BOOKKEEPING = {BACKLOG.md, JOURNAL.md, docs/lessons.md, docs/decisions.md}`, so every previous sweep classified it under "other" and it read as a code conflict. **The mechanism is row archiving, and I walked into it myself this run — see below.**

**Verification artifact:** `tests/a38_lane_sweep_probe.py`, new this run. Reproduces the 09-25 measurement, then enumerates every ordering of a lane exhaustively and prints the depth-2 matrix. `--control` substitutes merge bases and **fails loudly** if they do not sweep fully in every ordering. Both arms above are its output; `rc=0` on both. No worktree, no checkout, no ref updates — safe against a tree the developer is working in, which mattered today.

### E83: the bookkeeping I did NOT do, and why that was the right call

E83 was `IN-REVIEW` on `main` with its only cited PR ([#581](https://github.com/JeffMcClintock/TideSynth/pull/581)) merged 2026-09-08, and its Accept met by its own row (*"one showing the input is constant"* — the row measures exactly that, two independent ways, with a control). STEP 4 says such a row becomes `DONE` and moves to `BACKLOG-DONE.md`. **I made that edit, then reverted it.**

The 09-26 macOS entry says E83's flip *"already rides on the E72 branch"*. I checked rather than took it on trust, and it is on **two** branches, not one:

| ref | E83 in `BACKLOG.md` | E83 in `BACKLOG-DONE.md` |
|---|---|---|
| `origin/main` | `IN-REVIEW` | absent |
| `origin/tide/mac/E72-cable-dsp-dirty` (#588) | absent | **`2026-09-09`** |
| `origin/tide/mac/A36-journal-rotation-rule` (#585) | absent | **`2026-09-09`** |

**And I had used `2026-09-08`** — #581's actual merge date — so a third copy would have differed from the other two in the `Done` column. That is not a whitespace conflict, it is two archives disagreeing about a fact, in a file whose own rule is *"rows here are verbatim"*. It is also the C15/C16 / A31 duplicate-work shape: **two lanes doing one job from branches where the other's edit is invisible.**

**This is the `BACKLOG-DONE.md` mechanism, caught live.** The prompt's defence against it is STEP 3's *"grep the backlog for the file you are about to name, against freshly-fetched `origin/main`"* — which **cannot see this**, because the competing edit is not on `main`. What found it was checking the other lanes' *branches*, and that is the check worth having: `git show origin/<branch>:BACKLOG-DONE.md` before archiving any row.

### STEP 1.5 — nothing to do, and that is new

All five `tide/win/**` PRs were already `MERGEABLE` / `CLEAN` with **13 of 13 checks green**, no reviews, no requested changes, and no comments but this lane's own resolution notes. Per STEP 1.5 that is *"waiting for merge — not yours to fix"*, so I pushed to none of them.

**What makes it notable: `main` advanced TWO commits since those branches were last touched, and they survived it.** The branches were re-merged 09-25 at 10:11–10:19 UTC; `main` then took `1db622cb4` (#615, this lane's own bookkeeping, 10:23 UTC) and `a7dec9ab5` (#616, macOS bookkeeping, 14:11 UTC), **both touching `BACKLOG.md` and `JOURNAL.md`**. On 09-23 a single docs-only bookkeeping merge (`0a8a87c`) re-conflicted all four. So the 09-25 recipe **did** buy real immunity to `main` moving — it is just not the property the cell claimed, and it does not make the five landable together.

### STEP 1 — structurally empty on this platform, unchanged

`gh issue list --label platform:win` returns nothing, and it verifies nothing: `build.yml:523` excludes `matrix.platform != 'win'` from issue filing. Read `main`'s build instead — **green on all seven jobs at `2e9235a97`**, which is `main`'s newest *code* commit (#615/#616 are docs-only). The two open issues are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (`platform:linux`) and #44 (the watchdog digest); neither is mine.

### STEP 2 — nothing eligible, walked in full

Fourteen `TODO` rows, and every one is out for a reason I can state:

| row | why not |
|---|---|
| A38, A39, E19, E80, E82 | **this lane's own open PRs** (#597, #614, #590, #586, #587) — green and clean, STEP 1.5 says leave them |
| E72, E81 | mac's open PRs (#588, #589) |
| E79, X2 | linux in substance; #584 open |
| A35 | parked on its **own two `PROPOSED:` entries** in `docs/decisions.md` — STEP 2 forbids work an open question would change |
| S8 | `NEEDS-SPEC`, and its real cause sits in GATED `SynthEditLib/CMakeLists.txt` |
| E2 | umbrella; its own row says the Accept cannot be stated (which modules is a product decision) |
| E76 | linux in substance, and wants a ruling on whether a measurement script may edit the caller's environment |
| E84 | a `.github/workflows/**` edit; **the bot's token deliberately lacks `workflow` scope** |

Nothing has entered the queue since 09-18. I did not invent work.

**Learned:**

- **A resolution that makes every branch mergeable against `main` is not a resolution that lets any two of them land, and this fleet has conflated those for six runs.** The cheap discriminator is one probe run, not another sweep: enumerate the orderings. Sixteen re-resolutions across six runs could not have raised the depth, because resolving against `main` says nothing about whether two branches agree with each other.
- **State the property you measured, not the one you want.** The 09-25 cell had the right measurement and wrote down a stronger claim than it supports. The next run then reasonably planned a batch merge on it. A claim about a *set* needs a measurement over the *set*.
- **`BACKLOG-DONE.md` belongs in every list of hot bookkeeping files.** Two lanes archiving the same row is a content disagreement, not a merge artifact, and the prompt's `origin/main` grep is blind to it by construction. Check the other lanes' branches before archiving a row.
- **A NEXT cell can be confidently wrong, and it is still the best input available.** This cell's instruction (1) — check `mergeStateStatus` — was right and cost one command. Its framing was wrong. Both were worth reading.

**Not verified:** I built nothing and ran no host. No build was needed for a measurement that runs entirely on git objects, and the developer was at the machine. Whether the five PRs pass CI *after* being made batch-mergeable is not something I can know, because nothing was re-resolved this run. The sweep numbers describe `origin/main` at `a7dec9ab5` and go stale the moment anything merges.

**Machine state.** `C:\SE\TideSynth` started and ended on `main`, clean, and never left it — all work in a `git worktree` under the scratchpad. Its local `main` is one commit behind `origin/main` (`2e9235a97` vs `a7dec9ab5`); that is the developer's checkout and not mine to fast-forward. `SE16` (`master`), `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers` (all `main`) were clean and untouched. **The developer was at the machine** — Chrome, Outlook and GitHub Desktop, no Visual Studio, so no build in flight; **no screen taken, no GUI, no build run.** The probes write only loose git objects, no refs.

**Next:** see the `win` NEXT cell. **For Jeff, the one thing worth knowing: a sweep of the ten open PRs lands two, and re-resolving them again will not change that.** The queue is not slow because review is slow. If you want this lane's five to land, the order `#614 -> #589` is what a sweep gets, and #614 (A39) is the cheapest first because it conflicts on `BACKLOG.md` alone. #604 is superseded and should be closed unmerged.

**Branch/PR:** `tide/win/2026-09-26-step15-and-sweep-measurement` holds this entry, the refreshed `win` NEXT cell and the new probe. I pushed to none of the five open PRs.

## 2026-09-26 — macos — STEP 1.5 again, resolved with the take-main's-`lessons.md` recipe; A39 already claimed by windows, nothing else eligible (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on two of this lane's PRs. I took no backlog item. STEP 1 was empty, with no open `platform:mac` issue.

### STEP 1.5

Of the four open `tide/mac/**` PRs, [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) and [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72) were `DIRTY`/`CONFLICTING` again. [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) was `CLEAN`, and [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) is superseded and was left for Jeff to close. None of the four had failing checks or unresolved review threads (GraphQL `reviewThreads`, 0 unresolved on each).

**I measured before applying any recipe**, following the windows cell's 09-25 lesson. `git merge-tree --write-tree --name-only origin/<branch> origin/main` gave:

- **E72:** only `docs/lessons.md`. `BACKLOG.md`, `JOURNAL.md` and `SynthEditSem/TideApp.cpp`/`.h` auto-merged. The `TideApp` merge combines E72's change with Jeff's `2e9235a` (properties-browser pin edits delivered to the DSP live).
- **A36:** `docs/lessons.md`, plus the usual `JOURNAL.md` hunk, with A36's Rotation header on one side and `main`'s two new 09-25 entries on the other.

**Resolution.** On both branches I took `docs/lessons.md` from `origin/main` verbatim, and did not regenerate it. `git hash-object` equals `git rev-parse origin/main:docs/lessons.md` (`fbc56f0`). This is the windows lane's fix for the livelock. A regenerated copy differs per branch, so merging one PR re-conflicts the rest, and a verbatim copy does not. For A36, I kept the Rotation header and then `main`'s entries, and deleted the three markers.

**Nothing was lost.** I compared `## 20…` heading sets:

| branch | branch ∪ main | merged | missing | extra | dup |
|---|---|---|---|---|---|
| E72 (`JOURNAL.md`) | 31 ∪ 32 = 33 | 33 | 0 | 0 | 0 |
| A36 (`JOURNAL.md` + `JOURNAL-2026-08.md` + `JOURNAL-2026-09.md`) | 455 | 455 | 0 | 0 | 0 |

A36 counted `JOURNAL.md` alone shows 17 "missing". These are the entries its own rotation moves into the archive files, so the archive-inclusive count is the right one.

**Lint, both branches, all exit 0:** `check-links`, `check-id-refs`, `check-next-block`, `check-backlog-archived`, `check-journal-prepend`, `check-backlog-diff` and `check-prompt-provenance`. The last three were given file paths written from `git show origin/main:…`, as CI does. `check-commit-completeness --record`/`--verify` ran around each commit (`--verify` skips merge commits). `check-commit-authorship --range origin/<branch>..HEAD` found no unpushed misattributed commit. It reported three already-pushed non-bot commits, which are `main`'s own: two squash merges stamped `Tide Funkster` and Jeff's `2e9235a`. These are all accounted for. `ls-remote --get-url origin` returned `https://`. Pushes: E72 `eac2888`, A36 `815fad0`. Right after the push, both PRs showed `UNSTABLE`/`MERGEABLE`, meaning checks were running and there was no conflict.

### STEP 2: nothing eligible

**A39 is no longer available to this lane.** The 09-25 mac cell named it as next, but windows claimed it the same day: branch `tide/win/A39-prefab-count-derived`, DOING mark `4efad04`, PR [#614](https://github.com/JeffMcClintock/TideSynth/pull/614). Under STEP 2 it is taken. Every other TODO row, `A35`, `A38`, `S8`, `E19`, `X2`, `E2`, `E72`, `E76`, `E79`, `E80`, `E81`, `E82` and `E84`, is **byte-identical on `origin/main` to its 09-24 text** (compared by row hash). Each stays ineligible for the reason already on record:

- **Waiting on open `PROPOSED:` entries:** A35.
- **Taken by an open PR:** A38/E19/E80/E82 (win), E79 (linux), E72/E81 (this lane).
- **Parked by other constraints:** S8 is NEEDS-SPEC, and X2 is linux in substance. E2 is an umbrella with no stated module set. E76 needs a ruling. E84 is a workflow edit the bot's token cannot make.

E83 is still `IN-REVIEW`. Its flip already rides on the E72 branch, per the 09-17 cell, so I did not repeat it.

**Learned:**

- **A NEXT cell's "take X next" is not a claim, and a lane that names an item without pushing a DOING mark can lose it overnight.** That is correct behaviour, not a collision. The windows cell did exactly what STEP 2 prescribes. The 09-25 mac cell was right not to take A39 under STEP 1's wording, and this run inherited nothing to take.
- **When a branch rotates the journal, heading-set arithmetic must include the archive files.** On A36, `JOURNAL.md` alone shows 17 false "missing" headings, and `JOURNAL.md` + `JOURNAL-2026-0{8,9}.md` shows 455/455. In zsh, pass the three files as an array. A space-separated string is not word-split, so `cat $F` then reads nothing.

**Not verified:** I built nothing locally. E72's re-merged `TideApp.cpp` (with `2e9235a`) will be compiled by CI's `macos`/`windows`/`linux` legs, and those were still running when this was written. A36 now carries `main`'s `lessons.md`, while its branch also changes `scripts/extract-lessons.py`, so `extract-lessons.py --check` on that branch is expected to differ. CI does not run it. One `--write` is owed after it lands, as the NEXT cell says.

**Machine state.** `~/Documents/GitHub/TideSynth` started and ended on `main`, clean. All work was done in `git worktree`s under the scratchpad, which I removed at the end. `SynthEdit` (`master`) is clean and untouched. The screen was locked (`CGSSessionScreenIsLocked` present). I launched no host and did no GUI work.

**Next:** see the `mac` NEXT cell. For Jeff: merge #585, then #588/#589, and close #604 unmerged.

**Branch/PR:** `tide/mac/2026-09-26-step15` holds this entry and the refreshed `mac` NEXT cell. The merges are on `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty`.

## 2026-09-25 — windows — the bookkeeping livelock is ONE GENERATED FILE, not three; four PRs unblocked in a way that survives a merge, then A39 taken (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.9939.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on four `CONFLICTING` win PRs, then STEP 2 — **A39**, [#614](https://github.com/JeffMcClintock/TideSynth/pull/614). STEP 1 was empty: no open `platform:win` issue.

### The finding, and it is bigger than the four PRs

**`BACKLOG.md` and `JOURNAL.md` DID NOT CONFLICT. On any of the four. The sole conflicting path was `docs/lessons.md`, which is GENERATED.**

Measured before touching anything, with `git merge-tree --write-tree --name-only origin/<branch> origin/main` on each of the four — one command per branch, no worktree needed:

```
--- tide/win/E80-clap-editor-arm (behind main by 2) ---
Auto-merging BACKLOG.md
Auto-merging JOURNAL.md
Auto-merging docs/lessons.md
CONFLICT (content): Merge conflict in docs/lessons.md
```

Identical on E82, E19 and A38. **Five consecutive win cells have recorded this livelock as a three-file problem** — the standing recipe names `BACKLOG.md` by ownership, `JOURNAL.md` by set arithmetic, `docs/lessons.md` regenerated — and today two thirds of that recipe had nothing to do. The two hand-written files merged themselves.

**Why that changes the diagnosis.** A38's `PROPOSED:` entry ([#597](https://github.com/JeffMcClintock/TideSynth/pull/597)) offers four options and recommends (d)-with-(b): blank-line sections in `BACKLOG.md` **plus per-run journal files**, the second being the expensive half that needs new files and three script changes. A38's own probe found `JOURNAL.md` conflicting in three of six layouts, so the recommendation is sound *for the corpus it measured*. **Today's corpus does not conflict there at all**, because `main` advanced only two commits and the two journal insertions were separated by an unchanged entry — which is exactly the mechanism A38's own `ctl-context` control isolated (*"`JOURNAL.md` drops out once one unchanged entry sits between the two insertions"*). So the journal half is **conditional on run density**, not structural, and the file that conflicts unconditionally is the generated one.

**The mechanism, stated plainly, because it is self-inflicted and cheap to fix:** `docs/lessons.md` is derived from `JOURNAL.md` by `scripts/extract-lessons.py`, it is TRACKED, and A30's convention tells every run to regenerate it. So every branch regenerates it from its own journal, every branch's copy differs from `main`'s **in the same region**, and every PR therefore conflicts there — permanently, by construction, on a file nobody reads for merge purposes. **CI does not check it.** I grepped all seven workflows: `extract-lessons` appears in none of them, so `--check` is convention only and no gate depends on the file being current.

### How I resolved it, and why not by regenerating

**I took `origin/main`'s `docs/lessons.md` verbatim on all four branches.** Regenerating on each branch — the standing recipe — produces four *different* copies, so `main` merging any one of them re-conflicts the other three. That is the livelock, and it is the merge side of it that five cells have not escaped. Taking `main`'s copy makes the merged tree's `lessons.md` **byte-identical to `main`'s**, so each branch now introduces no diff there at all, and **merging one does not re-conflict the rest.**

Nothing is lost by it. The file is 100% derived from `JOURNAL.md`, which merged cleanly and completely on every branch, so one `extract-lessons.py --write` after the four land restores every line. **That regeneration is owed, and it is in the `win` NEXT cell.**

Verified on each of the four, per the standing recipe:

| branch | journal headings branch ∪ main → merged | missing | extra | `lessons.md` == `main`'s |
|---|---|---|---|---|
| E80 | 38 ∪ 31 = 39 → 39 | 0 | 0 | yes |
| E82 | 36 ∪ 31 = 37 → 37 | 0 | 0 | yes |
| E19 | 36 ∪ 31 = 37 → 37 | 0 | 0 | yes |
| A38 | 33 ∪ 31 = 34 → 34 | 0 | 0 | yes |

Identity compared by `git hash-object` against `git rev-parse origin/main:docs/lessons.md`, not by eye. **All four went `CONFLICTING`/`DIRTY` → `MERGEABLE`.** Lint on all four, 24 invocations, every one exit 0: `check-links`, `check-journal-prepend`, `check-backlog-diff`, `check-prompt-provenance`, `check-next-block`, `check-id-refs`. `check-commit-completeness --record`/`--verify` around each commit (`--verify` skips merge commits). `check-commit-authorship --range origin/<branch>..HEAD` clean on all four — **and note it needs `--range` explicitly in a detached worktree**, where it exits non-zero with *"detached HEAD -- pass --range explicitly"* and that is not a finding about the commits.

### STEP 2 — A39, and the first win cell since 09-18 that is not STEP 1.5 alone

A39 was `TODO`, `any`, unclaimed (`ls-remote` and `gh pr list` both empty on it), filed by this lane on 09-24. The open `PROPOSED:` entries — backlog `Plat` corrections, what to do about a check judged wrong, and A38's own shape — none change what A39 builds, so it was eligible. Claimed with a pushed DOING mark before any work, per STEP 2.

**The `mac` cell names A39 as its next item.** It stated intent but never claimed: no branch, no PR, and STEP 2's test for "taken" is a remote branch or open PR from another platform. Pushing the DOING mark first is the sanctioned way to settle that, and I did it before writing a line of the fix.

**What it changes.** `EXPECTED_PREFABS` is gone. `scripts/check-rack-populated.py` now counts `RackModules/` — the directory CMake stages wholesale into the bundle — with the same recursive walk and extension test `seedPrefabsFromBundle()` uses, so the two cannot drift. A missing or empty `RackModules/` **raises** rather than returning 0, because a zero expectation is vacuous in the one direction that matters.

**The risk this change carries, and the control for it.** Deriving an expectation is safe only when it comes from the build's **input** and never from its **output** — a gate that reads the count off its own subject cannot fail at all, and **looks identical to one that works**. That is A39's own warning, and it is why the Accept made a negative control mandatory. `tests/a39_prefab_count_probe.py` (16 arms, rc=0, no build, no network) carries an explicit **vacuity control**: one tree, two captured logs claiming different counts, requiring different verdicts.

**The Accept, run through the shipped CLI rather than a fixture:**

| clause | run | result |
|---|---|---|
| (a) prefab ADDED, no script edit | real file into `RackModules/`, log says 8 | `expecting 8 prefab(s) (derived from RackModules/)` · **exit 0** |
| (a) prefab DELETED, no script edit | `Logger.synthedit` moved aside, log says 6 | expects 6 · **exit 0** |
| (b) **negative control** | 8 staged, 7 seeded | **exit 1** — *"7 rack prefab(s) seeded, but RackModules/ holds 8"* |
| (b) other direction (E40's stale bundle) | 6 staged, 7 seeded | **exit 1**, naming both numbers |
| (c) probe | `python3 tests/a39_prefab_count_probe.py` | **rc=0**, 16 arms |

**A/B against `origin/main`'s script on all four shipped fixtures — verdicts identical** (rc=1 with 11 / 2 / 3 / 2 failures, before and after), so nothing was disarmed on the way in. `RackModules/` restored to its exact seven files afterwards.

**Learned:**

- **Measure which files actually conflict before applying a conflict recipe.** `git merge-tree --write-tree --name-only origin/<branch> origin/main` answers it in one command, with no worktree and no checkout, and today it cut the work by two thirds and changed the diagnosis. Five cells inherited "BACKLOG + JOURNAL + lessons" as a description of the problem and re-applied all three.
- **A tracked GENERATED file conflicts on every branch forever, and regenerating it on the branch is what sustains that.** The resolution that breaks the cycle is to take the default branch's copy, so the branch introduces no diff there and merging one branch cannot re-conflict the others. It is only safe because the file is derived from something that *did* merge — check that first.
- **A38's journal half is conditional on run density, not structural.** Two insertions with one unchanged entry between them do not conflict; A38's own `ctl-context` control says so. A quiet week measures differently from a busy one, so `JOURNAL.md`'s absence from today's conflicts is not evidence against A38 — but the generated file's presence in *every* conflict, for five cells running, is evidence about where the cost actually is.
- **`check-commit-authorship.py` needs `--range` in a detached worktree.** It exits non-zero with *"detached HEAD -- pass --range explicitly"*, which reads like an authorship failure and is not one. `--range origin/<branch>..HEAD`.
- **GitHub stamps a squash merge with the bot ACCOUNT's profile identity, not the run's `GIT_*` variables.** `a351fbf40` on `main` is authored `Tide Funkster <mcclintock.jeff+bot@gmail.com>`, not `tide-rack-bot <314850083+...>`. Expected, not a misconfigured box — but `check-commit-authorship` reports it on every branch that merges `main`, and it is worth recognising rather than re-investigating.

**Not verified:** **no TIDE build was run on this box.** The rack gate's CI step is `macos`-only, so A39's live `--standalone` arm is unexercised by me — the derivation is verified against captured and synthesised logs and against the real `RackModules/` tree, and the `macos` leg on [#614](https://github.com/JeffMcClintock/TideSynth/pull/614) is what exercises it end to end. Whether `main` builds on windows is likewise unknown to me; I compiled nothing. The four merged branches' CI was still running when this was written. `tests/rack-content/README.md`'s claim that `m5-empty-rack.log` yields *"12 failures"* is off by one (11, before and after) — pre-existing drift, left alone.

**Machine state.** `C:\SE\TideSynth` started and ended on `main`, clean, and **was never checked out to a branch** — every edit happened in `git worktree`s under the session scratchpad, all removed at the end. No other repo touched. Nothing built, no host launched, no screen taken, no GUI driven. **The developer was at the machine throughout** (Chrome, Outlook and GitHub Desktop open; no IDE or SynthEdit running), which is why the shared checkout was left alone entirely.

**Next:** see the `win` NEXT cell. For Jeff: **[#614](https://github.com/JeffMcClintock/TideSynth/pull/614) (A39) is the one with code in it**; #586/#587/#590/#597 are unblocked and waiting. **One `python3 scripts/extract-lessons.py --write` is owed** once they land.

**Branch/PR:** `tide/win/2026-09-25-step15-and-A39` holds this entry and the refreshed `win` NEXT cell. A39 is `tide/win/A39-prefab-count-derived` / [#614](https://github.com/JeffMcClintock/TideSynth/pull/614). The four merges are on their own existing branches.

## 2026-09-25 — macos — STEP 1 was two inherited gate failures on this lane's own PRs, fixed by the STEP 1.5 merge; #604 superseded (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.7032.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1 and STEP 1.5, which turned out to be the same fix. Took no backlog item, because STEP 1 was not empty.

### What changed since the 09-24 cell

- **`main` is green and #599 is closed.** Jeff fixed the break the other way: `53a23a0` shipped `AR`, `Keyboard`, `Logger` and `Sine` and moved `EXPECTED_PREFABS` to **7**. The S41 auto-closer closed [#599](https://github.com/JeffMcClintock/TideSynth/issues/599) at 04:29 UTC on 09-24, on a green `macos` at that commit. #600–#603 closed the same day.
- **So [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) is superseded.** It sets the constant to **3**, and `RackModules/` on `origin/main` has 7 `.synthedit` files. The gate compares with `!=`, so merging #604 would turn `main` red again. The windows cell showed this with a `check()` transcript, and I agree with it. I retitled #604 to start *"SUPERSEDED, do not merge"* and added a comment. **I did not close it.** Closing is Jeff's call, and the PR is `DIRTY`, so it cannot land by accident.

### STEP 1: two open `platform:mac` issues, both on this lane's branches

[#609](https://github.com/JeffMcClintock/TideSynth/issues/609) (`tide/mac/A36-journal-rotation-rule`) and [#610](https://github.com/JeffMcClintock/TideSynth/issues/610) (`tide/mac/E72-cable-dsp-dirty`). Both are from `github-actions`, filed 09-23 14:12 and 14:16 UTC. I read the logs of runs 35871916880 and 35871921549 instead of trusting the titles. Both runs failed in the rack-content gate, not in compilation:

```
FAIL 3 rack prefab(s) seeded, expected 5. Either a prefab failed to stage, or one was added and this script's EXPECTED_PREFABS was not updated.
1 assertion(s) failed -- the rack did NOT come up populated.
```

These are the windows cell's "red check that was the base's break", on this lane this time. The push that went red was the 09-23 cell's own merge of `origin/main` at `479d90a`, which was then the broken base. **The fix was merging the current `origin/main`.** STEP 1.5 needed that merge anyway, because both PRs had gone `DIRTY` again after #612 and Jeff's four 09-24 commits.

### The merges

I did both in `git worktree`s under the scratchpad.

- **E72:** `BACKLOG.md`, `JOURNAL.md` and `SynthEditSem/TideApp.cpp` auto-merged. The `TideApp.cpp` merge combines E72's change with Jeff's `7738abf`, so the compile legs below are what check it. Only `docs/lessons.md` conflicted, and I regenerated it: 1,497 lessons from 351 entries, `--check` exit 0.
- **A36:** the usual shape. HEAD's Rotation header block (lines 12–97) was on one side, and `origin/main`'s two new 09-24 entries were on the other. I kept the header, then the entries, and deleted the three markers. `docs/lessons.md` regenerated to 1,498 lessons.
- **Nothing lost (three-way heading sets, including archives):** 453 in the resolved set, 30 on `origin/main`, 451 in `ORIG_HEAD`. **Zero** missing against either side, and **zero** duplicates.
- **Lint on both, all exit 0:** `check-next-block`, `check-id-refs`, `check-backlog-archived`, `check-links`, and also `check-journal-prepend`, `check-backlog-diff` and `check-prompt-provenance`. The last three take **file paths** for base and head, not refs. Passing `origin/<branch>` gives a `FileNotFoundError`, so I passed `git show origin/main:<file>` written to a temp file.
- `check-commit-completeness --record`/`--verify` ran around both commits (`--verify` skips merge commits). `check-commit-authorship` was clean: 12 commits each, all `tide-rack-bot`. `ls-remote --get-url origin` returned `https://` before each push. I pushed to the existing branches and opened no new PRs.

### CI verification

The macOS CI leg rebuilt both pushed merge commits, and both passed. Runs **36010607518** (A36) and **36010606467** (E72) show `macos: success`, and the job log contains the gate's own lines:

```
  ok   7 rack prefab(s) seeded
rack is populated.
```

The `linux`, `guard` and three `render-*` legs are also `success` on both. **The S41 auto-closer then closed #609 and #610** on those green runs, so no issue was closed by hand. Before that, both issues had gone from `FAIL 3 ... expected 5` to `ok 7`, and the only new commit on each branch was the merge.

### STEP 2: A39 is eligible, but not taken

**A39** was filed by windows on 09-24. It is `TODO`, `any` and unclaimed (`ls-remote` shows no `A39` branch). It has a stated Accept, including a mandatory negative control, and its scope is one file this lane may edit. That makes it the first eligible item a mac cell has found since 09-17. I did not take it. STEP 1 says a run with platform issues fixes those *"instead of taking a backlog item, then go to STEP 4"*, and I read that literally, as the 09-23 cell did. The other TODO rows (`A35`, `A38`, `S8`, `E19`, `X2`, `E2`, `E72`, `E76`, `E79`, `E80`, `E81`, `E82`, `E84`) have not changed since 09-24, and I did not re-examine them.

**Learned:**

- **One merge can clear both a CONFLICTING state and a red check, and when it does, STEP 1 and STEP 1.5 are one piece of work.** Before reading any compiler output on an issue that names this lane's own branch, check whether the failing push merged a base that was red at the time: `gh run list --branch main --workflow build.yml`. Here it had.
- **The fleet's own fix can go stale.** #604 was a correct and verified fix, and one Jeff commit made it a regression. A PR that fixes `main` stays valid only as long as `main` has not been fixed some other way. Its owning lane should re-check that before the PR sits another day as "merge this first".
- **`check-journal-prepend.py`, `check-backlog-diff.py` and `check-prompt-provenance.py` take files, not git refs.** CI passes paths. Locally, write `git show origin/main:<file>` to a temp file first.

**Not verified:** the `windows` compile leg on both pushes was still `in_progress` when this was written. This merge touched no Windows-specific code, but E72's merged `TideApp.cpp` has not been seen compiling on Windows yet. I built nothing locally. The macOS verification is CI's, from the self-hosted `macos` leg, not from a build I ran myself. The standing STEP 2 rows were not re-walked.

**Machine state.** `~/Documents/GitHub/TideSynth` was left on `main`, clean. It never left `main`, because all work was done in worktrees under the scratchpad, which I removed. `SynthEdit` (`master`) and `SynthEditLib` (`main`) are clean and untouched. The screen was locked (`CGSSessionScreenIsLocked` present). I launched no host and built nothing locally.

**Next:** see the `mac` NEXT cell. (1) Check `mergeStateStatus` on #585/#588/#589. (2) **Take A39.** For Jeff: merge #585, then #588/#589, and close #604 unmerged.

**Branch/PR:** `tide/mac/2026-09-25-step1-inherited-gate` holds this entry and the refreshed `mac` NEXT cell. The merges are on `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty`.

## 2026-09-24 — windows — STEP 1.5 again, but the four red `macos` checks were `main`'s break and not the branches': all four now CLEAN and green (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.7032.0** · as **tide-rack-bot** (both paths — REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, which is the number in `GIT_AUTHOR_EMAIL`; transport assertion printed `git@github.com:`)

**Did:** STEP 1, then STEP 1.5, which was the whole run — the fifth consecutive `win` cell for which that is true, exactly as the 09-18 and 09-23 cells predicted. **All four of this lane's PRs are now `CLEAN` / `MERGEABLE` with 13 SUCCESS + 2 SKIPPED and zero failures**, which is the first time this lane has had all four simultaneously green and unconflicted.

### STEP 1: the feed is structurally empty, so `main`'s own build is the real check

`gh issue list --label platform:win --state open` → empty, and that still verifies nothing: `build.yml:523` excludes `matrix.platform != 'win'` from filing platform issues, so this lane's STEP 1 feed cannot fill. Read `main`'s latest `build` instead — **green on all seven jobs at `7f9716996`** (run [35960104114](https://github.com/JeffMcClintock/TideSynth/actions/runs/35960104114)): `guard`, `render-windows`, `render-linux`, `render-macos`, `linux`, `windows`, `macos`.

The five open `platform:mac` issues ([#600](https://github.com/JeffMcClintock/TideSynth/issues/600)–[#603](https://github.com/JeffMcClintock/TideSynth/issues/603), [#609](https://github.com/JeffMcClintock/TideSynth/issues/609), [#610](https://github.com/JeffMcClintock/TideSynth/issues/610)) are `github-actions`-authored and four of them name **this lane's** branches. They are not mine to fix — STEP 3's "do not fix build failures for a platform you cannot compile on" — but they were the entry point to the finding below, and #600–#603 are answered by it.

### STEP 1.5: four CONFLICTING PRs, and a red check that was not theirs

All four `tide/win/**` PRs were `CONFLICTING` **and** carried a red `macos` check: [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) (E80), [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) (E82), [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) (E19), [#597](https://github.com/JeffMcClintock/TideSynth/pull/597) (A38). No reviews, no unresolved comments.

The conflicts were A38's shape exactly, for the sixth-plus time: **`BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md` on every one of the four, and never a line of code.** Resolved by the standing recipe, in worktrees:

- **`JOURNAL.md` by set arithmetic.** One hunk per branch, `main`'s 09-24 macos entry against the branch's 09-23 windows entry, both kept, newest first. Verified rather than eyeballed — on #597: branch 31 entries, `main` 29, result **32**, with `comm` showing **0 missing from the union and 0 extra**.
- **`docs/lessons.md` regenerated, never merged** — `extract-lessons.py --write` then `--check` (exit 0) on each. The counts differ per branch (1499/1523/1507/1508 lessons from 351/355/353/353 entries) because each branch carries its own entries, which is the tell that the regeneration is reading the merged journal and not a stale copy.
- **`BACKLOG.md` by ownership** — the `win` row from the branch (09-23, its own lane and the newer of the two), the `mac` row from `main` (09-24, likewise). One hunk, two rows, on all four.

**One trap avoided by measuring instead of grepping:** `JOURNAL.md` contains literal conflict-start markers in prose — three occurrences, from earlier entries discussing merges — so a naive marker count says "3 left" on a fully resolved file. Anchor on line-start markers (`grep '^<<<<<<<'`), and let the set arithmetic be the actual proof.

Lint green on all four before committing: `check-id-refs`, `check-links`, `check-next-block`, `extract-lessons --check`, `check-journal-prepend`, `check-backlog-diff`, `check-prompt-provenance` — all exit 0. `check-commit-completeness --record`/`--verify` around each commit (`--verify` correctly skips on a merge commit), and `check-commit-authorship --repo .` clean on all four (10/29/22/23 commits, all `tide-rack-bot`).

### The finding: a stale branch's red check can be the BASE's break, and costs one command to tell apart

**The red `macos` check on all four was not a compile break and was not caused by these branches.** The log shows the build reaching `[100%] Built target TIDE_Rack_AU3_assemble`, signing the bundle, and *then* failing the rack-content gate:

```
FAIL 3 rack prefab(s) seeded, expected 5.
1 assertion(s) failed -- the rack did NOT come up populated.
```

That is `main`'s break, already fixed on `main`, and the four branches inherited it by being based before the fix. The mechanism, read out of the tree rather than guessed — `EXPECTED_PREFABS` against the actual `RackModules/*.synthedit` count at each revision:

| rev | date | shipped | `EXPECTED_PREFABS` | gate |
|---|---|---|---|---|
| `ccda7ad97` "fix knobs not drawing" | 09-22 | 3 | 5 | **RED** |
| `479d90acf` | 09-22 | 3 | 5 | **RED** |
| `7738abf90` | 09-24 | 3 | 5 | **RED** |
| `53a23a03e` "Ship the AR, Keyboard, Logger and Sine rack prefabs" | 09-24 | **7** | **7** | green |
| `7f9716996` (current `main`) | 09-24 | 7 | 7 | green |

**So the fix for four red checks was the merge itself, with no code change from this run at all**, and that is the measurement: `macos` **FAILURE → SUCCESS on all four**, same branches, the only new commit on each being the merge of `origin/main`.

**The cheap discriminator, which this lane should reach for before ever treating a red check as its own defect:** ask whether `main` was red at the branch's merge-base and green now. `gh run list --branch main --workflow build.yml` answers it in one command — here it read `failure` at `ccda7ad97` (09-21) and `479d90acf` (09-22), `success` at `53a23a03e` and `7f9716996` (09-24). A red check on a branch that has sat for days is a claim about the branch **and** its base, and the fleet has been reading it as the former.

### Second finding: [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) is superseded, and merging it would re-break the gate the other way

The `mac` NEXT cell on `main` says *"merge **#604** first, because it is the one thing standing between `main` and green."* That was true when written and is not now. #604 changes one file, `scripts/check-rack-populated.py`, setting `EXPECTED_PREFABS = 3`; Jeff fixed the same mismatch on `main` by **addition** instead (`53a23a03e` shipped the four missing prefabs and moved the constant to 7). The comparison is `count != expect_prefabs` at `scripts/check-rack-populated.py:255`, strict in both directions. Run against the gate's own `check()`, no build required:

```
main's EXPECTED_PREFABS = 7
expect_prefabs=7 (main as it stands) -> PASS
expect_prefabs=3 (#604's value)      -> FAIL
   FAIL: 7 rack prefab(s) seeded, expected 3.
```

Posted as a [comment on #604](https://github.com/JeffMcClintock/TideSynth/pull/604#issuecomment-5812386072) with that transcript. **Not merged, not closed, not modified** — it is another platform's PR, and it is `DIRTY` against `main` anyway, so nothing lands without someone looking. The durable part of #604 is its *diagnosis*, which is correct and which its added comment states well; only its resolution direction is stale.

**Filed A39** for the underlying defect: the gate couples a hand-maintained constant to the contents of a directory, so any commit that changes `RackModules/` without it turns `main` red. **Third occurrence** — `14a8fd376` and `322df0f27` (both 08-26), now `ccda7ad97` (09-22) — and `995ebfab2`'s own subject line is *"main is red: EXPECTED_PREFABS was 7, RackModules/ holds 5"*. `main` already contains the same class of fix next door, `7738abf90` *"TIDE: read every staged pin XML, not a hand-copied list of them"*, landed the same day for the same reason, which is the shape A39 should copy. Grepped `origin/main`'s `BACKLOG.md` for `check-rack-populated.py` and `EXPECTED_PREFABS` before filing, per STEP 3: the only hit is the `mac` NEXT cell, not a row, so A39 is not a duplicate of the macOS box's work — their work is #604, which is the instance, not the class.

**Learned:**

- **A red CI check on a branch that has sat for days is a claim about the branch AND its base, and the fleet has been reading only the first half.** Four `platform:mac` issues (#600–#603) were filed against this lane's branches for a break that was `main`'s, and no run had to fix anything: the merge that STEP 1.5 was going to do anyway cleared all four. One command tells the cases apart — `gh run list --branch main --workflow build.yml` — and it is worth running before reading the compiler output, let alone before filing.
- **Read where in the job a red check died, not just that it died.** This one reached `[100%] Built target` and signed the bundle before failing a *content gate*, which makes "Build failure on macos" the wrong description of every one of #600–#603. Nothing compiles differently on the four branches; the issue title says otherwise and cost the mac lane a real investigation.
- **A gate whose expectation is a hand-maintained constant will go stale at exactly the rate the thing it counts changes.** Three occurrences in a month for `EXPECTED_PREFABS`, each turning `main` red for a day or more, each fixed by editing the constant — which is the fix that guarantees a fourth. The constant's own comment predicts its staleness ("if you delete a prefab, this number moves with it"), and a comment that accurately predicts the defect is evidence the design is wrong, not that the warning is sufficient.
- **Two correct fixes for one break, applied in opposite directions on two branches, is worse than one.** #604 lowered the expectation, `main` raised the supply; both were right when made, and the second silently turned the first into a regression. When a fleet branch fixes a break on `main`, its PR is only valid while `main` has not fixed it differently — so check the base's own history before merging a days-old build fix, not only its mergeability.
- **`JOURNAL.md` has literal conflict markers in its prose**, so a bare `grep -c` for them on a resolved file is not zero and that is not a bug. Anchor at line start and verify the resolution by set arithmetic over `^## 2026` headings; it caught nothing wrong here, which is the point of running it.
- **The `mac` NEXT cell is 53,474 bytes on one line** — A37's curve continuing: 41,216 on 09-18, so **+30% in six days**. `BACKLOG.md` is 306,755 bytes and `JOURNAL.md` 302,984. A38's measurement stands and its PR is now green and clean.

**Not verified:**

- **Nothing was built on this box, and no host was launched.** The developer was at the machine all run (below), so the `macos` → SUCCESS result is CI's, not a local reproduction, and the A39 mechanism is read out of `git ls-tree` counts plus the gate's own `check()` rather than from a standalone binary. The `check()` run is a genuine execution of the shipped comparison against a synthetic transcript of the shape the gate parses — it proves the direction of the inequality, not that any particular binary seeds seven prefabs.
- **A39 is filed, not started.** No attempt was made to derive the count from the staged prefabs.
- **`JOURNAL.md` NOT rotated — ninth consecutive cell to defer it.** 302,984 bytes / 29 entries against A24's 60 KB ceiling. Five PRs from this lane are now open and a rotation rewrites the bottom of the file, so it would conflict with all five at once; [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36), which restores the rotation instruction currently ABSENT from `main`, is still open and should land first.
- **#584 (linux) and the three `tide/mac/**` PRs were left alone.** STEP 1.5 is scoped to this platform's branches. #604 got a comment and nothing else.

**Machine state.** `C:\SE\TideSynth` left on `main`, and **it never left it** — all five branches were handled in `git worktree`s under the scratchpad, which are removed. **The developer was at the machine for the whole run**: three Visual Studio instances (`TiDEModules — TiDEPanelGui.cpp`, `TideSynth — SeAudioMaster.cpp`, `SynthEditStore — ResizeAdorner.cpp`) plus Slack, per `Get-Process | Where-Object { $_.MainWindowTitle }`. No build, no host, no screenshot, no screen taken — and a text-merge run is the right thing to take on a busy box, which is the third consecutive `win` cell to say so.

**Two files in that checkout are dirty and were left strictly alone**: `SynthEditSem/TideApp.cpp` (+6 lines) and `SynthEditSem/TideApp.h` (+1), both `mtime` 2026-09-24 17:43, i.e. **during this run**. They are STEP 5's third kind — the developer's work in progress — and **not** CRLF churn: `git diff --ignore-all-space` returns 17 and 12 lines respectively, so there is real content there. Not committed, not reverted, not stashed. They are plausibly the live edit behind the `TideSynth` VS window.

**Next:**

1. **Merge the four `tide/win/**` PRs — they are all `CLEAN`, `MERGEABLE` and 13/13 green right now**, and that state is perishable: A38 measures that any merge into `main` re-conflicts the rest, so merging them as a batch is worth more than merging one and coming back.
2. **Do NOT merge [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) on the strength of the `mac` cell's instruction.** See the second finding; its comment carries the transcript.
3. **[#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) before any `JOURNAL.md` rotation**, then rotate — nine cells have now deferred it.
4. **A39** if this lane is otherwise empty, and it very likely is: the queue was already empty on 09-18 and nothing has entered it since.

**Branch/PR:** [#612](https://github.com/JeffMcClintock/TideSynth/pull/612) on `tide/win/2026-09-24-step15-conflicts` holds this entry, the refreshed `win` NEXT cell and the A39 row. The four merges are on their own branches and pushed — `e15c5fe73` (#597), `7d5beb6cd` (#586), `a901ba42e` (#587), `cd7d5c65f` (#590).

## 2026-09-24 — macos — STEP 1.5 for the eighth time: A36 and E72 re-conflicted after #606 and Jeff's two knob commits; #604 still awaiting merge (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.7032.0** (`CFBundleShortVersionString` of `/Applications/Claude.app`; earlier cells recorded 2.2553.1, so either the app updated or the earlier figure came from somewhere else) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1, then STEP 1.5, per the 09-23 cell's three-item "Next" list.

### STEP 1: nothing new to fix

The same five `platform:mac` issues are open: [#599](https://github.com/JeffMcClintock/TideSynth/issues/599) (`main`) and [#600](https://github.com/JeffMcClintock/TideSynth/issues/600)–[#603](https://github.com/JeffMcClintock/TideSynth/issues/603) (windows' branches). The fix is still [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) (`EXPECTED_PREFABS` 5 -> 3). It is **still open**, `CLEAN`/`MERGEABLE`, and every check passes (`gh pr checks 604`: `macos`, `linux`, `windows`, three `render-*` legs, `lint`, `e57-delete-key`, `guard` all `pass`). `origin/main` has not moved in code since the 09-23 cell: its tip is `0a8a87c` (#606, that cell's bookkeeping), and `git show origin/main:scripts/check-rack-populated.py` still says `EXPECTED_PREFABS = 5`, so `main` is still broken and still has exactly one fix waiting. **#599 stays open.** It can only be closed once #604 is on `main` and `main` has been rebuilt. Nothing was rebuilt this run, because nothing would have changed since yesterday's reproduction.

### STEP 1.5: the resolution

`mergeStateStatus` read via `gh pr list --json`: [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) and [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72) were `DIRTY`/`CONFLICTING` again. [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) and #604 were `CLEAN`. Both merges were done in `git worktree`s under the session scratchpad.

- **E72:** `BACKLOG.md` and `JOURNAL.md` auto-merged. Only `docs/lessons.md` conflicted, and it was regenerated with `extract-lessons.py --write` (1,489 lessons from 349 entries).
- **A36:** `BACKLOG.md` auto-merged. `JOURNAL.md` conflicted in the usual shape: HEAD had the Rotation header block, and `origin/main` had the new 09-23 and 09-22 entries. Resolved by keeping HEAD's block and then `origin/main`'s entries. `docs/lessons.md` was regenerated.
- **Nothing lost, by the three-way heading-set comparison from the 09-22 entry.** Resolved branch plus archives: 451 unique headings. `origin/main`: 28. Pre-merge `ORIG_HEAD` plus archives: 449. Headings missing from the resolved set: 0 against either side. Duplicates across `JOURNAL.md` and its archives: 0.
- `check-next-block.py`, `check-id-refs.py`, `check-backlog-archived.py` and `check-links.py` are clean on both branches. Both merges also pulled in Jeff's `ccda7ad`/`479d90a` (the two prefab deletions and the TiDEknob sources). That is expected, since those commits are on `main`.
- `check-commit-completeness.py --record`/`--verify` bracketed both commits. `--verify` skips merge commits by design. `check-commit-authorship.py` is clean on both, and both were authored `tide-rack-bot` on the first attempt. `ls-remote --get-url origin` returned `https://` before each push.
- Pushed to the existing branches. No new PRs. Read three times after the pushes: both are `MERGEABLE`. `UNSTABLE` only means the compile legs are still `pending`. Nothing is `fail`.

### STEP 2: walked, nothing eligible

The rows are the same set as 09-22: `A35`, `A38`, `S8`, `E19`, `X2`, `E2`, `E72`, `E76`, `E79`, `E80`, `E81`, `E82`, `E84`. Each one is ineligible for the same reason already on record. The screen was locked (`CGSSessionScreenIsLocked` present). `main`'s only code change since 09-17 is Jeff's knob fix, and that makes nothing newly eligible. E83 is still `IN-REVIEW` on `main` with #581 merged. Its flip is already on the E72 branch, per the 09-17 cell, so it was not repeated here.

**Learned:**

- **Once STEP 1's fix is in review, STEP 1 has no work left. It is still worth re-checking**, because the fix could merge overnight and turn "leave #599 open" into "rebuild `main` and close it". One `gh pr view 604 --json state` settles which case applies.
- **Every mac bookkeeping PR re-conflicts #585 and #588 when it merges**, and this entry's PR will do the same. That is A38's mechanism, and nothing a run does on its own lane avoids it. The only exit is merging #585/#588 or ruling on A38 / [#597](https://github.com/JeffMcClintock/TideSynth/pull/597).

**Not verified:** the compile legs on the re-pushed #585/#588 were still `pending` when this was written. `main` was not rebuilt, because it is unchanged since the 09-23 reproduction.

**Machine state.** `~/Documents/GitHub/TideSynth` left on `main`, clean. The merges were done in worktrees under the scratchpad, which were removed afterwards. No other repo was touched, no host was launched, and nothing was built.

**Next:** merge **#604** first, because it is the one thing standing between `main` and green. Then **#585** (A36), then #588/#589. The next mac run should check whether #604 has merged. If it has, build `TIDE_Rack_STANDALONE` from `main`, run `check-rack-populated.py`, and close #599 on a pass. After that, check `mergeStateStatus` on this platform's PRs as usual.

**Branch/PR:** `tide/mac/2026-09-24-step15-conflicts` holds this entry and the refreshed `mac` NEXT cell. The merges themselves are on `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty`.

## 2026-09-23 — windows — a whole-fleet merge sweep lands exactly TWO of nine open PRs, and one of the two has to be #604 (scheduled run)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.7032.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on all four of this lane's PRs, `CONFLICTING`/`DIRTY` again for the third consecutive cell. Resolved, verified lossless, pushed; all four are `MERGEABLE`. Then measured the thing A38 has never had a number for — **not "can this lane's four PRs be swept" but "can the FLEET's nine be"** — and the answer is small enough to act on. Took no backlog item: the queue is empty for this lane for the ninth consecutive cell, for the reasons already on record.

### STEP 1 — `main` IS STILL RED, 30 HOURS ON, AND THE REASON IS NOT THAT NOBODY NOTICED

`gh issue list --label platform:win` is empty, which as seven prior cells have said **verifies nothing**: `build.yml:523` (`matrix.platform != 'win'`) excludes this platform from filing platform issues at all. Reading `main`'s latest `build` run instead, as those cells instruct:

| run | sha | when | `windows` | `macos` | `linux` |
|---|---|---|---|---|---|
| [35688124166](https://github.com/JeffMcClintock/TideSynth/actions/runs/35688124166) | `479d90acf` | 09-22 04:45 | **success** | **failure** | success |

Same run the 09-22 cell read, because `main` has had no code commit since. So **`main` is broken and it is not broken on this platform** — STEP 1 does not fire here and STEP 3's "do not fix build failures for a platform you cannot compile on" governs the rest.

**What is new is WHY it is still broken.** The macOS box diagnosed it correctly on 09-22: `ccda7ad` deleted two prefabs without moving `EXPECTED_PREFABS`, so [`scripts/check-rack-populated.py`](scripts/check-rack-populated.py) fails `3 rack prefab(s) seeded, expected 5`. That run split its work the way STEP 4 asks — the **fix** on [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) (`tide/mac/issue-599`), its **journal and NEXT cell** on [#606](https://github.com/JeffMcClintock/TideSynth/pull/606) — and then:

| PR | opened | content | outcome |
|---|---|---|---|
| [#606](https://github.com/JeffMcClintock/TideSynth/pull/606) | 09-22 14:09 | `JOURNAL.md`, `BACKLOG.md`, `docs/lessons.md` | **auto-merged 14:14, five minutes later**, by `github-actions` |
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) | 09-22 14:09 | one constant in `scripts/check-rack-populated.py` | **still open**, `MERGEABLE`/`CLEAN`, 15/15 green including `macos` |

**So the REPORT of the break merged itself in five minutes and the FIX has waited twenty hours.** This is A4 working exactly as designed and not a defect in it — [`scripts/automerge_eligible.py`](scripts/automerge_eligible.py) is a strict inclusion list and `scripts/**` is deliberately absent, so a code change fails closed and waits for a human. That rule is right. **The consequence nobody has stated is that the fleet's automation is systematically faster at recording problems than at fixing them**, and on a day when the fix is one integer that is the difference between a green `main` and a red one across every open PR in the repo. Every one of the four `tide/win/**` PRs below is red on `macos` for this reason and no other; so is `main`.

Not filed as a row, because the next section says the same thing with a bigger number and A38 already owns the shape of it.

### STEP 1.5 — all four `CONFLICTING` again, resolved again, eighth-plus occurrence

All four read `mergeable: CONFLICTING` / `mergeStateStatus: DIRTY`, every check green except `macos`, **zero reviews and zero comments** — again the combination STEP 1.5's literal list of three does not name. `mergeStateStatus` is still one extra field on a `gh pr view` this step already makes you run.

`main` moved by **exactly one commit** since the last resolution — `0a8a87c` — and it is macOS's bookkeeping PR #606, touching `BACKLOG.md`, `JOURNAL.md` and `docs/lessons.md` and nothing else. **One docs-only merge re-conflicted four PRs.** That is A38's claim demonstrated again at the smallest possible scale.

| PR | row | conflicted in | resolved | ours | theirs | union | lost | invented |
|---|---|---|---|---|---|---|---|---|
| [#597](https://github.com/JeffMcClintock/TideSynth/pull/597) | A38 | `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md` | 30 | 29 | 28 | 30 | 0 | 0 |
| [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) | E19 | same three | 33 | 32 | 28 | 33 | 0 | 0 |
| [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) | E82 | same three | 33 | 32 | 28 | 33 | 0 | 0 |
| [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) | E80 | same three | 35 | 34 | 28 | 35 | 0 | 0 |

Counts are `JOURNAL.md` **heading sets**, not line counts — a heading dropped from one side and one duplicated from the other cancel in a count and cannot cancel in a set comparison. Resolution per the standing recipe: `BACKLOG.md` by **ownership** (win cell from the branch, mac cell from `main` — 09-23 beats the branches' 09-22, and each resolved cell was checked byte-identical to the side it came from, not eyeballed); `JOURNAL.md` by **date, newest first**; `docs/lessons.md` **regenerated** with `extract-lessons.py --write`, never hand-merged. All seven lint checks `rc=0` on every branch, `check-commit-authorship.py` clean on all four, every commit `tide-rack-bot` on the first attempt. All four now `MERGEABLE`.

**ONE TRAP WORTH THE NEXT RUN'S TIME, BECAUSE IT SILENTLY MIS-SLICES THE FILE.** `JOURNAL.md` contains the strings `<<<<<<<`, `=======` and `>>>>>>>` **inside its own prose** — three occurrences, in entries describing previous resolutions, one of which is itself a warning about this. A resolver that matches conflict markers **unanchored** will both cut the file in the wrong place and then report "markers remain" on a correct resolution. Anchor every marker pattern to a line start (`re.M` plus `^`). The 2026-09-19 entry that warns "a resolver script that pattern-matches the marker text…" was right, and this run walked into the adjacent version of it anyway.

### THE FINDING — A FULL FLEET SWEEP LANDS **2 OF 9**, AND EVERY WINNING PAIR CONTAINS #604

The 09-22 cell measured the sweep over **this lane's four** branches and got depth 1 of 4. Jeff does not sweep one lane. There are **nine** open PRs today across three lanes plus the build fix, and nobody has ever measured that, because 9! = 362,880 orderings is not something you enumerate.

`tests/a38_fleet_sweep_probe.py` (new, this run, on #597's branch) searches instead. It uses `git merge-tree --write-tree` to merge **in memory** and `git commit-tree` to chain the result, so a sweep costs object writes and no checkout at all — which is why it can run against a repo the developer is working in. Depth-first over subsets, memoised on the set of branches already merged: 2^9 states, not 9!. Control (`--control`, every branch replaced by its own merge base, which must sweep fully) **9/9, rc=0**.

The memo can understate, so the headline was then re-derived by **brute force with no memo at all** — every ordered prefix of length ≤3, 9x8x7 paths:

| | count | which |
|---|---|---|
| PRs that merge into `main` **on their own** | **6 of 9** | #586, #587, #589, #590, #597, #604 — *not* #584, #585, #588, which are `CONFLICTING` on `main` right now |
| ordered **pairs** that merge | **10** | **every single one contains #604** |
| ordered **triples** that merge | **0** | — |

**MAXIMUM SWEEP DEPTH, EXHAUSTIVELY: 2.** Not "2 in the best case found" — 2 with no triple existing at all.

**And the cause is one column of a table.** Counting each open PR's changed files against `origin/main`, split into bookkeeping (`BACKLOG.md`, `JOURNAL.md`, their archives, `docs/lessons.md`, `docs/decisions.md`) and everything else:

| PR | lane | bookkeeping files | other files |
|---|---|---|---|
| [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) | mac | **0** | 1 |
| [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) | mac | 3 | 1 |
| [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) | linux | 3 | 2 |
| [#597](https://github.com/JeffMcClintock/TideSynth/pull/597) | win | 4 | 2 |
| [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) | win | 4 | 3 |
| [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) | win | 4 | 5 |
| [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) | win | 4 | 10 |
| [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) | mac | 5 | 2 |
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) | mac | 6 | 1 |

**#604 is the only open PR in the repository that touches no bookkeeping file, and it is the only PR that can be merged alongside any other one.** The correlation is 9 for 9. Everything else carries three to six bookkeeping files, so every other pair collides in them — and the probe confirms it path by path: **every conflict it found in the entire search was in `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md` or `docs/decisions.md`**, with exactly one exception, `docs/ci/headless-gui-verification.md` on #584, which is also a document.

**So the queue is not slow because Jeff is slow. A sweep of nine PRs, however long he spends on it, lands two.** Sixteen re-resolutions across six runs and eight bookkeeping merges since 09-07 have not moved that number, because they cannot: resolving a branch against `main` says nothing about whether it agrees with the other eight branches, and it is the branch-vs-branch disagreement that caps the sweep.

**`docs/decisions.md` is now a fourth hot file**, which the 09-22 cell's list of three does not have. It is on five of the nine PRs and it is the one file A4 deliberately refuses to auto-merge, because merging it *is* the decision. So it will keep arriving in pairs that need a human and it will keep conflicting while it waits.

**THE ONE ACTION WORTH MORE THAN ANOTHER SWEEP: MERGE [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) FIRST, AND ALONE.** It is green on all fifteen checks, `MERGEABLE`/`CLEAN`, one integer, and it unbreaks `main` — which is why every other open PR is red on `macos`. It is also, by the table above, free: it is the only merge that costs no other PR its mergeability. Then merge **one** other, and expect the remaining seven to need a mechanical resolution each, one run apiece, exactly as they have all month.

### Verification, with the exit codes actually returned

Per branch, all `rc=0`: `check-links` · `check-journal-prepend` · `check-backlog-diff` · `check-prompt-provenance` · `check-id-refs` · `check-next-block` · `check-backlog-archived`. The three diff-based ones take **file paths, not git refs**, and their `--changed-file` list must include files changed in the **working tree**, not only `origin/main...HEAD` — pass the committed list alone and an archived row reads as silently dropped. That is how this run's E83 move first came back red, and the error message names the right problem for the wrong reason.

`check-commit-completeness.py --record`/`--verify` around every commit; `--verify` reports "HEAD is a merge commit -- skipping", which is correct and is not a pass. `check-commit-authorship.py --repo .` on all four branches, every unpushed commit `tide-rack-bot`. `check-no-direct-commits.py` not run: no GATED repo was touched.

`tests/a38_fleet_sweep_probe.py` **rc=0**, control **rc=0 at 9/9**; the brute-force cross-check is a throwaway and its result is the table above. No network beyond `gh pr list`, no build, ~2 minutes.

**Learned:**

1. **A sweep's depth is a property of branch-vs-branch agreement, and resolving every branch against `main` cannot raise it.** Six runs and sixteen re-resolutions took this lane from depth 0 to depth 1 and stopped there; the fleet-wide number is 2 of 9 and it is capped by branches disagreeing with each OTHER, which no amount of careful merging against `main` touches. Measure the sweep, not the individual PR, before concluding a resolution run achieved anything.
2. **The PR that merges alongside others is the one that touches no bookkeeping file, and that is one command per PR to check.** `git diff --name-only $(git merge-base origin/$br origin/main) origin/$br` split against `BACKLOG.md` / `JOURNAL.md` / `docs/lessons.md` / `docs/decisions.md` predicted all nine outcomes correctly. It is a cheaper way to tell Jeff what to merge than resolving anything.
3. **`git merge-tree --write-tree` plus `git commit-tree` replays a whole merge sweep with no checkout, no worktree and no working-tree mutation.** That is what makes measuring a sweep affordable on a machine the developer is using, and it is faster than checkout-based merging by enough to turn an enumeration into a search.
4. **A subset-memoised search reports a FLOOR, so brute-force the small depths before publishing the headline.** The memo keeps the first tree that reached a subset and two orders reaching it can differ; here the exhaustive depth-3 replay (9x8x7) confirmed max depth 2 and additionally revealed the 6-of-9 and every-pair-contains-#604 facts the memoised run had not surfaced.
5. **`JOURNAL.md` quotes conflict-marker strings inside its own prose, so every marker regex must be line-anchored.** An unanchored pattern slices the file at a sentence and then reports "markers remain" on a correct resolution — a false failure on top of a real corruption, which is the worst pairing.
6. **`check-backlog-diff.py`'s `--changed-file` list must include WORKING-TREE changes, not just `origin/main...HEAD`.** Give it the committed list alone while the archive edit is still unstaged and a correctly archived row reads as "MISSING from head... silently dropped". The check is right; the invocation was wrong.
7. **An auto-merge tier that allowlists bookkeeping and fails closed on code makes the fleet structurally faster at recording a problem than at fixing it.** Both PRs opened at 14:09; the report merged itself at 14:14 and the one-integer fix is still open twenty hours later, with `main` red the whole time. The rule is still the right rule — the point is that the asymmetry is a predictable output of it and worth watching for.

**Machine state.** `C:\SE\TideSynth` started and ended on `main` and **was never checked out to any branch** — all four resolutions were done in `git worktree`s under the session scratchpad, all removed at the end. No other repo was touched, nothing was built, no host was launched, no screen was taken.

**The developer WAS at the machine and WAS in a tree** — `Get-Process | Where-Object { $_.MainWindowTitle }` showed Visual Studio open on `SimulatorGmpi - ptc_mind.cpp`, plus Settings. That is a *different* reading from 09-22, where the same command showed only Chrome and the run felt free to work four worktrees at once. It is a different reading from 09-09 too: the tree VS is open on is **not** a TIDE tree, so the 09-09 hazard (a source file changing underneath a build, twice) does not apply — but a text-merge run that never builds and never takes the screen is the right shape of work either way, and that is what this one was.

The same three untracked files sit in `RackModules/` (`AR.synthedit`, `Logger.synthedit`, `Sine.synthedit`) that the 09-22 entry recorded. They **predate this run**, they are the developer's, and per STEP 5's third category they were not committed, reverted or stashed — only recorded here. Worth one line to the next run, though: `ccda7ad` deleted `AR_jef.synthedit` and `Sine_jef.synthedit` and these three are sitting there unstaged, so `EXPECTED_PREFABS` may well want to move again after Jeff finishes. #604 sets it to 3 and is right for `main` as it stands today.

### Bookkeeping done, and where it went

**E83 flipped `IN-REVIEW` -> `DONE` and moved to `BACKLOG-DONE.md` verbatim**, dated 2026-09-08. Its only linked PR, [#581](https://github.com/JeffMcClintock/TideSynth/pull/581), merged on 2026-09-08 and its commit `e5c58879b` is an ancestor of `main` — both checked, not assumed. It had sat `IN-REVIEW` for fifteen days and roughly ten runs, which is what STEP 4's "flip it as part of your STEP 4" exists to stop.

**No fifth PR, for the second cell running.** This entry and the re-pointed `win` NEXT cell are committed **byte-identically onto all four existing branches**, so they are a no-op in any merge between them. The 09-22 cell established the tactic and its one constraint — *identical prose is safe, identical relative links are only safe for files already on `main`* — which is why `tests/a38_fleet_sweep_probe.py` is named as inline code above and not linked: it exists only on #597's branch.

### What I did NOT do, deliberately

- **Did not duplicate #604's fix onto these four branches.** It is another platform's work with an open PR, STEP 2's collision rule says leave it, and a second copy of a one-line change is a guaranteed conflict the day the first one lands. The four `macos` legs stay red until it merges and that is the correct state, not a thing to paper over.
- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (linux) or [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) / [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) / [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (mac)** — STEP 1.5 is scoped to `tide/{PLATFORM}/**`. Three of them (#584, #585, #588) do not even merge into `main` on their own today; each is one mechanical resolution from mergeable and none needs a Windows toolchain.
- **Did not comment on [#599](https://github.com/JeffMcClintock/TideSynth/issues/599)-[#603](https://github.com/JeffMcClintock/TideSynth/issues/603).** They are `platform:mac`, the macOS run already commented the diagnosis and the fix link on all five, and a second agent agreeing adds noise rather than information.
- **Did not act on A38's proposal, and did not implement the `docs/lessons.md` half either.** Dropping a generated file from branch commits is still the cheapest single reduction available — two consecutive windows cells have now identified it — but A38's shape is Jeff's ruling and #597's body says merging it is the decision.
- **Did not rotate `JOURNAL.md`** — eighth consecutive cell to defer it, still blocked on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) merging, which restores the rotation instruction that is **absent from `main`**. The measurement above is the strongest argument yet for not rotating: a rotation rewrites the bottom of the file and would take the sweep from 2 to 1.
- **Did not take a backlog item.** Every `TODO` row is ineligible on the record and none of the reasons has changed: `A35` parked on its own two `PROPOSED:` entries, `A38`/`E19`/`E80`/`E82` are this platform's own open PRs, `E72`/`E81` are mac's, `E76`/`E79`/`X2` are linux in substance, `S8` is `NEEDS-SPEC`, `E2` is an umbrella its own row calls not takeable, and **`E84` is a `.github/workflows/**` edit the bot's token deliberately cannot make.**

## 2026-09-23 — macos — STEP 1 was not empty: `main` itself was broken, fixed and verified (scheduled run)

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1, checked before anything else. `gh issue list --label platform:mac --state open` across all five repos was **not empty for the first time since 09-07** — five open issues in `TideSynth`: [#599](https://github.com/JeffMcClintock/TideSynth/issues/599) (branch `main`) and [#600](https://github.com/JeffMcClintock/TideSynth/issues/600)–[#603](https://github.com/JeffMcClintock/TideSynth/issues/603) (windows' four `tide/win/**` branches: A38, E19, E82, E80). All authored by `app/github-actions`, which the rules treat as an authorised author. All five carried the identical failure line, confirmed by reading each run's log rather than assuming from the title:

```
FAIL 3 rack prefab(s) seeded, expected 5. Either a prefab failed to stage,
or one was added and this script's EXPECTED_PREFABS was not updated.
```

**#599 (`main`) is the one that matters most** — every other platform's branches build on top of it, so this was not a windows-branch-local problem.

### Diagnosis

`git log` on `main` showed two commits landed after yesterday's mac cell (#598): `ccda7ad` ("fix knobs not drawing") and `479d90a` ("fix knob range"), both Jeff, interactive (`jef@synthedit.com`), pushed directly — not a scheduled run's work, and correctly not mine to second-guess. `ccda7ad`'s diff deletes `RackModules/AR_jef.synthedit` and `RackModules/Sine_jef.synthedit` (526 lines) alongside the knob fix. `scripts/check-rack-populated.py`'s `EXPECTED_PREFABS` constant is documented, in its own multi-paragraph comment, as "not a policy, it is a MIRROR of `RackModules/`" that "went stale within hours" the last two times a prefab was deleted (`995ebfa`, `322df0f`) — this is the third occurrence of exactly that, and the comment's own prediction held.

**Reproduced before touching anything:** built `TIDE_Rack_STANDALONE` locally (Release/arm64, `SE_LOCAL_BUILD=OFF`, pre-existing incremental build tree, ccache warm) against `origin/main` as pulled, then ran `scripts/check-rack-populated.py --standalone build/SynthEditSem/TIDE-Rack.app/Contents/MacOS/TIDE-Rack`:

```
ok   default rack loaded, 25110 byte document
FAIL 3 rack prefab(s) seeded, expected 5. ...
1 assertion(s) failed -- the rack did NOT come up populated.
```

Matches the issue's own captured text exactly.

### The fix

One-line change, `EXPECTED_PREFABS = 5` → `3`, with a comment entry added in the same style as the existing history (mirrors the current `RackModules/` contents: `Envelope.synthedit`, `Filter.synthedit`, `Oscillator.synthedit`). Branch `tide/mac/issue-599` off freshly-fetched `origin/main`. `check-commit-completeness.py --record`/`--verify` around the commit, no discrepancy. `check-commit-authorship.py --repo .` clean, authored `tide-rack-bot` on the first attempt (`GH_TOKEN`/`GIT_AUTHOR_*`/`GIT_COMMITTER_*` re-exported immediately before the commit, per the 09-19 lesson). Transport spot-check (`git ls-remote --get-url origin`) answered `https://...` both before the branch push and before the fix push.

**Re-verified after the fix, same binary rebuilt, same script:**

```
ok   3 rack prefab(s) seeded
ok   default rack loaded, 25110 byte document

rack is populated.
```

Exit 0. Pushed to `tide/mac/issue-599`, opened [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) against `main`, with the before/after transcript in the PR body per STEP 4's verification-artifact requirement.

### Issue bookkeeping

Commented on all five issues (#599–#603) naming the root cause and linking #604. **Left #599 OPEN** — the fix is verified on my own branch, not on `main` itself, and I cannot merge my own PR (STEP 5); the issue's own close instructions say "verify the fix by building on that platform," which for a `main`-branch issue means after #604 lands there, not before. **Did not touch #600–#603's branches** — they are windows' `tide/win/**` branches, out of scope the same way STEP 1.5 scopes PR-conflict work to one's own platform; commented that they share #599's root cause and should clear once whoever owns them merges or rebases past #604.

### What this run deliberately did NOT do

- **STEP 1.5 was not run.** STEP 1's own wording is "fix that instead of taking a backlog item, then go to STEP 4" — read literally, that skips STEP 1.5 and STEP 2 entirely for a STEP-1 run. This platform's three standing PRs ([#585](https://github.com/JeffMcClintock/TideSynth/pull/585) A36, [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) E72, [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) E81) were **not re-checked for `mergeStateStatus` this run**. Given `main` moved twice more since the 09-22 cell resolved them, they may well have re-conflicted a seventh/eighth time on the established shape — **the next mac run should check this first**, same as every cell since 09-19.
- **STEP 2's backlog walk was not repeated** — no eligible item was taken; the queue's standing state (`A35`, `A38`, `S8`, `E19`, `X2`, `E2`, `E72`, `E76`, `E79`, `E80`, `E81`, `E82`, `E84` all ineligible for the reasons every prior cell records) is unchanged as far as this run observed, but it was not re-walked, so treat that as unverified rather than confirmed for 09-23.
- **`scripts/check-prefab-modules.py` was run out of curiosity while diagnosing, not fixed, not filed as a row.** It is not wired into any CI job (`grep` of `.github/workflows/*.yml` confirms) and exits 0 regardless, so it gated nothing here — but it reports 5 module types the 3 shipped prefabs use that TIDE does not register in the compiled-in set (`IO Mod`, `Multiply`, `1 Pole LP`). Pre-existing, unrelated to this break. Worth a look next time someone touches the prefab set or wires this script into `lint.yml` (that wiring is itself a workflow edit the bot's token cannot make — same constraint E84 already names).

**Learned:**

- **STEP 1 finding real work is rare enough on this platform (first time since 09-07) that it is worth stating plainly: the fix protocol worked as documented** — reproduce locally before touching anything, minimal fix, own branch, own PR, verification artifact in both the PR and the journal, issue left open until `main` itself is verified.
- **A constant documented as "a MIRROR of `RackModules/`, and it goes stale" is not a hypothetical warning — it has now gone stale three times** (`995ebfa`, `322df0f`, and this one). If a fourth prefab deletion lands without this constant moving with it, the fix is the same three-line diff every time; worth considering whether the count should be derived from `ls RackModules/*.synthedit | wc -l` at build time instead of hand-maintained, though that is a product/process decision beyond a STEP 1 item's scope, not something this run should decide unilaterally.
- **An interactive commit from Jeff can break a scheduled-run platform's build same as any other commit** — STEP 1 does not distinguish by author, and neither should the run reading it. Nothing here should be read as a complaint about the commit itself (`AR_jef`/`Sine_jef` look like superseded personal scratch prefabs given the naming and the unaffected panel/knob fix alongside them); the gap was purely the missing constant update, which is exactly what the gate exists to catch.

**Not verified:** `#604`'s CI checks — still running/queued when this entry was written. The four windows branches (`#600`–`#603`) were not rebuilt; the shared-signature claim rests on log comparison, not a rebuild on my box of those specific branches.

**Machine state.** `TideSynth` ended on `main`, clean, fast-forwarded to `479d90a` (`origin/main` at run start). No other repo touched. Screen was locked throughout (`CGSSessionScreenIsLocked` present, `ioreg -n Root -d1 -a`); no GUI needed — this was a build-and-script fix, not a hosted-plugin question.

**Next:** merging **#604** is the highest-value single action — it clears `main`'s own break and, once the four `tide/win/**` branches merge or rebase past it, should close #600–#603 too without further action from any platform. The next mac run should (1) verify #604 merged and close #599 by building `main` directly if so, (2) re-check `mergeStateStatus` on #585/#588/#589 before anything else, per every cell since 09-19, and (3) only then resume STEP 2's backlog walk, which this run did not repeat.

**Branch/PR:** `tide/mac/issue-599` — [#604](https://github.com/JeffMcClintock/TideSynth/pull/604). This entry and the refreshed `mac` NEXT cell are on a separate branch (`tide/mac/2026-09-23-issue-599-journal`), per the established split between a fix's own branch and the bookkeeping branch.

## 2026-09-22 — windows — the fleet's bookkeeping PRs are winning the merge race 8/8 while its product PRs lose it 8/8, and that is A38 seen from the merge side (scheduled run)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on all four of this lane's PRs, which were all `CONFLICTING`/`DIRTY` again. Resolved and pushed all four. **Then measured why this keeps happening and got a sharper answer than A38's row has** — see below. Took no backlog item: the queue is empty for this lane for the eighth consecutive cell, for the same reasons already on record.

### STEP 1 — `main` IS RED, AND FOR THE FIRST TIME THE "READ THE BUILD RUN INSTEAD" ADVICE ACTUALLY EARNED ITS KEEP

`gh issue list --label platform:win` is empty, which — as six prior cells have said — **verifies nothing**: `build.yml:523` (`matrix.platform != 'win'`) excludes this platform from filing platform issues at all. Every one of those cells added "read `main`'s latest `build` run instead" as a precaution. **Today is the first time that precaution changed the answer**, and it changed it in both directions at once:

| run | sha | when | `windows` | `macos` | `linux` |
|---|---|---|---|---|---|
| [35688124166](https://github.com/JeffMcClintock/TideSynth/actions/runs/35688124166) | `479d90acf` "fix knob range" | 09-22 04:45 | **success** | **failure** | success |
| [35669846013](https://github.com/JeffMcClintock/TideSynth/actions/runs/35669846013) | `ccda7ad97` "fix knobs not drawing" | 09-21 23:57 | **success** | **failure** | success |

So **`main` is broken, and it is not broken on this platform** — `windows` and `render-windows` are green in both. STEP 1 therefore does not fire here, and STEP 3's "do not fix build failures for a platform you cannot compile on" governs the rest. The macOS break is **already filed** as [#599](https://github.com/JeffMcClintock/TideSynth/issues/599) (`platform:mac`, author `github-actions`, opened 09-22), so there was nothing for this run to file either; the mac box owns it.

Two things worth keeping from this:

- **The empty-issue-feed trap is real and this run walked up to it.** An agent that had trusted `gh issue list --label platform:win` would have recorded "no breaks" on a day `main` was red. The feed was empty *and* `main` was red *and* this platform was fine — three facts the feed cannot distinguish between.
- **Both breaking commits are Jeff's own, pushed straight to `main`** (`ccda7ad97`, `479d90acf`), which is his bypass working as designed and is noted only because it dates the break precisely. The last all-green run is [34438892984](https://github.com/JeffMcClintock/TideSynth/actions/runs/34438892984) at `13095a395` (09-10); everything between is docs-only with no `build` run, which is `guard` working.

### STEP 1.5 — all four resolved, and `mergeStateStatus` was again the only field that showed it

All four `tide/win/**` PRs read **`mergeable: CONFLICTING` / `mergeStateStatus: DIRTY`, every check green, no reviews, no comments** — the combination that STEP 1.5's literal list of three (failing checks, requested changes, unresolved review comments) does not name, and which therefore reads as "waiting on Jeff" when it is not. **This is at least the seventh occurrence across the fleet.** `mergeStateStatus` is still one extra field on a `gh pr view` that STEP 1.5 already makes you run.

| PR | row | age at run start | conflicted in |
|---|---|---|---|
| [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) | E80 | 12 days | `BACKLOG.md`, `JOURNAL.md` |
| [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) | E82 | 8 days | `BACKLOG.md`, `JOURNAL.md` |
| [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) | E19 | 6 days | `BACKLOG.md`, `JOURNAL.md` |
| [#597](https://github.com/JeffMcClintock/TideSynth/pull/597) | A38 | 0 days (opened 09-21) | `BACKLOG.md`, `JOURNAL.md` |

**Every conflict in all four was in those two files and never in code** — `git merge-tree --write-tree --name-only` against `origin/main`, run before touching anything, which is a read-only way to get the conflict set of all four branches without checking any of them out. `docs/lessons.md` did not conflict this cycle.

Resolution per the standing recipe: `BACKLOG.md` by **ownership** (newest `win` cell from the branch, newest `mac` cell from `main` — 09-22 beat the branches' 09-21), `JOURNAL.md` by **set arithmetic**, entries ordered newest-first. **Verified lossless on every branch by full three-way heading comparison, not spot-checking:**

| branch | resolved | HEAD | main | union | lost from HEAD | lost from main | invented |
|---|---|---|---|---|---|---|---|
| A38 (#597) | 28 | 27 | 27 | 28 | 0 | 0 | 0 |
| E19 (#590) | 31 | 30 | 27 | 31 | 0 | 0 | 0 |
| E82 (#587) | 31 | 30 | 27 | 31 | 0 | 0 | 0 |
| E80 (#586) | 33 | 32 | 27 | 33 | 0 | 0 | 0 |

### THE FINDING: A38'S LIVELOCK HAS A WINNER AND A LOSER, AND IT IS NOT THE PLATFORMS — IT IS BOOKKEEPING vs PRODUCT

A38's row counts the livelock's cost as *re-resolutions* (16 across five runs, zero product change). That is the branch side. **Counted from the merge side, over every PR opened since 09-01, the picture is much sharper.** A PR was classed `book` if its title says STEP 1.5 / "journal + NEXT cell" / "CONFLICTING" / "merge-sweep", `prod` otherwise:

| lane | kind | merged | avg days open | still open |
|---|---|---|---|---|
| win | **book** | **3** | **0.0** | 0 |
| win | prod | 4 | 1.2 | **4** — #586 12d, #587 8d, #590 6d, #597 0d |
| mac | **book** | **5** | **0.0** | 0 |
| mac | prod | 8 | 0.9 | **3** — #585 12d, #588 7d, #589 6d |
| linux | prod | 0 | — | **1** — #584 13d |

**Eight bookkeeping PRs opened since 09-07. All eight merged, every one of them the same day it opened.** **Eight product PRs have been opened since 09-08. Not one has merged.** The cutover is a date, not a gradient: the last product PR to merge was [#582](https://github.com/JeffMcClintock/TideSynth/pull/582) on 09-08, and **`main` has had no fleet product change in the 14 days since** — the same "six commits, three files, no code" fact #597 measured, now with the reason attached.

**The mechanism, and it is selection rather than coincidence.** A PR merges if and only if it is `MERGEABLE` at the moment a human looks at it.

- A **bookkeeping** PR is cut fresh from `main`, lives a few hours, and is mergeable for essentially its whole life. It always wins.
- A **product** PR has to survive days of waiting, and *every bookkeeping merge re-conflicts it*. It is `CONFLICTING` for most of its life, so it is almost never mergeable at the moment anyone looks.

So the fleet's bookkeeping is not merely *costing* runs — **it is out-competing the fleet's own product work for the merge channel, and it is generated by the very step that exists to record that the product work happened.** Each run's STEP 4 PR is one more merge event, and each merge event pushes every open product PR back to `CONFLICTING`.

**This run's remedy needed no rule change and is the reason there is no fifth PR today.** The journal entry you are reading and the re-pointed `win` NEXT cell were committed **byte-identically onto all four existing branches** instead of onto a new bookkeeping branch of their own. The 09-21 windows run had already half-discovered this — its 09-18/09-19/09-21 entries sit identically on #586, #587 and #590 — and the property that makes it work is worth naming: **content that is byte-identical on both sides of a merge is not a conflict, it is a no-op.** So whichever of the four merges first carries this cell to `main`, and the other three then agree with `main` about it exactly.

**What that buys, stated as a number rather than a hope:** this run adds **zero** new merge events to the queue, where every previous cell in this chain added one.

**THE TACTIC HAS EXACTLY ONE CONSTRAINT, AND `check-links` FOUND IT RATHER THAN A LATER RUN.** Shared content must not link to a file that exists on only some of the branches carrying it. The canonical `win` cell carries the 09-21 A38 generation, which linked to `tests/a38_bookkeeping_merge_probe.py` — a file that lives **only on [#597](https://github.com/JeffMcClintock/TideSynth/pull/597)'s branch**. Byte-identical text plus per-branch link checking meant `check-links` passed on A38's branch and failed `BACKLOG.md:11 (no such file)` on the other three. Fixed by de-linking that one path to inline code in the carried generation; the path is still readable and the link returns for free once any of the four merges and the file reaches `main`. **The general rule for anyone reusing this: identical prose is safe, identical *relative links* are only safe for files already on `main`.**

### THE SWEEP IS NOT CLEAN, AND MEASURING IT IS HOW I FOUND THAT OUT — THE TACTIC BUYS EXACTLY ONE MERGE

**This section originally claimed the sweep was clean in all 24 orderings. That was wrong, and the probe written to demonstrate it is what caught it.** The result is better than the claim would have been, because it puts a number on the ceiling the whole STEP 1.5 ritual is working under.

`tests/a38_sweep_probe.py` (new, this run, on #597's branch) clones the repo into a throwaway and, for **all 24 orderings** of the four branches, replays what a human sweep actually does — merge each PR into `main` in turn — and records **how many merge cleanly before it stalls**. Depth, not a clean/dirty bit, because "0 orderings clean" hides the difference between stalling on the first PR and stalling on the last, and those are opposite states of the queue.

| | depth 0 | depth 1 | depth 2 | depth 3 | full sweep |
|---|---|---|---|---|---|
| **control** — branches as they were at run start | **24/24** | 0 | 0 | 0 | **0** |
| **treatment** — after this run's resolution | 0 | **24/24** | 0 | 0 | **0** |

**So the resolution moved every ordering from depth 0 to depth 1, and no further.** Before it, not even the first PR could merge; after it, exactly one can, whichever one is picked — and then the sweep stalls again. `python3 tests/a38_sweep_probe.py` **rc=0**, `--control` **rc=0**, ~30 s, no network after the clone, no build.

**That is the ceiling on this entire ritual, stated as a number for the first time: a windows run that resolves every PR it owns delivers ONE merge, not four.** Six prior cells have done this work and none of them could have known that, because nobody replayed the sweep. It explains the 8-vs-8 count above exactly — a sweep of N product PRs yields one merge and re-conflicts N−1, which is why the queue has grown monotonically since 09-08 while every bookkeeping PR sailed through.

**What blocks depth 2 is branch-vs-branch divergence, not branch-vs-`main`** — the four files are `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md` and `docs/decisions.md`. Resolving against `main`, however carefully, cannot touch it: each branch still carries its own dated journal entries and its own row edits, and those collide with each other the moment the second one lands.

**One of those four is free to remove, and nobody has connected it to A38 before.** **`docs/lessons.md` is GENERATED** — A30 says regenerate it, never hand-merge it — and yet every branch commits its own regenerated copy, so **any two branches conflict in it by construction, forever.** Dropping it from branch commits and generating it on `main` after a merge (or in CI) removes one of the four blockers outright, costs nothing, and needs no ruling: it is not a process change, it is declining to commit a build artifact. **Filed below as a suggestion on A38's row rather than done here, because A38's shape is Jeff's call and this run must not pre-empt it.**

**What the byte-identical tactic did and did not do, now that it is measured.** It did what it claimed for the two regions it covers — the `win` cell and this entry are byte-identical on all four branches, so they are a no-op in any merge between them and they are not among the depth-2 blockers. It did **not** make the sweep clean, and this entry no longer says it does. It remains worth doing for the reason that survives the measurement: **it adds no fifth PR and no ninth merge event**, which is a cost avoided rather than a cure.

### Verification, with the exit codes actually returned

Run on **each of the four branches** after resolution, all **rc=0**: `check-next-block` · `check-journal-prepend` · `check-backlog-diff` · `check-id-refs` · `check-links` · `check-backlog-archived` · `check-prompt-provenance`. The three diff-based ones take **file paths, not git refs** — invoke them the way `lint.yml:53/61/66` does, with `origin/main`'s copy extracted to a file, or they exit 2 on their own usage message and it is easy to mistake that for a pass.

`tests/a38_sweep_probe.py` **rc=0** and `--control` **rc=0** — and note the probe is written so that **rc=0 does not mean the sweep is clean**; it means every ordering merged at least one PR, which is the bar the treatment actually clears. The full-sweep count is reported separately and is **0/24**. A probe whose green light hid that would have been worse than no probe.

`check-commit-completeness.py --record`/`--verify` around every commit. `check-commit-authorship.py` on all four branches, every commit `tide-rack-bot`. `docs/lessons.md` regenerated with `extract-lessons.py --write` (A30) where it moved, never hand-edited. `check-no-direct-commits.py` not run: no GATED repo was touched.

### Machine state

`C:\SE\TideSynth` started and ended on `main`, and **was never checked out to any branch** — all four resolutions were done in `git worktree`s under the session scratchpad, removed at the end. No other repo was touched, nothing was built, no host was launched, no screen was taken.

**The developer was at the machine but not in the tree** — `Get-Process | Where-Object { $_.MainWindowTitle }` showed Chrome and nothing else: no Visual Studio, no SynthEdit, no build. That is a *different* reading from the 09-09 and 09-18 cells, where VS was open on files that changed underneath the run, and it is why this run was willing to touch four worktrees at once. **The command is still worth running even when you expect it to be boring**; it is what distinguishes "safe to work" from "assume it is safe to work".

Three untracked files sit in `RackModules/` (`AR.synthedit`, `Logger.synthedit`, `Sine.synthedit`). They **predate this run**, they are the developer's, and per STEP 5's third category they were not committed, reverted or stashed — only recorded here.

### What I did NOT do, deliberately

- **Did not open a bookkeeping PR.** That is the whole point of the finding above; opening one would have been the ninth same-day merge and the ninth re-conflict of everything else.
- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (linux), [#585](https://github.com/JeffMcClintock/TideSynth/pull/585), [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) or [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (mac)** — STEP 1.5 is scoped to `tide/{PLATFORM}/**`. All four are one mechanical resolution from mergeable and none of them needs a Windows toolchain; the sweep probe deliberately does not include them for the same scoping reason.
- **Did not act on A38's proposal.** Its `PROPOSED:` entry is Jeff's ruling, and #597's own body says merging it is the decision. This run's byte-identical-content tactic is a *resolution technique available today*, not an implementation of any of A38's four options, and it does not pre-empt the ruling.
- **Did not rotate `JOURNAL.md`** — still blocked on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) merging, which restores the rotation instruction that is **absent from `main`**. This is the seventh consecutive cell to defer it. A rotation rewrites the bottom of the file and would conflict with all eight open PRs at once.
- **Did not take a backlog item.** Every `TODO` row is ineligible on the record: `A35` parked on its own two `PROPOSED:` entries, `A38`/`E19`/`E80`/`E82` are this platform's own open PRs, `E72`/`E81` are mac's, `E76`/`E79`/`X2` are linux in substance, `S8` is `NEEDS-SPEC`, `E2` is an umbrella its own row calls not takeable, and **`E84` is a workflow edit the bot's token deliberately cannot make.**


## 2026-09-22 — macos — STEP 1.5 for the seventh time on the same shape: A36 and E72 re-conflicted again, and windows has now filed a PR proposing A38's own fix (scheduled run)

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** checked `mergeStateStatus` on this platform's three open PRs before touching anything, per STEP 1.5. [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) and [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72) had gone `mergeStateStatus: DIRTY` / `mergeable: CONFLICTING` again — the same two branches every mac cell since 09-19 has resolved, re-conflicted by `main` moving once more (`#596`, the 09-21 cell's own journal+NEXT-cell PR). [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) stayed `CLEAN`/`MERGEABLE` for a third cycle running.

### STEP 1 / STEP 1.5

`platform:mac` issues empty (checked `TideSynth`, `SynthEditLib`, `GMPI_Wrappers`, `gmpi_ui`, `SynthEdit_Rack_Adaptor`). Screen locked (`CGSSessionScreenIsLocked` present, `ioreg -n Root -d1 -a`). `mergeStateStatus` read via GraphQL (`gh pr list --json mergeable,mergeStateStatus`), not assumed from the 09-21 entry's final state.

### The resolution

Both branches merged `origin/main` in worktrees under the session scratchpad (no concurrent session detected on this box). **E72: `BACKLOG.md` and `JOURNAL.md` both auto-merged cleanly**; only `docs/lessons.md` conflicted (generated-content drift) and was regenerated via `extract-lessons.py --write`, never hand-merged. **A36: `BACKLOG.md` auto-merged cleanly; `JOURNAL.md` and `docs/lessons.md` both conflicted**, the same shape every prior A36 cycle documents — HEAD (A36's own branch) held the restored "Rotation" header block above the first dated entry, `origin/main` held the new 09-21 entry appended below where that header used to sit. Resolved by keeping HEAD's header block, then `origin/main`'s new entry, then the unconflicted remainder — one splice. `docs/lessons.md` regenerated the same way as E72's.

**Verified nothing was lost by full three-way set comparison, not spot-checking:** collected every `## 202...` heading across the resolved branch's `JOURNAL.md` + both its archives (`JOURNAL-2026-09.md`, `JOURNAL-2026-08.md` — 449 headings total) and diffed against both `origin/main`'s unrotated `JOURNAL.md` (26 headings) and the branch's own pre-merge set across its own file + archives (448 headings, `ORIG_HEAD`). Zero headings present on either side and missing from the resolved set, in both directions.

`check-next-block.py`, `check-id-refs.py` (E2-umbrella advisory only, standing and unrelated), `check-backlog-archived.py` and `check-links.py` all clean on both branches post-merge. `check-commit-completeness.py --record`/`--verify` around each commit, no discrepancy. Exported `GH_TOKEN`/`GIT_AUTHOR_*`/`GIT_COMMITTER_*` immediately before each `git commit`, per the 09-19 lesson; both commits landed authored as `tide-rack-bot` on the first attempt, `check-commit-authorship.py` clean on both, no `--reset-author` needed.

Pushed both to their existing branches (STEP 1.5: fix in place, no new PRs) — `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty`. Did not touch #589 (E81), already `MERGEABLE`/`CLEAN`. Re-checked `mergeStateStatus` after both pushes (8-second wait, then `gh pr list`): both `MERGEABLE` again, `UNSTABLE` only on still-queued/pending compile legs (`gh pr checks` on both: nothing `fail`, only `pending`/`pass`/`skipping`) — nothing red.

### What's new since 09-21: windows has filed a PR against A38 itself

**[#597](https://github.com/JeffMcClintock/TideSynth/pull/597) (`tide/win/A38-bookkeeping-livelock`)** is now open — windows measuring and proposing "the cheap half" of A38's own per-lane-file fix. Read only, not touched: it is a `tide/win/**` branch (out of STEP 1.5's scope for this platform) and A38's own row says the ruling is Jeff's, not a run's, so its content is not something this cell acts on either way. Noted here because it is the first sign the livelock itself may be addressed rather than merely worked around every cycle.

### What I did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584)** (linux's `tide/linux/E79-clap-headless-document`) — STEP 1.5 is scoped to `tide/{PLATFORM}/**`.
- **Did not touch [#597](https://github.com/JeffMcClintock/TideSynth/pull/597)** (windows' A38 proposal) for the same reason, and because acting on its content would be pre-empting Jeff's ruling.
- **Did not re-walk STEP 2's backlog in depth** — walked the file order quickly: the TODO set (`A35`, `A38`, `S8`, `E19`, `X2`, `E2`, `E72`, `E76`, `E79`, `E80`, `E81`, `E82`, `E84`) is unchanged from the 09-21 cell's own walk and every row is ineligible for the same reasons already on record (parked on rulings, this platform's or another's own open PR, GATED/NEEDS-SPEC, linux-in-substance, wants the unlocked screen, or a workflow edit the bot's token cannot make). Screen locked throughout, same as the nine prior mac cells (09-07 through 09-21) — nothing GUI-dependent was attempted.
- **Did not rotate `JOURNAL.md` further** — still blocked on #585 (A36) itself merging.

**Learned:**

- **The livelock is now confirmed on a fourth consecutive run boundary, same two branches, same recipe, same result.** Nothing about repeating this a seventh time changed the mechanics; the only new datum is that a fix is now proposed (#597) rather than only diagnosed.
- **A full three-way heading-set comparison (resolved vs. `origin/main` vs. pre-merge `ORIG_HEAD`, each including archives) is cheap — one `grep`/`sort`/`comm` pipeline — and is strictly stronger evidence than the `grep -c` count check prior cells used**, since a count match can hide a swap (one entry dropped, a different one duplicated). Worth using this shape going forward rather than the cheaper count-only check.

**Not verified:** the self-hosted `macos`/`windows` compile legs on both re-pushed PRs — still `pending` when this entry was written.

**Machine state.** All repos started and ended clean on their default branches except `TideSynth`. No host was launched, no plug-in built or installed. Screen was locked throughout. Work done in `git worktree`s under the session scratchpad; the main checkout (`~/Documents/GitHub/TideSynth`) was untouched beyond a `fetch` and was left on `main`/`origin/main` throughout.

**Next:** unchanged — merging **#585 (A36) first** unblocks `JOURNAL.md` rotation and is the highest-value single action available; every day it and #588 stay open costs another box another run repeating this recipe. **#597 is worth Jeff's attention specifically**, since it is the first PR that would change this recipe rather than just re-run it. The next run touching `BACKLOG.md`/`JOURNAL.md` should check `mergeStateStatus` per-PR, expect a possible `UNKNOWN` on the first read, and check whether #597 has merged before assuming this recipe is still the right one to reach for.

**Branch/PR:** `tide/mac/2026-09-22-step15-conflicts` — this entry and the refreshed `mac` NEXT cell only. The two fixes are on `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty` themselves (their own pushed merge commits).

## 2026-09-21 — windows — the conflict was provably about nothing: all four NEXT lanes hashed at the merge base, and every merge to `main` in eleven days is bookkeeping (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.2553.1** (Code tab) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 was the entire run for the **fifth consecutive windows run**. All three `tide/win/**` PRs were `CONFLICTING/DIRTY` again — [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) (E80), [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) (E82), [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) (E19) — resolved and pushed all three, all now `MERGEABLE`. No product code changed, and none was available to change. **What this run adds is not the resolution, which is now routine, but two measurements that turn A38 from a well-argued model into an arithmetic one.**

### Measurement 1 — the conflicting hunk contained zero disagreement, proved by hash

A38 argues the conflicts are caused by adjacency rather than disagreement. The 09-19 cell showed the `win` cell was byte-identical across the three branches; it did not show what `main` had done to it. **Hashing all four NEXT lanes at the merge base (`792330672`), at the branch head (`bcf81aaf5`) and at `origin/main` (`f4fc888a4`) settles it in one table:**

| lane | base sha256[:10] | branch | `origin/main` | bytes base/branch/main | changed on |
|---|---|---|---|---|---|
| `win`   | `73b73dd0fe` | `cefdc9ed01` | `73b73dd0fe` | 18,904 / 40,818 / 18,904 | **BRANCH only** |
| `mac`   | `8b2cc8c7f7` | `8b2cc8c7f7` | `32e990713e` | 43,261 / 43,261 / 47,058 | **MAIN only** |
| `linux` | `899d375ecd` | `899d375ecd` | `899d375ecd` | 6,331 / 6,331 / 6,331 | neither |
| `any`   | `297302d0f4` | `297302d0f4` | `297302d0f4` | 4,635 / 4,635 / 4,635 | neither |

**The two sides' edit sets are disjoint — not approximately, but by hash — and git conflicted anyway.** `origin/main`'s `win` cell is byte-identical to the merge base, so `main` expressed no opinion whatever about the row this lane changed, and the branch expressed none about the row `main` changed. The conflict is `win` on line 11 and `mac` on line 12 of a markdown table that cannot carry a blank line between rows. **This is arm 1 of `scripts/a38-merge-layout-experiment.py` reproduced in production with the disjointness asserted rather than assumed**, and it is the strongest form of A38's negative control: *there was nothing to resolve, and a human-scale resolution was required anyway.*

`JOURNAL.md` has the same shape and a simpler cause. **The first dated entry is line 11 in all three revisions — base, branch and `origin/main`.** "Newest at the top" gives every run in the fleet the identical insertion anchor, so two in-flight runs collide there by construction. The 09-18 cell's observation that `JOURNAL.md` later *auto-merged* is consistent and is not the file behaving well: by then those branches had absorbed an intervening entry, so their own insertion was no longer at the top and a shared line sat between the two sides.

### Measurement 2 — eleven days, and every single merge to `main` is bookkeeping

The 09-19 cell measured nine days and four merges. Two days later:

    git log --first-parent 13095a395..origin/main   ->  6 merges
    git diff  --stat       13095a395..origin/main   ->  3 files, 321 insertions, 4 deletions

Last product line on `main` is **`13095a395`, 2026-09-10**. The six merges since, listed with the files each one touched:

| PR | head branch | merged | files |
|---|---|---|---|
| [#591](https://github.com/JeffMcClintock/TideSynth/pull/591) | `tide/mac/2026-09-17-queue-blocked` | 09-16 | the three |
| [#592](https://github.com/JeffMcClintock/TideSynth/pull/592) | `tide/mac/2026-09-18-step15-conflict-fix` | 09-17 | the three |
| [#593](https://github.com/JeffMcClintock/TideSynth/pull/593) | `tide/win/2026-09-18-step15-conflicts` | 09-17 | the three |
| [#594](https://github.com/JeffMcClintock/TideSynth/pull/594) | `tide/mac/2026-09-19-step15-conflicts` | 09-18 | the three |
| [#595](https://github.com/JeffMcClintock/TideSynth/pull/595) | `tide/mac/2026-09-20-step15-conflicts` | 09-19 | the three |
| [#596](https://github.com/JeffMcClintock/TideSynth/pull/596) | `tide/mac/2026-09-21-step15-conflicts` | 09-20 | the three |

"The three" is `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md`, and for all six PRs it is the **complete** file list, read from `gh pr view --json files`, not inferred. **Six merges, six bookkeeping PRs, zero product files. Against that, the seven open PRs still carry the 23 non-bookkeeping files the 09-19 cell tabulated.**

### The two measurements compose, and the result is the actionable part

This run's merge base is `792330672`, which *is* #594. The only thing that changed on `main` between that and `f4fc888a4` is #595 and #596 — two bookkeeping PRs — and per Measurement 1 the only NEXT lane they touched is `mac`. **So this entire run was caused by two merges that carried no product change and edited no row this lane has any opinion about.** That is not a figure of speech; it is the whole causal chain, and every link in it is in the tables above.

**Which makes one mitigation available with no ruling at all: a lane that rides its bookkeeping on an already-open PR adds no merge event.** The 09-18 evening cell called this Change 2 and the win lane has followed it since — this run opens no new branch and no new PR either, so it contributes nothing to the next box's STEP 1.5. In the eleven-day window the win lane produced **one** merge event across four runs (#593, before it adopted the convention) and the mac lane produced **five** across five runs.

**Stated fairly, because the asymmetry is not a fault of that lane: Change 2 was written into a windows journal entry and a `win` NEXT cell, and there is no mechanism by which either reaches the macOS box.** A lane reads the shared prompt and its own cell. **A convention that lives in one lane's cell cannot propagate to another lane, and this is the first measured instance of that costing real runs.** The fix is one sentence in STEP 4, it is Jeff's to make because the prompt is shared, and — this is the point — **it is independent of A38's ruling and very much smaller.** Noted as an addendum on A38's row rather than filed as a new id, per STEP 3's grep rule: A38's Scope already names STEP 4 of the prompt, so a second id here is C15/C16 exactly.

Honest limit on the claim: riding bookkeeping on a product PR **defers** the merge event, it does not delete it. When #586/#587/#590 merge they will carry this bookkeeping and will re-conflict whatever else is open. The saving is that merge events then equal the number of *product* PRs instead of product PRs plus one per run — and in a window whose product-PR count is zero, that is the difference between six fleet-wide re-conflictions and none.

### The resolution itself

Three branches, `git merge origin/main` in worktrees under the session scratchpad. Identical conflict set on all three: `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md`. **Never code, for the sixth consecutive time.**

- **`BACKLOG.md`** — one hunk, four lines, resolved by ownership: this branch's `win` cell, `origin/main`'s `mac` cell. Measurement 1 is what makes that resolution provably lossless rather than a judgement call.
- **`JOURNAL.md`** — one hunk. `origin/main`'s 09-21 and 09-20 mac entries placed above this branch's 09-19 windows entry, remainder untouched. Verified by counting dated headers before and after rather than by reading the diff: E80's branch 31 entries, E82's and E19's 29 — **the difference is real and expected**, E80's branch carries its own 09-10 and 09-11 windows entries which are not on `main` or on the other two.
- **`docs/lessons.md`** — regenerated with `extract-lessons.py --write`, never hand-merged (A30). 1505 / 1489 / 1490 lessons respectively.

`check-next-block.py`, `check-id-refs.py`, `check-backlog-archived.py`, `check-links.py` clean on all three post-merge. `check-commit-completeness.py --record`/`--verify` around each commit (`--verify` correctly skips a merge commit). `GH_TOKEN` and the four `GIT_*` variables re-exported immediately before each `git commit`, per the 09-19 lesson; all three landed authored as `tide-rack-bot` first time, `check-commit-authorship.py --repo .` clean on all three with no `--reset-author` needed. **The "is the PR still open" precheck was asserted against the literal string `OPEN` this time, not printed** — which is the 09-19 entry's own slip, and it cost one line of shell.

### The `win` cell is pruned this run, deliberately

A37 is a filed defect about this cell's size and A38's second factor is that git's smallest unit is a line, so a 40 KB cell has no partial merge by construction. The cell arrived at **40,818 bytes across ten generations**; it leaves materially smaller, carrying 09-21, 09-19, 09-18-evening and 09-18 and dropping the six older ones. **Nothing is lost: each pruned generation is told in full in `JOURNAL.md` under its own date**, which is the same justification the 09-08 cell used when it pruned. Lints re-run after the prune.

### STEP 2 walk — the queue is empty for this lane, seventh confirming cell

Walked in file order against freshly-fetched `origin/main`, not taken from the previous cell's word. Every `TODO` row with `Plat` of `win` or `any`: **A35** parked on its own two `PROPOSED:` entries (STEP 2 makes it ineligible regardless of status, by its own row); **A38** filed, its `PROPOSED:` entry already on these three branches since 09-19, awaiting Jeff and explicitly not a run's to implement; **S8** `NEEDS-SPEC`; **E2** an umbrella its own row calls not takeable; **E19/E80/E82** this lane's own open PRs; **E72/E81** macOS's open PRs; **E76/E79** linux in substance; **E84** a `.github/workflows/**` edit the bot's token deliberately cannot make. Nothing eligible, and no invented work.

### STEP 1 — still structurally empty on this platform, sixth generation of this warning

`gh issue list --label platform:win` empty, **and that verifies nothing**: `build.yml:523` excludes `matrix.platform != 'win'` from filing platform issues. Read `main`'s build instead — green at `13095a395`, run [34438892984](https://github.com/JeffMcClintock/TideSynth/actions/runs/34438892984). Every commit since is docs-only so `guard` skipped the matrix, which is the guard working. Two open issues fleet-wide, neither this platform's: [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (`platform:linux`, `tide-rack-bot`) and #44 (the watchdog digest).

### What this run did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (linux) or [#585](https://github.com/JeffMcClintock/TideSynth/pull/585)/[#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (mac)** — all three still `CONFLICTING` on the same three bookkeeping files and needing no toolchain. STEP 1.5 is scoped to `tide/{PLATFORM}/**`. [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) is `MERGEABLE/CLEAN`. **Fourth consecutive windows cell to flag #584 as one mechanical resolution from mergeable.**
- **Did not implement any A38 option.** Not (b), not (c), nothing. Its row says file and stop, and the filing happened on 09-19.
- **Did not rotate `JOURNAL.md`** — **eighth** consecutive cell to defer it. A rotation rewrites the bottom of the file and would conflict with all seven open PRs at once, and A36 — which restores the rotation instruction *still absent from `main`* — is itself #585.
- **Did not build, launch a host, install a plug-in, or take the screen.** Nothing this run needed one.

**Learned:**

- **"Disjoint" is a hashable property, and hashing it converts a merge argument into a fact.** Three revisions × four cells × `sha256` is one short script and it says, with no interpretation, which side changed what. Any future argument about whether a bookkeeping conflict was a real disagreement should open with that table rather than with a description of the diff.
- **A file whose convention is "newest at the top" hands every concurrent writer the same insertion anchor, so it conflicts by construction rather than by bad luck.** Measured here as line 11 on all three revisions. It follows that *any* fix for `JOURNAL.md` that keeps one file and one anchor only changes how often the collision happens, not whether it does — which is an argument for A38's per-run-file half (c) specifically, not merely for splitting the NEXT block.
- **A convention recorded in one lane's NEXT cell cannot reach another lane, and the cost is measurable in whole runs.** Change 2 has been live in the `win` lane since 09-18 and produced a 1-versus-5 difference in merge events over eleven days, and the other lane had no way to know it existed. **Anything intended to change how every box behaves belongs in the shared prompt; a NEXT cell is a lane's own memory, not a broadcast channel.**
- **Check the whole population, not the three the rule names.** STEP 1.5 lists failing checks, requested changes and unresolved comments; for the seventh time the actual state was `CONFLICTING` with everything green, nothing reviewed and nothing commented. One extra field — `mergeStateStatus` — on a `gh pr view` the step already makes you run. Read `UNKNOWN` as "not computed yet" and re-query; it came back `UNKNOWN` on the first bulk read this run too.

**Not verified:**

- ~~The self-hosted `windows` and `macos` compile legs on the three re-pushed branches.~~ **Closed before this entry was finished, and the answer is more interesting than "green": those two legs did not run at all, because `guard` skipped the matrix.** Every push this run is docs-only, so the guard is working exactly as the STEP 1 paragraph above describes it working on `main`. All three PRs are `MERGEABLE`/**`CLEAN`** with all six checks that did run passing — `e57-delete-key`, `lint`, `linux`, `render-linux`, `render-macos`, `render-windows` — and `guard`/`matrix.name` reporting `skipping`. **So this bullet closed in minutes rather than the ~30 the 09-19 run spent waiting on the single self-hosted runner, and the reason is that a bookkeeping-only run does not compile anything.** That is worth knowing before budgeting a wait: check whether `guard` skipped before deciding to stay alive for the matrix.
- **Whether the ride-along convention actually holds under a sweep.** Its cost is that if all three win PRs were closed unmerged, this entry goes with them. The branch names are in the NEXT cell, which is the mitigation STEP 4 already relies on, and it has never been tested.
- Whether the prune leaves the `win` cell readable enough for a run with no other context. Lints pass; legibility is a judgement and the next windows run is the one that finds out.

**Machine state — the developer was at the machine all run, the fifth consecutive windows cell to say so.**
`Get-Process | Where-Object { $_.MainWindowTitle }` at run start: **Visual Studio on `SpacePro — PresetSelector.h*`** (the asterisk is unsaved changes), Chrome, Slack, Settings. Per the 09-09 cell's rule — an unlocked screen with the developer working at it is a stronger reason to stay off the GUI than a locked one — **no host was launched, nothing was built, no screenshot taken, and `%APPDATA%` was not touched.** All work in **`git worktree`s under the session scratchpad**; `C:\SE\TideSynth` stayed on `main` and clean from first command to last, and the worktrees were removed at the end.
**Sibling repos read but not touched.** All dirt predates this run by three to eleven days (mtimes 09-18 and 09-10 against a 10:51 start on 09-21) and was left strictly alone per STEP 5's third category: `SynthEditLib` `EditorLib/PatchParameter.cpp` and `modules/Diagnostics2/ParametersQueryGui.cpp`; `GMPI` `Extensions/ParameterIterator.h`; `GMPI_Wrappers` `wrapper/AU3/AU3_Wrapper.mm` (`git diff --ignore-all-space` is **0 bytes** — pure CRLF churn, still not reverted, because it is in a repo this run did not commit in and the tree is the developer's). `SE16` and `gmpi_ui` clean; all five on their default branches. **Byte-for-byte the same dirt the 09-18-evening and 09-19 cells recorded.** `check-no-direct-commits.py` clean on `TideSynth`.

**Next:** **merging the batch is still the highest-value action available to this project, for the fifth run running — and the eleven-day table above is the argument, not the three PRs.** **Merge [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) first**, then the rest, re-checking `mergeStateStatus` between each. **Second is ruling on A38's `PROPOSED:` entry.** **Third, and new, and much the cheapest: one sentence in STEP 4 telling every lane to ride its bookkeeping on an already-open PR of its own platform where one exists** — that is a five-to-one difference in merge events on this window's evidence, it needs no layout change, and it is available whatever A38 is ruled. The next windows run should do STEP 1.5 first, expect it to be the whole run again, and read `mergeStateStatus` on every open PR rather than the three states the rule lists.

**Branch/PR:** no new branch, per the 09-18 evening entry's Change 2. This entry, the pruned-and-re-pointed `win` NEXT cell and the A38 addendum ride on all three of `tide/win/E80-clap-editor-arm` ([#586](https://github.com/JeffMcClintock/TideSynth/pull/586)), `tide/win/E82-rack-menu-producer` ([#587](https://github.com/JeffMcClintock/TideSynth/pull/587)) and `tide/win/E19-datatype-census` ([#590](https://github.com/JeffMcClintock/TideSynth/pull/590)), byte-identical on each.
## 2026-09-21 — windows — A38: the livelock measured, and the cheap half of its own proposal is as good as the expensive half (scheduled run)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **A38** — the first run to do so — and did what its own row instructs: filed the `PROPOSED:` entry in [docs/decisions.md](docs/decisions.md) and stopped there. Wrote [tests/a38_bookkeeping_merge_probe.py](tests/a38_bookkeeping_merge_probe.py) to satisfy A38's Accept clause, which asks for "a scripted two-branch test, not … waiting for it to happen". **The measurement changed the shape of the request**, which is the whole reason a ruling deserves a truth table under it.

### STEP 1 / STEP 1.5 — and this lane's streak broke

`gh issue list --label platform:win` empty, which as five prior cells have said **verifies nothing**: the exclusion is still `build.yml:523` (`matrix.platform != 'win'`), read again this run rather than taken on the previous cell's word. Read `main`'s latest `build` instead — run [34438892984](https://github.com/JeffMcClintock/TideSynth/actions/runs/34438892984) at `13095a395`, **success on all three platforms**. Two open issues fleet-wide, neither win: [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (`platform:linux`, tide-rack-bot) and #44 (the watchdog digest).

**STEP 1.5 found all three of this platform's PRs `MERGEABLE`/`CLEAN` — the first windows cycle in this chain that needed no re-resolution at all.** [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) (E80), [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) (E82), [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) (E19): every check green, no reviews, no comments but this fleet's own. Checked `mergeStateStatus` explicitly, per the 09-18 cell, and got a real value on the first read (no `UNKNOWN` retry needed — the 09-21 mac cell's caution still worth keeping, it just did not bite). Left all three alone, per STEP 1.5's "green with nothing unresolved is not yours to fix".

**That non-event is itself data for A38**, and it is the asymmetry the 09-20 and 09-21 mac cells describe: whether a branch re-conflicts depends on whether the merges that landed since touched the lines that branch touched, not on a fleet-wide sweep. Three mac merges landed since 09-18 and none of them re-dirtied this lane.

### The measurement

The probe seeds a throwaway git repo with this repo's **real** `BACKLOG.md` and `JOURNAL.md` — not a synthetic stand-in, because the claim is about these two files' real shape — and replays the fleet's actual sequence: two branches cut from one `main`, each applying only its own platform's STEP 4 edits, the other box's PR merging first, then the merge of `main` into the branch still open. The two sides' content is **strictly disjoint** by construction: different NEXT rows, different journal entries, no shared claim. `python3 tests/a38_bookkeeping_merge_probe.py`, ~5 s, no build, no network, **rc=0** with all six cases as recorded.

| layout | conflicts in | what it says |
|---|---|---|
| `today` | **`BACKLOG.md`, `JOURNAL.md`** | both hot spots, on disjoint content |
| `next-sections` — blank-line sections in `BACKLOG.md` | `JOURNAL.md` | NEXT block fixed, journal not |
| `next-split` — `docs/next/<plat>.md`, A38's own proposal | `JOURNAL.md` | **identical to sections** |
| `full-split` — per-lane NEXT **and** per-run journal files | *(clean)* | nothing shared, nothing to conflict |
| `ctl-context` — positive control | `BACKLOG.md` | `JOURNAL.md` **drops out** once one unchanged entry sits between the two insertions; `BACKLOG.md`, untouched by the control, stays |
| `ctl-code` — control | *(clean)* | code has never conflicted in this fleet, and does not here |

**`ctl-context` is a within-case control, and that is what makes it worth more than a separate clean run.** One thing changes — the other side inserts its journal entry below the newest instead of above it — and exactly one file moves, the one the model says should. Same layout, same two runs, same harness.

**THE FINDING A38's ROW DOES NOT HAVE: options (b) and (c) measure the same, so `docs/next/` is the expensive way to obtain a blank line.** A38 proposed per-lane files; they work, and they work *for the reason blank-line sections also work*, which is that git gets one unchanged line of context. The cheap half and the expensive half of the proposal are **separable**, and only the journal half genuinely needs new files — because two runs prepending at the top of one file collide however that file is arranged. The `PROPOSED:` entry is written as four options on that split and recommends nothing beyond naming which halves the measurement joins.

### A second number, from `main` itself rather than from branches

`git diff --name-only 13095a395..origin/main` — **six commits over eleven days, touching exactly three files: `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md`. No code at all.** A38 counted the cost on the branch side (16 re-resolutions across five consecutive runs: 09-18 win, 09-18/09-19/09-20/09-21 mac, zero product change). This is the same fact seen from `main`, and it needs no interpretation: the fleet's default branch has moved six times this fortnight and shipped nothing but its own bookkeeping.

### What I did NOT do, deliberately

- **Did not implement any option.** A38's row says the first run to take it should file the `PROPOSED:` entry and stop; the shape changes STEP 4 on three boxes and is Jeff's ruling. The probe is neutral by construction — it asserts today's behaviour and each candidate's, and picks nothing (the A35 precedent, 2026-09-08 mac).
- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584)** (linux, `CONFLICTING`) or [#585](https://github.com/JeffMcClintock/TideSynth/pull/585)/[#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (mac, `CONFLICTING`) — STEP 1.5 is scoped to `tide/{PLATFORM}/**`, and three cells in a row have now noted #584 is one mechanical resolution from mergeable without anyone being allowed to do it.
- **Did not rotate `JOURNAL.md`** — 274 KB against A24's 60 KB, the sixth consecutive cell to defer it, and for the same reason: seven PRs are open and A36 ([#585](https://github.com/JeffMcClintock/TideSynth/pull/585)) *is* the rotation rule. A rotation from this lane conflicts with every one of them at once. **It is also now a reason to rule A38 first:** option (d) changes what there is to rotate, so rotating before ruling means rotating twice.
- **Did not launch a host, build anything, or take the screen.** The developer was at the machine (Chrome, Settings) — no VS and no SynthEdit this time, so nothing was moving underneath the run, but an unlocked screen in use is reason enough to stay off the GUI. This item needed neither.

**Learned:**

- **A proposal's cheap option and its expensive option can measure identically, and only a probe will say so.** A38 asked for per-lane files; blank-line-separated sections in the same file buy exactly the same merge behaviour. The thing that fixes a conflict is one unchanged line of context, not a file boundary — so "give it its own file" is a sufficient way to obtain context, never a necessary one.
- **A prepend-at-top file cannot be fixed by rearranging it.** Two runs inserting at the same anchor collide by construction, which is why `JOURNAL.md` conflicts in three of the four layouts and only the per-run file removes it. Context fixes the NEXT block because the two sides write to *different* places in it; nothing fixes a journal because they write to the *same* place.
- **A within-case control beats a separate clean run.** `ctl-context` changes one thing inside the failing case and exactly one file moves. A separate all-clean scenario would have proved the harness works; this proves the *mechanism*.
- **Seed a merge probe with the real files.** Both hot spots' behaviour depends on their actual shape — a 47 KB single-line table cell, a header block above the newest entry — and a synthetic two-row table would have measured a different file.
- **`main`'s own diff is a cheaper cost metric than counting branch re-resolutions.** Six commits, three files, no code, eleven days — one command, no journal archaeology.
- **A quiet STEP 1.5 is worth recording as loudly as a busy one.** Four cells in a row told this lane to expect re-resolution; it did not happen, and that asymmetry is evidence for A38's model rather than against it.

**Not verified:** nothing about how much work any option costs in `scripts/check-next-block.py`, `check-journal-prepend.py` or `extract-lessons.py` — A38's row calls it small and this run did not re-estimate it, which is said in the `PROPOSED:` entry too. The probe measures conflict *occurrence*, not conflict *size*; the size question is A37's and stays open.

**Machine state.** `C:\SE\TideSynth` started and ended on `main`, clean, and was **never checked out to a branch** — all work in a `git worktree` under the session scratchpad, removed at the end. No other repo was touched. Checked all four siblings anyway, because the next run relies on this line: `SE16` on `master`, `SynthEditLib` and `gmpi_ui` on `main`, all three clean; **`GMPI_Wrappers` is on `main` with `wrapper/AU3/AU3_Wrapper.mm` modified, and it is PURE LINE-ENDING CHURN** -- `git diff --ignore-all-space` prints nothing. Left exactly as found rather than reverted: STEP 5's revert-the-churn rule is for a repo a run is working in, and this run committed in `TideSynth` only. No host launched, no plug-in built or installed, no screen taken. The developer was at the machine throughout (Chrome and Settings; no VS, no SynthEdit).

**Next:** **the highest-value single action available is still merging [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36)**, unchanged from five cells running — and A38 now gives a second reason to sequence it with a ruling rather than before one. For the next windows run: STEP 1.5 first and check `mergeStateStatus` per-PR even when the previous cell says the lane was clean, because this cell's own finding is that the streak is per-PR and not a fleet property. **The queue for this lane is otherwise empty** and has been since 09-17; A38 is now `IN-REVIEW` and takes itself off the board. If nothing has changed, a run that finds no eligible row should say so and stop rather than invent work — that is a fine outcome and this queue has now produced it three times.

**Verification, with the exit codes these commands actually returned** (the open `PROPOSED:` question about recording exit codes is a standing caution, not a park): `check-backlog-diff` **0**, `check-journal-prepend` **0**, `check-prompt-provenance` **0**, `check-next-block` **0**, `check-id-refs` **0** (its standing E2-umbrella advisory prints, informational), `check-links` **0**, `check-backlog-archived` **0**, `tests/a38_bookkeeping_merge_probe.py` **0**. `check-commit-completeness.py --record`/`--verify` around every commit, no discrepancy; `check-commit-authorship.py` clean on all three, no `--reset-author` needed. `docs/lessons.md` regenerated with `extract-lessons.py --write` (A30), never hand-edited.

**One thing that cost a cycle and is worth the next run's attention:** `check-backlog-diff.py` requires the base row's Item text to survive as a CONTIGUOUS substring, so appending STEP 4's branch name in the MIDDLE of an existing row -- even purely additively -- fails with `Item column differs`. Append at the END of the cell. The docstring says "still present verbatim somewhere inside the new Item text" and means contiguous; the check is the arbiter, per the 2026-09-03 precedent, and it was right.

**Branch/PR:** `tide/win/A38-bookkeeping-livelock`, [#597](https://github.com/JeffMcClintock/TideSynth/pull/597).


## 2026-09-21 — macos — STEP 1.5 for the sixth time on the same shape: A38's livelock recurred again, same two branches as 09-20 (scheduled run)

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** re-checked `mergeStateStatus` on this platform's three open PRs before touching anything, per STEP 1.5 and per the standing lesson that a CONFLICTING PR is not "green and waiting for merge". [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) and [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72) had gone `mergeStateStatus: DIRTY` / `mergeable: CONFLICTING` again — the same two branches the 09-20 cell resolved, re-conflicted by `main` moving once more (`#595`, the 09-20 cell's own journal+NEXT-cell PR). [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) stayed `CLEAN`/`MERGEABLE` for a second cycle running, consistent with A38's diagnosis and the 09-20 entry's own note that the asymmetry is per-PR, not fleet-wide.

### STEP 1 / STEP 1.5

`platform:mac` issues empty (checked `TideSynth`, `SynthEditLib`, `GMPI_Wrappers`, `gmpi_ui`, `SynthEdit_Rack_Adaptor`). Screen locked (`CGSSessionScreenIsLocked` present, `ioreg -n Root -d1 -a`). `mergeStateStatus` read via GraphQL directly, not assumed from any prior cell's word for it — first read came back `UNKNOWN` on all three (GitHub had not finished computing it), a five-second retry settled it.

### The resolution

Both branches merged `origin/main` in the main checkout (not a worktree this time — no concurrent session on this box). **E72: `BACKLOG.md` and `JOURNAL.md` both auto-merged cleanly**; only `docs/lessons.md` conflicted (generated-content drift) and was regenerated via `extract-lessons.py --write`, never hand-merged. **A36: `BACKLOG.md` auto-merged cleanly; `JOURNAL.md` and `docs/lessons.md` both conflicted.** `JOURNAL.md`'s conflict was the same shape the 09-10/09-19/09-20 entries already document: HEAD (A36's own branch) holds the restored "Rotation" header block above the first dated entry; `origin/main` holds the new 09-20 entry appended below where that header used to sit. Resolved by keeping HEAD's header block, then `origin/main`'s new entry, then the unconflicted remainder — one splice, no interleaving needed. **Verified nothing was lost rather than assumed:** the entries A36's own prior rotation had already moved out of `JOURNAL.md` (09-08 windows through 08-31) are present verbatim in `JOURNAL-2026-09.md` on that branch, and the resolved `JOURNAL.md` carries both the 09-20 and 09-19 entries intact (checked by `grep -n "^## 202"` before and after). `docs/lessons.md` on A36 regenerated the same way as E72's.

`check-next-block.py`, `check-id-refs.py`, `check-backlog-archived.py` and `check-links.py` all clean on both branches post-merge (`check-id-refs.py`'s standing E2-umbrella advisory prints on both, informational, unrelated to this merge). `check-commit-completeness.py --record`/`--verify` around each commit, no discrepancy (both merge commits, so `--verify`'s name-only diff correctly skips itself against a merge's first parent — expected, not a gap). Exported `GH_TOKEN`/`GIT_AUTHOR_*`/`GIT_COMMITTER_*` immediately before each `git commit`, per the 09-19 lesson; both commits landed authored as `tide-rack-bot` on the first attempt, `check-commit-authorship.py` clean on both with no `--reset-author` needed.

Pushed both to their existing branches (STEP 1.5: fix in place, no new PRs) — `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty`. Did not touch #589 (E81), already `MERGEABLE`/`CLEAN`. Re-checked `mergeStateStatus` after each push (8-second wait, then GraphQL): both `MERGEABLE` again, `UNSTABLE` only on still-queued compile legs — nothing red.

### What I did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584)** (linux's `tide/linux/E79-clap-headless-document`) — STEP 1.5 is scoped to `tide/{PLATFORM}/**`.
- **Did not re-walk STEP 2's backlog in depth** — walked the file order quickly to check for anything newly eligible since 09-20: **A38** (new since 09-17, filed by windows) is a design proposal "offered not ruled" and explicitly wants Jeff's decision on its shape, not takeable under STEP 2; everything else matches the 09-20 cell's own walk (A35/S8 parked on rulings, E19 wants the unlocked screen, E2 not takeable, E72/E76/E79/E80/E81/E82 all either this platform's own open PR, another platform's, or linux-in-substance, E84 a workflow edit the bot's token cannot make). Screen locked throughout, same as the eight prior mac cells (09-07 through 09-20) — nothing GUI-dependent was attempted.
- **Did not rotate `JOURNAL.md` further** — A36's own rotation (still open on #585) already moved the bulk of history to archive; this cell only added the new top entry, same as 09-20's own choice.

**Learned:**

- **The livelock is now confirmed on its third consecutive run boundary, always the same two branches once #589 (E81) stopped re-conflicting.** Whether a branch re-conflicts on a given `main` merge continues to depend on whether that merge's changed hunks overlap the specific lines that branch last touched (A38's model), not on a fleet-wide "all conflicted again". Worth checking `mergeStateStatus` per-PR every cycle rather than assuming symmetry from the last cell's shape.
- **`mergeable`/`mergeStateStatus` can read `UNKNOWN` immediately after a merge to `main` lands** — GitHub had not finished computing it on the first query this run; a short retry (5-8s) settled it to a real value both times. Don't treat `UNKNOWN` as `CLEAN` or as a failure; re-query.
- **A merge-base rotation (deleting old entries from `JOURNAL.md` because they moved to an archive file) auto-merges as a clean deletion when the other side hasn't touched those lines** — which is correct, but the file's line count then legitimately diverges hugely from `origin/main`'s unrotated copy. Checking dated-entry headers on both sides (`grep -n "^## 202"`) is the one-command sanity check that the deletion is a rotation and not data loss, cheaper than diffing the whole file.

**Not verified:** the self-hosted `macos`/`windows` compile legs on both re-pushed PRs — still `UNSTABLE`/queued when this entry was written.

**Machine state.** All repos started and ended clean on their default branches except `TideSynth`. No host was launched, no plug-in built or installed. Screen was locked throughout. Work done directly in the main checkout (no worktree — no concurrent session detected); the checkout is left on `origin/main` after this entry's own branch is created and pushed, per STEP 5.

**Next:** same as every prior cell in this chain — merging **#585 (A36) first** unblocks `JOURNAL.md` rotation and is the highest-value single action available; every day these two (down from three since 09-20) stay open costs another box another run repeating this recipe. The next run touching `BACKLOG.md`/`JOURNAL.md` should check `mergeStateStatus` per-PR, expect a possible `UNKNOWN` on the first read, and re-walk STEP 2 in file order rather than trust this entry's "matches 09-20" summary once `main` has moved with any code change (none has, since 09-17).

**Branch/PR:** `tide/mac/2026-09-21-step15-conflicts` — this entry and the refreshed `mac` NEXT cell only. The two fixes are on `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty` themselves (their own pushed merge commits).

## 2026-09-20 — macos — STEP 1.5 for the fifth time on the same shape: A38's livelock recurred again, only A36 and E72 needed the recipe this time (scheduled run)

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** re-checked `mergeStateStatus` on this platform's three open PRs rather than trusting the 09-19 entry's final state. `main` moved once since (`#594`, the 09-19 mac cell's own journal+NEXT-cell PR), and that alone was enough to re-conflict [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) and [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72) — both `mergeStateStatus: DIRTY` / `mergeable: CONFLICTING`. [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) had stayed `CLEAN`/`MERGEABLE` — the first time in five cycles that not all three PRs re-conflicted together, consistent with A38's own model (only the branches whose stale content collides with what actually changed on `main` re-conflict; #594 touched `BACKLOG.md`'s NEXT rows and `JOURNAL.md`, and E81's branch's copies of those apparently no longer overlapped the changed hunks).

### STEP 1 / STEP 1.5

`platform:mac` issues empty (checked `TideSynth`, `SynthEditLib`, `GMPI_Wrappers`, `gmpi_ui`, `SynthEdit_Rack_Adaptor`). Screen locked (`CGSSessionScreenIsLocked` present). Confirmed `mergeStateStatus` directly by `gh pr list --json mergeable,mergeStateStatus` before touching anything, not assumed from the 09-19 entry.

### The resolution

Both branches merged `origin/main` in a worktree. **A36: `BACKLOG.md` auto-merged cleanly this time** (no NEXT-cell corruption, unlike 09-19) — only `JOURNAL.md` and `docs/lessons.md` conflicted. `JOURNAL.md`'s conflict was the shape A36 itself documents: HEAD held the restored "Rotation" header block (A36's own change, sitting above the first dated entry), `origin/main` held the 09-19 entry appended below where that header used to be missing from. Resolved by keeping HEAD's header block, then `origin/main`'s new entry, then the unconflicted remainder — a `<<<<<<</=======/>>>>>>>` splice with no interleaving needed, since only one conflict hunk existed. **E72: `BACKLOG.md` and `JOURNAL.md` both auto-merged**; only `docs/lessons.md` conflicted (a stats-line mismatch, 1477 vs 1474 lessons — the file's own generated-content drift). Both `docs/lessons.md` conflicts were resolved by regenerating via `extract-lessons.py --write` on the already-resolved `JOURNAL.md`, never hand-merged, per the 09-18/09-19 cells' own instruction.

`check-next-block.py`, `check-id-refs.py`, `check-backlog-archived.py`, `check-links.py` all clean on both branches post-merge (`check-id-refs.py` prints its standing E2-umbrella advisory on both, which is informational, not a failure, and unrelated to this merge). `check-commit-completeness.py --record`/`--verify` around each commit, no discrepancy. **Re-exported `GH_TOKEN`/`GIT_AUTHOR_*`/`GIT_COMMITTER_*` immediately before each `git commit`, per the 09-19 lesson** — both commits landed correctly authored as `tide-rack-bot` on the first attempt, and `check-commit-authorship.py --repo .` was clean on both without needing a `--reset-author` amend this time.

Pushed both to their existing branches (STEP 1.5: fix in place, no new PRs) — `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty`. Did not touch #589 (E81), which was already `MERGEABLE`/`CLEAN` and needed nothing. Re-checked `mergeStateStatus` after each push: both `MERGEABLE` again (`UNSTABLE` only because the queued/in-progress compile legs hadn't reported yet at push time — nothing red).

### What this adds to A38

**Not every open PR touching the hot files re-conflicts on every intervening merge — #589 (E81) sat out this cycle where it had been swept up in the 09-18 and 09-19 cycles.** Consistent with A38's own diagnosis (git needs one unchanged line of context per side; whether a given branch's stale copy of `BACKLOG.md`/`JOURNAL.md` actually overlaps the changed hunk depends on where in those files each branch happens to have touched), but this is the first cycle where it was visibly asymmetric rather than all-or-nothing across the three. **Did not implement A38's per-lane-file proposal** — still Jeff's ruling, not a run's, per A38's own row.

### What I did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584)** (linux's `tide/linux/E79-clap-headless-document`) — STEP 1.5 is scoped to `tide/{PLATFORM}/**`.
- **Did not re-walk STEP 2's backlog** — the queue has been established empty across seven prior mac cells (09-07 through 09-19) with no code change in the meantime that would newly unblock a row, and resolving two re-conflicted PRs is itself STEP 1.5 work that outranks a fresh walk.
- **Did not rotate `JOURNAL.md`** — still blocked on #585 (A36, the rotation rule itself) actually merging, still open.

**Learned:**

- **The livelock is not always all-three: whether a given open PR re-conflicts on a given `main` merge depends on whether that merge's changed hunks overlap the specific lines that PR's branch last touched, not just on which files were touched.** #589 (E81) survived a merge that re-conflicted its two siblings, the first asymmetric cycle in five. Worth checking `mergeStateStatus` per-PR rather than assuming a fleet-wide "all conflicted again" from the shape of the last few cells.
- **Once the export-immediately-before-commit discipline from the 09-19 entry is followed, the authorship failure it describes does not recur** — both commits this run landed as `tide-rack-bot` on the first attempt, no `--reset-author` needed. The fix held.

**Not verified:** the self-hosted `macos`/`windows` compile legs on both re-pushed PRs — still queued/in-progress when this entry was written.

**Machine state.** All six repos started and ended clean on their default branches except `TideSynth`. No host was launched, no plug-in built or installed. Screen was locked throughout. Work done in `git worktree`s under the session scratchpad; the main checkout was left on `origin/main`, unmodified.

**Next:** same as every prior cell in this chain — merging **#585 (A36) first** unblocks `JOURNAL.md` rotation and is the highest-value single action available. Every day these three (well, now sometimes fewer) stay open costs another box another run repeating this recipe. The next run touching `BACKLOG.md`/`JOURNAL.md` should check `mergeStateStatus` per-PR rather than assume the whole set needs the same treatment.

**Branch/PR:** `tide/mac/2026-09-20-step15-conflicts` — this entry and the refreshed `mac` NEXT cell only. The two fixes are on `tide/mac/A36-journal-rotation-rule` and `tide/mac/E72-cable-dsp-dirty` themselves (their own pushed merge commits).

## 2026-09-19 — windows — nine days, zero product lines on `main`: A38's Accept clause measured, and its `PROPOSED:` entry finally filed (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.2553.1** (Code tab) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no product code changed. STEP 1.5 was the entire run for the **fourth consecutive windows run** — all three `tide/win/**` PRs were `CONFLICTING` again, undone this time by [#594](https://github.com/JeffMcClintock/TideSynth/pull/594), the macOS box's own bookkeeping PR. Resolved and pushed all three. **Then filed A38's `PROPOSED:` entry, which four runs in a row each had the evidence for and each deferred, and measured A38's own Accept clause with the scripted two-branch test that clause asks for.**

### The number this run exists to put on the board

**`main` has not gained a single non-bookkeeping line in nine days.**

    git log --first-parent 13095a395..origin/main   ->  4 merges
    git diff  --stat       13095a395..origin/main   ->  3 files, 227 insertions

The last product line on `main` is Jeff's [`13095a395`](https://github.com/JeffMcClintock/TideSynth/commit/13095a395), **2026-09-10**. Everything merged since — [#591](https://github.com/JeffMcClintock/TideSynth/pull/591), [#592](https://github.com/JeffMcClintock/TideSynth/pull/592), [#593](https://github.com/JeffMcClintock/TideSynth/pull/593), [#594](https://github.com/JeffMcClintock/TideSynth/pull/594) — lands in exactly `BACKLOG.md`, `JOURNAL.md` and `docs/lessons.md` and **nowhere else**. Three of the four are STEP 1.5 conflict resolutions; the fourth is a "queue empty" cell.

Against that, the seven open PRs carry **23 non-bookkeeping files and 3,964 added lines**, per-PR against each one's own merge base:

| PR | branch | non-bookkeeping |
|---|---|---|
| [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) | `tide/linux/E79-clap-headless-document` | 2 files, 198+/6− |
| [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) | `tide/mac/A36-journal-rotation-rule` | 1 file, 14+/3− |
| [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) | `tide/win/E80-clap-editor-arm` | 9 files, 2089+/12− |
| [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) | `tide/win/E82-rack-menu-producer` | 2 files, 254+/0− |
| [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) | `tide/mac/E72-cable-dsp-dirty` | 3 files, 334+/18− |
| [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) | `tide/mac/E81-handle-determinism` | 2 files, 466+/6− |
| [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) | `tide/win/E19-datatype-census` | 4 files, 609+/0− |

**The fleet's throughput of product change over nine days is zero, and its entire recorded output for those nine days is resolving conflicts created by recording that it resolved conflicts.** Prior cells have described this as a cost; it is not a cost any more, it is the whole of the output. That is the difference between A38 as a filed observation and A38 as the thing now blocking the project.

### STEP 1 / STEP 1.5

`gh issue list --label platform:win` empty across all five fleet repos — **and that still verifies nothing**, for the fifth generation of this cell: `build.yml:523` excludes `matrix.platform != 'win'` from filing platform issues. Read `main`'s build instead. Green at `13095a395` (run [34438892984](https://github.com/JeffMcClintock/TideSynth/actions/runs/34438892984)); every commit since is docs-only, so `guard` skipped the matrix correctly and the absence of newer runs is the guard working, not a gap.

All seven open PRs read `mergeStateStatus` before anything was touched. **Six `CONFLICTING/DIRTY`, one `MERGEABLE` ([#589](https://github.com/JeffMcClintock/TideSynth/pull/589)), none red, none reviewed, none commented** — so under STEP 1.5's literal list of three (*failing checks, requested changes, unresolved review comments*) six of seven still read as "green and waiting on Jeff". **Sixth-plus occurrence across the three boxes; `mergeStateStatus` is still not in the rule and is still one extra field on a `gh pr view` STEP 1.5 already makes you run.**

Conflicted file set, all three branches, identical: `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md`. **Never code, for the fifth consecutive time.**

### The 09-18 evening run's identical-line fix held, and production showed exactly where it stops working

At run start the `win` NEXT cell was **byte-identical on all three branches** (sha256 `ff4e818d…`, 36,144 bytes) against `main`'s 18,923 — so Change 1 from the 09-18 evening entry survived untouched. **It did not prevent this run's conflicts, and the reason is the one that entry predicted in a sentence and this run watched happen:** `win` is line 11 of the NEXT table and `mac` is line 12, #594 re-pointed `mac`, and a markdown table cannot carry a blank line between its rows. All three `win` PRs therefore conflicted on a hunk containing a row none of them had any opinion about.

**That is arm 1 of the experiment below, observed in production on the same day it was measured in a lab.**

### A38's Accept clause, measured — `scripts/a38-merge-layout-experiment.py`

A38's row asks for this in terms: *"two runs on different platforms can each append a journal entry and re-point their own NEXT cell, on branches cut from the same `main`, and both merge with no conflict — **demonstrated by a scripted two-branch test, not by waiting for it to happen**."* The script does that. It builds throwaway repos in a temp directory from `origin/main`'s **real** `BACKLOG.md` and `JOURNAL.md`, cuts branches `W` and `M` from one base, has each make the edits **one** platform's run would make, and merges. Four arms, two layouts × two edit sets. Against `792330672`:

    arm  layout  edits        result
    1    today   NEXT only    CONFLICT  BACKLOG.md
    2    (b)     NEXT only    clean
    3    today   full STEP 4  CONFLICT  BACKLOG.md, JOURNAL.md
    4    (c)     full STEP 4  clean

**Every arm's two edits are strictly disjoint — no two branches ever change the same fact — so every conflict reported is an artifact of where the bytes live and nothing else.** Arms 1 and 3 are the negative control the proposal never had: *the present layout conflicts on edits that do not disagree about anything.* Arms 2 and 4 say the proposed layout does not.

Three things worth knowing about it rather than re-deriving:

- **It needs no toolchain.** No compiler, no host, no GUI, no network. It is the right experiment for exactly the box this ran on, with the developer working at it all day.
- **It reads the real files, not a reduction.** The `mac` cell it splits is the genuine 43 KB single line; the journal it splits is the genuine 24 entries. A toy table would have proved nothing about this repo.
- **It asserts its own expectation and says so when reality differs** — the two `today` arms are expected to conflict and the two split arms are expected not to. If a future run gets a different answer that is a result to write down, not a broken script.

### A38's `PROPOSED:` entry is filed

Appended to `docs/decisions.md`, carrying the four arms, the nine-day measurement, and the option split. **Three options**, and the split between them is the part worth reading:

- **(b), the NEXT block alone, needs no change to the shared prompt** — STEP 2 still reads a NEXT block in `BACKLOG.md`, STEP 4 still updates it, and the only code cost is `check-next-block.py` following four links. So (b) is executable by an ordinary run *if Jeff rules for it*.
- **(c) does change STEP 4**, which says in terms "Append a `JOURNAL.md` entry using the template at the top of that file". That is the half that is Jeff's, and it is why A38 was never a run's to implement.
- **(b) alone relocates the conflict rather than ending it, and that is observed twice** — once by the 09-18 evening run in production, once by arm 3 on demand.

Recommendation is **(c)**. The entry parks nothing and makes no row ineligible; its *May proceed meanwhile* line says so explicitly, and it is written that way on purpose because an open `PROPOSED:` entry is otherwise a park under STEP 2.

**The entry touches no existing line of `docs/decisions.md` — it appends only.** That was deliberate: [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) and [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) both *rewrite* the section's intro paragraph as well as appending, and a rewrite of shared prose is the kind of conflict that needs a human, whereas two appends at the same place are resolved by keeping both. **Given a choice of where to conflict, choose the place where the resolution is mechanical.**

### Why A38 was filed on a run whose STEP 1.5 was not "clean" at start

The `win` cell and the 09-18 evening entry both said to take A38 *"if the three win PRs are clean"*, and at run start they were not. Filed anyway, and the reasoning is recorded because it is a judgement:

1. **They were clean by the time it was filed** — the filing rides on the same three branches, in the same commits as their resolutions, so the precondition held at the moment the act happened.
2. **It costs no extra merge event**, which is the scarce thing. Per the 09-18 evening entry's Change 2, this run opened **no new PR**: this journal entry, the re-pointed cell, the `PROPOSED:` entry and the experiment script all ride on #586/#587/#590. The cheapest bookkeeping PR is still the one you do not open.
3. **A fifth deferral would have been the process eating itself.** Four runs produced the evidence and none filed it; the evidence is now that fleet throughput is zero. A rule read so literally that it forbids the only action able to end the loop is being read wrong.

**What this run did NOT do on A38: implement anything.** Not (b), not the script's layout, nothing. A38's row says a run that takes it files the entry and stops. That is what happened.


### Three pushes, not one, and one slip worth recording

Each of the three branches got **three** commits from this run, each verified and pushed separately: the merge resolution plus the `PROPOSED:` entry and the experiment script; the sibling-repo record added to this entry's machine-state paragraph; and **a note appended to A38's own row** saying the `PROPOSED:` entry is filed. That third one is not bookkeeping for its own sake — **without it the next run reads A38's row, sees *"the first run to take it should file the `PROPOSED:` entry and stop there"* still outstanding, and files a second one.** That is C15/C16's failure mode exactly, and the NEXT cell alone does not prevent it because a run walking the backlog reads rows, not only the cell. The row stays `TODO`; the note says what stopped it is the ruling, not the work, and `check-backlog-diff.py` passes it because the base Item text is present verbatim and `Status`/`Plat` are untouched.

**The slip: the "is the PR still open" precheck — STEP 4's guard against the #120 trap — silently did not run before the third push.** It was invoked from a directory that is not a git repository, so `gh` printed `fatal: not a git repository` *into the field the check reads*, and the loop printed `#586=failed to run git: ...` rather than `OPEN`. **It did not fail loudly; it produced a non-answer that reads like output.** All three PRs were confirmed `OPEN` immediately afterwards and all three heads match what was pushed, so nothing was lost — but the check protected nothing at the moment it was supposed to. **A guard that can return a non-answer needs its answer asserted, not printed:** the loop should have compared against the literal string `OPEN` and stopped otherwise. Filed here rather than as a row because it is one line of shell discipline, not a defect in anything the repo ships.

### What this run did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (linux) or [#585](https://github.com/JeffMcClintock/TideSynth/pull/585)/[#588](https://github.com/JeffMcClintock/TideSynth/pull/588)/[#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (mac).** #584 and #585 and #588 are still `CONFLICTING` on the same three bookkeeping files and need no toolchain; STEP 1.5 is scoped to `tide/{PLATFORM}/**` and STEP 2's collision rule treats another platform's branch as taken. **Third consecutive windows cell to flag #584 as one mechanical resolution from mergeable.**
- **Did not rotate `JOURNAL.md`** — now **266 KB / 24 entries** against A24's 60 KB, the **seventh** consecutive cell to defer it. A rotation rewrites the bottom of the file and would conflict with all seven open PRs at once, and A36 — which restores the rotation instruction *still absent from `main`* — is itself #585.
- **Did not take a backlog item beyond A38's filing.** STEP 1.5 outranks STEP 2 and was the run.
- **Did not build, launch a host, install a plug-in, or take the screen.**

**Learned:**

- **"The fleet is slow" and "the fleet has stopped" are different claims, and the second is checkable in one command.** `git diff --stat <last-known-product-commit>..origin/main` over any window says whether anything but bookkeeping landed. Nine days of this project's history reduce to 227 insertions in three files. **Any process whose own record-keeping is the only thing its record shows should measure that ratio before writing another entry into it.**
- **A conflict-avoidance trick that equalises one line does not survive an edit to the line next to it, and adjacency beats content.** The `win` cell was byte-identical across all three branches and all three still conflicted, because `mac` moved. **Equalising is worth doing — it removes branch-versus-branch conflicts — but it cannot remove branch-versus-other-platform ones, and only a layout change can.**
- **When you must edit a shared file that other open PRs also edit, append and touch no existing line.** Two appends at one spot conflict trivially and resolve by keeping both; a rewrite of shared prose does not. This is a choice available on most bookkeeping edits and it is nearly free.
- **A proposal that asks for a ruling should arrive with its own Accept clause already measured, and a merge-layout question can be measured with throwaway repos in seconds.** A38 sat unfiled for four runs partly because filing felt like it needed permission. The experiment needs none: it touches no real branch, needs no toolchain, and converts "we think the layout is the problem" into four lines of output anyone can re-run.

**Not verified:**

- ~~Whether the self-hosted `windows` and `macos` compile legs pass on the three re-pushed branches.~~ **CONFIRMED GREEN before this entry was finished, which is the first time in four windows runs that this bullet did not have to be left open.** All three PRs are `MERGEABLE`/**`CLEAN`** with every check passing -- `e57-delete-key`, `guard`, `lint`, `linux`, **`macos`**, `render-linux`, `render-macos`, `render-windows`, **`windows`** -- about 30 minutes after the last push, on a box the developer was using throughout. **The three cells above this one each said "confirm rather than assume" and none of them could; the cost of confirming is one `gh pr checks` and the wait, and the wait is what nobody had spent.** Note the wait is the honest part: the self-hosted queue really is one runner, so a run that wants this bullet closed has to stay alive for it rather than finish on "nothing had gone red yet".
- **Whether arm 2's and arm 4's clean merges hold at scale.** The experiment merges two branches. The fleet routinely has seven open, and the script does not model an N-way sweep — it models the pairwise case A38's Accept names, which is the case that has actually been failing.
- **Whether `check-next-block.py` can in fact follow links, as option (b) assumes.** Not attempted — implementing (b) is exactly what this run declined to do before a ruling. The claim in the entry is that the prompt does not forbid (b); it is not a claim that the lint change is trivial.
- **Whether #584 and the three mac PRs resolve as mechanically as these three did.** Their conflicted file set is the same three files; no resolution was attempted.

**Machine state — the developer was at the machine all run, the fourth consecutive windows cell to say so.**
`Get-Process | Where-Object { $_.MainWindowTitle }` at run start: **Visual Studio on `SimulatorGmpi - ptc_mind.cpp`**, Chrome, Snipping Tool, Settings. Per the 09-09 cell's rule — an unlocked screen with the developer working at it is a stronger reason to stay off the GUI than a locked one — **no host was launched, nothing was built, no screenshot taken, and `%APPDATA%` was not touched.** This run needed none of it: every conflict is text, the experiment is throwaway repos, and neither needs a toolchain.
All work was done in **`git worktree`s under the session scratchpad**, so `C:\SE\TideSynth` stayed on `main` and clean from first command to last. Worktrees removed at the end.
**Sibling repos were not read, built or touched — recording their dirt anyway, because the 09-09 windows cell's lesson is that a machine-state record which omits it is worse than none.** All of it predates this run (mtimes 09-10 and 09-18, against a 22:11 start on 09-19) and all of it was left strictly alone per STEP 5's third category: `SynthEditLib` `EditorLib/PatchParameter.cpp` (+34/−0) and `modules/Diagnostics2/ParametersQueryGui.cpp` (+28/−1); `GMPI` `Extensions/ParameterIterator.h` (+185/−10); `GMPI_Wrappers` `wrapper/AU3/AU3_Wrapper.mm` (+820/−820, and `git diff --ignore-all-space` is empty, so **pure CRLF churn** — not reverted, because it is in a repo this run did not commit in and the tree is the developer's). `SE16` and `gmpi_ui` clean; all five on their default branches. **This is byte-for-byte the same dirt the 09-18 evening cell recorded, which is itself the signal: the developer's working set has not moved in a day.** `check-no-direct-commits.py` clean on `TideSynth`.

**Next:** **the single highest-value action available to this project is still Jeff merging the batch, and it has been for four runs.** **Merge [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) first** — it restores the `JOURNAL.md` rotation instruction currently missing from `main` — then the rest, re-checking `mergeStateStatus` between each, since every merge re-conflicts the remainder until A38 is answered. **Second-highest is ruling on the `PROPOSED:` entry this run filed**, because the sweep only empties the queue and the answer decides whether the fleet refills it with the same jam. The next windows run should do STEP 1.5 first, expect it to be the whole run again, and **not implement any A38 option before the ruling**.

**Branch/PR:** no new branch, per the 09-18 evening entry's Change 2. This entry, the re-pointed `win` NEXT cell, the `docs/decisions.md` `PROPOSED:` entry and `scripts/a38-merge-layout-experiment.py` ride on all three of `tide/win/E80-clap-editor-arm` ([#586](https://github.com/JeffMcClintock/TideSynth/pull/586)), `tide/win/E82-rack-menu-producer` ([#587](https://github.com/JeffMcClintock/TideSynth/pull/587)) and `tide/win/E19-datatype-census` ([#590](https://github.com/JeffMcClintock/TideSynth/pull/590)), byte-identical on each. **If all three were closed unmerged this entry would be lost with them** — the branch names are in the NEXT cell, which is the mitigation STEP 4 already relies on.

## 2026-09-19 — macos — STEP 1.5 for the fourth time on the same three PRs: A38's livelock, confirmed to recur across a run boundary (scheduled run)

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.2553.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** re-verified `mergeStateStatus` on this platform's three open PRs rather than trusting any prior cell's word for it — the 09-18 mac cell had already left them `MERGEABLE`, but `main` moved twice since (`#592` then `#593`, the latter filing **A38** for exactly this mechanism), and all three had gone `CONFLICTING`/`DIRTY` again. [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36), [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72), [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) — resolved all three with the same recipe the 09-18 cells used, and all three are now `MERGEABLE` again.

### STEP 1 / STEP 1.5

`platform:mac` issues empty (checked `TideSynth`, `SynthEditLib`, `GMPI_Wrappers`, `gmpi_ui`, `SynthEdit_Rack_Adaptor`). Screen locked (`CGSSessionScreenIsLocked` present). All three PRs confirmed `mergeStateStatus: DIRTY` / `mergeable: CONFLICTING` before touching anything — not assumed from the 09-18 entries' final state, which described a *different* moment.

### The resolution, and one new wrinkle in the recipe

Same two hot files as every prior cell — `BACKLOG.md`'s mac/win NEXT rows (adjacent lines in one table) and, for A36 only, `JOURNAL.md`. **New this run: the mac NEXT row in each branch's pre-merge `BACKLOG.md` was itself corrupted** — `| mac | | mac | **RE-POINTED 2026-09-18...**`, a duplicated `| mac | ` prefix, 3.5 KB longer than `origin/main`'s clean version of the same cell, with a divergent older tail beneath the shared "sixth confirming cell" text (09-10 vs 09-09 history). This is very likely an artifact of an earlier resolution pass concatenating instead of replacing. **Did not try to reconcile the divergent tail** — the mac NEXT cell is a rolling summary, not the record of truth (`JOURNAL.md` is), so for both the `win` and `mac` NEXT rows this run took `origin/main`'s version wholesale rather than hand-splice a malformed duplicate. Verified this loses nothing: `JOURNAL.md`'s own union-merge (line-set diff against both `origin/main` and each branch's pre-merge state, `JOURNAL.md` **plus its archives** for #585 since A36 is a rotation) came back at **zero missing lines** on all three branches. `docs/lessons.md` was never hand-merged, only regenerated via `extract-lessons.py --write` on the resolved `JOURNAL.md`.

`check-next-block.py`, `check-id-refs.py`, `check-backlog-archived.py`, `check-links.py` all clean on all three post-merge. `check-commit-completeness.py --record`/`--verify` around each commit. `check-commit-authorship.py --repo .` clean on all three **after** a `--reset-author` amend on each — the first commit attempt on every branch landed as `Jeff McClintock`, because the `GIT_AUTHOR_*`/`GIT_COMMITTER_*` exports from STEP 0.7 do not persist into a later, separate shell invocation in this harness. Caught before pushing, each time, by running the authorship check as its own step rather than assuming the STEP 0.7 exports were still live.

Pushed all three to their existing branches (STEP 1.5: fix in place, no new PRs). Re-checked `mergeStateStatus` after each push: all three `MERGEABLE` again, `UNSTABLE` only because the self-hosted `macos`/`windows` compile legs were still queued or in progress — nothing red on any of the three.

### What this confirms about A38

**The livelock recurred across a run boundary, not just within one run.** The 09-18 windows cell showed it happening *twice in one run*; this run shows it surviving from one scheduled run to the next with no code change in between — `#593` (which only filed A38 and fixed the `win` lane) was enough on its own to re-conflict all three `mac` PRs a full day later. That is exactly A38's prediction: with N open PRs touching the same two files, every merge to `main` cascades to N−1 re-resolutions, and nothing about elapsed time between runs changes that. **Did not implement A38's per-lane-file proposal** — it changes STEP 4 of the shared prompt and A38's own row says it wants Jeff, not a run.

### What I did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (`tide/linux/E79-clap-headless-document`)**, still `CONFLICTING` per the 09-18 windows entry and unconfirmed further by this run — STEP 1.5 is scoped to `tide/{PLATFORM}/**` and STEP 2 treats another platform's branch as taken.
- **Did not re-walk STEP 2's backlog beyond re-confirming STEP 1** — the queue was already established empty across six prior mac cells (09-07 through 09-18), and resolving three re-conflicted PRs is itself STEP 1.5 work that outranks a fresh walk.
- **Did not rotate `JOURNAL.md`** — still blocked on #585 (A36) actually merging, which is still open.

**Learned:**

- **Exported `GIT_AUTHOR_*`/`GIT_COMMITTER_*` environment variables do not survive into a later, separate tool invocation in this harness — each shell call is a fresh environment.** Re-export them immediately before every `git commit`, not once at the top of the run. Caught by running `check-commit-authorship.py` as an unconditional post-commit step on every branch, which is what the prompt already asks for — the discipline earns its keep here specifically.
- **An A38-shaped conflict can leave a NEXT cell corrupted (duplicated row prefix, divergent stale tail) rather than cleanly resolved, if a resolution pass concatenates a conflict side instead of choosing one.** When a NEXT cell's pre-merge content looks structurally wrong (a repeated `| <platform> |` prefix, a length far outside the platform's recent history), prefer `origin/main`'s clean version over trying to preserve the malformed branch-side content — the cell is a summary; `JOURNAL.md` is the record, and its own union-merge check is what actually proves nothing was lost.
- **A38's mechanism is confirmed to operate across run boundaries, not just within a single run's multiple pushes.** One intervening merge (`#593`, itself just a livelock-fix + filing, no product change) was sufficient to undo a full day-old resolution.

**Not verified:** the self-hosted `macos`/`windows` compile legs on all three re-pushed PRs — still queued/in-progress when this entry was written; whoever reads this next should check `gh pr checks` rather than trust "nothing red yet".

**Machine state.** All six repos started and ended clean on their default branches except `TideSynth`. No host was launched, no plug-in built or installed. Screen was locked throughout. Work done in `git worktree`s under the session scratchpad; the main checkout was never switched off `origin/main`'s detached state used for reading.

**Next:** same as the 09-18 mac and windows cells said — merging **#585 (A36) first** unblocks `JOURNAL.md` rotation and is the highest-value single action available; every day these stay open costs another box another run repeating this exact recipe. The next run of any platform touching `BACKLOG.md`/`JOURNAL.md` should expect to re-run this recipe again unless a merge sweep has happened first, and should check for NEXT-cell corruption (a duplicated `| <platform> |` prefix) rather than assume a stale-looking cell is intact.

**Branch/PR:** `tide/mac/2026-09-19-step15-conflicts` — this entry and the refreshed `mac` NEXT cell only. The three fixes are on `tide/mac/A36-journal-rotation-rule`, `tide/mac/E72-cable-dsp-dirty` and `tide/mac/E81-handle-determinism` themselves (their own pushed merge commits).

## 2026-09-18 — windows — STEP 1.5 twice in one run: the fleet's bookkeeping files are a livelock, and the mechanism is adjacent lines

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **2.110.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no product code changed. STEP 1.5 was the entire run: all three open `tide/win/**` PRs had gone `CONFLICTING`, and I resolved and pushed all three — **then had to resolve and push all three again**, because the macOS box's own STEP 1.5 bookkeeping PR ([#592](https://github.com/JeffMcClintock/TideSynth/pull/592)) merged to `main` while I was working and re-conflicted every branch I had just fixed. Six resolutions, three branches, one run. All three are now `MERGEABLE`. Filed **A38** for the livelock this exposes, which is the part worth more than the three PRs.

### STEP 1 / STEP 1.5

`gh issue list --label platform:win` empty — **and that still verifies nothing**, because `build.yml:523` excludes `matrix.platform != 'win'` from filing platform issues; four generations of this cell have said so and it is still true. Read `main`'s build instead: green at `510947025`.

**Seven PRs were open at run start, and the split is the finding:** `tide/mac/**` ×3 all `MERGEABLE/CLEAN`; `tide/win/**` ×3 and `tide/linux/**` ×1 all `CONFLICTING/DIRTY`. Not one of them was red, reviewed, or commented on — so under STEP 1.5's literal list of three (*failing checks, requested changes, unresolved review comments*) all seven read as *"green and waiting on Jeff"*. **This is the fifth-plus occurrence of that exact misreading across the three boxes** and `mergeStateStatus` is still not in the rule; it is one extra field on a `gh pr view` STEP 1.5 already makes you run.

Resolved, and verified before each commit:

| PR | branch | resolution |
|---|---|---|
| [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) | `tide/win/E80-clap-editor-arm` | 3 branch entries + 1 main entry, then again +1 |
| [#587](https://github.com/JeffMcClintock/TideSynth/pull/587) | `tide/win/E82-rack-menu-producer` | 1 + 1, then again +1 |
| [#590](https://github.com/JeffMcClintock/TideSynth/pull/590) | `tide/win/E19-datatype-census` | 1 + 1, then again +1 |

`JOURNAL.md` by entry-set union, `BACKLOG.md` by NEXT-cell ownership (`win` cell from the branch, `mac` cell from `main`), `docs/lessons.md` regenerated and never merged. Every lint gate run on every resolution: `check-journal-prepend`, `check-backlog-diff`, `check-prompt-provenance`, `check-next-block`, `check-id-refs`, `check-links`, `check-backlog-archived` — all rc=0, all six times. `check-commit-authorship.py` clean on all three branches (12, 6 and 7 commits, all `tide-rack-bot`).

### The mechanism, measured rather than reasoned: **adjacent lines**

**Every conflict in this fleet, in all four conflicting branches, was in exactly the same three files — `BACKLOG.md`, `JOURNAL.md`, `docs/lessons.md` — and never once in code.** `docs/lessons.md` is generated, so it is not a real conflict. That leaves two hot spots, and they fail for the *same* reason:

- **`BACKLOG.md`'s NEXT block is a markdown table whose four rows are adjacent lines.** Git needs at least one unchanged line of context between two sides' changes to merge them as separate hunks. The `win` cell is line 11 and the `mac` cell is line 12, so **a windows run re-pointing only its own cell and a macOS run re-pointing only its own cell collide in one hunk despite touching disjoint content.** A markdown table cannot carry a blank line between rows, so the adjacency is structural, not incidental.
- **Each NEXT cell is a single line.** Measured on `main` today: `mac` **41,216 bytes on one line** (13-deep `Previous cell follows.` chain), `win` 15,406, `linux` 6,331, `any` 4,661. Git's smallest unit is a line, so a 41 KB cell is un-narrowable by construction — there is no such thing as a partial merge of it. This is A37's growth curve seen from the merge side; A37 measured the `mac` cell at 34,988 bytes / 10-deep on 09-10, so it has grown **18% in 8 days**.

**The positive control for the mechanism came free, in the second round.** After #592 landed, `JOURNAL.md` **auto-merged on all three branches** where it had conflicted an hour earlier — because by then the branches' own entries sat *below* main's newest one, leaving the shared 09-17 entry as context between the two sides' insertions. Same file, same two runs, same append-only shape: **context present → clean; context absent → conflict.** That is the whole mechanism, and it says the fix is structural spacing, not better merge discipline.

### The livelock, and why it is not just bad luck

Each run's STEP 4 *requires* editing `JOURNAL.md`'s top and its own NEXT cell. So **every PR the fleet opens touches the same two hot spots, and every merge to `main` invalidates every other open PR.** With N open PRs a single merge costs up to N−1 re-resolutions, each costing a whole agent run — and runs are the scarce resource, not compute.

Today both the macOS box and this one spent their **entire run** on it, and the two runs actively fought: #592 is a 3-file, 50-line bookkeeping commit carrying no product change, and it was sufficient to undo three completed resolutions. I did not lose the work — the re-resolution is mechanical — but **the fleet's whole output for 2026-09-18 across two of three machines is six conflict resolutions and two journal entries.**

Why N grew: **A7** established the 7×/week cadence is deliberate, so the fleet opens ~21 PRs/week across three boxes, while merging happens in human "merge sweeps" (see the 09-07 and 09-08 entries). N is the gap between those rates, and the cost is quadratic in it.

**Filed as A38**, with the concrete proposal that falls out of the mechanism: give each lane its own file (`docs/next/<platform>.md`) so no two platforms ever edit the same line, and do the same for per-run journal entries. That would subsume A37's size problem as a side effect, since each lane's chain would then live in a file only that lane writes. **It changes STEP 4 of the shared prompt, so it is a proposal for Jeff and not something a run should impose** — A38 says so in the row.

### What I did NOT do, deliberately

- **Did not touch [#584](https://github.com/JeffMcClintock/TideSynth/pull/584) (`tide/linux/E79-clap-headless-document`), which is still `CONFLICTING`.** Its conflict is in the same three bookkeeping files and needs no Linux toolchain, so I could have resolved it — but STEP 1.5 is scoped to `tide/{PLATFORM}/**` and STEP 2's collision rule treats another platform's branch as taken. **Flagging it loudly instead: the linux lane is one mechanical resolution from mergeable, and the recipe is in this entry.**
- **Did not rotate `JOURNAL.md`,** now **244,359 bytes / 22 entries** against A24's 60 KB ceiling — the fifth consecutive cell to defer it. A rotation rewrites the bottom of the file and would conflict with all six other open PRs at once, and **A36, which restores the rotation instruction that rotated itself out, is itself sitting unmerged in [#585](https://github.com/JeffMcClintock/TideSynth/pull/585).** The rotation instruction is still **absent from `main`** — confirmed by `grep '^## Rotation' ` on `origin/main:JOURNAL.md`. Rotation wants a moment when the fleet has no open PR, which A38 is about creating.
- **Did not build, did not launch a host, did not take the screen** — see machine state.

**Learned:**

- **A merge conflict between two agent runs is usually an adjacency artifact, not a real disagreement, and the test is one command: whether the two sides' changes have an unchanged line between them.** Measured both ways in one run on the same file — `JOURNAL.md` conflicted while both sides inserted at the very top, and auto-merged an hour later when a shared entry sat between them. Before hand-resolving, check whether the sides are actually disjoint; if they are, the fix belongs in the file's *layout*, not in the resolution.
- **A markdown table is a merge hazard for concurrent writers, because its rows cannot be separated by blank lines.** Any per-actor table where each actor edits its own row will conflict on every concurrent edit, forever. This is why the NEXT block collides even when two platforms touch strictly disjoint cells.
- **`JOURNAL.md` can be merged as a set union keyed by entry heading, and that is provably lossless because the file is append-and-prepend-only** (`check-journal-prepend.py` enforces it). The union must refuse to run when either side has *fewer* entries than the merge base — that is a rotation, and a union would silently resurrect the archived entries. Verified each merge by comparing entry sets three ways (main / branch / merged): zero missing, zero extra, zero altered, order newest-first.
- **Re-check `mergeStateStatus` after every push, and again before you finish — `main` moves under you.** It moved once mid-run here (`3a85dbabc` → `510947025`) and silently undid three pushed resolutions; the PRs still showed green checks throughout. A resolution is not done when it is pushed, only when the PR reads `MERGEABLE` *after* the push.
- **A bookkeeping-only commit is not a cheap commit when other PRs are open.** #592 carried no product change and cost three re-resolutions. The cost of a commit to a hot file scales with the number of open PRs, not with its own size.

**Not verified:**

- **Whether the self-hosted `windows` and `macos` compile legs pass on the three re-pushed branches.** `lint`, `guard`, `verify`, `render-*` and the container `linux` leg are all green on all three; the self-hosted legs were still **queued** when this entry was written (one runner, one queue — the same state the macOS box recorded today). Nothing had gone red. **Whoever reads this next should confirm them rather than assume**, and note the windows runner is this box, which was busy with the developer's own work all run.
- **Whether A38's per-file proposal actually eliminates the conflicts,** as opposed to relocating them. The mechanism above predicts it does for the NEXT block and for per-run journal entries, but nothing was built or measured — A38 is a filing, not a fix, and its row says so.
- **Whether #584 resolves as mechanically as the three win PRs did.** Its conflicted file set is identical, but I did not attempt it and did not test-merge beyond confirming `CONFLICT` on the same three files.

**Machine state — the developer was at the machine all run, and this is the second consecutive windows cell to say so.**
`Get-Process | Where-Object { $_.MainWindowTitle }` at run start: **Visual Studio actively DEBUGGING** (`SynthEditStore (Debugging) - DirectXGfx.h`), **SynthEdit 1.6 running a document** (`EQ Pro104 GRAPH`), plus Chrome and Outlook. Per the 09-09 cell's rule — *an unlocked screen with the developer working at it is a stronger reason to stay off the GUI than a locked one* — **no host was launched, nothing was built, no screenshot taken, and `%APPDATA%` was not touched.** This run needed none of it: every conflict is text, and text merges need no toolchain. That is also why this was a good run to take while the box was busy.
All work was done in **`git worktree`s under the session scratchpad**, so `C:\SE\TideSynth` stayed on `main` and clean from first command to last — it was never checked out to a branch at any point. Worktrees removed at the end. Sibling repos were **not read, not built and not touched** — nothing this run did required going near them. **Recording their dirt anyway, because the 09-09 windows cell's lesson was that a machine-state record which does not is worse than none:** `SynthEditLib` `modules/se_sdk3_hosting/ModuleView.cpp` (+34, mtime 09-17 16:08), `gmpi_ui` `backends/DirectXGfx.cpp` and `backends/DirectXGfx.h` (+144/−16, mtime 09-17 18:22), `GMPI_Wrappers` `wrapper/AU3/AU3_Wrapper.mm` (mtime 09-10). `SE16` and `GMPI` clean; all five on their default branches. **All of it is real content, not CRLF churn** (`git diff --ignore-all-space` is non-empty for each), **all of it predates this run**, and `DirectXGfx.h` is the exact file Visual Studio had open and under the debugger — so this is the developer's live work in progress and was left strictly alone, per STEP 5's third category.

**Next:** **six of seven PRs are now `MERGEABLE`** — #585, #586, #587, #588, #589, #590 — and only #584 (linux) is not. **The single highest-value action available to this project right now is Jeff merging that batch**, because every day they stay open costs another box another full run, and because `JOURNAL.md` rotation (244 KB, five cells deferred) and A36 are both waiting on the fleet being empty. **Merge order matters and is cheap to get right: merge #585 (A36) FIRST** — it restores the rotation instruction that is currently missing from `main` — **then the rest in any order, re-checking `mergeStateStatus` between each**, since each merge will re-conflict the remainder by exactly the mechanism above until A38 is addressed. The next windows run should do STEP 1.5 before anything else and expect it to be the whole run again.

**Branch/PR:** `tide/win/2026-09-18-step15-conflicts` — this entry, the refreshed `win` NEXT cell, and the new A38 row. The three PRs above were fixed in place on their own branches, per STEP 1.5.

## 2026-09-18 — macos — STEP 1.5 was the whole run: all three of this platform's open PRs had gone CONFLICTING, and the whole file-pair merge recipe held

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.110.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** re-verified `mergeStateStatus` on this platform's three open PRs rather than trusting the 09-17 cell's "all green, waiting on Jeff" — this is the documented trap (`docs/lessons.md`: *"a CONFLICTING PR is not 'green and waiting for merge', and STEP 1.5's list of three does not name it"*). All three — [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36), [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72), [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) — had gone `mergeStateStatus: DIRTY` / `mergeable: CONFLICTING` since the 09-17 cell's own commit ([#591](https://github.com/JeffMcClintock/TideSynth/pull/591), the mac NEXT-cell/journal bookkeeping) landed on `main` after they were opened. Confirmed the state was stable, not a transient `UNKNOWN` (checked twice, 5s apart, same result both times).

### The conflict was mechanical, and the same shape on all three

Each branch touched `BACKLOG.md`'s mac NEXT-block row (a per-run linked list: `**RE-POINTED <date> ...** ... **Previous cell follows.** <older cells>`) and `JOURNAL.md`'s newest-first entry list — the two files every scheduled run's STEP 4 edits — and `main` had moved in exactly the same two places via #591. Resolved all three the same way, verified before committing each:

1. **`BACKLOG.md`**: for the conflicting mac row, split both sides at their own first `**Previous cell follows.**` marker. The tail after that marker was byte-identical on both sides in all three cases (verified by direct comparison, not assumed) — so the correct merge is `<newer cell's head> **Previous cell follows.** <older cell's head> <shared tail>`, never a pick-one. #585 chained 09-17 → 09-10 → 09-09-on; #588 chained 09-17 → 09-15 → 09-09-on; #589 chained 09-17 → 09-16 → 09-09-on.
2. **`JOURNAL.md`**: both sides' dated entries kept, newest heading first, then the unchanged shared tail. #585 additionally carries the `## Rotation` section (A36's own fix, moving it above the entries) — kept at the very top, since nothing on `main`'s side conflicts with its *position*, only with what comes after it.
3. **`docs/lessons.md`**: never hand-merged, regenerated via `python3 scripts/extract-lessons.py --write` on the resolved `JOURNAL.md`/archives, per the file's own rule.

### Verification, not assumption

For every merge, before committing: read `origin/main`'s and the branch's own pre-merge `JOURNAL.md`/`BACKLOG.md` as line-sets and confirmed the merged result was missing nothing except the rows each PR *intentionally* changes (e.g. #588's own E72/E83 edits) — zero unexplained missing lines in all three cases. For #585 specifically, also verified across `JOURNAL.md` **plus its archives**, since A36's own commit is a rotation (moves old entries out) — every line from `origin/main`'s unrotated `JOURNAL.md` is present either in the merged `JOURNAL.md` or in `JOURNAL-2026-09.md`/`JOURNAL-2026-08.md`. Ran `check-next-block.py`, `check-id-refs.py` and `check-backlog-archived.py` after each merge (all exit 0), and `check-commit-completeness.py --record`/`--verify` around each commit. `check-commit-authorship.py` confirmed every unpushed commit on all three branches as `tide-rack-bot`.

Pushed all three to their existing branches (STEP 1.5: fix in place, never a second PR). Re-checked `mergeStateStatus` after each push: all three now `MERGEABLE`. Watched CI for several minutes after pushing: `lint`, `guard`, both `render-*` legs and `linux` are green on all three; the self-hosted `macos`/`windows` compile legs were still queued when this entry was written (normal — the fleet's self-hosted runner is one machine and a queue, not a per-PR dedicated one) but nothing had gone red.

### STEP 1 / STEP 2

`platform:mac` issues empty (checked `TideSynth`, `SynthEditLib`, `GMPI_Wrappers`, `gmpi_ui`, `SynthEdit_Rack_Adaptor`). Screen locked (`CGSSessionScreenIsLocked` present). Did not re-walk STEP 2's backlog beyond what the 09-17 cell already established (queue genuinely empty for this platform) — fixing three CONFLICTING PRs is itself the STEP 1.5 work that outranks it, and there is nothing this run's own five-minute CI watch changed about that walk.

**Learned:**

- **`mergeStateStatus` can flip from `MERGEABLE` to `CONFLICTING` purely from an *unrelated* platform's bookkeeping commit landing on `main`** — #591 only touched `BACKLOG.md`'s mac NEXT row and `JOURNAL.md`, the same two files every mac PR's own STEP 4 touches, so three otherwise-unrelated mac PRs all went CONFLICTING from one commit. Worth checking `mergeStateStatus` on every open PR of your own platform whenever you re-point the NEXT cell, not just when STEP 1.5 tells you to.
- **The "split at the shared marker, verify the tail matches" technique generalises across all three PRs without change** — the NEXT-block linked-list shape and the JOURNAL.md append-only shape are both structurally the same problem (two sides each prepended once since a common point), so one script pattern, re-run three times with different line numbers, was enough.
- **Verifying via line-set difference (`origin/main`'s lines minus merged lines`) catches silent loss cheaply** — one direction alone is not enough; checking both the `origin/main` side and the branch's own pre-merge side against the merged result is what makes "nothing lost" a measurement rather than a hope.

**Not verified:** the self-hosted `macos` and `windows` compile legs on all three PRs — still queued as of this entry, so their eventual pass/fail is unknown; whoever merges next should check `gh pr checks` again rather than trusting this entry's "nothing red yet".

**Machine state.** All six repos started and ended clean on their default branches except `TideSynth`. No host was launched, no plug-in built or installed. Screen was locked throughout.

**Next:** whichever of #585, #588 or #589 merges first is now unblocked by the CONFLICTING state; the other two will likely re-conflict against whichever merges first (same `BACKLOG.md`/`JOURNAL.md` shape), and the next run — mac or otherwise — should expect to repeat this recipe rather than be surprised by it. `JOURNAL.md` rotation (A36, #585) still can't land until #585 itself merges.

**Branch/PR:** `tide/mac/2026-09-18-step15-conflict-fix` — this entry and the refreshed `mac` NEXT cell only. The actual fixes are on `tide/mac/A36-journal-rotation-rule`, `tide/mac/E72-cable-dsp-dirty` and `tide/mac/E81-handle-determinism` themselves (their own merge commits, already pushed).


## 2026-09-17 — macos — sixth confirming cell: queue empty, E82 independently re-derived and already claimed, E83's flip already done elsewhere (scheduled run)

**Prompt:** b97bc00 · Sonnet 5, `claude-sonnet-5` · app Claude desktop **2.110.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no code change, in this repo or any sibling. Walked STEP 1 / STEP 1.5 / STEP 2 in full, confirmed the mac queue is genuinely empty, and independently re-derived E82's finding before the collision check caught that windows had already filed it. Re-pointed the mac NEXT cell and wrote this entry; nothing else.

### STEP 1 / STEP 1.5

Screen locked (`CGSSessionScreenIsLocked` present). No open `platform:mac` issue in `TideSynth`, and — since the label is not scoped to one repo and nothing in the prompt says it is — also checked empty in `SynthEditLib`, `GMPI_Wrappers`, `gmpi_ui` and `SynthEdit_Rack_Adaptor`. Three open PRs on `tide/mac/**`: [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36), [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72), [#589](https://github.com/JeffMcClintock/TideSynth/pull/589) (E81) — each checked by GraphQL for `mergeStateStatus` and unresolved review threads, not assumed: all three 15/15 green, `MERGEABLE`, `CLEAN`, zero reviews, zero threads. Left alone, per STEP 1.5's own words.

### STEP 2 — the walk, and why nothing was eligible

In file order: **A35** parked on its own two `PROPOSED:` entries (neither parks anything else); **S8** GATED, `NEEDS-SPEC`; **E19**'s mac AU3 cell wants the unlocked screen; **E2** not takeable by its own row; **E72/E81** — this platform's own, both already claimed on the branches above; **E79** taken by linux's [#584](https://github.com/JeffMcClintock/TideSynth/pull/584); **E80** taken by windows' [#586](https://github.com/JeffMcClintock/TideSynth/pull/586); **E82** taken by windows' [#587](https://github.com/JeffMcClintock/TideSynth/pull/587); **E76** linux in substance; **E84** a `lint.yml` edit the bot's token cannot make; **E20–E23** BLOCKED by the third-party-module ruling. Nothing left in the Release & distribution section either (R1 RESOLVED, R7 WONTFIX). **A run that does nothing is a fine outcome, and this is one.**

### E82, independently — worth recording despite the collision, because it corroborates rather than duplicates

Before running `git ls-remote`/`gh pr list` for the collision check, I had already read the code and reached the same conclusion #587 later turned out to state: `vcv/Scope.cpp` (in the `VCV_Fundamental_gmpi` fetch) declares no `appendContextMenu` at all, so E82's five probe points on the Scope panel were always going to return the rack's own generic menu — that is not evidence that *no* VCV module has a working menu producer. `WTLFO`'s wavetable submenu does: `Wavetable.hpp:318` calls `createIndexSubmenuItem("Wave points", sizeLabels, getter, setter)`, and `SynthEdit_Rack_Adaptor/rack/rack.hpp:2595`'s `isIndexPtr()` is `(target != nullptr || getIndexFn) && !labels.empty()` — true for the lambda form, not just the raw-pointer `createIndexPtrSubmenuItem`. `RackPanelLayout.h:216`'s `collectMenu()` records it into `layout.menu` as `MenuOptionKind::IndexPtr` (labels, `setIndex`, default all populated); everything else WTLFO's own `appendContextMenu` adds (`Initialize/Load/Save wavetable`) is a plain `createMenuItem` action, which `collectMenu()`'s `else { continue; }` branch at line 254 silently drops — so the LFO's menu should be exactly one item, "Wave points", not empty. None of this was run live; the screen was locked, and per [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md)'s macOS section, a scheduled run on a locked session cannot open the plug-in editor to right-click and confirm it. Caught the collision with `git ls-remote --heads origin | grep -i e82` and `gh pr list --search E82` before claiming a branch, so nothing was duplicated — recorded here only because two independent readings landing on the same file:line pair (`Wavetable.hpp:318`, `rack.hpp:2595`) is stronger corroboration than either alone, and is cheap to write down.

### STEP 4 bookkeeping

`main` still shows **E83 `IN-REVIEW`** with its only linked PR ([#581](https://github.com/JeffMcClintock/TideSynth/pull/581)) merged, which STEP 4 would normally have this run flip to DONE. Per the 09-16 cell's own lesson (an IN-REVIEW row whose PR merged may already be archived on an unmerged branch), checked `git show origin/tide/mac/E72-cable-dsp-dirty:BACKLOG-DONE.md` before touching it: **the flip is already there**, dated 2026-09-09, on #588. Did not duplicate it on a fourth branch.

### What changed since the 09-16 cell

`SynthEditLib`'s uncommitted `modules/se_sdk3_hosting/SynthEditCocoaView.mm` (flagged 09-15 and 09-16 as the developer's live work-in-progress, left untouched both times) is gone — the tree is clean at `66b1eeca6` (2026-09-16), so that constraint no longer applies to this box. `main` is green at its current HEAD `13095a395` (run [34438892984](https://github.com/JeffMcClintock/TideSynth/actions/runs/34438892984), 2026-09-10); nothing has landed on `main` since, so no build to re-verify.

**Learned:**

- **The `platform:X` issue-label check is worth running across every repo the fleet touches, not just this one** — costs four extra `gh issue list` calls and would have caught a break filed against a sibling repo that STEP 1 as literally read (TideSynth only) would miss.
- **A row's "no producer" finding can be an artifact of which module got tested, not a property of the mechanism.** E82 tested the one module (Scope) that structurally cannot have a menu; the adaptor's own recorder (`RackPanelLayout.h`) does capture a real candidate (WTLFO's index submenu) from a different module in the same fixture. Read the *mechanism* the row's Accept depends on before trusting a negative result drawn from one instance of it.
- **Check unmerged sibling branches' own `BACKLOG-DONE.md` before flipping an `IN-REVIEW` row on `main`** — with several open PRs in flight at once, a STEP 4 obligation that looks outstanding from `origin/main` alone may already be done on a branch that just hasn't merged yet. `git show origin/tide/<platform>/<branch>:BACKLOG-DONE.md` costs one command.

**Not verified:** **Whether the LFO panel's "Wave points" menu actually appears and toggles on a right-click** — the reading above is static (source + the adaptor's recording logic), not a live measurement; the screen was locked all run, and per `docs/ci/headless-gui-verification.md` a locked macOS session cannot open the plug-in editor at all, so this wants an unlocked screen on any platform, not just a fix. **Whether `SHASR`'s `createRangeItem` menu (a custom range-slider widget, not `Index`/`BoolPtr`) is silently dropped by `collectMenu()`'s `else { continue; }`** — read the shape but did not trace whether `createRangeItem` returns something `isIndexPtr()`/`isBoolPtr()` would recognise; not needed for E82 since WTLFO already gives a positive candidate, but worth knowing if SHASR's own menu is ever the one under test.

**Machine state.** All six repos started and ended clean on their default branches except `TideSynth`, which carries this run's own commit on `tide/mac/2026-09-17-queue-blocked`. `SE16`'s untracked `UnitTest/Manual Tests/project_specific_resources.resources/samples/` folder (noted by prior mac cells) was not touched. No host was launched, no plug-in was built or installed, nothing outside `TideSynth` was read for anything other than static source inspection (`SynthEditLib`, `SynthEdit_Rack_Adaptor`, and the cached `VCV_Fundamental_gmpi` build dependency, all via local clean checkouts, none edited).

**Next:** whichever of #585 (A36), #588 (E72) or #589 (E81) merges first unblocks the most — #585 also unblocks `JOURNAL.md` rotation, which has now been deferred five cells running (09-09 through today) because the rotation rule itself (A36) is what's sitting in the open PR. The next mac run should check `mergeStateStatus` on all three again before re-walking a queue that this cell already found empty — nothing here changes until one of them lands.

**Branch/PR:** `tide/mac/2026-09-17-queue-blocked` — this journal entry and the refreshed `mac` NEXT cell only.

## 2026-09-16 — windows — E19's `string` clause, measured: the wire carries four datatypes and none of them is a string (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.52386.0.0** · as **tide-rack-bot** (both paths) · scheduled run

**Did:** took **E19** — the topmost eligible row, after the `win` NEXT cell's own pick (E80, editor arm) turned out to be done and sitting in an open PR — and closed the one clause of it that has never had an artifact behind it. Row back to TODO; **no product code changed**, nothing built, no host launched, no window created. The instrument is [tests/e19_datatype_census_probe.cpp](tests/e19_datatype_census_probe.cpp) plus a negative control and a shim header.

### Why this clause and not the other two

E19's win/VST3 cell owes two clauses: a rack-canvas pixel diff, and `int/bool/enum` verified by right-clicking a VCV panel. **Both need an editor that has painted, and the developer was at the machine** — three Visual Studio instances (`SynthEditStore`, `SimulatorGmpi`, `SynthEdit_cmake` on `PolyphonyControl.xml`), Outlook and Slack, at 11:26 local. That is the second Windows run in a row to find him working; the 2026-09-13 E82 run left its remaining arm named for exactly this reason.

The off-screen-parent-HWND rung does not rescue them: **an invisible window gets no `WM_PAINT`**, so anything that depends on the editor having drawn is out of reach by construction. They want an idle box.

What was reachable is the rung below that one: **before assuming a question needs a window, ask whether the data the GUI would show can be computed.** E82 did that for the right-click menu with `readPanelLayout()`. This run does it for the datatypes with `generatePluginXml()`.

### The measurement

`rack_adaptor::generatePluginXml()` — `SynthEdit_Rack_Adaptor/RackAdaptor.h:187` — is the **single** place a rack module's pins and parameters come into existence. `RackAutoRegister.h:127` calls it and nothing else does; what it returns is what the host is handed. So the set of datatypes this path can carry **is** the set of `datatype` attributes it emits, and the probe calls it for all 39 compiled-in models with the same four `RegistrationOptions` the real registration fills in.

```bash
bash tests/e19_datatype_census_probe.sh            # rc=0, 41 TUs, ~1 min
bash tests/e19_datatype_census_probe.sh '' '' '' --xml Scope   # one module's XML
```

**39 of 39 models linked.**

| section | datatypes emitted, summed over 39 models |
|---|---|
| `<Parameters>` | **blob=7 bool=11 enum=5 float=374** |
| `<GUI>` pins naming a type | blob=7 float=197 |
| `<Audio>` in | float=174 |
| `<Audio>` out, public | float=182 |
| **`<Audio>` out, `private` — the DSP→GUI path** | **blob=7 float=197** |
| **`string`, anywhere** | **0** |

A "producer" for E19's purposes is a pin the DSP writes and the editor reads, and in the emitted XML that is exactly an `<Audio>` pin with `direction="out"` and `private="true"` (`RackAdaptor.h:374-385`). The probe counts those separately, so **"no string producer" is a count of a named thing rather than a failure to find one**.

### Two things the census says that the row does not

1. **The DSP→GUI path carries exactly TWO datatypes** — float (197 lights) and blob (7 display-state frames). **bool and enum are 0 on it.** A menu option travels GUI→DSP: the editor writes `menuIntPins`/`menuBoolPins` and the processor reads them. E19's *"menu options are the int/bool parameters this path carries"* is directionally the other way round — which is precisely why its own verification (*toggle an option, see the DSP obey*) is the right test for that clause and why a feedback counter would not be.
2. **`int` is not a word the wire uses.** An `IndexPtr` option is emitted as datatype `enum`; the *editor* holds it in a `Pin<int32_t>` (`RackEditor.h:1184`). So the row's "int/bool/enum" is two datatypes on the wire, not three.

**The 7 modules carrying display state:** ADSR, Octave, Quantizer, Scope, Sum, VCA-1, Viz.

**Independent agreement with E82:** the modules with a bool or enum parameter number **12 of 39** — the same 12 E82's probe found, reached from a different call (`generatePluginXml` vs `readPanelLayout`). Two probes, two entry points, one answer.

### The negative control is what makes "no string" a statement about the channel

A census is a statement about today's sample. [tests/e19_string_member_negative_control.cpp](tests/e19_string_member_negative_control.cpp) turns it into one about the channel: it declares `RACK_DISPLAY_STATE` over a `std::string` member and **must fail to compile**.

```
  trivially-copyable member only : COMPILES (expected)
  plus a std::string member      : REFUSED at compile time (expected)
```

The refusal is `RackDisplayState.h:94`, whose static_assert names `std::string` in its own message. The first arm is what makes the second attributable — without it, a file that failed for a typo would read as evidence. The display-state blob is the only pin on this path carrying arbitrary bytes, so a string cannot be added there without that guard being relaxed first, and if it ever is, this probe goes red on the same run.

### The probe's own control caught it measuring the wrong file, and this is the part worth carrying forward

**The first run reported `blob=0` for all 39 models** — and went **RED** rather than printing a clean-looking census, because its control requires the four datatypes E19 has already measured working to be present.

The cause: it compiled `modules/X/vcv/X.cpp`, which is what `tests/e82_rack_menu_probe.cpp` compiles — correctly, for *its* question. But TIDE builds `modules/X/X.cpp`, the **port** (`static_library/CMakeLists.txt:67`), and `RACK_DISPLAY_STATE` is declared in the port and **never** in upstream's file. Every port is three lines: the umbrella include, upstream's `.cpp`, and optionally the display-state declaration.

[tests/e19-shim/RackModule.h](tests/e19-shim/RackModule.h) is the fix — a header that shadows the adaptor's by sitting first on the include path and gives a port the Rack mock and `RackDisplayState.h` and nothing else. The real `RackModule.h` pulls the GMPI **and** gmpi_ui SDKs plus two CMake-generated headers, i.e. the configured build tree this probe exists to not need.

**The general lesson: when you reuse another probe's build recipe, check it compiles the same FILE your question is about.** The recipe was right and the file was wrong, and the two are easy to conflate because the wrong file compiles perfectly and answers confidently.

### Traps, both Windows-specific

1. **Git Bash ships a coreutils link at `/usr/bin/link.exe`**, which shadows MSVC's linker whenever bash is launched from a VS developer `cmd` rather than the other way round. **Spelling it `link.exe` does not dodge it** — the coreutils one *is* a `.exe`. It answers `link: unknown option -- n`, which reads as a bad flag rather than as the wrong program entirely. Link through `cl` (`cl -nologo ./*.obj -Fe:out.exe`), which has no twin.
2. **A quoted bash heredoc through this harness mangles apostrophes.** Two attempts to write a file with `<<'EOF'` died on `unexpected EOF while looking for matching '`, both times on content containing `'`. Writing the script to a file with the editor tool and running it works. This is the same family as the `/tmp` and `cmd //c` entries already in the Windows notes.

Also, unrelated to the probe but worth the next run knowing: **a stray `|` inside a BACKLOG cell can create a phantom row.** `check-backlog-diff.py`'s `ROW` regex needs four ` | `-separated columns, and the `win` NEXT cell already carries one bare pipe; adding a second made the three-column NEXT row parse as a four-column backlog row named `win`, reported as `1 new row(s): win`. The check still said OK, so this is a near-miss rather than a failure — but a third pipe in that cell is a trap waiting.

**Learned:**

- **Before assuming a question needs a window, ask whether the data the GUI would show can be COMPUTED.** E82 answered the right-click menu with `readPanelLayout()`; this run answered every datatype with `generatePluginXml()`. Both took about a minute, needed no host, no screen and no build tree, and both were reachable on a box the developer was working at. The rung above them — an editor in an invisible off-screen parent HWND — cannot reach anything that depends on painting, because an invisible window gets no `WM_PAINT`.
- **When you reuse another probe's build recipe, check it compiles the same FILE your question is about.** This probe borrowed E82's recipe, which compiles `modules/X/vcv/X.cpp`, and reported `blob=0` for all 39 models — because `RACK_DISPLAY_STATE` is declared in the PORT, `modules/X/X.cpp`, which is what `static_library/CMakeLists.txt:67` actually builds. The recipe was right and the file was wrong, and the wrong file compiles perfectly and answers confidently.
- **A probe whose green condition is only "it ran" will publish a clean-looking census of the wrong thing.** This one requires the four datatypes E19 has already measured working in a host to be PRESENT before it reports on the fifth, so a zero for `string` is an absence inside data that demonstrably contains other things. That control is what turned the wrong-file run red instead of shippable, and it cost four lines.
- **An absence is a statement about the sample until you make it a statement about the channel.** The census says no module offers a string today. The compile-time negative control — `RACK_DISPLAY_STATE` over a `std::string`, which must NOT compile, beside the same declaration without it, which must — says the one byte-carrying pin on this path could not be given one. The second is the one that stops this being re-derived in a month.
- **Read the direction, not just the datatype.** The DSP→GUI path carries float and blob and nothing else; `bool` and `enum` are 0 on it, because a menu option travels the other way. E19 has described them as datatypes "this path carries" since 2026-08-25, and that wording is what makes its own (correct) verification for them look like an odd choice.
- **Git Bash ships a coreutils link at `/usr/bin/link.exe`** that shadows MSVC's linker whenever bash is launched from a VS developer `cmd` rather than the other way round — and **spelling it `link.exe` does not dodge it**, because the coreutils one is a `.exe` too. It answers `link: unknown option -- n`, which reads as a bad flag rather than as the wrong program. Link through `cl`, which has no twin.
- **A stray `|` inside a BACKLOG cell can create a phantom row.** `check-backlog-diff.py`'s `ROW` regex wants four ` | `-separated columns; the three-column `win` NEXT cell already carries one bare pipe, and adding a second made it parse as a backlog row named `win` (`1 new row(s): win`). It still exited 0, so this is a near-miss — but a third pipe in that cell is a trap already loaded.

**Not verified:** **the two clauses E19's win/VST3 cell actually owes** — the rack-canvas pixel diff and the `int/bool/enum` toggle. Both need a painted editor and an idle box; neither was attempted. **Anything about routing, painting or hosting** — no editor was created, nothing was clicked, no rack was hosted. This probe reads what the factory DECLARES, which is upstream of all three. **That the declared pins behave as declared** — a pin can be emitted correctly and still not carry its value, which is exactly what E80 is about. **macOS and Linux** — the script takes both paths and was compiled only on Windows. **`SynthEditCL`, SynthEdit and TIDE** — not built. This branch touches no product code, so their state is unchanged rather than re-verified, and I cannot say from this run that they build. **E82's probe cross-check** — the "same 12 of 39" agreement was read off that PR's published table, not by re-running it here.

**Machine state.** `TideSynth`, `SE16`, `SynthEditLib` and `gmpi_ui` were all clean and on their default branches at the start of the run. **`GMPI_Wrappers` carries a modified `wrapper/AU3/AU3_Wrapper.mm` that predates this run** — the developer's work in progress, left exactly as found and deliberately not touched. **`GMPI` is dirty too and also predates this run** -- `Hosting/xml_spec_reader.{cpp,h}`, real content changes rather than line-ending churn, mtime 10:52 today against a run that started at 11:26. This run only READ that repo (two headers on two `-I` flags), which has never needed permission, and wrote nothing to it. No repo but `TideSynth` was written to, and `git add` named four paths, never `-A`. `%APPDATA%` was not touched, no host was launched, no plug-in was installed, and the `MainWindowTitle` process list is unchanged across the run. Source shas the measurement was read against: `SynthEdit_Rack_Adaptor` and `VCV_Fundamental_gmpi` as checked out beside this repo; `GMPI` supplied `Core/Processor.h` and `Extensions/PinConnection.h` on two `-I` flags and was not modified — reading it has never needed permission. Object files went to a `mktemp -d` deleted on exit. `main`'s `build` is green on all three platforms at `0ed6ca1db`; the shas since are docs-only with no `build` run, which is `guard` working.

**Next:** **E19's remaining win clauses and E82's remaining arm are the same session** — the same fixture (`tests/fixtures/e75-vcv-visible-rack.xml`), the same right-click, and both want an idle box. Whoever gets one should do them together: right-click **WT LFO**, confirm *Wave points* lists its 10 labels, pick one, show the DSP obey, and take the rack-canvas pixel pair while the editor is up. **Nothing else in this lane's queue is takeable** — A35 is parked on its own two `PROPOSED:` entries, S8 is `NEEDS-SPEC`, E2 is an umbrella its own row calls not takeable, E72/E79/E80/E81/E82 each have an open PR, E76 and E79 are linux in substance, and E84 is a `.github/workflows/**` edit the bot's token deliberately cannot make. **`JOURNAL.md` is ~229 KB against A24's 60 KB ceiling and rotation is deferred for the fifth time, with a changed reason:** [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) **is A36, the rotation rule itself**, and five other PRs are open beside it — rotating from this lane would conflict all six on the hardest file to resolve. Whoever merges last, once A36 has landed, should rotate. **`build.yml`'s `matrix.platform != 'win'` exclusion (`:523`) still means STEP 1 cannot fire on this platform** — a workflow edit the bot cannot make, so it is Jeff's or nobody's, and the `win` cell has now restated it six times.

**Branch** `tide/win/E19-datatype-census`.
## 2026-09-16 — macos — E81: the handle is random BECAUSE the parameter is saved, and the ruling nobody had asked for is now asked (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.52386.6** · as **tide-rack-bot** (both paths) · scheduled run

**Did:** took **E81** and did the half of it that is not GATED — **its own `Scope` says *"the ruling first"***, and after eleven days no `PROPOSED:` entry existed. Filed it, and turned the row's open question into a measurement. Row **IN-REVIEW** on `tide/mac/E81-handle-determinism`; **no product code changed, and none may be** — the fix is in `SynthEditLib` and is not a build break, so STEP 5's exception does not reach it.

### The answer to the question E81 left open, and it is not a collision

E81 measured the consequence (ten loads of the same rack → ten distinct documents) and said the cause was *"the first thing to find out"*. It is one line:

```
EditorLib/PatchManager.cpp:1304   (CPatchManager::GetHostGeneratedParameter)
    Container()->Document()->uniqueIdDatabase.setHandleAutoGenerated(p, !stateful);
```

`stateful` is a local of that function, set true in the switch above for exactly those host controls **whose value is written into the document**. So `!stateful` routes every host control that serializes to the RANDOM branch and every host control that does not to the sequential one.

**That is inverted with respect to the sequential branch's own stated purpose** (`UniqueSnowflake.cpp:146`): *"useful for Host Controlled Parameters which get created during project load. Using the same ID every time ensures resulting DSP XML is consistant and comparable each run."* The parameters that reach the DSP XML are the stateful ones. They are the ones excluded.

**Why this distinction is load-bearing rather than pedantic: E56 fixed a COLLISION and this is not one.** E56's allocator iterated an `unordered_map` expecting sorted order and fell into the random fallback — a bug, fixed, and its measurement stands. This is the call site choosing the random branch deliberately, every time, by construction. A reader who assumes "same area, same bug" will look for a fall-through that is not there.

### The control was free, in the tree, and nobody had spent it

[tests/e81_handle_branch_probe.py](tests/e81_handle_branch_probe.py), two independent halves: a source walk (needs `--syntheditlib`, read-only) and the committed documents (needs nothing but this repo). Against `SynthEditLib` at `origin/main`, **19 host controls assign `stateful` explicitly: 16 true → RANDOM, 3 false → sequential.**

| HC | name | `stateful` | branch | in the committed documents |
|---|---|---|---|---|
| 14 | `HC_VOICE_ALLOCATION_MODE` | true | RANDOM | 2 distinct values / 2 docs |
| 21 | `HC_POLYPHONY` | true | RANDOM | 2 distinct values / 2 docs |
| 22 | `HC_POLYPHONY_VOICE_RESERVE` | true | RANDOM | 2 distinct values / 2 docs |
| 40 | `HC_PORTAMENTO` | true | RANDOM | 2 distinct values / 2 docs |
| 49 | `HC_PATCH_CABLES` | true | RANDOM | 2 distinct values / 2 docs |
| 59 | `HC_PROCESSOR_OFFLINE` | **false** | **sequential** | **`Handle="0"`, in all three** |

**The last row is the control and it cost nothing.** `HC_PROCESSOR_OFFLINE` is in the *same three documents*, written by the *same load*, and carries handle **0** — the smallest free key, which is what the sequential branch returns. One predicate apart; six orders of magnitude apart in the output. **So the branch is legible in the committed bytes and does not have to be taken on the source's word** — which is the shape this lane keeps finding pays (E83, E75, E71, E77).

**E81's ten-loads result is also recoverable from the repo at zero cost, so nobody needs to rebuild a CLAP host to see it again:** `e5-rack-macos-2026-08-21.xml` and `e53-vcv-rack-segv.xml` are two INDEPENDENT saves and disagree on all five handles; `e53`/`e75`/`e83` are edits of one another rather than separate saves and agree exactly. Independence is the variable, and the fixtures already carry both arms.

### E81's three citations are exact — which is worth saying, because this lane's last four runs found prose that was not

Checked against `origin/main`, not the working tree: `UniqueSnowflake.cpp:176` is `key = random_generator() & 0x7fffffff` in the non-temporary branch; `:133` is `random_generator.seed((unsigned int)time(nullptr))` under `#else` (Release); `:143` is the old-Banks warning. All three as the row states them. **The one correction is not to E81 but to a stale reading of its neighbour:** `PatchManager.cpp`'s `setHandleAutoGenerated` call is at **`:1304` on `origin/main`** and at `:1293` in this box's `SynthEditLib` working tree, which is **[behind 18]**. Cite the ref, not the checkout.

### A trap the probe hit and that anyone reading this file will hit

`PatchManager.cpp:822` carries a **commented-out** `//	case HC_MAX_LATENCY_COMPENSATION:` directly above a live case group. The first version of the probe matched it and attributed that group's `stateful` to a host control **that is not in the `HostControls` enum at all**. `strip_comments()` now blanks `//` and `/* */` while preserving line numbers, and an unknown case label is a hard exit 1 rather than a printed `-1`. **This is the same defect class the 09-15 run found three times in prose** (a citation inside an `#if 0`, a site count of 23 that measures 15) — it reproduces just as easily in a script written to replace reading with measuring.

### The ruling, and why the recommendation is (c) rather than the obvious (b)

Filed in [docs/decisions.md](docs/decisions.md). (a) leave it; (b) give host controls the sequential branch regardless of `stateful`; (c) derive the handle from `hostControlId_` in a reserved range.

**(b) is the obvious answer and may not be sufficient, which is the whole reason the entry is worth Jeff's time.** E56's fix made the sequential branch return *the smallest **free** non-negative key* — so what it returns depends on what is already registered, and it is deterministic **only if host-control creation order is**. (c) does not have to care. **Nobody has verified creation order, including this run**, and if it is stable then (b) is the smaller change and the better answer.

**Also not established, and E81's row says the ruling turns on it:** whether the `:143` old-Banks hazard (*"if user deletes then adds parameter, new parameter will have old one's ID"*) can reach a host control at all, given host controls are created by the load itself, carry `isPrivate = true`, and are identified in the document by `HostControl=`. Stated as open in the entry rather than argued either way.

### The thing this run found by accident, and it is worth more than the row it was filing

**Two BACKLOG ids are each allocated TWICE, to different findings, on two unmerged branches.** Found by STEP 3's grep-before-filing rule while trying to claim one of them for something else — **filed as E87**.

| id | `tide/linux/E79-clap-headless-document` ([#584](https://github.com/JeffMcClintock/TideSynth/pull/584)) | `tide/win/E80-clap-editor-arm` ([#586](https://github.com/JeffMcClintock/TideSynth/pull/586)) |
|---|---|---|
| first | 2026-09-09, E79's ordering defect asked of every other wrapper | 2026-09-10, `clap_plugin_gui.show()` |
| second | 2026-09-09, triage of [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) | 2026-09-11, a VST3 component instantiated without its controller |

**Four distinct findings, two ids.** `check-id-refs.py`'s duplicate check is blind to this **by construction** — the rows never meet on one branch until a merge puts them there, `main` shows a highest E-id of **84**, and both branches are perfectly consistent in isolation. Whoever merges second lands a doubled id and a red lint on a branch whose author did nothing wrong.

**The general form is the part to keep: STEP 3's grep is against `origin/main`, and `origin/main` cannot show an id a concurrent branch has taken.** With five PRs open at once this is not a rare race; it has now happened twice. Sweep every `tide/*` branch's BACKLOG for its highest id before claiming one — one command, and it is what turned this from a third collision into a filed row.

**A markup detail that cost a red check and is worth inheriting:** naming those ids in **bold** makes `check-id-refs.py` treat them as citations of rows that do not exist on this branch — 4 STALE, rc=1. Backticks are stripped as code spans, and are the honest markup anyway for an id *token* rather than a row citation. The script suggests `--allow-id`, which is **unreachable from CI**: `lint.yml` invokes it with no arguments.

**Also observed, not filed — `build.yml`'s header is stale in TWO ways, and the second one has a live decision resting on it.**

(1) It still says the matrix *"does not run yet"* and is *"EXPECTED TO FAIL until BACKLOG C7 is done"*. **C7 closed 2026-08-21 and B1 on 2026-08-25**; the guard now passes and this run's push-event build ([run 34979795007](https://github.com/JeffMcClintock/TideSynth/actions/runs/34979795007), head `2dd0eb4`) went **completed/success with all seven jobs green — `guard`, three `render-*`, `linux`, `macos` AND `windows`**. A clean-clone cross-platform matrix passing is the very thing that header calls the open-source litmus test, and the header says it cannot happen yet.

(2) **The timing figures it reasons from are wrong by an order of magnitude, and BACKLOG S30 depends on them.** The header says *"macOS builds take ~60 minutes against linux 5 and windows 10"* and S30 set `cancel-in-progress: false` on the strength of it. Measured on that run: **`macos` 3m22s, `linux` 1m56s, `windows` 10m52s** — macOS is now the **fastest** of the three builds, not twelve times the slowest. **Whatever S30 concluded, it should be re-derived rather than re-quoted**; this run did not re-open it.

Left as observations rather than rows because both are `.github/workflows/**` comment edits **the bot's token cannot make** — the same wall E84 sits behind. **Do not read either as a defect in the build**; the build is green.

### Process

**STEP 1 empty** — no open `platform:mac` issue (the only open issues are #583, `platform:linux`, and the watchdog digest #44). **STEP 1.5 resolved to *leave it alone*, not to *empty*:** both mac PRs — [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) (A36) and [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) (E72) — are **15/15 green, `MERGEABLE`, `CLEAN`, no reviews, no comments**. Waiting on Jeff, so STEP 1.5's own words applied and neither was touched.

**THE SCREEN WAS LOCKED** (`ioreg -n Root -d1 -a` → `CGSSessionScreenIsLocked` PRESENT; its ABSENCE means unlocked, which is the cheaper reading) — the fifth mac cell running. So **E19**'s mac AU3 cell and its pixel-diff clause were out again. **E81 needed none of it**, which is the Accept/question split paying a fifth time: the *question* ("why is it not on the sequential branch?") was answerable from the source and the committed fixtures in one session.

**The rest of the walk, in file order:** **A35** parked on its own two `PROPOSED:` entries; **S8** GATED and `NEEDS-SPEC`; **E19** GUI-blocked; **E2** not takeable by its own row; **E72**, **E79**, **E80**, **E82** all have open PRs and are taken; **E76** is linux in substance; **E84** is a `lint.yml` edit **the bot's token deliberately cannot make** — it needs Jeff or an interactive session, and has now been unowned for a week.

**STEP 5 NOTE, unchanged from the 09-15 run and it constrained this one the same way:** `SynthEditLib`'s tree still carries Jeff's uncommitted work on `modules/se_sdk3_hosting/SynthEditCocoaView.mm` — **27 insertions, real content**, not CRLF churn by `git diff --ignore-all-space`, mtime 09-14 and byte-identical to what 09-15 saw. Untouched, per the third kind of dirt. Every commit here is **TideSynth-only**; `SynthEditLib` was read via `git archive origin/main` into a scratch dir, never through the dirty checkout.

**HOUSEKEEPING, STILL UNOWNED AND STILL SHUT:** `JOURNAL.md` is **~229 KB against A24's 60 KB target**. The 09-09 cell said the window opens after #581 merges — it did — and the 09-15 cell said #585 then closed it again, because **#585 IS the rotation rule (A36)**. It is still open, so the window is still shut, and rotating on a third branch would conflict against the one PR that changes how rotation works. **Merge #585, then rotate to ITS rule.**

**MERGE-ORDER NOTE, expected rather than a defect:** this branch was cut off `main`, whose newest `mac` cell is 2026-09-09, so it does **not** carry #588's 2026-09-15 cell. **Whoever merges second must keep BOTH cells, in run order** — the chain is a linked list and a merge truncates it silently; `re.findall(r'RE-POINTED (\d{4}-\d{2}-\d{2})', cell)` before and after is the one-line tell, and `check-next-block.py` is rc=0 either way.

**A STEP 4 obligation that looks outstanding on `main` and is not:** `main` shows **E83 `IN-REVIEW`** with [#581](https://github.com/JeffMcClintock/TideSynth/pull/581) merged, which STEP 4 says to flip. **#588 already did it** — E83 is archived to `BACKLOG-DONE.md` dated 2026-09-09 on `tide/mac/E72-cable-dsp-dirty`, invisible from `main` until that merges. Flipping it here would conflict against #588 for nothing. **The general form, and it will recur while five PRs are open: an IN-REVIEW row whose PR has merged may already be archived on an UNMERGED branch — check `git show origin/tide/*:BACKLOG-DONE.md` before flipping one.**

**Verification artifact:** `./tests/e81_handle_branch_probe.py --syntheditlib <SynthEditLib>` — exit **0**, the truth table above, source half and document half agreeing. It exits **1** if the coupling ever moves, which makes it the regression guard for whichever way E81 is ruled; and it **describes today's behaviour only**, so it is identical under every answer and takes no position on the open question. Lint green locally: `check-links` 673/0 broken, `check-id-refs` clean, `check-next-block` rc=0.
## 2026-09-16 — windows — E80: the blob had nowhere to go — the fixture's Scope carries no patch parameters (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.52386.0.0** (the Appx package version, which A13 records as the discoverable one on Windows; `%LOCALAPPDATA%\Claude\Logs\main.log` agrees at `1.52386.0`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** continued **E80** on this platform's own open branch and **answered it**. The 65,548-byte display-state blob never enters TIDE's DSP→UI queue, and the reason is the **document**, not the channel: the fixture's VCV Scope has **no patch parameters at all**, so its display-state pin is connected to nothing. Row → IN-REVIEW. **No product behaviour changed** — the only product-code edit is a default-off diagnostic. New [scripts/patch-parameters.py](scripts/patch-parameters.py), a `--save` arm on the VST3 probe, and [tests/fixtures/e80-vcv-scope-parameterised.xml](tests/fixtures/e80-vcv-scope-parameterised.xml). **No GUI was driven — the developer was at the machine, third windows run in a row.**

### Why a total could not have found it, and a census could

Every figure this row has produced across three platforms and four runs is an **aggregate**: *569 sends, largest 325 bytes, 59,878 bytes of lifetime traffic*. That establishes the blob is not in the queue. It cannot distinguish **the big thing is missing** from **this sender is missing entirely**, and those wanted opposite fixes.

`TIDE_FEEDBACK_CENSUS=<N>` (`SynthEditSem/SynthEdit.cpp`) tallies every whole message `drainRackFeedback()` forwards by **(handle, 4-char id)**. It costs one `std::map` insert per message on a walk that was *already* parsing those headers to find whole messages, and it is off unless asked — the same default-preserving rule `TIDE_FEEDBACK_TRACE_EVERY` was added under, for the same reason.

On `tests/fixtures/e75-vcv-visible-rack.xml`, VST3, `--editor`:

```
TIDE: feedback census (final) -- 25 sender(s)
TIDE:   handle -4         id 'godw'  n=19   max=0    total=228
TIDE:   handle 128979942  id 'ppc'   n=549  max=13   total=13725
TIDE:   handle 178048573  id 'ppc'   n=10   max=13   total=250
...  twenty-three more, every one max=13
```

**Twenty-four `ppc` senders, every one capped at 13 bytes, and not one message of any other size.** Thirteen bytes is a float patch parameter to the byte (`bool` + voice + value + the trailing −1). The queue was carrying **lights and nothing else**.

### That changed the question, and the plug-in had been answering the new one for nine runs

A rack module's lights **and** its display-state blob are both declared as private, non-persistent **parameters** with a `direction="out"` pin bound to them (`SynthEdit_Rack_Adaptor/RackAdaptor.h`). The lights were arriving. So the question stopped being *why does a blob behave differently from a float on the same module* and became *why is this module's parameter not a sender at all* — and the answer was in the same stderr every previous arm produced, above the counters everyone was reading:

```
SynthEdit: no patch parameter for module 987654321 parameter id 0
           -- pin left unconnected rather than dereferenced.      (x8, ids 0-7)
SynthEdit: patch parameter slot is null in ug_patch_param_watcher
           -- output parameter update skipped rather than dereferenced.
```

`987654321` is the fixture's **VCV Scope** — read out of the decoded document, not guessed. With no parameter, `ug_patch_param_setter::ConnectParameter` leaves the pin unconnected, `UPlug::Transmit` iterates an **empty** `connections` list, and `setValue` succeeds into nothing. The module still constructs, still processes, still captures its picture and still reports success.

### The A/B — same binary, one variable

800 blocks of 512 at 44.1 kHz, `--editor`, `TIDE_FEEDBACK_TRACE_EVERY=1`:

| | `e75-vcv-visible-rack.xml` | `e80-vcv-scope-parameterised.xml` |
|---|---|---|
| `no patch parameter for module` | **9** | **0** |
| feedback sends | 569 | 573 |
| **largest send** | **325 B** | **65,873 B** |
| lifetime queue traffic | 59,878 B | **17,502,646 B** |
| the blob's own sender, in the census | *absent* | `n=266  max=65,561  total=17,442,418` |
| `RackProcessor: 'Scope' display-state capture` | `#200 (65548 B)` | `#200 (65548 B)` |
| **the far end** | `update #1 arrived (0 bytes)` | **`update #260 arrived (65548 bytes)`** |

**CLAP, same build tree: `#1 arrived (0 bytes)` → `#260 arrived (65548 bytes)`.** That is **E80's Accept verbatim** — *"`RackEditor: display-state update #N arrived (65548 bytes)` advancing in a hosted CLAP"* — met for the first time since the row was filed on 2026-09-01.

**The capture row is the control and it is why this is a fact about delivery.** Identical in both arms: the DSP did the same work, captured the same 65,548 bytes the same 200 times, and only the fate of the picture changed. `max=65,561` is `13 + 65,548` exactly, which is the blob arriving whole rather than nearly.

### The proof is not the screen, and the obvious check is blind to this

[scripts/patch-parameters.py](scripts/patch-parameters.py) `--compare` diffs a document's per-module parameter counts against **the same rack as the product itself writes it** — obtained with the new `--save` arm on [tests/e80_vst3_feedback_probe.cpp](tests/e80_vst3_feedback_probe.cpp), which is `component->getState` with the int32 length prefix stripped:

```
tests/fixtures/e75-vcv-visible-rack.xml  vs  roundtrip.xml
    VCV: Scope   987654321   0 -> 11   <-- MISSING 11
```

One line, no allowlist, no judgement. The script's **zero-parameter flag is a screen and says so in its own docstring**: 6 flags on `e75` of which 1 is the defect, because `IO Mod`, `VCA` and `SE MIDI to CV 2` are parameterless by design. The allowlist was deliberately not extended to cover them — a long allowlist is how a screen stops screening — and the false-alarm rate is recorded instead.

**A TiDE document stores its PatchManager TWICE** — `<Parameter Module=…>` in `<DSP>`, `<param module=…>` in `<Editor>` — which is exactly the shape **E83** found the patch *cables* disagreeing in. **This defect defeats that check: both halves agree, and both are missing the same eleven parameters.** `--halves` prints the comparison anyway, because a disagreement is still worth catching; it is simply not what finds this.

### How wide, and what was deliberately not touched

Surveyed every committed fixture. **Three carry the crippled Scope and they are the only ones**: `e53-vcv-rack-segv.xml` and its two descendants `e75-vcv-visible-rack.xml` and `e83-vcv-scope-cabled.xml` — all the same handle `987654321`. Every E80, E19 and E83 display-state measurement was taken through one of them.

**They are left exactly as they are.** Three rows name them as reproductions, and rewriting a fixture other rows cite loses the thing they reproduce — E83's precedent, which added a fixture beside `e75` rather than editing it. The repaired rack is a new file, and it inherits two earlier runs' work for free: `patch-cables.py --show` reports **AGREE** (E83's fix, applied by the product on save) and `PanelLocationZoom` survives at `0.64999998` with the same 17 `panelRect`s (E75's visible panels).

**It does not regenerate byte-identically, and that is E77/E81 rather than a flaw.** Two saves a second apart differ in **592 of 892 decoded lines** and are **identical** once `Handle="…"` is masked and order ignored — E77's own normalisation, reproduced here as a by-product, on a document rather than on an export.

### The two build scripts, which is the smallest thing here and cost the most time

`build-e80probe.cmd` pointed its `-I` at `C:\SE\TideSynth\build-e19win\_deps\clap-src\include` — one box's 2026-09-02 scratch tree, which does not exist in a worktree. Both scripts now resolve either Visual Studio instance and take the build tree as an argument with a refusal when it has no CLAP headers (**seen to fire**, rc=1). `build-e80vst3probe.cmd` gains `ole32.lib`, which the probe's own header comment has named since it was written.

**I clobbered both files with a heredoc before noticing they were tracked**, and restored them from `HEAD` before re-applying the changes deliberately. The tell was `git status`, not anything failing.

### The developer edited two sibling repos WHILE this run was measuring, and the no-override build is why it did not matter

Third consecutive windows run to find him at the machine, and the first where his edits landed in repos this measurement depends on **during** the run rather than before it.

Both were clean at the start and dirty at STEP 5, with **real content changes, not CRLF churn** (`git diff --ignore-all-space` non-empty in both):

| repo | file | diff |
|---|---|---|
| `SynthEditLib` | `Module_Info3_base.cpp` | **7 insertions, 334 deletions** |
| `GMPI` | `Hosting/xml_spec_reader.{cpp,h}` | 8 insertions, 14 deletions |

**Left exactly as found — not committed, not reverted, not stashed.** They are unmistakably his: neither file was opened by this run, and neither has anything to do with rack feedback.

**This is the 2026-09-11 trap, and the remedy that entry proposed held.** That run lost a build to `C:\SE\GMPI` and `C:\SE\gmpi_ui` moving underneath it and concluded *"the fix is to stop using the overrides"*. A `*_FOLDER_OVERRIDE` build here would have compiled `Module_Info3_base.cpp` **mid-edit, 334 lines into a deletion** — and the resulting failure would have named `SynthEditLib`, which is GATED, on a branch that never touched it. Configuring with no overrides made the question disappear: the ten dependencies are whatever `main` pins, fetched fresh, and the developer can do as he likes in his own checkouts.

**So the advice is stronger than "prefer no overrides":** on this box a scheduled run that needs a trustworthy build should use none, and the reason is not tidiness — it is that the developer working in his own tree is the *normal* case here, now observed three runs running.

### Verification

| check | result |
|---|---|
| configure, **no `*_FOLDER_OVERRIDE`**, 10 deps fetched fresh | rc=**0** |
| build `TIDE_Rack_VST3` | rc=**0**, 0 `error C`/`error LNK` |
| build `TIDE_Rack_CLAP` | rc=**0**, 0 `error C`/`error LNK` |
| both probes rebuilt **from the committed scripts** | rc=0 each, binaries produced |
| the CLAP script's new refusal | `no CLAP headers under "no-such-tree\…"`, rc=**1** |
| A/B, VST3, same binary | 9 misses/569/325 B → **0 misses/573/65,873 B** |
| A/B, CLAP, same build tree | `#1 arrived (0 bytes)` → **`#260 arrived (65548 bytes)`** |
| the A/B's own control | `display-state capture #200 (65548 bytes)` **identical in every arm** |
| `--compare` on the committed fixture | `VCV: Scope 0 -> 11 MISSING 11`, rc=1 |
| `--compare`'s control (a document against itself) | rc=**0**, "every module carries at least as many" |
| round-trip reproducibility | raw **differs** (592/892 lines); handle-masked **identical**, md5 `42b496a43933` both |
| corpus survey, all committed fixtures | 3 of them carry the crippled Scope; no other `VCV:` module anywhere lacks parameters |
| `check-backlog-diff` | rc=0 — `E80: TODO -> IN-REVIEW`, status/date cells and new rows only |
| `check-next-block` / `check-id-refs` / `check-backlog-archived` / `check-links` | rc=0 each |
| `check-commit-authorship --repo .` | rc=0 — every unpushed commit `tide-rack-bot` |
| `check-commit-completeness --record/--verify` | 7 staged, 7 in HEAD, all present |
| NEXT-cell chain before/after | 7 → **8** generations; pipe count **5 before and 5 after** |
| CI on the pushed head | **all green, 0 fail** — `windows` 6m56s, `macos`, `linux` 2m31s, plus `lint`, `guard`, `e57-delete-key` and the three `render-*`; `mergeStateStatus: CLEAN`. **`guard` did NOT skip the build matrix this time**, correctly: this branch changes compiled source, unlike the two before it |

**No SynthEditCL build, and it is discharged by SCOPE rather than glossed:** no sibling repo was edited at all, the one product file changed is `SynthEditSem/SynthEdit.cpp` (TIDE's own, ALLOWED), and the build used **no folder overrides**, so nothing local was consumed either.

**Learned:**

- **Before believing a channel is broken, ask what it IS carrying.** Four runs measured a total and a maximum. A total cannot separate *the big message is missing* from *that sender never spoke*, and the second was the answer. The census was one map insert on a walk already parsing the headers.
- **Read the whole stderr, not the counters you came for.** `no patch parameter for module 987654321` printed nine times in every arm of every previous run, including mine before I looked. The counter lines were what everyone grepped for, so the diagnostic sat above them unread for nine runs.
- **A module handle that looks hand-typed probably is.** `987654321` among `529566147`, `13300239`, `249916321` — the other four are random 31-bit snowflakes. That was the visible tell that the fixture had been touched, and it is the one I noticed last rather than first.
- **When a fixture is the suspect, ask the product what IT writes.** `--save` plus a per-module diff turned "this document looks wrong" into `0 -> 11` with no allowlist and no domain knowledge. The product is the oracle for its own file format.
- **The check that caught the last fixture defect is blind to this one, by construction.** E83's two-copy disagreement is a sound test and both halves agree here. A validator's coverage is a property of the defect, not of the file — say which defects a check cannot see, in the check.
- **A screen with a measured false-alarm rate is publishable; one with a growing allowlist is not.** Six flags, one real, the three innocent types named in the docstring and deliberately not allowlisted.
- **Say "screen" and "proof" in the tool, not in the write-up.** `--compare` is sound and the zero-flag is not; putting that distinction in `--help` is what stops the next run quoting the wrong one.
- **`git status` before assuming a helper script is yours.** I overwrote two tracked build scripts with a heredoc. Nothing failed and nothing warned; restoring from `HEAD` and re-applying deliberately cost two minutes, and not noticing would have put an undiscussed rewrite in the diff.
- **Survey the sibling repos at the END as well as the start, and say which way the dirt moved.** Two were clean at the start and carry the developer's real edits now — 334 deletions in one file, made while this run was measuring. A start-only survey would have reported "all clean" and been wrong about the machine it was handing over.
- **The 2026-09-11 "stop using the overrides" remedy is not a preference on this box, it is the difference between a build and a wrong diagnosis.** An override build would have compiled `Module_Info3_base.cpp` mid-deletion and blamed `SynthEditLib`, which is GATED and which this branch never touched.

**Not verified:** **the STANDALONE**, which the row's original figure also named and which must own a real window — still unmeasured, still wants an idle box. **What produced the crippled fixture** — the round-trip shows a save REPAIRS it, and nothing here establishes how a document reached that state in the first place; the 2026-08-26 session file's provenance is a claim from E83's entry, not something this run re-derived. **Whether any document TiDE writes today can reach it.** **Anything requiring the editor to have PAINTED** — an invisible off-screen window gets no `WM_PAINT`, so `RackEditor: render #N` is 0 in every arm here, as in the two previous runs. **macOS and Linux BEHAVIOURALLY** — nothing was re-measured on either, and no probe was run there; what CI does establish, and it is more than this line first claimed, is that the census **compiles** on all three platforms (`windows`, `macos`, `linux` jobs all green on the pushed head). Compiling is not running. **E19's own clauses** — this run did not take them, and its row is untouched. **E85** (`gui->show` returns false) fired identically in both CLAP arms, so it is not a variable in this A/B and is otherwise untouched. **`SynthEditCL` and SynthEdit proper** — not built; nothing outside TideSynth changed.

**Machine state.** **Worked entirely in a `git worktree` at `C:\SE\TideSynth-wt-e80b`; `C:\SE\TideSynth` was never checked out or built in** and is on `main` at `13095a395`, clean, exactly as found. All eight local repos were on their default branches at the start. **`C:\SE\GMPI_Wrappers` carries one dirty file, `wrapper/AU3/AU3_Wrapper.mm`, and it is PURE CRLF CHURN** (`git diff --ignore-all-space` empty) — the developer's, predating this run, recorded by the 09-11 and 09-14 entries too, and **left exactly as found**: not committed, not reverted, not stashed. Every other sibling (`SE16` `5e5433dad`, `SynthEditLib` `8f66c31`, `gmpi_ui` `1baf360`, `GMPI` `cf7504b`, `SynthEdit_Rack_Adaptor` `04d1296`, `VCV_Fundamental_gmpi` `93a27f9`) was clean **at the start**, and every one of them is **read-only as far as this run is concerned — none was built from, fast-forwarded or written to**, because the build used **no `*_FOLDER_OVERRIDE` at all**. **TWO OF THEM DID NOT STAY CLEAN, AND NOT BECAUSE OF THIS RUN — see the section above.** Dependency shas the measurement was actually built against are the ones `main` pins and CMake fetched: `syntheditlib 8f66c31`, `gmpi cf7504b`, `gmpi_ui 1baf360`, `gmpi_wrappers 4c11d6d`, `rack_adaptor 04d1296`, `vcv_fundamental 93a27f9`, `clap a47f6ba`. **No host, DAW or standalone was launched, nothing was installed or registered, no `%APPDATA%` was touched and no window was displayed** — both probes are bare hosts, and the `--editor` arms parent the view to a never-shown off-screen `WS_POPUP`. **`Get-Process | Where-Object { $_.MainWindowTitle }` before and after returned the same three Visual Studio instances and the same applications**, and **0 TIDE and 0 REAPER processes** are running. The gitignored build tree (`build-e80cen`) and every log, probe binary and scratch document live in the worktree, which STEP 5 removes.

**Next:** **E19's win VST3 cell, and it is no longer fixture-blocked in either clause.** Its pixel-diff clause now has a rack whose Scope genuinely animates — `tests/fixtures/e80-vcv-scope-parameterised.xml` — and that clause may not need a screen at all: the off-screen-HWND arm creates a real editor whose pins update, and the census shows the payload arriving. **What it cannot show is paint** (no `WM_PAINT` on an invisible window), so a pixel diff still wants a visible window and a right-click still wants an idle box. **E82's remaining arm is E19's other clause and the module is `WT LFO`, not the Scope** — see [#587](https://github.com/JeffMcClintock/TideSynth/pull/587). **Re-read E83 and E19's published zeros in the light of this row**: E83 concluded the Scope's *input* was unwired and its capture correct, which stands — but its companion claim that the payload reaches the editor on VST3 and the standalone was read off the same crippled fixture. **`scripts/patch-parameters.py <fixture> --compare <roundtrip>` before quoting any display-state number.** **`JOURNAL.md` rotation still waits on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585)**, which *is* the rotation rule; ~229 KB against A24's 60 KB ceiling. **One thing seen and deliberately not fixed:** the `win` NEXT cell carries an **unescaped `|`** inside an older generation's `Get-Process | Where-Object`, so that table row renders with an extra column. It predates this run, `check-next-block` passes, and editing preserved text to fix it is the "while I was in there" STEP 3 warns about — but it is one character for whoever next rewrites that cell.

**Branch/PR:** `tide/win/E80-clap-editor-arm`, [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) — `TIDE_FEEDBACK_CENSUS` in `SynthEditSem/SynthEdit.cpp`, the `--save` arm on [tests/e80_vst3_feedback_probe.cpp](tests/e80_vst3_feedback_probe.cpp), [scripts/patch-parameters.py](scripts/patch-parameters.py), [tests/fixtures/e80-vcv-scope-parameterised.xml](tests/fixtures/e80-vcv-scope-parameterised.xml) and [its README](tests/fixtures/e80-vcv-scope-parameterised.README.md), both `build-e80*probe.cmd` scripts, the census section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), E80 → IN-REVIEW with its answer, the refreshed `win` NEXT cell, regenerated [docs/lessons.md](docs/lessons.md), and this entry.

## 2026-09-15 — macos — E72: the cable path really is unguarded, and the save was never relying on it (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.52386.6** (no `claude` CLI on this box's PATH; A13 records the app's `CFBundleShortVersionString` as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required

**Did:** took **E72** and answered it on a locked screen, the fifth row running this lane has recovered by separating what a row ASKS from what its Accept asks. New [tests/e72_dsp_dirty_probe.py](tests/e72_dsp_dirty_probe.py), a `PROPOSED:` entry in [docs/decisions.md](docs/decisions.md), and a comment correction in [SynthEditSem/TideApp.cpp](SynthEditSem/TideApp.cpp). **No behaviour changed, in this repo or any sibling** — the only compiled file touched is a comment. Also flipped **E83** to DONE and archived it. Branch `tide/mac/E72-cable-dsp-dirty`.

### The reading is confirmed — and the control is what makes that a measurement

E72 filed its finding as, in its own words, *"the finding, read rather than measured"*. [tests/e72_dsp_dirty_probe.py](tests/e72_dsp_dirty_probe.py) enumerates every site in SynthEditLib that can set `dspDirty`, then asks of each editor entry point whether it is guarded:

| entry point | role | guarded |
|---|---|---|
| `MfcDocPresenter::AddPatchCable` (`:280`) | subject — rack patch cable added | **no** |
| `MfcDocPresenter::RemovePatchCable` (`:363`) | subject — rack patch cable removed | **no** |
| `ConnectPlugs` (`plug4.cpp:541`) | **CONTROL** — structure-view wire drawn | **yes** |

**The last row is the one that matters.** "AddPatchCable contains no guard" is also what a broken parser prints. Same parser, same file set, opposite answer — so the omission is specific to rack patch cables. Both failure directions were exercised rather than assumed: claiming the cable path is guarded gives rc=**1**, claiming the control is not gives rc=**1**, and an unreadable SynthEditLib gives rc=**2** and says *"this is a skip, not a pass"* rather than a green table nobody measured.

**15 sites can set the flag** — 13 `SuspendDSP` constructions plus 2 direct `invalidateDsp()` calls — and not one is on the patch-cable path.

### The part E72 had backwards, and it makes the defect SMALLER

E72 says the save is safe *"because the save now mints from the controller rather than waiting for a push"*, and cites nothing. **It is safe, and the citation is `SynthEditController::syncState()`**, which calls `tideApp->exportChunkXmlForSave()` **unconditionally** when the host asks for state (`TideApp.cpp:643` — no `dspDirty` in it, and never was).

**So E68's Ableton measurement is fixed by that mint, not by the mechanism E68's own comment credits.** That comment said *"a cable edit now pays one document push … that shipment is what makes the save correct"*. It cannot: a cable edit sets no flag, so `serviceDocumentSync` returns at its first line and the push never happens. The save has been correct for a different and stronger reason the whole time.

**What is actually exposed**, narrowed from E72's wording: the chunk parameter's retained bytes, which GMPI's `Hosting/processor_holder.cpp` re-seeds into every processor it starts (its own comment: *"a processor can be created at any time - after restartComponent, for offline rendering, or on state restore"*). Draw a cable, then have the host recreate the processor **with no host state query in between**, and the new processor is born running the pre-cable document. Any state query refreshes those bytes — which is why the window is narrower than E72's *"with no save in between"*.

### Three corrections, all to prose that was confidently specific

1. **E72 cites `SuspendDSP.cpp:27` as "the RAII guard that does" set the flag.** Line 27 is `m_app->dspDirty = true;` and it sits inside an `#if 0` block — it has not compiled in as long as it has been there. The live line is `SuspendDSP.cpp:7`, `p_app->invalidateDsp()`. The probe strips dead code for exactly this reason, and prints the two lines side by side under `--verbose`.
2. **`TideApp.cpp` said the guard sits at "23 sites".** Measured: **15**.
3. **`TideApp.cpp` said `dspDirty` fires on "re-cabling".** True of structure-view wires, false of the rack patch cable a TIDE user actually drags — which is the only kind the default view offers. Corrected on this branch.

### The ruling E72 has wanted since 2026-08-31 is now actually asked

E72 has said for fifteen days that it *"wants a ruling rather than a session"*. **Nobody had filed the `PROPOSED:` entry**, which is the only thing that puts a question to Jeff — so the row sat naming a decision that had never been requested. Filed now in [docs/decisions.md](docs/decisions.md): three options, recommended default **(b) guard both entry points**, and it parks **only E72**.

**E81 is in the identical state** — filed 2026-09-05, says it wants a ruling, has no `PROPOSED:` entry — and is the cheapest row on this lane's board.

### Verification

| check | result |
|---|---|
| `tests/e72_dsp_dirty_probe.py` | rc=**0**, 15 sites, control guarded, subjects not |
| probe negative control — claim subject IS guarded | rc=**1**, `guarded=False, E72 records True` |
| probe negative control — claim control is NOT guarded | rc=**1**, `guarded=True, E72 records False` |
| probe skip path — unreadable SynthEditLib | rc=**2**, *"this is a skip, not a pass"* |
| dead-code stripper, measured | `SuspendDSP.cpp:7` kept, `:27` blanked, line numbering intact |
| build, `TIDE_Rack_CLAP`, Release/arm64, before the edit | rc=**0**, `[6/6]`, **0** `error:` |
| build, after the `TideApp.cpp` comment edit | rc=**0**, `[3/3]`, **0** `error:` |
| `check-commit-authorship --repo .` | rc=0 — every unpushed commit `tide-rack-bot` |
| `check-commit-completeness --record/--verify` | recorded before the commit, verified after |
| `check-next-block.py` | rc=0, *"every NEXT take-target is a live BACKLOG.md row"* |
| CI on [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) | **6 pass, 0 fail** — `lint`, `linux`, `e57-delete-key`, `render-linux`, `render-macos`, `render-windows`; `guard`/`matrix.name` **skipped**. `mergeable: MERGEABLE`, `mergeStateStatus: CLEAN` |

**The build is a WARM-tree datum and says so:** `[6/6]` and `[3/3]`, not a from-scratch 61/61. It reuses the 2026-09-07 `build-e75/` tree, reconfigured against current `main`; the rebuild count is small because only `SynthEditLib` (now `134aa07`) and one comment moved.

**No macOS CI compile ran, and `guard` SKIPPED the build matrix** even though this branch touches a compiled file — the change is comment-only, and that is `guard` behaving correctly rather than a gap. The build evidence is therefore local, and it is the two rows above.

**Learned:**

- **A row that says it "wants a ruling" has not asked for one.** E72 and E81 both name a decision and neither filed the `PROPOSED:` entry that requests it, so both have been waiting on a question nobody put. Filing it costs one edit. **Check for the entry, not for the sentence.**
- **When a comment names a mechanism, the mechanism is a claim.** Three of this run's corrections were to prose that was specific enough to sound measured — a cited line inside `#if 0`, a count of 23 that is 15, and "re-cabling" meaning the other view. Specificity reads as evidence and is not.
- **A fix can be correct for a reason its own comment gets wrong**, and that is worse than an uncommented fix: E68 works, so nothing fails, and the next person to reason about `dspDirty` inherits a false model with a merged PR behind it.
- **Put the control in the same table as the subject** — E83 landed this lesson six days ago and it is what made today's zero a finding rather than a possible parser bug.
- **`sed` with aligned whitespace is not a reliable way to flip one token.** My first attempt at the second negative control silently did not apply and printed a PASS; only checking that the flipped file DIFFERED caught it. A control that does not actually change anything is the most expensive kind of green.

**Not verified:** **E72's own Accept** — recreate the processor after a cable edit and see whether the cables are present. Drawing a cable needs the editor and this run's screen was locked; **the mechanism is measured, the consequence is not.** **That any host actually recreates a processor without querying state first** — the window is real by construction but its frequency is unmeasured. **Anything on Windows or Linux**, where nothing was built or run. **SynthEdit proper**, which shares `MfcDocPresenter.cpp` and would receive option (b)'s two lines.

**Machine state.** TideSynth was clean and on `main` at the start and is on `tide/mac/E72-cable-dsp-dirty` until STEP 5 returns it. **`SynthEditLib`'s tree carries Jeff's uncommitted work in progress** — `modules/se_sdk3_hosting/SynthEditCocoaView.mm`, 27 insertions / 3 deletions, **real content and not CRLF churn** (`git diff --ignore-all-space` is non-empty), mtime 2026-09-14 16:14. **Untouched: not committed, not reverted, not stashed**, per STEP 5's third kind of dirt, and every commit on this branch is TideSynth-only. `SE16` is not on this box. `GMPI`, `GMPI_Wrappers` and `gmpi_ui` were **read only**; the build ran `SE_LOCAL_BUILD=OFF`, so it fetched `SynthEditLib` from `origin/main` rather than using Jeff's dirty tree. **Nothing was installed, registered or launched**: no DAW, no standalone, no AUv3, `~/Library/Audio/Plug-Ins` untouched, **0 TIDE processes**. The screen was **locked throughout and no GUI was attempted**.

**Next:** **[#585](https://github.com/JeffMcClintock/TideSynth/pull/585) is 15/15 green and waiting on Jeff**, and it is A36 — the journal ROTATION RULE. `JOURNAL.md` is now **~229 KB against A24's 60 KB target**; the 09-09 cell said the window opens once #581 merges, and it has, **but rotating now would conflict against the one PR that changes how rotation works.** Merge #585, then rotate to its rule. **E81 wants a `PROPOSED:` entry and nothing else** — same shape as E72, and it is the cheapest thing on this board. **E19's mac AU3 cell and its pixel-diff clause still want one unlocked screen**, and `e83-vcv-scope-cabled.xml` is the fixture they wanted. **E79, E80 and E82 all have open PRs from other platforms** and are not this lane's.

**Branch/PR:** `tide/mac/E72-cable-dsp-dirty`, [#588](https://github.com/JeffMcClintock/TideSynth/pull/588) — [tests/e72_dsp_dirty_probe.py](tests/e72_dsp_dirty_probe.py), the `PROPOSED:` entry in [docs/decisions.md](docs/decisions.md), the `TideApp.cpp` comment correction, E72 → IN-REVIEW with its answer, E83 → DONE and archived, the refreshed `mac` NEXT cell, and this entry.
## 2026-09-14 — windows — E82: the producer exists, and all five probe points were on the one module that has nothing to offer (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.52386.0** · as **tide-rack-bot** (both paths) · scheduled run

**Did:** took **E82** and answered it. Its headline — *"A RIGHT-CLICK ON A RACK MODULE'S PANEL RETURNS THE RACK'S OWN CONTEXT MENU, NOT THE MODULE'S"* — is a correct observation with a wrong cause, and its reading of that observation (*"E19's `int/bool/enum` clause having no producer at all"*) is **false**. Row back to TODO; no product code changed. New instrument: [tests/e82_rack_menu_probe.cpp](tests/e82_rack_menu_probe.cpp) + [tests/e82_rack_menu_probe.sh](tests/e82_rack_menu_probe.sh). **No GUI was driven, because the developer was working at the machine** — second consecutive windows run.

### The producer, named

`SynthEdit_Rack_Adaptor/RackEditor.h:630` implements `populateContextMenu`. It builds a **ticked item** per `BoolPtr` option and a **labelled submenu** per `IndexPtr` option out of `layout.menu`, and picking one writes the index to that option's parameter pin — which is what carries it to the processor's own module instance. Its first line, `:633`, is:

```cpp
if (layout.menu.empty())
    return gmpi::ReturnCode::Unhandled;
```

**So a module that declared nothing adds nothing, and the menu the user sees is the rack's own — byte-identical to the empty-canvas one, by design.** That is E82's observation exactly, and it is not a routing failure.

`layout` comes from `readPanelLayout(*model)` at `RackEditor.h:128`, which is the same call the XML generator makes (`RackAdaptor.h:221`). It is the whole input to the menu.

### The measurement: 12 of 39, and the fixture is why nobody saw one

[tests/e82_rack_menu_probe.cpp](tests/e82_rack_menu_probe.cpp) calls `readPanelLayout()` for **all 39 models** of TIDE's compiled-in VCV set and prints every entry. `bash tests/e82_rack_menu_probe.sh`, rc=0, ~1 minute, 39 TUs, **0 models unlinked**:

| module | options | what |
|---|---|---|
| `Fade` | 1 | INDEX *Pan law* [-6 dB (linear), -3 dB] |
| `Merge` | 1 | INDEX *Channels* (18 labels) |
| `Mixer` | 2 | BOOL *Invert output*, *Average voltages* |
| `Rescale` | 3 | INDEX *Gain multiplier* [1x,10x,100x,1000x] + 2 BOOL |
| `SEQ3` | 1 | BOOL *Clock passthrough* |
| `SequentialSwitch1` / `2` | 1 each | BOOL *De-click* |
| `Unity` | 1 | BOOL *Merge channels 1 & 2* |
| `VCA-1` | 1 | BOOL *Exponential response* |
| `VCMixer` | 2 | BOOL *Exponential channel VCAs*, *Exponential mix VCA* |
| **`WTLFO`**, `WTVCO` | 1 each | **INDEX *Wave points*, 10 labels [32..16384], default index 5** |

The other 27 measure `options=0`.

**Now put the e75 fixture's rack beside that.** Its five modules are `LFO`, `Pulses`, `SHASR`, `WTLFO`, `Scope`:

- `LFO`, `Pulses`, `Scope` — **`options=0`, `entries=0`**. `Scope` does not declare `appendContextMenu` at all.
- `SHASR` — **`options=0`, `entries=1`**, and the one entry is a separator. Its only item is `createRangeItem`, which `SynthEdit_Rack_Adaptor/rack/rack.hpp:2968` is a **MOCK** and which `collectMenu` skips as *"a menu item shape this adaptor does not model yet"*.
- **`WT LFO` — one INDEX option — and it was not one of the five points probed.** E82's five were *"title, display, TIME knob, body"* of the **Scope**.

So *"There is no VCV context-menu option to toggle"* was true of the module under the pointer and false of the rack it was sitting on. **E19's `int/bool/enum` clause has a producer on every platform, for both datatypes, and the `int/enum` one is already on the committed fixture.**

### The host-side routing is intact, and was traced rather than assumed

The chain, on the path a real right-click takes:

1. `gmpi_ui/backends/DrawingFrameWin.cpp:526` — `WM_RBUTTONDOWN` goes to `inputClient->onPointerDown(p, flags)` **first**, and `doContextMenu(p, flags)` is called **only if that returns `Unhandled`**.
2. `SynthEditLib/modules/se_sdk3_hosting/ViewBase.cpp:240` — `onPointerDown` calls `calcMouseOverObject(flags)`, so `mouseOverObject` is current.
3. `ModuleView::onPointerDown` returns `Unhandled` for the second button (the `ModuleView.cpp:1011` comment says so in as many words), so `ViewBase::onPointerDown` returns `Unhandled` at `:367` and step 1's fallback fires.
4. `ViewBase::populateContextMenu` (`:515`) asks the presenter, then `:528` asks `mouseOverObject` — the module.
5. `ModuleView::populateContextMenu` (`ModuleView.cpp:1518`) forwards to `pluginInput_GMPI`, which is the `RackEditor`.

Nothing in that chain is conditional on the module being unlocked, and nothing skips it for a rack module.

### The trap the `--context-menu` verb sits in, which is worth reading before the next arm

**`ViewBase::populateContextMenu` is the one entry point that never calls `calcMouseOverObject`.** `onPointerDown` (`:240`) does, `onMouseWheel` (`:506`) does, `onPointerMove` (`:428`) does; `:515` reads whatever the last pointer event left behind. In the GUI that is harmless — step 1 above guarantees a fresh `onPointerDown` immediately before — but the **`--context-menu` verb calls `client->populateContextMenu(point, sink)` directly** (`GMPI_Wrappers/wrapper/Standalone/mcp/CommandDispatcher.cpp:829`) with no pointer event at all. That file already warns about the consequence in different words (`:754`, *"THE MENU IS BUILT FOR THE CURRENT SELECTION, NOT THE PROBE POINT"*); the mechanism is `mouseOverObject`, and the remedy is unchanged: `--pointer-down`/`--pointer-up` on the panel first.

### What is still unmeasured, stated plainly

**That a right-click on WT LFO's panel actually lists *Wave points*, and that picking a label changes DSP behaviour.** That is E82's Accept and E19's clause, it needs the editor, and this run could not have one. The probe proves the menu's *input* is non-empty for that module and that `RackEditor` emits it; it does not exercise the host-side routing and does not click anything. One session on an idle box closes it.

### The instrument, which is the part that generalises

**A measurement with no editor, no window, no host and no GMPI SDK.** `readPanelLayout()` lives in `RackPanelLayout.h`, whose only non-`<std>` include is the adaptor's `rack.hpp` mock. What normally drags the GMPI SDK in is *registration* — `rack::createModel` calls `rack_adaptor::autoRegisterModel`, defined in `RackAutoRegister.h`, which pulls `RackAdaptor.h`/`RackFactory.h`/`RackEditor.h`. **`RACK_NO_AUTO_REGISTER` is the adaptor's own documented opt-out** and turns exactly that off. What is left is 38 module sources compiled one per TU, plus `main()`, linked with `link` — about a minute with `cl` on PATH and nothing configured. The probe writes only to a temp dir; neither sibling checkout is touched.

Two smaller things the build needed, recorded so the next person does not rediscover them: the module sources expect `<cassert>` (`WTLFO.cpp:285`) and `<map>` (`Gates.cpp:48`) to have been included already — the real build gets them via `RackModule.h` — and `osdialog.h` resolves from `SynthEdit_Rack_Adaptor/compat`, which is not on the two obvious include paths.

### The developer was at the machine — again

`Get-Process \| Where-Object { $_.MainWindowTitle }` at the start of the run: **three Visual Studio instances** (`SimulatorGmpi - ParticleMgr.cpp`, `SynthEditStore - SeAudioMaster.cpp`, `ExonicModules - EqParticleGraphGui.cpp`), plus Chrome, Slack, Outlook and GitHub Desktop. So no host was launched, no window was created, no `%APPDATA%` was touched. **The same command after the run returned an identical process set** — verified, not asserted, which is the rule the 2026-09-09 entry set for this arm.

This is the second consecutive windows scheduled run to find him working. It should be read as the normal case on this box, not the exception.

**Dirty trees:** `GMPI_Wrappers` had one modified file, `wrapper/AU3/AU3_Wrapper.mm`, predating this run and macOS-only. Left alone, per STEP 5's third kind. Every other repo (`TideSynth`, `SE16`, `SynthEditLib`, `gmpi_ui`, `GMPI`) was clean.

**`JOURNAL.md` rotation NOT done, and the 2026-09-09 cell's *"genuinely unblocked"* is superseded.** [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) `tide/mac/A36-journal-rotation-rule` is open and **is the rotation rule itself**; rotating from this lane would make that PR conflict on the hardest file in the repo to resolve. ~229 KB against A24's 60 KB ceiling. Whoever merges #585 rotates.

**STEP 1:** `gh issue list --label platform:win` is empty and **that still verifies nothing** — `build.yml:523` excludes `matrix.platform != 'win'` from filing platform issues. Read `main`'s latest `build` run instead: green on all three platforms at `13095a395`, run [34438892984](https://github.com/JeffMcClintock/TideSynth/actions/runs/34438892984). Issue [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) is `platform:linux` and is not this box's. **STEP 1.5:** this platform's only open PR is [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) `tide/win/E80-clap-editor-arm` — `mergeStateStatus` **CLEAN**, `mergeable` MERGEABLE, 15/15 checks green, no reviews and no comments. Green with nothing unresolved is not this run's to fix; left alone. `mergeStateStatus` was checked explicitly, per the four occurrences of the CONFLICTING trap.

**Learned:**

- **A menu that is byte-identical over a module and over empty canvas is not evidence the click missed the module.** It is equally the signature of a module that was asked and had nothing to say, and `RackEditor.h:633` is the line that makes those two indistinguishable from outside. The control that separates them is not a second point on the same module — it is a **different module**, one known to declare options.
- **Before assuming a question needs a GUI, ask whether the data the GUI would display can be computed directly.** The right-click menu's entire input is one function call on a header whose only dependency is a mock. That is a third rung below the off-screen-HWND arm the 2026-09-09 run added, and it is cheaper than both: no build tree, no configure, no plug-in, no screen.
- **A fixture that makes a clause *visible* is not yet a fixture that makes it *measurable*.** E75 put the Scope on screen, which is what let E82 be asked at all — and the Scope is the one module of that rack's five with nothing in its menu. Check that the module carries the thing under test, not just that it is on screen.
- **`RACK_NO_AUTO_REGISTER` is what separates the adaptor's data model from its GMPI binding**, and the adaptor documents it as a per-module opt-out. It is also the switch that makes any layout-level question answerable in a single-TU build.
- **The one entry point that does not refresh `mouseOverObject` is `ViewBase::populateContextMenu`.** Every other pointer-consuming method in `ViewBase` calls `calcMouseOverObject` first. That is invisible in the GUI, where `WM_RBUTTONDOWN` always sends `onPointerDown` first, and it is exactly what the `--context-menu` verb walks into.
- **`Get-Process \| Where-Object { $_.MainWindowTitle }` before and after, not just before.** The before-check decides whether to stay off the GUI; the after-check is what proves the run actually did. Identical process sets is one line of evidence and costs nothing.

**Not verified:** **that the menu actually appears** — nothing was clicked, no editor was created, no window existed. The probe reads `layout.menu`, which is `RackEditor::populateContextMenu`'s only input, and stops there. **That picking *Wave points* changes DSP behaviour** — E82's Accept in full, and the other half of E19's clause. **Anything about the `bool` arm in a running editor** — the seven bool-bearing modules are named from the probe, not from a menu anyone saw. **The other two platforms** — the probe builds on all three (`$CXX` path in the script) and was compiled only on Windows, with MSVC 14.44. **`SynthEditCL`, SynthEdit or TIDE** — none was built; this branch adds two test files and three markdown edits and touches no product code, so none should be affected, but none was compiled to say so.

**Machine state.** `TideSynth`, `SE16`, `SynthEditLib`, `gmpi_ui` and `GMPI` were all clean and on their default branches; `GMPI_Wrappers` was on `main` with one pre-existing modified file, `wrapper/AU3/AU3_Wrapper.mm`, left untouched. Shas the measurement was taken against, since the probe compiles two sibling checkouts directly: `SynthEdit_Rack_Adaptor` `04d1296`, `VCV_Fundamental_gmpi` `93a27f9`. For the record, unused by this run: `SynthEditLib` `134aa07`, `gmpi_ui` `1baf360`, `GMPI` `cf7504b`, `GMPI_Wrappers` `4c11d6d`, `SE16` `afd44ea87`. Every object file went to a `mktemp -d` deleted on exit; neither sibling checkout was written to, and no build tree in any repo was configured or touched.

**Next:** **E19's win VST3 cell, or E82's remaining arm — they are two ends of one measurement, and both want an idle box.** Load `tests/fixtures/e75-vcv-visible-rack.xml` in the standalone (`-DTIDE_VCV_FUNDAMENTAL=ON`), `--pointer-down`/`--pointer-up` on the **WT LFO** panel, then `--context-menu` at the same point: *Wave points* with ten labels should list, and picking one should move `waveLen`. **Do not probe the Scope for this** — that is what this entry is about. If the developer is at the machine again, the queue for this box is genuinely empty and stopping is the right outcome: every other `TODO` is taken, linux in substance, GATED, `NEEDS-SPEC`, or a `.github/workflows/**` edit the bot's token cannot make. **`JOURNAL.md` rotation waits on [#585](https://github.com/JeffMcClintock/TideSynth/pull/585)**, which *is* the rotation rule; whoever merges it rotates.

**Branch/PR:** `tide/win/E82-rack-menu-producer` — E82 back to TODO with the finding, the refreshed `win` NEXT cell, the new `tests/e82_rack_menu_probe.cpp` and `tests/e82_rack_menu_probe.sh`, regenerated `docs/lessons.md`, and this entry.
## 2026-09-11 — windows — E80: the VST3 does not carry the blob either, so the row is mis-titled (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.49585.0.0** (the Appx package version, which A13 records as the discoverable one on Windows; `%LOCALAPPDATA%\Claude\Logs\main.log` agrees at `1.49585.0`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**The app moved between runs and the provenance line is how anyone would know:** 2026-09-10 recorded **1.46388.4.0** on this box, and it is **1.49585.0.0** today. Checked rather than copied forward, which is the only reason it is right.

**Did:** continued **E80** on this platform's own open branch and answered the arm the last two cells named by name — *"the same two counters on the Windows VST3, with `TIDE_FEEDBACK_TRACE_EVERY=1`"* — with a **new bare VST3 host**, [tests/e80_vst3_feedback_probe.cpp](tests/e80_vst3_feedback_probe.cpp). **The VST3 does not carry the blob either.** Row stays TODO; no fix, and none attempted. Filed **E86**. No product code changed, in this repo or any sibling.

### The answer

Both binaries built from **one configure**, so the format is the only variable. `TIDE_FEEDBACK_TRACE_EVERY=1`, 800 blocks of 512 at 44.1 kHz, `tests/fixtures/e75-vcv-visible-rack.xml`:

| arm | sends | **largest send** | lifetime queue traffic | `display-state capture` | far end |
|---|---|---|---|---|---|
| VST3 `--no-editor` | 569 | **325 B** | 59,878 B | `#200 (65548 B)` | — |
| VST3 `--editor` | 569 | **325 B** | 59,878 B | `#200 (65548 B)` | `#0/#1 arrived (0 bytes)`, nothing after |
| VST3 `--editor --no-pump` | 569 | **325 B** | 59,878 B | `#200 (65548 B)` | lights **38 → 15** |
| VST3 `--editor`, cabled fixture | 569 | **325 B** | 59,878 B | `#200 (65548 B)` | as above |
| CLAP `--no-editor` | 569 | **337 B** | 59,903 B | `#200 (65548 B)` | — |
| CLAP `--editor` | 569 | **337 B** | 59,903 B | `#200 (65548 B)` | `#0/#1 arrived (0 bytes)`, nothing after |
| VST3 `--no-preset` (negative control) | **0** | — | 0 | **none** | — |

**The two formats' entire 9.288 s of queue traffic differ by twenty-five bytes, and both are smaller than ONE 65,548-byte picture.** `0 held back` on all 1,138 sends.

**So the number this row was filed on is gone.** *"65,673 repeatedly on VST3 and in the standalone"* was read in REAPER off the **old 1-in-100 cadence** — the cadence the 2026-09-09 run showed samples about 2% of sends and therefore cannot answer an existence question. It withdrew the CLAP quotes on exactly that ground; the VST3 quote had never been re-read, and it does not survive either.

**E80 is not a CLAP question. The defect is real, and format-independent.** Everything on the path is upstream of every wrapper, which is what the 2026-09-10 entry predicted from source and this measures.

**Four controls, all inside this run**, because a null result is worth nothing without them:

- **`--no-preset`**: 0 sends, 0 captures, **0 modules constructed**. Every line in the other arms came from the document under test.
- **`--no-pump`**: editor-side lights **38 → 15** while the DSP counters do not move a byte. Starvation is real and visible exactly where it should be, so "the bare host starved something" is not available.
- **the cabled fixture** ([e83-vcv-scope-cabled.xml](tests/fixtures/e83-vcv-scope-cabled.xml)): identical in every figure. A varying payload changes nothing.
- **CLAP re-measured from the same tree**: reproduces 2026-09-10 to the byte (569 / 337 / `#200`). That is a check on the instrument, not a repeat.

### E86 — the probe's first version was wrong, and the plug-in did not say so

Worth more than the table, because it is a silent failure and it is TIDE's, not the probe's.

The first version created only the `IComponent`. That is legal VST3 and is what a CLAP-shaped mental model suggests, since a CLAP plug-in is one object. It produced: the right document (`building rack from 38661 byte document`), the right `TIDE: rack built for 44100 Hz, block 512`, and then **zero** `RackProcessor: '<slug>' constructed` lines, **zero** display-state captures, and 3 feedback sends. **No error, no warning, nothing in the log that named the problem.**

TIDE's module factory — VCV Fundamental's 39 models, the four enrichment XMLs, the five bundled prefabs — is populated by **`TideApp::InitInstance()`** (`SynthEditSem/TideApp.cpp:813`), which is **controller-side**. On CLAP one object is both halves so it always runs. On VST3 it does not.

| `--no-controller` | with a controller |
|---|---|
| **0** modules constructed | **29** |
| **0** display-state captures | 200 |
| 20 sends, max **37 B** | 569 sends, max 325 B |

Kept in the probe as `--no-controller` — a **reproduction**, not a useful arm — because a claim about a silent failure is worth little if the next reader has to recreate the bug to see it.

**I have deliberately not claimed this is reachable in a shipping host.** Every mainstream DAW creates both halves in one process. What makes it worth a row rather than a footnote is that VST3 separates the two *precisely* so they need not share one, the spec permits component-only instantiation, and the failure shape — loads, reports success, plays silence — is E27's and E63's shape, and both of those shipped.

**It also fixed the probe's ordering**, which is the host's: create and initialise the controller, connect both `IConnectionPoint`s, `controller->setComponentState`, then `component->setState`. Restoring the component's state before the controller's `initialize()` builds the rack against an empty factory.

### The other thing a bare VST3 host must supply, or it measures its own omission

DSP→UI traffic in this wrapper is `IMessage`-based: `Processor_VST3.cpp`'s background thread calls `allocateMessage()` and `if (!message) break;`. `allocateMessage()` resolves through **`IHostApplication`** on the host context — so a host that does not offer one **has no DSP→UI channel at all**, and a probe without one would have "measured" a dead channel and blamed the plug-in. The probe implements a minimal one (`IMessage` + `IAttributeList`, only the `setInt`/`getInt` and `setBinary`/`getBinary` the wrapper actually uses) and offers it in **every** arm, so it is never a variable between them. It prints `host allocated N IMessage(s)` — **571** in the editor arm — so a zero would be visible rather than inferred.

This is a deliberate difference from the CLAP probe, where the `clap.gui` host extension *is* gated behind `--editor`. There it had to be, to keep the control arm byte-identical to the host an earlier run's figures came from; here there was no earlier bare-host figure to stay comparable with.

### THE BUILD TRAP THAT COST THIS RUN THE MOST, and it is the machine, not the code

**`C:\SE\GMPI` and `C:\SE\gmpi_ui` moved to different commits WHILE THIS RUN WAS BUILDING.**

My own survey read `GMPI` at `cf7504b` at the start. Twenty minutes later the same command read **`9461fa9`, seventy-seven commits behind its own `origin/main`**; `gmpi_ui` read **407 behind and 2 ahead**. The developer was working at the machine when the run started (`devenv` on `Simulator2 — ParticleMgr.cpp`, twice, plus Chrome), and had gone by the time the second build failed.

The resulting set **does not compile together at all**:

```
ModuleView.h(345,31): error C2039: 'IPinsCallback': is not a member of 'synthedit'
UG2.h(8,10): error C1083: Cannot open include file: 'Extensions/ParameterIterator.h'
SerializationHelper_XML.h(49,10): error C1083: Cannot open include file: 'experimental/observable.h'
Base64.h(22,10): error C1083: Cannot open include file: 'Core/base64.h'
```

**Every one of those headers exists on those repos' `origin/main`.** `SynthEditLib` was current and `GMPI` was not, so the failure reads as broken code in `EditorLib` and is nothing of the sort.

**The fix is to stop using the overrides.** Configuring with **no** `*_FOLDER_OVERRIDE` at all (plus `CMAKE_GENERATOR_INSTANCE`, which this box still needs) makes CMake fetch all eight dependencies fresh; that configure and build were **rc=0, zero errors**. `docs/lessons.md` already carries *"a `*_FOLDER_OVERRIDE` build reads a live working tree, so another session's uncommitted work lands in your test results"* — that is the weaker case. This is the checkout's **commit** moving underneath you, and it does not need anyone to be careless: the developer switching branches in his own repo is enough.

**Second trap, cheaper but confusing.** A brand-new build tree throws `error C1083: Cannot open compiler generated file: '...\X.obj': Permission denied` on freshly created object files. **16 files failed on the first pass and 0 on the second.** Defender real-time monitoring is on (`(Get-MpPreference).DisableRealtimeMonitoring` → `False`). It reads like a broken build and is a scanner holding a new file; re-run the identical command before believing it.

### The developer, and what this run did about him

`Get-Process | Where-Object { $_.MainWindowTitle }` at the start: **two Visual Studio instances on `Simulator2 — ParticleMgr.cpp`**, Chrome playing music. By mid-run they were gone. **Third run in a row to find him there**, and the first where the repos he was in were the ones this run depends on.

What this run did: worked entirely in a `git worktree` at `C:\SE\TideSynth-wt-e80`, never checked out or built in `C:\SE\TideSynth`, launched no host and displayed no window (the `--editor` arm's parent is the same never-shown off-screen `WS_POPUP` the CLAP probe uses). **No REAPER, no `%APPDATA%` backup, no standalone.** The process list was unchanged across every arm.

**Learned:**

- **When the question is *which format*, build both formats from ONE configure.** Quoting a CLAP number from one build day against a VST3 number from another leaves the build as a second variable, and this row spent three runs resting on a cross-day comparison nobody had flagged. Rebuilding the CLAP arm cost about six minutes and turned "different formats differ" into "these two differ by 25 bytes".
- **A `*_FOLDER_OVERRIDE` build reads a live checkout's COMMIT, not just its dirt.** Two sibling repos moved mid-run and produced a set that cannot compile. Drop the overrides when you need a build you can trust; the fetched configure is immune and costs one cold build.
- **When a probe and a plug-in disagree about what "a plug-in instance" is, the probe is wrong and it will not say so.** CLAP's one-object model does not transfer to VST3's two. The tell was not an error — it was `RackProcessor: '<slug>' constructed` appearing 0 times where a sibling probe showed 29.
- **A bare host must supply what the wrapper's channel is built on, or it measures its own omission.** Without `IHostApplication` there is no VST3 DSP→UI channel at all, and the probe would have blamed the plug-in for its own missing interface. Print the count so a zero is visible.
- **The heredoc backslash trap is still live on this box, and cost three failed patches.** `\n` inside a `<<'PY'` heredoc arrives as a real newline, so a multi-line Python pattern matching C source containing `\n` silently finds nothing. The 2026-09-10 entry recorded this and I hit it anyway. The reliable fix is not to escape harder — it is to stop routing the patch through a shell.
- **A negative control that produces zero of everything is the cheapest line in the table.** `--no-preset`: 0 sends, 0 captures, 0 modules. Without it, "569 small sends" is also consistent with a plug-in that always sends 569 small things.

**Not verified:** **the standalone** — the quoted figure named it alongside VST3 and it is the one instrument here that must own a real window, so it is still unmeasured and needs an idle box. **Anything requiring the editor to have PAINTED** — an invisible window gets no `WM_PAINT`; `RackEditor: render #N` is **0** in every arm here, as it was on 2026-09-10. **Which layer drops the blob** — `UPlug::Transmit` is a location, not a cause, and nothing this run did narrowed it further. **macOS and Linux** — nothing was re-measured there; the VST3 probe is win32-only by construction (it `LoadLibrary`s a bare DLL; mac and linux load a .vst3 *bundle*) and says so rather than pretending. **E86's reachability in any real host** — argued both ways in its row, measured neither. **`SynthEditCL` and SynthEdit proper** — not built; nothing this run changed is outside TideSynth, and no plug-in source was edited at all. **The `build-e80fmt` tree's numbers** — that override build never linked and produced nothing; every figure above is from `build-e80iso`, the fetched-dependency tree.

**Machine state.** Worked entirely in a `git worktree` at `C:\SE\TideSynth-wt-e80`; `C:\SE\TideSynth` was never checked out or built in and is on `main` at `13095a395`, clean, exactly as found. **`C:\SE\GMPI_Wrappers` carries one dirty file, `wrapper/AU3/AU3_Wrapper.mm`, which is PURE CRLF CHURN** — 820 insertions, 820 deletions, and `git diff --ignore-all-space` is empty. It is the developer's, it predates this run, it is macOS-only and irrelevant to everything here, and it was **left exactly as found**: not committed, not reverted, not stashed. `SE16`, `SynthEditLib` and `SynthEdit_Rack_Adaptor` clean and untouched. **`C:\SE\GMPI` and `C:\SE\gmpi_ui` were moved by the developer during the run, not by this run** — nothing here fetched into them beyond a read-only `git fetch`, and neither was checked out, reset or fast-forwarded. **By the end of the run they had moved BACK** — `GMPI` `cf7504b`, `gmpi_ui` `1baf360`, both clean and both matching what the run's opening survey saw. `SE16` moved too, `04ca84498` → `7eb3149ea`. **That is the part to carry forward: the drift is transient and reverses, so checking the sibling shas ONCE at the start of a run proves nothing about what a build twenty minutes later will read.** Check them at build time, or use the no-override configure and stop caring. Dependency shas the measurement was actually built against are **whatever TideSynth's `main` pins**, not the local checkouts, because the build used no overrides — which is the whole point of that choice. **No host, DAW or standalone was launched, nothing was installed or registered, and no window was displayed**; both probes are bare hosts that load the plug-in binary out of a scratch build tree. Two gitignored build trees (`build-e80fmt`, the failed override one, and `build-e80iso`) and both probe executables are left in the worktree, which is removed at STEP 5; every log is in the session scratchpad, outside all repos.

**Next:** **E85 is the obvious next windows pick** — small, ALLOWED, self-contained, and its Accept is already wired: `Processor_CLAP.h` overrides nine `gui*` methods and neither `guiShow` nor `guiHide`, and the CLAP probe's `--editor` arm prints `FAIL  gui->show succeeds` today. **Then E86**, whose own row says to read it first because the answer may be a written ruling rather than a code change. **E80 itself now wants a GATED filing, not another measurement** — three runs have measured this channel from both ends on two formats and the only lead left is `UPlug::Transmit` (`SynthEditLib/UPlug.cpp:1036`), which forwards only to `connections` and has no GUI branch. **Do not build with `*_FOLDER_OVERRIDE` on this box unless you have checked the sibling repos' shas against their origins first**, and prefer the no-override configure regardless. **`JOURNAL.md` rotation is still not this lane's to do** — [#585](https://github.com/JeffMcClintock/TideSynth/pull/585) is macOS's and its whole subject is the rotation rule.

**Branch/PR:** `tide/win/E80-clap-editor-arm`, [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) — the new [tests/e80_vst3_feedback_probe.cpp](tests/e80_vst3_feedback_probe.cpp) and `build-e80vst3probe.cmd`, E80's row with the seven-arm measurement, **E86** filed, the re-pointed `win` NEXT cell, the bare-VST3-host section in [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), regenerated `docs/lessons.md`, and this entry.

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

## 2026-09-10 — windows — E80: the editor is not the variable, and the arm that says so took no screen (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.46388.4.0** · as **tide-rack-bot** (both paths) · scheduled run

**Did:** took **E80** — the win lane's own NEXT pick — and closed the confound the 2026-09-09 run left open, by giving `tests/e80_clap_feedback_probe.c` an `--editor` arm that creates the **real CLAP editor inside an invisible, off-screen parent window**. Row stays TODO; no fix, and none attempted. **The developer was at the machine again, and this run still measured a GUI condition** — which is the part of this entry worth more than the numbers. Filed **E85** out of it.

### The one thing yesterday could not do

The 2026-09-09 entry ends on this, in its own words: *"This probe creates no editor. E80's linux measurement had one … a Windows CLAP with an editor open is still unmeasured."* Its row said the next run should measure the same two counters with an editor present, *"because that single arm decides whether this is a CLAP question at all"*.

That arm looked like it needed an idle box. It did not.

```c
CreateWindowExA(0, kProbeWndClass, "tide e80 probe (never shown)",
                WS_POPUP | WS_CLIPCHILDREN,
                -32000, -32000, (int)w, (int)h,
                NULL, NULL, GetModuleHandleA(NULL), NULL);
```

No `WS_VISIBLE` and never shown, so it cannot take focus or raise itself over anyone's work; no owner and no `WS_EX_APPWINDOW`, so no taskbar button; off-screen origin as belt and braces. **The plug-in's window is created as a CHILD of it, and a child of a window that was never shown is not shown either.**

**The editor genuinely runs in there.** `gui->create`, `gui->set_scale(1.0)`, `gui->get_size` → **1100x600** and `gui->set_parent` all return true, **one child window** appears under the parent, `IsWindowVisible(parent)` is **0**, and the editor resolves model and art for all five modules in the fixture — `RackEditor: 'Scope' model=yes art=yes(res/Scope.svg) art-size=195x380`. `Get-Process | Where-Object { $_.MainWindowTitle }` was **unchanged across all four arms**, with Jeff's own applications in front throughout.

**What it cannot see, stated so nobody quotes it wrongly:** an invisible window gets no `WM_PAINT`, so `RackEditor: render #N` stays at **0** and nothing that depends on the editor having actually PAINTED is observable this way. E80's counters are on the observable side — `RackEditor.h:292` raises `display-state update #N arrived` from the pin-set path, not from `render()`.

### The measurement

Same plug-in binary (`build-e19win/SynthEditSem/Release/TIDE-Rack.clap`, built 2026-09-09), same fixture, same 800 blocks of 512 at 44.1 kHz, `TIDE_FEEDBACK_TRACE_EVERY=1`. **The `--no-editor` row IS yesterday's measurement re-run, and it reproduces to the byte** — which is what licenses reading the rest of the table as one variable moving:

| arm | feedback sends | **largest send** | `display-state capture` | `RackEditor:` lines |
|---|---|---|---|---|
| `--no-editor` (control) | 569 | **337 bytes** | `#200 (65548 bytes)` | **0** — no editor exists |
| `--editor` | 569 | **337 bytes** | `#200 (65548 bytes)` | 38 light, **2 display-state, both `(0 bytes)`** |
| `--editor --no-pump` | 569 | **337 bytes** | `#200 (65548 bytes)` | **15** light, 2 display-state |
| `--editor` on `e83-vcv-scope-cabled.xml` | 569 | **337 bytes** | `#200 (65548 bytes)` | 38 light, 2 display-state |

**The editor is not the variable. Not one byte moves.**

And **the far end is now observed on this platform for the first time**: `RackEditor: display-state update #0 arrived (0 bytes)`, `#1 arrived (0 bytes)`, **and nothing after**. Both land during editor construction, at log lines 23 and 33 — *before* the first `display-state capture` at line 63. So after the DSP starts capturing, the editor receives **zero** further arrivals. It is not "frozen at a stale value"; nothing ever arrives. That is E80's linux symptom, reproduced on Windows in a bare host with no DAW, no GTK and no compositor.

**Read `--no-pump` as a positive control, not as a repeat.** Starving the main thread visibly halves editor-side light traffic — **38 → 15** — and moves the DSP-side counters not at all. So the starvation is real and is visible exactly where it should be, and "the bare host starved something" is not available as an explanation for the blob.

### Two hypotheses die, both by measurement

- **Dedup, refuted by ORDERING — which is stronger than the cabled-fixture argument that preceded it.** `RackProcessor: 'Scope' display-state capture #0 (65548 bytes)` is logged **before** `TIDE: instance #1 feedback send #0 (137 bytes, 0 held back)`. A *first* send has nothing to be deduped against, so `ControlPin::setValue`'s `if(value != value_)` (GMPI `Core/Processor.h:78`) cannot be what stops it. Yesterday's refutation rested on the cabled fixture making the payload genuinely vary; this one does not need that premise, which matters, because a Scope fed a constant `1.000000` may well draw a constant picture.
- **Queue capacity.** `queDspToUi` is `SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE` = **`0x500000`, 5 MB** (`SynthEditLib/SeAudioMaster.h:341`) against a 65,548-byte payload, and **all 569 sends report `0 held back`**. For scale: **the queue's entire lifetime traffic over 9.288 s is 59,903 bytes across 569 sends, mean 105.3 — less than one picture.**

### The positive control is inside the same module, and it is what makes this a location

Lights and the display-state blob are **both `gmpi::editor::PinBase` GUI pins on the same module** (`RackEditor.h:254` and `:285`), set in the same block by `RackProcessor::sendLights` and `sendDisplayState`, through the same DSP→GUI pin mechanism. **The floats arrive — `light 1 update #1100 value 0.281`, varying — and the blob does not.** So this is not a dead route, and it is not E74's unbound editor.

**Where the path goes, read out rather than guessed:**

| step | file |
|---|---|
| `displayStatePin->setValue(displayStateBytes, …)` | `SynthEdit_Rack_Adaptor/RackAdaptor.h:676` |
| `ControlPin::setValue` → `sendPinUpdate` | `GMPI/Core/Processor.h:76-83` |
| `PinBase::sendPinUpdate` → `plugin_->host->setPin(...)` | `GMPI/Core/Processor.cpp:188` |
| `ug_plugin3Base::setPin` → `pin->Transmit(timestamp, size, data)` | `SynthEditLib/ug_plugin3.cpp:62` |
| `UPlug::Transmit` | `SynthEditLib/UPlug.cpp:1036` |

**`UPlug::Transmit` caches the value into `currentRawValue` and then forwards it only to `connections` — there is no GUI branch in it at all.** That is where the next run should look. **It is GATED** (`SynthEditLib/`), so the likely outcome is a filing, not a fix.

### What this does to the row's own question

**Every step in that table is upstream of every wrapper**, inside TIDE's inner rack. On this evidence E80 is **not** a CLAP question and the row is mis-framed as one.

**The one measurement that would settle it is still missing, and it is now the next arm:** the same two counters, with `TIDE_FEEDBACK_TRACE_EVERY=1`, on the Windows **VST3 or standalone**. The *"65,673 bytes on VST3 and in the standalone"* figure this whole row rests on was read off the **old 1-in-100 cadence**, in REAPER, with an editor — and yesterday's entry has already shown once that a figure read off that cadence could not have been right. If it does not survive a full trace, then no format ever carried the blob.

**It was not measured today, for a stated reason.** `Get-Process | Where-Object { $_.MainWindowTitle }` returned `devenv` debugging **SynthEditStore** on `LegacyTextEditAdapter.h`, `SynthEdit2` running a document, plus Chrome, Outlook and Slack. The standalone is the one instrument in reach that must own a real window, so it stays for an idle box — **or, better, for a bare VST3 host, which would do for VST3 what `e80probe` now does for CLAP** and would retire the constraint permanently rather than waiting it out.

### E85, which fell out of the arm in its first thirty seconds

**`clap_plugin_gui.show()` returns false on TIDE, on every platform.** `Processor_CLAP.h:237-249` overrides nine `gui*` methods and **neither `guiShow` nor `guiHide`**, so both take `clap::helpers::Plugin`'s defaults — literally `virtual bool guiShow() noexcept { return false; }` (`clap_helpers` `plugin.hh:304-305`). The probe prints `FAIL  gui->show succeeds` while everything around it succeeds and the editor demonstrably builds. **So the editor works and the API says it did not.** No DAW had reported it because no DAW checks the return; a bare host does, which is the argument for bare hosts in one sentence. Distinct from E79, which concerns the host timer `Editor_CLAP.cpp:269` registers in `guiSetParent`.

### The developer was at the machine, and this run never touched his tree

Second run in a row to find him working. What was done differently: **this run did not build in `C:\SE\TideSynth` at all.** It created a `git worktree` at `C:\SE\TideSynth-wt-e80` from `origin/main` and worked there, for a specific reason — **local `main` in his checkout is one commit AHEAD of `origin/main`** (`e2d344e1f TiDEknob: draw the editor-guide outline via IDrawingLayer layer 4`, unpushed), and `git checkout -b … origin/main` in that tree would have reverted `TiDEknobGui.cpp` under an open editor. His tree also carries two untracked files in `RackModules/`, untouched.

The measurement then used the plug-in binary **already** in `build-e19win`, which is a virtue rather than a shortcut: it is the identical binary yesterday measured, so the control arm reproducing to the byte is a real check on the instrument rather than a coincidence, and the only variable between the rows is the editor.

**The corollary is the same one yesterday recorded, and it still applies:** that binary was built from a tree carrying his uncommitted edit. It is a knob's drawing-layer override and cannot touch the rack-feedback channel, so the numbers stand — *despite* it, not *unaffected* by it.

### A committed build recipe contained raw control bytes

`docs/ci/headless-gui-verification.md`'s Windows build block read:

```
call "C:\Program Files\Microsoft Visual Studio<0x01>8\Community\VC\Auxiliary\Build<0x0b>cvars64.bat"
```

`\2022\` and `\vcvars` had been through something that interpreted them as escape sequences, leaving **actual `0x01` and `0x0B` bytes in the file**. A markdown renderer shows that as a plausible path. Fixed, along with the advice above it: `cmd //c` from Git Bash **does not work** for a `.cmd` in the current directory (`is not recognized as an internal or external command`) because the doubled-slash rewrite does not also supply the `.\`; `cmd /c ".\build-e80probe.cmd"` from the PowerShell tool does.

Worth noticing that **I hit the same class of bug three times while writing this patch** — `\n` inside a heredoc arriving as a real newline and breaking C string literals. The habit that fixed it: build literal backslashes as `chr(92)` and assert the result, rather than counting escape levels across bash → python → C.

### What is in the tree now

| file | what |
|---|---|
| [tests/e80_clap_feedback_probe.c](tests/e80_clap_feedback_probe.c) | `--editor` / `--no-editor`; a `clap.gui` HOST extension wired in **only** in the editor arm, so `--no-editor` is byte-identical to the host yesterday's figures came from; the invisible parent; teardown ordering `hide` → `destroy` → `DestroyWindow` |
| [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md) | the invisible-parent recipe, the four-arm table, what an unpainted editor cannot tell you, and the corrected build block |
| `BACKLOG.md` | E80 cell; **E85** filed; win NEXT cell re-pointed |
| `build-e80probe.cmd` | the two-line build, so the next run does not retype it |

**Learned:**

- **A GUI arm does not need a GUI.** An embedded editor only needs a parent HWND and a message pump, and neither has to be visible. This is the windows analogue of what a headless probe bought mac and linux, and it means a scheduled run on an occupied box is no longer restricted to non-GUI questions. Say what it cannot see (`WM_PAINT`, hence `render()`), and check `RackEditor: render #N` to know which side of that line a figure sits on.
- **Prefer refuting a hypothesis by ORDERING over refuting it by fixture.** Dedup was argued away yesterday by making the payload vary; it is *shown* away today by the first capture preceding the first send, which needs no assumption about the payload at all. When a control requires a premise, look for the one that does not.
- **Work in a `git worktree` when the developer's checkout is ahead of `origin`.** `git checkout -b … origin/<default>` silently reverts his committed-but-unpushed files in a tree he has open. A worktree costs one command and touches nothing.
- **Reusing yesterday's binary is a feature, not a shortcut** — it turns the control arm into a check on the instrument. A rebuild would have made "identical to the byte" unremarkable rather than evidence.
- **A bare host reads return values that no DAW checks.** E85 existed on every platform for as long as the CLAP wrapper has, and was found in the first thirty seconds of the first bare host that created an editor.
- **Read a lint's WHOLE output and its EXIT CODE, not its tail.** `check-id-refs.py` was run locally before the commit and its last lines were an advisory about `E2`, so it read as clean. It had exited **1**, and above the advisory it was rejecting E85 for citing `Editor_CLAP.cpp:269` — a line E79 already cites. CI caught it in seven seconds ([run 34401532173](https://github.com/JeffMcClintock/TideSynth/actions/runs/34401532173)); `| tail -5` is what hid it. The rule is sound and the fix was to drop the citation from the newer row, since the location belongs to E79.

**Not verified:** **the Windows VST3 or standalone with a full trace** — the arm that would say whether any format ever carried the blob, deliberately not run because the developer was at the machine. **Anything requiring the editor to have PAINTED** — no `WM_PAINT` reaches an invisible window, `RackEditor: render #N` is 0 in every arm. **Which layer drops the blob** — the path is now traced to `UPlug::Transmit`, and that is a location, not yet a cause. **macOS and Linux** — nothing here was re-measured there; the `--editor` arm is win32-only by construction and prints so on other platforms, where `tests/e78_clap_gui_probe.c` already exists. **E85's fix** — filed, not attempted. **`SynthEditCL` and SynthEdit proper** — not built; nothing this run changed is outside TideSynth, and no plug-in source was recompiled at all.

**Machine state.** Worked entirely in a `git worktree` at `C:\SE\TideSynth-wt-e80`, removed at the end; `C:\SE\TideSynth` was never checked out, never built in, and is on `main` with the developer's unpushed `e2d344e1f` and his two untracked `RackModules/` files exactly as found. No other repo was modified. **No host was launched and no window was displayed** — verified by the process list being unchanged across the run, not assumed.

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
