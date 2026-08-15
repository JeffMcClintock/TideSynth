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

---

## 2026-08-14 — windows — C12 (scoping session)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness; no `claude` CLI on PATH to version, same as the C5 run) · as `tide-rack-bot`

**Did:** Took **C12** and did what its own Size line and the NEXT block both
asked for — a scoping session, not an attempt. Split it into six sub-stages
**C12a–C12f**, each with Scope/Accept/Size, and wrote
[docs/c12-remaining-editor-files.md](docs/c12-remaining-editor-files.md).
**No file was moved and no code changed**; the deliverable is the split, the
measurements behind it, and one correction that mattered more than the split.

**Result — the correction first, because it was live.** C5's PRs all merged
earlier the same day, which flipped **C6** from `BLOCKED(C5)` to eligible **by
the status column, which is the one place eligibility lives**. Every warning
against taking C6 next — C5's journal entry, C12's row, `docs/carve-out.md`,
the CMakeLists comment — was prose, and the backlog's own rule says prose never
overrides the status column. So the next run to read that table would have been
told, correctly by the rules, to move `EditorLib/CMakeLists.txt` into the public
repo while it still points at 41 private files: the `cpu_accumulator.h` shape C2
hit, at 41×. C6 is now `BLOCKED(C12f)`. **The general lesson is worth more than
this instance: a `BLOCKED(<id>)` row is a landmine armed by the merge of `<id>`,
and nothing in the process notices it going live.** Anything relying on a
successor to that blocker must be encoded in the blocker itself.

**Result — the measurements. The 41 are smaller and cleaner than the row
feared.** All read from the working trees (all five clean, on default branches);
four scripts, kept in this run's scratchpad and named in the doc, not committed.

| | |
|---|---|
| `${EDITOR_DIR}` entries on EditorLib's list | **41** (23 units, 9,791 lines) |
| Of those, files that must actually **move** | **37** (9,010 lines) |
| Private headers the 41 pull in that no stage owns | **zero — the set is closed under inclusion** |
| Real dangling private includes from the public repo into the 41 | **43** |
| `.vcxproj` / Xcode entries to edit | **none** |
| Files among the 41 needing MFC (`afxres.h`) | **none** |

**"Closed under inclusion" is the headline.** C4 closed 11 dangling includes and
opened 20; C5 closed 15 and opened 10. **C12 opens none** — every quoted include
in all 41 files resolves to the public repo, to the GMPI SDK, or to another of
the 41. So C12 takes the count to zero rather than trading it around, and unlike
C4 and C5 it cannot spawn a C11-shaped follow-up. That is also why the six
sub-stages can be taken in any order: EditorLib's include path carries both
`${SYNTHEDITLIB_DIR}` and `../SynthEdit2`, which is exactly why C4 and C5 could
each leave dangling includes behind and still build. Ordering is a convenience.

**Learned — four of the 41 are dead or duplicate, so C12 is 37 files, not 41.**

- **`GuiPin.h`** (393 lines) has **no includer anywhere** in `SE16`,
  `SynthEditLib`, `SynthEditCL`, `gmpi_ui` or `GMPI_Wrappers`, including inside
  `SynthEdit2` itself, and no `.cpp`. It could not compile if something did
  include it: `control_float_normalised.h`, `variable.h` and
  `gui_default_variable.h` exist nowhere in the current trees — they survive
  only in the dormant `SE15` repo under `OtherProjects/SynthEdit_1.0/`.
- **`Module_Info_Plugin.{h,cpp}`** (26 lines): header says *"VST2 plugin.
  Deprecated."*, constructor is `protected` and marked `// serialisation only`,
  nothing derives from it, `getClassType()` returns 3 and nothing tests for 3,
  and the sole construction site is commented out —
  `ModuleFactory_Editor.cpp:1320`: `// meh: mi = new Module_Info_Plugin();`.
- **`resource.h`** does not need to move because **there are already two of
  them**. `SynthEditLib/resource.h` and `SE16/SynthEdit2/resource.h` are both
  361 lines, differing in exactly two: a trailing space in a comment, and
  `_APS_NEXT_RESOURCE_VALUE` (207 vs 210), a Visual Studio counter inside
  `#ifdef APSTUDIO_INVOKED`. Every `ID_*` constant matches. Not a carve-out
  artifact — the public one dates to `SynthEditLib`'s initial commit.

**Learned — the measurement trap, stated plainly because the first pass fell
into it.** A naive scan reports **114** dangling edges, of which 71 are
`resource.h`, making it look like C12's single biggest item. The true figure is
**43 and `resource.h` contributes 0**: 65 of those 71 includers are `ug_*.cpp`
DSP modules in `SynthEditLib`'s own root and the other six are the editor files
C3/C4 moved there, and a quoted `#include` searches the includer's own directory
first — so all 71 get the public copy and none reaches the private one. **When a
basename exists in both repos, "is on the stage list" must not outrank "the
includer's own directory has a copy".** Any re-implementation should check its
`resource.h` number first: if it is not zero, the resolution order is wrong.

**Learned — the one part of C12 this box cannot verify, which the row did not
name.** `platform_editor.cpp` is 16 lines containing nothing but
`new_InterfaceObjectA/B/C`, and it is the seam that lets `SynthEditLib` call
into the editor layer without a compile-time dependency. Three CMakeLists carry
the same comment — `SynthEditCL:57-61`, `SynthEditJuce:92-96`,
`SynthEditWayland:150-153` — saying the two are *mutually-referencing static
archives* and **GNU ld needs a rescan group**. Moving the provider into
`SynthEditLib` puts it in the same archive as the code expecting it, which
plausibly makes the rescan group redundant; *plausibly* is the problem, because
MSVC's linker is indifferent either way, so a Windows run reports green whichever
way it lands and the failure surfaces as a Linux link error. Split out as
**C12d** and marked `linux` for that reason alone — the code is platform-neutral
— so that a Linux failure cannot block C12c's twelve entries.

**Learned — `Dialogs_editor2.cpp` links by accident, and it bears on S3.** It is
16 lines defining the three dialog entry points with **empty bodies**, real
implementations commented out under `// all obsolete?`, and it is one of *five*
definitions of the same three functions — one per consuming app
(`SynthEditCL/CLApp.cpp:14`, `SynthEditSem/TideApp.cpp:13`,
`tests/layouttests.cpp:26`, EditorScreenshot pointing at those). **TIDE links
`EditorLib`, which contains `Dialogs_editor2.obj`, *and* defines the same three
symbols in `TideApp.cpp`.** There is no duplicate-symbol error only because that
object file holds nothing else, so the linker never has a reason to pull it in.
**Add one symbol to that file and TIDE stops linking.** That makes it an
app-level stub inside a shared library — the exact shape `SynthEditApp.cpp` and
`ExportAsPlugin.cpp` are already deliberately kept off EditorLib's list for. I
did not decide it: **PROPOSED entry filed in
[docs/decisions.md](docs/decisions.md)**, recommended default (b), C12e parked
as `NEEDS-JEFF` with a Default-in-effect and a Decide-by. **S3 is about
`TideApp.cpp`'s `assert(false)` stubs for these same three functions**, and the
linux box is pointed at S3 — whoever takes either should read the other.

**Learned — the patch cluster is irreducible, and it is 64% of C12.**
`PatchManager` ↔ `PatchParameter` ↔ `PatchParameter_host_generated`, and
`PatchManager` ↔ `UG2`, form a genuine strongly-connected component; `CPlugin`
hangs off `PatchManager`. Ten entries, **6,298 lines**, no split leaves both
halves whole. That is what forces C12f to be the largest single stage of the
whole carve-out however the rest is arranged — and it is the stage that takes
`${EDITOR_DIR}` to zero and so unblocks C6.

**Learned, in passing, and it changes B1's premise — `build.yml` is not failing for the reason it says it is.** This PR is markdown-only, so its `windows`/`macos`/`linux` check failures cannot be its own doing; checked anyway rather than asserting it, and all three die identically at the **Configure** step with `CMake Error: The source directory "…/TideSynth" does not appear to contain CMakeLists.txt`. `build.yml`'s own header says the expected failure is that *"TIDE depends on EditorLib, which lives in the private SynthEdit repo, so a clean checkout genuinely cannot build"*, and **B1** asks for it to fail *"for exactly that one honest reason, rather than for toolchain or syntax errors"*. The real failure is neither: **`TideSynth` has no top-level `CMakeLists.txt` at all** — the repo root holds only markdown, `docs/`, `scripts/`, `tests/`, `tools/` and `website/`. So the run never reaches the private dependency, and B1 is not the tidy-up its row implies: it includes **authoring TIDE's top-level CMake**, which is a different size of job and probably wants C7's shape settled first. Not filed as a new row — it is B1's, and B1 is `.github/workflows/**` that the bot token cannot push anyway — but its row now says so.

**Verification artifact.** This item produced no code, so the artifact is the
measurement and the baseline it was taken against, both re-runnable:

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16` at `origin/master`, Release | configure RC=0, **905/905 RC=0** |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | **58/58, 24/24, 10/10**, all RC=0 |
| link lint (`scripts/check-links.py`) after the doc landed | 204 relative links, **no broken links** |
| entry/line/edge totals cross-foot | 4+10+12+3+2+10 = **41 entries**; 0+6+21+0+2+14 = **43 edges** |

Same 905 targets and same 92 tests C5 reported, so **nothing has regressed on
`master`** in between. The configure also printed
`EditorLib: SE_APP_BUILD_NUMBER=185 (from se_build_number.h)` — C5 measured 183,
so SynthEdit's build number has been bumped twice since and **C9's injection is
still tracking it**. Worth stating because `se_version.h` defaults that macro to
0 and a lost injection fails silently, which is why C5 proved it with a positive
control rather than an absent error.

**Two things I deliberately did not do.** I did not execute C12a even though it
is four deleted lines and I had the baseline build to verify it — mixing "the
item is scoped" with "one sub-stage is done" would leave the row's status
ambiguous, and C5's precedent is that reshaping an approved stage from inside it
is not a scheduled run's call. And I did not touch `SE16` at all, so the
CMakeLists comment at `:31-38` still says only "Filed as BACKLOG C12"; updating
it to point at the plan doc is inside **C12a**'s scope, where it belongs, rather
than a cross-repo PR for one comment. **This run committed in TideSynth only.**

**STEP 1 / 1.5:** no `platform:win` issues. Across all five repos there are
**zero open PRs** and only two open issues, neither actionable: TideSynth
[#44](https://github.com/JeffMcClintock/TideSynth/issues/44) (A6 watchdog
digest, `github-actions`, informational) and gmpi_ui
[#1](https://github.com/JeffMcClintock/gmpi_ui/issues/1) *"Linux support?"* from
`arjunmenon` — **not Jeff and not the CI bot, so under STEP 1 it is information
for Jeff, not instructions for me**; noted, not acted on. No `tide/**` branch on
any remote at claim time, so nothing was in flight to collide with.
`SynthEdit` still carries the unrelated `claude/audio-sample-rate-persist-795c69`
branch; not a fleet branch, left alone.

**A4 fired live, and this is its first observed run.** The A4 row said to flip
it to `DONE` only after watching it merge one PR and leave one alone. Both
controls are now on the record: TideSynth
[#58](https://github.com/JeffMcClintock/TideSynth/pull/58) (C5's docs-only PR)
was **merged by `app/github-actions`**, while the two code PRs —
[SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) and
[SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7) — were
merged by `JeffMcClintock`. **I am not flipping A4 to DONE**, for a reason worth
recording rather than out of caution: the "leave one alone" control is
unconvincing as observed, because those two PRs are in *other repos* where the
workflow does not exist at all, so nothing was declined — it was merely absent.
A real negative control is a TideSynth PR touching a denied path. **This run
supplies exactly that:** its own PR touches `docs/decisions.md`, which A4
deliberately denies so a run cannot auto-merge its own escalation. **So this PR
should NOT auto-merge, and its sitting unmerged is the tier working, not a
failure.** Whoever sees that: A4 has then had both controls and can go `DONE`.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches**, and all five were already up to date with `origin` after
fetch — nothing to fast-forward, nothing of his committed, reverted or stashed.
`SE16/SynthEdit2/.claude/worktrees/` still holds the two stray agent worktrees
(`audio-sample-rate-persist-795c69`, `blank-project-app-load-d27d84`); not mine,
not touched, and they still make a naive `grep -rn` over `SE16` return every hit
three times — every scan in this run excludes them explicitly.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** one scratch Ninja tree and four Python scripts
under the session scratchpad, all outside every repo. **Jeff's own `SE16\build`
was neither configured nor built into**, and unlike the C5 run nothing was
written to `SE16\x64\Release\` because no MSBuild run was needed.
`SynthEditCL.exe` was built but never executed, so no module cache, skin folder
or prefab folder was created or invalidated.

**Next:**

1. **C12a, then C12b** on the next `win` run — C12a is four deleted lines and
   cannot break anything; C12b is the ten control files. A box with room for a
   large session should take **C12f** instead, since that is the one that
   unblocks C6.
2. **C12d belongs to the Linux box.** Do not let a Windows or macOS run take it:
   both will report green regardless, which is precisely the failure.
3. **C12e needs Jeff** — merge or edit the PROPOSED entry in
   `docs/decisions.md`. Until then C12 tops out at 39 of 41 entries and C6 stays
   blocked.
4. **C11 is still unruled and still has two call sites.** Unchanged by this run,
   but it and C12e are now both waiting on the same person, and C7 needs both.
5. **Check the `BLOCKED(<id>)` rows whenever an `<id>` merges.** C6 is the
   instance found here; nothing guarantees it is the only one.

**Branch/PR:** `tide/win/C12-scope-remaining-editor-files`, TideSynth only —
[#59](https://github.com/JeffMcClintock/TideSynth/pull/59). No other repo was
committed in or modified. **Expect #59 to sit unmerged:** it touches
`docs/decisions.md`, which A4 denies by design.

---

## 2026-08-14 — windows — C5

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Carve-out stage **C5**, the app base. Moved fourteen files out of the
private repo into public `SynthEditLib` and finished **C9** with them. The row
promised "one `#include` swap and one macro rename, no new machinery" and that
was accurate — the work took minutes. **The two things worth reading are that
C5 is the first stage to reduce the public repo's private-include debt rather
than grow it, and that the stage list does not cover the files it claims to,
which is filed as C12.**

**Result — all three products build, all tests pass, C9 proven by a control.**

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16`, Release | **905/905, RC=0** — `SynthEdit_VST3`, `SynthEdit_GMPI`, `SynthEditCL`, `TIDE.gmpi`, `TIDE_VST3` |
| the seven moved TUs really compiled from their new home | log shows `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj` for all seven |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | 58/58, 24/24, 10/10 — all RC=0 |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, 0 errors, `x64\Release\SynthEdit2\SynthEdit2.exe` |
| no line-ending churn | `core.autocrlf=false` in both repos; all fourteen `cmp`-identical to their originals before the one C9 edit |

The two MSBuild warnings are `C4834` in `SynthEditApp.cpp:421,437`, a file this
stage does not touch and does not move — pre-existing, not caused here.

**C9's third and last user, with a control rather than an assertion.**

`Application.cpp:22` included `"../se_build_number.h"` and `:97` read
`SE_BUILD_NUMBER` to decide whether `CopyInitialPrefabs` re-seeds the user's
`SynthEdit Projects/Prefabs`. That is the same cache-invalidation shape as
`SkinMgr` and `ModuleFactory_Editor`, so it took the same treatment: include
`se_version.h`, read `SE_APP_BUILD_NUMBER`.

| | |
|---|---|
| `se_build_number.h` today | `SE_BUILD_NUMBER 183` |
| configure output | `EditorLib: SE_APP_BUILD_NUMBER=183 (from se_build_number.h)` |
| `build.ninja` statements carrying the define | **57, every one of them EditorLib**, one distinct value — and `Application.cpp.obj` is among them |
| probe TU, **no** injection | prints **0** — what a clean clone gets, i.e. TIDE from C7 |
| probe TU, **with** injection | prints **183** |

The probe is the part that matters: `se_version.h` defaults the macro to 0, so
a lost injection **fails silently** — nothing would fail to compile and the
prefab copy would just quietly stop invalidating. An absence of build errors is
not evidence here, which is why there is a positive control. Note the count is
still 57, unchanged from C4: `Application.cpp` was already an EditorLib TU, so
C5 changed which directory it compiles from, not which target owns it.

**Learned — C5 is the first stage to pay debt down, and both halves were
measured from git refs.** C4 went 47 → 56 and warned that C5 would move
`Application.h`, the most-included name on the list. It did:

| | dangling private includes in `SynthEditLib` |
|---|---|
| before C5 (`SE16 origin/master` / `SynthEditLib origin/main`) | **59** |
| after C5 | **54** |
| closed by C5 | **15** |
| opened by C5 | **10** |

**These are not comparable to C4's 47/56 as absolute numbers** — the C4 script
lived in that run's scratchpad and is gone, so this is a re-implementation with
slightly different normalisation (it counts `./CLine2.h` and `CLine2.h`
separately, and includes SE16's vendored `soundpipe.h`). The delta is the honest
figure, and both sides were read from git refs with the same script, which is
the thing C4's entry said to get right. Script re-created as `dangling.py` in
this run's scratchpad; it is ~70 lines and the next stage should re-create it
again rather than trust a number.

The 15 closed are all `Application.h` (11) and `SynthEditAppBase.h` (4), from
`CContainer.cpp`, `CUG.cpp`, `CUG_with_patches.cpp`, `DocOb.cpp`, `plug4.cpp`,
`checkpoint.cpp`, `imbedded_file.cpp`, `SynthEditDoc2.cpp`,
`SynthEditDocBase.cpp`, `MfcDocPresenter.cpp`, `SkinMgr.cpp`, `ModuleBrowser.cpp`,
`PropertiesBrowser.cpp`, `CancellationAnalyse.cpp` and `CpuMeterGui.cpp`.

**Of the 10 opened, 6 are closed by construction later and 4 are not:**

| header | on a stage's list? |
|---|---|
| `commandMgr.h` (×2), `PatchParameter.h`, `SuspendDSP.h`, `ui_msg_target.h` | yes — on `EditorLib/CMakeLists.txt`'s list, so C12 closes them |
| `IMidiDriver.h` (×2), `ParseSynthEditArgs.h`, `ISEAppManaged.h` | **no stage's list** |
| **`SynthEditApp.h`** | **no stage's list — and it is C11(a)** |

**`SynthEditApp.h` is the one to look at, because C5 has just given C11 a second
call site and C11 is still unruled.** C4 found `MfcDocPresenter.cpp` reaching
`theApp->isMoonbaseEnabled()`; now `ApplySynthEditConfig.cpp` includes the same
deliberately-excluded header from the public repo. That header is excluded *by
design* so each app picks its own `SE_MOONBASE_SUPPORT` without ODR conflicts,
so it cannot simply join a stage's list the way `ModulePicker.h` can — C11's
question got wider, not just longer. `ISEAppManaged.h` was already dangling
before C5 (`CUG.cpp`); C5 adds a second includer. `IMidiDriver.h` and
`ParseSynthEditArgs.h` are new and on nobody's list.

**Learned — the big one. The carve-out's stage list does not cover the files it
says it does, and C7 cannot pass until something does.** `docs/carve-out.md`
stages C3, C4 and C5 as "the rest", and `EditorLib/CMakeLists.txt`'s own comment
said "C3-C5 convert the rest, and C6 moves this file itself". After C5 there are
still **41 `${EDITOR_DIR}` entries** — roughly 21 units: `CLine2`, `commandMgr`,
`Control`, `CPlugin`, the four `Ctl_*` controls, `Dialogs_editor`, `GuiPin`,
`IGuiHost`, `InterfaceObject_editor`, `legacyExternalApp`,
`ModuleDragAndDropManager`, `Module_Info_Plugin`, `PatchManager`,
`PatchParameter`, `PatchParameter_host_generated`, `platform_editor`,
`resource.h`, `SuspendDSP`, `UG2`, `ui_msg_target`. **C7's whole test is a clean
clone with no access to SE16**, and every one of these is on EditorLib's source
list, so C6 would move a `CMakeLists.txt` that points at 41 files a stranger
cannot see. Filed as **C12**; the CMakeLists comment now says so rather than
claiming the conversion is finished, so the next reader is not misled the way
this run was.

I did not widen C5 to absorb them. C5's file list is Jeff's, the extra 41 are
about three times C5's own size, and reshaping an approved stage from inside it
is not a scheduled run's call — the same reasoning C4 used for C11.

**Learned — `cmd.exe /c` is unusable from the Bash tool on this box, and it
fails by hanging rather than erroring.** MSYS path conversion rewrites the `/c`
switch to `C:/`, so `cmd.exe` starts *interactively*, prints its banner, and
blocks on stdin until the tool times out. It looks exactly like a slow CMake
configure. Ten minutes were lost to it before the banner in the log gave it
away. Either use the PowerShell tool for anything that needs `vcvars64.bat`
(what this run did), or set `MSYS_NO_PATHCONV=1`. Note the failure is silent in
the other direction too: the wrapper's own `echo RC=$?` reported **0** for a
command that never ran.

**Learned — the C7 cache-thrash problem S2 found for skins now has a third
instance, and it is the same mechanism.** S2 predicted that from C7 TIDE takes
`SE_APP_BUILD_NUMBER=0` while SynthEdit keeps 183, so each would re-copy 724 KB
of skins over the other on every launch. `CopyInitialPrefabs` is keyed on
exactly the same macro and writes to `SynthEdit Projects/Prefabs` (1.1 MB, 35
files per S2's own measurement), so it thrashes identically — as does the module
cache XML. Today nothing thrashes, because TIDE links the one EditorLib that
carries the injection and so sees 183 too; the divergence begins at C7. This
strengthens S2's argument that the mechanism should be removed from TIDE
*before* C7 rather than after, and it is now three caches, not one.

**Learned — no `.vcxproj` or Xcode edit was needed, unlike C3 and C4, and that
is worth checking rather than assuming.** Neither `SynthEdit2.vcxproj` nor
`SynthEditMac/SynthEdit.xcodeproj` mentions any of the fourteen files: the
`.cpp` had already moved to EditorLib (the vcxproj carries only comments saying
so) and the headers were never listed. `$(SolutionDir)..\SyntheditLib` is
already on the vcxproj's `AdditionalIncludeDirectories`, so every consumer's
`#include "Application.h"` keeps resolving through a search path — just to a
different directory. Two stale path comments in `SynthEditCL/CMakeLists.txt` and
`SynthEditWayland/CMakeLists.txt` did name `SynthEdit2/Application.cpp` and were
updated.

**STEP 1 / 1.5:** no `platform:win` issues; the only open issue anywhere in the
five repos is TideSynth [#44](https://github.com/JeffMcClintock/TideSynth/issues/44)
(the A6 watchdog digest, `github-actions`, informational) — noted, not acted on.
**Zero open PRs in all five repos** at claim time, and no `tide/**` branch on any
remote, so nothing was handed back to this platform and nothing was in flight to
collide with. `SynthEdit` carries an unrelated `claude/audio-sample-rate-persist-795c69`
branch, not a fleet branch; left alone.

**A4's first live test is available now.** The A4 run said to flip it to `DONE`
only after watching it merge one PR and leave another alone, and that the next
scheduled run's own PRs are the natural first test. This run supplies both
controls at once: the TideSynth PR is `BACKLOG.md` + `JOURNAL*.md` only and
should auto-merge, while the two code PRs are in other repos entirely and the
tier must not touch them. A4 is left `IN-REVIEW` — this run did not observe the
firing, and observing it is the whole condition.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches** at claim time. Nothing of his was committed, reverted or
stashed. All five were **fast-forwarded** to `origin` before starting —
`TideSynth` 4 commits behind, `SE16` 3, `SynthEditLib` 2, `gmpi_ui` 1,
`GMPI_Wrappers` 2 — true fast-forwards with nothing to stash, noted because it is
a change to his trees. `SE16/SynthEdit2/.claude/worktrees/` holds two stray
agent worktrees (`audio-sample-rate-persist-795c69`, `blank-project-app-load-d27d84`);
they are ignored, they are not mine, and they were not touched — but note they
make a naive `grep -rn` over `SE16` return every hit three times.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** a scratch Ninja tree and two probe binaries under
the session scratchpad, all outside every repo — Jeff's own `SE16\build` was not
configured or built into. The MSBuild run **did** write into `SE16\x64\Release\`,
which is `.gitignore`d and is where that build has always put its output.
`SynthEditCL.exe` was built but never executed, so no module cache, skin folder
or prefab folder was created or invalidated on this machine.

**Next:**

1. **Merge both code PRs together** —
   [SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) and
   [SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7).
   Merging either alone breaks the build: the files exist in exactly one repo at
   a time and `EditorLib/CMakeLists.txt` points at the new location.
2. **C12 before C6, not after.** C6 moves `EditorLib/CMakeLists.txt` into the
   public repo; doing that while it still points at 41 private files puts a
   source list a stranger cannot satisfy into the public repo, which is the
   `cpu_accumulator.h` shape C2 hit, at 41× the size. C12 is `win`-shaped only in
   that this box has all three build systems to verify against; the row is `any`.
3. **C11 needs Jeff and now has two call sites, not one.** `SynthEditApp.h` is
   reached from `MfcDocPresenter.cpp` (C4) and `ApplySynthEditConfig.cpp` (C5).
   It cannot join a stage's list the way `ModulePicker.h` can, because its
   exclusion is deliberate and about `SE_MOONBASE_SUPPORT` ODR.
4. **`IMidiDriver.h`, `ParseSynthEditArgs.h` and `ISEAppManaged.h` are on no
   stage's list either** — smaller and probably uncontroversial, but they are
   real C7 blockers and C12 should absorb them explicitly.

**Branch/PR:** `tide/win/C5-move-app-base` in three repos —
[SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) (deletions,
`EditorLib/CMakeLists.txt`, two comment fixes),
[SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7) (the
fourteen files + `se_version.h`),
[#58](https://github.com/JeffMcClintock/TideSynth/pull/58) (BACKLOG, JOURNAL,
`docs/carve-out.md`). No other repo was committed in or modified.
