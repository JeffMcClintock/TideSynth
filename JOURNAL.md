# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

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
2. Stop when this file is **under 30 KB**, or when the **four most recent
   entries** remain — whichever comes first. **The floor of four wins**: the run
   prompt tells every run to read the last four entries, so a verbose month
   pushing this file over 30 KB is correct, not a rotation failure.
3. Never edit an entry while archiving it. The archive is the record.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-08-15 — windows — C12e (interactive session, Jeff ruling)

**Did:** **C12e**, ruled in session ("go with your recommendation") — option
(b). `Dialogs_editor.h` moved to `SynthEditLib`; `Dialogs_editor2.cpp` came off
EditorLib's source list and is now compiled by each app that needs it.
**27 → 25 `${EDITOR_DIR}` entries.** Also struck a stale NEEDS-JEFF from **A9**
(below), and recorded the ruling in
[docs/decisions.md](docs/decisions.md), which now has **no open PROPOSED
entries**.

**The headline is not the move. It is that the recommendation's stated
reasoning was wrong on the one point that decided the work, and measuring
before implementing is what caught it.**

Both this row and the PROPOSED entry said the other consumers *"each supply
their own definitions"*, which made (b) a one-`vcxproj`-entry change.
**`SynthEditCL` does not supply its own.** Its CMake target compiles
`main.cpp`, **not** `CLApp.cpp` — `CLApp.cpp` is in no build file at all — and
`main.cpp` carries a comment saying in as many words that it relies on
EditorLib for these stubs, as does `EditorScreenshot/EditorCommandDispatcher.cpp`.
So (b) as literally written would have removed the only definition SynthEditCL
had and broken its link.

Implemented as (b) **properly**, which is what option (b)'s own text points at:
the `SynthEditApp.cpp` / `ExportAsPlugin.cpp` pattern, where **every** app that
needs the symbols compiles the file itself. `SynthEditCL/CMakeLists.txt` and
`SynthEdit2.vcxproj` each gained an entry; `TideApp.cpp` and `layouttests.cpp`
already define their own.

**Two more of the row's facts were wrong**, both harmless but worth correcting
because they were repeated in three places:

- The file defines **two** functions, not three. `doDialogBuildCodeSkeleton`
  appears in neither `Dialogs_editor2.cpp` nor `Dialogs_editor.h` — it is
  declared and defined only in `CLApp.cpp` and `TideApp.cpp`, and belongs to
  **S3**, not here.
- There are **four** definitions in the tree, not five:
  `Dialogs_editor2.cpp`, `CLApp.cpp` (unbuilt), `TideApp.cpp`,
  `layouttests.cpp`. `EditorScreenshot` and `SynthEditCL/main.cpp` carry only
  comments pointing at EditorLib's copy.

**Result.**

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **27 → 25** |
| fresh Ninja tree, Release | **904/904 RC=0** — net zero, one TU left EditorLib and one joined SynthEditCL |
| `Dialogs_editor2.cpp.obj` | now built **only** by `SynthEditCL.dir`; `EditorLib.dir` has zero |
| **TIDE still links** — the specific thing the row demanded | **`TIDE.gmpi` and `TIDE_VST3.vst3` both produced** |
| `SynthEditCL.exe` | links |
| tests | **91/92** — see below |

**The one test failure is not this change, and that is proven rather than
argued.** `Layout.ModuleSizeDoesNotGrowOnReopens` fails with a `bad_alloc`,
reproducibly. **A/B: `SE16` at `origin/master`, with none of C12e, in a
detached worktree against the same libraries — fails identically.** So it is
pre-existing on master.

**What it actually is, since the next run will hit it too:** in-flight
`ITextLayout` work spanning two repos. `gmpi_ui` committed
**`d3bacf3` "feat: ITextLayout, a retained immutable styled text layout
(Direct2D)"** partway through this session, and
`SynthEditLib/modules/se_sdk3_hosting/GmpiCpuUniversalContext.h` — **still
uncommitted** — already calls `gmpi::drawing::api::ITextLayout`. Pinning
`gmpi_ui` back to `3ab5524` does not restore green either; it fails to
*compile*, because that uncommitted header needs the new API. The two are
mid-flight together and neither half stands alone right now. **Do not "fix"
this**; it is Jeff's live work in another session. Left untouched, as the
STEP 5 dirt rule requires.

**Learned — a `*_FOLDER_OVERRIDE` build reads a live working tree, so another
session's uncommitted work lands in your test results.** This is the first time
that has actually bitten. It is not a reason to stop using the overrides
(**X4** settled that), but it is a reason to A/B against the default branch
**before** blaming your own change — and to do it in a `git worktree`, which
leaves the developer's tree untouched. The whole diagnosis cost one worktree
and two targeted builds.

**Learned — A9 has been listing a NEEDS-JEFF that PLAN.md already answered.**
The row asks for "TIDE's product philosophy in 2–3 sentences as the auto-reject
filter, Cardinal-style". [PLAN.md](PLAN.md) has carried it since before the row
was written: **"What TIDE Rack is"** is the one-sentence identity, and the
**eight design constraints** are the reject filter in more detail than three
sentences would be. Written 2026-08-09 from the process review and never
re-pointed. Struck, with the reasoning in the row. **A9 needs nothing from Jeff
to start.**

**Next:** **C12c** and **C12f** are the remaining sub-stages a Windows box can
take (C12d is `linux`). C12 now stands at 25 of its original 41 entries, and
**C12f is what takes it to zero and unblocks C6**. Whoever takes either should
expect `Layout.ModuleSizeDoesNotGrowOnReopens` to be red until the ITextLayout
work lands, and should **not** treat it as their own regression — A/B first.

**Side effects on this box:** a scratch Ninja tree, a pristine `gmpi_ui` clone
and a detached `SE16` worktree, all under the session scratchpad; the worktree
was removed and `git worktree prune` run, leaving `git worktree list` with only
Jeff's own entries. `SynthEditLib` was committed in **while dirty with Jeff's
uncommitted `GmpiCpuUniversalContext.h`** — staged by explicit path, never
`git add -A`, and that file is untouched.


**Postscript — the A14 guard fired on its first real outing, and it mattered.**
The two code branches were created in the *shared* working copies, and the
other session commits into whatever branch is checked out there. So its
in-flight `ITextLayout` commits landed on my branches, interleaved by seconds
(`SynthEditLib` `eae673b` 13:15:04, mine `93f5ea9` 13:19:27, its `a7eb0bf`
13:19:50; `SE16` mine `7563bd151` 13:20:02, its `eb66d2ae9` 13:20:05) — and I
pushed them before noticing. **`scripts/check-commit-authorship.py`, written
this morning for exactly this, is what caught it**, in the STEP 4 pre-push
position its own docstring argues for.

Resolved without rewriting or deleting anything: my single commit was
cherry-picked onto fresh `tide/win/C12e-clean` branches off the default
branches, **in temporary worktrees so neither shared checkout was touched**,
and the PRs raised from those. Each clean branch contains exactly one commit,
verified. The mixed `tide/win/C12e-dialogs-editor` branches are left exactly as
they are — they hold Jeff's work, and they are not mine to rewrite.

**Two things for the next run.** First: **both shared checkouts are still parked
on `tide/win/C12e-dialogs-editor`**, so further commits there keep landing on
it; I deliberately did not switch them, because doing that under a live session
risks its working tree. Second, and more general: **creating a branch in a
shared working copy is itself the hazard.** A14's assertion catches the result;
it does not prevent it. The durable fix is to do code work in a `git worktree`
rather than by switching the developer's checkout — which is what the cleanup
had to do anyway.

**Branch/PR:** [SynthEdit#20](https://github.com/JeffMcClintock/SynthEdit/pull/20)
+ [SynthEditLib#9](https://github.com/JeffMcClintock/SynthEditLib/pull/9) —
**these two must merge together** — and the TideSynth PR carrying this entry,
the backlog and the ruling.

---

## 2026-08-15 — windows — A15 (interactive session, Jeff directing)

**Did:** **A15** — wired A10's [scripts/check-id-refs.py](scripts/check-id-refs.py)
into the `lint` job, which is the half A10 could not do because the bot token
is `repo` scope with **no `workflow`**. Pushed as Jeff rather than
`tide-rack-bot` for exactly that reason; his keyring token on this box does
carry `workflow` (checked with `gh auth status` first, since the A11 entry
records that this bites on other boxes and not here).

**Result — both halves of the Accept proven in the PR's own two-commit
history, in order, rather than asserted.**

| push | carried | `lint` |
|---|---|---|
| first | the wiring **+ a probe file naming a nonexistent row `Z9`** | **fail** |
| second | probe deleted | **pass** |

The failing run reads `id-refs: failure` with `links`, `journal`, `backlog` and
`provenance` all `success` — so it failed **for the right reason and only that
reason**, and the `ID_REFS` Summary wiring genuinely converts a step failure
into a job failure. The passing run reads all five `success` and
`456 ID reference(s) checked against 98 row(s), 91 distinct ID(s) named`.
Together those rule out the two ways this could have looked installed while
doing nothing: failing always, and passing vacuously.

**Learned — the Summary wiring is the part that would have silently rotted, and
A15's row was right to insist on it.** Every step in this job is
`continue-on-error`, so a step added *without* `ID_REFS` in the Summary's `env`
and `for` loop runs, prints its findings, fails — and the job goes green. It
would look wired for as long as nobody read a log. Same shape as A4's finding
that a path allowlist can look built while firing on nothing, and the reason
both halves are now demonstrated separately rather than assumed together.

**Learned — this check is deliberately not diff-based, unlike the four above
it, and the workflow now carries a comment saying so.** The other four compare
a base version against head. A cross-reference goes stale when the row it names
is **renamed or archived** — an edit to a *different file* than the one holding
the reference. A diff-scoped check would see the reference file unchanged and
pass, which is precisely the case A10 was filed for. So it reads the whole tree
every run. Cost is bounded: 456 references, 9 seconds.

**Next:** nothing on this row. The lint job is now five checks, all green on
`main`. Whoever next touches [scripts/check-id-refs.py](scripts/check-id-refs.py)
should re-run `--selftest` (20 cases) as well as the tree scan, since CI runs
only the latter.

**Side effects on this box:** none outside the scratchpad. TideSynth only; no
other repo was committed in or modified.

**Branch/PR:** [TideSynth#65](https://github.com/JeffMcClintock/TideSynth/pull/65).

---

## 2026-08-15 — windows — P9

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** **P9** — a lint that fails when the two `resource.h` copies stop
agreeing: [scripts/check-resource-h-drift.py](scripts/check-resource-h-drift.py).
Fifth item this session, at Jeff's direction, TideSynth-only. Took the row's
cheaper of two options — the lint rather than merging the files into one — on
the row's own pricing ("minutes for the lint; longer if the two are actually
merged") and because merging a Visual-Studio-generated header that VS will
rewrite is a change with its own failure mode.

**Result:** **318 `ID*` constants on each side, all agreeing.** Selftest 7/7.
Exit 1 on both divergence kinds — a same-name-different-value conflict, and a
constant present on only one side — tested on **scratch copies**, so neither
`SE16` nor `SynthEditLib` was written to; both were verified clean afterwards.

**Learned — excluding the `APSTUDIO_INVOKED` block is what makes this check
survivable, and it is not a detail.** The one thing the two files actually
differ in today is `_APS_NEXT_RESOURCE_VALUE`: **210** private, **207** public.
That is Visual Studio's own allocation counter, it compiles to nothing, and it
moves whenever anyone adds a resource in the IDE. A check that compared it would
be **red from birth and red forever**, and the first person to see it would turn
it off — taking the 318 real constants with it. So the whole `#ifdef
APSTUDIO_INVOKED` block is skipped, nesting-aware, and the selftest pins that
behaviour with the real 207→210 case as one of its seven.

The counter gap is still worth reading as evidence rather than noise: it says
the private copy has had **three resource slots allocated that the public one
has not**. Nothing has collided yet. Nothing would announce it if it did — which
is the entire reason this row exists.

**Learned — this closes a loop under C12a rather than sitting beside it.** C12a
delisted `${EDITOR_DIR}/resource.h` on the strength of the two copies being
identical, and that argument only holds while they stay identical, because
public and private TUs each resolve to their own copy by the own-directory-first
rule. That assumption had nothing enforcing it and was made this morning. Now it
has a check, and the check independently re-derives the same 318 that C12a
relied on.

**Next:** the row's larger option — pick one copy as the source of truth — is
still open and still unowned; the lint makes it safe to defer, not unnecessary.
**Note it cannot run in TideSynth CI**, since one of the two files is in the
private repo, so it is a dev-box/agent tool like
[scripts/dangling_private_includes.py](scripts/dangling_private_includes.py).
Its docstring says to run it as part of **any carve-out stage that moves a
`.cpp` out of `SynthEdit2`** — such a TU switches from the private copy to the
public one, a no-op only while this passes. **C12f is the next stage that does
that**, and its row already says to re-check `resource.h`; this is the command
for it.

**STEP 1 / 1.5:** unchanged. Open PRs are all this session's own.

**Side effects on this box:** copies of both `resource.h` files in the
scratchpad, and a throwaway git repo from the A14 run. Nothing outside it. This
run committed in TideSynth only; `SE16` and `SynthEditLib` were read but never
written, and were confirmed clean afterwards.

**Branch/PR:** [TideSynth#64](https://github.com/JeffMcClintock/TideSynth/pull/64),
fourth in the stack (61 → 62 → 63 → 64), each retargeting to `main` as its
parent merges.

---

## 2026-08-15 — windows — A14 (the guard for this morning's collision)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Built **A14**, filed earlier in this same session after a concurrent
Claude session on this box committed this run's staged changes as Jeff.
[scripts/check-commit-authorship.py](scripts/check-commit-authorship.py), plus
the STEP 3 and STEP 4 wording in
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md) that makes running it
part of every run. **Fourth item this session, at Jeff's direction, and still
TideSynth-only** — the second agent was confirmed still live in `SE16` and
`SynthEditLib`, which this run has not touched since C12b.

**Result — verified against a reconstruction of the actual incident, not by
reasoning about it.** A throwaway repo in the scratchpad, branch of four
commits:

| commit | author / committer | flagged |
|---|---|---|
| `legit bot commit` | bot / bot | no |
| `the foreign commit` | **Jeff / Jeff** | **yes** |
| `another bot commit` | bot / bot | no |
| half-set `GIT_*` | bot / **Jeff** | **yes** |

Exit 1, both flagged, the two clean ones untouched. Also exits 0 on a clean
branch and prints "on the default branch, nothing to check" when there is
nothing to compare, so it is safe to run unconditionally.

**The fourth row is the one I would not have thought to test if the prompt had
not already documented it.** `GIT_AUTHOR_*` exported but `GIT_COMMITTER_*` not
is the exact shape measured on this box before the four variables were
mandated — the run prompt records a bot-pushed test commit coming back from the
API as `author: JeffMcClintock, committer: JeffMcClintock`. Checking only the
author would have let half of it through. The check tests both identities.

**Learned — why the check belongs *before* the push and nowhere else.** A
foreign commit that has been pushed cannot be rewritten; the branch is shared
and the run prompt forbids rewriting pushed history. So the only useful moment
is the last one at which the run still owns its history entirely. That also
shapes the failure output: it prints the remedy (`--amend --reset-author`, or a
`rebase --exec` for a range) **and** the two cases where the remedy must not be
applied — anything already pushed, and anything a concurrent session may be
building on. This morning the branches were unpushed, and that is the only
reason amending was safe. That condition is easy to lose sight of when the
content is obviously yours.

**Learned — the guard STEP 0.7 gives is narrower than it reads.** It is a
property of *the process*, asserted *once*, at the start. Nothing about it is
wrong; it simply cannot see a second writer, and a run that reads it as "this
repository is safe" has over-read it. The new STEP 4 text says this in as many
words, because the failure looked completely normal from inside — correct
content, clean exit codes, a passed STEP 0.7 — and the only visible sign was a
name in `git log` that nobody had a reason to read.

**Also added to STEP 3:** commit as soon as a coherent change exists rather
than staging and going away to build. That window was open about ten minutes
this morning. It is a smaller, softer mitigation than the assertion and does
not replace it — it just makes the assertion fire less often.

**Next:** nothing blocking. **A15** (five lines of `lint.yml` for A10's check)
and this row's own PR both need Jeff. On the carve-out, **C12c** is the win
NEXT item and is the largest dangling-edge reduction of any sub-stage — but it
must wait for C12b to merge, and for the second agent to be clear of `SE16` and
`SynthEditLib`.

**One process note for whoever reads this file next.** Four entries were
written today from one box, which is not the cadence this journal was designed
around. It happened because Jeff was directing interactively and told the run
to keep going; a scheduled run still takes exactly one item. Rotation kept the
file at four entries throughout, so **three of today's four entries are already
in [JOURNAL-2026-08.md](JOURNAL-2026-08.md)** by the time anyone reads this —
look there before concluding a day is missing.

**Branch/PR:** [TideSynth#63](https://github.com/JeffMcClintock/TideSynth/pull/63),
stacked on the A10 branch, which is stacked on C12b. All three retarget to
`main` as their parents merge.

