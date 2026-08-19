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

## 2026-08-19 — macos — E13: the MIDI prefab gets an E1 case; the other two uncovered prefabs cannot have one

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Did:** Took the mac NEXT block's fallback — "**E2** or the per-prefab **E1**
cases", E10 being GATED and not a build break. Filed the per-prefab work as
**E13** and shipped it: one new harness case, plus a measured account of why the
remaining two prefabs get none.

TIDE ships **five** prefabs and `tests/cases/` covered **two**. It now covers
three, and the README says — with evidence — why three is the ceiling for an
audio harness.

### The case: `prefab_midi`

`SE MIDItoGate2` was in **no** existing case. `voice_midi_note` goes via
`SE Keyboard2`, so the module the shipped MIDI prefab is actually built on, and
which V3 depends on, was untested. The case records Gate on the left channel and
Trigger on the right, and locks both of the prefab's promises for a note-on:

| channel | reference content |
|---|---|
| L (Gate) | constant full scale for all 23,999 frames — the gate opens and **stays** open |
| R (Trigger) | full scale for exactly **24 samples**, zero thereafter = **0.5 ms** at 48 kHz |

The 0.5 ms independently matches `build-prefabs.py`'s own note about the pulse
(`triggerCounter=22` at 44.1 kHz), which is the closest thing to a second opinion
available without listening.

**Both gates positive-controlled, through the harness itself, not by argument:**

```
control 1  MIDI connect deleted   FAIL  peak=-inf dBFS -- render is silent (<= -90.0 floor)
control 2  3 LSB on 200 samples   FAIL  peakdiff=-80.8 dBFS > -86.0   null=-104.6 dBFS
full suite (5 cases)              5/5 PASS, prefab_midi nulls at -inf (bit-exact)
```

Control 1 is what makes the case worth having: cut MIDI arrival and it goes to
**digital silence**, so the case tests that a note happens, not that a graph
builds. Control 2 is **finding (b) reproduced live on a real case** — the RMS
residual is −104.6 dBFS, comfortably *inside* the −100 dBFS gate, so RMS alone
would have passed the damage; the peak gate is the only thing that caught it.

### The thing I expected to be true and measured to be false

I took this case partly because it looked like a regression test for **E7's**
converter finding — a bool `Gate` into a float patch point is exactly the
mixed-datatype connection that needs an auto-inserted `SE BoolToVolts`
(`ug_base.cpp:1751`), and whose silent abandonment in Release was "a whole class
of silent failure".

**It is not, and nobody should re-derive this.** Removing **both**
`Converters.sem` and `Converters2.gmpi` from the engine's module folder leaves
the render **byte-identical** — same sha256, `a99c6714…` — with no
`==== MISSING MODULES ====` and no error. Nothing on this box declares
`SE BoolToVolts` at all: not the CLI binary (checked as ASCII *and* UTF-32LE,
because wide-string ids are invisible to `strings` — the false negative E2a
warned about) and not any of the 59 module files. SynthEditCL resolves that
conversion some other way.

The general point is worth more than the detail: **this harness measures the
ENGINE, and TIDE's converter linkage is a property of what `SynthEditSem`
links.** No case in `tests/cases/` can stand in for it. A guard for E7's
regression has to live where TIDE's own binary is what renders.

### The reference is bit-exact on Linux CI, first try

Added after the PR went green, because it is a stronger result than this run
expected. `verify` on the PR renders on **Linux**, against the *published*
engine, and compares to a reference this box seeded on **macOS**:

```
PASS  prefab_midi peak=-0.0dBFS null=-infdBFS peakdiff=-infdBFS
5/5 passed.
```

`null=-inf` is **bit-exact** — not "inside the gates", identical. That contrasts
with the two prefab references seeded on macOS on 2026-08-18, whose own case
files warn that their first CI run is an untested cross-platform comparison.
The reason is worth keeping rather than treating as luck: this reference contains
**no floating-point arithmetic to drift**. It is a step and a rectangular pulse,
both at clipped full scale, so there is no phase increment to integrate and
E1a's whole class of cross-platform residual cannot arise. A case with this shape
is expected to be exact everywhere, and a *non*-zero residual here would mean a
real defect rather than platform drift — which makes this case a sharper
instrument than the oscillator ones, not a blunter one.

### Why the other two prefabs get no case — measured, not assumed

**TIDE Output: the harness structurally cannot observe it.** `Sound Out`'s `Out`
pin is an **input**, so the recorder has nothing to attach to:

```
{"cmd":"render-audio","ok":false,"error":"could not connect --from source to recorder"}
```

Recording the patch points feeding it would test the patch points. The prefab's
real promise — that L and R become **two** channels because the input is
`IO_AUTODUPLICATE` — is visible in the connect commands (`to:[…,0]` then
`to:[…,1]`, a *new* pin), which is a graph-shape assertion, not an audio one.
End-to-end it is already covered by the v0.1 fixture pair in `tests/hosts/`.

**TIDE MIDI-CV: there is nothing to render.** It is a facade — four jacks and a
faceplate, every jack fed from *outside* by the root `SE MIDI to CV 2` (E7).
Rendered alone it reproduces its own scaffolding.

Both are now written down in [tests/README.md](tests/README.md) with the
evidence, so the next run does not re-derive either.

### Notes for whoever is next

- **A stale row, observed and deliberately not changed: E12 reads `TODO`, but
  both of its PRs are MERGED** — [SynthEditLib#23](https://github.com/JeffMcClintock/SynthEditLib/pull/23)
  (2026-08-18T23:30Z) and [SynthEdit#54](https://github.com/JeffMcClintock/SynthEdit/pull/54)
  (2026-08-18T23:07Z) — and its own row says the fix is verified at four clean
  shutdowns. I did not flip it: it is not my item, and I did not re-run its
  Accept clause on this box. It is a one-line repro
  (`TIDE_STANDALONE & sleep 9; kill -TERM $!`, then read
  `~/Library/Logs/DiagnosticReports`) and this is the box that can do it.
- **A pushed branch with no open PR:** `tide/mac/V3-midi-findings` sits 2 commits
  ahead of `main` with its PR ([#142](https://github.com/JeffMcClintock/TideSynth/pull/142))
  already **merged** — STEP 5's forbidden third state, left by an earlier run.
  The two commits are `25216c1` and `4e65874`, both E7/S8 findings. Someone
  should confirm whether that content reached `main` by another route and then
  delete the branch.
- **Harness baseline on this box before I touched anything: 4/4 PASS**, engine
  `SynthEditCL V1.6.186`. So the additions did not paper over a red suite.
- **The foreign-scan warning fires on this box** (`/Library/Audio/Plug-Ins/GMPI`),
  which finding (d) says is normal for a developer machine — but it does mean
  none of these local runs *prove* the named module set is what rendered. CI is
  where that gets proven, and CI is still skipped (**B1**/**C7d**).
- **Not built and not claimed this run:** TIDE itself, `SynthEditCL` and
  `SynthEdit`. This item touches only `tests/` and needed no rebuild; the engine
  used was the Release `SynthEditCL` already on disk from earlier today. So I
  have **no evidence either way** about whether mac's default branches build, and
  am not claiming any.

**Next:**

1. **E2** — the rest of the first Eurorack module set. It is the mac NEXT
   block's other named target and is now the larger half of that row. Each new
   prefab should arrive with its own case, and this run's README section is the
   guide to whether one is even possible for it.
2. **E12 wants five minutes from this box** to flip it DONE honestly — see above.
3. **A guard for E7's converter linkage** has nowhere to live today. It needs the
   TIDE binary rendering, which is the REAPER route (`scripts/render-and-measure.py`),
   not this harness. Worth a row if anyone wants that regression caught.

**Branch/PR:** [#168](https://github.com/JeffMcClintock/TideSynth/pull/168). Work done in a throwaway
git worktree, so no shared checkout left its default branch; all six working
copies were clean and on their defaults at start and at finish.

## 2026-08-19 — linux — C7 is four separate problems, not one; C7a done, the other three scoped

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Did:** Took **C7**, the topmost eligible row and the one the linux NEXT block
points at. Measured it before implementing it, found it is not one session, and
did what the C12 scoping run did: **split it, and shipped the one stage that was
completable and verifiable today.**

- **C7a — shipped.** [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59): TIDE's dead private-repo
  references removed from `SynthEditSem/CMakeLists.txt`, the one real one named in place.
- **C7b, C7c, C7d, C7e — filed**, each with Scope / Accept / Size. Inventory and
  reasoning: [docs/c7-clean-clone.md](docs/c7-clean-clone.md).
- **S18 — filed**, a public-repo defect found while inventorying.

### C7's row was wrong about what C7 contains

The row and the last four journal entries all say C7's content is "the 7 public
-file includes resolving only in `SynthEdit2`". **That is one of four things, and
it is the one this box does not own** — it is C13 (in review, windows) and C14
(filed, windows). The other three had never been named:

| # | The dependency | Measured size |
|---|---|---|
| 1 | TIDE's CMake listed five private include paths | **four were DEAD** — C7a |
| 2 | TIDE's own source lives in the private repo | 16 + 10 files, **blast radius 3 files** — C7b |
| 3 | TIDE links `EditorScreenshot` = `SE16/EditorScreenshot/` | on **neither** STEP 5 list — C7c |
| 4 | **`SynthEditLib` cannot configure standalone** | needs a root CMakeLists here — C7d |

**(4) is the one nobody had named and it is the structural gap.**
`SynthEditLib/CMakeLists.txt` consumes `${GMPI_SDK}`, `${GMPI_UI_SDK}` and
`${VST3_SDK}` and **never sets them** — `SE16/CMakeLists.txt:78-161` does, via
the override-or-fetch pattern. So "a stranger can build TIDE" needs a
superproject root `CMakeLists.txt` in *this* repo playing SE16's role for TIDE's
subset. **It is also free CI:** `build.yml`'s guard job (B1) gates the
three-platform matrix on a root `CMakeLists.txt` existing, so C7d turns the
matrix from *skipped* to *running* **with no `.github/workflows/**` edit** — which
matters, because the fleet's token deliberately has no `workflow` scope.

### C7a — four of five private include paths were dead

Each measured separately, not swept:

| Entry | Verdict | Evidence |
|---|---|---|
| `EDITOR_DIR` (`../SynthEdit`) | dead | `set()` once, referenced nowhere in the file |
| `EDITOR2_DIR` (`../SynthEdit2`) | dead | same — the identical pair C6 deleted from `EditorLib/CMakeLists.txt` |
| `../Shared` | dead | **the directory does not exist in the tree** |
| `../SynthEdit` | dead | icons, skins, `.chm`; no headers. `SynthEdit.rc` includes `resource.h` + `windows.h` only |
| `../SynthEdit2` | **REAL** | one consumer: `TideAppStubs.cpp:31` → `SynthEditApp.h` |

Also dropped a stray `PRIVATE` keyword — `include_directories()` is the
*directory*-scoped command and takes paths only, so CMake was adding a relative
directory literally named `PRIVATE`. Harmless, but it made the block read as if
it were `target_include_directories()`.

**Result — fresh Ninja/GCC/Release tree, and a baseline taken in the same tree
immediately before the change so the comparison means something:**

```
baseline   configure RC=0   928/928 RC=0   ctest 67/67
after C7a  configure RC=0   928/928 RC=0   ctest 67/67
```

Zero `error:`, zero `undefined reference`, both runs. `TIDE.gmpi`,
`TIDE_VST3.vst3`, `SynthEditCL` and `SynthEditWayland` all built — so the
standing "leave SynthEdit, SynthEditCL and TIDE building" rule holds on this
platform. **`SynthEdit2` (WinUI3) was not built; it is Windows-only.**

### Two measurements the next stage should not re-derive

**TIDE's own sources have exactly TWO private includes.** Every `#include "..."`
in `SynthEditSem/*.{cpp,h,mm}` resolved against the public repo first, then
`SE16`:

```
SynthEditGui.cpp   "ContainerThumbnail.h"  ->  SE16/EditorScreenshot/   (C7c)
TideAppStubs.cpp   "SynthEditApp.h"        ->  SE16/SynthEdit2/         (C14)
```

**Moving TIDE's source out of `SE16` has a blast radius of three files.**
`grep -rl 'SynthEditSem\|TideModules'` over `SE16`'s build files returns
`CMakeLists.txt` (one `add_subdirectory` at `:409`), `SynthEditSem/CMakeLists.txt`
itself, and `se_gmpi/vst3/CMakeLists.txt` where **both hits are comments**. No
`.vcxproj`, no `.pbxproj`, no `.sln`. That is why C7b is sized at one session.

### A correction to C14's framing, measured rather than inherited

[#165](https://github.com/JeffMcClintock/TideSynth/pull/165) files C14 on the grounds that `SynthEditApp.h` "pulls in
**`moonbasepp_Licensing.h`**". It does — **inside `#ifdef SE_MOONBASE_SUPPORT`**
(`SE16/SynthEdit2/SynthEditApp.h:6-11`), and **that header is not tracked by git
at all**: its own comment says to copy `moonbase_lib/` in from `SynthEdit_Azure`,
and `find` over `~/SE` returns only two workflow files. So "it drags the
licensing surface into the public repo" holds only for a moonbase build. **That
narrows C14 rather than blocking it.** The header is still a private one
declaring a private app class, and that is the real reason it cannot simply move.

### S18 — and why the committed script could not have found it

Three public files include `soundpipe.h`, which resolves only in
`SE16/SDKs/Soundpipe/`: `modules/SoundPipe/ReverbChowning.cpp:4`,
`ReverbSp.cpp:4`, `ReverbZita.cpp:4`. **Not a C7 blocker** — `modules/` is added
by `SE16`'s root (`:416`), never by `SynthEditLib`'s own, and TIDE links none of
it. It is a defect in the public repo *as a public repo*.

`scripts/dangling_private_includes.py` skips `SDKs` by design (`SKIP_DIRS`,
`:57-63`) because vendored SDK headers are not carve-out edges. **That rule is
correct for what the script measures and is exactly why this was invisible.**
Cross-checked both ways this run: script and hand scan agree exactly on the
**seven** carve-out edges across four headers, and differ only here. Use the
script — it is right, and a naive grep is wrong by ~3x for the reason its
docstring gives.

### STEP 1 / STEP 1.5

Two open `platform:linux` issues, both authored by `tide-rack-bot`, **neither
actionable and neither a build break** — so no STEP 1 override:
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88)'s remaining half is `SynthEditJuce`, which its own
CMakeLists calls deprecated and which nothing adds to the build, and
[#156](https://github.com/JeffMcClintock/TideSynth/issues/156) is the ctest path default. Both are GATED-by-default paths
(`SE16/SynthEditJuce/`, `SE16/tests/`), so A17's exception does not reach them.
No open `tide/linux/**` PRs. All six working copies were clean and on their
default branches at start.

**Next:**

1. **C7b** — move `SynthEditSem/` and `TideModules/` into this repo, `SE16`
   consuming them via `TIDESYNTH_FOLDER_OVERRIDE` + `FetchContent`. Both are
   ALLOWED paths, so **no ruling needed**, and it does not depend on C13/C14.
2. **C7c is NEEDS-JEFF** — `EditorScreenshot` is on neither STEP 5 list. G3 is
   the precedent; the answer took one day last time.
3. **C7d after C7b**, and do not expect it to *pass* until C13 and C14 land — a
   clean clone is precisely what those two dangling headers fail.
4. **C13 and [#165](https://github.com/JeffMcClintock/TideSynth/pull/165) merged while this run was working**, which is why this entry's
   branch needed a merge from `main`. Re-measured afterwards rather than assumed:
   `scripts/dangling_private_includes.py` on the merged defaults reports **1**
   dangling private include, down from **7** — and the survivor,
   `ApplySynthEditConfig.cpp:2` → `SynthEditApp.h`, is **the same header TIDE's
   own `TideAppStubs.cpp:31` includes**. So **C14 is now the entire remaining
   private-include debt, and it closes two consumers at once**, one of them
   TIDE's. C7e needs it; C7b does not.

   Note the C7a build numbers above were measured against `SE16` `5ae7fbc67`,
   i.e. **before** C13 merged. C13 moved three headers into `SynthEditLib` and
   did not touch `SynthEditSem/`, so the two changes are orthogonal — but that is
   reasoning, and the re-verification against the post-C13 tip is recorded
   separately on [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59).

**Branch/PR:** [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59) + [#167](https://github.com/JeffMcClintock/TideSynth/pull/167).
Both working copies returned to their default branches.

## 2026-08-19 — windows — C13 filed and shipped; C14 and A27 filed; PLAN's licence claim corrected (interactive session, Jeff directing)

**Did:** Verified C6 on Windows, then took the gap C12 left behind: filed and
executed **C13**, filed **C14** and **A27**, and corrected two false claims in
PLAN.md that Jeff spotted.

### C6 verified on Windows — nobody had

C6 landed verified on a **linux** tree only, and it moves a CMakeLists between
repos, which is exactly the shape MSVC can disagree about. Fresh scratch Ninja
tree, Release: **1020/1020 RC=0**, zero `error C`, **ctest 92/92**, all three
artifacts.

**The specific thing worth checking was the build-number injection**, which C6
moved out of `EditorLib/CMakeLists.txt` into `SE16/CMakeLists.txt` because it
reads a private path — and which **defaults to 0 and fails silently**. It
survived: `SE_APP_BUILD_NUMBER=186`, read from the generated ninja rules rather
than from a configure line scrolling past. Check this on any tree that touches
that seam.

### C13 — the three headers no stage owned

`ISEAppManaged.h`, `IMidiDriver.h`, `ParseSynthEditArgs.h` → `SynthEditLib` root.
850 lines, header-only. `docs/c12-remaining-editor-files.md` had listed them
under "What C12 does not cover" and said they "need their own decision about
which stage's list grows", left "as an explicit gap". C12 and C6 both completed
and they were still sitting there.

**Dangling private includes 7 → 1**, measured.

**The finding: C12's non-CMake-build-edit check does not cover `CMakeLists.txt`,
and this stage needed five build files, not two.** Every prior C-stage grepped
tracked `*.vcxproj`/`*.filters`/`*.pbxproj` for the moving names and got "none".
That convention missed three app source lists:

    CMake Error at SynthEditCL/CMakeLists.txt:22 (add_executable):
      No SOURCES given to target: SynthEditCL

`SynthEditCL`, `SynthEditJuce` and `SynthEditWayland` each carried
`${EDITOR2_DIR}/ParseSynthEditArgs.h`. **A header on a source list is never
compiled, so it contributes nothing but its own existence — and its absence
empties the whole list.** Found by building, not by grepping. **Add
`CMakeLists.txt` to that grep before C10**, which is the next stage that moves
files.

**A measurement error I nearly shipped.** The first dangling run after the move
reported **0 edges** — a better number than predicted, and wrong. The script is
native Python and had been handed MSYS-style paths (`/c/Users/...`), which it
cannot resolve, so it walked nothing and truthfully reported nothing found. The
re-run with Windows paths walks 1338 public and 70 private files and reports the
correct **1**. **A zero from a walker is worthless without a count of what it
walked**; assert the walk found files before believing its result. This is the
same Git-Bash-vs-native-Python path split that bites `/tmp` on this box.

### C14 — the last private include, and it must not move

The 1 that remains is `SynthEditLib/ApplySynthEditConfig.cpp:2` → `SynthEditApp.h`,
which pulls in **`moonbasepp_Licensing.h`**, the commercial licensing library.
Moving it would push the licensing surface into the public repo — the exact
boundary PLAN protects — so it is not a carve-out move. It wants **C11's**
treatment: narrow to an interface. The shape is easier than C11's was, because
`ApplySynthEditConfig.h` **already** forward-declares `class SynthEditApp;` and
both functions take it by reference, so only the `.cpp` needs the complete type.

**C14 is now the single thing between the carve-out and C7**, and C7 gates C10
and the release track R2–R6. C7 is eligible by status and **cannot pass** until
C14 lands, because the clean-clone test is precisely what that one include fails.
C7's row now says so.

### A27 — the NEXT-block lint does not read the Take column

Found the only way it could be: the `any` NEXT row's Take cell said **C6**, C6
had been archived hours earlier, and `lint` was green.

**This is a stale-comment bug, not a logic bug, and I got it wrong first.** My
initial read was "implementation bug — the docstring says the Take column is
checked and it isn't." The code is behaving as intended: the loop at
`scripts/check-next-block.py:174-180` carries a deliberate note saying there is
no "every ID in the Take column counts" rule, because measuring it produced
**seven** false alarms back when Take cells were long editorial paragraphs. That
measurement was correct. **The module docstring was simply never updated** and
still claims the opposite, which is what made the behaviour look like a defect.

**What has changed is the shape of the rows.** `win` and `any` Take cells are now
short fields (`**P3** — the only win-marked TODO left`); `mac` and `linux` are
still paragraphs. So A27 proposes a narrower rule than the one that failed —
**only a bolded ID the Take cell *begins* with** — which catches `win` and `any`,
touches neither `mac` nor `linux`, and should reproduce none of the original
seven. Accept is a positive control out of git history, as A20 did for itself.

### PLAN.md — two false claims, both Jeff's catch

1. **"`SynthEditLib` is a public repo with no LICENSE file"** and "nobody may
   legally use or redistribute it" — stated in the present tense, **twelve days
   after** ISC landed (`a2143a4`, 2026-08-07, both repos, matching GMPI and
   gmpi_ui; L1 resolved and archived). The most consequential thing in that
   document to have wrong: it is the first thing a prospective contributor or
   packager reads, and it told them the project was legally untouchable. Fixed in
   both places it appeared — the section heading and the closing line of "Price
   and funding". The rule the section existed to enforce is kept: **an agent must
   not pick or change a licence.**

2. **"E2 is currently `BLOCKED` on V1"** — checked at Jeff's request and it is
   invalid: **V1 is DONE** (2026-08-18) and **E2 is TODO**, unblocked that same
   day. The sentence sits inside a paragraph explicitly preserved as a superseded
   note, so the record is legitimate; the hazard is the word *"currently"* in
   preserved text. Left verbatim per this file's convention, with a dated warning
   **above** it saying the "currently" is 2026-08-13's, not today's.

**Next:** **C14**, then C7. Nothing else in the carve-out is left.

**Branch/PR:** [SynthEdit#58](https://github.com/JeffMcClintock/SynthEdit/pull/58)
+ [SynthEditLib#26](https://github.com/JeffMcClintock/SynthEditLib/pull/26),
which must merge together, plus the TideSynth PR carrying this entry. Work done
in throwaway git worktrees for all three repos, so no shared checkout was ever
switched off its default branch — the mitigation for this morning's collision.
All shared trees left clean and on their defaults.

---

## 2026-08-19 — linux — C6 DONE; C7 and C10 unblocked, and C7's first move is already measured

**Prompt:** 397330d · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Did:** STEP 4 bookkeeping after Jeff merged C6's two PRs. **C6 → DONE** and archived; **C7 and C10 → TODO**.

**Result — verified on the MERGED default branches from a fresh tree**, not from the PRs, because a two-repo change is only really tested once both halves are on their default branches:

| check | result |
|---|---|
| `EditorLib/CMakeLists.txt` | present in `SynthEditLib`, **gone** from `SE16` |
| configure | RC=0 |
| full build | **935/935 RC=0**, zero `error:`, zero `undefined reference` |
| `ctest` | **67/67** |
| `SE_APP_BUILD_NUMBER` | **186**, unchanged |

**The carve-out now has exactly two stages left, C7 and C10, and C7's opening move is already measured — do not re-derive it.** Configuring the public `EditorLib` standalone with no SE16 anywhere is **RC=0**. A standalone *build* stops on `GmpiUiDrawing.h`, `RawView.h` and `Hosting/message_queues.h`, and **`grep -c "SE16\|SynthEdit2"` over that log is 0**. So the gap between the public repo and a clean-clone build is the external GMPI/gmpi_ui SDKs — packaging, not privacy.

**The one genuinely private dependency that remains** is the include directory SE16 re-adds to EditorLib: the 7 public-file includes that resolve only in `SynthEdit2` (`ISEAppManaged.h`, `IMidiDriver.h`, `ParseSynthEditArgs.h`, `SynthEditApp.h`). C6 did not close them and never claimed to — it made them *visible* by moving them from an invisible include path inside the moved file to an explicit line in SE16's root. **That is the real content of C7's clean-clone test**, and it is C11's territory. Anyone starting C7 should read this paragraph before scoping it.

**Learned — a bookkeeping trap worth naming, because the next run will meet it.** `BACKLOG.md` on `main` currently says **A26 is `TODO`**, and it is not — it is IN-REVIEW with [#163](https://github.com/JeffMcClintock/TideSynth/pull/163) open. The IN-REVIEW mark lives *in that PR*, so it is invisible until the PR merges. **This is BACKLOG A22 exactly.** The protection that still works is STEP 2's claim check: `git ls-remote --heads origin` and `gh pr list --state open` both show `tide/linux/A26-authorship-range` and #163, and STEP 2 says a branch or open PR naming the id from a different platform means the item is taken. **Trust the branch/PR check over the status column when they disagree** — the status column lags by exactly one merge.

**Next:**

1. **C7** — point TIDE at the public repo only, plus the clean-clone CI build that is the carve-out's real proof. `any`. Its scope is the 7 private includes above plus SDK packaging, not a search for what is left.
2. **C10** also just unblocked. `any`.
3. **[#163](https://github.com/JeffMcClintock/TideSynth/pull/163) (A26) is still open** and deliberately so — it changes a rule in `docs/weekly-run-prompt.md`, which reaches all three boxes on their next run. It wants Jeff's eye rather than an auto-merge.
4. Still open and nobody's platform: the `SynthEditJuce` line in [#88](https://github.com/JeffMcClintock/TideSynth/issues/88) and the ctest path default in [#156](https://github.com/JeffMcClintock/TideSynth/issues/156). Both GATED-by-default, neither a build break, so A17's exception does not reach them.

**Branch/PR:** the TideSynth PR carrying this entry. All six repos on their default branches, clean.
