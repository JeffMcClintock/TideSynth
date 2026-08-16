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

## 2026-08-16 — macos — U2e first pass: crash-free placeholders, one question left (interactive session, Jeff present)

**Prompt:** n/a — interactive session, fifth of the day; Jeff verified
middle-drag by hand, merged
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8),
[SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27) and
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12)
mid-session, and said "keep going". Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** flipped **U2b** and **U2d** to DONE (merged + verified — U2b by
Jeff's own middle-drag), then took **U2e** far enough that the classic
controls are **crash-free, visible, and one isolated question from
working**: PRs
[SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28) and
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13).

**U2e finding 1 — TIDE never seeded the resource folders.** The base
`CSynthEditAppBase::InitInstance` seeds
`GmpiResourceManager::resourceFolders` (skin images among them);
`TideApp::InitInstance` replaced the base wholesale for S1a and dropped
that seeding, so every skin-image URI resolved against an **empty map**.
Seeded now (`Image` only — the one type panel controls read). This is a
real prerequisite for widget bitmaps, **but it was not the crash gate**:
rebuilding with the seed alone still crashed in
`ListEntryGui::arrange`.

**U2e finding 2 — the actual gate, isolated to one sentence.** Widgets
are built inside `onSetAppearance()` — a **pin-update handler**
(`ListEntryGui.cpp`: ctor `initializePin(pinAppearance, …onSetAppearance)`,
handler gated only by `currentAppearance == pinAppearance`, ctor default
`-2`). Had the handlers fired even once with default pin values,
`ACM_PLAIN` would have built a ListWidget — the vector being empty at
crash time means **the pin-update handlers never run at all in TIDE's
SDK3 hosting**. The next U2e step is therefore a single directed trace:
how `ModuleView`'s Sdk3 path delivers initial pin values (the "fake
plugs" `Ctl_Combo::Export` writes) and why the handler pass never
happens — the same wiring the editor exercises when these controls work
in full SynthEdit.

**U2e finding 3 — with `arrange()` guarded, the state is honest and
stable.** Third SIGSEGV site from the same root (initialize → measure →
arrange, all `widgets[]` on empty); guard landed
([SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13)).
**Verified in REAPER on the final build: no crash through instantiate →
insert → select; the placed List Entry draws as a right-sized, selectable
100×20 placeholder at the click point; Background Image renders
alongside; properties pane fully correct.** The classic controls stay in
the module list — a visible empty placeholder plus a stderr breadcrumb
beats both a dead host and an invisible module.

**Learned — pin defaults argue the diagnosis for us.** When a handler's
absence can be inferred from what default values *would* have built, the
"is it invoked at all vs does it fail inside" fork resolves without
instrumentation. That saved a fourth build-and-crash cycle.

**Next:** **U2e's pin-delivery trace** is the single remaining step
between TIDE and usable classic controls — after it, the combo should
draw for real and U1c's costing finally has a live control to look at.
**U1b** remains the headline. **P10** untouched as fallback.

**Side effects on this box:** `SynthEdit/build/` rebuilt `TIDE_VST3`
three more times; the installed plugin now carries U2a+U2b+U2c+U2d+the
U2e prerequisites and is crash-free (verified). REAPER crashed twice
more during diagnosis (both filed in the U2e row's stack list, same
root) and was relaunched; "Optimus HP" untouched throughout. Working
copies: `SynthEdit` on `tide/mac/U2e-resource-folders`, `SynthEditLib`
on `tide/mac/U2e-arrange-guards` (both pushed, PRs open); returned to
defaults after push.

**Branch/PR:** this TideSynth PR +
[SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28) +
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13);
merged mid-session by Jeff:
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8) (U2b, his
own middle-drag as the verify),
[SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27) +
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12)
(U2d).

---

## 2026-08-16 — macos — U2b + U2d fix session: first modern panel renders (interactive session, Jeff present)

**Prompt:** n/a — interactive session, fourth of the day on this box; Jeff
said "work on as many tasks as possible, don't stop" and merged PRs live as
they opened. Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** **U2b** (mac middle-button pan,
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8)), and the
**U2d fix session** — root cause found two layers deeper than yesterday's
hypotheses, first fix landed and **verified: TIDE drew a modern module
panel in a host for the first time.** Jeff merged
[gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7) (U2a wheel)
and [SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26)
(U2c centring) mid-session, so **U2a and U2c are DONE** — both were
live-verified before their PRs opened.

**U2d, the actual mechanism, nailed by one trace line.** The typeId
instrumentation printed `ctor(json): 'SE List Entry' -> moduleInfo=0x0` —
correct id, same `CModuleFactory` singleton the browser uses (the two-
factory theory died: `ModuleFactory()` is a `#define` for `Instance()`),
so the id is simply **not registered in the VST3**. Why: **the modern
control modules compile only inside `IF(SE2JUCE)`**
(`SynthEditLib/CMakeLists.txt:553`) — mirrored by
`initialise_synthedit_modules`'s force-link lists sitting in
`#if GMPI_IS_PLATFORM_JUCE==1` and `#if SE_GRAPHICS_SUPPORT` blocks, the
latter macro **never defined for the compiler anywhere** (undefined
identifiers are 0 in `#if`). Full SynthEdit never noticed because it
scans modules from disk; TIDE — scan removed by S1a, by design — is the
first product that needed the static path, and it never existed. The
browser still lists "List Entry" because the **legacy DocObs**
(`Ctl_Combo`) link via direct reference; `Ctl_Combo::Export` writes
`"type": "SE List Entry"` into the panel JSON — so model right, view
empty, both platforms: **win's P11 dialog and mac's silence are one
defect's two faces**, and it explains #78's "(3) persists after P11
fixed".

**The fix, and what it proved.** TIDE now lists its fixed module set
(PLAN constraint 7) as **direct target sources** in
`SynthEditSem/CMakeLists.txt`
([SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27),
stacked on #26) — target sources cannot be dead-stripped. First entry:
`Controls/PlainImageGui.cpp` (`SE Background Image`). **Verified in
REAPER: the default document's Background Image module — the thing P11's
win error named "at instantiate" — draws as a real panel with working
resize adorners.** Deliberately NOT done: defining `SE_GRAPHICS_SUPPORT`
lib-wide — it gates dormant code in `Controller.cpp`/`MpParameter.cpp`
and would double-register modules in the scanning editor
("Module found twice" boxes).

**The classic SDK3 controls are a further layer, filed as U2e with three
crash stacks.** Listed with their widget deps, `SE List Entry` registers,
constructs — and crashed REAPER three different ways in one hour:
`ClassicControlGuiBase::initialize` (`widgets.back()` on an empty vector —
address -16 IS the tell: empty `back()` with 16-byte `shared_ptr`
elements), then `ListEntryGui::measure` (`widgets[0]`, null at 0x0), then
`ListEntryGui::arrange` after both guards. **The widget layer assumes skin
bitmaps/fonts load during pin init and never checks** — 47 unguarded
`widgets[` sites in `ListEntryGui.cpp` alone, so guarding call-sites is
whack-a-mole; the fix session must make widget-building succeed (or fail
into a placeholder widget) instead. Guards for the first two crash sites
plus the Release-loud `ModuleViewPanel` unregistered-type diagnostic are
in [SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12);
the classic controls stay **out** of TIDE's module list until U2e lands —
**final state verified: no crash, Background Image draws, a placed List
Entry degrades to the adorner + a stderr line instead of killing the
host.**

**Learned — the module-set list is the constraint-7 lever.** TIDE's
"fixed module set, compiled in" now has a literal, reviewable home: the
source list in `SynthEditSem/CMakeLists.txt`. Growing the rack's palette
= adding a file there and verifying its layer actually renders. That is
a better shape than any registry define.

**Learned — U2b is code-complete but untested by hand:** synthetic
middle-drag isn't available to the agent's tooling, so
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8) awaits
Jeff's real mouse. The handlers mirror the proven left/right pairs and
are gated to `buttonNumber == 2`.

**P10 was deliberately not taken** despite being minutes: it would have
meant a third stacked SynthEdit branch mid-session with the build tree
checked out elsewhere; it stays the mac fallback. **P11 gained a mac
finding:** the win post-build module-DB copy has **no mac counterpart at
all** — `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` sat 3.5 months stale
(May 7, pre-P5 identity) until this session refreshed it by hand; noted
in [docs/building.md](docs/building.md).

**Next:** **U2e** is the gate on the rack showing *controls* (Background
Image proves the pipeline; the classic widget layer is what stands
between TIDE and a usable List Entry). **U1b** remains the headline.
U2b/U2d flip on their PRs; U2a/U2c archived DONE.

**Side effects on this box:** `SynthEdit/build/` rebuilt `TIDE_VST3`
five times and `TIDE` once; the installed VST3 now carries U2a+U2b+U2c+
the PlainImage registration and is **crash-free**. REAPER crashed three
times (all TIDE_VST3 faults, all filed with stacks) and was relaunched;
"Optimus HP" never saved or modified. `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi`
refreshed to today's build. Temp trace logging in `SynthEditLib` was
reverted; the four files now changed there are the real guards/diagnostic
on the PR branch. Throwaway REAPER tabs left open for Jeff.

**Branch/PR:** this TideSynth PR +
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8) +
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12) +
[SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27); merged
mid-session by Jeff: [gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7),
[SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26).

---

## 2026-08-16 — macos — U2c fix + U2d falsifier (interactive session, Jeff present)

**Prompt:** n/a — interactive session, third of the day on this box, at Jeff's
direction ("do the U2c one-liner and U2d falsifier now"; logging explicitly
blessed). Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** **U2c** — the one-line centre default,
[SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26), verified
live. **U2d** — ran the row's falsifier and kept going until the mechanism
fell out: **both cheap hypotheses are refuted, and the real one is named.**

**U2c, closed end to end in one cycle.** `TideApp::OpenView` now calls
`setCenter({viewDimensions/2, viewDimensions/2})` after `setDocument`.
REAPER 7.45/macOS-arm64, fresh process: **the gridded canvas fills the whole
pane** — no corner dead-strips, across every subsequent fresh load this
session — and click-placed modules land at the click point, in view. The §6
"offset" is dead as a default; U1c still owns what "home" ultimately means.

**U2d falsifier, round 1 — skins were never missing, and the hypothesis dies
on a path quirk worth keeping:** `BundleInfo::getCommonDocumentFolder()`
resolves to plain **`$HOME`**, not `~/Documents` — so SkinMgr reads
`~/SynthEdit Projects/skins/`, which on this box already held 8 real skins
from Jeff's SynthEdit install, and the temp logging showed
`getSkin('default3')` scoring an **exact hit** at plugin load. (A seeding
copy aimed at `~/Documents/SynthEdit Projects/skins` — where this session
first looked — would target the wrong place entirely.) Placed List Entry:
still adorner-only.

**Round 2 — the stale module DB was real, and it still was not the cause.**
`/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` was **3.5 months stale** (May 7:
pre-P5 identity `name="SynthEdit"`, 0 `ContainerViewPanel` symbols), and
**the mac build never refreshes it — P11's win row describes a post-build
copy that simply has no mac counterpart**; today's `TIDE.gmpi` sits only in
`build/SynthEditSem/Release/`. Manually installing the fresh one and
sidelining the editor's `Plugin-Cache*.xml` (renamed `*.u2d-bak`, restorable)
changed nothing: still adorner-only. Also learned in passing: the VST3 runs
fine with no cache XMLs present, so those caches belong to the editor app,
not the plugin.

**Round 3 — the log that ends the hunt.** With `ModuleView::Build`
instrumented: it fires for `SE PatchCableChangeNotifier` and
`PatchAutomator` (invisible utilities, `windowType=0`, GUI2 objects
constructed fine) and **never fires at all for the placed `SE List Entry`.**
The only silent pre-`Build` exit is `ModuleViewPanel`'s
`if (!moduleInfo) return;` — *"unregistered module type"*, whose diagnostic
is `_RPTN`, Debug-only (`ModuleView.cpp:659`). **So the GUI class
registration for the SE control modules never reaches the panel view's
module factory in the mac VST3, and the view constructs empty — the adorner
then hugs a zero rect, which is exactly what Jeff identified on screen.**
This unifies the two platforms: win's P11 dialog and mac's silence are one
defect with two failure surfaces, and it explains the win re-test's
"(3) persists after P11 fixed" — refreshing the DB satisfied the *export*
path, not the view's factory. U2d's row now carries the next moves: trace
where the mac VST3 populates the module factory
(`LoadOrScanModuleData`/CUG), and make the unregistered-type path **loud in
Release** so this class of failure can never be silent again.

**Learned — a latent trap for whoever ships skins with TIDE:**
`GetHomeDir()` is the dylib's own directory, so SkinMgr's seeding source
`{home}/Resources/skins` resolves to **`Contents/MacOS/Resources/`** inside
the bundle — the exact layout P6 spent six days learning that `codesign`
refuses. Bundle staging for TIDE must target `Contents/Resources` AND teach
the seeding path to find it, or skip user-folder seeding entirely
(constraint: sandbox-safe means the plugin should read its own bundle, not
write `~/…`).

**Temp instrumentation:** SkinMgr + ModuleView logging was local-only and is
reverted; the misplaced `~/Documents/SynthEdit Projects/skins` seed is
removed (the pre-existing "Mac Export" content untouched). What remains on
the box deliberately: the **refreshed `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi`**
(a legitimate update of a 3.5-month-stale install) and the sidelined
`*.u2d-bak` cache files.

**Next:** **U2d** is now a scoped fix session (factory-population trace +
loud failure), and it still gates the rack displaying anything; **U1b**
after it, per the NEXT row. The win box can cheaply confirm the unified
mechanism by checking whether its `SE List Entry` `ModuleViewPanel` gets a
`moduleInfo` after a DB refresh.

**Side effects on this box:** `SynthEdit/build/` rebuilt `TIDE_VST3` twice
and `TIDE` once (PostBuild reinstalled the VST3 each time — it now carries
U2a's wheel fix and U2c's centring); REAPER quit/relaunched three times
(module-reload lesson), "Optimus HP" never saved or modified, throwaway
tabs left open. `gmpi_ui` and `GMPI_Wrappers` untouched this entry;
`SynthEditLib` was instrumented and **restored byte-clean**.

**Branch/PR:** this TideSynth PR +
[SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26) — the
SynthEdit one carries the code; they need not merge together.

---

## 2026-08-16 — macos — U2 triage + U2a wheel fix (interactive session, Jeff present)

**Prompt:** n/a — interactive session, second of the day on this box; Jeff at
the keyboard contributing live observations. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** the triage U2's own Accept asked for — root causes named for all
four symptoms, split into **U2a/U2b/U2c/U2d**, full note in
[docs/u2-triage-2026-08-16.md](docs/u2-triage-2026-08-16.md) — **and fixed
U2a in the same sitting**: the mac scroll wheel,
[gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7), live-verified
in REAPER before the PR was opened.

**The wheel was a TODO, not a bug.** `DrawingFrameMac.mm scrollWheel:`
computed deltas and flags, then dropped the event — both dispatch lines
commented out (`:813`/`:818`), with the VST3-level `onWheel` fallback also
returning `kResultFalse`. The fix mirrors the proven Windows path
(`inputClient->onMouseWheel`, 120-per-notch, `ScrollHoriz` for `deltaX`)
using the `inputClient` the mac frame already used for pointer events —
which is why clicking always worked while the wheel did not. **Verified on a
fresh REAPER process:** canvas pans both directions, Ctrl+wheel zooms, the
module browser list scrolls and reveals entries that were previously
unreachable by any input, since TIDE hides scrollbars and middle-pan is also
dead (that one is **U2b**, filed: the backend has no `otherMouse*` handlers
at all).

**The Windows re-test ([#78](https://github.com/JeffMcClintock/TideSynth/pull/78))
landed mid-triage, and the two boxes answered each other's open questions.**
Their "(2) does not reproduce on Windows" is this box's root cause seen from
the other side — the Windows dispatch was always finished. Their *"every
later module landing on that same point"* is **U2c**'s mechanism named on
this box: `TopView::centerPos` defaults `{0,0}` (`ViewBase.h`), TIDE never
calls `setCenter`/`setPanZoom`, and `AddModule(moduleId, view->getCenter())`
inserts at exactly that corner — so the §6 "canvas offset / dead strip" both
boxes see is a **pan default, not a drawing bug**, and the fix is one line in
ALLOWED `TideApp.cpp`. Their *"(3)+(4) one geometry cause"* guess was half
right: adjacent, but (4) is that one-line default while **(3) is the real
remaining unknown — U2d**.

**U2d is the gate on the rack showing anything, and Jeff cracked its
description live:** the placed control draws **only its ResizeAdorner**
around a degenerate rect — "blue rectangle with white circles, only the
resizer" — model fully correct in the properties pane, panel drawing
nothing. Standing hypothesis, one leg short of proof: the panel pipeline is
skin-driven (`ContainerView.cpp:25` → SkinMgr/GmpiResourceManager) and
`TIDE_VST3.vst3` stages **no Resources at all** (binary + Info.plist +
signature; contrast SynthEditCL's staged `fonts`/`skins`/`templates`). Cheap
falsifier in the row. P11 is ruled out as its cause — the Windows session
fixed that trap and (3) still reproduced.

**And (1) is not a code defect on either platform.** Placement is
click-to-arm → click-to-place by design (`OM_DRAG_NEW_MODULE` →
`ViewBase::DragNewModule` → drop on the next `onPointerDown`), **proven
live**: browser click, canvas click, module placed at the exact click point.
What both boxes reproduced is that the press-drag-release gesture users try
first does not place. UX decision recorded in the doc; default in effect:
the design stands.

**Learned — a host keeps a VST3 module mapped after FX-remove.** Remove →
replace bundle → re-add loaded the OLD dylib, and the first post-fix wheel
test "failed" purely for that reason; a full REAPER restart picks up the
replacement. Budget a restart into every mac edit-build-verify loop.

**Learned — correcting this morning's entry:** `reaper-vstplugins_arm64.ini`
is not just laggy, it is **not a live mirror at all** — byte-identical
through a clear-cache re-scan *and* a clean quit while the FX browser showed
the new identity throughout. "The ini rewrites when REAPER exits" was an
overclaim; the durable rule is: read the FX browser, never the ini. (Also
told Jeff live: this box's `reaper.ini`/`reaper-reginfo2.ini` are owned by
**root**, so REAPER cannot persist its preferences here — his machine's
quirk, not TIDE's.)

**Next:** **U2c** is the best minutes-sized item on this box (one line,
ALLOWED, fixes the corner anchor and where inserts land on both platforms);
**U2d**'s falsifier decides whether the rack can display anything and wants
running before or alongside **U1b**'s chrome. U1b remains the headline item;
**U1c stays uncosted until U2d lands**. **P10** unchanged as fallback.

**Side effects on this box:** `gmpi_ui` gained one commit on a PR branch
([gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7), 5+/6-);
`SynthEdit/build/` rebuilt `TIDE_VST3` (Release) and its PostBuild step
re-installed `~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3` — now carrying
the wheel fix. REAPER was restarted twice (module-reload lesson above);
"Optimus HP" was never saved or modified; the throwaway test tab was left
open for Jeff, who was driving the plugin UI himself between my steps.
`SynthEdit` and `SynthEditLib` were read only; `GMPI_Wrappers` read only.

**Branch/PR:** this TideSynth PR (triage doc, U2 split, this entry) +
[gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7) — **the gmpi_ui
one carries the code**; they need not merge together.
