# TIDE Synth — Backlog

Ordered. A weekly agent takes the **topmost item that is not blocked and matches
its machine's platform role**, does that one item, and stops.

Status: `TODO` · `DOING` · `DONE` · `BLOCKED` · `NEEDS-JEFF`
Platform: `any` · `win` · `mac` · `linux`

Update this file at the end of every run. An item left in `DOING` with no
matching JOURNAL entry means a previous run died — reset it to `TODO` and note
that in the journal.

---

## Blocked on Jeff — agents must not start these

| ID | Status | Item |
|---|---|---|
| L1 | NEEDS-JEFF | **Choose a licence** for `SynthEditLib` and TIDE. `SynthEditLib` is currently public with no LICENSE file, which means all rights reserved — it is not open source today. Blocks C1–C7 and any "open source" claim. See [docs/carve-out.md](docs/carve-out.md). |
| C0 | NEEDS-JEFF | **Approve the carve-out plan** in [docs/carve-out.md](docs/carve-out.md), including the staging order and the decision on `SynthEditCL`. Blocks C1–C7. |
| G1 | NEEDS-JEFF | **Create the GitHub repo** and decide public vs private at creation. Agents must not create or push a public repo. |

---

## Ready now — no dependency on the above

| ID | Status | Plat | Item |
|---|---|---|---|
| P1 | TODO | win | **Verify the prototype still builds.** Build `SE16/SynthEditSem` (TIDE, VST3 + GMPI) from a clean CMake configure. Record the exact commands that work in `docs/building.md`. Everything downstream assumes this baseline; nobody has confirmed it recently. |
| P2 | TODO | win | **Load TIDE in a DAW and record what actually happens.** Screenshot the structure view, note what is broken or missing versus [docs/design-notes.md](docs/design-notes.md). Write findings to `docs/state-of-the-prototype.md`. Do not fix anything in this item — observe only, then file follow-up items. |
| S1 | TODO | any | **Design module enumeration without filesystem scanning.** `TideApp::InitInstance` calls `LoadOrScanModuleData()` and points `BundleInfo::semFolder` at `GetHomeDir() + "modules\\"` (`SE16/SynthEditSem/TideApp.cpp:109`). Neither works under an iOS AUv3 sandbox. Produce a design note first — do not implement yet. Options to weigh: compile-in a static module registry; enumerate from inside the plugin bundle; a hybrid. |
| S2 | TODO | any | **Audit every filesystem and cache write** reachable from a TIDE build. Grep for `GetHomeDir`, `AppData`, `%TEMP%`, `~/Library`, `CreateFile`, `fopen`, registry access. Produce `docs/sandbox-audit.md` listing each hit as keep / stub / remove. Constraint 4 in [PLAN.md](PLAN.md) cannot be verified without this. |
| S3 | TODO | any | **Make the removed dialogs genuinely absent.** `TideApp.cpp` currently stubs `doDialogConnectUg`, `doDialogPatchManager` and `doDialogBuildCodeSkeleton` with `assert(false)`. In a release build these silently fall through. Make the code paths that reach them unreachable, or fail loudly in release too. |
| B1 | TODO | any | **CI that builds nothing yet but is correct.** `.github/workflows/build.yml` exists as a skeleton and is expected to fail until C7. Get it to the point where it fails for exactly one honest reason (missing private dependency) rather than for syntax or toolchain errors. |
| W1 | TODO | any | **tidesynth.com holding page.** Static, self-contained, no trackers. Say what TIDE is and link the repo. Do not deploy — build it under `website/` and leave deployment to Jeff. |

---

## Carve-out — blocked on L1 + C0

Stages are sequential. Each must leave SynthEdit, SynthEditCL and TIDE all
building before the next starts. See [docs/carve-out.md](docs/carve-out.md).

| ID | Status | Plat | Item |
|---|---|---|---|
| C1 | BLOCKED | any | Add LICENSE to `SynthEditLib` per L1. |
| C2 | BLOCKED | win | Move leaf files (`FuzzyMatch.h`, `checkpoint`, `cpu_accumulator`, `FrameRateLogger`, `imbedded_file`, `it_*`) to `SynthEditLib`; repoint `EditorLib/CMakeLists.txt`. |
| C3 | BLOCKED | win | Move the document model (`DocOb`, `CContainer`, `CUG`, `Plug*`, `SynthEditDocBase`, `SynthEditDoc2`). Largest and riskiest stage — split it if it resists. `CContainer.h`'s `friend ExportAsPlugin` declaration moves unchanged. |
| C4 | BLOCKED | win | Move views and browsers (`ModuleBrowser`, `PropertiesBrowser`, `MfcDocPresenter`, `ModuleFactory_Editor`, `SkinMgr`, `ThemeManager`). |
| C5 | BLOCKED | win | Move the app base (`SynthEditAppBase`, `ApplySynthEditConfig`, `SynthRuntime_editor`, `UIoManager`, `IO_base`, `IO_None`). |
| C6 | BLOCKED | any | Move `EditorLib/CMakeLists.txt` itself into `SynthEditLib`. |
| C7 | BLOCKED | any | Repoint TIDE at public `SynthEditLib` only. Prove it with a clean-clone CI build that has no access to the private repo. |

---

## After the carve-out

| ID | Status | Plat | Item |
|---|---|---|---|
| M1 | BLOCKED | mac | AU + AUv3 targets building on macOS. |
| M2 | BLOCKED | mac | iOS AUv3 build; validate under real sandbox rules. This is the constraint that validates the whole design. |
| M3 | BLOCKED | mac | `auval` clean. |
| X1 | BLOCKED | linux | VST3 + CLAP on Linux, GCC 13+. See the Linux toolchain memory for WSL specifics. |
| X2 | BLOCKED | linux | Zero-warning build at `-Wall -Wextra`. |
| V1 | BLOCKED | any | Patch state survives host save/reload — the v0.1 acceptance test in [PLAN.md](PLAN.md). |
| V2 | BLOCKED | any | Host-automatable parameters without a panel view (design note 4 in [docs/design-notes.md](docs/design-notes.md)). |

---

## Done

*(nothing yet — append here with the date and the JOURNAL entry it refers to)*
