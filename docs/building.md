# Building TiDE Rack

Windows-first, with the macOS-specific traps in their own sections near the
bottom. Originally verified 2026-08-06 (BACKLOG P1); the standalone path below
re-verified end to end on 2026-08-24.

## Two independent choices, not one fork

**Corrected 2026-08-24 (windows, interactive) — twice, and the second correction
matters more than the first.**

**(1)** This file opened by saying *"TIDE has no build of its own … That changes
at carve-out stage C7"*, in the present tense, for the three days after C7
closed. It is the first thing a reader sees, and the only recipe the file gave
needed the private `SE16` repo — so someone with a fresh public clone would have
concluded they could not build at all.

**(2)** The first attempt at this section then replaced that with a *different*
wrong claim: a two-row table pairing "standalone superproject" with *fetched*
siblings and "via SE16" with *local* ones, as though you had to take them
together. **You do not.** Jeff, reading the draft: *"if I'm building on the dev
box, why would I clone tide as a dependency and not use local copy?"* Quite —
and the answer is that you would not, and nothing stops you.

**These are two separate choices. Pick one from each column.**

| choice | option | what it means |
|---|---|---|
| **which root** | `C:/SE/TideSynth` | TiDE's own `CMakeLists.txt`, added at carve-out stage **C7d**. Needs no `SE16` on disk. **Use this unless you want SE16's other targets.** |
| | `C:/SE/SE16` | the whole SynthEdit tree. Only needed when you also want `SynthEditCL`, `SynthEdit2` or SE16's own test suite in the same build. |
| **where the siblings come from** | **the four `*_FOLDER_OVERRIDE` variables** | your local working copies at `C:/SE/…`, which you can edit, push and pull as you go. **This is the dev-box default.** |
| | omit them | each sibling is fetched fresh from its own `main`. |

**On this machine, the default is the TideSynth root WITH all four overrides** —
verified 2026-08-24, configure rc=0 in 11 s reporting `[local override]` for
SynthEditLib, GMPI, gmpi_ui and GMPI_Wrappers.

**The fetch-everything build is not a second way to develop; it answers exactly
one question:** *does this build without my uncommitted local work?* That is
what CI runs and what a stranger's clone does, so it is the right check before
tagging a release or when a green local build and a red CI run disagree. It is
the wrong default for daily work, because every sibling change you have not
pushed is invisible to it.

**`GMPI_WRAPPER_FOLDER_OVERRIDE` is declared in `SynthEditSem/CMakeLists.txt:3`
rather than the root**, unlike the other three — it still works from the command
line, it is just not where you would look for it.

**`VST3_SDK_FOLDER_OVERRIDE` is used but never declared** in the root's cache
block, so it is settable only via `-D` and invisible to `cmake-gui`. Recorded on
BACKLOG **S22**, which fixed the same gap in SE16 and left this one filed.

The original opening, for the record: *"TIDE has no build of its own — it is
the `SynthEditSem` subdirectory of the private `SynthEdit` repo at
`C:\SE\SE16`, so you configure the whole SE16 tree and build two targets out
of it. That changes at carve-out stage C7."*

## Prerequisites

| Thing | Version used | Notes |
|---|---|---|
| CMake | 4.2.0 | `CMakeLists.txt` requires ≥ 3.30. |
| Visual Studio | **18 Community**, MSVC 14.51.36231, toolset v145 | Must include the **MFC** component — see "The MFC trap" below. |
| Windows SDK | 10.0.26100.0 | |
| Git | any | FetchContent/CPM clone several SDKs. |

## The four overrides are the normal path, not an optimisation

**They apply to either root** — TiDE's own and SE16's alike. The only build that
deliberately omits them is the release check above.

**Pass all four or none.** These are not third-party libraries that occasionally
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

**Read the configure banner. It tells you which path every dependency took.**
Since **S17** there are two forms, and the second is the one worth reading —
the older `Using local … folder` / `Fetching … from github` lines say *whether*
a dependency was local, while the provenance block says *which directory*, which
is the question you actually have:

```
-- TIDE dependency provenance -- 8 resolved:
--   vst3_sdk <- C:/Users/jef/.cpm/vst3_sdk/<hash>/vst3_sdk [CPM cache]
--   SynthEditLib <- C:/SE/SynthEditLib [local override]
--   GMPI <- C:/SE/GMPI [local override]
--   gmpi_ui <- C:/SE/gmpi_ui [local override]
--   CLAP <- <build>/_deps/clap-src [fetched]
--   clap_helpers <- <build>/_deps/clap_helpers-src [fetched]
--   harfbuzz <- C:/Users/jef/.cpm/harfbuzz/<hash>/harfbuzz [CPM cache]
--   GMPI_Wrappers <- C:/SE/GMPI_Wrappers [local override]
```

An unexpected `[fetched]` on one of the four means you forgot a `-D` and are
building against stale code. S17 also added `tide_check_not_shadowed`, which
**fails the configure outright** if an override is set while a fetched copy of
the same dependency sits in `build/_deps/` — the E12 shape, where the class
layout being read was not the one being compiled.

Genuinely fetched every time, and fine to leave alone: the VST3 SDK (via CPM
into `%USERPROFILE%\.cpm`), HarfBuzz (CPM), AudioUnitSDK, CLAP and clap-helpers
(FetchContent, into the build tree). These are stable third-party SDKs with no
override option, which is why a frozen checkout of them is harmless. A first
configure in a fresh build directory therefore needs internet and takes about a
minute.

## The MFC trap — read this before you file a build bug

Two files that TiDE links through `EditorLib` include an MFC header on Windows.
**Both moved to the public repo in the carve-out — paths corrected 2026-08-24;
this file still named their pre-C3 `SE16/SynthEdit2/` homes:**

- `SynthEditLib/EditorLib/CContainer.cpp:8` — `#include "afxres.h"` under `#ifdef _WIN32`
- `SynthEditLib/EditorLib/MfcDocPresenter.cpp:4` — same

Those are the same two files BACKLOG **P3** exists to de-MFC, which is the trap
inside the trap: a reader who greps the backlog for the failing filenames finds
a known open code item and stops looking at their toolchain.

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
...\EditorLib\CContainer.cpp(8,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [<build>\EditorLib\EditorLib.vcxproj]
...\EditorLib\MfcDocPresenter.cpp(4,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [<build>\EditorLib\EditorLib.vcxproj]
```

**The prefix differs by which build you ran**, which is worth knowing because it
is the one clue that says which tree you are in: a standalone superproject build
reports `<build>\_deps\syntheditlib-src\EditorLib\…`, an SE16 build with the
overrides reports `C:\SE\SynthEditLib\EditorLib\…`.

The message is misleading — the header is not missing from the machine, the
build is just looking inside the wrong VS install. Pin the instance (below), or
install MFC into whichever instance CMake selects.

`CMAKE_GENERATOR_INSTANCE` cannot be changed in an existing build tree. If you
already configured against the wrong one, delete the build directory.

## Commands that work

`CMAKE_GENERATOR_INSTANCE` is required on this machine for **every** variant
below — see the MFC trap above. Substitute your own build directory.

### The dev-box default: TiDE's own root, local siblings

```bash
cmake -S C:/SE/TideSynth -B C:/SE/_scratch/bt -DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community" -DSYNTHEDITLIB_FOLDER_OVERRIDE=C:/SE/SynthEditLib -DGMPI_SDK_FOLDER_OVERRIDE=C:/SE/GMPI -DGMPI_UI_FOLDER_OVERRIDE=C:/SE/gmpi_ui -DGMPI_WRAPPER_FOLDER_OVERRIDE=C:/SE/GMPI_Wrappers
```

```bash
cmake --build C:/SE/_scratch/bt --config Release --target TIDE_Rack TIDE_Rack_VST3 TIDE_Rack_STANDALONE
```

Verified 2026-08-24: **configure rc=0 in 11 s**, banner reporting
`[local override]` for all four siblings. Add `TIDE_Rack_CLAP` for the CLAP;
`TIDE_Rack_AU3` is macOS-only and does not exist in a Windows generate.

**Read the banner every time.** It names the resolved path of every dependency:

```
--   dep SynthEditLib: C:/SE/SynthEditLib   [local override]
--   dep GMPI: C:/SE/GMPI                   [local override]
--   dep gmpi_ui: C:/SE/gmpi_ui             [local override]
--   dep GMPI_Wrappers: C:/SE/GMPI_Wrappers [local override]
--   dep vst3_sdk: …/.cpm/vst3_sdk/…        [CPM cache]
--   dep CLAP: <build>/_deps/clap-src       [fetched]
```

An unexpected `[fetched]` on one of the four means you dropped a `-D` and are
compiling against someone else's `main` — see the silent-freeze warning below,
because that mistake does not heal on the next configure.

### The release check: nothing local at all

```bash
cmake -S C:/SE/TideSynth -B C:/SE/_scratch/bt-clean -DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community"
```

Same build, every sibling fetched from its own `main`. **This is not a second
way to develop** — it answers *"does this build without my uncommitted local
work?"*, which is what CI runs and what a stranger's clone gets. Run it before
tagging a release, or when a green local build and a red CI run disagree.

Verified 2026-08-24 from a cold tree: **configure rc=0 in 25 s, build rc=0,
zero `error C` / `error LNK` lines and zero warnings**, about five minutes wall
clock, producing all three artifacts.

### If you also want SynthEditCL, SynthEdit2 or SE16's test suite

```bash
cmake -S C:/SE/SE16 -B C:/SE/build-tide-p1 -G "Visual Studio 18 2026" -A x64 -DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community" -DGMPI_SDK_FOLDER_OVERRIDE=C:/SE/GMPI -DGMPI_UI_FOLDER_OVERRIDE=C:/SE/gmpi_ui -DGMPI_WRAPPER_FOLDER_OVERRIDE=C:/SE/GMPI_Wrappers -DSYNTHEDITLIB_FOLDER_OVERRIDE=C:/SE/SynthEditLib
```

The SE16 root is the *only* thing this buys you. It is not a different way of
building TiDE.

### Where the output lands, and what sits beside it

`<build>/SynthEditSem/Release/` — flat files; Windows builds no VST3 bundle.

**The four pin-descriptor XMLs and `Prefabs/` are staged LOOSE in that same
directory**, mixed in with the binaries, not in a `Resources/` subfolder. That
looks wrong and is right: for a non-bundle Windows plug-in
`BundleInfo::getResourceFolder()` returns the binary's own directory with no
subfolder appended (S36). If a run prints `missing from bundle resources` or
`no Prefabs folder in bundle resources`, that staging is what to check.

## Current target and artifact names (N1a, 2026-08-22)

The canonical list. Older documents under `docs/` are **dated records** and name
the pre-rename artifacts on purpose — check the date at the top of one before
believing a name in it.

| | CMake target | shipped file | platforms |
|---|---|---|---|
| GMPI | `TIDE_Rack` | `TIDE-Rack.gmpi` | all |
| VST3 | `TIDE_Rack_VST3` | `TIDE-Rack.vst3` (bundle); the binary inside is `TIDE-Rack.so` on Linux | all |
| CLAP | `TIDE_Rack_CLAP` | `TIDE-Rack.clap` | all |
| AUv3 | `TIDE_Rack_AU3` | — | **macOS/iOS only** |
| standalone | `TIDE_Rack_STANDALONE` | `TIDE-Rack` (`TIDE-Rack.exe` on Windows) | all |

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

The authority for that list is `SynthEditSem/CMakeLists.txt`'s `FORMATS_LIST`.
**Corrected 2026-08-24 — this line read `SE16/SynthEditSem/CMakeLists.txt:41`
(`FORMATS_LIST GMPI VST3`), and both halves had drifted:** the file is now in
the TideSynth repo rather than under `SE16`, the setting is at **line 163**, and
it reads `GMPI VST3 CLAP AU3 STANDALONE`. Three formats had been added since
anyone updated this sentence, which is why a line number in prose is worth
re-reading rather than trusting — grep `FORMATS_LIST` instead.

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

### macOS: the build installed to a folder the scanner never read — FIXED 2026-08-23 (S35)

**This section described a live defect until 2026-08-23. It is fixed, and the
description is kept because the mechanism explains any stale build you still
have lying around.** `SynthEditLib` now scans the user domain as well:
`getUserPluginsFolder()` (`modules/se_sdk3_hosting/BundleInfo.cpp:170`) is the
user-domain twin of `getPlatformPluginsFolder()`, and
`EditorLib/Application.cpp:567-581` scans it after the system one. Landed as
[SynthEditLib#36](https://github.com/JeffMcClintock/SynthEditLib/pull/36).

**It changes nothing on Windows or Linux, by design** — `getUserPluginsFolder()`
returns empty on both. Windows has no per-user plug-ins convention, and Linux
already keeps everything under the per-user data dir, so a second scan would be
the same folder. Empty means no second scan.

**The user path is derived from `ModulePath`, not hardcoded**, so if you have
repointed `ModulePath` you keep one scan rather than silently gaining a folder.

**What it looked like before, and why a stale install can still bite you.**

| | path | who writes it |
|---|---|---|
| `SE_LOCAL_BUILD` installs to | `~/Library/Audio/Plug-Ins/GMPI` | `copy_plugin()`, `GMPI/gmpi_plugin.cmake:1225` |
| the module scanner used to read, only | `/Library/Audio/Plug-Ins/GMPI` | hard-coded |

The scan root was a string literal, not a domain lookup:
`getPlatformPluginsFolder()` returns `"/Library/Audio/Plug-Ins/"`
(`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:152`), `"GMPI"` is
appended in `SynthEdit/SynthEdit2/SynthEditApp.cpp:155-164`, and that one path
was all `RefreshModuleData` scanned. There was no
`NSSearchPathForDirectoriesInDomains` anywhere in it, so unlike VST3 and AU —
which search user *and* system by convention — the user domain was never
consulted. Measured on one mac before the fix: **7 modules in the system domain,
9 in the user domain, none of the 9 visible.**

**Measured, not inferred.** Across every `Plugin-Cache-16-override-*.xml` in
`~/Library/Application Support/SynthEdit/`, all 602 recorded module paths were
under `/Library/Audio/Plug-Ins/GMPI`. **Zero** user-domain paths had ever been
recorded, in any cache file, at any date — so it was not a stale-cache artifact.

**Do not use that cache count to check the fix — it stays zero either way.** The
caches store module metadata without absolute paths, so a user-domain module
that is now being scanned adds no user-domain path to them. Verify with
`SynthEditCL -rescan` and read its output instead: it prints a second
`Scanning for 3rd-party SEMs in (user domain): …` line, and duplicates of
factory SEMs are correctly reported as `Module FOUND TWICE!`. Whoever fixed this
nearly read the unchanged zero as the fix not working.

**With the fix, a locally built module is simply found where it was installed.**
The copy below is no longer needed on an up-to-date `SynthEditLib`, and is kept
for anyone building against an older one:

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
