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

```
PROPOSED: Where does SynthEdit2/Dialogs_editor2.cpp go when C12 moves the rest
          of EditorLib's source list into the public repo?  (BACKLOG C12e)
  Options: (a) move it to SynthEditLib alongside Dialogs_editor.h, keeping
               today's arrangement;
           (b) take it off EditorLib's source list entirely and let
               SynthEdit2.vcxproj compile it directly, matching how
               SynthEditApp.cpp and ExportAsPlugin.cpp are already handled;
           (c) delete it — the three function bodies are already empty, and the
               other three consumers each supply their own definitions.
  Recommended default: (b) — the file is an app-level stub sitting in a shared
    library, and today it only works by static-library accident: TIDE links
    EditorLib (which contains Dialogs_editor2.obj) AND defines the same three
    symbols in TideApp.cpp, with no duplicate-symbol error solely because that
    object holds nothing else and so is never pulled in. Adding one symbol to
    that file breaks TIDE's link. (b) turns the accident into the deliberate
    arrangement the other two app-level files already use, for the cost of one
    vcxproj entry.
  Default in effect meanwhile: the file stays where it is, C12e is skipped, C12
    reaches 39 of its 41 entries, and C6 stays blocked.
  May proceed meanwhile: C12a, C12b, C12c, C12d and C12f — all five are
    identical under every option.
  Decide-by: before C6.
```

Filed 2026-08-14 by the windows box while scoping C12. Evidence and the full
five-definition table: [c12-remaining-editor-files.md](c12-remaining-editor-files.md).

---

## Decisions

| Date | Decision | Notes |
|---|---|---|
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
