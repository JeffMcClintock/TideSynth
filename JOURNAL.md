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

## 2026-08-18 — macos — issue #117 (STEP 1)

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** closed [#117](https://github.com/JeffMcClintock/TideSynth/issues/117)
on a fresh Release build of `master`, and archived the **S11** row, whose five
PRs had all merged. No code changed anywhere.

**Read the prompt again mid-session, and it had moved: `b3e9876` -> `397330d`.**
Worth saying because a run normally reads it once at STEP 0 and would not
notice. Three changes land on this box: STEP 1 now admits `tide-rack-bot`
issues (A19); STEP 5 gained the GATED build-break exception with six bounds
(A17); STEP 3/4 now want `check-commit-completeness.py --record/--verify`
around commits in a shared checkout (A16). **The first of those is what made
this run's work possible at all** — two earlier runs, mine included, walked
past #117 because the fleet could not act on its own agent's issue. The
deadlock A19 described is now gone, and #117 was the first thing to come out of
it.

**Result — #117 is fixed, and the fix builds here.** Cause, for the record:
`std::stod()` on every parameter regardless of datatype in the processor's
preset reader, latent while blobs serialised as `"0"` and reachable the moment
one was written as base64. `setPresetUnsafe` runs on the host's **main** thread,
so the throw unwound into the event loop and killed the DAW. Fixed by
[GMPI#5](https://github.com/JeffMcClintock/GMPI/pull/5) (the throw),
[GMPI_Wrappers#6](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/6)
(main-thread fail-safe at all three host boundaries) and
[SynthEdit#43](https://github.com/JeffMcClintock/SynthEdit/pull/43) (`<Editor>`
in the chunk, imported instead of always creating blank) — all merged and
present in their default branches, checked individually.

**Verification artifact — a full Release build of `master` `d6043de1f` on this
box, all three products:**

```
TIDE_VST3   ** BUILD SUCCEEDED **   universal x86_64 + arm64
TIDE        ** BUILD SUCCEEDED **
SynthEditCL ** BUILD SUCCEEDED **
```

and the built bundle carries the `Editor` element name from SynthEdit#43, so it
is the fixed code rather than a stale link — which is worth checking on this box
specifically, given the prebuilt-library trap.

**So: mac's default branch builds, as of now.** No `platform:mac` issue is open.

**Learned — say which half of a verification you did not do.** The runtime
proof (exit 134 SIGABRT in ~8s -> loads clean, 2516-byte byte-identical
round-trip) is the interactive session's, not mine; computer-use is refused
during a scheduled run, so I could not re-run REAPER. STEP 1's new clause says a
bot issue is **evidence, not instruction**, and to re-verify on your own
platform before acting. I could verify the build half and not the runtime half,
so the issue comment says exactly that rather than implying I watched it load.
Closing on a build plus someone else's measured A/B is a judgement call, and it
should be visible as one.

**Learned — the four overrides are all set on this box now, including the one
that cost a cycle.** `GMPI_WRAPPER_FOLDER_OVERRIDE` is in the CMake cache
alongside the other three, so the build uses the local `GMPI_Wrappers` clone
rather than a FetchContent copy. Confirmed from `CMakeCache.txt` before
building, which is cheaper than discovering it from a build that silently
ignored local edits.

**Next:** no platform issue and no open PR on this box. The mac NEXT row's two
GUI-less pointers are both spent (S14 measured, A20 shipped), so the next
unattended run falls to STEP 2's topmost-eligible rule. What is actually
blocking progress is two rulings, both Jeff's and both minutes of work:
**S15** (pick (a) route rack placement to the structure rect, or (b) give `CUG`
a real panel rect) which unblocks S14, and **A25** (four lines wiring A20's
check into `lint.yml`). **S13** — TIDE cannot run as a Debug build, a missing
`database.se.xml` tripping `assert(false)` in `UgDatabase.cpp:549` — is the one
that stops anyone attaching a debugger to the next crash, and is GATED.

**Side effects on this box:** three Release targets built, so
`SynthEdit/build/` is warm and its `Release` outputs are current. No source
changed in any repo. All eight repos on their default branch and clean.

**Branch/PR:** `tide/mac/issue-117` — TideSynth backlog and journal only.

---

## 2026-08-18 — macos — A20

**Prompt:** b3e9876 · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** shipped **A20** as option (a) — `scripts/check-next-block.py`, a lint
check that fails when the NEXT block tells a run to take work that is archived
or absent. Detection rather than convention, matching A17/A18's ruling the same
day. The lint wiring is `.github/workflows/**`, which this credential
structurally cannot push, so it is filed as **A25** with the exact four lines.

**Why A20.** The mac NEXT row sends a GUI-less run to S14's measurement or A20;
S14's measurement landed earlier today ([#132](https://github.com/JeffMcClintock/TideSynth/pull/132)),
so A20 was what was left. STEP 1.5 first: no open PRs in any repo. #117 is
still open and still authored by `tide-rack-bot`, so STEP 1 still reads it as
information (A19 is archived but the underlying rule is unchanged).

**Result — verified with a positive control taken out of git history, not a
fixture.** The check is run against `4a8154d:BACKLOG.md`, the exact state that
produced this row:

```
2 take-target(s) checked across 4 NEXT row(s)
  BACKLOG.md:12  [mac]  D6  -- archived DONE     matched: 'should take U1b D6'
  BACKLOG.md:12  [mac]  U1b -- archived DONE     matched: 'should take U1b D6'
rc=1
```

and against today's tree: `every NEXT take-target is a live BACKLOG.md row`,
`rc=0`. **It fails on the bug and passes on the fix**, with no synthetic input
— the A/B is a real commit. `--selftest` is 13 cases green: ten phrase cases
plus three end-to-end (archived target fails, live target passes, absent target
fails).

**Learned — the obvious rule was the wrong rule, and measuring is what showed
it.** The first draft also treated *every* id in the Take column as a
take-target, on the reasoning that the column is definitionally what to take.
Against the real block that produced **seven false alarms**: `E2a`, `S1b`,
`S5`, `S7`, `S8`, `A12`, `B1` out of *"do not fall back to…"* warnings, and
`C12c`, `P10`, `A10`, `A14`, `A15`, `A4`, `P9` out of precedent mentions.
A Take cell in this backlog is a long editorial paragraph, not a field —
the mac cell alone names eleven ids and instructs on two. So the rule was
dropped: the trigger set is imperative phrases only, with any clause carrying a
negation disarmed. **This is A10's trade restated:** a false negative costs a
run minutes, a false positive costs trust in five other checks, so recall is
deliberately the side that gives.

**Learned — the recall limit is real and is written into the row rather than
left to be discovered.** `should take **S14**'s cheap first measurement … or
**A20** itself` matches `S14` and misses the trailing `A20`, because the
list-walk stops at the first non-id word (`'s`). It catches
`take **U1b** or **D6**`, which is the shape that actually occurred. Extending
it to arbitrary distance is how the seven false alarms come back.

**Learned — "take the next task" surfaced three states the branch listing hid.**
Before starting I synced all eight repos and classified every `tide/` branch.
Ancestry alone is misleading here because A4 squash-merges: four local branches
looked unmerged and all four had landed. `git cherry` proved two by patch-id;
the other two needed a content check (the A19 row is in BACKLOG-DONE, the
`std::stod`/`std::get` findings are in main). **Deleting on ancestry alone
would have been wrong twice, and keeping on ancestry alone leaves permanent
clutter.**

**Next:** A25 is Jeff's four lines, and A15's precedent says the Summary
wiring — not the step — is the part that actually fails the job; prove it with
the same two-commit probe. S14 stays BLOCKED(S15) until Jeff picks (a) or (b).
The mac NEXT row's remaining GUI-less pointers are now both spent, so the next
unattended run falls to STEP 2's topmost-eligible rule — which is exactly the
situation A20 was filed about, and the check now guards the row that describes
it.

**Side effects on this box:** merged [GMPI_Wrappers#7](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/7)
at Jeff's explicit request — one docs commit of his own that PR #5 had left
stranded on a branch with no PR. Then cleaned every stale branch across all
eight repos at his request: **10 merged remote branches and 12 local ones
deleted, 0 remaining, local or remote**, each verified merged-or-landed first.
All eight repos are on their default branch and clean. No builds. Nothing
written outside `TideSynth` and the scratch dir.

**Branch/PR:** `tide/mac/A20-next-block-check` — TideSynth only, one new
script plus rows. (Branch name rather than PR number in the row, per A22.)

---

## 2026-08-18 — macos — S14

**Prompt:** b3e9876 · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** ran S14's "cheap first measurement" and it answered the row's
question outright. **No code changed anywhere** — every fix site is in GATED
`SynthEditLib`, filed as **S15**. Working:
[docs/s14-rect-measurement.md](docs/s14-rect-measurement.md).

**Why S14.** S11 is done and merged, so the mac NEXT row now points a GUI-less
run at S14's measurement or A20. I still have no GUI (`request_access` for
REAPER is refused during a scheduled run, re-checked this session, and the
user confirmed it cannot be granted from inside one). STEP 1.5 first:
GMPI_Wrappers#6 and GMPI#5 are S11's remaining PRs, both **green, mergeable,
no reviews, nothing unresolved** — waiting for merge, so left alone. #117 is
still open but authored by `tide-rack-bot`, so STEP 1 still makes it
information, not instructions (A19 unresolved).

**Result — the row asked an either/or and the answer is "both, and they are
the same event".** Rack mode addresses the *panel* rect; a plain DSP module
has no panel rect; the assignment lands on an empty base-class no-op and is
discarded; `structRect`, the only rect such a module persists, is never
written and keeps its constructed zero.

**Verification artifact — a comparison with a positive control, from two files
on disk, no host and no build.** TIDE's 2516-byte S11 chunk against a shipped
full-SynthEdit prefab:

```
TIDE          1 KHz Tone       structRect=ZERO             panelRect=ABSENT
full SE       FloatToVolts     structRect=176,264,260,300  panelRect=ABSENT
full SE       IO Mod           structRect=296,288,356,312  panelRect=ABSENT
full SE       SE Text Entry4   structRect=572,276,680,436  panelRect=32,76,97,99
```

`FloatToVolts` and `IO Mod` are the control: **plain DSP modules placed by
full SynthEdit carry no `panelRect` either**, and a healthy non-zero
`structRect`. So "TIDE is missing panelRect" is refuted — TIDE matches them on
the absent panelRect and differs only in the zero structRect.

**Mechanism, four facts each checkable alone:** `CDocOb::setViewObRect` has an
**empty body** and `getViewObRect` returns `{}` (`DocOb.h:89-93`); `CUG`
implements both **only** for `CF_STRUCTURE_VIEW` and serialises only
`structRect` (`CUG.cpp:2557,2566`, `CUG.h:32`); `panelRect` is `CControl`'s,
not `CUG`'s (`Control.h:23`); rack mode places through the top-level **panel**
view (`MfcDocPresenter.cpp:811,1421`, prose at `TideApp.cpp:497-500`).
Composed: every TIDE placement is `CUG::setViewObRect(CF_PANEL_VIEW, …)` ->
`CDocOb`'s no-op. The `master_container` is zero for the same reason — it is a
`CUG` placed the same way.

**Learned — "cheap first measurement" was right, and cheaper than the row
guessed.** The row sized this as "place a module in full SynthEdit, export,
compare", which needs a GUI. It is not needed: the **shipped prefabs are
already full-SynthEdit output**, so the control was sitting in
`SynthEdit2/Resources/prefabs/` all along. A GUI-less run could have answered
this at any point. Worth generalising — when a measurement wants "what does
full SynthEdit produce here", look for a checked-in artefact before booking a
GUI session.

**Learned — the same red-herring shape as S11, one week apart.** S11's trace
reasoned from an absence ("no TIDE frames on thread 0") and that turned out to
mean nothing, because the stack was already unwound. This row reasoned from an
absence too ("no panelRect"), and it also means nothing, because full
SynthEdit's DSP modules have none either. **An absence is only evidence once
you have shown the healthy case has the thing present** — that is what a
positive control is for, and both times it was one file away.

**Learned — A17's ruling does not stretch to this.** It permits repairing a
**build break** whose cause is GATED. S14 is a functional defect in
`SynthEditLib`, so the gate still holds; filed as S15 with the two candidate
fixes rather than picking one, because option (b) changes the on-disk schema
for every SynthEdit module and that is not a run's call.

**Next:** S15 needs Jeff to pick (a) route rack placement to the structure
rect — one line, reversible, testable this week — or (b) give `CUG` a real
panel rect, better model but a shared-format change. Then S14's headless
half is re-exporting the chunk and asserting non-zero distinct rects, which
separates "geometry is stored" from "geometry is stored and the rack draws
it". The visible-in-rack acceptance still needs an interactive session.

**Side effects on this box:** synced all eight local repos and returned
`TideSynth` and `SynthEdit` to their default branches — both were parked on
S11 branches that were **already fully merged with zero commits beyond the
default**, so nothing was lost; the two local branch pointers were deleted.
`GMPI_Wrappers` and `GMPI` were deliberately **left on their branches** —
those carry a peer session's open PRs (#6, #5) and relocating another
session's checkout is not mine to do. `SynthEditLib` fast-forwarded 2 commits.
No repo was dirty at any point. No builds. Nothing written outside
`TideSynth` and the scratch dir.

**Branch/PR:** `tide/mac/S14-zero-structrect` — TideSynth docs only, no code
in any repo. (Row carries the branch name rather than a PR number, per A22.)

---

## 2026-08-18 — macos — S11: the restore crash is FIXED, and the rack now survives reload (interactive)

**Interactive session, Jeff directing.** He owned the ordering (fail safe first,
then find the throw, then wire the editor), ruled GMPI in scope when the fix
landed there, and REAPER was available throughout — every claim below is a
measurement in REAPER, not an inference.

**Did:** all three steps, verified, three PRs.

1. **Fail safe on the main thread.** `setState` / `setComponentState` /
   `stateLoad` are host boundaries the DAW calls on the **main thread** while
   opening a project. An exception escaping one unwinds into the host's own
   event loop, where there is no handler — on macOS it reaches
   `-[NSApplication run]` and `terminate()` aborts the DAW. All three now
   catch and report failure. CLAP's `stateLoad` is `noexcept`, so an escape
   there does not even unwind; it calls `std::terminate` directly.
2. **Found the throw — measured, not reasoned.**
   `GMPI/Hosting/processor_holder.cpp:503` called `std::stod()` on every
   parameter's `val` whatever the datatype. Fixed by dispatching on datatype,
   which the **controller-side reader of the same loop already did** via
   `GmpiParameter::setFromXml` — so the fix is one line reusing the correct
   implementation, not a second copy of the dispatch.
3. **Gave parameter 1 an editor route and imported the document.**

**Result:**

| | Before | After |
|---|---|---|
| open `/tmp/tide-restore-test.rpp` | exit **134** SIGABRT in ~8s | loads, REAPER stays up |
| stderr | `libc++abi: terminating due to uncaught exception of type std::invalid_argument: stod: no conversion` | clean |
| crash report | `REAPER-2026-08-18-084525.ips` (thread 0, `EXC_CRASH`) | none |
| rack round trip | modules gone on reload | **2516 bytes, byte-identical** |

The round-trip test is the strong one, and it is objective rather than visual:
place a module → save → quit → reopen → save again → decode the VST state out
of both `.rpp` files and diff. Identical, 2516 bytes, `<Document>` + `<DSP>` +
`<Editor>` + `<master_container>` with all three modules. Before the change the
same trip came back with the modules gone. Decoder kept at
`scripts/decode_rpp.py`; artefacts `/tmp/tide-s11-verify.rpp` (the save) and
`/tmp/tide-s11-final.rpp` (the round trip, shipping binary).

**Learned — five things, three of them traps:**

- **THE STDERR TRICK. Launch the DAW from a shell, not `open -a`, and the
  uncaught exception names itself.** One line — `std::invalid_argument: stod:
  no conversion` — turned a static suspicion into proof with no debugger, no
  breakpoint and no rebuild. Four previous sessions characterised this crash
  from `.ips` reports alone. Do this **first** next time.
- **The absent-TIDE-frames reasoning was a red herring, and here is why.** The
  trace concluded the main thread was innocent of TIDE because no TIDE frames
  appear on thread 0. But for an **uncaught** exception the stack is already
  unwound by the time `terminate` runs — the frames are *gone*, not absent.
  `Processor_VST3::setState` was on that thread all along. An `.ips` for
  SIGABRT-via-terminate cannot tell you where the throw was.
- **BUILD TRAP, new and expensive: `GMPI_WRAPPER_FOLDER_OVERRIDE` was not set
  on this box** — the only one of the four that wasn't. `GMPI_SDK`,
  `GMPI_UI` and `SYNTHEDITLIB` all point at local checkouts, so
  `GMPI_Wrappers` looked like it did too. It did not: the build used a
  FetchContent clone at `build/_deps/gmpi_wrappers-src` (pinned Aug 17 15:33),
  and **every edit to the local `GMPI_Wrappers` checkout was silently
  ignored** — compiled fine, changed nothing, no warning. Cost a full
  build-and-test cycle chasing a route that was never in the binary. Fixed
  with `cmake -DGMPI_WRAPPER_FOLDER_OVERRIDE=~/Documents/GitHub/GMPI_Wrappers .`
  in `SynthEdit/build`, and CMake then prints `Using local GMPI WRAPPERS
  folder`. **Check for that line.**
- **TIDE cannot run as a Debug build.** `database.se.xml` is absent from the
  bundle in both configs, so `CModuleFactory::RegisterExternalPluginsXmlOnce`
  (`SynthEditLib/UgDatabase.cpp:549`) hits `assert(false)` and aborts REAPER.
  In Release the assert compiles out and it silently returns. Filed as **S13**
  — it is why every previous session was Release, and it blocks debugging the
  very crashes we keep chasing. Fix is in a GATED path.
- **The saved chunk was DSP-only, and the editor cannot read that format.**
  This is the architectural half nobody had noticed: `<Modules>` (capital,
  consumed by `SeAudioMaster`) versus `<modules>` under `<master_container>`
  (consumed by `ImportModules`), and the DSP shape carries **no positions at
  all**. So the rack could never have come back however well the blob
  survived. The chunk now carries both sections under one `<Document>`;
  `BuildDspGraph` navigates `Document->DSP` explicitly, so the sibling is
  invisible to it.

**Also corrected:** the row's claim that the A/B "exonerated base64" and that
the crash was unrelated to it. The row itself already carried that correction;
this run confirms the accurate framing — **the defect is pre-existing and
latent, and base64 made it reachable** by being the first thing ever to
serialise a blob as text.

**Next:** two follow-ups filed, neither blocking. **S13** — the Debug-build
assert above. **S14** — modules placed from the browser get `structRect` all
zeros, so they persist correctly but do not *render*; the rack looks empty
though the document is right. Persistence and placement are separate problems
and only the first is fixed. Also unmeasured still: whether **audio** returns
now that the document reaches both sides.

**Branch/PR:** [GMPI#5](https://github.com/JeffMcClintock/GMPI/pull/5) ·
[GMPI_Wrappers#6](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/6) ·
[SynthEdit#43](https://github.com/JeffMcClintock/SynthEdit/pull/43). **Merge
GMPI_Wrappers#6 and SynthEdit#43 together** — without the wrapper's caller,
`SynthEditController::setParameter` is dead code.

---

## 2026-08-18 — macos — A17 resolved (b), A18 answered: detection instead of prevention

**Prompt:** b3e9876 · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** recorded two rulings that turned out to be one decision. **A17 =
option (b)** — a run may repair a build break whose cause is in a GATED path,
under the six bounds. **A18 answered by "I'm not upgrading my plan. Let's make a
best-effort approach."** Shipped the control that "best effort" has to mean:
`scripts/check-no-direct-commits.py`.

**Why they are one decision.** A17's own entry flagged that its premise — *it
gets reviewed* — was only two-thirds true: `SynthEditLib` has a protected `main`
with an active ruleset, but the private `SE16` has an unprotected `master`,
because private repos cannot carry rulesets without a plan upgrade. Relaxing the
gate in the one repo where nothing enforces the replacement would have been the
worst of both. Jeff ruled out the upgrade, so prevention is off the table there
permanently and detection is what is left.

**The check, and the thing that nearly made it useless.** First draft: flag any
non-merge commit on the default branch's first-parent chain authored by
`tide-rack-bot`. Run against `SE16` it reported **eight commits bypassing
review** — an alarming, headline-shaped result.

**Every one was a false positive, and checking took one command.** All eight are
author `tide-rack-bot`, **committer `Jeff McClintock`** — Jeff landing agent work
by rebase, squash or cherry-pick, which preserves authorship and restamps the
committer. That is not the gate being bypassed; **that is A17's premise being
satisfied.** The fix is to require the agent in *both* fields, which a scheduled
run always has, since STEP 0.7 exports all four `GIT_*` variables.

**Corrected baseline, which is the actually useful output: all six repos clean.**
No agent commit has ever reached a default branch without a PR — including on
`SE16/master`, where nothing mechanical was stopping it. Two years of "voluntary
compliance with the run prompt" turns out to have held.

**Learned — a detector's first alarming result is a test of the detector, not of
the system.** I had a finding shaped exactly like the one this project keeps
producing (agent bypasses a control, nobody noticed) and I was one `git log`
away from reporting it. The habit that caught it is the same one that caught the
A/B earlier today: **before reporting a measurement, check what else could
produce it.** Author-without-committer produces this signature, and it is the
*good* case.

**Learned — my own exported `GIT_*` variables silently broke my selftest.** The
selftest's "human" commits came out authored as the bot, because a run's shell
exports `GIT_AUTHOR_NAME` and `-c user.name` does not override an environment
variable. It passed vacuously in the sense that mattered. Fixed by setting the
env explicitly per commit. **A test that constructs git history must control the
environment, not just the config** — and this is a general trap for these
guards, since the very shell that runs them is the one with the overrides set.

**What did NOT change, deliberately:** option (c) — GATED becomes advisory — was
declined, because review discharges correctness and irreversibility but not the
reviewer's attention budget. STEP 3's *"never fix a build failure for a platform
you cannot compile on"* is untouched and orthogonal; it still keeps #88 with the
linux box.

**Residual risk, stated rather than buried:** detection is after the fact, and a
run that ignores its own check still pushes. `SE16` is protected by convention
plus an alarm, by explicit decision. That is what best-effort means here, and it
should be written down as a choice so nobody later mistakes it for an oversight.

**Next:** [#87](https://github.com/JeffMcClintock/TideSynth/issues/87),
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88) and
[#111](https://github.com/JeffMcClintock/TideSynth/issues/111) are now repairable
by the boxes that own them — linux and windows both have a broken `main` and, as
of this ruling, permission to fix it.

**Side effects on this box:** none. Docs and one new script, TideSynth only. No
builds. The repo scans were read-only.

**Branch/PR:** `tide/mac/A17-gated-build-fix`.

---

## 2026-08-18 — macos — GMPI ruled PR-GATED: a third STEP 5 category

**Prompt:** b3e9876 · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** recorded Jeff's ruling — *"GMPI is our most highly curated repo,
changing it is not to be done lightly. i would prefer that modifications to GMPI
go via a human-approved PR."* STEP 5 gains a **PR-GATED** category holding GMPI;
`docs/decisions.md` carries the ruling.

**Why a third category rather than putting GMPI on an existing list.** Neither
fits. **ALLOWED** is wrong — that is the list for TIDE's own folders, where
changing things is ordinary backlog work, and the whole point of the ruling is
that GMPI is not that. **GATED** is wrong too — GATED means file the question and
stop, and Jeff did not say stop, he named a route. Forcing GMPI onto either list
would have lost half the instruction.

**The interpretation I took, flagged rather than buried.** "Human-approved PR"
could mean (a) agents may propose, humans approve at merge — or (b) humans author
GMPI changes, full stop. **I read it as (a)**, because a scheduled run already
never merges its own PRs, so an agent-authored GMPI PR *is* human-approved at
merge time; if he meant agents never touch it, "GATED" was the word already
available. **The two readings differ materially** — under (a) [#117](https://github.com/JeffMcClintock/TideSynth/issues/117)
becomes work a run can do, under (b) it stays Jeff's — so the decisions entry
carries a `Default in effect` line: if the reading is wrong, GMPI reverts to
GATED and file-and-stop.

**What the category costs a run, deliberately.** Propose, never merge; keep it
minimal and say in the PR what you did *not* verify; **a GMPI PR is a proposal,
not a fix**, so no row goes DONE on the strength of one and no later work builds
on it as though it had landed; and **if the change can be made TIDE-side instead,
make it there** — reaching into GMPI because it is the tidier place is the reflex
this exists to slow down.

**One thing worth stating explicitly, because it nearly got lost:** the rule is
about *modifications*. **Reading GMPI has never needed permission and still does
not** — today's `std::stod` throw site was found by tracing into it and filed
without editing a line. A category that accidentally discouraged reading would
have made this class of bug harder to find, not safer.

**Learned — when a ruling does not fit the existing taxonomy, extend the
taxonomy rather than round the ruling to the nearest slot.** The tempting move
was "add GMPI to ALLOWED with a warning comment", which is how the instruction
would have decayed: the warning is prose, the list membership is what a run
actually checks. A named category with its own rules is what survives being read
by a fresh agent with no memory.

**Context — this is the fourth platform break in two days stranded behind a
permissions question** (#87, #88, #111 on GATED `SynthEditLib`/app folders, #117
on GMPI). This ruling clears the fourth. **A17 — may a run repair a build break
whose cause is in a GATED path? — still governs the other three** and is still
NEEDS-JEFF.

**Next:** #117's fix is now proposable by a run: datatype dispatch in the
processor's preset reader plus a loud assert, per Jeff's framing. It still wants
a debugger to confirm `std::stod` is the throw before anyone writes it, and that
needs a GUI session.

**Side effects on this box:** none. Docs only, TideSynth only. No builds, and
**no GMPI modification** — the finding that prompted this was read-only.

**Branch/PR:** `tide/mac/gmpi-pr-gated`.

---
