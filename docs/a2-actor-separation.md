# A2 — actor separation: what is done and what each box still needs

Status **NEEDS-JEFF**, platform **—**. Lifted verbatim out of the
[BACKLOG.md](../BACKLOG.md) row by **A8**, 2026-08-12, when that file had
reached 76 KB and every run on three machines was reading all of it. The row
now carries the decision-shaped summary and points here; this file is the
detail. Wording below is unchanged from the row — only the line breaks are
new.

---

**Actor separation — MOSTLY DONE 2026-08-09; only the per-box wiring remains,
and until it lands the whole item is inert.** Done: bot account `tide-rack-bot`
created and accepted as collaborator on all five repos; classic `repo`-scope
PAT `tide-fleet-actor` minted (expires **2026-11-07**); "Agent PRs only"
rulesets active on the four **public** repos (pull_request with 0 required
approvals, block force pushes, restrict deletions, `~DEFAULT_BRANCH` targeting,
Repository-admin bypass always). **Remaining: the per-box half of step 5.** The
mechanism is designed, proven on this box and in the prompt (STEP 0.7 +
"Becoming the agent" in
[docs/weekly-run-prompt.md](weekly-run-prompt.md)): git is pointed at `gh`
as its credential helper, so identity simply follows whether `GH_TOKEN` is
exported — bot when it is, Jeff when it is not, one setting correct in both
directions. **Each box still needs its own two-line setup: the `git config` and
the token in `~/.tide/agent-token`.** Windows: config done 2026-08-09, token
pending. macOS and Linux: both pending, and each is a short interactive
session. **Until a box has both, its runs still authenticate as Jeff and bypass
every ruleset via the admin exemption** — which is precisely why STEP 0.7
asserts `gh api user` returns `tide-rack-bot` and stops the run dead otherwise,
rather than letting the fallback pass unnoticed. **Two platform constraints
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
