# Lessons — the standing digest

**Every run reads this. Rotation cannot age it out — that is the whole point.**

`JOURNAL.md` is rotated to stay under 60 KB (**A8**, floored by **A24**), and no
run reads `JOURNAL-<YYYY>-<MM>.md` — the prompt's reading list names the live
journal only. So until **A30** a lesson was load-bearing for about a day and then
silently stopped being read, while the entry that carried it sat in an archive
nobody opens.

**This file is GENERATED. Do not hand-edit it** — run
`python3 scripts/extract-lessons.py --write`. It is rebuilt from both journal
files, so it cannot drift from them, and adding a lesson means writing a
**Learned** bullet in your entry exactly as you already do.

**One line per bullet: the claim, not its working.** The journal's convention is
that each Learned bullet opens with a bold claim and then argues it; this keeps
the claim and drops the argument. Measured when this file was last written: the
Learned sections are **337 KB** across **263** entries, so copying them
whole into a file every run reads would be worse than the 192 KB that triggered
A8. This is **96 KB / 1063 lessons — 3.5x smaller**, and represents
**every** entry that has a lesson, none dropped.

**To read the working**, find the entry by its date and machine — in
[JOURNAL.md](../JOURNAL.md) if recent, else
[JOURNAL-2026-08.md](../JOURNAL-2026-08.md).

**This file GROWS, and someone will have to prune it.** ~4 lessons an entry at
~90 bytes is ~360 bytes per entry, and this fleet writes ~10 entries a day —
roughly **3.5 KB a day, ~100 KB a month**. Affordable today at 56 KB; not by
October. Stated here rather than discovered later, because that is exactly how
A8 happened.

**Pruning means fixing the SOURCE, never editing this file or the archive.** It
is regenerated from the journals, so a lesson that no longer holds is corrected
by a newer entry saying so. The lever nobody has pulled yet: drop `SOURCES`'
archive file once its lessons are genuinely spent, which halves this at a
stroke — that is a judgement call and belongs to Jeff, not to a run.

## 2026-08-27

**macos — R5 shipped a day before its row said so, and R6 was blocked behind an ask nobody still owed (interactive, Jeff directing)**

- "Am I synced?" has two answers and only one of them is `git`.
- A PR that satisfies a row must move the row.
- Three rows in 24 hours (E32, X2, R5) were found saying something their own merged PRs had made false.
- An artifact can answer a question about a credential you may not look at.
- When a row's blocker is one HTTP status, spend the one command.

**windows — E19's VST3 cell is still not measured, and what stopped it is three defects upstream of REAPER (scheduled run)**

- A deadline needs a connection to answer on.
- `Responding=True` with 0.09 s of CPU is the modal-dialog signature
- Rule a nearby defect out by ITS stated condition, not by impression.
- A control has to be verified, not assembled.
- The command channel cannot lay out a rack.
- `-ErrorAction SilentlyContinue` turns "I was not allowed to look" into "it is not there."
- Check a lint by its EXIT CODE, not by reading the first lines of its output.

**linux — X2: the tree has 1,982 unique warnings and fifteen of them are ours (interactive, Jeff directing)**

- Count unique warnings, not warning lines.
- Bucket by repo before planning.
- An unused variable can be a live one in another build configuration.
- A comment claiming a consumer is evidence, and it can be stale.
- Two of three "blocked" linux rows were not blocked.

**macos — E43: the command channel now always answers, and the fix is a heartbeat rather than a list of verbs (scheduled run)**

- A deadline on "did it start" cannot bound "did it come back", and the two failures are indistinguishable from outside.
- Invert an allowlist when the unknown case is the dangerous one.
- A bounded error must not reuse a wording that was true of a different failure.
- When the app cannot exercise the path, the probe is the deliverable — and it is only worth anything once you have made it fail.
- Check with the regex the tool uses, not one that looks equivalent.
- STEP 4's archive move is a written rule that nothing enforces, and it stopped happening six days ago without anyone noticing.

## 2026-08-18

**windows — the Marathon design language, researched from source and revised live by Jeff (interactive session, Jeff directing)**

- The live site is a far better source than any article.
- A rule stated as a ban will outlaw something the source actually does.
- Jeff's corrections were consistently about *reading over hours*, not about looking right in a screenshot
- as uncommitted

**windows — the first competitive review, a module set, and a false belief corrected (interactive session, Jeff directing)**

- A9's standing hypothesis is FALSE as written and two docs reason from it.
- V2 has a settled shape and a documented trap, both from precedent.
- The module set is not a DSP job.
- AUv3 extensions are memory-capped at ~300 MB (32-bit) / ~360 MB (64-bit) per instance

## 2026-08-13

**windows — A11, win half (interactive session, Jeff directing)**

- The fleet's "5 repos" framing (used everywhere A2 discusses scope) undercounts what's actually on disk — Windows alone has 22 local git repos under `C:\SE`, most unrelated to TIDE (SE15, SSG, Waves, and other dormant product repos)

**linux — A11 (new; A2 follow-up, interactive session, Jeff directing)**

- three things, and the second is the one that matters
- A credential helper keyed to `credential.https://github.com.helper` is never consulted for a `git@github.com:` URL.
- Both guards A2 rests on are blind to it, including the one used to close it.
- Authorship proves authorship, not authentication.
- The macOS evidence A2 cited — `gmpi_ui#3`/`#4`, `GMPI_Wrappers#1`/`#2` — is drawn entirely from repos that were SSH on this box
- A stale local `main` makes the bot's first push fail with an error about a file you did not touch.

**jeff — decision: rack mode is TIDE's default view (interactive session, not a scheduled run)**

- The underlying SynthEdit feature already exists — `SE16` `a056d3f5b chore(se) : experimental eurorack 'rack mode' for the panel view`, from earlier this same day — so this ruling is catching up to code already landing, not speculating ahead of it

**windows — C3**

- C2's "nothing outside EditorLib compiles it" test caught exactly one file, and it was not obvious.
- The `#include "../` check needs a second step C2's note did not state.
- `se_build_number.h` blocks C4 and C5 — filed as C9.
- Root vs subfolder: chose root, deliberately, and filed the re-home as C10 blocked on C6.
- The two repos normalise line endings differently, so blob comparison across them is worthless.
- P3 partly moved out from under itself.

**macos — P7b**

- AddressSanitizer cannot see this, and an ASan-only run is a confident false PASS.
- The measurement bug from the P7 entry recurred, in the same shape.
- The whole test needs no CMake, no VST3 and no plugin.
- `GraphicsContext` caching `cgContext_` is a general hazard, not a P7b detail.

## 2026-08-12

**windows — A8 (C3 not taken — see below)**

- Lifting a row from a root-level file into `docs/` silently breaks its relative links.
- The rotation rule needs a floor, and the floor has to win.
- A grooming item conflicts with every open PR by construction
- The `## Blocked on Jeff` section held nothing blocked on Jeff
- STEP 0.5 requires an app version this box does not expose.

**macos — P7a**

- "Copy the Windows clamp" was the trap the row warned about, and it is worse than the row says.
- The backing scale is the whole reason a points-based bound needs a byte budget.
- The X3 trap is still live in `~/Documents/GitHub/SynthEdit/build`.
- A before/after that reproduces the prior run's number is worth building.
- `gh pr create --base <branch>` for a stacked PR works as the bot

## 2026-08-11

**linux — E1**

- (d), and it is the significant one: `--modules` is not authoritative on a developer box
- freshly isolated `HOME`
- (e): the two null tolerances contradict each other
- false of the RMS gate two lines above it
- ~10.7% of samples
- Learned — finding (c) is stronger than it was.
- a relative `--render-audio` path does not land in the CWD
- It resolves against `$HOME`: `--render-audio rel.wav` reported `"resolvedPath":"/home/jef/rel.wav"` and wrote there

**macos — C8 executed (interactive session, Jeff directing)**

- The gate was deliberately not widened.
- `gh pr edit` does not work as the bot.
- The #31/#32 `JOURNAL.md` conflict predicted in #32 was real and Jeff resolved it cleanly

**macos — C8**

- `SynthEditLib` does not build any of C2's 16 files.
- So the C8 row's "public API surface" overstates the case.
- A CMake source list is an inventory, not a dependency graph.
- Why I stopped short of deleting, since it is the arguable part.
- Eligibility and authority are different questions, and the prompt is consistent about it.

## 2026-08-10

**macos — P7**

- The port's danger was not the crash, it was the liveness probe.
- And the resize allocates nothing, so `onSize` alone tests nothing.
- I produced a tidy wrong finding and caught it; the catch is the lesson.
- `cacheDisplayInRect:toBitmap:` does not exist.
- This box's SynthEdit build tree has the X3 trap live in it.
- `GMPI-plugins` cannot link a GUI plugin on macOS at all
- the environment moved under this session, and the reflog is how you find out
- A pushed branch is not the same commit as what lands.
- `origin/tide/win/C2-leaf-files` is now a stale branch in `SE16` with no PR

## 2026-08-09

**windows — A1 done, A2 mostly done (interactive session with Jeff)**

- two GitHub platform limits that A2 as written did not know about, both permanent
- A fine-grained PAT cannot serve a collaborator on repos they do not own.
- `workflow` scope was withheld on purpose
- Private repos cannot have rulesets without GitHub Pro.
- `~DEFAULT_BRANCH` is the right targeting primitive
- Required approvals is 0 by choice.

**windows — process review adopted (interactive session with Jeff)**

- The review's three sharpest facts, each verified against the live repos: the CI→platform-issue loop has never once executed (read-only token, 403 under continue-on-error, zero issues ever); "green CI" is currently meaningless (continue-on-error at job level); and the 8 signing secrets are reachable from any workflow edit on any `tide/**` branch — but no workflow references them yet, so A1 is free to do now
- The stated human/AI split is implemented inverted
- The red team killed four plausible recommendations (cloud/Actions as agent hosts, merge queues, dependency-bot policy, encrypted-secrets-in-repo)
- Both G4 and G5 landed while this review was being written, and both PRs reached `main` before this one did — from two different bases, since G4 and G5 were claimed from the same pre-review commit.

**macos — G4**

- The staleness was real, it was aimed at a `mac` item specifically, and this run would have walked into it.
- The collision check saved this run, for the second week running.
- `update_scheduled_task` is a partial update, and that is the safe property to lean on.
- A `~` path inside `git -C` is shell-expanded, so it works — but prove it per box.
- This closes the state table in `agent-setup.md`, and that is the actual deliverable.

**linux — G5**

- The staleness was not theoretical, and this run walked into it.
- `agent-setup.md` was wrong about this box, and overstated the damage.
- The install looks wrong twice when it is right.
- Paste STEP 0's commands into a shell before believing them.
- `git show origin/main:<path>` genuinely works from a parked branch
- The prompt version stamp works as designed.

**windows — C2 landed, tide-rack archived (state update, interactive)**

- Leftover branches named `tide/win/C2-leaf-files` still exist locally in `SE16`, `SynthEditLib` and TideSynth, and on TideSynth's origin

**windows — the prompt is fetched, not copied (interactive session with Jeff)**

- Read the prompt from `origin/main`, never the working tree.
- The blob sha is the version stamp.
- The trade is blast radius for visibility, and it is the right way round.
- A bootstrap cannot install itself.
- twice in four days

**windows — two end states, never a third (interactive session with Jeff)**

- `SE16`'s default branch is `master`. The other four are `main`.
- The gap was in the prompt, not in the run that hit it.
- Windows was stale on G3 and the table said it was current.
- The task is named `tide-synth-weekly-windows`, with a hyphen.

**windows — Eurorack ruling (interactive session with Jeff, not a scheduled run)**

- Do not re-file `tide-rack`'s backlog.
- The salvage is the harness, and it is worth taking seriously.
- That repo's own "Awaiting Jeff" had already asked this question
- Minor, but it will confuse a grep: the repo slug `tide-rack` and the product name `TIDE Rack` are now different things

## 2026-08-08

**windows — C2**

- One file did break on the move, and it was the one that looked cleanest.
- The public repo already had a source dependency on a private header.
- `file(GLOB)` was checked for, deliberately.
- `it_empty.h` has zero includers anywhere in the tree
- C1b's "three build systems" lesson paid again, and cheaply.
- Do not build `SynthEdit2.vcxproj` standalone to test it.

**linux — H1 (interactive session with Jeff, not the scheduled run)**

- things that will cost the next person an hour each
- The Domainz DNS editor is at `/~/dns` and nothing links to it.
- "Export Zone" is server-side broken — 500 every time.
- The domain has live email, and that was the real risk.
- Their nameservers propagate inconsistently, and briefly lie.
- Local resolver cache made the finished site look broken.
- `https_enforced` gets set back to `false` when you save a custom domain,
- The `website/CNAME` file is load-bearing, and now proven.

**linux — X3**

- Why `GIT_TAG origin/main` freezes, precisely.
- Chose the pin over the two alternatives, for a reason.
- The bump carries a Windows fix, not a Windows risk.
- `/usr/bin/cmake` on this box is 3.28.3 and cannot configure this tree
- A prebuilt Steinberg `validator` is on this box
- P5 confirmed on Linux, in passing.

**macos — S1b (partial: the ALLOWED part; the rest is gated)**

- `nm | c++filt` on the built plugin is the right verifier for this class of item, and it is nearly free.
- "Do the TIDE-side part" is a real instruction, but you have to go looking for the TIDE-side part.
- A1's soundcard prediction is confirmed, and it is worse than predicted.
- `CSynthEditAppBase::MonitorFileSystem` survives my change
- SynthEditCL does not build on macOS
- My working copy was 48 commits stale and sitting on an already-merged branch

**windows — C1b (interactive session, Jeff directing)**

- "Who consumes this library" has three answers in SE16, not one.
- A dependency path can be stale-but-inert until your change makes it load-bearing.
- P8, found in passing and A/B-confirmed pre-existing
- The claim-first discipline held even interactively

**jeff — decisions: queue order, artifact naming, X4 closed (interactive session, not a scheduled run)**

- no bare "TIDE" is left in the visible copy, and that is a rule now, not a tidy-up.

**jeff — decision: the product is TIDE Rack; donation link live (interactive session, not a scheduled run)**

- Verify the handle resolves before committing the href — every time.
- The Ko-fi page title is "Support Jef", not TIDE Rack.
- `.github/FUNDING.yml` was deliberately withheld until the handle existed.
- The rename is not a search-and-replace, and N1 says so in bold.
- Settle the release asset names before R2–R6 ship anything.

**jeff — decision: carve-out APPROVED (interactive session, not a scheduled run)**

- the direction is not a restatement of the existing boundary, and checking it found a hole in the plan
- The carve-out plan, followed literally, would have published the export implementation.
- TIDE does not ship the export code today — verified, not assumed.
- `SynthEditCL` stays private
- A doc correction worth not rediscovering
- Stale framing removed rather than left to mislead.

**windows — P4c**

- five things, and the first two are the ones that matter
- P4c asked the wrong question, and answering it as asked would have failed again.
- P2's `MoveWindow` lead was a red herring — and it nearly cost this run too.
- A "did not crash" result needs a liveness proof, or it is worthless.
- This is the same discipline as S1a's screenshot hash — build the verifier so it can *fail*, not just so it can pass
- The Release PDB from P4 paid for itself immediately.
- A/B by reverting the file with git, not by editing the fix out.

## 2026-08-07

**windows — distribution plan (at Jeff's request, interactive)**

- SynthEdit's shipping infrastructure, located by reading `SE16`
- Windows signing is Azure Trusted Signing and already paid for
- Apple identity + DMG/notarization pipeline exist
- Inno Setup is the Windows installer precedent
- All of it runs in Azure Pipelines in the private repo.

**windows — S1a**

- S4 verified at runtime on Windows, in passing.
- S7's write confirmed at runtime, with a nuance.
- `Plugin-Cache-16.xml` was rewritten today by another agent, mostly.
- `Set-Content -NoNewline` on a line array concatenates the lines.
- Screenshot comparison is a strong, cheap verifier

**jeff — decision: no user skins (interactive session, not a scheduled run)**

- the ruling names a live behaviour, not a hypothetical
- recursively copies the built-in skins there on first use

**linux — S4**

- things the next run should not have to rediscover
- TIDE already builds on Linux, and X1 is stale in one direction.
- X3: the Linux VST3 cannot be loaded by any host, and it is a stale `FetchContent`, not a bug.
- A negative result, so nobody chases it: TIDE does *not* scan the user's Documents folder.
- The cache is only *written* when it is missing or stale.
- `TideApp.cpp:109` still hard-codes `L"modules\\"` and I deliberately left it.
- `isSemFolderOverridden` is named for its caller, not its effect.
- This machine's installed task is still the pre-G3 prompt.

**windows — P4**

- This machine *does* have `cdb.exe`, and P2's journal is wrong about that.
- Two cdb flags are the difference between an answer and a wall of noise.
- The Debug artifacts from P1 are still on disk and still match.
- `dv` in the Debug dump gives the host's actual arguments, and they are absurd.
- The bug is visible in the source once you know where to look, and the codebase already knows about it.
- A minidump gives you locals but not the whole object.

**windows — L1 + H1 resolved, C1 done (Jeff's decisions, executed same run)**

- "MIT" and "same as my other repos" are different answers; ask which one is meant when they conflict.
- GitHub's licence badge lags.
- PR #12 appeared mid-run from another machine, claiming S4.

**windows — W1 (same run, at Jeff's request)**

- synthedit.com is not on Netlify, despite `netlify.toml` in its repo root.
- Do not put TIDE at `synthedit.com/tide/`.
- G1 resolved mid-item and changed the answer.
- Public is not open source, and TIDE is now the second repo in that trap.

**windows — push + branch cleanup (same run, at Jeff's request)**

- the correction, and it is the useful part of this entry
- A dirty tree is not necessarily work.
- Revert churn; never stash and restore it.
- Check the PR you are adding to is still open.
- Do not delete other sessions' branches.

**windows — P4a + P4b (same run, continued after Jeff lifted the scope block)**

- Do the A/B before claiming a fix works.
- Disable the *whole* fix when you A/B.
- PowerShell tool state does not persist between calls.
- `reSize` is Windows-only.
- `OnSize` still has the over-limit weakness
- Staging discipline in the shared repos.

## 2026-08-06

**jeff — decision: free, donation-supported (manual, not a scheduled run)**

- why the plugin side is a design note and not a task
- Two existing constraints delete most of the obvious answers.
- The remaining answer may not work on the platform that matters.
- Free is not open source, and this decision does not touch L1.
- The website side is the easy half and should not wait.

**windows — P2**

- A portable REAPER is the right harness for this, and it takes one copy command.
- Drive it from `Scripts/__startup.lua`, not from the mouse.
- Screenshots and window control need no MCP.
- How to prove a crash is caused by what you think it is.
- `MoveWindow` on the plugin window does not resize it
- The Release configuration produces no PDB.
- Windows kept full minidumps
- `GetHomeDir()` has no trailing separator.
- The module cache filename does not distinguish TIDE from SynthEdit.
- Reordering note

**windows — P1**

- A clean configure does NOT work with default settings, and the error looks like something else entirely.
- Do not debug this by building the failing `.vcxproj` directly — it lies.
- Two files in the carve-out set require MFC on Windows.
- Building `--target TIDE TIDE_VST3` pulls in only SynthEditLib, EditorLib and HarfBuzz, not SynthEditCL or the test suite
- Even with all four `*_FOLDER_OVERRIDE` variables pointed at local clones, a fresh configure still hits the network for the VST3 SDK and HarfBuzz (CPM, cached in `%USERPROFILE%\.cpm`) and for CLAP + clap-helpers (FetchContent, into the build tree, so re-downloaded per build tree)

**jeff — decision: fixed module set (manual, not a scheduled run)**

- the durable channels into a memoryless run are PLAN.md (rulings), BACKLOG.md (queue) and JOURNAL.md (reasoning)

**macos — S1 (duplicate run — see "The collision" below)**

- additive to the Linux entry, not repeating it
- The `INIT_STATIC_FILE` list is three regions, not two, and this changes the §9 prediction.
- The "third, explicit list" the Linux note asks for already has a hook.
- The metadata half has a shipping mechanism too.
- `-DSE_EXTERNAL_SEM_SUPPORT=0` will not work.
- `SE16/SE_IOS_APP/TIDE/Plugins/` is a decoy — do not try to make it load.
- One scan root that stage 1 will not remove.
- Where the trees are on the Mac

**linux — S1**

- The Linux box has a full copy of the source tree
- The static-registration mechanism S1 was asked to design already exists.
- The module browser never touches the filesystem.
- Separate the two iOS prohibitions or you will over-scope.
- Trap for S1a/stage 3
- Two real bugs found in passing, filed not fixed
- S5: `TideApp::InitInstance` never calls `CSynthEditAppBase::InitInstance`, so `refreshFolderLocations()` never runs, `m_folder_settings` is empty, and `getFolderInfo` (`Application.cpp:167`) indexes `[0]` on an empty vector
- A third, cosmetic: `TideApp.cpp:109` hard-codes `L"modules\\"`, which on macOS/Linux names a directory ending in a literal backslash
- Process note

**windows — project setup (manual session, not a scheduled run)**

- TIDE is not a greenfield project
- The blocker for open-sourcing is `EditorLib`, which lives in the private `SynthEdit` repo
- The commercial boundary is cleaner than expected
- Moonbase licensing is already outside `EditorLib` by deliberate design (see the comment at `EditorLib/CMakeLists.txt:179`)

## 2026-08-13

**macos — A11, mac half — halted at STEP 0.7, then resolved in session (part 1 of 2)**

- the finding that matters, and it is new

**macos — S6 (part 2 of 2)**

- the big one, and it is much larger than S6
- `database.se.xml` is the same architecture constraint 7 forbids.
- `gh pr edit` fails with the bot's token; `gh api ... -X PATCH` does not.

**linux — P7c (E1a not taken — see below)**

- the reachability question the row asked is answerable, and the answer is no
- None expose `processEvents`, `onTimer`, `reSize`, or the `Display*`
- `reSize()` alone cannot cause this.
- AddressSanitizer is a false-negative machine on this path, for a brand-new reason
- ASan does not instrument a shm segment.
- work out who owns the memory before choosing the detector.
- Learned — two of this row's premises were wrong, and one changed the approach.
- not in `gmpi_ui`
- `GMPI_Wrappers`
- positive control for the guard
- A3's link check is red on `main` right now, and it is the checker that is wrong, not the link. Filed as A13
- Every entry heading in this journal uses em-dashes.
- It is wrong in both directions
- Learned — the Platform vocabulary cannot express "any box except linux", and the backlog lint is right to stop you inventing one.
- the correction goes in prose; the column is Jeff's to change.

**windows — C4**

- C3's second check has a false-alarm case and a real one, and telling them apart took a filesystem test, not a grep
- does not exist.
- Learned — the big one. C4 makes the public repo's private-include problem worse, not better, and I measured it instead of assuming either way.
- `MSBuild SynthEditStore.sln` now needs the VS 2026 (v145) toolset, and P8's recorded command no longer works on this box
- before reaching `SynthEdit2` at all

## 2026-08-14

**macos — P6**

- Any signing-shaped question on mac must be answered with `-G Xcode`.
- A bundle-level A/B is enough to prove a staging-path fix, and it needs no source edit.
- `GMPI_WRAPPER_FOLDER_OVERRIDE` is empty in Jeff's `build/CMakeCache.txt`
- The `any` NEXT pointer is A4, and a scheduled run cannot do it.
- My PR's lint will be red, pre-existing.

**windows — A13 (C4 not re-taken, A4 not takeable — see below)**

- `/markdown` is not an oracle.
- The em-dash convention and the checker were on a collision course from the start.
- The app version STEP 0.5 asks for is not discoverable on this box.
- Two lint scripts cannot be fed a process substitution on this box.

**windows — merge cleanup for A13/P6/C4 (interactive session, Jeff directing)**

- A same-day multi-branch queue racing the same rotation files will always produce this shape of conflict
- `gh run rerun --failed` is the right tool when a cross-repo CI failure's cause has already been fixed by merging the other repo

**macos — E1a**

- `SynthEditCL_mac.zip` at `https://www.synthedit.com/release_1_6/` is a working download for this harness
- Finding (c) extends and slightly retracts.
- A residual that grows through the render is diagnostic on its own.
- The app version STEP 0.5 asks for IS discoverable on the mac desktop app

**linux — S2 (plus a platform:linux build break found and filed)**

- The GATED wall is the `any` lane's real filter, and it is invisible in the rows.
- A build tree you did not make is a first-class measuring instrument.
- `readelf --debug-dump=info --dwarf-depth=1` is the cheap way to get the linked compile-unit list
- The skin-version stamp becomes a thrash bug at C7, and does not look like one today.
- `std::remove` and `Processor::open(phost)` are the two false positives any filesystem grep of this codebase will hit

**linux — #53 fixed; S2 landed (interactive session, Jeff directing)**

- A platform-gated `return()` above a `REQUIRED` probe hides the probe from two of three platforms.
- Check for consumers before deciding between "make it optional" and "make it opt-in".
- Resolve-then-generate, never resolve-while-generating.

**linux — A4 built (interactive session, Jeff directing)**

- A path allowlist ages badly and silently.
- Strict inclusion, never exclusion.
- `--auto` is not "merge when checks pass" unless a ruleset says which checks.

**windows — C5**

- C5 is the first stage to pay debt down, and both halves were measured from git refs
- the big one. The carve-out's stage list does not cover the files it says it does, and C7 cannot pass until something does
- 41 `${EDITOR_DIR}` entries
- C7's whole test is a clean clone with no access to SE16
- Learned — `cmd.exe /c` is unusable from the Bash tool on this box, and it fails by hanging rather than erroring.
- Learned — the C7 cache-thrash problem S2 found for skins now has a third instance, and it is the same mechanism.
- Learned — no `.vcxproj` or Xcode edit was needed, unlike C3 and C4, and that is worth checking rather than assuming.

**windows — C12 (scoping session)**

- four of the 41 are dead or duplicate, so C12 is 37 files, not 41
- `Module_Info_Plugin.{h,cpp}`
- `resource.h`
- `Dialogs_editor2.cpp` links by accident, and it bears on S3
- empty bodies
- TIDE links `EditorLib`, which contains `Dialogs_editor2.obj`, *and* defines the same three symbols in `TideApp.cpp`.
- the patch cluster is irreducible, and it is 64% of C12
- Learned, in passing, and it changes B1's premise — `build.yml` is not failing for the reason it says it is.
- `TideSynth` has no top-level `CMakeLists.txt` at all
- authoring TIDE's top-level CMake

## 2026-08-15

**windows — C12a**

- all three delisting claims verified, and the checks are cheap enough that no future stage should skip them
- `Module_Info_Plugin`

**windows — C12b (and a second-agent collision worth more than the item)**

- `PatchManager.cpp` was resolving two of these headers from its own directory, and C12f inherits that
- it is the one own-directory resolution C12b disturbed, and it was found by grepping for it rather than by anything failing.
- absolute dangling counts are not comparable across runs, only deltas within one script
- So the script is now committed

**windows — A10 (script half; A15 filed for the gated half)**

- `BLOCKED(<id>)` is the highest-value case and A10's row never named it
- eligibility lives in the status column alone
- Learned — the check has real coverage, which was not obvious in advance.
- 447 references examined across 91 distinct IDs
- Learned — stacked PRs are a genuine false-positive source, and this run hit it within minutes.
- stacked on `tide/win/C12b-controls` rather than `origin/main`
- Whoever reviews: this PR's base is the C12b branch and GitHub will retarget it to `main` when C12b merges.

**windows — A14 (the guard for this morning's collision)**

- why the check belongs *before* the push and nowhere else
- Learned — the guard STEP 0.7 gives is narrower than it reads.

**windows — P9**

- excluding the `APSTUDIO_INVOKED` block is what makes this check survivable, and it is not a detail
- red from birth and red forever
- three resource slots allocated that the public one has not
- Learned — this closes a loop under C12a rather than sitting beside it.

**windows — A15 (interactive session, Jeff directing)**

- the Summary wiring is the part that would have silently rotted, and A15's row was right to insist on it
- Learned — this check is deliberately not diff-based, unlike the four above it, and the workflow now carries a comment saying so.
- renamed or archived

**windows — C12e (interactive session, Jeff ruling)**

- a `*_FOLDER_OVERRIDE` build reads a live working tree, so another session's uncommitted work lands in your test results
- Learned — A9 has been listing a NEEDS-JEFF that PLAN.md already answered.
- "What TIDE Rack is"
- eight design constraints
- A9 needs nothing from Jeff to start.

**windows — C11, S9, S10, M2 (interactive session, Jeff ruling)**

- TIDE's stub was already correct in behaviour and I nearly made it correct in behaviour for the wrong reason
- Learned — `isLicensed()` had to be non-`const` in the interface, and that's not cosmetic.

## 2026-08-16

**macos — D1**

- a `success` return from `openURL` does not mean the URL arrived, and this cost an hour
- the marker never appeared
- Anyone re-testing this must check the far end, not the return value.
- Learned — the mac box has no mac-only work left that it may actually do.
- Learned — check the ID column before filing a new row.
- a D2 already existed
- A4's auto-merge is live now, and it can merge your PR out from under you before STEP 4 finishes. This run hit it, and the next one will too
- The journal entry missed the merge
- A placeholder reached `main`.

**macos — D2**

- a raw grep of `website/index.html` finds `#donate-url-tbd`, and it is a false alarm both README and page comments will make you doubt
- only inside an HTML comment
- Learned — `curl` cannot tell you whether a Ko-fi handle exists.
- 403 to any user-agent it dislikes, including for handles that plainly do not exist

**macos — A9**

- the fix for (3) is that the watch has to SEARCH, and searching needs one more correction than it looks like
- Requiring the match in the topic TITLE cut 54 hits to 17, all on-topic
- do not predict your own PR number, even to avoid a placeholder
- The rule that actually works: push STEP 4 first, open the PR, then correct the number in a follow-up commit on the same branch.

**macos — P5**

- the mac build tree was stale in a way that looks like your own breakage, and the fix is one command
- generated Xcode project
- Any mac run touching this tree after a carve-out stage should expect this and re-run `cmake .` before believing a build failure is theirs.
- Learned — `getVendor4charCode()` is unaffected, checked rather than assumed.

**macos — U1**

- "the class exists" and "the class ships" are different claims, and only one was true. Measured with both controls
- the linker never extracts that archive member
- two of the P2-era findings survive the pivot, and the visual ones could not be re-measured
- need a running host and this audit did not run one.

**macos — U1a**

- the interesting number is the one that went to zero, not the one that went to thirteen
- the structure view is currently unreachable
- Nobody would have noticed this from the diff
- Learned — it was four files, and typing the interface to the base is what makes it the last time.
- The one thing the compiler did catch

**macos — U1a·P5 host verification (interactive session, Jeff present)**

- REAPER's plug-in cache ini flushes on exit, not on scan
- Learned — `REAPER -nonewinst <script.lua>` runs a ReaScript inside the already-running instance
- Learned — the a2 doc's macOS caveat is settled.

**windows — U1a·P5·U2 host verification, Windows half (interactive session, Jeff present)**

- the mac entry's cache-flush rule is macOS's, not REAPER's
- at scan time
- Learned — building `TIDE_VST3` alone ships a plug-in that cannot build its DSP graph. Filed as P11.
- "Export failed: required module is missing from the module database"
- The error blames the user's install
- U2's (3) survives the fix

**macos — U2 triage + U2a wheel fix (interactive session, Jeff present)**

- a host keeps a VST3 module mapped after FX-remove
- Learned — correcting this morning's entry
- not a live mirror at all

**macos — U2c fix + U2d falsifier (interactive session, Jeff present)**

- a latent trap for whoever ships skins with TIDE
- `Contents/MacOS/Resources/`

**macos — U2b + U2d fix session: first modern panel renders (interactive session, Jeff present)**

- the module-set list is the constraint-7 lever
- Learned — U2b is code-complete but untested by hand

**macos — U2e first pass: crash-free placeholders, one question left (interactive session, Jeff present)**

- pin defaults argue the diagnosis for us
- When a handler's absence can be inferred from what default values *would* have built, the "is it invoked at all vs does it fail inside" fork resolves without instrumentation

## 2026-08-17

**macos — U2e: the pin-delivery trace, completed (scheduled run, unattended)**

- three things the next run would otherwise redo
- The U2e row's skin-bitmap/ImageCache suspicion is not the gate
- Pin IDs are fine
- This is a wall in front of TIDE's whole fixed module set, not one broken control.

**macos — U2e closed on mac: the combo box draws (interactive session, Jeff directing)**

- the browser does NOT filter unavailable internal modules
- `ExportModuleNames` skips `!isDllAvailable()`, but `Module_Info3_internal` never sets that flag false for XML-only entries (the assignment in `RegisterPluginConstructor` is commented out as "might be needed?")

**linux — S3 (TIDE-side half), plus two platform:linux breaks found and filed**

- A `#include` inside a namespace is a platform-dependent time bomb, and the guard is what hides it.
- "Loud in release" has a narrow menu in a plugin.
- A stub is not evidence that a path is guarded.
- Reading a shared working tree read-only has a limit.

**macos — U1b complete: two depths, both directions (interactive session, Jeff directing)**

- a false negative that cost an hour, and the tell
- Instrumenting `TideApp::OpenView` settled it in one build
- Jeff's rack machinery is right there, and U1c should start from it
- `POPUP_MENU_TOGGLE_RACKMODULE`
- `POPUP_MENU_TOGGLE_LOCKED` → `toggleLocked()`

**macos — U1c: rack mode on, modules bolt to the rails (interactive session, Jeff directing)**

- verify against the branch that has the prerequisite, not against `master`
- my own staging
- U1b's rack-as-default (#33) is still an open PR
- The tell was that two independent probes both showed "never called" — that pattern means the code is not on the path, so check what you are running before you debug what you wrote.
- Learned — a rebase can silently put someone else's commit on your branch, and the authorship check is what catches it.
- exactly the class of thing A14 exists for, caught by the tool rather than by luck.

**macos — D3 done, D4 refuted by measurement, U1 closed (interactive session, Jeff directing)**

- a codesign failure on SynthEditCL can be stale-bundle detritus, and my first A/B was not controlled
- builds clean
- `rm -rf` the .app before believing a codesign failure on that target

**macos — D6: the about pane is built (interactive session, Jeff directing)**

- verify a clipboard by reading it back, not by watching the label change
- The button flips to "Copied" on its own return value, which is exactly the kind of self-report that can be true while the write failed

**macos — crumb thumbnails, and what they cost (interactive session, Jeff directing)**

- the strongest visual test is a CHANGE, not a picture
- containing the module I had just placed

**macos — TIDE does not save the user's rack (interactive session, Jeff directing)**

- chase the follow-up, find the feature
- Six sessions of host verification never caught this

**macos — S11's design answered by Jeff; S12 filed: TIDE makes no sound (interactive session, Jeff directing)**

- when a ruling arrives, check its premise before building to it
- Building S11's restore first and discovering the silence afterwards would have produced code whose central claim — "the graph rebuilds" — nobody could test.

**macos — S12 mapped: the machinery exists, in the sibling VST3 target (interactive session, Jeff directing)**

- "scope it before costing it" can cost one hour and change the answer
- Yesterday's S12 said "unknown and probably large: building a DSP graph at runtime is what the exporter does at build time"

**macos — S12 built to its last step: the rack has an engine; the tone is one gate away (interactive session, Jeff directing)**

- a three-point log turns "it doesn't work" into a one-run bisect
- sync-push / setParameter-rc / onSetPins-size located a missing wrapper subsystem in a single REAPER launch, then re-verified the fix the same way

**macos — FIRST SOUND (interactive session, Jeff present — "i can hear it!")**

- when both gates pass and the output is still empty, the skip is between the gates
- The probe pattern (log every candidate's name plus each gate's verdict) took one build and turned "modules missing, cause unknown" into "read `ExportXml_Pt2`"

**macos — the sound reproduces from upstream alone (interactive session, Jeff directing)**

- "it built" and "the host is running it" are different claims, and a cache reset can split them silently
- This is the same discipline as the clipboard sentinel and the thumbnail change-test: make the artefact prove it is the one under test.

**macos — S12(a): MIDI reaches the processor but not the graph (interactive session, Jeff directing)**

- a tight Lua polling loop measures nothing
- the loop was re-reading one frozen snapshot millions of times and reporting it as a result.
- A high sample count is not evidence; a changing input is.

**macos — suspect (a) refuted; the real cause found: the rack exposes no plugs (interactive session, Jeff directing)**

- probe the chain, not the theory
- at four points
- Instrumenting several points at once beats bisecting one hypothesis at a time.

**macos — container-IO contract reverse-engineered; the MIDI In module is standalone-only (interactive session, Jeff directing)**

- read the importer, not the exporter, when synthesising a format
- Every attribute that matters here (`Direction` as the IO-plug marker, `Datatype`'s enum ordering, `TiedTo`/`TiedToPinIdx`, the defaulting of `FromPin`/`ToPin`) came from the ~40 lines that *parse* the XML

**macos — choice (ii) built: the runtime feeds MIDI In; a second defect surfaced (interactive session, Jeff directing)**

- log the instance pointer when two objects can wear the same name
- And read the log's ORDER before concluding

**macos — the MIDI mystery solved: a second processor instance, and it never gets the document (interactive session, Jeff directing)**

- retract cleanly when the evidence changes
- the transport had stopped
- The pointer evidence was real and the conclusion drawn from it was wrong

**macos — MIDI notes play: the blob pin was never seeded on a new processor (interactive session, Jeff directing)**

- a `default: assert(false)` is a to-do list
- I fixed one arm of the pattern and did not check the other

**macos — MIDI notes verified from pure upstream; the patch workaround is retired (interactive session, then unsupervised)**

- a clean instance beats a debugged one
- When a test rig has been poked at by failed automation, rebuild the rig before debugging the product.

**macos — S11's restore has exactly one blocker: blobs are not encoded in the preset (unsupervised)**

- "it reaches the processor" and "it is saved" are different claims
- The chunk parameter now demonstrably delivers the document to the DSP (notes play), which made it tempting to assume persistence followed

**macos — presets now carry the rack: blobs base64'd, non-stateful params excluded (unsupervised)**

- making serialisation work can resurrect things that were only safe while broken
- When you fix a lossy round-trip, audit what was relying on the loss

**windows — C12c done, and the Windows build of `main` is broken by something else**

- a control compile is cheap and it is what makes "not my fault" a fact rather than a claim
- When a build breaks during your change, re-run the one failing compile against the untouched tree before you diagnose anything.
- Learned — four things about that failure were ruled out, so nobody repeats them
- Learned — the run prompt's GATED wording says "C1-C7" and the carve-out is up to C12.

**windows — the GATED build-break question written up as PROPOSED, and an enforcement gap found while checking its premise (interactive session, Jeff directing)**

- verify the mechanism a rule leans on, not just the rule
- A recorded verification is evidence about the thing that was verified, not about the class it appears to belong to.

**macos — S11: the restore side measured — it does not merely fail, it aborts the host**

- a negative result is worth more when you say which kind it is
- nobody has run the A/B against a pre-base64 binary
- Learned — a guard that catches nothing is evidence about threads, not about exceptions.
- Learned — the claim protocol does not protect a working tree.
- Asking the other sessions cost two messages and corrected both my ownership model and my conclusion.

**macos — backlog id collision: A17 filed twice, renumbered to A19**

- "next free id" is a read-modify-write race, and this backlog has no lock
- Cheap mitigation for the next run: re-check the id against freshly-fetched `origin/main` immediately before committing, not when you first read the file.
- Learned — the journal's prepend-only check caught me doing the wrong repair.
- A log you edit is not a log.

**macos — A16: the short-commit race reproduced, and guarded**

- two guards that look redundant can be complementary, and the way to tell is to check each against the other's incident
- Learned — "no repro was attempted" is often a much cheaper gap than it looks.

## 2026-08-18

**macos — C9**

- a NEXT row's named fallback is prose, and prose does not expire
- Learned — STEP 0.7's identity gate is single-pathed, and GitHub was degraded today.
- `tide-rack-bot`, databaseId 314850083
- I proceeded on the GraphQL assertion and am flagging the substitution here rather than burying it.

**macos — A19: the fleet may now act on its own agent's platform issues, as evidence**

- an allowlist written against outsiders can deadlock on insiders, and the failure is invisible from inside the rule
- The tell was not a review of the rule — it was hitting it: filing a verified host-abort and then reading the rule that forbade acting on my own report.
- Learned — "do X" on a security-relevant row still needs the reasoning written down, not just the edit.

**windows — unblocked the macOS A16 PR, filed the duplicate-id gap as A23 (interactive session, Jeff directing)**

- "git merged it cleanly" is not "the merge was correct"
- For files that are ordered lists rather than code, absence of conflict carries almost no information; check the invariant (ids unique, entries appear once, order is newest-first) explicitly after every merge.

**macos — GMPI ruled PR-GATED: a third STEP 5 category**

- when a ruling does not fit the existing taxonomy, extend the taxonomy rather than round the ruling to the nearest slot
- The tempting move was "add GMPI to ALLOWED with a warning comment", which is how the instruction would have decayed: the warning is prose, the list membership is what a run actually checks

**macos — A17 resolved (b), A18 answered: detection instead of prevention**

- a detector's first alarming result is a test of the detector, not of the system
- before reporting a measurement, check what else could produce it.
- Learned — my own exported `GIT_*` variables silently broke my selftest.
- A test that constructs git history must control the environment, not just the config

**macos — S11: the restore crash is FIXED, and the rack now survives reload (interactive)**

- five things, three of them traps
- THE STDERR TRICK. Launch the DAW from a shell, not `open -a`, and the uncaught exception names itself.
- The absent-TIDE-frames reasoning was a red herring, and here is why.
- BUILD TRAP, new and expensive: `GMPI_WRAPPER_FOLDER_OVERRIDE` was not set on this box
- TIDE cannot run as a Debug build.
- The saved chunk was DSP-only, and the editor cannot read that format.

**macos — S14**

- "cheap first measurement" was right, and cheaper than the row guessed
- shipped prefabs are already full-SynthEdit output
- Learned — the same red-herring shape as S11, one week apart.
- An absence is only evidence once you have shown the healthy case has the thing present
- Learned — A17's ruling does not stretch to this.

**macos — A20**

- the obvious rule was the wrong rule, and measuring is what showed it
- seven false alarms
- This is A10's trade restated
- Learned — the recall limit is real and is written into the row rather than left to be discovered.
- "take the next task" surfaced three states the branch listing hid
- Deleting on ancestry alone would have been wrong twice, and keeping on ancestry alone leaves permanent clutter.

**macos — S13 (Jeff directing)**

- the obvious API would have broken the commercial product, and one grep caught it
- unconditionally
- In shared code, prefer the pattern the neighbouring line already uses over the API that reads better.
- Learned — always A/B the test suite, even when the change cannot plausibly touch it.
- 44 failed / 13 passed
- exactly the same 44/13

**macos — S14 closed not-a-defect, S15 withdrawn (Jeff directing)**

- I measured the artefact and assumed the architecture
- One question first — what is a rack module supposed to be? — would have replaced the row, the ruling request and this correction.
- Learned — a wrong conclusion in a docs file is more dangerous than a wrong row.

**macos — issue #117 (STEP 1)**

- say which half of a verification you did not do
- evidence, not instruction
- Learned — the four overrides are all set on this box now, including the one that cost a cycle.

**macos — S13 verified by A/B, and a wrong assumption corrected**

- , and this is the entry's real content — I wrongly believed I could not test in REAPER
- That conflates two different things.
- launching a binary and reading its stderr is a Bash operation.
- while still believing I couldn't use it myself.

**macos — the audio measurement: harness built, answer is "not yet, and here is exactly why"**

- three layers of encoding in a `.rpp`, each of which cost a wrong guess, written down so nobody re-derives them
- but the first line is REAPER's own header block with its own `=` padding
- `val=`, not `value=`

**macos — V1's audio half: THE RACK SOUNDS (interactive session, Jeff directing)**

- five things, two of them corrections to me
- A patch point carries VOLTS, and 10 V is full scale.
- A DAW lists TIDE under its product name `TIDE Rack`, not its filename.
- Corrected in-session, by Jeff, twice.
- A dropped prefab does not keep the generator's slot size.
- The "places but does not draw" intermittency did not appear once

**macos — V3 attempted: MIDI reaches the rack, but does not cross a patch cable (interactive session, Jeff directing)**

- `MIDI In` is `modules_internal/MidiInGui.cpp`, id `MIDI In`, and its audio output `MIDI Data` is pin 1, not pin 0.
- A jack's hit-area is a few pixels, and TIDE has no undo.
- The prefab staging step copies but never prunes.
- Every generator run rewrites all the prefab handles

**macos — E2a and V1 flipped DONE and archived (state update, interactive)**

- prepend-only

**macos — PROBE D: MIDI does exit the MIDI In module (interactive session, Jeff directing)**

- an A/B that moves two variables is worth exactly as much as its weaker leg

**macos — MIDItoGate2 traced: it IS MIDI-2 compatible and DOES set the gate (interactive session, Jeff directing)**

- the shape of my own error, because it repeated
- `build-prefabs.py --diagnostics` exists for the first; a throwaway `fprintf` is fine for the second, as long as it is reverted.

**macos — A25: the NEXT-block check now actually runs, proven by probe (interactive session, Jeff directing)**

- `gh run view --log` interleaves ANSI escapes and tab-separated job/step prefixes, so grepping it for a Summary line finds the `echo` command rather than its output

**macos — PLAN's v0.1 acceptance test is COMPLETE (interactive session, Jeff directing)**

- , and it is the pattern of the whole session
- I attributed a silence or an error measured at the END of a chain to a component inside it.
- Validate the instrument on a case whose answer you already know, before believing what it says about the case you care about.

**macos — E9 researched: a rate change is absorbed by REPLACING the plugin, not by re-reading the rate (interactive session, Jeff directing)**

- the thing that reframes the row
- Not one of those eight lines carried the `(rate CHANGED)` suffix, and it never can.
- My earlier comment drew the wrong conclusion from correct evidence

## 2026-08-19

**macos — E9 (re-specced; E10 and A26 filed)**

- , and worth not rediscovering
- "It has an exact precedent to copy" is a claim about TWO call sites, and the 2026-08-18 research only checked one.
- The guard at `SeAudioMaster.cpp:409-413` is one line short of its own intent.
- STEP 2's continue-a-branch rule trips STEP 4's authorship check, and I hit it.

**windows — C12f (and #111 verified closed)**

- , and the reason the bookkeeping changed
- C12f's Accept was wrong, and wrong in the direction that unblocks an unsafe item.
- The A14 collision happened to me, live, and the authorship check is not what caught it.
- `git checkout -b` in a shared working tree drags the other session onto your branch.
- Include resolution under a move is worth checking rather than assuming.

**macos — E9's sliver was a silence writer, and next door to it was a live host crash (interactive session, Jeff directing)**

- the honest boundary of TIDE's guard, measured rather than assumed
- still crashes
- Learned — E10's Accept clause would have passed while the process still crashed.
- Learned — one trap for the next person measuring this.

**linux — STEP 1 build break (#153 filed and fixed; #87 closed, #88 half-closed, #156 filed)**

- the mac box works by coincidence too
- That `#else` is correct on exactly one machine, the one whose home directory it names
- the A14 shared-tree race did not recur, and I think I know why
- The windows box hit it at 36 seconds after `git checkout -b`

**linux — C12d: the carve-out's last stage, and its stated reason was wrong**

- a control before the change is worth more than a check after it
- Two consecutive carve-out stages have now shipped with an Accept clause that was wrong in the direction of unblocking something unsafe.
- It cannot, on any box.

**linux — C12 is COMPLETE; C12d and the umbrella flipped to DONE, C6 unblocked**

- `check-backlog-diff.py` caught a real rewrite of mine, and it was right to
- The general shape: prepend to an Item, never edit inside it.

**linux — A26: the authorship check fails on what you can fix, and reports the rest**

- the five cases are worth keeping in this shape
- Case 5 (a pushed foreign commit *and* an unpushed one on the same branch) is the one that would catch a future regression collapsing the two categories: it must report one and block on the other in the same run

**linux — C6: EditorLib's CMakeLists is public, and the plan its own comment left was wrong**

- , and this is the third instance this session
- Treat a predecessor stage's instructions as a hypothesis, not a specification.
- `grep -c "SE16\|SynthEdit2"` over that build log is 0

**linux — C6 DONE; C7 and C10 unblocked, and C7's first move is already measured**

- a bookkeeping trap worth naming, because the next run will meet it
- A26 is `TODO`
- This is BACKLOG A22 exactly.
- Trust the branch/PR check over the status column when they disagree

## 2026-08-20

**macos — A27: the NEXT block's Take column is read now, and the docstring stops lying about it**

- A comment that contradicts the code is a bug with no test that can fail.
- The fleet's CI gate can be down without anyone noticing
- Superseded text in a NEXT cell must lose its imperative, not just its position.
- `git checkout origin/main -- .` inside a worktree will silently revert your own uncommitted work

**macos — A28: the refuted hypothesis, corrected in the four places that state it and the one that originates it**

- A row that names the files to fix is naming symptoms, not the set.
- "Add it to the watch list" is a two-part change in this script
- The A27 check caught this run's own regression, twice.
- Name the *kind* of thing being watched, not just the thing.
- A test control built from a real name has a shelf life.

**macos — A21: the identity gate stops on a wrong answer, not on no answer**

- "Add a fallback" was the wrong frame and the row's own wording carried it.
- A rule that requires the reader to classify a failure will eventually be classified wrong

**windows — C14: the last private include was never needed, and it was hiding an ODR violation**

- "It needs a private header" and "it needs the private type" are different claims, and only the second is a real dependency.
- A same-named class in two apps is an ODR violation the toolchain will never report.
- A `PUBLIC` include directory is a dependency you have loaned to every consumer.
- Do not build in the session scratchpad on Windows.
- The mac run's "C14 and C10 are `SynthEditLib`, rejected as GATED" reads the gate too widely.
- `origin/main` moved three times while this run built.

**macos — C7c answered by removal, and the two questions that answer creates**

- "Remove the feature" can be the right answer to a licensing-boundary question, and it is not one an agent would have proposed.
- A one-line product decision can have load-bearing code underneath it.

**macos — the mac test drift is FMA contraction, and my own diagnosis was wrong first**

- A hypothesis that explains the summary statistic is not a diagnosis.
- Eliminating the obvious suspect is worth more than confirming it.
- `-ffp-contract` is invisible in a fast-math discussion.

**macos — A22: the row names the branch, not the PR; and SynthEdit's CI never runs on push**

- A rule that cannot be obeyed in one step will be obeyed in two, and the second step is where the damage is.
- "CI is green" means nothing until you know what triggers CI.

**macos — A23: duplicate-ID detection, and the three false alarms that shaped the rule**

- Run a new lint against real history before believing it.
- Two properties that look like one.

**macos — A24: the journal floor is one DATE, because seven days measures 651 KB**

- Measure the remedy, not just the problem.
- A24 nearly cited a taken ID.

**macos — C7b: TIDE's own source leaves the private repo**

- Moving two folders together is cheaper than moving one.
- A "same object count" acceptance clause is worth more than it looks.

**macos — C16: the last private include was three dead symbols**

- A deletion is a legitimate answer to "narrow this to an interface", and it is cheaper to check for than to build toward.
- Stale comments do not just mislead about behaviour — they set the expected SIZE of the work.

**macos — C7d: TideSynth builds on its own**

- A subproject you do not use can still block configure.
- Copying a fiddly block verbatim beats improving it.

**macos — C7e: the clean clone builds; the CI clause is one apt-get away**

- "CI is green" and "a stranger can build it" are different claims, and C7e asks for the first while carve-out.md calls the second the real proof.
- A fail-fast probe reports one missing dependency and hides the rest.

**linux — #190: the Linux CI package set, measured**

- A fail-fast dependency probe costs one CI round trip per missing package, and the cheap fix is to walk the chain locally.
- "CI is green" would not have caught S21.
- The runner's package set is partly luck.
- `PKG_CONFIG_LIBDIR` pointed at a pruned copy of the system `.pc` files is an accurate, seconds-long stand-in for a differently-provisioned machine, and it isolates the variable better than a container would have

**linux — S21: the Linux bundle's resources were staged outside it**

- `$<TARGET_FILE_DIR>` is not inside the bundle on Linux
- A silent cross-repo disagreement needs a test written from ONE side.
- CI would not have caught this and still will not.

**macos — C15 was C16: two ids, one job, and a NEXT block pointing three runs at it**

- The duplicate-work check that matters is not about ids.
- Writing a rule down is not the same as being able to follow it.

**macos — U2 was finished four days ago, and it was the last mac row**

- Third stale-status row today
- Measuring a proposed lint against the live tree before writing it has now paid off four times today

**linux — S21 verified at runtime, and three things I got wrong**

- Data from a deliberately broken environment must be labelled at the moment it is written down.
- A claim used to justify NOT doing work deserves more scrutiny than one used to justify doing it, not less.
- Report a crash with a rate, or don't report it as a consequence.
- Export the identity in every shell that commits, not once per task.
- `libpipewire-0.3-dev` is all that stands between this box and a working `TIDE_STANDALONE`

**linux — A30: the lessons digest, and why the literal spec would have backfired**

- A spec that says "copy X into a file every run reads" is a size decision in disguise, and it should be measured before it is implemented.
- A generated index that silently covers half its input looks exactly like one that covers all of it.
- Sentence-splitting on "." is wrong in any corpus that names files.
- This project's writing conventions are load-bearing infrastructure.

**linux — S17: name the folder, not the decision**

- A message that names a DECISION cannot catch a wrong RESOLUTION.
- Check whether the bug you were sent to fix is present in the tree you are fixing.
- `set(PARENT_SCOPE)` from a function called in a subdirectory reaches that subdirectory, not the top level.
- Dependencies here come from three places, not two

**linux — E14: TIDE's own two modules are in the product, and half of Accept is met**

- Control the tool before blaming the subject.
- A backlog row's warnings age with the tree, and three of E14's had.
- A per-target compile option is the wrong tool when a requirement belongs to one file.
- `pkill -f <name>` matches the shell running it

**linux — insertion is arm-then-click, and I had blamed the wrong thing**

- When an experiment has two readings and one of them blames my tools, that is the one to distrust.
- Read the interaction's own header before guessing at it.
- A feature with no feedback is indistinguishable from a broken one
- `TIDE_STANDALONE` restores its last session

**linux — S26: the se_sdk timers never fired, and Jeff's mouse was the instrument that found it**

- "It redraws" and "it refreshes" are different systems with different drivers, and the user who owns the product knew to distinguish them.
- A platform port is complete when every pump the reference app runs has an owner.
- When two input paths disagree, say so and hand verification to the one that failed.

**macos — A31: the granularity was the whole design, and three measurements chose it**

- A check's granularity is not a style choice — each candidate tier had a measurable false-alarm rate (14, 0, 6) and only one was shippable.
- The C15/C16 collision left a fingerprint neither filer intended: both rows cite the same `file:line` verbatim.
- The check's first catch was its own author, in the same commit that adds it.

**macos — A32: the umbrella advisory, and the measurement that was already done**

- A row that carries its own false-positive measurement is a different kind of spec: the build step is obedience, not design.
- "Advisory" needs the reason printed with it, or it decays into noise.

**macos — #222: two of today's merges only ever built standalone, and SE16-hosted TIDE lost configure entirely**

- "Verified on this box" quietly became "verified in the only mode this box builds".
- The second break was hiding behind the first, again.

**macos — C10: 104 editor files leave the root, and the reference count fell as it was measured**

- A reference count taken without word boundaries is an upper bound, not a work list.
- "Same object count" needs the same instrument on both sides.

**macos — P7d was already fixed, from a third direction, and its parked question is moot**

- A row that parks on a question can be closed by running its Accept — check that before re-raising the question.

**macos — E15: the rack's faceplate is TIDE's own panel, and two breaks the swap flushed out**

- A module that measures itself is a different contract from a rectangle that takes orders, and the prefab grid only worked because the old faceplate obeyed it.
- "It builds in the product" says nothing about the authoring path.

**macos — S24: the cross cursor was already there on Windows, and mac got the same shortcut**

- Third time today a row's central premise had moved before it was taken

**macos — S25 does not reproduce on mac, and the negative result is the deliverable**

- A cross-platform row can be closed on one platform and open on another, and saying which is the whole value of a cheap reproduction.

**macos — E6's honest tell: renders that ignored your state now say so**

- A row that names its own fallback scope can be half-shipped honestly

## 2026-08-21

**macos — E11's hazard is unreachable, and the reason is a stub nobody had noticed**

- A row that asks "is this ordering safe?" can be answered by showing the code never reaches the ordering at all.
- A stub that returns `true` is worse than one that returns `false`.
- The grep-before-filing habit paid for itself the day after it shipped

**macos — arm64-only, and the FORCE that made the obvious change a no-op**

- `set(... CACHE ... FORCE)` in a dependency silently outranks the consumer AND the command line.
- When a change must reach shared code, the negative control is the deliverable.

**macos — Linux CI is green, and the macOS job that would confirm it cannot say anything**

- Ask what a pending check could possibly prove before waiting on it.
- "All three platforms pass" and "all three passed in one run" are different claims, and only one of them is what an Accept clause usually means.

**macos — STEP 4 bookkeeping: seven rows flipped on merged PRs**

- A day that merges thirteen PRs leaves the backlog lying by seven rows.

**macos — E16 ruled Tier 1, and four conventions came with it**

- A correction that removes a blocker is worth checking hardest, not least.
- "Rack module" was a statement about the USER'S MODEL and I read it as one about implementation
- Corrections are cheapest when the work is still unmerged.
- Two wrong calls this session died to one habit: reasoning from a measurement without asking what produced it.

**macos — E5: the rack grid ruled, and the snap is gcd(12, 15)**

- A constant that serves both a layout rule and a drawing rule will be changed for one and silently break the other.
- The most expensive thing in this item was an assumption inside a probe.

**linux — S23 did not reproduce, but a mechanism explains why it never would**

- A self-healing mechanism upstream of a bug will make that bug look intermittent, and a controlled-run count cannot see it.
- `XDG_CONFIG_HOME` is honoured by the standalone
- A branch's CI platform issue can be reporting `main`'s break.
- A clean SIGTERM shutdown exits 0
- `pkill -f <pattern>` matched my own shell and killed it (exit 144)
- Report a crash with its control, not just its correlation.

**linux — S23: the session file is innocent, and the kernel had both crashes logged**

- `journalctl` keeps a kernel record of every segfault, with a module-relative offset, and nobody in this fleet had looked.
- The same module offset under two different ASLR bases means one deterministic site.
- A fault at `0xfffffffffffffff8` is -8, and -8 is a signature, not an address.
- A timeline can refute a hypothesis that a reproduction attempt cannot.
- Resolving an address in a rebuilt binary is not evidence.
- A "GNOME Shell crashed" line and a login prompt are not the same event.

**macos — S29 fixed, after measuring that S29's own recommendation was wrong**

- A concurrency group is only as good as what its expressions evaluate to, and `github.ref` is not the branch on a `pull_request` event.
- The second-best fix won on a constraint from a different row.

**linux — A12: the wall this row recorded was not there, and the check it wanted had a false alarm in it**

- A row can inherit a blocker from the rows filed beside it, and nobody re-checks.
- A watchdog's own false alarms are the expensive kind.
- Rotation is a hazard for anything that reads the journal, not just for readers of it.
- The archive is not reliably ordered
- Some Accept clauses are unsatisfiable by construction, and saying so beats half-meeting them.

**linux — the compositor problem is solved, and S23 does not reproduce once you can safely look**

- Fixing the tooling blocker was worth more than any single item it unblocked.
- A headless compositor loses the view and keeps the verification
- A negative result is only worth what its control is worth.
- `check-id-refs.py` caught me filing A31's exact hazard.
- A crash row that no longer describes anything observable should be closed, not left open.

**macos — S33 filed: a live defect was sitting on a closed row**

- Recording a finding on a row you are about to close loses it.
- Two other agents were filing ids concurrently.

**linux — N1 costed: 91% of what a grep finds must not be touched**

- Counting a rename by bucket, not by total, changes the decision.
- A grep total is not a work estimate when the repo keeps a historical record.
- Ask which box can VERIFY a change before asking which box can make it.
- A row that says "needs decisions rather than edits" is worth re-reading after its blocker clears.
- When the developer overrides a standing rule, write down which rule and which instance.

**linux — S23: what -8 means, measured — and the fleet has been bitten by this exact class before**

- `/var/log/apport.log` names the executable path for crashes apport declined to report.
- An address that lands mid-instruction is proof the binary is wrong
- Negative fault addresses are arithmetic, and the arithmetic is worth measuring rather than recalling
- `error 4` vs `error 5` separates a null dereference from a wild read
- Grep the tree for your own crash signature before theorising.
- A dead end closed with evidence is worth more than a lead kept alive on hope.

**macos — N1a: the rename shipped, and it silently unlinked half the build first**

- A guard that makes missing work silent turns a rename into a downgrade.
- When the old artifact is still installed and shares an ID, matching numbers are not evidence.

**macos — the release track was free for three days and the backlog said otherwise**

- A blocked row is never obviously wrong, so nothing ever re-reads it.
- "Unblock the section" is not the same as "unblock every row in it."

**macos — S29's coverage-hole fix, rebuilt clean after the branch went stale**

- An unpushed branch decays the moment other agents keep merging.

**macos — R3: the pkg builds, and productbuild would have shipped it to the wrong hardware**

- A packaging tool's defaults describe the tool, not your payload.
- When a row names two payloads, confirm both exist before starting.

## 2026-08-22

**macos — R3a: the AU goes into the pkg, before the first tag**

- Check the preconditions of a release before cutting the tag, not after.
- A comment that was true when written is a liability the moment its subject changes.
- Put the check where the artifact is sealed.
- Sign every bundle in the payload, not the first one.

**macos — E1c's deciding render: my hypothesis is refuted, and the row is still open**

- A pre-committed binary outcome is worth the effort of setting up.
- Answering the experiment's question is not the same as answering the row's.
- Check the confounds on the case you are ARGUING FROM, not just the one you ran.
- A residual can be a property of the VALUE, not the code.

**macos — R5: the release workflow, and the credentials were already there**

- Check the ENVIRONMENTS before concluding a repo has no secrets.
- A protected environment is a better gate than a trigger condition.
- Sign before you package, not after.
- Two Apple certificates, not one.
- `security find-identity -v -p codesigning` hides Installer certificates.
- Identically-named certificates are a live hazard, not a tidiness issue.
- Look up every action version.
- When behaviour cannot be tested, assert structure.

**macos — S30's two fixes, and a design that could not have worked**

- `matrix` is not available to job-level `concurrency`
- Check the context list before writing an expression that reads naturally.
- A compiler cache that hashes absolute paths never hits across CI runs.
- Print the cache statistics unconditionally.
- When you cannot measure the benefit, ship the measurement.

**macos — S30 re-measured: the queue is gone, the build is the problem**

- Re-measure a row before recommending against it, especially a performance one.
- A cancellation rate is a symptom of DURATION, not only of policy.
- Compare work done, not just time taken.
- A stale diagnosis is worse than no diagnosis

**macos — AU is on, and four rows closed on one build**

- Enable the thing that exercises the most decisions at once.
- A prediction confirmed on a real artifact is worth more than the same value read off a build.
- "It builds" is not "nothing broke".
- Put the history at the switch.
- File the leftover warning as a row, immediately.

**macos — R8: every bundle now has an identifier TIDE owns, and codesign stops inventing one**

- "Present and empty" and "absent" are different bugs.
- `codesign` succeeding is not evidence of a correct identity.
- An override needs a fresh build directory.
- Test the no-op claim on a pass-through argument.
- Keep the enabling change separate from the identity change.

**macos — R9: TIDE owns its identity, and the id was a fossil of the old product**

- Validate a derivation model against the CURRENT value before trusting its prediction.
- When part of a model fails, say so and route around it.
- A "DO NOT RENAME" comment is an argument, not a law.
- Grep for the identifier in TOOLS, not just in source.
- A negative control that makes a GUI app prompt is not free when someone is at the keyboard.
- If a tool crashes something on purpose, put that in its header in capitals.

**macos — M1 and M3 fixed properly, and the override my own notes warned about**

- FetchContent's source override makes `_deps/<name>-src` ABSENT, not populated.
- A two-PR fix can have an ORDER, not just a pairing.
- `#if 0` around a correctness feature is a bug with a comment.
- "The flags are optional so nothing changes" is a claim, and a cheap one to test.
- Verify from a plain build before claiming a fix.

**linux — S7: TIDE does write to the user's home, and does not spew skins — the guard is an accident**

- `HOME=` is not a sandbox when the code uses `getpwuid`.
- Validate a test harness against a probe before trusting its result.
- "It does not do the bad thing" and "it cannot do the bad thing" are different findings
- Snapshot the thing you are about to test before you test it.

**macos — M1 and M3 were never blocked by the carve-out, and the AU passes auval**

- A `BLOCKED` row with no stated blocker is a claim nobody has retested.
- A CMake property can be set, announced, and overwritten one loop later.
- A working control on the same machine beats any amount of reading.
- `OUTPUT_NAME` breaks every hand-derived sibling name, not just the one you fixed.
- Fix the first error and expect the count to go UP.
- Objective-C class names are process-global.

**macos — loading the CLAP for the first time found that TIDE ships SynthEdit's identity**

- Load the artifact in a real host before believing it works.
- A stale caveat is most dangerous when it is your own and hours old.
- Write the host when the DAW is the expensive part.
- When a rename touches `name` and `vendor`, check `id`.
- Follow an identifier to what is DERIVED from it.
- Verify a hash claim by computing it.

**linux — E1c: the deciding case, and the control that makes it decide anything**

- A "single-variable" experiment is a claim, and it is cheap to check.
- The audio harness runs on Linux
- The harness warns when the engine scanned module folders outside `--modules`
- Do not regenerate another box's provenance record to improve its label.

**linux — R4: the tarball, and the CLAP's resources have nowhere to live**

- Verify a two-repo fix from the shared branch, not from the branch that made it.
- "Where does this format keep its data?" is a packaging question with a different answer per format.
- A bare `.so` plugin format has no namespace
- Check for a validator before promising verification.

**macos — STEP 4: six PRs merged in one go, and every NEXT cell went stale at once**

- A batch merge invalidates the WHOLE NEXT block, not one cell.
- `check-next-block.py` cannot see a cell that cites a dead row as history.
- Do not wrap a NEXT cell in `
- When you replace the Take column, replace the Why column.
- Flip a row on verified PR state, not on memory of having pushed it.
- Not every leftover deserves a row.

**windows — R2: the Windows installer, and the payload it must carry is not the file the build emits**

- A packaging script's real job is deciding what the shipped layout IS, not copying a build tree into a zip.
- `afxres.h` names a missing header and means a wrong Visual Studio instance.
- Windows has no sandboxed installer run, so the way to prove one is to compile it twice.
- The app version STEP 0.5 asks for IS discoverable on this box
- A "portable" REAPER on Windows is neither portable nor unattended.

**macos — S27: four suspects eliminated, and the reference box turns out to be x86_64**

- An Apple Silicon Mac separates ISA from OS/libm in a way no other box can
- `$?` after a pipeline is the LAST command's status.
- Check whether a hand-rolled RNG is actually the portable kind before blaming it.
- `sqrt` is not a cross-platform divergence source.
- A passing subset is a control, not noise.
- Rebuild at the commit that produced the artifact before assuming drift.

**macos — E1c: the hypothesis was already refuted by a table in this repo**

- Before designing an experiment, check whether the repo already ran it.
- When two cases differ by 50 dB, list every way they differ before believing the first explanation.
- A measurement without its provenance is not a measurement.
- Grade backfilled facts explicitly.
- A harness that needs an engine should still have a mode that does not.

**macos — S31: the trap only exists on Linux, and that is why writing it down four times did not work**

- When a negative control refuses to reproduce a documented bug, that is a result, not a broken harness.
- `pkill -f` self-kill is a Linux-only trap.
- A lesson that two of three boxes cannot reproduce will not stick by being written down again.
- Test what the OS might be doing for you, directly.
- Ask whether the signal was delivered, not whether the process died.
- Silence expected noise in test output.

**macos — R4a: CLAP was in nobody's build, and my own Linux fix was a half-fix**

- A format missing from a build list produces no error, only an absent file.
- Grepping for a variable name closes the uses of that variable, not the defect class.
- Never edit a CRLF file with Python text mode.
- Checking a control turns a bug report into an elimination.
- A duplicate found from two boxes is not waste
- Check a lint's EXIT CODE, never grep its output.
- Invoke a lint exactly as CI does or the local run means nothing.
- Filing a row out of another row duplicates its citations.
- `gh pr edit` needs `read:org` and the agent token has only `repo`.

**linux — #271: fixing the bundle name alone would have emptied the bundle**

- When two files are documented as mirroring each other, changing one is a half-fix by construction.
- A build that succeeds can still package nothing.
- `GMPI_SDK_FOLDER_OVERRIDE` makes a PR-GATED change testable
- Write to a CRLF file with Python and you get a 1,280-line diff.
- Prove a no-op instead of claiming one.

**macos — P11's mac half had the right symptom and the wrong mechanism**

- A stale row is most expensive when its symptom is right and its mechanism is wrong.
- "The cache doesn't list X" is not evidence X is ignored
- `SE_LOCAL_BUILD` on macOS does not do what its name implies.
- Check `[ -w ]` before telling someone to use `sudo`.
- The shared-citation lint (A31) earns its keep on rows you split.

**linux — N1b: the rename's live docs, and a Linux-only gap N1a could not have seen**

- Classify a doc by reading its opening, not its filename.
- A doc you wrote yesterday is not exempt from going stale.
- The first build on a platform after a cross-platform rename is a real test.
- When two things must agree, check them against each other, not against spec.
- "Verified on two platforms" is not "verified".

**macos — N1a: OUTPUT_NAME renamed three things, and only one of them had an extension**

- `OUTPUT_NAME` is three renames, and only the one with an extension is collision-proof.
- `LNK1201` names disk space, privilege and path, and means none of them.
- Configure twice and diff the generated build system.
- A build that does not install is a measurement trap.

**macos — ccache went into build.yml and not release.yml, and the numbers are in**

- Two workflows that build the same thing need the same fixes.
- Correlate a duration and its statistics to the same run before quoting a ratio.
- Say what a cache key prefix couples.
- An option sized against an old measurement expires with it.

**macos — v0.1.0: Windows and Linux shipped, macOS wanted a certificate nobody had sent**

- A configuration variable naming a credential is not the credential.
- Log what the job can actually do, not what it was configured to do.
- A release that fails before `publish` costs time and nothing else.
- Two platforms passing is real evidence.

**macos — STEP 4 after v0.1.0, and a branch I pushed and never opened**

- A pushed branch with no PR is invisible.
- "Unknown to fix" deserves one grep before it is written.
- A row can be closed by a run rather than by a commit.

**macos — the AUv3 works, and my "it doesn't register" was a wrong query**

- When a tool reports nothing, check the query before blaming the subject.
- Two wrong diagnoses in a row, both pointing at someone else's code, is a signal about the instrument.
- Sharing fourCCs between v2 and v3 is correct AND means one is unreachable.
- Prove the artifact works before asking which artifact to ship.

**macos — E1c's second discriminator, and verifying the pitch before seeding it**

- Verify the thing a golden encodes before writing the golden.
- Two eliminated hypotheses can leave a third that neither suggested.
- Hold the control constant and say which control.

**macos — S38 is three problems, not seven instances of one**

- Read the mechanism before costing the fix.
- A per-class question needs a per-class answer.
- A correct sibling is evidence about the design.
- Grep for string-based class lookup before renaming an Objective-C class.
- Confirm an override reached the compiler, not just the command line.

**macos — S40 ruled: AUv3 only, and the install story is a copy**

- Measure the install story before costing a format change.
- `set -euo pipefail` can kill a guard before it speaks.
- A non-zero exit is not evidence a guard fired.
- A format that ships as an app inherits a naming decision the build never had to make.

**macos — STEP 4, and the hour-long feedback loop that caused two of today's failures**

- Put the cheap failure first.
- A stale row is found by sweeps, not by people.
- The question asked is not always the question that matters.

## 2026-08-23

**linux — S43: the CLAP now refuses an API it just said it did not support**

- I hit the CRLF trap the journal documents, on the first try.
- A guard needs the passing case tested, not just the failing one.
- An unenforced assumption written in a comment is a defect with documentation.

**linux — S37: the collision is unreachable, because the Linux CLAP has no GUI at all**

- A probe that stops before the code under test reports a healthy subject and proves nothing.
- `ldd` answers "can this code path exist" faster than any amount of reading.
- I nearly filed a bug inside `#if 1`'s dead `#else`.
- A strings match is evidence the file was compiled, not that the code runs.
- Absence of a syscall wants an A/B.
- A23's duplicate-id race is real and I hit it inside one session.
- The shared-citation check (A31) fired on my own two rows

**windows — P11: the diagnostic can't name the file, so it stops naming the cause (interactive, Jeff directing)**

- A NEXT cell's paraphrase and a row's own Accept can disagree, and the row wins.
- A diagnostic that can't tell two causes apart should say so, not guess.
- `cd` inside a command chain does not substitute for checking cwd before the NEXT command that assumes it.

**linux — E1c: the deciding render lands at −140 dBFS, and neither the module nor the pitch is the variable on its own**

- A pre-committed binary outcome is worth the setup, and worth distrusting when it lands.
- A regression control must separate the two gates, not merely fail.
- Check the engine version against the reference's before believing a cross-platform residual.
- A scan warning can be eliminated instead of noted.
- A warning that fires on every run is not a caveat, it is noise — and the way to find out is to check its own claim.
- `recorded: "unknown"` provenance can sometimes be settled by measurement.

**linux — the render references fail on Linux too, and the tolerance that merged four hours ago does not reach it**

- A tolerance is calibrated against a variable, and the variable is not in the number.
- The largest number in a table is worth checking before it drives a decision.
- A comparator that ignores alpha is measuring pixels the product never shows
- Two boxes agreeing to two decimals from opposite directions is worth more than either measurement.
- Run the cheap control before the expensive fix.
- A check that has only ever seen the easy case is untested, not proven.
- When a lint fails, prove the artefact by hand before believing either of you.
- A dependency-free subproject is a gift.

**windows — S41: nothing ever closed a platform issue, and the option the row favoured would not have helped (interactive, Jeff directing)**

- An option written into a row is a hypothesis, not a plan.
- Read the Accept clause as the specification and the options as guesses.
- A mechanism with one half is worse than none, because it looks complete.
- Test the boring branch.

**windows — #314 reproduced here: 5 failures in 25 builds of plain `main`, then 0 in 40**

- A race whose CI rate is about 8% was 20% on a developer's box, and the difference is the compile.
- Reproduce in the real tree even when a synthetic repro is easier.
- Delete the outputs, not the build tree, to re-run a POST_BUILD race.
- A dedup has a second failure mode the first one hides: building one target.
- The platform that differs is not the platform with the bug.
- "Nothing shared to collide over" is a claim about a generator expression, and one command settles it.

**macos — three red signals, three different mechanisms, and none of them a defect in this repo**

- A race cannot be verified fixed by watching CI go green.
- `origin/main` is not a snapshot when you fetch more than one repo.
- A self-healing break still costs three sessions.
- Three red signals on one platform in one hour is not evidence of one cause.
- Read the first error, not the loudest.
- A checkout error that names `.gitmodules` means someone committed a directory.

**linux — A34: the scan warning now reports a finding instead of a probe, and S37's inference is corrected**

- A negative grep is only evidence if you know the symbol would be there.
- "Compare the artifact against a sibling that works" beats any amount of reading.
- A conservative classifier needs its false branch tested hardest.
- Splitting "absent" from "empty" was worth the extra case

**windows — S36: the Windows resources move beside the binary, and my first attempt at "beside" was wrong (interactive, Jeff directing)**

- A clean build and a clean log are not evidence the destination is right.
- "Drop the `/..`" meant drop it entirely, not shorten it by one segment.
- A destination change to a build-tree path can force a packaging-script change even when the shipped layout doesn't move.

**linux — S43(ii): the Linux CLAP has an X11 editor. It embeds, the host drives it, and it paints nothing**

- A macro that is never defined is worse than a `#if 0`, because it reads as live code.
- Structure beats pixels on a headless server.
- The teardown order bug was mine, in the probe.
- Mixed line endings in one repo.
- A CMake block inserted next to the right line can still land in the wrong `if()`.

**windows — S34: two guards in SynthEditLib, and a stale row flipped on the way in (interactive, Jeff directing)**

- A row that names its own precedent site is most of the fix.
- Check the cell's own "before doing anything else" instruction before doing anything else.

**linux — the CLAP editor PAINTS, and the cause was M4's defect on a third wrapper**

- Instrument the LAST link first.
- Two real bugs fixed on the way to the wrong place are still two real bugs.
- The third instance is the one to generalise from.
- A blank window has two very different causes and they look identical from outside

**windows — S22: the repo where the trap was found was the one that never got the fix (interactive, Jeff directing)**

- "Fixed in repo A, filed for repo B" can leave the bug in the repo that taught you about it.
- A guard needs its positive control run, not just its passing case.

**linux — S37 is real: another plugin's uninstall permanently breaks TIDE Rack**

- A premise that fails measurement can be waiting on a different bug.
- The uninstall case is the one that makes a shared-folder bug undeniable.
- The plugin's own diagnostics were the instrument.

## 2026-08-24

**windows — S44: the stranded reference split, landed and verified on the platform that could not check it**

- A "not verified" line in a commit message is an assignment, and the box it is addressed to may never read it.
- Count the callers before moving a path.
- A resolver needs its wrong branch tested, not its right one.
- Byte-identity to a source is worth asserting mechanically.
- A branch can be stranded because of what it contains, not because someone forgot.

**macos — E9: the AU absorbs a rate change, and the pitch is the proof**

- A probe that prints the option it requested is not reporting a measurement.
- When every failure mode collapses onto the same wrong number, the right number is strong evidence.
- A backgrounded build you think you killed is still building.
- Check whether a row's blocker is still real before believing the row is closed.
- A fixture saved for one format can drive another without being re-authored
- `pluginkit -a <appex>` registers an AUv3 straight out of a build tree

**linux — STEP 1: #156 verified green on Linux, and the 44 failures reproduce on demand**

- A "NOT VERIFIED on your platform" line in a DONE row is a work item, and nothing points at it.
- The negative control was one environment variable and it is the whole entry.
- `gtest_discover_tests` registers a `<target>_NOT_BUILT` placeholder
- Three quoted pass counts for one suite — 63, 67, 73 — and all three are correct.
- A scratch worktree is not just tidiness here, it is the test.

**linux — the A4 auto-merge trap, hit a third time, and the branch DELETION is the new half**

- A `gh pr view` state check is worth nothing in the same unconditional command block as the push it is meant to gate.
- A follow-up push that reports `[new branch]` has re-created a deleted branch, not updated one.
- The A22 follow-up is optional by design, so "drop it" is cheap.
- A merge that happens between writing an entry and pushing it makes that entry wrong about its own ending.

**linux — S37: CLAP has no bundle on Linux, so TIDE ships a semi-bundle instead (interactive, Jeff directing)**

- "Can we use a bundle?" and "can we get a private directory?" are different questions, and only the second one mattered.
- A negative control that leaves the hazard in place beats one that removes it.
- The expensive option was expensive because of who else it touched, not because of its size.
- A row's own wording can smuggle in a false premise that survives three runs.
- `weston --backend=headless --xwayland` plus an isolated `HOME` makes a real commercial DAW a scriptable instrument on this box.

**linux — S25's Accept passes; the tofu that survives is a different bug (interactive, Jeff directing)**

- Run a row's Accept before building toward its diagnosis.
- A symptom that appears in BOTH arms of a comparison cannot be evidence about the comparison.
- The same window rendering some text correctly is the cheapest possible elimination
- `__has_include` guards deserve a check, not a conclusion.
- `pgrep -f <pattern>` self-kill bit me twice in one session

**linux — S45: the captions were UTF-32 in a UTF-8 string, and S23 closed (interactive, Jeff directing)**

- A probe on one branch of a multi-branch decision can report a clean bill of health for a failure happening in a sibling branch.
- Log the actual string, early.
- `backtrace()` + `addr2line -f -C` turns "who called this" into one build.
- "Seen on platform X" and "X-only" are different claims
- Eliminations are worth their cost when they shrink the search space enough to instrument.

**linux — S46: the technology is recorded now, not sniffed off a filename (interactive, Jeff directing)**

- When two guards both need the same missing input, the second one is not a fallback.
- "Which platform" was the wrong axis, and the row's title encoded it.
- Ask where the data can arrive from before choosing where to fix.
- A comment admitting "dodgy" is a filed bug nobody filed.
- The CRLF trap caught me again

**linux — S45 closed on its own Accept, and a 50-minute window where `main` did not link (interactive, Jeff directing)**

- "The branch fixes it" and "the product is fixed" are different claims
- Pick a metric the bug makes big.
- A cross-repo "must land together" is a real hazard even when everyone does it right.
- A link error naming a symbol you have never heard of is usually someone else's half-landed change

**linux — SynthEditJuce ruled unmaintained, and #88 closes WONTFIX (interactive, Jeff directing)**

- "Deprecated" and "unmaintained" answer different questions
- A comment that hedges toward revival will be read as a to-do.
- Some issues close by ruling, not by code
- Check line endings before editing, not after

**windows — P3: the MFC requirement is gone, and a no-MFC toolchain is the proof (interactive, Jeff directing)**

- A row's own guess can be right and still worth measuring, because the SIZE is the actionable part.
- A value that must match an external header is not a code change, it is a data change, and it fails silently.
- Guard macros need the both-headers case actually compiled.
- A build "hanging" for ten minutes was `| grep … | head -5`.
- Check the row's file paths before believing its plan.

**windows — V4: the three candidate markers, measured — one of them selects nothing (interactive, Jeff directing)**

- "Ruled rather than invented" does not mean "stop" — it means measure, then escalate with numbers.
- An option can fail because the data it reads does not exist for the thing being selected.
- A flattening commit changed what a proposed option would select.

**linux — V6's risk discharged: a root paste does carry connections, and the row's fork is now live (interactive, Jeff directing)**

- "Verify X before deleting Y" earns its place when X has never happened.
- Put the control in the same artifact as the subject.
- A minimal repro is worth more than a faithful one here.
- A CI issue can name a head that no longer exists.
- Check what a folder scan actually enumerates before shipping a file into it.

## 2026-08-25

**macos — The queue is blocked, so the run proved the platform instead; B1 closed on a green matrix**

- A suspicious performance number deserves its mundane explanation checked first.
- "Persistent workspace" and "incremental build" are different claims.
- A process gap with zero live instances is a lesson, not a row.
- When the queue is blocked, building your own platform's default branch is the one thing left that is not busywork
- Two independent routes to the same fact is worth noticing.

**windows — `main` is red on all three platforms from ONE unpushed half, and the fix is uncommitted in Jeff's tree**

- A red build across all three platforms is evidence of ONE cause, not three, and the shared cause is nearly always a sibling repo.
- "The other half was never committed" and "the other half merged 26 seconds late" look identical from CI and need opposite responses
- You cannot `/I`-shadow a quoted include.
- A missing `override` fails loudly; a missing `virtual` fails silently.
- The Windows box has no automatic STEP 1 feed.
- A run that starts minutes after an interactive commit should expect to be looking at half a change, not at a defect.

**windows — the unpushed half was pushed, and `main` is green on all three platforms (interactive, Jeff directing)**

- A rule that stops a run short is not thereby a rule that cost the fleet anything.
- Re-run the failed run rather than pushing a new commit when the variable you changed lives in another repo.
- `build.yml`'s close-on-success step works, observed in place
- The `matrix.platform != 'win'` exclusion cuts both ways.
- A commit whose meaning depends on a commit in another repo has to say so in its message.

**macos — The adaptor is dual-licensed, and the permissive packs are picked and filed (interactive, Jeff directing)**

- "Why is this GPL?" is worth asking of your own code.
- A permissive code licence is not permission to ship the panels
- A licence is the one thing to prepare rather than land.
- A file being big is not evidence about what is in it.
- Check push access before designing the delivery.

**macos — Rendered the candidate panels: the permissive packs don't draw their controls, and that inverts the pick (interactive, Jeff directing)**

- A screen that fails its positive control is broken, not informative
- Counting SVG primitives measures the authoring tool.
- "Can we use the artwork" has a legal reading and a practical one
- `rsvg-convert` + looking is cheaper than any amount of parsing

**macos — A stale comment inverted a recommendation, twice; the adaptor already draws the components (interactive, Jeff directing)**

- A comment twelve lines above the code it contradicts will be believed.
- Validating the instrument is not validating the premise.
- "Explain X" is a real check on X.
- Correct the source, not just the conclusion.

**macos — E20: 66 of HetrickCV's 79 files compile, and the CC0 pack has an MIT dependency (interactive, Jeff directing)**

- A CC0 headline does not survive a submodule.
- "Copy the working option" hid the actual work.
- Compile-everything is a cheap sizing instrument.
- The regression control is the thing that makes mock edits safe.

**macos — HetrickCV runs in TIDE: 66 modules registered, 104 with Fundamental alongside (interactive, Jeff directing)**

- A cache variable is the wrong tool for a once-per-configure guard
- Syntax-only tells you nothing about shared implementation units.
- A comment asserting a capability is a claim; test it.
- A missing enum can surface as a missing vtable

**macos — The port is pushed and fetches; panels were 100 wide; and a new crash signature (interactive, Jeff directing)**

- `"100%"` parses as `100`
- A floor on one axis hides a parse failure on both.
- Rendering an asset and reading it are different tests.
- `gh repo create --source` will not add a remote that already exists
- Public does not mean writable.

## 2026-08-26

**macos — M6: auval passes an empty rack and this does not; the appex finally has a voice (scheduled run)**

- `auval` passing is compatible with the plugin containing nothing
- An absent line is a failure mode a grep cannot see.
- The AU3 appex is assembled by its own always-run target
- `pluginkit -a` registers the extension; a GUI launch is not required.
- The appex is reused across instantiations

**linux — three boxes fixed one duplicate ID in two minutes, and a duplicate breaks two checks not one (interactive, Jeff directing)**

- A duplicate ID is not one broken check.
- Two correct-looking arguments can agree by luck.
- `cmd | tail -n; echo $?` cannot report the command's status
- STEP 2's collision check does not cover opportunistic fixes.
- Losing a race is cheap if you rebase to the delta.

**linux — E32's size half: the standalone reopens where it was, and the save had to move before closeWindow() (interactive, Jeff directing)**

- "Save on shutdown" has an ordering, and the wrong one fails silently.
- Share the validity bound between read and write, not the clamp.
- A protocol limit is a scope decision, not a TODO.
- `git merge` into a worktree, then editing before resolving, corrupts the edit.
- Backticks in a `--body` argument are shell-interpreted.
