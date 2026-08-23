# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-23 — windows — #314 reproduced here: 5 failures in 25 builds of plain `main`, then 0 in 40

**Prompt:** 5146a612b · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1. The only open `platform:win` issue is
[#314](https://github.com/JeffMcClintock/TideSynth/issues/314), filed from the
macos box, which said plainly that it could not run any of it and that whoever
took it should *"reproduce locally under `--parallel` ... and verify by
construction, not by exit code."* That is what this run is.

### The issue is evidence, not instruction, so it got reproduced before it got fixed

**It reproduces on the unmodified tree, at a rate nothing in CI would suggest.**
A full Release build of `origin/main` (rc=0, all four Windows artifacts), then 25
iterations of: delete the four binaries and `build/SynthEditSem/Resources`, then
`cmake --build --config Release --parallel`.

```
ITER  8  Error copying file (if different) ... MidiPlayer2/MidiPlayer2.xml -> .../Release/../Resources/MidiPlayer2.xml
ITER 14  Error copying file (if different) ... ControlsXp/ControlsXp.xml   -> .../Release/../Resources/ControlsXp.xml
ITER 19  Error copying file (if different) ... VaFilters/VaFilters.xml     -> .../Release/../Resources/VaFilters.xml
ITER 20  Error copying directory ... TideModules/prefabs -> .../Release/../Resources/Prefabs
ITER 23  Error copying file (if different) ... ControlsXp/ControlsXp.xml   -> .../Release/../Resources/ControlsXp.xml
RESULT: 5 failures out of 25
```

Iteration 20 is the CI failure verbatim, down to the unnormalised
`Release/../Resources` the issue read off the log. **20% here against roughly one
Windows job in twelve in CI** — the mechanism is the same and the exposure is
not, because a local relink puts all four targets in the staging window at once
while a CI build spreads them across a cold compile.

**Four racers, confirmed rather than inferred.** The issue could only infer
`TIDE_Rack_STANDALONE`'s participation from `FORMATS_LIST`. The control build's
log has all four: `Staging rack prefabs (TIDE_Rack)`, `(TIDE_Rack_VST3)`,
`(TIDE_Rack_CLAP)`, `(TIDE_Rack_STANDALONE)`. On Windows `AU3` builds no target,
so it is four and not five.

### The fix, and why it is a shared target rather than a moved directory

`SynthEditSem/CMakeLists.txt` now stages Windows resources **once**, in a
`TIDE_Rack_stage_resources` custom target that every format target
`add_dependencies` on, instead of hanging the identical five copies off four
targets' `POST_BUILD`. `_tide_xmls` was hoisted out of the per-format loop so the
new target and the surviving per-target commands read the same list — the file's
own comment already forbids two copies of it.

**Every other platform is untouched and keeps the per-target commands**, because
there each format target owns its own bundle directory and there is nothing
shared to collide over. The Windows arm sets `_tide_resources` to the empty
string, which is what skips the per-target copies; the arm's comment says so,
because "no resources on Windows" is exactly the wrong reading of that line.

**The A/B, on the real tree, same box, same recipe:**

| tree | parallel builds | failures |
|---|---|---|
| `origin/main` (`2d15e13f3`) | 25 | **5** |
| the same tree + this fix | 40 | **0** |

`0.8^40` is about `1.3e-4`, so the fixed tree is not a lucky run of the broken
one.

**And a synthetic control, because 5-in-25 is a rate and not a mechanism.** Four
executables, one output directory, the identical five copies into one
`$<TARGET_FILE_DIR>/../Resources`, and a 200-file prefabs directory to widen the
copy window: **8 failures in 20** with the copies per-target, **0 in 30** behind
one shared target. Same shape, same error text, no TIDE in it at all.

**The staged output is byte-identical to what `main` produces** — `diff -r`
between the control tree's `Resources` and the fixed tree's is clean: four XMLs
and six prefabs, same place, same content. This changes when the copies run, not
what they write or where.

### Two things I checked because they are how a dedup like this goes wrong

- **Building one format target still stages.** `cmake --build --target
  TIDE_Rack_VST3` on a tree with `Resources` deleted brings the staging target in
  via `add_dependencies` and produces all four XMLs and six prefabs. A
  build-everything-only fix would have looked identical in every measurement
  above.
- **A no-op build costs 0.9 s and rewrites nothing.** A custom target has no
  outputs, so its commands run on every build where a `POST_BUILD` ran only on a
  relink. All five are `*_if_different`, so the staged mtimes are unchanged; the
  cost is five stat-and-skip invocations.

### Not fixed, deliberately

**S36's destination is untouched.** The resources still land one directory above
the binary, so a Windows developer build still has an empty module browser —
that is S36's defect, not this one, and the two are independent. Recorded on that
row: dropping the `/..` would not have helped here, and now that the staging is
de-duplicated, S36's (a) is a one-expression change with no race left attached to
it.

`scripts/package-windows.ps1` reads the same destination and needed no change;
its comment naming `POST_BUILD` as the writer did, and got one.

**The default branch builds on Windows.** `origin/main` at `2d15e13f3`: configure
rc=0, build rc=0, `TIDE-Rack.gmpi` / `.vst3` / `.clap` / `.exe` all emitted. No
new platform issue to file.

**Learned:**

- **A race whose CI rate is about 8% was 20% on a developer's box, and the
  difference is the compile.** CI spreads four link steps across a cold build; a
  local relink puts them in the same 6 ms. Anyone judging "how often does this
  really fire" from the CI history alone would have under-rated it by more than
  twofold.
- **Reproduce in the real tree even when a synthetic repro is easier.** I wrote
  the four-target synthetic first because it was cheap and gave a loud signal. It
  was the real tree that produced the CI error *verbatim*, including the
  `Release/../Resources` spelling — which is what makes this the same bug rather
  than a bug of the same shape.
- **Delete the outputs, not the build tree, to re-run a POST_BUILD race.**
  Twenty-five iterations took 48 seconds because only the four link steps and the
  staging re-ran. A fresh configure per iteration would have made the same
  experiment cost an hour, and I would have run five iterations instead of
  twenty-five.
- **A dedup has a second failure mode the first one hides: building one target.**
  Moving work out of `POST_BUILD` into a shared target is only correct if the
  dependency edges are there, and every measurement of the race passes whether
  they are or not.

**Not verified:** CI itself — this is verified on this box against a local
`main`, and by construction rather than by watching a run go green, which is the
one thing #314 says proves nothing. The four sibling repos on this box are 2, 3,
3 and 8 commits behind their `origin/main` and were **not** pulled (they are
Jeff's checkouts, they were clean, and this change is TIDE-only CMake), so CI
will build the same file against slightly newer siblings than I did. macOS and
Linux were not built — the change is inside `if(WIN32)` and an `else()` arm those
platforms never reach, but nobody ran them.

**Machine left clean.** All work in two throwaway worktrees, `C:/SE/wt314` and
`C:/SE/wt314c`, removed afterwards; nothing was built in `C:/SE/TideSynth`, and
no plug-in was installed. `C:/SE/TideSynth` had one pre-existing dirty file,
`tools/tidepanel-screenshot.synthedit` — a real content diff, not CRLF churn
(`git diff --ignore-all-space` shows it) — so it is Jeff's work in progress and
was left exactly as found. `SE16`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers` and
`GMPI` were clean and on their default branches at the start, were not written
to, and still are.

**Next:**

1. **S36 is the natural follow-on and it is now smaller than its row says** — its
   (a) is one expression, the objection that it would not fix the race is spent,
   and it is `any` platform so it does not have to wait for this box.
2. **#314 stays open until the PR merges**, which closes it. A green CI run on
   this branch is not evidence either way, for the reason the issue gives.
3. **The win NEXT cell still points at P11**, untouched by this run.

**Branch/PR:** `tide/win/issue-314` — TideSynth, [#321](https://github.com/JeffMcClintock/TideSynth/pull/321).

---

## 2026-08-23 — macos — TIDE in GarageBand: the editor is blank, and four suspects are dead (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Jeff offered to try it in GarageBand, which closed the gap M1 shipped with —
*"the extension was never opened in a real DAW; auval is the evidence, not a
host."* It should have been closed before M1 merged, not after.

**IT LOADS AND IT DOES NOT DRAW.** GarageBand lists it under AU Instruments >
TIDE Synth > TIDE Rack, instantiates it, exposes its parameters in Smart
Controls, and runs it out-of-process with no crash report. Its editor opens a
1100x600 window containing **nothing** but a ~28x26pt fragment top-left.

**Four suspects eliminated, each by measurement, none by reading code.**

1. **"The empty rack measures tiny."** My theory, and wrong. Jeff authorised
   temporary logging; `measure()` returns **1100.0 x 600.0** and
   `preferredContentSize` is set to exactly that. The window really is that big.
2. **"The wiring never completes."** It completes. `connectEditorToUnit`
   early-returns once while `_audioUnit` is still null — by design — is
   re-entered when the unit arrives, and then `createNativeView` returns a
   **valid view at 1100x600** and `initUi` returns with `subviews=1`.
3. **"It is silent, so the DSP is broken."** Jeff heard the metronome and no
   plug-in audio. But `TideApp.cpp:546` creates a blank document — *"creates an
   empty main container"* — so **an empty rack is silent by design.** Jeff asked
   the question that killed this one: *"does tide load a playable patch by
   default?"* It does not. Silence was never the bug; the bug is that you cannot
   build a patch in an editor that will not draw.
4. **"The timer is not wired up."** Jeff's hypothesis, and the best of the four
   because it is where the drawing actually comes from. Instrumented: in the
   AUv3 `Timer::start` runs on the **main thread**, with
   `CFRunLoopGetCurrent() == CFRunLoopGetMain()`, and the callback **fires
   repeatedly**. The timer is fine.

**NEGATIVE CONTROL, and it is what makes all of this a real bug rather than a
misconfiguration: the STANDALONE renders perfectly** — same build, same engine,
full module browser and rack.

**THE LIVE LEAD came out of eliminating suspect 4, not out of confirming it.**
The standalone starts **THREE** timers; the AUv3 starts **ONE**. Two
timer-driven subsystems never start inside the extension.
`SynthEditLib/ModulePicker.h` declares two `gmpi::TimerClient` subclasses — the
module browser, which the standalone shows and the AUv3 does not. That is where
I would look next.

**Filed as M4.** All diagnostic logging lived on throwaway branches in `gmpi_ui`
and `GMPI_Wrappers`, both now deleted; `grep TIDEDIAG` is clean in every repo.
The dev build was removed from `/Applications`, and both GarageBand projects
touched — Jeff's own "Test Preset Change" and my scratch "Untitled" — were
closed **without saving**.

**Also seen and NOT diagnosed:** closing the editor made GarageBand report *"An
Audio Unit plug-in reported a problem which might cause the system to become
unstable"*, with the appex process gone and **no crash report written**. The
teardown path (`dealloc` -> `gmpi_onCloseNativeView`) is the obvious suspect and
is unmeasured. It is in M4's row so it is not lost.

**Not verified:** why the view does not paint. Everything upstream of the draw
is now eliminated, which is progress, but the draw path itself was never
instrumented.

## 2026-08-23 — macos — M1 closes, and it takes two of my own claims down with it (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

M1 was never blocked on work — its row says so: *"BLOCKED ON A RULING, NOT ON
WORK"*. AU2 and AU3 share fourCCs, so `AU3` stayed out of `FORMATS_LIST` until
Jeff chose. **He chose: S40 dropped AU2**, so the clash is gone by construction
and the row only needed evidence.

**The evidence, from plain `main`:** `AU3` is in `FORMATS_LIST`, the build is
rc=0 with zero errors and produces all five artifacts, the extension registers,
and `auval -v aumu Drck Dsyh` reports *"version 3 implementation"*, *"Loaded
AudioUnit out-of-process: true"*, **AU VALIDATION SUCCEEDED**, rc=0.

**Then two things in the row turned out to be wrong, and both were mine.**

**The identifier it cites is stale.** `pluginkit -m -i
com.gmpi.au3.TIDE_Rack.extension -v` returns "(no matches)" — GMPI#13 made the
appex id derive from the containing app, so it is now
`com.tidesynth.tiderack.au3app.extension`. Anyone following the row's own
command would conclude registration is broken. That is precisely the false
negative this row already records me making once, preserved in amber and waiting
for the next person.

**"macOS registers the AUv3 automatically, with no launch required" is wrong.**
I wrote that during S40, in `SynthEditSem/CMakeLists.txt` and — much worse — in
user-facing `docs/distribution.md`. Re-measured today, deliberately, because the
first `pluginkit` query came back empty and I wanted to know whether that was
the stale id or a real gap:

  - `~/Applications`, 30s after the copy: **(no matches)**
  - `/Applications`, at 5s, 15s and 30s: **(no matches)**
  - within **12 seconds of `open`ing the app**: registered, and reporting
    version 0.1.1, which is R10's wiring showing up in a third place

So the install story is a copy **plus a first launch**. A user who follows the
docs as they stood installs the pkg, opens their DAW, and finds nothing. The pkg
puts the app in place and cannot open it for them, so whatever tells the user how
to install this has to tell them to open it once. Both places corrected.

**What made the difference was not being satisfied by the first "(no matches)".**
The tempting read was "stale id, mystery solved" — the id WAS stale, and fixing
it alone would have left the doc bug shipping.

**Not verified:** the extension was never opened in a real DAW; `auval` is the
evidence, not a host. Nothing was tested on a clean machine, so whether the first
launch is needed once per install or once per user is unmeasured. The dev build
was removed from `/Applications` and `~/Applications` afterwards, so this box is
back to where it started.

## 2026-08-23 — macos — I broke the Linux release with the class I spent all day fixing (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

v0.1.1 failed. Windows succeeded, macOS failed at notarization (Apple, below),
and **`build-linux` failed because of me**:

    ##[error]missing or empty asset: TIDE-Rack-Linux.tar.gz

R10 renamed the tarball to `TIDE-Rack-Linux-0.1.1.tar.gz` while `release.yml`'s
asset check still expected the old name. **Two places that must agree and I
changed one** — the exact class I fixed four times today: the AU2 class name
beside its string, the MIDI menu beside its handler, the appex name beside its
executable, the XML chooser beside its consumer. I wrote the commit messages for
all four and then introduced a fresh instance.

**And the rename was wrong on its own terms, which one line of the file I was
editing would have told me.** `release.yml`'s header says: *"Asset names carry NO
version (docs/distribution.md): the version lives in the tag, so
`releases/latest/download/TIDE-Rack-macOS.pkg` is a permalink R6 can promise
never to change."* Two identically-named files across releases is the POINT.
I had justified the rename in a comment as fixing a problem that was a
deliberate design decision, stated in the file, which I had not read.

Reverted the name. The rest of R10 stands and is what actually matters: the
version lives in the tag and INSIDE the artifacts — `<Plugin version="0.1.1">`
drives every bundle, `--version "$VERSION"` drives the pkg metadata — not in the
filenames.

**The notarization retry worked exactly as designed and did not help.** The log
shows `notarytool submit (attempt 4 of 4)` and then *"Apple's notary service
still failing after 4 attempts"* — so the code from #311 ran, correctly
identified four consecutive 500s as transient, and backed off through all of
them. **Apple returned HTTP 500 at 07:22, 10:40 and 20:58 UTC**, which is not a
blip; it is a service that has been failing all day. The retry covers about
fifteen minutes. Nothing in TIDE can fix that, and the right response is to wait
rather than burn another build.

**Verified:** the Linux tarball name matches `release.yml`'s expectation again
(`TIDE-Rack-Linux.tar.gz`), and the version is still carried inside the
artifacts. **Not verified:** the Linux packager was not executed — this is a
name match by inspection, and the proof is the next release run.

## 2026-08-23 — macos — "bump to 0.1.1" was not a one-number change (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Jeff asked to increment the version. Before touching anything I looked for where
the version lives, and found **four consumers giving four different answers,
none of them the tag**:

  - the BUNDLES said `1.0.0` — GMPI's documented default for a `<Plugin>` tag
    with no `version` attribute, and TIDE's carried none;
  - `package-macos.sh` fell back to a hardcoded `0.1.0`;
  - `package-windows.ps1` read `$env:TIDE_RACK_VERSION`, which **nothing ever
    set**, so it got an empty string;
  - `package-linux.sh` put no version in the tarball name at all, so two
    releases would have produced two identically-named files.

`release.yml` mentioned `TIDE_RACK_VERSION` **zero times**. Tagging `v0.1.1`
would have shipped artifacts labelled 1.0.0, 0.1.0, nothing, and nothing.

**Fixed by giving it one source.** `SynthEdit.cpp`'s `<Plugin>` tag now carries
`version="0.1.1"`, which every bundle derives from; `release.yml` derives
`TIDE_RACK_VERSION` from the tag; the two shell packagers' fallbacks match it.

**The release.yml half is a shell step, not a job-level `env:`, and I got that
wrong first.** The leading `v` has to come off, and I wrote
`substring(github.ref_name, 1)` — **GitHub's expression syntax has no substring
function.** The YAML parsed happily, because it is just a string until Actions
evaluates it; nothing would have complained until a tagged release ran. Replaced
with `${GITHUB_REF_NAME#v}` in a `shell: bash` step, so the one script also runs
on the Windows runner.

**Verified by building:** all four macOS bundles — `.vst3`, `.clap`, `.gmpi` and
the AUv3 app — report **0.1.1** where the same build previously reported
`1.0.0`. The tag-strip was exercised directly (`v0.1.1` → `0.1.1`).

**And the first build after the change still said 1.0.0**, which was worth the
five minutes it cost. The local GMPI checkout was **14 commits stale**, so
`gmpi_find_plugin_element` still preferred a MODULE source for the plugin XML
and never saw the new attribute. GMPI#12's `<PluginList>` preference is what
makes this work, and the stale build was an accidental demonstration of the very
bug that PR fixed. `git pull` in the local checkouts, rebuild, 0.1.1 everywhere.
[[cmake-local-repo-overrides]] warns about exactly this and I still hit it.

**Not verified:** no packager was executed — they need signing identities and a
release environment — so the version reaching the installer FILENAMES is by
inspection, not by run. Windows and Linux packaging were not exercised at all.

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
