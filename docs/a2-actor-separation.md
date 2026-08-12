# A2 — actor separation: what is done and what each box still needs

Status **DONE**, all three boxes, verified 2026-08-13. Originally lifted
verbatim out of the [BACKLOG.md](../BACKLOG.md) row by **A8**, 2026-08-12 —
that lift preserved the row's 2026-08-09 claim that macOS and Linux still had
tokens pending, and neither this file nor the row was ever updated as each box
actually finished setup. **The claim was stale on both counts.** Caught
2026-08-13 by checking PR authorship on GitHub directly rather than trusting
either document: every commit on every merged PR from all three boxes —
macOS ([gmpi_ui#3](https://github.com/JeffMcClintock/gmpi_ui/pull/3),
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1)/[#2](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/2),
[gmpi_ui#4](https://github.com/JeffMcClintock/gmpi_ui/pull/4), 2026-08-10
through 2026-08-12), linux ([TideSynth#34](https://github.com/JeffMcClintock/TideSynth/pull/34),
2026-08-11), windows (every `tide/win/**` PR since) — is authored **and**
committed as `tide-rack-bot`, matching each PR's own journal entry
(`## macos`/`## linux` on the dates above). That is exactly what STEP 0.7's
identity assertion exists to prove, verified independently of the assertion
itself. The rest of this file is kept for the mechanism and the two permanent
constraints, both still true.

---

**Actor separation — done on all three boxes as of 2026-08-13.** Bot account
`tide-rack-bot` created and accepted as collaborator on all five repos;
classic `repo`-scope PAT `tide-fleet-actor` minted (expires **2026-11-07**);
"Agent PRs only" rulesets active on the four **public** repos (pull_request
with 0 required approvals, block force pushes, restrict deletions,
`~DEFAULT_BRANCH` targeting, Repository-admin bypass always). The mechanism:
git is pointed at `gh` as its credential helper, so identity simply follows
whether `GH_TOKEN` is exported — bot when it is, Jeff when it is not, one
setting correct in both directions (STEP 0.7 + "Becoming the agent" in
[docs/weekly-run-prompt.md](weekly-run-prompt.md)). Each box's two-line setup
— the `git config` credential-helper line and the token in
`~/.tide/agent-token` — is done: windows 2026-08-09, macOS and linux
independently confirmed working by 2026-08-10/11 respectively, contrary to
what this file previously said. **A box without the token authenticates as
Jeff and bypasses every ruleset via the admin exemption** — which is
precisely why STEP 0.7 asserts `gh api user` returns `tide-rack-bot` and stops
the run dead otherwise, rather than letting the fallback pass unnoticed; that
assertion is what makes it possible to state the above as *verified* rather
than *assumed*. **Two platform constraints
found the hard way, both permanent, full detail in
[docs/process-review-2026-08-09.md](process-review-2026-08-09.md):**
**(a)** a fine-grained PAT *cannot* work here — GitHub documents the gap for
"repositories where the user is an outside or repository collaborator", and the
bot owns none of these repos — hence a classic token, which is all-or-nothing
across whatever the bot can reach, and which deliberately lacks `workflow`
scope, so **A3 and A5 need Jeff or a temporary scope bump**; **(b)**
`SynthEdit` (SE16) **cannot have a ruleset at all** — private repos need GitHub
Pro (403 *"Upgrade to GitHub Pro or make this repository public"*), so the
commercial repo the ALLOWED/GATED boundary exists to protect is the one place
protection stays prose-only. **Both are fixed by moving the repos under an
organization**, which is now the recommended follow-up and does not force any
rename.
