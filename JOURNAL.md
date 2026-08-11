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

## 2026-08-12 — windows — A8 (C3 not taken — see below)

**Did:** Rotated the journal, archived the landed backlog rows, distilled the
five largest live rows into `docs/`, and wrote a link checker to prove none of it
stranded a reference. **I did not take C3, which the NEXT block points win at** —
that first, since it is what affects the next run.

**Why C3 was not taken.** SE16 was dirty with **Jeff's live work** — an
experimental Eurorack `rackMode` across `SynthEdit2/MfcDocPresenter.{cpp,h}`,
`SynthEdit2/SynthEditDocBase.h` and three files in
`SynthEditLib/modules/se_sdk3_hosting/`; real content, not CRLF churn.
**`SynthEditDocBase.h` is one of the six files C3 must `git mv`**, and a move
either commits his uncommitted line or destroys it — STEP 5's third dirt
category forbids both. Full detail is now a **precondition on the C3 row**, and
on C4 by implication, since the `MfcDocPresenter` pair is C4's scope. Transient:
re-check, do not assume.

**A3, the row above A8, was equally unavailable** — as are A5 and A6. All three
edit `.github/workflows/` and the bot's token deliberately lacks `workflow`
scope. The credential is enforcing the rule as designed, but it means **the
`any` NEXT pointer (A3) can never be satisfied by a scheduled run.**

**Result** — A8 asked for "under 30 KB after rotation, no broken links":

| | before | after |
|---|---|---|
| `JOURNAL.md` | 192,010 B / 37 entries | **25,209 B / 3 entries** (+ this one) |
| `BACKLOG.md` | 76,304 B / 76 rows | **45,003 B / 52 rows** |
| broken links | 4 | **0** of 164 relative links |

New: `JOURNAL-2026-08.md` (34 entries), `BACKLOG-DONE.md` (24 rows),
`scripts/check-links.py`, and five `docs/<id>.md` files.

**Verification artifacts — all run this session:**

- **Rotation is lossless.** Re-parsed both journal files, concatenated live +
  archive, compared to the pre-rotation file: **37 entries before, 37 after,
  headings and order identical, every body byte-identical** by SHA-256.
- **The five lifted rows are verbatim** — whitespace-normalised comparison of
  each row against its new `docs/` file. Only line breaks are new.
- **`scripts/check-links.py`, with a positive control.** Clean: 164 links, 0
  broken, exit 0. One bad link appended: exit 1, naming `BACKLOG.md:174`.
- **C8 flipped to DONE on evidence:** `SynthEditLib#4` merged
  2026-08-11T22:23:32Z, `SynthEdit#10` 22:22:56Z, both confirmed via the API.
  **P7 stays IN-REVIEW** — `GMPI_Wrappers#1` is still open.

**Learned:**

- **Lifting a row from a root-level file into `docs/` silently breaks its
  relative links.** Three of the five (`A2`, `N1`, `S1b`) carried `](docs/…)`
  links, correct in `BACKLOG.md` and wrong one directory down. The text is
  verbatim and still broken — **verbatim is faithful, not safe.** That is the
  whole argument for the checker; it caught all three, plus a pre-existing break
  in `docs/distribution.md:6` (`PLAN.md` → `../PLAN.md`).
- **The rotation rule needs a floor, and the floor has to win.** "Under 30 KB"
  and "the last four entries must be readable" genuinely conflict here: three of
  the four retained entries are 3.7–10.3 KB. **This file lands at 30,715 bytes —
  under 30 KiB, but above a decimal 30,000.** I kept the floor rather than
  archive a fourth entry: a size rule that starves the handoff is worse than a
  marginally large file. That precedence is now written into the rule above.
- **A grooming item conflicts with every open PR by construction**, so the only
  mitigation is choosing what *not* to touch. #34 edits `E1` and #35 edits
  `P7a`/`P6`, so I distilled neither — I had lifted `E1` and reverted it on
  finding #34 also adds a near-identically-named `docs/e1-verification-harness.md`.
  **Merge #34 and #35 first**; this PR then rebases, and the journal hunk
  resolves as: my header, their entries, my kept entries.
- **The `## Blocked on Jeff` section held nothing blocked on Jeff** — all six
  rows were `RESOLVED`. A section titled "agents must not start these" holding
  only settled history is a small trap; archived.
- **STEP 0.5 requires an app version this box does not expose.** No
  `AppData\Local\AnthropicClaude`; the only version string under
  `AppData\Local\Claude\Logs` is the Chrome native host's. The provenance line
  says "undetermined" rather than copying the last entry's number — but someone
  should settle where a run is meant to read it.

**Build health:** nothing built, no code changed — this run touched **TideSynth
only** (docs, backlog, journal, one script). `gmpi_ui` and `GMPI_Wrappers` were
clean and untouched; **SE16 and SynthEditLib were left exactly as found, dirty
with Jeff's `rackMode` work in six files** — not committed, reverted or stashed.
All five checkouts are on their default branches. TideSynth's `README.md` showed
a real 17-line diff at the start of this run and was clean minutes later,
consistent with Jeff editing live.

**STEP 1 / 1.5:** no open issues in TideSynth at all, so no `platform:win` issue,
and no open `tide/win/**` PR. #34 (linux/E1) and #35 (mac/P7a) are not mine. Per
the C8 entry, CI red is uninformative here until C7.

**Next:** merge **#34** and **#35**, then this PR after a rebase. Then win is on
**C3** — *check SE16 is clean first*. If `rackMode` is still in flight, C3 and C4
are both unavailable; the next win-eligible items are **S1b** (which recommends
riding along with C4 anyway) and **P3** — but P3's scope includes
`MfcDocPresenter.cpp`, so it carries the same precondition. **A3/A5/A6 need a
`workflow`-scoped credential or Jeff.**

**Prompt:** `e09e766` · claude-opus-5[1m] · app version undetermined on this box
· as `tide-rack-bot`

**Branch/PR:** `tide/win/A8-journal-rotation`, TideSynth only — no other repo was
committed in.

---

## 2026-08-11 — macos — C8 executed (interactive session, Jeff directing)

**Did:** Jeff merged [#31](https://github.com/JeffMcClintock/TideSynth/pull/31)
and [#32](https://github.com/JeffMcClintock/TideSynth/pull/32) and asked whether
that counted as a go-ahead. It counts for the *ruling*; the entry as written then
needed one more word, and he gave it. Deleted the header and recorded the
decision.

**Result:** the `PROPOSED:` entry merged **unedited**, which under the Proposed
mechanism selects the recommended default — **option (b), C8 is Jeff's call**.
Worth being precise about, because (b) is the one option that does not
self-execute: it reads *"Jeff deletes it, or says 'go' on this PR and a later run
does"*. So the merge settled **what** was decided but not **who acts**, and
answering "yes, merging is enough" without that distinction would have been wrong
in a way that only shows up the next time an agent reads this file for precedent.

Three PRs, all open, none merged by me:

| Repo | PR | Change |
|---|---|---|
| `SynthEditLib` | [#4](https://github.com/JeffMcClintock/SynthEditLib/pull/4) | delete `it_empty.h` |
| `SynthEdit` (SE16) | [#10](https://github.com/JeffMcClintock/SynthEdit/pull/10) | drop `EditorLib/CMakeLists.txt:74` |
| `TideSynth` | this branch | ruling into `decisions.md`, C8 → IN-REVIEW |

**They must merge together.** Either alone leaves a source list naming a file
that does not exist — and because CMake tolerates that for *headers*, it fails
silently rather than loudly. That is the same mechanism that let this orphan
survive three dead-code passes, so landing half of C8 would recreate C8.

**Learned:**

- **The gate was deliberately not widened.** Option (c) — relax STEP 5's
  exception from `C1-C7` to any C-series item — was the tidiest fix and was
  rejected on the merits, not forgotten. It is recorded as rejected in
  `decisions.md` so a later run does not re-derive it and read C8 as licence to
  reach. **C8 is a precedent for escalating, not for reaching.**
- **`gh pr edit` does not work as the bot.** It goes through GraphQL, which wants
  `read:org`; the classic token has `repo` only. Same for `gh pr view --comments`.
  **Use the REST API** — `gh api -X PATCH repos/<o>/<r>/pulls/<n> --input <json>`
  and `gh api …/pulls/<n>/reviews|comments` — which the `repo` scope does cover.
  This will bite every run that tries to amend its own PR body.
- **The #31/#32 `JOURNAL.md` conflict predicted in #32 was real and Jeff resolved
  it cleanly** — both entries are present and in date order on `main`. The
  mitigation that worked was keeping the *BACKLOG* edits disjoint (#31 took the
  NEXT block and the P7 rows, #32 took only the C8 row), so only the journal
  needed hand-merging. Worth repeating whenever two PRs are open at once; **A4**
  is the row that would automate it away.

**Still open, and not mine to close:**
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) — P7's
regression test. TideSynth #31 merged but this did not, so **P7 correctly stays
IN-REVIEW**; do not flip it to DONE until that one lands. It is additive and
changes no shipped code.

**Build health:** nothing built; the only code change is a one-line CMake source-
list deletion and the removal of a header with zero includers. No claim beyond
that.

**Next:** merge #4 and #10 **together**, then flip C8 to DONE and move the row to
the Done section. Then `mac` is on **P7a**, per the NEXT block #31 set.

**Prompt:** n/a — interactive session, not a scheduled run. Acted as
`tide-rack-bot`, so every change went via a PR rather than to a default branch.

**Branch/PR:** `tide/mac/C8-ruling-recorded` (TideSynth), plus the two above.

---

## 2026-08-11 — macos — C8

**Did:** Took **C8** (`SynthEditLib/it_empty.h` — delete it, or find out why it
exists) and did the finding-out half in full. Audit in
[docs/c8-it-empty-header.md](docs/c8-it-empty-header.md). **I did not delete the
file**, and that is the judgement call of this run — reasons under *Learned*.
Also filed the blocking question as a `PROPOSED:` entry in
[docs/decisions.md](docs/decisions.md) and rewrote the C8 row to NEEDS-JEFF with
`Default in effect` / `Decide-by`.

**Result — the file is dead by every measure, and I can date its death.**
Recommendation is **delete**. Reconstructed with `git log -S` over `SE16`:

| When | Commit | What went |
|---|---|---|
| 2022-03-03 | `27f28b54e` | the last live instantiation, `it_visual_ob_list_empty : EmptyIterator<CVisualOb, it_visual_ob>` |
| 2025-01-24 | `176c6c26f` | the archived V1 copies under `OtherProjects/SynthEdit_1.0/` |
| 2026-04-13 | `671457fc5` | `SynthEdit2/it_empty.cpp` — **the header's last includer anywhere** |
| 2026-08-08 | C2 | moved the orphan into the public repo |

The 2026-04-13 step is the punchline: that `.cpp` had its entire body commented
out, so its only live line was `#include "it_empty.h"`. For its last four months
the header's sole reason to exist was a file that existed only to include it.

**Verification artifact — six checks, all run this session, none carried over
from C2's claim:**

| Check | Result |
|---|---|
| `git grep -nIE 'it_empty\|EmptyIterator'` over 8 repos (`SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`, `GMPI-plugins`, `GMPI_Adaptors`, `TideSynth`) | **zero `#include`**; only the file itself, `EditorLib/CMakeLists.txt:74`, and TideSynth prose |
| same as a filesystem grep incl. build dirs | same, plus one pre-C2 copy in an abandoned `.claude/worktrees/` scratch tree — not a reference |
| `file(GLOB)` / `GLOB_RECURSE` anywhere in `SynthEditLib` | **none** — nothing can sweep it in |
| `install()` / `export()` / `PUBLIC_HEADER` / `FILE_SET` in `SynthEditLib` | **none at all** |
| `it_empty` in any `.vcxproj` / `.filters` / `.pbxproj` / `.xcconfig` / `.yml` | **none** (unlike `FuzzyMatch.h`, which C2 had to repoint in three) |
| creation date | **2002-01-10 18:50:19 UTC**, decoded from the v1 UUID in its own ClassWizard guard `AFX_IT_EMPTY_H__E50CDB53_05FA_11D6_…` |

**No build was run, deliberately.** A header that appears in zero `#include`
directives cannot affect any translation unit — that is a proof, and a build
could only fail to contradict it. Building `SE16` here also has known live traps
(P6; and the half-overridden `GMPI_WRAPPER_FOLDER_OVERRIDE` the P7 entry found in
Jeff's tree). Saying "verified by build" would have been weaker evidence dressed
as stronger.

**Learned:**

- **`SynthEditLib` does not build any of C2's 16 files.** Its own
  `CMakeLists.txt` lists none of `it_doc_ob.cpp`, `imbedded_file.cpp`,
  `checkpoint.cpp`, `it_plug_destinations.cpp` — they sit in the public repo but
  are still compiled only by `EditorLib`, reaching across via
  `${SYNTHEDITLIB_DIR}`. Correct and expected until **C6** moves the list, but it
  means **"it's in SynthEditLib" does not yet imply "SynthEditLib builds it"**, and
  a run reasoning about the public repo's surface should not assume otherwise.
- **So the C8 row's "public API surface" overstates the case.** Nothing in
  `SynthEditLib` is exported or installed — there are no such rules in the repo.
  The file is *visible*, which is a real cost for a repo whose point is to be
  read, but no consumer can depend on it. That distinction is what makes the
  deletion risk-free rather than merely low-risk.
- **A CMake source list is an inventory, not a dependency graph.** This is how
  the file survived: it was on `EditorLib/CMakeLists.txt:74`, C2 moved everything
  on that list, and listing a header contributes nothing to compilation so nothing
  ever complains. Any future carve-out stage should expect the same — **C3 moves
  ~120 files off that list and the list is not evidence any of them are live.**
- **Why I stopped short of deleting, since it is the arguable part.** Both files
  the change needs — `SynthEditLib/it_empty.h` and `SE16/EditorLib/CMakeLists.txt`
  — are on STEP 5's **GATED** list, whose single exception is "an approved
  carve-out stage (C1-C7)". C8 is numbered outside that range and is a cleanup,
  not a stage. The prompt's remedy for a GATED fix is "do the TIDE-side part, then
  file the gated part as its own BACKLOG item naming the exact file" — but that is
  already spent: **C8 *is* that item**, filed by C2, naming the exact file, and
  there is no TIDE-side part. So the remedy terminates in a question, not an
  action. Widening the exception myself would be a run rewriting the rule that
  protects the commercial repo because the rule inconvenienced it, which is the
  shape of the mistake the gate exists to prevent; **G3 is the precedent for
  asking, and Jeff answered that one in a day.** The C8 row also asks for "a
  deliberate keep or a deliberate delete" — C2 span it off precisely so the call
  would not be a side effect of a file move, and deciding it unilaterally makes it
  a side effect again, one layer up.
- **Eligibility and authority are different questions, and the prompt is
  consistent about it.** STEP 2 says eligibility lives in the Status column
  alone — C8 is `TODO`/`any`, so taking it was correct. STEP 5 then constrains
  *how* it may be executed. A run that conflates the two would either refuse an
  eligible item or reach across a gate; the right answer is take it, do
  everything in bounds, escalate the one act that is not.

**Build health:** nothing was built and no code changed, in any repo. This run
touched only TideSynth (docs + backlog + journal), so nothing that consumes
`gmpi_ui`, `GMPI_Wrappers`, `SynthEditLib` or `SE16` is affected. All five
working copies were clean before this run and were left on their default
branches.

**STEP 1 / 1.5 — what I found before picking an item, since it changes what the
next run should expect:**

- **No `platform:mac` issues** — none open in TideSynth at all. `gmpi_ui#1`
  ("Linux support?", 2024) is unlabelled and not from the CI bot; noted, not acted
  on.
- **P7's PRs are open and their checks are red, and it is not P7's fault.**
  TideSynth [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) shows
  `windows`, `macos` and `linux` all FAILURE — but **`main`'s own latest run
  (`31352435423`) fails identically on all three**, with
  `CMake Error: The source directory … does not appear to contain CMakeLists.txt`.
  That is the documented pre-C7 failure the workflow header calls "the point"
  (`build.yml`: *EXPECTED TO FAIL until BACKLOG C7*). So it is branch-independent
  and **not** "changes requested handed back to my platform"; I left both PRs
  alone per STEP 1.5. No reviews and no comments on either.
- **Worth flagging plainly: STEP 1.5's "failing checks" trigger is currently
  unusable on this project.** Every PR and every branch has three red checks and
  will until C7 lands, so "does my platform's PR have failing checks" is always
  yes and carries no signal. A run that took the rule literally would spend every
  session re-investigating a known-expected failure instead of taking backlog
  work. I resolved it by comparing against `main`'s own run, which is the check
  that actually discriminates — **recommend that comparison become the rule**, and
  note **B1** is the row that fixes the underlying problem.
- Run-level conclusion says `success` while every job says `failure`, because of
  the job-level `continue-on-error`. `gh run list` is therefore actively
  misleading here; read `…/actions/runs/<id>/jobs`, not the run.

**Next:** **C8 needs one line from Jeff** — merge the PROPOSED entry to answer
it. Under option (b) the execution is one commit in each of two repos and the
audit already contains both exact edits. Otherwise the mac NEXT pointer is
**P7a** once [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) merges
(that PR moves it), and P7a is well-specified with its measurements already
taken. Independently: **B1** is now doing more damage than its row suggests —
see the STEP 1.5 note above.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/C8-it-empty-header`, TideSynth only — no other repo was
committed in. **Expect a JOURNAL.md conflict with [#31](https://github.com/JeffMcClintock/TideSynth/pull/31)**:
both insert at the top of the file, and both are open at once. BACKLOG.md should
merge cleanly — #31 edits the NEXT block and the P7 row region, this edits the C8
row only, and I deliberately left the NEXT block untouched because #31 already
moves `mac` to P7a.

---

## 2026-08-10 — macos — P7

**Did:** Audited the macOS and X11 resize paths for the P4
time-of-check/time-of-use crash, and ported the Windows regression test to Cocoa.
Findings in [docs/p7-resize-audit-mac-x11.md](docs/p7-resize-audit-mac-x11.md);
the test is `GMPI_Wrappers/tests/mac_editor_resize_host.mm`. **No behavioural
change was made to `gmpi_ui` or `SEVSTGUIEditorMac.cpp`** — reasons below, they
are deliberate and they are the main judgement call in this run.

**Result — the crash does not exist on macOS, and cannot by that mechanism.**
Not "was not observed": the structure rules it out. On Windows the device is
rebuilt *inside* the resize, so a checked pointer can be invalidated before it is
used. On macOS the resize only tears down:

```
SEVSTGUIEditorMac::onSize  ->  resizeNativeView  ->  [NSView setFrame:]
    ->  DrawingFrameCocoa::onResize()  ->  CGContextRelease(backBuffer); backBuffer = nullptr;
```

`onResize` ([DrawingFrameMac.mm:434](#)) is three lines and uses nothing
afterwards. Reallocation is lazy, in `onRender`, where the `if(!backBuffer)` test
and the use are adjacent. `onSize` uses nothing after `resizeNativeView`. There is
no window for staleness. X11's `reSize` ([DrawingFrameX11.cpp:932](#)) is safe for
a different reason: it writes its fields and calls `XResizeWindow` *last*, and X
requests queue rather than dispatch synchronously, so there is no re-entrancy to
survive.

**Verification artifact.** `mac_editor_resize_host` built universal (x86_64 +
arm64), AppleClang 21, macOS 26.3.1:

| Plugin | runs | oversized resize+paint pairs | result |
|---|---|---|---|
| `GainGui_VST3`, built from local `gmpi_ui` + `GMPI_Wrappers` | 3 | 6 per run | **exit 0, survived 3/3** |
| `TIDE_VST3` Release, the existing bundle | 1 | 6 | **exit 0, survived** |

Both were provably live *before* the oversized rects (liveness A and B, 19–65
distinct colours) and still drawing *after* recovering to their original size, so
"survived because it never ran" and "survived because resize became a no-op" are
both excluded. Rects exercised each pass: `2178 x 32672` (the rect from the
Windows dump), `0 x 0`, `1 x 1`, `16385 x 600`, `600 x 16385`, then recovery.

**What is actually wrong on macOS, since it is not a crash:**

- **No upper bound on extent, anywhere** — not in `onSize`, not in
  `resizeNativeView` ([:788](#)), not in `initBackingBitmap` ([:406](#)).
- **And — measured, not assumed — there is no NULL to fall back on.** I expected
  `CGBitmapContextCreate` to refuse these sizes and `onRender`'s `:113` guard to
  catch it. It does not refuse them. Probed directly with the exact format
  `initBackingBitmap` asks for: `2178 x 32672` **ok**, `16384 x 16384` **ok**,
  `65536 x 600` **ok**; binary search puts the square limit at **131071** and one
  axis at **4194303**. CoreGraphics reserves lazily. The only extent it refuses is
  `0 x 0`. So the whole `if(!backBuffer) return;` safety story applies at exactly
  one size.
- **The consequence is memory.** Resident size climbs into the hundreds of MiB and
  one measured paint at `16385 x 600` cost **+253 MiB**. Numbers are noisy — they
  include the harness's own bitmaps — but the order of magnitude is the finding.
- **`checkSizeConstraint` never writes the rect back** ([SEVSTGUIEditorMac.cpp:79](#)),
  the pre-P4b shape. Both branches observed: GainGui (resizable) returns
  **`kResultTrue`** and TIDE returns `kResultFalse`, and *neither* touches the
  rect. The resizable case is the sharp one — the wrapper **affirmatively
  approves** `2178 x 32672`, with no clamp behind the answer.
- **Two latent TOCTOUs, neither demonstrated.** `onRender` re-checks `backBuffer`
  after `arrange` at `:113` — correct — and then does *not* re-check it after
  `drawingClient->render()` at `:207` before using it at `:212`/`:215` (**P7b**).
  X11 `present()` is worse-shaped: `pw`/`ph` are cached at `:1262`, checked via
  `ensureImage` at `:1267`, and used at `:1319` after `measure`/`arrange`/`render`
  all re-enter client code — `d.image` is re-read but its *extents* are not, so a
  nested `present()` at a smaller size would overflow the heap rather than
  dereference null (**P7c**).

**Why I changed no shared code, since that is the arguable part.** Three reasons,
weightiest first. (1) P7 asks two questions and asks for the test ported; the
answer is "no crash", so nothing here justifies editing the backend every GMPI
plugin and SynthEdit itself depend on. (2) A clamp needs a defensible number and
there is not one — Windows' 16384 is a hard D3D11 limit, CoreGraphics accepts
131071², so a macOS bound is a product decision about how much memory an editor
may reserve. Picking one silently inside shared code is the guess the run prompt
warns about. (3) The standing direction for these repos is to rebuild SynthEditCL
as well as TIDE, and **P6** says SynthEditCL does not build on macOS — so a
behavioural change to shared rendering code cannot be validated against its other
consumer on this box today. Filed as **P7a** with all the measurements, so
whoever takes it chooses a number with evidence instead of copying 16384.

**Learned:**

- **The port's danger was not the crash, it was the liveness probe.** The Windows
  harness proves the renderer is live by making a benign resize and checking the
  window adopted it. That is sound *there* because adoption proves `reSize` got
  past its device check. On macOS `resizeNativeView` calls `setFrame:` with **no
  device check at all**, so adoption holds with no renderer whatsoever. A literal
  port reports a confident false PASS. Liveness here had to become two facts: the
  view adopted the size, **and** a forced paint produced real drawing.
- **And the resize allocates nothing, so `onSize` alone tests nothing.** Windows
  gets the reallocation free from `SetWindowPos` sending `WM_SIZE`. On macOS it
  happens in the next `drawRect:`, so every resize in the harness is followed by a
  forced synchronous paint. Without that the test would have "passed" while
  exercising only `CGContextRelease`.
- **I produced a tidy wrong finding and caught it; the catch is the lesson.**
  First version sampled one 200x200 tile at the view's origin, got one distinct
  colour at `2178 x 32672`, and I wrote down "the editor goes blank". It does not.
  The view is far larger than its 200x200 window so most of it is clipped and never
  rendered, and AppKit's unflipped origin is the **bottom**-left, which after that
  resize sits far below the window — the tile was in a region that legitimately
  never drew. Exactly P2's error of measuring the parent `HWND`, in Cocoa dress.
  Fixed by sampling corners and centre of `[v visibleRect]`. **Then it was still
  wrong to gate on**: at `16385 x 600` GainGui's tiles are uniform while TIDE's
  return 65 distinct colours, because whether a sampled region has content depends
  on where the client puts it. Distinct-colour counts at absurd extents are now
  diagnostics only. Two plugins is what made this visible — one would have left me
  with a plausible false claim in the doc.
- **`cacheDisplayInRect:toBitmap:` does not exist.** It compiles as an unknown
  selector returning `id` and throws at run time. `...toBitmapImageRep:` is the
  real one. The compiler warned and the warning was the only thing standing
  between this and a runtime exception mid-probe.
- **This box's SynthEdit build tree has the X3 trap live in it.**
  `~/Documents/GitHub/SynthEdit/build/CMakeCache.txt` has
  `GMPI_WRAPPER_FOLDER_OVERRIDE:PATH=` **empty**, so it links a `GMPI_Wrappers`
  frozen at `1a68601`, 2026-05-14, out of `build/_deps/gmpi_wrappers-src` — while
  `gmpi_ui`, `GMPI` and `SynthEditLib` are all correctly overridden. Exactly the
  half-overridden state that made a Linux VST3 unloadable in X3. It is Jeff's tree
  and I did not touch it, but any run that "tests a GMPI_Wrappers change" by
  building TIDE from that directory is testing May's code. I built into a fresh
  scratch directory instead and read the configure banner to confirm all three
  local paths were taken.
- **`GMPI-plugins` cannot link a GUI plugin on macOS at all** — `_OBJC_CLASS_$_UTType`
  undefined, because `gmpi_ui/backends/MacFileDialog.h:8` uses `UTType` and that
  repo's link line never adds the framework, while
  `SynthEdit/EditorLib/CMakeLists.txt:166` does. Pre-existing, invisible until
  someone built a GUI plugin outside SynthEdit. `GMPI-plugins` is on neither the
  ALLOWED nor the GATED list, so it is GATED by default; worked around at configure
  time with `-DCMAKE_MODULE_LINKER_FLAGS="-framework UniformTypeIdentifiers"`,
  touching no file, and filed as **P7d** with the scope question named — the
  requirement plausibly belongs in `gmpi_ui`, which is ALLOWED.

**Build health:** no claim about SynthEdit or SynthEditCL — neither was built, and
P6 says SynthEditCL does not build on macOS anyway. TIDE's existing Release bundle
loads, instantiates, opens its editor and renders under this harness, which is a
narrower claim than "TIDE builds" and is the one I can actually support. `gmpi_ui`
was not modified, so nothing that consumes it is at risk from this run.

**Next:** **P7a** — the one real macOS defect this found, and the audit already did
the measuring a clamp needs; read the doc before reaching for Windows' 16384,
because it does not apply. **P7b** is minutes if someone is in that file anyway.
**P7c** is the X11 half and is `linux` by necessity — this box cannot build or run
X11, and the first thing to establish there is whether a nested `present()` is
reachable at all. **P7d** needs a one-line ruling from Jeff before it is one line of
CMake.

Nothing is in flight on this box. All working copies were clean before this run and
are back on their default branches after it; the only repos committed in are
TideSynth and GMPI_Wrappers, each with an open PR.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/P7-resize-audit` in both repos —
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) (test +
CMake, additive) and TideSynth (audit doc, backlog, this entry). Independent:
the wrappers PR changes no shipped code, so merging either alone cannot break a
build.

---