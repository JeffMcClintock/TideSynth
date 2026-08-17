# Decision log

One line per decision, newest first. This is the single place a ruling lives;
PLAN.md carries the *reasoning* for the ones that shape the product, BACKLOG
rows carry the *work* they unblock, but the authoritative "what was decided,
when" is here. Agents cite this file instead of re-deriving rulings from
journal prose.

**How a new decision gets made (the Proposed mechanism):** an agent that hits a
genuine design fork opens a PR adding a `PROPOSED:` entry here — using the
template below — and parks the affected task. **Jeff's merge of that PR is the
decision.** Editing the entry before merging is answering differently. This
keeps decisions asynchronous, recorded, and impossible to mistake for made when
they are still open.

**Escalation template** for a `PROPOSED:` entry and for NEEDS-JEFF rows:

```
PROPOSED: <one-line question>
  Options: <a> / <b> / <c>
  Recommended default: <which, and why in one clause>
  Default in effect meanwhile: <what happens if nobody answers>
  May proceed meanwhile: <work that is identical under every option>
  Decide-by: <the event that makes the default permanent, e.g. "before R5 runs">
```

The "default in effect" line exists because of R1(a): an unanswered question
(may TIDE ship under the SynthEdit signing identity?) was silently becoming
answered-by-default as release work approached. Defaults are fine; *invisible*
defaults are not.

---

## Open — PROPOSED, awaiting a merge to become decisions

### May a scheduled run repair a build break in a GATED path?

Raised 2026-08-17 by Jeff, after the same standoff occurred twice in two days
on two different boxes ([#87](https://github.com/JeffMcClintock/TideSynth/issues/87)
linux, [#111](https://github.com/JeffMcClintock/TideSynth/issues/111) windows).
Jeff's framing: *"is it reasonable to allow fixing gated repo provided the
resulting PR is reviewed by me?"*

```
PROPOSED: may a scheduled run fix a build break whose cause is in a GATED
  path (SE16/EditorLib/, SE16/SynthEdit2/, the SynthEditLib repo), given that
  it cannot merge its own PR?
  Options:
    (a) No -- today's rule. File a platform issue and stop.
    (b) Yes, narrowly -- build-break repair only, under the six bounds below.
    (c) Yes, generally -- GATED becomes advisory; any item may edit shared
        code because everything arrives as a reviewed PR anyway.
  Recommended default: (b) -- review discharges the correctness and
    irreversibility risk the gate was mostly protecting, but not the
    attention-budget risk, which is what keeps (c) off the table.
  Default in effect meanwhile: (a). Runs file the issue and stop, so a
    break in shared code waits for Jeff or an interactive session, and each
    box rediscovers it in turn.
  May proceed meanwhile: everything. No backlog item is contingent on this;
    it changes only what a run does when it finds a broken default branch.
  Decide-by: before the next scheduled run on any box, or this decides
    itself -- the linux box already has two such issues waiting and will
    meet the same wall.
```

**The premise this rests on, and it is only two-thirds true today.** "It gets
reviewed" assumes a PR exists. Measured 2026-08-17:

| repo | GATED paths it holds | enforcement |
|---|---|---|
| `SynthEditLib` (public) | the repo itself | `main` protected, "Agent PRs only" ruleset **active** |
| `SynthEdit` (private) | `EditorLib/`, `SynthEdit2/` | **`master` unprotected, no ruleset** |

Private repos on the current plan cannot carry rulesets — the API answers
*"Upgrade to GitHub Pro or make this repository public."* So in `SE16` the bot
holds write access to `master` with nothing mechanical in the way; every PR a
run has opened there (#41, #20, #15) was voluntary compliance with the run
prompt. **If review is to be the control that replaces the gate, it has to
exist where the gate is being relaxed.** Either upgrade the plan so the same
ruleset covers `SE16`, or add a detection control — the A6 watchdog digest
flagging any `tide-rack-bot` commit on `SE16/master` that did not arrive as a
merge. Prevention is better; detection is cheaper; convention alone is what is
in force right now.

**The six bounds that make (b) narrow rather than (c) by another name:**

1. **Trigger only.** Your platform's default branch does not build, and the
   cause is in a GATED path. Not "I noticed something in EditorLib."
2. **Minimal restoration.** Prefer reverting the named commit. A forward fix
   only where a revert would remove working functionality — which is exactly
   [#111](https://github.com/JeffMcClintock/TideSynth/issues/111), where
   reverting `e14970e` would undo the macOS box's MIDI work.
3. **Nothing the break did not force.** No refactoring, no cleanup, no
   behaviour change, no "while I was in there."
4. **Its own PR**, GATED-only, never bundled with backlog work, naming the
   breaking commit and the verification.
5. **State which consumers were built.** `SynthEditLib` ships in SynthEdit as
   well as TIDE, so "TIDE builds now" is not evidence the commercial product
   is safe.
6. **Fall back to (a)** whenever the minimal fix is not obvious. A run that
   cannot state the fix in one sentence files the issue and stops.

STEP 3's *"do not fix build failures for a platform you cannot compile on"* is
untouched and orthogonal. It is the rule least worth relaxing, and #88 is its
current example: a Windows run caused it and only the linux box can verify the
repair.

**Why not (c).** The gate protects two different things and review only
discharges one. Correctness and irreversibility, it handles — nothing lands
without Jeff. Attention budget, it does not: (c) invites runs to generate
review load on the commercial product for work nobody asked for, and the
reviewer is the bottleneck the whole arrangement is trying to conserve. The
C8 ruling already declined to widen the gate as a side effect of a 30-line
header; (c) is that same widening with more steps.

**Why the gate is arguably over-tight today.** STEP 5's ALLOWED/GATED split
was written 2026-08-06 (G2). The bot identity and the branch rulesets landed
2026-08-09 (A2). So the gate was calibrated for a world in which a run pushed
as Jeff, carrying his bypass — and on `SynthEditLib` that world is genuinely
gone. It has not been re-examined since the thing that made it partly
redundant arrived.

**Disclosure.** This entry was drafted by an agent, which is the party the
looser rule benefits. Weigh it accordingly. The honest cost of (b) is review
load on commercial code plus the risk that a plausible-but-wrong fix reads
fine at merge time; the honest cost of (a) is this week — three boxes each
spending a session rediscovering the same standoff while the break sits on
`main`.

**What (b) would have done to the three open breaks:** repaired
[#87](https://github.com/JeffMcClintock/TideSynth/issues/87) (a revert) and
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88) (two lines, on
the box that can verify them); left
[#111](https://github.com/JeffMcClintock/TideSynth/issues/111) as a PR in
Jeff's queue, since a shared-code change Clang accepts and MSVC rejects is
exactly the case bound 2 sends to review.

---

## Decisions

| Date | Decision | Notes |
|---|---|---|
| 2026-08-15 | **C11 resolved: option (b)-shaped — narrow to an interface. TIDE needs no licensing.** `MfcDocPresenter.cpp` no longer depends on `SynthEditApp` or its Moonbase-named methods; it queries a new public `ILicenseState` instead | Ruling given directly ("narrow it to an interface. TIDE needs no licensing."). SynthEditApp implements the interface via thin forwarders for the real app; TIDE's own stub returns `nullptr` outright rather than routing through SynthEditApp's stubbed-false methods, so the code says *there is nothing to gate* rather than *gate, but always open* — same behaviour today, different claim, and the claim is the one that's actually true for a free product. `ModulePicker.h` (C11's part (b), not separately ruled) closed alongside it on the row's own reasoning. See BACKLOG C11 |
| 2026-08-15 | **S10 resolved: retire `SE_IOS_APP.xcodeproj`, lean on a generic AUv3 backend for gmpi_ui** | "It's a very old project. TIDE should lean on a generic AUv3 iOS backend for gmpi_ui as much as possible." All four targets already failed to build (RC=65 each), so deleting the `.xcodeproj` broke nothing working. **This also settles M2's shape**: no per-product Xcode scaffolding to revive means M2 is authored fresh, and the ruling's own direction points at a new `GMPI_Wrappers/wrapper/AUv3`-shaped sibling to the existing VST3/AU2/CLAP wrappers rather than a TIDE-specific rebuild. S9 (surgical cleanup inside the now-deleted project) is moot. See BACKLOG S9, S10, M2 |
| 2026-08-15 | **C12e resolved: option (b) — `Dialogs_editor2.cpp` comes off EditorLib's source list and each consuming app compiles it directly.** `Dialogs_editor.h` moves to `SynthEditLib` as both options assumed | Answered in session ("go with your recommendation"). **The recommendation was right but its stated reasoning was wrong on a load-bearing point, found by measuring before implementing:** the PROPOSED entry said the other consumers "each supply their own definitions", so (b) would cost only one vcxproj entry. **`SynthEditCL` does not** — its CMake target compiles `main.cpp`, **not** `CLApp.cpp`, and `main.cpp` carries a comment saying it deliberately relies on EditorLib for these stubs. So (b) as literally written breaks SynthEditCL's link. Implemented as (b) *properly* — the `SynthEditApp.cpp`/`ExportAsPlugin.cpp` pattern the option itself points at, where **every** app that needs the symbols compiles the file: `SynthEditCL/CMakeLists.txt` and `SynthEdit2.vcxproj` both gain an entry; `TideApp.cpp` and `layouttests.cpp` already define their own. **Two further corrections to the entry's facts:** the file defines **two** functions, not three (`doDialogBuildCodeSkeleton` is not in `Dialogs_editor.h` at all and belongs to S3), and there are **four** definitions in the tree, not five — `EditorScreenshot` and `SynthEditCL/main.cpp` only carry comments pointing at EditorLib's. Verified: 27 → 25 `${EDITOR_DIR}` entries, fresh Ninja tree **904/904 RC=0**, **TIDE.gmpi and TIDE_VST3.vst3 both link** — the specific thing C12e's Accept named. Execution: [SynthEdit#20](https://github.com/JeffMcClintock/SynthEdit/pull/20) + [SynthEditLib#9](https://github.com/JeffMcClintock/SynthEditLib/pull/9), which must merge together. See BACKLOG C12e |
| 2026-08-13 | **Constraint 1 reversed: the rack (Panel View rendered as a Eurorack case) is now TIDE's default top-level view, not the structure view.** Unlocking a module/Container takes the user into its own structure view to rewire signal flow; unlocking is still breadcrumb navigation, only the default rendering per level flips from structure to rack | Rides on SynthEdit's new "rack mode" (Panel View rendered as a Eurorack case, modules/Containers drag-and-snap into rack slots — already shipping in SynthEdit as an option, `SE16` `a056d3f5b`), which becomes TIDE's *only* top-level option rather than one of two. Makes TIDE's design closer to Cardinal (a rack UI over compiled-in modules) with the added feature of editing each module's own signal flow. The 2026-08-09 Eurorack-rack section of PLAN.md already gestured at "opening a Container is optional" as the differentiator; this is the concrete mechanism that fulfils it, and the piece that was still missing — constraint 1's own wording still said "No panel view" until now. **Open follow-ups, not resolved by this ruling alone:** whether the v0.1 acceptance test (PLAN.md, "What done looks like") should now be rack-first rather than structure-view-first; whether BACKLOG U1 ("close the gap to the one-view UX") needs rescoping around the rack as the default rather than the structure view. Full ruling: PLAN.md constraint 1 |
| 2026-08-13 | **C9 resolved: option (c) — `SynthEditLib` gets its own version header**, decoupled from `SE16/se_build_number.h` | TIDE Rack's release cycle is not tied to SynthEdit's, so it should not read SynthEdit's version number, and the header cannot move to the public repo without a `.github/workflows/**` edit no scheduled run may make. C4/C5 implement: new header (or compile definition) in `SynthEditLib` for the two live users (`ModuleFactory_Editor.cpp`, `SkinMgr.cpp` at C4; `Application.cpp` at C5); `se_build_number.h` stays exactly where SynthEdit's three release workflows expect it. See BACKLOG C9 |
| 2026-08-13 | **R1(a) resolved: TIDE ships under the existing `SynthEdit Limited` signing identity.** No second Azure certificate profile | Cost — a second cert profile is not affordable right now. Mitigated with a branding line rather than a new identity: **"TIDE Synth — by SynthEdit Ltd"**, placed subtly (about pane / footer / installer credit, not the plugin name itself — constraints 1 and 5 still rule out anything that reads as a splash or nag). Placement is a small design task, not yet filed — see BACKLOG note |
| 2026-08-11 | **C8 is Jeff's call, not the taking agent's — option (b).** `SynthEditLib/it_empty.h` is deleted; the STEP 5 GATED exception stays at "C1-C7" and was **not** widened | Answered by merging the `PROPOSED:` entry unedited ([#32](https://github.com/JeffMcClintock/TideSynth/pull/32)), then "go" in session. Option (c) — widen the exception to any C-series item — was considered and **not** taken: the gate protects the commercial repo and should not move as a side effect of a 30-line header. So a future non-C1-C7 item needing a GATED edit escalates the same way; C8 is not a precedent for reaching. Execution: [SynthEditLib#4](https://github.com/JeffMcClintock/SynthEditLib/pull/4) + [SynthEdit#10](https://github.com/JeffMcClintock/SynthEdit/pull/10), which must merge together. Evidence: [c8-it-empty-header.md](c8-it-empty-header.md) |
| 2026-08-09 | **P8 fixed directly on `SE16` master**, not on a branch — Jeff's call, in session, clearing the "check with Jeff before touching" gate that row carried | The bug was failing the SynthEdit **Store release pipeline** on master, not just local clean builds, so a branch awaiting review would have left shipping blocked for the wait. `SE16` has no branch protection (private repos need GitHub Pro), so master was reachable. This is the interactive-session convention C1b used, not a weekly run deciding for itself: **a scheduled run still must not push to main** |
| 2026-08-09 | Agent identity switches on `GH_TOKEN` presence, with git using `gh` as its credential helper; STEP 0.7 asserts `tide-rack-bot` or stops | One global setting, correct for both identities, nothing to toggle or restore if a run dies. Rejected: `gh auth switch` (global, strands Jeff as the bot on a crash), `settings.json` env (hits interactive sessions), token in remote URL (plaintext in `.git/config`) |
| 2026-08-09 | Agent identity uses a **classic** `repo`-scope PAT, not fine-grained; `workflow` scope withheld | Fine-grained tokens cannot serve a collaborator on repos they don't own (GitHub platform limit). Withholding `workflow` enforces the no-workflow-edits rule at the credential layer — A3/A5 need Jeff or a scope bump |
| 2026-08-09 | Branch rulesets: 0 required approvals, admin bypass always | The gate is "must go through a PR", not "must be approved" — self-approval is forbidden, so requiring 1 is ceremony at solo scale. SE16 unprotected: private repos need GitHub Pro |
| 2026-08-09 | Verbally-relayed decisions get a read-back confirmation before execution | The MIT/ISC flip-flop was a real public push of a misheard decision |
| 2026-08-09 | Process-review adoptions: actor separation, coordination auto-merge tier, watchdog, cadence raise — in that order | [process-review-2026-08-09.md](process-review-2026-08-09.md); rejected items listed there, do not re-file |
| 2026-08-09 | Product is **TIDE Rack**; **TIDE Synth** is the organisation; repo and domain keep the org name | Reaffirmation of 2026-08-08 ruling; README corrected |
| 2026-08-09 | tide-rack repo superseded and archived; Eurorack rack is a feature of TIDE Rack, not a second product | PLAN "The Eurorack rack"; harness salvaged as E1 |
| 2026-08-09 | Run prompt is fetched from origin/main at run time, not copied per machine | agent-setup.md; bootstrap per box, prompt sha in every journal entry |
| 2026-08-09 | Two end states, never a third: work on default branch, or pushed branch with an OPEN PR; checkouts restored | weekly-run-prompt.md STEP 5 |
| 2026-08-08 | Carve-out **C0 APPROVED**, with standing direction: keep as much ExportAsPlugin code private as practical | SynthEditCL stays private; C1b filed |
| 2026-08-08 | Naming forms: display "TIDE Rack" (space), shipped files `TIDE-Rack` (dash), CMake targets `TIDE_Rack` (underscore); never mixed | distribution.md; spaces in shipped filenames are unfixable in permalinks |
| 2026-08-08 | Keep `SynthEditLib` name; keep `TideSynth` repo name | Two "keep it" naming rulings |
| 2026-08-08 | X4 WONTFIX: leave `GIT_TAG origin/main` FetchContent pins alone; local overrides are the intended dev path | BACKLOG X4 carries the full reasoning |
| 2026-08-07 | Licence: **ISC**, both TideSynth and SynthEditLib | Same licence as GMPI and gmpi_ui; L1 resolved |
| 2026-08-07 | `gmpi_ui` and `GMPI_Wrappers` are ALLOWED paths for agents | G3; shared with SynthEdit — tight changes, rebuild SynthEditCL too |
| 2026-08-07 | No user skins, permanently (constraint 8) | Stricter than "defer skinning"; enforcement is S7 |
| 2026-08-07 | tide-rack scaffolded as separate repo | **Superseded 2026-08-09** — see above |
| 2026-08-06 | TIDE is **free**; funding by donation; no nagging | PLAN "Price and funding" |
| 2026-08-06 | Fixed module set, compiled in, all platforms (constraint 7) | module-enumeration.md |
| 2026-08-06 | ALLOWED/GATED split replaces blanket SE16 ban (G2) | PR #4 |
| 2026-08-06 | Three-machine weekly agent arrangement created | agent-setup.md |
