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

---

## 2026-08-17 — macos — U2e: the pin-delivery trace, completed (scheduled run, unattended)

**Prompt:** b3e9876 · claude-opus-5[1m] · app 1.30096.5 · as tide-rack-bot

**Did:** answered U2e's one directed question — *"trace how `ModuleView`'s Sdk3
path delivers initial pin values and why the handler pass is skipped"* — and the
answer corrects the row's own suspicion. Written up in full at
[docs/u2e-pin-delivery-trace.md](docs/u2e-pin-delivery-trace.md). **No code
changed; that was the call, see Next.**

**Result — the handler pass is not skipped, it is never requested.** TIDE's
`Module_Info` for `SE List Entry` has **zero GUI pin descriptions**, and every
initial-value path reads that list, so `setPin` is called zero times.

`Module_Info::gui_plugs` is populated by exactly two mechanisms, and the classic
SDK3 controls use neither:

  1. classic internal DSP — `REGISTER_MODULE_1` + `LIST_PIN2` in C++
     (`UgDatabase.cpp:347`). This is why **Phase Dist Osc drew with its pins**
     last night and List Entry did not.
  2. modern GMPI — `gmpi::Register<T>::withXml(...)` → `RegisterPluginWithXml`
     (`UgDatabase.cpp:267`) → `ScanXml` (`:287`). This is `PlainImageGui.cpp:182`,
     i.e. **SE Background Image**.
  3. SDK3 official module — `GMPI_REGISTER_GUI` (`mp_sdk_gui.h:12`) →
     `RegisterPlugin` (`UgDatabase.cpp:242`), which stores **a constructor and
     nothing else**. Its pins live in `ControlsXp.xml:262-283`, which
     `modules/plugin_helper.cmake:236` copies into the `.sem` bundle for the
     **module scan — the scan S1a deliberately removed** (PLAN constraints 4 & 7).

The chain, every link at file:line, is in the doc. Short form:
`ViewBase::ConnectModules` gates **both** default paths on
`moduleInfo->gui_plugs` (`ViewBase.cpp:711`→`:742`, and `:778`→`:799`) → zero
`setPin` → `ModuleView.cpp:1404`'s `notifyPin` never fires →
`GuiPinOwner::notifyPin` (`mp_sdk_gui.cpp:125`) never reaches `doNotify` →
`ListEntryGui::onSetAppearance` (`ListEntryGui.cpp:49`, the **only**
`widgets.push_back` site) never runs → `widgets` empty at
`initialize`/`measure`/`arrange` — the three SIGSEGVs, in `Refresh`'s own call
order.

**Verification artifact — the shipping binary, A/B with positive controls.**
`master` built clean first (`cmake --build . --config Release --target
TIDE_VST3` → `** BUILD SUCCEEDED **`, 0 errors, universal x86_64+arm64), then:

```
                       id-string   embedded <Plugin id="…"> XML
SE List Entry               2                 0     <- family 3
SE Text Entry               2                 0     <- family 3
SE Background Image         4                 2     <- family 2 (control)
SE Patch Point in           2                 2     <- family 2 (control)
PatchAutomator              4                 2     <- family 2 (control)
"LED Stack" / "Up/Down Select" / "Appearance": 0 / 0 / 0
```

The controls are the point: the same `strings` command finds full pin metadata
for family-2 modules in the same binary, so absence is absence, not a tooling
artifact. `SE List Entry` is **a registered module id with no pins attached**.
Only **29** `<Plugin id=…>` blocks exist in the entire TIDE binary.

**Learned — three things the next run would otherwise redo.**

  - **The U2e row's skin-bitmap/ImageCache suspicion is not the gate** and should
    drop to second hypothesis. Nothing in widget construction is *reached* to
    fail. It also explains why [SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
    (resource-folder seeding) measured as a real prerequisite yet was proven
    insufficient alone: it is a prerequisite for a step the run never gets to.
  - **Pin IDs are fine** and were worth ruling out explicitly:
    `ListEntryGui.cpp:32-40` seeds `initializePin(10, pinValueIn, …)` and lets
    the rest auto-increment to 11-16, matching `ControlsXp.xml:271-281` exactly.
  - **This is a wall in front of TIDE's whole fixed module set, not one broken
    control.** Every family-3 module TIDE adds lands in the same state. S1a's
    trade (no scan → no cache write → sandbox-safe) has this as its unpriced
    cost, and it is now priced.

**What did NOT work / was ruled out by reading rather than guessing:** the
`pluginParameters2B` queryInterface path is sound (`MpGuiBase2` derives from
`IMpUserInterface2B`, `mp_sdk_gui.h:365` answers `MP_IID_GUI_PLUGIN2B`), so the
"host never got a notify-capable interface" theory is dead. `Module_Info::ScanXml`
(`Module_Info3_base.cpp:213`) does **not** call `ClearPlugs()` first — it clears
only `pinXmlDiagnostics_` — which is what makes the tidiest fix risky (below).

**Next:** the fix is a **choice, not an investigation**, and the doc costs three
options. Recommended: embed `ControlsXp.xml` at build time and
`RegisterPluginXml` it from `TideApp::InitInstance` — confined to
`SE16/SynthEditSem/` (ALLOWED), cannot regress the scanning editor. Rejected for
now: swapping ControlsXp to `withXml`, which is tidier but double-populates
`Module_Info` in the scanning editor (map `insert` drops and leaks the second)
and changes commercially-shipped behaviour no macOS box can test.
**Give U2e's remaining half to an interactive session** — its acceptance is *"a
placed List Entry draws as a usable combo box"*, a GUI observable an unattended
run cannot check; and the fix stacks on the still-open #28. The mac NEXT row now
says so. **U1b** is the unattended-safe mac item, with the caveat that its
default-flip half must wait on U2e and would undo the open
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29).

**Process note for Jeff, no row filed.** The run prompt's STEP 5 lists *"the
SynthEditLib repo"* as GATED, while U2e's own row says its scope is *"ALLOWED
(public repo)"* — and precedent agrees with the row (you merged SynthEditLib#12
and #13 for this item). The contradiction did not bite this run, because nothing
was written outside TideSynth, but the next run to attempt the U2e fix will hit
it. Worth one line in whichever of the two is wrong.

**Side effects on this box:** one `TIDE_VST3` Release rebuild from `master` (the
build artifact above); no REAPER, no GUI, no computer-use — scheduled runs cannot
get that approval. All five working copies (`TideSynth`, `SynthEdit`,
`SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`) were **clean at start** and all are
returned to their default branches. Nothing of Jeff's was touched.

**Branch/PR:** `tide/mac/U2e-pin-delivery` → this TideSynth PR. No other repo was
committed in. Three earlier mac PRs remain open, clean and mergeable, and were
deliberately left alone per STEP 1.5:
[SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28),
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29),
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13).

---

## 2026-08-16 — macos — structure view interim: an oscillator draws with its pins (interactive session, Jeff directing)

**Prompt:** n/a — interactive session, sixth of the day; Jeff set the goal
directly: *"first it would be nice to see a basic module drawn in structure
view, like the oscillator … just draws in the structure view with its
pins."* Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** flipped `TideApp::OpenView` to `ContainerViewStruct` /
`CF_STRUCTURE_VIEW` as the **interim default** —
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29) — and
**verified the goal exactly: Phase Dist Osc draws in REAPER as a full
structure-view module box** — title, rounded body, blue input pins (Pitch,
Modulation Depth), green list pins (Wave1, Wave2), Audio Out on the right,
selection chrome, properties pane in sync, placed at the click point.
First module TIDE's structure view has ever drawn in a host on this
platform. `ModuleViewStruct` went **0 → 53 symbols** with the flip (it had
been dead-stripped along with `ContainerViewStruct` since U1a).

**Why this is a flip and not a revert of U1a.** The rack pivot stands;
what changed is sequencing. U2e isolated the panel-view CONTROLS behind
one remaining question (the SDK3 pin-delivery pass never runs), while the
structure view's generic module rendering — box + typed pins, no custom
GUIs, no skins, exactly as Jeff noted — is the decades-proven path and
worked on the first try. So the structure view is the *interim* default
until panel controls land; **U1b's job is unchanged and now easier**: make
the rack the default again with the structure view behind its unlock —
both classes now link, and the `SE2::TopView` typing keeps the flip one
line. The two views' remaining difference is which one `OpenView` names.

**Recorded from Jeff, and it reframes U1c:** *"we already added a basic
rack mode to synthedit. code is there already."* That matches what the
code shows — `ModuleViewPanel`'s JSON ctor reads an `isRackModule` flag,
and `ViewBase::snapToGrid`'s comment describes rack-mode axes ("one HP
across, one whole rack row down — so modules land in real slots"). U1c's
row said "genuinely not written / from-scratch build" — **corrected: U1c
is wiring and enabling existing rack code**, the same shape U1a turned out
to be. Its row now says so.

**The navigation stack earned its keep immediately:** finding the
oscillator meant scrolling the module browser (the wheel fix), and the
centred canvas (U2c) put the placed module exactly where clicked, in
view. Everything from this morning compounds.

**Next:** unchanged in priority, sharper in shape — **U2e's pin-delivery
trace** (usable panel controls), then **U1b** (rack default + unlock,
now trivially two linked classes and chrome), then **U1c** (enable the
existing rack code). The module-browser filter box (type-to-find) would
have made tonight faster; noted as a UX nicety for U1-series work, not
filed as a row.

**Side effects on this box:** one more `TIDE_VST3` rebuild + auto-install
— the installed plugin now opens the structure view; REAPER quit/relaunch
once, "Optimus HP" untouched; throwaway tabs remain. `SynthEdit` working
copy returned to `master` after push.

**Branch/PR:** this TideSynth PR +
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)
(stacks cleanly beside [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
— same file, different functions, merge order irrelevant).

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
