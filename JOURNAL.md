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

## 2026-08-20 — macos — A21: the identity gate stops on a wrong answer, not on no answer

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (the Claude Code CLI version is not resolvable on this box — `claude --version` is *command not found*) · as **tide-rack-bot** (both paths, and this entry is the first to say so — see below)

**Third item this session**, on Jeff's instruction, after syncing the repos.
Claimed with a pushed DOING mark before any work.

**Did:** STEP 0.7 now runs **two identity paths** — `gh api user` and
`gh api graphql '{ viewer { login databaseId } }'` — and reads them with rules
that separate **asserted wrong** (STOP, always) from **could not assert** (retry,
then STOP and journal). Wording only; no code.

### All four branches were exercised, not reasoned about

| scenario | REST | GraphQL | rule |
|---|---|---|---|
| healthy | `tide-rack-bot` | `tide-rack-bot 314850083` | proceed, record `(both)` |
| **one path down** — REST forced through a dead proxy | `Get "https://api.github.com/user": proxyconnect …` | `tide-rack-bot` | proceed, record `(GraphQL)` |
| both down | transport error | transport error | retry ~1 min, then **STOP** |
| **credential missing** — `unset GH_TOKEN` | **`JeffMcClintock`** | **`JeffMcClintock`** | **STOP**, unconditionally |

**Row 4 is the one that decides whether any of this is safe, and it is the good
news.** The GraphQL path falls back to Jeff's keyring credential *identically* to
the REST path, so it **cannot launder a missing token**. Adding it is therefore a
second equivalent assertion, not a bypass — which is the only thing that would
have made A21 a weakening of the gate rather than a fix to it.

### The finding worth more than the change

**The two failure kinds are textually distinguishable, so a run never has to
judge which one it is in.**

- A **transport** failure yields **no login at all** — an error string.
- A **credential** failure yields a **perfectly valid login that is the wrong
  one**.

So: *if you are holding a login string, you are in the asserted case, and the
only question is whether it says `tide-rack-bot`. If you are holding an error,
you are in the could-not-assert case, and retrying is correct.* Never treat an
error as licence to continue; never try to retry a wrong name away. That is now a
table in the prompt rather than a judgement call, which matters because the run
making the call is the one whose credentials are in question.

### Two things the row did not anticipate

**GraphQL is the STRONGER assertion, not a lesser fallback.** It returns
`databaseId` = **314850083**, which is exactly the number hard-coded into
`GIT_AUTHOR_EMAIL` (`314850083+tide-rack-bot@users.noreply.github.com`). So it
checks the identity the run's **commits** will carry, where REST only checks the
one its API calls will. Prefer it when both answer.

**A latent documentation bug, in the one rule where being read correctly matters
most.** The paragraph beginning *"That second command MUST print
`tide-rack-bot`"* named the **wrong command**: the second command was the
`insteadOf` transport check, and the identity call was the first. It had said
that ever since the transport check was added, and every run since has had to
silently repair it while reading. Fixed, with a note saying so.

**Also updated:** STEP 0.5's record line, and STEP 4's provenance template, which
now reads `as <login> (<REST|GraphQL|both>)`. This entry's own `**Prompt:**` line
is the first to carry it.

### Repos synced first, per the instruction

Five advanced, five already current, three skipped:

| skipped | why |
|---|---|
| `JUCE` | untracked `modules/juce_audio_processors/format_types/VST3_SDK/` |
| `gimpi_ui_tests` | untracked `build-bench/` |
| `VST_SDK` | detached HEAD, no upstream, untracked `build/` |

All three are build artefacts or a vendored SDK rather than work in progress, but
they predate this run and STEP 5 says the developer's dirt is not mine to clean.
`SynthEdit` `2f5fca5e3 → 6c7e90053`, `SynthEditLib` `65d55cd → 5af259e`,
`TideSynth` `8917c04 → 61b4707`, `gmpi_ui` `6700070 → ad5b681`,
`synthedit-website` `83771db → e51a7c3`; all fast-forwards, none ahead.

**Learned:**

1. **"Add a fallback" was the wrong frame and the row's own wording carried it.**
   The question that decides safety is not *is the second path good enough*, it is
   *does the second path fail the same way the first does when the credential is
   missing*. Measured: it does. Had it not, the correct answer would have been to
   leave the gate alone and accept the lost runs.
2. **A rule that requires the reader to classify a failure will eventually be
   classified wrong**, by whoever is most motivated to continue — which is the
   run itself. Giving the classification a mechanical tell (login string vs error
   string) is what makes it a rule rather than an appeal to judgement.

**Next:**

1. **A22, A23, A24** are the remaining A-series rows. A23 is the best-specced —
   duplicate-id detection in `check-id-refs.py`, with a positive control already
   written into the row.
2. **A22 pairs with this one** — both are rules whose premise stopped matching how
   the fleet runs.
3. **E17** still gates every E2 module stage; **E10** still needs `SynthEditLib`
   authority.

**Branch/PR:** [#176](https://github.com/JeffMcClintock/TideSynth/pull/176), branch
`tide/mac/A21-identity-gate` — one repo, TideSynth only.
**Merge order matters:** this branch and `tide/mac/A28-community-research` (#175)
were both cut from `main` and both touch `BACKLOG.md` and `JOURNAL.md`, so
whichever merges second will want a rebase. No overlap in `docs/`.
Throwaway worktree; the developer's checkout stayed on `main` and clean.

## 2026-08-20 — macos — A28: the refuted hypothesis, corrected in the four places that state it and the one that originates it

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (the Claude Code CLI version is not resolvable on this box — `claude --version` is *command not found*) · as **tide-rack-bot**

**Second item this session**, on Jeff's instruction after A27 merged. Claimed
with a pushed DOING mark before any work, per STEP 2.

**Did:** A9's standing hypothesis — *"no open-source modular exists on iOS AUv3"* —
is false, refuted by plugdata on 2026-08-18. Every live statement of it now
states the surviving narrower form instead: **no open-source *Eurorack-style
rack* on iOS AUv3.**

### The row named two docs. There were four sites, and the fourth is the source

| site | what it was |
|---|---|
| `docs/community-research.md:58` | named by the row |
| `scripts/community-research.py` — `HYPOTHESIS_RE` comment | **not named** |
| `scripts/community-research.py` — `source_hypothesis()` docstring | **not named** |
| `docs/process-review-2026-08-09.md:124` | **not named — and it is where the hypothesis originates** |

That last one matters more than its size. The script's own comment reads *"The
standing product hypothesis from docs/process-review-2026-08-09.md"* — so
correcting the two docs the row named would have left the **citation pointing at
the false version**, which is a worse state than before: a corrected doc that
cites an uncorrected source reads as though the source agrees with it.

**PLAN.md needed no change**, which is worth recording because it looks like it
should have. `PLAN.md:138` already quotes the hypothesis and calls it *"false as
written"* in the next line, and already handles this row's other caveat — the
AUv3 memory ceiling is there as *"Flagged for Jeff; not added unilaterally."* So
no PLAN amendment was made or needed.

**The review doc is MARKED, not rewritten**, and this is a judgement worth
overruling if you disagree: `docs/process-review-2026-08-09.md` is the record of
what the 2026-08-09 review concluded, so its paragraph is left standing with an
inline ⚠ and a dated correction block underneath, rather than edited to say
something the review did not say. The inline marker exists so the paragraph
cannot be read standalone.

### plugdata is on the watch list operationally, not just in prose

The row asked for plugdata on the watch list. Prose alone would not have done it:
`plugdata` is now in **both** `HYPOTHESIS_QUERIES` and `HYPOTHESIS_RE`.

**They have to move together, and nothing said so before.**
`source_hypothesis()` searches for each query, then discards any hit whose
**title** `HYPOTHESIS_RE` does not match. A query with no matching regex term
therefore returns nothing — silently — which is precisely the *"working watch
that had simply found nothing"* failure the `watch` source was built to avoid.
That coupling is now asserted in `--selftest`, documented at both sites, and
written into `docs/community-research.md`.

### Verification

**The coupling assertion is proven able to fail**, not merely present. It is
demonstrated with a synthetic sentinel:

```
FAIL query 'zzq-not-a-product' matches no HYPOTHESIS_RE term -- source_hypothesis()
would discard every hit for it
23 classification case(s), 1 failed
```

exit **1**. Unmodified, exit **0**.

**That sentinel replaced a real product name, and the reason is the finding.**
The first version of this control used a `drambo` query — and `drambo` became a
real title-match term a few hours later, when Jeff pointed out miRack is the
actual iOS competitor. The control would then have passed **for the wrong
reason**, silently, while appearing to still test something. `--selftest` now
asserts the sentinel stays unmatched, so the next person to broaden the regex is
told rather than left to notice. **A control that a later, correct change
disarms is worse than no control**, and this one had a half-day lifetime.

#### Jeff's refinement, and why it is more than a wording change

*"plugdata is not really in the category of eurorack simulators. However mirack
is an ios competitor."*

Correct, and the first version of this change did not encode it — it put plugdata
on the watch list and left the actual competitor off. The list now splits by
**kind**, in the regex comment, in the doc, and in the selftest:

| kind | terms | why |
|---|---|---|
| **Competitor** | `mirack` (query + title), `drambo`, `audulus` (title) | they hold TIDE's real square — an iOS Eurorack-style rack — and all three are closed. **miRack is the product the surviving hypothesis is a claim about**, so an open-source answer to it is the highest-value item this routine could ever surface. |
| **Precedent** | `plugdata` (query + title) | a Pd patcher, **not a rack**. Watched because it already solved the iOS AUv3 packaging, distribution and review problems TIDE will hit, and because it refuted the broader claim. |

A query needs a matching title term; a title term needs no query. So `drambo` and
`audulus` are title-only, which still catches mentions the other sources surface.

**A/B, shipped script vs this branch** — four discriminating positives and two
negative controls:

| title | shipped | this branch |
|---|---|---|
| `miRack 4.6 adds a new sequencer` | `keep` | **`flag`** |
| `Anyone tried Drambo for generative patches?` | `keep` | **`flag`** |
| `Audulus 4 module sharing` | `keep` | **`flag`** |
| `plugdata 0.9.3 released` | `keep` | **`flag`** |
| `Rack-style sequencer module request` | `keep` | `keep` |
| `Pure Data style dataflow patching in a rack` | `keep` | `keep` |

**Stated rather than glossed:** `How does plugdata ship a standalone AND an
AUv3?` is `flag` on *both* — the existing `auv3` term already caught it — so it
is kept as a case for the hypothesis-beats-reject rule and proves nothing about
the new terms. The two `keep` rows are what rule out "everything is flagged".

`--selftest` **23 cases, 0 failed** (was 17). `check-links.py` goes 418 → **421**
relative links with the broken count unchanged at 1, so the new relative links
resolve.

### The lint break cleared mid-run — A29 is DONE

**Jeff pushed `modules/PanelTest/` at
[`61b4707`](https://github.com/JeffMcClintock/TideSynth/commit/61b4707) while this
item was in flight, so A29 is closed and `lint` is green end to end.** Re-verified
here rather than inferred from the commit: `check-links.py` → **418 relative
links, `no broken links`, rc=0** (was 1 broken), and the `modules/` tree builds
from a fresh worktree with the new subdirectory — **configure rc=0, build rc=0,
zero `error` lines**, targets `tide_render`, `tide_render_preview`,
`tide_render_regression`, `TiDEknob`, `TiDEPanel`, **`PanelTest`**.
[#174](https://github.com/JeffMcClintock/TideSynth/issues/174) closed, A29 flipped
to DONE and archived, `win` NEXT re-pointed at P3. **The A4 auto-merge tier is
live again**, which was the whole cost of the row.

**Measured before the push, since the idea of commenting the module out came up,
and worth keeping because it settles the question:** there was **nothing to
comment out and no broken build**. At `41785ea`, `PanelTest` appeared in **no
CMakeLists anywhere** on `origin/main` — only three prose/comment references — and
the `modules/` tree configured and built clean without it (**configure rc=0,
build rc=0, zero `error` lines**). The break was only ever the dangling
reference, so the push was the whole fix.

**The one thing that survives the fix:** the broken link was **one of three**
references to that file. The other two are C++ comments at
`modules/common/TidePathTracer.h:21` and `:883`. All three resolve today, but
**no check reads the comments**, so if `PanelTest/` ever moves or is unpublished,
those two dangle silently and only the README's would be caught. Noted rather
than filed — a one-line risk, not a defect.

**Learned:**

1. **A row that names the files to fix is naming symptoms, not the set.** Two of
   the four sites here were in a script rather than a doc, and the fourth was the
   *origin* of the claim. `grep` for the sentence, not for the filenames the row
   lists — and check whether anything **cites** the file you are correcting.
2. **"Add it to the watch list" is a two-part change in this script**, and the
   two parts are 100 lines apart with nothing linking them. A prose-only or
   query-only edit would have looked done and watched for nothing.
3. **The A27 check caught this run's own regression, twice.** Re-pointing the
   `mac` row earlier today left a live *"Take A29"* phrase in the superseded
   quote, and `check-next-block.py` flagged it. Flipping A29 to DONE just now, I
   did **the same thing again** in the `win` row — the quoted history still read
   as an instruction, and the check failed with
   `BACKLOG.md:11 [win] A29 -- archived DONE`, matched on `'Take A29'`. Both
   times the fix was to strip the imperative from the quote. That is the check
   working on exactly the class of mistake it was written for, on a file its own
   author was editing, within hours of shipping — which is better evidence than
   any fixture.
4. **Name the *kind* of thing being watched, not just the thing.** The first
   version of this change watched plugdata — the refutation — and left miRack,
   the actual competitor, off the list entirely. Nothing in the row or the code
   would have caught that; it took Jeff reading it. A watch list of bare product
   names invites exactly this, so the list now says competitor or precedent
   against each name.
5. **A test control built from a real name has a shelf life.** The `drambo`
   sentinel was correct when written and wrong within hours, in the ordinary
   course of the code getting better. Controls want values that cannot become
   legitimate.

**Next:**

1. **A29 / #174 is DONE** — cleared mid-run, verified here, auto-merge live again.
2. **A21, A22, A23, A24** are the remaining A-series rows — all small, all in this
   repo, all with stated acceptance checks. A23 is the one with a positive control
   already specced.
3. **E17** still gates every E2 module stage; **E10** still needs `SynthEditLib`
   authority.

**Branch/PR:** [#175](https://github.com/JeffMcClintock/TideSynth/pull/175), branch
`tide/mac/A28-community-research` — one repo, TideSynth only.

Throwaway worktree; the developer's checkout stayed on `main` and clean.

## 2026-08-20 — macos — A27: the NEXT block's Take column is read now, and the docstring stops lying about it

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** — the Claude Code CLI version the previous entries report as "app 2.1.220" was **not resolvable on this box** (`claude --version` → *command not found*; nothing under `~/.claude` or the app bundle carries an unambiguous CLI version), so the measurable number is recorded rather than a guessed one · as **tide-rack-bot**

**Did:** `scripts/check-next-block.py` now treats **a bolded ID that a Take cell
*begins* with** as a take-target, which is exactly the narrowed rule A27 specced.
Also filed **A29** (`lint` is red on `main`), archived four IN-REVIEW rows whose
PRs have all merged, and re-pointed the `mac` NEXT row, which was sending runs at
an item that cannot be taken.

### The bug was a stale comment, and that is the whole reason it survived

The module docstring said the trigger set was *"the Take column, which is
definitionally a take-target, plus a short list of imperative phrases"*. The Take
column was **never** in the trigger set — `run()` read only the phrases, and the
loop carried the opposite note. So on 2026-08-19 the `any` row read
`**C6** — move EditorLib/CMakeLists.txt` with C6 archived DONE hours earlier, and
`lint` was green. Nobody was going to find that by reading the code, because the
code's own description of itself was wrong.

`RE_LEADING_TAKE_ID` is anchored at the start of the cell and **requires the bold
markers**. It is deliberately *not* subject to the negation rule the phrase
matcher uses: position is the assertion. A cell opening `**C6**` points at C6
whatever the rest of the paragraph says.

### Verification — the positive control, and an A/B on a fixture

**Positive control out of git history**, as the row demanded. `781ecbb` is the
commit that filed A27; `781ecbb^` is the state it was filed about:

| script | on `781ecbb^` | on this tree |
|---|---|---|
| shipped (`origin/main`) | 3 checked, **rc=0** | 5 checked, rc=0 |
| fixed | 5 checked, **rc=1**, `C6 -- archived DONE` | 9 checked, rc=0 |

The failure names C6 **alone** out of five take-targets and quotes what matched
(`Take cell begins with **C6**`), so it is not a blanket alarm.

**Zero false alarms on the live block**, which was the risk the original author
measured and declined to take. The new rule adds exactly three take-targets —
`win`/**P3**, `linux`/**C7b**, `any`/**C14** — and all three are live rows. None
of the original seven false alarms is among them: they were mid-paragraph IDs in
warning clauses, and the leading position is not a position any of them occupied.

**A standalone A/B fixture**, because a check that has only ever been run on real
files is hard to trust. A Take cell reading `**D6** — move the CMakeLists`, with
D6 archived and **no take-verb anywhere in the cell**: shipped script *0 checked,
rc=0*; fixed script *1 checked, rc=1*. That case is now `archived leading id` in
`--selftest`, which is **22 cases, 0 failed** (was 13).

**One thing A27 predicted wrong, worth correcting rather than quietly passing:**
the row said the rule "catches `win` and `any` and touches neither `mac` nor
`linux`". `linux`'s Take cell has since become a short field too, so it fires on
**three** rows. Still no false alarm — C7b is live — but the prediction was made
against a block that has already changed shape once since, which is itself the
argument for the check.

### `lint` is RED on `main` — filed as A29, deliberately not fixed

`python3 scripts/check-links.py` on `origin/main`: **1 BROKEN** of 418 relative
links — `modules/common/README.md:14` → `../PanelTest/PanelTestGui.cpp`, no such
file. Introduced by `41785ea`, the current tip of `main`, which is a direct
(interactive) commit.

It is **not a typo**: `PanelTestGui.cpp` has never existed in this repo
(`git log --all --diff-filter=A -- '*PanelTest*'` is empty) and nothing matching
`*PanelTest*` exists anywhere under this box's `~/Documents/GitHub`, across all
fifteen checkouts. The reference is to a tree that is not public, or to a file
not yet committed.

**Why it is worth an entry rather than a footnote:** `auto-merge.yml` fires only
on a `lint` run whose conclusion is `success`, so **the entire A4 auto-merge tier
is dead** until this is cleared, and every PR from every box shows a red check
that is nobody's. **This PR's `lint` will be red for that reason and no other** —
the five checks that read what this branch changed (`check-backlog-diff`,
`check-id-refs`, `check-next-block`, `check-journal-prepend`,
`check-prompt-provenance`) all pass locally.

Not fixed here because STEP 3 says file, do not fix, and because the fix is a
judgement only `41785ea`'s author can make: add the file, repoint the link, or
de-link the prose. Guessing puts a false statement in a README.

#### ANSWERED BY JEFF, mid-run: the file is on the Windows box

*"Maybe forgot to push it … it's on the windows machine."* So the link is
**correct in intent** and `modules/PanelTest/` simply never reached `origin/main`.
A29 is re-pointed from `any` to **`win`** — the fix is a push only that box can
make — and filed as [#174](https://github.com/JeffMcClintock/TideSynth/issues/174)
(`platform:win`) so the Windows run takes it at STEP 1 instead of waiting for
file order. The `win` NEXT row now says A29 before P3.

**This is the run's most useful negative result, so it is recorded rather than
quietly dropped.** Before Jeff answered, this run had assembled what looked like
a conclusive repoint target: `modules/TiDEPanel/TiDEPanelGui.cpp`. The evidence
was not thin —

- same `modules/` parent, so `../<Module>/` resolves;
- same `<Module>Gui.cpp` naming, and it is **one of only two** `*Gui.cpp` files
  in the entire repo (`git ls-tree -r origin/main | grep -i 'gui\.cpp$'`);
- the other one, `TiDEknobGui.cpp`, greps **zero** hits for `cache`, so the
  discrimination looked clean;
- `TiDEPanelGui.cpp`'s own header comment says **"TWO CACHED BITMAPS"** and it
  has a `renderFace()` that procedurally generates the face — which is the
  README sentence (*"already caches its procedurally generated faces"*) almost
  word for word;
- both files were authored the same day by the same author, `20fa184` … `41785ea`.

**Every one of those is true and the conclusion is still wrong.** The lesson is
not "check harder" — the evidence would not have improved. It is that STEP 3's
*file, do not fix* is doing real work precisely when the fix looks obvious: a
repoint would have merged green, closed the lint failure, and left a README
citing the wrong file with nothing left to notice it. Cheap to avoid, expensive
to detect later.

### Bookkeeping done this run

- **Archived, PRs all merged:** **A26** ([#163](https://github.com/JeffMcClintock/TideSynth/pull/163)),
  **C7a** ([SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59) + [#167](https://github.com/JeffMcClintock/TideSynth/pull/167)),
  **E2b** ([SynthEdit#60](https://github.com/JeffMcClintock/SynthEdit/pull/60) + [#170](https://github.com/JeffMcClintock/TideSynth/pull/170)),
  **E2c** ([SynthEdit#61](https://github.com/JeffMcClintock/SynthEdit/pull/61) + [#171](https://github.com/JeffMcClintock/TideSynth/pull/171)).
  Moved verbatim; `check-backlog-diff` confirms all four.
- **`tide/mac/V3-midi-findings` is a pushed branch with no open PR** — its PR
  ([#142](https://github.com/JeffMcClintock/TideSynth/pull/142)) merged
  2026-08-18 and **two commits were pushed to it afterwards**, `25216c1` and
  `4e65874`. That is A22's failure mode, still live on this platform's branch.
  Left alone rather than force-tidied; STEP 4 forbids rewriting pushed commits.

### Why this run took a process row and not E2 — read this before re-deriving it

The `mac` NEXT cell said "Take E2". **E2 is not takeable and its own row says so**
— it never names which modules a "first curated set" contains, so its acceptance
check cannot be stated before starting. Its two live stages, E2b and E2c, are
both finished and archived above.

**The next E2 stage would be a new module prefab, and that is blocked by E17, not
by anything technical.** E17 is open `NEEDS-JEFF` on TIDE's visual design
language, and its own `Decide-by` reads *"before E2 authors the curated set"*.
STEP 2 forbids work an open NEEDS-JEFF answer would change. The blocker is a
ruling, not a missing measurement.

Checked and rejected, so the next run need not repeat it: **C14** and **C10** are
`SynthEditLib`; **P3**, **E10**, **S1b**, **S5**, **S7**, **S8** are GATED and
none is a build break, so A17's exception does not reach them; **C7b** is the
`linux` row's own NEXT target; **C7d** says *"after C7b"* and its Accept cannot
pass before C14; **A12** and **B1** are `.github/workflows/**` the bot token
structurally cannot push. What is left is the A-series, which is where A27 came
from.

**Learned:**

1. **A comment that contradicts the code is a bug with no test that can fail.**
   Both A20's original measurement and this fix are correct; what rotted between
   them is the *shape of the data* — Take cells turned from paragraphs into short
   fields — and only the prose recorded the assumption that shape broke.
2. **The fleet's CI gate can be down without anyone noticing**, because a red
   check on your own PR reads as your own problem. Run `check-links.py` against
   `origin/main`, not just your branch, if you want to know whose red it is.
3. **Superseded text in a NEXT cell must lose its imperative, not just its
   position.** Re-pointing the `mac` row this run left the old *"Take A29"*
   wording quoted underneath, and `check-next-block.py` — the very check this
   run shipped — read it as the `mac` row still naming A29, a `win` item. The
   phrase rule matches mid-cell by design, so a preserved quote is
   indistinguishable from a live instruction. Reworded the quote instead of
   widening the rule: the fleet's habit of keeping previous text is worth more
   than a byte-exact quote of an instruction that no longer holds.
4. `git checkout origin/main -- .` inside a worktree will silently revert your
   own uncommitted work. Recovered here via `git stash`; the cheap habit is to
   commit as soon as a coherent change exists, which STEP 3 already says.

**Next:**

1. **A29 is the Windows box's**, not mac's — [#174](https://github.com/JeffMcClintock/TideSynth/issues/174),
   `platform:win`. It is a push, and it revives auto-merge for all three boxes.
2. **E17** needs Jeff. Until it is answered, E2 has no takeable stage, and the
   `mac` column's real queue is the A-series.
3. **E10** remains the biggest thing on this platform and needs `SynthEditLib`
   authority.

**Branch/PR:** [#173](https://github.com/JeffMcClintock/TideSynth/pull/173), branch
`tide/mac/A27-next-block-take-column` — one repo, TideSynth only.
Worked in a throwaway worktree; the developer's checkout was left on `main`
untouched and was clean throughout.

## 2026-08-19 — macos — E2c: SV Filter4 and Oscillator HD linked into TIDE, and the XML list that is hardcoded twice

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Fourth item this session**, on Jeff's instruction: *"lets get the SVFilter in
TIDE and also the 'Oscillator HD' these are our go-to MVP modules"*. Claimed
with a pushed DOING mark before any work, per STEP 2.

**Did:** Linked both into TIDE. `SE SV Filter4` ("StateVar Filter"),
`SE SV Filter4B` and `SE Oscillator4` ("Oscillator HD") are now registered in
TIDE's factory with their pins, verified from inside the running app.

### Why they were absent, and the trap in the reason

Both build into **separate loadable targets** (`VaFilters`, `OscillatorHD`)
rather than into the `SynthEditLib` static library — the same shape as E7's
converters and S8's `OscillatorNaive`.

**`UgDatabase.cpp` already had `INIT_STATIC_FILE(SVFilter4)` at `:1114`, and the
class was still absent from TIDE.** That is the thing to carry forward: **the
INIT list is `SynthEditLib`'s, and TIDE links a subset of it, so it is not a
menu of what TIDE can instantiate.** E2b's entry said the same from the other
direction; this is the confirmation, and it means "it is in UgDatabase" is never
sufficient evidence.

### Three sources, and the third is one only the linker finds

| source | brings |
|---|---|
| `modules/VaFilters/SvFilter.cpp` | `SE SV Filter4`, `SE SV Filter4B` |
| `modules/OscillatorHD/Oscillator.cpp` | `SE Oscillator4` |
| `modules/shared/real_fft.cpp` | `realft()` |

The link failed first time on exactly one symbol —
`realft(float*, unsigned int, int)`, referenced from
`MipMapCalculator::generateWavetable` in `Oscillator.o`. Oscillator HD builds
mipmapped wavetables with an FFT that lives in another translation unit. **This
is the same second-TU rule `my_type_convert.cpp` records for Converters**, and
it is now two for two: a module added to TIDE's source list should be expected
to drag a helper TU with it.

### The finding worth more than the change: the XML list is hardcoded TWICE

`SE SV Filter4`'s pins live in `VaFilters.xml`, so the file has to be both
**staged** and **read** — and those are two separate hardcoded lists:

- `_tide_xmls` in `SynthEditSem/CMakeLists.txt` copies it into
  `Contents/Resources`
- the loop at `TideApp.cpp:488` is what actually reads it back

Adding it to only one **fails silently in whichever direction you missed**.
Staged-but-never-read says nothing at all; read-but-never-staged at least prints
to stderr. The CMake side already carries a comment about a near-identical split
that "cost a debugging cycle on V3" — that comment is about two *staging* blocks,
and this is a **third** place the same list is written. Both sites now say they
must move together.

**A module with only its `.cpp` is worse than a missing module**, per the note
already in `TideApp.cpp:474`: a class with no pins "takes the whole enclosing
container's widget layer down with it — a blank rack rather than one missing
module".

### Making that failure visible for good

The merge loop now reports what it did, which is three lines and is also this
change's own verification artifact:

```
TIDE: ControlsXp.xml    enriched  2 of 18 described class(es)
TIDE: SubControlsXp.xml enriched  1 of 27
TIDE: MidiPlayer2.xml   enriched  2 of  7
TIDE: Converters.xml    enriched 26 of 70
TIDE: VaFilters.xml     enriched  2 of  7
```

A **zero** means the `.cpp` half was forgotten. The gap between the two numbers
is expected and harmless — it is the entries TIDE deliberately does not link.
Flagged in the PR as a behaviour addition beyond the literal ask, so Jeff can
drop it.

### Verification — from inside TIDE's factory, with negative controls

`VaFilters.xml enriched 2 of 7` is itself proof for the filter: exactly the two
classes whose `.cpp` is now linked, with RMS / Korg / Moog / SV Filter2 / Moog
test correctly skipped.

Oscillator HD has no XML to count, so a **temporary probe, since reverted** (the
issue **#87** lesson — `grep PROBE` on the branch returns nothing):

```
SE Oscillator4         REGISTERED
SE SV Filter4          REGISTERED
SE SV Filter4B         REGISTERED
1 Pole LP              REGISTERED   <- known-good positive control
SE Oscillator (naive)  absent       <- S8's module: the probe still discriminates
SE SV Filter2          absent       <- described in VaFilters.xml, .cpp not linked
```

**Two positive and two negative controls**, so "everything says REGISTERED" is
ruled out. The last row is the more interesting one: it shows the enrichment
guard skipping a described-but-unlinked class rather than creating a phantom
browser entry, which is the property that makes it safe to point the merge at a
file describing far more modules than TIDE links.

**TIDE, TIDE_VST3, TIDE_STANDALONE and SynthEditCL all build.** TIDE_STANDALONE
runs, seeds 6 prefabs, and shuts down clean with no crash report.

### Not done, deliberately

**The prefabs still use the old DSP.** "Go-to" suggests Jeff wants
`SE Oscillator4` and `SE SV Filter4` to be what TIDE's Oscillator and Filter
rack modules are built from — but swapping the DSP inside two already-shipped
prefabs is a product change, and E2b's Filter prefab is still in an open PR. It
is a small change to `build-prefabs.py` plus re-measuring two E1 cases. Asked
rather than assumed; noted in both PR bodies.

Also worth knowing before that swap: **SV Filter4's pin defaults are not on the
same scale as the modules already in the prefabs.** Its `Pitch` defaults to
`0.5` and it has `Resonance`, `Strength` and `Mode` (LP/HP/BP/BR) pins, where
`1 Pole LP` has `Signal`/`Pitch`/`Output` and takes 5 V = 440 Hz. Whoever does
the swap should measure the mapping rather than assume it matches, exactly as
E2b measured the 1-pole's.

**Next:**

1. **The prefab swap**, if Jeff wants it — see above.
2. **The over-wide relaxed gates filed earlier this session** — two harness
   cases declare tolerances ~55 dB wider than their own measured cross-platform
   residual. The row for it is in [#170](https://github.com/JeffMcClintock/TideSynth/pull/170),
   still open, so it is deliberately not named by id here: `check-id-refs.py`
   validates prose mentions against rows that exist ON THIS BRANCH, and naming a
   row that lives only in another open PR fails `lint`. Worth knowing, because
   it will catch anyone cross-referencing between two open PRs — the check is
   right, and the fix is to describe the row rather than cite it.
3. **E10** remains the biggest thing on this platform and needs `SynthEditLib`
   authority.

**Branch/PR:** [SynthEdit#61](https://github.com/JeffMcClintock/SynthEdit/pull/61)
plus the TideSynth PR carrying this entry. Two repos; the SynthEdit half is the
whole change and the TideSynth half is bookkeeping, so neither blocks the other.
Work done in a throwaway worktree for TideSynth; `SynthEdit` was branched in
place and is returned to `master`.
