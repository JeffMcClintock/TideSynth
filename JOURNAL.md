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

## 2026-08-21 — macos — E5: the rack grid ruled, and the snap is gcd(12, 15)

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** implemented Jeff's ruling on the rack grid. `hpWidth` stays **15**
(the visual HP), a new **`snapWidth = 3`** is the placement pitch, the row is
**384**, TIDE's standard module is **48** wide, and a panel is the **whole
row**.

### Jeff proposed 384 / snap 12 / width 48, and the measurement moved one of the three

Snapping on 12 is the arm that would have cost VCV compatibility, which is the
opposite of what it looks like — every VCV module is a multiple of 15 by
construction, so today's 15-snap fits them all exactly:

| snap | exact for | worst gap |
|---|---|---|
| 15 (today) | 13 of 13 common VCV widths | 0 |
| **12** | **5 of 13** | **9 DIPs (3.0 mm)** |
| 6 | 10 of 13 | 3 DIPs |
| **3 = gcd(12,15)** | **13 of 13** | **0** |

12 and 15 first agree at 60 DIPs (4 HP), so a 12-snap misaligns every VCV
module that is not a multiple of 4 HP — worst on the 1–2 HP utilities there are
many of. **3 is the coarsest snap that satisfies both worlds**, and it keeps
every TIDE dimension a multiple of 12 anyway, because 384 and 48 both are.
Jeff took it.

### `hpWidth` was doing two jobs, and this would have wrecked the second

`ViewBase::renderRack` derives the rail hole pitch *and* the hole radius from
`hpWidth` — *"one threaded mounting hole per HP"*, `holeRadius = hpWidth *
0.13`. Setting it to 3 would have given the rails a hole every 3 DIPs at
**0.39 DIP radius**: fine sandpaper instead of Eurorack rails, and nothing
would have failed to build. Six consumers, all in one file; the split sends
`snapToGrid` to `snapWidth` and leaves the four rendering sites on `hpWidth`.

### The rail allowance never existed — Jeff's correction, and it is the bigger one

*"useable interior is the entire rack module surface (we don't draw mounting
hardware)."* Confirmed in the render order rather than taken on trust:
`ContainerView.cpp:71` calls `renderRack` inside the **background fill**, so
modules draw over the rails exactly as a real panel covers the rails it is
screwed to. Rails show only in empty slots.

So `build-prefabs.py`'s `380 - 2*15 = 350` subtracted an allowance that does
not exist — and that assumption had propagated: **#239's probe encoded it too**
and reported TIDE's own prefab as `TALLER THAN ROW`, a verdict measured against
a constant that does not govern. Both corrected. **A wrong model in a
verification tool is worse than no tool**, because its output looks like
evidence.

### Verified

- probe rewritten to the ruled model: **12 selftest cases, 0 failed**, and the
  new cases are the ruling's own claims — a 380 VCV panel and a 384 TIDE panel
  both pass, 400 fails, 48 / 30 / 36 all land on the 3-grid, 100 does not.
- all four targets build rc=0 against the modified `SynthEditLib`.
- MidiCv regenerated at 384 and **re-measured on a running rack**, not just
  rebuilt: `TIDE MIDI-CV  w=96 h=384  6.400 HP  on-grid  fits row`, no
  overlaps — the two clauses this row could never satisfy, satisfied — and the
  rails visibly pass *behind* the panel
  ([docs/images/e5-rack-ruled-grid-macos.png](docs/images/e5-rack-ruled-grid-macos.png)).

**The one violation left is not a rack module, and I did not silence it.**
`MIDI In` 8x14 is TIDE's seeded root plumbing (`TideApp.cpp:727` — V3's
polyphony workaround, a root MIDI-CV feeding the rack module as a facade).
Teaching the probe that name would make it lie about what it measures, so the
measurement stands and the question — should root plumbing carry a panel rect
at all? — is filed as **S28**.

**The GATED half is its own PR** ([SynthEditLib#30](https://github.com/JeffMcClintock/SynthEditLib/pull/30)) — two constants and one line, shared with SynthEdit's
own rack mode, so it is reviewable on its own.

**Learned:**

1. **A constant that serves both a layout rule and a drawing rule will be
   changed for one and silently break the other.** `hpWidth` read as "the HP",
   and it was also the hole pitch. The tell was reading every consumer before
   editing the definition, which took one grep.
2. **The most expensive thing in this item was an assumption inside a probe.**
   `350` was arithmetic on a model nobody had checked against the renderer, and
   it had already produced a confident false verdict about TIDE's own shipped
   prefab.

**Next:**

1. **#239 is superseded by this branch** — it carried the same work stacked
   behind E16's unresolved ruling, and its probe held the wrong rail model.
2. E5's second clause (no overlaps) is now measurable on the ruled grid.

**Branch/PR:** `tide/mac/E5-rack-grid` (TideSynth) + [SynthEditLib#30](https://github.com/JeffMcClintock/SynthEditLib/pull/30) — merge together.

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

## 2026-08-20 — macos — #222: two of today's merges only ever built standalone, and SE16-hosted TIDE lost configure entirely

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · STEP 1, found while baselining C10

**Did:** un-broke every SE16-hosted configure of TIDE. Filed and fixed
[#222](https://github.com/JeffMcClintock/TideSynth/issues/222).

### The break, and why three verification surfaces all missed it

`cmake` on a fresh SE16 tree died at
`SynthEditSem/CMakeLists.txt:26: Unknown CMake command "tide_check_not_shadowed"`
— and behind it, a second identical break: `tide_render_strict_fp_sources`
(line 352). Two independent merges from today, one shape:

| merge | defines its commands in | SynthEditSem calls them |
|---|---|---|
| S17 ([#200](https://github.com/JeffMcClintock/TideSynth/pull/200)) | TideSynth **root** CMakeLists | lines 20/26/27 |
| E14 ([#209](https://github.com/JeffMcClintock/TideSynth/pull/209)) | `modules/common/` (also root-added) | line 352, plus `tide_render` itself at line 335 |

SE16 consumes TIDE via `add_subdirectory(${tidesynth_folder}/SynthEditSem ...)`
— **TideSynth's root never runs there**, so anything defined only in it does
not exist. TideSynth CI builds standalone (root first), SE16 CI runs on
dispatch only (S20), and the linux runs verified on clean TideSynth clones. A
whole configuration — the one Jeff's own dev builds use — had no owner.

E14's half is more than a function: SynthEditSem **links `tide_render`**, so
hosted mode was missing a target, not just a name.

### The fix, one mechanism per half

1. **S17's machinery** moved to `cmake/S17DependencyProvenance.cmake` —
   `include_guard(GLOBAL)`, the `TIDE_DEP_REPORT` FORCE-clear, both functions —
   included by the root **and** by SynthEditSem. Whichever entry point runs
   first defines everything once; the guard makes the second include a no-op,
   so the report still clears exactly once per configure.
2. **`modules/common`** is added by SynthEditSem itself when `tide_render` is
   not already a target. It is dependency-free by design (its own header
   comment), which is what makes adding it that early safe. Standalone mode is
   unchanged: root adds it first, the guard sees the target, no-op.

### Verified, both modes plus the control

| check | result |
|---|---|
| SE16-hosted configure (the break) | **rc=0** |
| full SE16-hosted build | **1072/1072, rc=0** — TIDE.gmpi, TIDE_VST3.vst3, TIDE_STANDALONE.app, SynthEditCL.app all produced |
| TideSynth standalone configure | rc=0, provenance block prints **9 resolved** |
| S17's shadow check still fires | override + planted `_deps/gmpi_ui-src` → **rc=1** with the S17 message |

The positive control matters: a refactor that moves a FATAL_ERROR check is one
typo away from a check that never runs, and "configure passed" cannot tell
those apart.

### What the newly-reachable test immediately caught — filed as S27

Restoring hosted configure made `ctest` in the SE16 tree include
`render_regression` for the first time anywhere: TideSynth's own root
force-disables `TIDE_RENDER_PREVIEW`, so no standalone or CI build has ever
run it. It **fails 5 of 5 scenes on mac arm64** — 34–66% of pixels moved
(limit 0.4%), worst deltas 53–107 (limit 40) — against references that are in
sync with their scenes (both `37d65d5`). **Contraction is eliminated:**
`-ffp-contract=off` leaves shapes/glass/glow bit-identical and all five still
failing, so this is not S19's mechanism. Dense, large differences point at
libm/arch divergence through the Monte Carlo render — labelled a hypothesis.
Filed as **S27** with the numbers; the fix here mirrors the root's
`TIDE_RENDER_PREVIEW OFF` into hosted mode so both modes build identically
(with that parity line, the full hosted suite is **67/67**; without env vars
for the S16-class test paths, 45 of 68 fail from a fresh tree — the #156
recipe applies on mac too when the tree is not at the box's usual path).

**Not done here:** `docs/lessons.md` is deliberately not regenerated on this
branch (A30's own precedent) — this branch races two open stacked PRs
([#218](https://github.com/JeffMcClintock/TideSynth/pull/218),
[#220](https://github.com/JeffMcClintock/TideSynth/pull/220)) that both
regenerate it, and a whole-file regen is the worst possible merge surface.
Whoever merges last runs `extract-lessons.py --write`. The `TIDE_DEP_REPORT`
newline-truncation warning CMake prints at cache-write time predates this fix
and is untouched (S17's design; harmless — the report is FORCE-cleared each
configure).

**Learned:**

1. **"Verified on this box" quietly became "verified in the only mode this box
   builds".** S17 and E14 were both real, both verified, both merged — and both
   only ever ran standalone. A repo consumed two ways needs its checks defined
   where BOTH entry points load them, and a function defined at one root is a
   standalone-only feature.
2. **The second break was hiding behind the first, again.** Configure stops at
   the first unknown command exactly like a fail-fast package probe; fixing
   `tide_check_not_shadowed` alone would have turned one red configure into
   two. Walking the chain to rc=0 before writing anything up is what found the
   missing `tide_render` target.

**Next:**

1. C10's baseline build is this fix's verification build — that item resumes
   on top of it.
2. S22 (the SE16-side provenance half) would have caught nothing here; the gap
   was TideSynth-side. Unchanged.

**Branch/PR:** `tide/mac/issue-222` — TideSynth only.
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

## 2026-08-20 — linux — A30: the lessons digest, and why the literal spec would have backfired

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Third item this session**, continued at Jeff's direction.

**Did:** built [docs/lessons.md](docs/lessons.md) — one line per lesson, every
run reads it, and **rotation cannot age it out** because
[scripts/extract-lessons.py](scripts/extract-lessons.py) generates it from
`JOURNAL.md` **and** `JOURNAL-2026-08.md`. Rotation gained a step 4 that
regenerates it, and the run prompt's reading list gained one line — that half is
the one A30 said wants Jeff's eye, and it is why this is a PR.

**Result — Accept met:** **599 lessons from 152 entries, 0 dropped**; the digest
is **56 KB against the 223 KB of Learned sections it distils, 4.1x smaller**.
`--check` fails when it is stale, so it cannot drift from the journals.

### The literal scope would have made things worse, and that is the finding

A30 says *"a standing, append-only digest that rotation copies each entry's
Learned bullets into"*. Copied verbatim that is **223 KB today — 24% of the
entire corpus** — added to what every run already reads. That is **worse than
the 192 KB that triggered A8 in the first place**, and it is the same trap A24
measured and rejected when it killed the 7-day journal floor.

So I measured before building, the way A24 did:

| approach | size | verdict |
|---|---|---|
| every Learned bullet, verbatim | 223 KB | worse than the problem |
| bold claim only, one line each | **56 KB** | shipped |

**What makes the cheap version work is the journal's own convention**: every
Learned bullet opens with a bold claim and then argues it, so the claim alone is
a real lesson rather than a title. That is a property of how this project already
writes, not something I imposed.

### The bug worth not repeating

The first cut read only markdown list items and reported **72 entries** — and
looked entirely complete while dropping **80 of the 152** entries that have
lessons. Three Learned shapes exist here and only one is a list:

```
1. **Learned:**  followed by "1." / "-" items          72 entries
2. **Learned:**  followed by one prose paragraph    }  the other 80
3. **Learned — headline.** **1. Claim...**          }
```

A second bug in the same pass: the no-bold fallback split on the first `.`, which
truncated inside inline code — `` `PKG_CONFIG_LIBDIR` pointed at a pruned copy of
the system ` `` was a real output line. This journal is full of `.pc` and
`foo.cpp:31`, so sentence-splitting has to ignore dots inside backticks.

**C15 — I reached the same finding as the macOS box, independently and hours later.** I checked C15 before starting (it was the topmost eligible row), found C16 had already closed it, and flipped it to DONE here. **[#202](https://github.com/JeffMcClintock/TideSynth/pull/202) landed the same conclusion first**, archived the row into `BACKLOG-DONE.md`, and filed **A31** for the underlying gap — *two ids, one job*, which A23's duplicate-id check cannot see by construction. **My duplicate edit is dropped from this branch**, per STEP 2's rule for a collision found after opening a PR; this branch is now a delta on top of theirs. The verification is not wasted, because it was done on merged `main` and agrees with theirs clause for clause: the only remaining includes resolve in the **public** `SynthEditLib`, `../SynthEdit2` survives only in comments, and the three `SynthEditApp` symbols appear only in comments. **Worth noting for A31:** two boxes spent a session each re-deriving one answer, and neither could see the other's row — the same shape as the collision itself.

**Learned:**

1. **A spec that says "copy X into a file every run reads" is a size decision in
   disguise, and it should be measured before it is implemented.** A30's own
   scope, followed literally, would have recreated A8 — the row that exists
   because this journal already grew past what runs could afford once.
2. **A generated index that silently covers half its input looks exactly like one
   that covers all of it.** The count came out plausible (72 entries, 306
   lessons) and nothing was obviously missing; only asking "of the entries that
   *have* lessons, how many are represented?" exposed it. Assert coverage against
   the source, not against your own output.
3. **Sentence-splitting on "." is wrong in any corpus that names files.** Dots
   inside backticks are the common case here, not the exception.
4. **This project's writing conventions are load-bearing infrastructure.** The
   digest is only affordable because Learned bullets already lead with a bold
   claim. A convention nobody enforced turned out to be the thing that made a
   mechanical distillation possible.

**Next:**

1. **The digest grows ~3.5 KB/day, ~100 KB/month** — 56 KB is affordable now and
   will not be by October. The lever is dropping the archive from the script's
   `SOURCES` once its lessons are spent, which halves it at a stroke. That is a
   judgement call and belongs to Jeff, not to a run.
2. **Nothing enforces that the digest is regenerated.** Rotation step 4 says to,
   and `--check` will catch a stale file, but no lint step runs it — adding one
   to `lint.yml` is a `.github/workflows/**` edit the token cannot push.
3. Unchanged: **the `apt-get` in `build.yml` is still the only thing between C7e
   and closed**, and CI filed a fifth issue (#195) for it while this ran.

**Branch/PR:** `tide/linux/A30-lessons-digest` — TideSynth only.

---

## 2026-08-20 — linux — S21 verified at runtime, and three things I got wrong

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Fifth item this session**, at Jeff's direction. It exists because he pushed back
on a claim I had repeated four times without rechecking.

### 1. The wayland claim was false, and I had the right data all along

I wrote, in the S21 entry and in two PR bodies, that `TIDE_STANDALONE`
*"cannot build on this box (wayland-client, xkbcommon, libdecor-0,
libpipewire-0.3 all absent)"*. Jeff: *"I have run synthedit wayland here, why no
wayland deps?"*

Measured:

| dep | this box |
|---|---|
| `wayland-client` | **1.22.0** |
| `xkbcommon` | **1.6.0** |
| `libdecor-0` | **0.2.2** |
| `wayland-scanner`, `wayland-protocols` | **present** |
| `libpipewire-0.3` | **missing — the only one** |

A normal configure says so in one line: `STANDALONE skipped -- missing:
libpipewire-0.3`. **That list of four was the CI runner's**, read out of the tree
I had deliberately crippled with a pruned `PKG_CONFIG_LIBDIR` to reproduce #190,
and then repeated as a fact about this machine.

**The failure was not a missing measurement.** My first probe this session
measured all three wayland modules as present. I overwrote correct data with a
number from a different experiment, and then used it to justify *not* verifying
two separate items — S21 shipped with "the plugin was never loaded", and I ruled
E14 not-takeable-here on the same premise.

### 2. So S21 is now verified by the reader, not by a directory listing

`sudo` needs a password no unattended run has, so rather than change the box:
`apt-get download libpipewire-0.3-dev libspa-0.2-dev`, `dpkg-deb -x` into a
scratch prefix, repoint `prefix=` in the two `.pc` files, point
`PKG_CONFIG_PATH` at them, and symlink `libpipewire-0.3.so` at the already
installed runtime `.so.0`. **TIDE_STANDALONE then builds and runs on Linux —
the first time it has.**

**Deterministic A/B, 3 runs each layout:**

```
post-fix : seeded=1  enriched=5  missing=0
pre-fix  : seeded=0  enriched=0  missing=6
```

With the fix: `TIDE: 6 rack prefab(s) seeded from the bundle`, and all five pin
XMLs enriched. Without: six `missing from bundle resources` lines, including
*"no Prefabs folder in bundle resources - the rack module browser will be
empty"*.

**And visually** — [before](docs/images/s21-prefabs-linux-before.png) /
[after](docs/images/s21-prefabs-linux-after.png). With the fix the browser's
**Prefabs** group lists Envelope, Filter, Midi, MidiCv, Oscillator, Output.
Without it **the Prefabs group is absent from the tree altogether**, and
`Sub-Controls` collapses from 27 classes to one (`Label`) because the XMLs never
load — the second half of S21, which I had only ever inferred.

### 3. A crash I reported before I had measured it

The first pre-fix run segfaulted, and I wrote *"the pre-fix layout segfaults"*.
**It does not.** 28 controlled runs — 8 per layout at 6s, 6 per layout at 12s,
both layouts — produced **zero** crashes. Two crashes were real, both on the
first run after relocating the resource directory with a 10s window, but the
correlation with the layout is unsupported. Filed as **S23** with what is and is
not known, unattached to S21. `gdb` is not installed here, so nothing names a
frame.

### 4. A merge commit of mine is authored as Jeff, and is already pushed

`72cc7c7` on `tide/linux/A30-lessons-digest` — I dropped the `GIT_*` exports on
the shell call that ran `git commit`, so the merge is stamped
`Jeff McClintock <jef@synthedit.com>`. `check-commit-authorship.py` reports it
and exits 0, by A26's design, because STEP 4 forbids rewriting anything already
pushed. **I have not rewritten it.** It is metadata rather than privilege — the
push authenticated as the bot, and authorship does not bypass a ruleset — but it
is exactly the misattribution A14 exists to prevent, and it is Jeff's call
whether to force-push a re-authored commit.

**Learned:**

1. **Data from a deliberately broken environment must be labelled at the moment
   it is written down.** The pruned-`PKG_CONFIG_LIBDIR` tree existed to answer
   "what does the CI runner lack?" — and its answer reads exactly like an answer
   to "what does this box lack?". Nothing in the log distinguishes them.
2. **A claim used to justify NOT doing work deserves more scrutiny than one used
   to justify doing it, not less.** "I can't verify this here" closed two items
   without review. It was wrong, and it was cheap to check.
3. **Report a crash with a rate, or don't report it as a consequence.** One
   observation became "the pre-fix layout segfaults" in the same message. 28 runs
   said otherwise.
4. **Export the identity in every shell that commits, not once per task.** Each
   Bash call is a fresh environment; the exports do not persist, and the failure
   is silent until the check prints it.
5. **`libpipewire-0.3-dev` is all that stands between this box and a working
   `TIDE_STANDALONE`**, and the download-and-extract route needs no root — so
   visual verification IS available on linux, which several rows assume it is
   not.

**Next:**

1. **E14 should be re-examined for this box.** I ruled it out because its
   authoritative check is *"place it in TIDE and look"*; that is now possible
   here, as the screenshots show.
2. **S23** needs `gdb` and a longer run window.
3. **Install `libpipewire-0.3-dev` properly** if the standalone is to be routine
   here — the scratch-prefix workaround is per-build and undocumented outside
   this entry and S23.
4. Unchanged: **the `apt-get` in `build.yml`** is still all that stands between
   C7e and closed; CI has now filed eight of those issues.

**Branch/PR:** `tide/linux/S21-runtime-verification` — TideSynth only.

---
## 2026-08-20 — macos — U2 was finished four days ago, and it was the last mac row

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Seventeenth item this session**, and taken with a concurrent agent running —
so: throwaway worktree only, claim pushed before any work, every change
committed as soon as it was coherent, `check-commit-authorship` and
`check-commit-completeness` on every commit. None of Jeff's trees were touched.

**Did:** closed **U2**, whose Accept had been met on 2026-08-16, and filed **A32**
for the gap that let it sit.

### The row had nothing in it

U2 asked for exactly one deliverable — *"a triage note naming root cause(s) and
the split; per-defect accepts land on the split rows."* Verified rather than
assumed:

- `docs/u2-triage-2026-08-16.md` — **163 lines**, exists
- **all five splits archived DONE with merged PRs**: U2a (gmpi_ui#7), U2b
  (gmpi_ui#8), U2c (SynthEdit#26), U2d (SynthEdit#27), U2e

**Four days stale — and it was the ONLY `mac`-marked row left in the queue.** A
run looking for mac work takes it and finds nothing to do. The failure is silent
and self-concealing: from outside, the queue looks like it has work in it.

### The obvious lint would be red on day one, and I measured that before proposing it

The rule writes itself: *flag any live row all of whose `X[a-z]` splits are
archived*. Run against the real tree it fires on **two** rows:

| row | verdict |
|---|---|
| **U2** | correct — nothing left in it |
| **E2** | **wrong** — a/b/c are done, but E2 is legitimately open; its remaining module stages are simply not filed yet |

It correctly does **not** fire on C7, whose splits include open ones.

**One real, one false: a 50% false-positive rate.** That is the same shape A23
and A27 were each nearly shipped with, and the same shape A24's proposed remedy
had. So A32 asks for an **advisory report** that never sets a non-zero exit,
not a gate.

**E2 is the interesting half of that false positive.** An umbrella with every
child done and more intended is, from the outside, indistinguishable from one
that is finished. That is a row-writing problem, not a lint problem, and A32 says
so.

**Learned:**

1. **Third stale-status row today** — C15 (duplicate), U2 (splits all landed).
   Different causes, one shared consequence: the queue advertises work that does
   not exist. A31 covers the first, A32 the second.
2. **Measuring a proposed lint against the live tree before writing it has now
   paid off four times today** (A23, A27, A24, A32). It is cheap, it is one
   command, and every time it changed the design.

**Next:**

1. **No `mac`-marked rows remain.** M1/M2/M3 and R3 are all BLOCKED; S9 is
   WONTFIX. Mac's queue is `any` rows or nothing.
2. **A31, A32** are the takeable process rows; **C10** wants `SynthEditLib`
   authority.
3. **The carve-out needs one `apt-get`** — [#189](https://github.com/JeffMcClintock/TideSynth/issues/189), Jeff's.

**Branch/PR:** `tide/mac/U2-close` — TideSynth only, backlog and journal.

## 2026-08-20 — macos — C15 was C16: two ids, one job, and a NEXT block pointing three runs at it

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Sixteenth item this session.** Repos synced first; the **linux box is awake
again** and holds A30 ([#198](https://github.com/JeffMcClintock/TideSynth/pull/198))
and S17 ([#200](https://github.com/JeffMcClintock/TideSynth/pull/200)), so
neither was taken here.

**Did:** took C15, found it already delivered, closed it, and filed the process
gap it exposed as **A31**.

### C15 and C16 are the same job under two ids

Every clause of C15's Accept, checked against `main` rather than assumed:

| C15 required | on `main` |
|---|---|
| `TideAppStubs.cpp` includes no private header | **0** hits |
| the three `SynthEditApp` symbols deleted | **0** |
| `SafeMessagebox` + `GetLicenseState` kept | **2** |
| `SynthEditSem`'s `../SynthEdit2` include gone | **0** |

**The windows box filed C15** while landing C14 (`fa75989`, [#177](https://github.com/JeffMcClintock/TideSynth/pull/177)).
**This box filed C16** for the same file, the same three symbols and the same
include-path deletion, hours later while landing C7b (`830c77c`) — from a branch
cut off the same `main`, where C15 was not yet visible.

**A23 solved the neighbouring problem and is blind to this one.** It detects
*one id, two rows*, which is what the two A17s were. This is *two ids, one job*,
and no id-based check can see it. Filed as **A31** with three candidate fixes and
a recommendation: the cheap habit — grep `BACKLOG.md` for the file you are about
to name — over a lint that can only catch rows citing a `path:line`.

**The cost was small only by luck.** C16 happened to land first, so C15 closed in
minutes. Filed the other way round, a run would have spent a session re-doing
finished work and discovered it at merge.

**One thing C15 got right and is worth keeping:** it predicted that C14 would
*not* close this half — *"narrowing a signature cannot remove a definition of
somebody else's member function"*. Correct, and C16 reached the same conclusion
independently by grepping.

### The NEXT block was pointing three rows at it

`check-next-block.py` — A27's own check — flagged the `win` and `any` cells, both
naming C15 as a take-target. **The windows box was about to take work that was
already done.** Without A27 that would have been a whole wasted run.

Re-pointed all three cells. And the *third* instance of a lesson I have now
written down twice and violated twice more in one sitting:

1. the superseded quote in `any` said "then **C15**" — flagged;
2. my replacement said "and then C15" — **flagged again**, because `then` before
   an ID is itself a take-verb;
3. deeper in the same cell, "if linux has it, take **C15**" — flagged a third
   time.

**Preserving NEXT-cell text verbatim is in direct tension with the check**, and
A22 already ruled which way that goes: superseded text loses its imperative. It
took three passes here because the cell is long and the phrases are buried.

**Learned:**

1. **The duplicate-work check that matters is not about ids.** A23 makes id
   collisions visible; nothing makes *job* collisions visible, and the branch
   model guarantees both. A31.
2. **Writing a rule down is not the same as being able to follow it.** I wrote
   A22's "superseded text must lose its imperative", then broke it twice in the
   same edit — once in text I had just written to explain the rule. The check
   caught all three; the habit caught none.

**Next:**

1. **A31** — the process fix, and the only thing this session leaves takeable
   that is not blocked.
2. **The carve-out is down to one workflow edit** ([#189](https://github.com/JeffMcClintock/TideSynth/issues/189)) — everything else in C7 has landed.
3. **U2** is the only `mac`-marked row left.

**Branch/PR:** `tide/mac/C15-duplicate-of-C16` — TideSynth only, backlog and journal.

## 2026-08-20 — linux — S21: the Linux bundle's resources were staged outside it

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Second item this session.** Jeff said "take next task" mid-run, which overrides
STEP 2's one-item rule; recording that here because otherwise the entry looks
like a run that helped itself to a second row.

**Did:** fixed the defect the previous entry filed. `SynthEditSem/CMakeLists.txt`
staged TIDE's bundle resources with `$<TARGET_FILE_DIR:tgt>/../Resources`,
commented *"the binary sits in Contents/<arch>/, so Resources is its sibling"* —
true on Windows, false on Linux, where `gmpi_plugin.cmake:853-861` links a bare
`.so` into the target directory and assembles the bundle around it afterwards.

**Linux needs TWO answers, not one**, and that is the part worth not
re-deriving. `BundleInfo::getBundleContentsFolder` (`BundleInfo.cpp:204`) walks
the loaded module's path for a `Contents` element and falls back to
`parent_path()` when there is none:

| format | loaded from | reader wants |
|---|---|---|
| VST3 | `TIDE_VST3.vst3/Contents/x86_64-linux/TIDE_VST3.so` | `…/Contents/Resources` |
| GMPI | a bare `TIDE.gmpi`, no `Contents` at all | `Resources` beside the binary |

So the old expression was wrong for both, by different amounts.

**The fix is a new `elseif(UNIX)` branch. The Windows expression is unchanged,
byte for byte, and that was the point** — S21 was found on Linux and Windows is
not verifiable from here, so it keeps its own branch rather than being retuned
by someone who cannot build it.

**Result:** fresh clone of the branch, full Release build — configure rc=0,
build rc=0, **zero error lines**. Accept clause met exactly as written:

```
TIDE_VST3.vst3/Contents/Resources/Prefabs/  6 .synthedit   + 5 XMLs beside it
build/SynthEditSem/Resources/Prefabs/       6 .synthedit   + 5 XMLs  (TIDE.gmpi)
build/Resources/                            NO LONGER EXISTS  (negative control)
```

**Verification artifact:** [tests/s21_bundle_resources_probe.py](tests/s21_bundle_resources_probe.py)
replicates the reader's own path algorithm and answers from its point of view,
so the check is not "a human looked at a directory listing". Run as an A/B with
a positive control:

```
pre-fix tree (main)  -> RESULT: FAIL   both formats, "EMPTY BROWSER"
post-fix tree        -> RESULT: PASS   both formats, prefabs=6 xml=5
```

The positive control is load-bearing here: a probe that only ever passes would
have looked identical and proved nothing.

**What this does NOT claim.** The plugin was never loaded. `TIDE_STANDALONE`
cannot build on this box (wayland-client, xkbcommon, libdecor-0, libpipewire-0.3
all absent) and there is no DAW here, so the claim is that the reader's computed
path now contains the files — not that a running TIDE listed six prefabs in its
browser. Someone on mac or windows can close that gap in seconds.

**Learned:**

1. **`$<TARGET_FILE_DIR>` is not inside the bundle on Linux**, unlike macOS where
   `$<TARGET_BUNDLE_CONTENT_DIR>` exists precisely because the linker writes into
   the bundle. Any "resources go next to the binary" reasoning has to ask which
   binary — the linker's output or the copy the bundle step made.
2. **A silent cross-repo disagreement needs a test written from ONE side.** The
   writer is in TideSynth and the reader is in SynthEditLib; each is internally
   consistent, which is why this survived. The probe deliberately encodes only
   the reader's algorithm.
3. **CI would not have caught this and still will not.** The matrix asserts
   compilation; nothing checks the staging step's output. A green Linux row says
   the platform builds, not that its module browser has anything in it.

**Next:**

1. **A mac or windows run should confirm its own platform still stages
   correctly** — neither expression changed for them, but the file did, and that
   is cheap to check with the same directory listing.
2. **The `SE_LOCAL_BUILD=TRUE` install copy is still wrong on Linux** and is left
   alone: `copy_plugin` copies to `~/.vst3` *before* these POST_BUILD steps run,
   the same ordering trap the APPLE block works around with `_tide_installed`.
   Not reproduced, because reproducing it means writing a plugin into Jeff's
   `~/.vst3`. Worth a row if anyone builds Linux with that flag.
3. Unchanged from the previous entry: **the `apt-get` in `build.yml` is still the
   only thing between C7e and closed**, and CI filed a fourth issue (#193) for
   the same cause while this ran.

**Branch/PR:** `tide/linux/S21-bundle-resources` — TideSynth only.

## 2026-08-20 — linux — #190: the Linux CI package set, measured

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Did:** took no backlog row. STEP 1 outranked it — `build.yml`'s matrix ran for
the first time overnight and filed three `platform:linux` failures
([#189](https://github.com/JeffMcClintock/TideSynth/issues/189),
[#190](https://github.com/JeffMcClintock/TideSynth/issues/190),
[#191](https://github.com/JeffMcClintock/TideSynth/issues/191)), all one cause.
The macOS run that triggered them diagnosed it correctly and said in as many
words that *"the exact package set wants checking on a real ubuntu box rather
than guessed at from the probe names"*. This box is that ubuntu box.

### The CI failure, reproduced exactly

No containers on this machine, so the runner was mirrored at the layer the
failure actually lives in: a `PKG_CONFIG_LIBDIR` holding every `.pc` on this box
**minus** the seven the CI log reported as not found. Configuring a clean clone
under it reproduces the failure to the line:

```
CMake Error at FindPkgConfig.cmake (message):
  The following required packages were not found:
   - xext
Call Stack:  .../gmpi_wrappers-src/wrapper/VST3/CMakeLists.txt:257
```

Same error, same file:line, same package as
[run 32329948996](https://github.com/JeffMcClintock/TideSynth/actions/runs/32329948996).

### The chain, walked rather than read

`pkg_check_modules(... REQUIRED)` fails fast, so the log names one missing
module and hides the rest — the trap the mac entry flagged twice in one day.
Restoring one `.pc` at a time and re-configuring:

| step | rc | missing | probe |
|---|---|---|---|
| 1 | 1 | `xext` | `VST3/CMakeLists.txt:257` |
| 2 | 1 | `harfbuzz` | `:260` |
| 3 | 1 | `dbus-1` | `:264` |
| 4 | **0** | — | — |

**Three packages, not one.** Each `.pc`→Debian mapping was read with `dpkg -S`
rather than guessed: `libxext-dev`, `libharfbuzz-dev`, `libdbus-1-dev`. A fourth,
`libpng-dev`, passes today only because the runner image happens to ship it.

### Linux builds — the first time TIDE has ever been built here

Clean `git clone` of the public URL, 158 files, then CI's own two commands with
only the fixed package set visible:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release    rc=0
cmake --build build --config Release --parallel   rc=0, 0 error lines  (76s)
  -> TIDE.gmpi      7,378,536 bytes  ELF 64-bit LSB shared object, x86-64
  -> TIDE_VST3.so   8,494,704 bytes  ELF 64-bit LSB shared object, x86-64
  -> TIDE_VST3.vst3/Contents/x86_64-linux/TIDE_VST3.so
350 objects; grep -ci se16 over configure log / build log / CMakeCache.txt = 0
```

So all three platforms are proven and **the `apt-get` is the only thing left in
C7e**. It is still Jeff's: the token has no `workflow` scope, by design.

### Relaxing the X11 probe is measurably wrong, not merely inelegant

`GMPI_Wrappers` is ALLOWED, so I could have made the probe optional and turned
CI green without anyone. mac declined this on principle; here it is a number.
`ldd` shows all three libraries linked, and the undefined-symbol counts show they
are used: **5 `XShm*`**, **50 `hb_*`**, **27 `dbus_*`**. An optional probe does
not yield a Linux VST3 with a lesser editor — it yields one that fails to link.
The Wayland trio genuinely is optional: without it configure prints *"Wayland
support off … X11 editor only"* and *"STANDALONE skipped"*, then succeeds.

### A separate defect this build found — filed as S21, not fixed

Building on Linux for the first time exposed something CI would never have
caught, because CI stops at "did it compile": **TIDE's resources are staged
outside the Linux bundle.**

`SynthEditSem/CMakeLists.txt:339` uses `$<TARGET_FILE_DIR:tgt>/../Resources`,
commented *"the binary sits in Contents/<arch>/, so Resources is its sibling"*.
On Linux it does not: `gmpi_plugin.cmake:849-861` links a bare `.so` in the
target dir and copies it into the bundle **afterwards**. Measured — the XMLs and
`Prefabs/` land in `build/Resources/`, while `TIDE_VST3.vst3/Contents/` holds
**only** `x86_64-linux/`. The reader disagrees explicitly
(`BundleInfo.cpp:296-299`): Linux resources live at
`<name>.vst3/Contents/Resources/`. Both formats are wrong by different amounts —
the bare `.gmpi` has no `Contents` in its path at all, so it wants `Resources/`
beside the binary.

Consequence is already spelled out in the source: `seedPrefabsFromBundle` prints
*"no Prefabs folder in bundle resources - the rack module browser will be
empty"*, and the five pin-description XMLs never load, which is the linked-but-
pinless failure the CMake comment records from V3.

**Filed, not fixed** — STEP 3 scopes me to one item, the build is rc=0 so this is
not the build break, and the expression is shared with Windows, which I cannot
compile on.

**Learned:**

1. **A fail-fast dependency probe costs one CI round trip per missing package,
   and the cheap fix is to walk the chain locally.** Reading the list from source
   got 6 of 7 names right but could not say which were actually absent on the
   runner; restoring them one at a time answered both questions in one pass.
2. **"CI is green" would not have caught S21.** The matrix asserts compilation;
   the prefabs are a packaging step whose output nothing checks. A green Linux
   row would have said the platform works while its module browser was empty.
3. **The runner's package set is partly luck.** `libpng` is satisfied by the base
   image, not by anything this repo declares — so it is one image bump away from
   being the next `xext`, diagnosed one name at a time all over again.
4. `PKG_CONFIG_LIBDIR` pointed at a pruned copy of the system `.pc` files is an
   accurate, seconds-long stand-in for a differently-provisioned machine, and it
   isolates the variable better than a container would have.

**Next:**

1. **Jeff: one `apt-get` step and C7 closes**, unblocking C10 and R2–R6. The
   verified block is in [docs/ci/linux-build-deps.md](docs/ci/linux-build-deps.md),
   ready to paste. #189/#190/#191 stay open until a green run closes them.
2. **S21** is the next linux-takeable row, and it is small.
3. C7b, C16 and C7d were flipped IN-REVIEW→DONE here on their merged PRs. Their
   rows were not moved into `BACKLOG-DONE.md`; that archiving is still owed.

**Branch/PR:** `tide/linux/issue-190` — TideSynth only; docs, backlog, journal.

## 2026-08-20 — macos — C7e: the clean clone builds; the CI clause is one apt-get away

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Fifteenth item this session.** C7d merged first.

**Did:** ran C7's real proof, and it splits cleanly in two.

### The clean clone builds — proven by the literal test

Not a worktree, not an override: `git clone` of the public URL into an empty
directory with **no sibling repos and no SE16 anywhere on the path**.

```
git clone https://github.com/JeffMcClintock/TideSynth   158 files
cmake -B build -G Ninja                                 rc=0
cmake --build build                                     rc=0, zero error lines
  -> TIDE.gmpi, TIDE_VST3.vst3, TIDE_STANDALONE.app
lipo -archs TIDE.gmpi/Contents/MacOS/TIDE               x86_64 arm64
TIDE_STANDALONE                                         runs, enriches XML, seeds prefabs
grep SE16 in cc logs / CMakeCache / build.ninja         ZERO
```

**Universal, not a single-arch dev build** — worth stating, because that is the
difference between "it compiles" and "this is the artefact you would ship".

**A stranger can now clone this repo and build TIDE.** That is what C1–C7 were
for, and it is done.

### The CI clause is not met, and the reason has nothing to do with the carve-out

C7e is written as *"build.yml's three platforms **run rather than skip**, and
pass, on a PR."* They run now (C7d) and **windows passes**. **Linux fails on
missing system packages.**

`GMPI_Wrappers/wrapper/VST3/CMakeLists.txt:249-263` hard-requires six pkg-config
modules on Linux — `x11`, `xext`, `fontconfig`, `freetype2`, `harfbuzz`,
`libpng`, `dbus-1` — and the ubuntu runner has none.

**`pkg_check_modules(REQUIRED)` fails fast, so the log names only `xext`.**
Fixing that one moves the failure to the next probe — the same shape as
`CoreMidiDriver.h` this morning, where CI printed one missing header and a second
was waiting behind it. The full list and a ready-to-paste step are on
[#189](https://github.com/JeffMcClintock/TideSynth/issues/189), which `build.yml`
filed by itself.

**So C7e is NEEDS-JEFF, not blocked on a decision:** the remaining step is an
`apt-get` in `.github/workflows/build.yml`, and the fleet's token deliberately
lacks `workflow` scope.

**I did not reach for the alternative**, and it is worth saying why. `GMPI_Wrappers`
is on STEP 5's ALLOWED list, so I *could* have made the X11 probe optional the
way the Wayland one already is. That would turn CI green by removing a real
requirement — a Linux VST3 with no editor — which is papering over the gap, not
closing it. The dependency is genuine; installing it is the honest fix.

**Learned:**

1. **"CI is green" and "a stranger can build it" are different claims, and C7e
   asks for the first while carve-out.md calls the second the real proof.** Both
   were worth measuring separately; only one landed.
2. **A fail-fast probe reports one missing dependency and hides the rest.** Twice
   today. When a required-package check fails, read the *whole* list from the
   source rather than the one name in the log.

**Next:**

1. **One `apt-get` step and C7 closes**, unblocking C10 and the release track
   R2–R6. It is Jeff's to push.
2. The macOS matrix job was still running when this was written — windows green,
   linux red, macos unknown. It builds a universal binary from scratch, so it is
   slow.

**Branch/PR:** `tide/mac/C7e-clean-clone` — TideSynth only, backlog and journal.

## 2026-08-20 — macos — C7d: TideSynth builds on its own

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Fourteenth item this session.** C7b and C16 merged first.

**Did:** wrote TideSynth's root `CMakeLists.txt`. **The repo now configures and
builds TIDE without SE16.**

```
cmake -B build -G Ninja        rc=0
cmake --build build            rc=0, 460 edges, 456 objects
                               TIDE.gmpi, TIDE_VST3.vst3, TIDE_STANDALONE.app
TIDE_STANDALONE                runs, seeds 6 prefabs, builds the rack
grep for SE16 paths            ZERO, in both the configure and build logs
```

That is C7's whole point, reached: a stranger with this repo and a compiler gets
a plugin.

### The one thing the row did not predict, and it cost the first configure

TIDE's `FORMATS_LIST` is `GMPI VST3 STANDALONE` — no AU, no CLAP. But
**`GMPI_Wrappers` configures its AU2 and CLAP wrappers unconditionally**, and
CMake needs those SDK sources to exist even for a target nothing links:

```
CMake Error at build/_deps/gmpi_wrappers-src/wrapper/AU2/CMakeLists.txt:95:
  Cannot find source file: /include/AudioUnitSDK/AUBase.h
```

So the AudioUnit and CLAP fetches are in TIDE's root purely to satisfy
configure. Copied from SE16 verbatim, with a comment saying why — the temptation
next time will be to delete them as "TIDE doesn't ship AU".

**More generally: the SDK fetches and the CPM bootstrap are copied close to
verbatim rather than reworked.** They are fiddly, not TIDE-specific, and
divergence between the two roots is the likeliest way to break a build here
without breaking one there.

### What is deliberately absent

`se_vst3` / `se_gmpi` / `se_au` (SynthEdit's own plugin engine), `EditorScreenshot`
(dropped by U3 with the breadcrumb bar), `SynthEditCL`, `SynthEditWayland`,
`tests`, and the desktop apps. TIDE's subset is SynthEditLib + EditorLib +
SynthEditSem, and that is all.

### This turns the CI matrix ON, and that is the point

`build.yml`'s guard job keys on a root `CMakeLists.txt` existing — *"the moment
C7 adds a root CMakeLists.txt the matrix starts running again with nobody having
to remember to remove anything"*. So all three platforms begin building on
`tide/**` pushes and PRs, **with no `.github/workflows/**` edit**, which the
fleet's token could never have made.

**macOS is proven. Windows and Linux are unproven and may go red on first
contact.** That is the mechanism working rather than a regression, and
`build.yml` files a platform issue automatically on the push run.

**Learned:**

1. **A subproject you do not use can still block configure.** Nothing links AU2
   or CLAP, and both had to be fetched anyway. "Which formats do I ship" and
   "which SDKs must exist for CMake to generate" are different questions.
2. **Copying a fiddly block verbatim beats improving it.** The CPM bootstrap and
   SDK fetches are near-duplicates of SE16's on purpose; the failure mode of a
   tidied-up copy is a divergence nobody notices until one root builds and the
   other does not.

**Next:**

1. **C7e** — the clean-clone test, now genuinely runnable. After it C7 closes,
   and C10 plus the release track R2–R6 unblock.
2. Watch the first three-platform matrix run: win/linux may need work, and the
   issues will file themselves.

**Branch/PR:** `tide/mac/C7d-root-cmake` — TideSynth only.

## 2026-08-20 — macos — C16: the last private include was three dead symbols

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Thirteenth item this session**, stacked directly on C7b.

**Did:** deleted `#include "SynthEditApp.h"` from
`SynthEditSem/TideAppStubs.cpp` — the last include in TIDE's own source that
resolved only inside the private repo.

### The row expected C14's treatment. Measuring first showed there was nothing to narrow

C16 was filed assuming the two member definitions —
`SynthEditApp::isMoonbaseEnabled()` and `::licenseIsActive()` — needed the
complete type and therefore an interface, the way C14 handled
`ApplySynthEditConfig.cpp`.

**They are not referenced anywhere in `EditorLib` or `SynthEditLib`.** C11 had
already replaced that path with `ILicenseState`, and `GetLicenseState()` —
returning `nullptr`, in this very file — is what EditorLib actually calls. Every
surviving caller of those members is desktop-app code (`SynthEditMac/`,
`SynthEdit2/`) which links the real `SynthEditApp.cpp` and never this file.

**So the fix is a deletion, not an interface.** All three symbols go: the two
members and the `theApp` global.

**Why the row got it wrong is worth naming:** the file's own header comment still
described the pre-C11 world — *"three symbols that EditorLib references"* — and
I wrote C16 from that comment. **A stale comment set the expected difficulty of
the work**, exactly as it did for A27, where `check-next-block.py`'s docstring
claimed a behaviour the code never had.

One include replaced it: `ILicenseState.h`, which had been arriving
*transitively* through the private header and lives in the **public**
`SynthEditLib`. The build error that revealed it (`unknown type name
'ILicenseState'`) is the useful kind.

### Verification

| | |
|---|---|
| objects | **943 — identical to C7b's baseline**, so nothing stopped being built |
| artefacts | `TIDE.gmpi` + `TIDE_VST3.vst3` |
| ctest | **86/86** |
| runtime | `TIDE_STANDALONE` runs, seeds 6 prefabs |
| `dangling_private_includes.py` | **3 → 2**, `SynthEditApp.h` gone |

**The two survivors are not real.** Both are `tinyxml/tinyxml.h`, which resolves
in the **public** `SynthEditLib` — on TIDE's include path via `SYNTHEDITLIB_DIR`
— and is only reported because `--public` was pointed at TideSynth alone, while
TIDE's public surface is TideSynth *plus* SynthEditLib. **So TIDE's own source
has zero real dependencies on the private repo**, which is what C7d needs.

`SE16_SYNTHEDIT2_DIR`, which C7b added an hour earlier, is deleted from both
CMakeLists — it existed solely for this include.

**Learned:**

1. **A deletion is a legitimate answer to "narrow this to an interface", and it
   is cheaper to check for than to build toward.** Two greps over EditorLib and
   SynthEditLib decided it before any code was written.
2. **Stale comments do not just mislead about behaviour — they set the expected
   SIZE of the work.** C16's row inherited its difficulty estimate from a comment
   describing a world C11 had already ended. Second time today (see A27).

**Next:**

1. **C7d** — the root `CMakeLists.txt` TideSynth does not have. Now genuinely
   unblocked: nothing in TIDE's source reaches into SE16.
2. Then **C7e**, the clean-clone test, and C7 closes.

**Branch/PR:** `tide/mac/C16-tideappstubs` in TideSynth and SynthEdit, **stacked
on C7b's branches** and to be merged after them.

## 2026-08-20 — macos — C7b: TIDE's own source leaves the private repo

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Twelfth item this session.**

**Did:** `SynthEditSem/` (16 files) and `TideModules/` (11) now live in
**TideSynth**. SE16 consumes them through a `TIDESYNTH_FOLDER_OVERRIDE` +
`FetchContent` pair mirroring `SYNTHEDITLIB_FOLDER_OVERRIDE` line for line — one
pattern to learn, and anyone who already sets the SynthEditLib override knows
what to do with this one.

### Accept, met exactly as the row wrote it

| clause | result |
|---|---|
| `git ls-files` in SE16 shows zero `SynthEditSem/` / `TideModules/` | **0 and 0** |
| SE16 still produces `TIDE.gmpi` **and** `TIDE_VST3.vst3` | both |
| same object count | **943, identical** to the baseline taken in the same tree immediately before |
| ctest green | **86/86** |

Plus: `TIDE_STANDALONE` runs, seeds **6** prefabs from the moved `TideModules`,
and the bundle stages all six.

### Only ONE of the three `../` paths actually broke

The row predicted three. Both `../TideModules/prefabs` entries **still resolve**,
because the two folders moved *together* — `TideSynth/SynthEditSem/../TideModules`
is `TideSynth/TideModules`. Only `../SynthEdit2` broke, and it is now
`${SE16_SYNTHEDIT2_DIR}`, set by SE16's root before `add_subdirectory` and empty
when TideSynth builds standalone.

**`SOURCE_SUBDIR docs` in the FetchContent block is deliberate**, the same trick
the SynthEditLib block uses: it points at a folder with no `CMakeLists.txt` so
FetchContent does not add the fetched tree as a subproject. It also survives C7d
adding a root `CMakeLists.txt` to TideSynth, which would otherwise start being
configured twice.

### Taken on mac, not linux

The NEXT block nominated linux. Linux has not run since 2026-08-19 and left **no
branch and no claim**, the row is platform `any`, and it gates C7e → C7 → C10 →
R2–R6. STEP 2's collision test is a remote branch or open PR naming the id, and
there was neither. The `linux` NEXT row now says so and points at C7d.

### The residual, filed as C16

`SynthEditSem/TideAppStubs.cpp:31` still includes the private `SynthEditApp.h`.
**C14's twin**, and the row records the asymmetry that matters: two of the three
symbols are **member definitions** (`isMoonbaseEnabled`, `licenseIsActive`, both
`return false`) which need the complete type, but the third — `SynthEditApp*
theApp = nullptr` — needs **only a forward declaration**, because a pointer
definition does not require a complete type and a global's mangled name carries
no type in the Itanium ABI. So the symbol is unchanged either way.

Until C16 lands, a *standalone* TideSynth build still cannot compile this target.
That is expected and is C7d/C7e's business, not a regression: C7b's Accept never
claimed otherwise.

**Also this run:** flipped **C14, A22, A23, A24** to DONE on their merged PRs;
re-specced **E5** and closed **S20** on Jeff's answers.

**Learned:**

1. **Moving two folders together is cheaper than moving one.** Every relative
   path *between* them survives untouched. The row's "three `../` paths" became
   one purely because `TideModules` travelled with `SynthEditSem`.
2. **A "same object count" acceptance clause is worth more than it looks.** 943
   before and 943 after says no target silently stopped being built — which a
   green build and a green ctest would both have tolerated, since a dropped
   optional target breaks neither.

**Next:**

1. **C16** — the last private include; after it, C7d and then the clean-clone
   test. Mac is taking it.
2. **A12** is the only A-series row left and is `.github/workflows/**`.

**Branch/PR:** `tide/mac/C7b-tide-source` in TideSynth and SynthEdit — **two
repos, and they MUST merge together.** SE16's default (no override) fetches
TideSynth's `origin/main`; until the TideSynth half is on `main`, that fetch
returns a tree with no `SynthEditSem/` and SE16's configure fails.

## 2026-08-20 — macos — A24: the journal floor is one DATE, because seven days measures 651 KB

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Eleventh item this session.** A22 and A23 both merged.

**Did:** changed the rotation floor from four entries to **the later of four
entries or every entry sharing the most recent date**, raised the trim target
30 KB → 60 KB, and changed the run prompt from *"read at least the last four
entries"* to **"read all of it"**.

### A24's premise is right and its remedy is wrong, by a factor of twenty

**Premise, re-measured and now sharper than when filed:** `JOURNAL.md` held
**five entries all carrying the same date**. A run obeying *"read the last four
entries"* saw under one day. A24 measured "about half a day" on 08-18; it has
got worse.

**Remedy refuted.** A24 asked to *"retain everything from the last 7 days"*.
Counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it. **Seven days is 3.4× the 192 KB
that triggered A8 in the first place** — the cure twenty times more expensive
than the disease. Even *two* days is worse than the state A8 was created to fix.

**So the floor is one date.** It bounds the cost at roughly a day's work while
guaranteeing a run sees everything that happened most recently, which is the
failure A24 correctly identified. On a quiet week the four-entry floor still
binds and nothing changes — the rule only bites on days like this one.

### What it does not fix — filed as A30

Rotation carries an entry's **Learned** bullets into the archive with it, and
**no run reads the archive**. So a lesson is load-bearing for about a day and
then silently stops being read. A24's one-date bound is the most that is
affordable, so the window cannot be widened to solve this: the lessons have to
leave the rotating file, into a standing digest.

This is not hypothetical. Twice today a run re-derived something an earlier entry
had already recorded, and this session is the shortest possible distance from
those entries.

**Learned:**

1. **Measure the remedy, not just the problem.** A24 was filed with careful
   numbers for the *premise* and an unmeasured guess for the *fix*. The guess was
   off by 20×, and nothing in the row's own reasoning would have revealed it —
   only counting the bytes did.
2. **A24 nearly cited a taken ID.** I wrote "filed as A25" and A25 has been in
   `BACKLOG-DONE.md` since 08-18. Caught by checking rather than by lint —
   `check-id-refs` validates that a *referenced* ID exists, and A25 does exist.
   The duplicate check shipped this morning would have caught the *row*, had I
   written one. Worth knowing that "the ID exists" and "the ID is free" are
   different questions and only one of them is linted.

**Next:**

1. **A30** — the lessons digest. Wants Jeff's eye on the prompt half.
2. **A12** is the only A-series row left, and it is `.github/workflows/**`.
3. **C7b / C7d** are the carve-out's critical path; **C14** is IN-REVIEW with all
   three PRs merged and should flip to DONE.

**Branch/PR:** `tide/mac/A24-journal-floor` — TideSynth only.

## 2026-08-20 — macos — A23: duplicate-ID detection, and the three false alarms that shaped the rule

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Tenth item this session.** A22 is IN-REVIEW on
[#183](https://github.com/JeffMcClintock/TideSynth/pull/183), green and waiting.

**Did:** `scripts/check-id-refs.py` now fails when one ID owns more than one row.
It already parsed every row ID; the change is that it records **locations**
rather than a set, because a duplicate is only actionable if the report names
*both* lines — renumbering means editing one of them.

### The naive rule was red on day one, and that is the finding

A first cut flagged **three** IDs in the live tree. **All three were
legitimate**, and each taught the rule something:

| flagged | what it actually is |
|---|---|
| `~~P8~~` (BACKLOG.md) | a **superseded row kept beside its replacement**. `RE_ID_CELL` tolerates the tildes *on purpose*, so references to it still resolve |
| `~~G3~~` (BACKLOG-DONE.md) | the same shape |
| `S1` ×2 (BACKLOG-DONE.md) | **a genuine, deliberate duplicate** — the linux and macOS boxes both took S1 on 2026-08-06, before the cron stagger took effect, and the second row says so in its own text |

So the rule is narrower than "one ID, one row":

- **Superseded rows are excluded from the duplicate test only**, not from the
  known-ID set. Both properties are needed and they are not the same property.
- **Archive-only duplicates are not flagged at all.** The archive is history,
  *"archiving never rewrites a row"*, and flagging S1 would demand an edit the
  rules forbid — on every run, forever.
- **What IS flagged is any duplicate touching `BACKLOG.md`:** two live rows (the
  A17 collision A23 was filed for), or one row in each file — an archive move
  that *copied* instead of moving, which makes the row's status ambiguous.

### Verification

Two positive controls rather than the one A23 asked for, because the second
shape only became visible while writing the rule:

```
duplicate row in BACKLOG.md   -> rc=1, names BACKLOG.md:76 and :77
copied into the archive       -> rc=1, names BACKLOG.md:76 and BACKLOG-DONE.md:20
the real tree                 -> rc=0
```

`--selftest` is **25 cases, 0 failed** (was 20), on real file bodies rather than
regex snippets because every subtlety here is about *which file* a row is in.
**The selftest is itself proven able to fail:** flipping one case's expectation
gives `FAIL duplicate/clean`, rc=1.

**Noted, not fixed:** `lint.yml` invokes this script **without** `--selftest`, so
those 25 cases run only by hand — exactly like `check-next-block.py`. Making CI
run them is a `.github/workflows/**` edit, so it needs Jeff either way.

**Learned:**

1. **Run a new lint against real history before believing it.** Three false
   alarms, zero of them predictable from the row's description — and A23's own
   text said the check was "a few lines on data it has already collected", which
   was true and still nearly shipped a rule that failed `main`.
2. **Two properties that look like one.** "Is this a known ID?" and "does this ID
   own a row?" differ precisely on struck-through entries, and the existing
   regex's tolerance of `~~` was deliberate for the first question. Reusing a
   collector for a second question is where that kind of assumption breaks.

**Next:**

1. **A24** — the last A-series row: the journal's rotation floor is counted in
   entries, so cross-run memory shrinks as cadence rises.
2. **S20** and the CI-on-push question are Jeff's; **U3's click path** is still
   unverified.

**Branch/PR:** [#184](https://github.com/JeffMcClintock/TideSynth/pull/184), branch
`tide/mac/A23-duplicate-ids` — TideSynth only.

## 2026-08-20 — macos — A22: the row names the branch, not the PR; and SynthEdit's CI never runs on push

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Ninth item this session.** S19 is fully closed —
[SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66),
[#67](https://github.com/JeffMcClintock/SynthEdit/pull/67) and
[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28) all
merged, [#178](https://github.com/JeffMcClintock/TideSynth/issues/178) closed.

**Did:** STEP 4 now says the backlog row must name your **branch**, with the PR
link a best-effort extra.

### Why (a)+(d) rather than either option the row offered

The old wording — *"mark the item IN-REVIEW with links to every PR you opened"* —
**cannot be satisfied in the commit that makes the mark**, because the PR does
not exist until after it. So it guaranteed a follow-up.

A22 offered (a) *name the branch* and (d) *accept the follow-up PR*. Taking (d)
alone would legalise the second PR rather than remove the need for it — and #120
showed the second PR is not the real cost. Its follow-up landed on a branch whose
PR had already auto-merged, which is **a pushed branch with no PR: the one end
state STEP 5 forbids.** So the new text adds a precondition A22 did not ask for:

> check the PR is still open before pushing the follow-up, and if it has already
> merged, **drop the commit** — `gh pr view <n> --json state --jq .state`

Pushing nothing is always safe. A commit whose only content is a link is never
worth a second PR, and the branch name in the row is what makes the follow-up
**optional rather than load-bearing**.

**The evidence is use, not argument:** this session ran the
branch-then-follow-up shape about eight times across three repos — A27, A28, A21,
S19, U3 — and never needed a second PR.

### Found while confirming S19: the CI chain never runs on push — filed as S20

`SE16 Kickstart Build` is **`on: workflow_dispatch:` and nothing else**.
`cmake_win` triggers off *it*; `cmake_mac` triggers off `cmake_win`. **So a push
to `master` runs no build and no tests.** The last dispatch was 2026-08-19T09:34
and `master` has moved eight times since with **zero** runs.

That is half the reason S19's five failures survived a week: `continue-on-error`
hid them, and the chain fired rarely enough that few people ever saw a log.

**The obvious fix is wrong, which is why it is a row and not a patch.** That
chain is a *release* pipeline — it signs, notarizes, staples and **FTP-uploads to
synthedit.com**. `on: push` would publish on every commit. What it wants is a
separate build-and-test-only workflow, and that is `.github/workflows/**`, so it
needs Jeff either way.

**I did not dispatch it.** Triggering that chain is a release, not a test run —
so today's mac fixes are verified locally (86/86) and remain unconfirmed in CI
until Jeff next kicks one off.

**Learned:**

1. **A rule that cannot be obeyed in one step will be obeyed in two, and the
   second step is where the damage is.** Nobody was going to skip linking the PR;
   they were going to link it badly. The fix was not to demand less but to move
   the required content to something knowable *before* the push.
2. **"CI is green" means nothing until you know what triggers CI.** I spent the
   session treating cmake_mac as the arbiter of mac health. It runs when a human
   asks it to, and it had not been asked since before any of today's work.

**Next:**

1. **A23** — duplicate-id detection in `check-id-refs.py`, the best-specced row
   left, with a positive control already written into it. **A24** after it.
2. **S20** and the `continue-on-error` follow-through both want Jeff.
3. **U3's click path** is still the one unverified thing from today.

**Branch/PR:** [#183](https://github.com/JeffMcClintock/TideSynth/pull/183), branch
`tide/mac/A22-pr-link` — TideSynth only, docs and backlog.


## 2026-08-20 — macos — the mac test drift is FMA contraction, and my own diagnosis was wrong first

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Seventh and eighth items this session**, on Jeff's instruction. Also closes
**U3** (shipped as [SynthEdit#65](https://github.com/JeffMcClintock/SynthEdit/pull/65)).

**Did:** diagnosed the four `TestVoiceAllocation` failures that S19 papered over
this morning with raised gates, and reverted those gates because they turned out
to be unnecessary. [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66).

### The hypothesis I wrote into three documents was wrong

This morning's row, issue and PR all said the error shape — max −68 dB against an
average −150 dB — looked like **a one-sample timing difference at voice
transitions**. Measured against the reference `.wav`, every part of that is false:

| claim | measurement |
|---|---|
| a handful of samples | **81.9% of all samples**, continuous from 0.127 s |
| a one-sample shift | shift 0 = −68.73 dB, shift ±1 = **−22.69 dB** — zero wins by 46 dB |
| (unstated) a gain error | best scalar fit **0.999999192**, residual unchanged |

It was a plausible story fitted to one summary statistic, and it survived into
three places because nobody had opened the file. **The average/max ratio I
reasoned from was the cancellation utility's own metric, not something I had
computed.**

### The actual cause

**FMA contraction.** clang defaults `-ffp-contract` to *on*, fusing `a*b+c` into
one `fma`. arm64 always has FMA; x86-64 under MSVC or GCC does not emit it by
default — **which is exactly why Windows and Linux reproduce the references and
macOS does not.**

It is *not* the Apple fast-math subset that was already in `CMakeLists.txt`. That
was the obvious suspect and it was eliminated this morning: with
`-fassociative-math` and `-freciprocal-math` removed, the four residuals were
**bit-identical**. Only contraction moved them.

```
test                        contract=on   contract=off
Unterminated_Poly_Modules   -80.77 dB     -90.31 dB
Voice_Allocation_Mono_High  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Last  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Off   -73.41 dB     -90.31 dB
```

**−90.31 dB is exactly 1 LSB at 16 bits** (`20·log10(1/32768)`), i.e. bit-identical
within the file format. Full suite with the strict gates restored: **3 failures
with contraction on, 86/86 with it off.**

So there was never a voice-allocation defect, and **the four gates raised this
morning are reverted to 85/75/75/75.**

### Checked rather than assumed

[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28)'s
soundfont scoping is **still load-bearing**: rebuilt with it reverted *and*
contraction off, `SoundfontOsc` still fails. Reassociation and contraction are
different mechanisms and neither fix makes the other redundant.

**Learned:**

1. **A hypothesis that explains the summary statistic is not a diagnosis.** Max
   ≫ average genuinely does suggest sparse differences — and the differences were
   dense. Ten minutes of `numpy` against the two files would have prevented three
   documents asserting it. **Open the artifact.**
2. **Eliminating the obvious suspect is worth more than confirming it.** This
   morning's A/B on `-fassociative-math` looked like a dead end — the figures were
   bit-identical, so the flags "weren't it". That negative result is what pointed
   at a *different* FP mechanism rather than a DSP bug, and it is why the second
   experiment was aimed correctly.
3. **`-ffp-contract` is invisible in a fast-math discussion.** It is not part of
   `-ffast-math`, is not mentioned by the flags this project already reasons
   about, and is on by default. On any arm64 target it is the first thing to check
   when a render differs from an x86-baked reference.

**Next:**

1. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178) can close once [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) merges** — except for the
   `continue-on-error` removal, which stays Jeff's.
2. **U3's click path is unverified** — one right-click on the rack background.
3. **A22, A23, A24** are the remaining A-series rows; **C7e** is unblocked from
   the `EditorScreenshot` direction now C7c is closed.

**Branch/PR:** `tide/mac/s19-fma-record` (this) + [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) (the code).

## 2026-08-20 — macos — C7c answered by removal, and the two questions that answer creates

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Sixth item this session**, on Jeff's instruction. Also closes **S19** — both its
PRs merged and macOS `ctest` is **100% of 86**; the `continue-on-error` removal
is still Jeff's and stays on [#178](https://github.com/JeffMcClintock/TideSynth/issues/178).

**Did:** recorded Jeff's C7c ruling and filed the work it implies as **U3**.

C7c asked whether `EditorScreenshot` should become public so a stranger's clone
can link it. **Jeff answered by deleting the need:** *"let's remove the
breadcrumb bar from TIDE, it's a bit redundant in a product where you seldom dig
deeper than 1 level in."*

That is the better answer than either option the row offered. TIDE's only uses of
`EditorScreenshot` are `SynthEditGui.cpp`'s `ContainerThumbnail.h` include and
the link line in `SynthEditSem/CMakeLists.txt`, and **both exist solely to draw
crumb thumbnails** — so the dependency leaves with the feature, nothing has to
come out of the commercial repo, and **C7e loses its last non-`SynthEditLib`
blocker.**

### Why the removal is U3 and NEEDS-JEFF rather than something I did

Reading the code before cutting found two things the ruling does not settle, and
either one guessed wrong ships a worse product than the bar:

1. **The crumbs are the only way back UP a level.**
   `breadcrumbBar->onNavigate` (`SynthEditGui.cpp:699`) is one of exactly **two**
   navigation entry points. The other, `seApp->onOpenContainerView` (`:703`),
   goes only *in*, or to the master via "Goto Structure…". I grepped
   `SynthEditLib`, `EditorLib` and `SynthEdit2`: **there is no existing
   go-to-parent affordance.** Remove the crumbs with no replacement and a user
   who opens a module is stranded in it.

2. **The About pane's only entry point is anchored to the crumb strip.** D6's own
   comment calls it *"the about pane and the only way in"*
   (`SynthEditGui.cpp:292-299`), a plain text affordance at the strip's right end.

U3 carries three options and recommends **(a) keep a thin strip with just
"◀ Back" and "About"** — the only one that changes no interaction the user
already has, while dropping exactly the part Jeff called redundant: the thumbnail
trail.

**`SE2::BreadcrumbBar` itself is not being deleted.** It is shared with the
WinUI3, Wayland, JUCE and Mac frontends; only TIDE stops using it.

**Learned:**

1. **"Remove the feature" can be the right answer to a licensing-boundary
   question, and it is not one an agent would have proposed.** C7c framed the
   choice as *which files move*; the cheapest answer was that none do. Worth
   remembering the next time a row's options list looks exhaustive.
2. **A one-line product decision can have load-bearing code underneath it.** The
   bar looked like a widget and is also the navigation model and the About
   pane's front door. Reading before cutting cost ten minutes.

**Next:**

1. **U3 wants Jeff's answer on Back and About**, then it is one session.
2. **C7e** is now unblocked from the `EditorScreenshot` direction; **C7b** and
   **C7d** are unchanged and still the linux box's.
3. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — the workflow edit, and the unexplained `TestVoiceAllocation` residual.

**Branch/PR:** `tide/mac/C7c-drop-breadcrumb` — TideSynth only, no code change.

## 2026-08-20 — macos — a mac build break from the carve-out, and five test failures CI has been hiding for a week

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (Claude Code CLI version not resolvable on this box) · as **tide-rack-bot** (both paths)

**Fourth and fifth items this session**, on Jeff's instruction — he pointed at a
failing Actions run rather than the backlog. Repos synced first.

## 1. The build break: [SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63)

`cmake_mac` had been red on `master` since `6c7e90053` while `cmake_win` and
`cmake_linux` were green at the same sha.

**The first thing to get right was WHICH step failed.** The 5 test failures in
that log are a decoy: `ctest` is `continue-on-error: true`, so **step 6 is marked
success**. The run failed at **step 20, the Xcode build**:

```
SynthEditMac/SynthEditMac/MidiAutomationWindowController.mm:3:10:
  fatal error: '../../SynthEdit2/PatchParameter.h' file not found
```

Two headers left `SynthEdit2/` during the carve-out and the Xcode consumer was
never re-pointed:

| header | moved by | now at |
|---|---|---|
| `PatchParameter.h` | `9a53a4882` — C12f, the patch cluster | `SynthEditLib/` |
| `IMidiDriver.h` | `4f6f5b1ca` — C13, the three orphan headers | `SynthEditLib/` |

CMake follows them because SE16's `CMakeLists` hands EditorLib the include
directories; the Xcode project quotes the paths literally. **This is exactly the
hazard C10's row already names** — *"Non-CMake consumers still need checking by
hand: `SynthEdit2.vcxproj` and the SynthEditMac Xcode project"* — so the carve-out
stages should treat that line as a checklist item, not a footnote.

**I fixed both files although CI named only one.** Xcode stops at the first fatal
error. Reverting both edits and rebuilding — with the `.mm` already compiled —
gives `CoreMidiDriver.h:6:10: fatal error: '../../SynthEdit2/IMidiDriver.h' file
not found`. Fixing only what CI printed would have turned one red run into two.

**Verified by building, twice**: at the original tip, and again after merging the
current `master` (`61eaf744b`, C14 landed, which changes EditorLib's include
directories so it was worth re-checking rather than assuming). Both times:
`cmake --build` **1064/1064 rc=0**, then `xcodebuild -scheme SynthEdit -arch
arm64 -configuration Release` → ***\*\* BUILD SUCCEEDED\****.

### A dev-box trap found on the way to that link

Reaching the link first produced
`Undefined symbols: ApplyConfigPreInit(SynthEditApp&, ...)`. Cause:
`libEditorLib.a` is built from `build/_deps/syntheditlib-src` (FetchContent,
**`GIT_TAG origin/main`** — a live ref), while the Xcode project resolves
`ApplySynthEditConfig.h` from the **sibling** `../SynthEditLib` clone. The clone
was behind C14, so the library exported the new `CSynthEditAppBase&` signature
and the header still declared `SynthEditApp&`.

**CI cannot hit this**: its *"Symlink CMake-fetched deps for Xcode"* step points
`../SynthEditLib` at `build/_deps/syntheditlib-src`, so header and library are one
tree by construction. On a developer box they are two trees tracking a moving
ref. Fixed by fast-forwarding the clone to `86ab11c`. **This is S17's shape one
level up** and worth knowing before it costs someone an afternoon.

## 2. The five test failures — filed as S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)

**They are not a regression, and they are not one bug.**

`94% tests passed, 5 tests failed out of 86` appears in *every* mac run I
checked, including 2026-08-13, 08-14 and 08-18 — **all of which are marked
success**. `continue-on-error: true` is why nobody knew.

| platform | at `6c7e90053` |
|---|---|
| Windows | 92/92 |
| Linux | 86/86 |
| **macOS** | **5 of 86 fail** |

Reproduced locally with figures **identical to CI's to four decimal places**, so
deterministic rather than flaky.

**The A/B that splits them.** `SynthEdit/CMakeLists.txt:281` adds, on Apple only,
`-fno-math-errno -fno-trapping-math -fno-signed-zeros -fassociative-math
-freciprocal-math`. Windows gets `/fp:fast`; **Linux gets no fast-math at all**,
which is consistent with Linux being the platform that passes. Rebuilding the
whole tree with the two reassociating flags removed and nothing else changed:

| test | with | without |
|---|---|---|
| `TestSoundfont.SoundfontOsc` | FAIL | **PASS** |
| `Unterminated_Poly_Modules` | −80.7666 dB | **−80.7666 dB** |
| `Voice_Allocation_Mono_High` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Last` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Off` | −73.407 dB | **−73.407 dB** |

So `SoundfontOsc` is FP reassociation, and **the four `TestVoiceAllocation` cases
are something else entirely** — bit-identical output either way, i.e. a real
deterministic mac-vs-reference difference the flags do not touch.

**Hypothesis, and labelled as one:** max −68 dB against an average of −150 dB
means a handful of samples differ, not a level or timbre — a constant offset
would put the average near the max. That is the shape of a one-sample timing
difference at voice transitions, which fits tests whose whole subject is voice
allocation. Unconfirmed.

**Not fixed on purpose.** Bumping four tolerances would turn the suite green and
throw the finding away, and whether −68 dB is acceptable in this product is not
an agent's call. The reporting half — removing `continue-on-error` — is a
`.github/workflows/**` edit the token structurally cannot push.

**Learned:**

1. **A `continue-on-error` step turns a failing suite into a decoy twice over.**
   It hid five real failures for a week, *and* it put a wall of red test output
   at the top of a log whose actual failure was fourteen steps later. Read the
   per-step conclusions before reading the log.
2. **"Last green run" is not a baseline when a step can fail without failing the
   run.** My first instinct was to bisect `9674bbfc7..6c7e90053`; the tests were
   already failing at `9674bbfc7` and in every run before it. Checking the older
   *green* runs' logs cost one command and saved a bisect that would have found
   nothing.
3. **An A/B that changes one flag is worth more than a plausible story.** The
   mac-only fast-math subset explained all five failures beautifully. It explains
   exactly one.

**Next:**

1. **S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — diagnose the four, rule on SoundfontOsc's tolerance, and
   Jeff removes `continue-on-error`.
2. **[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) is open and green** — mac `master` stays broken until it merges.
3. **A22, A23, A24** are the remaining A-series rows; A23 is the best-specced.

**Branch/PR:** `tide/mac/mac-ci-findings` (this bookkeeping) plus
[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) (the code).
Two repos; the SynthEdit half is the whole fix and this half is the record, so
neither blocks the other. Throwaway worktrees; every checkout left on its default
branch and clean.
