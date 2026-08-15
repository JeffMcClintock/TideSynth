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

## 2026-08-15 — windows — A15 (interactive session, Jeff directing)

**Did:** **A15** — wired A10's [scripts/check-id-refs.py](scripts/check-id-refs.py)
into the `lint` job, which is the half A10 could not do because the bot token
is `repo` scope with **no `workflow`**. Pushed as Jeff rather than
`tide-rack-bot` for exactly that reason; his keyring token on this box does
carry `workflow` (checked with `gh auth status` first, since the A11 entry
records that this bites on other boxes and not here).

**Result — both halves of the Accept proven in the PR's own two-commit
history, in order, rather than asserted.**

| push | carried | `lint` |
|---|---|---|
| first | the wiring **+ a probe file naming a nonexistent row `Z9`** | **fail** |
| second | probe deleted | **pass** |

The failing run reads `id-refs: failure` with `links`, `journal`, `backlog` and
`provenance` all `success` — so it failed **for the right reason and only that
reason**, and the `ID_REFS` Summary wiring genuinely converts a step failure
into a job failure. The passing run reads all five `success` and
`456 ID reference(s) checked against 98 row(s), 91 distinct ID(s) named`.
Together those rule out the two ways this could have looked installed while
doing nothing: failing always, and passing vacuously.

**Learned — the Summary wiring is the part that would have silently rotted, and
A15's row was right to insist on it.** Every step in this job is
`continue-on-error`, so a step added *without* `ID_REFS` in the Summary's `env`
and `for` loop runs, prints its findings, fails — and the job goes green. It
would look wired for as long as nobody read a log. Same shape as A4's finding
that a path allowlist can look built while firing on nothing, and the reason
both halves are now demonstrated separately rather than assumed together.

**Learned — this check is deliberately not diff-based, unlike the four above
it, and the workflow now carries a comment saying so.** The other four compare
a base version against head. A cross-reference goes stale when the row it names
is **renamed or archived** — an edit to a *different file* than the one holding
the reference. A diff-scoped check would see the reference file unchanged and
pass, which is precisely the case A10 was filed for. So it reads the whole tree
every run. Cost is bounded: 456 references, 9 seconds.

**Next:** nothing on this row. The lint job is now five checks, all green on
`main`. Whoever next touches [scripts/check-id-refs.py](scripts/check-id-refs.py)
should re-run `--selftest` (20 cases) as well as the tree scan, since CI runs
only the latter.

**Side effects on this box:** none outside the scratchpad. TideSynth only; no
other repo was committed in or modified.

**Branch/PR:** [TideSynth#65](https://github.com/JeffMcClintock/TideSynth/pull/65).

---

## 2026-08-15 — windows — P9

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** **P9** — a lint that fails when the two `resource.h` copies stop
agreeing: [scripts/check-resource-h-drift.py](scripts/check-resource-h-drift.py).
Fifth item this session, at Jeff's direction, TideSynth-only. Took the row's
cheaper of two options — the lint rather than merging the files into one — on
the row's own pricing ("minutes for the lint; longer if the two are actually
merged") and because merging a Visual-Studio-generated header that VS will
rewrite is a change with its own failure mode.

**Result:** **318 `ID*` constants on each side, all agreeing.** Selftest 7/7.
Exit 1 on both divergence kinds — a same-name-different-value conflict, and a
constant present on only one side — tested on **scratch copies**, so neither
`SE16` nor `SynthEditLib` was written to; both were verified clean afterwards.

**Learned — excluding the `APSTUDIO_INVOKED` block is what makes this check
survivable, and it is not a detail.** The one thing the two files actually
differ in today is `_APS_NEXT_RESOURCE_VALUE`: **210** private, **207** public.
That is Visual Studio's own allocation counter, it compiles to nothing, and it
moves whenever anyone adds a resource in the IDE. A check that compared it would
be **red from birth and red forever**, and the first person to see it would turn
it off — taking the 318 real constants with it. So the whole `#ifdef
APSTUDIO_INVOKED` block is skipped, nesting-aware, and the selftest pins that
behaviour with the real 207→210 case as one of its seven.

The counter gap is still worth reading as evidence rather than noise: it says
the private copy has had **three resource slots allocated that the public one
has not**. Nothing has collided yet. Nothing would announce it if it did — which
is the entire reason this row exists.

**Learned — this closes a loop under C12a rather than sitting beside it.** C12a
delisted `${EDITOR_DIR}/resource.h` on the strength of the two copies being
identical, and that argument only holds while they stay identical, because
public and private TUs each resolve to their own copy by the own-directory-first
rule. That assumption had nothing enforcing it and was made this morning. Now it
has a check, and the check independently re-derives the same 318 that C12a
relied on.

**Next:** the row's larger option — pick one copy as the source of truth — is
still open and still unowned; the lint makes it safe to defer, not unnecessary.
**Note it cannot run in TideSynth CI**, since one of the two files is in the
private repo, so it is a dev-box/agent tool like
[scripts/dangling_private_includes.py](scripts/dangling_private_includes.py).
Its docstring says to run it as part of **any carve-out stage that moves a
`.cpp` out of `SynthEdit2`** — such a TU switches from the private copy to the
public one, a no-op only while this passes. **C12f is the next stage that does
that**, and its row already says to re-check `resource.h`; this is the command
for it.

**STEP 1 / 1.5:** unchanged. Open PRs are all this session's own.

**Side effects on this box:** copies of both `resource.h` files in the
scratchpad, and a throwaway git repo from the A14 run. Nothing outside it. This
run committed in TideSynth only; `SE16` and `SynthEditLib` were read but never
written, and were confirmed clean afterwards.

**Branch/PR:** [TideSynth#64](https://github.com/JeffMcClintock/TideSynth/pull/64),
fourth in the stack (61 → 62 → 63 → 64), each retargeting to `main` as its
parent merges.

---

## 2026-08-15 — windows — A14 (the guard for this morning's collision)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Built **A14**, filed earlier in this same session after a concurrent
Claude session on this box committed this run's staged changes as Jeff.
[scripts/check-commit-authorship.py](scripts/check-commit-authorship.py), plus
the STEP 3 and STEP 4 wording in
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md) that makes running it
part of every run. **Fourth item this session, at Jeff's direction, and still
TideSynth-only** — the second agent was confirmed still live in `SE16` and
`SynthEditLib`, which this run has not touched since C12b.

**Result — verified against a reconstruction of the actual incident, not by
reasoning about it.** A throwaway repo in the scratchpad, branch of four
commits:

| commit | author / committer | flagged |
|---|---|---|
| `legit bot commit` | bot / bot | no |
| `the foreign commit` | **Jeff / Jeff** | **yes** |
| `another bot commit` | bot / bot | no |
| half-set `GIT_*` | bot / **Jeff** | **yes** |

Exit 1, both flagged, the two clean ones untouched. Also exits 0 on a clean
branch and prints "on the default branch, nothing to check" when there is
nothing to compare, so it is safe to run unconditionally.

**The fourth row is the one I would not have thought to test if the prompt had
not already documented it.** `GIT_AUTHOR_*` exported but `GIT_COMMITTER_*` not
is the exact shape measured on this box before the four variables were
mandated — the run prompt records a bot-pushed test commit coming back from the
API as `author: JeffMcClintock, committer: JeffMcClintock`. Checking only the
author would have let half of it through. The check tests both identities.

**Learned — why the check belongs *before* the push and nowhere else.** A
foreign commit that has been pushed cannot be rewritten; the branch is shared
and the run prompt forbids rewriting pushed history. So the only useful moment
is the last one at which the run still owns its history entirely. That also
shapes the failure output: it prints the remedy (`--amend --reset-author`, or a
`rebase --exec` for a range) **and** the two cases where the remedy must not be
applied — anything already pushed, and anything a concurrent session may be
building on. This morning the branches were unpushed, and that is the only
reason amending was safe. That condition is easy to lose sight of when the
content is obviously yours.

**Learned — the guard STEP 0.7 gives is narrower than it reads.** It is a
property of *the process*, asserted *once*, at the start. Nothing about it is
wrong; it simply cannot see a second writer, and a run that reads it as "this
repository is safe" has over-read it. The new STEP 4 text says this in as many
words, because the failure looked completely normal from inside — correct
content, clean exit codes, a passed STEP 0.7 — and the only visible sign was a
name in `git log` that nobody had a reason to read.

**Also added to STEP 3:** commit as soon as a coherent change exists rather
than staging and going away to build. That window was open about ten minutes
this morning. It is a smaller, softer mitigation than the assertion and does
not replace it — it just makes the assertion fire less often.

**Next:** nothing blocking. **A15** (five lines of `lint.yml` for A10's check)
and this row's own PR both need Jeff. On the carve-out, **C12c** is the win
NEXT item and is the largest dangling-edge reduction of any sub-stage — but it
must wait for C12b to merge, and for the second agent to be clear of `SE16` and
`SynthEditLib`.

**One process note for whoever reads this file next.** Four entries were
written today from one box, which is not the cadence this journal was designed
around. It happened because Jeff was directing interactively and told the run
to keep going; a scheduled run still takes exactly one item. Rotation kept the
file at four entries throughout, so **three of today's four entries are already
in [JOURNAL-2026-08.md](JOURNAL-2026-08.md)** by the time anyone reads this —
look there before concluding a day is missing.

**Branch/PR:** [TideSynth#63](https://github.com/JeffMcClintock/TideSynth/pull/63),
stacked on the A10 branch, which is stacked on C12b. All three retarget to
`main` as their parents merge.

---

## 2026-08-15 — windows — A10 (script half; A15 filed for the gated half)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Built **A10**, the bare-ID cross-reference lint A3 deferred:
[scripts/check-id-refs.py](scripts/check-id-refs.py). **Third item this
session, at Jeff's explicit direction in an interactive session** — and picked
because he confirmed a second agent was still writing to `SE16` and
`SynthEditLib`, so this run stayed inside TideSynth. **The workflow step that
turns the script into an actual gate is not here** — `.github/workflows/**` is
structurally unpushable by the bot token — so that is filed as **A15** with the
exact YAML, per STEP 5's "do the allowed-side part, file the gated part naming
the exact file".

**Result.**

| check | result |
|---|---|
| against the tree as it stands | **447 references checked, 97 rows, 91 distinct IDs, zero flagged** |
| built-in selftest | **20 cases, 0 failed** |
| deliberately stale `**Z9**` | detected, **exit 1** |
| deliberately stale `BLOCKED(C99)` | detected, exit 1 |
| deliberately stale `superseded by P42` | detected, exit 1 |
| clean tree / selftest exit codes | 0 and 0 |
| `scripts/check-links.py` after the change | 208 links, no breakage |

**The row said to budget this as a design session first, and that was right —
the implementation is 40 lines and the design is the whole item.** A10 predicted
the failure mode (a naive `\b[A-Z]\d+[a-z]?\b` false-positives constantly, and a
noisy lint erodes trust in the other four checks) but not the fix. **The fix is
two shape rules, both read off the ID column rather than guessed:**

- **A real ID has exactly one uppercase letter.** All 96 rows do — A, B, C, D,
  E, G, H, L, M, N, P, R, S, U, V, W, X. This single rule kills **`SE16`, which
  occurs 331 times in these docs** and is by far the worst offender, plus
  `SE15` and `SE14`.
- **One or two digits.** The longest real ID is `C12f`/`S10`/`A13`; MSVC
  diagnostics are four. This kills `C1083` (8 occurrences), `C2664` and
  `C4834` — all of which appear in journal entries today.

Everything the row worried about is gone before context is even considered.
Context then does the remaining work: only **bold**, `BLOCKED(...)`, or an
explicit trigger phrase (`see`, `blocked on`, `filed as`, `unblocks`,
`supersedes`, …) counts as a reference, so the row's own example — `P7a` inside
a sentence about `checkSizeConstraint(0,0,2178,32672)` — is never examined, and
neither is a bare `V1`. Fenced blocks, inline code spans and link targets are
skipped.

**Learned — `BLOCKED(<id>)` is the highest-value case and A10's row never named
it.** The row is written entirely about prose mentions. But `BLOCKED(<id>)` is
the one cross-reference this process treats as load-bearing: the backlog says
outright that **eligibility lives in the status column alone** and that prose
never overrides it. A typo there does not read as wrong — `BLOCKED(C12g)` would
sit in the table looking exactly like a valid blocker and would never clear,
because no row will ever be `C12g`. It is 8 of the 447 references and the only
ones where being wrong silently changes what a run is allowed to do. Worth
saying because it inverts the row's own priority: the prose half is hygiene, the
`BLOCKED()` half is correctness.

**Learned — the check has real coverage, which was not obvious in advance.** A
lint that fires on nothing is indistinguishable from one that examines nothing,
so the number to record is not "zero stale" but **447 references examined across
91 distinct IDs**, the most-referenced being C6 (15), then C12, C9, S1b and B1
(11 each). It is not decorative.

**Learned — stacked PRs are a genuine false-positive source, and this run hit it
within minutes.** The C12b PR adds row **A14**; this run's journal entry
references it. Branched from `origin/main`, the reference is dangling — and the
check is *right*, because on that branch the row really does not exist. Two
things came out of it: `--allow-id` as the documented escape, and this branch
being **stacked on `tide/win/C12b-controls` rather than `origin/main`**, which
was the better fix and also avoids a certain `JOURNAL.md` conflict between two
open PRs that both prepend an entry. **Whoever reviews: this PR's base is the
C12b branch and GitHub will retarget it to `main` when C12b merges.**

**Next:** **A15** — five lines in `.github/workflows/lint.yml`, Jeff's to push:

```yaml
      - name: ID cross-references
        id: idrefs
        continue-on-error: true
        run: python3 scripts/check-id-refs.py
```

plus `ID_REFS: ${{ steps.idrefs.outcome }}` in the Summary step's `env` and
`"$ID_REFS"` in its `for` loop. **Both halves matter** — every step is
`continue-on-error`, so a step added without the Summary wiring reports into the
void and fails nothing. That is the same shape as A4's finding that an allowlist
can look built while firing on nothing.

**STEP 1 / 1.5:** unchanged from earlier today — no `platform:win` issues; the
open PRs are this session's own (SynthEdit#19, SynthEditLib#8, TideSynth#61).

**Side effects on this box:** none outside the scratchpad. This run committed in
TideSynth only, and did not touch `SE16` or `SynthEditLib` at all — both were
left on their default branches, clean, before it started.

**Branch/PR:** [TideSynth#62](https://github.com/JeffMcClintock/TideSynth/pull/62),
based on `tide/win/C12b-controls`. No code outside `scripts/`.

