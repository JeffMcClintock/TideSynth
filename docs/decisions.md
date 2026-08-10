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

## Open

```
PROPOSED: Does an agent taking C8 have authority to delete SynthEditLib/it_empty.h,
          or is C8 a Jeff decision because both files it touches are GATED?
  Options: (a) C8 authorizes it — the row is TODO/any, so the fleet already meant
               an agent to do it, and the audit removes the judgement it needed
           (b) Jeff deletes it, or says "go" on this PR and a later run does
           (c) Widen the STEP 5 exception from "C1-C7" to "any C-series item"
  Recommended default: (b) — narrowest, and it is what the C8 row already asks
               for by calling for "a deliberate keep or a deliberate delete";
               (c) is rejected as a recommendation, see below
  Default in effect meanwhile: it_empty.h stays in the public repo, dead. Costs
               nothing functionally; every reader of the newly-public
               SynthEditLib meets a misleading 2002 orphan
  May proceed meanwhile: nothing further — the audit
               ([c8-it-empty-header.md](c8-it-empty-header.md)) is the whole of
               the work that is identical under every option, and it is done
  Decide-by: before C6 moves EditorLib/CMakeLists.txt into SynthEditLib. After
               that the file is on the public repo's own source list and
               deleting it stops being a two-repo change
```

**Why (c) is not recommended even though it is the tidiest.** Widening the gate
is a change to the rule that protects the commercial repo, made by the run that
happens to be inconvenienced by it — which is the shape of exactly the mistake
the gate exists to prevent. G3 is the precedent in the other direction: the P4
crash fix sat entirely in two unlisted repos, the run filed the scope question
instead of reaching, and Jeff answered in a day. If (c) is right it should be
decided on its own merits, not as a side effect of a 30-line header.

---

## Decisions

| Date | Decision | Notes |
|---|---|---|
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
