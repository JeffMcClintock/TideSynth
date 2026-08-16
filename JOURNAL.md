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

## 2026-08-16 — macos — A9

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **A9** — the community research routine,
[scripts/community-research.py](scripts/community-research.py), with
[docs/community-research.md](docs/community-research.md) and a human-editable
[rejection memory](docs/community-research-rejected.md). **Third item this
session, at Jeff's direction** ("keep working"); a scheduled run still takes
exactly one. Also flipped **D5** to DONE — Jeff updated the Ko-fi page mid-session
and it is verified below.

**The guardrails are structural rather than policy**, which is the part worth
keeping. A9 lists them as hard rules, so `_get()` is the only network call in
the file and can only issue GET: **there is no write path to disable.** Posting,
voting or DMing would require adding the capability first, which is the point at
which a human says no. Courtesy rate 1.5s process-wide, identifying User-Agent,
https-only, and the script **prints** — it cannot edit `BACKLOG.md`.

**Result — three things the first LIVE run found that no selftest would have.**
This is the entry's real content, because all three looked fine in isolation.

1. **It auto-rejected a real Surge XT crash report.** *"Surge XT CLAP crashes
   REAPER on load (SIGSEGV in JUCE repaint)"* was dropped under "constraint 2 —
   the DAW owns I/O", because somewhere in the reporter's diagnostics was the
   phrase *"The standalone app also works fine"*. An incidental mention in a bug
   report is not a feature request, and **discarding crash reports is the worst
   thing this filter could do.** Constraint rules now read the **title** only;
   the hypothesis flag still reads the body, because its failure mode is the
   opposite and much cheaper. Both pinned by selftest cases built from **the
   real incident**, not an invented example.
2. **Output was sorted ascending by date**, burying 2026 items under 2024 ones.
   Worth naming because **it looked like a fetch bug and was not** — I checked
   the raw API response and it returns newest-first; the defect was entirely in
   my display sort. Ranking is now hypothesis-first, then engagement, then
   recency, all descending.
3. **A passive scan cannot serve the standing hypothesis at all.** Across **48
   real items** (30 forum topics + 18 Cardinal issues) the hypothesis filter
   matched **zero** — an iPad thread appears on that forum roughly once a year.
   So a "watch" built on reading the newest topics would have looked like a
   working watch that had simply found nothing, which is the exact failure shape
   this project keeps hitting.

**Learned — the fix for (3) is that the watch has to SEARCH, and searching needs
one more correction than it looks like.** A `watch` source now queries Discourse
search directly and finds the signal on the first call: *"VCV Rack for iPad -
2025?"*, *"VCV Rack on iOS/Android devices?"*, *"How are you connecting/using
VCV with an iPad?"*. But Discourse search matches **post bodies**, so the top
hits were the forum's megathreads — *"What are you listening to?"* (6,135
replies) and *"Member Introductions"* — which merely contain "iPad" somewhere
across thousands of posts. **Requiring the match in the topic TITLE cut 54 hits
to 17, all on-topic**, and watch items rank by recency rather than engagement,
because the hypothesis is about someone moving into the gap *now*.

**Verified:** selftest **17/17** (offline); a live run across all five sources;
and the rejection memory proven by **A/B** rather than by reading the code —
with `surge#7782` listed a run reports `1 already rejected before · 24
proposed`, and with the file removed the same run reports `0 · 25` and the item
reappears.

**Measured in passing, and it corrects a doc:** the VCV ecosystem is **553
plugins and 4,958 modules**.
[docs/process-review-2026-08-09.md](docs/process-review-2026-08-09.md) describes
it as *"the 8,000+ module ecosystem"* — roughly 1.6× over. Not edited there, since
that document is a dated record of a review; the correction lives in A9's row and
in the new doc.

**The limitation I did not paper over.** Ranking is engagement, which is a proxy
for *worth a glance*, not for relevance to TIDE — so other projects' housekeeping
("Do a windows arm64ec build", "Release checklist for Surge XT 1.4") still
reaches the output. **The routine filters what TIDE has ruled out; it does not
judge what TIDE needs, and it should not pretend to.** Triage stays human, which
is what A9's PROPOSED-only design asks for anyway. A relevance signal is the next
real improvement and wants thought rather than more regexes. Stated at the top of
the doc's limitations section, not buried.

**D5 — DONE, and verified rather than taken on trust.** Jeff updated the Ko-fi
page during the session. It now renders as **"Jef [TIDE Rack]"** with the title
*"Buy Jef [TIDE Rack] a Coffee"*; an hour earlier it was plain *"Jef"* with
nothing naming the product. Checked in a real browser, which is the only way —
this session established that Ko-fi 403s unfamiliar user-agents **even for
handles that do not exist**, so `curl` cannot answer the question. The website's
*"Donate to TIDE Rack on Ko-fi"* link now lands somewhere that agrees with its
own link text. Flipped on the D2 branch, because D5 is defined there and does not
exist on `main` yet.

**STEP 1 / 1.5:** no `platform:mac` issues. [#69](https://github.com/JeffMcClintock/TideSynth/pull/69)
is open and is this session's own — `lint` green, and its three red build checks
are the pre-existing **B1** condition (all three die at Configure because
TideSynth has no top-level `CMakeLists.txt`), so it is waiting for merge rather
than for work. This branch is **stacked on it**, as the 61→64 stack was.

**A concurrent run exists, and I found it late — say so plainly, per the prompt.**
[#70](https://github.com/JeffMcClintock/TideSynth/pull/70)
(`tide/win/C11-S10-rulings`, also `tide-rack-bot`, opened 00:42Z) was created
*after* I opened #69, and I only noticed it because it took the PR number I had
predicted. It touches `BACKLOG.md`, `JOURNAL.md` and `JOURNAL-2026-08.md` — the
same three coordination files every run edits — so **conflicts with this stack
are expected, and merge order matters.** It is a different item set (C11, S10,
S9, M2, A16), not a duplicate claim, so nothing was wasted.

**It does supersede one thing I wrote above and in the NEXT row**: #70 rules
**S9 → WONTFIX** and **S10 → IN-REVIEW** (retire, not revive), and rescopes
**M2**. My screening said the remaining `mac`-only rows were "GATED or Jeff's",
naming S9 and S10 — that conclusion still holds (neither is available work), but
the *reason* for S9/S10 changes once #70 lands. **I did not rebase this stack
onto #70.** The prompt says to make your branch a delta on top of theirs when you
collide, and that is written for colliding on the same *item*; here the overlap
is only the shared coordination files, which is the ordinary condition for every
run. Rebasing three stacked branches onto a fourth unmerged one would make all of
them depend on #70 merging first, for no gain. Flagged on the PR instead so
whoever merges sequences it.

**Next:** **P5** for the mac box. Its scope got cleaner this session without
anyone editing it: [docs/about-pane.md](docs/about-pane.md) now says the about
pane is a *third* surface that does **not** change the host-visible plug-in name
or the vendor string, so P5 owns exactly two fields. **Whoever next runs the
research routine should read its limitations section first** — the output is a
proposal list, and treating it as a to-do list is the way this becomes noise.

**Side effects on this box:** none outside the scratchpad. TideSynth was the only
repo committed in; `SynthEdit` was read only, and `SynthEditLib`, `gmpi_ui` and
`GMPI_Wrappers` were untouched. The routine made read-only GET requests to
community.vcvrack.com, api.github.com and raw.githubusercontent.com; **nothing
was posted, voted on, or logged into.**

**Learned — do not predict your own PR number, even to avoid a placeholder.** The previous entry's fix for the auto-merge race was to finish STEP 4 *before* opening the PR, which means writing the number before it exists. I wrote #70; GitHub issued **#71**. Predicting is the same defect as a placeholder wearing a plausible disguise — and worse, because a wrong-but-real number links to someone else's PR rather than looking obviously unfinished. **The rule that actually works: push STEP 4 first, open the PR, then correct the number in a follow-up commit on the same branch.** Safe whenever the PR cannot auto-merge before you get there, which is any PR touching `scripts/` or `website/`.

**Branch/PR:** [TideSynth#71](https://github.com/JeffMcClintock/TideSynth/pull/71),
stacked on [#69](https://github.com/JeffMcClintock/TideSynth/pull/69) and
retargeting to `main` as its parent merges.

---

## 2026-08-16 — macos — D2

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **D2** — placed the *"TIDE Synth — by SynthEdit Ltd"* credit. **Second
item this session, at Jeff's direction** ("do the next task"); a scheduled run
still takes exactly one. Both halves the row named: the website footer, which is
real code, and the plugin-side placement, which is a spec only.

**The design decision worth recording is that D1 and D2 were about to get two
different answers to one question.** Both need "a subtle surface that is not a
dialog", and D1's note had already argued one into existence. So this run wrote
[docs/about-pane.md](docs/about-pane.md) as the *single* answer — **one about
pane off the breadcrumb bar holding exactly four things** (version, credit,
donation, licence). The fixed list is the point: an about pane is where things
get *put*, so it is precisely the surface that accretes until it becomes the
splash screen PLAN forbids. Adding a fifth item now needs a ruling.

**Result — website half.**

| check | result |
|---|---|
| footer wording | **"TIDE Synth — by SynthEdit Ltd"** — R1(a) verbatim |
| tag balance | OK |
| subresource tags (`script`/`img`/`link`/`iframe`/…) | **zero** — the page's "no external requests" promise is intact |
| `https://www.synthedit.com/` | **200, no redirect** |
| live outbound links | 4, all real |

The credit is a plain `<a href>`, which is the same settled arrangement as the
existing ko-fi and github links — an outbound href loads nothing, so linking
does not cost the page its zero-request property.

**Learned — a raw grep of `website/index.html` finds `#donate-url-tbd`, and it
is a false alarm both README and page comments will make you doubt.** That
string is the placeholder the W1 history says was removed; it survives **only
inside an HTML comment** describing the old mistake. Stripping comments before
counting shows **0** occurrences in live markup. Anyone auditing that file
should strip comments first — the file is more comment than markup by volume,
deliberately, so raw greps mislead in both directions.

**Learned — `curl` cannot tell you whether a Ko-fi handle exists.** Ko-fi
returns **403 to any user-agent it dislikes, including for handles that plainly
do not exist** — checked with a deliberately nonsense handle as a control, which
also returned 403. So a status-code check proves nothing, and the website
README's standing rule ("confirm the URL resolves before committing it") needs a
real browser to satisfy. Done that way here.

**Two things found by actually opening it, neither of which a code reading would
have surfaced:**

- **The Ko-fi page does not identify itself as TIDE Rack's.**
  <https://ko-fi.com/tiderack> renders as *"Buy Jef a Coffee"*, display name
  **"Jef"**, bio *"I'm a dude in New Zealand"* — nothing naming TIDE Rack, TIDE
  Synth or SynthEdit. **This defeats D2's own justification one hop later**: the
  website's link text is *"Donate to TIDE Rack on Ko-fi"*, so a user meets
  exactly the unexplained-identity surprise the credit exists to prevent. Filed
  as **D5**, `NEEDS-JEFF` — it is account settings and needs the password.
- **`ko-fi.com/TideRack` (website) and `ko-fi.com/tiderack` (Wayland code,
  `WaylandMainWindow.cpp:50`) reach the same page** — Ko-fi canonicalises to
  lowercase. The inconsistency is harmless; recorded so nobody "fixes" one of
  them and re-checks this.

**Prior art reused rather than reinvented:** `WaylandMainWindow::showAbout()`
(`:953-959`) already puts version, company and donation URL in one place as
plain text. TIDE takes the **content** and not the **container** — that one is
`SeMessageBoxAsync`, a modal dialog, which constraint 5 rules out. The spec says
so explicitly, because the temptation on implementation day will be to copy the
function.

**Sequencing — I applied last run's lesson and it worked.** The previous entry
learned that A4's auto-merge can merge a PR mid-STEP-4, and the fix was to
finish STEP 4 and push it **before** opening the PR. Done that way here: this
entry, the backlog and the D5 row were all committed and pushed first, so the PR
was complete the moment it existed. **No placeholder was ever pushed** — the
`__D2PR__` substitution happened before the first push, not after it.

**This PR will not auto-merge, and that is correct.** It touches `website/**`,
which A4's allowlist deliberately excludes because **a merge there IS a
production deploy of tidesynth.com**. So it waits for Jeff, unlike the last two.

**STEP 1 / 1.5:** re-checked at the start of this item — no `platform:mac`
issues, no open PRs in any of the five repos (both of this session's earlier PRs
had already auto-merged).

**Next:** **A9** for the mac box — the D-series is now exhausted for a scheduled
run (D1/D2 IN-REVIEW; D3/D4 GATED in `SE16/EditorLib/CMakeLists.txt`; D5 needs
Jeff's Ko-fi password). A9 is `any`, unblocked, PROPOSED-output-only, and should
be budgeted as a design session first. **D5 is small and worth doing before
v0.1 links that page from inside the plugin as well as from the website.**

**Side effects on this box:** none outside the scratchpad. TideSynth was the
only repo committed in; `SynthEdit` was read (`WaylandMainWindow.cpp`) and not
written, and `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were untouched — all
four confirmed clean and on their default branches. Two public pages were
fetched read-only (ko-fi.com, synthedit.com); nothing was posted, and no account
was logged into.

**Branch/PR:** [TideSynth#69](https://github.com/JeffMcClintock/TideSynth/pull/69).

---

## 2026-08-16 — macos — D1

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **D1** — the in-plugin donation affordance design note,
[docs/donations.md](docs/donations.md). Answered the row's first question
("can an AUv3 open a URL at all") by measuring it on this box, then designed
against the answer. Also filed **D3**/**D4**, archived seven landed rows, and
re-pointed three NEXT rows.

**This was a resumed claim, not a fresh one, and the branch is why.**
`tide/mac/D1-donation-affordance` was already on the remote carrying exactly
one commit — the DOING mark, pushed 2026-08-15 06:05, **no journal entry, no
PR, no work commits**. A previous macOS run died immediately after claiming.
Author date was 23h 59m old at the start of this run, i.e. the previous
firing of this same scheduled task. Per the resume rule (own platform → mine
to continue) I rebased that commit onto `main` — it conflicted, because `main`
had grown a **P9** row directly beneath D1's — and carried on. **That rebase
rewrote a pushed commit and needed `--force-with-lease`.** I judged that safe
and it is worth stating plainly rather than burying: the commit was a one-line
status flip, on a mac claim branch, with no PR and nothing built on it, and
its content survives byte-identical. It is *not* the case the prompt forbids
(re-authoring someone else's commit, or rewriting under a live session). If
anyone disagrees, the cheaper alternative was a merge commit.

**Result — two independent measurements, each with its own control.**

*1. Compile-time*, via `-fapplication-extension` (what Xcode sets for any
appex target — the real gate, not a proxy). Xcode 26.6.

| # | target | code under test | flag | clang exit |
|---|---|---|---|---|
| 1 | macOS | `NSWorkspace openURL:` + `activateFileViewerSelectingURLs:` | yes | **0** |
| 2 | iOS | `[[UIApplication sharedApplication] openURL:...]` | yes | **1 — hard error** |
| 3 | iOS | *same source as #2* | **no** (control) | **0** |
| 4 | iOS | `[vc.extensionContext openURL:completionHandler:]` | yes | **0** |

Row 3 is what makes row 2 mean anything. The diagnostic:
`error: 'sharedApplication' is unavailable: not available on iOS (App
Extension)`, `UIApplication.h:87`.

*2. Runtime*, because compiling is not permission. Ad-hoc-signed `.app`
carrying **only** `com.apple.security.app-sandbox`, versus an identical
unsandboxed build. **Sandbox proven active before any result was read** —
`NSHomeDirectory()` redirected to
`~/Library/Containers/com.tidesynth.d1lsprobe/Data`. Result: **identical in
both** — `https` handler resolved to Chrome, and a URL genuinely launched,
confirmed by a marker file at the far end.

**So: iOS closes the UIKit route at compile time; macOS closes nothing.** The
design consequence is the whole note — the affordance must not *depend* on
opening a URL. Recommended is an About pane off the breadcrumb bar
(constraints 1 and 5) with the URL as text plus a Copy button, and the click
as progressive enhancement only.

**Learned — a `success` return from `openURL` does not mean the URL arrived,
and this cost an hour.** With the probe's handler receiving URLs the *legacy*
way (`kAEGetURL` Apple Event), the sandboxed run returned `app=yes, err=none`
and **the marker never appeared**. That reads exactly like a sandbox denial and
is not one: Apple Events need
`com.apple.security.automation.apple-events`; URL *opening* does not. Moving
the handler to modern `application:openURLs:` delivery — what browsers actually
use — made both runs identical. **Anyone re-testing this must check the far
end, not the return value.** I would have filed the opposite conclusion if I
had trusted the API.

**Learned — the mac box has no mac-only work left that it may actually do.**
Screening the queue for this run's NEXT re-point: every remaining `mac`-labelled
row is GATED or Jeff's — S9 and S10 are the shared `SE_IOS_APP.xcodeproj`
(S10 being a revive-or-retire decision), and **D3**, filed by this run, is
`SE16/EditorLib/CMakeLists.txt`. So a mac run's real queue is the `any` pool,
and this box's distinctive value is answering questions the other two cannot —
which is what D1 was. Worth knowing before someone re-points that row again.

**Learned — check the ID column before filing a new row.** I wrote the note
referring to its two findings as D2 and D3; **a D2 already existed** (the
SynthEdit Ltd credit). Caught only because `check-id-refs.py` and a grep of the
ID column disagreed with my draft. Renumbered to D3/D4. `check-id-refs.py`
cannot catch this — a *duplicate* ID is not a *stale* reference, and both rows
would resolve. One grep of `^| D[0-9]` costs nothing.

**Two GATED findings, filed not fixed** (STEP 5: do the allowed-side part, file
the gated part naming the exact file):

- **D3** — `SE16/EditorLib/CMakeLists.txt:161-166` adds `browseto.mm` and
  `openurl.mm` under plain `if(APPLE)`. Both `#import <AppKit/AppKit.h>`, and
  **AppKit does not exist on iOS**, so an iOS EditorLib build fails to
  *compile* — earlier and more basic than the sandbox restrictions
  [docs/design-notes.md](docs/design-notes.md) anticipated. **This lands on
  M2**, which is written as though the iOS target merely needs building.
- **D4** — `gmpi::browse_to` has **zero** call sites across `SynthEdit`,
  `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers`, yet `browseto.mm` is compiled
  into every Apple build. (`gmpi::open_url` has real callers, so the grep is not
  simply missing them.)

**Prior art the row did not know about**, and the next person designing this
should not re-derive: `kDonationUrl = "https://ko-fi.com/tiderack"` already
exists at `SynthEditWayland/WaylandMainWindow.cpp:50`, wired to a Help-menu item
(`:276`) **and shown as plain About-box text** (`:957`). The recommended
fallback already ships. What TIDE cannot copy is the container — that About box
is `SeMessageBoxAsync`, a modal dialog, which constraint 5 rules out.

**Learned — A4's auto-merge is live now, and it can merge your PR out from
under you before STEP 4 finishes. This run hit it, and the next one will too.**
I opened [#67](https://github.com/JeffMcClintock/TideSynth/pull/67) after the
code/backlog commits so the D1 row could cite its own PR number, then wrote the
journal. **`auto-merge.yml` squash-merged #67 while I was still writing it** —
merged 18:23:16Z by `app/github-actions`, and it was eligible precisely because
this is a docs/journal/backlog-only PR, which is the tier's whole purpose. Two
consequences, neither obvious in advance:

  - **The journal entry missed the merge**, so the handoff — the thing STEP 4
    calls the product of a run — did not land with the work. Recovered by a
    second PR, [#68](https://github.com/JeffMcClintock/TideSynth/pull/68), off
    the new `main`.
  - **A placeholder reached `main`.** I had written `PR __D1PR__` into the D1
    row intending to substitute the number once the PR existed; the substitution
    was in the *journal* commit, so the squash carried the placeholder onto the
    default branch. #68 fixes it. **Never commit a placeholder** now that a
    merge can happen without a human in the loop.

**The durable fix is ordering, and it is one line: finish STEP 4 completely —
journal included — and push it BEFORE opening the PR.** Cite the branch in the
row and let the PR number be added by whoever needs it, or accept that the row
names its PR only in the follow-up. The old sequence (push, open PR, then
write up) was safe only while every merge waited for Jeff. It no longer does.

**STEP 1 / 1.5:** no `platform:mac` issues; **no open PRs in any of the five
repos** at the start of this run. Issue [#44](https://github.com/JeffMcClintock/TideSynth/issues/44)
("Fleet watchdog digest", `github-actions`) is open and unlabelled — not
platform work, noted and left.

**STEP 4 bookkeeping, since it was unusually large.** Every IN-REVIEW row's PRs
had merged, so seven rows flipped to DONE and moved to
[BACKLOG-DONE.md](BACKLOG-DONE.md): C12b, C12e, A4, A10, A14, A15, P9. **A4 was
not flipped merely because its PR merged** — its row demanded watching
auto-merge take one PR and leave one alone, and that is now real traffic:
`auto-merge.yml` runs at 00:46:48 / 00:48:18 / 00:49:32 are each followed within
2-10 seconds by [#62](https://github.com/JeffMcClintock/TideSynth/pull/62),
[#63](https://github.com/JeffMcClintock/TideSynth/pull/63),
[#64](https://github.com/JeffMcClintock/TideSynth/pull/64) merging as
`tide-rack-bot`, while [#65](https://github.com/JeffMcClintock/TideSynth/pull/65)
— `.github/workflows/**`, denied by its allowlist — was left for Jeff. **A7 was
re-pointed from `BLOCKED(A4)` to `NEEDS-JEFF`**, not left alone: A4 going DONE
would have made it claimable by the status column, and its own text says the
remaining work is per-box cron edits a scheduled run cannot do for the two
machines it is not on. Same shape as C6's blocker correction.

**Next:** **D2** for the mac box — the credit placement, whose own row names
D1's landing as its precondition, and which lands on the **same About pane**
D1 just designed; doing them apart risks two answers to one placement question.
The two open questions D1 could not close are stated in the note with what
would close each: the macOS **appex** sandbox profile (needs a buildable AUv3,
so after S10 is ruled) and whether `extensionContext.openURL` succeeds from an
iOS AUv3 at runtime (needs a device or simulator test). **Neither changes the
recommended design** — that was deliberate — so nothing downstream should wait
on them.

**Side effects on this box:** all probe artifacts removed — the throwaway
`.app`, its sandbox container, the marker files, and the LaunchServices
registration for `x-tide-donate-probe:`, which was unregistered and no longer
resolves (`https` handling verified unchanged). Sources stayed in the session
scratchpad and were deliberately **not** committed; the method in the note is
enough to rebuild them. TideSynth was the only repo committed in. `SynthEdit`,
`SynthEditLib` and `gmpi_ui` were **read only** and were clean before and after.

**Branch/PR:** [TideSynth#67](https://github.com/JeffMcClintock/TideSynth/pull/67) (the note, backlog and archive — **already auto-merged**) and [TideSynth#68](https://github.com/JeffMcClintock/TideSynth/pull/68) (this entry, plus the `__D1PR__` placeholder #67 carried onto `main`).

---

## 2026-08-15 — windows — C11, S9, S10, M2 (interactive session, Jeff ruling)

**Did:** Two rulings from the same session, both against carve-out/product
questions that had been sitting open. **C11**: narrow the private licence gate
to a public interface, TIDE needs no licensing. **S10**: retire the dead iOS
Xcode project, lean on a generic AUv3 backend for `gmpi_ui`. Also corrected
**S9** (moot) and **M2** (rescoped) as direct consequences of the S10 ruling.

**This entry's own production hit the collision this session has now hit
twice — worth reading before the content, because it changed how the work got
verified.** Mid-C11, a `git add`+`git commit` in the shared `SE16` checkout
produced a commit containing only 1 of my 5 changed files — the other four
(`SynthEdit2.vcxproj`, `SynthEditApp.h`, `SynthEditApp.cpp`,
`TideAppStubs.cpp`) were present and correct in the working tree throughout,
but silently absent from the commit. Not a wrong-branch problem this time — a
**race on the shared git index** with the other session's concurrent
operations. Recommitted immediately, verified via `git show --stat` before
doing anything else, confirmed clean. **Filed as A16**, since A14's assertion
(commit authorship) does not catch this — the commit it flags is correctly
authored, just short.

The recovery method from this morning's collision generalised cleanly:
cherry-pick onto a fresh worktree off the current default branch, verify the
full diff against that branch by explicit SHA (not a symbolic ref, and not
trusting a "pushed" echo — one push silently failed against a broken
worktree, caught only by re-fetching and diffing from a completely separate
repo location), push, and only then remove the worktree. Every one of today's
four code branches (`SE16`×2, `SynthEditLib`×1, and this pattern reused a
third time within the same hour) went through this, and every one was
verified from outside the worktree that produced it before being trusted.

### C11

**Result.**

| check | result |
|---|---|
| `SynthEdit2.vcxproj` change | `ModulePicker.h` repointed to `..\..\SynthEditLib\ModulePicker.h`, matching C4's `ModuleBrowser.h` precedent |
| fresh worktree build, TIDE-only targets | **`TIDE.gmpi` and `TIDE_VST3.vst3` both link** |
| `GetLicenseState` reaches the linker | confirmed present in `TIDE.dir`'s compiled objects, not inferred from source |
| `dsp_tests` / `ui_tests` (the suites not blocked by the build issue below) | **11/11** |

**Learned — TIDE's stub was already correct in behaviour and I nearly made it
correct in behaviour for the wrong reason.** `TideAppStubs.cpp` already
returned `false`/`false` for `isMoonbaseEnabled()`/`licenseIsActive()`, so the
menu item was never grayed for TIDE before this change. The easy path would
have been routing `GetLicenseState()` through those same stubbed methods —
same runtime result, less code. Jeff's ruling said something stronger: *TIDE
needs no licensing*, not *TIDE's licence check always passes*. So
`GetLicenseState()` returns `nullptr` outright for TIDE, and the calling code
never asks the question at all. Behaviourally identical today; the two would
diverge the moment anyone ever added a real gated feature to either side, and
only one of them is actually what was decided.

**Learned — `isLicensed()` had to be non-`const` in the interface, and that's
not cosmetic.** The underlying `licenseIsActive()` is non-`const` (it can
refresh cached activation state), and `hasGatedFeatures()`'s underlying
`isMoonbaseEnabled() const noexcept` is `const`. An interface that forced both
to the same const-ness would either lie about one of them or fail to compile;
mixed const-ness across the two methods is the honest shape.

**A finding NOT acted on, deliberately left for the next run to hit knowingly
rather than blind:** a full build fails at `EditorScreenshot/ScreenshotRenderer.cpp`
— `se::DeviceContextLegacyAdapter: cannot instantiate abstract class` — because
`gmpi_ui`'s in-flight `ITextLayout` work (`d3bacf3`) added a pure-virtual
`drawTextLayout` that `SynthEditLib`'s own adapter (still uncommitted in the
shared checkout) hasn't caught up to yet. **Proven unrelated to C11 by two
independent A/B builds**: `origin/master` + `origin/main`, zero C11 content,
against the same `gmpi_ui`, fails at the identical file and lines. This is the
same cross-repo instability C12e's journal entry flagged this morning, now
manifesting as a hard compile error rather than a runtime `bad_alloc` — it has
gotten worse, not better, since then. `SynthEditCL` and anything depending on
`EditorScreenshot` cannot currently be verified by anyone until that work
lands; TIDE's own targets don't depend on it and were the ones actually
checked.

### S10 / S9 / M2

**Result:** `SE_IOS_APP.xcodeproj` deleted — 7 files, 3,192 lines, all dead.
Nothing in the CMake tree referenced it; confirmed rather than assumed.

**Ruling, verbatim:** *"it's a very old project. TIDE should lean on a generic
AUv3 iOS backend for gmpi_ui as much as possible."* Read as two decisions, not
one: retire (not revive) the existing project, and shape whatever replaces it
around a backend `gmpi_ui`/`GMPI_Wrappers` owns generically, not a TIDE-specific
rebuild.

**Checked before writing the M2 rescoping, not assumed:** `GMPI_Wrappers/wrapper/`
holds `VST3`, `AU2`, `CLAP`, `Standalone` — no `AUv3` sibling exists yet. That
confirms the ruling's second half is a real, unstarted piece of work, not
already-done infrastructure this session simply didn't know about. M2's row
now says so, so whoever picks it up next isn't the one who has to discover it.

**What deliberately did *not* happen:** the four target source folders
(`SE_IOS_APP/`, `SE_IOS_AUDIOUNIT/`, `SeAppMacOS/`, `SeAudioUnitMacOS/`, ~330KB)
were left in place. S10's own row named the retire action precisely — *"delete
the .xcodeproj"* — and going further than that on a GATED, shared path is
exactly the kind of reach the STEP 5 rules warn against, even when the broader
deletion would probably also be fine. The ALLOWED `SE_IOS_APP/TIDE/` folder
(S6's, last touched 2026-08-13) was not touched at all.

**Next:** **A16** (the git-index race) needs the same kind of fix A14 got this
morning — likely a pre-commit `git show --stat HEAD` self-check comparing
against what was staged, since authorship alone doesn't catch a short commit.
On the carve-out, **C12c** and **C12f** remain the win-box items, both
currently unverifiable by build for anything touching `EditorScreenshot` until
the `ITextLayout` work lands — check `gmpi_ui`'s tip before assuming a build
failure is your own. On iOS, the newly-unblocked-in-shape **M2** is a real
authoring task now, not a repair job; nobody has started the generic AUv3
wrapper.

**Side effects on this box:** four scratch build trees, two throwaway A/B
worktrees, and one throwaway repro repo, all under the session scratchpad.
Both shared checkouts (`SE16`, `SynthEditLib`) remain parked on
`tide/win/C12e-dialogs-editor`, untouched by this work, as they have been all
session — that branch is not mine to move out from under a live session.

**Branch/PR:** [SynthEdit#21](https://github.com/JeffMcClintock/SynthEdit/pull/21)
+ [SynthEditLib#10](https://github.com/JeffMcClintock/SynthEditLib/pull/10)
(C11, must merge together) and [SynthEdit#22](https://github.com/JeffMcClintock/SynthEdit/pull/22)
(S10), plus this TideSynth PR carrying the rulings, journal and backlog.
