# Carve-out: making the shared core public

**Status: APPROVED by Jeff 2026-08-08 (BACKLOG C0). Stage 1 already done; work
starts at stage 2.**

Approved with one standing direction:

> **Keep as much `ExportAsPlugin` code private as practical.**

That is not a restatement of the existing boundary — it changes two things in
this document. See [What the direction changes](#what-the-direction-changes).

This is the migration that unblocks everything else. It touches a commercial
product's repo layout, so a weekly agent must not begin it unprompted — but C0
is now that prompt, and stages C2–C7 are TODO rather than BLOCKED.

## The problem in one paragraph

TIDE links two libraries: `SynthEditLib` (public repo, `C:\SE\SynthEditLib`)
and `EditorLib` (lives in the **private** `SynthEdit` repo at `C:\SE\SE16`).
`EditorLib` is where the structure view, document model, module browser and
properties browser live — i.e. most of what makes TIDE TIDE. So TIDE cannot be
built by anyone outside without that code becoming public.

## What EditorLib actually is

`SE16/EditorLib/` now contains exactly one file of its own: `CMakeLists.txt`.
(It had two until C2 moved `FuzzyMatch.h` out.) The library is assembled from
roughly 120 source files that live in `SE16/SynthEdit2/`, plus a growing number
already in `SynthEditLib`. So the carve-out is not "extract a library" — it is
"move ~120 files from the private repo into the public one, and repoint the
CMake paths".

That is mechanical, which is good news. It is also large, which means it should
happen in reviewable stages, not one commit.

## The commercial boundary

Jeff sells SynthEdit's ability to **export patches as plugins**. That capability
must stay private. Fortunately the seam is unusually clean:

`ExportAsPlugin` is a single free function — 2,470 lines in one file:

```cpp
// SynthEdit2/ExportAsPlugin.h
bool ExportAsPlugin(CSynthEditDocBase* doc, int autoSave = 0, std::wstring presetsFolder = L"");
```

Its only callers are:

| Caller | Repo | Fate |
|---|---|---|
| `SynthEdit2/MainWindow.xaml.cpp` | private (WinUI3 IDE) | stays private |
| `SynthEditCL/main.cpp` | private (CLI) | stays private |
| `SynthEdit2/CContainer.h` | **needs to be public** | see below |

`CContainer.h` mentions it twice — a plain declaration at line 23 and a friend
declaration at line 32 — and neither is a definition:

```cpp
friend bool ::ExportAsPlugin(CSynthEditDocBase*, int, std::wstring);
```

A friend declaration naming a function that is never defined in the build is
legal C++ and costs nothing. So `CContainer.h` can move to the public repo
**unchanged**, and `ExportAsPlugin.cpp` simply stays behind. No interface
shimming, no `#ifdef` maze, no null implementation needed.

The licensing code (Moonbase) is already outside `EditorLib` by design —
`EditorLib/CMakeLists.txt` notes that `SE_MOONBASE_SUPPORT` is deliberately not
defined there, and `SynthEditApp.cpp` is compiled separately by each consuming
app. That boundary already works and needs no change.

### Summary of the split

**Moves to public `SynthEditLib`:** the ~120 files in `EditorLib/CMakeLists.txt`
— document model (`DocOb`, `CContainer`, `CUG`, `SynthEditDocBase`,
`SynthEditDoc2`), structure view (`ModuleViewStruct`, `ConnectorViewStruct`,
`ContainerViewStruct`), browsers (`ModuleBrowser`, `PropertiesBrowser`), pin and
plug machinery (`Plug4`, `PlugIO4`, decorators), `commandMgr`, `SkinMgr`,
`ThemeManager`, `SynthEditAppBase`.

**Stays private:** `ExportAsPlugin.{cpp,h}`; the WinUI3 IDE shell
(`MainWindow.xaml*`, `ExportDialog.xaml*`, `BuildSkeletonDialog.xaml*`);
Moonbase licensing; `SynthEditCL`; the panel-view editor; anything else not on
EditorLib's list.

## What the direction changes

"Keep as much `ExportAsPlugin` code private as practical" sounds like a
restatement of the boundary above. It is not — checking it against the build
found a contradiction this document had, and a fact worth writing down.

### The plan as written would have published `ExportAsPlugin.cpp`

The "Stays private" list says `ExportAsPlugin.{cpp,h}`. But the "Moves to public"
list says *"the ~120 files in `EditorLib/CMakeLists.txt`"* — and those two files
are on that list, unconditionally:

```cmake
# SE16/EditorLib/CMakeLists.txt:113-114
${EDITOR_DIR}/ExportAsPlugin.cpp
${EDITOR_DIR}/ExportAsPlugin.h
```

Stage 6 then moves `EditorLib/CMakeLists.txt` itself into the public repo, which
would carry those entries with it. Followed literally, the staging publishes the
2,470-line export implementation — the one thing the carve-out exists to keep
back. Filed as **C1b**: strike them from that source list before stage 2 touches
it.

### The good news: TIDE does not ship the export code today

Verified on the Windows box, 2026-08-08, against the Release build:

```
0:000> x TIDE_VST3!*ExportAsPlugin*
          (nothing)
0:000> x TIDE_VST3!*SkinMgr*getSkin*
00000001`8017e5e0 TIDE_VST3!SkinMgr::getSkin
00000001`8017f100 TIDE_VST3!SkinMgr::getSkin
```

The control query proves symbol lookup works, so the empty result is real. The
object compiles (`ExportAsPlugin.obj`, 2,319,260 bytes in Release) and the symbol
is `External` in `EditorLib.lib`, but **the linker never pulls it into the
plugin**, because nothing in TIDE references it — the only public-side mentions
are declarations, which generate no reference.

So the commercial boundary already holds at link time, by accident rather than by
design. C1b turns the accident into a guarantee, and costs TIDE nothing: a file
that is not in the binary cannot be missed from the build.

### `SynthEditCL` stays private

That answers the second open question below. `SynthEditCL/main.cpp:1230` calls
`ExportAsPlugin` directly, so a public CLI means either publishing the export
code or splitting the tool in two. Both cut against the direction, and neither
buys TIDE anything it needs — TIDE embeds patches rather than exporting them.
Revisit only if contributors turn out to want the CLI badly enough to justify the
split.

### One correction while checking

`CContainer.h` carries **two** mentions, not one: a plain declaration at line 23
as well as the friend declaration at line 32. Both are declarations with no
definition in the public build, so the conclusion is unchanged — the header still
moves unaltered — but anyone grepping for a single line will be confused.

## Why this is low-risk for SynthEdit

After the move, SynthEdit consumes the same files from the public repo instead
of from its own tree. The build output is byte-for-byte the same code. The risk
is not correctness — it is that SynthEdit's core becomes readable by
competitors, and that future changes to shared files need a little more care
about which repo they land in.

The mitigating fact: **it is already mostly public.** `SynthEditLib` — the DSP
engine, the module hosting, the voice allocator — is already up there. This
carve-out adds the editor layer.

## Proposed staging

Each stage should build and pass tests before the next begins. Stage *n* below is
BACKLOG item **C*n***, except for the one precondition:

> **Before stage 2 — BACKLOG C1b: DONE 2026-08-08** (`SE16` `f313fe37e`). The
> export entries are gone from `EditorLib/CMakeLists.txt` (an explanatory
> comment stands where they were), and each of the four calling apps compiles
> the `.cpp` itself per the `SynthEditApp.cpp` precedent — including the
> SynthEditMac Xcode project and `SynthEdit2.vcxproj`, which consume EditorLib
> as a prebuilt library and so needed their own source-list entries. Verified by
> rebuild + `dumpbin` on Windows (no `?ExportAsPlugin@@` symbol left in
> `EditorLib.lib`); mac is edit-verified only. See the C1b row in BACKLOG for
> the full verification record, including the stale mac VST3-SDK pin the
> pre-landing review caught.

1. **Licence first — DONE 2026-08-07.** Add a LICENSE to `SynthEditLib` *before*
   moving anything new into it. Moving code into an unlicensed public repo does
   not make it open source. Resolved as **ISC**, matching GMPI and gmpi_ui;
   `SynthEditLib` `a2143a4`, TideSynth `a58a6f1`. (BACKLOG L1/C1.)
2. **Leaf files — DONE 2026-08-08** (`SE16` `d933e5e03`, `SynthEditLib`
   `6e49dbf`). `FuzzyMatch.h`, `checkpoint`, `cpu_accumulator`,
   `FrameRateLogger`, `imbedded_file` and the `it_*` iterators are in the
   **root** of `SynthEditLib`; `EditorLib/CMakeLists.txt` points at them there.
   Two corrections to how this stage was described, both of which change what
   stage 3 should do:

   - **"Files with no dependencies on the rest of `SynthEdit2`" was not the
     property that made them safe** — most of the `.cpp` include
     `CContainer.h`, `Application.h` or `PlugIO4.h`, all of which are still
     private until stages 3–5. What made them safe is that **nothing outside
     `EditorLib`'s own source list compiles them**, and every `#include` of them
     resolves through a search path rather than a relative path. Apply *that*
     test to stage 3's candidates.
   - **The destination was not specified, and root was chosen for a reason that
     expires.** Root is already an include directory in all three build systems,
     so 16 files moved with zero include-path edits. At 120 files a subdirectory
     is likely worth its three edits — decide it at stage 3 rather than
     inheriting root by default. Re-homing these 16 is a `git mv`.

   See the JOURNAL entry for 2026-08-08 for the rest, including the relative
   `#include` that had been resolving by luck, and the public file that was
   including a private header.
3. **Document model.** `DocOb`, `CContainer` (with its untouched friend
   declaration), `CUG`, `Plug*`, `SynthEditDocBase`, `SynthEditDoc2`. This is
   the biggest and riskiest stage; split it further if it resists.
4. **Views and browsers — IN-REVIEW 2026-08-13.** `ModuleBrowser`,
   `PropertiesBrowser`, `MfcDocPresenter`, `ModuleFactory_Editor`, `SkinMgr`,
   `ThemeManager` — twelve files, in `SynthEditLib`'s root like C2 and C3.
   PRs: [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15),
   [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6),
   [#49](https://github.com/JeffMcClintock/TideSynth/pull/49) — all three merge
   together. Verified: Ninja 905/905 RC=0 with five products, 92 tests across
   three suites all RC=0, and **SynthEdit2 (WinUI3) links** — the first stage to
   reach that step, which C1b and C2 both had to leave open behind P8.

   Two things this stage established that stage 5 should inherit:

   - **C9's mechanism now exists.** `SynthEditLib/se_version.h` defaults
     `SE_APP_BUILD_NUMBER` to 0 and `EditorLib/CMakeLists.txt` injects
     SynthEdit's real value from `se_build_number.h`, which stays where its
     three release workflows grep for it. `Application.cpp` — stage 5's own
     `se_build_number.h` user — needs one `#include` swap and one macro rename,
     not new machinery. The injection block reads a private-repo path, so **it
     cannot travel with `EditorLib/CMakeLists.txt` when stage 6 moves that
     file**; the block carries a comment saying so.
   - **Measure the dangling-private-include count on both sides, from git
     refs.** C4 closed 11 and opened 20 — net 47 → 56 — because the moved `.cpp`
     carry their own private dependencies in with them. Expect stage 5 to close
     far more than it opens (`Application.h` is the most-included name on the
     list), but measure rather than assume: computing "before" from a working
     tree the run has already edited hides the closures and flatters the change.
     Two of C4's twenty are on no stage's list at all and are filed as **C11**.
5. **App base — IN-REVIEW 2026-08-14.** `SynthEditAppBase`,
   `ApplySynthEditConfig`, `SynthRuntime_editor`, `UIoManager`, `IO_base`,
   `IO_None` — plus `Application`, which this list never named but which C9
   always assigned here. Fourteen files, in `SynthEditLib`'s root like C2, C3
   and C4. PRs: [SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16),
   [SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7) —
   both merge together. Verified: Ninja 905/905 RC=0 with five products, 92
   tests across three suites all RC=0, and SynthEdit2 (WinUI3) links.

   **C9 is now finished.** `Application.cpp` was its third and last live user;
   it takes `se_version.h` and `SE_APP_BUILD_NUMBER` exactly as C4's two did.
   Proven with a positive control rather than an absent error, which matters
   because `se_version.h` defaults the macro to 0 — a lost injection compiles
   fine and silently stops invalidating. Probe TU: **0** without the injection,
   **183** with.

   Two things stage 6 must inherit:

   - **This staging does not cover every file, and stage 6 is where that
     becomes a problem.** 41 `${EDITOR_DIR}` entries remain on EditorLib's
     source list after stage 5 and no stage owns them, so **stage 7's
     clean-clone test cannot pass** and stage 6 would move a `CMakeLists.txt`
     pointing at 41 files a stranger cannot see. Filed as BACKLOG **C12**, to
     be done *before* stage 6. The phrase "C3-C5 convert the rest", in this
     document and in the CMakeLists comment, was simply wrong.
   - **The private-include count fell for the first time: 59 → 54** (closed 15,
     opened 10), both sides read from git refs. Of the 10 opened, 6 are on a
     stage's list; four are not, and one is `SynthEditApp.h` — so **C11 now has
     two public-side call sites**, `MfcDocPresenter.cpp` from C4 and
     `ApplySynthEditConfig.cpp` from C5. It still needs Jeff.
6. **Move `EditorLib/CMakeLists.txt` itself** into `SynthEditLib`, so the public
   repo can build the editor library standalone. **Do C12 first** — see stage 5.
7. **Point TIDE at it.** TIDE's CMake consumes public `SynthEditLib` only, with
   no reference to `C:\SE\SE16`. At this point a stranger can build TIDE.

Verification after every stage: SynthEdit (WinUI3) builds, `SynthEditCL` builds,
TIDE builds, unit tests pass. Stage 7 additionally requires a clean-clone build
in CI with no access to the private repo — that is the real proof.

## Open questions for Jeff

- ~~**Licence for `SynthEditLib` and TIDE.**~~ **Answered 2026-08-07: ISC**, the
  same licence as GMPI and gmpi_ui. *(The question as originally written offered
  GPLv3 vs MIT/BSD and framed it as a competitive-moat decision. The answer went
  a third way — match the sibling repos — so do not read the old framing as
  having been weighed and rejected.)*
- ~~**Does `SynthEditCL` need to stay private?**~~ **Answered 2026-08-08: yes,
  stays private**, as a consequence of the "keep export private as practical"
  direction. Reasoning above.
- ~~**Repo naming.**~~ **Answered 2026-08-08: keep `SynthEditLib`.** The name
  stays as-is once the editor moves in. No rename, no redirect, no follow-up
  item — write it as `SynthEditLib` in build instructions, CI and the README
  without hedging.

**That closes every open question on this plan.** C0 is approved, the licence is
ISC, `SynthEditCL` stays private, the repo keeps its name. Nothing on the
carve-out is waiting on a decision — only on the work.
