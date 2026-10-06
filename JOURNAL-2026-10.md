# Journal — October 2026 (archive)

Rotated out of [JOURNAL.md](JOURNAL.md) by **A8**, 2026-08-12. Newest first,
same as the live file. **Entries here are verbatim** — archiving never edits an
entry, so this is the record.

October 2026 is split between two files: the most recent entries stay in
[JOURNAL.md](JOURNAL.md) and move here as later runs rotate them out. Read
[JOURNAL.md](JOURNAL.md) first; come here only when you need history older than
the entries it still holds.

---

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

## 2026-10-01 — windows — the queue reopened: all eleven PRs merged, A41 taken and measured, and the probes' verdict had THREE causes rather than one (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.16120.0** (CLI `2.1.284`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **A41** and shipped option (a) — both A38 probes now take their fleet state as a pinned input. Then the STEP 4 bookkeeping the merge sweep made due: eight landed rows archived, and `JOURNAL.md` rotated for the first time since 2026-09-08. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded.

### The thing that changed everything about this run: Jeff merged the entire queue

`gh pr list --state open` is **empty**. Eleven PRs landed between 00:22 and 01:09 UTC — #622, #618, #585, #614, #589, #588, #597, #590, #587, #586, #584 — so **thirteen consecutive cells of "STEP 1.5 and nothing else" ended in one sweep**, on both this lane and mac's. The only remaining `tide/**` ref is `tide/mac/issue-599`, whose [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) Jeff closed unmerged exactly as five mac cells asked.

STEP 1 was empty and, unusually, **verifiable**: `main` at `be3930ec3` is `success` on `windows`, `macos`, `linux`, all three `render-*`, `guard` and `digest`. The structural caveat is unchanged — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing, so an empty `gh issue list --label platform:win` verifies nothing on its own. STEP 1.5 had nothing to do: zero open PRs.

### STEP 2: A41, which the `win` NEXT cell already named and which `check-next-block` agreed was live

A35 is parked on its own two open `PROPOSED:` entries. **A37 I read rather than inherited, and ruled it ineligible on my own reasoning**: the open `PROPOSED:` entry *"Should the fleet's two bookkeeping hot spots stop being single shared files"* offers options (b) and (c) that move the NEXT block into per-platform sections or `docs/next/<platform>.md` — which is A37's entire scope. That entry's own *"May proceed meanwhile: EVERYTHING, without exception"* clause rests on the reasoning that the question is about *where a run writes* and not *what a row builds*; A37 is the one row where that does not hold, because what A37 builds is where the NEXT cells live. So STEP 2's "identical under every open answer" test fails for it.

### A41, and the row named one cause where there were three

The row's finding was that the probes' verdict flips on fleet movement. True, and two more defects were sitting underneath it. All three are now stated in `tests/a38_fleet_state.py`'s docstring, which is where a reader of the probes will be.

**1. The probes had TWO ref sources, and they can disagree.** `branches()` enumerated from `ls-remote --heads origin` — the remote, live — while every read resolved `origin/<branch>`, which is whatever the local clone last fetched. At **one instant, from one `origin/main` sha (`be3930e`), with the probe blob byte-identical**, `a38_row_adjacency_probe.py` arm 1 reported:

| repo | `BACKLOG.md` verdict |
|---|---|
| `C:\SE\TideSynth` (full clone, 14 stale `origin/tide/*` refs) | `3 distinct, 2 of 2 differ` — **MAXIMALLY DIVERGENT** |
| a fresh `--depth 1 --single-branch` clone | `1 distinct, 0 of 2 differ` — **inert on most branches** |

The two most opposite verdicts the script can print. Arm 2 in the same fresh clone said my branch touched `(none)` of `BACKLOG.md` when it demonstrably touched L73, and C1 reported `0 (branch, file) pairs checked, 0 mismatches` — a control that checked nothing, printed as a pass.

**2. A ref it could not resolve was skipped in silence, and the skip was ASYMMETRIC.** `blob()` passes `check=False` and returns `None`; the divergence loop skipped the `None` **while `len(allb)` still counted that branch in the denominator**. So an unfetched branch quietly moved the numerator down and left the denominator alone — which is precisely what manufactured `0 of 2` above instead of an error.

**3. An empty fixture read as a pass, and this one I measured on `main`'s own blob before changing anything.** With one branch in the lane there are no pairs, so C2 — the discriminator, the control that makes arm 3 a measurement — held because nothing was tested:

| | controls | last line | rc |
|---|---|---|---|
| `main`'s blob, live fleet, today | `C1 OK  C2 OK  C3 OK` | `PROBE OK` | **0** |
| this branch, same fleet | `C1 OK  C2 VACUOUS  C3 VACUOUS` | `PROBE VACUOUS` | **3** |

That is A39's trap (*"a gate that derives its expectation from the subject passes vacuously and looks identical to a working one"*) inside the A38 probes, and A41 was the inverse half of the same family.

**The fix.** `tests/a38_fleet_state.py` resolves one input for both probes. Live mode takes each sha from the **same `ls-remote` answer** as the name and never consults `origin/<branch>`. A ref the input names and the repo lacks is a hard stop (rc=2) printing the exact `git fetch` that repairs it, or `--fetch` runs it. A control that held vacuously prints `VACUOUS`. Both probes now use **four exit codes** — 0 measured, 1 control failed, 2 input unresolvable, 3 vacuous — because A41's whole complaint is that one code carried all four meanings.

**`refs/pull/<N>/head` is what makes a pinned default possible at all.** All seven branches the 10-01 cell measured were deleted when Jeff merged them, so `refs/heads/tide/win/**` would not have survived the week. GitHub keeps the pull refs: `git ls-remote origin refs/pull/618/head` still answers `cc025a2be`, and a shallow clone can fetch it.

**Verification: a recorded run reproduces byte-for-byte across two independent repositories.** Pinned, the row-adjacency probe measures a real fixture — 7 branches, 21 pairs, 35 `(branch, file)` pairs in C1, all three controls exercised — and the output hashes identically from the developer's tree and from a fresh shallow clone that had to fetch all eight refs:

| run | repo | rc | sha256 (line-endings normalised) |
|---|---|---|---|
| 1 | `C:\SE\TideSynth` worktree | 0 | `d644b1fa0dcba1ab…` |
| 2 | same, immediately again | 0 | `d644b1fa0dcba1ab…` |
| 3 | fresh `--depth 1` clone, `--fetch` | 0 | `d644b1fa0dcba1ab…` |

### The lane sweep: `0 of 5040` reproduces, the DEPTH HISTOGRAM DOES NOT, and the input that moved it NO LONGER EXISTS

Pinned to `main = 57a1bf593` (the sha this run's STEP 0 fetch recorded for the 10-01 cell) the lane sweep reproduces the recorded conclusions exactly — `7 of 7 merge into main individually`, `orderings landing ALL 7: 0 of 5040`, and `after #622 blocks nothing` / `after #618 blocks nothing`, the two zero-diff branches. **But the histogram differs from the one the 10-01 entry records:**

| | depth histogram | deepest order |
|---|---|---|
| recorded, 2026-10-01 entry | `{1: 2400, 2: 1920, 3: 720}` | `#622 -> #618 -> #614` |
| pinned to `main = 57a1bf593` | `{1: 1920, 2: 1824, 3: 1008, 4: 288}` | `#622 -> #618 -> #590 -> #587` |

Same seven PR heads. **And the branch set is NOT the variable: all seven tips are byte-identical to what is pinned**, checked `refs/pull/<N>/head` against this box's own `refs/remotes/origin/<branch>` one at a time. **And `main` is not the variable either.** I ran the pinned sweep against three candidate `main`s — `57a1bf593` (that cell's own fetch target), `7c92c3a8e` (#627) and `c6a3c0ee0` (#628), the only commits it could have been — and **all three give the identical histogram**, `{1: 1920, 2: 1824, 3: 1008, 4: 288}`. Their `docs/decisions.md` blobs are the same (`d7494d564`), so there was nothing for `main` to change.

**What settles it is a path in the recorded matrix that CANNOT conflict among the commits I can still recover.** The recorded depth-2 matrix names `docs/decisions.md` on five rows; mine names it on none. The reason is exact: four of the seven branches — #597, #590, #587 and #586 — carry `docs/decisions.md` at the **byte-identical blob `5da644d6a`**, against a merge base holding `d7494d564`. An identical change on both sides of a three-way merge is not a conflict, so no pair drawn from those four heads can produce the recorded line, under any `main`.

**So the heads the sweep measured are NOT the heads I can recover.** `refs/pull/<N>/head` is the branch head at **merge** time, and the sweep ran hours earlier; the branches were then pushed to again before Jeff merged them, and this box's stale `refs/remotes/origin/*` refs — never pruned, so they survived the deletion — agree with the pull refs rather than preserving the earlier tips. Nothing anywhere recorded them. **The recorded histogram is therefore unreproducible from any input that still exists**, and no amount of pinning fixes that after the fact: what A41's option (a) buys is that the NEXT such measurement is reproducible, not that this one can be recovered.

**This is the sharpest possible statement of A41's thesis, and I did not expect to find it:** a recorded measurement in this repository cannot be reproduced **at all** — not by pinning every branch, not by trying every candidate `main`, because the commits it actually measured were overwritten before anyone thought to name them. Its *conclusions* survived (`0 of 5040`, `after #618 blocks nothing`); its distribution is gone. A41's fix is prospective by nature.

### STEP 4's other half: the bookkeeping the merge sweep made due

**Eight `IN-REVIEW` rows whose PRs had all merged** — A36, A38, A39, A40, E72, E79, E80, E81 — are archived to `BACKLOG-DONE.md` with `Done = 2026-10-01`. `BACKLOG.md` **369,423 → 317,545 bytes**, 62 table rows → 54. (The archive move alone took it to 303 KB; my own `win` cell and A41 row additions put 14 KB back, which is the cost of this entry's own bookkeeping and is worth naming.) Every cited PR was checked individually, not inferred: all `MERGED` except A39's citation of #604, which is `CLOSED` and is a reference to a related PR rather than A39's own work ([#614](https://github.com/JeffMcClintock/TideSynth/pull/614) is A39's and merged).

**Two traps, both of which a future archiving run will hit.**

**A bare `DONE` row left in `BACKLOG.md` FAILS `check-backlog-archived.py`** — so the flip and the move are one edit, not two. `check-backlog-diff.py` permits an archive move and never requires one, which is how the gap existed; E45's script is what closes it.

**Archiving a row turns every HISTORICAL take clause naming it into a stale take-target, and `check-next-block.py` cannot tell a lane's live instruction from the `Previous cell follows.` chain below it.** Archiving these eight made the `mac` and `linux` cells red on three clauses naming A39, E79 and E80 — **all three in chain history**, one of them written on 2026-09-01. The fix that keeps the chain intact is a negation **in the same sentence as the verb**, because `RE_NEGATED` is scoped per sentence and `sentences()` splits on ` -- `: `(LANDED 2026-10-01 as #614 and archived, so do not take it)` works, while `-- landed, do not take` does not. **And I tripped the same lint with my own prose**, describing the three clauses by quoting them; rewording to *"three clauses naming A39, E79 and E80"* fixed it. That is A37's growth curve biting a second lint, and **the `mac` NEXT cell is now 61,508 bytes on one line** — A37 measured 34,988 on 09-10, A38 measured 41,216 on 09-18, so **+49% in 13 days**.

**`JOURNAL.md` is rotated, for the first time since 2026-09-08.** It had reached **470,307 bytes across 43 entries** — A36 landed the rule that stopped it rotating itself out, and the rule was then visible but unapplied for three weeks. **470,307 → 53,277 bytes, 43 entries → 6**, with 38 moved into `JOURNAL-2026-09.md` (09-29 back to 09-08). The floor is 4 — the entries carrying the most recent date, 2026-10-01, now that mine is one of them — and the 60 KB ceiling bound first at 6, so the floor never came into it. I targeted **60,000 bytes rather than 60 KiB**, because *"under 60 KB"* is ambiguous and 7 entries came to 60,514 bytes: under 60 KiB, over 60 kB. **Lossless, checked by heading sets over all three journal files: 482 → 483 unique headings, 0 missing, 0 duplicated, and the one addition is this entry.** `check-journal-prepend.py` confirms it independently — `1 new entry prepended`, 38 rotated out and each verified verbatim in the diff. **And A36's companion warning held:** `extract-lessons.py` sees the archive by glob, so moving 38 entries into it did not drop their lessons — `--check` reports **1,622 lessons from 379 entries**, up from the 1,534 / 364 the 10-01 cell recorded, with **0 date sections removed**.

**One thing in the rotation rule that does not hold, left for a ruling rather than fixed.** The rule says to append moved entries **below** what is already in the archive *"so the archive stays newest-first"*. The rationale is false in general and false here: `JOURNAL-2026-09.md` runs 09-08 → 09-01 newest-first, and the batch I moved (09-30 → 09-08) is **newer than everything in it**, so appending below cannot keep newest-first. I followed the instruction as written, because `JOURNAL-2026-08.md` shows previous runs did the same — its dates are **not** monotone — and inventing a prepend convention mid-run would diverge from what the other two boxes will do. The discrepancy is recorded here rather than filed as a row, because the `PROPOSED:` entry on the bookkeeping hot spots already covers the journal's layout and its **`Decide-by` was *"the next `JOURNAL.md` rotation"*, which this run has now done.**

**Learned:**

- **An unrecorded input can become unrecoverable, and then the measurement is simply lost.** A41 was filed about the branch set. Pinning all seven branches did not reproduce the recorded histogram, nor did any of the three candidate `main`s — because `refs/pull/<N>/head` preserves each branch's head at MERGE time and the sweep ran against earlier tips that were pushed over. The tell was a path in the recorded output (`docs/decisions.md`) that provably cannot conflict among the commits that still exist. **Record every ref at the moment you measure**; afterwards there may be nothing left to pin.
- **A silent skip and a hard failure differ most when the input is partly available.** `blob()` returning `None` for an unresolvable ref moved arm 1's numerator while leaving its denominator alone, so a half-fetched clone produced not an error but a *confident opposite answer*. The asymmetry is the defect; `check=False` was the mechanism.
- **Archiving a row has a blast radius in the NEXT block's history, not just its present.** `check-next-block.py` reads a whole cell, and a cell carries a ten-deep chain of superseded instructions. Expect to defuse clauses written weeks ago by another lane, and expect your own description of the problem to trip the same lint.
- **A `DONE` row and an archived row are not two states but one edit.** `check-backlog-diff.py` permits the move without requiring it, and `check-backlog-archived.py` requires it — so flipping to `DONE` and stopping is the one combination that is red.
- **"May proceed meanwhile: EVERYTHING, without exception" is a claim about a question, and a row can still fall inside it.** The bookkeeping `PROPOSED:` entry says it makes no row ineligible, reasoning that it is about where runs write rather than what rows build. A37 builds where the NEXT cells live, so the reasoning does not reach it — read the *reason* a question is declared non-blocking, not just the declaration.
- **Thirteen cells of pure STEP 1.5 ended the moment a human merged**, which is what A38 measured and what its `PROPOSED:` entry's *"A run is the scarce resource here, not a merge"* says. Nothing in the fleet's own machinery ended it.

**Not verified:** no build and no host — nothing this run touched is compiled code. Neither probe is run by any workflow (`grep -rn a38_ .github/workflows/` is empty) and this run did not change that, so the pinned default makes a recorded run reproducible without putting either probe on a gate. I did not judge the Accepts of E19 or E82, whose rows read `TODO` with merged PRs — the X2 shape, left for the next win run with a note in the `win` cell. I did not resolve why `tide/mac/issue-599` still exists as a pushed branch with a closed PR; it is mac's lane and predates this run, though it is now the only input the probes' `--live` mode sees besides my own branch. The bot PAT's real expiry remains unverifiable from here (A40's `NEEDS-JEFF` half, still unanswered).

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and **never left it** — all work in `git worktree`s under the scratchpad plus two throwaway shallow clones, all removed. **All five repos were clean at the start:** `TideSynth` (`main`, behind 14), `SE16` (`master`), `SynthEditLib`, `gmpi_ui` (both `main`) and `GMPI_Wrappers` (`main`, behind 1); I touched none of the other four. The developer was at the machine throughout — Visual Studio on `SynthEdit_cmake` / `ModuleFactory_Editor.cpp`, plus Outlook, Slack and Settings — so **no GUI work and no screen taken**, which A41 needed none of. `git fetch` again warned *"too many unreachable loose objects"* in `C:\SE\TideSynth`; that is a local housekeeping note for Jeff (`git gc`), not a repository problem, and I did not run it on his tree. No credential value appears in any commit, PR, comment, journal entry or row.

**Next:** see the `win` NEXT cell. **For Jeff, three things:** (1) [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) is A41's code and cannot auto-merge — `tests/**` is not on the allowlist, by design. (2) The bookkeeping `PROPOSED:` entry's `Decide-by` has arrived: the rotation is done, so the question *"should the fleet's two bookkeeping hot spots stop being single shared files"* is ripe, and A37 is parked behind it with a 61.5 KB `mac` cell as the cost. (3) A40's `NEEDS-JEFF` half is still one question only the GitHub UI can answer — does the fleet PAT expire, and if so when?

**Branch/PR:** A41's code is on `tide/win/A41-probe-ref-pinning`, [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) — `BACKLOG.md` byte-identical to `main` there, so it conflicts with nothing. This entry, the A41 row flip, the eight archive moves, the rotation, the regenerated `docs/lessons.md` and the refreshed `win` cell are on `tide/win/2026-10-01-a41-bookkeeping`, which is bookkeeping-only and should auto-merge.

## 2026-10-02 — macos — E85: `clap_plugin_gui.show()`/`.hide()` now report success; A/B 3/3 on macOS, with the probe's editor arm ported to Cocoa (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.16120.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E85** and fixed it in `GMPI_Wrappers` ([#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41)). I also ported `tests/e80_clap_feedback_probe.c`'s `--editor` arm to macOS so its Accept can be read on this platform. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`25bf45e..d58bdd1`).

### STEP 1 / 1.5 / 2

STEP 1 was empty: no open `platform:mac` issue. The only open issues are #583 (linux) and #44 (the digest). STEP 1.5 was also empty, because Jeff merged the whole queue on 10-01 and `gh pr list --state open` shows only win's #629 (A41). So this lane's five-cell #585 livelock is over. In STEP 2, the mac NEXT cell said "walk STEP 2". Five rows reached `main` with the 10-01 merges and had never been walked by this lane: E85, E86, E87, E88 and E89. **E85** was the topmost eligible `any` row: small, ALLOWED scope (`GMPI_Wrappers/`), and an Accept that is a command. No remote ref or open PR named it. I claimed it on `tide/mac/E85-clap-gui-show` and pushed the claim before starting work.

### The fix

`Processor_CLAP.h` overrode nine `gui*` methods but not `guiShow` or `guiHide`. `clap_helpers` `plugin.hh:304-305` defaults both to `return false`. I added both overrides. They return `editor != nullptr`, with a comment explaining why: `guiIsApiSupported()` refuses `isFloating`, so the editor is always embedded, and the host shows or hides its own parent. That is +22 lines in `wrapper/CLAP/Editor_CLAP.cpp` and `Processor_CLAP.h`, and nothing else.

### Making the Accept observable on macOS

The Accept names the probe's `--editor` arm, which was **win32-only** (`#else` printed *"--editor is win32-only"*). I added an `__APPLE__` branch, written against the Objective-C runtime so the probe stays one `.c` file:

- `NSApplicationActivationPolicyProhibited` is set before any window exists, so there is no Dock icon and the process can never become active.
- The parent is a borderless `NSWindow` at (-32000,-32000) that is **never ordered front**. The plug-in gets its `contentView` as `clap_window.cocoa`. The probe prints `isVisible=0` before and after `show`.

`gui->hide`'s return value is now **checked** on every platform. The probe used to discard it, so half of E85's Accept ("`guiHide` implemented alongside") had nothing to read it with. Build line on macOS: add `-framework AppKit`. The header says so.

### Verification artifact: A/B, one tree, one variable

`cmake -S <ts worktree> -B <scratch>/bld -G Ninja -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS=<worktree>`. A points that variable at `origin/main` 3da5548 and B at the fix branch. Nothing else differs. Both builds completed with rc=0. Then `./e80probe <arm>/TIDE-Rack.clap --no-preset --editor --blocks 200`, three runs per arm, interleaved:

| arm | `gui->show` | `gui->hide` | rc | binary sha256 |
|---|---|---|---|---|
| A, `origin/main` | **FAIL** ×3 | **FAIL** ×3 | 1 | `af0a5378be2d781a…` |
| B, fix | **PASS** ×3 | **PASS** ×3 | 0 | `16b5adde3ebba2b9…` |

`diff A.out B.out` is exactly those two lines plus the summary line. In both arms `is_api_supported(cocoa)`, `create`, `set_scale(1.0)`, `get_size` (1100x600) and `set_parent` pass, and the editor adds **1 subview**, so A's FAIL is the API lying about a working editor, which is E85's claim reproduced on a second platform. `nm -C` shows `Processor_CLAP::guiShow()`/`guiHide()` exported only by B. Nothing appeared on screen: `isVisible=0` throughout.

**Learned:**

- **E85 was not Windows-specific, and a mac bare host reproduces it exactly.** The windows run that filed it could only see it through a win32 arm. The probe now has the same arm on Cocoa, so the next CLAP GUI question on macOS has an instrument.
- **An NSView parent can be made fully headless from C without a `.m` file.** Use `objc_msgSend` casts, with `setActivationPolicy:2` *before* the window exists, and never order the window front. Embedded editors still build and attach. I did not check whether they paint.
- **`FETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS` is the one-variable A/B for a wrapper change.** Swapping it on an existing build dir rebuilt 73 of 319 steps. Re-running `cmake -B` alone fails when the shell's cwd is a different source tree, so always pass `-S` explicitly.
- **A `.clap` bundle's directory name must match its binary name.** The probe derives `Contents/MacOS/<name>` from the bundle name, so `cp -R X.clap A-X.clap` makes it fail to load. Copy into `A/X.clap` instead.

**Not verified:** I did not run on Windows, which is the arm E85's Accept literally names. I did not compile on Windows or Linux, though the change has no platform branches. **TideSynth CI fetches GMPI_Wrappers at `origin/main`, so no CI run can see the fix until #41 merges.** I did no DAW test. I did not rebuild SynthEdit/SynthEditCL: neither compiles the CLAP wrapper (the only `gmpi_wrappers` consumer in SE16 is `se_gmpi/vst3`, Linux), so a CLAP-only change cannot reach them. I did not check whether the hidden editor paints. Audio was silent in both arms, as expected for `--no-preset` with no VCV modules compiled in (`TIDE_VCV_FUNDAMENTAL=OFF`). It is not this item's subject.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean and 34 behind, and I did not touch it. All work was in scratchpad worktrees, which I removed. `GMPI_Wrappers` stayed on `main`, clean (1 behind), and its work was also in a scratchpad worktree. `SynthEdit` was on `master`, clean, `ahead 1, behind 1`. That predates this run and I left it alone. `SynthEditLib`, `gmpi_ui` and `GMPI` were clean and untouched. I did no computer-use or GUI work. The probe's window was never on screen.

**Next:** see the `mac` NEXT cell. **For Jeff:** [GMPI_Wrappers#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41) is the fix and can merge independently of this PR. **E87's Accept reads as met on `main` today**: E85/E86/E88/E89 are distinct and `check-id-refs.py` is rc=0. Its prompt-vs-check question is still open, so I left it for a run that takes it.

**Branch/PR:** `tide/mac/E85-clap-gui-show` in TideSynth (probe + this entry + row + mac cell) and in GMPI_Wrappers ([#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41)).

## 2026-10-03 — macos — E87: its Accept is met on `main` -- the second merger renumbered, `Plat` untouched; the general form filed as A42 (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.19675.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E87**, measured its Accept, and found it already met; flipped it to IN-REVIEW with the evidence. Filed **A42** for the question E87 carried but did not need to answer. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded.

### STEP 1 / 1.5 / 2

STEP 1: no open `platform:mac` issue (open issues are #583, linux, and #44, the digest). STEP 1.5: this lane's only open PR is [#631](https://github.com/JeffMcClintock/TideSynth/pull/631) (E85, from the 10-02 run whose entry is on that branch, not yet on `main`), plus its companion [GMPI_Wrappers#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41). #631 is `MERGEABLE`/`CLEAN`, 13 SUCCESS + 2 SKIPPED, no reviews, no review comments; #41 is `OPEN`/`MERGEABLE`. Both are waiting for merge, so I left them alone.

STEP 2, walked in file order, each reason checked rather than inherited:

- **A35** waits on its own two open `PROPOSED:` entries. **A37** I read against the bookkeeping `PROPOSED:` entry (*"Should the fleet's two bookkeeping hot spots stop being single shared files"*), whose options move the NEXT block, which is all of A37. I agree with the 10-01 windows reasoning: it is not identical under every answer. **S8** is `NEEDS-SPEC`.
- **E19**: the remaining mac cell is AU3 in a real host with an editor on screen. A scheduled run cannot take the screen. **E82**: same, its Accept is a right-click on a VCV panel. Both are also the X2-shape judgements the `win` NEXT cell reserves for the next win run.
- **X2** and **E89** are linux. **E2** is an umbrella. **E76** wants a ruling. **E84** is a workflow edit this credential cannot make. **E85** is this lane's open #631.
- **E86** I read before taking it, as its row asks. Its Accept is a fork: either the processor builds the factory, or Jeff rules that TIDE requires a same-process controller. Which one is a product and lifetime ruling, not a run's. The only bare-host instrument for it (`tests/e80_vst3_feedback_probe.cpp`) is win32-only (`:561`). **Recommendation:** E86 wants a `PROPOSED:` entry before anyone builds anything.
- **E87** was next, `any`, small, BACKLOG-only. No branch or PR named it. I claimed it on `tide/mac/E87-id-collision-verified` and pushed the claim first.

### E87, measured

**The second merger did exactly what the row prescribed.** `82a9c49` (`tide-rack-bot`, 2026-10-01 13:58 +1300) is *"Merge origin/main into E79: renumber this lane's E85/E86 to E88/E89, per E87"*. It was pushed to [#584](https://github.com/JeffMcClintock/TideSynth/pull/584)'s branch two minutes after [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) landed (`98decfc`, 13:56) and merged as `be3930e`.

| check | result |
|---|---|
| linux rows at `82a9c49^1` vs **E88**/**E89** at `82a9c49`, id cell stripped | `diff` empty: Status, `Plat`, Item byte-identical |
| same rows, `82a9c49` vs `origin/main` | byte-identical |
| `python3 scripts/check-id-refs.py` on `origin/main` `d58bdd1` | *"no stale ID references, no duplicate IDs, no shared live citations"*, **rc=0** |
| `grep -c "^\| E8N \|"` in `BACKLOG.md` / `BACKLOG-DONE.md` | E85, E86, E88, E89: **1 / 0** each |
| **positive control**: same tree, E88/E89 id cells set back to E85/E86 | **rc=1**, *"2 DUPLICATE ID(s) -- one ID, more than one row: E85 BACKLOG.md:115, BACKLOG.md:118; E86 …"* |
| archived E79 row | cites **E88**, not the old id |

So `Plat` did not move (E89 is still `linux`), which was the trap E87 warned about. The only residue is that `JOURNAL-2026-09.md` names the linux findings by their old ids in two entries. That is append-only history, correctly left alone.

### Bookkeeping choices, and why

- **I did not touch the `mac` NEXT cell.** `main`'s cell is the 10-01 one; #631 carries a 10-02 cell on the same single line. Any edit of mine would conflict with #631 on that line. That is A38's livelock in miniature, and the 10-01 windows finding is that a zero diff in a contended cell is the durable resting state. Instead, the next mac run's instruction is here: **(1) STEP 1.5 on #631 / GMPI_Wrappers#41. (2) Walk STEP 2: E88 is the next `any` row below E87.**
- **The rotation is byte-identical to #631's.** Adding this entry put `JOURNAL.md` over 60 KB, and the oldest entry is the 09-29 windows one, which #631 also rotated. I took #631's `JOURNAL-2026-09.md` blob verbatim, so the two branches make the *same* change there and cannot conflict on it. `JOURNAL.md` will still conflict with #631 at the prepend point. Every pair of journal PRs does that, and whichever lands second resolves it.
- **I checked for id collisions before filing A42**, using the guard A42 describes: the highest A-id is A41 on `main`, on `tide/mac/E85-clap-gui-show` and on `tide/win/A41-probe-ref-pinning`, and A38 on `tide/mac/issue-599`.

**Learned:**

- **E87 resolved itself the way it said it should, through a run doing a re-sync, not through anyone taking E87.** The row's value was the instruction, which the merging run read and followed (its commit subject says *"per E87"*). That only works if the row is on `main` before the second merge. It was, because #584 had carried it.
- **`check-id-refs.py` passes on a renumbered id, but that does not mean every old mention is right.** It proves no id is *missing*. It cannot tell that a journal entry saying "E85" meant the linux finding now called E88. A renumber is safe for rows. For prose it is only as safe as the reader's willingness to look up the renumbering commit.
- **To check an Accept that reads "a lint is rc=0", break it back to the predicted failure first.** rc=0 on a check that cannot see the problem looks identical to rc=0 on a fixed tree. A39's lesson applies to a BACKLOG-only item too.

**Not verified:** no build, no host; nothing compiled was touched. I did not judge E19's or E82's Accepts (reserved for win). I did not answer E86.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean (behind `origin/main`); all work in a scratchpad `git worktree`, removed at the end. `SynthEdit` was on `master`, clean; I did not touch it or any other repo. No GUI work, no screen taken. No credential value appears anywhere.

**Next:** above, under *Bookkeeping choices*. **For Jeff:** #631 + GMPI_Wrappers#41 (E85) and #629 (A41) are green and waiting. A42's prompt-vs-script question is yours. The stale pushed branch `tide/mac/issue-599` (PR #604 closed unmerged) is still there, and deleting it is yours too.

**Branch/PR:** `tide/mac/E87-id-collision-verified`: the E87 flip, the A42 row, this entry, and the rotation.

## 2026-10-04 — macos — STEP 1.5: #631 (E85) went `DIRTY` in `JOURNAL.md` when #632 landed; re-synced, E87 archived, no backlog item (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.19675.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** STEP 1.5 on [#631](https://github.com/JeffMcClintock/TideSynth/pull/631), plus the STEP 4 bookkeeping #632's merge made due. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`d58bdd1..2489013`).

### STEP 1 / 1.5

STEP 1: no open `platform:mac` issue. The open issues are #583 (linux) and #44 (digest). STEP 1.5: this lane's only open PR is #631. Its checks were 13 SUCCESS + 2 SKIPPED, with no reviews and no review comments, but it was **`CONFLICTING` / `DIRTY`**. [#632](https://github.com/JeffMcClintock/TideSynth/pull/632) (E87) merged at 2026-10-02 13:13 UTC and prepended its 10-03 entry at the same point where #631 prepends its 10-02 entry. The 10-03 entry predicted exactly that. `git merge-tree` named **`JOURNAL.md` alone**. `BACKLOG.md` auto-merged, and `JOURNAL-2026-09.md` was already identical on both sides, because #632 took #631's rotation blob verbatim. That choice paid off. [GMPI_Wrappers#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41) is still `OPEN` / `MERGEABLE`.

**The resolution, and the one trap in it.** Both sides of the hunk are whole entries, so I kept both. **Putting #631's 10-02 entry first fails `check-journal-prepend.py` (rc=1)**: *"entries are not newest-first: 2026-10-02 > 2026-10-03 > …"*. The 10-02 entry therefore goes *below* `main`'s 10-03 entry, and the check then passes (`1 new entry prepended`, rc=0). The rule is "newest-first by date", not "the new entry on top". A branch that sits open across a later run's merge has to slot its entry in below. The second hunk was a trailing blank line, and I took `main`'s side.

Then all of the lint workflow's checks (`check-links`, `check-journal-prepend`, `check-backlog-diff`, `check-prompt-provenance`, `check-id-refs`, `check-next-block`) plus `check-backlog-archived` exit 0 on the merge. `extract-lessons.py --check` reported `docs/lessons.md is stale` after the merge, so I regenerated it: **1629 lessons from 381 entries**. `git merge-tree origin/main HEAD` is clean afterwards.

### Bookkeeping, and why it is on #631's branch

- **E87 archived.** #632 is `MERGED`, checked with `gh pr view` rather than inferred. The flip and the move are one edit (`check-backlog-archived.py`), so `Done = 2026-10-02` uses the UTC merge date.
- **This entry, the archive move and the `mac` NEXT cell are pushed to `tide/mac/E85-clap-gui-show`, not to a bookkeeping branch of their own.** The 10-03 run used a separate branch and #631 went `DIRTY` the moment it landed. That is A38's adjacency livelock, with this lane on both sides of it. One branch cannot conflict with itself. The cost is that #631 now carries an E87 archive move beside E85's probe. Both are bookkeeping-sized, and #631 cannot auto-merge anyway (`tests/**`).
- **Rotated, 72,779 → 49,700 bytes, 8 entries → 5.** I first misread the floor as a stopping point, so I am recording the rule as it actually reads: rotate oldest-first until the file is under 60 KB, and the floor of four entries only limits how far that can go. Three entries moved out. The 09-30 macos entry was appended below `JOURNAL-2026-09.md`. The 10-01 windows STEP 1.5 entry and the 10-01 macos entry went into a **new `JOURNAL-2026-10.md`**, whose header copies September's, and the `Archives:` line now links it. `extract-lessons.py` finds it by glob (A36's warning): `--check` gives **1632 lessons from 382 entries** both before and after the rotation. Across `JOURNAL.md` and both archives, the entry headings show 0 missing and 0 duplicated against `main` plus #631, and the only new heading is this entry's.

### STEP 2: not taken, and why

**STEP 1.5 is "the same tier as a broken build", and STEP 1 says to fix a broken build instead of taking a backlog item and then go to STEP 4.** I read that as consuming the run. I walked the queue anyway, so the next run inherits a reading rather than a guess. Nothing on `main` changed since the 10-03 walk except E87 (now archived) and **A42**. A42's own row says option (b) without (a) *"is not identical under every answer"*, so it waits on Jeff. **E88** is the next `any` row. For mac it means AU2/AU3 and VST3 in the activate-then-state order. Two facts the next run should start from: only `TIDE-Rack.clap` and `TIDE-Rack.vst3` are installed in `~/Library/Audio/Plug-Ins`, with no `.component`; and `tests/e80_vst3_feedback_probe.cpp:561` is `#if !defined(_WIN32)` around a `LoadLibraryA` loader. **A mac VST3 arm therefore needs the bundle loader ported first, which is most of the work.** Note also the E79 probe's own prediction that macOS passes activate-first anyway, because `CFRunLoopTimer` ticks the controller with no window. A mac E88 measurement needs a starved-run-loop control, or a pass will mean nothing.

**Learned:**

- **A branch that stays open across another run's merge must insert its journal entry below the newer one, not on top.** `check-journal-prepend.py` checks date order as well as prepend-only, so "keep both sides, ours first" is rc=1 whenever the other side is newer.
- **Taking the other open branch's rotation blob verbatim works.** #632 did it, and `JOURNAL-2026-09.md` did not conflict. Only the prepend point did.
- **Putting a lane's bookkeeping on its own open PR's branch removes the conflict at the source.** A second journal PR from the same lane is guaranteed to conflict with the first.

**Not verified:** no build and no host. Nothing compiled changed; the merge touched only `JOURNAL.md`, `BACKLOG*.md` and `docs/lessons.md`. I did not re-run #631's E85 A/B; its evidence stands as the 10-02 entry records it.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean (behind `origin/main`). All work was in a scratchpad `git worktree`, removed at the end. `SynthEdit` (`master`), `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers` and `GMPI` (`main`) were all clean, and I did not touch them. No GUI work and no screen taken. No credential value appears anywhere.

**Next:** see the `mac` NEXT cell. **For Jeff:** #631 + GMPI_Wrappers#41 (E85) and #629 (A41) are waiting; #631 is clean again. A42's question is yours. The stale `tide/mac/issue-599` branch is still there.

**Branch/PR:** `tide/mac/E85-clap-gui-show`, [#631](https://github.com/JeffMcClintock/TideSynth/pull/631): the merge, this entry, the E87 archive move and the `mac` cell.

## 2026-10-05 — macos — E88, VST3 on macOS: activate-then-state plays silence with or without the run loop; the probe now loads a bundle on macOS; fix not chosen (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.19675.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** ported `tests/e80_vst3_feedback_probe.cpp`'s loader to macOS, added an `--activate-first` arm, and measured E88's VST3 half on this box. **The defect is real on VST3.** E88 goes back to `TODO` with the measurement, because the fix is a choice between three mechanisms and this run did not make it. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`origin/main` = `2489013`, unchanged since the 10-04 run).

### STEP 1 / 1.5 / 2

STEP 1: no open `platform:mac` issue. STEP 1.5: [#631](https://github.com/JeffMcClintock/TideSynth/pull/631) (E85) is `MERGEABLE`, 13 SUCCESS + 2 SKIPPED, no reviews, and its only comment is the 10-04 run's own. [GMPI_Wrappers#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41) is `MERGEABLE`, no reviews. Both are waiting for merge, so I left them alone. STEP 2: the 10-04 walk still holds. `main` has not moved, A42 waits on Jeff by its own words, and **E88** is the next eligible `any` row. No branch or PR named it. None of the five open `PROPOSED:` entries (Plat edits, a wrong required check, deterministic handles, patch-cable dirty, the bookkeeping hot spots) changes what E88 builds. Claimed on `tide/mac/E88-vst3-activate-first` and pushed the claim first.

### The probe

On macOS a `.vst3` is a bundle, and the module contract is `bundleEntry(CFBundleRef)` before `GetPluginFactory` and `bundleExit()` after the last release. The probe now does that through `CFBundle`. Its `pump_main_thread` runs `CFRunLoopRunInMode` on the clock, because that call returns `kCFRunLoopRunFinished` immediately when the loop has no timers yet. `--activate-first` runs the same controller-then-component restore, just after `setActive(true)` instead of before, followed by the same 0.5 s handover. The `ComponentHandler` stub now **counts** `restartComponent` instead of dropping it, which is the same lesson the e79 probe learned about `request_restart`. `--editor` still needs an NSView parent and refuses on macOS with rc=2. The Windows path is unchanged in logic, but **I did not compile it**: only `#if` guards moved around it.

Build: `clang++ -std=c++17 -O2 -I <CPM>/vst3_sdk tests/e80_vst3_feedback_probe.cpp -framework CoreFoundation`, with the SDK headers TideSynth's own CMake had already fetched. The header says so.

### E88, measured

TIDE built from this branch: `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release`, 659/659, rc=0. GMPI_Wrappers was fetched at `3da5548`, which **includes E79's CLAP fix**. VST3 binary sha256 `e65fe83ec8c493e0…`, probe `ce1999d8e0d32141…`. Preset: `v1-rack.rpp`'s 18,893-byte `<Preset>`, from `scripts/decode_rpp.py --preset-out`. 400 blocks of 512 at 44.1 kHz, three runs per arm, interleaved:

| arm | stderr | peak | `restartComponent` | runs |
|---|---|---|---|---|
| state-then-activate, `--pump` | `instance #2 building rack from 14136 byte document`, `rack built for 44100 Hz, block 512` | **0.482431 (-6.3 dBFS)** | 0 | 3/3 |
| state-then-activate, `--no-pump` | same | 0.482431 | 0 | 1/1 |
| **activate-then-state, `--pump`** | `controller #1 restore of a 14136 byte document -> imported`, then `TIDE: unprepared - writing silence`; **no `building rack`** | **0 (-inf)** | **0** | 3/3 |
| activate-then-state, `--no-pump` | identical | 0 | 0 | 3/3 |
| `--no-preset` (negative control) | `unprepared`, no build | 0 | 0 | 1/1 |
| **CLAP reference, same build**: `e79_clap_headless_probe.c --activate-first`, with and without `--runloop` | `building rack`, 1 build | **0.482431** | n/a | 1/1 each |

**My prediction was wrong.** Before running, I expected activate-then-state to pass with the pump and fail without it. The controller's tick is a `CFRunLoopTimer` here, and the E79 probe's header predicts exactly that rescue for CLAP on macOS. It does not happen on VST3. The pump is not the variable: the controller imports the document either way, and nothing re-runs `start_processor` on a processor that is already live. The cause can be read straight off the code. `Processor_VST3::setState` (`GMPI_Wrappers/wrapper/VST3/Processor_VST3.cpp:1077`) ends in `setPresetUnsafe`. The only caller of `reInitialise()` after `initialize` is `setActive(true)` (`:357-372`). CLAP passing on the same build shows the difference is E79's `requestRestart`, which VST3 has no counterpart to.

### Why I stopped short of the fix

Detecting the case is trivial, since `Processor_VST3` already keeps `active_`. The remedy is a choice of mechanism, and the row warns against carrying the CLAP one over:

- **(a)** `restartComponent(kReloadComponent)`. Only the controller holds the component handler, so the processor has to message it, and what a host does on `kReloadComponent` is host-defined. The probe would also have to **act** on it (`setActive(false/true)` between blocks), or the fix cannot be observed. That is the same trap the e79 probe documents.
- **(b)** Calling `reInitialise()` from `setState`. That races the audio thread.
- **(c)** Setting a flag in `setState` and rebuilding at the next `process()`. That puts the rack build on the audio thread.

I could not state the fix in one sentence. A wrong choice here is a shipping regression in every VST3 host, so I filed the options in the row rather than committing one.

**Learned:**

- **The macOS run loop rescues CLAP's controller queue, but not VST3's restore.** A prediction drawn from the CLAP wrapper's structure did not transfer to VST3, even on the same platform and the same build. The row's *"Do NOT assume the CLAP fix transfers"* applies to the diagnosis too, not only to the fix.
- **`CFRunLoopRunInMode` returns at once on a loop with no sources.** A pump written as a single call waits for nothing until the plug-in has registered its timer. Loop on the clock.
- **A host stub has to count `restartComponent`.** Without the count, a later fix built on `kReloadComponent` would read as "no effect". The e79 lesson about `request_restart` applied to VST3 too.

**Not verified:** no Windows or Linux compile of the probe, and no VST3 measurement on either. No AU2/AU3: this build has no `.component`, and `tests/e9_au_rate_probe.mm` is the nearest AU host. No DAW. No fix was made, so nothing in `GMPI_Wrappers` changed. SynthEdit/SynthEditCL were not rebuilt because nothing they compile changed.

**Collision note:** this branch's `BACKLOG.md` **conflicts with #631**. #631 archives the E87 row, which sits directly above E88, and git treats adjacent hunks as a conflict. Whichever lands second has to re-sync. Both sides are whole-row edits, so the resolution is to keep both. I took #631's rotation blobs verbatim (`JOURNAL-2026-09.md`, `JOURNAL-2026-10.md`, the `Archives:` line), so those files cannot conflict. **That moved one entry more than the rule strictly needed.** Removing 09-30 and the 10-01 macos entry would already have brought the file under 60 KB (about 57 KB). I also moved the 10-01 windows STEP 1.5 entry, because a `JOURNAL-2026-10.md` that differs from #631's would be an add/add conflict on a brand-new file. Nothing is lost: it is in the archive, verbatim, and `JOURNAL.md` still meets the floor of four entries. `JOURNAL.md` will conflict at the prepend point as usual: the newest date goes on top. **I did not touch the `mac` NEXT cell**, because #631 edits the same line.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main` and was not touched. All work was in a scratchpad `git worktree` and build directory, both removed at the end. `GMPI_Wrappers` stayed on `main` (behind 1), clean, and was read only. `SynthEdit` was on `master`, clean, and untouched. No GUI, no screen taken. No credential value appears anywhere.

**Next:** **(1)** STEP 1.5 on #631 + GMPI_Wrappers#41, and on this branch's PR. **(2)** E88's VST3 fix: pick (a), (b) or (c) above, then A/B it with `--activate-first` on this probe. If it is (a), teach the probe to act on `kReloadComponent` first. **(3)** E88's AU2/AU3 half still needs an AU build and a host.

**Branch/PR:** `tide/mac/E88-vst3-activate-first`: the probe port, the E88 row, this entry and the rotation.

## 2026-10-05 — windows — E86: the processor-only VST3 finding reproduces, and its "unknown to fix" is now ONE MEASURED LINE; the fork escalated as a PROPOSED entry (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.19675.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E86**, re-measured its finding on today's `main`, measured the cost of the option its row calls *"unknown to fix"*, and escalated the fork as a `PROPOSED:` entry. Archived **E87**, whose [#632](https://github.com/JeffMcClintock/TideSynth/pull/632) merged 2026-10-02. Regenerated the stale `docs/lessons.md` and rotated `JOURNAL.md`. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`d58bdd138..248901335`).

### STEP 1 / 1.5 / 2

**STEP 1:** no open `platform:win` issue, which on this platform still verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing. The two open issues are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux) and [#44](https://github.com/JeffMcClintock/TideSynth/issues/44), the digest. **This run has something better than CI for once: `main` builds on Windows, measured** — see below.

**STEP 1.5:** this lane's only open PR is [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) (A41), `MERGEABLE`/`CLEAN`, 13 pass + 2 skipping, no reviews and no unresolved review comments. STEP 1.5 says a green PR with nothing unresolved is waiting for merge and not a run's to fix, so I left it alone. The fleet's other two open PRs are mac's: [#631](https://github.com/JeffMcClintock/TideSynth/pull/631) (E85) and [#633](https://github.com/JeffMcClintock/TideSynth/pull/633) (E88), both `MERGEABLE`/`CLEAN`.

**STEP 2, walked in file order, every reason re-checked rather than inherited:**

- **A35** is parked on its own two open `PROPOSED:` entries, which is what its row says and what reading them confirms. **A37** is overlapped by the bookkeeping `PROPOSED:` entry: its options (b) and (c) move the NEXT block into per-platform sections or `docs/next/<platform>.md`, and *where the NEXT cells live* is the whole of what A37 builds — so that entry's *"May proceed meanwhile: EVERYTHING"* does not reach it, for the reason the 10-01 cell gives. I reached the same conclusion from the entry itself.
- **A42 is ineligible by its own words**, which is the cleanest case in the queue: *"(b) alone is buildable by a run, but a script nobody is told to run guards nothing, so (b) without (a) is not identical under every answer."*
- **S8** is `NEEDS-SPEC`. **E2** is an umbrella with no statable Accept. **E84** is a `.github/workflows/**` edit this credential deliberately cannot make. **X2** and **E89** are linux.
- **E76 I read rather than inheriting.** Its `Plat` is `any`, but its Accept is a fork — `render-and-measure.py` returning `-6.3/-17.0` **on linux**, *or* a docstring line saying a wrapper is required. Choosing the second branch presumes the answer to the row's own open question (may a measurement script edit the caller's environment), so it is not identical under every answer.
- **E19 and E82 are the two X2-shape rows the `win` cell flagged, and I checked them rather than assuming.** Both are correctly `TODO`: each row already states its own remaining work, written by the run that measured it. E19 owes a **pixel-diff** clause and an **int/bool/enum** clause; E82 owes the right-click on **WT LFO**'s panel. **Those two are the same measurement**, and they are the only rows left in this queue that need a screen. Nothing to flip.
- **E85 and E88 are mac's**, with open PRs naming them. **E87** was `IN-REVIEW` with its PR merged — a STEP 4 chore, not an item.
- **E86 was next.** Its row says *"read it before taking it"*, so I did, and took it.

### E86, re-measured — and the row's finding reproduces, silently

The instrument is the row's own: `tests/e80_vst3_feedback_probe.cpp`, win32-only, which is why this lane is the only one that can run it. Built from `origin/main` with **no `*_FOLDER_OVERRIDE`**, so all eight dependencies are their own `main` — configure rc=0, build rc=0, **zero `error C`/`error LNK` lines**, both on the first pass and on a second (Defender's cold-tree trap did not fire this time). The probe compiles with `cl` against the headers-only VST3 SDK. Same `TIDE-Rack.vst3`, same 43,247-byte document, same `TIDE: rack built for 44100 Hz, block 512`, 400 blocks at 44.1 kHz, `TIDE_FEEDBACK_TRACE_EVERY=1`:

| arm | modules constructed | display-state captures | feedback sends | max send |
|---|---|---|---|---|
| with controller (what a DAW does) | **5** — `LFO`, `LFO2`, `Pulses`, `SHASR`, `Scope` | 4 | 287 | 65,798 B |
| `--no-controller`, `main` as it stands | **0** | 0 | 11 | 37 B |
| `--no-controller` + one `registerDeferredModules()` call | **5** — the same five, by name | 4 | 287 | 65,823 B |

**Every probe check passed in all three arms and no arm logged an error.** That is the finding rather than a caveat.

**Row 3 is the part the row called *"unknown to fix"*, and it is one line.** `rack_adaptor::registerDeferredModules()`, called from `SynthEdit::open()` (`SynthEditSem/SynthEdit.cpp:376`). **The scratch patch was reverted and `SynthEdit.cpp` is byte-identical to `main` on both of this run's branches** — I did not land it, because which fork to take is a ruling.

**The control that says that call is SUFFICIENT and not merely necessary.** Normalising the volatile counters out of the plug-in's own stderr and diffing the arms, the patched `--no-controller` run is identical to the with-controller run on **every rack and DSP line**. The only lines it still lacks are controller-side by nature: the seven enrichment XMLs (`ControlsXp.xml enriched 4 of 18 described class(es)` and six more), `7 rack prefab(s) seeded from the bundle`, and TideApp's own document state. Distinct line shapes: **44** with a controller, **33** with the one call, **5** on `main`'s `--no-controller`.

**What the control cannot settle, stated because the measurement cannot:** this fixture's DSP needed neither the enrichment XMLs nor the prefabs, so it says nothing about a document that would. What would settle it is a fixture using a class whose pins come ONLY from one of the seven XMLs — and `SE MIDI to CV 2`, the one SE module in this fixture, is described by none of them (checked against all seven).

### Two findings the row did not ask for

**(1) The guard that makes the call idempotent is not thread-safe, and the fix would add the second caller it has ever had.** `registerDeferredModules()` is guarded by a plain function-local `static bool done` — not an atomic, not a `call_once` (`SynthEdit_Rack_Adaptor/RackFactoryStatic.cpp:75-82`). Its comment says the guard sits there *"rather than in the host"* so that several instances in one process are safe, and that reasoning holds for several instances on **one thread**. Today the only caller is `TideApp::InitInstance()`, from the controller's `initialize()`.

**What makes the `open()` site safe on the thread that matters** — and this is SDK-documented rather than inferred: `open()` is reached from `Processor_VST3::setActive(true)` → `reInitialise()` → `gmpi_processor::start_processor()` → `processor->open(host)`, and the VST3 SDK annotates `IComponent::setActive` **`[UI-thread & Setup Done]`** (`pluginterfaces/vst/ivstcomponent.h:195`). The rack build is **not** on that thread: `rack.prepareToPlay()` runs synchronously on the audio thread from `onSetPins` (`SynthEditSem/SynthEdit.cpp:528`, whose own comment says so) and again from `subProcess` (`:598`). So registering in `open()` lands strictly before any audio-thread lookup, while registering from the rack-build path would put a process-global database write on the audio thread.

**That guard is in `SynthEdit_Rack_Adaptor`, which is on NEITHER the run prompt's ALLOWED nor its GATED list** and is therefore GATED by default — as is `VCV_Fundamental_gmpi`. This is the **G3** shape exactly, so it is filed inside the `PROPOSED:` entry rather than reached into.

**(2) A second defect, on `main` today and independent of E86.** The guard returns **0** on every call after the first, and both callers print that number as a count. Measured directly: the two processor objects the probe creates printed `-> 39 module(s)` then `-> 0 module(s)`. `TideApp.cpp:855` prints the same number the same way (`TIDE: %s — %d module(s) registered`), so **a host that creates a second TIDE instance in one process already logs `0 module(s) registered`** — which reads as a failure and is a success. A TIDE-side one-liner under any answer; recorded here rather than filed as a row, because it is the same call site the entry is about.

### Bookkeeping

- **E87 archived.** [#632](https://github.com/JeffMcClintock/TideSynth/pull/632) merged 2026-10-02, checked individually rather than inferred. Flip and move are **one edit**, per the 10-01 cell's trap, and `check-backlog-archived.py` is rc=0. A41 stays `IN-REVIEW`: [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) is still open.
- **Archiving E87 did NOT create a stale take-target**, which is the trap the 10-01 cell warned about. The previous `win` cell's *"THEN READ E85, E86, E87 AND E88"* is now chain history, and the defusing negation is in the same sentence as the verb in my own cell (*"…is archived by this cell, so it is not a take-target any more"*). `check-next-block.py`: **2 take-targets across 4 NEXT rows, every one live, rc=0.**
- **`docs/lessons.md` was stale again** — `extract-lessons.py --check` said *"stale -- run --write"* on `origin/main`, so the 10-03 cell left it owed. Regenerated; figures below.
- **Two PRs, because the lane's measured shape demands it.** `docs/decisions.md` is **deliberately carved out of the auto-merge allowlist** — `automerge_eligible.py` says *"allowlisted by location but carved out (merging it is a decision, not bookkeeping)"*, rc=1 — so putting the escalation in the bookkeeping PR would park this entry and the E87 archive behind a human indefinitely. The escalation branch carries **`docs/decisions.md` and nothing else**, with `BACKLOG.md` byte-identical to `origin/main` so it cannot re-conflict as `main` moves (the 10-01 zero-diff resting state).
- **The A42 guard was not needed:** this run allocated no new id. A `PROPOSED:` entry is keyed to its row.

**Learned:**

- **"Unknown to fix" is sometimes one line, and the cheap way to find out is to patch, measure and revert.** E86 sat `TODO` for 24 days with `Size` reading *"small to answer, unknown to fix"*. A scratch patch plus an incremental rebuild — about two minutes on a warm tree — turned that into a measured three-row A/B. **Measuring an option is not taking it**, and it is what makes an escalation decisive rather than speculative; the A38 entry's probe is the precedent.
- **The strongest evidence for a fix being sufficient was a DIFF OF THE STDERR, not a count.** Equal module counts prove the fix works; the normalised line-shape diff (44 / 33 / 5) proves what is still missing and that none of it mattered here. It is also what let me state the limit honestly — the fixture cannot discriminate the enrichment-XML half, and no count would have shown that.
- **An idempotence guard is a thread-safety claim in disguise, and adding a caller is what tests it.** The comment on `registerDeferredModules()` is careful and correct about *several instances in one process*; it is silent about *two threads*, because until this measurement there was only ever one caller. **The moment a fix adds a second call site, re-read the guard rather than its comment.**
- **An idempotent function that returns a COUNT lies to its second caller**, and both of TIDE's callers print it. This was visible only because the patched arm printed the line twice in one run — `39` then `0`. A guard that returns 0 for "already done", formatted as `%d module(s) registered`, is a success that logs like a failure.
- **`docs/decisions.md` being denied on the auto-merge allowlist makes the split MANDATORY, not stylistic.** The lane's standing shape is *documents in one PR, code in another*; the sharper rule is **anything that cannot auto-merge goes in its own PR**, and an escalation is the clearest case there is, because the whole point is that a human merges it.
- **The invisible-HWND arm has a hard boundary, and E19's remaining clause is on the wrong side of it.** An unshown window gets no `WM_PAINT`, so a pixel-diff cannot use it. Worth knowing before the next run reaches for the trick to clear E19: its int/bool/enum clause might go that way, its pixel diff cannot, and the box was not idle today either.

**Not verified:** I launched no DAW and took no screen. The `main`-builds-on-Windows claim is this run's own build of a tree that is `main` plus one `BACKLOG.md` status cell — real, but not a CI run. I did not judge E19's or E82's Accepts beyond confirming each row already states its remaining work; I measured neither, because both need a screen. The enrichment-XML and prefab halves of the processor-only gap are **unmeasured**, as the control section says. Audio was silent (`-1000.0 dBFS`) in all three arms, including with a controller, so this fixture proves nothing about audio either way. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half, still open).

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and **never left it** — all work in two scratchpad `git worktree`s, both removed at the end, with the build tree also in the scratchpad. **All eight repos were clean at the start** and I touched none of the other seven: `SE16` (`master`), `SynthEditLib`, `gmpi_ui`, `GMPI`, `SynthEdit_Rack_Adaptor`, `VCV_Fundamental_gmpi` (all `main`) and `GMPI_Wrappers` (`main`, behind 1). **The developer was at the machine throughout** — Visual Studio on `ToneMaster`, the ToneMaster app running, Paint.NET, Slack, Settings — so no GUI work and no screen taken, which E86 needed none of. `git fetch` again warned *"too many unreachable loose objects"* in `C:\SE\TideSynth`; a local housekeeping note for Jeff (`git gc`), not a repository problem, and not mine to run on his tree. No credential value appears in any commit, PR, journal entry or row.

**Next:** see the `win` NEXT cell. **For Jeff, two things:** (1) **[#634](https://github.com/JeffMcClintock/TideSynth/pull/634) is E86's fork and cannot auto-merge by design** — four options, with (c) recommended and (a) measured; merging it is the ruling. (2) [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) (A41) has been green and waiting since 10-01, and A40's one question — does the fleet PAT expire at all — is still answerable only in the GitHub UI.

**Branch/PR:** the `PROPOSED:` entry is on `tide/win/E86-vst3-processor-factory`, [#634](https://github.com/JeffMcClintock/TideSynth/pull/634) (`docs/decisions.md` only). This entry, the E86 row, the E87 flip and archive, the rotation, the regenerated `docs/lessons.md` and the refreshed `win` cell are on `tide/win/2026-10-05-e86-bookkeeping`, which is bookkeeping-only and should auto-merge.
