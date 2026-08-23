# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-24 — windows — S44: the stranded reference split, landed and verified on the platform that could not check it

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** (Claude Code 2.1.237) · as **tide-rack-bot** (both paths)

**Did:** took **S44**. Both NEXT cells that could point here — `win` and `any` —
named it as the single ungated row left on the board, filed by the mac box
eighteen hours earlier while it cleaned up after E9. STEP 1 clear (no open
`platform:win` issues), STEP 1.5 clear (no open PR from `tide/win/**`).

### The row's premise, and the thing it could not know

`origin/tide/mac/S27-render-ci` carries the per-platform reference split whose
PR [#331](https://github.com/JeffMcClintock/TideSynth/pull/331) had already
merged when the follow-ups were pushed onto it. Its own commit message ends:

> Not verified: Windows and Linux have not yet run against `windows-linux/`.
> That is the next CI run, and the 0.083% figure predicts both pass.

**This is one of those two platforms.** The claim was measurable here and nobody
had measured it, so that came before deciding what to do with the branch.

### The measurement

`tide_render_regression` built from `main` at `7b34d8155`, MSVC 14.51 x64,
Release, in a scratch tree.

| references | result |
|---|---|
| **`windows-linux/` from the stranded branch** | **10 of 10 match, rc=0** — three consecutive runs identical |
| `main`'s current flat `tests/references/` (the macOS-arm64 bake from `246399a`) | **5 of 10 FAIL** |

The failing five, against limits of 0.800% and delta 40:

    knob      35.359%  worst delta 142 at (24,39)
    materials 34.847%  worst delta  63 at (117,37)
    shapes    67.014%  worst delta  46 at (126,40)
    glass     54.562%  worst delta  53 at (38,27)
    glow      61.528%  worst delta  62 at (82,12)

All five `-fast` variants pass at 0.000% on both sets, which is the stranded
commit's own claim that Fast is bit-identical everywhere.

**The prediction is confirmed to the digit, not merely in direction.** The
stranded commit measured Windows-vs-Linux as *"0.083% of pixels, worst delta 10
— glass, glow, knob and materials are 0.000%, only `shapes` moves"*. This box,
against images baked on an **ubuntu** runner, reads glass/glow/knob/materials at
**0.000%** and `shapes` at **0.083%, worst delta 10**. Same scene, same figure,
same delta. And the 35–67% macOS gap it quotes reproduces here as 34.8–67.0%.

**So `main`'s render job is red on Windows today** — the Windows half of
[#291](https://github.com/JeffMcClintock/TideSynth/issues/291), which was
labelled `platform:linux` and is not only Linux's.

### The defect the stranded commit had, which is why this is not a straight cherry-pick

It put the platform choice in `build.yml`'s render matrix as a `refs:` column
and updated **only that caller**. There are three:

    .github/workflows/build.yml:611       "$exe" tests/references …
    modules/common/CMakeLists.txt:119     add_test(… "${CMAKE_CURRENT_SOURCE_DIR}/tests/references" …)
    modules/common/README.md:277          tide_render_preview --references modules/common/tests/references

After the split `tests/references` holds no PNGs at all — only two
subdirectories — so `ctest` would have gone red comparing against an empty
directory, and a developer following the README would have re-baked into it.

**Selection now lives in `tide_render_regression` itself.** Hand it the root and
it descends into `macos` or `windows-linux` for the platform it was built for;
hand it a set and it uses that, which is what keeps `--references .../macos`
working for re-approving an intended look change. One change fixes all three
callers, and **`.github/workflows/build.yml` needs no edit at all** — which is
also what puts this inside what a scheduled run may push, since the bot token
deliberately lacks `workflow` scope. **The stranded commit's shape was
unlandable from a scheduled run on any box**, and that is not a small detail:
it is why the branch sat.

### Why not simply open a PR from the branch, which is what the row asks for first

It does not merge. `origin/tide/mac/S27-render-ci` conflicts with `main` in
three files — `.github/workflows/build.yml`, `BACKLOG.md`, and `JOURNAL.md`,
which has rotated since. Resolving it means committing to `build.yml`, and no
scheduled run on any box can push that. The PR would have been unmergeable by
construction and unfixable by the fleet that opened it.

So the substance lands instead, with the expensive part carried over verbatim:
**all twenty PNGs are byte-identical to their sources**, hashed against
`origin/main` (the ten `macos/`) and `origin/tide/mac/S27-render-ci` (the ten
`windows-linux/`). Nothing was re-baked here. The `windows-linux` images came
off a real ubuntu runner, and reconstructing them on this box would have
silently replaced a Linux bake with a Windows one — the two agree to 0.083%,
which is close enough that the substitution would not have shown up in any test
and far enough that it would have been the wrong thing to ship.

**The branch is deliberately left alone.** It is another session's, the standing
rule is not to delete other sessions' branches, and its commits are pushed so no
rewrite is permitted. It is superseded and wants a human to delete it.

**Verified:**

- 10/10 against `windows-linux/`, rc=0, three consecutive runs byte-identical.
- The **exact absolute argument `add_test()` passes** resolves to `…/windows-linux` and passes 10/10.
- The set named directly (what the README documents) — same.
- **Negative control:** `tests/references/macos` named directly → **5 of 10 FAIL**. The resolver does not quietly fall through to the set that would pass, which is the failure a "look for the right directory" fallback most easily hides.
- Twenty reference PNGs hashed against their two sources; all twenty identical.
- Clean rebuild, no warnings.

**Not verified:**

- **Linux.** The other half of `windows-linux/` is still unmeasured on a Linux box. The images came from an ubuntu runner, so the expectation is 0.000% everywhere rather than 0.083% — Linux should agree with its own bake more closely than Windows does. **That is a prediction, and it is the Linux box's to check.**
- **macOS.** The `macos/` set is unmoved bytes and the resolver picks it under `__APPLE__`, but no Mac ran this. That arm is reasoning here, not measurement.
- **CI.** `main`'s workflow line is unchanged and needs to be, which is the point — but the render job has not run with the new binary. The first run on this PR is the test.
- **`ctest` end-to-end.** `modules/common` alone registers `add_test` without ever calling `enable_testing()` — that lives in `modules/CMakeLists.txt:58`, one level up — so `ctest` in a standalone `modules/common` build reports that no tests were found. Pre-existing, unrelated to this change, and not worth a row: the parent build is the one that runs it. I verified the argument instead of the harness.

**Learned:**

- **A "not verified" line in a commit message is an assignment, and the box it is addressed to may never read it.** This one named Windows and Linux explicitly, sat for a day, and was found only because a mac run tripped over the branch while tidying. The verification cost twenty minutes once someone looked.
- **Count the callers before moving a path.** The split moved a directory and updated one of three consumers. Nothing catches that — `ctest` is not in the workflow that was edited, and the workflow is not in the build that runs `ctest`. Grepping the moved path across the tree is one command and it is the whole check.
- **A resolver needs its wrong branch tested, not its right one.** "Root resolves to `windows-linux` and passes" is also what a resolver that ignores its argument entirely would print. Pointing it at `macos` and watching five scenes fail is what separates those.
- **Byte-identity to a source is worth asserting mechanically.** Twenty images that "look right" and twenty images hashed against the two commits they came from are different claims, and only the second survives someone asking where a picture came from six weeks later.
- **A branch can be stranded because of what it contains, not because someone forgot.** This one holds a `.github/workflows/**` edit, so no scheduled run could ever have rebased or merged it. Reading the credential's limits explains a stall that otherwise looks like carelessness.

**STEP 4 bookkeeping, all on verified PR state rather than memory:**

- **E9** IN-REVIEW → DONE ([#347](https://github.com/JeffMcClintock/TideSynth/pull/347) merged).
- **A34** IN-REVIEW → DONE ([#338](https://github.com/JeffMcClintock/TideSynth/pull/338) merged).
- **S41** IN-REVIEW → DONE ([#327](https://github.com/JeffMcClintock/TideSynth/pull/327) and [#315](https://github.com/JeffMcClintock/TideSynth/pull/315) merged).
- **E10 was deliberately NOT flipped.** Its TideSynth PR [#346](https://github.com/JeffMcClintock/TideSynth/pull/346) merged but [SynthEditLib#35](https://github.com/JeffMcClintock/SynthEditLib/pull/35) is still open, and IN-REVIEW means *every* linked PR.
- The `win` cell's own instruction — check S22's PR state — was followed: [#344](https://github.com/JeffMcClintock/TideSynth/pull/344) merged and the row already read DONE.

**Next:**

1. **A Linux run of `windows-linux/`** finishes the set's verification. One build, one command, and the prediction is 0.000% across the board.
2. **`main`'s render job goes green on Windows and Linux when this merges**, which is [#291](https://github.com/JeffMcClintock/TideSynth/issues/291)'s remedy. That issue is `platform:linux`-labelled and is not only Linux's.
3. **`origin/tide/mac/S27-render-ci` wants deleting by a human** once this merges, along with the mac-box worktree registered against it.
4. **The `any` queue now has no ungated row at all.** Both cells say so; the next scheduled run on any box should expect to find nothing takeable and stop rather than invent work.

**Machine left clean.** All work in a throwaway worktree and build tree under the
session scratchpad; nothing was built in `C:\SE\TideSynth`. **Two pre-existing
things on this box were left alone, both predating this run:** `C:\SE\TideSynth`
has a modified `tools/tidepanel-screenshot.synthedit` — real content, not CRLF
churn (`git diff --ignore-all-space` shows the `PanelLocationZoom` and
`panelRect` values changing), so it is the developer's work in progress; and a
registered worktree at `C:\SE\wt345` on `tide/linux/S37-clap-collision`, clean,
whose PR [#345](https://github.com/JeffMcClintock/TideSynth/pull/345) has merged
and whose branch is gone from origin. Neither is this run's. `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were clean and on their default
branches at the start and were never touched.

**Branch/PR:** `tide/win/S44-s27-reference-split` — TideSynth only.

## 2026-08-24 — macos — E9: the AU absorbs a rate change, and the pitch is the proof

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** took **E9** — the mac NEXT cell named S42, which is DONE, so it fell to
the topmost eligible row and the `any` cell pointed here anyway. E9's only
remaining clause is *"AU remains genuinely unmeasured"*, which defers to **R3a**,
which the row calls `BLOCKED(M1)` with *"TIDE builds no AU"*. **Both halves went
stale two days ago:** M1 and R3a are DONE, and `SynthEditSem/CMakeLists.txt:163`
is `GMPI VST3 CLAP AU3 STANDALONE`. So the AU path became measurable and nobody
had noticed.

### The result

`tests/e9_au_rate_probe.mm` — a real AUv3 host handshake, then the
allocate/render/deallocate bracket at **48000 → 44100 → 48000**, which is what a
DAW does on a device rate change. **25 checks, all passing, byte-identical
across three consecutive runs.**

| rate | measured pitch | peak |
|---|---:|---:|
| 48000 | **440.0093 Hz** | −6.29 dBFS |
| 44100 | **440.0093 Hz** | −6.33 dBFS |
| 48000 again | **440.0093 Hz** | −6.29 dBFS |

**−0.000 cents**, against a stale-rate prediction of **404.2586 Hz** — 1.47
semitones flat, the "sounds wrong rather than broken" case the row's original
text calls the worst kind.

### Why this asks a harder question than the VST3 and CLAP halves did

The CLAP probe says so of itself: *"Deliberately NOT a null test: it asserts the
handshake completes and the plugin reports the rate it was given."* That is a
fair test of the mechanism and it is **not this row's Accept**, which is
*"changing the host's sample rate on a loaded project **re-tunes correctly**"*.
A handshake that completes at 44100 says nothing about tuning.

So the probe loads the **actual rack from `tests/hosts/v1-rack.rpp`** — the
fixture documented at 440.0 Hz / −6.3 dBFS — through `setFullState`, and
measures the output pitch. Getting the document there needed no new format
work: the AU3 wrapper carries TIDE's state verbatim as the `GMPIPRESET` key
(`AU3_Wrapper.mm:509,522`), which is the same outer `<Preset>` element the
`.rpp` already holds, so `scripts/decode_rpp.py` grew `--preset-out` and a VST3
fixture drives the AU unmodified.

### The reading is self-validating, which is why 440 twice is a strong result rather than a suspicious one

I did distrust it — identical to four decimal places at two different sample
rates looks like something that is not changing. It is the opposite:

**A plugin that ignored `setFormat`, one pinned to a fixed rate, and a rack that
kept a stale rate all emit samples on the 48 kHz grid for the 44.1 kHz leg — and
all three then measure 404.26 Hz, not 440.** Reading 440 Hz at 44100 is only
possible if the bus rate really changed *and* the rack rebuilt for it. The
failure modes collapse onto the same number, and it is not the number I got.

**Three controls, because a null result is worth what its controls are worth:**

1. `--selftest` measures synthetic tones including the predicted stale-rate
   404.25 Hz. The two hypotheses come out **35.75 Hz apart**, and digital
   silence correctly yields no frequency at all — without that last case,
   "it reported 440" and "it reports 440 for anything" look identical.
2. Each leg's own audio re-read at the *other* rate gives **478.9217** and
   **404.2586 Hz**, matching the predicted ratios to 4 dp. So the analyser
   demonstrably tracks its rate argument.
3. The legs rendered **96000 vs 88200 samples** — not the same buffer.

**The mechanism is the same one this row established for VST3 and CLAP:**
`AU3_Wrapper.mm:577` reads the rate off the output bus and `:600` calls
`plugin.start_processor(...)`, so `processor_holder.cpp` releases the old
processor, creates a fresh one, `open()`s it, and re-seeds the blob from
retained bytes. Instance replacement, on all three wrappers.

### The claim I printed that was false, and the instrument that caught it

The probe reported **"loaded in-process"** for three runs. It was printing the
option I had *requested*, not what happened. Two instruments settled it:
`NSStringFromClass` gives **`AUAudioUnit_XH`**, a proxy, and — dispositive —
walking `_dyld_image_count()` shows the **appex binary is not in this process's
address space at all**.

So the AU is hosted out-of-process, TIDE's own `TIDE: rack built for N Hz`
diagnostic cannot reach the probe, and the absence of that line means nothing.
I had spent time hunting for it in stderr and in the unified log before asking
whether it *could* be there. The audio is the whole of the evidence, and it is
the better evidence anyway.

### The build failure that was mine

The first build came back **rc=2** — codesign failing on a missing
`TIDE-Rack.appex`, with 0 compiler errors — which looks exactly like a
`platform:mac` break worth filing. It was not. The log showed the `TIDE_Rack_AU3`
target compiled and linked **twice**, with the progress counter going backwards
from 100% to 96%: an earlier backgrounded build I believed had been killed was
still alive and building into the same directory as the new one. Two `make`
processes, one build tree, racing over the appex.

A clean single rebuild is **rc=0, 0 errors, all five artifacts**. **So macOS
`main` builds and there is no platform issue to file** — and I nearly filed the
fleet's own #314-class race report against a bug I had caused.

**Verified:** 25/25 probe checks; three byte-identical runs; `--selftest` 6/6;
clean `main` build rc=0 with all five macOS artifacts.

**Not verified:**

- **No real DAW.** `e9_au_rate_probe` is ours. What Logic or GarageBand does on
  an actual device rate change is unmeasured — though the AU API bracket the
  probe drives is the one those hosts use.
- **iOS AUv3 was not exercised at all.** This is the macOS AU only.
- **The plug-in's own diagnostic**, for the out-of-process reason above.
- **Windows and Linux** build nothing AU-shaped, so nothing there was touched.
- **The analyser carries a ~0.2 Hz systematic bias on a decaying tone** (visible
  in `--selftest`: 439.80 for a true 440). It is a threshold artefact, it is
  common-mode across rates, and the verdict is a *ratio* between legs, so it
  cancels. Stated because the absolute figure 440.0093 should not be read as a
  tuning measurement of the rack.

**Learned:**

- **A probe that prints the option it requested is not reporting a measurement.**
  "Loaded in-process" was wrong for three runs and would have gone into this
  entry as fact. The class of the returned object, and better the loaded-image
  list, are the things that actually answer it.
- **When every failure mode collapses onto the same wrong number, the right
  number is strong evidence.** Working out what an ignored `setFormat`, a
  fixed-rate plugin and a stale rack would each measure — all 404.26 — is what
  turned a suspicious-looking 440-at-both-rates into a result.
- **A backgrounded build you think you killed is still building.** The
  double-linked target and a progress counter running backwards are the tell,
  and the failure it produced was a perfect imitation of a real parallel-staging
  race this fleet has already fixed twice (#314, S21).
- **Check whether a row's blocker is still real before believing the row is
  closed.** E9 had one clause left, that clause pointed at R3a, and R3a had been
  DONE for two days. Nothing re-reads a deferral.
- **A fixture saved for one format can drive another without being re-authored**
  when both wrappers carry the same preset XML — one `--preset-out` flag beat
  authoring an AU-specific fixture by hand.
- **`pluginkit -a <appex>` registers an AUv3 straight out of a build tree**, no
  copy into `/Applications` and no first launch, which makes an AU measurable in
  seconds. It is a *developer* shortcut and **does not revise M1's install
  story** for a shipped pkg — and it must be undone with `pluginkit -r`, or it
  leaves a live registration pointing at a deleted tree.

**Next:**

1. **E9 is IN-REVIEW, not DONE** — a later run flips it when the PR merges.
   Its Accept is now met on all three wrappers TIDE builds.
2. **A real AU host is the honest next test**, and it needs a human: Logic or
   GarageBand, change the device rate on a loaded project.
3. **The mac box has run out of ungated scheduled work.** There are no
   `platform: mac` TODO rows at all, and the `any` queue's takeable set emptied
   this week. Both NEXT cells now say so. The real mac-only work left is
   verification that wants a keyboard — starting with **M2's** own record that
   the iOS app was installed but never launched and its Audio Unit never opened
   in an iOS host.

### Found while cleaning up: two commits of S27 stranded with no PR

Checking this box for leftover worktrees at the end turned up one belonging to
another session, on `tide/mac/S27-render-ci` — and that branch is **two commits
ahead of `origin/main` while its PR [#331](https://github.com/JeffMcClintock/TideSynth/pull/331)
is MERGED.** The follow-ups landed on a branch whose PR had already closed: **the
trap STEP 4 documents from #120/#121, hit again a week later.**

**It is not tidy-up — it is the answer S27 was waiting for.** The stranded commit
is *"two reference sets — macos and windows-linux — selected per platform"*: 24
files, +207 lines, the reference PNGs split per platform. S27's own history frames
the open question as *"per-platform references or pinning the math"* and measured
that only **two** sets are needed, because Linux and Windows agree to three
decimals. Someone built exactly that and nobody was ever asked to review it —
and **S27 is marked DONE**, so nothing would have looked again.

Filed as **S44**. Not fixed here: it is another session's branch, the standing
rule is not to delete other sessions' branches, and STEP 4 forbids rewriting a
pushed commit — so the only correct move is a PR someone chooses to open.

**Machine left clean.** All work in a throwaway worktree under the session
scratchpad; nothing was built in `~/Documents/GitHub/TideSynth`. The AUv3 was
registered from the build tree with `pluginkit -a` and **deregistered with
`pluginkit -r` afterwards**; nothing was copied to `/Applications` or
`~/Applications` and no plug-in was installed into `~/Library/Audio/Plug-Ins`.
All six repos were clean and on their default branches at the start and are back
on them at the end. **One worktree on this box is NOT mine and was left alone:**
another session's, under `/private/tmp/claude-501/…-GitHub/a3974193…/scratchpad/wref`,
registered against `tide/mac/S27-render-ci` — see S44.

**Branch/PR:** `tide/mac/E9-au-rate-verify` — [#347](https://github.com/JeffMcClintock/TideSynth/pull/347). TideSynth: one new test probe, one
flag on an existing script, the backlog and this entry. No product code change.

## 2026-08-23 — macos — E10: the host-crashing chunk is fixed (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Fixed in [SynthEditLib#35](https://github.com/JeffMcClintock/SynthEditLib/pull/35).
**TIDE's probe crashes REAPER once on purpose every run. It now crashes it zero
times** — six cases rc=0, no new crash reports written.

**The row had already done the hard part** — it named `BuildDspGraph` returning
void as the structural defect, listed the sites, and warned that moving one line
would not do it. All true. It returns `bool` now, four derefs are guarded, and
both `SynthRuntime` call sites honour the result.

**The round trip is the part worth keeping.** My first version also did
`generator.reset()` on failure. That MOVED the crash instead of removing it:
modules register themselves with the master during the build, so destroying it
left those registrations dangling and `SeAudioMaster::MidiIn` crashed on freed
memory. The probe still said rc=-11 and I could easily have read that as "the
fix does not work". What settled it was comparing the two crash reports:

    before   QueryIntAttribute <- ug_container::BuildPatchManager
             <- SeAudioMaster::BuildDspGraph <- SynthRuntime::prepareToPlay
    after    SeAudioMaster::MidiIn

Different stacks. The original site WAS closed; I had opened a new one. Dropping
the reset — leaving the master constructed but unopened, which the "unprepared"
path already handles — closed both.

**A near miss earlier in the same item.** After the first build I checked for a
marker string in the installed binary, found none, and nearly concluded my code
had not been compiled. It had: the marker was inside `_RPT0`, which compiles out
in Release. The build log naming `[local override]` and compiling
`SynthRuntime.cpp` was the real evidence. Absence of a debug string proves
nothing.

**Verified:** all six probe cases rc=0, including the real REAPER chunk as a
positive control; zero new crash reports across a full run. Jeff's installed
VST3 was backed up before I replaced it with a test build and restored
afterwards.

**Not verified:** Windows and Linux — platform-neutral C++, macOS only.

**Follow-up:** the probe now prints *"skeleton: NO LONGER CRASHES -- has E10
landed?"*, which is its own request to be updated once this merges.

## 2026-08-23 — macos — the mac test suite is green: 63 of 63 (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

S42, the row I filed an hour ago while fixing S16. Fixed in
[SynthEdit#74](https://github.com/JeffMcClintock/SynthEdit/pull/74).

**The whole arc, one checkout, measured at each step:**

| | as found | after S16 | after S42 |
|---|---|---|---|
| failed | 44 | 40 | **0** |
| passed | 13 | 17 | **63** |
| refs to the dead checkout | 0 | 536 | **0** |

`63 tests from 16 suites, rc=0`, stable across two consecutive runs.

**The middle column is the whole argument.** Dead-path references appear only
AFTER S16, because until the harness stopped dying on a missing binary the tests
never got far enough to load a fixture. One defect was hiding the other, and the
count barely moved when the first was fixed — 44 to 40 — which is exactly the
shape that tempts you to call a fix a failure.

**Why not just rewrite the 46 files.** That re-bakes some other machine's path,
and the format gives no relative affordance: the value is a plug VALUE, a string
the patch concatenates, not a file reference the loader resolves. So `render2()`
normalises at run time — any absolute path up to and including a `UnitTest`
component becomes this checkout's folder. The copy goes NEXT TO the original
rather than into a temp directory, so anything resolved relative to the
project's own location still works, and it is deleted after the render. A
fixture needing no rewrite returns early and is rendered untouched, so this
costs nothing once the fixtures are clean.

**Two mistakes on the way, both caught by the compiler rather than by me.** I
inserted the helper inside another function's body ("function definition is not
allowed here"), and I anchored the cleanup on `return system(command.c_str());`
which appears three times in the file. Reverting and re-deriving the insertion
points from the parsed file was faster than patching the patch.

**This also completes S16's Accept** — *"dsp_tests on mac reports the same pass
count as CI, from a checkout at any path"* — which S16 alone could not reach.

**Not verified: Windows and Linux.** The regex accepts both separators and both
root shapes, but only macOS was run.

---

## 2026-08-23 — linux — S37 is real: another plugin's uninstall permanently breaks TIDE Rack

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Seventh item this session, at Jeff's direction. **S37**, which this box re-framed
this morning as *"the premise does not survive measurement"* — and which S43(ii)
made true a few hours later.

**This entry supersedes that re-framing, and the reason is the interesting part:
the premise was false because the CLAP had no editor, not because the reasoning
was wrong.** Now that it draws, it reads the folder, and the collision is
observable for the first time.

### The premise, confirmed

`strace` on a clean `main` build, CLAP embedded and drawing:

    Resources/ControlsXp.xml    Resources/Converters.xml
    Resources/MidiPlayer2.xml   Resources/Prefabs/Envelope.synthedit
    Resources/Prefabs           Resources/Prefabs/MidiCv.synthedit  …

This morning the same command showed **zero** accesses. With the folder absent it
emits exactly the diagnostics the row quoted off the binary — *"no Prefabs folder
in bundle resources"*, *"%s missing from bundle resources"* — which are now
reached rather than merely compiled.

### The collision, measured in three steps

A shared install directory, exactly as R4's `README.txt` tells a user to set up.

| step | prefabs TIDE loads | TIDE's own diagnostics |
|---|---:|---|
| **1. TIDE alone** | **6** | none |
| **2. a second GMPI CLAP installed alongside** | **7** | `ControlsXp.xml enriched 0 of 0 … ZERO` |
| **3. that plugin UNINSTALLED** | 6 | `ControlsXp.xml missing from bundle resources - those controls will have no pins` |

**Step 2 is data leaking between products.** TIDE's rack module browser offers
**7** prefabs, one of which belongs to the other plugin. And the other plugin's
`ControlsXp.xml` — same name, different product — overwrote TIDE's, which TIDE
reports itself: it went from enriching classes to **`0 of 0`**, so TIDE's own
controls silently lost their pin descriptions.

**Step 3 is the one that decides this row.** The other plugin's uninstaller
removes the files its package shipped. One of those names is `ControlsXp.xml` —
**which by then was TIDE's file**. Uninstalling an unrelated plugin leaves TIDE
Rack permanently degraded, and nothing in TIDE can detect or prevent it.

That is no longer a hypothesis about `parent_path()`. It is three commands.

### Which option, and why not the one the row leads with

The row offers (a) make the Linux CLAP a bundle directory, (b) namespace the
folder, (c) embed the resources.

**(a) is not forbidden by the spec but rests on host behaviour I cannot verify
here.** `clap/entry.h` says a host should search *"for files and/or bundles as
appropriate in your OS ending with the extension `.clap`"* — so bundles are
contemplated, and "as appropriate in your OS" is exactly the ambiguity. On Linux
the convention is a bare shared object, the search is **recursive**, and a
directory `TIDE-Rack.clap/` containing a `TIDE-Rack.clap` is the kind of thing
different hosts will resolve differently. **Testing that needs real hosts, which
this box does not have** — the row said so and it is still true.

**(b) is host-independent and is what I would recommend.** One function derives
both shared paths, and it already knows the module's own filename:

    BundleInfo.cpp:299   getBundleContentsFolder() / "Resources"
    BundleInfo.cpp:699   getBundleContentsFolder() / "PlugIns"

Deriving a per-module subfolder in the non-bundle fallback fixes both at once
and stays generic — every GMPI plugin gets its own, with no per-plugin
configuration. **Note it is both folders, not just `Resources`**; the row named
one and I flagged the second this morning.

**What it costs, and this is why it is Jeff's:** that fallback is what EVERY
non-bundled GMPI consumer uses — the Linux `.gmpi`, the standalone, and Windows.
Changing it moves where all of them look, so it is a compatibility break for
anything already installed, not a Linux-CLAP-only fix. `SynthEditLib` is GATED
and this is not a build break, so it is filed rather than attempted.

**Status changed to NEEDS-JEFF** with a `Default in effect` and a `Decide-by`,
per the escalation template, so an unanswered question cannot quietly become the
answer — and so the next run does not pick this up and rediscover that it cannot
act.

**Learned:**

- **A premise that fails measurement can be waiting on a different bug.** I
  closed this row's premise as unreachable this morning with good evidence, and
  the evidence was about the missing editor, not about the reasoning. Re-testing
  a "does not reproduce" row after the thing that blocked it lands is cheap and
  it was the whole of this item.
- **The uninstall case is the one that makes a shared-folder bug undeniable.**
  Two plugins overwriting each other reads as an edge case; *plugin B's
  uninstaller deleting plugin A's file* does not, and it is the same mechanism.
- **The plugin's own diagnostics were the instrument.** `enriched 0 of 0 …
  ZERO` and `missing from bundle resources` did the measuring; I did not have to
  instrument anything. Worth remembering that TideApp already narrates this.

**Not verified:**

- **Whether Linux CLAP hosts load a `.clap` directory** — option (a)'s
  precondition, and unanswerable without real hosts.
- **The second plugin was TIDE's own binary under another name**, with a small
  hand-made resource set. A genuinely different GMPI plugin that ships resources
  would be a better subject; none exists on this box (Jeff's two CLAPs carry
  none, which is why no `~/.clap/Resources` has ever appeared here).
- **Windows.** The same `parent_path()` fallback applies there, and the same
  `%COMMONPROGRAMFILES%\CLAP` sharing, but nothing was measured on it.

**Next:**

1. **Jeff picks (a), (b) or (c).** The measurement is done and the recommendation
   is (b) with its cost stated.
2. **`PlugIns` travels with `Resources`** whichever is chosen.
3. **R4's `README.txt` currently documents the shared folder as the install**,
   which is now known to be unsafe alongside another GMPI CLAP. Worth a sentence
   even before the fix lands.

**Machine left clean.** One throwaway worktree under the session scratchpad;
headless weston stopped. **Jeff's `~/.clap` was never written to** — every
install in this experiment was in the scratchpad. All six repos on their default
branches and clean. Nothing installed.

**Branch/PR:** `tide/linux/S37-clap-collision` — TideSynth, backlog and journal
only. No code change: every fix location is GATED or PR-GATED.

---

## 2026-08-23 — windows — S22: the repo where the trap was found was the one that never got the fix (interactive, Jeff directing)

**Did:** took **S22** — the GATED half of S17 — after confirming S34's two PRs
merged and STEP 1 was clear (no open `platform:win` issues, no open PRs from
this box).

### The point of the row, which is easy to miss

S17 fixed dependency provenance in TideSynth's root and **deliberately left
SE16 alone**. But SE16 is where the defect was *originally observed*: its
configure printed `Using local GMPI-UI folder` while the include path was
really `.../SynthEdit/build/_deps/gmpi_ui-src`, at a **different commit**
(`9094d79` fetched vs `83f3de2` local). The class layout being read was not the
one being compiled, and that is what cost **E12** its time. So the repo that
generated the lesson was the one still carrying the bug.

### What shipped

`tide_report_dependency` / `tide_check_not_shadowed` ported into
`SE16/cmake/S17DependencyProvenance.cmake`, wired into every resolution block
in SE16's root. **Every dependency now names its resolved path**, including two
that previously announced nothing whatsoever: the VST3 SDK in CPM's `~/.cpm`
cache, and `clap`/`clap_helpers`.

**Duplicated rather than shared, and the reason is structural:** SE16 consumes
TideSynth *via FetchContent*, and this module must load **before** the first
dependency resolves — so it cannot include a file out of a dependency it has
not resolved yet. There is no common ancestor directory on disk to share from.
The module's header says to keep the two copies in step, since a fix to one is
a fix to the other.

### The control is the part worth keeping

A check that never fires is indistinguishable from one that does not work, so
this one was made to fire on demand:

| condition | result |
|---|---|
| five overrides + fetched SDKs | configure **rc=0**, every dep reports |
| planted `build/_deps/gmpi_ui-src` beside the override — **the exact E12 shape** | configure **FAILS rc=1**, naming both paths |
| shadow removed | **rc=0**, silent, `gmpi_ui` reports the override |
| `SynthEditCL` | **262/262, rc=0** |

The positive control is a real reproduction of E12's setup, not an analogue —
same dependency, same shadow path, same override.

### Carried along, because the row asked for it

`VST3_SDK_FOLDER_OVERRIDE` was **commented out** at SE16's line 15 while lines
below both branch on it and assign `VST3_SDK` from it — settable only via `-D`,
invisible to `cmake-gui`, and the only override in that list not declared. Now
declared. **TideSynth's root has the identical gap**, recorded on the row
rather than fixed from here, since this row's scope is SE16.

**Not verified:** macOS and Linux configures were not run. The change is
platform-neutral CMake with no generator- or OS-specific branches touched, but
nobody ran them — so the `APPLE`-guarded `AudioUnit` report is unexercised,
though it is the same one-line shape as the CLAP reports beside it that did.

**Learned:**

- **"Fixed in repo A, filed for repo B" can leave the bug in the repo that
  taught you about it.** S17's own measurement came out of SE16; the fix went
  to TideSynth because that was the row being worked, and SE16 kept the
  defect for four days with a row open against it the whole time.
- **A guard needs its positive control run, not just its passing case.** Both
  configure rc=0 either way if the check is silently broken; only planting the
  shadow distinguishes "correct" from "inert".

**Next:** the `any` queue's remaining GATED candidates are **S3g** (needs a
scope ruling, not code — the `SynthEditLib` ALLOWED/GATED contradiction lands
on it) and **S18** (a licensing/vendoring question about Soundpipe first).
Neither is takeable without Jeff answering something. **P3** is still this
platform's only own-boxed row and is GATED.

**Branch/PR:** `tide/win/S22-record` — TideSynth, bookkeeping only. Product
change is [SynthEdit#73](https://github.com/JeffMcClintock/SynthEdit/pull/73), not merged.

---

## 2026-08-23 — linux — the CLAP editor PAINTS, and the cause was M4's defect on a third wrapper

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Follow-up to this morning's S43(ii) entry, which ended *"it embeds, the host
drives it, and it paints nothing"*. Jeff: *"keep going. add temporary logging if
it helps."* It helped, and the answer came in one run.

**A separate entry rather than an edit to that one** — it merged as
[#340](https://github.com/JeffMcClintock/TideSynth/pull/340) while I was working,
and a log you edit is not a log.

### The logging, and what it killed

Three temporary probes in `gmpi_ui/backends/DrawingFrameX11.cpp`'s `present()`:
entry state, every early-return, and — the one that mattered — the client's own
output surface.

**`present()` was doing everything right:**

    present#1: display=.. window=.. client=.. dirtyAll=1 w=1100 h=600
    present: calling client->render
    present: BLIT 1100x600 at 0,0 via XShmPutImage

**And the client was writing an entirely blank surface:**

    client surface: 1100x600 stride=1104 nonzero-samples=0 first=0x00000000

**So windowing was never the problem.** Not the linking, not the embedding, not
the size, not the pump. And the two bugs I had already found and fixed on the way
— `Editor_CLAP::width/height` stuck at their `{100}` defaults, and `arrange()`
never called — were both real and neither was ever going to move this.

### The cause: `wrapper/CLAP` never created the plug-in's Controller at all

`PluginSubtype::Controller` appeared **nowhere** in the CLAP wrapper directory.
AU3 gained exactly this in **M4**; VST3 has always had it.

The chain is written out in `AU3_Wrapper.mm` by whoever fixed it there, and it is
worth quoting because it predicts precisely what I measured: with no
`initialize()` the plug-in controller never publishes its `seApp` pointer through
parameter 0, so the editor's `notifyPin(0)` arrives with a **zero-byte payload
instead of 8**, so the editor's guard on that size fails, so its whole GUI is
never constructed.

Created and initialised against the wrapper's controller holder, and **held** on
`Processor_CLAP` rather than merely initialised — it publishes state through the
holder and must outlive the editor that reads it, the same reason `AU3Core` holds
its one.

**Same probe, same display, after:**

    client surface: 1100x600 nonzero-samples=41250 first=0x29102910
    window 0x600002 content: 64 distinct colours sampled  <-- IT PAINTED

The captured window shows the **module browser category tree** — All, Controls,
Conversion, Diagnostic, Effects, Experimental, Filters, Flow Control,
Input-Output, Logic, Math, MIDI, Modifiers, Old, Special, Sub-Controls, TiDE,
Waveform — the module list beneath it, and the **rack rails with their mounting
holes**. That is TIDE Rack's editor, in a CLAP host, on Linux.

**Housekeeping.** All diagnostics were temporary: `grep TIDEDIAG` is clean in all
six repos, and the `gmpi_ui` worktree they lived in is back to `origin/main` with
an empty `git status` — nothing of that repo is in either PR. Whole TIDE tree
**rc=0**, all four Linux artifacts.

**Learned:**

- **Instrument the LAST link first.** I spent the session on windowing — linking,
  embedding, sizing, the event loop — and the answer was one line showing the
  client's own surface was all zeroes. `nonzero-samples=0` on the first run would
  have pointed at content immediately and skipped every windowing theory. The
  chain here is long and I started at the end I had just built.
- **Two real bugs fixed on the way to the wrong place are still two real bugs.**
  The `{100}` size and the missing `arrange()` were genuine, and fixing something
  true is not evidence you are on the path to the cause.
- **The third instance is the one to generalise from.** AU3 (M4), now CLAP; VST3
  was always correct. *"Does this wrapper create the plug-in's `<Controller/>`?"*
  is one grep, and it is now the first question to ask of any wrapper whose
  editor misbehaves.
- **A blank window has two very different causes and they look identical from
  outside** — nothing drew, or something drew nothing. Reading the client's
  surface separates them in one measurement; everything upstream of it cannot.

**Not verified:**

- **`state->save` still returns 86 bytes**, and my earlier inference that this
  meant "no document" was **wrong** — the editor plainly has content now. What 86
  bytes actually represents is unmeasured, and I have removed the claim rather
  than repair it.
- **`guiShow`/`guiHide` are still unimplemented**, so `show()` returns false from
  the clap-helpers base. The editor draws regardless, because
  `X11DrawingFrame::open()` maps its own window — so this is a gap, not a
  blocker, and a host that respects `show()` may still hide it.
- **macOS and Windows were not built.** The controller creation is NOT inside a
  platform guard — it runs on every platform — so those two are the ones to
  check before this merges. On Linux it is measured; elsewhere it is reasoning.
- **No real DAW.** Everything is our own probe on a headless Xwayland.

**Next:**

1. **S37 is live for the first time.** The editor draws, so it reads
   `getBundleContentsFolder() / "Resources"` — the shared-folder collision that
   row describes is finally observable and its options finally sizable.
2. **Build the CLAP on macOS and Windows** before merging, since the controller
   change is unguarded.
3. **A real DAW on Linux** — Bitwig or Reaper — is the honest next test.

**Machine left clean.** Three throwaway worktrees under the session scratchpad,
one per repo; the `gmpi_ui` one is unmodified and exists only because the
diagnostics lived there. Headless weston stopped. All six repos on their default
branches and clean. Nothing installed.

**Branch/PR:** `tide/linux/S43ii-clap-x11` in both repos —
[GMPI_Wrappers#16](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/16) now
carries the controller fix as a second commit; this repo gets the probe's
screenshot dump plus the row and this entry.

## 2026-08-23 — windows — S34: two guards in SynthEditLib, and a stale row flipped on the way in (interactive, Jeff directing)

**Did:** S36 confirmed merged and flipped to DONE — [#339](https://github.com/JeffMcClintock/TideSynth/pull/339)
landed while this session's sync ran. Then took **S34** off the `any` queue: two
unguarded `plugs.back()` calls on `std::vector<UPlug*>`, GATED (`SynthEditLib`),
takeable here because this is interactive with Jeff directing.

### S34

Both sites fault at exactly **-8** when `plugs` is empty — `data[-1]` on an
8-byte pointer element, not a null-pointer read, so no null check catches it.
Same class the fleet already fixed once at -16 for `ClassicControlGuiBase.cpp`'s
16-byte `widgets` elements (**U2d**). The row named both sites, the exact
mechanism, and a third sibling (`ug_oversampler.cpp:337`) that already guards
the identical pattern — nothing here needed re-deriving, only applying.

`ug_adder2.cpp:81` — first line of `NewConnection()`, TIDE's automatic input
summing, reachable whenever a graph is built from a restored patch with an
empty pin list, which is exactly what a missing bundle resource causes.
`ug_feedback_delays.cpp:72` — `BypassFeedbackModule()`, identical shape.

**Fix:** guard, one loud stderr line naming what will not work, return rather
than crash — `ClassicControlGuiBase.cpp`'s own established pattern, and its
comment states the rule this copies: *"a host where those don't fire must not
bring the whole process down. Loud, not silent."*

**Verified by building, per the row's own Accept** — `SynthEditCL` (this
repo's Release config is a shared library, so TIDE building alone would not be
evidence): a scratch Ninja tree, `SYNTHEDITLIB_FOLDER_OVERRIDE` on the fix
branch, `262/262` targets, rc=0, zero errors. Both new stderr strings read back
out of the built `SynthEditCL.exe` verbatim. Smoke-ran the exe: scans modules,
exits cleanly on an unrecognised verb, no crash.

**Not verified:** neither path was exercised at runtime with a genuinely empty
`plugs` vector — the row itself frames this as latent UB surfaced by
investigation (S23), not a currently-reproducing crash, so the fix is
defensive against a reachable condition rather than a reproduction of a live
symptom.

### The stale-row catch

The win NEXT cell (written by this box a session ago) still said *"S36 is
IN-REVIEW, no PR link yet — check whether it has one and whether it merged
before doing anything else with it."* It had, ten minutes before this run
started. Confirmed via `gh pr view` before touching anything else — flipping a
row on verified PR state, not on memory of having pushed it, per the standing
lesson this backlog's own history keeps recording.

**Learned:**

- **A row that names its own precedent site is most of the fix.** S34 named
  both defect locations, the exact fault address to expect, and a working
  sibling to copy. The work was verifying and applying, not investigating.
- **Check the cell's own "before doing anything else" instruction before doing
  anything else.** It was there specifically so this wouldn't be skipped.

**Next:** the `any` queue still carries several other GATED rows (S5, S3g,
S22, S18) that want either Jeff or another interactive session; none was
sized as tightly as S34 was.

**Branch/PR:** `tide/win/S34-guard-record` — TideSynth, bookkeeping only.
Product change is [SynthEditLib#34](https://github.com/JeffMcClintock/SynthEditLib/pull/34), not merged.

---

## 2026-08-23 — linux — S43(ii): the Linux CLAP has an X11 editor. It embeds, the host drives it, and it paints nothing

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Sixth item this session, at Jeff's direction — *"do any linux-only task"*, after
he ruled: *"if CLAP wrapper lacks GUI support, we need to add it"* and *"I
suspect DAWs support only X11."*

**Read the last section before reading the rest as a success.** The editor
embeds at the right size and the host loop drives it. It does not paint.

### What was actually wrong, and it was not the CMake alone

`wrapper/CLAP/CMakeLists.txt` had a `WIN32` block, an `APPLE` block, and **no
Linux arm at all**. But the deeper finding is the macros:

**`IS_LINUX` and `HAS_GUI` are defined NOWHERE in the repo — 11 uses, zero
definitions.** Every block guarded by them has always compiled to nothing, which
is why the bodies in `Processor_CLAP.cpp` still name **`ClapSawDemo`**, the demo
class this wrapper was derived from. The Windows and macOS editors work because
they are guarded by `_WIN32` and `__APPLE__` instead, and one block was forced
live by hand with `#if 1 //HAS_GUI`.

So this was a dormant, never-compiled skeleton — not a wiring oversight. I used
`__linux__` for everything new and **left the dead blocks exactly as found**,
annotated: reviving them would change Windows and macOS too, and that is not
this item's to do.

### What shipped

Mirrors `wrapper/VST3`'s X11 arm source-for-source, deliberately — that editor
is older and working, and this project's own lesson is that copying a fiddly
block verbatim beats improving it in passing.

- CMake: `DrawingFrameX11` plus the CPU renderer stack and the same six
  `REQUIRED` pkg-config deps. Those headers select their implementation with
  `__has_include`, so a **missing** include path does not fail the build — it
  silently compiles a plugin that draws no text. REQUIRED and PUBLIC for that
  reason.
- `X11DrawingFrame` on `Editor_CLAP`, wired as its header specifies:
  `wireTextStack`, menu font, `setFallbackHost` **before** any `setHost`.
- `guiIsApiSupported` answers X11 again, gated on the host offering **both**
  timer and posix-fd support — the frame runs no loop of its own, so without
  them we cannot drive it and yes would be the lie option (i) just removed.
- `guiSetParent` embeds and registers the fd + a 16 ms timer; `guiDestroy`
  unregisters **before** closing, because the host's loop holds that fd.
- `onPosixFd`/`onTimer` drive `processEvents()`/`onTimer()`.

### Two bugs found by running it, both fixed

- **`Editor_CLAP::width/height` stay at `{100}`** unless a host calls
  `set_size` — and `getSize()` measures but **never stores**. A host that
  accepts `get_size()`'s answer embedded a **100x100** editor into an 1100x600
  window, which is exactly what the probe showed. Now refreshed unless the host
  chose.
- **Nothing called `arrange()`** on that path, so the client had no layout.

### Verified, with a real X11 CLAP host

`tools/clap_probe.c` grew `--embed`: it offers the posix-fd and timer
extensions, creates a parent window, embeds, and **pumps** — polls the fd the
plugin registered and ticks its timer. Without the pump an embedded editor is a
window that never paints, so a probe stopping at `set_parent` would report
success and show nothing.

On an isolated Xwayland (weston headless per **S32**; Jeff's session untouched
throughout, `gnome-shell` never restarted):

| | before | after |
|---|---|---|
| `is_api_supported(x11)` | **0** | **1** |
| `ldd` libX11/xcb | **0** | **2** |
| `X11DrawingFrame` symbols | **0** | **89** |
| `set_parent` | n/a | **OK** |
| child window | none | **0x600002, 1100x600, IsViewable** |
| host loop | n/a | **fd 4 + 16 ms timer, 226 ticks, unregistered in order** |

Whole TIDE tree **rc=0**, all four Linux artifacts, and the standalone and VST3
link exactly what they linked before.

### WHAT IS NOT DONE: it paints nothing

**The child window is uniformly `0x000000`.** Embedding, sizing and the event
loop are demonstrated; pixels are not. I found and fixed the two causes I could
identify and **neither moved it**, so the remaining cause is something else and
I have not named it. Saying "the Linux CLAP has a GUI now" would be exactly the
kind of claim this fleet keeps having to retract.

Three leads, none chased to ground:

1. **`guiShow`/`guiHide` are not implemented at all**, so `show()` returns false
   from the clap-helpers base. Not needed for embedding; a host may still call it.
2. **`state->save` returns 86 bytes**, which looks like no document. That is the
   shape of **M4** on the AUv3 — a controller that never receives the document —
   and would explain having nothing to draw.
3. **gmpi_ui's own X11 backend may never have been GUI-verified either.** Its
   `x11_menu_test` reports *"frame open, 300x200"* here and then fails on
   synthetic input, so it does not settle paint either way. The VST3's Linux
   verification on this box was `ardour-vst3-scanner`, which scans and does not
   open an editor.

**Learned:**

- **A macro that is never defined is worse than a `#if 0`, because it reads as
  live code.** `IS_LINUX && HAS_GUI` looked like a platform gate and was a
  tombstone. Checking `grep -rn IS_LINUX` for a *definition* rather than for
  *uses* took one command and reframed the whole item.
- **Structure beats pixels on a headless server.** `XQueryTree` +
  `XGetWindowAttributes` said "child 0x600002, 1100x600, IsViewable"
  unambiguously; the pixel sample needed the child window, the right teardown
  order, and still only answers half the question.
- **The teardown order bug was mine, in the probe.** Closing my display before
  `gui->destroy` produced `BadWindow` on `X_DestroyWindow` — the plugin
  destroying a child of a window I had already freed. A real host tears the
  plugin GUI down first; the probe now does too.
- **Mixed line endings in one repo.** `Editor_CLAP.cpp` is CRLF and
  `Processor_CLAP.h` is LF. I hit the CRLF trap earlier today and then nearly hit
  its mirror image; `file` on the target before editing is the cheap check.
- **A CMake block inserted next to the right line can still land in the wrong
  `if()`.** Mine went inside `if(APPLE)` and silently did nothing on Linux; the
  tell was `flags.make` carrying none of the include dirs while the VST3's
  carried all of them.

**Not verified:** paint, as above. macOS and Windows were not built — the change
is inside `if(UNIX AND NOT APPLE)` and `#if defined(__linux__)`, and their
artifacts link what they always did, but neither was compiled. And no real DAW:
the probe is ours.

**Next:**

1. **Chase the paint.** Lead 2 is the one I would take first — if the CLAP holds
   no document, the editor has nothing to draw and this is M4's shape on a third
   wrapper.
2. **`guiShow`/`guiHide`** are a small, separate gap.
3. **S37 becomes real when the editor does** — once it draws, it reads
   `getBundleContentsFolder() / "Resources"` and the shared-folder collision is
   finally observable.

**Machine left clean.** Two throwaway worktrees under the session scratchpad, one
per repo. The headless weston is still running on its own socket and can be
killed with `pkill -x weston`; it touched nothing of Jeff's. All six repos are on
their default branches and clean. Nothing installed.

**Branch/PR:** `tide/linux/S43ii-clap-x11` in **both** repos —
[GMPI_Wrappers#16](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/16) is
the change; TideSynth carries the probe's `--embed` mode, the backlog and the
journal. **Merging one without the other is harmless:** TIDE fetches
`GMPI_Wrappers` at `origin/main`, so the editor lands with that PR and this one
only records it and supplies the verifier.

---


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
