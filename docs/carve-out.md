# Carve-out: making the shared core public

**Status: proposed, not started. Needs Jeff's approval before any file moves.**

This is the migration that unblocks everything else. It touches a commercial
product's repo layout, so a weekly agent must not begin it unprompted.

## The problem in one paragraph

TIDE links two libraries: `SynthEditLib` (public repo, `C:\SE\SynthEditLib`)
and `EditorLib` (lives in the **private** `SynthEdit` repo at `C:\SE\SE16`).
`EditorLib` is where the structure view, document model, module browser and
properties browser live — i.e. most of what makes TIDE TIDE. So TIDE cannot be
built by anyone outside without that code becoming public.

## What EditorLib actually is

`SE16/EditorLib/` contains only two files of its own: `CMakeLists.txt` and
`FuzzyMatch.h`. The library is assembled from roughly 120 source files that
live in `SE16/SynthEdit2/`, plus a handful already in `SynthEditLib`. So the
carve-out is not "extract a library" — it is "move ~120 files from the private
repo into the public one, and repoint the CMake paths".

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

`CContainer.h` line 32 contains only a friend declaration:

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

Each stage should build and pass tests before the next begins.

1. **Licence first.** Add a LICENSE to `SynthEditLib` *before* moving anything
   new into it. Moving code into an unlicensed public repo does not make it
   open source. (BACKLOG L1 — Jeff's call, not an agent's.)
2. **Leaf files.** Move the files with no dependencies on the rest of
   `SynthEdit2`: `FuzzyMatch.h`, `checkpoint`, `cpu_accumulator`,
   `FrameRateLogger`, `imbedded_file`, the `it_*` iterators. Repoint
   `EditorLib/CMakeLists.txt` at the new locations. Confirm SynthEdit still
   builds.
3. **Document model.** `DocOb`, `CContainer` (with its untouched friend
   declaration), `CUG`, `Plug*`, `SynthEditDocBase`, `SynthEditDoc2`. This is
   the biggest and riskiest stage; split it further if it resists.
4. **Views and browsers.** `ModuleBrowser`, `PropertiesBrowser`,
   `MfcDocPresenter`, `ModuleFactory_Editor`, `SkinMgr`, `ThemeManager`.
5. **App base.** `SynthEditAppBase`, `ApplySynthEditConfig`,
   `SynthRuntime_editor`, `UIoManager`, `IO_base`, `IO_None`.
6. **Move `EditorLib/CMakeLists.txt` itself** into `SynthEditLib`, so the public
   repo can build the editor library standalone.
7. **Point TIDE at it.** TIDE's CMake consumes public `SynthEditLib` only, with
   no reference to `C:\SE\SE16`. At this point a stranger can build TIDE.

Verification after every stage: SynthEdit (WinUI3) builds, `SynthEditCL` builds,
TIDE builds, unit tests pass. Stage 7 additionally requires a clean-clone build
in CI with no access to the private repo — that is the real proof.

## Open questions for Jeff

- **Licence for `SynthEditLib` and TIDE.** GPLv3 keeps competitors from taking
  the editor closed-source; MIT/BSD maximises adoption but lets anyone ship a
  rival. This choice is yours and blocks stage 1.
- **Does `SynthEditCL` need to stay private?** It is a useful tool for TIDE
  contributors, but it calls `ExportAsPlugin`. It could be split into a public
  build without export and a private build with it — worth doing only if you
  want contributors to have the CLI.
- **Repo naming.** Once `SynthEditLib` contains the editor too, the name is a
  little off. Renaming is cheap now and expensive later.
