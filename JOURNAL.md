# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

Every run on three machines reads this file in full, so its size is a cost paid
forever. It hit **192 KB across 37 entries in six days** before the first
rotation (**A8**, 2026-08-12). Nothing is ever deleted or rewritten — old
entries just move to a per-month archive.

**The rule, applied after you append your own entry:**

1. Move the oldest entries out, in order, into `JOURNAL-<YYYY>-<MM>.md` for the
   month each entry belongs to, appending **below** what is already there so the
   archive stays newest-first. Copy the template from
   [JOURNAL-2026-08.md](JOURNAL-2026-08.md) if that month has no file yet.
2. Stop when this file is **under 30 KB**, or when the **four most recent
   entries** remain — whichever comes first. **The floor of four wins**: the run
   prompt tells every run to read the last four entries, so a verbose month
   pushing this file over 30 KB is correct, not a rotation failure.
3. Never edit an entry while archiving it. The archive is the record.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-08-17 — macos — U1b complete: two depths, both directions (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged the outstanding PRs, asked
for the remaining one to be resolved and merged, then "do U1b's second half".
Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** resolved and landed the last open PR, then **finished U1b** —
[SynthEdit#33](https://github.com/JeffMcClintock/SynthEdit/pull/33). **The
rack is the default again and the structure view sits behind an unlock, with
all four navigation paths verified in REAPER.** Constraint 1's two depths are
now real in the plug-in.

**The routing, in one sentence:** the **master** container opens as the rack
(panel view — the product's face, workable now that U2d/U2e made panel
modules and controls draw); **any other** container opens as its structure
view; and `OpenViewForContainer` grows an optional explicit `view_flag` where
**0 routes by depth and a `CF_*` honours the caller**. That one parameter is
what turns SynthEdit's *existing* menu commands into the unlock and its
inverse — no new UI invented:

| gesture | result |
|---|---|
| open the plug-in | **rack** (panel view, modules drawing) |
| double-click a Container | its **structure** view, breadcrumb follows |
| **"Goto Structure…"** on the master | the master's **structure** view — the unlock |
| **"Panel Edit…"** | back to the **rack** |
| **"Main"** crumb | back to the **rack** |

**Learned — a false negative that cost an hour, and the tell.** "Panel
Edit…" appeared not to work: the canvas stayed on the fine structure grid,
and a List Entry inserted afterwards drew structure-style (module box + a
"Value Out" pin), which looked like confirmation. It was an artifact: the
context menu had been left open across a model switch, macOS auto-dismissed
it, and the click landed on the canvas instead. **Instrumenting
`TideApp::OpenView` settled it in one build** — `flag=256` (structure) then
`flag=128` (panel) both logged, with the two-tone rack canvas back on screen.
**The rule worth keeping: when a GUI verification contradicts a code path
that reads correct, suspect the input, not the code — and re-run the gesture
fresh before believing the failure.** A stale menu is invisible in a
screenshot.

**Learned — Jeff's rack machinery is right there, and U1c should start from
it.** `CContainer::OnMenuCommand` already handles
**`POPUP_MENU_TOGGLE_RACKMODULE`** (toggling `m_is_rack_module`, the flag
`ModuleViewPanel`'s JSON ctor already reads) and **`POPUP_MENU_TOGGLE_LOCKED`
→ `toggleLocked()`**. So U1c is enabling and surfacing existing code, exactly
as Jeff said — and the lock machinery is the natural home for a future
unlock UX if the menu command is ever felt to be too hidden.

**Also did — the PR queue is empty.** [#90](https://github.com/JeffMcClintock/TideSynth/pull/90)
(the linux S3 run's) was conflicting on all three coordination files; resolved
by keeping **both** sides' journal entries (S3 below the newer U1b entry, both
archives unioned) and **cross-picking** the NEXT rows — main's `mac`, the
branch's `linux`. **One lint trap worth recording:** the S3 entry quoted `nm`
output containing `[abi:cxx11]` immediately followed by `(CUG*)`, which `check-links.py` reads as a
markdown link to a file named `CUG*`; a space between `]` and `(` defuses it
without touching the quoted output's meaning. Every repo is now at zero open
PRs except this session's own.

**Next:** **U1c** — enable the existing rack-mode code (`m_is_rack_module`,
the rack axes already documented in `ViewBase::snapToGrid`), which is what
makes modules *snap into rack rows* rather than free-float. After that the
D-series surfaces (the about pane hangs off the breadcrumb bar, which now
exists). The win box still owes U2e's two follow-ups (staging + combo
re-verify).

**Side effects on this box:** four `TIDE_VST3` rebuilds; the installed
plug-in now opens as the rack. REAPER restarted twice; "Optimus HP" untouched
throughout; test tabs left open. Temp navigation logging was local-only and
is removed.

**Branch/PR:** this TideSynth PR +
[SynthEdit#33](https://github.com/JeffMcClintock/SynthEdit/pull/33) (against
`master`, no stack — the queue is clear).

---

## 2026-08-17 — macos — U1b: the breadcrumb bar navigates in and out (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff confirmed cable-drag and module
insertion work, had the repos synced and old branches cleaned, and said
"take next task". Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** **U1b's chrome-and-navigation half, wired and verified** —
[SynthEdit#31](https://github.com/JeffMcClintock/SynthEdit/pull/31), stacked
on [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29). **Verified in
REAPER, the full loop:** the bar shows "Main"; placing a Container (which
draws as a proper module box with pins) and double-clicking it navigates
inside — trail reads "Main › Container", the container's own IO Mod visible
— and clicking "Main" navigates back out with the forward trail retained
for one-click re-entry. Both directions of U1b's Accept, live.

**The build was mostly discovery, not invention.** `SE2::BreadcrumbBar`
already existed in `se_sdk3_hosting` — cross-platform, thumbnail-caching,
retained-trail, powering every editor frontend (Wayland/JUCE/WinUI/mac
bridge) — and `TopStripLayout`'s own comment says it grew from exactly this
strip. TIDE's work was wiring: the bar becomes a fourth strip in
`SynthEditGui`'s manual pane layout (origin-rooted arrange + PaneHostWrapper
offset + pane pointer routing, the exact pattern of the two browsers), and
`ISeApp` grows `OpenViewForContainer` plus two callbacks.

**The enter path was a latent crash, now a feature.** Double-clicking a
Container runs `PresenterCommand::Open` → `CContainer::OnMenuCommand` →
`Document()->OpenView` → `CSynthEditAppBase::OpenView` →
**`m_app_user_interface->OpenView` — and TIDE never sets
`m_app_user_interface`**, so the gesture was a null deref waiting for the
first curious user. `TideApp` now overrides that virtual and routes to the
GUI's navigation callback instead.

**One deliberate mechanism worth keeping: navigation is deferred.**
Requests originate inside pointer dispatch — a crumb click dispatched by
the GUI, or a double-click dispatched by the very view being replaced —
and rebuilding the view stack from within its own dispatch destroys the
object mid-call. The Wayland app defers to its event-loop tick; TIDE
defers to a one-shot `gmpi::TimerClient` tick (30 ms), with the callbacks
cleared and the timer stopped in the destructor. The scroll-wiring block
was extracted to `wireViewScrollbars()` so navigation re-opens rewire
identically to the first open.

**Scoped out, recorded rather than hidden:** thumbnails (`renderThumbnail`
left unset — the bar draws name-only crumbs; the EditorScreenshot helper
`se_cl::renderContainerThumbnail` is the follow-up), and **U1b's second
half** — restoring the rack as the *default* with the structure view
behind an unlock — which waits on the open PR queue
([#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)/[#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)/[#30](https://github.com/JeffMcClintock/SynthEdit/pull/30))
and on the unlock UX being decided. The row stays IN-REVIEW listing both.

**Housekeeping done at Jeff's ask:** all five repos synced (gmpi_ui#8 had
merged — U2b's middle-pan is on main), and thirteen local branches with
merged PRs deleted across four repos; only open-PR branches and Jeff's own
release branches remain.

**Next:** merge queue for Jeff — [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
→ [#30](https://github.com/JeffMcClintock/SynthEdit/pull/30), [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)
→ [#31](https://github.com/JeffMcClintock/SynthEdit/pull/31), plus
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13).
Once the queue clears: U1b's default-flip/unlock half on a clean base, then
**U1c** (enable Jeff's existing rack-mode code). The win box still has
U2e's two cheap follow-ups.

**Side effects on this box:** two `TIDE_VST3` rebuilds; the installed
plugin now carries the breadcrumb (struct-interim lineage). REAPER
restarted once; "Optimus HP" untouched; the test tab holds a Container
demonstrating the trail.

**Branch/PR:** this TideSynth PR +
[SynthEdit#31](https://github.com/JeffMcClintock/SynthEdit/pull/31)
(stacked on [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)).

---

## 2026-08-17 — linux — S3 (TIDE-side half), plus two platform:linux breaks found and filed

**Prompt:** b3e9876 · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as tide-rack-bot

**Did:** closed **S3**'s TIDE-side half —
[SynthEdit#32](https://github.com/JeffMcClintock/SynthEdit/pull/32) — and, while
building the baseline it needed, found that **this box cannot build `main` at
all** and filed both causes as
[#87](https://github.com/JeffMcClintock/TideSynth/issues/87) and
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88). The build breaks
are the more important half of this run.

**STEP 1 and 1.5 were clean at the start** — no open `platform:linux` issue in
any of the five repos, no `tide/linux/**` PR. The three open `tide/mac/**` PRs
are green with nothing unresolved and were left alone. The `linux` NEXT row said
**S3**, it survived screening (`SE16/SynthEditSem/TideApp.cpp` is ALLOWED, no
open PROPOSED entry touches it), and its acceptance check was stateable before
starting, so it was takeable.

**Break 1 — [#87](https://github.com/JeffMcClintock/TideSynth/issues/87):
`SynthEditLib` does not compile with GCC, and the cause says so itself.**
`modules/se_sdk3_hosting/ModuleView.cpp:621-633` carries

```
// TEMPORARY U2d trace - local only, do not commit.
#include <cstdio>
#include <cstdarg>
static void tideTraceLog(...)  { if (FILE* f = fopen("/tmp/tide-skin-debug.log", "a")) ... }
```

committed as `227ba48` via
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12) (U2d).
`namespace SE2 {` opens at `:38` and closes at `:1893`, so those two `#include`s
are **inside `SE2`** and declare a nested `SE2::std`. Every later unqualified
`std::` in the file then resolves there and fails — 30+ errors starting at
`:696`, the first `std::` use after the includes: *"‘SE2::std::map’ has not been
declared"*, then `make_unique`, `vector`, `max`, `min`, `string`, `unique_ptr`.
The file's own `using namespace std;` at `:34` cannot help, because qualified
lookup finds `SE2::std` first.

**Why no other box has seen it, and this is the part worth keeping:** both
headers are include-guarded. On a toolchain that already pulled them in
transitively before line 623, the two lines expand to **nothing** and no
`SE2::std` is created. libstdc++ here does not, so it is created. **The bug is
equally present in the source on all three platforms; only Linux is unlucky
enough to be told.** A misplaced `#include` inside a namespace is invisible
wherever the header happens to have been included already.

**It is also a live constraints 3 and 4 violation in the shared library**, not
just a build break: `fopen("/tmp/tide-skin-debug.log", "a")` runs unconditionally
in both `ModuleView` constructors, no `#ifdef`, in Release — a hard-coded
absolute path outside the bundle, in code SynthEdit links too. One revert fixes
both problems.

**Break 2 — [#88](https://github.com/JeffMcClintock/TideSynth/issues/88):
`SynthEditWayland` fails to link, and C12e is why.** `undefined reference to
doDialogConnectUg(CUG*)` and `doDialogPatchManager(CUG_with_patches*)`. C12e
(`a2ffdcd3c`) took `Dialogs_editor2.cpp` off EditorLib's source list so each app
compiles it directly; `SynthEditCL/CMakeLists.txt:42` and
`SynthEdit2/SynthEdit2.vcxproj:290` got the entry, **`SynthEditWayland` and
`SynthEditJuce` did not**. Neither target is generated on Windows, where C12e was
verified — its journal entry's "904/904 RC=0, TIDE.gmpi and TIDE_VST3.vst3 both
link" is all true and touches neither. Same shape as the 2026-08-14 finding here:
a target below a platform gate is only ever tested below that gate.
`SynthEditWayland/CMakeLists.txt` already sets `EDITOR2_DIR` (`:134`) and already
compiles `SynthEditApp.cpp` (`:160`), so it is one line short. `SynthEditJuce`
is not generated on this box, so that half of the issue is by inspection and the
issue says so.

**Neither was fixed, and that is the run's one real judgement call.** STEP 1 says
a broken build on your platform outranks all backlog work and tells you to fix
it. STEP 5 says `SynthEditLib` is GATED, and `SE16/SynthEditWayland/` and
`SE16/SynthEditJuce/` are on neither list so they are GATED by default. **Both
fixes are one revert and one line, which is exactly the situation STEP 5 warns
about** — *"do not reach across the line because the fix looks small — that is
precisely when it is tempting"*. So: filed, with the full diagnosis and the exact
fix, and not touched. The 2026-08-17 macOS run's process note about
`SynthEditLib` being called ALLOWED in a row and GATED in the prompt is no longer
abstract; it now blocks a build fix on a broken platform. **That contradiction is
the thing to resolve, and it is Jeff's.**

**S3 itself, and this row named one of its three functions wrongly.**
`doDialogBuildCodeSkeleton` is declared by **no header anywhere** and called by
**nothing** — checked across `SE16`, `SynthEditLib`, `gmpi_ui` and
`GMPI_Wrappers`. It was dead weight, not a guard, so it is deleted rather than
made loud. The live "Build Code Skeleton..." path never went through it:
`MfcDocPresenter.cpp:1276` → `POPUP_MENU_DEBUG_CODE` → `CUG.cpp:2034`'s
`VO_Notify(OM_SHOW_CODE_SKELETON_DIALOG)`, whose only handler is the WinUI3 app's
`MainWindow.xaml.cpp:762`. TIDE registers none, so it is dropped. **Consequence
for the sandbox audit: finding A6's `create_directory`/`copy_file` sites in
`CUG::BuildSkeletonCode` are unreachable in TIDE, though still linked** — A6 read
the stub as the guard on that path and it never was. The other two,
`doDialogConnectUg` and `doDialogPatchManager`, *are* reachable
(`CUG.cpp:2635`, `CUG_with_patches.cpp:164`) and now report on stderr on every
build, keeping the `assert` for debug.

**Why stderr and not something louder**, since the row said "fail loudly":
`abort()`/`std::terminate()` kills the host DAW, which is strictly worse than the
no-op it replaces and is the P4 failure; a message box is a modal dialog
(constraint 5) needing a parent window TIDE may not have under AUv3, which is why
`TideAppStubs.cpp` already stubs `SafeMessagebox` to nothing; a log file is a
write outside the bundle (constraints 3 and 4) — the very thing the audit filed
these under. stderr is what is left, and it is already this project's answer to
the same question at `ModuleView.cpp:684` (*"Loud in Release on purpose … stderr,
not a dialog"*). The reasoning is in the code, not just here.

**Verification artifact — A/B on the shipping binary, with a positive control.**
`TIDE_VST3.so`, Release, `-DNDEBUG -O3` confirmed from `ninja -t commands` on
`TideApp.cpp.o`:

| Measurement | before | after |
|---|---|---|
| `"TIDE ships no such dialog"` in `strings` | 0 | **1** |
| `doDialogBuildCodeSkeleton` in `nm -C` | `T doDialogBuildCodeSkeleton[abi:cxx11] (CUG*)` | **absent** |
| `doDialogConnectUg` / `doDialogPatchManager` | present | present |
| `__assert_fail` in `nm -uC` | **0** | **0** |

That last row is the one that matters: it measures S3's premise rather than
asserting it. The old `assert(false)` compiled to **literally nothing** in a
shipping build — there is no `__assert_fail` reference in the binary at all, so
the stubs really did return as though the dialog had been shown and cancelled.
The control is the pair of symbols present in both binaries, which shows the
absence of the third is a real deletion and not a tooling artifact.

Builds, in the same tree: **`TIDE_VST3` 297/297** (links `TIDE_VST3.so`,
assembles the bundle), **`TIDE.gmpi`**, **`SynthEditCL` 19/19**. `SynthEditWayland`
is red for #88's reasons, not this change's — its two undefined symbols are
defined in `TideApp.cpp`, which is not on that target's link line before or after.

**Learned:**

- **A `#include` inside a namespace is a platform-dependent time bomb, and the
  guard is what hides it.** Whether it does damage depends entirely on whether
  something else already included that header in that TU. Worth a lint; nothing
  about the source tells you which platforms are affected.
- **"Loud in release" has a narrow menu in a plugin.** Three of the four obvious
  options each break a PLAN constraint or kill the host. Anyone reaching for
  `abort()` on a future S3-shaped row should read `TideApp.cpp`'s comment first.
- **A stub is not evidence that a path is guarded.** A6 assumed
  `doDialogBuildCodeSkeleton` sat on the Build Code Skeleton path; it sat on
  nothing. Check the call graph, not the name.
- **Reading a shared working tree read-only has a limit.** Verifying S3 needed a
  `SynthEditLib` that compiles, and #87 meant there was none. Solved with a
  throwaway `git clone` of it into the scratch dir with the trace removed, used
  only as a `SYNTHEDITLIB_FOLDER_OVERRIDE`. Nothing was committed there and Jeff's
  checkout was never modified — worth repeating rather than patching his tree
  and hoping to restore it.

**Next:** **[#87](https://github.com/JeffMcClintock/TideSynth/issues/87) and
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88) first**, by whoever
is allowed to touch them — until then Linux is red and every "linux verified"
claim on this repo is worth re-checking. **S3g** carries S3's other half (the
menu entries, all GATED, NEEDS-JEFF). **Do not take C12d** despite its `linux`
mark: its Accept requires `SynthEditWayland` and `SynthEditJuce` to link under
GCC and #88 stops both. The next thing this box can actually finish is **P10**.

**Side effects on this box:** none to Jeff's trees. All five working copies were
**clean at start**, all are back on their default branches, and only
`SE16/SynthEditSem/TideApp.cpp` was ever modified. The build tree, the
`SynthEditLib` clone and the logs are all under the session scratch dir, not in
`~/SE`; Jeff's own `~/SE/build` was not touched or read into. No GUI, no host —
a scheduled run cannot get that approval, so nothing here is a runtime
observation.

**Branch/PR:** `tide/linux/S3-dialog-stubs` in both repos — this TideSynth PR +
[SynthEdit#32](https://github.com/JeffMcClintock/SynthEdit/pull/32). Merging one
without the other is harmless here: the TideSynth side is bookkeeping only and
the SynthEdit side is self-contained.

---

## 2026-08-17 — macos — U2e closed on mac: the combo box draws (interactive session, Jeff directing)

**Prompt:** n/a — interactive session continuing from yesterday's five; Jeff
confirmed cable-drag and module insertion work in the structure view, then
said "do the next task". The box's own scheduled run fired unattended at 06:19 and completed the same trace from source ([docs/u2e-pin-delivery-trace.md](docs/u2e-pin-delivery-trace.md)) — this session read SDK and code independently, converged on the same mechanism, and shipped the fix it framed as "a decision, not an investigation". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** **U2e's pin-delivery trace, to the bottom, and the fix** —
[SynthEdit#30](https://github.com/JeffMcClintock/SynthEdit/pull/30), stacked
on [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28). **Verified in
REAPER: a placed List Entry draws as a real combo box with styled title, and
the module browser shows exactly the fixed module set.** The row's Accept is
met on mac.

**The trace, mechanically.** SDK3 semantics read from the SDK itself:
`setPin` stores only; handlers fire **only on `notifyPin`**; the base
`initialize()` does nothing (its fire-all is deprecated in place). Initial
values are sent by `ViewBase::ConnectModules` STEP 2 — which iterates
**`moduleInfo->gui_plugs`** to parse and send each pin default. And
`gui_plugs` was **empty**: the pin descriptions live in `ControlsXp.xml`,
which only the module scan ever loaded — and TIDE's scan is gone by design
(S1a). No descriptions → nothing sent → no `notifyPin` → `onSetAppearance`
never ran → no widgets. Every layer below (registration, resources, guards)
was real but insufficient; this was the last missing piece.

**The fix, in two parts.** CMake stages `ControlsXp.xml` into
`Contents/Resources` **from SynthEditLib's copy** — single source of truth,
no drift (`BundleInfo::getResource` falls back to exactly that folder on
mac; the P6 rule keeps data out of `MacOS/`). `TideApp::InitInstance` then
merges it — **into already-registered classes only**. The merge-only filter
was learned live, not designed up front: a plain `RegisterPluginsXml` call
**grew the browser** with insertable phantoms (Keyboard (MPE), Scope3, Volt
Meter… XML-only, no class — one placed as an empty adorner before the
filter existed). `Module_Info3_internal`s without constructors are NOT
hidden by the browser's `isDllAvailable()` filter, so curation must happen
at registration: iterate the XML, `GetById`, `ScanXml` only on hits.
Constraint 7's fixed set stays exactly as curated — verified by eye against
the browser before and after.

**What the day-and-a-half arc adds up to.** U2 filed four symptoms two days
ago; every one is now DONE or IN-REVIEW with the mac verify green: wheel
(U2a ✓ merged), middle-pan (U2b ✓ merged, Jeff's hand on the mouse),
centring (U2c ✓ merged), registration + first modern panel (U2d ✓ merged),
and now pin delivery + a drawing control (U2e, #28+#30 +
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13)
in review). The structure view draws full module boxes with pins and
patchable cables ([#29](https://github.com/JeffMcClintock/SynthEdit/pull/29),
Jeff verified cable-drag by hand). **TIDE went from "renders a grid and
silent adorners" to "a patchable structure view AND a panel view that draws
real skinned controls" in two sessions' worth of days.**

**Learned — the browser does NOT filter unavailable internal modules.**
`ExportModuleNames` skips `!isDllAvailable()`, but `Module_Info3_internal`
never sets that flag false for XML-only entries (the assignment in
`RegisterPluginConstructor` is commented out as "might be needed?"). Anyone
registering module XML wholesale into a scanless product will grow the
insert menu with phantoms. The merge-only loop is the pattern; noted here
because the win box will want it too.

**Next:** the win box has two cheap U2e follow-ups (staging equivalent —
win reads the same file as a fallback after the exe resource — and the
combo re-verify). Then the board is exactly what the NEXT row says: **U1b**
(breadcrumb + restore rack-as-default with the structure view behind the
unlock — both classes link, the flip is one line) and **U1c** (enable
Jeff's existing rack-mode code). **P10** unchanged as fallback.

**Side effects on this box:** three more `TIDE_VST3` rebuilds; the
installed plugin now carries the full U2a–U2e stack (panel view default on
this branch lineage) and staged `ControlsXp.xml`; REAPER restarted three
times, "Optimus HP" untouched; the throwaway tab with Jeff's two-oscillator
cable patch from last night was lost to a restart — two modules and one
cable, noted for honesty.

**Branch/PR:** this TideSynth PR +
[SynthEdit#30](https://github.com/JeffMcClintock/SynthEdit/pull/30) (stacked
on [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28); merge #28 →
#30, or together; [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)
is independent).
