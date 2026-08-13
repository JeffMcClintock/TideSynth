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

## 2026-08-14 — windows — merge cleanup for A13/P6/C4 (interactive session, Jeff directing)

**Did:** After the scheduled A13 run finished, Jeff took over interactively
and asked to fix merge conflicts and merge the queue's open PRs oldest first.
Four PRs were outstanding across three repos, all touching the same shared
docs (`BACKLOG.md`, `JOURNAL.md`, `JOURNAL-2026-08.md`), so #49 merging first
put #50 and #51 into real conflict. Resolved both, then merged all four in
creation order: [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6)
(07:24) → [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15)
(07:24) → [#50](https://github.com/JeffMcClintock/TideSynth/pull/50) (18:14,
P6) → [#51](https://github.com/JeffMcClintock/TideSynth/pull/51) (20:48, A13).

**Result:**

- **SynthEdit#15 needed a CI re-run, not a fix.** It was failing on
  `CMake Error: Cannot find source file` because it pulls `SynthEditLib` from
  `origin/main`, where the twelve C4 files didn't exist yet. Merged
  `SynthEditLib#6` first, then `gh run rerun --failed` on the same run —
  both `windows-latest` and `windows-2025-vs2026` jobs went from fail to pass
  with no code change, confirming the dependency was exactly what the PR body
  said it was.
- **#50 and #51 both had real git conflicts** in `BACKLOG.md`/`JOURNAL.md`/
  `JOURNAL-2026-08.md`, all from independent same-day rotations racing against
  each other rather than genuine content disagreement. Each resolved the same
  way: whichever side's rotation work was **already accepted on `origin/main`**
  won; a branch's own independent rotation of an entry `origin/main` had
  already archived was dropped as a duplicate rather than merged in twice.
  Confirmed no duplication by grepping each archive for the entry heading
  before and after.
- **The A13 conflict resolution accidentally became A13's own best test.**
  `docs/carve-out.md`, `JOURNAL.md` and `JOURNAL-2026-08.md` all use em-dash
  headings, and the S6 part-1/part-2 cross-reference — the link A13's own row
  was written about — ended up with both halves landing in
  `JOURNAL-2026-08.md` together as a direct result of this session's
  rotations. `check-links.py` (with A13's fix) reports 0 broken on the
  resulting tree; the pre-fix slugger would have flagged that exact link.
- **STEP 4 chore, done live rather than left for a later run:** C4, P6 and
  A13 were all `IN-REVIEW` with every linked PR now observed merged in this
  same session, so flipped all three to `DONE` and moved the rows verbatim
  into `BACKLOG-DONE.md`, newest first. Also fixed a stray blank line left
  inside the carve-out table by the row removal.
- **Found, not fixed: the `linux` NEXT row was already stale on `origin/main`
  before this session started.** It still pointed at P7c, which C4's own PR
  (#49) had already flipped to `DONE` and archived — the PR that archived it
  never updated the pointer that named it. Every other `linux`-platform row
  (X1, X2, R4) is `BLOCKED`, so a scheduled linux run today falls through to
  the `any` fallback, and nobody has run that fallback through the same
  NEEDS-JEFF/workflow-wall screening the `any` row itself just needed for A4.
  Flagged in the row rather than guessed at — the queue already has one
  instance this session of a wrong guess (the original E1a/linux pointer)
  costing a run its whole session on discovery, and a second wrong guess here
  would cost another.

**Learned:**

- **A same-day multi-branch queue racing the same rotation files will always
  produce this shape of conflict**, and it resolves the same way every time:
  trust whichever side is already on `origin/main`, treat the other branch's
  independent rotation of the same entry as a duplicate, and grep the archive
  before/after to prove no entry was dropped or doubled. Doing this by hand
  three times in one session is exactly the kind of load A4 (the auto-merge
  tier) was filed to remove — its row is more urgent than its own text says.
- **`gh run rerun --failed` is the right tool when a cross-repo CI failure's
  cause has already been fixed by merging the other repo** — cheaper and more
  informative than pushing an empty commit to retrigger, since the log shows
  the exact same job going from fail to pass with nothing else different.

**Next:** the `linux` NEXT row genuinely needs someone to work out what a
linux-eligible scheduled run should take, not just notice it's wrong — left
as a flagged question rather than a guess, on purpose. `E1a` still needs a mac
or win run to do its render half.

**Branch/PR:** none — merge-conflict fixes were pushed to the branches being
merged (`tide/mac/P6-cl-codesign-xcode`, `tide/win/A13-check-links-slugger`),
which then merged into `main` via their own PRs. This entry and the
DONE-row/NEXT-row cleanup are committed directly to `main`, interactive
session.

---

## 2026-08-14 — windows — A13 (C4 not re-taken, A4 not takeable — see below)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code Desktop, app version not
discoverable on this box (see **Learned**) · as `tide-rack-bot`

**Did:** Fixed `scripts/check-links.py`'s slug function, which disagreed with
GitHub's on every em-dash heading, and fixed a second defect of the same
character sitting a few lines below it. Verified both against GitHub's own
renderer rather than against my reading of the algorithm.

**Neither NEXT pointer was takeable, and one of them is a standing trap.**

- **`win` → C4 is already done.** The 2026-08-13 windows run completed it; it is
  IN-REVIEW across three PRs ([#49](https://github.com/JeffMcClintock/TideSynth/pull/49),
  [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15),
  [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6)). The
  resume rule makes an open PR from my own platform mine to *continue*, but
  there is nothing to continue — the work is complete and waiting on Jeff.
  I checked both red PRs before leaving them alone, and **neither red is a
  defect in the C4 work**: #49's `lint` is red only on `links`, which is this
  very row and is red on `origin/main` too; SynthEdit#15's `WASDK build check`
  dies at `CMake Error at EditorLib/CMakeLists.txt:10 (add_library): Cannot
  find source file`, which is the cross-repo condition the PR body already
  states — CI fetches `SynthEditLib` from `origin/main`, where the twelve moved
  files do not exist until SynthEditLib#6 merges. C2 and C3 failed that same
  check for that same reason, and C3 merged anyway. Structural; not fixable
  from a branch.
- **`any` → A4 cannot be done by a scheduled run at all, and its row does not
  say so.** A4 is an auto-merge *action*, i.e. a file under
  `.github/workflows/**`, and the bot token is deliberately `repo` scope with
  no `workflow` — measured this run, not assumed: `X-OAuth-Scopes: repo` from
  the API response headers. The push is rejected however correct the work is.
  **A10 and A12 hit the identical wall** (both name `.github/workflows/` in
  their own scope lines), and **A9 has an open NEEDS-JEFF prerequisite** —
  TIDE's product philosophy as the auto-reject filter — which would change what
  gets built, so STEP 2 rules it out. That leaves **A13** as the topmost `any`
  item a scheduled run can actually finish, and A13's own row had already
  argued it should precede A4 anyway ("a noisy lint erodes trust in the other
  three checks fast"). Updated the `any` NEXT row to record all of this.

**Result:** Both bugs reproduced, fixed, and verified.

*The bug, exactly.* `slugify()` did `re.sub(r'\s+', '-', s)` — one hyphen per
*run* of whitespace. GitHub emits one per *space*. The line above had already
deleted the em-dash, leaving the two spaces that surrounded it, so
`## 2026-08-13 — macos — S6 (part 2 of 2)` produced
`2026-08-13-macos-s6-part-2-of-2` here and `2026-08-13--macos--s6-part-2-of-2`
on GitHub. The fix is `s.replace(' ', '-')`, plus deleting tabs and other
non-space whitespace outright, which is what GitHub does with them.

*The second defect, which was not in the row.* `anchors_of()` never tracked
code fences — `main()` did, but `anchors_of()` did not — so **nine fenced lines
across four files were registered as real anchors**, among them `JOURNAL.md`'s
own entry template `## YYYY-MM-DD — <machine> — <BACKLOG id>`,
`#include "it_empty.h"` in `docs/c8-it-empty-header.md`, and three `#0`/`#1`/`#2`
gdb backtrace frames. That is a false *negative* — a link to
`#include-it_emptyh` passed the check. Same both-directions character as the
slug bug, same function, so fixed in the same pass.

*The verification artifact — an A/B against GitHub's own renderer.* Fetch each
file through the contents API with `Accept: application/vnd.github.html`, which
returns it rendered by GitHub's real markdown pipeline with anchors intact
(`<a id="user-content-…" class="anchor">`), and compare every anchor to what
this script generates for the same bytes:

```
files 28 | headings compared 254 | mismatching files 0
```

**254 of 254, zero mismatches.** Two negative controls, so the A/B is not
passing vacuously:

```
CONTROL 1  old slugify (collapsing run)        headings wrong: 111   files affected: 23
CONTROL 2  new slugify, fences NOT skipped     headings wrong:  34   files affected:  4
FIXED      new slugify, fences skipped         headings wrong:   0   files affected:  0
```

*Deliberate breaks, all three as specified in the row's Accept clause:* a
nonexistent anchor fails (RC=1); a link written in the **old collapsed form**
now fails (RC=1) — that is the false negative closing, and it is the half that
matters; the correct GitHub form passes (RC=0).

*Tree state:* 186 relative links, **0 broken**. The one previously-flagged line
(`JOURNAL.md:376`) was read and is exactly the false positive A13 predicted —
the first intra-journal anchor anyone wrote.

*Added `--selftest`* to the same script: six golden slugs **read off GitHub's
renderer**, plus a duplicate/fence case, baked in and offline so the regression
is permanent rather than something a future run must remember to re-measure.
Confirmed discriminating — reverting `slugify` to the old algorithm fails 5 of
its 6 cases, RC=1. Also implemented GitHub's duplicate-heading suffixes
(`-1`, `-2`), which no heading in the tree exercises today but `anchors_of()`
was silently collapsing into one.

**Learned:**

- **`/markdown` is not an oracle.** The obvious endpoint for "what would GitHub
  render this as" emits headings with **no `id` attribute at all**, so the
  first A/B came back 0-for-254 and looked like a catastrophic failure of the
  fix rather than of the measurement. The endpoint that works is
  `GET /repos/{owner}/{repo}/contents/{path}` with
  `Accept: application/vnd.github.html`. Anyone verifying anchor behaviour
  again should start there and skip the hour.
- **The em-dash convention and the checker were on a collision course from the
  start.** Every journal entry heading in this repo uses em-dashes, so the
  moment anyone wrote the first intra-journal anchor link the check went red —
  which is exactly what happened, and A3 could honestly claim zero false
  positives when it landed only because nobody had written one yet.
- **The app version STEP 0.5 asks for is not discoverable on this box.** `claude`
  is not on `PATH` under the desktop app, and there is no `app-*` directory or
  `package.json` under `%LOCALAPPDATA%\Claude` carrying a version. The linux and
  mac entries record `Claude Code CLI 2.1.220` because those boxes run the CLI.
  So the provenance line's `app <version>` field is silently unfillable on
  Windows-under-desktop, and `check-prompt-provenance.py` cannot catch that —
  it only looks for the literal `**Prompt:**` marker, not for the fields after
  it. Recording it in prose here rather than inventing a number.
- **Two lint scripts cannot be fed a process substitution on this box.**
  `python scripts/check-backlog-diff.py <(git show origin/main:BACKLOG.md) …`
  fails with `FileNotFoundError: '/proc/1398/fd/63'` — Git Bash creates the fd,
  Windows Python cannot open it. Write the base version to a real temp file.

**Next:** A13's PR is [#51](https://github.com/JeffMcClintock/TideSynth/pull/51)
and should merge **after** [#49](https://github.com/JeffMcClintock/TideSynth/pull/49)
— both prepend to `JOURNAL.md` and both rotate `JOURNAL-2026-08.md`, so
whichever lands second needs a rebase, and #49 is the older and larger of the
two. `BACKLOG.md` does not collide: #49 touches the `win` NEXT row and the
C4/C9/C11/P7c rows, this one touches the `any` NEXT row and A13.

The `any` lane needs a decision, not a run: **A4, A10 and A12 are all
`.github/workflows/**` work that the bot token structurally cannot push**, and
all three sit in the queue marked TODO as though a scheduled run could take
them. Each will burn a session on discovery until someone re-marks them.
That is the same shape as **A12's own finding** — a box that cannot proceed and
nothing escalating it — one level up, applied to the queue instead of to a box.

**Branch/PR:** `tide/win/A13-check-links-slugger` →
[#51](https://github.com/JeffMcClintock/TideSynth/pull/51)

---

## 2026-08-14 — macos — P6

**Prompt:** `dd93251` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Did:** Closed P6 by measurement, not by edit. Built `SynthEditCL` on this box
with the **Xcode** generator — the one thing the P7a and P7b runs could not do,
and the whole reason the row stayed open — and confirmed the `CodeSign` failure
is gone. **Changed no source file:** `SE16/SynthEditCL/CMakeLists.txt` is on
neither the ALLOWED nor the GATED list, so it is GATED by default, and the fix
had already landed in SynthEdit's own commits. The work was verification.

**Result — fixed, and proven on the generator that actually signs.**

| | |
|---|---|
| source | `SE16` `b3c1efb07` — `git diff HEAD origin/master -- SynthEditCL/ SynthEditLib/ CMakeLists.txt` is **empty**, so this is current `master`'s content for everything in scope |
| configure | `cmake -G Xcode`, fresh scratch tree outside both repos, `x86_64;arm64`, deployment 13.3, three local overrides matching Jeff's own `build/CMakeCache.txt` |
| `cmake --build --config Debug --target SynthEditCL` | **RC=0** |
| `codesign --verify --deep --strict --verbose=2` | *"valid on disk"*, *"satisfies its Designated Requirement"* |
| sealed | 336 files, 98 signed `.sem` under `Contents/PlugIns` |
| smoke | the signed binary runs: `SynthEditCL V1.6.182` |

**The Xcode generator does emit the step, and the log proves it** —
`CodeSign …/SynthEditCL.app (in target 'SynthEditCL')`, `Signing Identity:
"Sign to Run Locally"`, `/usr/bin/codesign --force --sign - --entitlements … `.
That is the asymmetry this row was stuck on: **Ninja emits no `codesign`
invocation at all**, so an RC=0 Ninja build is not evidence either way.

**Layout, which is the actual fix (`691270c5d`):** `Contents/MacOS` holds
**only** the executable. `Prefabs`, `fonts`, `skins` and `templates` are all
under `Contents/Resources`, and the exact file the row named is now
`Contents/Resources/Prefabs/Controls/Button Small2.syntheditprefab`.

**Verification artifact — A/B positive control on the same signed binary, no
source edit and no rebuild.** Copied the built bundle, moved the four staged
directories back under `Contents/MacOS/Resources` to recreate the pre-fix
layout, and re-ran the *same* `codesign` command Xcode ran:

| bundle layout, same binary, same codesign command | RC | output |
|---|---|---|
| pre-fix (`Contents/MacOS/Resources/…`) | **1** | `SynthEditCL.app: code object is not signed at all` / `In subcomponent: …/Contents/MacOS/Resources/Prefabs/Controls/Button Small2.syntheditprefab` |
| current (`Contents/Resources/…`) | **0** | — |

That reproduces P6's error string **verbatim, down to the same subcomponent
file**, and shows the staging path is what closes it — not a toolchain or Xcode
version difference, and not luck.

**The second commit (`4792f4bf2`, Finder detritus) also holds:** the built
bundle contains **0** `.DS_Store` and no extended attributes.

**Standing rule — all five products build under the Xcode generator, each
RC=0, each verifying:**

| target | build | `codesign --verify --deep --strict` |
|---|---|---|
| `SynthEditCL` | RC=0 | RC=0 |
| `SynthEdit_VST3` | RC=0 | RC=0 |
| `SynthEdit_GMPI` | RC=0 | RC=0 |
| `TIDE` | RC=0 | RC=0 |
| `TIDE_VST3` | RC=0 | RC=0 |

So SynthEdit, SynthEditCL and TIDE all build on macOS on `master` today, under
the generator Jeff's own tree uses — which is a stronger statement than the
Ninja RC=0 the last three mac runs could make.

**Learned:**

- **Any signing-shaped question on mac must be answered with `-G Xcode`.** Ninja
  never emits `codesign`, so a Ninja build cannot confirm *or* deny a codesign
  bug. P6 sat open for six days because two runs reported RC=0 from a generator
  that structurally could not see the failure. Worth treating as a fleet rule,
  not a P6 detail.
- **A bundle-level A/B is enough to prove a staging-path fix, and it needs no
  source edit.** That matters when the file lives on a GATED-by-default path:
  the positive control was `cp -R` + `mv` + re-run `codesign`, and it produced
  the row's exact error text. No branch in `SE16`, nothing to review there.
- **`GMPI_WRAPPER_FOLDER_OVERRIDE` is empty in Jeff's `build/CMakeCache.txt`**,
  so `GMPI WRAPPERS` is fetched from github rather than taken from the local
  clone — the configure output says `Fetching GMPI WRAPPERS from github` while
  `SynthEditLib`, `GMPI` and `GMPI-UI` all say `Using local … folder`. This is
  exactly the asymmetry **X4** says to watch for; I matched Jeff's cache rather
  than "fixing" it, so this run's result reflects his tree, but anyone debugging
  a wrapper-side problem on this box should know the wrapper is not local.
- **The `any` NEXT pointer is A4, and a scheduled run cannot do it.** A4 is a
  path-allowlisted auto-merge *action* — i.e. a file under
  `.github/workflows/**`, which the bot token deliberately cannot write. Noted,
  not acted on: it is not this run's item, and A12 already covers the general
  shape of "the fleet points a box at work it structurally cannot do". Flagging
  it so the next `any`-eligible run does not burn its session discovering it.
- **My PR's lint will be red, pre-existing.** `check-links.py`'s slugger bug is
  **A13**, found by the P7c run; it is already red on `main`. Nothing here
  caused it and fixing it would be a second item.

**Next:**

1. **Merge [#50](https://github.com/JeffMcClintock/TideSynth/pull/50)** (this
   run — docs only, no code in any repo). P6 then flips IN-REVIEW → DONE.
2. **`mac` NEXT moved P6 → E1a**, taking the P7c run's correction at its word:
   E1a's Accept clause is *"one render of both cases on a second platform"*,
   second to linux, so it is mac-or-win work and the linux box can never satisfy
   it. This box can: the render half is a download and two numbers, and it is
   the topmost item that is genuinely mac's rather than anyone's. S9 and S10,
   the only other `mac` rows, both still need Jeff.
3. **A13 is the fallback** if a mac run finds E1a blocked — it is small, it is on
   `scripts/` (a scheduled run may edit it), and it gates A4's usefulness as a
   merge check.
4. **P6's own text should not be re-filed.** It is closed on evidence, and the
   evidence is a positive control rather than an absence of failure.

**Tree hygiene:** nothing was written into either working copy's source. `SE16`
and `TideSynth` were both clean at claim time (`SE16` 5 commits behind
`origin/master`, `TideSynth` 3 behind `origin/main`) and I left `SE16` untouched
and un-updated — no fetch-into-tree, no checkout, no build inside it. All build
output went to a scratch tree under the session's temp dir; Jeff's own
`SynthEdit/build` Xcode tree was not read from, written to, or invalidated.
`git status` in `SE16` is unchanged from how I found it.

**Branch/PR:** `tide/mac/P6-cl-codesign-xcode` —
[#50](https://github.com/JeffMcClintock/TideSynth/pull/50), TideSynth only.
**No code repo was committed in**, so STEP 5's two-end-states rule has exactly
one repo to satisfy.

---

## 2026-08-13 — windows — C4

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Carve-out stage **C4**. Moved the twelve view/browser files out of the
private repo into public `SynthEditLib`, implemented **C9** with them (Jeff's
option (c)), and rebuilt everything including the SynthEdit2 link step no
previous carve-out stage had been able to reach. Also measured what the move
costs C7, which is the part worth reading.

**Result — all three products build, all tests pass, and C9 is proven rather
than asserted.**

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16`, Release | **905/905, RC=0** — `SynthEdit_VST3`, `SynthEdit_GMPI`, `SynthEditCL`, `TIDE.gmpi`, `TIDE_VST3` |
| the six moved TUs really compiled from their new home | log shows `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj` for all six |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | 58/58, 24/24, 10/10 — all RC=0 |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, `x64\Release\SynthEdit2\SynthEdit2.exe` |
| no line-ending churn | `core.autocrlf=false` in both repos; all twelve files `cmp`-identical to their originals |

The SynthEdit2 row matters beyond "it builds": **C1b and C2 both had to leave
link-stage verification of SynthEdit2 open**, because P8 blocked it. P8 is fixed,
so this is the first carve-out stage whose WinUI3 link step was actually reached.

**C9 — implemented as ruled, and the value is injected rather than owned.**

New `SynthEditLib/se_version.h` defaults `SE_APP_BUILD_NUMBER` to 0;
`EditorLib/CMakeLists.txt` injects SynthEdit's real value with a `file(READ)` +
`string(REGEX MATCH)` on `se_build_number.h`, which **stays exactly where
`SynthEdit_cmake_mac.yml:153`, `SynthEdit_cmake_win.yml:206` and
`SynthEdit_store_win.yml:266` grep for it.** No workflow file touched, and none
needed to be — the bot token's missing `workflow` scope never came into it.

I did not give the library its own constant, and the reason is worth recording
because "SynthEditLib gets its own version header" reads like it means that.
**Both uses are cache invalidation, and both track the *application*, not the
library:** `SkinMgr` re-copies skins out of the *application's* own `Resources`
folder, and `ModuleFactory_Editor` caches the modules the *application* links. A
constant `SynthEditLib` owned would never move on a SynthEdit release, so
SynthEdit would silently stop invalidating both caches on upgrade — a behaviour
regression wearing a decoupling's clothes. Injection keeps SynthEdit's behaviour
bit-identical and still leaves the public repo able to compile with no private
header.

Proof, not assertion:

| | |
|---|---|
| `se_build_number.h` today | `SE_BUILD_NUMBER 183` |
| `build.ninja` | `SE_APP_BUILD_NUMBER=183` |
| probe TU, no injection | prints **0** (what a clean clone gets, i.e. TIDE from C7) |
| probe TU, with injection | prints **183** |
| scope | the define appears on **57 build statements, every one of them EditorLib** — `PRIVATE` holds, nothing leaks to TIDE, SynthEditCL or SynthEditLib's own targets |

**Learned — C3's second check has a false-alarm case and a real one, and telling
them apart took a filesystem test, not a grep.** Two of the twelve files use
`#include "../"`:

| include | resolves relative to the file? |
|---|---|
| `ThemeManager.cpp` → `"../tinyxml2/tinyxml2.h"` | **no.** `SE16/tinyxml2/` **does not exist.** It already resolves through a search path — `SynthEditLib/modules/se_sdk3_hosting/../tinyXml2/tinyxml2.h` — so the file's own directory is irrelevant and a move cannot affect it |
| `ModuleFactory_Editor.cpp`, `SkinMgr.cpp` → `"../se_build_number.h"` | **yes.** `SE16/se_build_number.h` exists. This is the one that breaks |

That is exactly the shape C4's row predicted ("most resolve through the search
path and are harmless, and the one that resolves for real is the one that
breaks") — recording it because the `tinyxml2` line *looks* identical to the
`se_build_number` line and would have been "fixed" by anyone pattern-matching on
the `../`. Note also the case difference: the file says `tinyxml2/`, the
directory on disk is `tinyXml2/`. Harmless on Windows and on the search-path
route; worth knowing before anyone moves that directory.

**Learned — the big one. C4 makes the public repo's private-include problem
worse, not better, and I measured it instead of assuming either way.**

I expected the opposite. `SkinMgr.h`, `ModuleFactory_Editor.h` and
`MfcDocPresenter.h` were being included by 11 files already public — the exact
"a public file its own repo cannot compile" shape C2 hit with
`cpu_accumulator.h`. C4 closes all 11. But the six moved `.cpp` bring their own
private-header dependencies with them:

| | dangling private includes in `SynthEditLib` |
|---|---|
| before C4 (`SE16 origin/master` / `SynthEditLib origin/main`) | **47** |
| after C4 | **56** |
| closed by C4 | 11 |
| opened by C4 | 20 |

**Read both sides from git refs, not working trees.** My first attempt computed
"before" with the private-header list taken from the already-modified `SE16`
working tree, so the eleven closures were invisible and the number was wrong in
a direction that flattered the change. The script that does it properly is
`dangling2.py` in the run scratchpad; it is 60 lines and worth re-creating for
C5, which will move the single most-included name on the list (`Application.h`).

18 of the 20 name headers already on `EditorLib/CMakeLists.txt`'s source list, so
C5 and later stages close them by construction. **Two are on no stage's list at
all, and both are reached from `MfcDocPresenter.cpp`:**

- **`SynthEditApp.h`.** Deliberately excluded from EditorLib so each app picks
  its own `SE_MOONBASE_SUPPORT` without ODR conflicts — the CMakeLists says so
  in a comment. `MfcDocPresenter.cpp:1258-1261` declares
  `extern SynthEditApp* theApp` and calls `theApp->isMoonbaseEnabled()` and
  `theApp->licenseIsActive()` to gray out "Selection to Prefab". **So a
  licence-gate call site is now in the public repo.** No Moonbase implementation
  is published — two method names and the existence of the gate are. This is the
  first time the carve-out has put licensing-adjacent code on the public side,
  and the standing direction is to keep commercial code private as practical.
- **`ModulePicker.h`.** 19 KB, header-only, no `.cpp`, on no stage's list.
  `MfcDocPresenter.cpp:536` does `return new ModulePicker(pparent)`.

**Filed as C11, and deliberately not acted on.** C4's file list is Jeff's and
`MfcDocPresenter` is on it; reshaping an approved stage because one of its files
turned out to be awkward is not a scheduled run's call. It is flagged at the top
of both code PR bodies so it cannot be merged without being seen. Neither breaks
anything today — both resolve through EditorLib's `../SynthEdit2` include path —
they break at **C7**, whose whole test is a clean clone with no access to SE16.

**Learned — `MSBuild SynthEditStore.sln` now needs the VS 2026 (v145) toolset,
and P8's recorded command no longer works on this box.** Under VS 2022 it dies at
`build\ZERO_CHECK.vcxproj` with `MSB8020: The build tools for v145
(Platform Toolset = 'v145') cannot be found`, **before reaching `SynthEdit2` at
all** — so it reads like a project failure and is not one. The solution
references two projects inside `SE16\build`, Jeff's CMake-generated VS tree,
which is now generated for VS 2026. Building through
`Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat` succeeds. Both VS
2022 Community and VS 18 Community are installed here; Ninja + `cl` from either
is fine for the CMake side, it is only the `.sln` that is pinned.

**Also done as STEP 4 chores:** **P7c** flipped IN-REVIEW → DONE and moved
verbatim to `BACKLOG-DONE.md` — [gmpi_ui#5](https://github.com/JeffMcClintock/gmpi_ui/pull/5)
and [#48](https://github.com/JeffMcClintock/TideSynth/pull/48) both merged
2026-08-13. **C9's row grew** to record that its mechanism now exists and is
proven, so C5 reuses it with one `#include` swap and one macro rename rather than
building anything; it stays TODO because `Application.cpp` has not been done.
**The `win` NEXT row** now points at C5-if-merged, else E1a — the linux run
recommended exactly that E1a move on 2026-08-13 and could not make it from its
own row.

**STEP 1 / 1.5:** no `platform:win` issues; no `platform:*` labelled issues at
all across the five repos. The only open issues anywhere are TideSynth
[#44](https://github.com/JeffMcClintock/TideSynth/issues/44) (the A6 watchdog
digest, `github-actions`, informational) and `gmpi_ui#1` ("Linux support?", 2024,
third-party and unlabelled) — noted, not acted on, per the issue-authenticity
rule. **Zero open PRs in all five repos** at claim time, so nothing was handed
back to this platform and nothing was in flight to collide with.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches** at claim time. Nothing of his was committed, reverted or
stashed. Note `SE16` local `master` was two commits behind `origin/master`; I
branched from `origin/master`, as the prompt requires, so the `[Build-Machine]`
bump to build 183 is included — which is why C9's injected value reads 183 and
not the 182 in the stale local tree.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** a scratch Ninja tree and a probe binary under the
session scratchpad, both outside every repo — Jeff's own `SE16\build` was not
configured or built into. The MSBuild run **did** write into `SE16\x64\Release\`,
which is `.gitignore`d (`.gitignore:13`) and is where that build has always put
its output; `git status` in `SE16` is clean apart from my own commit. No engine
state was written: `SynthEditCL.exe` was built but never executed, so no module
cache or skin folder was created or invalidated on this machine.

**Next:**

1. **Merge all three C4 PRs together** —
   [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15),
   [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6),
   [#49](https://github.com/JeffMcClintock/TideSynth/pull/49). Unlike P7c's
   independent pair, **merging any one alone breaks the build**: the files exist
   in exactly one repo at a time and `EditorLib/CMakeLists.txt` points at the new
   location.
2. **Rule C11 before C7, and it needs Jeff.** The `SynthEditApp.h` half is a
   boundary decision, not work — the call is two bools behind a pointer, so a
   small interface hook would do it, but whether licensing-adjacent code may sit
   in the public repo at all is not an agent's call. `ModulePicker.h` is the easy
   half and probably just joins a stage's list.
3. **C5 is cheap now.** C9's mechanism shipped with C4, so `Application.cpp` is
   one `#include` swap and one macro rename. It is `win`, and it is the
   carve-out's critical path the moment C4 lands.
4. **A13 is still open and still red on `main`.** I did not hit it as a blocker
   this run, but the link checker's em-dash slug bug is unchanged and every
   heading in this file has em-dashes.

**Branch/PR:** `tide/win/C4-move-views-browsers` in three repos —
[SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15) (deletions,
`EditorLib/CMakeLists.txt`, vcxproj),
[SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6) (the
twelve files + `se_version.h`),
[#49](https://github.com/JeffMcClintock/TideSynth/pull/49) (BACKLOG, JOURNAL,
`docs/carve-out.md`). No other repo was committed in or modified.

---
