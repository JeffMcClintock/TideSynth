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
4. **C13 is still open** ([SynthEdit#58](https://github.com/JeffMcClintock/SynthEdit/pull/58) + [SynthEditLib#26](https://github.com/JeffMcClintock/SynthEditLib/pull/26), must merge
   together) and C14 is filed inside [#165](https://github.com/JeffMcClintock/TideSynth/pull/165), which is also still open. Both
   windows. C7e needs them.

**Branch/PR:** [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59) plus the TideSynth PR carrying this entry.
Both working copies returned to their default branches.

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

## 2026-08-19 — linux — A26: the authorship check fails on what you can fix, and reports the rest

**Prompt:** 397330d · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Fifth item this session**, on Jeff's instruction ("if all good take the next task"). Claimed with a pushed DOING mark first. Topmost eligible TODO/`any` row; C7 and C10 were skipped correctly — they are `BLOCKED(C6)` and C6's two code PRs are still open, and STEP 2 says never start a BLOCKED item.

**Did:** Fixed the STEP 2 / STEP 4 contradiction. `scripts/check-commit-authorship.py` plus the STEP 4 wording in `docs/weekly-run-prompt.md`.

### The fix is not the one the row proposed, and the difference matters

A26 suggested passing `--range <pre-run tip>..HEAD`, or defaulting to the upstream. **Both narrow the range, and narrowing the range is wrong** — the old docstring already explained why, and it was right: a branch pushed once has an upstream, so comparing against it hides a foreign commit that an *earlier attempt of this same run* had already pushed. Whoever wrote that comment had thought about it.

The range was never the problem. **The verdict was.** So severity is now decided per commit, by whether the run can still act on it:

| commit | verdict | why |
|---|---|---|
| misattributed, **not yet pushed** | **BLOCKING**, exit 1, "do not push" | A14's case exactly. `--amend --reset-author` is available. **Unchanged.** |
| misattributed, **already pushed** | **ADVISORY**, exit 0, printed in full | the run cannot rewrite it — STEP 4 forbids that — so failing demands the one forbidden action |

Nothing is hidden either way; only the verdict moves. `--strict` restores the old fail-on-everything behaviour, so nothing is lost.

**Result — A/B on a real branch, not just the synthetic cases.** `tide/win/competitive-review` is genuinely three Jeff-authored interactive commits, which is precisely A26's scenario:

```
old (--strict):  rc=1   "Do not push"        <- the deadlock, reproduced
new (default):   rc=0   all three listed as ALREADY PUSHED -- not blocking
```

### The selftest earned itself inside five minutes, and the bug is the interesting part

Added `--selftest`, which builds throwaway repos and pins five cases. It failed on first run — and the bug was mine, in this change, and it was **silent and in the dangerous direction**.

`unpushed()` set-matches SHAs against `git rev-list` output, which is always full 40-char. `FORMAT` used `%h`. **No commit ever matched, so every commit was classified "already pushed", so nothing ever blocked.** The check would have exited 0 on the exact A14 scenario it exists to catch — a concurrent session's local commit — while printing a confident, reasonable-looking report. Every one of my real-repo spot checks still passed, because they had no misattributed commits to misclassify.

`FORMAT` now uses `%H`, with the reason recorded beside it. **The general lesson: a check that can only fail open needs a test that makes it fail.** Three of this session's five items have now turned on measuring something instead of reasoning about it; this is the one where the thing being measured was my own work.

**Learned — the five cases are worth keeping in this shape.** Case 5 (a pushed foreign commit *and* an unpushed one on the same branch) is the one that would catch a future regression collapsing the two categories: it must report one and block on the other in the same run.

**Next:** **C7** — point TIDE at the public repo only, plus the clean-clone CI build that is the carve-out's real proof. Still `BLOCKED(C6)` until [SynthEdit#57](https://github.com/JeffMcClintock/SynthEdit/pull/57) and [SynthEditLib#25](https://github.com/JeffMcClintock/SynthEditLib/pull/25) merge. **C7's starting point is already measured** — see the C6 entry: the public `EditorLib` configures standalone RC=0 and stops only on `GmpiUiDrawing.h` / `RawView.h` / `Hosting/message_queues.h`, with zero private-repo references. **C10** also unblocks on C6.

**Also worth someone's eye:** A21, A22, A23 and A24 are all process rows of the same family as A26 — each one a rule that contradicts another rule or a check that misfires. They are cheap, they are `any`, and every one of them was filed by a run that lost time to it.

**Branch/PR:** the TideSynth PR carrying this entry.

---

## 2026-08-19 — linux — C6: EditorLib's CMakeLists is public, and the plan its own comment left was wrong

**Prompt:** 397330d · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Fourth item this session**, on Jeff's instruction ("take next task"). Claimed properly: DOING mark committed and **pushed before any work**, per STEP 2.

**Did:** Carve-out **stage 6** — moved `EditorLib/CMakeLists.txt` into the public `SynthEditLib`, beside the ~120 sources it already compiles.

**Result — fresh tree, Linux, GCC, Release:** configure RC=0, **935/935 RC=0**, zero `error:`, zero `undefined reference`, **ctest 67/67**, `SE_APP_BUILD_NUMBER` **186** and unchanged.

### What was actually in the way — three kinds of private reference, all measured

| reference | disposition | evidence |
|---|---|---|
| `EDITOR_DIR`, `EDITOR2_DIR` | deleted | after C12d, **zero** uses left in the file — both were pure dead weight pointing at `SE16/SynthEdit2` |
| `../Shared`, `../SynthEdit` include dirs | deleted | `../Shared` **does not exist in the tree at all**; `../SynthEdit` holds only icons and skins. Dropped both and rebuilt clean before trusting it |
| `../SynthEdit2` include dir | kept, re-added by SE16 | genuinely load-bearing: the 7 public-file includes that resolve nowhere else |

**Do not read C6 as closing those seven.** `ISEAppManaged.h`, `IMidiDriver.h`, `ParseSynthEditArgs.h` and `SynthEditApp.h` are exactly the headers no carve-out stage owns; they are C7's clean-clone problem and are tracked as C11. C6 moves the file; it does not make the private dependency go away, and the include directory being supplied from SE16's root is that dependency made *visible* rather than removed.

### The finding: the plan this file left for its own successor does not work

The pre-C6 comment said, in the file, that when C6 moved it the `SE_APP_BUILD_NUMBER` injection "belongs in each SynthEdit application's own build (SynthEdit2.vcxproj, SynthEditCL, SynthEditMac)". **It cannot.** The definition is `PRIVATE` to EditorLib, so it is baked in when **EditorLib's own** TUs compile — `ModuleFactory_Editor.cpp`, `SkinMgr.cpp`, `Application.cpp` — and this tree builds **one** EditorLib that `SynthEditCL`, `SynthEditWayland`, `SynthEdit2` and TIDE all link. A definition set on an application target cannot reach those TUs. Following that instruction would have silently dropped every consumer to the `0` default, which means "never invalidate the module cache or skin folder on upgrade" — a behaviour regression that nothing would have failed on, because 0 is a legal value and the build stays green.

Kept as **one** injection on the shared EditorLib target, moved to SE16's root immediately after the `add_subdirectory`. Verified it reaches the compiler rather than just the configure log:

```
-- EditorLib: SE_APP_BUILD_NUMBER=186 (from se_build_number.h)
$ grep -o '\-DSE_APP_BUILD_NUMBER=[0-9]*' build.ninja | sort -u
-DSE_APP_BUILD_NUMBER=186
```

**Learned, and this is the third instance this session:** a comment or Accept clause written by the stage *before* the one doing the work has now been wrong three times in a row — C12f's "zero entries" (was three), C12d's rescan-group premise (twice wrong), and now C6's injection plan. Each was wrong in the direction of "the next stage will be easy", and each was caught only by measuring before implementing. **Treat a predecessor stage's instructions as a hypothesis, not a specification.**

### Does C6's goal hold? Measured, and the answer is precise

C6 exists "so the public repo can build the editor library standalone". Configuring the public `EditorLib` alone, no SE16 anywhere: **RC=0**. A standalone *build* then stops on `GmpiUiDrawing.h`, `RawView.h`, `Hosting/message_queues.h` — and **`grep -c "SE16\|SynthEdit2"` over that build log is 0**.

**Zero private-repo references.** What remains between the public repo and a standalone editor library is the external GMPI / gmpi_ui SDKs, which SE16 fetches and a public consumer must fetch too. That is C7's scope. This is the cleanest evidence available that C6 did its job, and it is worth re-running as C7's starting point rather than re-deriving.

### Not verified, not claimed

**Windows and macOS were not built** — cannot compile them here. `SynthEdit2.vcxproj` and the SynthEditMac Xcode project consume the EditorLib *target*, not `EditorLib/CMakeLists.txt` by path, so neither should notice the move — but that is reasoning, not measurement, and the run prompt is explicit about not claiming a platform I cannot build. **The v0.1 audio harness did not run: REAPER is not installed here.**

**Next:** **C7** — point TIDE at the public repo only, and the clean-clone CI build that is the carve-out's real proof. It is BLOCKED(C6) and stays blocked until these two PRs merge; do not start it before then. Its first concrete task is already measured above: the three external SDK headers, not anything private. **C10** also unblocks on C6.

**Branch/PR:** [SynthEdit#57](https://github.com/JeffMcClintock/SynthEdit/pull/57) + [SynthEditLib#25](https://github.com/JeffMcClintock/SynthEditLib/pull/25), which must merge together, plus the TideSynth PR carrying this entry.

---

## 2026-08-19 — linux — C12 is COMPLETE; C12d and the umbrella flipped to DONE, C6 unblocked

**Prompt:** 397330d · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Third and last item this session**, again at Jeff's explicit instruction ("merged. sync repos. any additional Linux tasks?") after he merged C12d's two PRs. Bookkeeping plus the verification that makes the bookkeeping mean something.

**Did:** Flipped **C12d** (IN-REVIEW → DONE) and the **C12 umbrella** (BLOCKED(C12d) → DONE) into [BACKLOG-DONE.md](BACKLOG-DONE.md), and unblocked **C6** (BLOCKED(C12d) → TODO). Precedent for flipping my own work in the same session is C12f on 2026-08-18, which the windows run flipped at Jeff's request the same way.

**Result — verified on the MERGED default branches, from scratch, not from the PRs.** This is the check that matters for a two-repo change, because merging one without the other was the stated breakage risk and nothing before this moment had built the combination:

| check | result |
|---|---|
| `${EDITOR_DIR}` entries in `EditorLib/CMakeLists.txt` | **0** |
| three files present in `SynthEditLib`, absent from `SE16/SynthEdit2/` | yes / yes |
| configure, fresh tree | RC=0 |
| full build | **935/935 RC=0**, zero `error:`, zero `undefined reference` |
| `ctest` | **67/67** |
| moved TUs' objects | both from `.../SynthEditLib/`, **zero** under any `SynthEdit2` path |

935 edges rather than the 549 measured pre-merge, because this tree was configured from scratch after the merges rather than reusing a warm one — worth knowing so nobody reads the two numbers as a regression.

**Learned — `check-backlog-diff.py` caught a real rewrite of mine, and it was right to.** My first C6 edit **replaced** the row's leading sentence instead of growing it. The script's rule is that an existing row's Item text must still be present *verbatim* somewhere inside the new text; a replacement is a rewrite, which needs a human rather than an auto-merge. Rebuilt the row as `<new note> + <original Item verbatim>` and it passed. **The general shape: prepend to an Item, never edit inside it.** The three earlier runs that hit this are why the check exists; add this one.

**State of the queue, which is the actual answer to "any additional Linux tasks?":**

- **The `linux` column is empty of takeable work.** C12d was the last one. `X1`, `X2` and `R4` are the only other `linux`-marked rows and all three are BLOCKED. That is the correct state, not a gap.
- **C6 is next and is `any`** — move `EditorLib/CMakeLists.txt` itself into `SynthEditLib`. Whichever box wakes first. It is not linux's in particular, and there is no reason to hold it for this one.
- **Two one-line jobs remain open and belong to no platform:** the `SynthEditJuce` entry in [#88](https://github.com/JeffMcClintock/TideSynth/issues/88) and the ctest path default in [#156](https://github.com/JeffMcClintock/TideSynth/issues/156). Both are in GATED-by-default paths (`SE16/SynthEditJuce/`, `SE16/tests/`) and **neither is a build break**, so STEP 5's A17 exception does not reach either. They need Jeff or a ruling — they are not blocked on capacity.

**Next:** **C6**, by any box, after re-reading its 2026-08-14 near-miss. Then C7 and C10, which C6 unblocks in turn.

**Branch/PR:** the TideSynth PR carrying this entry. All six repos on their default branches, clean; merged `main`/`master` re-verified building on this platform.

