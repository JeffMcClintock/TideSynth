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
| P4 | TODO | any | **Resizing the plugin editor window crashes the host.** Reproducible 3/3 in REAPER 7.78: access violation inside `TIDE_VST3.vst3` while handling the size change, which kills the whole DAW process. Release fault RVA `0x44d8c`, Debug `0x184ed9`; 37 MB minidumps in `%LOCALAPPDATA%\CrashDumps\reaper.exe.{42964,44464}.dmp` while they last. Repro recipe and controls in [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md) §2. **Release builds emit no PDB** — symbolise from the Debug dump, or add `/DEBUG` to the Release link first. Blocks V1: nothing can be tested by hand until the window survives being resized. The fix may land in `SE16/SynthEdit2` (GATED) — if so, do the TIDE-side part and file the rest. |
| S1 | TODO | any | **Design module enumeration without filesystem scanning.** `TideApp::InitInstance` calls `LoadOrScanModuleData()` and points `BundleInfo::semFolder` at `GetHomeDir() + "modules\\"` (`SE16/SynthEditSem/TideApp.cpp:109`). Neither works under an iOS AUv3 sandbox. Produce a design note first — do not implement yet. Options to weigh: compile-in a static module registry; enumerate from inside the plugin bundle; a hybrid. |
| S2 | TODO | any | **Audit every filesystem and cache write** reachable from a TIDE build. Grep for `GetHomeDir`, `AppData`, `%TEMP%`, `~/Library`, `CreateFile`, `fopen`, registry access. Produce `docs/sandbox-audit.md` listing each hit as keep / stub / remove. Constraint 4 in [PLAN.md](PLAN.md) cannot be verified without this. |
| S3 | TODO | any | **Make the removed dialogs genuinely absent.** `TideApp.cpp` currently stubs `doDialogConnectUg`, `doDialogPatchManager` and `doDialogBuildCodeSkeleton` with `assert(false)`. In a release build these silently fall through. Make the code paths that reach them unreachable, or fail loudly in release too. |
| B1 | TODO | any | **CI that builds nothing yet but is correct.** `.github/workflows/build.yml` exists as a skeleton and is expected to fail until C7. Get it to the point where it fails for exactly one honest reason (missing private dependency) rather than for syntax or toolchain errors. |
| W1 | TODO | any | **tidesynth.com holding page.** Static, self-contained, no trackers. Say what TIDE is and link the repo. Do not deploy — build it under `website/` and leave deployment to Jeff. |
| P3 | TODO | win | **Remove the MFC dependency from the two shared-core files that have one.** `SynthEdit2/CContainer.cpp:8` and `SynthEdit2/MfcDocPresenter.cpp:4` include `afxres.h` under `#ifdef _WIN32`. That header ships only with Visual Studio's MFC component, so a contributor with VS Build Tools (no MFC) cannot build TIDE at all — see the "MFC trap" section of [docs/building.md](docs/building.md). Both files are on the carve-out list (C3 and C4), so this lands in the public repo as a hard MFC requirement unless it is removed first. Find out what they actually need from `afxres.h` — it is likely only `ID_*`/`IDR_*` resource constants, which could move to the local `resource.h`. Verify SynthEdit and SynthEditCL still build. |
| P5 | TODO | any | **The plugin does not call itself TIDE.** REAPER lists it as `VST3i: SynthEdit (GMPI)`; "TIDE" appears only in the filename, so `TrackFX_AddByName(tr, "TIDE_VST3", ...)` returns -1 and a user searching their FX browser for "TIDE" finds nothing. `TideApp::getVendor4charCode()` already returns `"TIDE"` — it is the plug-in name and vendor string in the VST3 wrapper that still say SynthEdit. See [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md) §3. |
| U1 | TODO | any | **Close the gap to the one-view UX in [docs/design-notes.md](docs/design-notes.md).** As of P2 the editor has no breadcrumb bar, no properties pane on screen (`TideApp::OpenPropertiesBrowser` exists but nothing corresponds to it), the module browser is SynthEdit's two-column browser with no collapse control, and the document canvas is drawn ~440 px in from the top-left of the content area with a dead strip down the right. Measurements and screenshots in [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md) §6. Probably wants splitting once someone costs it. |

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

| ID | Done | Plat | Item |
|---|---|---|---|
| P2 | 2026-08-06 | win | **Load TIDE in a DAW and record what actually happens.** Loaded the P1 Release and Debug builds in a portable REAPER 7.78; it instantiates and opens its editor. Findings in [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md). Spun off P4 (host-killing crash on editor resize), P5 (plugin is not named TIDE) and U1 (one-view UX gaps); added runtime evidence for S1/S2 and V2. See the JOURNAL entry for 2026-08-06. |
| P1 | 2026-08-06 | win | **Verify the prototype still builds.** Clean CMake configure + Release and Debug builds of `TIDE` (GMPI) and `TIDE_VST3`, both exit 0 with zero warnings. Commands and the MFC/`afxres.h` trap recorded in [docs/building.md](docs/building.md). Spun off P3. See the JOURNAL entry for 2026-08-06. |
