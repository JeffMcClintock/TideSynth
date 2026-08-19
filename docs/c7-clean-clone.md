# C7 — point TIDE at the public repo, and prove it with a clean clone

Carve-out stage 7, from [carve-out.md](carve-out.md):

> **Point TIDE at it.** TIDE's CMake consumes public `SynthEditLib` only, with
> no reference to `C:\SE\SE16`. At this point a stranger can build TIDE.
> […] Stage 7 additionally requires a clean-clone build in CI with no access to
> the private repo — that is the real proof.

Scoped 2026-08-19 on the linux box by measuring, the way C12 was scoped in
[c12-remaining-editor-files.md](c12-remaining-editor-files.md). **C7 is not one
session and it is not one PR.** This document is the inventory and the split.

Everything below was measured on the merged default branches — `SE16`
`5ae7fbc67`, `SynthEditLib` `0aa207b`, `TideSynth` `041b920` — with a
from-scratch Ninja/GCC/Release tree, not read off a status column.

---

## What is actually in the way

TIDE's dependency on the private repo is **four separate things**, and they were
being discussed as one. Three of them are not about `SynthEditLib` at all.

| # | The dependency | Size | Stage |
|---|---|---|---|
| 1 | TIDE's CMake listed four private include paths | **4 of the 5 were dead** | **C7a — done** |
| 2 | TIDE's own source lives in the private repo (`SE16/SynthEditSem/`, `SE16/TideModules/`) | 16 + 10 files, **3 referring sites** | C7b |
| 3 | TIDE links `EditorScreenshot`, which is `SE16/EditorScreenshot/` | 13 files, needs a ruling | C7c |
| 4 | `SynthEditLib` cannot configure standalone — no root superproject exists | new file in this repo | C7d |

Plus the public-file→private-header includes, already owned elsewhere: **C13**
and **C14**. Those are the only part of C7 that the `SynthEditLib` repo itself
has to change, and they are not this stage's to do.

**C13 merged mid-run** (SynthEdit#58 + SynthEditLib#26, 2026-08-19). Re-measured
afterwards with `scripts/dangling_private_includes.py` on the merged defaults:
**7 dangling includes across 4 headers → 1 across 1**. The survivor is
`ApplySynthEditConfig.cpp:2` → `SynthEditApp.h`, which is C14 — and it is the
same header TIDE's own `TideAppStubs.cpp:31` includes. **One header is now the
whole of the carve-out's remaining private-include debt, and it has two
consumers.**

## 1. TIDE's include paths — measured, and mostly dead (C7a, done)

`SE16/SynthEditSem/CMakeLists.txt` pointed at four places in the private repo.
Measured one at a time:

| Entry | Verdict | Evidence |
|---|---|---|
| `EDITOR_DIR` (`../SynthEdit`) | **dead** | `set()` once, referenced nowhere in the file |
| `EDITOR2_DIR` (`../SynthEdit2`) | **dead** | same — the identical dead pair C6 deleted from `EditorLib/CMakeLists.txt` |
| `../Shared` | **dead** | the directory **does not exist** anywhere in the tree |
| `../SynthEdit` | **dead** | contains `sem.ico`, `SynthEdit.ico`, `synthedit.chm`, `skins/` — no headers. `SynthEdit.rc` includes `resource.h` and `windows.h` only, so the Windows resource compiler does not need it either |
| `../SynthEdit2` | **REAL — keep** | exactly one consumer, below |

Also removed: a stray `PRIVATE` keyword. `include_directories()` is the
*directory*-scoped command and takes paths only, so CMake was adding a relative
directory literally named `PRIVATE`. Harmless, but it made the block read as if
it were `target_include_directories()`.

**Verification:** fresh tree, Linux, GCC, Ninja, Release — configure RC=0,
**928/928 RC=0**, zero `error:`, zero `undefined reference`, **ctest 67/67**.
`TIDE.gmpi`, `TIDE_VST3.vst3`, `SynthEditCL` and `SynthEditWayland` all built.
Identical object count to the baseline built immediately before the change, in
the same tree, from the same sources. `SynthEdit2` (WinUI3) is Windows-only and
was **not** built.

## 2. TIDE's own private includes — there are exactly two

Every `#include "..."` in `SE16/SynthEditSem/*.{cpp,h,mm}` resolved against the
public repo first, then against `SE16`:

```
SynthEditGui.cpp   #include "ContainerThumbnail.h"  ->  SE16/EditorScreenshot/
TideAppStubs.cpp   #include "SynthEditApp.h"        ->  SE16/SynthEdit2/
```

**Two.** Everything else TIDE includes is already public. That is a much smaller
surface than the stage's own row implies, and it maps exactly onto C7c and C14.

### A correction for whoever takes C14

`SynthEditApp.h`'s `moonbasepp_Licensing.h` include is **inside
`#ifdef SE_MOONBASE_SUPPORT`** (`SE16/SynthEdit2/SynthEditApp.h:6-11`), and that
header **is not tracked by git at all** — its own comment says to copy
`moonbase_lib/` in from `SynthEdit_Azure`. So "this header drags the licensing
surface into the public repo" holds only for a moonbase build. That narrows C14
rather than blocking it. The header is still a private one declaring a private
app class; *that*, not the licensing include, is why it cannot simply move.

## 3. What is NOT in the way

Worth stating, because each of these was a plausible fear:

- **Everything except `SynthEdit` is already public.** Checked with
  `gh repo view --json isPrivate`: `SynthEditLib`, `TideSynth`, `gmpi_ui`,
  `GMPI_Wrappers` and `GMPI` are all public. Only `SynthEdit` (`SE16`) is not.
- **Moving TIDE's source out of `SE16` has a blast radius of three files.**
  `grep -rl 'SynthEditSem\|TideModules'` over `SE16`'s build files returns
  `CMakeLists.txt` (one `add_subdirectory` at `:409`),
  `SynthEditSem/CMakeLists.txt` itself, and `se_gmpi/vst3/CMakeLists.txt` — where
  both hits are *comments*. No `.vcxproj`, no `.pbxproj`, no `.sln`.
- **`EditorLib/CMakeLists.txt` is already clean.** After C6 it carries no
  `${EDITOR_DIR}` entries and no private paths; `SE16` re-adds the one include
  directory from its own root (`SE16/CMakeLists.txt:386`).
- **`SynthEditLib/modules/SoundPipe` is not a TIDE blocker.** Three public files
  there include `soundpipe.h`, which resolves only in `SE16/SDKs/Soundpipe/`.
  But `modules/` is added by `SE16`'s root (`:416`), never by `SynthEditLib`'s
  own, and TIDE links none of it. It is a genuine defect in the *public repo* —
  source that cannot compile without a private SDK — and is filed separately as
  **S18**, not as part of C7. Note `scripts/dangling_private_includes.py` skips
  `SDKs` by design (`SKIP_DIRS`, `:57-63`), which is why no earlier stage saw
  it; the script and a hand scan were cross-checked this run and agree exactly
  on the seven carve-out edges, differing only here.

## 4. The real structural gap: there is no superproject

`SynthEditLib/CMakeLists.txt` consumes `${GMPI_SDK}`, `${GMPI_UI_SDK}` and
`${VST3_SDK}` and **never sets them**. `SE16/CMakeLists.txt:78-161` does, via the
override-or-fetch pattern (`*_FOLDER_OVERRIDE` blank ⇒ `CPMAddPackage` /
`FetchContent`). It also sets `syntheditlib_folder`, `DSP_CORE` and
`external_sdk_folder`, and it is what calls `add_subdirectory` on `SynthEditLib`,
`EditorLib`, `EditorScreenshot` and `SynthEditSem`.

**So "a stranger can build TIDE" needs a root `CMakeLists.txt` in this repo that
plays SE16's role for TIDE's subset.** That is C7d, and it is the piece nobody
had named. It is also what switches TideSynth's CI on: `build.yml`'s guard job
(BACKLOG B1) gates the three-platform matrix on a root `CMakeLists.txt` existing,
so C7d turns the matrix from *skipped* to *running* **with no workflow edit** —
which matters, because the fleet's token deliberately has no `workflow` scope.

---

## The stages

Sequential. Each must leave SynthEdit, SynthEditCL and TIDE building.

| ID | Plat | Scope | Accept | Size |
|---|---|---|---|---|
| **C7a** | any | Delete TIDE's four dead private include paths; document the one real one in place. `SE16/SynthEditSem/CMakeLists.txt` | fresh-tree build RC=0 with an unchanged object count, ctest green, `TIDE.gmpi` + `TIDE_VST3.vst3` produced | **done 2026-08-19 (linux)** |
| **C7b** | any | Move `SE16/SynthEditSem/` and `SE16/TideModules/` into this repo. `SE16` consumes them through a `TIDESYNTH_FOLDER_OVERRIDE` + `FetchContent` pair, mirroring `SYNTHEDITLIB_FOLDER_OVERRIDE` exactly. Fix the three `../` paths in `SynthEditSem/CMakeLists.txt` that stop resolving after the move (`../SynthEdit2`, and `../TideModules/prefabs` twice) | `git ls-files` in `SE16` shows **zero** `SynthEditSem/` or `TideModules/` entries; `SE16`'s own build still produces `TIDE.gmpi` and `TIDE_VST3.vst3` at the same object count, ctest green | one session — the move is mechanical and the blast radius is 3 files |
| **C7c** | any | `EditorScreenshot`. TIDE links the target and `SynthEditGui.cpp` includes `ContainerThumbnail.h`. Either it becomes public or TIDE gets a thumbnail-free path | TIDE builds with no `EditorScreenshot` reference, **or** `EditorScreenshot` lives in a public repo and TIDE links it from there | **needs a ruling first** — `SE16/EditorScreenshot/` is on neither the ALLOWED nor the GATED list, so it is GATED by default. Do not start it on a guess |
| **C7d** | any | Root `CMakeLists.txt` in this repo: the override-or-fetch block for `GMPI`, `gmpi_ui`, `GMPI_Wrappers`, the VST3 SDK and `SynthEditLib`, then `add_subdirectory` for `SynthEditLib`, `EditorLib` and TIDE. Model it on `SE16/CMakeLists.txt:1-230` and take only TIDE's subset | `cmake` + `ninja` in a **fresh clone of this repo alone**, with `SE16` absent from the disk, produces `TIDE.gmpi` | one session, after C7b |
| **C7e** | any | The proof. CI green on the public matrix from a checkout with no access to `SynthEdit` | `build.yml`'s three platforms run (not skip) and pass on a PR | needs C7b, C7c, C7d, **C13** and **C14** |

**C7d cannot pass before C14 lands** — C13 merged mid-run, so `SynthEditApp.h`
is the only header left that a clean clone fails. C7b does not depend on it, so
it is the next thing to take.

## What this stage did not verify

Windows and macOS were not built — this box cannot compile them. The C7a change
is a CMake include-path deletion in a directory-scoped block, so nothing in it is
platform-conditional, but that is reasoning, not measurement. The claim made is
the Linux one.
