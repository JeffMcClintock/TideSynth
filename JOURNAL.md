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

## 2026-08-17 — macos — TIDE does not save the user's rack (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "keep working, no mercy" after
merging the thumbnails. I went after the smallest remaining follow-up
(`rackMode` on project load) and found something much larger on the way in.
Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** filed **S11** — **TIDE never persists or restores its document, so the
user's rack is lost the moment a project is reloaded.** No code change this
entry: the finding, its evidence, and the mechanism are the deliverable, and
the fix is a real feature that should be scoped deliberately rather than
started at the end of a long session.

**How it surfaced, which is the useful part.** U1c's follow-up asks what
happens to `rackMode` when a project is loaded, since the flag is serialised
(`s("rack_mode", rackMode)` in `SynthEditDocBase.h`). Following D4's lesson I
went to measure rather than reason — and the measurement kept coming back
wrong in a way that only made sense if **nothing loads a document at all.**

**The evidence, in three steps, each one cheap:**

1. **The saved state is 250 bytes of base64** for a project containing a placed
   List Entry in a rack (`GetTrackStateChunk` via ReaScript). A document with a
   module in it cannot fit in 250 bytes.
2. **Decoded, it is two parameters and nothing else** — the `.rpp`'s VST block
   reads `<Preset><Param id="1" val="0"/><Param id="0" val="0"/></Preset>`.
   Those are `controllerPtr` and `chunk`, both zero.
3. **Save → close → reopen → the module is gone.** The reloaded plug-in draws
   an empty rack: rails present (rack mode is set at document creation), no
   List Entry. Verified visually.

**The mechanism, so the row is actionable rather than alarming.** TIDE's XML
already declares the parameter this needs —
`<Parameter id="1" name="chunk" ignorePatchChange="true" datatype="blob"/>` —
and **nothing in the codebase ever writes it or reads it**;
`TideApp::InitInstance` unconditionally does `createNewDocument()` +
`OnNewDocument()`, so every instance starts empty by construction. The
controller's preset system (`MpController` / `DawPreset`) serialises
*parameter values*, which is exactly the two-param XML observed. The document
has its own serialisers already — `CSynthEditDocBase::ExportXml` /
`ImportXml` — so the shape of the fix is: export the document into that blob
parameter on save, import it back and rebuild the view on load.

**Why this is an architecture difference and not an oversight to be ashamed
of.** In a normal SynthEdit-exported plug-in the document IS the product: it
is baked in at export time and the chunk only has to carry knob values. TIDE
inverts that — **the document is what the user edits at runtime** — so it must
ride in the state. Nobody wrote that because nothing before TIDE needed it.
That framing belongs in the row so the next reader does not go looking for a
regression.

**What it means for the release, stated plainly:** the mac NEXT row said this
morning that the board was finished and the remaining question was v0.1. **It
still is, and this is now the answer**: a synthesiser that cannot save its
patch is not shippable, so **S11 blocks the R-series** more concretely than
"there is nothing to ship" did. That is a better problem than it sounds —
the question moved from "what should we build?" to "build this one thing".

**Also settled, and it retires a follow-up:** U1c's `rackMode`-on-load worry is
**moot in the form it was written**. Nothing loads a document, so nothing can
override the flag; the rack survives *because* the document is always fresh.
When S11 lands, the question becomes live again and S11's own work has to
answer it — noted in both rows so the retirement is not silently forgotten.

**Learned — chase the follow-up, find the feature.** The smallest item on the
list was the one that exposed the largest gap, because verifying it required
exercising a path (state round-trip) that no previous session had reason to
touch. **Six sessions of host verification never caught this**: every test
opened a fresh plug-in, and a fresh plug-in looks identical whether or not
persistence exists. The failure is only visible across a save/reload boundary,
which is a class of test worth adding deliberately rather than stumbling into.

**Next:** **S11** is the item, and it is Jeff's call how far to take it — the
row proposes the minimum honest version (round-trip the document through the
existing blob parameter) and lists the questions that need his answer, chiefly
what happens to the DSP graph on restore and whether patch-change should
reload the rack.

**Side effects on this box:** no code changed, nothing rebuilt. REAPER was
driven and a throwaway project was written to `/tmp/tide-persist-test.rpp` as
part of the test; **"Optimus HP" was never opened, saved or modified** — the
test script aborts if it sees that project active, which it checked and
reported.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — crumb thumbnails, and what they cost (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff picked the U1b crumb-thumbnail
follow-up off the list the previous entry left. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** wired the breadcrumb bar's thumbnails —
[SynthEdit#38](https://github.com/JeffMcClintock/SynthEdit/pull/38). **Verified
in REAPER: crumbs render the container's real content, and switching the master
from rack to structure view swaps the tile from the dark case to the light
structure grid with the placed Container visible inside it** — so it is
genuinely rendering the container, not drawing a placeholder.

**There was nothing to invent.** `BreadcrumbBar::renderThumbnail` has always
been the way in, `se_cl::renderContainerThumbnail` has been shared since it was
lifted out of SynthEdit2 ("which made thumbnails a Windows-only feature by
accident rather than by design"), and both the Wayland and WinUI editors
already install the callback. TIDE left it unset and silently got name-only
crumbs. **This is the fourth item in a row where the answer was wiring, not
building** — U1a, U1c, D6's content, and now this.

**The one interface change, and why it is two methods rather than one.**
`ISeApp` grows `setQuiet(bool)` returning the **previous** value. The offscreen
render walks the module factory, whose duplicate-module dialogs must be
suppressed around it; the Wayland version scopes `app_.quiet` directly, but
`ISeApp` exists to firewall SE SDK3 off from the GMPI side, so exposing the
application object to get at one bool would have been the wrong shape.
Returning the previous value means callers restore rather than assume `false`.

**Measured the cost rather than waving at it, because a plug-in pays for every
byte.** TIDE_VST3 went **10,149,744 → 10,414,832 bytes (+265,088, +2.6%)**.
Static-archive extraction did most of what C12e's rule predicts —
`EditorCommandDispatcher` is **not** linked (0 symbols) — **but
`SamplingProfiler` IS pulled in (8 symbols)** through `ScreenshotRenderer`.
That is the finding worth keeping: **the screenshot library is not free of its
tooling, and "only the members you reference" is true transitively, which is
not the same as "only the members you wanted".** If the cost is unwanted the
revert is two lines, and the PR says so.

**Learned — the strongest visual test is a CHANGE, not a picture.** A dark
thumbnail of a dark rack is indistinguishable from a black rectangle, and I
nearly recorded "it renders" on that basis. Switching the same container to its
structure view and watching the tile change to a light grid **containing the
module I had just placed** is proof that content is being rendered per
container and per view flag. Same discipline as yesterday's clipboard sentinel:
make the thing prove it changed, do not photograph it once.

**Next:** three small follow-ups remain from the finished-board list —
`rackMode` on project load (**U1c**), Windows/Linux clipboard for Copy link
(**D6**), and the win box's two **U2e** items — plus the **R-series**, which is
Jeff's call. The mac NEXT row's "do not invent scope" still stands.

**Side effects on this box:** two `TIDE_VST3` builds; the installed plug-in now
draws thumbnails. REAPER restarted once; **"Optimus HP" untouched** (it
reloaded on its own and was left alone). Only `SynthEdit` was committed in.

**Branch/PR:** this TideSynth PR +
[SynthEdit#38](https://github.com/JeffMcClintock/SynthEdit/pull/38).

---

## 2026-08-17 — macos — P10: the dead XML is gone (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged the D3/D6 stack, then
"sync repos, clean up branches, continue". Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** synced all five repos and deleted every merged local branch (only
`main`/`master` and Jeff's own release branches remain; **zero open PRs** in
all four repos at the start), then took **P10** — the item the NEXT row names
for exactly this situation — [SynthEdit#37](https://github.com/JeffMcClintock/SynthEdit/pull/37).
`SynthEditSem/SynthEdit.xml` and its `SynthEdit.rc` resource line are deleted.

**Why the file had to go, restated because it is the trap P5 nearly fell
into:** the file looks like the source of truth — 12 lines, named after the
plug-in, holding `id` and `name` — and **is not what ships**. The live identity
is the embedded XML string literal in `SynthEdit.cpp`'s
`getPluginInformation()`. Editing the `.xml` alone changes nothing at runtime
on any platform: a no-op PR that reviews as correct.

**Re-verified before deleting rather than trusting the row, and the discipline
mattered twice this week.** D4's central measurement turned out false
yesterday, so P10 got the same treatment: only two references exist (the `.rc`
line and an explanatory comment in `SynthEdit.cpp`), and the sole loader sits
inside `#if 0`. **The near-miss worth recording: the loader's first visible
guard is `#if _WIN32` at `MyVstPluginFactory.cpp:472`, which reads as live —
the `#if 0` that kills it is the *enclosing* one at `:462`.** I read the inner
guard first and briefly concluded the row was wrong, exactly as I had concluded
about D4. Checking the enclosing guard settled it in one command. **When a
"this code is dead" claim rests on a preprocessor guard, find the outermost
one, not the nearest.**

**Accept met, both halves:** TIDE_VST3 and SynthEdit_VST3 build on macOS, and
the built binary's identity is byte-identical — `id="SE SynthEdit"
name="TIDE Rack" vendor="TIDE Synth"`, the strings P5 put there.

**The limit this box cannot close, stated rather than glossed:** `.rc` files
are Windows-only, so the deletion is verified *consistent* here but the Windows
resource compile is unexercised. It should be trivially fine — the only line
naming the file goes with the file — but the Windows box is the real check, and
the PR says so.

**Next:** with P10 done, the mac backlog has **no remaining item a scheduled
run should take on its own initiative**. What is left is either Jeff's call
(the R-series, all blocked on there being something to ship) or small
follow-ups already recorded in their rows: crumb thumbnails (U1b), `rackMode`
on project load (U1c), Windows/Linux clipboard for Copy link (D6), and the win
box's two U2e items. **That is a genuinely finished board rather than a tired
one**, and the NEXT row now says so in those words so the next run does not
invent scope to fill the gap.

**Side effects on this box:** two `TIDE_VST3` builds and one `SynthEdit_VST3`
build; the installed plug-in is current. REAPER was not driven this entry.
Only `SynthEdit` was committed in; the other four repos were read only and are
clean on their default branches.

**Branch/PR:** this TideSynth PR +
[SynthEdit#37](https://github.com/JeffMcClintock/SynthEdit/pull/37).

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
