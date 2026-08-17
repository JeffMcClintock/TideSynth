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

## 2026-08-18 — macos — S13 (Jeff directing)

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** fixed **S13** — TIDE could not run as a Debug build. One-file change in
GATED `SynthEditLib/UgDatabase.cpp`, taken on Jeff's direct instruction, which
is the "needs Jeff or an interactive session" the row was waiting for.
[SynthEditLib#19](https://github.com/JeffMcClintock/SynthEditLib/pull/19).
Filed **S16** for something found while A/B-ing it.

**Result — the fix is the one the row called honest: don't assert on an absent
database.** `RegisterExternalPluginsXmlOnce` read `database.se.xml`, parsed it,
and treated *any* parse error as `assert(false)`, so absent and corrupt were
indistinguishable. It now returns early on an empty resource and **keeps the
assert for a database that is present but will not parse**.

**Absent is legitimate, and that was established rather than assumed:**
`database.se.xml` is written by `ExportAsPlugin`
(`SynthEdit2/ExportAsPlugin.cpp:1704,1722,1731,1742`), so an *exported* plugin
has one and a plugin built directly from source does not. TIDE is the latter —
constraint 7 compiles the module set in, S1a removed the scan — so there is
nothing to register. Release compiled the assert out and returned silently,
which is why this survived so long; Debug aborted the host.

**Verification artifact — a runnable probe against the real `tinyxml2`,
showing precisely what the old code could not tell apart:**

```
absent    -> Error()=true   id=15 XML_ERROR_EMPTY_DOCUMENT
malformed -> Error()=true   id=16 XML_ERROR_MISMATCHED_ELEMENT
valid     -> Error()=false
```

Both error rows hit the one `assert(false)`. Builds, consumers included because
this library ships in SynthEdit too: `TIDE_VST3` **Debug and Release**,
`SynthEditCL`, `SynthEdit_VST3`, `SynthEdit_GMPI` — all SUCCEEDED.

**Learned — the obvious API would have broken the commercial product, and one
grep caught it.** `BundleInfo::ResourceExists()` is exactly what this code
wants and reads as the clean fix. Off JUCE it is `return false;`
**unconditionally** (`BundleInfo.cpp:490`), so using it would have made
*SynthEdit* — which does ship a database — skip module registration entirely.
The correct test was the boring one, and it was already in the codebase six
lines away: `SynthRuntime.cpp:80` guards `dsp.se.xml` with `.empty()` right
after calling this same function. **In shared code, prefer the pattern the
neighbouring line already uses over the API that reads better.**

**Learned — always A/B the test suite, even when the change cannot plausibly
touch it.** `dsp_tests` came back **44 failed / 13 passed** after my change,
which looks damning. Stashing the change and rebuilding gave **exactly the same
44/13**, so it is pre-existing. Without that control I would either have
believed I broke it or, worse, waved it away by reasoning that a database guard
cannot affect DSP maths — and been right by luck.

**And the cause of those 44 is worth its own row (S16).**
`tests/projecttests.cpp:103` and `tests/layouttests.cpp:56` hardcode
`/Users/jeffmcclintock/SynthEdit/build/`; this checkout is
`~/Documents/GitHub/SynthEdit`, so every test that shells out to `SynthEditCL`
fails with `No such file or directory`. **None is a real DSP failure.** It
matters because the C-stage rows cite "92 tests all RC=0" as evidence and that
is a *Windows ninja* number — on mac the suite has been almost entirely red,
and a run building here cannot distinguish a regression from the path bug.
`SynthEdit/tests/` is on neither STEP 5 list, so GATED by default: filed, not
fixed.

**Next:** **S13's Accept is not met by me** and the row stays IN-REVIEW for it —
a Debug `TIDE_VST3` actually loading a project in REAPER. Computer-use is
refused in a scheduled run and there is no runnable standalone TIDE, so nobody
has watched the Debug build survive a load; that is one check for an
interactive session. Then **S16** makes the mac suite a usable signal, which
every later run benefits from.

**Side effects on this box:** `SynthEdit/build/` now has Debug **and** Release
outputs for several targets, where before only Release was current. One source
file changed in `SynthEditLib`, on its own branch with its own PR. All other
repos untouched and clean.

**Branch/PR:** `tide/mac/S13-debug-assert` in both repos —
[SynthEditLib#19](https://github.com/JeffMcClintock/SynthEditLib/pull/19) (the
fix) and the TideSynth half (rows and this entry, docs only). Neither blocks
the other's build.

---

## 2026-08-18 — macos — S14 closed not-a-defect, S15 withdrawn (Jeff directing)

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** closed **S14** as not-a-defect and withdrew **S15**, both on Jeff's
correction the same day they were filed. Corrected
[docs/s14-rect-measurement.md](docs/s14-rect-measurement.md) in place — its
measurement was right and its conclusion was wrong, and a future run reading it
would have built the wrong thing. Filed **E5** for the styling intent he stated
while doing it. No code changed.

**The architecture, which is the thing to carry forward.** Rack modules are
**Containers designed in advance and shipped as prefabs**, added to the rack at
runtime. The container carries the panel — patch-points and knobs/sliders on
it, wired internally to non-GUI modules like an oscillator — and the container
is what the rack draws. A module that has its own GUI, a scope for instance,
can also sit on the rack directly, and **that already works**. Placing a bare
non-GUI module on the rack is not a thing an end user does.

**So the three bare `1 KHz Tone` modules I measured were never a product
composition.** They are what audio testing looks like: drop plain modules in,
switch to the **structure view**, check basic audio works. A developer
workflow. Nothing was supposed to give them rack geometry, and nothing did.

**Result — the measurement stands, the inference did not.** Rack mode routes
placement through the panel view and a plain `CUG`'s panel setter is `CDocOb`'s
empty body: still true, still checkable. Two things flip meaning once the
architecture is known:

- **The prefab split is confirmation, not a complaint.** In full SynthEdit's
  own prefabs the modules carrying a `panelRect` are exactly the GUI-bearing
  ones; the plain DSP modules carry none. That is the same GUI/non-GUI line
  Jeff drew at product level, visible in the file format.
- **The container half was already there, under a name I did not look for.**
  `CContainer : CUG_with_patches : CUG` — not a `CControl`, which is why it has
  no `panelRect` — overrides the rect accessors itself
  (`SynthEditLib/CContainer.h:60-62`) and serialises its panel geometry as
  **`PanelWndPosition`** (`:214`). **That element was sitting in the chunk I
  measured**, on the `master_container`, and I read past it because I was
  grepping for `panelRect`.

**Learned — I measured the artefact and assumed the architecture.** The
measurement was careful: positive control, four sourced facts, a red herring
explicitly refuted. Then it concluded the code was broken, proposed a fork in
GATED shared code, and asked Jeff to rule between two options — all resting on
"three bare DSP modules in a rack document is what TIDE means to produce",
which I never checked and which is false. **One question first — what is a rack
module supposed to be? — would have replaced the row, the ruling request and
this correction.** Cheaper than any of the measuring I did.

**Learned — a wrong conclusion in a docs file is more dangerous than a wrong
row.** S15 would have been read as a decision awaiting Jeff, which is visible;
but `docs/s14-rect-measurement.md` reads as settled evidence, and its
"Mechanism, from sources" section is exactly the kind of thing a later run
trusts instead of re-deriving. It is corrected in place with the correction
marked as such, rather than left to be discovered — the same reason the journal
is append-only but a *document* must be fixed where it sits.

**Next:** the live work is **E2a**/**E2** — the prefab rack modules themselves —
and **E5**, rack-shaped styling for GUI-bearing modules, which is NEEDS-JEFF
because the visual language is his and PLAN constraint 8 means whatever is
chosen ships as *the* look. Unattended runs still have nothing substantial:
**A25** (four lines wiring A20's check into `lint.yml`) and **S13** (TIDE
cannot run as a Debug build, `assert(false)` at `UgDatabase.cpp:549`, GATED)
are the two smallest things that would change that, and both are Jeff's.

**Side effects on this box:** none. Docs and rows only, TideSynth only; all
eight repos on their default branch and clean.

**Branch/PR:** `tide/mac/S14-not-a-defect`.

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
