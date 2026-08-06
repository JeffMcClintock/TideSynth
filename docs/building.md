# Building TIDE on Windows

Verified 2026-08-06 on the Windows machine (BACKLOG P1). Both Debug and Release
produced working binaries from a clean CMake configure.

TIDE has no build of its own — it is the `SynthEditSem` subdirectory of the
private `SynthEdit` repo at `C:\SE\SE16`, so you configure the whole SE16 tree
and build two targets out of it. That changes at carve-out stage C7.

## Prerequisites

| Thing | Version used | Notes |
|---|---|---|
| CMake | 4.2.0 | `CMakeLists.txt` requires ≥ 3.30. |
| Visual Studio | **18 Community**, MSVC 14.51.36231, toolset v145 | Must include the **MFC** component — see "The MFC trap" below. |
| Windows SDK | 10.0.26100.0 | |
| Git | any | FetchContent/CPM clone several SDKs. |

Dependencies are fetched automatically unless you point CMake at local clones.
On this machine they are all local:

| Variable | Local path |
|---|---|
| `SYNTHEDITLIB_FOLDER_OVERRIDE` | `C:/SE/SynthEditLib` |
| `GMPI_SDK_FOLDER_OVERRIDE` | `C:/SE/GMPI` |
| `GMPI_UI_FOLDER_OVERRIDE` | `C:/SE/gmpi_ui` |
| `GMPI_WRAPPER_FOLDER_OVERRIDE` | `C:/SE/GMPI_Wrappers` |

Still fetched from the network even with all four set: the VST3 SDK
(v3.7.14_build_55, via CPM into `%USERPROFILE%\.cpm`), HarfBuzz 14.2.1 (CPM),
CLAP and clap-helpers (FetchContent, into the build tree). A first configure in
a fresh build directory therefore needs internet and takes about a minute.

## The MFC trap — read this before you file a build bug

Two files that TIDE links through `EditorLib` include an MFC header on Windows:

- `SE16/SynthEdit2/CContainer.cpp:8` — `#include "afxres.h"` under `#ifdef _WIN32`
- `SE16/SynthEdit2/MfcDocPresenter.cpp:4` — same

`afxres.h` ships only with the Visual Studio **MFC** component, in
`VC\Tools\MSVC\<ver>\atlmfc\include`. It is on the default include path
whenever it exists, so nothing in the CMake files mentions it.

This machine has two VS 18 instances:

| Instance | MSBuild | `atlmfc` present? |
|---|---|---|
| `C:\Program Files\Microsoft Visual Studio\18\Community` | 18.8.2 | yes |
| `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` | 18.7.8 | **no** |

If you do not pin the instance, CMake picks **BuildTools**, and the build dies
with two errors and nothing else:

```
C:\SE\SE16\SynthEdit2\CContainer.cpp(8,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [<build>\EditorLib\EditorLib.vcxproj]
C:\SE\SE16\SynthEdit2\MfcDocPresenter.cpp(4,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [<build>\EditorLib\EditorLib.vcxproj]
```

The message is misleading — the header is not missing from the machine, the
build is just looking inside the wrong VS install. Pin the instance (below), or
install MFC into whichever instance CMake selects.

`CMAKE_GENERATOR_INSTANCE` cannot be changed in an existing build tree. If you
already configured against the wrong one, delete the build directory.

## Commands that work

From PowerShell. Substitute your own build directory.

Configure:

```bash
cmake -S C:/SE/SE16 -B C:/SE/build-tide-p1 -G "Visual Studio 18 2026" -A x64 -DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community" -DGMPI_SDK_FOLDER_OVERRIDE=C:/SE/GMPI -DGMPI_UI_FOLDER_OVERRIDE=C:/SE/gmpi_ui -DGMPI_WRAPPER_FOLDER_OVERRIDE=C:/SE/GMPI_Wrappers -DSYNTHEDITLIB_FOLDER_OVERRIDE=C:/SE/SynthEditLib
```

Build both plugin formats:

```bash
cmake --build C:/SE/build-tide-p1 --config Release --target TIDE TIDE_VST3
```

`TIDE` is the GMPI target, `TIDE_VST3` the VST3 one — see
`SE16/SynthEditSem/CMakeLists.txt:41` (`FORMATS_LIST GMPI VST3`).

Swap `Release` for `Debug` for a debug build; both were verified.

## Results, 2026-08-06

| Config | Exit | Warnings | Artifacts |
|---|---|---|---|
| Release | 0 | 0 | `TIDE.gmpi` 2.7 MB, `TIDE_VST3.vst3` 2.9 MB |
| Debug | 0 | 0 | `TIDE.gmpi` 10.2 MB, `TIDE_VST3.vst3` 11.6 MB |

Output lands in `<build>/SynthEditSem/<config>/`.

Wall-clock on this machine: configure ~20 s (fresh build tree, network),
Release build ~6 min from cold.

## Things that will bite you

- **Do not build with `SE16/build`'s settings by accident.** The developer's
  existing tree at `C:\SE\SE16\build` was configured against the Community
  instance, which is why it has always worked. A fresh tree is where the
  BuildTools default shows up.
- **`cmake --build` vs building one `.vcxproj`.** Running MSBuild directly on
  `EditorLib.vcxproj` picks the Community instance regardless of the cache, so
  it succeeds while `cmake --build` on the same tree fails. If you are chasing
  the `afxres.h` error, this difference will send you the wrong way — check
  `CMAKE_GENERATOR_INSTANCE` and `CMAKE_LINKER` in `CMakeCache.txt` first, they
  name the instance actually in use.
- **Building other targets pulls in the whole product.** `--target TIDE
  TIDE_VST3` builds only what TIDE needs (SynthEditLib, EditorLib, HarfBuzz).
  A bare `cmake --build` builds SynthEditCL, the tests and the rest too.
