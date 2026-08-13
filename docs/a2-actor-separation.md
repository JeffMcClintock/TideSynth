# A2 — actor separation: what is done and what each box still needs

> **Correction, 2026-08-13 (later the same day, linux box, interactive session
> with Jeff).** The setup is **three steps, not two** — see **A11**. Steps 1–2
> are inert on any repo whose remote is `git@github.com:`, because a credential
> helper keyed to `credential.https://github.com.helper` is never consulted for
> an SSH URL. Such a push authenticates with **Jeff's key**, through his admin
> bypass, and nothing below detects it.
>
> **This includes the PR-authorship check that this file's own header rests on.**
> Commit author and committer come from the four `GIT_*` exports in STEP 0.7,
> which are set unconditionally; they say nothing about which credential moved
> the bytes. A push made over SSH as Jeff still arrives stamped
> `tide-rack-bot`, and `gh api user` still answers `tide-rack-bot`, because
> `gh`'s API path *does* read `GH_TOKEN`. Authorship proves authorship. It is
> not independent verification of authentication, and no evidence cited below
> is.
>
> Measured on the **linux** box: eight of nine repos were SSH
> (`SynthEditLib`, `SE16`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`, `GMPI_Adaptors`,
> `GMPI-plugins`, `gimpi_ui_tests`); only `TideSynth` was HTTPS. Linux therefore
> looked correct for one accidental reason — [TideSynth#34](https://github.com/JeffMcClintock/TideSynth/pull/34),
> the single PR cited for it below, is in the single repo the mechanism covered.
> Fixed on that box with a global
> `url."https://github.com/".insteadOf "git@github.com:"`.
>
> **Windows checked 2026-08-13, same session: clean, and now hardened too.**
> All 22 local repos under `C:\SE` — not just the fleet's usual 5 — already
> used HTTPS; nothing on that box was ever actually exposed. Applied the
> global rewrite anyway (belt and braces against a future SSH clone), then
> proved the wiring in both directions against the private `SynthEdit` over
> HTTPS: a bogus `GH_TOKEN` fails auth (exit 128 — the proof `gh` is
> genuinely consulted, not bypassed), the real bot token succeeds as
> `tide-rack-bot`, no token succeeds as `JeffMcClintock`. So for Windows,
> every "verified" below can now be read as authentication-verified, not
> just authorship-verified — see **A11**.
>
> **macOS remotes have still never been inspected**, and the macOS evidence
> below — `gmpi_ui#3`/`#4`, `GMPI_Wrappers#1`/`#2` — is drawn entirely from
> repos that were SSH on linux. Until someone runs
> `git -C <repo> ls-remote --get-url origin` on that box, **read every
> "verified" in this file as "authorship verified, authentication unknown"
> for macOS specifically.** STEP 0.7 now carries a second assertion that
> catches this; it did not exist when the claims below were made.

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
