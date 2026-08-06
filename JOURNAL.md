# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

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

## 2026-08-06 — windows — P2

**Did:** Loaded the P1 build of `TIDE_VST3.vst3` in REAPER 7.78 and watched it.
Wrote [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md) with two
screenshots under `docs/images/`. Observation only — nothing under `SE16`,
`SynthEditLib` or `C:\SE\build-tide-p1` was modified, and no bug was fixed.

Branched from `tide/win/P1-verify-prototype-build`, not `main`: that PR (#2) is
still open, P2 uses the build tree P1 produced, and both runs edit the same
BACKLOG rows. Before claiming P2 I checked `git ls-remote --heads origin` and
`gh pr list` — no branch or PR named it. (There *are* open PRs #1 and #3 from the
Linux and macOS boxes, both for S1; they collided. #4 is the macOS run's fix to
the run prompt, which is what told me to check remotes first. None are merged, so
`main` still shows P1 as TODO.)

**Result:** It loads and the editor opens — the prototype really is a working
plugin in a real DAW. Four findings, in descending order of how much they hurt:

1. **Resizing the editor window crashes the host.** 3/3 reproductions,
   `0xc0000005` inside `TIDE_VST3.vst3`, Release fault RVA `0x44d8c`, Debug
   `0x184ed9`, followed 3–5 s later by `0xc000041d` at the same offset (unhandled
   exception in a user callback — so it is dying in a window proc, not on the
   audio thread). Filed **P4**.
2. **The plugin is not called TIDE anywhere the user can see it** — REAPER shows
   `VST3i: SynthEdit (GMPI)`. Filed **P5**.
3. **Zero host-automatable parameters.** REAPER reports 3 params and all three
   are its own wrapper's (Bypass/Wet/Delta). That is the concrete state of V2.
4. **The module browser is populated from `C:\ProgramData\SynthEdit\Plugin-Cache-16.xml`**,
   written by the *installed SynthEdit app* at 11:30 that morning. TIDE works on
   this machine only because SynthEdit is installed on it. Evidence for S1/S2.
5. **No breadcrumb bar, no properties pane, canvas drawn ~440 px in from the
   top-left.** Filed **U1**.

**Learned:**

1. **A portable REAPER is the right harness for this, and it takes one copy
   command.** Copy `C:\Program Files\REAPER (x64)\*` and then `%APPDATA%\REAPER\*`
   into one scratch directory (152 MB, ~30 s). Because `reaper.ini` now sits next
   to `reaper.exe`, REAPER runs portable: its own config, its own plug-in scan
   cache, its own `Scripts` folder, and the developer's REAPER is untouched. Set
   `vstpath64` to just the build folder so the scan finds exactly one plug-in.
   `-splashlog <file>` gives a timestamped startup trace.

2. **Drive it from `Scripts/__startup.lua`, not from the mouse.** REAPER runs that
   file automatically at startup, so `InsertTrackAtIndex` + `TrackFX_AddByName` +
   `TrackFX_Show(tr, idx, 3)` instantiates the plugin and floats its editor with
   no UI automation at all, and `TrackFX_GetNumParams`/`GetParamName` dump the
   host-visible parameter list to a log file. This is how finding 3 was measured.
   Two traps: `TrackFX_AddByName` needs `"TIDE_VST3.vst3"` (the *filename*) —
   `"TIDE_VST3"` returns -1 because of finding 2. And the startup script does not
   re-run if REAPER restores project tabs from a previous session; strip
   `projecttab*` and `lastproject=` from the portable ini between runs or you will
   test an empty REAPER and think the plugin is stable. I lost one run to exactly
   that and briefly believed the crash was spontaneous.

3. **Screenshots and window control need no MCP.** `Graphics.CopyFromScreen` from
   PowerShell captures the virtual desktop; `EnumWindows` + `GetWindowRect`
   locates the plugin's floating window by title. One gotcha: declare
   `GetWindowTextW` with `CharSet=CharSet.Unicode`, otherwise StringBuilder
   marshals as ANSI and every window title comes back as its first character.

4. **How to prove a crash is caused by what you think it is.** Run 4 sat idle
   2.5 minutes with the editor open, polled every 5 s, `Responding=True`
   throughout, then died 1 second after the resize. An earlier
   `SetForegroundWindow` + screenshot on the same window did not kill it. Without
   that idle control I could not have ruled out a timer or idle callback.

5. **`MoveWindow` on the plugin window does not resize it** — `GetWindowRect`
   returns the same `1672x995` before and after — and it crashes anyway. So the
   fault is in *handling* the size-change message, before any new size is adopted.
   Useful narrowing for whoever takes P4.

6. **The Release configuration produces no PDB.** `build-tide-p1/SynthEditSem/Release`
   has the `.vst3` and nothing else, so the Release fault RVA cannot be
   symbolised. Debug does have `TIDE_VST3.pdb` (57 MB). I tried to symbolise the
   Debug RVA with `dbghelp.dll` from `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64`
   P/Invoked from PowerShell; `SymLoadModuleExW` succeeded but `SymFromAddrW`
   returned `<no symbol>` and I did not chase it further. **There is no `cdb.exe`
   on this machine** — that Debuggers folder holds only `dbghelp/dbgcore/srcsrv/symsrv`
   DLLs. Installing the Debugging Tools for Windows feature, or opening the dump
   in Visual Studio, is the shorter road.

7. **Windows kept full minidumps** (`%LOCALAPPDATA%\CrashDumps\reaper.exe.<pid>.dmp`,
   ~37 MB each) because LocalDumps is enabled on this box. The WER `ReportArchive`
   copies, by contrast, contain only `Report.wer` — the `.tmp.dmp` files it
   references are already deleted by the time you look. Go to `CrashDumps`, not
   `ReportArchive`.

8. **`GetHomeDir()` has no trailing separator.** It ends in
   `std::filesystem::path::parent_path()` (`SynthEdit2/Application.cpp:203-234`),
   so `TideApp.cpp:109`'s `GetHomeDir() + L"modules\\"` composes
   `...\Releasemodules\`, not `...\Release\modules\`. Harmless here because
   neither path exists, but `SynthEditAppBase.cpp:1108` concatenates the same way.
   Not filed separately — it belongs with S1, which rewrites that line anyway.

9. **The module cache filename does not distinguish TIDE from SynthEdit.**
   `SemCacheName()` only adds a per-folder hash when `BundleInfo::isSemFolderOverridden`
   is set (`ModuleFactory_Editor.cpp:175-191`), and `TideApp::InitInstance` assigns
   `semFolder` directly rather than through the setter — so TIDE reads, and on a
   cache miss would *rewrite*, `C:\ProgramData\SynthEdit\Plugin-Cache-16.xml`, the
   installed app's own file. Whoever takes S2 should force the cache-miss path,
   but do it on a machine without SynthEdit installed, or back that file up first.

10. **Reordering note:** P4 is now the topmost Ready-now item, ahead of S1/S2/S3.
    A crash that kills the host blocks V1 and makes every by-hand test impossible,
    so it seemed to belong there. P5 and U1 went to the bottom of the table.

**Next:** P4. It has a one-line repro, two minidumps and a narrowed message path,
and nothing else that touches the editor by hand is testable until it is fixed.
Add `/DEBUG` to the Release link while you are there, so the next crash report is
symbolisable. Whoever takes it: the fix may sit in `SE16/SynthEdit2` (GATED under
the run prompt's ALLOWED/GATED split) — do the TIDE-side part and file the rest.
The portable-REAPER harness in §"How it was observed" is worth rebuilding rather
than clicking; it took about 15 minutes.

**Branch/PR:** `tide/win/P2-daw-load-observation`

---

## 2026-08-06 — windows — P1

**Did:** Verified the prototype builds from a clean CMake configure, in a fresh
build tree at `C:\SE\build-tide-p1` (deliberately *not* the developer's existing
`C:\SE\SE16\build` — that tree is a decade of accumulated cache and would have
hidden the finding below). Wrote `docs/building.md`. Nothing under `C:\SE\SE16`
or `C:\SE\SynthEditLib` was modified; the only writes outside this repo were the
build tree.

**Result:** It builds. `cmake --build ... --target TIDE TIDE_VST3` exits 0 for
both configs, zero compiler warnings at default verbosity:

| Config | Artifacts in `<build>/SynthEditSem/<config>/` |
|---|---|
| Release | `TIDE.gmpi` 2,712,576 B · `TIDE_VST3.vst3` 2,969,600 B |
| Debug | `TIDE.gmpi` 10,235,904 B · `TIDE_VST3.vst3` 11,616,768 B |

Exact commands are in [docs/building.md](docs/building.md). Environment: CMake
4.2.0, VS 18 Community, MSVC 14.51.36231, toolset v145, Windows SDK
10.0.26100.0.

**Learned:**

1. **A clean configure does NOT work with default settings, and the error looks
   like something else entirely.** First attempt failed with exactly two errors:

   ```
   C:\SE\SE16\SynthEdit2\CContainer.cpp(8,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [EditorLib.vcxproj]
   C:\SE\SE16\SynthEdit2\MfcDocPresenter.cpp(4,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [EditorLib.vcxproj]
   ```

   Root cause: this machine has two VS 18 instances. `...\18\Community` has the
   MFC component (`VC\Tools\MSVC\14.51.36231\atlmfc\include\afxres.h` exists);
   `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` has MSVC
   14.51.36231 but **no `atlmfc` directory at all**. With no
   `CMAKE_GENERATOR_INSTANCE` given, CMake picks BuildTools. Fix: pass
   `-DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community"`.
   `CMAKE_LINKER` in `CMakeCache.txt` is the quickest way to see which instance a
   tree is actually using. The instance cannot be changed in place — delete the
   build tree and reconfigure.

   The developer's `C:\SE\SE16\build` was configured with
   `CMAKE_GENERATOR_INSTANCE:UNINITIALIZED=C:/Program Files/Microsoft Visual Studio/18/Community`,
   i.e. it was passed on the command line at some point. That is the whole reason
   it has always worked and a fresh tree does not.

2. **Do not debug this by building the failing `.vcxproj` directly — it lies.**
   `MSBuild.exe EditorLib.vcxproj /t:...` succeeds on the *same* build tree that
   `cmake --build` fails on, because MSBuild launched by hand resolves its own VS
   instance (Community, MSBuild 18.8.2) while `cmake --build` uses the cached one
   (BuildTools, MSBuild 18.7.8). I lost about half an hour to this: the two
   `.vcxproj` files are byte-identical apart from paths and GUIDs, the
   `IncludePath` property printed by `msbuild -getProperty:IncludePath` contains
   `atlmfc\include`, and `cl.exe` invoked by hand with those include dirs
   compiles `#include "afxres.h"` fine. All of that is true and all of it is
   irrelevant. To get the truth, run `cmake --build ... -- /v:diag` and grep the
   log for `EXTERNAL_INCLUDE=` — the VS install path in that string is the one
   actually in use. It is also not shell-related; it reproduces identically from
   Git Bash, PowerShell and a `.bat` wrapper.

3. **Two files in the carve-out set require MFC on Windows.**
   `SynthEdit2/CContainer.cpp:8` and `SynthEdit2/MfcDocPresenter.cpp:4` both do
   `#include "afxres.h"` inside `#ifdef _WIN32`. Nothing in any CMakeLists
   mentions MFC — it works only because `atlmfc\include` is on the default
   include path when the component happens to be installed. Both files are
   scheduled to move to public `SynthEditLib` (C3 and C4), which would make "you
   must have Visual Studio's MFC component" a build requirement of the open-source
   repo. Filed as **P3**. Not fixed here — out of scope for P1.

4. Building `--target TIDE TIDE_VST3` pulls in only SynthEditLib, EditorLib and
   HarfBuzz, not SynthEditCL or the test suite. Useful: it is a much shorter
   build than the default all-targets one, ~6 min cold.

5. Even with all four `*_FOLDER_OVERRIDE` variables pointed at local clones, a
   fresh configure still hits the network for the VST3 SDK and HarfBuzz (CPM,
   cached in `%USERPROFILE%\.cpm`) and for CLAP + clap-helpers (FetchContent,
   into the build tree, so re-downloaded per build tree).

**Next:** P2 — load TIDE in a DAW and record what happens, observing only. The
build tree at `C:\SE\build-tide-p1` is current and correct as of today, so P2 can
use `SynthEditSem/Debug/TIDE_VST3.vst3` from it without rebuilding. P3 is the
useful item to pair with the carve-out when C0/L1 clear; it is worth doing
*before* C3/C4 move those files, not after.

**Branch/PR:** `tide/win/P1-verify-prototype-build`

---

## 2026-08-06 — windows — project setup (manual session, not a scheduled run)

**Did:** Created this repo as the coordination point for TIDE Synth. Wrote
PLAN.md, BACKLOG.md, docs/carve-out.md, docs/design-notes.md,
docs/agent-setup.md, and a CI skeleton. No code was written and nothing in
`C:\SE\SE16` or `C:\SE\SynthEditLib` was modified.

**Learned:**

- TIDE is not a greenfield project. A working prototype exists at
  `C:\SE\SE16\SynthEditSem` — `TideApp` implements `ISeApp`, opens a
  `ContainerViewStruct` in `CF_STRUCTURE_VIEW` mode, and builds as VST3 + GMPI.
  There is also an existing iOS target at `SE16/SE_IOS_APP/TIDE/` and demo
  patches at `SE16/TideModules/`.
- The blocker for open-sourcing is `EditorLib`, which lives in the private
  `SynthEdit` repo. It has only 2 files of its own; the other ~120 come from
  `SE16/SynthEdit2/` via `EditorLib/CMakeLists.txt`. That file is the
  authoritative scope of the carve-out.
- `SynthEditLib` is already a **public** repo — but it has **no LICENSE file**,
  so it is not open source yet. This surprised the setup session and is now
  BACKLOG L1.
- The commercial boundary is cleaner than expected. `ExportAsPlugin` is one
  free function in one 2,470-line file. Its only callers are the private WinUI3
  IDE and `SynthEditCL`. `CContainer.h` references it solely through a `friend`
  declaration, which is legal C++ even when the function is never defined — so
  that header can go public unchanged. No shimming required.
- Moonbase licensing is already outside `EditorLib` by deliberate design
  (see the comment at `EditorLib/CMakeLists.txt:179`).
- "RNBW" in the original spec was a misreading; the reference is **RNBO** by
  Cycling '74. Note that a `getUniqueId() == id_to_long("RNBW")` special case
  does exist in `SE14/SynthEdit/VST_Wrapper.cpp` for a plugin called "rainbow" —
  unrelated, and a trap for a future agent grepping for it.

**Next:** L1, C0 and G1 need Jeff. P1 (verify the prototype builds) is the
first thing an agent can do unaided, and everything else depends on knowing
that baseline.

**Branch/PR:** none — scaffolding committed directly.
