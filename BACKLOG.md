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
| G2 | NEEDS-JEFF | **Resolve the SE16 write-permission conflict in the run prompt.** STEP 5 says agents must not modify anything in `SE16` or `SynthEditLib` unless the item is an approved carve-out stage (C1–C7). But S1a, S3, S4 and S5 all edit `SE16/SynthEditSem/TideApp.cpp` or `SE16/SynthEdit2/`. As written, **no agent can ever write TIDE code — only design notes.** Either widen STEP 5 (e.g. "TIDE-owned files under `SE16/SynthEditSem/` are fair game; the carve-out gate covers `SynthEdit2/` and `SynthEditLib/`") or accept design-only output. Blocks S1a in practice. Raised by the 2026-08-06 macOS run. |

---

## Ready now — no dependency on the above

| ID | Status | Plat | Item |
|---|---|---|---|
| P1 | TODO | win | **Verify the prototype still builds.** Build `SE16/SynthEditSem` (TIDE, VST3 + GMPI) from a clean CMake configure. Record the exact commands that work in `docs/building.md`. Everything downstream assumes this baseline; nobody has confirmed it recently. |
| P2 | TODO | win | **Load TIDE in a DAW and record what actually happens.** Screenshot the structure view, note what is broken or missing versus [docs/design-notes.md](docs/design-notes.md). Write findings to `docs/state-of-the-prototype.md`. Do not fix anything in this item — observe only, then file follow-up items. |
| S1a | TODO | win | **Stop scanning: stage 1 of [docs/module-enumeration.md](docs/module-enumeration.md).** Delete the `semFolder` assignment and the `LoadOrScanModuleData()` call from `TideApp::InitInstance` (`SE16/SynthEditSem/TideApp.cpp:109,114`). Touches TIDE's own file only — no EditorLib/SynthEditLib change, so it does not wait on the carve-out. Run the §9 verification in that note **first**: if the module browser is not fully populated with the cache deleted and `ModulePath` empty, stop and revise the note instead. Also fixes S4 as a side effect. |
| S4 | TODO | any | **TIDE can clobber the installed SynthEdit's module cache.** `TideApp` sets `BundleInfo::semFolder` but not `isSemFolderOverridden` (`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.h:63`), so `SemCacheName()` (`SE16/SynthEdit2/ModuleFactory_Editor.cpp:174`) omits its `-override-<hash>` suffix and TIDE reads **and writes** the same `Plugin-Cache-16.xml` as the desktop SynthEdit app. A TIDE instance in a DAW can overwrite the desktop app's cache with TIDE's smaller module set. Live Windows/macOS hazard today, independent of iOS. Subsumed by S1a if S1a lands first — close it then rather than fixing twice. |
| S5 | TODO | any | **`getFolderInfo` is undefined behaviour in TIDE.** `TideApp::InitInstance` never calls `CSynthEditAppBase::InitInstance`, so `refreshFolderLocations()` never runs and `m_folder_settings` stays empty; `getFolderInfo` (`SE16/SynthEdit2/Application.cpp:167`) then evaluates `m_folder_settings[0]->current_folder` on an empty vector. Reachable via `CSynthEditAppBase::ShortenFilename` (`SE16/SynthEdit2/SynthEditAppBase.cpp:238`). `ResolveFilename` is unaffected. Fix in `Application.cpp` (guard the empty case) — that is EditorLib, so check carve-out ordering before moving it. |
| S6 | TODO | mac | **Delete the dead iOS module artifact.** `SE16/SE_IOS_APP/TIDE/Plugins/` holds six checked-in `.sem` bundles that are all `Mach-O 64-bit bundle x86_64` in macOS bundle layout, installed by a Run Script (`SE_IOS_APP.xcodeproj/project.pbxproj:2064`) that copies them to `.../Contents/` — a macOS-only destination. Nothing there can load on arm64 iOS. It reads as a working iOS module story and is not one. Remove it, or add a README saying what it actually is. See addendum A5 of [docs/module-enumeration.md](docs/module-enumeration.md). Gated by **G2**. |
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

| ID | Date | Item | Journal |
|---|---|---|---|
| S1 | 2026-08-06 | **Design module enumeration without filesystem scanning.** Design note delivered as [docs/module-enumeration.md](docs/module-enumeration.md). Recommends the hybrid: static registry for modules (already the mechanism — the factory constructor registers all ~157 built-ins before any scan runs), bundle-local resources for prefabs. Nothing implemented, per the item. Spun off S1a, S4, S5. | 2026-08-06 — linux — S1 |
| S1 | 2026-08-06 | *(duplicate run — macOS took S1 the same day, before the cron stagger took effect.)* Independent analysis reached the same recommendation; the additive findings are folded into the same note as **Addendum A1–A6** rather than landing a competing document. Spun off G2 and S6. | 2026-08-06 — macos — S1 |
