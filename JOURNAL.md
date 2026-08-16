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

---

## 2026-08-15 — windows — C12e (interactive session, Jeff ruling)

**Did:** **C12e**, ruled in session ("go with your recommendation") — option
(b). `Dialogs_editor.h` moved to `SynthEditLib`; `Dialogs_editor2.cpp` came off
EditorLib's source list and is now compiled by each app that needs it.
**27 → 25 `${EDITOR_DIR}` entries.** Also struck a stale NEEDS-JEFF from **A9**
(below), and recorded the ruling in
[docs/decisions.md](docs/decisions.md), which now has **no open PROPOSED
entries**.

**The headline is not the move. It is that the recommendation's stated
reasoning was wrong on the one point that decided the work, and measuring
before implementing is what caught it.**

Both this row and the PROPOSED entry said the other consumers *"each supply
their own definitions"*, which made (b) a one-`vcxproj`-entry change.
**`SynthEditCL` does not supply its own.** Its CMake target compiles
`main.cpp`, **not** `CLApp.cpp` — `CLApp.cpp` is in no build file at all — and
`main.cpp` carries a comment saying in as many words that it relies on
EditorLib for these stubs, as does `EditorScreenshot/EditorCommandDispatcher.cpp`.
So (b) as literally written would have removed the only definition SynthEditCL
had and broken its link.

Implemented as (b) **properly**, which is what option (b)'s own text points at:
the `SynthEditApp.cpp` / `ExportAsPlugin.cpp` pattern, where **every** app that
needs the symbols compiles the file itself. `SynthEditCL/CMakeLists.txt` and
`SynthEdit2.vcxproj` each gained an entry; `TideApp.cpp` and `layouttests.cpp`
already define their own.

**Two more of the row's facts were wrong**, both harmless but worth correcting
because they were repeated in three places:

- The file defines **two** functions, not three. `doDialogBuildCodeSkeleton`
  appears in neither `Dialogs_editor2.cpp` nor `Dialogs_editor.h` — it is
  declared and defined only in `CLApp.cpp` and `TideApp.cpp`, and belongs to
  **S3**, not here.
- There are **four** definitions in the tree, not five:
  `Dialogs_editor2.cpp`, `CLApp.cpp` (unbuilt), `TideApp.cpp`,
  `layouttests.cpp`. `EditorScreenshot` and `SynthEditCL/main.cpp` carry only
  comments pointing at EditorLib's copy.

**Result.**

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **27 → 25** |
| fresh Ninja tree, Release | **904/904 RC=0** — net zero, one TU left EditorLib and one joined SynthEditCL |
| `Dialogs_editor2.cpp.obj` | now built **only** by `SynthEditCL.dir`; `EditorLib.dir` has zero |
| **TIDE still links** — the specific thing the row demanded | **`TIDE.gmpi` and `TIDE_VST3.vst3` both produced** |
| `SynthEditCL.exe` | links |
| tests | **91/92** — see below |

**The one test failure is not this change, and that is proven rather than
argued.** `Layout.ModuleSizeDoesNotGrowOnReopens` fails with a `bad_alloc`,
reproducibly. **A/B: `SE16` at `origin/master`, with none of C12e, in a
detached worktree against the same libraries — fails identically.** So it is
pre-existing on master.

**What it actually is, since the next run will hit it too:** in-flight
`ITextLayout` work spanning two repos. `gmpi_ui` committed
**`d3bacf3` "feat: ITextLayout, a retained immutable styled text layout
(Direct2D)"** partway through this session, and
`SynthEditLib/modules/se_sdk3_hosting/GmpiCpuUniversalContext.h` — **still
uncommitted** — already calls `gmpi::drawing::api::ITextLayout`. Pinning
`gmpi_ui` back to `3ab5524` does not restore green either; it fails to
*compile*, because that uncommitted header needs the new API. The two are
mid-flight together and neither half stands alone right now. **Do not "fix"
this**; it is Jeff's live work in another session. Left untouched, as the
STEP 5 dirt rule requires.

**Learned — a `*_FOLDER_OVERRIDE` build reads a live working tree, so another
session's uncommitted work lands in your test results.** This is the first time
that has actually bitten. It is not a reason to stop using the overrides
(**X4** settled that), but it is a reason to A/B against the default branch
**before** blaming your own change — and to do it in a `git worktree`, which
leaves the developer's tree untouched. The whole diagnosis cost one worktree
and two targeted builds.

**Learned — A9 has been listing a NEEDS-JEFF that PLAN.md already answered.**
The row asks for "TIDE's product philosophy in 2–3 sentences as the auto-reject
filter, Cardinal-style". [PLAN.md](PLAN.md) has carried it since before the row
was written: **"What TIDE Rack is"** is the one-sentence identity, and the
**eight design constraints** are the reject filter in more detail than three
sentences would be. Written 2026-08-09 from the process review and never
re-pointed. Struck, with the reasoning in the row. **A9 needs nothing from Jeff
to start.**

**Next:** **C12c** and **C12f** are the remaining sub-stages a Windows box can
take (C12d is `linux`). C12 now stands at 25 of its original 41 entries, and
**C12f is what takes it to zero and unblocks C6**. Whoever takes either should
expect `Layout.ModuleSizeDoesNotGrowOnReopens` to be red until the ITextLayout
work lands, and should **not** treat it as their own regression — A/B first.

**Side effects on this box:** a scratch Ninja tree, a pristine `gmpi_ui` clone
and a detached `SE16` worktree, all under the session scratchpad; the worktree
was removed and `git worktree prune` run, leaving `git worktree list` with only
Jeff's own entries. `SynthEditLib` was committed in **while dirty with Jeff's
uncommitted `GmpiCpuUniversalContext.h`** — staged by explicit path, never
`git add -A`, and that file is untouched.


**Postscript — the A14 guard fired on its first real outing, and it mattered.**
The two code branches were created in the *shared* working copies, and the
other session commits into whatever branch is checked out there. So its
in-flight `ITextLayout` commits landed on my branches, interleaved by seconds
(`SynthEditLib` `eae673b` 13:15:04, mine `93f5ea9` 13:19:27, its `a7eb0bf`
13:19:50; `SE16` mine `7563bd151` 13:20:02, its `eb66d2ae9` 13:20:05) — and I
pushed them before noticing. **`scripts/check-commit-authorship.py`, written
this morning for exactly this, is what caught it**, in the STEP 4 pre-push
position its own docstring argues for.

Resolved without rewriting or deleting anything: my single commit was
cherry-picked onto fresh `tide/win/C12e-clean` branches off the default
branches, **in temporary worktrees so neither shared checkout was touched**,
and the PRs raised from those. Each clean branch contains exactly one commit,
verified. The mixed `tide/win/C12e-dialogs-editor` branches are left exactly as
they are — they hold Jeff's work, and they are not mine to rewrite.

**Two things for the next run.** First: **both shared checkouts are still parked
on `tide/win/C12e-dialogs-editor`**, so further commits there keep landing on
it; I deliberately did not switch them, because doing that under a live session
risks its working tree. Second, and more general: **creating a branch in a
shared working copy is itself the hazard.** A14's assertion catches the result;
it does not prevent it. The durable fix is to do code work in a `git worktree`
rather than by switching the developer's checkout — which is what the cleanup
had to do anyway.

**Branch/PR:** [SynthEdit#20](https://github.com/JeffMcClintock/SynthEdit/pull/20)
+ [SynthEditLib#9](https://github.com/JeffMcClintock/SynthEditLib/pull/9) —
**these two must merge together** — and the TideSynth PR carrying this entry,
the backlog and the ruling.
