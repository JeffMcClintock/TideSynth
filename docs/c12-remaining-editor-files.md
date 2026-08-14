# C12 — the 41 files the carve-out never staged

Scoping session, 2026-08-14, windows box. **This document is C12's deliverable
for that session; it does not move any file.** BACKLOG C12's own Size line says
the item is roughly 3× C5 and should name sub-stages rather than be attempted in
one go, so this splits it into six — **C12a–C12f** — each with its own scope,
acceptance check and size.

Everything below was measured from the working trees (all five clean and on
their default branches at the time), not read off the stage list. The scripts
are named at the end and are ~40 lines each; re-create and re-run them rather
than trusting these numbers after any stage lands.

---

## The short version

Three of the assumptions in C12's own row turn out to be wrong in C12's favour,
and one risk it did not name turns out to be the only part a Windows box cannot
verify.

| | |
|---|---|
| Entries on `EditorLib/CMakeLists.txt` still pointing at `${EDITOR_DIR}` | **41** (23 units, 9,791 lines) |
| Of those, files that actually need to **move** | **37** (9,010 lines) — four are dead or duplicate and are simply delisted |
| Private headers the 41 pull in that no stage owns | **zero** — the set is closed under inclusion |
| Real dangling private includes from the public repo into the 41 | **43** |
| `.vcxproj` / Xcode entries to edit | **none** |
| Files among the 41 that need MFC (`afxres.h`) | **none** |

The "closed under inclusion" line is the important one. C4 closed 11 dangling
includes and opened 20; C5 closed 15 and opened 10. **C12 opens none.** Every
`#include "..."` in all 41 files resolves either to the public repo, to the GMPI
SDK, or to another of the 41. So C12 is the stage that takes the count to zero
rather than trading it around, and no follow-up C11-shaped item can come out of
it.

---

## Three things that are not what the row assumed

### 1. `resource.h` does not need to move — there are already two of them

The row lists `resource.h` among the units to move. It should not be moved; its
line should be deleted.

`SE16/SynthEdit2/resource.h` and `SynthEditLib/resource.h` are both 361 lines
and differ in exactly two lines: one trailing space in a comment, and
`_APS_NEXT_RESOURCE_VALUE` (207 public, 210 private) — a Visual Studio
auto-counter inside `#ifdef APSTUDIO_INVOKED`. **Every `ID_*`/`IDR_*` constant
is identical.** They are not a C3 copy of one another: the public one goes back
to `SynthEditLib`'s initial commit, the private one arrived with SynthEdit's own
"move source out of the old SynthEdit folder" commit. They have been duplicates
for a long time.

This also corrects a measurement. A naive count says 71 public files reach the
private `resource.h`, which would make it C12's single biggest item. It is zero:
**65** of those 71 are `ug_*.cpp` DSP modules sitting in `SynthEditLib`'s own
root, and a quoted `#include` searches the includer's own directory first, so
they all get the public copy. The other **six** — `CContainer.cpp`, `CUG.cpp`,
`CUG_with_patches.cpp`, `DocOb.cpp`, `MfcDocPresenter.cpp`,
`SynthEditDocBase.cpp` — are in that same root for the same reason, C3 and C4
having moved them. **No public file reaches the private copy at all.**

The private copy is still live for the SynthEdit2-resident TUs among the 41,
by the same own-directory-first rule. So the check belongs in the last sub-stage
that moves a `.cpp` out of `SynthEdit2`: after it, those TUs resolve
`resource.h` to the public copy, which is a no-op only while the constants stay
identical. **They are identical today and that is the whole basis for this
being safe** — verify it again at that point rather than trusting this
paragraph.

**Filed separately, because it is not C12's to fix:** two divergent copies of a
generated resource header with no single source of truth is a hazard, and the
counter has already drifted by three slots. See [P9](#p9--two-divergent-copies-of-resourceh) below.

### 2. `GuiPin.h` and `Module_Info_Plugin` are dead

Both are on the source list; neither is reachable.

**`GuiPin.h`** — 393 lines, no `.cpp`, and **no includer anywhere** in `SE16`,
`SynthEditLib`, `SynthEditCL`, `gmpi_ui` or `GMPI_Wrappers`, including within
`SynthEdit2` itself. It could not compile if something did include it: three of
its own includes — `control_float_normalised.h`, `variable.h`,
`gui_default_variable.h` — exist nowhere in the current trees. They survive only
in the dormant `SE15` repo, under `OtherProjects/SynthEdit_1.0/`.

**`Module_Info_Plugin.{h,cpp}`** — 26 lines. The header says *"VST2 plugin.
Deprecated."*, the constructor is `protected` and commented `// serialisation
only`, no class derives from it, and the sole construction site is commented
out: `ModuleFactory_Editor.cpp:1320` reads `// meh: mi = new
Module_Info_Plugin();`. Its `getClassType()` returns 3 and nothing anywhere
tests for 3.

Both are C12a — delete four lines from the source list, move nothing. Whether
the files themselves should also be deleted from `SE16` is SynthEdit's call, not
the carve-out's; delisting is sufficient for C7 and is all C12a does.

### 3. `platform_editor.cpp` carries link-topology risk, and it is Linux-only

This is the one part of C12 the Windows box cannot fully verify, and the row
does not mention it.

`platform_editor.cpp` is 16 lines and defines nothing but three factory
functions — `new_InterfaceObjectA/B/C` — each returning an
`InterfaceObject_editor`. It is the seam that lets `SynthEditLib` call into the
editor layer without depending on it at compile time, and **three CMakeLists
carry a comment about it**:

> `SynthEditLib` and `EditorLib` are mutually-referencing static archives
> (`SynthEditLib` expects the app layer's `platform_editor.cpp` to provide
> `new_InterfaceObject*`), so GNU ld needs a rescan group.

— `SynthEditCL/CMakeLists.txt:57-61`, `SynthEditJuce/CMakeLists.txt:92-96`,
`SynthEditWayland/CMakeLists.txt:150-153`. (All three are comments; no target
other than `EditorLib` compiles the file.)

Moving `platform_editor.cpp` and `InterfaceObject_editor.*` into `SynthEditLib`
puts the provider and the expecting code in the same archive. That plausibly
*removes* the need for the rescan group — but "plausibly" is the problem: MSVC's
linker does not care either way, so a Windows box will see green whichever way
it lands, and the failure mode is a Linux link error in a repo the Windows run
never builds. **C12d is therefore the one sub-stage that needs the Linux box to
confirm**, and it is deliberately separated from the other leaves so that a
Linux failure does not block the twelve entries in C12c that carry no such risk.

---

## What the 41 actually depend on

By unit, edges within the set only (nothing else is left to depend on):

```
L0  Control  Ctl_Keyboard2  Dialogs_editor  GuiPin  IGuiHost
    InterfaceObject_editor  ModuleDragAndDropManager  Module_Info_Plugin
    SuspendDSP  legacyExternalApp  resource  ui_msg_target
L1  CLine2  Ctl_Combo  Ctl_Slider  Ctl_Text  commandMgr  platform_editor
SCC PatchManager <-> PatchParameter <-> PatchParameter_host_generated
    PatchManager <-> UG2
L2  CPlugin  (needs PatchManager; nothing needs CPlugin)
```

Eighteen of the 23 units are acyclic and independently movable. The remaining
five are the patch cluster: a genuine four-unit strongly-connected component
plus `CPlugin` hanging off it. **That cluster is 6,298 lines — 64% of C12 by
volume — and cannot be split**, which is what forces C12f to be a large stage
however the rest is arranged.

**Ordering between sub-stages is a convenience, not a constraint.** EditorLib's
include path carries both `${SYNTHEDITLIB_DIR}` and `../SynthEdit2`, so a moved
file may still include an unmoved one and compile; that is exactly the dangling
debt C4 and C5 each left behind and it did not stop either from building. The
order below is chosen to retire risk early and leave the immovable cluster last,
not because a different order would fail to build.

---

## The sub-stages

Each leaves SynthEdit, SynthEditCL, SynthEdit2 (WinUI3) and TIDE building with
tests green — the standing rule for every carve-out stage. Each also reports the
dangling-private-include count before and after, read from git refs, as C4 and
C5 both did. Files land in `SynthEditLib`'s root, like C2–C5; re-homing into a
subfolder is **C10** and stays blocked on C6.

| Stage | Entries | Lines | Closes | Plat | What |
|---|---|---|---|---|---|
| **C12a** | 4 | 781 | 0 | any | Delist dead and duplicate — nothing moves |
| **C12b** | 10 | 1,054 | 6 | any | The `Ctl_*` controls and `Control` |
| **C12c** | 12 | 1,316 | 21 | any | The independent leaves |
| **C12d** | 3 | 319 | 0 | **linux** | `InterfaceObject_editor` + `platform_editor` |
| **C12e** | 2 | 23 | 2 | any | `Dialogs_editor` — needs a placement ruling first |
| **C12f** | 10 | 6,298 | 14 | any | The patch cluster, atomic |
| | **41** | **9,791** | **43** | | |

### C12a — delist dead and duplicate

**Scope** Four lines in `SE16/EditorLib/CMakeLists.txt`: `${EDITOR_DIR}/GuiPin.h`,
`${EDITOR_DIR}/resource.h`, `${EDITOR_DIR}/Module_Info_Plugin.cpp`,
`${EDITOR_DIR}/Module_Info_Plugin.h`. No file moves and no file is deleted.
Update the stage comment at `:31-38` to point here.

**Accept** 41 → 37 `${EDITOR_DIR}` entries; a fresh Ninja tree still builds
905/905 RC=0 with the three test suites green. `Module_Info_Plugin.cpp` leaving
the list removes a TU from `EditorLib`, so this is a real (if small) link-surface
change and needs the build, not just a configure.

**Size** Minutes. Do it first — it makes every later stage's counting simpler,
and it is the only stage that cannot break anything.

### C12b — the controls

**Scope** `Control`, `Ctl_Combo`, `Ctl_Keyboard2`, `Ctl_Slider`, `Ctl_Text` —
`.cpp` and `.h` each. `git mv` into `SynthEditLib` root, repoint the ten
CMakeLists entries.

**Accept** Zero `${EDITOR_DIR}` entries named `Ctl_*` or `Control`; the six
dangling edges from `ModuleFactory_Editor.cpp` (4) and `CContainer.cpp` (2)
close; full build and tests green.

**Size** One session, comfortably. Cohesive and self-contained — `Ctl_Combo`,
`Ctl_Slider` and `Ctl_Text` depend only on `Control`, and `Ctl_Keyboard2` on
nothing.

### C12c — the independent leaves

**Scope** `CLine2`, `commandMgr`, `SuspendDSP`, `legacyExternalApp`,
`ModuleDragAndDropManager` (`.cpp` + `.h` each), plus the header-only
`ui_msg_target.h` and `IGuiHost.h`.

**Accept** Twelve entries gone; 21 dangling edges close — the largest single
reduction of any sub-stage, and more than C5 closed in total. Full build and
tests green.

**Size** One session. Deliberately excludes `InterfaceObject_editor` and
`platform_editor`, which look like leaves but are not — see C12d.

### C12d — the InterfaceObject factory (linux)

**Scope** `InterfaceObject_editor.{cpp,h}` and `platform_editor.cpp`.

**Accept** Three entries gone, **and the Linux link verified** — `SynthEditCL`,
`SynthEditWayland` and `SynthEditJuce` all link on GCC. If the rescan group in
those three CMakeLists is now redundant, say so with a measurement (build once
with it removed) rather than removing it on reasoning; if it is still needed,
leave it and record why. Windows and macOS builds green as usual.

**Size** One session, but **it must be the Linux box's session** — the risk is
invisible to MSVC's linker. Marked `linux` for that reason alone; the code is
platform-neutral.

### C12e — `Dialogs_editor` (needs a ruling first)

**Not eligible until the placement question below is answered.** The header
plainly belongs in `SynthEditLib` — `CUG.cpp:2635` calls `doDialogConnectUg` and
two public files include it. `Dialogs_editor2.cpp` is the question.

That file is 16 lines and defines the three dialog entry points with **empty
bodies**, the real implementations commented out under `// all obsolete?`. It is
one of *five* definitions of the same three functions, one per consuming app:

| definer | for |
|---|---|
| `SynthEdit2/Dialogs_editor2.cpp` | SynthEdit2 (the WinUI3 app), via EditorLib |
| `SynthEditCL/CLApp.cpp:14` | SynthEditCL |
| `SynthEditSem/TideApp.cpp:13` | TIDE |
| `tests/layouttests.cpp:26` | the layout tests |
| `EditorScreenshot/EditorCommandDispatcher.cpp:52` | (comment pointing at the above) |

**This works only by static-library semantics.** TIDE links `EditorLib`, which
contains `Dialogs_editor2.obj`, and also defines the same three symbols in
`TideApp.cpp` — no duplicate-symbol error occurs because the object file holds
nothing else, so the linker never has a reason to pull it in. Add one more
symbol to that file and TIDE stops linking.

So `Dialogs_editor2.cpp` is an *app-level* stub sitting in a shared library, and
the carve-out already has a settled pattern for exactly that: `SynthEditApp.cpp`
and `ExportAsPlugin.cpp` are both kept off EditorLib's list and compiled
directly by each consuming app.

> **PROPOSED: where does `Dialogs_editor2.cpp` go?**
> Options: (a) move it to `SynthEditLib` with the header, keeping today's
> arrangement; (b) take it off EditorLib's list entirely and let
> `SynthEdit2.vcxproj` compile it directly, matching `SynthEditApp.cpp`;
> (c) delete it — the bodies are already empty and three other consumers supply
> their own.
> Recommended default: **(b)** — it makes the accidental static-library
> behaviour into the deliberate arrangement the other two app-level files
> already use, and costs one `vcxproj` entry.
> Default in effect meanwhile: the file stays where it is and C12e is skipped;
> C12 reaches 39 of 41 entries, and C6 stays blocked.
> May proceed meanwhile: C12a, C12b, C12c, C12d and C12f are all identical
> under every option.
> Decide-by: before C6.

**Accept** (under any option) two entries gone, the two dangling edges from
`CUG.cpp` and `CUG_with_patches.cpp` closed, and **TIDE still links** — that is
the specific thing to check here, not a formality.

**Size** Minutes once ruled.

### C12f — the patch cluster

**Scope** `PatchManager`, `PatchParameter`, `PatchParameter_host_generated`,
`UG2`, `CPlugin` — `.cpp` and `.h` each, ten entries, 6,298 lines. Must move as
one commit: the first four are a strongly-connected component.

**Accept** Zero `${EDITOR_DIR}` entries remain on the source list — this is the
stage that satisfies C12's own top-level acceptance check. Fourteen dangling
edges close, taking the total from the 41 to zero. Full build, three test suites
green, SynthEdit2 (WinUI3) links. **Also re-check `resource.h` here** if C12f is
the last stage to move a `.cpp` out of `SynthEdit2`: those TUs switch from the
private copy to the public one, which is a no-op only while the constants match.

**Size** The largest single stage of the whole carve-out — larger than C3, C4
and C5 individually. Still one session, because it is a mechanical `git mv` of a
closed set with no include-path edits and no `.vcxproj`/Xcode work, but do not
combine it with anything else.

---

## What C12 does not cover

- **`SynthEditApp.h` is C11, not C12.** Its exclusion from EditorLib is
  deliberate (each app picks its own `SE_MOONBASE_SUPPORT` without ODR
  conflicts), it is a licence gate, and it needs Jeff. C12's row says the same;
  repeated here because it is the obvious thing to sweep in and must not be.
- **`IMidiDriver.h`, `ParseSynthEditArgs.h`, `ISEAppManaged.h`** — the three
  headers C5 measured as dangling and on no stage's list. C12's row says to
  absorb them. **They are not among the 41**, so no sub-stage above picks them
  up as a side effect; they need their own decision about which stage's list
  grows. Left as an explicit gap rather than quietly attached to C12f.
- **`ModulePicker.h`** — C11(b), same reasoning.
- **Re-homing out of `SynthEditLib`'s flat root** — C10, blocked on C6.

## New rows this session files

### P9 — two divergent copies of `resource.h`

`SynthEditLib/resource.h` and
`SE16/SynthEdit2/resource.h` are independently maintained near-duplicates whose
Visual Studio auto-counters have already drifted (207 vs 210). Every real
constant matches *today*, which is the only reason C12a is safe, and nothing
enforces that. Not C12's to fix and not blocking it. Scope: pick one as the
source of truth, or add a lint that fails when the `ID_*` sets diverge. Accept:
a deliberate divergence in one copy is caught. Size: minutes for the lint,
longer for the merge.

---

## Reproducing the measurements

Four scripts, each 40–80 lines, written to this session's scratchpad and not
committed — the C5 entry's advice, followed here, is that the next stage should
re-create them rather than trust a stale number.

| script | what it answers |
|---|---|
| `c12scope2.py` | forward edges (what the 41 pull in) and real reverse edges, with own-directory-first resolution |
| `c12live.py` | which of the 41 headers have an includer, and in which repo |
| `c12graph.py` | the internal dependency graph and its layering / SCC |
| `substages.py` | per-sub-stage entry, line and edge totals |

**The one bug worth knowing about**, because the first version of the analysis
had it: when a basename exists in *both* repos, membership of the 41 must not
outrank "the includer's own directory has a copy". Getting that wrong reports
`resource.h` as 71 dangling edges instead of 0, and turns a four-line delete into
the largest item in the stage. Any re-implementation should check its
`resource.h` number first — if it is not zero, the resolution order is wrong.

**Baseline for these numbers:** `SE16` `origin/master`, `SynthEditLib`
`origin/main`, both at 2026-08-14 with C5 merged. A fresh scratch Ninja tree of
`SE16` at that baseline configures RC=0 and builds **905/905 RC=0**, with
`dsp_tests` **58/58**, `synth_ui_tests` **24/24** and `ui_tests` **10/10**, all
RC=0. That is the starting state every sub-stage above is measured against, and
it is the same 905 targets and the same 92 tests C5 reported — so nothing has
regressed on `master` in the meantime.

One incidental confirmation from that configure: it printed
`EditorLib: SE_APP_BUILD_NUMBER=185 (from se_build_number.h)`. C5 measured 183,
so SynthEdit's build number has been bumped twice since and **C9's injection is
still tracking it** — worth noting because `se_version.h` defaults the macro to
0 and a lost injection fails silently, which is why C5 proved it with a control
rather than an absent error.
