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

---

## 2026-08-15 — windows — A10 (script half; A15 filed for the gated half)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Built **A10**, the bare-ID cross-reference lint A3 deferred:
[scripts/check-id-refs.py](scripts/check-id-refs.py). **Third item this
session, at Jeff's explicit direction in an interactive session** — and picked
because he confirmed a second agent was still writing to `SE16` and
`SynthEditLib`, so this run stayed inside TideSynth. **The workflow step that
turns the script into an actual gate is not here** — `.github/workflows/**` is
structurally unpushable by the bot token — so that is filed as **A15** with the
exact YAML, per STEP 5's "do the allowed-side part, file the gated part naming
the exact file".

**Result.**

| check | result |
|---|---|
| against the tree as it stands | **447 references checked, 97 rows, 91 distinct IDs, zero flagged** |
| built-in selftest | **20 cases, 0 failed** |
| deliberately stale `**Z9**` | detected, **exit 1** |
| deliberately stale `BLOCKED(C99)` | detected, exit 1 |
| deliberately stale `superseded by P42` | detected, exit 1 |
| clean tree / selftest exit codes | 0 and 0 |
| `scripts/check-links.py` after the change | 208 links, no breakage |

**The row said to budget this as a design session first, and that was right —
the implementation is 40 lines and the design is the whole item.** A10 predicted
the failure mode (a naive `\b[A-Z]\d+[a-z]?\b` false-positives constantly, and a
noisy lint erodes trust in the other four checks) but not the fix. **The fix is
two shape rules, both read off the ID column rather than guessed:**

- **A real ID has exactly one uppercase letter.** All 96 rows do — A, B, C, D,
  E, G, H, L, M, N, P, R, S, U, V, W, X. This single rule kills **`SE16`, which
  occurs 331 times in these docs** and is by far the worst offender, plus
  `SE15` and `SE14`.
- **One or two digits.** The longest real ID is `C12f`/`S10`/`A13`; MSVC
  diagnostics are four. This kills `C1083` (8 occurrences), `C2664` and
  `C4834` — all of which appear in journal entries today.

Everything the row worried about is gone before context is even considered.
Context then does the remaining work: only **bold**, `BLOCKED(...)`, or an
explicit trigger phrase (`see`, `blocked on`, `filed as`, `unblocks`,
`supersedes`, …) counts as a reference, so the row's own example — `P7a` inside
a sentence about `checkSizeConstraint(0,0,2178,32672)` — is never examined, and
neither is a bare `V1`. Fenced blocks, inline code spans and link targets are
skipped.

**Learned — `BLOCKED(<id>)` is the highest-value case and A10's row never named
it.** The row is written entirely about prose mentions. But `BLOCKED(<id>)` is
the one cross-reference this process treats as load-bearing: the backlog says
outright that **eligibility lives in the status column alone** and that prose
never overrides it. A typo there does not read as wrong — `BLOCKED(C12g)` would
sit in the table looking exactly like a valid blocker and would never clear,
because no row will ever be `C12g`. It is 8 of the 447 references and the only
ones where being wrong silently changes what a run is allowed to do. Worth
saying because it inverts the row's own priority: the prose half is hygiene, the
`BLOCKED()` half is correctness.

**Learned — the check has real coverage, which was not obvious in advance.** A
lint that fires on nothing is indistinguishable from one that examines nothing,
so the number to record is not "zero stale" but **447 references examined across
91 distinct IDs**, the most-referenced being C6 (15), then C12, C9, S1b and B1
(11 each). It is not decorative.

**Learned — stacked PRs are a genuine false-positive source, and this run hit it
within minutes.** The C12b PR adds row **A14**; this run's journal entry
references it. Branched from `origin/main`, the reference is dangling — and the
check is *right*, because on that branch the row really does not exist. Two
things came out of it: `--allow-id` as the documented escape, and this branch
being **stacked on `tide/win/C12b-controls` rather than `origin/main`**, which
was the better fix and also avoids a certain `JOURNAL.md` conflict between two
open PRs that both prepend an entry. **Whoever reviews: this PR's base is the
C12b branch and GitHub will retarget it to `main` when C12b merges.**

**Next:** **A15** — five lines in `.github/workflows/lint.yml`, Jeff's to push:

```yaml
      - name: ID cross-references
        id: idrefs
        continue-on-error: true
        run: python3 scripts/check-id-refs.py
```

plus `ID_REFS: ${{ steps.idrefs.outcome }}` in the Summary step's `env` and
`"$ID_REFS"` in its `for` loop. **Both halves matter** — every step is
`continue-on-error`, so a step added without the Summary wiring reports into the
void and fails nothing. That is the same shape as A4's finding that an allowlist
can look built while firing on nothing.

**STEP 1 / 1.5:** unchanged from earlier today — no `platform:win` issues; the
open PRs are this session's own (SynthEdit#19, SynthEditLib#8, TideSynth#61).

**Side effects on this box:** none outside the scratchpad. This run committed in
TideSynth only, and did not touch `SE16` or `SynthEditLib` at all — both were
left on their default branches, clean, before it started.

**Branch/PR:** [TideSynth#62](https://github.com/JeffMcClintock/TideSynth/pull/62),
based on `tide/win/C12b-controls`. No code outside `scripts/`.

---

## 2026-08-15 — windows — C12b (and a second-agent collision worth more than the item)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness; no `claude` CLI on PATH to version) · as `tide-rack-bot`

**Did:** Carve-out sub-stage **C12b** — `Control`, `Ctl_Combo`, `Ctl_Keyboard2`,
`Ctl_Slider`, `Ctl_Text`, ten files and 1,053 lines, moved from private
`SE16/SynthEdit2` into public `SynthEditLib`'s root. **37 → 27
`${EDITOR_DIR}` entries.** Also added a committed measurement script, and
archived C12a to [BACKLOG-DONE.md](BACKLOG-DONE.md) after seeing both its PRs
merge. **This was a second item in one session, taken at Jeff's explicit
direction in an interactive session — not a scheduled run deciding to keep
going.** STEP 2's one-item rule stands for unattended runs.

**Read the collision section first if you are short of time. The stage itself
went exactly as scoped; the collision is the part that changes how a run should
behave.**

### The collision — another agent committed my work as Jeff

Midway through C12b, with the ten files staged in two repos and nothing yet
committed, **a second Claude session (Fable 5) running on this same box picked
up my staged changes and committed them** — on my branches, in both `SE16` and
`SynthEditLib`, at 10:49:35, authored **and** committed as
`Jeff McClintock <jef@synthedit.com>`.

The content was mine and was correct: my CMakeLists comment verbatim, my
repointing, my ten file moves, nothing foreign mixed in. Checked by diffing
against the merge-base before doing anything else. **The damage was purely to
authorship — which is exactly what STEP 0.7's four `GIT_*` variables exist to
prevent**, and the reason they exist is stated in the run prompt: without them
`git log` cannot tell agent work from Jeff's. Here the mechanism was inverted —
the variables were set correctly in *my* environment, and a different process
with a different environment committed my working tree anyway.

**Resolved** by `git commit --amend --reset-author` in both repos with
`GIT_AUTHOR_*`/`GIT_COMMITTER_*` exported, then an immediate push, on Jeff's
instruction. Both commits are now `tide-rack-bot`. He also confirmed the other
session was **still running**, so C12c was deliberately not started and this
run stopped touching `SE16` and `SynthEditLib` after the push.

**What the next run should take from this, in order of usefulness:**

1. **A clean `git status` is not proof your work is uncommitted.** I read an
   empty `status --porcelain` in both repos and briefly took it as the changes
   having been lost. They had been committed by someone else. Check
   `git log -1 --format='%an'` before concluding anything from a clean tree.
2. **The staged-but-uncommitted window is the vulnerable one.** Between `git
   add` and `git commit` the work is in a shared tree with no owner's name on
   it. On a box that may be running another agent, commit as soon as a coherent
   change exists and amend later, rather than staging and going off to build for
   ten minutes — which is exactly what I did.
3. **The identity assertion in STEP 0.7 cannot detect this.** It proves *this*
   process is the bot. It says nothing about any other process with write access
   to the same working trees, and there is currently nothing in the process that
   would notice. **Filed as A14.**
4. Do not rewrite commits a concurrent session may be building on without
   asking. I asked; the answer was to fix the authorship, and it was fine
   because the branches were unpushed. Unpushed is the condition that made it
   safe, not the fact that the content was mine.

### Result — the stage itself, all green

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **37 → 27**, zero named `Control` or `Ctl_*` |
| fresh scratch Ninja tree, Release, configure | RC=0 |
| build | **904/904, RC=0** — unchanged, as a move should be |
| `ctest` | **92/92 passed, 0 failed** |
| the five moved TUs compiled from their **new** home | `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj`, all five |
| stale copies left behind in `SE16` | none |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, links `SynthEdit2.exe` |
| dangling private includes, public repo | **51 → 45** |
| `SE_APP_BUILD_NUMBER` at configure | 185 |

**Zero new dangling edges opened — the first stage of which that is true.** C4
closed 11 and opened 20; C5 closed 15 and opened 10; C12b closed 6 and opened
**0**, so 51 − 6 = 45 exactly. That is the "closed under inclusion" property the
C12 scoping run predicted for all of C12, now measured for one stage rather than
inferred. It is also the cheapest possible check that the moved set is really
self-contained: if any of the five controls had pulled in a private header no
stage owns, the total would have landed above 45.

**Learned — `PatchManager.cpp` was resolving two of these headers from its own
directory, and C12f inherits that.** `SynthEdit2/PatchManager.cpp` includes
`"Ctl_Slider.h"` and `"Control.h"`. Before this stage both resolved
own-directory-first inside `SynthEdit2`; now they resolve through EditorLib's
include path to the public copies. It still builds — that is what 904/904
proves — but **it is the one own-directory resolution C12b disturbed, and it was
found by grepping for it rather than by anything failing.** Whoever takes
**C12f** (which owns `PatchManager`) should know the dependency now runs
private → public. Worth generalising: every later stage should grep the
*private* repo for includers of what it is about to move, not just the public
one, because the public-side scan is blind to this direction.

**Learned — absolute dangling counts are not comparable across runs, only
deltas within one script.** C5's entry reports 59 → 54; this run's script reads
51 before C12b, and C12a cannot have closed any (delisting a source-list entry
changes no `#include`). The gap is definitional — which private directories
count, whether a repeated include counts once or per site. **So the script is
now committed:**
[scripts/dangling_private_includes.py](scripts/dangling_private_includes.py).
The C12 doc said outright that each stage should re-create it; C4, C5 and the
C12 scoping run each did, and each got a number nobody else can reproduce. Its
positive control is that it independently reproduces this stage's Accept line —
6 edges, `ModuleFactory_Editor.cpp` (4) and `CContainer.cpp` (2) — exactly. It
documents the own-directory-first rule that makes `resource.h` zero rather than
71, which is the trap that nearly turned C12a into the largest item in C12.

**Next:** **C12c**, the independent leaves — twelve entries, 1,316 lines,
closing **21** dangling edges, the largest reduction of any sub-stage. **Take it
only once C12b has merged**, or you are moving files out of a tree whose
companion PR is still open. Baselines for whoever does: **904/904, 92/92, and 45
dangling edges** — and measure with the committed script, not a fresh one.

**STEP 1 / 1.5 at the time C12b was claimed:** no `platform:win` issues; the
only open PRs were this run's own C12a pair, both since merged by Jeff.

**Side effects on this box:** two scratch Ninja trees, a build script and an
MSBuild script under the session scratchpad, all outside every repo. The
MSBuild of `SynthEdit2` wrote into `SE16\x64\Release\` — that is gitignored and
left `git status` clean, checked. **Jeff's own `SE16\build` was not touched.**
`SE16` and `SynthEditLib` are back on their default branches; TideSynth is on
this branch until its PR lands.

**Branch/PR:** [SynthEdit#19](https://github.com/JeffMcClintock/SynthEdit/pull/19)
+ [SynthEditLib#8](https://github.com/JeffMcClintock/SynthEditLib/pull/8) —
**these two must merge together**, one removes the files and the other adds them.
This TideSynth PR carries the journal, the backlog and the script, and lands no
code.

---

## 2026-08-15 — windows — C12a

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness; no `claude` CLI on PATH to version, same as the C5 and C12 runs) · as `tide-rack-bot`

**Did:** Carve-out sub-stage **C12a**, the first of the six the C12 scoping run
split out the day before. Four `${EDITOR_DIR}` lines deleted from
`SE16/EditorLib/CMakeLists.txt` — `GuiPin.h`, `Module_Info_Plugin.{cpp,h}` and
`resource.h` — plus the stale stage comment replaced. **Nothing moved and no
file was deleted.** `SE16` `c58e4bc5a` on `tide/win/C12a-delist-dead-duplicate`,
pushed, [SynthEdit#18](https://github.com/JeffMcClintock/SynthEdit/pull/18).
**41 → 37 entries.**

**Result — green, and one number differs from the row's prediction in the
direction that confirms rather than undermines it.**

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **41 → 37** |
| fresh scratch Ninja tree of `SE16`, Release, configure | RC=0 |
| build | **904/904, RC=0** |
| `ctest` | **92/92 passed, 0 failed**, RC=0 |
| artifacts | `TIDE.gmpi`, `TIDE_VST3.vst3`, `SynthEditCL.exe` all produced |
| `Module_Info_Plugin` / `GuiPin` edges in `build.ninja` | **0** / **0** |
| `SynthEdit2.vcxproj` and `.filters` mentions of the four | **0** |
| `SE_APP_BUILD_NUMBER` at configure | **185** — C9's injection still tracking |

**904, where both the row and the plan doc said "still builds 905/905".** That
is the expected consequence of the change, not a regression: dropping
`Module_Info_Plugin.cpp` removes one TU from `EditorLib`, so the ninja edge
count falls by exactly one from the 905 the C12 scoping run measured on
`master` hours earlier. **Both documents predicted the count would hold while
also correctly identifying that this is a real link-surface change needing a
build — the two statements contradict each other and nobody noticed.** Worth
saying plainly because "905/905" was written as an acceptance check, and a
later run reading it literally would have treated a correct build as a failure.
The 92 tests are unchanged, which is the part that actually had to hold.

**Learned — all three delisting claims verified, and the checks are cheap
enough that no future stage should skip them.** The plan doc says *"do not take
it on trust, the checks are two greps"*; re-run this session, all three hold:

- **`GuiPin.h`** — zero includers across `SE16`, `SynthEditLib`, `SynthEditCL`,
  `gmpi_ui` and `GMPI_Wrappers`. The only apparent extra hits anywhere in the
  tree came from `SE16/SynthEdit2/.claude/worktrees/` (two stray agent
  worktrees, gitignored) and `SE16/build/_deps/syntheditlib-src/` — **worth
  knowing for any future grep-based measurement on this box, because both look
  like real source and neither is.** Scope greps to the five repo roots.
- **`Module_Info_Plugin`** — stronger than the row claimed. The row says its
  only construction site is commented out at `ModuleFactory_Editor.cpp:1320`.
  It is also true that the `switch` there has **no `case 3`** at all for its
  class-type id: `case 1` carries the commented-out line and falls through to
  `default`, i.e. plain `Module_Info` with `SetUnavailable()`. So the type is
  unreachable by two independent routes, not one.
- **`resource.h`** — both copies 361 lines; **all 318 `ID_*`/`IDR_*` constants
  byte-identical**; the only differences are a trailing space in a comment and
  `_APS_NEXT_RESOURCE_VALUE` (**210** private, **207** public). That counter gap
  is the measurable sign the private copy has had three resource slots the
  public one has not, which is **P9**'s whole point and the only thing making
  C12a safe.

**Learned — A4 has still not been observed firing, and this run's own PR is the
next chance.** A4's row says to flip it to DONE *"only after watching it merge
one PR and leave one alone"*. [#59](https://github.com/JeffMcClintock/TideSynth/pull/59),
the only candidate since it landed, was **merged by `JeffMcClintock`**, not by
the action — checked via the API rather than inferred from the timeline. The
workflow has run five times with `conclusion: success`, but success includes
"correctly declined", so those runs are not evidence either way. **This run's
TideSynth PR touches only `JOURNAL.md`, `JOURNAL-2026-08.md` and `BACKLOG.md`,
all three on A4's allowlist, and is authored by `tide-rack-bot` — so it should
auto-merge.** Whoever reads this next: check whether it did. If it did, that is
half of A4's flip condition met; the paired SynthEdit PR carries `.txt` build
code and must stay for Jeff, which is the other half. Left A4 IN-REVIEW.

**Learned — the C12 scoping run's "no non-CMake build edits" claim holds for
these four specifically**, checked rather than inherited: `SynthEdit2.vcxproj`
and `.vcxproj.filters` mention none of them. So C12a needed no Visual Studio or
Xcode project edit, as predicted.

**Next:** **C12b** — the ten control files (`Control`, `Ctl_Combo`,
`Ctl_Keyboard2`, `Ctl_Slider`, `Ctl_Text`), 1,054 lines, a comfortable single
session, and it closes 6 dangling edges. The win NEXT row already said "C12a,
then C12b"; it is now just C12b, and the baseline any successor should compare
against is **904/904 and 92/92**, not 905. C12c is the bigger win on dangling
edges (21, more than C5 closed in total) if a box wants that instead — ordering
between sub-stages is a convenience, not a constraint. **Do not take C12f
expecting its stated Accept to pass**: it says *"zero `${EDITOR_DIR}` entries
remain"*, which is only reachable once a–e have landed, so taken now it would
leave 27 and fail its own check as written.

**One thing deliberately not done.** The NEXT row reads "C12a, then C12b", but
STEP 2 says pick exactly one item, and one item is what this run took. C12b is
a `git mv` of ten files into `SynthEditLib` — a second repo, a second PR, and a
second full build — which is not a rider on a four-line delete. Re-pointing the
NEXT row at C12b is this entry's contribution to it.

**STEP 1 / 1.5:** no `platform:win` issues. Zero open PRs across all five repos
at the start of the run; one open issue, TideSynth
[#44](https://github.com/JeffMcClintock/TideSynth/issues/44) (A6 watchdog
digest, `github-actions`, informational). One remote branch from another
platform, `tide/mac/D1-donation-affordance`, with no PR yet — the mac box's live
D1 claim, not a collision with C12a.

**Side effects on this box:** one scratch Ninja tree and a build script under
the session scratchpad, both outside every repo. **Jeff's own `SE16\build` was
not touched** — it is a Visual Studio / Debug tree and was left exactly as
found. All five working copies were clean and on their default branches at the
start of the run, and are back on them at the end.

**Branch/PR:** [SynthEdit#18](https://github.com/JeffMcClintock/SynthEdit/pull/18)
(the code — four deleted lines and the comment) and the TideSynth PR carrying
this entry and the backlog status. **Merging the TideSynth one alone lands no
code**, and merging SynthEdit#18 alone is safe on its own — nothing in the two
depends on the other to build.
