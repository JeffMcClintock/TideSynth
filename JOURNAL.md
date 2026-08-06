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

## 2026-08-07 — windows — P4

**Did:** Diagnosed the host-killing resize crash down to file and line, from the
minidumps P2 left behind. Wrote [docs/p4-resize-crash.md](docs/p4-resize-crash.md).
**Did not fix it** — the entire fix is in `gmpi_ui` and `GMPI_Wrappers`, which
are outside my write scope; see "The scope problem" below. The one thing I did
change is `SE16/SynthEditSem/CMakeLists.txt` (ALLOWED), which now emits a PDB
for Release, as P4 itself asked.

STEP 1 was clear: `gh issue list` returns nothing, no issues of any label. STEP 2
— all seven PRs are merged and `main` is the only remote branch, so nothing had
claimed P4. Claimed it, pushed the DOING mark, then started.

**Result — the crash, in one sentence:** `DrawingFrame::reSize` checks that its
Direct2D device context is alive, calls `SetWindowPos`, and then uses the device
context — but `SetWindowPos` dispatches `WM_SIZE` *synchronously*, and that
handler releases the device. Time-of-check/time-of-use across a re-entrant Win32
call.

```
TIDE_VST3!gmpi::hosting::DrawingFrame::reSize+0x139
00007fff`24304ed9  mov  rax,qword ptr [rax]  ds:00000000`00000000   <- rax = 0
[C:\SE\gmpi_ui\backends\DrawingFrameWin.cpp @ 1418]
01  TIDE_VST3!wrapper::SEVSTGUIEditorWin::onSize+0x4c
[C:\SE\GMPI_Wrappers\wrapper\VST3\SEVSTGUIEditorWin.cpp @ 88]
02  reaper+0x405ae
```

Base `0x7fff24180000` → RVA `0x184ED9`, which is exactly the Debug RVA P2
recorded, so this is provably the same crash. The chain, step by step, is in the
note; the short version is `reSize:1406` checks → `SetWindowPos:1408` →
`WindowProc:592` `WM_SIZE` → `OnSize:1371` → `ResizeBuffers` fails →
`ReleaseDevice()` nulls both pointers (`DrawingFrameWin.h:376-377`) →
`reSize:1418` dereferences the null. Two defects, both needed:
**P4a** the stale check, **P4b** nothing clamps the size.

**Learned:**

1. **This machine *does* have `cdb.exe`, and P2's journal is wrong about that.**
   It is in the Store WinDbg package:
   `C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2603.20001.0_x64__8wekyb3d8bbwe\amd64\cdb.exe`.
   P2 concluded there was none after searching only `Windows Kits\10\Debuggers`
   (which holds just `dbghelp`/`dbgcore`/`srcsrv`/`symsrv` DLLs) and then lost
   time P/Invoking `dbghelp` from PowerShell. Search `WindowsApps` too. Nothing
   needed installing and the whole symbolisation took about five minutes.

2. **Two cdb flags are the difference between an answer and a wall of noise.**
   `.symopt-0x100` (clear `SYMOPT_NO_UNQUALIFIED_LOADS`) — without it `!analyze`
   prints the "you specified an unqualified symbol" boilerplate three times and
   resolves nothing. And `.reload /f TIDE_VST3.vst3` — the module loads
   *deferred*, so `lm` shows it with no symbols until you force it. My first run
   produced 340 lines of nothing because of these two.

3. **The Debug artifacts from P1 are still on disk and still match.**
   `C:\SE\build-tide-p1\SynthEditSem\Debug\TIDE_VST3.{vst3,pdb}`, timestamped
   14:07:29 on 2026-08-06 — before the 16:52 crash. So does
   `%LOCALAPPDATA%\CrashDumps\reaper.exe.44464.dmp`. Nothing had been cleaned up
   in the day between runs, but do not count on that indefinitely.

4. **`dv` in the Debug dump gives the host's actual arguments, and they are
   absurd.** `right = 2178`, `bottom = 32672`. 32672 is past the D3D11 maximum
   texture dimension of 16384, so `ResizeBuffers` *cannot* succeed — which is
   why this reproduced 3/3 rather than intermittently. Where REAPER got that
   number I could not establish; the note records the bit patterns as a loose
   end rather than guessing.

5. **The bug is visible in the source once you know where to look, and the
   codebase already knows about it.** `OnSize`, twenty lines above `reSize`,
   checks `!swapChain || !d2dDeviceContext` and carries a comment explaining
   that the device can legitimately be gone. `reSize` checks one of the two,
   before the re-entrant call instead of after, and never checks `swapChain` at
   all — line 1419 is a second latent null deref on the same path.

6. **A minidump gives you locals but not the whole object.** `dt -r1 this` died
   with "Memory read error" partway through — a minidump keeps stack and
   registers, not the full heap. I could not read `swapChain`/`d2dDeviceContext`
   out of the object directly and had to pin the null pointer from the
   disassembly instead: the fault at `+0x139` falls between the
   `ComPtr<ID2D1DeviceContext>::operator->` call at `+0x12a` and the indirect
   call at `+0x14f`, while the `swapChain` accessor is not reached until
   `+0x174`. `uf /c` on the function is what makes that legible.

**The scope problem — this one is Jeff's, and it is the reason P4 is not
fixed.** The run prompt's ALLOWED list is `SE16/SynthEditSem/`,
`SE16/TideModules/`, `SE16/SE_IOS_APP/TIDE/`; the GATED list is `EditorLib`,
`SynthEdit2`, `SynthEditLib`. **`gmpi_ui` and `GMPI_Wrappers` are in neither.**
They are separate public repos that the build consumes via
`GMPI_UI_FOLDER_OVERRIDE` / `GMPI_WRAPPER_FOLDER_OVERRIDE`, and they are where
TIDE's rendering and windowing bugs actually live. The prompt says "do the
TIDE-side part and file the rest" — but I grepped `SE16/SynthEditSem/` for
`DrawingFrame`, `reSize` and `SEVSTGUIEditorWin` and there are **no hits**.
There was no TIDE-side part. Filed as **G3**.

I did not reach across, for three reasons: the prompt explicitly warns that "the
fix looks small" is exactly when not to; `gmpi_ui` is the render backend for
every GMPI plugin *and* SynthEdit, so it is shared code in the sense the GATED
rule means; and **both working copies were dirty** with in-progress Wayland work
(`gmpi_ui` at `11051f1`, `backends/DrawingFrameWayland.h` modified;
`GMPI_Wrappers` at `4a6a733`, `tests/wayland_editor_host.cpp` modified).
Committing into someone's uncommitted branch is how you lose both.

**What I did change, and it builds.** `SE16/SynthEditSem/CMakeLists.txt` now
adds `/Zi` + `/DEBUG` for Release on the TIDE targets, with `/OPT:REF` and
`/OPT:ICF` restored explicitly because `/DEBUG` silently turns both off. Scoped
to `${SUB_PROJECT_NAME}` inside the existing `FORMATS_LIST` loop, so it reaches
`TIDE` and `TIDE_VST3` and nothing else.

| Target | Exit | Result |
|---|---|---|
| `TIDE`, `TIDE_VST3` Release | 0 | `TIDE_VST3.pdb` 10,031,104 B and `TIDE.pdb` 8,998,912 B now exist |
| `SynthEditCL` Release | 0 | unaffected, built to confirm rather than assumed |

Binary cost is 11,776 bytes each (`TIDE_VST3.vst3` 2,969,600 → 2,981,376), which
is the debug directory entry — if it had grown by megabytes, `/OPT:REF` would
have been lost. The next Release crash report symbolises without needing a Debug
repro, which is what P4 asked for.

**Next:** **G3 first** — it is one ruling and it unblocks a crash that kills the
host. P4a is a few lines (re-check both pointers after `SetWindowPos`, mirroring
`OnSize:1376`) and P4b is a clamp in `checkSizeConstraint`; both are written up
with exact line numbers in the note, so whoever is allowed to touch those repos
can land them quickly — including Jeff directly, which may be the fastest route.
Until then **V1 stays untestable by hand**, since the editor still cannot be
resized without killing the DAW.

After that the queue is S1a (win, and the §9 check still wants doing first),
then S1b/S4/S5. Note S4 is still open and still worth closing as a side effect
of S1a rather than fixing twice.

**Branch/PR:** `tide/win/P4-editor-resize-crash`

---

## 2026-08-06 — jeff — decision: free, donation-supported (manual, not a scheduled run)

**Did:** Recorded a product decision that had not been written down anywhere:

> **TIDE is free. No paid tier, no trial, no licence key. Funding is by
> donation, and both the plugin and tidesynth.com should support it.**

Added as a "Price and funding" section in [PLAN.md](PLAN.md), next to the
open-source section rather than as a numbered design constraint — it is a
commercial fact like the plugin-export boundary, not something every backlog item
gets checked against. Amended **W1** to carry a donation link on the website, and
filed **D1** for the plugin side.

**Learned — why the plugin side is a design note and not a task:**

1. **Two existing constraints delete most of the obvious answers.** Constraints 1
   and 5 (one view, minimal dialogs) rule out a splash, a nag dialog or a second
   window, which is how almost every donation-funded plugin does this. What is
   left is the breadcrumb bar or an about pane.

2. **The remaining answer may not work on the platform that matters.** A "Donate"
   button that opens a browser is the natural fallback, and
   [docs/design-notes.md](docs/design-notes.md) already lists
   `browseto.mm`/`openurl.mm` as removed-or-restricted under AUv3. So the one
   implementation that survives the UX constraints may not survive constraint 3.
   Whether an AUv3 can open a URL at all is a **factual question only the macOS
   box can answer** — hence D1 says to establish that first and to say so plainly
   if you are running on Windows or Linux instead of guessing.

3. **Free is not open source, and this decision does not touch L1.** Price and
   licence are separate. A free binary with no LICENSE file is exactly what
   `SynthEditLib` is today. L1 stays NEEDS-JEFF.

4. **The website side is the easy half and should not wait.** A static page with
   an `<a href>` has none of the sandbox or one-view problems. The one trap is
   that W1 already says "no trackers", and hosted donate *widgets* ship
   third-party script and cookies — so W1 now says plain link, not embed, and
   leaves the destination as a placeholder because choosing the platform is a
   Jeff decision like L1 and G1.

**Next:** W1 can absorb the website half whenever it is taken. D1 is `any` but is
really a macOS question; if the Mac takes it, it can answer the AUv3 URL question
properly instead of deferring it.

**Branch/PR:** `plan/free-donation-supported`

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

## 2026-08-06 — jeff — decision: fixed module set (manual, not a scheduled run)

**Did:** Answered the open question raised by the same day's linux run (S1,
§7.1 of [docs/module-enumeration.md](docs/module-enumeration.md)):

> **TIDE ships a fixed module set, compiled in. No third-party module loading on
> any platform — not just iOS.**

Recorded as [PLAN.md](PLAN.md) **constraint 7**, so it is checked against every
future backlog item like the other six. Marked the question answered in the design
note, made stage 3 of that note a requirement rather than an option, and filed
stage 3 as BACKLOG **S1b**.

**Why it went in PLAN.md rather than a PR comment:** the weekly prompt has each run
read PLAN, BACKLOG, JOURNAL, carve-out, and open issues labelled for its own
platform. **PR comments, PR descriptions and review threads are read by nobody.**
An answer left on [PR #1](https://github.com/JeffMcClintock/TideSynth/pull/1) would
have been invisible to every future run. A GitHub issue would have been read, but
the prompt frames issues as broken builds, so a product decision filed as one gets
picked up as if it were a compile failure. The same trap applies to any new doc:
it only gets read if PLAN or BACKLOG links it, which is why
`docs/module-enumeration.md` is now in PLAN's companion-documents list.

**Learned:** the durable channels into a memoryless run are PLAN.md (rulings),
BACKLOG.md (queue) and JOURNAL.md (reasoning). Everything else on GitHub is
human-to-human only.

**Next:** unchanged — S1a still wants the §9 verification on a machine that can run
TIDE, and realistically P1/P2 first. S1b is queued behind it.

**Branch/PR:** committed to the S1 branch so the ruling lands with the note that
prompted it — `tide/linux/s1-module-enumeration-design`, PR #1.

---

## 2026-08-06 — macos — S1 (duplicate run — see "The collision" below)

**Did:** Took S1, independently analysed it, then discovered the Linux run had
taken the same item hours earlier and already had a PR open. Folded the macOS
findings into the Linux note as **Addendum A1–A6** rather than landing a
competing document at the same path. Filed G2 and S6. No code changed, no build
run, nothing in `SynthEdit` or `SynthEditLib` modified.

STEP 1 was clear — `gh issue list` on `JeffMcClintock/TideSynth` returns nothing
at all, there are no issues yet of any label. P1 and P2 are `win`, so S1 was the
topmost eligible item, exactly as it was for Linux.

**Result:** Addendum delivered. Independent analysis reached the same
recommendation as the Linux note (static registry, not scanning), by a different
route, which is worth something as corroboration. Four findings are additive and
one materially changes what the note's §9 verification will show.

**The collision — read this before assuming the stagger works.** Both boxes ran
on 2026-08-06. [agent-setup.md](docs/agent-setup.md) staggers Windows Fri /
macOS Sat / Linux Sun precisely so two machines cannot take the same backlog
item — but both were *set up* on the 6th, so both fired immediately and the
stagger had not taken effect yet. The mechanism is fine; the first week is the
hole. Nothing in the process caught it: I only noticed at `gh pr create` time,
when the push succeeded and the PR failed. Two cheap fixes, neither of which is
mine to make:

1. Have STEP 2 check open PRs and remote branches for `<backlog-id>` before
   marking an item DOING. `git ls-remote --heads origin` would have caught this
   in one call, before any work.
2. Mark DOING and **push that commit** before starting, not just commit it
   locally. The DOING mark is only useful as a claim if other machines can see
   it.

Also note the remote default branch is **`main`**, but a fresh clone here left
me on `master` — `gh pr create --base master` fails with a confusing "No commits
between…" rather than "no such branch". Use `main`.

**Learned — additive to the Linux entry, not repeating it:**

- **The `INIT_STATIC_FILE` list is three regions, not two, and this changes the
  §9 prediction.** The Linux note's §6 trap correctly spots that the JUCE arm
  (`UgDatabase.cpp:1063`–1139, ~70 entries) and the `#else` arm (`:1140`–1155,
  14) differ. But ~66 more entries sit *after* the `#endif` at `:1156` and are
  **unconditional**. Verified placements: `ug_adsr` `:1168`, `ug_oscillator2`
  `:1201`, `ug_vca` `:1215` are unconditional; `ug_filter_sv` `:1145` and
  `ug_filter_biquad` `:1144` are `#else`-only; `ADSR` `:1064`, `Converters`
  `:1070`, `OscillatorNaive` `:1082`, `Slider` `:1089` are JUCE-only.
  **Consequence:** after stage 1 the module browser will be *populated but
  wrong* — legacy `ug_*` modules present (enough for v0.1), modern SEM modules
  absent, and `ug_soundcard_in/out` + `ug_midi_out` present in violation of
  constraint 2. Whoever runs §9 must record *which* modules appear, not just
  whether the list is non-empty, or they will misread the result in either
  direction.

- **The "third, explicit list" the Linux note asks for already has a hook.**
  `SE_EXTRA_STATIC_FILE_CPP` at `UgDatabase.cpp:1239`, plus
  `initialise_synthedit_extra_modules()` at `:1243` (editor implementation at
  `ModuleFactory_Editor.cpp:170`). A TIDE module list can live in the TIDE
  target with **no edit to the shared function** — which makes stage 3 smaller
  than the note assumes. Four lines at the bottom of a 190-line function; easy
  to miss, and I nearly did.

- **The metadata half has a shipping mechanism too.**
  `RegisterExternalPluginsXmlOnce` (`UgDatabase.cpp:526`) reads
  `database.se.xml` from bundle resources (`:543`); the `imbeddedFilename`
  attribute (`:587`) is the switch between compiled-in and `dlopen`. And every
  module already ships its own descriptor beside its source (e.g.
  `SynthEditLib/modules/OscillatorNaive/OscillatorNaive.xml`), so the database
  can be **generated at build time by concatenation** with the attribute
  omitted. That is how the modern SEM modules get their pin metadata in without
  a scan. Blocker: `plugin_helper.cmake` emits `add_library(… MODULE)` at both
  `:70` and `:186` — there is no static-library variant of either macro today.

- **`-DSE_EXTERNAL_SEM_SUPPORT=0` will not work.** Both the Linux note's stage 3
  and its §5 want that macro settable independently. `xplatform.h:34` defines it
  unconditionally, so a CMake `-D` collides. `GMPI_IS_PLATFORM_JUCE` at `:25` is
  wrapped in `#if !defined(…)` for exactly this reason — giving
  `SE_EXTERNAL_SEM_SUPPORT` the same guard is a one-line change that alters no
  existing target's value.

- **`SE16/SE_IOS_APP/TIDE/Plugins/` is a decoy — do not try to make it load.**
  Six checked-in `.sem` bundles that look like an iOS module story. `file` says
  every binary is `Mach-O 64-bit bundle x86_64`; they are macOS bundle layout;
  and the Run Script that installs them
  (`SE_IOS_APP.xcodeproj/project.pbxproj:2064`) copies to
  `${BUILT_PRODUCTS_DIR}/${FULL_PRODUCT_NAME}/Contents/`, a macOS-only path.
  Nothing there can load on arm64. Filed as **S6**. This is the one finding only
  the Mac could have made, and it is a partial answer to the question the Linux
  note explicitly addressed to the Mac in its §4 — the full answer still needs
  M2 and a real device.

- **One scan root that stage 1 will not remove.** `TideApp.cpp:109` overrides a
  *good* default: `BundleInfo.cpp:699` already points `semFolder` inside the
  bundle. But `BundleInfo.cpp:712` adds a dev-tree fallback that walks **parent
  directories** hunting for a sibling `SynthEdit2/PlugIns`. It is not in
  `TideApp`, so deleting line 109 leaves it. One for S2.

- **Where the trees are on the Mac:** `~/Documents/GitHub/SynthEdit` (= `SE16`)
  and `~/Documents/GitHub/SynthEditLib`, **siblings, not nested** — same shape
  as the Linux box, different paths from PLAN.md's `C:\SE\…` table. `SynthEdit`
  was at `e6b50de2b`. `build/modules/Debug/` has 54 built `.sem`/`.gmpi`
  bundles, so the module set does build here.

**The process problem — Jeff should read this one.** STEP 5 of the run prompt
says agents must not modify `SE16` or `SynthEditLib` unless the item is an
approved carve-out stage. But S1a, S3, S4 and S5 all edit
`SE16/SynthEditSem/TideApp.cpp` or `SE16/SynthEdit2/`. As written, **no agent
can ever write TIDE code — only design notes.** Both today's runs happened to
draw design items so neither was blocked, but the next machine to pick up S1a
will either stop or quietly break the rule. Filed as **G2** (NEEDS-JEFF). I did
not downgrade S1a to BLOCKED: that is someone else's item and the reading is
ambiguous enough that the call should be Jeff's, not mine.

**Next:** G2 first — it gates everything with code in it, including S1a which is
otherwise the obvious next step. Then P1/P2 on Windows, since §9's check and
S1a both need a machine that can build and run TIDE. S6 is small and mac-owned
but also gated by G2.

**Branch/PR:** `tide/mac/s1-module-enumeration`, branched from
`tide/linux/s1-module-enumeration-design` rather than `main` so it is a clean
delta on PR #1 with no conflict. **If PR #1 is closed rather than merged, this
work goes with it** — the addendum lives in the Linux note's file.

---

## 2026-08-06 — linux — S1

**Did:** Wrote [docs/module-enumeration.md](docs/module-enumeration.md) — the design
note S1 asked for. No code changed; S1 explicitly said design only. Split three
follow-ups out of it into BACKLOG (S1a, S4, S5).

No open `platform:linux` issues, so STEP 1 was clear. S1 was the topmost TODO
matching `linux`/`any` (P1 and P2 are `win`).

**Result:** Note delivered. Nothing built — S1 required no build. The
recommendation has an explicit cheap empirical check (§9 of the note) that a
machine which can *run* TIDE must do before S1a is implemented; the Linux box
cannot do that yet (X1 is BLOCKED).

**Learned — things the next run should not have to rediscover:**

- **The Linux box has a full copy of the source tree**, at `~/SE/SE16` and
  `~/SE/SynthEditLib` — not just `C:\SE` as PLAN.md's table implies. SE16 was at
  `8a7b1ef7b`, SynthEditLib at `53f0979`. So `any` items that only need to *read*
  the source can be done on Linux, not just Windows. `~/SE` also has `build`,
  `build-vst3sdk`, `GMPI`, `GMPI-plugins`, `synthedit-website` and a
  `wayland-spike`. Whether SynthEditSem *builds* here is untested (that is X1).

- **The static-registration mechanism S1 was asked to design already exists.**
  `CModuleFactory`'s constructor (`SynthEditLib/UgDatabase.cpp:86`) calls
  `initialise_synthedit_modules()` (`:1054`), which force-links ~157
  self-registering modules; each registers with its full XML description in
  memory via `internalSdk::RegisterPlugin` (`UgDatabase.cpp:236`) or
  `RegisterPluginWithXml` (`:266`). No filesystem involved.

- **The module browser never touches the filesystem.**
  `ModuleBrowser::Init()` (`SE16/SynthEdit2/ModuleBrowser.cpp:99`) →
  `CSynthEditAppBase::ExportModules` (`SynthEditAppBase.cpp:1329`) →
  `ExportModuleNames()` (`ModuleFactory_Editor.cpp:2193`) → reads
  `CModuleFactory::Instance()->module_list`. That list is already populated by the
  constructor *before* `LoadOrScanModuleData()` runs, and the menu map is built
  lazily (`SynthEditAppBase.cpp:1331`), so it does not need `ReloadMenu()` either.
  **Therefore the scan contributes nothing for built-in modules** — stage 1 of the
  recommendation is a deletion, not a rewrite. This is the single most useful fact
  in the note.

- **Separate the two iOS prohibitions or you will over-scope.** Writing outside the
  container is banned for everything; loading code not signed into the bundle is
  banned for `dlopen`. But *reading inside the plugin's own bundle is allowed*. So
  modules (code) must be fixed at link time, while prefabs (XML data) can legally
  be enumerated from `Contents/Resources/`. Hence the hybrid recommendation rather
  than "compile everything in".

- **Trap for S1a/stage 3:** the two arms of `initialise_synthedit_modules` register
  *different* module sets. The `GMPI_IS_PLATFORM_JUCE==1` arm
  (`UgDatabase.cpp:1063`–`1138`) vs the `#else` arm (`:1140`–`1155`): the non-JUCE
  arm registers `ug_soundcard_in`, `ug_soundcard_out`, `ug_midi_out` — which TIDE
  must **not** have (constraint 2, the DAW owns I/O) — while the JUCE arm omits
  e.g. `ug_filter_sv`. Flipping `SE_EXTERNAL_SEM_SUPPORT`
  (`SynthEditLib/modules/shared/xplatform.h:34`, currently derived from
  `GMPI_IS_PLATFORM_JUCE` and not independently settable) silently changes which
  modules exist. TIDE probably needs a third, explicit list.

- **Two real bugs found in passing, filed not fixed** (S4, S5):
  - S4: `TideApp` sets `BundleInfo::semFolder` without `isSemFolderOverridden`
    (`BundleInfo.h:63`), so `SemCacheName()` (`ModuleFactory_Editor.cpp:174`) drops
    its `-override-<hash>` suffix and TIDE **writes** the desktop SynthEdit's
    `Plugin-Cache-16.xml`. A TIDE instance in a DAW can clobber the desktop app's
    module cache. Not an iOS issue — happens on Windows/macOS today.
  - S5: `TideApp::InitInstance` never calls `CSynthEditAppBase::InitInstance`, so
    `refreshFolderLocations()` never runs, `m_folder_settings` is empty, and
    `getFolderInfo` (`Application.cpp:167`) indexes `[0]` on an empty vector.
    Reachable from `ShortenFilename` (`SynthEditAppBase.cpp:238`).
    `ResolveFilename` is *not* affected — it uses `getDefaultPath`, which has a
    safe fallback (`Application.cpp:200`).
  - A third, cosmetic: `TideApp.cpp:109` hard-codes `L"modules\\"`, which on
    macOS/Linux names a directory ending in a literal backslash. Silent because
    `ScanFolder` swallows the error via `std::error_code`
    (`ModuleFactory_Editor.cpp:1009`). Not filed separately — stage 1 deletes the
    line.

- **Process note:** the run prompt says to commit the `DOING` mark before starting,
  but also never to work on `main`. I branched first, then committed the `DOING`
  mark on the branch (`4187556`). A crash after that point is still diagnosable,
  just from the branch rather than `main`. Suggest the prompt say so explicitly.

**Next:** S1a — stage 1 of the note — is `win` because it needs a machine that can
build and run TIDE. **Do the §9 check before touching code:** delete
`<settings>/SynthEdit/Plugin-Cache-16.xml`, point `ModulePath` at an empty folder,
launch TIDE, open the module browser. Full browser ⇒ the deletion is safe. Empty or
short browser ⇒ something outside `module_list` feeds it, and the note is wrong —
say so in the journal rather than pressing on. Realistically P1 and P2 should land
first anyway, since both S1a and that check need a working build.

Open question that is Jeff's, not an agent's: does TIDE ever want third-party
modules on desktop, or is a fixed module set the product? The note works either
way; only stage 3's shape depends on it.

**Branch/PR:** `tide/linux/s1-module-enumeration-design`

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
