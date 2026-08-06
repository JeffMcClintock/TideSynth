# P4 — the editor-resize crash, diagnosed

BACKLOG **P4**. Root cause found, from the minidumps P2 left behind. **No fix
was applied**, because the whole fix lives outside this agent's write scope —
see [Where the fix has to go](#where-the-fix-has-to-go).

Everything in "The chain" was read out of the crash dump or the source. The one
inference is marked so.

## Summary

`gmpi::hosting::DrawingFrame::reSize` checks that its Direct2D device context is
alive, then calls `SetWindowPos`, then uses the device context. `SetWindowPos`
dispatches `WM_SIZE` **synchronously**, and the `WM_SIZE` handler can release
the device. When it does, `reSize` resumes and dereferences a null pointer,
which takes the host process down with it.

It is a time-of-check/time-of-use bug across a re-entrant Win32 call. The
oversized rect REAPER passed is what makes it fire every time, but the
re-entrancy hazard is there regardless of the size.

## How it was symbolised

`C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2603.20001.0_x64__8wekyb3d8bbwe\amd64\cdb.exe`

**This machine does have `cdb.exe`** — the JOURNAL entry for P2 says it does not.
That conclusion came from searching only `Windows Kits\10\Debuggers`, which
holds just `dbghelp`/`dbgcore`/`srcsrv`/`symsrv` DLLs. The Store WinDbg package
under `WindowsApps` ships the full command-line debugger. No install needed.

```bash
cdb -z "%LOCALAPPDATA%\CrashDumps\reaper.exe.44464.dmp" -y "C:\SE\build-tide-p1\SynthEditSem\Debug" -c ".symopt-0x100; .reload /f TIDE_VST3.vst3; .lines -e; .ecxr; kb; q"
```

Two traps, both of which cost time here:

- **`.symopt-0x100`** (clear `SYMOPT_NO_UNQUALIFIED_LOADS`) is needed or
  `!analyze` refuses to resolve anything and prints a wall of "unqualified
  symbol" boilerplate instead of an answer.
- **`.reload /f TIDE_VST3.vst3`** — the module loads *deferred* and `lm` will
  happily show it with no symbols until you force it.

The Debug dump is the one to use: `reaper.exe.44464.dmp` (16:52:25). Its faulting
image is `C:\SE\build-tide-p1\SynthEditSem\Debug\TIDE_VST3.vst3`, timestamped
14:07:29 the same day — i.e. the P1 build, still on disk and still matching its
PDB.

## The chain

```
TIDE_VST3!gmpi::hosting::DrawingFrame::reSize+0x139
00007fff`24304ed9  mov  rax, qword ptr [rax]   ds:00000000`00000000=????????????????
                                                    rax = 0
[C:\SE\gmpi_ui\backends\DrawingFrameWin.cpp @ 1418]

01  TIDE_VST3!wrapper::SEVSTGUIEditorWin::onSize+0x4c
[C:\SE\GMPI_Wrappers\wrapper\VST3\SEVSTGUIEditorWin.cpp @ 88]
02  reaper+0x405ae
```

Module base `0x7fff24180000`, so the fault RVA is `0x184ED9` — exactly the Debug
RVA P2 recorded, confirming this dump is the crash P2 saw.

Locals at frame 0, straight out of the dump:

```
this   = 0x0063cf40
left   = 0        top    = 0
right  = 2178     bottom = 32672
width  = 2178     height = 32672
```

and the `ViewRect` REAPER actually passed, at frame 1:

```
0:000> dt Steinberg::ViewRect 0x001596d0
   +0x000 left   : 0n0
   +0x004 top    : 0n0
   +0x008 right  : 0n2178
   +0x00c bottom : 0n32672
```

Step by step:

1. REAPER calls `IPlugView::onSize({0, 0, 2178, 32672})`.
   `SEVSTGUIEditorWin::onSize` forwards it verbatim —
   [`SEVSTGUIEditorWin.cpp:87`](#) does no validation of any kind.

2. `DrawingFrame::reSize(0, 0, 2178, 32672)` → `width = 2178`, `height = 32672`.

3. `DrawingFrameWin.cpp:1406` — `d2dDeviceContext` is non-null and the size
   differs, so it enters the block. **This is the check.**

4. `DrawingFrameWin.cpp:1408` — `SetWindowPos(windowHandle, …, 2178, 32672, SWP_NOZORDER)`.
   `SetWindowPos` does not post; it **sends** `WM_WINDOWPOSCHANGING`,
   `WM_WINDOWPOSCHANGED` and `WM_SIZE` to the window procedure and only returns
   once they have been handled.

5. `DrawingFrameWin.cpp:592-597` — the frame's own `WindowProc` handles
   `WM_SIZE` by calling `OnSize(width, height)`.

6. `tempSharedD2DBase::OnSize` (`DrawingFrameWin.cpp:1371`) calls
   `swapChain->ResizeBuffers(0, 2178, 32672, …)` at line 1381. **32672 exceeds
   the Direct3D 11 maximum texture dimension of 16384** (8192 at feature level
   10_x; the device is created with `D3D_FEATURE_LEVEL_11_1` down to `10_0` at
   `DrawingFrameWin.cpp:972-977`). The call cannot succeed.

7. `DrawingFrameWin.cpp:1396` — the failure branch calls `ReleaseDevice()`,
   which at [`DrawingFrameWin.h:376-377`](#) does:

   ```cpp
   d2dDeviceContext = nullptr;
   swapChain        = nullptr;
   ```

8. `SetWindowPos` returns. `reSize` resumes at `DrawingFrameWin.cpp:1418` and
   dereferences `d2dDeviceContext`, which step 7 just nulled. **`0xc0000005`.**
   **This is the use.**

The disassembly pins the null pointer to `d2dDeviceContext` rather than
`swapChain`. The fault at `+0x139` sits between the call to
`ComPtr<ID2D1DeviceContext>::operator->` at `+0x12a` (line 1418) and the
indirect call at `+0x14f` (still line 1418); the `swapChain` accessor is not
reached until `+0x174` (line 1419). `mov rax,[rax]` with `rax = 0` is the vtable
load on a null COM pointer.

**Inferred, not observed:** that step 6 is the specific re-entrant path which
nulls the pointer. `WM_PAINT` can also be dispatched during `SetWindowPos`, and
`PaintFrame` calls `ReleaseDevice()` on a failed present
(`DrawingFrameWin.cpp:923, 941`). Either route produces the same crash and the
same defect — a liveness check that does not survive the call it is separated
from — so the fix does not depend on which one ran. The oversized height makes
step 6 by far the likeliest, and explains why P2 saw this 3 times out of 3
rather than intermittently.

## The two defects

**D1 — `reSize` caches a liveness check across a re-entrant call.**
`DrawingFrameWin.cpp:1401-1430`. Its sibling `OnSize` twenty lines above gets
this right and says why:

```cpp
// The device can legitimately be gone here: it is released on a failed present,
// and a size event can arrive before the pending rebuild has run. Nothing to do --
// CreateSwapPanel sizes the new swap chain from the window when the rebuild runs.
if (!swapChain || !d2dDeviceContext)
    return;
```

`reSize` tests `d2dDeviceContext` only, tests it *before* `SetWindowPos`, and
never tests `swapChain` at all — so line 1419 is a second latent null
dereference on the same path, reachable whenever the device is released but
`ResizeBuffers` is somehow reached.

**D2 — nothing clamps the requested size.** `SEVSTGUIEditorWin::onSize`
forwards the host's rect unvalidated, and `checkSizeConstraint`
(`SEVSTGUIEditorWin.cpp:119-133`) never adjusts the rect it is given — it
measures, returns `kResultTrue` on an exact match and `kResultFalse` otherwise.
The VST3 contract is that `checkSizeConstraint` *modifies* the rect to the
nearest size the view will accept, so a host that asks politely still gets no
usable answer back.

Fixing D1 alone stops the crash but leaves the editor unable to resize, because
a 2178×32672 swap chain will fail forever. Fixing D2 alone leaves the
re-entrancy hazard for every other device-loss cause (monitor change, driver
reset, GPU preemption). **Both are needed.**

Where the `{0, 0, 2178, 32672}` rect comes from is **not established**. 32672 is
`0x7FA0` and 2178 is `0x882`; the eight bytes following the rect on REAPER's
stack are `00000882 00000000`, and `0x7fa0` also appears in the caller's `r15`
(`0x7fc70882`), which has the shape of a 64-bit value being read as two 32-bit
fields somewhere. That is a loose end, not a conclusion. It does not change
either fix — a plugin must not crash on a rect it dislikes.

## Where the fix has to go

Both files are outside the ALLOWED list in the weekly run prompt, and outside
the GATED list too — the run prompt does not mention these repos at all:

| File | Repo | In run prompt? |
|---|---|---|
| `backends/DrawingFrameWin.cpp`, `.h` | `JeffMcClintock/gmpi_ui` | not listed |
| `wrapper/VST3/SEVSTGUIEditorWin.cpp` | `JeffMcClintock/GMPI_Wrappers` | not listed |

Nothing under `SE16/SynthEditSem/` references `DrawingFrame`, `reSize` or
`SEVSTGUIEditorWin`, so **there is no TIDE-side half of this fix to do**. The
build consumes those two repos directly, via `GMPI_UI_FOLDER_OVERRIDE` and
`GMPI_WRAPPER_FOLDER_OVERRIDE` ([building.md](building.md)).

Two further reasons not to reach across:

- `gmpi_ui` is the rendering backend for **every** GMPI plugin and for SynthEdit
  itself, not just TIDE. It is exactly the shared code the GATED rule exists to
  protect, and "the fix looks small" is what the run prompt warns about.
- Both working copies were **dirty** at the time of this run, with in-progress
  Wayland work (`gmpi_ui` at `11051f1` with `backends/DrawingFrameWayland.h`
  modified; `GMPI_Wrappers` at `4a6a733` with `tests/wayland_editor_host.cpp`
  modified). Committing a resize fix into someone else's uncommitted branch is a
  good way to lose both.

Filed as BACKLOG **P4a** (gmpi_ui) and **P4b** (GMPI_Wrappers), and the scope
gap itself as **G3**.

## What was changed

One thing, in an ALLOWED path: `SE16/SynthEditSem/CMakeLists.txt` now emits a
PDB for Release builds of `TIDE` and `TIDE_VST3`, which P4 asked for.

```cmake
if(MSVC)
    target_compile_options(${SUB_PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_link_options(${SUB_PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/DEBUG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()
```

`/OPT:REF` and `/OPT:ICF` are restored explicitly because `/DEBUG` silently
turns both off, which would stop the linker stripping unreferenced code. The
cost is 11,776 bytes:

| | Before | After |
|---|---|---|
| `TIDE_VST3.vst3` | 2,969,600 | 2,981,376 |
| `TIDE.gmpi` | 2,712,576 | 2,724,352 |
| `TIDE_VST3.pdb` | *absent* | 10,031,104 |
| `TIDE.pdb` | *absent* | 8,998,912 |

The next Release crash report will symbolise without needing a Debug repro.
