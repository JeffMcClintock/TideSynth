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

## 2026-08-17 — macos — D6: the about pane is built (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "do D6". Committed and pushed
as `tide-rack-bot` (claude-fable-5).

**Did:** **D6** — [SynthEdit#36](https://github.com/JeffMcClintock/SynthEdit/pull/36).
The about pane that D1 and D2 designed exists and works: it opens from the
breadcrumb bar, shows exactly the four specified items, the donation link opens
Ko-fi, **Copy link puts the exact URL on the system clipboard**, and clicking
away dismisses it. **This is the first D-series item that is a running feature
rather than a design note** — D1 and D2 produced the spec, and it survived
contact with implementation essentially unchanged.

**What it looks like:** a rounded panel centred over the editor with a soft
scrim behind it — *TIDE Rack — version 0.1 (unreleased)* in bold, the credit
under it, then the ko-fi URL in link blue with **Copy link** beside it, then
*ISC licence — github.com/JeffMcClintock/TideSynth*. An **X** at the corner and
click-away both dismiss.

**Every rule in the spec is kept, and the code says so where it matters.**
`AboutPane.h` restates the six rules at the top, next to the code that has to
keep them, because they are the kind of thing a later reader deletes by
accident: nothing unprompted (**the only way in is a plain "About" text
affordance** at the right end of the breadcrumb strip — no badge, no dot, no
splash); never a dialog or a second window; nothing blocking audio; no image
assets; the donation line degrades to text but never to nothing; **exactly four
items, and a fifth needs a ruling.**

**The one design decision I had to make, and it is rule 5 taken literally.**
"Copy link" needs a clipboard write, and GMPI has no clipboard abstraction —
only `KeyListenerCallback`'s cut/copy hooks, which are for text fields. So the
pane asks `tide::clipboardAvailable()` and **omits the button entirely where the
answer is no**, rather than drawing one that silently does nothing. Apple gets
`NSPasteboard`/`UIPasteboard` (split on `TARGET_OS_OSX`, the same pattern D3
just established); Windows and Linux get a stub returning false, with a comment
naming the reachable APIs for whoever wires them. **A dead button would have
broken the very rule the button exists to serve.**

**Where it lives, and why not in the shared bar.** The pane is TIDE's alone, so
it is in `SynthEditSem` (ALLOWED); `SE2::BreadcrumbBar` is shared with every
SynthEdit frontend and gains nothing TIDE-specific. The affordance is drawn by
`SynthEditGui` over the strip's right end, which also keeps the bar's own
hit-testing untouched.

**Learned — verify a clipboard by reading it back, not by watching the label
change.** The button flips to "Copied" on its own return value, which is
exactly the kind of self-report that can be true while the write failed. I
primed the system clipboard with a sentinel string via `pbcopy`, clicked Copy
link, and checked `pbpaste`: the sentinel was gone and the URL was there.
**That is a one-line check that turns "the UI said it worked" into evidence**,
and it is available to any mac session.

**Caught in my own first build:** the pane rendered with no Copy button at all —
`copyOffered` defaulted to `false` and I never wired it. The screenshot looked
fine (four items, all correct), which is precisely why the row's Accept lists
the button separately. Fixed by initialising it from `clipboardAvailable()`.

**Next:** the D-series is now exhausted — D1/D2 (design) and D6 (build) landed,
D3 is IN-REVIEW, D4 is WONTFIX, D5 was Jeff's. With constraint 1 delivered
(U1a/U1b/U1c) and the about pane built, **the plug-in has its shape**. The
honest next question is not another feature but **what v0.1 needs**, which is
the R-series' territory (R2–R6, all blocked on there being something to ship) —
and the version line in this pane will be the first thing that has to stop
saying "unreleased". The win box still owes U2e's two follow-ups.

**Side effects on this box:** three `TIDE_VST3` rebuilds; the installed
plug-in now has the about pane. REAPER restarted twice; **"Optimus HP" was
never saved or modified**. The system clipboard was overwritten twice as part
of the test (sentinel, then the ko-fi URL) — it now holds
`https://ko-fi.com/tiderack`, which is worth saying because it is Jeff's
clipboard.

**Branch/PR:** this TideSynth PR +
[SynthEdit#36](https://github.com/JeffMcClintock/SynthEdit/pull/36).

---

## 2026-08-17 — macos — D3 done, D4 refuted by measurement, U1 closed (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff asked me to merge the U1b/U1c
stack, sync, and "do the D-series". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** merged [SynthEdit#33](https://github.com/JeffMcClintock/SynthEdit/pull/33)
and [#34](https://github.com/JeffMcClintock/SynthEdit/pull/34) at Jeff's
request (so **U1a+U1b+U1c are all on `master`** and **U1 itself closes**),
synced all five repos, then took the D-series: **D3 is done**
([gmpi_ui#9](https://github.com/JeffMcClintock/gmpi_ui/pull/9) +
[SynthEdit#35](https://github.com/JeffMcClintock/SynthEdit/pull/35)) and
**D4 is WONTFIX — its central measurement is now false, and acting on it
would have broken the build.**

**D4 first, because it is the finding.** The row says *"grepping SynthEdit,
SynthEditLib, gmpi_ui and GMPI_Wrappers finds **zero** call sites for
`gmpi::browse_to`"* and concludes the file can be dropped *"at no functional
cost"*. Re-measured today: **two live call sites** —
`SynthEditLib/SkinMgr.cpp:111` (`SkinMgr::EditSkin`, the "open this skin's
folder" command) and `SynthEditLib/MfcDocPresenter.cpp:1106` (the
skin-folder context command). Deleting `browseto.mm` would have produced an
undefined symbol, not a saving. The row was filed 2026-08-16; the C-series
carve-out has been moving files into `SynthEditLib` throughout, so the
likeliest explanation is that the callers arrived with a move after the grep
ran. **The lesson is the cheap one: re-run a row's own measurement before
acting on its conclusion, especially a "delete this, nothing uses it" row in
a tree that is being actively carved up.** I ran the positive control too
(`gmpi::open_url`, 5+ call sites) so a zero would have been distinguishable
from a broken grep.

**D4's *intent* is nonetheless delivered — by D3.** Its real goal was
removing an AppKit dependency and a sandbox-hostile API
(`activateFileViewerSelectingURLs:`, i.e. reveal-in-Finder) from Apple builds
that should not have them. D3's split does exactly that for the platform
where it matters: `browse_to` compiles to a deliberate no-op off macOS. On
macOS it stays, because it is used. So D4 is WONTFIX with the goal met
elsewhere rather than dropped.

**D3, and why the fix is not where the row put it.** The row proposed making
`EditorLib/CMakeLists.txt`'s `if(APPLE)` block iOS-excluding. That alone
would only convert a **compile** error into a **link** error: the headers
dispatch on `__APPLE__` — true on iOS — so the call sites still reference
`browse_to_impl`/`open_url_impl`. **The split belongs in the `.mm` files**,
which now choose their framework internally on `TARGET_OS_OSX`: `open_url`
uses `NSWorkspace` on macOS and `UIApplication openURL:options:completionHandler:`
on iOS; `browse_to` is macOS-only behaviour and a no-op elsewhere. The CMake
change is then just the framework line — **AppKit is macOS-only, iOS wants
UIKit**; CoreText, CoreFoundation and UniformTypeIdentifiers exist on both.

**Verified, and the limit stated:** TIDE_VST3, SynthEdit_VST3 **and**
SynthEditCL all build on macOS (that was D4's own Accept, reused here as the
regression check). **The iOS side is unverifiable on this box** — no iOS
target exists (S10) — so this removes the known compile blocker rather than
proving an iOS build succeeds. Said that way in the row and both PRs.

**Learned — a codesign failure on SynthEditCL can be stale-bundle detritus,
and my first A/B was not controlled.** SynthEditCL failed with P6's exact
string (*"code object is not signed at all … Contents/MacOS/Resources/
Prefabs/Button Small2.syntheditprefab"*). My first check stashed the change
**and** deleted the .app, so a pass proved nothing about which variable
mattered. Re-run properly — change applied, fresh bundle — it **builds
clean**: the failure was a stale bundle carrying resources under
`Contents/MacOS/`, not P6 regressing and not my edit. **`rm -rf` the .app
before believing a codesign failure on that target**, and change one variable
at a time even when the first answer is the one you wanted.

**Bookkeeping done in the same pass:** U1b and U1c flip **DONE** (their PRs
merged this session) and **U1 itself flips DONE and archives** — its three
children have all landed, which is what its row was waiting for. Constraint
1 is now delivered end to end: rack by default, structure view behind an
unlock, breadcrumb navigation, and modules that bolt to rack rows.

**Next:** the D-series is exhausted for now — D1/D2 landed 2026-08-16, D3 is
IN-REVIEW, D4 is WONTFIX, **D5 is Jeff's Ko-fi account** (done). The about
pane that D1/D2 designed is the natural next build: it now has the
breadcrumb bar to hang from, and [docs/about-pane.md](docs/about-pane.md)
fixes its contents to exactly four items. It needs a row of its own — filed
as **D6**. The win box still owes U2e's two follow-ups.

**Side effects on this box:** three products rebuilt (TIDE_VST3,
SynthEdit_VST3, SynthEditCL — the last twice, once from a fresh bundle);
`SynthEditCL.app` was deleted and rebuilt in the build tree. REAPER was not
driven this session. `gmpi_ui` and `SynthEdit` each carry one commit on a PR
branch; `SynthEditLib`, `TideSynth` and `GMPI_Wrappers` were read only.

**Branch/PR:** this TideSynth PR +
[gmpi_ui#9](https://github.com/JeffMcClintock/gmpi_ui/pull/9) +
[SynthEdit#35](https://github.com/JeffMcClintock/SynthEdit/pull/35) — **the
two code PRs must merge together**: the CMake one alone changes nothing, the
helper one alone leaves iOS linking AppKit.

---

## 2026-08-17 — macos — U1c: rack mode on, modules bolt to the rails (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "do U1c". Committed and pushed
as `tide-rack-bot` (claude-fable-5).

**Did:** **U1c** — [SynthEdit#34](https://github.com/JeffMcClintock/SynthEdit/pull/34),
stacked on [#33](https://github.com/JeffMcClintock/SynthEdit/pull/33).
**Verified in REAPER: TIDE renders a real Eurorack case — dark interior,
bevelled aluminium rails, threaded mounting holes at every HP — and a dragged
module snaps onto a rack row instead of staying where it was dropped.**
Constraint 1's rack is now what the plug-in actually looks like.

**It was one line, because Jeff had already built the rack.** Everything sits
behind `Document()->rackMode`, and **the only thing missing was a way to turn
it on in a TIDE build**: the flag is a per-project setting whose sole toggle
lives in a `#if defined(_DEBUG)` context menu, so a Release TIDE could never
reach it. `TideApp::InitInstance` now sets it at document creation, because
TIDE *is* the rack rather than a project that opts in. What that unlocks, all
pre-existing: `MfcDocPresenter::getRackLayout()` enables **only for the
top-level panel view** (sub-panels and every structure view keep ordinary
layout — exactly what U1b's second depth needs); `ViewBase::snapToGrid()`
switches from the square snap to **one HP across, one rack row down**; and
`TopView::renderRack()` draws the case. **The row's original "the only part
of U1c that is a from-scratch build" was wrong in the same direction U1a's
and U1b's estimates were** — this is the third time the honest answer was
"the code is there, wire it up", and Jeff said so twice before the code
confirmed it.

**Learned — verify against the branch that has the prerequisite, not against
`master`.** The first build showed no rack at all. Probes proved
`ContainerViewPanel::render` was never called, then that `getRackLayout()`
was never called — mystifying until the cause turned out to be **my own
staging**: I branched U1c off `master`, where **U1b's rack-as-default
(#33) is still an open PR**, so the master container still opened as the
*structure* view and the panel-view rack path was unreachable by
construction. Rebasing onto `tide/mac/U1b-rack-default` made it render on the
first try. **The tell was that two independent probes both showed "never
called" — that pattern means the code is not on the path, so check what you
are running before you debug what you wrote.** Both probes are reverted.

**Learned — a rebase can silently put someone else's commit on your branch,
and the authorship check is what catches it.** Rebasing onto the U1b branch
replayed Jeff's `dbghelp` fix (already on `master` as `85cd689a0`) as a new
SHA on my topic branch, so the push carried a commit not authored by the bot.
`scripts/check-commit-authorship.py` flagged it — **exactly the class of thing
A14 exists for, caught by the tool rather than by luck.** Fixed with
`git rebase --onto` to drop the duplicate, then `--force-with-lease` on my own
just-pushed topic branch (PR #34 was a minute old, nothing else built on it;
Jeff's original commit on `master` was never touched). Stated plainly here
because it is a rewrite of a pushed ref, same as the D1 precedent.

**Known follow-up, not blocking, recorded rather than pre-solved:**
`rackMode` is persisted per project (`s("rack_mode", rackMode)`), so a patch
authored in full SynthEdit *without* rack mode could load into TIDE with the
rack off. TIDE forces it on at document creation; if project load overrides
that, forcing it after load is the fix.

**Next:** with U1a/U1b/U1c landed, **constraint 1 is substantially done** —
rack by default, structure view behind an unlock, breadcrumb navigation, and
modules that bolt to rails. The natural next work is the **D-series** (the
about pane now has the breadcrumb bar to hang from) and **U1**'s own row,
which can finally be closed once U1a–U1c merge. The win box still owes U2e's
two follow-ups.

**Side effects on this box:** five `TIDE_VST3` rebuilds; the installed
plug-in now opens as a rack case. REAPER restarted three times — one restart
raced a scripted quit and left a "save unsaved project?" prompt, answered
**No** for a throwaway tab; **"Optimus HP" was never saved or modified**, and
REAPER's own reload of it (with its pre-existing missing-plug-in warning) was
dismissed untouched. Temporary probes in `SynthEditLib` were local-only and
are reverted; that repo is clean.

**Branch/PR:** this TideSynth PR +
[SynthEdit#34](https://github.com/JeffMcClintock/SynthEdit/pull/34) (stacked
on [#33](https://github.com/JeffMcClintock/SynthEdit/pull/33) — merge that
first, or both together).

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
