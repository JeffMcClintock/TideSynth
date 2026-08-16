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

---

## 2026-08-16 — windows — U1a·P5·U2 host verification, Windows half (interactive session, Jeff present)

**Prompt:** n/a — interactive session, not a scheduled run. Jeff at the
keyboard; Claude (claude-opus-5) drove REAPER by computer use; committed and
pushed as `tide-rack-bot`.

**Did:** the Windows half of the verification the mac session finished earlier
today. Rebuilt `TIDE_VST3` from `origin/master`, cleared REAPER's VST cache,
re-scanned, and loaded the rack in a new empty project. **The point was never
to re-close U1a or P5** — both were already closed on mac evidence, and this
run does not touch their status. It was to find out **which of U2's four
first-render defects belong to the panel view and which belong to macOS**,
because U2's own text proposes a pairing that Windows disproves.

**Result — the rack renders on Windows too.** REAPER 7.78/win64 rev 608e49
(Jul 18 2026), x64. No crash through instantiate → UI open → insert → select →
window resize → remove → re-instantiate. Category tree, module list, gridded
panel canvas and properties pane all draw, and the properties pane populates
correctly on selection (List Entry: pins, parameters, Appearance=Combo Box —
the same cell the mac session read).

**Result — U2 is three-quarters cross-platform, and its own hypothesis is
wrong.** The row guesses *"(1)/(2) smell like one event-routing cause"*.
Windows splits that pair:

| U2 | mac | windows |
|---|---|---|
| (1) drag-drop from the module browser places nothing | fails | **reproduces** |
| (2) scroll wheel dead everywhere in the UI | fails | **works fine** |
| (3) a placed control draws at the wrong size | ~10 px glyph stack | **reproduces, worse** |
| (4) §6 canvas offset/dead-strip, re-anchors on resize | fails | **reproduces** |

So **(1), (3) and (4) are the panel view's own defects and (2) is macOS-only**,
and (1) and (2) cannot be one cause. Detail worth having before anyone opens
these: **(1)** a stepped-slow synthetic drag of both `Moog Filter` and
`List Entry` highlights the row in the browser, shows no drag ghost, and drops
nothing — while double-click inserts fine, the same split the mac saw, so it is
the drop path on both platforms. **(2)** the wheel scrolls the module list and
the canvas on real hardware. **(3)** on Windows the control does not draw at
all: only its selection/resize adorner draws, **collapsed onto a zero-size rect
at the canvas origin**, with every subsequent module landing on the same point.
Jeff identified the artifact at the keyboard — a blue outline with white circle
resize nodes, which is what proves the module *is* inserted and selected rather
than missing. `Text Entry` behaves identically. So mac's "~10 px" and Windows'
"zero" are the same defect at two magnitudes: the model is right and the panel
geometry is not. **(4)** the canvas is anchored to the **right and bottom** of
its pane with dead grey filling the top and left, and on a window resize it
translates with the right edge rather than reflowing.

**Result — P5's Windows half, with the UID evidence mac could not get.**
Windows showed the *same original symptom* first: before the re-scan,
`%APPDATA%\REAPER\reaper-vstplugins64.ini` read
`TIDE_VST3.vst3=6346B150292DDD01,741344739{67756C506E694D47504920501951ED43,SynthEdit (GMPI)!!!VSTi`.
After a clear-cache re-scan of all 153 plug-ins (the dialog confirmed
**+0 cached**) the same line reads `...,TIDE Rack (TIDE Synth)!!!VSTi`. **The
class UID `741344739{67756C506E694D47504920501951ED43` is byte-identical
across that change** — a direct, measured confirmation that leaving the XML id
`SE SynthEdit` alone kept the hashed VST3 class UID stable, so no saved host
project is orphaned. That is the one thing P5's row most feared and it had
never been observed; the mac could not observe it because its ini never
rewrote (see below). FX browser: **`VST3i: TIDE Rack (TIDE Synth)`**. Via
ReaScript, `TrackFX_AddByName(tr, "TIDE_VST3", false, -1)` → **-1**, exactly as
on mac and for the same reason; `"TIDE Rack"` → 0, `"VST3i: TIDE Rack (TIDE
Synth)"` → 1 and bare `"TIDE"` → 2, all three reporting the full name.
`EnumInstalledFX` over 354 installed FX returns exactly one TIDE entry, ident
`C:\Program Files\Common Files\VST3\TIDE_VST3.vst3`.

**Learned — the mac entry's cache-flush rule is macOS's, not REAPER's.** That
entry states, as a general REAPER fact, that the plug-in cache ini *"flushes on
exit, not on scan"*. On Windows/7.78 it flushed **at scan time**:
`reaper-vstplugins64.ini` was rewritten with the new identity while REAPER was
still running, which is what made the UID A/B above possible. Not edited there
— that entry is the record of what that box saw, the same reasoning A9 used for
the process review. Read it as platform-specific, and on Windows the file is
trustworthy mid-session.

**Learned — building `TIDE_VST3` alone ships a plug-in that cannot build its
DSP graph. Filed as P11.** On Windows the VST3 resolves its built-in `SE *` GUI
modules through the *installed module database*, whose TIDE entry is
`C:\Program Files\Common Files\SynthEdit\modules\TIDE.gmpi` — written by the
separate `TIDE` target, not by `TIDE_VST3`. With a stale `TIDE.gmpi` there, the
plug-in threw **"Export failed: required module is missing from the module
database"** naming `SE Background Image` at instantiate and `SE List Entry` /
`SE Text Entry` on each GUI insert, while DSP-only modules (`Moog Filter`)
exported clean — the tell that isolates it to the GUI half. Building the `TIDE`
target as well cleared every one of them. **The error blames the user's
install** (*"this installation is broken. Re-scan modules"*) for what is
actually a half-built tree, which is why this is worth a row rather than a
footnote. **U2's (3) survives the fix** — re-tested with a consistent database
and the control still draws as a zero-size rect, so the geometry defect and
this trap are independent.

**Next:** unchanged — **win NEXT stays C12c**; this run was verification, not a
claim on a work item. **U2 is now the triage-ready row** its Accept asks for on
three of four defects, and whoever takes it should start from (3)/(4) as one
geometry cause with (1) separate — not from U2's original (1)+(2) pairing.
**Note U2's `Plat` cell still reads `mac` and now understates the row**: the
BACKLOG lint ([scripts/check-backlog-diff.py](scripts/check-backlog-diff.py))
forbids a run changing `Plat` on an existing row, so that cell needs Jeff or a
deliberate human edit; the Item text carries the correction meanwhile. **P11**
is new, `any`, and small.

**Side effects on this box:** REAPER's VST cache cleared and re-scanned (153
plug-ins; the ini rewrote in place, see above). `TIDE_VST3.vst3` and
`TIDE.gmpi` in `C:\Program Files\Common Files\` are now current Release builds
rather than the stale 2:43 pm ones. A throwaway unsaved REAPER project with a
TIDE instance was left open for Jeff; no saved project was opened or modified.
`SE16` is on the pre-existing local branch `fix/synthedit2-dbghelp-link` and
was not committed to; TideSynth is the only repo committed in.

**Branch/PR:** this PR (TideSynth only — no code changed anywhere; the code
this verifies already landed as
[SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25) and
[#24](https://github.com/JeffMcClintock/SynthEdit/pull/24)).

---

## 2026-08-16 — macos — U1a·P5 host verification (interactive session, Jeff present)

**Prompt:** n/a — interactive session, not a scheduled run. Jeff at the
keyboard; Claude (claude-fable-5) drove REAPER by computer use; committed and
pushed as `tide-rack-bot`.

**Did:** the one observation the last two entries said mattered more than any
backlog item — **loaded the rebuilt `TIDE_VST3.vst3` in a host and looked at
it.** REAPER 7.45/macOS-arm64: cleared the VST cache and re-scanned all 93
plug-ins (progress dialog confirmed **+0 cached**), then in a NEW project tab
("Optimus HP" untouched throughout, per this session's own rule) inserted the
plug-in and opened its UI. That one sitting closed **P5**'s outstanding host
check and **U1a**'s bar (b), filed **U2**, unblocked **U1b**/**U1c**, and
re-pointed mac NEXT at U1b.

**Result — P5, now closed end to end.** The FX browser lists exactly
**`VST3i: TIDE Rack (TIDE Synth)`** — the strings P5 put in the binary,
finally observed in the host that motivated the row. The API half, run via
ReaScript (see Learned): `TrackFX_AddByName(tr, "TIDE Rack", false, -1)` →
**0**, and `"VST3i: TIDE Rack (TIDE Synth)"` → **1**, both instances
reporting the full name. **The row's literal cited call,
`TrackFX_AddByName(tr, "TIDE_VST3", ...)`, still returns -1 — and always
will**: that API matches display names, not bundle filenames, so the call was
only ever a proxy for "unfindable by name", and the thing it proxied is
fixed. Recorded in P5's archived row rather than left as a loose end.

**Result — U1a bar (b): the rack RENDERS.** No crash through instantiate, UI
open, module insert, selection, and a window resize. What draws: the module
browser (categories + list, working), the panel canvas with its grid, and the
properties pane, which populates correctly on selection (List Entry: pins,
parameters, Appearance=Combo Box). With
[SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25) and
[#76](https://github.com/JeffMcClintock/TideSynth/pull/76) both merged and
bar (b) met, **U1a is DONE and archived**; U1b and U1c flip to TODO.

**The bugs bar (b) promised to surface exist, and they are U2** — four, exact
symptoms in the row: (1) **drag-and-drop from the module browser places
nothing** — two synthetic drag profiles and Jeff's real mouse all failed;
double-click inserts fine, so it is the drop path, not insertion; (2) **the
scroll wheel is dead** everywhere in the plugin UI (real hardware); (3) a
placed **List Entry control draws as a ~10 px glyph stack**, not a usable
combo box, while its properties pane is fully correct — model right, panel
geometry wrong; (4) **the §6 canvas offset/dead-strip layout survives in the
panel view** and re-anchors oddly on resize. Moog Filter showing no panel is
correct panel-view semantics, not a fifth defect. A crash was the feared
outcome; the actual outcome — a rendering view whose input/geometry layer has
simply never been exercised — is cheaper than that, and it is exactly the
costing input U1c was waiting on.

**Learned — REAPER's plug-in cache ini flushes on exit, not on scan.** After
a completed clear-cache re-scan, `reaper-vstplugins_arm64.ini` on disk stayed
byte-identical (mtime included) while the FX browser and `TrackFX_AddByName`
both showed the new identity. Anyone re-checking P5's "cached symptom" from a
shell while REAPER is running will read the stale line and wrongly conclude
the re-scan failed. While the app lives, the in-app browser is the truth, not
the file.

**Learned — `REAPER -nonewinst <script.lua>` runs a ReaScript inside the
already-running instance**, no screen control needed. The AddByName numbers
above came from a script injected that way; it guarded against the wrong
project being active (abort if the active project path contains "optimus")
and deleted its own scratch track afterwards. That is the pattern for any
future agent needing REAPER API answers on a box where REAPER is already
open.

**Learned — the a2 doc's macOS caveat is settled.** All five repos on this
box answer `https://github.com/...` to `git ls-remote --get-url origin`, the
global `insteadOf` rewrite is present, and the credential helper chain is
`gh`'s — so
[docs/a2-actor-separation.md](docs/a2-actor-separation.md)'s "macOS remotes
have still never been inspected" is now answered, on the record here. Not
edited there — that file is a dated record, the same reasoning as A9's
non-edit of the process review.

**Next:** **U1b** (mac NEXT re-pointed): breadcrumb bar plus the
structure-view unlock path. **Read U2 before starting it** — the breadcrumb
lands in the same view whose input layer U2 describes, and U1c stays uncosted
until U2 is triaged. **P10** remains the cheap fallback.

**Side effects on this box:** REAPER's VST cache cleared and re-scanned (93
plug-ins; in-memory — the ini rewrites when REAPER exits). A throwaway
unsaved project tab was left open in REAPER for Jeff to play with; "Optimus
HP" was never saved or modified. TideSynth is the only repo committed in;
`SynthEdit`, `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were read only and
left clean.

**Branch/PR:** this PR (TideSynth only — no code changed anywhere; the code
already landed as
[SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)).

---

## 2026-08-16 — macos — U1a

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **U1a** — `TideApp::OpenView` now constructs `SE2::ContainerViewPanel`
with `CF_PANEL_VIEW`. **TIDE opens the rack/panel view.** Sixth item this
session, at Jeff's direction.

**Result — the symbol A/B, which is bar (a) and is the whole verifiable claim:**

| | before | after |
|---|---|---|
| `ContainerViewPanel` | **0** | **13** |
| `ContainerViewStruct` | 15 | **0** |
| `ModuleViewPanel` | 25 | 25 |
| bogus name (control) | 0 | 0 |

Release build **BUILD SUCCEEDED, 0 errors**, universal x86_64 + arm64, and P5's
identity strings verified un-regressed in the same binary.

**Learned — the interesting number is the one that went to zero, not the one
that went to thirteen.** `ContainerViewStruct` is now at **0 symbols**: nothing
constructs it, so **the structure view is currently unreachable**. Constraint 1
asks for *two* depths — rack by default, structure view on unlock — so this
change makes the rack default *and* removes the other depth. That is a real
regression against constraint 1 taken as a whole, not a tidy half-step, and I
have written it into **U1b**'s row: that item is now "breadcrumb bar **and** an
unlock path that constructs the structure view", which is more than it was
scoped as an hour ago. **Nobody would have noticed this from the diff** — it
only shows up because the audit had established the before-numbers.

**Learned — it was four files, and typing the interface to the base is what
makes it the last time.** `ISeApp` was typed to the concrete
`ContainerViewStruct`, threaded through `TideApp.h`, `TideAppWrapper.h` and
`SynthEditGui.cpp`. It is now `SE2::TopView`. Every member `SynthEditGui.cpp`
calls on the view — `arrange`, `Presenter`, `getCenter`, `DragNewModule`, the
scrollbar callbacks — is a base member, checked before the change rather than
discovered by the compiler. **The one thing the compiler did catch** was a
forward declaration: `TideAppWrapper.h` declared `class ContainerViewStruct;`
rather than including anything, so the first build failed with 17 errors that
all cascaded from `no type named 'TopView' in namespace 'SE2'`. One line.

**What is NOT done, and it is bar (b) of this row's own Accept.** *"Draws
something sane in a host"* — **the plug-in has not been loaded in a DAW.** The
class links and the binary builds; whether the rack renders, renders blank, or
crashes is unknown. U1a's row warned that a crash here would be information
rather than failure; that warning is still unspent, because nobody has looked.

**Next — and the most useful next action is not a backlog item.** **Load the
rebuilt `TIDE.gmpi` in a DAW.** One observation closes U1a's bar (b), closes
P5's outstanding "REAPER shows TIDE Rack" check, and unblocks U1b and U1c, which
should *stay* blocked until then — taking either now means building on a view
nobody has seen render. The mac NEXT row therefore points at **P10** (minutes,
ALLOWED, deletes the dead `SynthEdit.xml` that nearly caused a no-op PR during
P5) rather than at more rack work.

**STEP 1 / 1.5:** no `platform:mac` issues. [SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)
is this run's own and is the only open PR.

**Side effects on this box:** `SynthEdit/build/` rebuilt again (Release,
target `TIDE`). Committed in two repos: `SynthEdit` (the change) and `TideSynth`
(this entry and the backlog). `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were
read only and left clean.

**Branch/PR:** [SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)
— **that one carries the change**; the TideSynth PR is bookkeeping and they need
not merge together.
