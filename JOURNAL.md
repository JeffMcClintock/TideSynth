# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-23 — macos — TIDE Rack installs on iOS. The blocker was a folder (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Two fixes, measured after each as Jeff asked. `simctl install` now returns **0**
on the unmodified build output.

**FIRST, A CORRECTION I OWE THE RECORD.** I told Jeff this needed "a GMPI-wide
identity decision", then corrected that to "TIDE's own file, ALLOWED". Both were
wrong. The appex's `CFBundleIdentifier` is written by `plist_util` from a value
baked into its COMMAND LINE at GMPI configure time, so no TIDE target property
could ever have changed it. Reading `plist_util.cpp`'s usage string settled in
one command what I had twice asserted from memory.

**Fix 1 — the extension id (GMPI).** An extension's id must be PREFIXED by its
containing app's. GMPI's comment already said so, and its code then wrote both
halves from one string a plugin may replace. TIDE replaces it: S40 renamed the
containing app to `com.tidesynth.tiderack.au3app` — **and renamed only the app.**
The extension kept `com.gmpi.au3.TIDE_Rack.extension`, the two shared no prefix,
and macOS never complained because macOS does not enforce the rule. Now DERIVED
from the app target as a generator expression. **Measured:** the extension came
out `com.tidesynth.tiderack.au3app.extension` with TIDE setting nothing.
Install still failed, which is why there was a second fix.

**Fix 2 — and it was not the plist at all.** I had assumed missing iOS plist
keys. Only bisection showed otherwise. Eliminated in turn, each by experiment:
`MinimumOSVersion`, `LSRequiresIPhoneOS`, `CFBundleSupportedPlatforms`,
`DTPlatformName`, `UIDeviceFamily` — added all five by hand, still rc=2. Version
match with the container — already matched. Malformed plist — read the whole
file, well-formed. `NSExtension*` keys — all correct.

**Then: deleting `Resources/` from the appex made it install.** But removing
only the XMLs did not, and removing only `Prefabs` did not. **It was the
subdirectory itself.** An iOS bundle is FLAT — resources belong at the bundle
ROOT — and TIDE staged them into `Resources/`, the macOS layout, because the
destination was `$<TARGET_BUNDLE_CONTENT_DIR>/Resources` on all Apple platforms.
Moving the contents to the root: **rc=0**.

That is the same root cause as the codesign defect earlier today, where a flat
bundle takes its executable name from the bundle name. **Third time iOS's flat
layout has broken something written against macOS**, and the error named it in
none of the three cases — this one said *"Missing bundle ID"*, about a bundle
whose id was present and correct.

**A process note worth keeping.** I pushed the GMPI half onto GMPI#12's branch
after Jeff had already merged it, leaving an orphaned commit — the one end state
STEP 5 forbids. Caught by checking, moved to a fresh branch off `main`, stale
branch deleted. The lesson is STEP 4's, and it is cheap: **check the PR is still
open before pushing a follow-up**, because on an actively-merging day it may not
be.

**Verified.** iOS: clean build rc=0, `simctl install` **rc=0** on the build
output with nothing edited by hand, app registered as
`com.tidesynth.tiderack.au3app`. macOS negative control: rc=0, all five formats,
and the appex still uses `Contents/Resources` — the flat layout is iOS-only.

**Not verified.** The app was installed, **not launched**, and the Audio Unit was
never opened in an iOS host — so nothing here says TIDE RUNS on iOS, only that it
installs. Device (non-simulator) builds were never configured. The `__APPLE__`
population in SynthEditLib is still 31 files against 2 using `TARGET_OS_*`.

## 2026-08-23 — macos — three red signals, three different mechanisms, and none of them a defect in this repo

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.34493.1** (agent 2.1.237) · as **tide-rack-bot** (both paths)

**Did:** STEP 1 and STEP 1.5 only — no backlog item. Two `platform:mac` issues and
one red check on this platform's own PR, and the interesting result is that the
three have **three unrelated causes and zero overlap**, where the obvious reading
was one bad merge.

| signal | cause | where it lives |
|---|---|---|
| [#310](https://github.com/JeffMcClintock/TideSynth/issues/310) `main` red on macOS | a **26-second** window between two sibling repos' merges | nobody's commit — **S41** |
| [#307](https://github.com/JeffMcClintock/TideSynth/issues/307) red on a branch | 14 build-tree **gitlinks** committed by accident | fixed before I looked |
| [#313](https://github.com/JeffMcClintock/TideSynth/pull/313) `windows` FAILURE | a **parallel-build race** in the Windows resource staging | real, latent, **#314** |

### #310 — the failure was 26 seconds wide, and CI fell in it

```
build/_deps/gmpi_wrappers-src/wrapper/AU2/AU2_Wrapper.cpp:8:10:
  fatal error: 'backends/GmpiObjCNames.h' file not found
```

S38 shipped as two PRs in two repos. TIDE fetches both by `GIT_TAG origin/main`:

```
08:03:41Z   GMPI_Wrappers#13 merged   (adds the #include)
08:04:07Z   gmpi_ui 6aa8871 merged    (adds the header)      <- 26 s later
```

and the failing run's own configure log straddles the gap:

```
08:03:42.10Z   Fetching gmpi_ui from github     <- 25 s BEFORE the header existed
08:03:59.35Z   dep GMPI_Wrappers  [fetched]     <- 18 s AFTER the #include did
```

The build got the consumer without the provider. **Nothing in TideSynth was
wrong, and nothing needed fixing** — `9806401b` (09:08) and `318d2748` (09:41)
went green on macOS with no change in between.

**Re-verified here rather than inferred from those green runs**, because the
current head `736e75b8` had no macOS verdict at all (its run was cancelled):
clean scratch worktree, no overrides, the exact CI recipe — configure rc=0,
**build rc=0, 0 errors**, all five formats emitted, `lipo -archs` = `arm64`.
Closed on that.

**The part worth keeping is not the header.** X4 (WONTFIX, *"do not re-file"*)
reasons that *"CI clones fresh, so `origin/main` there is real"*. That is true
**per repo** and false **across** repos: CI resolves each sibling at a different
instant, so there is no snapshot consistent between them, and any two-repo change
merged non-atomically breaks every platform for the width of the interval. Filed
as **S41**, proposing no change to the pins — the fix worth having is in the
issue-filing step, not in the fetching.

**And the real cost is not the red build**, which nobody was waiting on. It is
that `build.yml` files a `platform:*` issue, that issue is STEP 1 work outranking
all backlog items, and three boxes then spend a session each re-verifying a break
that already fixed itself. This run is that cost, paid.

### #307 — the error names `.gitmodules` and means "a build tree got committed"

Not a compiler failure at all; the job died in `actions/checkout`:

```
fatal: No url found for submodule path 'build-ios/_deps/au2-src' in .gitmodules
```

`git ls-tree -r 1e2956b4 | awk '$2=="commit"'` → **14 gitlinks**: the fetched
dependency checkouts of `build-ios/` *and* `build-mac/`, staged as submodules
with no `.gitmodules` to resolve them. That breaks checkout on every platform,
which is why one run produced both this and #306 (linux).

Already resolved, and I checked each half rather than assuming: the merged result
`38bc306` has **0** gitlinks, `main` has no `.gitmodules` and no `build*` entry,
the branch is deleted — **and `38bc306` added `build-*/` to `.gitignore`**, with a
comment saying plain `build/` did not catch a per-platform tree and one "was very
nearly committed because of it". Not *nearly*: on that branch it was. Closed.
Commented on #306 with the same evidence and left it for the linux box, since
closing another platform's issue is not mine.

### #313 — the windows check, and the log proves the mechanism unaided

STEP 1.5 made this mine. The PR's own change is **iOS-only** (`if(CMAKE_SYSTEM_NAME
STREQUAL "iOS")`), so it cannot reach a Windows build — and the log says what did:

```
10:11:07.3390324  TIDE_Rack.vcxproj      -> build\SynthEditSem\Release\TIDE-Rack.gmpi
10:11:07.3593501  TIDE_Rack_VST3.vcxproj -> build\SynthEditSem\Release\TIDE-Rack.vst3
10:11:07.3934793  TIDE_Rack_CLAP.vcxproj -> build\SynthEditSem\Release\TIDE-Rack.clap
10:11:07.3983502  Staging rack prefabs (TIDE_Rack)
10:11:07.3985881  Staging rack prefabs (TIDE_Rack_VST3)
10:11:07.4048826  Staging rack prefabs (TIDE_Rack_CLAP)
10:11:07.5854032  Error copying directory ... Release/../Resources/Prefabs: Permission denied
```

**Three targets started staging the same directory 6.5 ms apart**, and one lost
180 ms later. On Windows every format target links into one directory — the three
`->` lines are the proof — so `$<TARGET_FILE_DIR:...>/../Resources` is a single
shared destination and all four targets run the identical five POST_BUILD copies
into it under `--parallel`. **N1a's class exactly**, which fixed `PDB_NAME` and
`ARCHIVE_OUTPUT_NAME` for the same underlying reason and never touched the
resource destination.

**Not fixed here.** STEP 3 forbids fixing a platform I cannot compile on, and
this one deserves the rule even more than usual — see the lesson below. Filed as
[#314](https://github.com/JeffMcClintock/TideSynth/issues/314) with the mechanism
and the frequency, and recorded on **S36**, which already owns this arm and whose
open (a)/(b) choice it settles: **(a) does not remove the race** (dropping the
`/..` still leaves one destination shared by four targets), **(b) does**.

**Re-ran the job on the identical commit and it passed.** #313 is now fully green
— `lint`, `guard`, `linux`, `macos`, `windows` — and nothing about it was
changed, which is the whole point.

**Learned:**

- **A race cannot be verified fixed by watching CI go green.** The 11 Windows
  jobs before this one all passed, and the same commit passed on re-run. Success
  is what the *broken* code produces most of the time, so a green run after a fix
  carries almost no information. This is the strongest argument I have met for
  STEP 3's "do not fix a platform you cannot compile on" — the usual objection,
  *"but CI is my instrument"*, is exactly what fails here.
- **`origin/main` is not a snapshot when you fetch more than one repo.** Two
  correctly-ordered merges 26 seconds apart broke every platform, because the
  consumer and the provider were resolved 17 seconds apart. The journal already
  says a two-PR fix has an order; what it lacked is that the **interval** is also
  load-bearing, and nothing in CI makes it atomic.
- **A self-healing break still costs three sessions.** The transient was over in
  under a minute and cost nothing to the build, but the issue it filed is STEP 1
  work, and STEP 1 outranks everything. The expensive part of a false alarm is
  the priority it inherits, not the alarm.
- **Three red signals on one platform in one hour is not evidence of one cause.**
  I opened this expecting the M2 merge to be behind all of them. They shared a
  branch name and nothing else — a merge-window transient, an accidental `git add`,
  and a latent race that had been there since CLAP joined `FORMATS_LIST`.
- **Read the first error, not the loudest.** The Windows failure printed ~45
  `MSB3073` lines; every one was cascade. The single `Error copying directory ...
  Permission denied` above them carried the whole diagnosis, including the
  unnormalised `Release/../Resources` that names the CMake expression verbatim.
- **A checkout error that names `.gitmodules` means someone committed a
  directory.** Nothing in the message resembles the actual mistake, and it fails
  identically on all three platforms — so it files a platform issue per box for
  what is one staging slip. The prompt's *"never `git add -A`"* is the rule; this
  is what breaking it looks like from the outside.

**Next:**

1. **M1 is closeable on evidence** and the mac NEXT cell now says so. Its only
   blocker was the AU2/AU3 fourCC clash awaiting a ruling; **S40 is DONE** — AUv3
   only, AU2 dropped — so the clash is gone by construction, and its Accept was
   demonstrated on plain `main` by this run's build.
2. **M2 is IN-REVIEW, not takeable** — #313 is open and green. A later run flips
   it once merged. Its **"Not verified"** list is the real remaining mac-only
   work: the iOS app was installed but **never launched**, the Audio Unit has
   never been opened in an iOS host, and device builds were never configured.
3. **#314 wants the Windows box**, and wants it to reproduce locally under
   `--parallel` rather than trusting a green run.
4. **S41 needs Jeff** — the fix lives in `.github/workflows/build.yml`, which the
   fleet token deliberately cannot touch.

**Machine left clean.** All work in a scratch worktree under the session
scratchpad, removed afterwards; Jeff's `~/Documents/GitHub/TideSynth/build` was
never written to, and no plug-in was installed into `~/Library/Audio/Plug-Ins`.
All six repos (`TideSynth`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`,
`SynthEdit`) were clean and on their default branches at the start, were not
touched during, and are back on them at the end. This item is two files in
TideSynth.

**Branch/PR:** `tide/mac/issue-310` — TideSynth, backlog and journal only. No
product code change. Issues: #310 and #307 closed, #314 filed, #306 commented.

---
## 2026-08-22 — macos — M2: iOS builds. Four defects, three of them one class (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Interactive session at Jeff's direction, so the GATED SynthEditLib half was in
scope. iOS went from a hard configure failure to a clean rc=0 build producing a
signed app and appex. It does NOT install yet; that is measured and scoped below.

**THREE OF THE FOUR ARE THE SAME MISTAKE: `APPLE` MEANS macOS TO THE AUTHOR AND
BOTH PLATFORMS TO THE COMPILER.**

1. `SynthEditLib/CMakeLists.txt`, `IF(APPLE AND SE_GRAPHICS_SUPPORT)` compiled
   NSView subclasses and a native modal dialog for iOS. Split it. Cocoa_Gfx.h /
   CocoaGfx.h STAY on both -- despite the name they are Core Graphics and Core
   Text, and gmpi_ui's own iOS backend includes CocoaGfx.h directly. iOS now
   gets `DrawingFrameIos.mm`, which existed all along and had simply never been
   reachable from here.
2. `TideCursorMac.mm` was gated `$<$<PLATFORM_ID:Darwin>:...>`. On iOS
   PLATFORM_ID is "iOS", so the file was not compiled at all and SynthEditGui.cpp
   failed to link against `tideShowCrossCursor`. Compiled on iOS now with an
   empty body -- `__APPLE__` is true there too, so the real AppKit version needs
   `TARGET_OS_OSX`. The caller stays platform-free: there is no cursor on a
   touch screen, and spreading the condition into shared code for one affordance
   would be worse.
3. `gmpi_plugin.cmake` copied the appex as `${SUB_PROJECT_NAME}.appex`, the
   TARGET name, while the bundle is named by OUTPUT_NAME -- **issue #271's class
   yet again, a derived name beside a real one.** On macOS cosmetic. On iOS
   fatal, and the error names nothing: codesign says *"bundle format
   unrecognized, invalid, or unsuitable"*. An iOS bundle is FLAT and for a flat
   bundle codesign takes the executable name from the BUNDLE'S OWN NAME, not
   from CFBundleExecutable. **Measured rather than reasoned: the identical
   bundle signs rc=0 as `TIDE-Rack.appex` and rc=1 as `TIDE_Rack_AU3.appex`.**

**The fourth is a different shape but the same disease -- one rule stated
twice.** `gmpi_find_plugin_element()` accepted any source containing a
`<Plugin>` tag, but `plist_util --xml` requires a `<PluginList>` wrapper. A
MODULE source legitimately has the former without the latter, so CMake handed
plist_util `modules/TiDEknob.cpp` and it exited *"No <PluginList> element"*.
Unseen until now because only the iOS AU3 path feeds a FILE to plist_util;
every other format derives its plist from the built binary.

**IT BUILDS, AND IT DOES NOT INSTALL. Do not confuse the two.** `simctl install`
fails. What that cost to narrow down, all measured:
  - the containing app installs **rc=0 on its own** with PlugIns removed, so the
    appex is what iOS rejects;
  - an extension's bundle id must be PREFIXED by its container's. TIDE's are
    `com.gmpi.au3.TIDE_Rack.extension` against `com.tidesynth.tiderack.au3app`,
    which share nothing. Setting a prefixed id **changed the error**, which is
    how the rule was confirmed rather than assumed;
  - it still fails after that, with *"Failed to create app extension
    placeholder"*. The appex plist lacks iOS bundle keys `plist_util` never
    emits -- `MinimumOSVersion`, `CFBundleSupportedPlatforms`, `UIDeviceFamily`.
    Adding `MinimumOSVersion` alone did not fix it, so it is not just that one.
  - `NSExtensionPointIdentifier`, `NSExtensionPrincipalClass` and
    `factoryFunction` are all present and correct, so the extension declaration
    itself is fine.

**Deliberately NOT fixed here:** the appex bundle id. Deriving it from the
containing app's id is the right answer but changes macOS's shipped identity,
which deserves its own change and its own verification rather than riding along
with a build fix.

**Verified.** iOS: clean build from an empty tree, rc=0, zero failures; one
correctly-named appex; `codesign -v` passes on both the .app and the nested
.appex; arm64. macOS negative control: rc=0 and still all five formats --
`.vst3`, `.clap`, `.gmpi`, `.app`, AUv3 app. All four repo overrides confirmed
local, so the edits were genuinely in the binaries.

**Not verified.** Nothing was run on a device or simulator -- the install is the
blocker above. Device (non-simulator) builds were never configured. And the
wider class is far from done: **31 files in SynthEditLib use `__APPLE__` and
only 2 use `TARGET_OS_*`.** This fixed the sites that block the build, not the
population.

## 2026-08-22 — macos — S38 remainder: the obvious place for the fix is the wrong one (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Interactive session at Jeff's direction, working the loop.

The plan was to move the per-plugin suffix into `gmpi_plugin.cmake` so every
GMPI plugin gets one instead of only TIDE. **It does not fit there, and finding
out cost one configure rather than an argument.**

The definition has to be directory-scoped and set before any
`add_subdirectory()`, because gmpi_ui's `.mm` sources compile into SEVERAL
targets per plugin -- `SynthEditLib`, `AU3_Wrapper`, `CLAP_Wrapper`,
`AU_Wrapper` -- living in different directories, several configured before the
one that calls `gmpi_plugin()`. So I wrote a helper for plugins to call at their
top level. TIDE cannot call it: it includes `gmpi_plugin.cmake` from
`SynthEditSem/CMakeLists.txt:29`, which is `add_subdirectory()`'d after
`SynthEditLib`, so at line 286 the function does not exist yet --
`Unknown CMake command "gmpi_objc_class_suffix"`. A plain
`add_compile_definitions(GMPI_OBJC_SUFFIX=_TIDE_Rack)` has no such
ordering requirement, which is why that is what TIDE ships and what #309 does.

**So GMPI#11 ships the CHECK instead, and the check is the part that was
actually worth having.** A plugin that never sets the suffix builds perfectly
and collides with every other GMPI plugin the user has installed; the only
symptom is a runtime warning nobody reads. `gmpi_plugin()` now warns at
configure time, names the project, and accepts either form -- the helper's
global property, or the definition found on the directory's inherited
`COMPILE_DEFINITIONS`, so it will not nag TIDE for doing it the simple way.

**Verified against MERGED upstream, not my own branches**, since gmpi_ui#13 and
GMPI_Wrappers#13 landed: no suffix set -> the warning fires and names
`TIDE_Rack`; helper called from the top level -> `Unknown CMake command`; plain
define -> configure rc=0, no warning, build rc=0, and the resulting VST3 exports
all six ObjC classes suffixed `_TIDE_Rack` with zero unsuffixed leftovers.

**Not verified:** Windows and Linux configures (the feature returns early, it is
APPLE-only), and no other GMPI plugin was configured against the new warning --
which is exactly why it is a WARNING and not a FATAL_ERROR. The three AU3
classes are still unsuffixed, deliberately, for the plist reasons in
GMPI_Wrappers#13.

## 2026-08-22 — macos — S38: the fix already exists in this codebase, and it fails open (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Interactive session at Jeff's direction, working the loop, not the Saturday
scheduled run.

The row said "deliberately not costed: the right shape (macro-suffix every class
name) touches every Objective-C class in the UI layer". That was true when it was
written and is not true now -- the macro indirection landed since, so every name
was already behind a `#define` and only the right-hand side needed changing. Seven
names in gmpi_ui, four in GMPI_Wrappers.

**The interesting part is that SynthEdit already solved this, and I could measure
how well.** Its AU plugins suffix every ObjC class with a per-plugin GUID, e.g.
`CocoaEventHelper_18ca02e51b5b407abca9a193d830c520ARTR`. But the GUID is patched
over a placeholder string in the built binary, and **that fails open**. Of the 25
installed AU components on this box that export ObjC classes, 19 carry a unique
GUID and **6 carry the literal, unpatched `GUID_GOES_HERE_PLUS_SOME_MORE_CHAR`**.
Those six collide with each other exactly as if nothing had been done. I loaded
three of them in one process and got the warning for every shared class; two
plugins with distinct GUIDs gave none. So the scheme is right and the delivery
mechanism is wrong, which is why this does it in the preprocessor instead.

**Two traps, both of the same kind: a class name that is also a string.**

1. `AU2_Wrapper.cpp` hands the host its Cocoa view class name as a hardcoded C
   string literal. Renaming only the `@interface` would leave the plugin
   advertising a class that no longer exists -- a GUI that silently fails to
   open, with **no runtime warning at all**, which is worse than the collision.
   Class and string now derive from the same macro.
2. `GmpiAUViewController` is named in FOUR places outside the source, as the
   appex's `factoryFunction` and `NSExtensionPrincipalClass`, and the plist is
   generated by `plist_util`, a host tool that would need telling over its
   command line. **So I did not rename the three AU3 classes.** AUv3 also runs
   out of process, so its collision risk is much narrower. Left as its own job.

**I got the stringify macro wrong first.** `#x` stringifies its argument
unexpanded, so the one-level version yields the literal text of the macro call
rather than the pasted name -- precisely the drift the header exists to prevent.
A compile test asserting the two agree is what caught it, not review.

**Verified.** Symbols: TIDE's VST3 exports all six classes suffixed `_TideSynth`
where the pre-change build exported them bare. Runtime: two copies of the
pre-change binary in one process produce 6 duplicate-class warnings; pre-change
plus post-change together produce 0, and both load. AU2 (temporarily added to
`FORMATS_LIST`, reverted before commit): class symbol and host-facing string are
both `GMPI_VIEW_MAKER_VERSION_02_TideSynth`. Build rc=0, zero errors.

**Not verified.** The iOS name -- iOS does not build yet (M2). SynthEdit's own
builds, which will see their classes renamed to `_NO_PLUGIN_SUFFIX_SET` until a
suffix is configured there; that is deliberate, since such a build genuinely does
still collide. The Standalone delegates were compiled, not exercised. And the
row's original open question is still open: **whether a collision has ever caused
an observed misbehaviour here.** The warning proves the condition, not a symptom.

**The general fix belongs in `gmpi_plugin.cmake`** so every GMPI plugin gets a
distinct suffix rather than only TIDE. That repo is PR-GATED and it is a separate
PR; TIDE sets the define itself in the meantime and does not wait on it. Note the
define must be **directory-scoped**: gmpi_ui's `.mm` sources compile into several
targets per plugin (SynthEditLib, AU3_Wrapper, CLAP_Wrapper, AU_Wrapper), so a
target-scoped define would reach almost none of them.

## 2026-08-22 — macos — M2 iOS configure: two fixes, then a gate (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Interactive session at Jeff's direction, working the loop, not the Saturday scheduled run. The commit is authored **Jeff McClintock** rather than the bot -- `check-commit-authorship.py` reports it and does not fail it, and STEP 4 forbids rewriting anything already pushed. The push itself went as `tide-rack-bot`.

Took M2 (iOS AUv3) expecting to write a wrapper. The row was sized for that. A wrapper
landed 2026-08-19 and S40 made AUv3 the shipped macOS format, so the real question was
only whether iOS configures and builds. It now does, to within three errors.

**My own M1 guard was the first blocker, and it was a false positive.** I added it last
week to stop AU/AU3 silently skipping when `plist_util` is missing. On iOS there is
legitimately no such target: `gmpi_plugin.cmake` compiles a *host* copy via a custom
command, because a Mac cannot load an iOS-built module. My check turned a real
invariant into a hard error on a platform it had never been run against. Guard now
excludes iOS.

**Second was nine `to_chars is unavailable` errors, and none of them were about code.**
`CMAKE_OSX_DEPLOYMENT_TARGET` was set unconditionally, so macOS 13.3 leaked into the
iOS build, where libc++ marks `std::to_chars` as introduced in iOS 16.3. Nine errors,
one availability attribute. Now platform-aware; macOS verified still 13.3, rc=0.

**iOS is 18.0, and that is the version before 26.** Jeff's ruling: target only the most
recent couple of iOS versions, so TIDE can use the newest C++ and STL instead of
working around old runtimes. I set 25.0 for "one before 26" and the compiler rejected
it outright -- invalid version number. Apple jumped 18 to 26 to match the year; 19
through 25 never existed. I probed the toolchain rather than reason about it further:
16.3, 17.0, 18.0 and 26.0 accepted, 25.0 not. Worth the thirty seconds -- I would have
written the same wrong number again.

**What is left is one class, three files, and two are behind a gate.** macOS-only Cocoa
sources compiled for iOS: `DrawingFrameMac.mm` wants `AudioUnit/AUCocoaUIView.h`,
`AssignControllerDialogMac.mm` and `SynthEditCocoaView.mm` want `Cocoa/Cocoa.h`. iOS is
UIKit. They need excluding from an iOS build -- mechanically small, but the latter two
live in `SynthEditLib`, which is GATED. **So M2 stops here and needs Jeff or an
interactive session.** I did not attempt `simctl install` or a launch; the build has to
finish first, and it does not yet.

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
### Correction: Ardour IS a host here, and it settles the question

**Jeff asked "don't we have Ardour host?" — yes, and that makes three separate
claims of mine wrong.** I wrote in the row, both PR bodies and the issue that
closing this needed REAPER on a win/mac box. **Ardour 8.4 is installed on this
box**, `ardour-vst3-scanner` answers precisely this question, and **my own memory
note from 2026-08-19 records using it**, including the
`LD_LIBRARY_PATH=/usr/lib/ardour8` quirk it needs.

```
BROKEN (main):  VST3 not a valid bundle:
                  '.../TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE_Rack_VST3.so'
FIXED  (both):  [Info]: Found Plugin: TIDE Rack
                  uid=506C7567696E474D504920501951ED43 category="Instrument|Synth"
                  n_outputs=2 n_midi_inputs=1
```

Ardour derives the payload name from the bundle name — exactly the rule GMPI's
own comment states — so **the Linux VST3 is unloadable today, not merely oddly
named**, and the fix is host-verified on the platform that has the bug. The
scanned UID also matches the one in all five `.rpp` fixtures.

**The lesson is not "use Ardour".** It is that I asserted an environment limit
three times without testing it, while holding a note that contradicted it.
"Not verifiable here" is a claim about the machine, and it deserves one command
before it goes into a row, two PR bodies and an issue.

Ardour's cache entry from the scan pointed into a scratch tree and was removed;
Jeff's other nine cached plugins were left alone.


**Learned:** anything the next run would otherwise rediscover the hard way.

0. **"Not verifiable on this box" is a measurable claim, and I shipped it three
   times unmeasured.** Ardour was installed the whole time and my own memory note
   named the command. Check the machine before writing a limit into a row.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---

## 2026-08-22 — macos — STEP 4, and the hour-long feedback loop that caused two of today's failures

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** bookkeeping — **S29** and **S40** to DONE — and recorded on R5 the fix
that matters most from this stretch.

### S29 had been stale for hours

Its PR merged long ago and nobody flipped the row. Confirmed by consequence
rather than by the merge: `build.yml` on `main` carries the
`startsWith(github.head_ref, 'tide/')` test, which is precisely the clause #258
shipped without. That is the second row today found stale by a sweep rather than
by anyone noticing — the first was R5's branch pushed with no PR.

### The fix worth remembering

The certificate import ran **after** the build. Both of v0.1.0's failed attempts
were credential problems:

| attempt | failed at | knowable at |
|---|---|---|
| 1 | `Package` — missing Developer ID Installer certificate | ~30 s |
| 2 | `Import` — passphrase did not match the archive | ~30 s |

Each surfaced only after **~60 minutes of compiling**. About two hours today
spent learning something knowable in thirty seconds, and I wrote that ordering
myself.

Nothing in the import depends on the build, and the keychain has a 6-hour
timeout, so it simply moves. What made it visible was Jeff asking why the build
was still slow — the question was about ccache, and the answer that mattered was
about step order.

**And the import now says which secret is wrong.** `MAC verification failed
during PKCS12 import (wrong password?)` cannot distinguish a bad passphrase from
bad certificate data, and those need opposite fixes.

**Learned:**

- **Put the cheap failure first.** A credential check is seconds and a compile is
  an hour; running them in that order costs nothing and saves the hour every time
  the credential is wrong. I had them backwards and paid twice before noticing.
- **A stale row is found by sweeps, not by people.** S29 sat IN-REVIEW for hours
  with its work merged; R5's branch sat pushed with no PR. Both surfaced only
  when something systematically asked "which rows disagree with reality?"
- **The question asked is not always the question that matters.** "Is it still
  slow?" was about ccache. The expensive slowness was ordering, and it only came
  out because answering properly meant reading the whole job.

**Next:** the mac cell now points at **M2**, whose row is the most out-of-date
thing on the board — it treats authoring an AUv3 wrapper as its blocker, one
landed 2026-08-19, and **S40 just made AUv3 the shipped macOS format**. What
remains is genuinely iOS, and a Mac is the only machine that can attempt it.
**Re-cost that row before working it.**

**Branch/PR:** `tide/mac/step4-s29-s40` — TideSynth, bookkeeping only.

---

## 2026-08-22 — macos — S40 ruled: AUv3 only, and the install story is a copy

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** Jeff ruled S40 — AU3 for macOS and iOS, drop AU2. *"Works in Apple DAWs.
Other DAWs on Apple tend to support VST3 and or CLAP. If we can omit AU2, just to
save some work and time, let's do so. Won't be difficult to add later anyhow."*

Implemented: `FORMATS_LIST` is `GMPI VST3 CLAP AU3 STANDALONE`, the `.component`
is gone, and `TIDE-Rack-AUv3.app` takes its place in the pkg.

### The expensive part turned out to be free

An AUv3 ships as an **app**, not a plug-in bundle, so the obvious worry was the
install story: does the extension register from a pkg-installed app, or does the
user have to launch it, or does the pkg need a postinstall script?

Measured rather than assumed:

```
baseline (nothing installed)                    0 Drck entries
copy app in, no launch, no pluginkit            1 Drck entry
explicit pluginkit -a afterwards                1 (unchanged)
```

**macOS registers it on the copy alone.** So the pkg needs no postinstall script,
which is the single thing that could have made "drop AU2" cost real time.

### The app is user-visible, so it needed a name

`gmpi_plugin.cmake` names the containing app after the CMake target —
`TIDE_Rack_AU3App.app` — which is underscored, a form `distribution.md` reserves
for internal target names, and it lands in `/Applications` where Finder shows it
to the customer.

Now `TIDE-Rack-AUv3.app`, `CFBundleName "TIDE Rack (AUv3)"`, R8's identifier
scheme. **Not plain `TIDE-Rack.app`**: the STANDALONE format already produces
that in the same directory, and two bundles cannot share a name.

### A guard of mine was defeated by `set -e`

The new "app contains no .appex" check ran `find` on a possibly-missing directory
inside a command substitution. Under `set -euo pipefail` that kills the script
**before** the message prints — so the negative control produced **rc=1 and zero
bytes of output**. A guard whose whole purpose is to say what went wrong, exiting
silently.

It only surfaced because I ran the control and looked at the output rather than
just the exit code. `rc=1` alone looks like the guard working.

**Learned:**

- **Measure the install story before costing a format change.** "AUv3 ships as an
  app" sounds like installer work; the extension registers on a plain copy, and
  the whole concern evaporated in one three-line experiment.
- **`set -euo pipefail` can kill a guard before it speaks.** A command
  substitution containing a failing pipeline aborts the script, so the carefully
  written error message never runs. Test the precondition, then run the command.
- **A non-zero exit is not evidence a guard fired.** Both look like `rc=1`. Read
  the output, not just the status — mine had none.
- **A format that ships as an app inherits a naming decision the build never had
  to make.** Plug-in bundles live in paths nobody reads; this one has an icon in
  `/Applications`.

**Next:** **R6** still wants a completed release, and the v0.1.0 rerun is still
building. **The pkg's payload changed shape today** — component out, app in — so
whatever the current run produces is already one revision behind. **NOT verified:
installing the pkg** (unsigned here), and `/Applications` is reasoned from
`~/Applications` working rather than separately measured.

**Branch/PR:** `tide/mac/S40-au3-only` — TideSynth.

---

## 2026-08-22 — macos — S38 is three problems, not seven instances of one

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took S38 — seven Objective-C class names shared between GMPI plugins.
The row treated it as one uniform problem and sized the fix as *"suffix every
Objective-C class in the UI layer"*. It is three different problems, and one of
them was already solved.

### The scheme is deliberate, which changes the question

`DrawingFrameMac.mm:573`:

> *"Objective-C can't handle loading the same class into different plugins, give
> each iteration of this class a unique name"*

So two plugins on the **same** gmpi_ui share the name **and the body** — harmless,
whichever wins. The danger is only ever **two different bodies under one name**.

That turns "seven classes collide" into a question with a per-class answer: *has
the suffix been bumped whenever the class changed?* Which is measurable by
diffing each `@implementation` between its introducing commit and HEAD.

### The per-class answer

```
GMPI_VIEW_VERSION_03           38 commits   CHANGED 8556 -> 10835 chars   STALE
GMPI_KEY_LISTENER_VERSION_03   38 commits   byte-identical                fine
GMPI_MAC_ColorPanelTarget_03    0 commits   fresh                         fine
GMPI_EVENT_HELPER_CLASSNAME_03  0 commits   fresh                         fine
GMPI_VIEW_MAKER_VERSION_02        —         introducing commit not found  UNCHECKED
GMPI_KeyListenerView              —         NO SUFFIX AT ALL              worse
GMPI_EscapableTextField           —         NO SUFFIX AT ALL              worse
```

**One stale suffix, fixed.** `GMPI_VIEW_VERSION_03`'s body grew by 27% across 38
commits under the same name, so an old plugin and a new one export it with
different implementations. Bumped to `_04` and verified where it counts — the
shipped TIDE binary carries `GMPI_VIEW_VERSION_04` once and `_03` zero times.

**The sibling is what makes this a discipline lapse rather than a design flaw.**
`GMPI_KEY_LISTENER_VERSION_03` is byte-identical across the *same* 38 commits and
correctly needs nothing. Had both drifted, the scheme itself would be suspect.

**Two are worse than the row says.** `GMPI_KeyListenerView` and
`GMPI_EscapableTextField` have no suffix at all, so they can never be
disambiguated — any two plugins collide on them regardless of version. Still
open, and it is two small changes rather than the sweeping one the row costed.

### What I checked before renaming anything

No `NSClassFromString` or `objc_getClass` anywhere in `gmpi_ui`. A class renamed
out from under a string lookup would fail at runtime and only in the path that
does the lookup — the worst possible failure to introduce while "fixing" a
collision.

### An override that silently did nothing, again

I built TIDE with `-DGMPI_UI_SDK_FOLDER_OVERRIDE=…` and the dependency report
said `[fetched]`. The variable is `GMPI_UI_FOLDER_OVERRIDE` — no `SDK`. Had I not
checked the report and grepped the compiled source for `_04`, I would have
"verified" the fix against a build that never contained it.

**Learned:**

- **Read the mechanism before costing the fix.** The version suffixes were not
  a half-measure someone abandoned; they are a working scheme with one lapsed
  instance. The row's *"suffix every Objective-C class"* estimate was an order of
  magnitude out because nobody had read the comment three lines above.
- **A per-class question needs a per-class answer.** "Seven classes collide"
  invited one sweeping fix. Diffing each implementation against its introducing
  commit gave four verdicts, and only one needed work.
- **A correct sibling is evidence about the design.** `KEY_LISTENER` being
  byte-identical across the same commits is what distinguishes "someone forgot"
  from "this does not work".
- **Grep for string-based class lookup before renaming an Objective-C class.**
  `NSClassFromString` turns a compile-time-safe rename into a runtime failure.
- **Confirm an override reached the compiler, not just the command line.** Wrong
  variable name, `[fetched]` in the report, and a "verified" fix that was never
  built. Checking the artifact for the new symbol is the check that cannot lie.

**Next:** the two **unversioned** classes are the remaining work — two small
changes in `MacKeyListener.h` and `MacTextEdit.h`. `GMPI_VIEW_MAKER_VERSION_02`
is **unchecked rather than cleared**. And the row's original question is still
open: whether a collision has ever caused an observed misbehaviour, as distinct
from the condition being demonstrably able to produce one.

**Branch/PR:** `tide/mac/S38-measured` — TideSynth; the fix is
[gmpi_ui#11](https://github.com/JeffMcClintock/gmpi_ui/pull/11).

---

## 2026-08-22 — macos — E1c's second discriminator, and verifying the pitch before seeding it

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** built the case the first experiment left open, and seeded its reference
on macOS. One Linux or Windows render finishes it.

### Where E1c actually stands

```
osc_naive_sine      naive osc, pitch UNDRIVEN          -73.5 dBFS
osc_naive_pitched   naive osc, pitch pinned to 5 V     -73.5 dBFS
voice_midi_note     naive osc, MIDI note 64            -123.1 dBFS
```

The first two killed my *undriven pitch input* hypothesis — driving the input
changes nothing. But the third uses the **same module** and sits 50 dB away, so
"the module is the variable" does not survive either. The only difference left
between them is the **pitch value**: 440.0 Hz against note 64.

`osc_naive_note64.json` is the naive oscillator at 4.583333 V — 5 V minus five
semitones, on the 1 V/octave convention where 5 V is 440 Hz — and nothing else
changed.

### The step I nearly skipped

I almost seeded the reference straight after the render. **A golden seeded on an
unverified frequency makes every later residual meaningless**, so I counted zero
crossings first:

```
measured  329.50 Hz      (95999 frames @ 48 kHz)
note 64   329.63 Hz      -> inside the counting resolution over 2.0 s
```

Only then did I write the reference. That is the same discipline the linux box
applied to the first case — it verified both cases rendered at *exactly* 440.0 Hz
before trusting them — and it is what makes these single-variable rather than
merely single-edit.

Level is held constant as a second control: all three `osc_naive_*` cases peak at
**−6.0 dBFS**, so a difference in residual cannot be a difference in level.

### Pre-committed, again

The case's own `tolerance_reason` states both outcomes before anyone has seen the
number:

- near **−123 dBFS** → the discriminator is the **pitch value**; 440.0 Hz happens
  to have a phase increment that rounds differently across platforms and note
  64's does not. That explains every case on the board and makes
  `prefab_oscillator`/`prefab_filter`'s wide gates plainly wrong.
- near **−73 dBFS** → the pitch value is not the variable, which leaves the
  ADSR/VCA voice chain as the remaining explanation for `voice_midi_note`, and is
  a much less comfortable place to be.

Gates stay provisional drift-class, copied from `osc_naive_sine`, and the case
says so — gating it as though the answer were known would beg the question.

**Learned:**

- **Verify the thing a golden encodes before writing the golden.** A reference is
  a claim; seeding one at an unconfirmed frequency would have produced a number
  that looked like evidence and was not. Thirty seconds of zero-crossing counting
  buys that.
- **Two eliminated hypotheses can leave a third that neither suggested.** "Which
  module" and "is the input driven" both died; the pitch VALUE was never
  considered until both were gone, and it was visible in the fixtures the whole
  time.
- **Hold the control constant and say which control.** All three cases peak at
  −6.0 dBFS, which is what lets a residual difference mean something. Naming it
  in the row costs a sentence and stops the next person re-checking.

**Next:** **a Linux or Windows render of `osc_naive_note64`** against this
macOS-seeded reference settles it. Either platform can. **E1c stays TODO** — and
its real cost is still `prefab_oscillator` and `prefab_filter` carrying
drift-class gates, which this case informs but does not fix.

**Branch/PR:** `tide/mac/E1c-pitch-value` — TideSynth.

---

## 2026-08-22 — macos — the AUv3 works, and my "it doesn't register" was a wrong query

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took the mac cell's item — settle whether the AUv3's registration
failure was expected. It was not a failure at all.

### I reported a defect that did not exist

Earlier today I wrote that the extension "did not register via `pluginkit`/`auval`
with an ad-hoc signature" and put that on the row as the unproven part.

```
pluginkit -mv | grep -i tide                             -> nothing
pluginkit -m -i com.gmpi.au3.TIDE_Rack.extension -v      -> lists it
```

The first form does not enumerate the extension; the second queries it directly.
**My query was wrong and I called it an extension problem.** Then:

```
auval -v aumu Drck Dsyh
    This AudioUnit is a version 3 implementation.
    Loaded AudioUnit out-of-process: true
    AU VALIDATION SUCCEEDED           rc=0
```

Under an ad-hoc signature. The AUv3 has worked the whole time.

Worth noting how the earlier session went wrong twice in the same direction:
first I stripped the sandbox entitlement with my own `codesign --deep` and
suspected the wrapper, then I used a query that cannot return the answer and
suspected the wrapper again. Both times the tooling was fine and my instrument
was not.

### And then the actual finding

AU2 and AU3 declare the **same four-character codes**. With both installed:

```
auval -a          ->  exactly ONE  aumu Drck Dsyh
auval -v          ->  "version 3 implementation", out-of-process
```

**The `.component` never loads.** That is by construction — the AU3 README says
the v3 plist derivation is *"the same one the AU2 `.component` gets, so v2 and v3
share their fourCCs"*, which is correct practice for one product shipping both,
and it means exactly one is reachable.

**It matters right now** because R3a added `TIDE-Rack.component` to the macOS pkg
hours ago. Enabling `AU3` today would ship a component that can never load —
worse than not shipping it, because the user sees it installed.

So `AU3` stays out of `FORMATS_LIST` and the choice is filed as **S40**. That is
a product decision — v3 only, v2 only, or distinct subtypes — not something to
settle by measurement.

**Learned:**

- **When a tool reports nothing, check the query before blaming the subject.**
  `pluginkit -mv` and `pluginkit -m -i <id>` answer different questions, and only
  one of them can find an extension by identifier. I built a row's "unproven"
  clause on the wrong one.
- **Two wrong diagnoses in a row, both pointing at someone else's code, is a
  signal about the instrument.** Stripping entitlements with my own `codesign`,
  then querying with the wrong `pluginkit` form — the wrapper was correct both
  times.
- **Sharing fourCCs between v2 and v3 is correct AND means one is unreachable.**
  Both halves are true, and the second only shows up if you install both and
  count what `auval -a` returns.
- **Prove the artifact works before asking which artifact to ship.** The S40
  decision is cheap to make precisely because registration and validation are
  already demonstrated; asking Jeff to choose between a working thing and an
  untested one would have been a worse question.

**Next:** **S40 is Jeff's ruling** and blocks `AU3` shipping. **M2 is now much
smaller than its row says** — it treats authoring an AUv3 wrapper as the blocker,
one landed 2026-08-19, and the macOS half is proven; what remains is genuinely
iOS (simulator install, sandbox rules). **A separate small finding** from the
same run: the AUv3 warns *"CurrentPreset property is deprectated. AU should
implement PresentPreset"* — recorded on S40, unexamined.

**Branch/PR:** `tide/mac/M1-au3-registration` — TideSynth, backlog and journal only.

---

## 2026-08-22 — macos — STEP 4 after v0.1.0, and a branch I pushed and never opened

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** bookkeeping after the first release run. **R3a, R4, S30 and S39** all
flipped to DONE, and one branch that had been pushed with no PR finally landed.

### The branch nobody was waiting on

`tide/mac/R5-installer-cert-finding` — the write-up of *why* the macOS leg failed
— was committed and pushed hours ago, and **no PR was ever opened for it.** I
pushed it, Jeff asked about ccache in the same breath, and I never came back.

It surfaced only because a STEP 4 sweep asked which IN-REVIEW rows had merged
PRs, and R5's had no PR at all. Nothing was lost, but the finding sat invisible
while the thing it describes was being fixed.

### Four rows, and what closed them

- **R3a** — confirmed in CI, not just locally: v0.1.0 built the AU, `codesign`
  called the component *"valid on disk"*, and `pkgbuild` added both payloads on a
  machine that had never seen the change.
- **R4** — the Linux leg of v0.1.0 succeeded, so `package-linux.sh` is exercised
  on a real tag rather than only on the box that wrote it.
- **S30** — closed on measurement rather than on the fixes landing: 71.5 and 60.1
  min before ccache, 0.3 and 0.2 min after. **Both caveats kept**: the exact
  speedup is not established, and both post-ccache runs were docs-only merges.
- **S39** — the answer was in the code the whole time. `Initialize()` carried a
  TODO stating the mechanism precisely, ending *"This is work still to be done"*.
  A row I filed as "unknown to fix" was a named pattern waiting to be applied.

### The one that is worth remembering

S39 said *"nobody has looked at what it means"*. Looking took one grep, and the
answer was a comment the original author had left explaining exactly what was
missing and what to copy. The row's cost estimate — *"small to measure, unknown
to fix"* — was wrong in the direction that matters: reading the code the row
pointed at would have sized it in minutes.

**Learned:**

- **A pushed branch with no PR is invisible.** Nothing checks for it — not the
  lints, not the row status, not the PR list. The STEP 4 sweep found it only
  because the row it belonged to had no PR to verify. Open the PR in the same
  breath as the push.
- **"Unknown to fix" deserves one grep before it is written.** S39's mechanism
  was documented in the function the row named. An honest unknown and an
  unread comment look identical on a row.
- **A row can be closed by a run rather than by a commit.** R3a and R4 were
  already merged; what closed them was v0.1.0 exercising them on machines that
  had never seen them. Worth distinguishing "landed" from "demonstrated".

**Next:** **M1's AUv3 half** is the largest thing this box can still both change
and verify — the wrapper exists, TIDE builds an appex from one word, and GMPI#10
fixed the name mismatch. **What is unproven is registration**: the extension did
not appear via `pluginkit`/`auval` with an ad-hoc signature, and whether that is
expected is the question to settle before `AU3` joins the shipped list.
**M2's row is four days stale** — it treats authoring an AUv3 wrapper as the
blocker, and one landed 2026-08-19.

**Branch/PR:** `tide/mac/step4-after-v010` — TideSynth, bookkeeping only.

---

## 2026-08-22 — macos — v0.1.0: Windows and Linux shipped, macOS wanted a certificate nobody had sent

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** cut `v0.1.0` and watched the pipeline run for the first time. **Windows
and Linux succeeded, signing included.** macOS failed at `Package (macOS)`:

```
productbuild: error: Could not find appropriate signing identity for
              "Developer ID Installer: SynthEdit Limited (36SNPLRFK3)"
```

### The correction, and it is mine

Earlier today I wrote on R5 that the missing credential was *"now
provisioned"*, because `APPLE_INSTALLER_SIGNING_IDENTITY` had appeared in the
`release` environment between one check and the next.

**The variable was provisioned. The certificate was not.** The workflow logs the
keychain after import, and it held exactly one identity — `E112A74081E6…`, the
Developer ID **Application** cert. `APPLE_CERT_P12_BASE64` carries only that
one, so `productbuild` had nothing to sign the pkg with.

Naming an identity is not the same as shipping its private key, and I treated a
variable appearing as evidence that the credential behind it existed. It is not
even weak evidence — the two are stored in different places, by different
mechanisms, for different reasons.

**The logging is what caught it in seconds.** I put `security find-identity -v
-p codesigning` at the end of the import step "so the job says what it can
actually sign with". That line turned a one-word error into a diagnosis.

### Two risks this run retired

**The ambiguity hazard is dead.** I flagged that the mac box holds two valid,
identically-named Developer ID Application certs, and that if
`APPLE_CERT_P12_BASE64` carried both, `codesign` would fail as *"ambiguous"*. It
carries one. `codesign` signed cleanly, and the risk is now closed by
measurement rather than left open as a caveat.

**R3a is confirmed in CI, not just locally.** Everything up to `productbuild`
worked on macOS: the AU built, `codesign` reported the component *"valid on
disk"* and *"satisfies its Designated Requirement"*, and `pkgbuild` added
**both** payloads. The change I landed an hour before the tag did what it
claimed on a machine that had never seen it.

### What this cost, and what it did not

The failed leg cost about an hour of macOS build time and no artifacts —
`publish` is `needs: build`, so it skipped rather than publishing a partial
release. **No half-finished release was created, and no tag needs deleting.**
That is `fail-fast: false` plus a gated publish doing exactly their job.

**Learned:**

- **A configuration variable naming a credential is not the credential.** They
  live in different stores. Seeing `APPLE_INSTALLER_SIGNING_IDENTITY` appear
  told me its *name* was known, and I wrote "provisioned" — which is a claim
  about the private key, and I had checked nothing about the private key.
- **Log what the job can actually do, not what it was configured to do.**
  Printing the keychain's identities after import turned this from "signing
  failed" into "the keychain has one cert and it is the wrong kind" with no
  extra round trip.
- **A release that fails before `publish` costs time and nothing else.**
  `needs: build` meant no partial release, no orphaned assets and no tag to
  delete. Worth keeping in mind against the temptation to publish per-platform
  as each finishes.
- **Two platforms passing is real evidence.** Azure Trusted Signing is now
  proven end to end on a real tag, which no amount of structural assertion could
  have established.

**Next:** **Jeff exports a `.p12` containing BOTH identities** — both are on the
mac box (`security find-identity -v` lists `D55D4DDE…` "Developer ID Installer",
valid to 2027-02-01) — base64s it and updates `APPLE_CERT_P12_BASE64`. **No
workflow change is needed**: `security import` handles a multi-identity P12 and
the import step already passes `-T /usr/bin/productbuild`. Then re-run the
failed job. **Notarization is still unverified** — the run never reached
`notarytool`, so Apple has never seen a submission and **R6** is blocked on the
same question it was this morning.

**Branch/PR:** `tide/mac/R5-installer-cert-finding` — TideSynth, backlog and journal only.

---

## 2026-08-22 — macos — ccache went into build.yml and not release.yml, and the numbers are in

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** Jeff asked whether the macOS build was still slow and whether a
self-hosted runner was needed. Both halves of the answer turned out to be
measurements I had promised and not yet taken.

### The gap: I fixed CI and not releases

S30 added ccache to `build.yml`. `release.yml` has **zero** ccache references —
I wrote them as separate items and never went back. So every release paid the
full ~60 minutes on macOS, which is the one place the wait is actually felt,
because a person is standing there waiting on a tag.

Now fixed, with the cache key prefix **deliberately matching `build.yml`'s**: a
tag run can read caches created on the default branch, so a release starts warm
from whatever `main` last compiled rather than from nothing.

### The measurement I owed

macOS `Build` step, either side of the ccache merge:

```
02:36   71.5 min    before
02:36   60.1 min    before
03:26    0.3 min    after
04:21    0.2 min    after
```

**I was about to report that as "60 minutes to 15 seconds" and stopped.** The
one hit-rate sample I pulled read **66.5% (1068 hits / 537 misses)** — and 537
C++ compiles do not finish in twelve seconds. Correlating properly showed I had
taken the duration from one run and the statistics from another.

So what is solid is narrower than the headline: **both post-ccache macOS builds
finished in well under a minute against 60+ before.** The exact speedup is not
established, and both post-ccache runs were docs-and-backlog merges whose C++ was
largely unchanged — a run that genuinely recompiles will be slower than 0.2 min.

That is still decisive for the question Jeff asked. **No self-hosted runner is
needed:** that option was sized against a 60-minute build, and the build is no
longer 60 minutes.

### A choice worth naming rather than sliding past

This caches the build of a **signed, shipped artifact**. ccache keys on
preprocessed source, compiler and flags, so a hit is a byte-identical object —
the same assumption an incremental local build makes every day. It is a
deliberate trade, not an oversight, and the comment at the point of use says how
to force a cold build if it is ever in doubt.

**Learned:**

- **Two workflows that build the same thing need the same fixes.** I treated
  "CI is slow" and "releases are slow" as one problem and fixed one file. The
  release path is the one with a human waiting on it.
- **Correlate a duration and its statistics to the same run before quoting a
  ratio.** 66.5% hits alongside a twelve-second build is a contradiction, and the
  contradiction was mine — two different runs. The narrower claim survives; the
  headline number did not.
- **Say what a cache key prefix couples.** `release.yml` sharing `build.yml`'s
  prefix is what makes a release start warm, and it silently stops working if
  either is changed alone.
- **An option sized against an old measurement expires with it.** The
  self-hosted runner was the right answer to a 60-minute build. It is not the
  right answer to this one, and nothing about the runner changed.

**Next:** the v0.1.0 rerun is in flight and will **not** benefit from this — it
started before this branch exists. The first release that does is the next tag.
**Notarization is still the unproven step**; nothing here touches it.

**Branch/PR:** `tide/mac/S30-ccache-in-release` — TideSynth.

---

