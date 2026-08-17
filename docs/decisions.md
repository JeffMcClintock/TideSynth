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

*(none open)*

---

## Decisions

| Date | Decision | Notes |
|---|---|---|
| 2026-08-17 | **A7 closed WONTFIX: the 7x/week agent cadence is deliberate, and same-day extra runs are Jeff starting them manually** | Answered in session ("7 times is on purpose, the clashes are me starting extra runs manually. not a scheduling problem") after a scheduled run measured `cronExpression: 0 6 * * *` against a task description still reading *"Weekly ... (Sat 02:00)"* and flagged the mismatch. A7 had asked to raise cadence 1x -> 2x and had sat at NEEDS-JEFF while the fleet already ran 7x. **Consequences deliberately NOT reopened as work:** the stale task description is cosmetic; the concurrent-session collisions are expected, being manual overlapping runs rather than a defect. **The one live consequence is A8's** — journal rotation was sized against a 1x assumption ("192 KB across 37 entries in six days"), and that sizing is A8's to revisit. See BACKLOG A7, A8 |
| 2026-08-17 | **Constraint 9: lowest common denominator.** TIDE Rack only implements features implementable on its most restricted target (today iOS AUv3); no per-platform blessed exceptions | Ruled in session, prompted by E4's question of where a desktop user-prefab library may live: rather than bless specific folders on specific platforms, the rule is that a feature exists only if the strictest sandbox can host it. Elevates PLAN's existing "if it runs there, it runs anywhere" from observation to constraint 9. Supersedes E4's desktop-folder NEEDS-JEFF question: the per-device library follows the AUv3 container model on every platform or does not exist. See PLAN.md constraint 9, BACKLOG E4 |
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
