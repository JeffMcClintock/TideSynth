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

## 2026-08-20 — macos — A21: the identity gate stops on a wrong answer, not on no answer

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (the Claude Code CLI version is not resolvable on this box — `claude --version` is *command not found*) · as **tide-rack-bot** (both paths, and this entry is the first to say so — see below)

**Third item this session**, on Jeff's instruction, after syncing the repos.
Claimed with a pushed DOING mark before any work.

**Did:** STEP 0.7 now runs **two identity paths** — `gh api user` and
`gh api graphql '{ viewer { login databaseId } }'` — and reads them with rules
that separate **asserted wrong** (STOP, always) from **could not assert** (retry,
then STOP and journal). Wording only; no code.

### All four branches were exercised, not reasoned about

| scenario | REST | GraphQL | rule |
|---|---|---|---|
| healthy | `tide-rack-bot` | `tide-rack-bot 314850083` | proceed, record `(both)` |
| **one path down** — REST forced through a dead proxy | `Get "https://api.github.com/user": proxyconnect …` | `tide-rack-bot` | proceed, record `(GraphQL)` |
| both down | transport error | transport error | retry ~1 min, then **STOP** |
| **credential missing** — `unset GH_TOKEN` | **`JeffMcClintock`** | **`JeffMcClintock`** | **STOP**, unconditionally |

**Row 4 is the one that decides whether any of this is safe, and it is the good
news.** The GraphQL path falls back to Jeff's keyring credential *identically* to
the REST path, so it **cannot launder a missing token**. Adding it is therefore a
second equivalent assertion, not a bypass — which is the only thing that would
have made A21 a weakening of the gate rather than a fix to it.

### The finding worth more than the change

**The two failure kinds are textually distinguishable, so a run never has to
judge which one it is in.**

- A **transport** failure yields **no login at all** — an error string.
- A **credential** failure yields a **perfectly valid login that is the wrong
  one**.

So: *if you are holding a login string, you are in the asserted case, and the
only question is whether it says `tide-rack-bot`. If you are holding an error,
you are in the could-not-assert case, and retrying is correct.* Never treat an
error as licence to continue; never try to retry a wrong name away. That is now a
table in the prompt rather than a judgement call, which matters because the run
making the call is the one whose credentials are in question.

### Two things the row did not anticipate

**GraphQL is the STRONGER assertion, not a lesser fallback.** It returns
`databaseId` = **314850083**, which is exactly the number hard-coded into
`GIT_AUTHOR_EMAIL` (`314850083+tide-rack-bot@users.noreply.github.com`). So it
checks the identity the run's **commits** will carry, where REST only checks the
one its API calls will. Prefer it when both answer.

**A latent documentation bug, in the one rule where being read correctly matters
most.** The paragraph beginning *"That second command MUST print
`tide-rack-bot`"* named the **wrong command**: the second command was the
`insteadOf` transport check, and the identity call was the first. It had said
that ever since the transport check was added, and every run since has had to
silently repair it while reading. Fixed, with a note saying so.

**Also updated:** STEP 0.5's record line, and STEP 4's provenance template, which
now reads `as <login> (<REST|GraphQL|both>)`. This entry's own `**Prompt:**` line
is the first to carry it.

### Repos synced first, per the instruction

Five advanced, five already current, three skipped:

| skipped | why |
|---|---|
| `JUCE` | untracked `modules/juce_audio_processors/format_types/VST3_SDK/` |
| `gimpi_ui_tests` | untracked `build-bench/` |
| `VST_SDK` | detached HEAD, no upstream, untracked `build/` |

All three are build artefacts or a vendored SDK rather than work in progress, but
they predate this run and STEP 5 says the developer's dirt is not mine to clean.
`SynthEdit` `2f5fca5e3 → 6c7e90053`, `SynthEditLib` `65d55cd → 5af259e`,
`TideSynth` `8917c04 → 61b4707`, `gmpi_ui` `6700070 → ad5b681`,
`synthedit-website` `83771db → e51a7c3`; all fast-forwards, none ahead.

**Learned:**

1. **"Add a fallback" was the wrong frame and the row's own wording carried it.**
   The question that decides safety is not *is the second path good enough*, it is
   *does the second path fail the same way the first does when the credential is
   missing*. Measured: it does. Had it not, the correct answer would have been to
   leave the gate alone and accept the lost runs.
2. **A rule that requires the reader to classify a failure will eventually be
   classified wrong**, by whoever is most motivated to continue — which is the
   run itself. Giving the classification a mechanical tell (login string vs error
   string) is what makes it a rule rather than an appeal to judgement.

**Next:**

1. **A22, A23, A24** are the remaining A-series rows. A23 is the best-specced —
   duplicate-id detection in `check-id-refs.py`, with a positive control already
   written into the row.
2. **A22 pairs with this one** — both are rules whose premise stopped matching how
   the fleet runs.
3. **E17** still gates every E2 module stage; **E10** still needs `SynthEditLib`
   authority.

**Branch/PR:** [#176](https://github.com/JeffMcClintock/TideSynth/pull/176), branch
`tide/mac/A21-identity-gate` — one repo, TideSynth only.
**Merge order matters:** this branch and `tide/mac/A28-community-research` (#175)
were both cut from `main` and both touch `BACKLOG.md` and `JOURNAL.md`, so
whichever merges second will want a rebase. No overlap in `docs/`.
Throwaway worktree; the developer's checkout stayed on `main` and clean.

## 2026-08-20 — macos — A28: the refuted hypothesis, corrected in the four places that state it and the one that originates it

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (the Claude Code CLI version is not resolvable on this box — `claude --version` is *command not found*) · as **tide-rack-bot**

**Second item this session**, on Jeff's instruction after A27 merged. Claimed
with a pushed DOING mark before any work, per STEP 2.

**Did:** A9's standing hypothesis — *"no open-source modular exists on iOS AUv3"* —
is false, refuted by plugdata on 2026-08-18. Every live statement of it now
states the surviving narrower form instead: **no open-source *Eurorack-style
rack* on iOS AUv3.**

### The row named two docs. There were four sites, and the fourth is the source

| site | what it was |
|---|---|
| `docs/community-research.md:58` | named by the row |
| `scripts/community-research.py` — `HYPOTHESIS_RE` comment | **not named** |
| `scripts/community-research.py` — `source_hypothesis()` docstring | **not named** |
| `docs/process-review-2026-08-09.md:124` | **not named — and it is where the hypothesis originates** |

That last one matters more than its size. The script's own comment reads *"The
standing product hypothesis from docs/process-review-2026-08-09.md"* — so
correcting the two docs the row named would have left the **citation pointing at
the false version**, which is a worse state than before: a corrected doc that
cites an uncorrected source reads as though the source agrees with it.

**PLAN.md needed no change**, which is worth recording because it looks like it
should have. `PLAN.md:138` already quotes the hypothesis and calls it *"false as
written"* in the next line, and already handles this row's other caveat — the
AUv3 memory ceiling is there as *"Flagged for Jeff; not added unilaterally."* So
no PLAN amendment was made or needed.

**The review doc is MARKED, not rewritten**, and this is a judgement worth
overruling if you disagree: `docs/process-review-2026-08-09.md` is the record of
what the 2026-08-09 review concluded, so its paragraph is left standing with an
inline ⚠ and a dated correction block underneath, rather than edited to say
something the review did not say. The inline marker exists so the paragraph
cannot be read standalone.

### plugdata is on the watch list operationally, not just in prose

The row asked for plugdata on the watch list. Prose alone would not have done it:
`plugdata` is now in **both** `HYPOTHESIS_QUERIES` and `HYPOTHESIS_RE`.

**They have to move together, and nothing said so before.**
`source_hypothesis()` searches for each query, then discards any hit whose
**title** `HYPOTHESIS_RE` does not match. A query with no matching regex term
therefore returns nothing — silently — which is precisely the *"working watch
that had simply found nothing"* failure the `watch` source was built to avoid.
That coupling is now asserted in `--selftest`, documented at both sites, and
written into `docs/community-research.md`.

### Verification

**The coupling assertion is proven able to fail**, not merely present. It is
demonstrated with a synthetic sentinel:

```
FAIL query 'zzq-not-a-product' matches no HYPOTHESIS_RE term -- source_hypothesis()
would discard every hit for it
23 classification case(s), 1 failed
```

exit **1**. Unmodified, exit **0**.

**That sentinel replaced a real product name, and the reason is the finding.**
The first version of this control used a `drambo` query — and `drambo` became a
real title-match term a few hours later, when Jeff pointed out miRack is the
actual iOS competitor. The control would then have passed **for the wrong
reason**, silently, while appearing to still test something. `--selftest` now
asserts the sentinel stays unmatched, so the next person to broaden the regex is
told rather than left to notice. **A control that a later, correct change
disarms is worse than no control**, and this one had a half-day lifetime.

#### Jeff's refinement, and why it is more than a wording change

*"plugdata is not really in the category of eurorack simulators. However mirack
is an ios competitor."*

Correct, and the first version of this change did not encode it — it put plugdata
on the watch list and left the actual competitor off. The list now splits by
**kind**, in the regex comment, in the doc, and in the selftest:

| kind | terms | why |
|---|---|---|
| **Competitor** | `mirack` (query + title), `drambo`, `audulus` (title) | they hold TIDE's real square — an iOS Eurorack-style rack — and all three are closed. **miRack is the product the surviving hypothesis is a claim about**, so an open-source answer to it is the highest-value item this routine could ever surface. |
| **Precedent** | `plugdata` (query + title) | a Pd patcher, **not a rack**. Watched because it already solved the iOS AUv3 packaging, distribution and review problems TIDE will hit, and because it refuted the broader claim. |

A query needs a matching title term; a title term needs no query. So `drambo` and
`audulus` are title-only, which still catches mentions the other sources surface.

**A/B, shipped script vs this branch** — four discriminating positives and two
negative controls:

| title | shipped | this branch |
|---|---|---|
| `miRack 4.6 adds a new sequencer` | `keep` | **`flag`** |
| `Anyone tried Drambo for generative patches?` | `keep` | **`flag`** |
| `Audulus 4 module sharing` | `keep` | **`flag`** |
| `plugdata 0.9.3 released` | `keep` | **`flag`** |
| `Rack-style sequencer module request` | `keep` | `keep` |
| `Pure Data style dataflow patching in a rack` | `keep` | `keep` |

**Stated rather than glossed:** `How does plugdata ship a standalone AND an
AUv3?` is `flag` on *both* — the existing `auv3` term already caught it — so it
is kept as a case for the hypothesis-beats-reject rule and proves nothing about
the new terms. The two `keep` rows are what rule out "everything is flagged".

`--selftest` **23 cases, 0 failed** (was 17). `check-links.py` goes 418 → **421**
relative links with the broken count unchanged at 1, so the new relative links
resolve.

### The lint break cleared mid-run — A29 is DONE

**Jeff pushed `modules/PanelTest/` at
[`61b4707`](https://github.com/JeffMcClintock/TideSynth/commit/61b4707) while this
item was in flight, so A29 is closed and `lint` is green end to end.** Re-verified
here rather than inferred from the commit: `check-links.py` → **418 relative
links, `no broken links`, rc=0** (was 1 broken), and the `modules/` tree builds
from a fresh worktree with the new subdirectory — **configure rc=0, build rc=0,
zero `error` lines**, targets `tide_render`, `tide_render_preview`,
`tide_render_regression`, `TiDEknob`, `TiDEPanel`, **`PanelTest`**.
[#174](https://github.com/JeffMcClintock/TideSynth/issues/174) closed, A29 flipped
to DONE and archived, `win` NEXT re-pointed at P3. **The A4 auto-merge tier is
live again**, which was the whole cost of the row.

**Measured before the push, since the idea of commenting the module out came up,
and worth keeping because it settles the question:** there was **nothing to
comment out and no broken build**. At `41785ea`, `PanelTest` appeared in **no
CMakeLists anywhere** on `origin/main` — only three prose/comment references — and
the `modules/` tree configured and built clean without it (**configure rc=0,
build rc=0, zero `error` lines**). The break was only ever the dangling
reference, so the push was the whole fix.

**The one thing that survives the fix:** the broken link was **one of three**
references to that file. The other two are C++ comments at
`modules/common/TidePathTracer.h:21` and `:883`. All three resolve today, but
**no check reads the comments**, so if `PanelTest/` ever moves or is unpublished,
those two dangle silently and only the README's would be caught. Noted rather
than filed — a one-line risk, not a defect.

**Learned:**

1. **A row that names the files to fix is naming symptoms, not the set.** Two of
   the four sites here were in a script rather than a doc, and the fourth was the
   *origin* of the claim. `grep` for the sentence, not for the filenames the row
   lists — and check whether anything **cites** the file you are correcting.
2. **"Add it to the watch list" is a two-part change in this script**, and the
   two parts are 100 lines apart with nothing linking them. A prose-only or
   query-only edit would have looked done and watched for nothing.
3. **The A27 check caught this run's own regression, twice.** Re-pointing the
   `mac` row earlier today left a live *"Take A29"* phrase in the superseded
   quote, and `check-next-block.py` flagged it. Flipping A29 to DONE just now, I
   did **the same thing again** in the `win` row — the quoted history still read
   as an instruction, and the check failed with
   `BACKLOG.md:11 [win] A29 -- archived DONE`, matched on `'Take A29'`. Both
   times the fix was to strip the imperative from the quote. That is the check
   working on exactly the class of mistake it was written for, on a file its own
   author was editing, within hours of shipping — which is better evidence than
   any fixture.
4. **Name the *kind* of thing being watched, not just the thing.** The first
   version of this change watched plugdata — the refutation — and left miRack,
   the actual competitor, off the list entirely. Nothing in the row or the code
   would have caught that; it took Jeff reading it. A watch list of bare product
   names invites exactly this, so the list now says competitor or precedent
   against each name.
5. **A test control built from a real name has a shelf life.** The `drambo`
   sentinel was correct when written and wrong within hours, in the ordinary
   course of the code getting better. Controls want values that cannot become
   legitimate.

**Next:**

1. **A29 / #174 is DONE** — cleared mid-run, verified here, auto-merge live again.
2. **A21, A22, A23, A24** are the remaining A-series rows — all small, all in this
   repo, all with stated acceptance checks. A23 is the one with a positive control
   already specced.
3. **E17** still gates every E2 module stage; **E10** still needs `SynthEditLib`
   authority.

**Branch/PR:** [#175](https://github.com/JeffMcClintock/TideSynth/pull/175), branch
`tide/mac/A28-community-research` — one repo, TideSynth only.

Throwaway worktree; the developer's checkout stayed on `main` and clean.
