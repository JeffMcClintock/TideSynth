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
2. Stop when this file is **under 30 KB**, or when the **four most recent
   entries** remain — whichever comes first. **The floor of four wins**: the run
   prompt tells every run to read the last four entries, so a verbose month
   pushing this file over 30 KB is correct, not a rotation failure.
3. Never edit an entry while archiving it. The archive is the record.

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

## 2026-08-19 — linux — STEP 1 build break (#153 filed and fixed; #87 closed, #88 half-closed, #156 filed)

**Prompt:** 397330d · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (asserted; `url."https://github.com/".insteadOf` = `git@github.com:`, and every repo remote spot-checked `https://`)

**Did:** No backlog item. STEP 1 took the whole run: this platform's default branch did not build, and the reason was not either of the two issues that say so.

### The headline: Linux `main` was broken by something nobody had filed

Both standing `platform:linux` issues carry a `fix it` from Jeff, so I started there — and re-verified before acting, per STEP 1. **Both were already substantially fixed.** What was not fixed, and what nobody had noticed, is that `SE16` `master` **does not configure at all** on this box:

```
-- gmpi_plugin(TIDE): STANDALONE skipped -- Standalone_Wrapper cannot be built here, missing: libpipewire-0.3
CMake Error at SynthEditSem/CMakeLists.txt:370 (target_link_libraries):
  Cannot specify link libraries for target "TIDE_STANDALONE" which is not built by this project.
```

Filed as [#153](https://github.com/JeffMcClintock/TideSynth/issues/153), fixed in [SynthEdit#55](https://github.com/JeffMcClintock/SynthEdit/pull/55).

**Cause, and it is a contract the fix's own author wrote.** GMPI [`03dd218`](https://github.com/JeffMcClintock/GMPI/commit/03dd218) (2026-08-18) made `gmpi_plugin()` run the Standalone wrapper's dependency probe and drop `STANDALONE` when a dependency is missing — its message says this is so "a missing pipewire SDK costs the bare app rather than the whole tree." It drops it from **its own parsed copy** (`GMPI_PLUGIN_FORMATS_LIST`). The caller's `FORMATS_LIST` (`SynthEditSem/CMakeLists.txt:59`) is a different variable and still reads `GMPI VST3 STANDALONE` afterwards. So on this box it cost exactly the whole tree.

**The part worth keeping: two of the three loops were already right.** `SynthEditSem/CMakeLists.txt` iterates `FORMATS_LIST` three times. `:219` guards with `if(TARGET …)`; `:247` guards with `if(NOT TARGET …) continue()`; `:361` had nothing. The fix is the third instance of a pattern the file already establishes — so this was a missed edit, not a design question, and that is why it was safe to take in one run.

**Why win/mac cannot see it.** `GMPI_Wrappers/wrapper/Standalone/dependencies.cmake` only reports anything on its Linux arm. Everywhere else the missing-list is empty, `STANDALONE` is never dropped, the target always exists, and the unguarded loop is always correct. Identical shape to #88, and to the 2026-08-14 finding before it: **a path that only executes below a platform gate is only tested below that gate.** That is now three occurrences; it is the fleet's most reliable bug generator.

**Verification — GCC, Ninja, Release, `libpipewire-0.3` absent:**

| check | before | after |
|---|---|---|
| configure | **RC=1** at `:370` | **RC=0** (`Configuring done 21.1s`) |
| `TIDE_VST3` | not reached | **322/322 RC=0**, links `.so`, assembles bundle |
| `TIDE` | not reached | **30/30 RC=0**, links `TIDE.gmpi` |
| `SynthEditCL` | not reached | **13/13 RC=0** |
| `SynthEditWayland` | not reached | **28/28 RC=0** |
| full tree | not reached | **549/549 RC=0**, zero `error:` |
| `ctest` | not reached | **67/67** (with the env vars — see below) |

The `STANDALONE skipped` status line still appears after the fix. That was deliberate: tolerate the decline, do not silence it. I also did **not** install the pipewire SDK, which would have made configure pass while leaving the contract violation in place for the next declined dependency.

### #87 — closed, verified by building rather than by reading

Fixed by [`5d6385e`](https://github.com/JeffMcClintock/SynthEditLib/commit/5d6385e). Both Accept clauses met: `grep -rn "/tmp/tide" SynthEditLib` is **empty**, and — the load-bearing bit — **`ModuleView.cpp.o`, the exact TU that failed, compiled at edge 164/322** with zero `error:` in the build. The `fprintf(stderr, …)` diagnostic U2d actually wanted survives at `ModuleView.cpp:669`.

**One hazard that outlived the fix.** `namespace SE2 {` opens at `ModuleView.cpp:38` and does not close until the end of the file, so *any* future `#include` added mid-file re-creates `SE2::std` and breaks GCC while staying invisible on MSVC and Clang. The trace is gone; the trap is not.

### #88 — left open, and the count changed

`SynthEditWayland` is fixed ([`6faf8cff9`](https://github.com/JeffMcClintock/SynthEdit/commit/6faf8cff9)) and **links, 28/28 RC=0, zero `undefined reference`** — its stated Accept, measured. `SynthEditJuce` still lacks the entry, so the title's "two of four" is now **one of four**.

I did not fix the Juce half, and the reason is worth stating because it is not the obvious one: `SE16/SynthEditJuce/` is GATED-by-default, **and** the STEP 5 build-break exception does not reach it either — that exception's trigger is "your platform's default branch does not build", and after #55 it does. The target is deprecated and **not reachable from the root `CMakeLists.txt`** (its own comment, `SynthEditJuce/CMakeLists.txt:49-51`), so there is no build that would fail *and none that would prove a fix correct*. Whoever takes it must say it is by inspection.

### #156 — `ctest` looked catastrophic and was fine

**44 of 67 tests "failed"; the real number is zero.** `tests/projecttests.cpp:78,103` resolve two fixture folders with a two-armed `#ifdef` on a three-platform project — `_WIN32` gets `C:\SE\SE16\…`, and the `#else` is the literal string `/Users/jeffmcclintock/SynthEdit/…`. Linux takes the `#else` and looks for `SynthEditCL` in a macOS developer's home directory; the `32512` in the gtest output is `system()` returning 127.

Both functions prefer an environment variable over the literal, so:

```bash
SE_BUILD_FOLDER="<build>/" SE_CANCELLATION_FOLDER="$HOME/SE/SE16/UnitTest/" ctest
100% tests passed out of 67
```

**Next run: do not spend time on a red ctest here before setting those two variables.** That is the single most useful line in this entry. Filed as [#156](https://github.com/JeffMcClintock/TideSynth/issues/156) with the CMake-side fix suggested (`set_tests_properties … ENVIRONMENT`, which needs no change to `projecttests.cpp` at all); `SE16/tests/` is GATED-by-default and this is not a build break, so the A17 exception does not cover it.

**Learned — the mac box works by coincidence too.** That `#else` is correct on exactly one machine, the one whose home directory it names. A second macOS checkout would fail identically.

### Not verified, and not claimed

**The v0.1 audio harness did not run: REAPER is not installed on this box.** `scripts/render-and-measure.py` needs it, so PLAN's "v0.1 PASSES" table cannot be re-measured from linux. The change here is CMake-only and cannot reach DSP, and the 549/549 + 67/67 evidence is the right artifact for it — but nobody should read this entry as re-confirming v0.1 on linux. **If the fleet wants that table re-measurable on more than one box, REAPER on linux is the missing piece**, and it is currently a silent single point of failure in the only end-to-end check the project has.

**Learned — the A14 shared-tree race did not recur, and I think I know why.** The windows box hit it at 36 seconds after `git checkout -b`. I committed within about a minute of branching and `--record`/`--verify` both reported real content (`1 path(s) staged, 1 in HEAD`), not the empty-manifest signature that means the race already happened. No concurrent session was active. The commit-immediately rule is doing its job; the scripts still cannot *detect* a total-unstage race, which is unchanged from the windows entry.

**Result:** `SE16` configure RC=1 → RC=0 on linux; full tree 549/549; ctest 67/67; `SynthEdit`, `SynthEditCL` and `TIDE` all building on this platform for the first time since 2026-08-17.

**Next:**

1. **Merge [SynthEdit#55](https://github.com/JeffMcClintock/SynthEdit/pull/55).** Until it lands, `SE16` `master` is RC=1 on any Linux box without the pipewire SDK — which is the supported configuration, not an unusual one.
2. **[#153](https://github.com/JeffMcClintock/TideSynth/issues/153) and [#156](https://github.com/JeffMcClintock/TideSynth/issues/156) are both open**; #88 stays open for the Juce line.
3. **C12d is still this box's, and is still the last thing between the carve-out and C6** — three `${EDITOR_DIR}` entries. It was not takeable before today because its Accept requires `SynthEditWayland` to link; **it now does (28/28)**, and with #55 the configure works too. So C12d is unblocked in practice for the first time. Its Accept also names `SynthEditJuce`, which cannot link on any box because it is not generated — **that clause wants re-specifying before someone starts, or C12d will deadlock on it the way this row deadlocked on #87/#88.**
4. Consider a CI job that configures with the pipewire SDK deliberately *absent*. Every finding in this entry is a platform-gated path that only one box executes, and the fleet keeps rediscovering them one run at a time.

**Branch/PR:** [SynthEdit#55](https://github.com/JeffMcClintock/SynthEdit/pull/55) (the fix) + the TideSynth PR carrying this entry. All repos left on their default branches; no working tree left dirty.

---

## 2026-08-19 — correction to the windows C12f entry: the two PR-less branches are Jeff's (interactive session, Jeff directing)

**Whose finding this is:** the windows run's own, from
[#151](https://github.com/JeffMcClintock/TideSynth/pull/151). It landed here as a
prepended correction rather than as the in-place edit that PR made, because
`check-journal-prepend.py` refuses an edit to an entry that is already published —
"An entry may move to an archive file verbatim (rotation), or stay. It may not be
edited or silently dropped." That PR passed its own CI only because the entry it
was correcting was still on the same branch; once [#150](https://github.com/JeffMcClintock/TideSynth/pull/150)
merged the original into `main`, the same diff became an edit and the check caught
it. Nothing is lost: the original paragraph stands in the C12f entry above, and
this supersedes it.

**The correction, in the windows run's own terms.** The C12f entry says two pushed
branches with unmerged commits and no PR — `tide/win/competitive-review` (3
commits) and `tide/mac/V3-midi-findings` (2 commits) — are STEP 5's named failure
state, and that it opened a PR for the win one. **Neither is true.** All five
commits across both branches are authored `Jeff McClintock <jef@synthedit.com>`,
dated 2026-08-18, from interactive sessions. STEP 5's two-end-states rule governs
what a *run* leaves behind, and a run left neither of these. No PR was opened for
either, and none should be: opening one on a developer's work in progress guesses
that he is finished with it. The run's first instinct was to open one because the
shape matched; the authorship check is what stopped it, and a future run should
make that check before treating an orphan branch as a failure.

**Still worth knowing:** `tide/win/competitive-review` touches `BACKLOG.md` and
`JOURNAL.md`, so it will conflict with recent changes whenever it lands.

**Branch/PR:** [#151](https://github.com/JeffMcClintock/TideSynth/pull/151), resolved
against `main` and reshaped into this entry.

---

## 2026-08-19 — macos — E9's sliver was a silence writer, and next door to it was a live host crash (interactive session, Jeff directing)

**Did:** verified the scheduled run's E9 correction independently (it holds, and is
stronger than it claimed), then measured the two things nobody had measured, and
shipped TIDE's half of both.

**Result 1 — a malformed saved chunk was a LIVE HOST CRASH.** The previous run filed
the `SeAudioMaster.cpp:410` deref as latent, reachable only if something prepared
before a document existed. It is reachable now. A REAPER project whose saved TIDE
chunk is `<Patch/>` — well-formed XML, wrong root — **segfaulted the render**:
`EXC_BAD_ACCESS at 0x28`, `TiXmlNode::FirstChildElement` ← `BuildDspGraph` ←
`prepareToPlay` ← `onSetPins` ← `Processor_VST3::process`, on a REAPER worker thread.
The trigger is "no `<Document>` root", **not** "empty": `<Patch/>` parses with no
error, so `RootElement()` is non-null and `SynthRuntime.cpp:76`'s bundle fallback
never runs — the absent `dsp.se.xml` is irrelevant on that route. Nothing validated
the bytes anywhere: `gmpi::base64Decode` silently skips anything outside its
alphabet, so a truncated or hand-edited project file is enough.

**Result 2 — an unprepared TIDE never wrote its output buffers.** `subProcess` was
installed only at the tail of `onSetPins`, which never runs for a no-chunk instance
(no audio inputs → no `PinStreamingStart`; outputs skipped; MIDI has no default; the
empty blob `continue`s at `processor_holder.cpp:226`). A/B measured in REAPER, same
build except two lines: without the `open()` install the log shows `host MIDI arrived
BEFORE the rack was prepared` and **no** silence line; with it, `TIDE: unprepared -
writing silence to the host's output buffers`. **REAPER is not exposed** — its render
measured identical either way — so this is a contract fix, not an audible bug on this
host. Other hosts are untested, and VST3 does not guarantee zeroed buffers.

**Shipped, both in `SynthEditSem/SynthEdit.cpp` (TIDE's own, ungated):** a
`documentIsBuildable` check that refuses a chunk which is not
`<Document>`/`<DSP>`/`<Module>`-shaped and logs why, and an `open()` override that
installs the silence writer immediately. `TIDE_VST3` Release builds clean;
`tests/hosts/v1-rack.rpp` still renders −6.3 dBFS / −17.0 rms / 2 cables, so no
regression.

**Learned — the honest boundary of TIDE's guard, measured rather than assumed.** A
`<Document><DSP><Module/></DSP></Document>` skeleton passes everything TIDE can check
and **still crashes** (a `<Module>` with no `<PatchManager>` reaches
`ug_container.cpp:1469`). TIDE can refuse the wrong *shape*; it cannot validate the
engine's schema. My first harness used that skeleton as its positive control and the
run correctly failed — the real chunk from `tests/hosts/v3-midi-pitch.rpp` is the
positive control now, and the skeleton is reported as a KNOWN LIMIT so nobody reads it
as a pass.

**Learned — E10's Accept clause would have passed while the process still crashed.**
`SynthRuntime.cpp:157` calls `OpenGenerator()` unconditionally after `BuildDspGraph`,
and `SeAudioMaster::Open()` dereferences `main_container` at `:2330`, which is null
after ANY early return from the build — including the `:413` return already there. So
"move the guard one line, rerun the probe, see the return" is a fix that does not fix.
The row now says so, and E10's real scope is at least three deref sites plus a way for
`SynthRuntime` to learn the build failed.

**Learned — one trap for the next person measuring this.** A fixture cannot be
hand-edited to carry a different chunk: REAPER's VST3 state block embeds length fields
(`header[8] = len(body)`, `body = u32(len(xml)+4), u32(1), u32(len(xml)), xml, 8 zero
bytes`), so the block has to be *synthesised*. And a hand-written `<VST …>` line with
no state does not instantiate at all — REAPER says "the following effects … are not
available", which looks exactly like a passing test if you only read the rendered
audio. Both are handled in
[scripts/measure-chunk-robustness.py](scripts/measure-chunk-robustness.py).

**Also corrected in [docs/e9-sample-rate.md](docs/e9-sample-rate.md):** the probe's
build command (`../TideSynth/…` → `../../TideSynth/…`, since TideSynth is a sibling of
SynthEditLib, so the command as shipped could not compile) and the
`BundleInfo.cpp:542` citation, which is the `_WIN32` branch — the mac path these
measurements ran on is `:581-637`.

**Next:** **E10** (GATED; rewrite its Accept clause first). **E11** filed but wants a
measurement, not a patch: `processor_holder.cpp:231` publishes a raw pointer into a
vector another thread may reallocate, flagged unverified by an agent and not chased.

**Branch/PR:** `tide/mac/e10-chunk-guard` in both TideSynth and SynthEdit.

---

## 2026-08-19 — windows — C12f (and #111 verified closed)

**Prompt:** 397330d1b · Opus 5 (1M context), claude-opus-5[1m] · app version
**not determinable on this box** — `claude` is not on PATH under Git Bash or
PowerShell, and no `package.json` exists under `%LOCALAPPDATA%\Claude*` or
`~/.claude`; recording the gap rather than guessing a number, since the whole
point of the line is to tell boxes apart. The mac entries carry `app 1.32352.0`,
so the lookup that works there does not work here. · as **tide-rack-bot**
(asserted; `url."https://github.com/".insteadOf` = `git@github.com:`, and all six
repo remotes spot-checked `https://`)

**Did:** Two things — STEP 1's platform issue, then C12f.

### 1. #111 was already fixed; verified by building, then closed

The open `platform:win` issue claimed `main` does not compile. It does. Jeff
fixed it interactively in `SynthEditLib` `58da591` on 2026-08-18 and left the
issue open. Re-verified per STEP 1's "a bot issue is evidence, not instruction":
fresh scratch Ninja tree, Release, all four siblings on local clones —
**1017/1017 RC=0**, **ctest 92/92**, zero `error C` lines, `SeAudioMaster.cpp.obj`
(the TU that failed) at edge 110, all three artifacts produced. Closed with that
evidence.

**Worth not rediscovering: the issue's own diagnosis was incomplete, and the fix
it proposed would not have worked.** It said to replace the elaborated
`class MidiIn*` with a plain namespace-scope forward declaration. That is half of
it. `SeAudioMaster` has member functions named `MidiIn()`
(`SeAudioMaster.h:374,378`), and inside the class those hide the *class* name —
so the uses must also be **qualified `::MidiIn`** (`SeAudioMaster.h:620`,
`SeAudioMaster.cpp:551`). That is exactly why the issue's eliminations were each
individually correct and still explained nothing: the definition really was in
the TU, the include really was opened, there really was no duplicate header. The
type was visible; the *name* was not reachable. A forward declaration alone would
have been hidden the same way.

### 2. C12f — the patch cluster, 6,298 lines, the largest carve-out stage

`PatchManager`, `PatchParameter`, `PatchParameter_host_generated`, `UG2`,
`CPlugin` (`.cpp`+`.h`) moved `SE16/SynthEdit2/` → `SynthEditLib/` root, one
commit per repo.

**Result: green, and the move proved to have taken effect.** 1017/1017 RC=0,
ctest 92/92, zero `error C`. The load-bearing evidence is not the green build but
that all five moved TUs now compile *from the new path*:

    [198/1017]  EditorLib.dir\C_\SE\SynthEditLib\CPlugin.cpp.obj
    [218/1017]  EditorLib.dir\C_\SE\SynthEditLib\PatchParameter_host_generated.cpp.obj
    [223/1017]  EditorLib.dir\C_\SE\SynthEditLib\PatchParameter.cpp.obj
    [230/1017]  EditorLib.dir\C_\SE\SynthEditLib\UG2.cpp.obj
    [247/1017]  EditorLib.dir\C_\SE\SynthEditLib\PatchManager.cpp.obj

Dangling private includes **21 → 7**, exactly the 14 predicted. Measured with a
re-created script (scratchpad, uncommitted, per the C5 convention), which
reproduced the recorded 21 baseline before any change and reported `resource.h` =
**0** — the sanity check the C12 doc says to run first, because a wrong
own-directory-first resolution order reports 71 there. The 7 remaining are
`ISEAppManaged.h` (3), `IMidiDriver.h` (2), `ParseSynthEditArgs.h` (1) and
`SynthEditApp.h` (1, C11) — all headers no sub-stage owns.

**Learned, and the reason the bookkeeping changed:**

1. **C12f's Accept was wrong, and wrong in the direction that unblocks an unsafe
   item.** Both the BACKLOG row and `docs/c12-remaining-editor-files.md` say C12f
   leaves **zero** `${EDITOR_DIR}` entries. It leaves **three**. Both were written
   assuming C12d had already landed; C12d is `linux` by design and is still TODO.
   Had this gone unnoticed, C12f → DONE would have made **C6** eligible — and
   C6's own row already records, from 2026-08-14, that exactly this once nearly
   moved `EditorLib/CMakeLists.txt` into the public repo while it still pointed at
   private files. So C6 and the C12 umbrella are re-pointed to `BLOCKED(C12d)`,
   and both the row and the doc now carry the correction rather than the claim.
   **The carve-out's last step is now the linux box's, not this one's.**

2. **The A14 collision happened to me, live, and the authorship check is not what
   caught it.** 36 seconds after I ran `git checkout -b` in the shared `SE16`
   tree, a concurrent session committed **my staged 11 paths plus its own
   `SynthEditWayland/IO_PipeWire.cpp` edit** as `6f5819178`, authored *and*
   committed **Jeff McClintock**, message
   `docs(se) : IO_PipeWire's callbacks are not on an RT thread`. My own
   `git commit` then reported "nothing to commit, working tree clean" — which is
   the only reason I looked.

   **Both A14/A16 scripts were blind to it.**
   `check-commit-completeness.py --record` printed *"nothing staged — recorded an
   empty manifest"* and `--verify` printed *"manifest was empty — nothing to
   verify"*; both exited 0. The peer's commit had already emptied my index before
   `--record` ran. `check-commit-authorship.py` would have caught the bad commit
   only at push time, well after the fact. **Neither script detects the race when
   the peer commits *everything* rather than unstaging a subset** — A16 is written
   against a partial unstage, and this was a total one. The signature to recognise
   is `git commit` saying *"nothing to commit"* immediately after a successful
   `git add`.

   Resolved without destroying their work: preserved their commit as local branch
   **`rescue/iopipewire-doc-6f58191`** in `C:\SE\SE16` (unpushed, on that box),
   `git reset --soft origin/master`, unstaged `IO_PipeWire.cpp` so their change
   sits in the working tree exactly as it did before they committed, then
   re-committed my 11 paths as the bot. Safe to rewrite because the commit was
   **local only and on no remote** — checked with `branch -a --contains` and
   `ls-remote` before touching it. **`SE16` is left with that one uncommitted
   modification, which is theirs and which I did not revert.**

3. **`git checkout -b` in a shared working tree drags the other session onto your
   branch.** The reflog shows my checkout at 08:18:00 and their commit at
   08:18:36. They were mid-edit on `master`; my branch switch moved the tree under
   them, and their commit landed on my branch. That is the mechanism behind A14,
   and it is caused by the claiming step the prompt requires, so it will recur.

4. **Include resolution under a move is worth checking rather than assuming.**
   `PatchManager.cpp` has two `../`-prefixed includes
   (`../tinyXml2/tinyxml2.h`, `../se_sdk3_hosting/GuiPatchAutomator3.h`). They
   resolve via the include path in *both* locations — `SE16/tinyXml2/` does not
   exist and neither does `C:/SE/tinyXml2/` — so the move is a no-op for them, and
   no own-directory hijack becomes possible at the destination. Checked by
   existence test in both directions, not by reading.

**Not verified, deliberately not claimed: SynthEdit2 (WinUI3) was not built.**
The Accept asks for it. Its vcxproj links `EditorLib.lib`/`SynthEditLib.lib` out
of `$(SolutionDir)build\...` — the developer's own Visual Studio Debug tree — so
building it means writing into Jeff's tree, which a scheduled run must not do.
Checked instead: the vcxproj lists none of the ten (they arrive via
`EditorLib.lib`, unchanged); its four includers use the plain `"PatchManager.h"`
form and fall through to `$(SolutionDir)..\SyntheditLib`, present in all four
configurations; and one of the four — **`ExportAsPlugin.cpp`, still resident in
`SynthEdit2/` and including `"UG2.h"` and `"PatchManager.h"` by plain name —
compiles clean after the move at edge 1016/1017**, which is a real positive
control for the resolution question from the right directory. A `cl /Zs` of the
three `.xaml.cpp` files was attempted and abandoned: they need CppWinRT generated
headers, which is a WinUI3 build, not a resolution question.

**Also noted:** two pushed branches carry unmerged commits and have **no PR** —
`tide/win/competitive-review` (3 commits, this platform) and
`tide/mac/V3-midi-findings` (2 commits). That is STEP 5's named failure state,
and this box's tree was parked on the first of them at session start. I opened a
PR for the win one and left the mac one for its own box.

**Next:** **C12d, on the linux box, finishes C12** — three entries, and it is the
only thing standing between the carve-out and C6. Nothing else in C12 is left.
For this box the NEXT row now points at **P3**; note P3 touches `SynthEditLib`,
which is GATED and is *not* a build break, so STEP 5's exception does not cover
it and the path question wants settling before someone starts.

**Branch/PR:** [SynthEdit#51](https://github.com/JeffMcClintock/SynthEdit/pull/51)
+ [SynthEditLib#21](https://github.com/JeffMcClintock/SynthEditLib/pull/21), which
must merge together, plus the TideSynth PR carrying this entry. All repos left on
their default branches; `SE16` retains the concurrent session's one uncommitted
file, untouched.
