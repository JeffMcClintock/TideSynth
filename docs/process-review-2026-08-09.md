# Process review — 2026-08-09

A multi-agent review of the development process itself (not the product),
commissioned by Jeff: three lenses over the process docs, three web-research
agents (prior art, cloud options, community mining), and an adversarial critique
of every recommendation against this project's real constraints. 73 findings and
43 verdicts were distilled to what follows. This document is the reference the
**A-series** BACKLOG rows point at.

## Verdict

The architecture is right and matches industry consensus almost exactly:
agents open PRs and never merge; coordination through durable git artifacts;
fresh context per run; product decisions stay human. GitHub's Copilot coding
agent, OpenHands' own documentation, and every production deployment surveyed
use this same trust model.

What is wrong: **three load-bearing mechanisms are prose or broken**, and the
implemented human/AI split is close to the inverse of the stated one.

## The three broken mechanisms

1. **"Agents never merge / never push main" is enforced by nothing.** No branch
   protection exists on any of the five repos, and agents run under Jeff's own
   GitHub identity — to GitHub, the agent *is* Jeff. PR #24's close-by-direct-push
   already demonstrated the ambiguity. Fix: A2 (machine account + rulesets).
2. **The CI→platform-issue loop has never executed.** agent-setup.md calls it
   "the mechanism the whole arrangement exists for"; the workflow token is
   read-only, so `gh issue create` 403s invisibly under `continue-on-error`.
   Zero issues have ever been filed; every run's STEP 1 polls a feed that cannot
   fill. Separately, "green CI" is meaningless — `continue-on-error: true` at
   job level makes every failing run report success. Fix: A5 (+ B1).
3. **The secrets hole.** Eight signing secrets sit at repo level;
   `build.yml` triggers on push to `tide/**` — exactly what every agent run
   pushes — so a workflow edit on any agent branch executes with full secret
   access, ungated. Nothing in the prompt forbade agents editing
   `.github/workflows/**` (it does now). No workflow references the secrets yet,
   which makes the fix free. Fix: A1, and it is a precondition for everything
   else — more runs, auto-merge, and cadence all widen this hole until it closes.

## The inverted split

Stated: Jeff reviews PRs touching shared libraries, plus product decisions.
Implemented: Jeff is the merge gate for *everything* — all 25 TideSynth PRs to
date are coordination churn (journal, backlog, docs), many merged within
seconds (#23: created 23:57:01, merged 23:57:25), while the shared-lib gate is
enforced by nothing. Agent memory (journal/backlog) currently propagates
machine-to-machine at Jeff's merge latency.

The prior-art warning is specific: a study of 11,429 reviews of agent PRs
("Habituation at the Gate", arXiv 2606.22721) found approval rates rise
+14.5pp with reviewer exposure while inline-comment effort falls 22%.
Habituation, not any single bad merge, is how "human merges everything" fails —
merging trivia in seconds trains the reflex that eventually rubber-stamps a
shared-lib PR. Intercom's production tiering (19% auto-approved / 60%
agent-assisted / 21% human, zero reverts in the pilot) is the working model:
keep the human tier small enough to review honestly.

Where the line is correctly placed: the NEEDS-JEFF perimeter (licence,
carve-out, naming, signing, audio references) — and the record shows agents
genuinely respecting it, including refusing tempting cross-boundary fixes (P4)
and filing rather than fixing SynthEdit's own bugs (P6, P8).

## Sequenced plan (the order is the decision)

| Seq | Row | What | Why this position |
|---|---|---|---|
| 0 | A1 | Secrets → `release` environment (Jeff, minutes) | Gates everything; every other change widens the hole until closed |
| 1 | A2 | Machine account + branch rulesets on all 5 repos | Turns the three most important prompt rules into impossibilities |
| 2 | A3 | Always-green lint checks (journal/backlog shape, links, prompt-sha) | Prerequisite for any auto-merge; today "green" means nothing |
| 3 | A4 | Auto-merge tier for coordination PRs | Fixes memory-propagation latency and the habituation trainer |
| 4 | A5 | Fix the CI→issue loop (+B1 honest failure) | The repair loop must exist before cadence rises |
| 5 | A6 | Watchdog digest (GitHub cron, flag-only) | Silence becomes observable; also the "awaiting Jeff" surface |
| 6 | A8 | Journal rotation + backlog grooming | Growth scales with cadence; must precede the raise |
| 7 | A7 | Cadence 2×/box/week | Only safe after all of the above |
| 8 | A9 | Community research routine (PROPOSED-only) | New recurring workload; last |

Prompt-specification fixes (resume semantics, PR triage, working-tree safety,
stale-read fix, verification requirement, and others) landed with this review —
see the PR that added this document and the "Why the prompt is shaped this way"
section of [weekly-run-prompt.md](weekly-run-prompt.md).

## Rejected by the red team (do not re-file)

- **Moving builds to cloud or GitHub-Actions runners as agent hosts.** The
  boxes are not build machines; they are agent hosts — where a session with a
  native toolchain compiles, runs, fixes, re-runs. auval, DAW-in-the-loop, iOS,
  and the SE16 working tree cannot move, and claude-code-action would put an
  `ANTHROPIC_API_KEY` secret in exactly the repo A1 exists to shrink.
- **Merge queues / branches-up-to-date requirements.** At 3 PRs/week with
  agents asleep between runs, this converts a theoretical semantic-conflict
  risk into certain recurring human rebasing work.
- **Dependency-bot auto-merge policy.** No dependency bot exists or is planned;
  the one transferable idea (auto-merge only what checks genuinely verify) is
  already the A3/A4 design.
- **Committing encrypted secrets to the repo** (one researcher suggested it;
  the critique killed it). Permanent history outliving rotation, in a project
  where a PAT already leaked once.

## Cloud: the narrow yes

Cloud Routines (always-on Anthropic infra, Ubuntu x86_64) fix the one genuine
reliability hole — desktop tasks only fire while the Claude app is open, which
is why the Linux run fired ~10 hours late on 2026-08-09. Endorsed narrowly: as
the home for the *new* doc-only recurring work this review creates (A8
grooming, A9 research) against the public TideSynth repo, where the GitHub
proxy means no PAT enters the VM. **SE16 stays out of any cloud environment**
unless Jeff decides that boundary deliberately. The three boxes stay, for what
only they can do.

## Community research: currently zero, design ready

A9 specifies it. Minable sources with structured access: VCV Community
Discourse (JSON API), the VCV Library repo (manifest database of the 8,000+
module ecosystem), Cardinal and Surge XT issue trackers. The iOS AUv3 audience
lives on the Audiobus/Loopy Pro forums (quarterly human skim, no good API).
Reddit and KVR are demoted: low yield and ToS-hostile to bulk fetch. Output is
**PROPOSED** rows for Jeff's triage — the routine never posts, votes, or
decides. Best-fit process prior art: Surge XT (issues as backlog of record,
nightly builds); copy Cardinal's move of writing the product philosophy in 2–3
sentences as an explicit auto-reject filter.

**Standing product hypothesis (evidence-backed, decision is Jeff's):** no
open-source modular synth exists on iOS AUv3. Cardinal ships every format
except iOS; miRack is closed-source; VCV Rack has no iOS story. TIDE Rack's
target square is empty. A9's routine should watch for anyone moving into it.

## What the reviewers themselves missed (red-team additions, now tracked)

1. **Prompt injection** — STEP 1 made GitHub issues the top-priority input with
   no authenticity rule. Fixed in the prompt: act only on platform issues
   authored by Jeff or the CI bot.
2. **Local blast radius** — scheduled runs execute with Jeff's full `gh`
   credential and filesystem. Mitigation is A2; per-box permission narrowing is
   part of its install.
3. **No canary for prompt changes** — fetch-not-copy concentrates blast radius.
   Accepted risk for now; the PR gate on `weekly-run-prompt.md` is the control.
   A `weekly-run-prompt-next.md` canary box remains an option if a bad edit
   ever ships.
4. **Run provenance** — journal entries now record model + app version
   alongside the prompt sha.
5. **No fleet-pause mode** — STEP 0.5 now checks for a `FLEET-PAUSED` file at
   the repo root on origin/main.
6. **Windows box is a single point of failure for the carve-out** — SE16 lives
   there and every C-stage is `platform:win`. No recovery path is documented.
   Open; filed as a note on C3 rather than a mechanism.
7. **pages.yml audit** (done during this review): triggers only on push to
   `main` with `website/**` paths, minimal permissions — clean. One standing
   rule from it: a merge touching `website/**` IS a production deploy, so
   `website/**` must never enter an auto-merge allowlist.

## Manual checklist (Jeff-only steps)

### A1 — secrets, ~10 minutes, do first

The 8 repo-level secrets (verified 2026-08-09): `APPLE_CERT_P12_BASE64`,
`APPLE_CERT_PASSWORD`, `APPLE_ID`, `APPLE_ID_PASSWORD`, `APPLE_TEAM_ID`,
`AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`, `AZURE_TENANT_ID`. No workflow
references any of them yet, so nothing breaks.

GitHub cannot show a secret's value back, so this is re-enter, not copy — have
the original values at hand (they were entered 2026-08-08). If any value is
lost, regenerate it at its source rather than hunting for it.

1. github.com/JeffMcClintock/TideSynth → Settings → Environments →
   **New environment** → name it `release`.
2. In the `release` environment: tick **Required reviewers**, add
   `JeffMcClintock`, save. Optionally also restrict **Deployment branches and
   tags** to tags matching `v*`.
3. Still in the environment, under **Environment secrets**: add all 8 secrets,
   same names, same values.
4. Settings → Secrets and variables → Actions → delete all 8 **repository**
   secrets.
5. Done. When R2/R3 release workflows are written, their signing job declares
   `environment: release` — and every run of it then waits for your click.

### A2 — machine account, ~20 minutes

1. Create a GitHub account for the bot (e.g. `tide-rack-bot`; a plus-address
   like `mcclintock.jeff+bot@gmail.com` works for the email).
2. From each of the five repos (TideSynth, SynthEdit, SynthEditLib, gmpi_ui,
   GMPI_Wrappers): Settings → Collaborators → invite the bot with **Write**.
   Accept the invitations as the bot.
3. As the bot: Settings → Developer settings → Fine-grained personal access
   tokens → new token scoped to exactly those five repos, permissions:
   Contents read/write, Pull requests read/write, Issues read/write. 90-day
   expiry. Store it in your password manager — it goes to the boxes next, never
   into a repo or a transcript.
4. On each repo: Settings → Rules → Rulesets → new **branch ruleset** targeting
   the default branch: require a pull request before merging, block force
   pushes; **Bypass list: Repository admin**. You keep your direct-push
   interactive habit; the bot does not have it.
5. Per-box wiring (pointing the scheduled runs' `gh`/git at the bot token) is
   an interactive session on each box — ask the agent there to do it with you.

### One-time toggles and cleanups

- On all five repos: Settings → General → tick **Automatically delete head
  branches**.
- Merge [PR #25](https://github.com/JeffMcClintock/TideSynth/pull/25) (G5 — the
  Linux box is on the bootstrap once merged).
- Install G4 on the Mac (the two-minute bootstrap paste; BACKLOG G4 has the
  exact block to use). The Saturday run may do it itself, but its frozen prompt
  may read a stale backlog — doing it by hand is certain.
- Revoke the `cowork-linux-build-test` PAT if it has not already expired
  (github.com → Settings → Developer settings → Personal access tokens).
- Delete the leftover local `tide/win/C2-leaf-files` branches on this box
  (SE16 needs `-D`; its commit was rebased on merge), and the stale one on
  TideSynth's origin.
- Optional, when convenient: write the 2–3 sentence product philosophy
  (A9's prerequisite), and decide whether A8/A9 may run as cloud Routines
  (public TideSynth repo only; SE16 stays out of cloud).

## Notable sources

- GitHub Copilot coding agent docs — draft PRs only, `copilot/` branch prefix,
  workflow-edit approval gate, assigner-cannot-approve.
- "Habituation at the Gate" (arXiv 2606.22721) — reviewer habituation data.
- Intercom, "AI is approving our pull requests" — production 3-tier review.
- MAST taxonomy (arXiv 2503.13657) — multi-agent failure modes; why role-play
  agent pipelines (ChatDev/MetaGPT) underperform simple strong baselines.
- SWE-agent paper — agents succeed on interface quality, not model quality;
  invest in one-command build/test entry points.
- HumanLayer Ralph-loop reports — small frequent runs beat large infrequent
  ones ("overbaking"); nightly cadence with incremental merges.
- Surge XT / Cardinal project documentation — closest-neighbour process models.
- bors / GitHub merge queue lineage — rejected here as premature, revisit at
  real CI + higher volume.
