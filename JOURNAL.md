# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-24 — macos — Close-out sweep: two real misses found (interactive)

**Prompt:** great. did we miss anything?

Checked rather than answered from memory, and there were two.

**Five rows were IN-REVIEW with every PR merged.** S7, S3g, P3, S18 and S35 —
thirteen PRs across three repos, all merged, none of the rows flipped. This is
the fourth time today this drift has needed fixing, and the pattern is always
the same: the PR merges, the row stays IN-REVIEW, and the next run cannot tell
finished work from work in flight.

**S8's row still named the wrong cause.** SynthEditLib#49 merged with the real
finding — the list is not "in a source list belonging to a separate target", it
sits inside `IF(SE2JUCE)` at `CMakeLists.txt:582` and is never evaluated, and 79
`.cpp` files are in there, not one module — but I never wrote it back to the row.
The evidence lived only in a merged PR body, which is exactly the place the next
run does not look. Prepended now, with the `ar -t` control.

**Two dirty worktrees, both safe, both checked before discarding.** `sl35` held
the `TEMP-DIAG` fprintf pair from the datatype hunt — throwaway instrumentation
that was never meant to land. `wt7` held earlier drafts of the S7 row and
journal, superseded by #376; I confirmed main carries both the row text and the
journal entry (line 63) before throwing them away rather than assuming the merge
had covered it.

**Still open and NOT missed, just not mine to close:**
[#373](https://github.com/JeffMcClintock/TideSynth/issues/373) needs a Linux
build — the cause is fixed in SynthEditLib#47 and verified on macOS, but that
issue's own rule is to close only after building on the platform, and this box
cannot. #378 (V4/V5/V6) is awaiting review.

**One thing genuinely unfinished and worth naming:** V5 asks for the rack view to
be resized to match VCV, and I never researched VCV's actual default window or
row count. The row records TIDE's own numbers — 7968 DIPs, 20.75 rows by 166
units against E5's ruled 384/48 — and says the VCV figures are unconfirmed
rather than guessing them. The resize cannot be done until someone gets them.

## 2026-08-24 — macos — One RackModules folder, and the comment that sent Jeff's prefabs nowhere (interactive)

**Prompt:** ok, simple misunderstanding. I was putting my prefabs in "Tidesynth/TideModules" yours was in "Tidesynth/TideModules/prefabs". / lets just keep them as .synthedit for now / rename mine with _jef appened for now / OK, we're not regenerating them any more, they need hand-tweaking of the layout. don't want that overwritten

**It was not a misunderstanding on Jeff's side. The comment told him to do what
he did.** `SynthEditSem/CMakeLists.txt` said *"copy_directory_if_different so
adding a prefab to TideModules/ needs no CMake edit"* while the command beneath
it copied `TideModules/prefabs`. So his `AR`, `Output` and `Sine` — committed in
`f814c0b` — were never staged into the bundle, silently. Zero `.seprefab` and
zero of his `.synthedit` files were in it.

Now one flat folder, `RackModules/`, per Jeff's ruling: they are modules in the
EURORACK sense, not the SynthEdit DLL sense, and they are all prefabs so no
`prefabs/` subfolder earns its keep. Nine `.synthedit` files, verified in the
built bundle, no strays.

**Two collisions I stopped on rather than resolved.** Flattening put two
different `Output.synthedit` in one folder (generated 12277b vs Jeff's 13542b),
and all three of his existed as BOTH `.seprefab` and `.synthedit`. Either guess
would have silently dropped one of his rack modules. Jeff's calls: keep
`.synthedit`, and suffix his with `_jef` so both `Output`s survive for
comparison. The substantive difference, for whoever compares them: the generated
one uses `type="SE Patch Point in"`, his uses `type="TiDE Patch Point In"`.

**The last instruction changed what `build-prefabs.py` IS.** *"we're not
regenerating them any more, they need hand-tweaking of the layout. don't want
that overwritten."* Its docstring still called those files "BUILD OUTPUT, not
hand-written" — which is exactly the belief that would destroy a day of layout
work. Layout is the half the script recomputes from its own tables, so it cannot
preserve a tweak.

So it is now unable to clobber, and that is tested three ways rather than
asserted: `--outdir` has no default so a bare run fails in argparse; pointing it
at `RackModules` exits with a refusal; a throwaway `--outdir` gets past the guard
and on to its normal SynthEditCL check. The guard sits immediately after
`parse_args()` — my first attempt put it lower, where the SynthEditCL check
fired first and the guard never ran at all. It is kept, not deleted, because it
still documents how the graphs were built, which is written down nowhere else.

The `_jef` suffix has a useful side effect: the script writes six fixed
filenames, none of them suffixed, so it structurally cannot touch Jeff's files
even if the guard were removed.

**Left alone deliberately:** `TideModules` in DONE backlog rows and in
`docs/n1-tide-rack-rename.md`'s table. Those are records of what was true then,
and the backlog's Item column is prepend-only.

**Not verified:** Windows and Linux. `tests/s21_bundle_resources_probe.py` would
have been the natural check and cannot run here — it looks for
`Contents/x86_64-linux/TIDE-Rack.so`, so it is Linux-only by construction. The
macOS check was a direct listing of the bundle's `Resources/Prefabs`.

## 2026-08-24 — macos — The S7 skins ruling, and the gap Jeff closed (interactive)

**Prompt:** oh, if it works with no skin at all, that good. / I've been running the rendered UI. No obvious issue yet.

Follow-up to the three S7 PRs, all merged (SynthEditLib#48, SynthEdit#77, #375).
`main` builds and links on macOS at 318/318 with all three in.

**The gap I flagged was closed by Jeff, not by me.** I shipped S7 saying plainly
that I had not looked at TIDE's rendered UI — I could establish that nothing is
written to the user's home and that the code path is safe, but not that it still
looks right. Jeff then ran the UI and reported "no obvious issue yet". Recording
the division deliberately: naming the unverified part is what got it verified,
and it would have been easy to let "the standalone launched clean" stand in for
a look at the pixels.

**Two rulings arrived after #48 merged, and they change how the empty folder
reads.** *"TIDE does not support user-defined skins. The default Skin files
should load from TIDE's own private resource folder only."* And: *"skins have
much less relevance in tide, which has only one default skin. eventually it
might ship with no skin files, just hardcoded defaults."*

So TIDE's empty bundle skins folder is the intended destination, not a missing
asset. I had been one step from treating it as a gap and proposing to vendor
SE16's `default3` into TideSynth — 524K of assets duplicated across repos, for a
product heading toward no skin files at all. The question was worth more than
the work would have been. Recorded at the query itself in SynthEditLib#50, so
the next reader is not tempted to "fix" the empty folder.

**Mechanically it holds with zero files on disk**, which matters because #48
changed TIDE from an empty folder that EXISTED to a path that does not:
`ScanFiles()` uses the `error_code` overload of `directory_iterator`, which
yields nothing for a missing path rather than throwing, and always pushes the
`"global"` `SkinInfo` first, so `getSkin()` still returns a usable skin. I said
earlier it would return null — it does not.

**A near-miss on my own process.** I amended the merged S7 commit to add this
comment and tried to force-push. `--force-with-lease` refused: #48 had merged
while I was editing. Without the lease I would have rewritten a merged branch.
The follow-ups are ordinary PRs off current main instead.

**Not verified:** Windows and Linux.

## 2026-08-24 — macos — S7: TIDE was resetting SynthEdit's skin version (interactive)

**Prompt:** next

Took S7 — TIDE writing outside its container on launch, verified at runtime by
the linux box on 08-22. Fixed in SynthEditLib#48 + SynthEdit#77 + this pr, which
must land together.

**The defect is worse than the row records, and the difference is the machine.**
Linux measured folder CREATION inside an `LD_PRELOAD` shim over `getpwuid`, in a
home with no existing `.resource_version`. On a real machine that also runs
SynthEdit, one TIDE launch rewrote `~/SynthEdit Projects/.resource_version` from
SynthEdit's **192** to **0**. TIDE never defines `SE_APP_BUILD_NUMBER`, so
`se_version.h`'s fallback of 0 wins, `versionChanged` is true against any real
build, and the stamp is rewritten. SynthEdit then re-copies every skin on its
next launch. Two apps, one version file.

That only shows up on a machine with both installed — which is why the isolated
measurement, correct as far as it went, missed it.

**The row's own suggested shortcut does not work.** It offers "the TIDE-side
part may be enough on its own": point SkinMgr at the bundle from TIDE. It
cannot, because `SkinMgr`'s CONSTRUCTOR does the writing, on the first
`Instance()` call — anything TIDE does afterwards is too late. So EditorLib had
to change, which is the GATED part; interactive satisfies that.

Fixed with a per-app `AppUsesUserSkinsFolder()`, third instance of the pattern
after `GetLicenseState()` (C11) and `AppHasModuleEditorDialogs()` (S3g).

**Measured both directions against Jeff's REAL home folder**, because the row
warns `$HOME` does not protect it — `getUserDocumentFolder()` uses
`getpwuid(getuid())->pw_dir` and ignores the environment. I snapshotted 335
entries and backed up `.resource_version` first, ran the unfixed binary, saw
`192 -> 0`, restored immediately, then ran the fixed one:

| | `.resource_version` | folder |
|---|---|---|
| unfixed | 192 -> 0, rewritten | modified |
| fixed | 192, untouched | diff of 0 lines |

**A process note on myself.** Four times today a check passed because the thing
under test never ran — most recently here, where I "launched" a standalone whose
binary did not exist and read the resulting no-op as a clean pass. The tell was
an `ls` guard whose output I did not look at. Every launch/build check in this
entry now asserts the artifact exists first, and that is what caught it.

**Not verified:** Windows, Linux, and TIDE's rendered UI. The standalone came up
with no new error output versus the pre-fix run, and TIDE ships no
`Resources/skins`, so its skin folder was empty before and is a different empty
path now — but that is an argument, not a look at the pixels.

## 2026-08-24 — macos — main did not compile, and a docs-only PR is what proved it (interactive)

**Prompt:** merged. go!

`main` stopped building on macOS and Linux. Fixed in
[SynthEditLib#47](https://github.com/JeffMcClintock/SynthEditLib/pull/47).

    CContainer.h:18:7: error: expected identifier
    enum{ ID_EDIT_COPY = 0xe122, ID_EDIT_PASTE = 0xe125, ... }

P3 (`a4d536a`) added `StandardCommandIds.h`, which `#define`s those four ids on
every platform, and included it from `CContainer.cpp` BEFORE `CContainer.h`. The
macros then expand inside `CContainer.h`'s enum of the same names. Windows never
saw it: that enum is guarded `#ifndef _WIN32` because MFC supplies the ids there.
**The platform that could not see the clash is the one that introduced it.**
Same four values in both places, so the enum was redundant — replaced with the
include.

**How it was found is the part worth keeping.** The macOS job went red on
TideSynth#368, a docs-only PR whose branch was provably `main` plus two markdown
files. A code failure on a change containing no code is impossible, and that
impossibility is what said "the fault is in main, not in your PR". Taking the
red at face value would have meant debugging my own change for nothing and then
merging onto a broken main anyway.

Two gaps this exposes, both for Jeff rather than this box: #44 merged green while
breaking every non-Windows consumer, and #368 merged onto a red main.

**A33 now has a rate, not an anecdote.** Three issues in one day — #364, #372,
#373 — each naming a branch that `git ls-remote` returns 0 refs for, so the
mechanism that files them can never close them. #306 has been open since
2026-08-22 for the same reason. All were handled by hand today. The row was
filed as tidiness about a single stale issue; it is actually producing one open
`platform:*` issue — STEP 1 work, outranking every backlog row — per merged
branch that happened to be red. Deleting branches at merge, which is the right
practice and what the S3g PRs did, makes it more frequent rather than less.
`.github/workflows/**` is Jeff's path, so recording the evidence is all this box
can do.

Closed #372 on a macOS build of `main` at `1efd676`: TIDE links 318/318,
dsp_tests / SynthEditCL / EditorScreenshot build, suite 62/2. Left #373 open with
the diagnosis — the cause is platform-independent, but I have no Linux machine
and the issue's own instruction is to close only after building on the platform.

## 2026-08-24 — macos — Deleted the stranded S27 branch, after proving it held nothing (interactive)

**Prompt:** stranded branch

`origin/tide/mac/S27-render-ci` was the branch S44 was filed about. S44 recovered
the WORK into main; the branch itself stayed behind, and CI kept building it —
it was one of the two branches that failed to link after S3g landed, alongside
the linux branch that produced #364.

Deleting a branch is easy to get wrong, so I proved it superseded before
touching it rather than reasoning from "S44 landed, so it must be covered":

- of the 24 files its 3 commits touched, **20 are byte-identical to main**
- **0 files exist only on the branch**
- the 4 that differ do so because main is 36 commits ahead, not because the
  branch has newer content
- the only branch-unique content was **9 lines of `build.yml`** passing the
  reference set as a CI matrix (`refs: macos` / `refs: windows-linux`)

Those 9 lines are superseded by a better design, not merely duplicated: S44 made
the test pick its own set at compile time (`kReferenceSet`), so the workflow does
not need to pass one. The valuable part of them was the measured comment — why
two sets and not three — and I checked that it survives in main in three places
(`modules/common/README.md`, `RenderRegression.cpp`, `BACKLOG.md`) before
deleting. Tip archived locally at `refs/archive/S27-render-ci` (`218ff24`).

**The trap in one line, because it has now bitten twice:** #331 merged FROM this
branch, and then two more commits were pushed onto it. A merged PR does not stop
a branch accepting more work, and nothing then opens a PR for it. Deleting the
branch at merge is what prevents the next one — the S3g PRs all used
`--delete-branch`.

No TideSynth branch is now missing the S3g definition, so the link failure class
that produced #364 is closed on this repo.
## 2026-08-24 — macos — S18: the public repo now says which sdk it wants (interactive)

**Prompt:** sync repos. next task

No open platform issues, so a backlog row. Took S18 — self-contained, and its
Accept offers two options of which one avoids a licensing decision entirely.

`modules/SoundPipe` is the only place in the public repo needing a third-party
sdk that lives outside it. `external_sdk_folder` is set ONLY by SE16
(`SynthEdit/CMakeLists.txt:288`), pointed at SE16's own `SDKs/`, and
`SynthEditLib/modules` is added by SE16's root and never by SynthEditLib's own.

**Reproduced the stranger's experience rather than describing it.** With the
variable unset the configure dies with

    Cannot find source file: /Soundpipe/modules/base.c
    No SOURCES given to target: Soundpipe

A path that exists nowhere, naming no sdk and offering no remedy. That is the
actual defect — not that the dependency exists, but that hitting it teaches you
nothing.

A detour worth recording: my first attempt to reproduce passed `-Dexternal_sdk_folder=`
on the command line and configured cleanly, which looked like the row was stale.
It was not — line 288 is a plain `set()`, not `set(... CACHE ...)`, so it
overwrites anything passed with `-D`. Had I stopped there I would have closed a
live row as fixed. Emptying the `set()` in a throwaway worktree reproduced it
immediately.

Guarded and explained. Measured both directions, because the second is the
regression check that matters:

| `external_sdk_folder` | before | after |
|---|---|---|
| unset | configure fails | rc=0, prints which sdk and where to point it |
| set by SE16 | `Soundpipe.sem` builds | builds, skip message absent |

**Deliberately not done:** vendoring or fetching Soundpipe publicly, the other
option S18 offers. Soundpipe is third-party, so that is a licensing question
before it is a build question, and the row keeps that half.

**Not verified:** Windows and Linux. The guard is an `EXISTS` test on a header
path, so nothing platform-specific, but neither was built.

## 2026-08-24 — macos — S35 re-landed, this time against a bar it can fail (interactive)

**Prompt:** go

The user-domain scan is back ([SynthEditLib#45](https://github.com/JeffMcClintock/SynthEditLib/pull/45)),
after the two things that actually broke it were fixed: #40 stopped a `.gmpi`
being read for any xml in its Resources, #41 stopped the factory modules
installing copies of themselves into the user domain.

**The part worth keeping is the third condition.** The revert recorded two
requirements, and I had only satisfied one by fixing the crash. The other was
*"it must not defeat `-factorysemsfolder`, which exists to make a test run
deterministic."* Re-landing the scan unchanged would have left a test run
picking up whatever the developer has installed — which is precisely how the
original took `dsp_tests` from 2 failures to 43.

So the scan is skipped when the sem folder was overridden. That flag was already
there and already means this: `SemCacheName()` namespaces the module cache on it
so a `-factorysemsfolder` sweep cannot clobber the installed app's cache
(`ModuleFactory_Editor.cpp:188`). Same signal, same reason, no new concept.

Four measurements, because any one alone would have been the original's mistake:

| | result |
|---|---|
| `-factorysemsfolder` given | no user scan, 0 duplicates, exit 0 |
| no override | user domain scanned, TIDE-Rack visible, exit 0 |
| `dsp_tests` | 62 passed, 2 failed — the baseline |
| duplicates, normal run | 2, both `SE SynthEdit` from the TIDE bundles |

**A near-miss worth recording.** The first run of the acceptance test reported
"user-domain scan lines: 0, FOUND TWICE: 0" and looked like a pass. It was
vacuous — the build had failed and the binary did not exist, so grep found
nothing in an empty log. Exit 127 was the only sign. The cause was unrelated (my
SE16 worktree was pinned before SynthEdit#75, so `AppHasModuleEditorDialogs()`
was undefined), but the shape is the same one that bit me on the
`IS_OFFICIAL_MODULE` flags earlier today: **a check that counts absences passes
when the thing under test never ran.** Assert the binary exists before believing
a zero.

**Deliberately not answered:** precedence when a user copy shadows a factory
module. Pre-existing, applies to the system domain too, and post-#40 a duplicate
is a warning rather than a crash. It deserves its own ruling, not a rider.

**Not verified:** Windows and Linux. `getUserPluginsFolder()` returns empty on
both so neither gains a scan, but neither was built.

## 2026-08-24 — windows — P3: the MFC requirement is gone, and a no-MFC toolchain is the proof (interactive, Jeff directing)

**Did:** took **P3** — this platform's only own-boxed row, GATED because both its
files are in `SynthEditLib`, and takeable because this is interactive with Jeff
directing. STEP 1 clear (no open `platform:win` issues), STEP 1.5 clear.

### The row's paths were stale for the third time, and this time in the useful direction

P3 said `MfcDocPresenter.cpp:4` was *"still `SE16/SynthEdit2/`"* and that the
second half was *"best done before or with C4"*. **C4 has happened.** Both files
are now `SynthEditLib/EditorLib/`, so there is no private-repo half and the row
is a single change in one public repo.

### What was actually needed, measured rather than guessed

The row said the include was *"likely only `ID_*`/`IDR_*` resource constants"*.
That was right, and the set is smaller than it sounds. Of the **twelve** `ID*`
symbols the two files reference, `afxres.h` supplies exactly **four**:

| symbol | comes from |
|---|---|
| `ID_EDIT_COPY`, `ID_EDIT_CUT`, `ID_EDIT_PASTE`, `ID_EDIT_SELECT_ALL` | **`afxres.h`** — the whole dependency |
| `IDOK`, `IDYES` | `<winuser.h>` |
| `ID_EDIT_CONTAIN`, `ID_EDIT_UNCONTAIN`, `ID_EDIT_MOVEBACK`, `ID_EDIT_MOVEFRONT`, `ID_EDIT_DELETE`, `ID_INS_PREFAB` | the application's own, already in `resource.h` |

Established by grepping the two files for `ID*` and testing each name against
`atlmfc/include/afxres.h` — four hits, eight misses — rather than by reading
the include and assuming.

New `EditorLib/StandardCommandIds.h` carries the four at MFC's exact values,
`#ifndef`-guarded so a TU that also sees `afxres.h` keeps MFC's definitions.

**Not `resource.h`, deliberately.** That header has two copies policed by
`check-resource-h-drift.py`, one of them in the private repo, and it is
Visual-Studio-managed — hand-written content there is what the resource editor
clobbers.

### The values are the whole risk, so they are asserted

`.rc` files elsewhere in the tree compile **with** `afxres.h`. A value that
disagreed with MFC's **would not fail to build** — it would silently wire a menu
item to nothing.

A translation unit including both headers compiles clean at `/W4` with four
`static_assert`s. **Negative-controlled**, because an assertion nobody has seen
fail is indistinguishable from one that cannot:

    correct                       CL_EXIT=0
    0xE122 changed to 0xE121      error C2338: static assertion failed:
                                  'ID_EDIT_COPY diverged from MFC'    CL_EXIT=2

Zero `C4005` at `/W4` with both headers in scope is the other half — that is
what shows the guards work rather than merely look right.

### Verified, single-variable

Same TIDE source at `origin/main`, same toolchain, only `SynthEditLib` differing:

| SynthEditLib | toolchain | result |
|---|---|---|
| **unmodified** | Build Tools (**no MFC**) | **rc=1** — 2 errors, both `afxres.h`, nothing else |
| **P3 branch** | Build Tools (**no MFC**) | **rc=0** — 0 errors, 0 warnings |
| **P3 branch** | Community (MFC present) | **rc=0** — 0 errors, 0 warnings, **0 `C4005`** |
| **P3 branch** | `SynthEditCL`, Community | **rc=0**, exe produced |

All four TIDE formats — `.gmpi`, `.vst3`, `.clap`, standalone — built in each
passing case. **The control carries the argument.** Without it, "it builds" is
compatible with the toolchain having had MFC all along.

**Not verified:**

- **`SynthEdit2`**, the MFC app. Its `.vcxproj` links out of the developer's
  Visual Studio tree, which must not be built into — the standing limit recorded
  for every SE16 verification on this box. It is the one consumer that still
  includes `afxres.h` itself, which is exactly the case the guards and the
  `static_assert` exercise, but nobody compiled it.
- **macOS and Linux** compile neither file's Windows branch, so neither is
  affected and neither was run.

**Learned:**

- **A row's own guess can be right and still worth measuring, because the SIZE
  is the actionable part.** "Likely only resource constants" was correct, but
  four-of-twelve is what made this an afternoon instead of a refactor. Testing
  each symbol against `afxres.h` is one loop.
- **A value that must match an external header is not a code change, it is a
  data change, and it fails silently.** Nothing would have broken at build time
  if `ID_EDIT_PASTE` were off by one — a menu item would just stop working. The
  `static_assert` plus its negative control is the cheapest thing that turns
  that into a compile error.
- **Guard macros need the both-headers case actually compiled.** `#ifndef` looks
  obviously correct and is exactly the kind of thing that is subtly wrong; `/W4`
  with `afxres.h` and this header in one TU, reading `C4005` count, is the test.
- **A build "hanging" for ten minutes was `| grep … | head -5`.** `head` closes
  the pipe, the writer blocks, and the task looks stuck while nothing is wrong.
  Redirect to a log and read the log; do not pipe a long build through `head`.
- **Check the row's file paths before believing its plan.** P3's staging advice
  ("before or with C4") was built on a path that had already moved. Three
  documents this week have named a pre-carve-out path.

### Found on the way: the two local checkouts are skewed, and it looks like a code bug

Mid-verification the link failed with:

    EditorLib.lib(MfcDocPresenter.obj) : error LNK2019: unresolved external symbol
      "bool __cdecl AppHasModuleEditorDialogs(void)"

**Nothing to do with P3.** `C:\SE\TideSynth` was **3 commits behind**
`origin/main` and did not define that symbol, while `SynthEditLib` at
`origin/main` calls it — it arrived with **S3g** ([#363](https://github.com/JeffMcClintock/TideSynth/pull/363)
+ [SynthEditLib#43](https://github.com/JeffMcClintock/SynthEditLib/pull/43))
under an hour earlier.

**This is S41's shape with local clones instead of CI runners**, and it is worth
recording because the failure mode is inverted: the developer's build had been
*succeeding* only because both checkouts were old **together** — `SynthEditLib` 6
behind, `TideSynth` 3 behind. Pulling one without the other is what breaks it.
Re-ran every measurement above against `origin/main` worktrees of both repos, so
none of the results carry the skew.

**Machine left clean.** All work in throwaway worktrees and build trees under
`C:\SE\_scratch`; nothing built into `C:\SE\SE16\build` or any repo working
tree. `C:\SE\TideSynth` still carries the developer's own uncommitted
`modules/PatchPoint/` and `tools/tidepanel-screenshot.synthedit`, untouched.
All repos on their default branches.

**Branch/PR:** `tide/win/P3-remove-mfc` — [SynthEditLib#44](https://github.com/JeffMcClintock/SynthEditLib/pull/44)
is the change; this repo carries the row and this entry.

## 2026-08-24 — linux — SynthEditJuce ruled unmaintained, and #88 closes WONTFIX (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** recorded Jeff's ruling — *"document that SynthEditJUCE is not to be
maintained. It's superseded by gmpi_ui based targets."* — and closed the last
open `platform:linux` issue on the strength of it. Docs only, in two repos.

### The wording that was already there, and why it was not enough

The tree said *"deprecated and no longer built"* in the root `CMakeLists.txt` and
in `WAYLAND.md`. Both true. But `SynthEditJuce/CMakeLists.txt` also said:

> NOTE this target is deprecated and not reachable from the root CMakeLists;
> **the entry keeps it honest if it is ever revived.**

That is an invitation to maintain it, sitting in the exact file someone would
open to do so. **"Deprecated" describes the build; "unmaintained" describes the
obligation, and only the second one answers "should I fix this?"**

So the ruling is written as an operational rule rather than a status: the sources
stay for reference, are **not expected to compile or link**, nobody repairs them
to match a refactor in shared code, and **build breaks are not filed against
them**. A carve-out stage that moves a file out of `EditorLib` owes
`SynthEditJuce` nothing.

### This closes #88, and closing it is the point

[#88](https://github.com/JeffMcClintock/TideSynth/issues/88) reported that **C12e**
updated two of the four apps that must compile `Dialogs_editor2.cpp` and left
this one out. Half was fixed on 2026-08-18 (`SynthEditWayland`, measured). The
`SynthEditJuce` half then sat open for six days as a one-line change that was
GATED, that no box could verify because nothing builds the target, and that
**nobody should have made**. Under this ruling it is the expected state.

**That is the shape worth noticing: an issue can stay open not because the work
is hard but because nobody has said it should not be done.**

### Recorded in four places, each with a different reader

| where | who reads it |
|---|---|
| `SE16/CMakeLists.txt`, where the `add_subdirectory` used to be | someone wondering why the target is absent |
| `SE16/SynthEditJuce/CMakeLists.txt` | someone editing that source list — **the person who would have "fixed" #88** |
| `SE16/WAYLAND.md` | someone reading how the Linux editor came to be |
| `docs/decisions.md` | a later run looking for the ruling rather than re-deriving it |

**Verified:** SE16 root configure **rc=0** after the edits (docs-only, but the
two edited files are CMake and a comment block can land inside the wrong
`if()` — that lesson is already in this journal); `check-links` clean; all three
SE16 files are LF and stayed LF, checked before editing rather than after.

**Not verified:** nothing was built beyond configure, deliberately — no target,
source list or flag changed.

**Learned:**

- **"Deprecated" and "unmaintained" answer different questions**, and a tree can
  say the first clearly while leaving the second open. The first is about the
  build; the second is about whether you owe it work.
- **A comment that hedges toward revival will be read as a to-do.** *"Keeps it
  honest if it is ever revived"* is why #88 looked actionable for six days.
- **Some issues close by ruling, not by code**, and the cheapest fix for a row
  that nobody may act on is a sentence from the person who can say so.
- **Check line endings before editing, not after** — third CRLF-adjacent lesson
  in this project, and this time it cost nothing because I looked first.

**Machine left clean.** Two scratch worktrees plus four read-only dependency
worktrees, all removed; nothing built into any of Jeff's trees; all six repos on
their default branches and clean.

**Branch/PR:** `tide/linux/juce-unmaintained` in both repos —
[SynthEdit#76](https://github.com/JeffMcClintock/SynthEdit/pull/76) carries the
three in-tree notes, TideSynth carries the decision entry and this journal entry.
**Merging either alone is harmless**; neither changes any build.

## 2026-08-24 — linux — S45 closed on its own Accept, and a 50-minute window where `main` did not link (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** flipped **S46** DONE on merged PRs, then closed **S45** by running its
own Accept against a clean `main` build. No product code changed.

### Why re-measure when S46 already showed the captions rendering

Because that measurement was taken on the fix **branch**, and "the branch fixes
it" is a different claim from "the product is fixed". Every dependency here is at
`origin/main` with no override pointing at a branch: TideSynth `9bb11d9`,
SynthEditLib `c6fc321`, gmpi_ui `463b044`, GMPI `d94c972`.

**Accept met:** captions read **MIDI-CV / PITCH / GATE / VEL / TRIG**, and the
run emits zero `missing from bundle resources` / `no Prefabs` lines.

### An objective metric instead of an eyeball

"The glyphs look right" is exactly the kind of claim this project keeps having to
retract, so the closure does not rest on it. Tofu strings are **~4x wider** than
their text — three NUL codepoints per character — and spill far past the
faceplate. So the band to the *right* of the panel is a tofu detector. Counting
pixels differing from the flat background there, same crop, same 3x zoom:

| build | pixels differing from background |
|---|---:|
| clean `main` | **0** |
| the verified fix branch | **0** |
| known-broken build | **6,399** |

The broken leg is the positive control. Without it, "0" is just an empty region.

### The 50-minute window where `main` genuinely did not link

The first build of this item **failed**:

```
EditorLib/libEditorLib.a(MfcDocPresenter.cpp.o):
  undefined reference to `AppHasModuleEditorDialogs()'
```

**S3g is a three-repo change whose own PR body says the parts "MUST LAND
TOGETHER"** — SynthEditLib declares and calls the symbol, TideSynth and SynthEdit
each define it, and any one alone is an undefined symbol *by design*, so a new
app gets a link error rather than a silent default. That is a good design.

What happened is the landing order: **SynthEditLib#43 merged at 02:25**, and
**TideSynth#363 merged ~50 minutes later**. I branched from `main` inside that
window, so `main` + `main` did not link. By the time I diagnosed it, #363 had
merged and a rebuild was clean — 0 errors, 0 undefined references.

**No platform issue filed, deliberately: there is nothing open to fix.** But it
is worth writing down, because:

- **CI could not have caught it.** TideSynth's last `main` run was `9bb11d9` at
  02:37, and CI only runs on a push to *this* repo — a merge in `SynthEditLib`
  changes what TideSynth's `main` builds against without producing any TideSynth
  event at all. **A cross-repo break is invisible to a per-repo CI by
  construction.**
- **The mitigation is landing order**, not more testing: merge the *definition*
  side before the *caller* side, and the window is zero-width instead of
  50 minutes.

**Verified:** clean `main` build 0 errors, 0 undefined references; Accept run as
written; the three-way pixel metric above with its positive control; zero
plug-in diagnostics.

**Not verified:**

- **Windows and macOS.** Per S46 these should ALSO have been affected — the axis
  is statically-linked vs file-scanned, not platform — and nobody has looked at
  either since the fix landed. **That is the open question this chain leaves.**
- **No real DAW.** TIDE standalone under headless weston, as ever on this box.

**Learned:**

- **"The branch fixes it" and "the product is fixed" are different claims**, and
  the second one costs one more build. Closing a row on the first is how a fix
  that never actually merged gets recorded as done.
- **Pick a metric the bug makes big.** The tofu's own signature — 4x width — is
  far easier to measure than glyph shapes, and it turns a judgement into a
  count with a positive control.
- **A cross-repo "must land together" is a real hazard even when everyone does it
  right.** Both PRs were correct, both were reviewed, both merged; the only
  defect was 50 minutes of ordering, and no CI in this fleet can see it.
- **A link error naming a symbol you have never heard of is usually someone
  else's half-landed change** — check the other repos' recent merges before
  assuming your own tree.

**Machine left clean.** Weston stopped, standalone stopped, scratch `HOME`s
throughout; nothing installed and Jeff's config untouched. All six repos on their
default branches and clean.

**Branch/PR:** `tide/linux/S45-close-on-accept` — TideSynth only: the S45 and S46
rows, the linux NEXT cell, one screenshot and this entry. **No product code
change in any repo.**

## 2026-08-24 — macos — S3g: two of the row's three clauses were wrong about the code (interactive)

**Prompt:** linux agent is working on S46. take the next task

S3g wanted three menu entries gone from TIDE's module context menu. Reading the
code before writing any changed two of them, and Jeff ruled on each.

**(a) "Connect..." is dead in every app, not just TIDE.** The row framed it as a
capability difference — the desktop app has the dialog, TIDE doesn't. It has
not: `dlg_connect_ug` exists nowhere in any repo, and every `doDialogConnectUg`,
the WinUI3 one included, is an empty stub with its body commented out under an
"all obsolete?" header. Gating it behind a capability query would have dressed
dead code up as a feature. Jeff: a low-priority SynthEdit 1.5 feature that may
eventually be implemented in SynthEdit, not TIDE — so the chain is KEPT and only
the menu entry is commented out. Not gated, because gating implies some app has
it.

**(b) "Build Code Skeleton..." is the one real capability.**
`OM_SHOW_CODE_SKELETON_DIALOG` has exactly one handler, the WinUI3 app's
`MainWindow.xaml.cpp`. That one gets `AppHasModuleEditorDialogs()` — a runtime
query, not an `#ifdef`, because EditorLib compiles once for every app. Same
shape and same reason as `GetLicenseState()` (C11), and defined once per app so
a new app gets a link error rather than silently inheriting a default.

**(c) The patch manager has no module-context-menu entry at all.** The row cites
`CUG_with_patches.cpp:33` and `:142`, but those are command HANDLERS.
`ID_PATCH_MGR` is a Windows menu resource and `HC_CopyPatch` arrives from a
module's own GUI, so nothing in that menu reaches it. The row was mistaken about
where it lives. Untouched, and its loud stub stays — Jeff expects "copy patch"
may become useful in TIDE later.

**Verified:** TIDE VST3 builds and links against current main;
`dsp_tests` / `SynthEditCL` / `EditorScreenshot` all build and the suite is
unchanged at its 62/2 baseline. The negative control is the one that matters:
deleting TIDE's definition produces
`Undefined symbols: AppHasModuleEditorDialogs()`, so the "a new app gets a link
error" claim is tested rather than asserted.

**Not verified, and worth being blunt about: I never opened the context menu and
looked.** The guard is a runtime branch; I checked the mechanism, not the
pixels. And the WinUI3 desktop app — the only place the query returns true — is
unbuildable on this Mac, so that branch is unexercised. Windows and Linux
untested.

This work was built earlier in the session and sat in worktrees while the
datatype investigation ran. Landing it now rather than leaving it stranded,
which is the end state S44 was filed about.

## 2026-08-24 — linux — S46: the technology is recorded now, not sniffed off a filename (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** fixed **S46**, the row the mac box filed after correctly rejecting my
S45 attempt. Product change is
[SynthEditLib#42](https://github.com/JeffMcClintock/SynthEditLib/pull/42).

**Jeff named the mechanism before it was measured:** *"we differentiate the
meaning of 'string' datatype, on SDK it's wstring, on gmpi it's plain string. I
think we do this from the file extension, which might not be available when
module is statically linked?"* Right on both halves.

### What the code says about itself

```cpp
// dodgy, only works when plugin binary file available. OK, since we only need
// this when scanning binary for XML (to determine native string type)
const bool isGMPI = GetExtension(Filename()) == L"gmpi" ...;
```

That is `Module_Info3::ModuleTechnology()`. But **a statically-registered module
never reaches that override**: it is a `Module_Info3_internal`, which does not
override `ModuleTechnology()` at all, so it inherited

```cpp
int Module_Info3_base::ModuleTechnology() { return MT_SDK3; }   // unconditional
```

So it did not fall back to a filename test that fails — it never had one.
`"string"` → `DT_TEXT` → wide → S45's three-NULs-per-character captions.

**Confirmed on this box:** TIDE opens **zero** `.gmpi` files and touches **no**
`PlugIns` folder. Everything is statically registered, per constraint 7.

### The correction that matters more than the fix

**S46's title says "typed differently on linux and macOS". It is not a platform
difference — it is statically-linked vs file-scanned.** macOS looked right
because that box has a scannable `Controls.gmpi` on disk for the extension test
to find. The **shipped** product compiles its modules in, so this should
reproduce everywhere, macOS included. The mac measurement was of a dev box, not
of a shipping build.

Two related facts, so nobody repeats the search:

- **This box's `Plugin-Cache-16*.xml` records `SE Label` as `string_utf8` — the
  same as macOS.** The row nominated dumping that cache as the first
  measurement; it does not differ, and TIDE never reads it anyway.
- **`LabelGui` needed no change.** Its `Pin<std::string>` was always correct,
  exactly as the row said. My [#39](https://github.com/JeffMcClintock/SynthEditLib/pull/39)
  fixed the symptom at the module and would have mirrored the corruption onto
  macOS; closing it was right.

### The fix, and the part Jeff's second point shaped

`Module_Info3_internal` **records** the technology; `ModuleTechnology()` reports
the record. `gmpi::RegisterPlugin` and `gmpi::RegisterPluginWithXml` stamp
`MT_GMPI` — GMPI's `Register<>::withId` **and** `::withXml` both route through
those two, so static registration is covered either way — and every other path
keeps the historical `MT_SDK3` default.

**The stamp is on the OBJECT rather than in any parse path**, and that is
entirely because of Jeff's follow-up: *"there is an alternative registration path
when the XML is read from the module bundle resources folder... uncommon with
.GMPI modules, but the possibility needs to be taken into account."* XML can
arrive long after registration — TideApp's own enrichment loop does exactly this
with `ControlsXp.xml` and friends — and that scan asks `ModuleTechnology()`. A
fix that decided the answer while parsing the *registration* XML would have been
correct for `withXml` and wrong for `withId`-plus-resources. Stamped before every
`ScanXml` instead.

`setModuleTechnology` is **highest-wins** so registration *order* cannot matter
for a module registering halves through different SDKs. No such module exists
today — I checked rather than assumed — but a nondeterministic answer is the one
outcome this field must never produce.

Also unified the odd site out: `Module_Info3_base.cpp` had **three** places doing
the same `DT_TEXT` dance under the same comment, and one sniffed the extension
inline instead of asking `ModuleTechnology()`.

### Verified — A/B with only `SynthEditLib` differing

| | result |
|---|---|
| `origin/main` | captions are `.notdef` boxes |
| this branch | **MIDI-CV / PITCH / GATE / VEL / TRIG** in real glyphs |
| module browser, same two runs | **byte-identical**, diff bbox `None` |
| resources-XML enrichment | identical counts — SDK3 modules correctly KEEP wide `"string"` |
| `SynthEditCL` | **312/312 rc=0**, runs, exits cleanly on a bad verb, no segfault |

The byte-identical browser is the control that matters: it says the change
touched the captions and nothing else in the frame.

**Not verified:**

- **Windows and macOS were not built.** Platform-neutral C++, no OS branches
  touched, but neither was compiled — and per the correction above, **macOS is
  the interesting one**, because it should now be reproducible there too.
- **No real DAW** — TIDE standalone under headless weston.
- **A mixed-SDK module** does not exist to test highest-wins against; that rule
  is reasoning plus a comment, not a measurement.

**Learned:**

- **When two guards both need the same missing input, the second one is not a
  fallback.** The extension sniff and the base default look like belt and
  braces; both are defeated by "there is no file", and the base's answer was not
  even a guess, it was a constant.
- **"Which platform" was the wrong axis, and the row's title encoded it.** The
  real axis was static-vs-scanned, and a dev box with a stray `.gmpi` on disk is
  exactly the thing that makes a shipping-build bug look platform-specific.
- **Ask where the data can arrive from before choosing where to fix.** Jeff's
  resources-folder point moved the fix from a parse path onto the object; the
  parse-path version would have passed every test I had and still been wrong for
  `withId` modules enriched later.
- **A comment admitting "dodgy" is a filed bug nobody filed.** It named its own
  precondition and the precondition is false for every statically-linked module.
- **The CRLF trap caught me again**, third time in this project's history: a
  Python rewrite of four CRLF files produced a 2,700-line phantom diff. `newline=''`
  on both read and write; check `git diff --stat` before believing an edit.

**Machine left clean.** Weston stopped, standalone stopped, scratch `HOME`s
throughout. **`SynthEditCL`'s rescan was pointed at a scratch `XDG_DATA_HOME`** —
Jeff's real `Plugin-Cache-16.xml` is still dated Aug 19, checked after. All six
repos on their default branches and clean.

**Branch/PR:** `tide/linux/S46-record-technology` in both repos — TideSynth
carries the row, two screenshots and this entry; **merging it alone changes no
behaviour**. The product change is
[SynthEditLib#42](https://github.com/JeffMcClintock/SynthEditLib/pull/42).

## 2026-08-24 — macos — Bookkeeping after the datatype work: S35 re-opened, S45 blocked (interactive)

**Prompt:** sync repos. where are we at?

Four PRs merged (SynthEditLib#38, #40, #41, GMPI#15) and #39 closed, which left
two rows describing a product that no longer matches them.

**S35 was DONE with its feature withdrawn.** #38 reverted the user-domain scan,
so the row asserted `Application.cpp` scans a folder it no longer scans. Anyone
reading it would believe locally built modules are visible. They are not. Back
to TODO, with the reason recorded: the scan was never the wrong idea — VST3 and
AU both search that domain — it was that the folder held stale duplicates of
factory modules and the scanner mis-read SDK3 xml out of a `.gmpi`'s Resources.
Both are now fixed (#40, #41), so it can be re-landed.

**S45 was IN-REVIEW citing a CLOSED pr.** Nothing shipped; the captions are
still wrong on linux. `BLOCKED(S46)` is the honest status — the fix belongs in
how the pin is typed, not in `LabelGui`.

**What the day's work actually established,** because it is one cause with four
faces and the next one will look unrelated again: `"string"` names the WIDE type
in SDK3 xml and the UTF-8 type in GMPI xml, the numeric enums agree, and nothing
arbitrates. That produced a host crash (#40), a shadowing mess (#41), a
migration trap (GMPI#15), and a platform-split still open (S46).

**The measurement that mattered most was Jeff's hint, not mine.** I verified the
`IS_OFFICIAL_MODULE` flags with 0 installs before and 0 after and called it
proof. It was vacuous: `SE_LOCAL_BUILD` defaults FALSE
(`SynthEdit/CMakeLists.txt:22`), so the copy step never runs in a plain build.
With it TRUE the control installs exactly the 7 bundles found on disk, and the
flagged tree installs 0. **Anything about developer-install behaviour has to be
tested with `SE_LOCAL_BUILD=TRUE`; a plain build and CI cannot see it.** That
also corrected my claim that the stale copies came from an external checkout —
they came from this build, with that flag on.

Deleted the 7 stale bundles from `~/Library/Audio/Plug-Ins/GMPI` at Jeff's
direction, backed up first. `Module FOUND TWICE` 43 -> 2, SynthEditCL rc=0. Kept
TIDE.gmpi and TIDE-Rack.gmpi: they are TIDE itself, and their 2 remaining
collisions (`SE Blob Size`, `SE Gamma Test`) come from TIDE statically bundling
factory modules — a different question that deletion would not fix.

**Not verified:** everything macOS. Windows and Linux untouched by any of it.

## 2026-08-24 — macos — One token, two vocabularies: the crash, and what S45 really found (interactive)

**Prompt:** so the one remaining issue is Linux getting the datatype wrong? Shall we give it a task?

Chasing why S35 segfaulted SynthEditCL ended in a root cause that explains three
separate symptoms across two machines. Writing it down properly because the next
instance will look like a fourth unrelated bug.

**`"string"` means opposite things in the two SDKs.** SDK3 xml (`conversion.cpp`)
maps `"string"` to `DT_TEXT` — the WIDE type — and `"string_utf8"` to
`DT_STRING_UTF8`. GMPI xml (`GMPI/Hosting/xml_spec_reader.cpp`) maps `"string"`
to `PinDatatype::String`, which is utf-8, and has no entry for `"string_utf8"`
at all. The numeric enums agree (1 and 12 in both); only the words disagree.
Nothing arbitrates, so whichever table happens to read a file decides.

**Symptom 1 — the crash.** `moduleXmlIn()` returned whichever `.xml` the
directory iterator yielded first from a bundle's Resources. TIDE keeps its
child modules' xml there (`Converters.xml`, `VaFilters.xml`, ...), which are
SDK3 files sitting inside a `.gmpi`, so they were read with the GMPI table and
every `std::wstring` pin was recorded as utf-8. `SE TextToText8` arrived as
Text8->Text8, converted nothing, and `ug_base::connect` inserted converter after
converter until the stack died. Fixed in SynthEditLib#40: a `.gmpi` descriptor
must be `plugin.gmpi.xml` or `<bundle-name>.xml`; `.sem` untouched. Of the 64
bundles installed here, 62 already obey that and the only 2 that don't are the
two at fault.

**Symptom 2 — silent misses.** An unrecognised datatype token doesn't error:
`lookup()` returns an optional and the guarded assignment is skipped, leaving
`pinInfo::datatype` — which has no default initialiser — indeterminate. GMPI#15
accepts `"string_utf8"` as a synonym at Jeff's direction, which covers the
common port; the silent-miss path itself is still there.

**Symptom 3 — S45's captions, and where that fix pointed.** The linux box traced
tofu captions to raw `wchar_t` on the wire and fixed it by moving `LabelGui` to
`Pin<std::wstring>`. The investigation was genuinely good — it measured the
codepoints (28 for 7 characters, 3 NULs each) and eliminated fonts with a
control. But `SE Label` is a GMPI module (`graphicsApi="GmpiUi"`, GMPI
`Register<>::withXml`), so its `datatype="string"` means utf-8 and
`Pin<std::string>` was already right. Jeff checked the module independently and
agrees it is self-consistent. On macOS the cache records that pin as
`string_utf8` and captions render fine, so the change would have corrupted macOS
the mirror way. SynthEditLib#39 is closed; `main` still has `Pin<std::string>`.

**What that leaves is the real defect, now S46:** the same GMPI module is
registered as DT_STRING_UTF8 on macOS and DT_TEXT on linux. Both platforms take
the same path — `ViewBase.cpp:792` uses `pinInfo.GetDatatype()`, the registered
type, not a re-parse — so the divergence is in registration, not in reading a
default. I checked that specifically rather than assume it, because S45's stack
made a re-parse look plausible.

The first measurement is one line: linux's cache record for `id="SE Label"`, its
`datatype` and the `file=` it came from. If it says `string` where macOS says
`string_utf8`, that is the bug located.

**A correction I had to make mid-investigation.** I claimed the duplicates were
irrelevant after an isolated-`HOME` run still failed. That control was worthless:
a fake `HOME` independently breaks SynthEditCL (exit 134 on pre-S35 too). The
comparison that held up was real-`HOME`, where pre-S35 exits 0 and S35 segfaults.

**Not verified:** everything here is macOS. The linux half is S45's measurement,
which I have not reproduced, and Windows is untouched by either.

## 2026-08-24 — linux — S45: the captions were UTF-32 in a UTF-8 string, and S23 closed (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** diagnosed and fixed **S45** — the rack-caption tofu split out of S25
this morning. Product change is
[SynthEditLib#39](https://github.com/JeffMcClintock/SynthEditLib/pull/39).
Also closed **S23** at Jeff's instruction (*"close S23"*).

**It was never a font problem.** It is a datatype mismatch, and the fix is two
pin declarations.

### The chain, from a backtrace rather than from reading

Four eliminations were already on the row and all of them held — which is what
made the remaining space small enough to instrument:

```
MfcDocPresenter::RefreshView   MfcDocPresenter.cpp:483
ViewBase::Refresh              ViewBase.cpp:2103
ViewBase::ConnectModules       ViewBase.cpp:679     <- setting GUI pin defaults
PluginEditorBase::setPin       GmpiPluginEditor.h:106
Pin<std::string>::setFromHost  GmpiPluginEditor.h:49
LabelGui                       LabelGui.cpp:136
```

`LabelGui` declares `<Pin name="Text" datatype="string"/>`. `conversion.cpp:996`
maps **`"string"` → `DT_TEXT`**, and `RawConversions.cpp:238` encodes `DT_TEXT`
as **raw `wchar_t` bytes**:

```cpp
result.resize( sizeof(wchar_t) * s.size() );
memcpy( &result[0], s.data(), result.size() );
```

The C++ pin was `Pin<std::string>`, and `valueFromData<std::string>` is a
**verbatim byte copy**. So `"MIDI-CV"` arrived as `'M',0,0,0,'I',0,0,0,…` and
every NUL drew `.notdef`.

### The measurement that made it undeniable

Logging the shaped codepoints inside the text engine:

| caption | codepoints | real chars | NULs |
|---|---:|---:|---:|
| `MIDI-CV` | 28 | 7 | 21 |
| `PITCH` | 20 | 5 | 15 |
| `GATE` | 16 | 4 | 12 |
| `TRIG` | 16 | 4 | 12 |
| `VEL` | 12 | 3 | 9 |

**Exactly three NULs after every character** — the signature of a 4-byte
`wchar_t` string read as bytes.

### Why the notdef probe said zero while boxes were on screen

My first probe logged only the `else if (fallbackFor)` branch, which fires when
the primary face does not cover a codepoint *and* a fallback is available. Here
`fallbackFor` was null, so control took the final `else` and the primary drew
`.notdef` silently. **A probe on one branch of a four-branch decision reported
"no misses" while the screen was full of them** — and I nearly read that as
evidence the text engine was innocent, when it only meant my instrument was in
the wrong arm.

### The fix, and the decision I deliberately did not take

Declare the pins `Pin<std::wstring>` — matching what the host actually sends,
since `valueFromData<std::wstring>` already divides by `sizeof(wchar_t)` — and
convert to UTF-8 once, at the three points that hand text to the drawing API.

**The wire format is unchanged on purpose.** `datatype="string_utf8"` exists in
the same table and would also work, and is arguably tidier. It also changes the
datatype of a pin existing patches may connect to, and this codebase
auto-inserts converters on mixed-datatype connections — v0.1's own findings
record a silent-drop bug of exactly that shape. That is Jeff's call, stated on
the PR rather than made by me.

**Scope checked, not assumed:** `LabelGui` is the **only** `graphicsApi="GmpiUi"`
module in the tree and holds the only two pins with this mismatch.

### This is probably not Linux-only

`wchar_t` is 4 bytes on Linux and macOS, 2 on Windows — so Windows gets **one**
NUL per character from the same code path, not none. Nobody has looked. The mac
S25 measurement rendered an *Oscillator* prefab correctly, and that prefab has no
Label, so it never exercised this path. **Do not read "seen on linux" as
"Linux-only".**

**Verified:** captions render **MIDI-CV / PITCH / GATE / VEL / TRIG** in real
glyphs; module browser unchanged in the same window; zero plug-in diagnostics;
build 312/312 rc=0 with the temporary text-engine probes reverted first
(`grep -c TIDEDIAG` = 0 in `gmpi_ui`).

**Not verified:**

- **`SynthEditCL` was not built.** The change is confined to one module's pin
  declarations and its three uses, and it compiles as part of TIDE, but the
  commercial consumer was not built here.
- **Windows and macOS** were not built.
- **Whether any shipped patch connects a `DT_TEXT` source to a Label's Text
  pin** — the reason the wire format was left alone, and unmeasured.

**Learned:**

- **A probe on one branch of a multi-branch decision can report a clean bill of
  health for a failure happening in a sibling branch.** Zero notdefs with boxes
  on screen was the single most misleading measurement of the session, and it
  was my instrument, not the system.
- **Log the actual string, early.** Two builds were spent on font resolution;
  one `fprintf` of the shaped codepoints answered it outright, and the hex made
  the diagnosis unmistakable.
- **`backtrace()` + `addr2line -f -C` turns "who called this" into one build.**
  RelWithDebInfo carries enough symbols; the raw `+0x…` offsets resolve fine.
- **"Seen on platform X" and "X-only" are different claims**, and a `sizeof`
  difference is exactly the kind of thing that makes the second one false while
  looking true.
- **Eliminations are worth their cost when they shrink the search space enough
  to instrument.** Four negatives from the morning meant the afternoon had one
  place left to look.

**Machine left clean.** All temporary diagnostics reverted before the
verification build — `grep -c TIDEDIAG` is 0 across `gmpi_ui`, and that worktree
is back to `origin/main`. Headless weston stopped, standalone stopped, scratch
`HOME`s only; **Jeff's `~/.config/TIDE Rack` was not written to** (timestamps
still 2026-08-20). All six repos on their default branches and clean.

**Branch/PR:** `tide/linux/S45-caption-tofu` — TideSynth, the row plus this entry
and one screenshot. Product change is
[SynthEditLib#39](https://github.com/JeffMcClintock/SynthEditLib/pull/39);
**merging TideSynth's side alone changes no behaviour**, it only records.

## 2026-08-24 — linux — S25's Accept passes; the tofu that survives is a different bug (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** ran S25's own Accept on current `main` instead of building toward its
diagnosis. **It passes.** The row is closed by measurement, and what is left of
the symptom is split out as **S45**.

No product code changed.

### The measurement

`main` `44be26e` — which already carries Jeff's *"new tide-specific patch-points
(don't draw anything)"* — `TIDE_Rack_STANDALONE`, RelWithDebInfo, headless
weston, driven through the `gmpi-standalone` command channel. **Isolated `HOME`,
with `~/.config/TIDE Rack/session.xml` confirmed absent at launch** — that is the
trap the 2026-08-20 entry says contaminated three earlier observations, and it is
still the first thing to check.

Insertion is **arm-then-click**, not a drag (2026-08-20). Click `Prefabs`, click
`Oscillator`, click the rack.

| | result |
|---|---|
| freshly inserted `Oscillator` | `TIDE Oscillator`, X 4264 Y 3832 **W 20 H 66** — two bare jacks |
| after kill + relaunch (restored) | **identical** — same jacks, same pixels, same position |
| only difference in the crop | the **blue selection border**, which a fresh insert has and a restore does not |

**That is the mac result.** So insert ≡ restore now holds on both platforms, and
the cross-platform divergence this row existed for is gone.

**The stronger control:** the entire 1100×626 window, before and after a restart,
is **byte-identical** — SHA-256 `97b33b66…` both times, `ImageChops.difference`
bbox `None`. Not "looks the same".

### What did not go away, and why it cannot be this row

The seeded root MIDI-CV facade's pin captions still draw as missing-glyph boxes.
**They do it in both paths** — which is exactly why it is not S25, whose entire
subject is a *difference* between the two paths. Filed as **S45**.

Four things are eliminated on the way, and they are the value of the session:

1. **Not a missing font stack.** The module browser, in the **same window, same
   process**, renders `Fixed Values (Volts)`, `Volts to Float2` and the rest
   perfectly. Whatever is broken is one text path, not the text system.
2. **Not fontconfig and not a missing font file.** `strace` shows four real fonts
   opened — `DejaVuSans.ttf`, `LiberationSans-Regular.ttf`, `NotoSans-Regular.ttf`,
   `UbuntuSans[wdth,wght].ttf` — **zero `ENOENT` on any `.ttf`/`.otf`**, and
   `/etc/fonts/conf.avail/*` read. **This refutes the row's own leading suspect**,
   *"the linux font/resource-binding suspect"*, in the sense the row meant it.
3. **Not the source strings.** `Resources/Prefabs/MidiCv.synthedit` has **zero
   non-ASCII bytes**.
4. **Not insert-vs-restore**, per the byte-identical window above.

The surviving lead, recorded on S45 as a lead and **not** as a diagnosis:
`ModuleViewStruct.cpp:1155` asks for `createTextFormat(9.0f, {}, …)` — an **empty
font family** — while the browser side (`EditorLib/CUG.cpp:3410`) calls
`createTextFormat()` with none. Whether those resolve differently is unmeasured.

### A trap I checked and cleared, so nobody spends the session on it

`gmpi_ui/helpers/FontProvider.h` gates its fontconfig matcher behind
`__has_include(<fontconfig/fontconfig.h>)` — the exact silent-compile-out shape
S43(ii) documented, and it looked like the answer. **It is not:** the header is at
`/usr/include/fontconfig/fontconfig.h`, a default system include path, so it
compiles in whether or not CMake adds anything. Written down because the pattern
is genuinely dangerous elsewhere and will attract the next reader too.

**Verified:** build 312/312 rc=0, 0 errors; Accept run as written; byte-identical
window across restart; `strace` font census; the prefab byte scan.

**Not verified:**

- **The diagnosis of S45.** Eliminations only. I did not instrument the font
  resolution, which is the next step and is written on the row.
- **Jeff's "nothing at all on insert"** is still unreproduced and still not
  assumed to be either bug — though it is worth noting the freshly-inserted
  Oscillator draws **two small jacks and no faceplate**, which on a full rack
  could easily read as "nothing appeared".
- **Windows.** Unmeasured for either symptom.

**Learned:**

- **Run a row's Accept before building toward its diagnosis.** S25 handed me a
  suspect list and a next step; the Accept took twenty minutes and made the whole
  list moot. Sixth re-point in a row where the cell aimed at work already done.
- **A symptom that appears in BOTH arms of a comparison cannot be evidence about
  the comparison.** The tofu was real every time anyone looked, which is why it
  kept being attached to a row about insert-vs-restore; the thing that separates
  them is asking whether the *other* arm has it too.
- **The same window rendering some text correctly is the cheapest possible
  elimination**, and it was visible in the very first screenshot. It kills "fonts
  are broken on this box" before any tooling.
- **`__has_include` guards deserve a check, not a conclusion.** This one looked
  exactly like S43(ii)'s and is inert here because the header sits in a default
  system path.
- **`pgrep -f <pattern>` self-kill bit me twice in one session**, both times on a
  pattern containing my own build path. The journal has warned about this three
  times. `pgrep -x <exename>` does not have the problem.

**Machine left clean.** Headless weston stopped, the standalone stopped, all work
in scratch worktrees and a scratch `HOME`; **nothing was written to Jeff's
`~/.config`** and nothing installed. All six repos on their default branches and
clean.

**Branch/PR:** `tide/linux/S25-fresh-insert-tofu` — TideSynth only: the S25 and
S37 rows, the new S45 row, the linux NEXT cell, three screenshots and this entry.

## 2026-08-24 — linux — S37: CLAP has no bundle on Linux, so TIDE ships a semi-bundle instead (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Jeff, on the shared-resources design: *"we can't have some Resources folder in
the CLAP directory that 100 plugins write into. That's amateur-hour. That's
nonsense."* Then, after the research: *"on Linux CLAP we ship a 'semi-bundle',
simple, easy. do it."*

**Zero code change in `SynthEditLib`.** This is `scripts/package-linux.sh` plus
two docs.

### He also caught that this row's framing was wrong

S37 says *"a second GMPI CLAP installed alongside"*, which reads as a `.gmpi`
module sharing a directory with a `.clap`. **That cannot happen** — different
formats, different scan roots. The only real case is **two GMPI-based CLAPs both
in `~/.clap`**, and a non-GMPI CLAP cannot trigger it at all: it never reads
`Resources/`, and its uninstaller ships no `ControlsXp.xml` to delete. The row
now says so.

### Option (a) is impossible, and that is measured rather than argued

`clap/entry.h`: *"plugin_path is the path to the DSO (Linux, Windows), or the
bundle (macOS)."* "Bundle" appears **twice in the entire CLAP repo** — that line
and the search rule.

REAPER 7.43, headless weston, isolated `HOME`, reading
`reaper-clap-linux-x86_64.ini`. Two plugins per scan so nothing could be deduped:

| layout | discovered |
|---|---|
| `~/.clap/SawDemo_CLAP.clap` (flat control) | **yes** |
| `~/.clap/nested/FreqAnalyser_CLAP.clap` | **yes** |
| `~/.clap/SawBundle.clap/SawDemo_CLAP.clap` | **yes** |
| `~/.clap/FreqBundle.clap/Contents/x86_64-linux/*.so` — **true bundle** | **NO** |

### The answer was in the same header the row already quoted

*"Each directory should be **recursively** searched."* So the plug-in lives in
its own subfolder, `getBundleContentsFolder()` walks for a `Contents` element,
finds none, returns `parent_path()` — and that is now `~/.clap/TIDE-Rack/`.

**That is why option (b) is not needed.** (b) moves the fallback for *every*
non-bundled GMPI consumer — the Linux `.gmpi`, the standalone, Windows — a
compatibility break for already-installed products, to fix a collision that today
has exactly one participant. The packaging change costs nobody anything.

### Verified

- **REAPER lists `TIDE Synth: TIDE Rack`** from `~/.clap/TIDE-Rack/TIDE-Rack.clap`
  — **one** entry, not two.
- **`strace` of the installed layout: 6 reads under `TIDE-Rack/Resources/`, 0
  under the shared `~/.clap/Resources`** — *while that shared folder still exists
  and still holds TIDE-named files.* That is the strong form: the collision target
  is present and ignored, not merely absent.
- Diagnostics clean: `enriched 26 of 70`, **`6 rack prefab(s) seeded`**,
  `root MIDI-CV seeded`.
- **Negative control:** same binary, no `Resources` beside it → all four
  `missing from bundle resources` plus `no Prefabs folder`. The instrument fires.
- **Upgrade path:** `install.sh` run against a simulated old install — new
  semi-bundle in, stale loose `TIDE-Rack.clap` removed, planted
  `OtherPluginData.xml` **survives**.

### The uninstall decision, which is the one judgement call here

`install.sh` **does not delete `~/.clap/Resources`**, and that is deliberate. By
the time we run, those files may be another product's — deleting a shared folder
by name is *precisely* the harm S37 documents. It removes only
`~/.clap/TIDE-Rack.clap`, which is unambiguously ours, and prints a note that the
shared folder is being left behind.

Removing the stale `.clap` is not optional: without it a host lists TIDE Rack
twice, and the old copy still reads the shared folder.

**Learned:**

- **"Can we use a bundle?" and "can we get a private directory?" are different
  questions, and only the second one mattered.** The spec says no to the first
  and yes to the second in adjacent paragraphs.
- **A negative control that leaves the hazard in place beats one that removes
  it.** Straceing against an install where the shared `Resources` is still
  present, still populated, and still ignored proves isolation; an empty parent
  directory would have proved nothing.
- **The expensive option was expensive because of who else it touched, not
  because of its size.** (b) is a few lines in `BundleInfo`; its cost is every
  other GMPI consumer. Ask which products a shared-code change moves before
  costing it.
- **A row's own wording can smuggle in a false premise that survives three
  runs.** "A second GMPI CLAP" was written by someone who knew what they meant;
  every later reader, including me, inherited the wrong picture until Jeff asked
  the plain question.
- **`weston --backend=headless --xwayland` plus an isolated `HOME` makes a real
  commercial DAW a scriptable instrument on this box.** REAPER's
  `reaper-clap-linux-x86_64.ini` is a readable scan report.

**Not verified:**

- **Only REAPER.** Ardour 8.4 here has no CLAP support at all; Bitwig, Qtractor
  and Carla are unmeasured. The spec mandates recursive search and REAPER honours
  it, but *"hosts should"* is not *"all hosts do"*.
- **Windows.** Same fallback and the same `%COMMONPROGRAMFILES%\CLAP` sharing,
  but TIDE ships no Windows CLAP today.
- **The VST3 is untouched** and did not need touching — its resources are inside
  its own bundle already.

**Machine left clean.** REAPER 7.43 downloaded to the session scratchpad and run
only against scratch `HOME`s — **Jeff's `~/.clap` and `~/.config/REAPER` were
never written to**, confirmed after. Headless weston stopped. Worktrees removed;
the `gmpi_ui/.claude/worktrees/` one belongs to another session and was left
alone.

**Branch/PR:** `tide/linux/S37-clap-semi-bundle` — TideSynth only:
`scripts/package-linux.sh`, `docs/distribution.md`, the S37 row and this entry.

## 2026-08-24 — macos — Merged the queue, then closed #291 on a green main (interactive)

**Prompt:** resolve and merge PRs / then take next task

Jeff directed merging, which overrides the standing "never merge your own PR".
Four landed: SynthEditLib#37 (S5), TideSynth#349 (S44, the Windows box's),
#350 (E18), and #353 (S5 record, which had already gone in).

**Order mattered and was not obvious.** #350's CI was red on `render-linux` and
`render-windows`, and the tempting read is "my PR broke CI". It did not:
`build.yml` had failed on `main` for five consecutive runs, and #350 touches two
docs and a script CI never runs. The fix for that red was sitting in #349 — the
Windows box's reference-set split. Merging #349 first turned the render jobs
green, and #350 then went in clean. Diagnosing before merging is what avoided
merging a red PR and calling it pre-existing.

**I shipped a broken commit and CI caught it.** Rebasing #350 the second time, I
resolved the BACKLOG conflict with a script that finds the FIRST
`<<<<<<<`/`=======`/`>>>>>>>` triple. There were TWO. `rebase --continue`
committed the file with live conflict markers in it, and I force-pushed. CI's
`lint` failed with `2 DUPLICATE ID(s): E9, E10`.

Worse, my own local lint run had said rc=0 on that same file. The reason is
worth keeping: I wrote

    printf "  %-24s rc=%s\n" "$(echo $c|cut -d' ' -f1)" "$?"

and `$?` there is the exit status of the `cut` in the command substitution, not
of the linter. Every rc I printed in that loop was `cut`'s. Same shape as the
earlier `git push | sed` bug — a status read through an intervening command.
**Run linters on their own line, or capture the status before anything else
runs.** After fixing that, the checks genuinely passed and so did CI.

Then STEP 1: issue **#291** (Jeff's, `platform:linux`) asked for three things —
decide the metric, re-bake references off the Mac, and actually run the test.
S44 and the render job satisfy all three, so I closed it on measured evidence
rather than on the fact that the PRs merged:

| platform | worst scene | changed | worst Δ |
|---|---|---|---|
| linux | — | 0.000% | 2 |
| windows | `shapes` | 0.083% | 10 |
| macos | `knob` | 0.023% | 17 |

Limits `0.800%` and `40`, 30 scene checks, run 32665903962. The issue's
*"`shapes` has 1% of margin, borderline flaky"* was a cross-ISA artefact — with
per-platform sets it is 0.083% against 0.800%. The tightest margin left is
macOS `knob` worst-delta **17 against 40**, recorded because it is the honest
counterpart to the comfortable changed-fraction numbers.

`main` is green on all three render platforms for the first time.

Rows flipped: S5, E18, S44 → DONE. Open PRs across all six repos: **zero**.

## 2026-08-24 — macos — E18: the probe stops excusing the case E10 fixed (interactive)

**Prompt:** merged. sync. continue.

E9, E10 and S35 all merged, so I flipped the three IN-REVIEW rows to DONE and
re-pointed the mac NEXT cell (it still announced E9 as IN-REVIEW; that is the
fourth time this cell has gone stale in two days).

E10 landing made `scripts/measure-chunk-robustness.py` wrong in a way that would
have quietly stayed wrong. Its `skeleton` case — `<Module>` with no
`<PatchManager>`, the one shape TIDE's own guard cannot see — was expectation
`known-limit`: reported, never failed, because the engine fix was GATED. The
probe even printed `NO LONGER CRASHES -- has E10 landed?`, which is the script
asking to be updated. Left alone it would have gone on passing whether or not
E10 held.

The case is now expectation `survive`: no `TIDE: REFUSED` line is expected (TIDE
genuinely cannot catch that shape), but the host must not die, and a crash is a
hard FAIL naming `SeAudioMaster::BuildDspGraph`. E10 now has a regression test
instead of an excuse.

Measured both directions on this box, same probe, minutes apart — that is the
whole point of the change, so both halves were run:

| binary | skeleton | REAPER crash reports | new logic |
|---|---|---|---|
| pre-E10 (Aug 23 release build) | `rc=-11` SIGSEGV | 2 → 3 | exit 1, FAIL |
| SynthEditLib main, E10 merged | `rc=0` rendered | 2 → 2 | exit 0, PASS |

The crash-report count is the independent evidence: `rc` alone is the probe's
own reading of its own run, and I wanted something outside the probe to agree.
The E10 build was configured with all four local overrides confirmed by their
`Using local ...` lines, so the SynthEditLib under test was the merged one.

The docstring's `*** THIS TOOL CRASHES REAPER ONCE, ON PURPOSE, EVERY RUN ***`
banner was true when written and is now false. Anyone reading it would expect a
crash and not investigate one. Rewritten to say the opposite, with the two
measurements recorded inline.

Jeff's installed plug-in was backed up before the swap and restored afterwards,
byte-identical — what is in `~/Library/Audio/Plug-Ins/VST3` is the Aug 23
release build he had, not my Debug build. The two REAPER crash reports the runs
produced were left in place rather than deleted: they are his data, and the
pre-E10 one is real evidence.

**Not verified:** Windows and Linux. The probe drives REAPER and has only ever
been run on macOS, so the new failure path is unexercised there.

## 2026-08-24 — macos — S5: the folder-info null deref, measured then guarded (interactive)

**Prompt:** sync all repos. next task

S5 was filed against `SE16/SynthEdit2/Application.cpp:167`. That path is dead:
the file moved repos, and neither `m_folder_settings` nor
`refreshFolderLocations` appears anywhere in SE16 today. It lives in
`SynthEditLib/EditorLib/Application.cpp` — EditorLib, which is GATED, and this
session is interactive, which satisfies the gate. Anyone taking this row from
its stated path would have concluded the bug was gone.

I nearly did. Reading `getFolderInfo` it looks safe — it loops over
`m_folder_settings`, and an empty vector just means zero iterations, then it
creates an entry and returns `.back()`. I wrote that down as "no UB". It was
wrong, and the reason is worth recording: the fall-through line is 300-odd
characters long, my grep truncated at 124, and the two `m_folder_settings[0]`
subscripts are in the tail. Printing the whole line is what found it.

So the row is right. An unrecognised extension inherits entry 0, `"All Files"`,
which exists only once `refreshFolderLocations()` has run — and
`TideApp::InitInstance` replaces `CSynthEditAppBase::InitInstance` wholesale
(S1a, the module-scan removal) and never calls it. In TIDE the vector is empty
for the life of the app.

Measured rather than argued. A harness linking the real `libEditorLib.a`,
subclassing `ApplicationBase`, deliberately not calling
`refreshFolderLocations()` — TIDE's exact state:

| | result |
|---|---|
| before | `getFolderInfo(L"sem")` → SIGSEGV, exit 139 |
| lldb | `EXC_BAD_ACCESS (code=1, address=0x0)` in `unique_ptr::operator->`, `this=0x0` |
| after | valid `folder_info`, description `"sem Files"`, exit 0 |

The second half matters as much: a populated entry 0 must still be inherited, or
the guard would have quietly broken full SynthEdit. Same harness, entry 0 seeded
— identical output before and after. `EditorLib` also builds clean against the
patched tree, all four overrides confirmed by their `Using local ...` lines.

**Not the fix:** calling `refreshFolderLocations()` from the `ApplicationBase`
constructor. It looks like the deeper repair — populate the vector once and no
caller can trip — but `getSettingString` is `virtual`, so a constructor call
dispatches to the base and seeds the wrong folders. The guard is the one
subscript in the repo; `.back()` on the next line is safe by construction.

**Left open deliberately:** `refreshFolderLocations()` still never runs in TIDE.
Its folders stay empty, so `ShortenFilename` returns absolute paths where it
would otherwise return relative ones — documents are less portable. That is a
behaviour question, not undefined behaviour, and it belongs to whoever decides
what TIDE's folder defaults should be, so it is on the row rather than changed
under cover of a crash fix.

**Not verified:** Windows and Linux. The harness was built and run on macOS.

Harness kept out of the repo on purpose — it needs three link stubs for UI
dialogs EditorLib references, which is scaffolding, not a test the project
should carry. SynthEditLib has no test target to put it in.

## 2026-08-24 — linux — the A4 auto-merge trap, hit a third time, and the branch DELETION is the new half

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**A correction to the entry directly below**, not a second item. That entry ends
*"Machine left clean"* and names no trap, because [#351](https://github.com/JeffMcClintock/TideSynth/pull/351)
merged after it was written. A separate entry rather than an edit, for the reason
#121 established: a log you edit is not a log.

**Nothing is wrong on `main`.** The rows and the entry landed in `9e5cb27`, whose
own subject carries `(#351)`. This is about how the run ENDED.

### What happened

STEP 4's A22 dance is: name the branch, push, open the PR, then push one more
commit adding the PR number — *"Check the PR is still open before you push that
follow-up, and if it has already merged, DROP it."* I did check. It said `OPEN`.
I made the commit. Then I ran the re-check and the push **in the same command
block**, chained unconditionally:

```
gh pr view 351 --json state --jq .state   ->  MERGED
git push                                  ->  * [new branch]
```

**The check fired correctly and I pushed anyway**, because I had already decided
to push when I wrote the block. STEP 4's rule is not "check", it is "check, and
branch on the answer" — a check whose result cannot stop the next command is
decoration.

### The new half: auto-delete means the follow-up RE-CREATES the branch

#120/#121 landed a follow-up on a branch whose PR had merged. Here the branch was
**already deleted** by merge auto-delete, so `git push` did not update anything —
`* [new branch]` — it **brought the branch back from the dead**, with a commit
nobody had asked to review.

That is strictly worse than #120/#121 and it does not look worse: the push output
is a cheerful `[new branch]` line identical to a first push. **The tell is that a
follow-up push should never say `[new branch]`.** If it does, the PR closed and
auto-delete ran.

### Fixed, by deleting rather than by a second PR

Deleted `origin/tide/linux/issue-156`. STEP 4 says pushing nothing is always safe
here and *"a commit whose only content is a link is not worth a second PR"* — and
A22's whole point is that **the branch name in the row is what makes the follow-up
optional**. Both rows name the branch, `9e5cb27`'s subject names the PR, so the
deleted commit carried nothing that is not already on `main`.

Checked before deleting that it was mine and that its parent was on `main`. The
three remaining remote branches belong to other boxes and were left alone.

**Verified:** `9e5cb27` is on `origin/main` and contains both row updates and the
entry; `git ls-remote --heads` shows no `tide/linux/**` branch; PR #351 `MERGED`;
issue #156 `CLOSED`.

**Not verified:** whether auto-delete is repo policy or was configured per-PR —
I observed the effect, not the setting.

**Learned:**

- **A `gh pr view` state check is worth nothing in the same unconditional command
  block as the push it is meant to gate.** Run it, read it, then decide. This is
  the third time this fleet has met the A4 auto-merge race and the first time the
  check was actually present and still did not help.
- **A follow-up push that reports `[new branch]` has re-created a deleted branch,
  not updated one.** With auto-delete on, the A22 follow-up window closes by
  removing the branch, so the failure mode is resurrection rather than a stranded
  commit — and the output looks like success.
- **The A22 follow-up is optional by design, so "drop it" is cheap.** The instinct
  to salvage the commit with a second PR is the expensive branch, and STEP 4
  already ruled it out; deleting is the one-command answer.
- **A merge that happens between writing an entry and pushing it makes that entry
  wrong about its own ending.** Cheaper to prepend a correction than to leave
  *"machine left clean"* as the last word on a run that left a stray branch.

**Machine left clean**, now genuinely: no `tide/linux/**` branch on the remote,
all scratch worktrees removed, all six repos on their default branches with clean
trees, nothing built in any of Jeff's checkouts, nothing installed.

**Branch/PR:** `tide/linux/issue-156-followup` — TideSynth, this entry only.

## 2026-08-24 — linux — STEP 1: #156 verified green on Linux, and the 44 failures reproduce on demand

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** took no backlog item. **STEP 1 outranked STEP 2** — there are three open
`platform:linux` issues, and one of them was actionable. Verified
[#156](https://github.com/JeffMcClintock/TideSynth/issues/156) on this platform
and closed it. **Zero product code changed**; this repo gets the two row updates,
a re-pointed NEXT cell and this entry.

### Why #156 and not the other two

STEP 1 says a platform issue outranks backlog work, so the first job was working
out which of the three is mine. Written down because the next Linux run will meet
the same three:

| issue | author | verdict |
|---|---|---|
| [#291](https://github.com/JeffMcClintock/TideSynth/issues/291) render references re-baked on the wrong box | Jeff | **not mine** — the fix is [#349](https://github.com/JeffMcClintock/TideSynth/pull/349), open and green from the windows box. Taking it would be the duplicate-work collision STEP 2 exists to prevent. |
| [#88](https://github.com/JeffMcClintock/TideSynth/issues/88) `SynthEditJuce` misses `Dialogs_editor2.cpp` | bot | **not takeable** — `SE16/SynthEditJuce/` is on neither STEP 5 list, so GATED by default. Not a build break either: the target is deprecated and reachable from no build on any box, so A17's exception does not stretch to it. |
| [#156](https://github.com/JeffMcClintock/TideSynth/issues/156) ctest 44/67 on Linux from macOS-hardcoded paths | bot | **this one.** Its stated cause was fixed by **S16** + **S42**, and *both rows say in bold "NOT VERIFIED: Windows and Linux"*. The verification is the work. |

STEP 1's bot-issue rule — *"re-verify the finding on your own platform before
acting on it"* — is the whole item here rather than a preamble to it.

### The measurement

Fresh scratch worktree of `SE16` at `origin/master` **`63ce2bb8e`** (the merge of
[SynthEdit#74](https://github.com/JeffMcClintock/SynthEdit/pull/74)), overrides
pointed at scratch worktrees of `SynthEditLib` `fb55275`, `TideSynth` `1364801`,
`gmpi_ui` `6aa8871`, `GMPI` `83b9de7`. Ninja, Release, GCC.

**The checkout path is the point.** It is
`.../scratchpad/wref/SE16` — not `~/SE/SE16`, not any developer's checkout, and
not the path baked into the fixtures. S16's Accept is *"from a checkout at any
path"*, and a build in the usual place cannot test that clause at all.

Configure **rc=0**. `cmake --build --target dsp_tests` is **327/327, rc=0, 0
errors** — and pulling in `SynthEditCL` and `cancellation` on its own is S16's
`add_dependencies` working, which the row says was necessary but not sufficient.

**Both defaults reached the binary, checked with `strings` rather than assumed:**

```
SE_BUILD_FOLDER_DEFAULT    -> .../scratchpad/build-se16
SE_UNITTEST_FOLDER_DEFAULT -> .../scratchpad/wref/SE16/UnitTest
/Users/jeffmcclintock       -> 0 occurrences
```

**ctest with `SE_BUILD_FOLDER` and `SE_CANCELLATION_FOLDER` deliberately unset:**

```
100% tests passed out of 73
```

**rc=0, stable across two consecutive runs.** Zero `not found` lines, zero
`32512`, and **zero references to the dead `/Users/jeffmcclintock/` checkout** —
against the 536 S42 measured mid-arc on macOS.

### The negative control, which is what makes the green mean anything

A suite that cannot fail reports 100% for the same reason a fixed one does. So,
same binary, same run, one variable:

| `SE_BUILD_FOLDER` | result |
|---|---|
| unset (the CMake default) | **73/73 pass, rc=0** |
| `/Users/jeffmcclintock/SynthEdit/build/` | **44 failed of 73, rc=8** |

**44 is the exact number in #156's title.** The issue's headline failure
reproduces on demand and disappears on demand, and the variable is the one thing
S16 changed.

One difference from #156's era, recorded because the next person will grep for
the old signature and not find it: the failure no longer surfaces as `system()`
returning `32512`. S16 added an explicit existence check, so
`Basics.Cancellation_Utility_Exists` and `TestUI.CancellationUtilExists` fail
first and name the missing helper. Better diagnostic, different string.

### The one red I had to chase, and it was not a defect

The first ctest run was **63/64 with `ui_tests_NOT_BUILT` failing**, which looks
exactly like a broken target. It is `gtest_discover_tests`' placeholder for
`ui_tests` — a `gmpi_ui` target registered at configure time that I had never
built, because I built `--target dsp_tests` and nothing else. Building `ui_tests`
(7/7, rc=0) turned 64 tests into 74 and the suite green.

Worth separating the two numbers, because they are both quoted in this project
and they are not the same suite: **`dsp_tests` is 63 of 63 here, which is exactly
the 63 the macOS run measured** — same size, both platforms, which is stronger
agreement than a pass rate. The 73 is `dsp_tests` + `ui_tests`; one further test
is `Disabled`. **#156's own "67" is a third number** from a differently-configured
tree, so do not read a mismatch against it as a regression.

`synth_ui_tests` was **not** generated: `GMPI_UI_TESTS_FOLDER` defaults to
`C:/SE/gimpi_ui_tests`, and this box's clone is at `~/SE/gimpi_ui_tests`. Not
part of this issue; noted so nobody reads its absence as breakage.

### Both default branches build on Linux — no platform issue to file

STEP 3 asks every run that builds anything to say whether its platform's default
branch also builds, and this run built two trees, so both answers are first-hand
rather than inherited from CI:

| tree | result |
|---|---|
| `SE16` `origin/master` `63ce2bb8e` | `dsp_tests` chain **327/327** + `ui_tests` **7/7**, rc=0, 0 errors |
| `TideSynth` `origin/main` `1364801` | **483/483, rc=0, 0 errors**, all four Linux artifacts |

The TIDE artifacts, from an unmodified `main`: `TIDE-Rack` (standalone),
`TIDE-Rack.clap`, `TIDE-Rack.gmpi`, and the `TIDE-Rack.vst3` bundle carrying
`TIDE-Rack.so`. **So there is no `platform:linux` build break to file**, and the
three open Linux issues are all about tests and packaging rather than compilation.

**Verified:** configure rc=0; `dsp_tests` 327/327 rc=0; `ui_tests` 7/7 rc=0;
ctest 73/73 rc=0 twice; `strings` control on the binary; the 44-failure negative
control; TIDE's own default branch built (below).

**Not verified:**

- **Windows.** Both S16 and S42 still say "NOT VERIFIED" for it and this run does
  not change that. The rows now say Linux is done and Windows is not.
- **`synth_ui_tests`**, for the folder-default reason above.
- **`SE16`'s full tree.** I built the `dsp_tests` and `ui_tests` chains, not all
  of `SE16`, so this says nothing about `SynthEditWayland` or #88.

**Learned:**

- **A "NOT VERIFIED on your platform" line in a DONE row is a work item, and
  nothing points at it.** S16 and S42 both carried one for a day. The thing that
  surfaced it was not the backlog — it was an open `platform:{PLATFORM}` issue
  describing the same defect, which STEP 1 forces every run to read first.
- **The negative control was one environment variable and it is the whole
  entry.** Everything else here is "the tests pass", which is what a suite with
  its fixtures missing also reports once it stops being able to fail. Reproducing
  #156's own 44 is what turns that into a measurement.
- **`gtest_discover_tests` registers a `<target>_NOT_BUILT` placeholder**, so a
  partial build produces a red test that names a target rather than a defect.
  Building one target and running the whole suite will always look like this.
- **Three quoted pass counts for one suite — 63, 67, 73 — and all three are
  correct.** They differ by which targets the tree generated. Quote the target
  with the number or the next run reads a configuration difference as a
  regression.
- **A scratch worktree is not just tidiness here, it is the test.** "From a
  checkout at any path" is unfalsifiable from the developer's own checkout.

**Next:**

1. **Windows is the remaining half of S16/S42.** Same recipe, one command, and
   the negative control transfers unchanged.
2. **#88's `SynthEditJuce` half needs an owner who can say "by inspection"** —
   it is one line, in a GATED path, in a target no box builds.
3. **S23 remains this box's take-target** and needs no ruling; the linux NEXT
   cell is re-pointed at it and says why S43(ii) and S37 are no longer options.

**Machine left clean.** Five throwaway worktrees under the session scratchpad,
one per repo, plus two scratch build trees. **Nothing was built in any of Jeff's
checkouts** and `~/SE/build` was not touched. No compositor was started, nothing
was installed, and no plug-in was copied anywhere. All six repos were clean and on
their default branches at the start and are back on them at the end.

**Branch/PR:** `tide/linux/issue-156` — TideSynth only: the S16 and S42 rows, the
linux NEXT cell, and this entry. **No product code change in any repo.**

## 2026-08-24 — macos — S35: the scanner searches both plug-in domains now (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Fixed in [SynthEditLib#36](https://github.com/JeffMcClintock/SynthEditLib/pull/36),
option (a) — the one the row calls the right fix.

**Confirmed the mismatch on this box before touching anything:** `ModulePath` is
`/Library/Audio/Plug-Ins/GMPI` with 7 modules; `~/Library/Audio/Plug-Ins/GMPI`
has 9; none of the 9 were scanned.

New `getUserPluginsFolder()` returns EMPTY on Windows and Linux — Windows has no
per-user plug-ins location, and Linux already keeps everything under the
per-user data dir, so a second scan would be the same folder. Empty means no
second scan, so neither platform changes behaviour. The user path is DERIVED
from `ModulePath` rather than hardcoded, so someone who has repointed it keeps
one scan instead of silently gaining a folder.

**The verification is where this got interesting.** The row's own evidence metric
is "user-domain paths in `Plugin-Cache-16-override-*.xml`" — and after the fix it
was still **zero**. It would have been easy to read that as the fix not working,
and equally easy to ship it claiming success without looking.

What it actually is: those caches store module metadata without absolute paths,
and the modules were already known. Running `SynthEditCL -rescan` and reading the
output settled it — the scanner now prints both scan lines, and of the nine
user-domain modules **eight are duplicates of factory SEMs**, reported as
*"Module FOUND TWICE!"*, which is correct behaviour. The ninth is
**`TIDE-Rack.gmpi`**, not a duplicate, now visible where it was not.

A locally built plug-in module is the entire case this row exists for, so that
one file is the result — not the cache metric the row happened to reach for
when it was filed.

**Not verified:** Windows and Linux. Both take the empty-string path and get no
second scan, but only macOS was run.

**Note:** I ran `-rescan` against Jeff's real settings with a scratch build,
which rewrites his module cache. Compared before and after: identical content,
so nothing of his changed.

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

### CI closed all three of the gaps this entry was going to list as unverified, and the prediction held

The PR's own run — [#349](https://github.com/JeffMcClintock/TideSynth/pull/349),
all three render jobs **pass**, `main`'s `build.yml` line unchanged:

| job | resolved set | result |
|---|---|---|
| **render-windows** | `tests/references/windows-linux` | 10/10 — `shapes` **0.083%, delta 10**, everything else 0.000% |
| **render-linux** | `tests/references/windows-linux` | 10/10 — **0.000% on all ten**, worst delta 2 |
| **render-macos** | `tests/references/macos` | 10/10 — `knob` 0.023% delta 17, everything else 0.000% |

**Three things fall out of that table, and none of them were guaranteed.**

**The Linux prediction was right for the stated reason.** I wrote before the run
that Linux should read 0.000% rather than Windows' 0.083%, because the images
came off an ubuntu runner and Linux is comparing against its own bake. It reads
**0.000% on all ten**. The 0.083% is specifically the Windows-vs-Linux gap, not
noise in the set.

**Two different Windows machines agree exactly.** This box (MSVC 14.51, local)
and `windows-latest` both read `shapes` at **0.083%, worst delta 10** and
everything else at 0.000%. Same figure to three decimals on unrelated hardware,
which is the reproducibility the whole image-test design claims and rarely gets
to demonstrate across machines.

**`macos` is picked and passes**, so the `#if defined(__APPLE__)` arm is measured
rather than reasoned — and it is not a trivial pass: `knob` moves 0.023% at delta
17 on `macos-latest` against references baked on a Mac, so that set has real
runner-to-runner variation and still lands well inside the limits.

**And the workflow was never touched.** Three platforms resolved three sets from
one unchanged command line, which is the whole claim of putting the selection in
the binary.

**Not verified:**

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

1. **[#291](https://github.com/JeffMcClintock/TideSynth/issues/291) closes when this merges** — its remedy is exactly this PR, and all three render jobs are green on the branch. The issue is `platform:linux`-labelled and was never only Linux's; Windows fails five of the same ten scenes. Commented there with the numbers rather than closing it, since it is Jeff's issue and the fix has not landed yet.
2. **Any future intended look change now has to be re-approved in BOTH sets**, on a machine of each family, or the platform that was not re-baked goes red. The README says so; nothing enforces it, and that is the obvious next defect this arrangement can produce.
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

**Branch/PR:** `tide/win/S44-s27-reference-split` — [#349](https://github.com/JeffMcClintock/TideSynth/pull/349), TideSynth only.

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
