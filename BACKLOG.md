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
| G2 | RESOLVED | **The SE16 write-permission conflict in the run prompt** — STEP 5's blanket ban meant no agent could ever write TIDE code, only design notes. **Fixed 2026-08-06** by Jeff: STEP 5 now has an ALLOWED/GATED split — `SE16/SynthEditSem/`, `SE16/TideModules/` and `SE16/SE_IOS_APP/TIDE/` are TIDE's to change; `EditorLib`, `SynthEdit2` and `SynthEditLib` stay behind the C0 gate. See PR #4. **Each machine's installed task must be reinstalled from [docs/weekly-run-prompt.md](docs/weekly-run-prompt.md) — only the macOS box has been updated; Windows and Linux still carry the old STEP 5.** |

---

## Ready now — no dependency on the above

| ID | Status | Plat | Item |
|---|---|---|---|
| P4 | TODO | any | **Resizing the plugin editor window crashes the host.** Reproducible 3/3 in REAPER 7.78: access violation inside `TIDE_VST3.vst3` while handling the size change, which kills the whole DAW process. Release fault RVA `0x44d8c`, Debug `0x184ed9`; 37 MB minidumps in `%LOCALAPPDATA%\CrashDumps\reaper.exe.{42964,44464}.dmp` while they last. Repro recipe and controls in [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md) §2. **Release builds emit no PDB** — symbolise from the Debug dump, or add `/DEBUG` to the Release link first. Blocks V1: nothing can be tested by hand until the window survives being resized. The fix may land in `SE16/SynthEdit2` (GATED) — if so, do the TIDE-side part and file the rest. |
| S1a | TODO | win | **Stop scanning: stage 1 of [docs/module-enumeration.md](docs/module-enumeration.md).** Delete the `semFolder` assignment and the `LoadOrScanModuleData()` call from `TideApp::InitInstance` (`SE16/SynthEditSem/TideApp.cpp:109,114`). Touches TIDE's own file only — no EditorLib/SynthEditLib change, so it does not wait on the carve-out. Run the §9 verification in that note **first**: if the module browser is not fully populated with the cache deleted and `ModulePath` empty, stop and revise the note instead. Also fixes S4 as a side effect. |
| S1b | TODO | any | **Compile the scan out: stage 3 of [docs/module-enumeration.md](docs/module-enumeration.md).** Now required rather than optional — [PLAN.md](PLAN.md) constraint 7 makes the fixed module set a non-negotiable, so the scan and cache code must be *absent*, not merely un-called, or S2's sandbox audit can never come back clean. Introduce `TIDE_NO_EXTERNAL_MODULES`, or decouple `SE_EXTERNAL_SEM_SUPPORT` from `GMPI_IS_PLATFORM_JUCE` (`SynthEditLib/modules/shared/xplatform.h:34`) so it is settable on its own. **Read the trap in §6 of the note first:** the two arms of `initialise_synthedit_modules` register different module sets, and neither is the set TIDE wants — the non-JUCE arm includes `ug_soundcard_in`/`ug_soundcard_out`/`ug_midi_out`, which constraint 2 forbids. Blocked behind S1a, and touches shared code, so check carve-out ordering. |
| S4 | TODO | any | **TIDE can clobber the installed SynthEdit's module cache.** `TideApp` sets `BundleInfo::semFolder` but not `isSemFolderOverridden` (`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.h:63`), so `SemCacheName()` (`SE16/SynthEdit2/ModuleFactory_Editor.cpp:174`) omits its `-override-<hash>` suffix and TIDE reads **and writes** the same `Plugin-Cache-16.xml` as the desktop SynthEdit app. A TIDE instance in a DAW can overwrite the desktop app's cache with TIDE's smaller module set. Live Windows/macOS hazard today, independent of iOS. Subsumed by S1a if S1a lands first — close it then rather than fixing twice. |
| S5 | TODO | any | **`getFolderInfo` is undefined behaviour in TIDE.** `TideApp::InitInstance` never calls `CSynthEditAppBase::InitInstance`, so `refreshFolderLocations()` never runs and `m_folder_settings` stays empty; `getFolderInfo` (`SE16/SynthEdit2/Application.cpp:167`) then evaluates `m_folder_settings[0]->current_folder` on an empty vector. Reachable via `CSynthEditAppBase::ShortenFilename` (`SE16/SynthEdit2/SynthEditAppBase.cpp:238`). `ResolveFilename` is unaffected. Fix in `Application.cpp` (guard the empty case) — that is EditorLib, so check carve-out ordering before moving it. |
| S6 | TODO | mac | **Delete the dead iOS module artifact.** `SE16/SE_IOS_APP/TIDE/Plugins/` holds six checked-in `.sem` bundles that are all `Mach-O 64-bit bundle x86_64` in macOS bundle layout, installed by a Run Script (`SE_IOS_APP.xcodeproj/project.pbxproj:2064`) that copies them to `.../Contents/` — a macOS-only destination. Nothing there can load on arm64 iOS. It reads as a working iOS module story and is not one. Remove it, or add a README saying what it actually is. See addendum A5 of [docs/module-enumeration.md](docs/module-enumeration.md). **Unblocked** — G2 is resolved and `SE16/SE_IOS_APP/TIDE/` is now ALLOWED. But the Run Script phase that installs it lives in the *shared* `SE_IOS_APP.xcodeproj/project.pbxproj`, which is still GATED — delete the folder, then file the build-phase removal separately rather than editing the project file. |
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
| S1 | 2026-08-06 | linux | **Design module enumeration without filesystem scanning.** Design note delivered as [docs/module-enumeration.md](docs/module-enumeration.md). Recommends the hybrid: static registry for modules (already the mechanism — the factory constructor registers all ~157 built-ins before any scan runs), bundle-local resources for prefabs. Nothing implemented, per the item. Spun off S1a, S4, S5. |
| S1 | 2026-08-06 | mac | *(duplicate run — macOS took S1 the same day, before the cron stagger took effect.)* Independent analysis reached the same recommendation; the additive findings are folded into the same note as **Addendum A1–A6** rather than landing a competing document. Spun off G2 and S6. See the JOURNAL entry for 2026-08-06. |
