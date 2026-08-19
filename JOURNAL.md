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

## 2026-08-20 — macos — the mac test drift is FMA contraction, and my own diagnosis was wrong first

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Seventh and eighth items this session**, on Jeff's instruction. Also closes
**U3** (shipped as [SynthEdit#65](https://github.com/JeffMcClintock/SynthEdit/pull/65)).

**Did:** diagnosed the four `TestVoiceAllocation` failures that S19 papered over
this morning with raised gates, and reverted those gates because they turned out
to be unnecessary. [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66).

### The hypothesis I wrote into three documents was wrong

This morning's row, issue and PR all said the error shape — max −68 dB against an
average −150 dB — looked like **a one-sample timing difference at voice
transitions**. Measured against the reference `.wav`, every part of that is false:

| claim | measurement |
|---|---|
| a handful of samples | **81.9% of all samples**, continuous from 0.127 s |
| a one-sample shift | shift 0 = −68.73 dB, shift ±1 = **−22.69 dB** — zero wins by 46 dB |
| (unstated) a gain error | best scalar fit **0.999999192**, residual unchanged |

It was a plausible story fitted to one summary statistic, and it survived into
three places because nobody had opened the file. **The average/max ratio I
reasoned from was the cancellation utility's own metric, not something I had
computed.**

### The actual cause

**FMA contraction.** clang defaults `-ffp-contract` to *on*, fusing `a*b+c` into
one `fma`. arm64 always has FMA; x86-64 under MSVC or GCC does not emit it by
default — **which is exactly why Windows and Linux reproduce the references and
macOS does not.**

It is *not* the Apple fast-math subset that was already in `CMakeLists.txt`. That
was the obvious suspect and it was eliminated this morning: with
`-fassociative-math` and `-freciprocal-math` removed, the four residuals were
**bit-identical**. Only contraction moved them.

```
test                        contract=on   contract=off
Unterminated_Poly_Modules   -80.77 dB     -90.31 dB
Voice_Allocation_Mono_High  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Last  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Off   -73.41 dB     -90.31 dB
```

**−90.31 dB is exactly 1 LSB at 16 bits** (`20·log10(1/32768)`), i.e. bit-identical
within the file format. Full suite with the strict gates restored: **3 failures
with contraction on, 86/86 with it off.**

So there was never a voice-allocation defect, and **the four gates raised this
morning are reverted to 85/75/75/75.**

### Checked rather than assumed

[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28)'s
soundfont scoping is **still load-bearing**: rebuilt with it reverted *and*
contraction off, `SoundfontOsc` still fails. Reassociation and contraction are
different mechanisms and neither fix makes the other redundant.

**Learned:**

1. **A hypothesis that explains the summary statistic is not a diagnosis.** Max
   ≫ average genuinely does suggest sparse differences — and the differences were
   dense. Ten minutes of `numpy` against the two files would have prevented three
   documents asserting it. **Open the artifact.**
2. **Eliminating the obvious suspect is worth more than confirming it.** This
   morning's A/B on `-fassociative-math` looked like a dead end — the figures were
   bit-identical, so the flags "weren't it". That negative result is what pointed
   at a *different* FP mechanism rather than a DSP bug, and it is why the second
   experiment was aimed correctly.
3. **`-ffp-contract` is invisible in a fast-math discussion.** It is not part of
   `-ffast-math`, is not mentioned by the flags this project already reasons
   about, and is on by default. On any arm64 target it is the first thing to check
   when a render differs from an x86-baked reference.

**Next:**

1. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178) can close once [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) merges** — except for the
   `continue-on-error` removal, which stays Jeff's.
2. **U3's click path is unverified** — one right-click on the rack background.
3. **A22, A23, A24** are the remaining A-series rows; **C7e** is unblocked from
   the `EditorScreenshot` direction now C7c is closed.

**Branch/PR:** `tide/mac/s19-fma-record` (this) + [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) (the code).

## 2026-08-20 — macos — C7c answered by removal, and the two questions that answer creates

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Sixth item this session**, on Jeff's instruction. Also closes **S19** — both its
PRs merged and macOS `ctest` is **100% of 86**; the `continue-on-error` removal
is still Jeff's and stays on [#178](https://github.com/JeffMcClintock/TideSynth/issues/178).

**Did:** recorded Jeff's C7c ruling and filed the work it implies as **U3**.

C7c asked whether `EditorScreenshot` should become public so a stranger's clone
can link it. **Jeff answered by deleting the need:** *"let's remove the
breadcrumb bar from TIDE, it's a bit redundant in a product where you seldom dig
deeper than 1 level in."*

That is the better answer than either option the row offered. TIDE's only uses of
`EditorScreenshot` are `SynthEditGui.cpp`'s `ContainerThumbnail.h` include and
the link line in `SynthEditSem/CMakeLists.txt`, and **both exist solely to draw
crumb thumbnails** — so the dependency leaves with the feature, nothing has to
come out of the commercial repo, and **C7e loses its last non-`SynthEditLib`
blocker.**

### Why the removal is U3 and NEEDS-JEFF rather than something I did

Reading the code before cutting found two things the ruling does not settle, and
either one guessed wrong ships a worse product than the bar:

1. **The crumbs are the only way back UP a level.**
   `breadcrumbBar->onNavigate` (`SynthEditGui.cpp:699`) is one of exactly **two**
   navigation entry points. The other, `seApp->onOpenContainerView` (`:703`),
   goes only *in*, or to the master via "Goto Structure…". I grepped
   `SynthEditLib`, `EditorLib` and `SynthEdit2`: **there is no existing
   go-to-parent affordance.** Remove the crumbs with no replacement and a user
   who opens a module is stranded in it.

2. **The About pane's only entry point is anchored to the crumb strip.** D6's own
   comment calls it *"the about pane and the only way in"*
   (`SynthEditGui.cpp:292-299`), a plain text affordance at the strip's right end.

U3 carries three options and recommends **(a) keep a thin strip with just
"◀ Back" and "About"** — the only one that changes no interaction the user
already has, while dropping exactly the part Jeff called redundant: the thumbnail
trail.

**`SE2::BreadcrumbBar` itself is not being deleted.** It is shared with the
WinUI3, Wayland, JUCE and Mac frontends; only TIDE stops using it.

**Learned:**

1. **"Remove the feature" can be the right answer to a licensing-boundary
   question, and it is not one an agent would have proposed.** C7c framed the
   choice as *which files move*; the cheapest answer was that none do. Worth
   remembering the next time a row's options list looks exhaustive.
2. **A one-line product decision can have load-bearing code underneath it.** The
   bar looked like a widget and is also the navigation model and the About
   pane's front door. Reading before cutting cost ten minutes.

**Next:**

1. **U3 wants Jeff's answer on Back and About**, then it is one session.
2. **C7e** is now unblocked from the `EditorScreenshot` direction; **C7b** and
   **C7d** are unchanged and still the linux box's.
3. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — the workflow edit, and the unexplained `TestVoiceAllocation` residual.

**Branch/PR:** `tide/mac/C7c-drop-breadcrumb` — TideSynth only, no code change.

## 2026-08-20 — macos — a mac build break from the carve-out, and five test failures CI has been hiding for a week

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (Claude Code CLI version not resolvable on this box) · as **tide-rack-bot** (both paths)

**Fourth and fifth items this session**, on Jeff's instruction — he pointed at a
failing Actions run rather than the backlog. Repos synced first.

## 1. The build break: [SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63)

`cmake_mac` had been red on `master` since `6c7e90053` while `cmake_win` and
`cmake_linux` were green at the same sha.

**The first thing to get right was WHICH step failed.** The 5 test failures in
that log are a decoy: `ctest` is `continue-on-error: true`, so **step 6 is marked
success**. The run failed at **step 20, the Xcode build**:

```
SynthEditMac/SynthEditMac/MidiAutomationWindowController.mm:3:10:
  fatal error: '../../SynthEdit2/PatchParameter.h' file not found
```

Two headers left `SynthEdit2/` during the carve-out and the Xcode consumer was
never re-pointed:

| header | moved by | now at |
|---|---|---|
| `PatchParameter.h` | `9a53a4882` — C12f, the patch cluster | `SynthEditLib/` |
| `IMidiDriver.h` | `4f6f5b1ca` — C13, the three orphan headers | `SynthEditLib/` |

CMake follows them because SE16's `CMakeLists` hands EditorLib the include
directories; the Xcode project quotes the paths literally. **This is exactly the
hazard C10's row already names** — *"Non-CMake consumers still need checking by
hand: `SynthEdit2.vcxproj` and the SynthEditMac Xcode project"* — so the carve-out
stages should treat that line as a checklist item, not a footnote.

**I fixed both files although CI named only one.** Xcode stops at the first fatal
error. Reverting both edits and rebuilding — with the `.mm` already compiled —
gives `CoreMidiDriver.h:6:10: fatal error: '../../SynthEdit2/IMidiDriver.h' file
not found`. Fixing only what CI printed would have turned one red run into two.

**Verified by building, twice**: at the original tip, and again after merging the
current `master` (`61eaf744b`, C14 landed, which changes EditorLib's include
directories so it was worth re-checking rather than assuming). Both times:
`cmake --build` **1064/1064 rc=0**, then `xcodebuild -scheme SynthEdit -arch
arm64 -configuration Release` → ***\*\* BUILD SUCCEEDED\****.

### A dev-box trap found on the way to that link

Reaching the link first produced
`Undefined symbols: ApplyConfigPreInit(SynthEditApp&, ...)`. Cause:
`libEditorLib.a` is built from `build/_deps/syntheditlib-src` (FetchContent,
**`GIT_TAG origin/main`** — a live ref), while the Xcode project resolves
`ApplySynthEditConfig.h` from the **sibling** `../SynthEditLib` clone. The clone
was behind C14, so the library exported the new `CSynthEditAppBase&` signature
and the header still declared `SynthEditApp&`.

**CI cannot hit this**: its *"Symlink CMake-fetched deps for Xcode"* step points
`../SynthEditLib` at `build/_deps/syntheditlib-src`, so header and library are one
tree by construction. On a developer box they are two trees tracking a moving
ref. Fixed by fast-forwarding the clone to `86ab11c`. **This is S17's shape one
level up** and worth knowing before it costs someone an afternoon.

## 2. The five test failures — filed as S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)

**They are not a regression, and they are not one bug.**

`94% tests passed, 5 tests failed out of 86` appears in *every* mac run I
checked, including 2026-08-13, 08-14 and 08-18 — **all of which are marked
success**. `continue-on-error: true` is why nobody knew.

| platform | at `6c7e90053` |
|---|---|
| Windows | 92/92 |
| Linux | 86/86 |
| **macOS** | **5 of 86 fail** |

Reproduced locally with figures **identical to CI's to four decimal places**, so
deterministic rather than flaky.

**The A/B that splits them.** `SynthEdit/CMakeLists.txt:281` adds, on Apple only,
`-fno-math-errno -fno-trapping-math -fno-signed-zeros -fassociative-math
-freciprocal-math`. Windows gets `/fp:fast`; **Linux gets no fast-math at all**,
which is consistent with Linux being the platform that passes. Rebuilding the
whole tree with the two reassociating flags removed and nothing else changed:

| test | with | without |
|---|---|---|
| `TestSoundfont.SoundfontOsc` | FAIL | **PASS** |
| `Unterminated_Poly_Modules` | −80.7666 dB | **−80.7666 dB** |
| `Voice_Allocation_Mono_High` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Last` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Off` | −73.407 dB | **−73.407 dB** |

So `SoundfontOsc` is FP reassociation, and **the four `TestVoiceAllocation` cases
are something else entirely** — bit-identical output either way, i.e. a real
deterministic mac-vs-reference difference the flags do not touch.

**Hypothesis, and labelled as one:** max −68 dB against an average of −150 dB
means a handful of samples differ, not a level or timbre — a constant offset
would put the average near the max. That is the shape of a one-sample timing
difference at voice transitions, which fits tests whose whole subject is voice
allocation. Unconfirmed.

**Not fixed on purpose.** Bumping four tolerances would turn the suite green and
throw the finding away, and whether −68 dB is acceptable in this product is not
an agent's call. The reporting half — removing `continue-on-error` — is a
`.github/workflows/**` edit the token structurally cannot push.

**Learned:**

1. **A `continue-on-error` step turns a failing suite into a decoy twice over.**
   It hid five real failures for a week, *and* it put a wall of red test output
   at the top of a log whose actual failure was fourteen steps later. Read the
   per-step conclusions before reading the log.
2. **"Last green run" is not a baseline when a step can fail without failing the
   run.** My first instinct was to bisect `9674bbfc7..6c7e90053`; the tests were
   already failing at `9674bbfc7` and in every run before it. Checking the older
   *green* runs' logs cost one command and saved a bisect that would have found
   nothing.
3. **An A/B that changes one flag is worth more than a plausible story.** The
   mac-only fast-math subset explained all five failures beautifully. It explains
   exactly one.

**Next:**

1. **S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — diagnose the four, rule on SoundfontOsc's tolerance, and
   Jeff removes `continue-on-error`.
2. **[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) is open and green** — mac `master` stays broken until it merges.
3. **A22, A23, A24** are the remaining A-series rows; A23 is the best-specced.

**Branch/PR:** `tide/mac/mac-ci-findings` (this bookkeeping) plus
[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) (the code).
Two repos; the SynthEdit half is the whole fix and this half is the record, so
neither blocks the other. Throwaway worktrees; every checkout left on its default
branch and clean.

## 2026-08-20 — windows — C14: the last private include was never needed, and it was hiding an ODR violation

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (the Claude Code CLI is not resolvable on this box either — `claude --version` is *command not found*, same as the mac entries report) · as **tide-rack-bot** (both paths: REST `gh api user` and GraphQL `viewer { login databaseId }` → `tide-rack-bot` / `314850083`) · transport check `git config --global --get url."https://github.com/".insteadOf` → `git@github.com:`

**The prompt changed under this run.** It started at **`eba799e`** and `origin/main` is now at **`2ce10c7`** — A21 landed mid-run and rewrote STEP 0.7 and STEP 4's provenance template. The sha above is what actually executed; the line is written in the *new* format because the new format is strictly more informative and this run had both paths' answers anyway. Nothing else in the diff changed what this run did.

**Did:** narrowed `ApplyConfigPreInit` / `ApplyConfigPostInit` from `SynthEditApp&` to `CSynthEditAppBase&`, which took the public repo's dangling private includes **1 → 0** and let `SE16`'s root `CMakeLists.txt` drop the `../SynthEdit2` include directory it was adding on `EditorLib`'s behalf. Also flipped and archived **A28**, filed **C15**, and re-pointed the `win` and `any` NEXT rows.

### It never needed the private type, and that is the whole fix

C14's row expected C11's treatment — invent a public interface. **No new interface was needed: `CSynthEditAppBase` already is one.** Every member the two functions touch is declared in the public repo:

| used | declared |
|---|---|
| `SetQuiet()` | `SynthEditAppBase.h:201` — `CSynthEditAppBase` |
| `setTemporaryRegistration()` | `SynthEditAppBase.h:270` — `CSynthEditAppBase` |
| `rescanIncludesVsts` | `Application.h:144` — `ApplicationBase` |
| `overrideSamplesFolder` | `Application.h:45` — `ApplicationBase` |
| `RefreshModuleData()` | `Application.h:138` — `ApplicationBase` |

`CSynthEditAppBase : public gmpi::hosting::interThreadQueUser, public ApplicationBase`, so one name covers all five. Derived-to-base reference conversion is implicit, so **no caller changed** — `MainWindow.xaml.cpp:500`, `SynthEditCL/main.cpp:128,138` and `SynthEditMac/.../AppDelegate.mm:175,279` compile unmodified. The header got *lighter*: it now forward-declares `class CSynthEditAppBase;` and the `.cpp` includes `SynthEditAppBase.h`.

**One premise in the row was true but was never the operative reason.** It says `SynthEditApp.h` "pulls in `moonbasepp_Licensing.h`, the commercial licensing library". C7a had already measured that the include sits inside `#ifdef SE_MOONBASE_SUPPORT`, and `SynthEditSem/CMakeLists.txt:56-62` says so in place. What actually blocked a move is simpler and was never written down: **`SynthEditApp` is a private application class**. The licensing framing made C14 look like a policy problem; it was a plain layering one.

### The finding worth more than the change: this was a live ODR violation

There are **two unrelated classes named `SynthEditApp` in the global namespace**:

- `SE16/SynthEdit2/SynthEditApp.h:14` — `: public CSynthEditAppBase, public ILicenseState`
- `SE16/SynthEditCL/SynthEditAppCl.h:16` — `: public CSynthEditAppBase`

**Both were passed to these functions**, which are compiled **exactly once**, into `EditorLib`, against the first. `SynthEditCL` then links that object and calls it with the other class. That is an ODR violation. It linked and ran only because `CSynthEditAppBase` is the **first** base of both, so the subobject the bodies actually touch sits at offset 0 in each — change either class's base order and it becomes a silent memory bug, not a link error.

**Positive control**, because "the mangled name ignores the definition" is the load-bearing claim. A four-file MSVC repro — two headers each defining a different `class SynthEditApp`, one defining TU, one calling TU — `dumpbin /symbols`:

```
OLD  definition (class A) : ?ApplyConfigPreInit@@YAXAEAVSynthEditApp@@@Z   SECT3 External
OLD  reference  (class B) : ?ApplyConfigPreInit@@YAXAEAVSynthEditApp@@@Z   UNDEF External
NEW  definition           : ?ApplyConfigPreInit@@YAXAEAUAppBase@@@Z
NEW  reference            : ?ApplyConfigPreInit@@YAXAEAUAppBase@@@Z
```

Identical symbol, different types, no diagnostic. And on the real build, definition and reference now agree on one type that both sides actually have:

```
EditorLib/.../ApplySynthEditConfig.cpp.obj  SECT22 External
  ?ApplyConfigPreInit@@YAXAEAVCSynthEditAppBase@@AEBUSynthEditConfig@@@Z
SynthEditCL/.../main.cpp.obj                UNDEF  External
  ?ApplyConfigPreInit@@YAXAEAVCSynthEditAppBase@@AEBUSynthEditConfig@@@Z
```

**Result:**

- `python3 scripts/dangling_private_includes.py` — **1 → 0**. Positive control: the same script against the untouched `C:\SE` checkouts still reports **1**, so the zero is the change and not a broken measurement.
- Fresh Ninja tree, Release, MSVC: **1029/1029 RC=0**, **zero `error C` lines**, all four folder overrides local, `SE_APP_BUILD_NUMBER=187`.
- **`ctest`: 92/92 passed, 0 failed** (93 listed, 1 disabled upstream) — the known-green figure.
- Linked: `SynthEdit_VST3.vst3`, `SynthEdit_GMPI.gmpi`, `SynthEditCL.exe`, `EditorScreenshot.lib`, `TIDE.gmpi`, `TIDE_VST3.vst3`, `TIDE_STANDALONE.exe`, `synth_ui_tests.exe`, `dsp_tests.exe`.
- **`EditorLib` now compiles with ZERO private include directories.**

**The edge count is 1029, not the 1017 the last win entry recorded.** Upstream growth (`SynthEditLib` gained `--set-plugin-info` and more), not a regression — which is exactly what the baseline note says to check rather than assume.

### One consumer broke, and its breaking is the correct outcome

```
C:\SE\_c14\SE16\tests\layouttests.cpp(9): fatal error C1083:
  Cannot open include file: 'SynthEditApp.h': No such file or directory
```

`synth_ui_tests` was reaching the private header through `EditorLib`'s **PUBLIC** include directory. It already compiles `../SynthEdit2/SynthEditApp.cpp`, so the dependency was never new — only the route. It now names `../SynthEdit2` in its own `target_include_directories`, which is where a private target's private dependency belongs.

**Nothing else in the tree was exposed, and that was measured, not hoped.** Enumerated every SE16 source outside `SynthEdit2/` that includes a `SynthEdit2`-only header: **23 file/include pairs** across `SynthEditCL`, `SynthEditJuce`, `SynthEditMac`, `SynthEditSem`, `SynthEditWayland` and `tests`. Every consuming target names the directory in its own build file — `SynthEditCL/CMakeLists.txt:18`, `SynthEditWayland/CMakeLists.txt:190`, `SynthEditJuce/CMakeLists.txt:59`, `SynthEditSem/CMakeLists.txt:74` — and `SynthEditMac` is an Xcode project that never saw the CMake variable at all. **So mac and linux are structurally safe here, not lucky**, which is the only honest thing a win box can say about them.

**Not verified, stated rather than glossed:** `SynthEdit2` (WinUI3) cannot be built without writing into Jeff's Visual Studio tree. Checked by inspection instead — it does not compile `ApplySynthEditConfig.cpp` (it links `EditorLib.lib`), its `AdditionalIncludeDirectories` already names `..\SyntheditLib`, and `MainWindow.xaml.cpp` sits in `SynthEdit2/` so its own include resolves own-directory-first.

### Filed, not fixed: C15 — TIDE's private include is now dead weight

`SynthEditSem/TideAppStubs.cpp:31` includes `"SynthEditApp.h"` to define `SynthEditApp* theApp`, `SynthEditApp::isMoonbaseEnabled()` and `SynthEditApp::licenseIsActive()`. **Grepping the whole public repo for those three names returns 0 hits** — C11 removed the last consumer when `MfcDocPresenter.cpp:1316` moved to `GetLicenseState()`.

C7a's and C14's own CMake comments both predicted the two would "want ONE fix". **They were wrong, and that is worth recording:** narrowing a *signature* cannot remove a *definition of someone else's member function*. The two halves are independent, and the second is a deletion with a link test for an acceptance check. Filed as C15 rather than done here, because `SynthEditSem` is outside C14's stated Scope and STEP 3 says file, do not fix.

**Learned:**

1. **"It needs a private header" and "it needs the private type" are different claims, and only the second is a real dependency.** C14 sat as the carve-out's last blocker for a day with the fix being *delete one include and widen one parameter to a base class that was already there*. Before designing an interface, check what the code actually touches — the answer here was five members, all already public.
2. **A same-named class in two apps is an ODR violation the toolchain will never report.** MSVC's mangling carries the *name*, not the definition, so the linker binds them silently and the program works exactly as long as the used members happen to live at the same offsets. `SynthEditApp` is not a unique name in this tree; anything taking one by reference across the `EditorLib` boundary has the same latent bug.
3. **A `PUBLIC` include directory is a dependency you have loaned to every consumer.** Removing `EditorLib`'s did not break `EditorLib` — it broke `synth_ui_tests`, three levels away, which had been silently borrowing it. The general form: when a shared target exports a path, you cannot tell from that target who is relying on it.
4. **Do not build in the session scratchpad on Windows.** Ninja embeds the absolute *source* path inside object paths (`CMakeFiles/EditorLib.dir/C_/SE/…/foo.cpp.obj`), so a scratchpad path is over the 260-character limit before the first TU compiles. `git worktree move` relocates a worktree after the fact, which is how this run recovered without re-cloning.
5. **The mac run's "C14 and C10 are `SynthEditLib`, rejected as GATED" reads the gate too widely.** STEP 5 gates that repo *unless the item is an approved carve-out stage*, C0 is approved, and C11/C12/C13 all landed there by PR on exactly that basis — C13 from this box. What the 2026-08-11 C8 ruling forbids is a **non**-carve-out item reaching in. That distinction is what rules out **P3**, and it is why the `any` cell now says so in place.
6. **`origin/main` moved three times while this run built.** A21, A28 and A29's archive all landed. Measuring `check-backlog-diff` against a *freshly fetched* base rather than the branch point is what caught it — the first run of that check reported "A21: Item column differs" for a row this session never touched, which is the tell that the base is stale, not that the edit is wrong.

**Next:**

1. **`win` has no eligible item of its own.** `P3` is the only `win`-marked TODO and it is GATED, not a carve-out stage, not a build break — so this platform falls through to `any` every week. **That wants a ruling from Jeff, not another run re-deriving it**: either P3 escalates as a `PROPOSED:` entry, or it is re-marked.
2. **C15** is small, ALLOWED, and has a link test for an acceptance check. It is the `win` cell's new target and the `any` cell's fallback.
3. **C7d still cannot pass**, and the reason has changed: not C14's dangling header any more, but that C14 has not *merged*.

**Branch/PR:** [SynthEditLib#27](https://github.com/JeffMcClintock/SynthEditLib/pull/27) + [SynthEdit#62](https://github.com/JeffMcClintock/SynthEdit/pull/62) + [#177](https://github.com/JeffMcClintock/TideSynth/pull/177), all on branch `tide/win/C14-licensing-seam`. **Merging any one without the other two breaks the build**, and it is said in each body.

**Machine state.** All three repos were worked in **throwaway worktrees**; Jeff's own checkouts were never switched and are on their default branches. **`C:\SE\SynthEditLib` was already dirty** with his work in progress — `CUG.cpp`, real content (`git diff --ignore-all-space` shows it, so not CRLF churn) — and was left exactly as found, per STEP 5's third dirt category. `check-commit-authorship.py` clean in all three repos; `check-commit-completeness.py` recorded and verified on every commit; `check-no-direct-commits.py` clean on both GATED repos.
