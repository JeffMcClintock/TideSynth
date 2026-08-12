# S1b — compiling the module scan out

Status **TODO**, platform **any**. Lifted verbatim out of the
[BACKLOG.md](../BACKLOG.md) row by **A8**, 2026-08-12, when that file had
reached 76 KB and every run on three machines was reading all of it. The row
now carries the decision-shaped summary and points here; this file is the
detail. Wording below is unchanged from the row — only the line breaks are
new.

---

**Compile the scan out — stages (b) and (c) only; (a) is done.** ~~Blocked on
**C0**~~ — **unblocked 2026-08-08, C0 approved.** Its own closing
recommendation still stands, though: (b) and (c) touch the same files C3 and C4
move, so doing them standalone means doing them twice — **ride along with C4.**
The macOS run 2026-08-08 did the whole of the ALLOWED part and measured the
rest: see addendum **B1–B5** of
[docs/module-enumeration.md](module-enumeration.md). **(a) DONE** — TIDE
no longer compiles `SynthEdit2/SynthEditApp.cpp`, which had been putting
`SynthEditApp::InitInstance()` (a second `LoadOrScanModuleData()` call site, a
`MonitorFileSystem()` watcher thread, the settings-file writes and the whole
licensing/activation surface) into the shipping plugin; `SE16` `40b6008ee`.
**(b) and (c) are GATED and cannot start until C0:** `ScanFolder`,
`SemCacheName`, `LoadModuleData`, `ClearModuleDataCache`,
`ApplicationBase::LoadOrScanModuleData`, `Module_Info3` and an imported
`dlopen`/`dlsym`/`dlclose` are all still in the Release binary, and all live in
`SynthEdit2/ModuleFactory_Editor.cpp`, `SynthEdit2/Application.cpp` and
`SynthEditLib/Module_Info3.cpp`. **Do not attempt this with an existing flag.**
`SE_EXTERNAL_SEM_SUPPORT` is the wrong lever — it guards exactly two lines in
the codebase and removes none of the above (B2, measured) — and
`GMPI_IS_PLATFORM_JUCE` is the §6 trap. It needs a new
`TIDE_NO_EXTERNAL_MODULES` through `EditorLib/CMakeLists.txt` and a real split
of `ModuleFactory_Editor.cpp`. These are the same files C3 and C4 move, so
doing them standalone means doing them twice — **recommend riding along with
C4.**
