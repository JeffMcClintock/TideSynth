# P4 — the editor-resize crash, diagnosed

BACKLOG **P4**. Root cause found, from the minidumps P2 left behind.

**Update, 2026-08-07:** Jeff lifted the scope block (G3), so the fixes were
written and landed — see [The fixes](#the-fixes).

**Update, 2026-08-08 (P4c): verified.** The crash is reproduced on demand and the
fixes are proven to stop it — not by driving a DAW, but by applying the input the
dump recorded straight to `IPlugView::onSize`. Crashes 3/3 with the fixes
reverted, survives 3/3 with them in. See
[Verification](#verification-p4c) — it supersedes
[What could not be reproduced](#what-could-not-be-reproduced), which is kept
because its dead end is worth not repeating.

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

## The fixes

Both landed 2026-08-07, after Jeff added `gmpi_ui` and `GMPI_Wrappers` to the
ALLOWED list (G3). Committed in their own repos, **not pushed**.

**P4a — `gmpi_ui/backends/DrawingFrameWin.cpp`.** `reSize` now re-reads both
pointers after `SetWindowPos` instead of trusting the test it made before it,
and refuses degenerate or over-limit extents up front:

```cpp
constexpr int maxSwapChainDimension = 16384;

if (width <= 0 || height <= 0 || width > maxSwapChainDimension || height > maxSwapChainDimension)
    return;

if (!d2dDeviceContext || (swapChainSize.width == width && swapChainSize.height == height))
    return;

SetWindowPos(windowHandle, nullptr, 0, 0, width, height, SWP_NOZORDER);

// SetWindowPos *sends* WM_SIZE and WM_PAINT rather than posting them, so OnSize
// and PaintFrame have already run by the time it returns -- and either releases
// the device. The check above is stale by this line.
if (!d2dDeviceContext || !swapChain)
    return;
```

This also closes the second latent deref: `swapChain` was never checked at all.

`reSize` is Windows-only. X11 has a separate `reSize(int, int)`
(`DrawingFrameX11.cpp:920`) and macOS an `onResize()` (`DrawingFrameMac.mm:434`);
neither was touched, and neither was audited for the same pattern — worth a look
by whoever owns those platforms.

**P4b — `GMPI_Wrappers/wrapper/VST3/SEVSTGUIEditorWin.cpp`.**
`checkSizeConstraint` now writes the nearest acceptable size back into the rect,
which is what the VST3 contract asks for, instead of returning `kResultFalse`
with the rect untouched.

**Verified 2026-08-08** — see [Verification](#verification-p4c).

**Not changed, deliberately:** `OnSize` has the same over-limit weakness as
`reSize` did — an out-of-range `WM_SIZE` arriving by some other route would still
fail `ResizeBuffers` and be misread as device loss, tearing down a working
device. It cannot crash (it checks both pointers), and with `reSize` clamped it
is no longer reachable from this path, so it was left alone rather than widening
a shared-code change. Worth filing if it ever bites.

## What could not be reproduced

**The original crash did not happen again, so the fixes are unverified.**

P2 recorded it 3 times out of 3. A fresh portable-REAPER harness, rebuilt from
P2's own recipe, could not produce it once — in any of these configurations:

| Build | Fixes | Resizes | Result |
|---|---|---|---|
| Release | both on | 7 | survived, window resized correctly |
| Release | gmpi_ui fix off | 1 | survived |
| Release | **both off** | 1 | survived |
| Debug | **both off** | 1 | survived |
| Release | both on (final) | 5 | survived, 0 crash dumps |

With both fixes disabled the code is behaviourally identical to what crashed, so
this is a harness difference, not a fix. One concrete discrepancy points at
where: **P2 reported that `MoveWindow` did not actually resize the window** —
`GetWindowRect` returned 1672×995 before and after — and that it crashed anyway.
In this harness `MoveWindow` resizes correctly every time (1672×995 → 1200×800 →
…). So the plugin window was in a different state in P2's session, and that state
is probably what produced the `{0, 0, 2178, 32672}` rect.

Untested guesses at what differs, for whoever picks this up: the portable copy
takes `%APPDATA%\REAPER` as it is *today*, which may have changed since 2026-08-06;
P2 made five launches with screenshots and `SetForegroundWindow` in between; the
monitor/DPI layout may differ. Filed as **P4c**.

So the fixes rest on the crash dump rather than on a before/after. That evidence
is strong — the stack proves the null dereference happened at that line, and the
re-entrancy is plain in the source — but it is not the same thing as watching the
crash stop.

> **Resolved by P4c, 2026-08-08 — and none of those guesses was the answer.**
> Chasing REAPER's state was the wrong problem. The dump already recorded the
> input; what was missing was a way to *apply* it. See below.

## Verification (P4c)

**The crash reproduces on demand, and the fixes stop it.**

The mistake in the paragraph above was treating the DAW as the experiment. It is
not — it is just a thing that once passed a bad rect. The dump already told us
exactly which rect, so the experiment is to pass that rect ourselves.

`GMPI_Wrappers/tests/win_editor_resize_host.cpp` is a ~330-line Win32 VST3 host:
load the plugin, create the editor, attach it to a real `HWND`, call
`IPlugView::onSize({0, 0, 2178, 32672})`. No REAPER, no config, no window state
to reproduce. Build it with `-DGMPI_WRAPPERS_BUILD_TESTS=ON`.

### The A/B, three runs of each

| `reSize` fix (P4a) | `checkSizeConstraint` fix (P4b) | `onSize(0,0,2178,32672)` |
|---|---|---|
| off | off | **`0xC0000005`, 3/3** |
| **on** | off | survived, 3/3 |
| **on** | **on** | survived, 3/3 |

P2 saw the original crash 3 out of 3 times. This reproduces it 3 out of 3 times.

**It is the same crash.** Frames 0 and 1 are identical to the minidump — only the
host frame differs, REAPER there and the harness here:

```
TIDE_VST3_prefix!gmpi::hosting::DrawingFrame::reSize+0x7c
00007ffe`4f7658ac 488b01          mov  rax,qword ptr [rcx]  ds:00000000`00000000
00007ffe`4f7658af ff9050020000    call qword ptr [rax+250h]
TIDE_VST3_prefix!wrapper::SEVSTGUIEditorWin::onSize+0x1e
win_editor_resize_host!main
```

`ExceptionCode: c0000005`, `Parameter[0]: 0` (read), `Parameter[1]: 0` (address).
A vtable load off a null COM pointer followed immediately by a virtual call —
`d2dDeviceContext->SetTarget(nullptr)`, the first use after `SetWindowPos`, which
is the "use" half of the time-of-check/time-of-use bug. The next instruction
loads the following member, `swapChain`, which is the second latent deref P4a
also closed.

*(This symbolised straight out of a **Release** build, using the PDB P4 added to
`SE16/SynthEditSem/CMakeLists.txt`. That change paid for itself here.)*

### The trap this test had to avoid

**A survival result is worthless unless the crash path was reachable.** The
editor's Direct2D device is created lazily on first paint; with no device the
unfixed `reSize` returns at its own first test, which looks exactly like the fix
working. Any harness that does not rule this out reports a false pass.

So the test performs a *benign* resize first and checks the window adopted it. A
resize that took effect proves `reSize` got past `if (d2dDeviceContext && ...)`
and reached `SetWindowPos` — the device is live and the crash path is genuinely
reachable. If it did not, the test exits **3, INCONCLUSIVE**, not 0.

This is not hypothetical: the first version of the harness measured the wrong
window and reported `editor live: NO`. Which brings us to —

### P2's `MoveWindow` lead was a red herring, and here is why

P2 recorded that `MoveWindow` "did not resize" the plugin window, and P4 built on
that, reasoning the window must have been in some unusual state. It was not.

**`attached()` creates a child window inside the HWND the host hands over, and
`DrawingFrame::reSize` calls `SetWindowPos` on that child.** The parent's client
rect never moves. P2 measured the parent. So did the first draft of this harness,
which is how the mistake was caught — the liveness probe failed on a build that
was demonstrably working.

Measure `GetWindow(hwnd, GW_CHILD)`. Nothing was ever in a strange state, and
there is no mystery DAW condition left to hunt.

### What is verified, and what is not

- **P4a is verified.** Reverting it alone brings the crash back, 3/3. It is the
  fix that stops the process dying.
- **P4b is verified as a contract fix, not as a crash fix**, because the crash
  path bypasses it — a host that ignores `checkSizeConstraint` (as the crashing
  one did) never calls it. Probed directly, the difference is plain:

  | | `checkSizeConstraint(0,0,2178,32672)` |
  |---|---|
  | before | `kResultFalse`, rect left at 2178 × 32672 — the host learns nothing |
  | after | `kResultTrue`, rect rewritten to 2178 × 600 — a size it can use |

  So P4b stops a polite host from ever reaching the crash input; P4a stops an
  impolite one from killing the process. Both were needed, as P4 argued.
- **Legitimate resizing still works** — the benign probe resizes the editor in
  every run, so the 16384 clamp did not turn resize into a no-op.
- Degenerate and over-limit rects (`0 × 0`, `1 × 1`, `16385 × 600`,
  `2178 × 16385`) are all refused without a crash.
- **Not covered:** the X11 and macOS resize paths (`DrawingFrameX11.cpp:920`,
  `DrawingFrameMac.mm:434`), which were never audited for the same pattern. The
  harness is Windows-only; `x11_editor_host` in the same directory is the obvious
  place to add the equivalent probe.

## Where the fix had to go

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
- Both working copies were **dirty** at the time of this run — at the time read
  as in-progress Wayland work (`gmpi_ui` at `11051f1` with `backends/DrawingFrameWayland.h`
  modified; `GMPI_Wrappers` at `4a6a733` with `tests/wayland_editor_host.cpp`
  modified). Committing a resize fix into someone else's uncommitted branch is a
  good way to lose both.

  **Corrected later the same day:** that was a misreading. The dirt was **pure
  CRLF line-ending churn** — `git diff --ignore-all-space` returns nothing for
  all three files, despite 8,424 and 1,019 lines of raw diff. No work was ever at
  risk. Always run that test before treating a dirty tree as work-in-progress,
  and revert churn rather than stashing it: restoring an 8,000-line CRLF rewrite
  is what turns a clean rebase into a merge conflict. See
  [the churn section](#the-crlf-churn-trap).

Filed as BACKLOG **P4a** (gmpi_ui) and **P4b** (GMPI_Wrappers), and the scope
gap itself as **G3**. **G3 was resolved the same day** — Jeff added both repos to
the ALLOWED list — so P4a and P4b were then implemented rather than left queued.
The two "do not reach across" reasons above still shape *how*: changes kept
tight, and only the intended file staged in each repo, leaving the Wayland work
untouched.

## The CRLF churn trap

Both shared repos sit dirty most of the time, and the dirt is **not** work. It is
whole files rewritten with the opposite line endings and no content change —
`backends/DrawingFrameWayland.h` (8,116 lines), `docs/vst3-linux-editor.md`,
`tests/wayland_editor_host.cpp`. The tell is a diffstat with matching insertion
and deletion counts:

```
 backends/DrawingFrameWayland.h | 8116 ++++++++++++-------------------
 docs/vst3-linux-editor.md      |  298 +-
 2 files changed, 4207 insertions(+), 4207 deletions(-)
                                 ^^^^ equal -- suspect churn
```

Confirm it, and act on it:

```bash
git diff --ignore-all-space -- <file>
```

Empty output means there is no real change. Revert with
`git checkout HEAD -- <file>`. Do **not** stash and restore it: this run stashed
the churn to rebase, and popping the stash conflicted against a genuine upstream
commit that touched the same file, leaving a conflicted tree for no reason. The
churn was reverted instead and the rebase went through clean.

This is also why the repo rule says never commit line-ending-only changes — a
churn commit would bury every real change to those files in the blame.

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
