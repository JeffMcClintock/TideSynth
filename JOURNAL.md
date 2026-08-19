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
3. `git checkout origin/main -- .` inside a worktree will silently revert your
   own uncommitted work. Recovered here via `git stash`; the cheap habit is to
   commit as soon as a coherent change exists, which STEP 3 already says.

**Next:**

1. **A29** — minutes, and it revives auto-merge for all three boxes.
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

## 2026-08-19 — macos — E2b: a Filter rack module, and the linkage check that actually discriminates

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Third item this session**, on Jeff's instruction ("next task"), after E13 and
E12 merged. Claimed with a pushed DOING mark before any work, per STEP 2.

**Did:** Took **E2**, the `mac` NEXT target. Found it is not one item, split it
the way C7 and C12 were split, and shipped the first stage as **E2b — a Filter
rack module**. TIDE now ships **six** prefabs and `tests/cases/` covers four.

### Why E2 had to be split rather than attempted

E2 says: *"Define the naming and I/O conventions for a module Container, then
build the rest of a first curated set, each with its own E1 test case."* **It
never says which modules that set contains**, which is a product decision and
not a run's to invent — so as one item it fails STEP 2's "state the acceptance
check before starting" bar. The conventions half is also largely *already
written down*, in `build-prefabs.py`'s header: panel geometry, the
`PanelWndPosition`-vs-`panel_rect` distinction, faceplate sizing, and the rack
grid (`hpWidth 15`, `rowHeight 380`, so a slot is a multiple of 15 wide and at
most 350 tall). E2's remaining content is therefore **modules**, one at a time,
and a filter is the obvious first: oscillator → filter → envelope → out is the
canonical subtractive voice.

### The linkage check, which is the transferable part

E2a left a trap: *"every module in a prefab must be a class TIDE links"*, with
`strings`/`nm` on the module-id string called out as a **false positive**
(`SE Rectangle XP` is in the binary via the rename table at `CUG.cpp:301`).
That is true of the *id string*. It is not true of the **static-init symbol**,
which is a real linkage fact, and that is the check E12's entry recommended:

```
se_static_library_init_ug_filter_1pole      PRESENT     <- 1 Pole LP
se_static_library_init_ug_filter_1pole_hp   PRESENT     <- 1 Pole HP
se_static_library_init_SVFilter4            absent
se_static_library_init_ButterworthHP        absent
se_static_library_init_OscillatorNaive      absent      <- S8's module, still absent
```

**It discriminates, which is the only reason to trust it** — three of the five
obvious filter candidates come back absent, including the one
(`OscillatorNaive`) already known absent by independent measurement. So the
positive result on `1 Pole LP` means something.

**A trap this exposes for anyone scanning `UgDatabase.cpp` for candidates:**
`INIT_STATIC_FILE(SVFilter4)` and `INIT_STATIC_FILE(ButterworthHP)` are both
*in that list*, and both are **absent from TIDE**. The list is
`SynthEditLib`'s, and TIDE links a subset. **Do not read the INIT list as a menu
of what TIDE can instantiate.** The core `ug_*` entries are the ones that come
free — `ug_filter_1pole`, `ug_vca`, `ug_pan`, `ug_sample_hold`, `ug_random`,
`ug_quantiser`, `ug_switch`, `ug_delay` all measured PRESENT, and all are
plausible future E2 stages.

### The module, and its default

`1 Pole LP`, `ug_filter_1pole_lp.cpp:21`, category Filters. Pins
`Signal` / `Pitch` / `Output` — the Oscillator prefab's shape with an audio
input added. `Pitch` is the **cutoff**, 1 V/octave on the same scale the
oscillator uses, so 5 V = 440 Hz.

Measured on a 440 Hz source, recording the filter output:

| cutoff | peak |
|---|---|
| 10 V (14 kHz) | −6.3 dBFS — effectively open |
| 8 V (3.5 kHz) | −6.6 dBFS |
| **5 V (440 Hz)** | **−9.5 dBFS — −3 dB AT cutoff, textbook 1-pole** |
| 2 V (55 Hz) | −22.2 dBFS — ~6 dB/octave beyond |

**The FREQ jack ships defaulted to 10 V, wide open.** That is not a preference,
it is the rule the Envelope's GATE default already follows and it is worth
stating as a convention: **an unpatched jack takes the value that lets the
module pass signal**, because a rack that does not patch everything must still
sound. A filter defaulting to 0 would render silence on drop-in and look broken.

### Verification

**In TIDE, not merely in the generator:**

```
TIDE: 6 rack prefab(s) seeded from the bundle     (was 5)
TIDE: rack built for 48000 Hz, block 512
shutdown rc=0, no new crash report
```

**Harness 6/6**, with the new case's reference independently checked rather than
trusted: 440.0 Hz by zero-crossing count, peak −9.5 dBFS = the −3 dB point.
Both gates positive-controlled:

```
control A  cutoff 5V -> 2V (filter-response regression)  FAIL  null=-16.6 dBFS
control B  reference scaled 0.99 (-0.09 dB level)        FAIL  null=-55.3 dBFS
full suite                                               6/6 PASS
```

**Control B matters more than it looks.** This case carries `prefab_oscillator`'s
*relaxed* gates (−67/−62), because a free-running oscillator is in its signal
path — and a relaxed gate invites the question of what it still catches. A **1%
level change fails it with 11.7 dB to spare**, so level, tuning and filter-response
regressions are all still caught; what is given up is localized damage below
~12 LSB, which `voice_midi_note` covers at the default gates.

### A finding out of CI, filed as E1c rather than fixed here

The PR's Linux `verify` job renders against these macOS-seeded references, and
the numbers say the relaxed gates on two cases are far too wide:

```
prefab_oscillator  null -131.1 dBFS  peakdiff -90.3     declared gates -67 / -62
prefab_filter      null -121.4 dBFS  peakdiff -90.3     declared gates -67 / -62
```

Both are **rounding class** — inside even the *default* −100/−86 — leaving ~55 dB
of margin for a real regression to hide in.

**Where the wide gates came from:** E1a measured a free-running oscillator at
−73.5 dBFS and sized the gates 6 dB above. Sound measurement, **different
oscillator**: E1a used `SE Oscillator (naive)`, a separately-loaded module, while
these two cases use `Oscillator`, the core `ug_oscillator2`. My own new case
inherited the relaxed gates *by analogy* — I copied `prefab_oscillator`'s reason
text, which says "same class of residual" — and CI then showed that analogy is
probably wrong.

I have not tightened them, because the honest fix is a measurement of each case's
own residual in both directions, not a guess in the other direction — that would
be the same reasoning-instead-of-measuring the row is about. Filed as **E1c** with
the Accept clause stating exactly that. Note `voice_midi_note` also contains the
naive oscillator and passes at *default* gates, so "naive drifts" is not the whole
story; its pitch is MIDI-derived rather than free-running, which is the variable
to isolate first.

### The five prefabs I regenerated and did NOT commit

Running the generator rewrote all six files. The other five are **handle churn
only** — randomised `handle` / `fMod` / `tMod` / `module` / `tiedtomod` values —
and I proved that rather than asserting it: normalising those five attributes
makes all five **byte-identical to HEAD**, and MidiCv's `tiedtopin` mapping stays
**7/8/9/10**, the contract `TideApp` hard-codes. Reverted, not committed, per
STEP 5.

**Worth knowing before the next E2 stage:** `build-prefabs.py` is not
reproducible — every run produces a different file for every prefab. So a
generator change always looks like a six-file diff, and **the only way to see
what you actually changed is to normalise the handles**. The check is four lines
of Python and is in this entry's PR body.

**Next:**

1. **E2c and onward** — more modules, one stage each. The measured candidate
   list is above; a **VCA** (`ug_vca`) and a **Sample & Hold** (`ug_sample_hold`)
   are the obvious next two, and `tests/README.md` now says how to decide
   whether a new prefab can have a case at all.
2. **E10** is still the biggest thing on this platform and needs `SynthEditLib`
   authority — a live host crash whose own Accept clause would not fix it.
3. **`tide/mac/V3-midi-findings`** is still a pushed branch with no open PR,
   reported for the third run running.

**Branch/PR:** [SynthEdit#60](https://github.com/JeffMcClintock/SynthEdit/pull/60)
plus the TideSynth PR carrying this entry — **two repos, and they are one
change**, though neither breaks the other's build: the E1 case rebuilds the
recipe from primitives rather than loading `Filter.synthedit`, so it does not
depend on the SynthEdit half landing first. Work done in a throwaway worktree
for TideSynth; `SynthEdit` was branched in place and is returned to `master`.

## 2026-08-19 — macos — E12 verified on the merged trees and closed; E13 archived; a mac build trap that survives ZERO_CHECK

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Second item this session**, on Jeff's instruction ("merged. next task"), after
**E13** merged as [#168](https://github.com/JeffMcClintock/TideSynth/pull/168).
Claimed with a pushed DOING mark before any work, per STEP 2.

**Did:** Took **E12** — the one row on this box that had a *merged* fix and a
`TODO` status — verified it on the merged default branches, and closed it. Also
did the STEP 4 chore E13 left behind and re-pointed the stale `mac` NEXT cell.

### Why E12 rather than the NEXT block's E2

The `mac` NEXT cell named E10 (GATED, no authority, not a build break), then
"**E2** or the per-prefab **E1** cases". The E1 cases closed an hour earlier as
E13, so the cell was already one item out of date. **E12 was `TODO` while its own
fix was merged**, which is the state most likely to be silently wrong: the row's
"four consecutive shutdowns" was measured **on the branch**, before
[SynthEditLib#23](https://github.com/JeffMcClintock/SynthEditLib/pull/23) and
[SynthEdit#54](https://github.com/JeffMcClintock/SynthEdit/pull/54) merged, and
nothing had re-measured it since. This is also the only box that can. E2 is a
large open-ended authoring item and is still there; the cell now points at it.

### Result — 4/4 clean shutdowns on the merged defaults

Trees: `SynthEdit` `2f5fca5e3`, `SynthEditLib` `65d55cd`, Release, Xcode.

```
run 1: alive at 9s, TERM sent, wait rc=0
run 2: alive at 9s, TERM sent, wait rc=0
run 3: alive at 9s, TERM sent, wait rc=0
run 4: alive at 9s, TERM sent, wait rc=0
TIDE_STANDALONE crash reports: 14 before, 14 after
```

Three things make that more than an exit code:

- **Each run reached a real working state before being killed**, so this is not
  four fast failures: `TIDE: 5 rack prefab(s) seeded from the bundle`, root
  MIDI-CV wired (`MIDI In → MIDI-CV 2 → facade`), and
  `TIDE: rack built for 48000 Hz, block 512`. All four logs identical.
- **`wait` returned rc=0, not 143.** The app handles SIGTERM and exits
  gracefully rather than dying by signal — the same path the row says a user's
  Quit takes.
- **No `.ips` from *any* process** after the build, not merely none named
  `TIDE_STANDALONE`, so the count is not hiding a differently-named crash.

The `ViewBase::setHost` override that severs child views' copied `dialogHost`
is present on the merged tip at `ViewBase.cpp:2736-2751`.

**What I did NOT re-measure, and will not claim:** the *before* half. The 3/3
crashes are the original interactive measurement, kept in the archived row.
Reproducing them would mean reverting a merged fix inside Jeff's shared tree,
which is not a thing a scheduled run should do to prove a point.

### The build trap, and why it is worth a paragraph

After fast-forwarding the six repos to their merged tips, **the existing Xcode
build tree would not build**:

```
error: Build input file cannot be found:
  '.../SynthEdit/SynthEdit2/InterfaceObject_editor.cpp'
  (in target 'EditorLib')
```

C12d moved that file into `SynthEditLib`'s root; the **generated Xcode project
kept the old path**. The interesting part is that this survives the mechanism
meant to prevent it — the same build log says
`Generate CMakeFiles/ZERO_CHECK will be run during every build`, and it did.
`cmake -S . -B build` clears it in seconds (configure RC=0, and all four folder
overrides report local, including `Using local GMPI WRAPPERS folder`).

**So: after any carve-out stage that MOVES files, reconfigure before building on
mac.** Do not read the resulting error as a broken carve-out — C12d, C13 and C6
are all fine; the generated project was stale. This will recur on C7b and C10,
both of which move files.

Related and still open: **S17** — the build may compile
`build/_deps/gmpi_ui-src` rather than the local `gmpi_ui` clone. It does not
affect this result (E12's fix is in `SynthEditLib`, not `gmpi_ui`) but anyone
verifying a *gmpi_ui* change here should settle S17 first.

### Bookkeeping done this run

- **E13 → DONE and archived.** [#168](https://github.com/JeffMcClintock/TideSynth/pull/168)
  merged 03:03:42Z. Worth carrying forward: its CI `verify` job renders on
  **Linux** against the published engine and returned `null=-inf` — **bit-exact**
  against the macOS-seeded reference, because that reference contains no
  floating-point arithmetic to drift.
- **E12 → DONE and archived**, with the verification above.
- **The `mac` NEXT cell re-pointed to E2**, since both of its other fallbacks are
  now closed, with a pointer to `tests/README.md`'s prefab-coverage section.

### Still open, and still nobody's

- **`tide/mac/V3-midi-findings`** remains a pushed branch with no open PR — its
  PR ([#142](https://github.com/JeffMcClintock/TideSynth/pull/142)) merged, and
  two commits (`25216c1`, `4e65874`) sit on top of `main`. Reported last run,
  unchanged. Someone should confirm that content landed elsewhere and delete it.
- **E10** is the real prize on this platform and needs `SynthEditLib` authority:
  a live host crash, whose own Accept clause would not fix it.

**Next:**

1. **E2** — the `mac` cell now points there, and it is the larger half of what is
   left in that area. Read `tests/README.md`'s coverage section first.
2. **E10**, for anyone who may edit `SynthEditLib`.
3. **S17**, before anyone tries to verify a `gmpi_ui` change on this box.

**Branch/PR:** `tide/mac/E12-standalone-shutdown` and the TideSynth PR carrying this entry (branch named rather than numbered: this PR is docs-only and therefore A4 auto-merge eligible, so a follow-up commit adding the number could land on a branch whose PR had already merged — **A22** exactly).
No code changed in any repo this run — the fix was already merged; this run
measured it and updated the queue. Build tree reconfigured (a build artifact,
not source). All six working copies clean and on their default branches, fast
-forwarded to their merged tips.
