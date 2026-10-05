# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

**WHY THIS SECTION IS ABOVE THE ENTRIES AND MUST STAY THERE (A36, 2026-09-10).**
It used to sit *below* the oldest entry, and on 2026-09-01 a rotation swept it
into the archive along with the entries it was standing behind — because a
rotation removes the oldest entries and the oldest entries are at the bottom.
**The instruction that says "do this every run" stopped being read every run,
and the file grew from 53 KB to 229 KB across the fourteen entries that
followed, written by runs that could no longer see this rule.** Nothing caught it: `check-journal-prepend.py` deliberately does
not gate the header block, and the rotation was legal in every other respect.
Above the first dated entry a rotation cannot reach this — which is also where
the run prompt ("the template at the top of that file"), that script's own
docstring ("the header/template block above the first real entry") and A8's own
row ("the live file holding the template + current month") have all said it was.

**One more thing to check before you rotate**, learned the same day: rotation
opens a NEW archive file whenever it crosses a month, and
`scripts/extract-lessons.py` must be able to SEE that file or every lesson you
move into it drops out of [docs/lessons.md](docs/lessons.md) silently — the
exact failure A30 exists to prevent. It discovers `JOURNAL-YYYY-MM.md` by glob
now, so this should stay true on its own; `python3 scripts/extract-lessons.py
--check` before and after a rotation is the one command that proves it did.

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

**Archives:** [JOURNAL-2026-10.md](JOURNAL-2026-10.md), [JOURNAL-2026-09.md](JOURNAL-2026-09.md), [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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
## 2026-10-06 — windows — E76: the Accept's free branch had been takeable for 36 days, and the fix is a guard that tested existence where a zero-length file exists (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.19675.0** (CLI `2.1.286`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E76** and met the second branch of its Accept, which three previous runs had ruled ineligible. Filed **E90** for the ruling its other branch needs. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded.

### STEP 1 / 1.5 / 2

**STEP 1:** no open `platform:win` issue, which on this platform still verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing. The two open issues are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux) and [#44](https://github.com/JeffMcClintock/TideSynth/issues/44), the digest.

**STEP 1.5:** this lane has two open PRs and both are `MERGEABLE`/`CLEAN` with no reviews and no unresolved review comments — [#634](https://github.com/JeffMcClintock/TideSynth/pull/634) (E86's escalation, 13 pass + 2 skipping) and [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) (A41, 13 pass + 2 skipping, carrying one `tide-rack-bot` comment that is a finding rather than a request). STEP 1.5 says a green PR with nothing unresolved is waiting for merge and not a run's to fix, so I left both alone. **#634 has NOT merged**, so E86 stays parked on its own ruling exactly as the `win` cell said it would be. The fleet's other two open PRs are mac's: [#631](https://github.com/JeffMcClintock/TideSynth/pull/631) (E85) and [#633](https://github.com/JeffMcClintock/TideSynth/pull/633) (E88).

**STEP 2, walked in file order.** A35 is parked on its own two open `PROPOSED:` entries — I read both, and note their *"May proceed meanwhile: everything"* lines do not rescue A35 itself, because A35's deliverable IS the exception those entries decide. A37 is overlapped by the bookkeeping `PROPOSED:` entry, whose options (b) and (c) move the NEXT block, which is all A37 builds. **A42 is ineligible by its own words** (*"(b) without (a) is not identical under every answer"*). S8 carries `NEEDS-SPEC` as the first thing in its row. E2 is an umbrella that says of itself *"as one item it is not takeable"*. E19 and E82 both need a screen. X2 and E89 are linux. E84 is a `.github/workflows/**` edit this credential cannot make. E85 and E88 are mac's, with open PRs. E86 is this lane's, parked on #634.

### The finding that matters most this run is about reading a row, not about the code

**The 10-03 and 10-05 cells both ruled E76 ineligible, and both were wrong — in the same way.** Their stated reason was that *"choosing the second branch presumes the answer to the row's own open question."* The row says the opposite. Its words are: *"this is either a documented wrapper (done, in the harness doc) or a linux arm inside `render()` that scrubs the two variables — **the second** needs a ruling on whether a measurement script may edit the caller's environment."* The ruling attaches to the **code** option. The documented-wrapper option is the one the row calls already done.

So **E76's Accept had a free branch for 36 days and three runs walked past it**, each inheriting a one-line verdict that reading the row disproves. The generalisable form: **when a row says something "needs a ruling", check WHICH of its options the ruling is attached to — a fork can have one branch free.** This is the clearest justification I have seen for STEP 2's instruction to re-check every reason rather than inherit it, and it is worth noting that the instruction worked: the only reason I found this is that the `win` cell told me to re-derive the verdicts.

### E76, and the gap was not the one the row names

The row frames this as a missing wrapper. **The wrapper is not missing — it is documented where the operator never looks.** `docs/ci/headless-gui-verification.md:200-205` carries the whole recipe. `scripts/render-and-measure.py` carried nothing: `grep -i "wayland\|GDK\|linux"` on `origin/main`'s copy returns three unrelated hits. And the script already passes `__doc__` to argparse through `RawDescriptionHelpFormatter`, so the docstring reaches `--help` for free — which is what makes a docstring the right place rather than a comment.

**Checked rather than eyeballed:** the six environment names in the new docstring block (`WAYLAND_DISPLAY`, `DISPLAY`, `GDK_BACKEND`, `XDG_RUNTIME_DIR`, `HOME`, `REAPER`) are set-wise identical to those in the harness doc's block.

**Then the other half, which the row states as a symptom and which turned out to have a one-word cause.** The row says the downstream symptom is *"an `EOFError` out of Python's `wave` module on a zero-length render, which reads as a corrupt fixture and is not one."* Reproduced here against `origin/main`'s copy:

```
zero-length size: 0
RAISED: EOFError ''        <- wave.py:117
```

**The message is the empty string.** That is the whole of why it reads as a corrupt fixture: there is nothing in it to read.

**And `main()` had a guard that should have caught it: `if not os.path.exists(out)`. A zero-length file exists.** So the one artefact a dead REAPER actually leaves behind went straight past the branch that prints the REAPER log tail — which is exactly where the `gdk_screen_get_root_window` assertions are — and into `analyse()`. The operator got a traceback with nothing in it instead of the log that names the cause.

Both halves are now fixed and **neither touches the caller's environment**, which is what keeps this identical under every answer to E90:

- `main()` tests size as well as existence, prints the log tail, and names the cause.
- `analyse()` turns `EOFError`/`wave.Error` into a diagnostic naming REAPER rather than a traceback.

### Verification

`python3 tests/e76_render_diagnostic_probe.py` — **8 arms, rc=0**:

| arm | holds |
|---|---|
| 1, 2 | zero-length and truncated renders are diagnosed, not raised |
| 3 | a real -6 dBFS render still measures **-6.02** |
| 4 | **digital silence still reports SILENCE** (peak `-inf`, `silent=True`) |
| 5 | end to end through a REAPER stub that exits 1 leaving a zero-length file: **rc=1, cause named, `Traceback` absent** |
| 6a-c | **vacuity controls at pinned `e7fba108d`, which MUST fail**: `EOFError('')`, `Error('fmt chunk and/or data chunk missing')`, and a **traceback** from its end-to-end run |

**Arm 4 is the discriminator and is the reason the probe is eight arms rather than three.** A genuinely silent render is the finding this script exists to report. A guard that called silence "unusable" would pass arms 1, 2 and 5 while destroying the script's entire purpose, and nothing else in the probe would have noticed.

**Arm 6 is pinned to a commit, not to `origin/main` — and I changed that during the run, having written it the wrong way first.** Against a moving ref the control goes **red the moment this fix merges**, because `origin/main` would then diagnose too: a probe that self-destructs on merge. That is A41's finding arriving in a new probe four days after A41 measured it. `--base <rev>` overrides the pin, and an unresolvable base is **rc=2** (input unresolvable) rather than a silent pass — the four-exit-code convention `tests/a38_fleet_state.py` settled on.

**REAPER is installed on this box**, which I did not expect and which is worth recording for the lane: `C:/Program Files/REAPER (x64)/reaper.exe`. So `--control` is a **real** A/B rather than a synthetic one — `origin/main`'s script and this one, same machine, same REAPER:

```
  control (known -6 dBFS 1 kHz sine)
    peak=   -6.0 dBFS  rms=   -9.0 dBFS  -> AUDIO PRESENT
  PASS -- the render-and-measure chain does detect audio
```

`diff` of the two runs' output is **empty**, both rc=0. The measurement path is untouched by a real render, not only by fabricated wavs.

### Two traps hit on the way, both about writing files rather than about E76

**A quoted bash heredoc collapsed `\\` to `\` in this box's shell, twice, and the second time it corrupted a docstring silently.** `return "\\n".join(msg)` reached the file as a string literal broken across two real lines — caught immediately by `SyntaxError`. But the same collapse inside the docstring's shell-continuation lines produced **valid Python that renders wrong**: a lone `\` before a newline is a line continuation *inside the string*, so `--help` printed the four-line wrapper as one 200-character line with the indentation flattened into spaces. **The file looked right in the diff and was wrong in the output**, and the only thing that caught it was running `--help` and reading it. Fixed with the Edit tool rather than a heredoc. The standing note that *"Bash heredocs eat backslashes"* on this box is right, and the sharp corner is that the damage is sometimes syntactically legal.

**`check-links.py` correctly rejects a markdown link to a file that lands in a different PR.** The E76 row cited `tests/e76_render_diagnostic_probe.py` as a link; the probe is on the code branch, so on the bookkeeping branch it does not exist — `BACKLOG.md:113 (no such file)`, rc=1. Backticks plus the PR number instead. **This is a structural consequence of the lane's documents-in-one-PR/code-in-another shape** and will recur on every split: the bookkeeping PR may *name* a new file but must not *link* it until the code PR merges.

### Bookkeeping

- **Two PRs, per the lane's measured shape.** `scripts/` and `tests/` are not on the auto-merge allowlist, so [#636](https://github.com/JeffMcClintock/TideSynth/pull/636) is code-only and waits for a human — with `BACKLOG.md` **byte-identical to `origin/main`** there, the 10-01 zero-diff resting state, so it cannot re-conflict as `main` moves. The DOING mark was pushed first (`6632beeae`) and removed at the end, so the claim was visible for the whole of the work.
- **E90 filed rather than folded into E76**, because E76 is now IN-REVIEW and a question left inside a closing row is archived with it. That is **A42's shape, four days after A42 was filed for exactly this**. The id was allocated by A42's own guard: highest `E` id is **89** on `main` and on every one of the five remote `tide/*` branches, so E90 was free.
- **STEP 3's grep before filing:** three rows name `render-and-measure.py` — E19 (as an instrument), E29 (`WONTFIX`) and E76 itself. No existing row owns this job.
- Lints on the bookkeeping branch: `check-id-refs` rc=0, `check-next-block` rc=0 (*1 take-target across 4 NEXT rows, every one live*), `check-links` rc=0.

### After the push: seven "failures" on these two PRs were all CANCELLATIONS, and the cause is duplicate runs

Worth knowing before the next run debugs its own diff, because `gh pr checks` prints a
**cancelled** job as **`fail`** and nothing in that output says which it was.

**Every one of the seven had NO RUNNER ASSIGNED** (`runner_name` empty) while sibling jobs on
the same run got runners and succeeded. They spanned three workflows (`build`, `lint`,
`verify`) on both PRs -- including [#637](https://github.com/JeffMcClintock/TideSynth/pull/637),
which touches **four markdown files and nothing else**, so content cannot be the cause.

**The mechanism is visible in the job lists: there are TWO `build` runs at one sha.** At
`431f45d2d`, run `37369556946` has `render-linux` and `render-macos` **succeeded**; duplicate
run `37369618746`, same sha, has those same two **cancelled**. So the work passed in one run
and was cancelled in its twin -- a concurrency group with `cancel-in-progress` racing two runs
of the same workflow, not a test result.

| | reported by `gh pr checks` | actual `conclusion` | runner |
|---|---|---|---|
| the seven | `fail` | `cancelled` | **none assigned** |
| their siblings, same runs | `pass` | `success` | assigned |

**The one-command tell**, because the check name and the bucket both mislead here:

    gh api repos/JeffMcClintock/TideSynth/actions/runs/<id>/jobs \
      --jq '.jobs[]|"\(.name) \(.conclusion) runner=[\(.runner_name // "")]"'

`conclusion == "cancelled"` with an empty runner is **never** a finding about the diff: the job
never started. `gh run rerun <id> --failed` clears them.

**RE-RUN EVERY DUPLICATE, NOT JUST THE NEWEST -- this is the part that cost an extra cycle.**
Nine cancelled jobs across four runs in the end. I re-ran `37369010701` on #636 and a *second*
pair of cancellations surfaced minutes later, which looked like the problem escalating and was
not: they came from `37368962011`, the un-rerun TWIN `build` run at the same sha, which keeps
reporting its own cancelled jobs into the PR's check list whatever you do to its sibling. Count
the runs per workflow before concluding anything -- `gh pr checks` flattens them, so two runs
of `build` appear as one set of names and a stale twin is invisible in that view.

**A re-run is the right response to a cancellation and the wrong response to a failure.** If a
job cancels again after every duplicate has been re-run, that is infrastructure for Jeff, not
something a run can fix by retrying.

**Learned:**

- **"Needs a ruling" attaches to an OPTION, not to a row, and a fork can have one branch free.** E76's Accept was an `or`; the ruling sat on one side of it. Three runs read the row's *"which is why this is filed rather than done"* as covering the whole row and inherited each other's verdict. The check costs one careful read of the sentence that names the ruling.
- **An existence check is not a non-emptiness check, and the difference is exactly the case a crashed process produces.** `os.path.exists` on a file a dying writer created and never filled is `True`. The guard that would have printed the diagnosing log tail was already there and was skipped by the only failure it was written for.
- **`wave` raises `EOFError` whose `str()` is empty.** An exception with no message is worse than a wrong message: there is nothing for the operator to search for, which is how it came to be read as a corrupt fixture for three days on another box.
- **A vacuity control must be pinned to a commit, or it inverts when the fix lands.** Mine read `origin/main` in its first draft and would have gone red on merge. The general rule: a control that asserts *"the baseline does NOT do X"* is a statement about a specific tree, and naming a branch instead of a commit makes it a statement about whenever it happens to run.
- **A docstring can be corrupted into something syntactically valid and semantically wrong, and only the rendered output shows it.** Reading the diff was not enough; `--help` was.
- **A cancelled CI job is reported as `fail`, and the difference is one API field.** Seven
  cancellations across both PRs, all with no runner assigned, one of them a job that
  SUCCEEDED in its duplicate run at the same sha. **Read `conclusion` and `runner_name`
  before reading your own diff** -- a job that never started cannot have been broken by it.
- **The bookkeeping/code PR split forbids markdown links across the seam.** Name a cross-PR file in backticks; `check-links` is right to reject the link, and it will reject it on every future split.

**Not verified:** **no committed fixture was rendered.** The developer was at this machine throughout and a fixture render loads the TIDE plug-in, which can raise the modal E29 documents — bounded by the script's 300 s timeout, but on his screen meanwhile. `--control` exercises the full REAPER round trip without a plug-in, so it was the right control to run and the wrong one to generalise from: **it says nothing about whether any fixture still measures at its reference figures**, only that the chain and the measurement arithmetic are unchanged. **Nothing was measured on linux** — the docstring's wrapper is transcribed from the harness doc's own 2026-08-31 linux measurement and checked for agreement with that doc, not re-derived against a running REAPER, so E76's **first** Accept branch remains unmet and unmeasured. No build of any product target, and no C++ compiled. I did not judge E19's or E82's Accepts beyond confirming each needs a screen. A40's `NEEDS-JEFF` half is still unanswered.

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and **never left it** — all work in two scratchpad `git worktree`s, both removed at the end. **Seven of the eight repos were clean at the start and I touched none of them:** `SE16` (`master`), `SynthEditLib`, `GMPI`, `GMPI_Wrappers`, `SynthEdit_Rack_Adaptor`, `VCV_Fundamental_gmpi` (all `main`). `gmpi_ui` (`main`) carried **one untracked directory, `examples/AluminiumDemo/`** — the developer's work in progress, predating this run, so left exactly alone per STEP 5's third kind. **The developer was at the machine throughout** (Visual Studio on `ToneMaster`/`HeaderPanel.h`, Outlook, Slack), so **no GUI work and no screen taken**; the one REAPER invocation is `-renderproject`, which renders and exits without a window. `git fetch` again warned *"too many unreachable loose objects"* in `C:\SE\TideSynth` — a local housekeeping note for Jeff (`git gc`), not a repository problem, and not mine to run on his tree. No credential value appears in any commit, PR, journal entry or row.

**Next:** see the `win` NEXT cell. **For Jeff, three things:** (1) [#634](https://github.com/JeffMcClintock/TideSynth/pull/634) is E86's fork and **cannot auto-merge by design** — merging it is the ruling. (2) [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) (A41, green since 10-01) and [#636](https://github.com/JeffMcClintock/TideSynth/pull/636) (E76) are both code-only and so allowlist-blocked by design, not stuck. (3) **E90 is a new ruling question** — may a measurement script edit the caller's environment — and it wants a `PROPOSED:` entry before anyone writes the linux arm.

**Branch/PR:** E76's code is on `tide/win/E76-render-wrapper-docstring`, [#636](https://github.com/JeffMcClintock/TideSynth/pull/636) — `BACKLOG.md` byte-identical to `main` there, so it conflicts with nothing. This entry, the E76 flip, the E90 row and the refreshed `win` cell are on `tide/win/2026-10-06-e76-bookkeeping`, which is bookkeeping-only and should auto-merge.

## 2026-10-05 — windows — E86: the processor-only VST3 finding reproduces, and its "unknown to fix" is now ONE MEASURED LINE; the fork escalated as a PROPOSED entry (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.19675.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E86**, re-measured its finding on today's `main`, measured the cost of the option its row calls *"unknown to fix"*, and escalated the fork as a `PROPOSED:` entry. Archived **E87**, whose [#632](https://github.com/JeffMcClintock/TideSynth/pull/632) merged 2026-10-02. Regenerated the stale `docs/lessons.md` and rotated `JOURNAL.md`. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`d58bdd138..248901335`).

### STEP 1 / 1.5 / 2

**STEP 1:** no open `platform:win` issue, which on this platform still verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing. The two open issues are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux) and [#44](https://github.com/JeffMcClintock/TideSynth/issues/44), the digest. **This run has something better than CI for once: `main` builds on Windows, measured** — see below.

**STEP 1.5:** this lane's only open PR is [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) (A41), `MERGEABLE`/`CLEAN`, 13 pass + 2 skipping, no reviews and no unresolved review comments. STEP 1.5 says a green PR with nothing unresolved is waiting for merge and not a run's to fix, so I left it alone. The fleet's other two open PRs are mac's: [#631](https://github.com/JeffMcClintock/TideSynth/pull/631) (E85) and [#633](https://github.com/JeffMcClintock/TideSynth/pull/633) (E88), both `MERGEABLE`/`CLEAN`.

**STEP 2, walked in file order, every reason re-checked rather than inherited:**

- **A35** is parked on its own two open `PROPOSED:` entries, which is what its row says and what reading them confirms. **A37** is overlapped by the bookkeeping `PROPOSED:` entry: its options (b) and (c) move the NEXT block into per-platform sections or `docs/next/<platform>.md`, and *where the NEXT cells live* is the whole of what A37 builds — so that entry's *"May proceed meanwhile: EVERYTHING"* does not reach it, for the reason the 10-01 cell gives. I reached the same conclusion from the entry itself.
- **A42 is ineligible by its own words**, which is the cleanest case in the queue: *"(b) alone is buildable by a run, but a script nobody is told to run guards nothing, so (b) without (a) is not identical under every answer."*
- **S8** is `NEEDS-SPEC`. **E2** is an umbrella with no statable Accept. **E84** is a `.github/workflows/**` edit this credential deliberately cannot make. **X2** and **E89** are linux.
- **E76 I read rather than inheriting.** Its `Plat` is `any`, but its Accept is a fork — `render-and-measure.py` returning `-6.3/-17.0` **on linux**, *or* a docstring line saying a wrapper is required. Choosing the second branch presumes the answer to the row's own open question (may a measurement script edit the caller's environment), so it is not identical under every answer.
- **E19 and E82 are the two X2-shape rows the `win` cell flagged, and I checked them rather than assuming.** Both are correctly `TODO`: each row already states its own remaining work, written by the run that measured it. E19 owes a **pixel-diff** clause and an **int/bool/enum** clause; E82 owes the right-click on **WT LFO**'s panel. **Those two are the same measurement**, and they are the only rows left in this queue that need a screen. Nothing to flip.
- **E85 and E88 are mac's**, with open PRs naming them. **E87** was `IN-REVIEW` with its PR merged — a STEP 4 chore, not an item.
- **E86 was next.** Its row says *"read it before taking it"*, so I did, and took it.

### E86, re-measured — and the row's finding reproduces, silently

The instrument is the row's own: `tests/e80_vst3_feedback_probe.cpp`, win32-only, which is why this lane is the only one that can run it. Built from `origin/main` with **no `*_FOLDER_OVERRIDE`**, so all eight dependencies are their own `main` — configure rc=0, build rc=0, **zero `error C`/`error LNK` lines**, both on the first pass and on a second (Defender's cold-tree trap did not fire this time). The probe compiles with `cl` against the headers-only VST3 SDK. Same `TIDE-Rack.vst3`, same 43,247-byte document, same `TIDE: rack built for 44100 Hz, block 512`, 400 blocks at 44.1 kHz, `TIDE_FEEDBACK_TRACE_EVERY=1`:

| arm | modules constructed | display-state captures | feedback sends | max send |
|---|---|---|---|---|
| with controller (what a DAW does) | **5** — `LFO`, `LFO2`, `Pulses`, `SHASR`, `Scope` | 4 | 287 | 65,798 B |
| `--no-controller`, `main` as it stands | **0** | 0 | 11 | 37 B |
| `--no-controller` + one `registerDeferredModules()` call | **5** — the same five, by name | 4 | 287 | 65,823 B |

**Every probe check passed in all three arms and no arm logged an error.** That is the finding rather than a caveat.

**Row 3 is the part the row called *"unknown to fix"*, and it is one line.** `rack_adaptor::registerDeferredModules()`, called from `SynthEdit::open()` (`SynthEditSem/SynthEdit.cpp:376`). **The scratch patch was reverted and `SynthEdit.cpp` is byte-identical to `main` on both of this run's branches** — I did not land it, because which fork to take is a ruling.

**The control that says that call is SUFFICIENT and not merely necessary.** Normalising the volatile counters out of the plug-in's own stderr and diffing the arms, the patched `--no-controller` run is identical to the with-controller run on **every rack and DSP line**. The only lines it still lacks are controller-side by nature: the seven enrichment XMLs (`ControlsXp.xml enriched 4 of 18 described class(es)` and six more), `7 rack prefab(s) seeded from the bundle`, and TideApp's own document state. Distinct line shapes: **44** with a controller, **33** with the one call, **5** on `main`'s `--no-controller`.

**What the control cannot settle, stated because the measurement cannot:** this fixture's DSP needed neither the enrichment XMLs nor the prefabs, so it says nothing about a document that would. What would settle it is a fixture using a class whose pins come ONLY from one of the seven XMLs — and `SE MIDI to CV 2`, the one SE module in this fixture, is described by none of them (checked against all seven).

### Two findings the row did not ask for

**(1) The guard that makes the call idempotent is not thread-safe, and the fix would add the second caller it has ever had.** `registerDeferredModules()` is guarded by a plain function-local `static bool done` — not an atomic, not a `call_once` (`SynthEdit_Rack_Adaptor/RackFactoryStatic.cpp:75-82`). Its comment says the guard sits there *"rather than in the host"* so that several instances in one process are safe, and that reasoning holds for several instances on **one thread**. Today the only caller is `TideApp::InitInstance()`, from the controller's `initialize()`.

**What makes the `open()` site safe on the thread that matters** — and this is SDK-documented rather than inferred: `open()` is reached from `Processor_VST3::setActive(true)` → `reInitialise()` → `gmpi_processor::start_processor()` → `processor->open(host)`, and the VST3 SDK annotates `IComponent::setActive` **`[UI-thread & Setup Done]`** (`pluginterfaces/vst/ivstcomponent.h:195`). The rack build is **not** on that thread: `rack.prepareToPlay()` runs synchronously on the audio thread from `onSetPins` (`SynthEditSem/SynthEdit.cpp:528`, whose own comment says so) and again from `subProcess` (`:598`). So registering in `open()` lands strictly before any audio-thread lookup, while registering from the rack-build path would put a process-global database write on the audio thread.

**That guard is in `SynthEdit_Rack_Adaptor`, which is on NEITHER the run prompt's ALLOWED nor its GATED list** and is therefore GATED by default — as is `VCV_Fundamental_gmpi`. This is the **G3** shape exactly, so it is filed inside the `PROPOSED:` entry rather than reached into.

**(2) A second defect, on `main` today and independent of E86.** The guard returns **0** on every call after the first, and both callers print that number as a count. Measured directly: the two processor objects the probe creates printed `-> 39 module(s)` then `-> 0 module(s)`. `TideApp.cpp:855` prints the same number the same way (`TIDE: %s — %d module(s) registered`), so **a host that creates a second TIDE instance in one process already logs `0 module(s) registered`** — which reads as a failure and is a success. A TIDE-side one-liner under any answer; recorded here rather than filed as a row, because it is the same call site the entry is about.

### Bookkeeping

- **E87 archived.** [#632](https://github.com/JeffMcClintock/TideSynth/pull/632) merged 2026-10-02, checked individually rather than inferred. Flip and move are **one edit**, per the 10-01 cell's trap, and `check-backlog-archived.py` is rc=0. A41 stays `IN-REVIEW`: [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) is still open.
- **Archiving E87 did NOT create a stale take-target**, which is the trap the 10-01 cell warned about. The previous `win` cell's *"THEN READ E85, E86, E87 AND E88"* is now chain history, and the defusing negation is in the same sentence as the verb in my own cell (*"…is archived by this cell, so it is not a take-target any more"*). `check-next-block.py`: **2 take-targets across 4 NEXT rows, every one live, rc=0.**
- **`docs/lessons.md` was stale again** — `extract-lessons.py --check` said *"stale -- run --write"* on `origin/main`, so the 10-03 cell left it owed. Regenerated; figures below.
- **Two PRs, because the lane's measured shape demands it.** `docs/decisions.md` is **deliberately carved out of the auto-merge allowlist** — `automerge_eligible.py` says *"allowlisted by location but carved out (merging it is a decision, not bookkeeping)"*, rc=1 — so putting the escalation in the bookkeeping PR would park this entry and the E87 archive behind a human indefinitely. The escalation branch carries **`docs/decisions.md` and nothing else**, with `BACKLOG.md` byte-identical to `origin/main` so it cannot re-conflict as `main` moves (the 10-01 zero-diff resting state).
- **The A42 guard was not needed:** this run allocated no new id. A `PROPOSED:` entry is keyed to its row.

**Learned:**

- **"Unknown to fix" is sometimes one line, and the cheap way to find out is to patch, measure and revert.** E86 sat `TODO` for 24 days with `Size` reading *"small to answer, unknown to fix"*. A scratch patch plus an incremental rebuild — about two minutes on a warm tree — turned that into a measured three-row A/B. **Measuring an option is not taking it**, and it is what makes an escalation decisive rather than speculative; the A38 entry's probe is the precedent.
- **The strongest evidence for a fix being sufficient was a DIFF OF THE STDERR, not a count.** Equal module counts prove the fix works; the normalised line-shape diff (44 / 33 / 5) proves what is still missing and that none of it mattered here. It is also what let me state the limit honestly — the fixture cannot discriminate the enrichment-XML half, and no count would have shown that.
- **An idempotence guard is a thread-safety claim in disguise, and adding a caller is what tests it.** The comment on `registerDeferredModules()` is careful and correct about *several instances in one process*; it is silent about *two threads*, because until this measurement there was only ever one caller. **The moment a fix adds a second call site, re-read the guard rather than its comment.**
- **An idempotent function that returns a COUNT lies to its second caller**, and both of TIDE's callers print it. This was visible only because the patched arm printed the line twice in one run — `39` then `0`. A guard that returns 0 for "already done", formatted as `%d module(s) registered`, is a success that logs like a failure.
- **`docs/decisions.md` being denied on the auto-merge allowlist makes the split MANDATORY, not stylistic.** The lane's standing shape is *documents in one PR, code in another*; the sharper rule is **anything that cannot auto-merge goes in its own PR**, and an escalation is the clearest case there is, because the whole point is that a human merges it.
- **The invisible-HWND arm has a hard boundary, and E19's remaining clause is on the wrong side of it.** An unshown window gets no `WM_PAINT`, so a pixel-diff cannot use it. Worth knowing before the next run reaches for the trick to clear E19: its int/bool/enum clause might go that way, its pixel diff cannot, and the box was not idle today either.

**Not verified:** I launched no DAW and took no screen. The `main`-builds-on-Windows claim is this run's own build of a tree that is `main` plus one `BACKLOG.md` status cell — real, but not a CI run. I did not judge E19's or E82's Accepts beyond confirming each row already states its remaining work; I measured neither, because both need a screen. The enrichment-XML and prefab halves of the processor-only gap are **unmeasured**, as the control section says. Audio was silent (`-1000.0 dBFS`) in all three arms, including with a controller, so this fixture proves nothing about audio either way. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half, still open).

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and **never left it** — all work in two scratchpad `git worktree`s, both removed at the end, with the build tree also in the scratchpad. **All eight repos were clean at the start** and I touched none of the other seven: `SE16` (`master`), `SynthEditLib`, `gmpi_ui`, `GMPI`, `SynthEdit_Rack_Adaptor`, `VCV_Fundamental_gmpi` (all `main`) and `GMPI_Wrappers` (`main`, behind 1). **The developer was at the machine throughout** — Visual Studio on `ToneMaster`, the ToneMaster app running, Paint.NET, Slack, Settings — so no GUI work and no screen taken, which E86 needed none of. `git fetch` again warned *"too many unreachable loose objects"* in `C:\SE\TideSynth`; a local housekeeping note for Jeff (`git gc`), not a repository problem, and not mine to run on his tree. No credential value appears in any commit, PR, journal entry or row.

**Next:** see the `win` NEXT cell. **For Jeff, two things:** (1) **[#634](https://github.com/JeffMcClintock/TideSynth/pull/634) is E86's fork and cannot auto-merge by design** — four options, with (c) recommended and (a) measured; merging it is the ruling. (2) [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) (A41) has been green and waiting since 10-01, and A40's one question — does the fleet PAT expire at all — is still answerable only in the GitHub UI.

**Branch/PR:** the `PROPOSED:` entry is on `tide/win/E86-vst3-processor-factory`, [#634](https://github.com/JeffMcClintock/TideSynth/pull/634) (`docs/decisions.md` only). This entry, the E86 row, the E87 flip and archive, the rotation, the regenerated `docs/lessons.md` and the refreshed `win` cell are on `tide/win/2026-10-05-e86-bookkeeping`, which is bookkeeping-only and should auto-merge.

## 2026-10-03 — macos — E87: its Accept is met on `main` -- the second merger renumbered, `Plat` untouched; the general form filed as A42 (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.19675.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E87**, measured its Accept, and found it already met; flipped it to IN-REVIEW with the evidence. Filed **A42** for the question E87 carried but did not need to answer. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded.

### STEP 1 / 1.5 / 2

STEP 1: no open `platform:mac` issue (open issues are #583, linux, and #44, the digest). STEP 1.5: this lane's only open PR is [#631](https://github.com/JeffMcClintock/TideSynth/pull/631) (E85, from the 10-02 run whose entry is on that branch, not yet on `main`), plus its companion [GMPI_Wrappers#41](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/41). #631 is `MERGEABLE`/`CLEAN`, 13 SUCCESS + 2 SKIPPED, no reviews, no review comments; #41 is `OPEN`/`MERGEABLE`. Both are waiting for merge, so I left them alone.

STEP 2, walked in file order, each reason checked rather than inherited:

- **A35** waits on its own two open `PROPOSED:` entries. **A37** I read against the bookkeeping `PROPOSED:` entry (*"Should the fleet's two bookkeeping hot spots stop being single shared files"*), whose options move the NEXT block, which is all of A37. I agree with the 10-01 windows reasoning: it is not identical under every answer. **S8** is `NEEDS-SPEC`.
- **E19**: the remaining mac cell is AU3 in a real host with an editor on screen. A scheduled run cannot take the screen. **E82**: same, its Accept is a right-click on a VCV panel. Both are also the X2-shape judgements the `win` NEXT cell reserves for the next win run.
- **X2** and **E89** are linux. **E2** is an umbrella. **E76** wants a ruling. **E84** is a workflow edit this credential cannot make. **E85** is this lane's open #631.
- **E86** I read before taking it, as its row asks. Its Accept is a fork: either the processor builds the factory, or Jeff rules that TIDE requires a same-process controller. Which one is a product and lifetime ruling, not a run's. The only bare-host instrument for it (`tests/e80_vst3_feedback_probe.cpp`) is win32-only (`:561`). **Recommendation:** E86 wants a `PROPOSED:` entry before anyone builds anything.
- **E87** was next, `any`, small, BACKLOG-only. No branch or PR named it. I claimed it on `tide/mac/E87-id-collision-verified` and pushed the claim first.

### E87, measured

**The second merger did exactly what the row prescribed.** `82a9c49` (`tide-rack-bot`, 2026-10-01 13:58 +1300) is *"Merge origin/main into E79: renumber this lane's E85/E86 to E88/E89, per E87"*. It was pushed to [#584](https://github.com/JeffMcClintock/TideSynth/pull/584)'s branch two minutes after [#586](https://github.com/JeffMcClintock/TideSynth/pull/586) landed (`98decfc`, 13:56) and merged as `be3930e`.

| check | result |
|---|---|
| linux rows at `82a9c49^1` vs **E88**/**E89** at `82a9c49`, id cell stripped | `diff` empty: Status, `Plat`, Item byte-identical |
| same rows, `82a9c49` vs `origin/main` | byte-identical |
| `python3 scripts/check-id-refs.py` on `origin/main` `d58bdd1` | *"no stale ID references, no duplicate IDs, no shared live citations"*, **rc=0** |
| `grep -c "^\| E8N \|"` in `BACKLOG.md` / `BACKLOG-DONE.md` | E85, E86, E88, E89: **1 / 0** each |
| **positive control**: same tree, E88/E89 id cells set back to E85/E86 | **rc=1**, *"2 DUPLICATE ID(s) -- one ID, more than one row: E85 BACKLOG.md:115, BACKLOG.md:118; E86 …"* |
| archived E79 row | cites **E88**, not the old id |

So `Plat` did not move (E89 is still `linux`), which was the trap E87 warned about. The only residue is that `JOURNAL-2026-09.md` names the linux findings by their old ids in two entries. That is append-only history, correctly left alone.

### Bookkeeping choices, and why

- **I did not touch the `mac` NEXT cell.** `main`'s cell is the 10-01 one; #631 carries a 10-02 cell on the same single line. Any edit of mine would conflict with #631 on that line. That is A38's livelock in miniature, and the 10-01 windows finding is that a zero diff in a contended cell is the durable resting state. Instead, the next mac run's instruction is here: **(1) STEP 1.5 on #631 / GMPI_Wrappers#41. (2) Walk STEP 2: E88 is the next `any` row below E87.**
- **The rotation is byte-identical to #631's.** Adding this entry put `JOURNAL.md` over 60 KB, and the oldest entry is the 09-29 windows one, which #631 also rotated. I took #631's `JOURNAL-2026-09.md` blob verbatim, so the two branches make the *same* change there and cannot conflict on it. `JOURNAL.md` will still conflict with #631 at the prepend point. Every pair of journal PRs does that, and whichever lands second resolves it.
- **I checked for id collisions before filing A42**, using the guard A42 describes: the highest A-id is A41 on `main`, on `tide/mac/E85-clap-gui-show` and on `tide/win/A41-probe-ref-pinning`, and A38 on `tide/mac/issue-599`.

**Learned:**

- **E87 resolved itself the way it said it should, through a run doing a re-sync, not through anyone taking E87.** The row's value was the instruction, which the merging run read and followed (its commit subject says *"per E87"*). That only works if the row is on `main` before the second merge. It was, because #584 had carried it.
- **`check-id-refs.py` passes on a renumbered id, but that does not mean every old mention is right.** It proves no id is *missing*. It cannot tell that a journal entry saying "E85" meant the linux finding now called E88. A renumber is safe for rows. For prose it is only as safe as the reader's willingness to look up the renumbering commit.
- **To check an Accept that reads "a lint is rc=0", break it back to the predicted failure first.** rc=0 on a check that cannot see the problem looks identical to rc=0 on a fixed tree. A39's lesson applies to a BACKLOG-only item too.

**Not verified:** no build, no host; nothing compiled was touched. I did not judge E19's or E82's Accepts (reserved for win). I did not answer E86.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean (behind `origin/main`); all work in a scratchpad `git worktree`, removed at the end. `SynthEdit` was on `master`, clean; I did not touch it or any other repo. No GUI work, no screen taken. No credential value appears anywhere.

**Next:** above, under *Bookkeeping choices*. **For Jeff:** #631 + GMPI_Wrappers#41 (E85) and #629 (A41) are green and waiting. A42's prompt-vs-script question is yours. The stale pushed branch `tide/mac/issue-599` (PR #604 closed unmerged) is still there, and deleting it is yours too.

**Branch/PR:** `tide/mac/E87-id-collision-verified`: the E87 flip, the A42 row, this entry, and the rotation.

## 2026-10-01 — windows — the queue reopened: all eleven PRs merged, A41 taken and measured, and the probes' verdict had THREE causes rather than one (scheduled run)

**Prompt:** b97bc00a5 · Opus 5, `claude-opus-5` · app Claude desktop **2.16120.0** (CLI `2.1.284`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **A41** and shipped option (a) — both A38 probes now take their fleet state as a pinned input. Then the STEP 4 bookkeeping the merge sweep made due: eight landed rows archived, and `JOURNAL.md` rotated for the first time since 2026-09-08. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded.

### The thing that changed everything about this run: Jeff merged the entire queue

`gh pr list --state open` is **empty**. Eleven PRs landed between 00:22 and 01:09 UTC — #622, #618, #585, #614, #589, #588, #597, #590, #587, #586, #584 — so **thirteen consecutive cells of "STEP 1.5 and nothing else" ended in one sweep**, on both this lane and mac's. The only remaining `tide/**` ref is `tide/mac/issue-599`, whose [#604](https://github.com/JeffMcClintock/TideSynth/pull/604) Jeff closed unmerged exactly as five mac cells asked.

STEP 1 was empty and, unusually, **verifiable**: `main` at `be3930ec3` is `success` on `windows`, `macos`, `linux`, all three `render-*`, `guard` and `digest`. The structural caveat is unchanged — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing, so an empty `gh issue list --label platform:win` verifies nothing on its own. STEP 1.5 had nothing to do: zero open PRs.

### STEP 2: A41, which the `win` NEXT cell already named and which `check-next-block` agreed was live

A35 is parked on its own two open `PROPOSED:` entries. **A37 I read rather than inherited, and ruled it ineligible on my own reasoning**: the open `PROPOSED:` entry *"Should the fleet's two bookkeeping hot spots stop being single shared files"* offers options (b) and (c) that move the NEXT block into per-platform sections or `docs/next/<platform>.md` — which is A37's entire scope. That entry's own *"May proceed meanwhile: EVERYTHING, without exception"* clause rests on the reasoning that the question is about *where a run writes* and not *what a row builds*; A37 is the one row where that does not hold, because what A37 builds is where the NEXT cells live. So STEP 2's "identical under every open answer" test fails for it.

### A41, and the row named one cause where there were three

The row's finding was that the probes' verdict flips on fleet movement. True, and two more defects were sitting underneath it. All three are now stated in `tests/a38_fleet_state.py`'s docstring, which is where a reader of the probes will be.

**1. The probes had TWO ref sources, and they can disagree.** `branches()` enumerated from `ls-remote --heads origin` — the remote, live — while every read resolved `origin/<branch>`, which is whatever the local clone last fetched. At **one instant, from one `origin/main` sha (`be3930e`), with the probe blob byte-identical**, `a38_row_adjacency_probe.py` arm 1 reported:

| repo | `BACKLOG.md` verdict |
|---|---|
| `C:\SE\TideSynth` (full clone, 14 stale `origin/tide/*` refs) | `3 distinct, 2 of 2 differ` — **MAXIMALLY DIVERGENT** |
| a fresh `--depth 1 --single-branch` clone | `1 distinct, 0 of 2 differ` — **inert on most branches** |

The two most opposite verdicts the script can print. Arm 2 in the same fresh clone said my branch touched `(none)` of `BACKLOG.md` when it demonstrably touched L73, and C1 reported `0 (branch, file) pairs checked, 0 mismatches` — a control that checked nothing, printed as a pass.

**2. A ref it could not resolve was skipped in silence, and the skip was ASYMMETRIC.** `blob()` passes `check=False` and returns `None`; the divergence loop skipped the `None` **while `len(allb)` still counted that branch in the denominator**. So an unfetched branch quietly moved the numerator down and left the denominator alone — which is precisely what manufactured `0 of 2` above instead of an error.

**3. An empty fixture read as a pass, and this one I measured on `main`'s own blob before changing anything.** With one branch in the lane there are no pairs, so C2 — the discriminator, the control that makes arm 3 a measurement — held because nothing was tested:

| | controls | last line | rc |
|---|---|---|---|
| `main`'s blob, live fleet, today | `C1 OK  C2 OK  C3 OK` | `PROBE OK` | **0** |
| this branch, same fleet | `C1 OK  C2 VACUOUS  C3 VACUOUS` | `PROBE VACUOUS` | **3** |

That is A39's trap (*"a gate that derives its expectation from the subject passes vacuously and looks identical to a working one"*) inside the A38 probes, and A41 was the inverse half of the same family.

**The fix.** `tests/a38_fleet_state.py` resolves one input for both probes. Live mode takes each sha from the **same `ls-remote` answer** as the name and never consults `origin/<branch>`. A ref the input names and the repo lacks is a hard stop (rc=2) printing the exact `git fetch` that repairs it, or `--fetch` runs it. A control that held vacuously prints `VACUOUS`. Both probes now use **four exit codes** — 0 measured, 1 control failed, 2 input unresolvable, 3 vacuous — because A41's whole complaint is that one code carried all four meanings.

**`refs/pull/<N>/head` is what makes a pinned default possible at all.** All seven branches the 10-01 cell measured were deleted when Jeff merged them, so `refs/heads/tide/win/**` would not have survived the week. GitHub keeps the pull refs: `git ls-remote origin refs/pull/618/head` still answers `cc025a2be`, and a shallow clone can fetch it.

**Verification: a recorded run reproduces byte-for-byte across two independent repositories.** Pinned, the row-adjacency probe measures a real fixture — 7 branches, 21 pairs, 35 `(branch, file)` pairs in C1, all three controls exercised — and the output hashes identically from the developer's tree and from a fresh shallow clone that had to fetch all eight refs:

| run | repo | rc | sha256 (line-endings normalised) |
|---|---|---|---|
| 1 | `C:\SE\TideSynth` worktree | 0 | `d644b1fa0dcba1ab…` |
| 2 | same, immediately again | 0 | `d644b1fa0dcba1ab…` |
| 3 | fresh `--depth 1` clone, `--fetch` | 0 | `d644b1fa0dcba1ab…` |

### The lane sweep: `0 of 5040` reproduces, the DEPTH HISTOGRAM DOES NOT, and the input that moved it NO LONGER EXISTS

Pinned to `main = 57a1bf593` (the sha this run's STEP 0 fetch recorded for the 10-01 cell) the lane sweep reproduces the recorded conclusions exactly — `7 of 7 merge into main individually`, `orderings landing ALL 7: 0 of 5040`, and `after #622 blocks nothing` / `after #618 blocks nothing`, the two zero-diff branches. **But the histogram differs from the one the 10-01 entry records:**

| | depth histogram | deepest order |
|---|---|---|
| recorded, 2026-10-01 entry | `{1: 2400, 2: 1920, 3: 720}` | `#622 -> #618 -> #614` |
| pinned to `main = 57a1bf593` | `{1: 1920, 2: 1824, 3: 1008, 4: 288}` | `#622 -> #618 -> #590 -> #587` |

Same seven PR heads. **And the branch set is NOT the variable: all seven tips are byte-identical to what is pinned**, checked `refs/pull/<N>/head` against this box's own `refs/remotes/origin/<branch>` one at a time. **And `main` is not the variable either.** I ran the pinned sweep against three candidate `main`s — `57a1bf593` (that cell's own fetch target), `7c92c3a8e` (#627) and `c6a3c0ee0` (#628), the only commits it could have been — and **all three give the identical histogram**, `{1: 1920, 2: 1824, 3: 1008, 4: 288}`. Their `docs/decisions.md` blobs are the same (`d7494d564`), so there was nothing for `main` to change.

**What settles it is a path in the recorded matrix that CANNOT conflict among the commits I can still recover.** The recorded depth-2 matrix names `docs/decisions.md` on five rows; mine names it on none. The reason is exact: four of the seven branches — #597, #590, #587 and #586 — carry `docs/decisions.md` at the **byte-identical blob `5da644d6a`**, against a merge base holding `d7494d564`. An identical change on both sides of a three-way merge is not a conflict, so no pair drawn from those four heads can produce the recorded line, under any `main`.

**So the heads the sweep measured are NOT the heads I can recover.** `refs/pull/<N>/head` is the branch head at **merge** time, and the sweep ran hours earlier; the branches were then pushed to again before Jeff merged them, and this box's stale `refs/remotes/origin/*` refs — never pruned, so they survived the deletion — agree with the pull refs rather than preserving the earlier tips. Nothing anywhere recorded them. **The recorded histogram is therefore unreproducible from any input that still exists**, and no amount of pinning fixes that after the fact: what A41's option (a) buys is that the NEXT such measurement is reproducible, not that this one can be recovered.

**This is the sharpest possible statement of A41's thesis, and I did not expect to find it:** a recorded measurement in this repository cannot be reproduced **at all** — not by pinning every branch, not by trying every candidate `main`, because the commits it actually measured were overwritten before anyone thought to name them. Its *conclusions* survived (`0 of 5040`, `after #618 blocks nothing`); its distribution is gone. A41's fix is prospective by nature.

### STEP 4's other half: the bookkeeping the merge sweep made due

**Eight `IN-REVIEW` rows whose PRs had all merged** — A36, A38, A39, A40, E72, E79, E80, E81 — are archived to `BACKLOG-DONE.md` with `Done = 2026-10-01`. `BACKLOG.md` **369,423 → 317,545 bytes**, 62 table rows → 54. (The archive move alone took it to 303 KB; my own `win` cell and A41 row additions put 14 KB back, which is the cost of this entry's own bookkeeping and is worth naming.) Every cited PR was checked individually, not inferred: all `MERGED` except A39's citation of #604, which is `CLOSED` and is a reference to a related PR rather than A39's own work ([#614](https://github.com/JeffMcClintock/TideSynth/pull/614) is A39's and merged).

**Two traps, both of which a future archiving run will hit.**

**A bare `DONE` row left in `BACKLOG.md` FAILS `check-backlog-archived.py`** — so the flip and the move are one edit, not two. `check-backlog-diff.py` permits an archive move and never requires one, which is how the gap existed; E45's script is what closes it.

**Archiving a row turns every HISTORICAL take clause naming it into a stale take-target, and `check-next-block.py` cannot tell a lane's live instruction from the `Previous cell follows.` chain below it.** Archiving these eight made the `mac` and `linux` cells red on three clauses naming A39, E79 and E80 — **all three in chain history**, one of them written on 2026-09-01. The fix that keeps the chain intact is a negation **in the same sentence as the verb**, because `RE_NEGATED` is scoped per sentence and `sentences()` splits on ` -- `: `(LANDED 2026-10-01 as #614 and archived, so do not take it)` works, while `-- landed, do not take` does not. **And I tripped the same lint with my own prose**, describing the three clauses by quoting them; rewording to *"three clauses naming A39, E79 and E80"* fixed it. That is A37's growth curve biting a second lint, and **the `mac` NEXT cell is now 61,508 bytes on one line** — A37 measured 34,988 on 09-10, A38 measured 41,216 on 09-18, so **+49% in 13 days**.

**`JOURNAL.md` is rotated, for the first time since 2026-09-08.** It had reached **470,307 bytes across 43 entries** — A36 landed the rule that stopped it rotating itself out, and the rule was then visible but unapplied for three weeks. **470,307 → 53,277 bytes, 43 entries → 6**, with 38 moved into `JOURNAL-2026-09.md` (09-29 back to 09-08). The floor is 4 — the entries carrying the most recent date, 2026-10-01, now that mine is one of them — and the 60 KB ceiling bound first at 6, so the floor never came into it. I targeted **60,000 bytes rather than 60 KiB**, because *"under 60 KB"* is ambiguous and 7 entries came to 60,514 bytes: under 60 KiB, over 60 kB. **Lossless, checked by heading sets over all three journal files: 482 → 483 unique headings, 0 missing, 0 duplicated, and the one addition is this entry.** `check-journal-prepend.py` confirms it independently — `1 new entry prepended`, 38 rotated out and each verified verbatim in the diff. **And A36's companion warning held:** `extract-lessons.py` sees the archive by glob, so moving 38 entries into it did not drop their lessons — `--check` reports **1,622 lessons from 379 entries**, up from the 1,534 / 364 the 10-01 cell recorded, with **0 date sections removed**.

**One thing in the rotation rule that does not hold, left for a ruling rather than fixed.** The rule says to append moved entries **below** what is already in the archive *"so the archive stays newest-first"*. The rationale is false in general and false here: `JOURNAL-2026-09.md` runs 09-08 → 09-01 newest-first, and the batch I moved (09-30 → 09-08) is **newer than everything in it**, so appending below cannot keep newest-first. I followed the instruction as written, because `JOURNAL-2026-08.md` shows previous runs did the same — its dates are **not** monotone — and inventing a prepend convention mid-run would diverge from what the other two boxes will do. The discrepancy is recorded here rather than filed as a row, because the `PROPOSED:` entry on the bookkeeping hot spots already covers the journal's layout and its **`Decide-by` was *"the next `JOURNAL.md` rotation"*, which this run has now done.**

**Learned:**

- **An unrecorded input can become unrecoverable, and then the measurement is simply lost.** A41 was filed about the branch set. Pinning all seven branches did not reproduce the recorded histogram, nor did any of the three candidate `main`s — because `refs/pull/<N>/head` preserves each branch's head at MERGE time and the sweep ran against earlier tips that were pushed over. The tell was a path in the recorded output (`docs/decisions.md`) that provably cannot conflict among the commits that still exist. **Record every ref at the moment you measure**; afterwards there may be nothing left to pin.
- **A silent skip and a hard failure differ most when the input is partly available.** `blob()` returning `None` for an unresolvable ref moved arm 1's numerator while leaving its denominator alone, so a half-fetched clone produced not an error but a *confident opposite answer*. The asymmetry is the defect; `check=False` was the mechanism.
- **Archiving a row has a blast radius in the NEXT block's history, not just its present.** `check-next-block.py` reads a whole cell, and a cell carries a ten-deep chain of superseded instructions. Expect to defuse clauses written weeks ago by another lane, and expect your own description of the problem to trip the same lint.
- **A `DONE` row and an archived row are not two states but one edit.** `check-backlog-diff.py` permits the move without requiring it, and `check-backlog-archived.py` requires it — so flipping to `DONE` and stopping is the one combination that is red.
- **"May proceed meanwhile: EVERYTHING, without exception" is a claim about a question, and a row can still fall inside it.** The bookkeeping `PROPOSED:` entry says it makes no row ineligible, reasoning that it is about where runs write rather than what rows build. A37 builds where the NEXT cells live, so the reasoning does not reach it — read the *reason* a question is declared non-blocking, not just the declaration.
- **Thirteen cells of pure STEP 1.5 ended the moment a human merged**, which is what A38 measured and what its `PROPOSED:` entry's *"A run is the scarce resource here, not a merge"* says. Nothing in the fleet's own machinery ended it.

**Not verified:** no build and no host — nothing this run touched is compiled code. Neither probe is run by any workflow (`grep -rn a38_ .github/workflows/` is empty) and this run did not change that, so the pinned default makes a recorded run reproducible without putting either probe on a gate. I did not judge the Accepts of E19 or E82, whose rows read `TODO` with merged PRs — the X2 shape, left for the next win run with a note in the `win` cell. I did not resolve why `tide/mac/issue-599` still exists as a pushed branch with a closed PR; it is mac's lane and predates this run, though it is now the only input the probes' `--live` mode sees besides my own branch. The bot PAT's real expiry remains unverifiable from here (A40's `NEEDS-JEFF` half, still unanswered).

**Machine state:** `C:\SE\TideSynth` started and ended on `main`, clean, and **never left it** — all work in `git worktree`s under the scratchpad plus two throwaway shallow clones, all removed. **All five repos were clean at the start:** `TideSynth` (`main`, behind 14), `SE16` (`master`), `SynthEditLib`, `gmpi_ui` (both `main`) and `GMPI_Wrappers` (`main`, behind 1); I touched none of the other four. The developer was at the machine throughout — Visual Studio on `SynthEdit_cmake` / `ModuleFactory_Editor.cpp`, plus Outlook, Slack and Settings — so **no GUI work and no screen taken**, which A41 needed none of. `git fetch` again warned *"too many unreachable loose objects"* in `C:\SE\TideSynth`; that is a local housekeeping note for Jeff (`git gc`), not a repository problem, and I did not run it on his tree. No credential value appears in any commit, PR, comment, journal entry or row.

**Next:** see the `win` NEXT cell. **For Jeff, three things:** (1) [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) is A41's code and cannot auto-merge — `tests/**` is not on the allowlist, by design. (2) The bookkeeping `PROPOSED:` entry's `Decide-by` has arrived: the rotation is done, so the question *"should the fleet's two bookkeeping hot spots stop being single shared files"* is ripe, and A37 is parked behind it with a 61.5 KB `mac` cell as the cost. (3) A40's `NEEDS-JEFF` half is still one question only the GitHub UI can answer — does the fleet PAT expire, and if so when?

**Branch/PR:** A41's code is on `tide/win/A41-probe-ref-pinning`, [#629](https://github.com/JeffMcClintock/TideSynth/pull/629) — `BACKLOG.md` byte-identical to `main` there, so it conflicts with nothing. This entry, the A41 row flip, the eight archive moves, the rotation, the regenerated `docs/lessons.md` and the refreshed `win` cell are on `tide/win/2026-10-01-a41-bookkeeping`, which is bookkeeping-only and should auto-merge.

