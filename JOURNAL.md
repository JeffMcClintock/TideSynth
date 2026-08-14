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

## 2026-08-14 — windows — C5

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Carve-out stage **C5**, the app base. Moved fourteen files out of the
private repo into public `SynthEditLib` and finished **C9** with them. The row
promised "one `#include` swap and one macro rename, no new machinery" and that
was accurate — the work took minutes. **The two things worth reading are that
C5 is the first stage to reduce the public repo's private-include debt rather
than grow it, and that the stage list does not cover the files it claims to,
which is filed as C12.**

**Result — all three products build, all tests pass, C9 proven by a control.**

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16`, Release | **905/905, RC=0** — `SynthEdit_VST3`, `SynthEdit_GMPI`, `SynthEditCL`, `TIDE.gmpi`, `TIDE_VST3` |
| the seven moved TUs really compiled from their new home | log shows `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj` for all seven |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | 58/58, 24/24, 10/10 — all RC=0 |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, 0 errors, `x64\Release\SynthEdit2\SynthEdit2.exe` |
| no line-ending churn | `core.autocrlf=false` in both repos; all fourteen `cmp`-identical to their originals before the one C9 edit |

The two MSBuild warnings are `C4834` in `SynthEditApp.cpp:421,437`, a file this
stage does not touch and does not move — pre-existing, not caused here.

**C9's third and last user, with a control rather than an assertion.**

`Application.cpp:22` included `"../se_build_number.h"` and `:97` read
`SE_BUILD_NUMBER` to decide whether `CopyInitialPrefabs` re-seeds the user's
`SynthEdit Projects/Prefabs`. That is the same cache-invalidation shape as
`SkinMgr` and `ModuleFactory_Editor`, so it took the same treatment: include
`se_version.h`, read `SE_APP_BUILD_NUMBER`.

| | |
|---|---|
| `se_build_number.h` today | `SE_BUILD_NUMBER 183` |
| configure output | `EditorLib: SE_APP_BUILD_NUMBER=183 (from se_build_number.h)` |
| `build.ninja` statements carrying the define | **57, every one of them EditorLib**, one distinct value — and `Application.cpp.obj` is among them |
| probe TU, **no** injection | prints **0** — what a clean clone gets, i.e. TIDE from C7 |
| probe TU, **with** injection | prints **183** |

The probe is the part that matters: `se_version.h` defaults the macro to 0, so
a lost injection **fails silently** — nothing would fail to compile and the
prefab copy would just quietly stop invalidating. An absence of build errors is
not evidence here, which is why there is a positive control. Note the count is
still 57, unchanged from C4: `Application.cpp` was already an EditorLib TU, so
C5 changed which directory it compiles from, not which target owns it.

**Learned — C5 is the first stage to pay debt down, and both halves were
measured from git refs.** C4 went 47 → 56 and warned that C5 would move
`Application.h`, the most-included name on the list. It did:

| | dangling private includes in `SynthEditLib` |
|---|---|
| before C5 (`SE16 origin/master` / `SynthEditLib origin/main`) | **59** |
| after C5 | **54** |
| closed by C5 | **15** |
| opened by C5 | **10** |

**These are not comparable to C4's 47/56 as absolute numbers** — the C4 script
lived in that run's scratchpad and is gone, so this is a re-implementation with
slightly different normalisation (it counts `./CLine2.h` and `CLine2.h`
separately, and includes SE16's vendored `soundpipe.h`). The delta is the honest
figure, and both sides were read from git refs with the same script, which is
the thing C4's entry said to get right. Script re-created as `dangling.py` in
this run's scratchpad; it is ~70 lines and the next stage should re-create it
again rather than trust a number.

The 15 closed are all `Application.h` (11) and `SynthEditAppBase.h` (4), from
`CContainer.cpp`, `CUG.cpp`, `CUG_with_patches.cpp`, `DocOb.cpp`, `plug4.cpp`,
`checkpoint.cpp`, `imbedded_file.cpp`, `SynthEditDoc2.cpp`,
`SynthEditDocBase.cpp`, `MfcDocPresenter.cpp`, `SkinMgr.cpp`, `ModuleBrowser.cpp`,
`PropertiesBrowser.cpp`, `CancellationAnalyse.cpp` and `CpuMeterGui.cpp`.

**Of the 10 opened, 6 are closed by construction later and 4 are not:**

| header | on a stage's list? |
|---|---|
| `commandMgr.h` (×2), `PatchParameter.h`, `SuspendDSP.h`, `ui_msg_target.h` | yes — on `EditorLib/CMakeLists.txt`'s list, so C12 closes them |
| `IMidiDriver.h` (×2), `ParseSynthEditArgs.h`, `ISEAppManaged.h` | **no stage's list** |
| **`SynthEditApp.h`** | **no stage's list — and it is C11(a)** |

**`SynthEditApp.h` is the one to look at, because C5 has just given C11 a second
call site and C11 is still unruled.** C4 found `MfcDocPresenter.cpp` reaching
`theApp->isMoonbaseEnabled()`; now `ApplySynthEditConfig.cpp` includes the same
deliberately-excluded header from the public repo. That header is excluded *by
design* so each app picks its own `SE_MOONBASE_SUPPORT` without ODR conflicts,
so it cannot simply join a stage's list the way `ModulePicker.h` can — C11's
question got wider, not just longer. `ISEAppManaged.h` was already dangling
before C5 (`CUG.cpp`); C5 adds a second includer. `IMidiDriver.h` and
`ParseSynthEditArgs.h` are new and on nobody's list.

**Learned — the big one. The carve-out's stage list does not cover the files it
says it does, and C7 cannot pass until something does.** `docs/carve-out.md`
stages C3, C4 and C5 as "the rest", and `EditorLib/CMakeLists.txt`'s own comment
said "C3-C5 convert the rest, and C6 moves this file itself". After C5 there are
still **41 `${EDITOR_DIR}` entries** — roughly 21 units: `CLine2`, `commandMgr`,
`Control`, `CPlugin`, the four `Ctl_*` controls, `Dialogs_editor`, `GuiPin`,
`IGuiHost`, `InterfaceObject_editor`, `legacyExternalApp`,
`ModuleDragAndDropManager`, `Module_Info_Plugin`, `PatchManager`,
`PatchParameter`, `PatchParameter_host_generated`, `platform_editor`,
`resource.h`, `SuspendDSP`, `UG2`, `ui_msg_target`. **C7's whole test is a clean
clone with no access to SE16**, and every one of these is on EditorLib's source
list, so C6 would move a `CMakeLists.txt` that points at 41 files a stranger
cannot see. Filed as **C12**; the CMakeLists comment now says so rather than
claiming the conversion is finished, so the next reader is not misled the way
this run was.

I did not widen C5 to absorb them. C5's file list is Jeff's, the extra 41 are
about three times C5's own size, and reshaping an approved stage from inside it
is not a scheduled run's call — the same reasoning C4 used for C11.

**Learned — `cmd.exe /c` is unusable from the Bash tool on this box, and it
fails by hanging rather than erroring.** MSYS path conversion rewrites the `/c`
switch to `C:/`, so `cmd.exe` starts *interactively*, prints its banner, and
blocks on stdin until the tool times out. It looks exactly like a slow CMake
configure. Ten minutes were lost to it before the banner in the log gave it
away. Either use the PowerShell tool for anything that needs `vcvars64.bat`
(what this run did), or set `MSYS_NO_PATHCONV=1`. Note the failure is silent in
the other direction too: the wrapper's own `echo RC=$?` reported **0** for a
command that never ran.

**Learned — the C7 cache-thrash problem S2 found for skins now has a third
instance, and it is the same mechanism.** S2 predicted that from C7 TIDE takes
`SE_APP_BUILD_NUMBER=0` while SynthEdit keeps 183, so each would re-copy 724 KB
of skins over the other on every launch. `CopyInitialPrefabs` is keyed on
exactly the same macro and writes to `SynthEdit Projects/Prefabs` (1.1 MB, 35
files per S2's own measurement), so it thrashes identically — as does the module
cache XML. Today nothing thrashes, because TIDE links the one EditorLib that
carries the injection and so sees 183 too; the divergence begins at C7. This
strengthens S2's argument that the mechanism should be removed from TIDE
*before* C7 rather than after, and it is now three caches, not one.

**Learned — no `.vcxproj` or Xcode edit was needed, unlike C3 and C4, and that
is worth checking rather than assuming.** Neither `SynthEdit2.vcxproj` nor
`SynthEditMac/SynthEdit.xcodeproj` mentions any of the fourteen files: the
`.cpp` had already moved to EditorLib (the vcxproj carries only comments saying
so) and the headers were never listed. `$(SolutionDir)..\SyntheditLib` is
already on the vcxproj's `AdditionalIncludeDirectories`, so every consumer's
`#include "Application.h"` keeps resolving through a search path — just to a
different directory. Two stale path comments in `SynthEditCL/CMakeLists.txt` and
`SynthEditWayland/CMakeLists.txt` did name `SynthEdit2/Application.cpp` and were
updated.

**STEP 1 / 1.5:** no `platform:win` issues; the only open issue anywhere in the
five repos is TideSynth [#44](https://github.com/JeffMcClintock/TideSynth/issues/44)
(the A6 watchdog digest, `github-actions`, informational) — noted, not acted on.
**Zero open PRs in all five repos** at claim time, and no `tide/**` branch on any
remote, so nothing was handed back to this platform and nothing was in flight to
collide with. `SynthEdit` carries an unrelated `claude/audio-sample-rate-persist-795c69`
branch, not a fleet branch; left alone.

**A4's first live test is available now.** The A4 run said to flip it to `DONE`
only after watching it merge one PR and leave another alone, and that the next
scheduled run's own PRs are the natural first test. This run supplies both
controls at once: the TideSynth PR is `BACKLOG.md` + `JOURNAL*.md` only and
should auto-merge, while the two code PRs are in other repos entirely and the
tier must not touch them. A4 is left `IN-REVIEW` — this run did not observe the
firing, and observing it is the whole condition.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches** at claim time. Nothing of his was committed, reverted or
stashed. All five were **fast-forwarded** to `origin` before starting —
`TideSynth` 4 commits behind, `SE16` 3, `SynthEditLib` 2, `gmpi_ui` 1,
`GMPI_Wrappers` 2 — true fast-forwards with nothing to stash, noted because it is
a change to his trees. `SE16/SynthEdit2/.claude/worktrees/` holds two stray
agent worktrees (`audio-sample-rate-persist-795c69`, `blank-project-app-load-d27d84`);
they are ignored, they are not mine, and they were not touched — but note they
make a naive `grep -rn` over `SE16` return every hit three times.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** a scratch Ninja tree and two probe binaries under
the session scratchpad, all outside every repo — Jeff's own `SE16\build` was not
configured or built into. The MSBuild run **did** write into `SE16\x64\Release\`,
which is `.gitignore`d and is where that build has always put its output.
`SynthEditCL.exe` was built but never executed, so no module cache, skin folder
or prefab folder was created or invalidated on this machine.

**Next:**

1. **Merge both code PRs together** —
   [SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) and
   [SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7).
   Merging either alone breaks the build: the files exist in exactly one repo at
   a time and `EditorLib/CMakeLists.txt` points at the new location.
2. **C12 before C6, not after.** C6 moves `EditorLib/CMakeLists.txt` into the
   public repo; doing that while it still points at 41 private files puts a
   source list a stranger cannot satisfy into the public repo, which is the
   `cpu_accumulator.h` shape C2 hit, at 41× the size. C12 is `win`-shaped only in
   that this box has all three build systems to verify against; the row is `any`.
3. **C11 needs Jeff and now has two call sites, not one.** `SynthEditApp.h` is
   reached from `MfcDocPresenter.cpp` (C4) and `ApplySynthEditConfig.cpp` (C5).
   It cannot join a stage's list the way `ModulePicker.h` can, because its
   exclusion is deliberate and about `SE_MOONBASE_SUPPORT` ODR.
4. **`IMidiDriver.h`, `ParseSynthEditArgs.h` and `ISEAppManaged.h` are on no
   stage's list either** — smaller and probably uncontroversial, but they are
   real C7 blockers and C12 should absorb them explicitly.

**Branch/PR:** `tide/win/C5-move-app-base` in three repos —
[SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) (deletions,
`EditorLib/CMakeLists.txt`, two comment fixes),
[SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7) (the
fourteen files + `se_version.h`),
[#56](https://github.com/JeffMcClintock/TideSynth/pull/56) (BACKLOG, JOURNAL,
`docs/carve-out.md`). No other repo was committed in or modified.

---

## 2026-08-14 — linux — A4 built (interactive session, Jeff directing)

**Did:** Built the auto-merge tier for coordination PRs, after Jeff asked why
BACKLOG/JOURNAL bookkeeping needs a human at all. It does not; A4 has been the
answer since 2026-08-09 and could not be built by the fleet itself. **Two
defects in its specification were found first, and the second is the serious
one.**

**Why no scheduled run could ever have done this.** A4 is a
`.github/workflows/**` file and the bot token is `repo` scope with no
`workflow` — measured this session, `x-oauth-scopes: repo`. That is the
credential-layer enforcement of the no-workflow-edits rule working exactly as
designed, and its consequence is that **the one item that would free scheduled
runs is the one item scheduled runs are structurally forbidden from building.**
Jeff's own token on this box already carries `workflow`, so no `gh auth refresh`
was needed here; the commit is his.

**Defect 1 — the allowlist would have fired close to never.** A4 says
"PRs touching only `JOURNAL.md`, `BACKLOG.md`, and `docs/**`". Checked against
the file lists of the last seven merged PRs, that scores **0 of 7**:

| Blocker | PRs |
|---|---|
| `JOURNAL-2026-08.md` | 7 of 7 |
| `BACKLOG-DONE.md` | #55, #54, #48 |

**A8 created both files on 2026-08-12 — three days after A4's allowlist was
written — and nobody updated the allowlist.** STEP 4 *mandates* rotating into
exactly those two files every run, so the tier would have shipped, looked
correct, and merged almost nothing. With them added the same seven score
**3 of 7**: #48, #50 and #55 auto-merge; #51, #52 and #54 correctly wait on
`scripts/`, `tests/` and `tools/`; #41 on `.github/`.

**Defect 2 — `docs/**` was too wide, and the miss was `docs/decisions.md`.**
A4 excluded `docs/weekly-run-prompt.md` and `PLAN.md` because "they steer the
fleet". `docs/decisions.md` steers it harder: that file *is* the PROPOSED
mechanism, and its own text says **"Jeff's merge of that PR is the decision."**
Auto-merging it would let a run answer its own escalation — the single file in
`docs/**` where merging is an act of authority rather than bookkeeping. Now
denied by name, and the selftest tries it both alone and smuggled in beside a
legitimate journal entry.

**Result — design, and the option deliberately not taken.**

The trigger is `workflow_run` on `lint`, not `pull_request`:

- A `workflow_run` job always runs the **default branch's** copy of the
  workflow, never the PR's. So a PR cannot edit the rules that judge it. The
  allowlist denies `.github/**` anyway; this is the second lock.
- It gates on lint having actually concluded green.

**`gh pr merge --auto` was rejected, and the reason is measurable rather than
stylistic.** It delegates the waiting to GitHub, which only works when a
ruleset marks lint a *required* check. This repo has `allow_auto_merge:false`,
and ruleset `20600401` ("Agent PRs only") carries **only** `deletion`,
`non_fast_forward` and `pull_request` — **no required-status-checks rule at
all.** With neither, `--auto` merges immediately and the lint gate is
decorative. Anyone reaching for `--auto` here should check those two settings
first; the failure is silent and looks like success.

Making lint a required check repo-wide was the other route and was **not**
taken: it changes the merge rules for every PR including code, which is wider
than A4's own "Human merge remains for … all code repos". Keeping the tier
inside one workflow plus one script means no repository setting can drift out
of sync with it.

**Guards beyond the allowlist**, because a path allowlist alone is not an
authorisation model: author must be `tide-rack-bot`, not a draft, open, and
based on the default branch. Without the author check, a docs-only PR from
anyone able to open one is an unauthenticated write path into `main`. The
workflow never checks out or executes PR code — it reads the changed-file list
from the API and runs the allowlist script from `main`, which is what makes
`contents: write` safe to grant.

**Verification artifact:** the eligibility decision is a script, not YAML, so
it is testable without GitHub. `scripts/automerge_eligible.py --selftest` —
**19 cases, 0 failed.** Seven are the real file lists of merged PRs, so those
expectations are measurements. The rest are edges: both carve-outs alone and
beside a legitimate journal edit, `PLAN.md`, `website/`, the auto-merge
workflow itself, the script itself, an empty list, an unrecognised new
top-level file, and a near-miss on the archive regex (`JOURNAL-2026-8.md`).

**The selftest earned its keep before it ever ran in CI:** `docs/../PLAN.md`
passed the first draft, because it starts with `docs/` and the prefix test was
happy. `git diff --name-only` normalises paths so it is not reachable in
practice — which is precisely why it deserved a guard rather than an
assumption about an upstream tool's output. Now rejected along with absolute
paths and backslashes.

**Learned:**

- **A path allowlist ages badly and silently.** A4's went stale three days
  after it was written, because a *different* item (A8) added two files, and
  nothing connected them. The lesson is not "update the allowlist" but
  "an allowlist needs a test that runs against real recent PRs" — which is why
  the selftest carries seven of them, and why a future run adding a new
  coordination file should add a case there in the same change.
- **Strict inclusion, never exclusion.** The failure mode of an exclusion list
  is that tomorrow's file merges by default. Everything unrecognised fails
  closed, which is why `tools/` and `tests/` blocked correctly without ever
  having been thought about when the list was written.
- **`--auto` is not "merge when checks pass" unless a ruleset says which
  checks.** With no required-status-checks rule it is just "merge now".

**Next:** **not verified live, and it cannot be** — a `workflow_run` workflow
only fires once it is on the default branch, so the first real firing can only
be watched after this merges. Flip A4 to `DONE` only after seeing it **merge
one PR and leave another alone**; the negative control matters more than the
positive one, since an auto-merge action that is wrong about its allowlist is
a worse problem than merging by hand. The next scheduled run's own
BACKLOG/JOURNAL PR is the natural first test.

**Branch/PR:** `tide/linux/A4-auto-merge-tier`. Committed as Jeff, not as
`tide-rack-bot` — the bot token deliberately cannot push `.github/workflows/**`.

---

## 2026-08-14 — linux — #53 fixed; S2 landed (interactive session, Jeff directing)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Did:** Fixed the platform:linux configure break this box filed earlier the same
day, at Jeff's direction, and did the STEP 4 chores that S2's merge unblocked.
**The linux box builds again**, verified from merged `main`.

**Result — the break was wider than the issue said, and the fix is one gate.**

[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) named the pipewire
probe. Reading the file properly found **three** hard-failure sites, any one of
which kills the whole SE16 configure:

1. nine `pkg_check_modules(... REQUIRED ...)`
2. `find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)`
3. `message(FATAL_ERROR)` when a wayland protocol XML is absent

Two facts decided the shape of the fix, and both were checked rather than assumed:

- **Nothing links `Standalone_Wrapper`.** `grep -rn 'Standalone_Wrapper'` across
  `GMPI_Wrappers`, `SE16`, `GMPI`, `gmpi_ui` and `GMPI-plugins` finds no consumer.
  So skipping the target costs nothing today.
- **The parent already documented the contract that was broken.**
  `wrapper/CMakeLists.txt:12-14` says of this very `add_subdirectory`: *"Returns
  immediately on platforms whose shell is not written yet, so this is safe to add
  unconditionally."* The `REQUIRED` probes made that false. The fix restores the
  stated contract rather than inventing a policy.

So: probe everything, collect what is missing, one gate — skip with a message
naming the missing packages and the `apt` line, or fail hard under the new
`GMPI_STANDALONE_STRICT` (default OFF). **That option is the mitigation for the
fix's own downside** — a silent skip on a machine that meant to build the
standalone host — so the change does not trade one invisible failure for another.

Two things kept deliberately, both of which a naive "just drop REQUIRED" would
have lost: the per-module `Found X, version Y` output, which is how you tell
*absent* from *present but too old*; and naming missing protocol XMLs
individually, because the usual cause is a distro too old for the staging
protocols (Ubuntu 22.04 ships wayland-protocols 1.25 and has neither
`fractional-scale-v1` nor `cursor-shape-v1`) and `GMPI_WAYLAND_PROTOCOLS_DIR`
fixes that without touching system packages.

**Verification artifact — built, not just configured:**

| Check | Before | After |
|---|---|---|
| `cmake -S SE16 -B <fresh> -G Ninja` | RC=1 | **RC=0** |
| `cmake --build . --target TIDE_VST3` | could not configure | **298/298**, links `TIDE_VST3.so`, assembles the `.vst3` |

The artifact is real: 91 MB ELF exporting `GetPluginFactory` and `ModuleEntry`.
Two controls so the gate is not passing vacuously — `GMPI_STANDALONE_STRICT=ON`
gives RC=1 naming `libpipewire-0.3`, and the build graph contains **zero**
`Standalone_Wrapper` targets while `VST3_Wrapper`, `CLAP_Wrapper`,
`SynthEditLib`, `EditorLib`, `TIDE_VST3` and `TIDE.gmpi` are all present. (17
`standalone` strings remain in the graph; all are CMake's per-directory
`install`/`test`/`edit_cache` boilerplate, emitted for any added subdirectory
even when it returns early.)

**Re-verified after the merge, which is the check that actually matters:**
configure of merged `main` with `GMPI_Wrappers` **fetched from GitHub rather than
a local override** — RC=0. That is what a fresh clone and CI get, not just what
this box's working tree gets.

**Learned:**

- **A platform-gated `return()` above a `REQUIRED` probe hides the probe from two
  of three platforms.** `if(NOT UNIX OR APPLE) return()` meant Windows and macOS
  never reached the pipewire line, so a hard dependency that stopped the entire
  linux tree could sit on `main` looking green. Any dependency probe below a
  platform gate is, by construction, only tested on the platforms below that gate
  — worth remembering before adding one.
- **Check for consumers before deciding between "make it optional" and "make it
  opt-in".** A target nothing links can be skipped silently at near-zero cost; a
  target something links cannot, because the consumer's
  `target_link_libraries` then fails on a nonexistent target. The grep is one
  command and it picked the option.
- **Resolve-then-generate, never resolve-while-generating.** The original loop
  emitted `add_custom_command`s for the protocols it had found and only then hit
  the missing one. Splitting the loop is what lets a missing XML join the same
  report as a missing package instead of being a separate failure mode.

**STEP 4 chores this unblocked:** **S2** flipped `IN-REVIEW` → `DONE`
([#54](https://github.com/JeffMcClintock/TideSynth/pull/54) merged 2026-08-14)
and moved verbatim to `BACKLOG-DONE.md`. The `linux` NEXT row, which this
morning pointed at "#53 first, then S3", now points at **S3** alone.

**Next:** **S3** for the next linux run — `SE16/SynthEditSem/TideApp.cpp` is
ALLOWED, the box can build again, and S2 supplied the evidence its row lacked
(finding A6: live write sites behind the `assert(false)` stubs). The two
judgements still waiting on Jeff are unchanged: **S1b/S5/S7/S8 are GATED-in-full
and should not read as `TODO`** to a scheduled run, and **A2's sandbox-escape
question gates doing S7 properly**.

**Branch/PR:** [GMPI_Wrappers#3](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/3)
(merged, `49ff927`) for the fix; this entry and the backlog cleanup on
`tide/linux/post-merge-cleanup`. Both working copies left on their default
branches, clean.

---

## 2026-08-14 — linux — S2 (plus a platform:linux build break found and filed)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Did:** Produced [docs/sandbox-audit.md](docs/sandbox-audit.md) — the audit
PLAN constraint 4 cannot be verified without. **The answer is that constraint 4
is violated, and it is not a code-reading inference: the files are on disk on
this machine.** Also found, while establishing TIDE's source set, that **SE16
does not configure on linux at all any more** — filed as
[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) rather than fixed,
per STEP 3.

**Neither of this box's usual entry points was takeable, and the NEXT row said so
itself.** No open `platform:linux` issue at STEP 1; no open PRs in any of the
five fleet repos and only `main` on TideSynth's remote, so nothing to resume at
STEP 1.5. The `linux` NEXT row was the stale P7c pointer, explicitly left as a
question by the 2026-08-14 windows interactive session — *"picking the wrong
replacement here is worse than an honest gap"*. So this run did the screening it
asked for, took the item that survived it, and re-pointed the row. That screening
is the second most useful thing here after the audit itself, because **the queue
is systematically misleading about what a scheduled run can take:**

- **The GATED wall is nowhere recorded in the rows it applies to.** `S1b`, `S5`,
  `S7` and `S8` are all `TODO`/`any`/unblocked and read as available. All four
  are **entirely** work in `SE16/EditorLib/`, `SE16/SynthEdit2/` or the
  `SynthEditLib` repo — GATED to C1-C7 only, per the 2026-08-11 C8 ruling that
  deliberately declined to widen the exception. S8's row actively misleads:
  *"~~GATED on C0~~ unblocked 2026-08-08, C0 approved"* reads as permission, but
  C0 gates the carve-out **stages**, not any item that wants to touch a shared
  file. Each of these will burn a session on discovery exactly as A4 did last run.
- **The workflow wall** (A4, A10, A12, and **B1**, which nobody had named
  before — it is `.github/workflows/build.yml`).
- Screened out for their own reasons: **C9** (its remaining work *is* C5, a `win`
  row — `Application.cpp`), **C11** (needs Jeff's ruling on the `SynthEditApp.h`
  licence gate), **A9** (open NEEDS-JEFF prerequisite), **N1** ("do it after
  C7"), **P7d** (scope question for Jeff, and macOS-only to verify), **D1/D2**
  (mac-shaped), **U1** (needs a fresh post-pivot audit first).

That left **S2**, which is `any`, needs no GATED edit (it *reads* gated code and
writes only `docs/` in this repo), and has no open question that would change
what gets written.

**Result — the audit, and why its file set is trustworthy.**

A grep of SE16 returns hundreds of hits in code TIDE never compiles, which
buries the real ones. The row says *"reachable from a TIDE build"*, so the file
set is derived:

| Step | Result |
|---|---|
| link closure from `build.ninja` | `VST3_Wrapper` + `SynthEditLib` + `EditorLib` — and **no** `.sem` module libraries |
| TUs from `compile_commands.json` | **264** |
| first-party after dropping VST3 SDK + generated Wayland C | **230**, **zero unresolved** |
| categorised hits | **202 across 46 files** |
| DWARF compile-unit list from the unstripped `.so` | **235 CUs linked**; of the 46 hit files, **37 are in the binary, 9 are not** |

That last row is the one that makes the audit worth reading: a static archive
contributes an object only when something references it, so "compiled" and
"linked" are different questions and only the second one matters. Re-runnable
with `python3 tools/sandbox_audit.py --build <tree>`.

**The violation, measured on disk rather than argued from source — ~12.8 MB
across 139 files:**

| Location | Size | Files |
|---|---|---|
| `~/SynthEdit Projects/skins/` | 724 KB | 74 |
| `~/SynthEdit Projects/Prefabs/` | 1.1 MB | 35 |
| `~/.local/share/SynthEdit/` | 11 MB | 30 (six ~330 KB `Plugin-Cache-16-override-*.xml`) |

**Rulings: 7 remove, 4 stub, rest keep.** Full reasoning per finding in the doc.

**Most of it attaches to rows that already exist rather than creating new ones** —
which is the point, and is why no new BACKLOG items came out of it:

- **S7** gets its gated remainder named to the line: `SynthEditLib/SkinMgr.cpp:28-32`
  (the *constructor* calls `setSkinFolder`) and `:48-100` (the recursive copy).
  Confirmed by measurement, not just the static chain S7 asserted — **27 `SkinMgr`
  symbols defined in `TIDE_VST3.so`**, and the literal `SynthEdit Projects` is
  **in the TIDE binary** (`default3` 4×).
- **S1b (b)/(c) reproduced on a second platform** (S1b measured macOS):
  `ScanFolder`, `LoadModuleData`, `LoadOrScanModuleData`, `RegisterExternalPluginsXml`
  all defined, `Module_Info3` 65 symbols, and `dlopen`/`dlsym`/`dlclose` genuinely
  imported per `nm -D --undefined-only`.
- **S1b (a) confirmed genuinely done** by the same measurement: `FileWatcher` has
  **zero** symbols and `FileWatcher.cpp` is **not** a linked CU. Replacing
  `SynthEditApp.cpp` with `TideAppStubs.cpp` did remove the watcher thread.
- **S8 reproduced on Linux** — `ug_soundcard_in` 28, `ug_soundcard_out` 33,
  `ug_midi_out` 24, **`OscillatorNaive` 0**. That last one independently confirms
  the blocker **E2a**'s row warns about, now on both measured platforms: there is
  no modern oscillator primitive registered. S8 also gains a fourth module —
  `ug_wave_recorder.cpp:216` `fopen(…,"wb")` writes a WAV to an arbitrary path.
- **S3 gains its evidence.** Its `assert(false)` stubs have live write sites
  behind them (`CUG.cpp:2833-2968` — `create_directory` ×2, `copy_file`,
  `fopen(…,"w")`), so "silently falls through in release" is measured now.
- **C5** inherits three findings (`Application.cpp` prefab copy,
  `SynthEditAppBase.cpp` module staging, `UG2.cpp`'s write-to-`%TEMP%`-and-
  `LoadLibrary`).

**One genuinely new finding, and it is a ruling rather than a bug.**
`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:343-352` prefers
`getpwuid(getuid())` over `getenv("HOME")`, and the comment says why in as many
words: *"getpwuid returns the real home directory even when sandboxed, whereas
getenv("HOME") returns the container path in a sandbox."* **A deliberate
sandbox escape** — correct for SynthEdit, which is a desktop app, and directly
contrary to constraint 3 for TIDE. It sits **upstream** of both folder-copy
findings, because `getCommonDocumentFolder` falls through to it on every
non-Windows platform, so **S7 cannot be fixed properly without ruling on it**.
GATED (`SynthEditLib`), so flagged, not touched.

**Learned:**

- **The GATED wall is the `any` lane's real filter, and it is invisible in the
  rows.** Last run found the workflow wall and re-marked A4/A10/A12 for it; this
  is the same shape one level over, and it disqualifies four more rows that all
  read as available. Worth a Status-cell pass by Jeff, the same way A4 needs one
  — S1b/S5/S7/S8 are not `TODO` for a scheduled run in any useful sense.
- **A build tree you did not make is a first-class measuring instrument.** The
  whole audit rests on reading `build.ninja`, `compile_commands.json` and the
  DWARF out of `/home/jef/SE/build` — Jeff's own tree, read-only, never
  reconfigured or built into. It answered "what does TIDE actually compile and
  link" exactly, which no amount of CMake-reading would have.
- **`readelf --debug-dump=info --dwarf-depth=1` is the cheap way to get the
  linked compile-unit list** from an unstripped `.so`. `--dwarf-depth=1` is what
  makes it tractable on a 91 MB binary; without it the dump is unusable.
- **The skin-version stamp becomes a thrash bug at C7, and does not look like
  one today.** `SkinMgr` invalidates on `SE_APP_BUILD_NUMBER`, which
  `EditorLib/CMakeLists.txt` injects (`183` today) and `se_version.h` defaults to
  `0`. TIDE links that same EditorLib, so **today TIDE and SynthEdit agree and
  nothing thrashes.** From C7 — clean clone, no private repo — TIDE takes `0`
  while SynthEdit keeps `183`, so each would re-copy 724 KB of skins over the
  other on **every** launch. Argues for removing the mechanism from TIDE *before*
  C7, not after.
- **`std::remove` and `Processor::open(phost)` are the two false positives any
  filesystem grep of this codebase will hit** — the `<algorithm>` overload and
  the GMPI lifecycle call. Both are listed in the doc's "keep" section so the
  next audit does not re-flag them.

**The build break, filed not fixed** ([#53](https://github.com/JeffMcClintock/TideSynth/issues/53),
`platform:linux`): `GMPI_Wrappers` `e707482` (*"feat(standalone): Linux/Wayland
standalone host"*, Jeff, 2026-08-13, current tip of `origin/main`) added
`pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)` at
`wrapper/Standalone/CMakeLists.txt:32`. `wrapper/CMakeLists.txt:14` adds that
subdirectory unconditionally, and the file's own early-out is
`if(NOT UNIX OR APPLE) return()` — **so Windows and macOS skip it and only linux
takes the hard dependency**, which is why it can sit on `main` looking green.
`libpipewire-0.3-dev` is not installed here. Reproduced both with a local
`GMPI_WRAPPER_FOLDER_OVERRIDE` and with it blank so CPM fetches `origin/main`.
Net effect: **the linux box cannot configure, so it cannot build or verify
anything in SE16, TIDE included.** `GMPI_Wrappers` is an ALLOWED path, so it is
ordinary takeable work once someone picks between "make Standalone opt-in" and
"probe without `REQUIRED` and skip".

Because of it, the audit is measured from the pre-existing `/home/jef/SE/build`
tree (configured 2026-08-10). **Drift was measured, not hoped for:**
`SynthEditLib`'s CMake source list is **unchanged** since then (267 entries, none
added, none removed), and `EditorLib`'s differs by exactly `browseto.mm` and
`openurl.mm`, both `if(APPLE)` and not compiled on linux. C4 changed those files'
**paths**, not the set of code compiled, so the audit is current in content; the
doc states this as a limitation rather than burying it.

**STEP 4 chores done as part of this run:** **E1a** flipped `IN-REVIEW` → `DONE`
([#52](https://github.com/JeffMcClintock/TideSynth/pull/52) confirmed merged
2026-08-13) and moved verbatim to `BACKLOG-DONE.md`; **A11** moved likewise (it
was already `DONE` and marked *"ready to rotate"*). Also re-verified A11's own
fix still holds on this box: all nine local repos are `https://`, and STEP 0.7's
second assertion prints `git@github.com:`.

**Next:** **[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) is
STEP 1 work for the next linux run** and outranks the backlog — nothing else on
this box can be built or verified until it lands. **Then S3**, which is the one
remaining `any` row a linux box can genuinely do: its target
`SE16/SynthEditSem/TideApp.cpp` is **ALLOWED**, and S2 just supplied its
evidence. The `linux` NEXT row now says both, with the screening written out.

For Jeff, two Status-cell judgements that are his and not a run's: **S1b, S5, S7
and S8 are GATED-in-full and should not read as `TODO`** to a scheduled run, and
**A2's sandbox-escape question has to be answered before S7 can be done
properly**.

- **`gh pr checks` reports the `build` workflow red on every PR, and `gh run
  list` reports the same runs green — both are correct, and the discrepancy will
  waste someone's time.** `build.yml` sets `continue-on-error` at **job** level,
  so the *run* concludes `success` while all three *jobs* conclude `failure`.
  `gh pr checks` surfaces the jobs; `gh run list` surfaces the run. Checked
  against `main` before assuming this PR caused it: run `31749003642` on
  `3ff987b5` has `failure` for linux, macos and windows too, with the identical
  `CMake Error: The source directory … does not appear to contain CMakeLists.txt`
  — TideSynth has no root `CMakeLists.txt`, on `main` or anywhere. That is the
  C7 failure `build.yml`'s own header says is the point, and **B1 is the row for
  it** (its comment already says "green here still means nothing"). **`lint` is
  the only check that currently gates anything, and it passes.**

**Machine state, for the record:** all nine local repos were clean and on their
default branches at the start and are again at the end; **only `TideSynth` was
committed in**, so STEP 5's two-end-states rule has exactly one repo to satisfy.
Four repos were **fast-forwarded** to `origin` before measuring — `SE16` (7
commits behind), `SynthEditLib` (2), `gmpi_ui` (3), `GMPI_Wrappers` (1) — because
an audit of stale source would have been wrong about what TIDE compiles. All four
were clean beforehand, so these were true fast-forwards on the default branch with
nothing to stash; noted because it is a change to Jeff's trees, small and
reversible though it is. **Pulling `GMPI_Wrappers` is also what surfaced #53** —
its 1 commit was `e707482`, the one that breaks the configure.

**Branch/PR:** `tide/linux/S2-sandbox-audit` →
[#54](https://github.com/JeffMcClintock/TideSynth/pull/54)

---
