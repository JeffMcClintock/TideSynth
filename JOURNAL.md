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
