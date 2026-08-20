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
2. Stop when this file is **under 60 KB**, or when the floor is reached —
   whichever comes first. **The floor is the LATER of: the four most recent
   entries, or every entry carrying the most recent date.** The floor always
   wins; a busy day pushing this file over 60 KB is correct, not a rotation
   failure.
3. Never edit an entry while archiving it. The archive is the record.

**Why a date and not a duration (A24, 2026-08-20).** A24 asked for a time-based
floor — *"retain everything from the last 7 days"* — and measuring what that
costs is what killed it. Entries per day, counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it, so 7 days is **3.4× the 192 KB that
triggered A8 in the first place** — the remedy would have been twenty times more
expensive than the problem. Even two days is worse than the state A8 was created
to fix.

So the floor is **one date**, which bounds the cost at roughly a day's work while
guaranteeing a run can always see everything that happened most recently — the
failure A24 correctly identified, where a 4-entry floor at ten entries a day
bought under half a day. On a quiet week the four-entry floor still binds and
nothing changes.

**What this does NOT fix, filed as A30:** the durable lessons still age out.
Rotation moves an entry's *"Learned"* bullets into the archive with it, and no
run reads the archive. The cheap answer is a standing digest that never rotates;
the expensive one is reading 651 KB.

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

## 2026-08-20 — macos — A24: the journal floor is one DATE, because seven days measures 651 KB

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Eleventh item this session.** A22 and A23 both merged.

**Did:** changed the rotation floor from four entries to **the later of four
entries or every entry sharing the most recent date**, raised the trim target
30 KB → 60 KB, and changed the run prompt from *"read at least the last four
entries"* to **"read all of it"**.

### A24's premise is right and its remedy is wrong, by a factor of twenty

**Premise, re-measured and now sharper than when filed:** `JOURNAL.md` held
**five entries all carrying the same date**. A run obeying *"read the last four
entries"* saw under one day. A24 measured "about half a day" on 08-18; it has
got worse.

**Remedy refuted.** A24 asked to *"retain everything from the last 7 days"*.
Counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it. **Seven days is 3.4× the 192 KB
that triggered A8 in the first place** — the cure twenty times more expensive
than the disease. Even *two* days is worse than the state A8 was created to fix.

**So the floor is one date.** It bounds the cost at roughly a day's work while
guaranteeing a run sees everything that happened most recently, which is the
failure A24 correctly identified. On a quiet week the four-entry floor still
binds and nothing changes — the rule only bites on days like this one.

### What it does not fix — filed as A30

Rotation carries an entry's **Learned** bullets into the archive with it, and
**no run reads the archive**. So a lesson is load-bearing for about a day and
then silently stops being read. A24's one-date bound is the most that is
affordable, so the window cannot be widened to solve this: the lessons have to
leave the rotating file, into a standing digest.

This is not hypothetical. Twice today a run re-derived something an earlier entry
had already recorded, and this session is the shortest possible distance from
those entries.

**Learned:**

1. **Measure the remedy, not just the problem.** A24 was filed with careful
   numbers for the *premise* and an unmeasured guess for the *fix*. The guess was
   off by 20×, and nothing in the row's own reasoning would have revealed it —
   only counting the bytes did.
2. **A24 nearly cited a taken ID.** I wrote "filed as A25" and A25 has been in
   `BACKLOG-DONE.md` since 08-18. Caught by checking rather than by lint —
   `check-id-refs` validates that a *referenced* ID exists, and A25 does exist.
   The duplicate check shipped this morning would have caught the *row*, had I
   written one. Worth knowing that "the ID exists" and "the ID is free" are
   different questions and only one of them is linted.

**Next:**

1. **A30** — the lessons digest. Wants Jeff's eye on the prompt half.
2. **A12** is the only A-series row left, and it is `.github/workflows/**`.
3. **C7b / C7d** are the carve-out's critical path; **C14** is IN-REVIEW with all
   three PRs merged and should flip to DONE.

**Branch/PR:** `tide/mac/A24-journal-floor` — TideSynth only.

## 2026-08-20 — macos — A23: duplicate-ID detection, and the three false alarms that shaped the rule

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Tenth item this session.** A22 is IN-REVIEW on
[#183](https://github.com/JeffMcClintock/TideSynth/pull/183), green and waiting.

**Did:** `scripts/check-id-refs.py` now fails when one ID owns more than one row.
It already parsed every row ID; the change is that it records **locations**
rather than a set, because a duplicate is only actionable if the report names
*both* lines — renumbering means editing one of them.

### The naive rule was red on day one, and that is the finding

A first cut flagged **three** IDs in the live tree. **All three were
legitimate**, and each taught the rule something:

| flagged | what it actually is |
|---|---|
| `~~P8~~` (BACKLOG.md) | a **superseded row kept beside its replacement**. `RE_ID_CELL` tolerates the tildes *on purpose*, so references to it still resolve |
| `~~G3~~` (BACKLOG-DONE.md) | the same shape |
| `S1` ×2 (BACKLOG-DONE.md) | **a genuine, deliberate duplicate** — the linux and macOS boxes both took S1 on 2026-08-06, before the cron stagger took effect, and the second row says so in its own text |

So the rule is narrower than "one ID, one row":

- **Superseded rows are excluded from the duplicate test only**, not from the
  known-ID set. Both properties are needed and they are not the same property.
- **Archive-only duplicates are not flagged at all.** The archive is history,
  *"archiving never rewrites a row"*, and flagging S1 would demand an edit the
  rules forbid — on every run, forever.
- **What IS flagged is any duplicate touching `BACKLOG.md`:** two live rows (the
  A17 collision A23 was filed for), or one row in each file — an archive move
  that *copied* instead of moving, which makes the row's status ambiguous.

### Verification

Two positive controls rather than the one A23 asked for, because the second
shape only became visible while writing the rule:

```
duplicate row in BACKLOG.md   -> rc=1, names BACKLOG.md:76 and :77
copied into the archive       -> rc=1, names BACKLOG.md:76 and BACKLOG-DONE.md:20
the real tree                 -> rc=0
```

`--selftest` is **25 cases, 0 failed** (was 20), on real file bodies rather than
regex snippets because every subtlety here is about *which file* a row is in.
**The selftest is itself proven able to fail:** flipping one case's expectation
gives `FAIL duplicate/clean`, rc=1.

**Noted, not fixed:** `lint.yml` invokes this script **without** `--selftest`, so
those 25 cases run only by hand — exactly like `check-next-block.py`. Making CI
run them is a `.github/workflows/**` edit, so it needs Jeff either way.

**Learned:**

1. **Run a new lint against real history before believing it.** Three false
   alarms, zero of them predictable from the row's description — and A23's own
   text said the check was "a few lines on data it has already collected", which
   was true and still nearly shipped a rule that failed `main`.
2. **Two properties that look like one.** "Is this a known ID?" and "does this ID
   own a row?" differ precisely on struck-through entries, and the existing
   regex's tolerance of `~~` was deliberate for the first question. Reusing a
   collector for a second question is where that kind of assumption breaks.

**Next:**

1. **A24** — the last A-series row: the journal's rotation floor is counted in
   entries, so cross-run memory shrinks as cadence rises.
2. **S20** and the CI-on-push question are Jeff's; **U3's click path** is still
   unverified.

**Branch/PR:** [#184](https://github.com/JeffMcClintock/TideSynth/pull/184), branch
`tide/mac/A23-duplicate-ids` — TideSynth only.

## 2026-08-20 — macos — A22: the row names the branch, not the PR; and SynthEdit's CI never runs on push

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Ninth item this session.** S19 is fully closed —
[SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66),
[#67](https://github.com/JeffMcClintock/SynthEdit/pull/67) and
[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28) all
merged, [#178](https://github.com/JeffMcClintock/TideSynth/issues/178) closed.

**Did:** STEP 4 now says the backlog row must name your **branch**, with the PR
link a best-effort extra.

### Why (a)+(d) rather than either option the row offered

The old wording — *"mark the item IN-REVIEW with links to every PR you opened"* —
**cannot be satisfied in the commit that makes the mark**, because the PR does
not exist until after it. So it guaranteed a follow-up.

A22 offered (a) *name the branch* and (d) *accept the follow-up PR*. Taking (d)
alone would legalise the second PR rather than remove the need for it — and #120
showed the second PR is not the real cost. Its follow-up landed on a branch whose
PR had already auto-merged, which is **a pushed branch with no PR: the one end
state STEP 5 forbids.** So the new text adds a precondition A22 did not ask for:

> check the PR is still open before pushing the follow-up, and if it has already
> merged, **drop the commit** — `gh pr view <n> --json state --jq .state`

Pushing nothing is always safe. A commit whose only content is a link is never
worth a second PR, and the branch name in the row is what makes the follow-up
**optional rather than load-bearing**.

**The evidence is use, not argument:** this session ran the
branch-then-follow-up shape about eight times across three repos — A27, A28, A21,
S19, U3 — and never needed a second PR.

### Found while confirming S19: the CI chain never runs on push — filed as S20

`SE16 Kickstart Build` is **`on: workflow_dispatch:` and nothing else**.
`cmake_win` triggers off *it*; `cmake_mac` triggers off `cmake_win`. **So a push
to `master` runs no build and no tests.** The last dispatch was 2026-08-19T09:34
and `master` has moved eight times since with **zero** runs.

That is half the reason S19's five failures survived a week: `continue-on-error`
hid them, and the chain fired rarely enough that few people ever saw a log.

**The obvious fix is wrong, which is why it is a row and not a patch.** That
chain is a *release* pipeline — it signs, notarizes, staples and **FTP-uploads to
synthedit.com**. `on: push` would publish on every commit. What it wants is a
separate build-and-test-only workflow, and that is `.github/workflows/**`, so it
needs Jeff either way.

**I did not dispatch it.** Triggering that chain is a release, not a test run —
so today's mac fixes are verified locally (86/86) and remain unconfirmed in CI
until Jeff next kicks one off.

**Learned:**

1. **A rule that cannot be obeyed in one step will be obeyed in two, and the
   second step is where the damage is.** Nobody was going to skip linking the PR;
   they were going to link it badly. The fix was not to demand less but to move
   the required content to something knowable *before* the push.
2. **"CI is green" means nothing until you know what triggers CI.** I spent the
   session treating cmake_mac as the arbiter of mac health. It runs when a human
   asks it to, and it had not been asked since before any of today's work.

**Next:**

1. **A23** — duplicate-id detection in `check-id-refs.py`, the best-specced row
   left, with a positive control already written into it. **A24** after it.
2. **S20** and the `continue-on-error` follow-through both want Jeff.
3. **U3's click path** is still the one unverified thing from today.

**Branch/PR:** [#183](https://github.com/JeffMcClintock/TideSynth/pull/183), branch
`tide/mac/A22-pr-link` — TideSynth only, docs and backlog.


## 2026-08-20 — macos — the mac test drift is FMA contraction, and my own diagnosis was wrong first

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Seventh and eighth items this session**, on Jeff's instruction. Also closes
**U3** (shipped as [SynthEdit#65](https://github.com/JeffMcClintock/SynthEdit/pull/65)).

**Did:** diagnosed the four `TestVoiceAllocation` failures that S19 papered over
this morning with raised gates, and reverted those gates because they turned out
to be unnecessary. [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66).

### The hypothesis I wrote into three documents was wrong

This morning's row, issue and PR all said the error shape — max −68 dB against an
average −150 dB — looked like **a one-sample timing difference at voice
transitions**. Measured against the reference `.wav`, every part of that is false:

| claim | measurement |
|---|---|
| a handful of samples | **81.9% of all samples**, continuous from 0.127 s |
| a one-sample shift | shift 0 = −68.73 dB, shift ±1 = **−22.69 dB** — zero wins by 46 dB |
| (unstated) a gain error | best scalar fit **0.999999192**, residual unchanged |

It was a plausible story fitted to one summary statistic, and it survived into
three places because nobody had opened the file. **The average/max ratio I
reasoned from was the cancellation utility's own metric, not something I had
computed.**

### The actual cause

**FMA contraction.** clang defaults `-ffp-contract` to *on*, fusing `a*b+c` into
one `fma`. arm64 always has FMA; x86-64 under MSVC or GCC does not emit it by
default — **which is exactly why Windows and Linux reproduce the references and
macOS does not.**

It is *not* the Apple fast-math subset that was already in `CMakeLists.txt`. That
was the obvious suspect and it was eliminated this morning: with
`-fassociative-math` and `-freciprocal-math` removed, the four residuals were
**bit-identical**. Only contraction moved them.

```
test                        contract=on   contract=off
Unterminated_Poly_Modules   -80.77 dB     -90.31 dB
Voice_Allocation_Mono_High  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Last  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Off   -73.41 dB     -90.31 dB
```

**−90.31 dB is exactly 1 LSB at 16 bits** (`20·log10(1/32768)`), i.e. bit-identical
within the file format. Full suite with the strict gates restored: **3 failures
with contraction on, 86/86 with it off.**

So there was never a voice-allocation defect, and **the four gates raised this
morning are reverted to 85/75/75/75.**

### Checked rather than assumed

[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28)'s
soundfont scoping is **still load-bearing**: rebuilt with it reverted *and*
contraction off, `SoundfontOsc` still fails. Reassociation and contraction are
different mechanisms and neither fix makes the other redundant.

**Learned:**

1. **A hypothesis that explains the summary statistic is not a diagnosis.** Max
   ≫ average genuinely does suggest sparse differences — and the differences were
   dense. Ten minutes of `numpy` against the two files would have prevented three
   documents asserting it. **Open the artifact.**
2. **Eliminating the obvious suspect is worth more than confirming it.** This
   morning's A/B on `-fassociative-math` looked like a dead end — the figures were
   bit-identical, so the flags "weren't it". That negative result is what pointed
   at a *different* FP mechanism rather than a DSP bug, and it is why the second
   experiment was aimed correctly.
3. **`-ffp-contract` is invisible in a fast-math discussion.** It is not part of
   `-ffast-math`, is not mentioned by the flags this project already reasons
   about, and is on by default. On any arm64 target it is the first thing to check
   when a render differs from an x86-baked reference.

**Next:**

1. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178) can close once [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) merges** — except for the
   `continue-on-error` removal, which stays Jeff's.
2. **U3's click path is unverified** — one right-click on the rack background.
3. **A22, A23, A24** are the remaining A-series rows; **C7e** is unblocked from
   the `EditorScreenshot` direction now C7c is closed.

**Branch/PR:** `tide/mac/s19-fma-record` (this) + [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) (the code).

## 2026-08-20 — macos — C7c answered by removal, and the two questions that answer creates

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Sixth item this session**, on Jeff's instruction. Also closes **S19** — both its
PRs merged and macOS `ctest` is **100% of 86**; the `continue-on-error` removal
is still Jeff's and stays on [#178](https://github.com/JeffMcClintock/TideSynth/issues/178).

**Did:** recorded Jeff's C7c ruling and filed the work it implies as **U3**.

C7c asked whether `EditorScreenshot` should become public so a stranger's clone
can link it. **Jeff answered by deleting the need:** *"let's remove the
breadcrumb bar from TIDE, it's a bit redundant in a product where you seldom dig
deeper than 1 level in."*

That is the better answer than either option the row offered. TIDE's only uses of
`EditorScreenshot` are `SynthEditGui.cpp`'s `ContainerThumbnail.h` include and
the link line in `SynthEditSem/CMakeLists.txt`, and **both exist solely to draw
crumb thumbnails** — so the dependency leaves with the feature, nothing has to
come out of the commercial repo, and **C7e loses its last non-`SynthEditLib`
blocker.**

### Why the removal is U3 and NEEDS-JEFF rather than something I did

Reading the code before cutting found two things the ruling does not settle, and
either one guessed wrong ships a worse product than the bar:

1. **The crumbs are the only way back UP a level.**
   `breadcrumbBar->onNavigate` (`SynthEditGui.cpp:699`) is one of exactly **two**
   navigation entry points. The other, `seApp->onOpenContainerView` (`:703`),
   goes only *in*, or to the master via "Goto Structure…". I grepped
   `SynthEditLib`, `EditorLib` and `SynthEdit2`: **there is no existing
   go-to-parent affordance.** Remove the crumbs with no replacement and a user
   who opens a module is stranded in it.

2. **The About pane's only entry point is anchored to the crumb strip.** D6's own
   comment calls it *"the about pane and the only way in"*
   (`SynthEditGui.cpp:292-299`), a plain text affordance at the strip's right end.

U3 carries three options and recommends **(a) keep a thin strip with just
"◀ Back" and "About"** — the only one that changes no interaction the user
already has, while dropping exactly the part Jeff called redundant: the thumbnail
trail.

**`SE2::BreadcrumbBar` itself is not being deleted.** It is shared with the
WinUI3, Wayland, JUCE and Mac frontends; only TIDE stops using it.

**Learned:**

1. **"Remove the feature" can be the right answer to a licensing-boundary
   question, and it is not one an agent would have proposed.** C7c framed the
   choice as *which files move*; the cheapest answer was that none do. Worth
   remembering the next time a row's options list looks exhaustive.
2. **A one-line product decision can have load-bearing code underneath it.** The
   bar looked like a widget and is also the navigation model and the About
   pane's front door. Reading before cutting cost ten minutes.

**Next:**

1. **U3 wants Jeff's answer on Back and About**, then it is one session.
2. **C7e** is now unblocked from the `EditorScreenshot` direction; **C7b** and
   **C7d** are unchanged and still the linux box's.
3. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — the workflow edit, and the unexplained `TestVoiceAllocation` residual.

**Branch/PR:** `tide/mac/C7c-drop-breadcrumb` — TideSynth only, no code change.

## 2026-08-20 — macos — a mac build break from the carve-out, and five test failures CI has been hiding for a week

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (Claude Code CLI version not resolvable on this box) · as **tide-rack-bot** (both paths)

**Fourth and fifth items this session**, on Jeff's instruction — he pointed at a
failing Actions run rather than the backlog. Repos synced first.

## 1. The build break: [SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63)

`cmake_mac` had been red on `master` since `6c7e90053` while `cmake_win` and
`cmake_linux` were green at the same sha.

**The first thing to get right was WHICH step failed.** The 5 test failures in
that log are a decoy: `ctest` is `continue-on-error: true`, so **step 6 is marked
success**. The run failed at **step 20, the Xcode build**:

```
SynthEditMac/SynthEditMac/MidiAutomationWindowController.mm:3:10:
  fatal error: '../../SynthEdit2/PatchParameter.h' file not found
```

Two headers left `SynthEdit2/` during the carve-out and the Xcode consumer was
never re-pointed:

| header | moved by | now at |
|---|---|---|
| `PatchParameter.h` | `9a53a4882` — C12f, the patch cluster | `SynthEditLib/` |
| `IMidiDriver.h` | `4f6f5b1ca` — C13, the three orphan headers | `SynthEditLib/` |

CMake follows them because SE16's `CMakeLists` hands EditorLib the include
directories; the Xcode project quotes the paths literally. **This is exactly the
hazard C10's row already names** — *"Non-CMake consumers still need checking by
hand: `SynthEdit2.vcxproj` and the SynthEditMac Xcode project"* — so the carve-out
stages should treat that line as a checklist item, not a footnote.

**I fixed both files although CI named only one.** Xcode stops at the first fatal
error. Reverting both edits and rebuilding — with the `.mm` already compiled —
gives `CoreMidiDriver.h:6:10: fatal error: '../../SynthEdit2/IMidiDriver.h' file
not found`. Fixing only what CI printed would have turned one red run into two.

**Verified by building, twice**: at the original tip, and again after merging the
current `master` (`61eaf744b`, C14 landed, which changes EditorLib's include
directories so it was worth re-checking rather than assuming). Both times:
`cmake --build` **1064/1064 rc=0**, then `xcodebuild -scheme SynthEdit -arch
arm64 -configuration Release` → ***\*\* BUILD SUCCEEDED\****.

### A dev-box trap found on the way to that link

Reaching the link first produced
`Undefined symbols: ApplyConfigPreInit(SynthEditApp&, ...)`. Cause:
`libEditorLib.a` is built from `build/_deps/syntheditlib-src` (FetchContent,
**`GIT_TAG origin/main`** — a live ref), while the Xcode project resolves
`ApplySynthEditConfig.h` from the **sibling** `../SynthEditLib` clone. The clone
was behind C14, so the library exported the new `CSynthEditAppBase&` signature
and the header still declared `SynthEditApp&`.

**CI cannot hit this**: its *"Symlink CMake-fetched deps for Xcode"* step points
`../SynthEditLib` at `build/_deps/syntheditlib-src`, so header and library are one
tree by construction. On a developer box they are two trees tracking a moving
ref. Fixed by fast-forwarding the clone to `86ab11c`. **This is S17's shape one
level up** and worth knowing before it costs someone an afternoon.

## 2. The five test failures — filed as S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)

**They are not a regression, and they are not one bug.**

`94% tests passed, 5 tests failed out of 86` appears in *every* mac run I
checked, including 2026-08-13, 08-14 and 08-18 — **all of which are marked
success**. `continue-on-error: true` is why nobody knew.

| platform | at `6c7e90053` |
|---|---|
| Windows | 92/92 |
| Linux | 86/86 |
| **macOS** | **5 of 86 fail** |

Reproduced locally with figures **identical to CI's to four decimal places**, so
deterministic rather than flaky.

**The A/B that splits them.** `SynthEdit/CMakeLists.txt:281` adds, on Apple only,
`-fno-math-errno -fno-trapping-math -fno-signed-zeros -fassociative-math
-freciprocal-math`. Windows gets `/fp:fast`; **Linux gets no fast-math at all**,
which is consistent with Linux being the platform that passes. Rebuilding the
whole tree with the two reassociating flags removed and nothing else changed:

| test | with | without |
|---|---|---|
| `TestSoundfont.SoundfontOsc` | FAIL | **PASS** |
| `Unterminated_Poly_Modules` | −80.7666 dB | **−80.7666 dB** |
| `Voice_Allocation_Mono_High` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Last` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Off` | −73.407 dB | **−73.407 dB** |

So `SoundfontOsc` is FP reassociation, and **the four `TestVoiceAllocation` cases
are something else entirely** — bit-identical output either way, i.e. a real
deterministic mac-vs-reference difference the flags do not touch.

**Hypothesis, and labelled as one:** max −68 dB against an average of −150 dB
means a handful of samples differ, not a level or timbre — a constant offset
would put the average near the max. That is the shape of a one-sample timing
difference at voice transitions, which fits tests whose whole subject is voice
allocation. Unconfirmed.

**Not fixed on purpose.** Bumping four tolerances would turn the suite green and
throw the finding away, and whether −68 dB is acceptable in this product is not
an agent's call. The reporting half — removing `continue-on-error` — is a
`.github/workflows/**` edit the token structurally cannot push.

**Learned:**

1. **A `continue-on-error` step turns a failing suite into a decoy twice over.**
   It hid five real failures for a week, *and* it put a wall of red test output
   at the top of a log whose actual failure was fourteen steps later. Read the
   per-step conclusions before reading the log.
2. **"Last green run" is not a baseline when a step can fail without failing the
   run.** My first instinct was to bisect `9674bbfc7..6c7e90053`; the tests were
   already failing at `9674bbfc7` and in every run before it. Checking the older
   *green* runs' logs cost one command and saved a bisect that would have found
   nothing.
3. **An A/B that changes one flag is worth more than a plausible story.** The
   mac-only fast-math subset explained all five failures beautifully. It explains
   exactly one.

**Next:**

1. **S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — diagnose the four, rule on SoundfontOsc's tolerance, and
   Jeff removes `continue-on-error`.
2. **[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) is open and green** — mac `master` stays broken until it merges.
3. **A22, A23, A24** are the remaining A-series rows; A23 is the best-specced.

**Branch/PR:** `tide/mac/mac-ci-findings` (this bookkeeping) plus
[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) (the code).
Two repos; the SynthEdit half is the whole fix and this half is the record, so
neither blocks the other. Throwaway worktrees; every checkout left on its default
branch and clean.
