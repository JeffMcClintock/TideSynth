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

## The four overrides are the normal path, not an optimisation

**Always pass all four.** These are not third-party libraries that occasionally
need a version bump — they implement a large part of the application's own
functionality and change daily, so the intended workflow is to build against
working copies you can edit, push and pull as you go. On this machine they are:

| Variable | Local path |
|---|---|
| `SYNTHEDITLIB_FOLDER_OVERRIDE` | `C:/SE/SynthEditLib` |
| `GMPI_SDK_FOLDER_OVERRIDE` | `C:/SE/GMPI` |
| `GMPI_UI_FOLDER_OVERRIDE` | `C:/SE/gmpi_ui` |
| `GMPI_WRAPPER_FOLDER_OVERRIDE` | `C:/SE/GMPI_Wrappers` |

**Omit one and it fails silently and permanently.** That dependency falls back to
`FetchContent` with `GIT_TAG origin/main`, which is a remote-tracking ref that
already resolves inside the cached clone — so CMake's update step sees "up to
date" and **never fetches again**. The checkout freezes at whatever `main` looked
like the first time you configured that tree, for the life of the tree.

That is not hypothetical: it is exactly how **X3** happened. `gmpi_ui` was
overridden and `GMPI_Wrappers` was not, so one sibling came from disk and the
other from a months-old clone, and the Linux box built and shipped a VST3 that no
host could load. Whether to pin the tags was considered and declined — see
BACKLOG **X4** — precisely because the override *is* the answer.

**Read the configure banner. It tells you which path every dependency took:**

```
-- Using local SynthEditLib folder
-- Using local GMPI folder
-- Using local GMPI-UI folder
-- Using local GMPI WRAPPERS folder
-- Fetching CLAP SDK from github
```

An unexpected `Fetching` on any of those four means you forgot a `-D` and are
building against stale code. If a build behaves impossibly and your source looks
right, confirm with `git log -1` in `build/_deps/<dep>-src` before trusting the
tree.

Genuinely fetched every time, and fine to leave alone: the VST3 SDK (via CPM
into `%USERPROFILE%\.cpm`), HarfBuzz (CPM), AudioUnitSDK, CLAP and clap-helpers
(FetchContent, into the build tree). These are stable third-party SDKs with no
override option, which is why a frozen checkout of them is harmless. A first
configure in a fresh build directory therefore needs internet and takes about a
minute.

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

## Current target and artifact names (N1a, 2026-08-22)

The canonical list. Older documents under `docs/` are **dated records** and name
the pre-rename artifacts on purpose — check the date at the top of one before
believing a name in it.

| | CMake target | shipped file |
|---|---|---|
| GMPI | `TIDE_Rack` | `TIDE-Rack.gmpi` |
| VST3 | `TIDE_Rack_VST3` | `TIDE-Rack.vst3` (bundle); the binary inside is `TIDE-Rack.so` on Linux |
| standalone | `TIDE_Rack_STANDALONE` | `TIDE-Rack` |

PDBs stay **target**-named — `TIDE_Rack.pdb`, `TIDE_Rack_VST3.pdb`,
`TIDE_Rack_STANDALONE.pdb` — which is deliberate, not an oversight (N1a).

The three forms never mix in one name: display **`TIDE Rack`**, shipped files
**`TIDE-Rack`**, CMake targets **`TIDE_Rack`**.

> **Linux exception, open:** the VST3 *bundle directory* is still target-named
> (`TIDE_Rack_VST3.vst3`) while the `.so` inside it took the shipped name, so the
> two disagree and a host scanning `~/.vst3` expects them to match —
> [#271](https://github.com/JeffMcClintock/TideSynth/issues/271). macOS and
> Windows are correct; only Linux builds the bundle path by hand.

Build both plugin formats:

```bash
cmake --build C:/SE/build-tide-p1 --config Release --target TIDE_Rack TIDE_Rack_VST3
```

`TIDE_Rack` is the GMPI target, `TIDE_Rack_VST3` the VST3 one — see
`SE16/SynthEditSem/CMakeLists.txt:41` (`FORMATS_LIST GMPI VST3`).

Swap `Release` for `Debug` for a debug build; both were verified.

## Results, 2026-08-06

**Artifact names below are pre-N1a and are left as measured** — this is a record
of that build, not a description of today's. The current names are
`TIDE-Rack.gmpi` and `TIDE-Rack.vst3` (`TIDE_Rack_VST3.vst3` on Linux until
[#271](https://github.com/JeffMcClintock/TideSynth/issues/271) is fixed).

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
- **Building other targets pulls in the whole product.** `--target TIDE_Rack
  TIDE_Rack_VST3` builds only what TIDE needs (SynthEditLib, EditorLib, HarfBuzz).
  A bare `cmake --build` builds SynthEditCL, the tests and the rest too.

## The two-target trap (P11): `TIDE_Rack_VST3` alone ships a half-built install

Building only `--target TIDE_Rack_VST3` leaves the *module database* stale: the
`SE *` GUI modules resolve through the installed `TIDE-Rack.gmpi`, which is
written by the **`TIDE_Rack` target's** post-build step, not by `TIDE_Rack_VST3`'s.
On Windows the stale-DB symptom is a modal *"required module is missing
from the module database"* that blames the user's installation; the fix is
building both targets (P11 tracks making this self-consistent or at least
honestly diagnosed).

### macOS: the build installs to a folder the scanner never reads

**Corrected 2026-08-22 (macos) — this section previously said "there is no
install step at all" on macOS. That is wrong, and wrong in a way that costs
time: it sends you off to add an install step that already exists.** There is
one. It installs to the wrong domain.

| | path | who writes it |
|---|---|---|
| `SE_LOCAL_BUILD` installs to | `~/Library/Audio/Plug-Ins/GMPI` | `copy_plugin()`, `GMPI/gmpi_plugin.cmake:1225` |
| the module scanner reads | `/Library/Audio/Plug-Ins/GMPI` | hard-coded, see below |

The scan root is a string literal, not a domain lookup:
`getPlatformPluginsFolder()` returns `"/Library/Audio/Plug-Ins/"`
(`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:152-164`), `"GMPI"` is
appended in `SynthEdit/SynthEdit2/SynthEditApp.cpp:155-164`, and that one path
is what `RefreshModuleData` scans (`SynthEditLib/EditorLib/Application.cpp:544`).
**There is no `NSSearchPathForDirectoriesInDomains` anywhere in the scan path**,
so unlike VST3 and AU — which search user *and* system by convention — the
user domain is never consulted.

**Measured, not inferred.** Across every `Plugin-Cache-16-override-*.xml` in
`~/Library/Application Support/SynthEdit/`, all 602 recorded module paths are
under `/Library/Audio/Plug-Ins/GMPI`. **Zero** user-domain paths have ever been
recorded, in any cache file, at any date — so this is not a stale-cache
artifact.

So on macOS a locally built module is installed, and invisible. Copy it across
to make a build visible to SynthEdit:

    cp -R build/SynthEditSem/Release/TIDE-Rack.gmpi "/Library/Audio/Plug-Ins/GMPI/TIDE-Rack.gmpi"

**That may need `sudo`.** On this machine the folder happens to be owned by the
developer and the plain `cp` works; on a fresh machine it is root-owned, which
is why SynthEdit's own CI runs `sudo mkdir -p` and `sudo chmod 777` on it
(`SynthEdit/.github/workflows/Export_Tests_mac.yml:34-35`). Check with
`ls -ld /Library/Audio/Plug-Ins/GMPI` rather than assuming either way.

**Who this actually affects.** *Not* TIDE. TIDE does no module scan at all —
S1a removed it, and the module browser reads a force-linked in-memory list
(`SynthEditSem/TideApp.cpp:434-442`, `SynthEditLib/UgDatabase.cpp`). This bites
**SynthEdit the editor** when it consumes TIDE as a third-party module, which is
the configuration P11's Windows symptom was found in.

**One consequence of the N1a rename to watch for:** any
`/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` left over from before the rename is now
a permanent orphan — the build emits `TIDE-Rack.gmpi`, so nothing will ever
update the old name again, and the scanner will keep serving whatever was last
copied there. Delete it rather than letting it shadow the current build. Both a
system-domain (2026-08-16) and a user-domain (2026-05-07) copy were found on the
mac box.

(Note: the stale DB was ruled out as the cause of U2d's blank panels — see
that row — so do not expect this copy to fix rendering; it fixes the
export/module-database path P11 describes.)

## `SE_LOCAL_BUILD` — why your build stopped installing itself (macOS)

The plug-in bundle is copied to `~/Library/Audio/Plug-Ins/VST3` by a POST_BUILD
step in GMPI's `gmpi_plugin.cmake` that sits inside **`if(SE_LOCAL_BUILD)`**.
That option is declared `FALSE` by default (`SynthEdit/CMakeLists.txt`), so a
developer machine only auto-installs because someone set it once and the value
lived in `CMakeCache.txt` ever after.

**Anything that resets the cache silently disables the auto-install** — most
easily `cmake --fresh`, which is otherwise the right tool for forcing a genuine
from-scratch configure. The symptom is confusing rather than loud: the build
succeeds, the bundle in the build tree is current, and **the host keeps loading
the previous binary**, so you appear to be testing a change that was never
deployed. Verified on 2026-08-17 while re-testing S12 against upstream.

Re-enable it with:

```bash
cmake -DSE_LOCAL_BUILD=TRUE .
```

Two habits that make this cheap to catch: compare the timestamp of
`build/SynthEditSem/Release/<name>.vst3/Contents/MacOS/<name>` against the
installed copy before believing a host test, and remember that
`--fresh` also resets **every** other cached option you rely on (the two
`GMPI_*_FOLDER_OVERRIDE` paths and the generator among them — a `--fresh`
without `-G Xcode` will quietly switch an Xcode tree to Unix Makefiles).
