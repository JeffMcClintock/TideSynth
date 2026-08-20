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
2. Stop when this file is **under 60 KB**, or when the floor is reached —
   whichever comes first. **The floor is the LATER of: the four most recent
   entries, or every entry carrying the most recent date.** The floor always
   wins; a busy day pushing this file over 60 KB is correct, not a rotation
   failure.
3. Never edit an entry while archiving it. The archive is the record.

**Why a date and not a duration (A24, 2026-08-20).** A24 asked for a time-based
floor — *"retain everything from the last 7 days"* — and measuring what that
costs is what killed it. Entries per day, counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it, so 7 days is **3.4× the 192 KB that
triggered A8 in the first place** — the remedy would have been twenty times more
expensive than the problem. Even two days is worse than the state A8 was created
to fix.

So the floor is **one date**, which bounds the cost at roughly a day's work while
guaranteeing a run can always see everything that happened most recently — the
failure A24 correctly identified, where a 4-entry floor at ten entries a day
bought under half a day. On a quiet week the four-entry floor still binds and
nothing changes.

**What this does NOT fix, filed as A30:** the durable lessons still age out.
Rotation moves an entry's *"Learned"* bullets into the archive with it, and no
run reads the archive. The cheap answer is a standing digest that never rotates;
the expensive one is reading 651 KB.

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

## 2026-08-21 — macos — E5: the rack grid and TIDE's own faceplate disagree, so nothing on the rack can pass

**Prompt:** f7ae1a4 · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, unattended

**Did:** measured E5's Accept on a running TIDE, found it fails for **every**
module on the rack, and split it into two clauses with two different owners.
Shipped [tests/e5_rack_footprint_probe.py](tests/e5_rack_footprint_probe.py)
so nobody has to measure it again.

### The measurement

`TIDE_STANDALONE` built from this branch, run on an isolated `HOME` (S25's
protocol — the session-restore trap at `Library/Application Support/TIDE Rack/`
is real), driven over its command channel, and read back out of the saved
document rather than off a screenshot:

```
python3 tests/e5_rack_footprint_probe.py tests/fixtures/e5-rack-macos-2026-08-21.xml
  FAIL List Entry            w=100 h=38    6.667 HP  OFF-GRID  fits row
  FAIL Container "TIDE MIDI-CV"  w=96 h=380  6.400 HP  OFF-GRID  TALLER THAN ROW
  FAIL overlap 1368 sq: List Entry x List Entry
RESULT: FAIL -- 7 violation(s)                                   rc=1
```

**The headline is the second row, not the first.** E5 is written as though the
problem were stock GUI modules. **TIDE's own shipped prefab fails the same
clause** — 96 DIPs is 6.400 HP — and it is 380 tall against a row's usable 350,
so it sits *on* the rails ([baseline](docs/images/e5-rack-baseline-macos.png)).
Two modules click-placed 60 DIPs apart overlap by 1368 square units, seen as
well as computed ([screenshot](docs/images/e5-overlap-macos.png)).

### Why nothing can pass: two constants that disagree

| | |
|---|---|
| `RackLayout::hpWidth` | **15 DIPs** = 1 HP (`SynthEditLib/…/Presenter.h:40`) |
| `SE TiDE:Panel` rack unit | **48 DIPs** (`modules/TiDEPanel/TiDEPanelGui.cpp:227`) |
| 48 / 15 | **3.2 HP** — not a module width that exists |

So a whole number of rack units is never a whole number of HP. Filed as a
`PROPOSED:` entry recommending 45 DIPs, because 15 is the **externally
anchored** number — E17 verified it against VCV's `rack.hpp` (1 HP = 0.2 in at
75 dpi) — while 48 is ours. **This is the E17 ruling's own flagged tweak**
(*"one rack unit is 3.2 HP rather than a whole HP"*), and
`panel-design-language.md` says moving it rescales every coordinate in every
layout string, so it is explicitly *"one deliberate edit"* and not something a
scheduled run should do on the way past.

### Nothing quantises a module's SIZE, anywhere

The thing E5 asks for does not exist for any module, by any path — established
by reading rather than inferred from the failure:

- `ViewBase::snapToGrid` (`ViewBase.cpp:1373`) takes a **Point** and returns a
  Point. Position only; there is no size in its signature.
- Its only two call sites are the module drag (`ViewBase.cpp:349`) and the
  resize adorner (`ResizeAdorner.cpp:393,422`). **Insert calls neither.**
- Every other use of `rack.hpWidth` / `rack.rowHeight` in the whole repo is
  `renderRack` drawing rails and holes (`ViewBase.cpp:1413-1461`). Grepped in
  full; there are no others.

So clause 1 (whole HP) is a **ruling**, and clause 2 (no overlap) is **GATED
`SynthEditLib`** — there is no placement or collision policy to extend, and it
is not a build break, so A17's exception does not reach it.

**Kept as ONE row on purpose.** A31 was filed the day before for *two ids, one
job*; opening a second row for the gated half would have been precisely that.

### The probe has both arms, and the selftest is proven able to fail

`--selftest` is 6 cases, 0 failed — three that must PASS (on-grid pair, zero
rect skipped, a Container read by `PanelWndPosition` and not by its inner
`panelRect`) and three that must FAIL (off-grid width, taller than a row,
overlapping pair). Flipping one expectation turns it red, so the suite is not
passing vacuously. It reads the grid from `Presenter.h`'s numbers and answers
from the *rack's* point of view, the S21 discipline.

### Two things this could not settle

1. **Drag is a no-op on the locked rack.** Three attempts moved nothing while
   insert worked in the same session, so a user cannot rearrange what they
   placed until they unlock — which is *why* clause 2 bites rather than being
   a cosmetic complaint. Recorded on the row, unclaimed.
2. **Why insert bypasses the grid on my path but Jeff saw different
   coordinates on linux (S26)** is untouched here.

**Learned:**

1. **When an Accept names one class of thing, check whether the thing you
   already ship passes it.** E5 reads as a defect in *stock* GUI modules;
   TIDE's own prefab fails identically, and that single extra row of the probe
   changed the item from "give modules a default size" into "two constants
   disagree". The narrow reading would have produced a fix that still failed.
2. **A grid constant with an external anchor and one without are not
   symmetrical candidates for moving.** 15 DIPs is 0.2 inches of real
   Eurorack; 48 is a number TIDE chose. That asymmetry decides the
   recommendation on its own, without any taste being involved.
3. **"It snaps to a grid" and "it is a whole number of grid units" are
   different mechanisms**, and this tree has the first and not the second.
   `snapToGrid` returning a `Point` is the whole proof — a size quantiser
   cannot hide behind that signature.
4. **The standalone's session file is a better instrument than its
   screenshot.** The overlap is visible in the image and is *1368 square
   units* in the document; only one of those is a number a later run can
   compare against.

**Next:**

1. **Jeff: merge or edit the `PROPOSED:` entry.** Clause 1 is one constant plus
   a prefab regeneration once ruled.
2. **Clause 2 wants `SynthEditLib` authority**, like C10 and E10.
3. Unchanged and still the fleet's bottleneck: **the apt-get in `build.yml`**
   ([#189](https://github.com/JeffMcClintock/TideSynth/issues/189)) — every one
   of the ten open mac PRs carries a red `linux` check that belongs to it.

**Branch/PR:** `tide/mac/E5-rack-default-size` — TideSynth only, **stacked on
[#236](https://github.com/JeffMcClintock/TideSynth/pull/236)** and to be merged
after it.

---

## 2026-08-20 — macos — E16 becomes a PROPOSED entry, and the takeable queue runs dry behind it

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · ninth item

**Did:** filed the E16 ruling request — a `PROPOSED:` entry in
[docs/decisions.md](docs/decisions.md) asking which
[docs/module-set.md](docs/module-set.md) tier E2 authors the curated set from,
recommending the 12-module playable tier (smallest set the doc argues is
playable; its V1 subset already shipped as E2a; per-module panel cost still
unmeasured, so the small commitment bounds the unmeasured half). Merging the
PR is the decision; E16 is NEEDS-JEFF until then.

**Skipped just before it, with the reasons on their rows or here:**

- **E1c** — its Accept requires the cross-platform residual measured in BOTH
  directions, and the linux-seeded→mac direction cannot be produced from this
  box (linux CI is red on the apt-get; the row itself forbids dropping to
  defaults unmeasured). Wants the linux box or green CI.
- **E7** (engine polyphony) and the S-series GATED rows — not build breaks,
  so no exception reaches them.
- S27 (on the #224 branch, not yet on this chain — a bare mention on purpose,
  the stacked-PR lint rule) — my own filing this morning; its design
  call is flagged for Jeff on the row.

**Learned:**

1. **A row whose remaining work is "the ruling is minutes" is a PROPOSED
   entry waiting to be typed.** The escalation template turns a stuck row
   into a one-merge decision; nine days of open §7.2 needed ten lines.

**Next:**

1. Jeff: merge or edit the PROPOSED entry ([#218](https://github.com/JeffMcClintock/TideSynth/pull/218)-chain PRs first — this one stacks on them).
2. After the ruling: E2's next child, panel-cost measurement, S8.

**Branch/PR:** `tide/mac/E16-module-list-proposed` — TideSynth only.

---

## 2026-08-20 — macos — E6's honest tell: renders that ignored your state now say so

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · eighth item

**Did:** the wrappers-side arm of E6 — `render-audio`'s result JSON now
carries `parametersUnprimed`, the count of non-scalar parameters the prime
loop skipped. One counter, one field
([GMPI_Wrappers#9](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/9)).

**Verified live**, TIDE built with the branch as its wrappers override:
`{peak: 0, parametersPrimed: 0, parametersUnprimed: 2, silent: true}` — the
before-state is the row's own 2026-08-18 finding (no such field, `peak: 0`
indistinguishable from a silent patch).

**Scope honesty:** this is the row's "or, failing that" arm. The full Accept
— a standalone render matching the live app's audio — needs a blob-capable
prime, and that is a non-scalar setter in GMPI's `processor_holder`
(PR-GATED) plus a real design question about blob event payload lifetime
through the queued PinSet path. Proposing that half-baked into the most
curated repo at midnight is exactly what the PR-GATED bar exists to slow
down; it stays filed.

**Learned:**

1. **A row that names its own fallback scope can be half-shipped honestly** —
   the field ships value now (every E2a-class measurement stops being fooled)
   while the row keeps the full Accept visible instead of being closed on the
   cheap arm.

**Next:**

1. The GMPI blob-prime, for whoever takes the design question to Jeff.
2. E1c is the last small takeable row in this section.

**Branch/PR:** `tide/mac/E6-unprimed-report` in GMPI_Wrappers
([#9](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/9)) + TideSynth (bookkeeping).

---

## 2026-08-20 — macos — S25 does not reproduce on mac, and the negative result is the deliverable

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · seventh item

**Did:** ran S25's own comparison on this box and returned the row to TODO
with the platform scoped: **insert ≡ restore on mac**.

Same protocol as the linux finding (fresh isolated HOME so the rack is
genuinely empty — the session-restore trap is real on mac too, at
`Library/Application Support/TIDE Rack/session.xml`, not `.config`): the
Oscillator prefab click-placed from the browser renders as its two bare
jacks with correct glyphs — no tofu, no grey panel — and the same instance
restored after kill/relaunch renders identically. Screenshots committed
beside the row.

**What the negative result eliminates:** any platform-independent divergence
between the freshly-inserted and restored paths. The E2a `PanelWndPosition`
suspect behaves the same in both (the 20×66 child-union size is the
documented measure mechanic, not a divergence). **What survives:** linux
font/resource binding — tofu is missing glyphs, and the linux fresh-insert
path failing to bind what its restore path binds fits everything measured.
That diagnosis needs the linux box; this one cannot observe the path.

**Learned:**

1. **A cross-platform row can be closed on one platform and open on another,
   and saying which is the whole value of a cheap reproduction.** Twenty
   minutes here spared the linux box the half of the suspect list that
   platform-independence just killed.

**Next:**

1. **linux box:** diff the font/resource binding between the two paths (the
   S21 probe discipline — write it from the reader's side).
2. Jeff's "nothing at all on insert" remains its own unreproduced report.

**Branch/PR:** `tide/mac/S25-fresh-insert-tofu` — TideSynth only, row + evidence.

---

## 2026-08-20 — macos — S24: the cross cursor was already there on Windows, and mac got the same shortcut

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · sixth item

**Did:** gave the module-arm state a cross cursor on mac.

### The row's premise had moved again, in a useful direction

S24 asks for a port of `SetCursorHandler` registration the way
`WaylandMainWindow.cpp:72` does it. Measuring first found TIDE **already
ships the affordance on Windows** — an inline `::SetCursor(IDC_CROSS)` in
`SynthEditGui.cpp` (`dragInProgress` set at OM_DRAG_NEW_MODULE, re-asserted
every pointer move, with its own comment: "GMPI doesn't expose setCursor on
IInputHost, so we go straight to Win32"). So the smallest correct change is
the AppKit analogue of TIDE's own mechanism, not new handler plumbing:
`TideCursorMac.mm` — `[[NSCursor crosshairCursor] set]` — called at
arm/disarm and re-asserted per move (AppKit cursor rects reset the cursor on
their own schedule, exactly like WM_SETCURSOR). One `.mm`, a Darwin-only
source entry, two `#ifdef __APPLE__` forks beside the `_WIN32` ones.

### Verified to the machine's limit, then handed to the instrument that found it

- builds rc=0; `tideShowCrossCursor` linked in the standalone (nm = 1).
- the full path driven live over the command channel: browser click **armed**
  (entry highlighted), rack click **placed a List Entry** at the click point
  with fully populated properties — the arm/disarm/re-assert calls all on
  that path, no crash. (The placed combo box also *draws* here — S25's tofu
  did not manifest for this control in this build.)
- **the pointer bitmap is the one thing a screenshot cannot carry** — the
  screenshot API excludes the cursor. Jeff reported the stuck cursor, so per
  S26's lesson the verification of the pixels goes to his mouse; the PR asks.

**Linux deliberately not attempted:** the Wayland APP works because
`WaylandToplevel` has its own `setCursor`; gmpi_ui's frames expose none, so a
linux TIDE cursor needs either a gmpi_ui frame API (a design question, not a
port) or a linux shortcut of its own. Left for the linux box rather than
guessed at from here.

**Learned:**

1. **Third time today a row's central premise had moved before it was taken**
   (P7d delivered elsewhere, E15's pin ruling unlanded, now S24's "defined
   and never called" — TIDE grew a Win32 path someone added without touching
   the row). Measuring the premise first is now the cheapest step of every
   item.

**Next:**

1. **Jeff, with a real mouse:** browser-click a module — the pointer should
   turn crosshair until the rack click lands (mac).
2. **S25** (tofu) — did not reproduce for List Entry here; wants re-measuring
   against the E15 panel stack before anyone chases it.

**Branch/PR:** `tide/mac/S24-cross-cursor` — TideSynth only.

---

## 2026-08-20 — macos — E15: the rack's faceplate is TIDE's own panel, and two breaks the swap flushed out

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · fifth item

**Did:** swapped MIDI-CV's faceplate from `SE Rectangle XP` to `SE TiDE:Panel`
and retired the Rectangle from TIDE entirely — both halves of the
hardcoded-twice pair (`SynthEditSem/CMakeLists.txt` staging AND `TideApp.cpp`'s
XML merge loop), per E2c's warning that missing either side fails silently.

**Taken with `BLOCKED(E14)` in the status cell:** E14 is DONE, and BLOCKED(id)
is defined as blocked-until-id-is-DONE, so the block had expired by its own
definition. Recorded because the letter of "never start a BLOCKED item" and
the definition point different ways for an expired blocker; the row now says
which reading was used.

### Every Accept clause, measured

| clause | evidence |
|---|---|
| MIDI-CV rebuilt on `SE TiDE:Panel` | regenerated via SynthEditCL; `assert_all_modules_linked` clean, its note naming the id |
| drawn in TIDE with legible captions | [docs/images/e15-midicv-tidepanel.png](docs/images/e15-midicv-tidepanel.png) — heading + PITCH/GATE/VEL/TRIG black on the light plate |
| Rectangle gone, build green | build rc=0; `__GLOBAL__sub_I_RectangleGui.cpp` **0** and `__GLOBAL__sub_I_TiDEPanelGui.cpp` **1** in BOTH `TIDE.gmpi` and `TIDE_VST3` |

### The two breaks the swap flushed out

1. **The first regeneration drew captions overhanging the plate.** The panel
   sizes ITSELF — RackUnits × 48 DIPs (`kRackUnitDips`) — and
   `SubView::measure` unions the children, so the authored 105-DIP slot
   produced a 48-wide plate under 58-wide labels. Fixed with `Rack Units 2`
   (96 DIPs; the one pin the swap sets — per-instance geometry, where the
   2026-08-19 ruling made *appearance* compile-time) and the faceplate grid
   re-authored to 96. The residual 3.2-HP-per-U vs 15-DIP-HP mismatch is
   E17's own open note, deliberately not settled here.
2. **`TiDEPanel.gmpi` had never been built on mac, and could not be.**
   `helpers/Timer.cpp`'s mac path is CFRunLoopTimer; the loadable-bundle link
   line carries no frameworks, so the authoring bundle failed with undefined
   `_kCFRunLoopCommonModes`. One APPLE-guarded CoreFoundation line. (TIDE's
   static build never sees it — the wrappers already link CF; same class as
   P7d, one repo over.)

### The authoring pipeline on mac, recorded so nobody re-derives it

SynthEditCL's factory scan root is **`SynthEditCL.app/Contents/PlugIns/`**
(the rescan prints it), NOT `build/modules/` — the TiDE `.gmpi` bundles must
be copied there, then `SynthEditCL -rescan`, then `build-prefabs.py --secl`.
The other five prefabs regenerate byte-identical except handle churn (the V3
lesson), so they were reverted; only `MidiCv.synthedit` is committed. And
`TIDE_STANDALONE` restores its session even under an isolated `HOME`
(`Library/Application Support/TIDE Rack/session.xml` on mac, not `.config`) —
two identical screenshots cost a relaunch before the session file was found.

**Flagged for Jeff, not acted on:** the row records a ruling that the panel
"ships exactly two pins", but `main`'s registration XML declares SIX (Text,
Text Color, Rack Units, Layout, Material, Panel Color). Rack Units earns its
keep as instance geometry; whether Layout/Material/Panel Color go compile-time
is the unimplemented half of that ruling.

**Skipped on the way here, with reasons:** **N1** defers itself ("after C7,
not during"; C7 waits on Jeff's apt-get, and a rename would double-conflict
with the open C10 PRs). **P11**'s remaining half is the diagnostic at
`DocOb.cpp:540` — GATED, not a build break; option (c) already landed in
docs/building.md. **S16/S22/S18** are GATED (S18 is also a licensing question
first). **E9**'s Accept needs a within-instance rate change no measurement has
ever produced — the wrappers replace the instance instead. **E5** waits on
rack styling (NEEDS-JEFF by its own text).

**Learned:**

1. **A module that measures itself is a different contract from a rectangle
   that takes orders, and the prefab grid only worked because the old
   faceplate obeyed it.** The swap surfaced the collision immediately —
   authored geometry vs self-sizing panels — and E17's units note is where the
   real reconciliation lives.
2. **"It builds in the product" says nothing about the authoring path.** The
   panel shipped statically into TIDE on three platforms while its loadable
   authoring bundle had never linked on mac. The first prefab regeneration
   needing it found that in one step.

**Next:**

1. **S24/S25** are the remaining takeable S-rows (cursor feedback; the tofu
   render — S25 may interact with this swap and wants re-measuring on top of
   it).
2. **E2's next children** now have both the panel and the pipeline; E16/E17's
   open notes gate the module list and sizing.

**Branch/PR:** `tide/mac/E15-panel-swap` — TideSynth only.

---

## 2026-08-20 — macos — P7d was already fixed, from a third direction, and its parked question is moot

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · fourth item

**Did:** ran P7d's own Accept before building anything — `ninja GainGui_VST3`
on a fresh `GMPI-plugins` configure, no extra flags — and it **passes**:
rc=0, bundle produced, `otool -l` shows the UniformTypeIdentifiers load
command with the UTType reference resolved. Closed the row as DONE.

The fix is GMPI_Wrappers `3838493` — `gmpi_weak_frameworks()` in
`wrapper/cmake/GmpiFrameworks.cmake`, included by **all five** wrappers, in
the `-weak_framework` form `MacFileDialog.h`'s own comment specifies. Its
comment even names this row's incident: *"GMPI-plugins macOS CI, July–August
2026"*.

**The row had parked itself waiting on a scope ruling** (may a run touch
`GMPI-plugins`, or should gmpi_ui declare the framework?) — and the landed
answer is a third place neither option named: the wrappers, which are what
actually compile `DrawingFrameMac.mm`. No `GMPI-plugins` edit, no per-consumer
link lines, ALLOWED path. The ruling request is moot.

**Learned:**

1. **A row that parks on a question can be closed by running its Accept —
   check that before re-raising the question.** The ruling P7d wanted was
   never given and never needed; the fix arrived while the row waited. Fourth
   already-delivered row in two days (C15, U2, E14's naming note, now P7d) —
   running the Accept first is cheaper than any of the work the row proposes.

**Next:**

1. Queue by file order continues: S-series GATED rows are skipped (not build
   breaks), **N1 defers itself until C7 closes** ("do it after C7, not
   during" — C7 waits on Jeff's apt-get, and a rename now would double-conflict
   with the open C10 PRs), **A12/B1** are workflow edits the token cannot push.

**Branch/PR:** `tide/mac/P7d-uttype-framework` — TideSynth only, bookkeeping.

---

## 2026-08-20 — macos — C10: 104 editor files leave the root, and the reference count fell as it was measured

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · third item (the #222 build break interrupted it mid-baseline)

**Did:** re-homed the carve-out's editor files from `SynthEditLib`'s root into
`EditorLib/`, beside their own CMakeLists. Branches
`tide/mac/C10-rehome-editor` in **SynthEditLib** (the move) and **SynthEdit**
(consumer rewrites) — those two must merge together; the TideSynth branch is
bookkeeping only.

### The move set is a measured list, not "the editor files"

**104 files** = EditorLib's compiled root-level entries ∩ files the carve-out
added since 2026-08-01 (`git log --diff-filter=A`). The intersection matters
from both sides: EditorLib also compiles **3 root files that predate the
carve-out** (`CancellationAnalyse.{cpp,h}`, `SafeMessageBox.h` — they stay),
and the carve-out also added root files EditorLib does NOT compile
(`SynthEditApp.{h,cpp}`, `ExportAsPlugin.{cpp,h}`, `ModulePicker.h` — the
compile-direct/app-level set, deliberately untouched).

### The consumer survey shrank fourfold under measurement

A boundary-less grep first claimed ~33 build-file references across 10 files,
including three in `se_au`'s pbxproj. With word boundaries and reading each
hit: `se_au`'s three are **substrings** (`IDspPatchManager.h` ⊃
"PatchManager.h", `UPlug.h` ⊃ "Plug.h", `HostVoiceControl.h` ⊃ "Control.h"),
SE16's root CMakeLists refs are **comments**, SynthEditCL/Wayland/Juce refs
are **comments**, and TideSynth's two refs are **comments**. What actually
needed editing:

| consumer | edit |
|---|---|
| `EditorLib/CMakeLists.txt` | 104 path prefixes + ONE `PUBLIC ${SYNTHEDITLIB_DIR}/EditorLib` include dir |
| `CpuMeterGui.cpp` | 3 explicit `EditorLib/` include prefixes — it is compiled by the **DSP target**, the one consumer that links no EditorLib |
| `SynthEdit2.vcxproj` + `.filters` | **6** literal paths (the other 8 grep hits are "moved to EditorLib" comments, now literally true) + the new dir in 4 AdditionalIncludeDirectories |
| both SynthEditMac xcconfigs | HEADER_SEARCH_PATHS + the new dir |
| pbxproj | one FuzzyMatch.h file ref |
| `MidiAutomationWindowController.mm` | the one literal include — the same file the carve-out broke on 2026-08-20's CI fix |

**52 SE16 source files include these headers bare** and needed nothing: CMake
consumers inherit the PUBLIC dir, vcxproj/Xcode get it from their own include
lists.

### Verified before vs after, same tree, same metrics

| check | before | after |
|---|---|---|
| SE16-hosted Ninja build | 1072/1072 rc=0 | 207/207 rc=0 (incremental) |
| **compile-graph edges** | **941** | **941** |
| ctest (S16-class env vars set) | 67/67 | 67/67 |
| SynthEditMac `xcodebuild` | BUILD SUCCEEDED | BUILD SUCCEEDED |
| `TIDE_STANDALONE` | — | runs, seeds 6 prefabs |
| movers left at root | — | 0 |

The 941 needed care: the base build tree's ninja graph was overwritten by the
reconfigure, so the pre-move graph was **reconstructed from a detached
worktree at origin/main** (configure-only) and both sides counted with the
identical command. A "same object count" clause is only evidence when both
numbers come from the same instrument.

**Stated unverified:** `SynthEdit2.vcxproj` (WinUI3/MSBuild — the win box or
the Store pipeline builds it; the edits are 6 path rewrites + 4 include-dir
lines) and `SynthEditWayland` (linux; its refs were comments, its CMake
consumers inherit the PUBLIC dir). `SynthEditJuce` is deprecated and not
generated (#88).

**Gate note, stated rather than assumed:** C10 edits `SynthEditLib` (GATED)
as a filed stage of the C0-approved carve-out — the same standing C12a–f, C14
and C16 ran under on all three boxes; the prompt's "C1-C7" enumeration has
been acknowledged-stale since C12c, and the windows box's C14 lesson
explicitly rebuked the narrow reading. Review still discharges it: nothing
merges without Jeff.

**Learned:**

1. **A reference count taken without word boundaries is an upper bound, not a
   work list.** 33 references shrank to ~15 real edits across four files once
   substrings and comments were read; `se_au` — a whole Xcode project I
   expected to edit — contained only substring matches.
2. **"Same object count" needs the same instrument on both sides.** The build
   log counts what compiled; the ninja graph counts what is scheduled, and `-c`
   greps catch `python -c` too. Reconstructing the base graph from a detached
   worktree cost one configure and made the numbers actually comparable.

**Next:**

1. **The win box should build `SynthEdit2.vcxproj`** on the SE16 branch before
   or at merge — the one consumer neither CI nor this box exercises.
2. S27 (tide_render references, filed during #222) and the E2 umbrella note
   are unchanged.

**Branch/PR:** `tide/mac/C10-rehome-editor` × 3 repos — SynthEditLib and
SynthEdit must merge together; TideSynth is bookkeeping.

---

## 2026-08-20 — macos — A32: the umbrella advisory, and the measurement that was already done

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · second item

**Did:** the advisory A32 specced, in `check-id-refs.py`: live rows whose
`X[a-z]` children are all closed print as **ADVISORY** — candidates for a
human, never an exit code.

The design work was already paid for: the U2 run measured the naive gate at a
50% false-positive rate (U2 real, E2 false) and A32's row pinned the remedy to
advisory output with `lint` staying green. So this session's job was mostly to
not re-decide that — build it as specced, then verify the measurement still
holds on today's tree.

### It does, with one moot half

| the U2 run predicted | today |
|---|---|
| fires on U2 (real) | **moot** — U2 was archived when it closed, and a closed/archived row is nobody's umbrella |
| fires on E2 (false, by design tolerated) | **fires on E2 alone**, rc=0, advisory printed |
| silent on C7 (open child) | **silent** — C7e is NEEDS-JEFF, live |

Positive control on real data: flipping C7e to DONE in a copy makes **C7 join
the report** with all five children named. Selftest **38 cases, 0 failed**
(6 new, one per shape above plus childless rows, closed umbrellas, and
child-of-child); proven able to fail — flipping the U2-shape expectation gives
`FAIL umbrella/U2's shape`, rc=1, restored.

**Children closed in the LIVE file count as closed** (E2b is WONTFIX in
BACKLOG.md, E2a/E2c archived — E2's children are 1 live-file + 2 archive), so
the rule reads status, not location. A child that is itself live keeps the
umbrella alive wherever its siblings sit.

**Deliberately not done:** E2's re-spec. A32's row asks for it "while someone
is here", but which module children E2 intends is a product call — E17's
path-traced design language resolved only yesterday, and the module set it
implies is E16/E2 territory, not a lint session's. The advisory's own text
explains E2's presence, which is what the Accept required.

**Stacked on A31's branch** (`tide/mac/A31-same-job-habit`) because both edit
`check-id-refs.py`; the PR targets that branch and GitHub retargets to `main`
when #218 merges (the A10 lesson).

**Learned:**

1. **A row that carries its own false-positive measurement is a different kind
   of spec: the build step is obedience, not design.** U2's session measured,
   A32's row recorded, this session implemented — three runs, no re-derivation.
   That is the backlog working as a memory the runs themselves lack.
2. **"Advisory" needs the reason printed with it, or it decays into noise.**
   The report explains the E2 class inline — an umbrella with unfiled future
   children is indistinguishable from a finished one — so a reader who has
   never seen A32 still knows why rc is 0 and what judgement is theirs to make.

**Next:**

1. **The takeable process rows are exhausted** — A31 and A32 are both
   IN-REVIEW on this box's stacked branches. The mac queue after them is C10
   (`SynthEditLib` authority) or Jeff's workflow edit
   ([#189](https://github.com/JeffMcClintock/TideSynth/issues/189)).
2. **E2 wants its next child filed or its row saying none are intended** — the
   advisory will name it every run until one of those happens; that is the
   advisory working, not failing.

**Branch/PR:** `tide/mac/A32-umbrella-advisory`, stacked on A31's branch — TideSynth only.

---

## 2026-08-20 — macos — A31: the granularity was the whole design, and three measurements chose it

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present

**Did:** shipped both halves of A31 — the filing-time habit into the run
prompt, and a shared-location check into `check-id-refs.py` that fails when
two LIVE rows cite the same `file:line`.

### The row said "only if it can be done without false alarms", so the alarms were counted first

Three candidate granularities, each run against the real tree before any code:

| granularity | fires on | verdict |
|---|---|---|
| same FILE, live rows | **14 groups**, all legitimate (`CMakeLists.txt` alone: 14 rows) | unusable |
| same FILE:LINE, live rows | **0** today — and the real C15/C16 pair IS caught | **shipped, as a gate** |
| file:line, live vs DONE/archived | **6 hits, 6 false** — umbrella C7 vs its own landed splits, S3g vs its parent S3, E6/E7 follow-ups | **excluded, 100% false** |

The middle row is the finding: both C15 and C16 cite
`SynthEditSem/TideAppStubs.cpp:31` **verbatim**, so the strict variant catches
the one real historical collision while firing on nothing else in the live
tree. Keying is basename:line, which is what lets C15's
`SE16/SynthEditSem/...` spelling meet C16's `SynthEditSem/...` spelling.

The excluded tier matters as much as the shipped one: a follow-up or remainder
row legitimately cites the sites its parent touched, so live-vs-closed pairing
is structurally noisy — the same shape A32 measured for umbrella rows. The
habit covers that side at filing time instead: grep **freshly-fetched**
`origin/main:BACKLOG.md` for the file you are about to name (fetch-fresh per
A19's id-recheck precedent — your branch's copy predates any concurrent run's
filing, which is the exact mechanism that produced C15/C16).

### Verification

- selftest **32 cases, 0 failed** — 7 new, one per measured tier, on real file
  bodies; **proven able to fail** (flipping one expectation → `FAIL
  shared/two live rows...`, rc=1, then restored).
- positive control on the real tree: restore archived C15 as TODO beside C16
  as IN-REVIEW → **rc=1**, naming every live citing row (C7, C15, C16) with
  file:line for each; unmodified tree → **rc=0**.
- `check-next-block.py`, `check-backlog-diff.py`, `check-journal-prepend.py`,
  `check-id-refs.py` all green on this branch.

**What is NOT verified:** `lint.yml` runs `check-id-refs.py` without
`--selftest` (known since A23), so the 25→32 selftest cases still run only by
hand; making CI run them stays a `.github/workflows/**` edit, Jeff's.

**Also this run (STEP 4):** flipped **S26** IN-REVIEW→DONE on merged
[#213](https://github.com/JeffMcClintock/TideSynth/pull/213); re-pointed the
mac NEXT cell at **A32** with the old imperative demoted.

**Learned:**

1. **A check's granularity is not a style choice — each candidate tier had a
   measurable false-alarm rate (14, 0, 6) and only one was shippable.** The
   fifth time this week that measuring a proposed lint against the live tree
   changed its design before it shipped (A23, A27, A24, A32, now A31).
2. **The C15/C16 collision left a fingerprint neither filer intended: both
   rows cite the same `file:line` verbatim.** Rows written independently about
   one job converge on its address, which is why location, not id, is the
   detectable invariant.
3. **The check's first catch was its own author, in the same commit that adds
   it.** The A31 row's outcome text quoted the C15/C16 citation as a backticked
   `file:line`, which made A31 (live) collide with umbrella C7 (live) the
   moment the lint ran. Fixed by spelling the line out in prose. A row ABOUT a
   location collision must not itself cite the location in citable form —
   that is now written into the row rather than left to be rediscovered.

**Next:**

1. **A32** — the umbrella advisory, the last takeable process row; its
   measurement is already in its own text.
2. The carve-out still waits on Jeff's `apt-get`
   ([#189](https://github.com/JeffMcClintock/TideSynth/issues/189)); linux
   issues #191–#216 are all that one cause.

**Branch/PR:** `tide/mac/A31-same-job-habit` — TideSynth only.

---

## 2026-08-20 — linux — S26: the se_sdk timers never fired, and Jeff's mouse was the instrument that found it

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** found and fixed why click-placing a module inserted it into the
document while the view never showed it until a view-switch. **Verified by
Jeff with a real mouse: "insert works now. module appears immediately."**

### First, a correction that a squash-merge nearly lost

**[#211](https://github.com/JeffMcClintock/TideSynth/pull/211) auto-merged
between my two pushes**, so only its first commit reached `main`. The second —
correcting S24 and filing S25 — landed on the branch afterwards and was
orphaned. `main` therefore carried S24's claim that `SetCursorHandler` is
*"defined and never called"*, which is **wrong**: it is never called in the
**public** tree, and `SE16/SynthEditWayland/WaylandMainWindow.cpp:72` calls it.
Both the corrected S24 and S25 are recovered in this branch. This is A22's
auto-merge trap in a new shape — there it was a link-only follow-up worth
dropping, here it was the substance.

### Jeff's repro was the diagnosis

He reported: insert appears to do nothing, **but the properties pane lights
up** — which only happens for modules in the document — and switching to
structure view and back **forces a refresh** that reveals the module. Then he
named the distinction that cracked it: *invalidating the pixels* (redrawing
what is there) versus *refreshing the view* (reconstructing it from the
document via fresh XML). The insert marked the view dirty; the refresh never
ran.

### The mechanism

`MfcDocPresenter` is its own `se_sdk::TimerClient` — `setView()` does
`StartTimer(50)`, and each tick services `viewDirty` -> `RefreshView()` ->
re-export the container to JSON -> rebuild. A view-switch works because
`setView()` calls `OnTimer(); // intial refresh` directly, needing no timer.

**On Linux, se_sdk timers have no source at all.** `TimerManager.h:94`: the
host *"must call [Pump(elapsedMs)] periodically"*. Jeff predicted SynthEdit
Wayland "must use some workaround" since it works on this same machine — it
does, literally: `Main.cpp:190` pumps it every loop (and `:189` pumps
gmpi_ui's separately). TIDE, a plugin with no main loop, pumped nothing.
Windows survives via `PatchManager.cpp:1850`'s DX-view path. And
`timerhelper = new AppTimerHelper(this)` turns out to be the **last line of
the base `InitInstance` that TideApp skips** — S5's known gap, second
consequence.

### The fix

`SeSdkTimerPump` in `SynthEditGui` — a Linux-only 30ms gmpi_ui `TimerClient`
passing real elapsed time, living and dying with the editor. It rides gmpi_ui
timers because those ARE serviced here (`StandaloneApp.cpp:332`). Not folded
into the 500ms heartbeat: S12's sync serialises the whole document per tick.
One wrinkle: `::TimerManager` needs the global qualifier — ambiguous with
`gmpi::TimerManager`, the exact collision the se_sdk header warns about.

**Every se_sdk timer client in a Linux TIDE benefits** — the scopes have
never ticked either.

### What this session's instruments could and could not see

Injected-pointer inserts refreshed **even before the fix**, so this box's A/B
could not distinguish — pre- and post-fix screenshots are identical for my
path. Only Jeff's mouse could verify, and did. Why the injected path behaved
differently is **unexplained and worth suspicion** — noted, not theorised.

**Still open, deliberately unclaimed:**

1. **The coordinates** (S26 row text): Jeff's inserts landed at structure-view
   bottom-right / off the left of panel view; injected inserts land at
   sensible coords (400,150 click -> 4104,3816). Different outcomes,
   unexplained.
2. **S25's tofu**: a freshly inserted prefab still draws grey with
   missing-glyph text post-fix, so that defect is independent of the refresh.

**Learned:**

1. **"It redraws" and "it refreshes" are different systems with different
   drivers, and the user who owns the product knew to distinguish them.** The
   pixels repainted fine all along (cables, hover, wheel); the
   document-to-view rebuild is a timer, and the timer was dead.
2. **A platform port is complete when every pump the reference app runs has an
   owner.** SynthEditWayland's main loop runs THREE (gmpi_ui, se_sdk,
   app.OnTimer); the standalone wrapper supplies the first for TIDE, nothing
   supplied the other two.
3. **When two input paths disagree, say so and hand verification to the one
   that failed.** Claiming the fix on my path's evidence would have been the
   wayland-deps mistake again.

**Next:**

1. Jeff's coordinate observation wants a reproduction with pan/zoom state
   recorded — it did not reproduce via the command channel.
2. S25 (tofu render) unchanged.
3. Whether `app.OnTimer()` (DSP queues, live-module updates) also needs a
   Linux pump in TIDE is S5-adjacent and deliberately not smuggled into S26.

**Branch/PR:** `tide/linux/S26-pump-se-timers` — [#213](https://github.com/JeffMcClintock/TideSynth/pull/213).

---
## 2026-08-20 — linux — insertion is arm-then-click, and I had blamed the wrong thing

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Did:** worked out why nothing could be inserted into the rack, after Jeff
reported that he could not do it either with a real mouse, and that his cursor
jammed on a left-right icon.

### The mechanism: it is not a drag

`ModuleDragAndDropManager.h` states it plainly:

```
1. User selects a module in the browser  -> ModuleBrowser calls BeginDrag(id)
2. Cross cursor is shown via the registered hook
3a. User clicks in the editor            -> presenter inserts module, EndDrag()
```

Plus `InsertAtViewCenter` for a double-click shortcut. **Nobody was dragging
wrong — dragging is not the gesture.** Clicking `Prefabs > Filter` and then
clicking the rack inserts `TIDE Filter` (properties: X 4304, Y 3968, W 100,
H 160). It has worked the whole time.

### I read my own control backwards

My E14 entry said the failure was *"the gesture, not the modules"*, on the
strength of the known-good `Oscillator` prefab failing identically. **That same
observation supports "placement is broken for everyone" exactly as well**, which
is what Jeff's report pointed at. I picked the reading that blamed my technique
and never considered the other. The truth was a third thing neither reading
named, and the file that explains it was one grep away.

### The real defect, filed as S24

**TIDE gives no cursor feedback for the armed state, on any platform.**
`ModuleDragAndDropManager::SetCursorHandler` is **defined and never called** in
the public tree — its own comment says MainWindow (WinUI) and SynthEditBridge
(Mac) register it, and both are private desktop-app classes. So the cross cursor
never appears, and the view keeps whatever cursor it last had; over a panel edge
that is a resize cursor, which is exactly the "stuck left-right icon". The
feature works, is undiscoverable, and looks broken.

### E14's clause 2 is now half closed, precisely

Clicking `TiDE > Panel` then the rack **inserts `SE TiDE:Panel`**, and the
properties pane shows its live pins — `Text`, `Text Color FF101010`,
`Rack Units 1`, `Layout`, `Material Brushed Aluminium`, `Panel Color FF2E3238`.
So it drops in and constructs.

**"And draws" is still unconfirmed** — nothing visibly appeared at the click
point. That smells like E2a's `PanelWndPosition` trap, where a module drops
successfully and draws nothing, and it is the next thing to check.

### One thing that made this harder to see

`TIDE_STANDALONE` persists its rack to **`~/.config/TIDE Rack/session.xml`** and
restores it on launch (`SessionState.cpp`). A relaunch is **not** a clean rack:
modules from an earlier session reappear, which is why a `List Entry` Jeff had
inserted kept showing up in what I called fresh instances.

**Learned:**

1. **When an experiment has two readings and one of them blames my tools, that
   is the one to distrust.** The Oscillator control was good evidence and I drew
   the self-flattering conclusion from it — "my gesture is wrong" is a much
   smaller claim than "insertion is broken", and I never tested the larger one.
2. **Read the interaction's own header before guessing at it.** Three failed
   gestures and a wrong conclusion cost more than the one grep that found
   `ModuleDragAndDropManager.h` and answered it in nine lines.
3. **A feature with no feedback is indistinguishable from a broken one**, and two
   people independently concluded "broken". That is a UX defect worth a row, not
   a documentation gap.
4. **`TIDE_STANDALONE` restores its last session**, so "restart for a clean rack"
   is false. Delete `~/.config/TIDE Rack/session.xml`.
5. **`pkill -f <name>` matches the shell running it.** I recorded this in the E14
   entry and then did it again an hour later.

**Next:**

1. **S24** — register a cursor handler in `TideApp`. Small, ALLOWED, and it is
   what makes the feature discoverable.
2. **E14's "and draws"** — check whether the inserted `SE TiDE:Panel` has a
   zero rack rect (`PanelWndPosition`), which is E2a's documented trap.
3. **A question for Jeff, not a run:** whether arm-then-click is the interaction
   TIDE wants. Two people who know the product both reached for a drag, which is
   what VCV and Blocks use. That is E2/E17 territory.

**Branch/PR:** `tide/linux/E14-insert-gesture` — TideSynth only, docs and rows.

---
## 2026-08-20 — linux — E14: TIDE's own two modules are in the product, and half of Accept is met

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Sixth item this session**, at Jeff's direction — and taken on linux only because
S21's correction showed the visual check IS possible here. I had ruled E14 out
on the wrong premise a few hours earlier.

**Did:** compiled `TiDEknob.cpp`, `TiDEknobGui.cpp` and `TiDEPanelGui.cpp` into
the plugin, linked `tide_render`, and allowlisted both ids in
`TIDE_STATIC_EXTRAS`.

### Accept clause 1 is met, and I looked at it

TIDE's browser now shows a **TiDE** category containing **Panel** and **knob**.
That is the authoritative check this row asks for, done the way it asks.

Symbol-level proof as well, because the row is explicit that an id check is a
false positive: `__GLOBAL__sub_I_TiDEknob.cpp`, `__GLOBAL__sub_I_TiDEknobGui.cpp`
and `__GLOBAL__sub_I_TiDEPanelGui.cpp` are all present in **both** `TIDE.gmpi`
and `TIDE_VST3.so`, with a negative control (`NotAFile.cpp` -> 0). Those symbols
name the FILE, so the rename table cannot fake them.

Clean clone, configure rc=0, build **rc=0, zero error lines**.

### Accept clause 2 is NOT met, and the reason is me

*"A prefab using them drops into the rack and draws."* I could not work out the
placement gesture — dragging a browser entry into the rack only highlights it
yellow. Batched drag, explicit press/move/release, and double-click all failed.

**Controlled before blaming the modules:** the **known-good `Oscillator` prefab
fails identically** under the same gesture, and E2a placed that one with real
mouse drags. So this is the interaction, not the two new classes. Anyone who
knows the gesture closes clause 2 in a minute; I have left the row IN-REVIEW
saying so rather than claiming it.

### Three of this row's own notes are stale, and acting on them would waste a run

1. **The naming inconsistency is already fixed.** The row says `SE TiDE:knob`
   declares `name="TiDE:knob"` against Panel's `name="Panel"`. On current `main`
   it declares **`name="knob"`** with `category="TiDE"` — identical shape
   (`TiDEknob.cpp:25`, `TiDEPanelGui.cpp:2680`). Nothing to settle.
2. **The awkward path is gone.** The row was written when `SynthEditSem/` lived
   in private `SE16` while these sources did not, and calls for a new CMake
   variable. **C7b moved SynthEditSem into this repo**; they are siblings now and
   a relative path was enough.
3. **The linker warning did not bite.** No helper TU was dragged in — unlike
   Converters (`my_type_convert.cpp`) and Oscillator HD (`real_fft.cpp`), these
   two are self-contained apart from `tide_render`.

### The one judgement call, flagged for review

`TiDEPanelGui.cpp` marches a distance field, and its own CMakeLists calls
`tide_render_strict_fp` because `modules/` turns on fast math globally. **TIDE
turns it on too** — `/fp:fast` on MSVC, an explicit reassociating subset on Apple
(`CMakeLists.txt:254-265`).

Applying `tide_render_strict_fp(TIDE)` would change the **DSP engine's** float
model in order to fix a renderer. That is far more than this row should touch, so
the opt-out is applied **per source file** through a new
`tide_render_strict_fp_sources()`, defined beside the existing function so one
place still decides what "strict" means.

**On Linux neither fast-math block is active, so this is a no-op here** and is
therefore **unverified on Windows and macOS** — the two platforms where it
actually does something.

**Learned:**

1. **Control the tool before blaming the subject.** The placement failure looked
   like "the new modules do not work"; one drag of a known-good prefab showed it
   was my gesture. That control cost thirty seconds and changed the conclusion
   completely.
2. **A backlog row's warnings age with the tree, and three of E14's had.** Two
   were closed by C7b, which was written after it. Re-check a row's premises
   against `main` before following its instructions — it is cheaper than the
   work it would misdirect.
3. **A per-target compile option is the wrong tool when a requirement belongs to
   one file.** `set_source_files_properties` keeps the audio path out of a
   renderer's floating point argument entirely.
4. **`pkill -f <name>` matches the shell running it**, because the pattern is in
   that process's own command line. It killed my own script mid-edit (exit 144).

**Next:**

1. **Clause 2 needs someone who knows the rack placement gesture.** Everything
   else about E14 is done and measured.
2. **The per-source FP opt-out wants a Windows or macOS build** to confirm the
   panel still renders — it is inert on Linux.
3. E14's row has been corrected in place for the three stale notes above.

**Branch/PR:** `tide/linux/E14-modules-in-product` — TideSynth only.

---

---

---

## 2026-08-20 — linux — S17: name the folder, not the decision

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Fourth item this session**, continued at Jeff's direction. TIDE-side half only;
the SE16 half is filed as **S22**, per STEP 5's rule for a fix spanning the gate.

**Did:** made TIDE's configure name the **resolved path** of every dependency,
and made a shadowed override fail loudly.

### The thing this row assumed, that nobody had checked

**TideSynth's own build does not have the defect.** Configured with
`-DGMPI_UI_FOLDER_OVERRIDE=~/SE/gmpi_ui` and read the compile flags rather than
the message: they are **`-I/home/jef/SE/gmpi_ui`**, and **no `_deps/gmpi_ui-src`
is created at all**. The shadowing is SE16's, not C7d's root — so a run that
"fixes" TideSynth expecting E12's symptom to move will be chasing nothing.

Accept clause 2 (*"editing the local clone changes the binary"*) therefore
already held here. Clause 1 (*"a build log line naming the resolved path per
dependency"*) did not, and is what landed.

### What the log says now

All **8** dependencies, in one block after every `add_subdirectory`:

```
-- TIDE dependency provenance -- 8 resolved:
--   vst3_sdk <- /home/jef/.cache/CPM/vst3_sdk/2df5ae7.../vst3_sdk [CPM cache]
--   SynthEditLib <- <tree>/build/_deps/syntheditlib-src [fetched]
--   GMPI <- <tree>/build/_deps/gmpi-src [fetched]
--   gmpi_ui <- <tree>/build/_deps/gmpi_ui-src [fetched]
--   CLAP, clap_helpers, harfbuzz, GMPI_Wrappers ...
```

`cmake -B build 2>&1 | grep "dep "` is the whole diagnostic.

**Two of the eight were never announced at all**, which I did not expect:
`vst3_sdk` and `harfbuzz` resolve out of **`~/.cache/CPM/`** — a *third* source,
mentioned neither by the old messages nor by S17's own text. A stale entry there
is invisible in exactly the way S17 describes, and it is shared across every
build on the box.

### The check is proven by a positive control, not by reading it

`tide_check_not_shadowed` fires when an override is set *and* a fetched copy
exists. Creating `build/_deps/gmpi_ui-src` by hand with the override set makes
configure exit **1**, naming both paths. Without that control it would be
untested code that always passes — the same class of thing as a probe that never
fails.

**Result:** configure rc=0; reconfigure holds at **8** rather than doubling; full
Release build **rc=0, zero error lines**; artifacts produced; and S21's probe
still passes on the same tree.

**One live bug found while building it:** the first version printed **7 of 8**.
`SynthEditSem/` resolves `GMPI_Wrappers` in its own directory scope, and
`set(... PARENT_SCOPE)` from a function called there reaches *that* directory,
never the root. Fixed by accumulating in a `CACHE INTERNAL` variable
`FORCE`-cleared at the top of each configure, so reconfigures rebuild the list
rather than doubling it.

**Learned:**

1. **A message that names a DECISION cannot catch a wrong RESOLUTION.** "Using
   local X" was true and useless — the override was read, and something else
   still won. Print the path, not the branch you took.
2. **Check whether the bug you were sent to fix is present in the tree you are
   fixing.** S17 is written about SynthEdit's build; TideSynth's turned out to be
   correct, and only reading the compile flags rather than the log showed it.
3. **`set(PARENT_SCOPE)` from a function called in a subdirectory reaches that
   subdirectory, not the top level.** The symptom was a count of 8 beside 7
   printed lines — arithmetic that looks self-consistent until you count twice.
4. **Dependencies here come from three places, not two:** an override,
   `build/_deps/`, and the shared `~/.cache/CPM/`. Any reasoning about "is this
   local?" that considers only the first two is incomplete.

**Next:**

1. **S22** carries the same ~30 lines into `SE16/CMakeLists.txt`, where the
   defect was actually observed. GATED and not a build break, so it needs Jeff or
   an approved stage — a run cannot take it.
2. **`VST3_SDK_FOLDER_OVERRIDE` is read but never declared** as a cache variable
   (`CMakeLists.txt:99` reads it; `:36` declares `VST3_SDK_USE_MIDI_UMP`
   instead), so unlike the other four it is settable only with `-D`. Left unfixed
   as outside this row, and recorded on S22 — SE16 is what this file was copied
   from and may share it.
3. **This branch predates A30's merge, so `docs/lessons.md` is not regenerated
   here.** Nothing enforces that it is; A30's own entry says so. Whoever merges
   both should run `python3 scripts/extract-lessons.py --write`.
4. Unchanged: **the `apt-get` in `build.yml`** is still the only thing between
   C7e and closed. CI has now filed six issues for it (#189, #190, #191, #193,
   #195, #197).

**Branch/PR:** `tide/linux/S17-dependency-provenance` — TideSynth only.

---

