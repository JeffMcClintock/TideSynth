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

## 2026-08-19 — macos — E13: the MIDI prefab gets an E1 case; the other two uncovered prefabs cannot have one

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Did:** Took the mac NEXT block's fallback — "**E2** or the per-prefab **E1**
cases", E10 being GATED and not a build break. Filed the per-prefab work as
**E13** and shipped it: one new harness case, plus a measured account of why the
remaining two prefabs get none.

TIDE ships **five** prefabs and `tests/cases/` covered **two**. It now covers
three, and the README says — with evidence — why three is the ceiling for an
audio harness.

### The case: `prefab_midi`

`SE MIDItoGate2` was in **no** existing case. `voice_midi_note` goes via
`SE Keyboard2`, so the module the shipped MIDI prefab is actually built on, and
which V3 depends on, was untested. The case records Gate on the left channel and
Trigger on the right, and locks both of the prefab's promises for a note-on:

| channel | reference content |
|---|---|
| L (Gate) | constant full scale for all 23,999 frames — the gate opens and **stays** open |
| R (Trigger) | full scale for exactly **24 samples**, zero thereafter = **0.5 ms** at 48 kHz |

The 0.5 ms independently matches `build-prefabs.py`'s own note about the pulse
(`triggerCounter=22` at 44.1 kHz), which is the closest thing to a second opinion
available without listening.

**Both gates positive-controlled, through the harness itself, not by argument:**

```
control 1  MIDI connect deleted   FAIL  peak=-inf dBFS -- render is silent (<= -90.0 floor)
control 2  3 LSB on 200 samples   FAIL  peakdiff=-80.8 dBFS > -86.0   null=-104.6 dBFS
full suite (5 cases)              5/5 PASS, prefab_midi nulls at -inf (bit-exact)
```

Control 1 is what makes the case worth having: cut MIDI arrival and it goes to
**digital silence**, so the case tests that a note happens, not that a graph
builds. Control 2 is **finding (b) reproduced live on a real case** — the RMS
residual is −104.6 dBFS, comfortably *inside* the −100 dBFS gate, so RMS alone
would have passed the damage; the peak gate is the only thing that caught it.

### The thing I expected to be true and measured to be false

I took this case partly because it looked like a regression test for **E7's**
converter finding — a bool `Gate` into a float patch point is exactly the
mixed-datatype connection that needs an auto-inserted `SE BoolToVolts`
(`ug_base.cpp:1751`), and whose silent abandonment in Release was "a whole class
of silent failure".

**It is not, and nobody should re-derive this.** Removing **both**
`Converters.sem` and `Converters2.gmpi` from the engine's module folder leaves
the render **byte-identical** — same sha256, `a99c6714…` — with no
`==== MISSING MODULES ====` and no error. Nothing on this box declares
`SE BoolToVolts` at all: not the CLI binary (checked as ASCII *and* UTF-32LE,
because wide-string ids are invisible to `strings` — the false negative E2a
warned about) and not any of the 59 module files. SynthEditCL resolves that
conversion some other way.

The general point is worth more than the detail: **this harness measures the
ENGINE, and TIDE's converter linkage is a property of what `SynthEditSem`
links.** No case in `tests/cases/` can stand in for it. A guard for E7's
regression has to live where TIDE's own binary is what renders.

### The reference is bit-exact on Linux CI, first try

Added after the PR went green, because it is a stronger result than this run
expected. `verify` on the PR renders on **Linux**, against the *published*
engine, and compares to a reference this box seeded on **macOS**:

```
PASS  prefab_midi peak=-0.0dBFS null=-infdBFS peakdiff=-infdBFS
5/5 passed.
```

`null=-inf` is **bit-exact** — not "inside the gates", identical. That contrasts
with the two prefab references seeded on macOS on 2026-08-18, whose own case
files warn that their first CI run is an untested cross-platform comparison.
The reason is worth keeping rather than treating as luck: this reference contains
**no floating-point arithmetic to drift**. It is a step and a rectangular pulse,
both at clipped full scale, so there is no phase increment to integrate and
E1a's whole class of cross-platform residual cannot arise. A case with this shape
is expected to be exact everywhere, and a *non*-zero residual here would mean a
real defect rather than platform drift — which makes this case a sharper
instrument than the oscillator ones, not a blunter one.

### Why the other two prefabs get no case — measured, not assumed

**TIDE Output: the harness structurally cannot observe it.** `Sound Out`'s `Out`
pin is an **input**, so the recorder has nothing to attach to:

```
{"cmd":"render-audio","ok":false,"error":"could not connect --from source to recorder"}
```

Recording the patch points feeding it would test the patch points. The prefab's
real promise — that L and R become **two** channels because the input is
`IO_AUTODUPLICATE` — is visible in the connect commands (`to:[…,0]` then
`to:[…,1]`, a *new* pin), which is a graph-shape assertion, not an audio one.
End-to-end it is already covered by the v0.1 fixture pair in `tests/hosts/`.

**TIDE MIDI-CV: there is nothing to render.** It is a facade — four jacks and a
faceplate, every jack fed from *outside* by the root `SE MIDI to CV 2` (E7).
Rendered alone it reproduces its own scaffolding.

Both are now written down in [tests/README.md](tests/README.md) with the
evidence, so the next run does not re-derive either.

### Notes for whoever is next

- **A stale row, observed and deliberately not changed: E12 reads `TODO`, but
  both of its PRs are MERGED** — [SynthEditLib#23](https://github.com/JeffMcClintock/SynthEditLib/pull/23)
  (2026-08-18T23:30Z) and [SynthEdit#54](https://github.com/JeffMcClintock/SynthEdit/pull/54)
  (2026-08-18T23:07Z) — and its own row says the fix is verified at four clean
  shutdowns. I did not flip it: it is not my item, and I did not re-run its
  Accept clause on this box. It is a one-line repro
  (`TIDE_STANDALONE & sleep 9; kill -TERM $!`, then read
  `~/Library/Logs/DiagnosticReports`) and this is the box that can do it.
- **A pushed branch with no open PR:** `tide/mac/V3-midi-findings` sits 2 commits
  ahead of `main` with its PR ([#142](https://github.com/JeffMcClintock/TideSynth/pull/142))
  already **merged** — STEP 5's forbidden third state, left by an earlier run.
  The two commits are `25216c1` and `4e65874`, both E7/S8 findings. Someone
  should confirm whether that content reached `main` by another route and then
  delete the branch.
- **Harness baseline on this box before I touched anything: 4/4 PASS**, engine
  `SynthEditCL V1.6.186`. So the additions did not paper over a red suite.
- **The foreign-scan warning fires on this box** (`/Library/Audio/Plug-Ins/GMPI`),
  which finding (d) says is normal for a developer machine — but it does mean
  none of these local runs *prove* the named module set is what rendered. CI is
  where that gets proven, and CI is still skipped (**B1**/**C7d**).
- **Not built and not claimed this run:** TIDE itself, `SynthEditCL` and
  `SynthEdit`. This item touches only `tests/` and needed no rebuild; the engine
  used was the Release `SynthEditCL` already on disk from earlier today. So I
  have **no evidence either way** about whether mac's default branches build, and
  am not claiming any.

**Next:**

1. **E2** — the rest of the first Eurorack module set. It is the mac NEXT
   block's other named target and is now the larger half of that row. Each new
   prefab should arrive with its own case, and this run's README section is the
   guide to whether one is even possible for it.
2. **E12 wants five minutes from this box** to flip it DONE honestly — see above.
3. **A guard for E7's converter linkage** has nowhere to live today. It needs the
   TIDE binary rendering, which is the REAPER route (`scripts/render-and-measure.py`),
   not this harness. Worth a row if anyone wants that regression caught.

**Branch/PR:** [#168](https://github.com/JeffMcClintock/TideSynth/pull/168). Work done in a throwaway
git worktree, so no shared checkout left its default branch; all six working
copies were clean and on their defaults at start and at finish.

## 2026-08-19 — linux — C7 is four separate problems, not one; C7a done, the other three scoped

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot**

**Did:** Took **C7**, the topmost eligible row and the one the linux NEXT block
points at. Measured it before implementing it, found it is not one session, and
did what the C12 scoping run did: **split it, and shipped the one stage that was
completable and verifiable today.**

- **C7a — shipped.** [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59): TIDE's dead private-repo
  references removed from `SynthEditSem/CMakeLists.txt`, the one real one named in place.
- **C7b, C7c, C7d, C7e — filed**, each with Scope / Accept / Size. Inventory and
  reasoning: [docs/c7-clean-clone.md](docs/c7-clean-clone.md).
- **S18 — filed**, a public-repo defect found while inventorying.

### C7's row was wrong about what C7 contains

The row and the last four journal entries all say C7's content is "the 7 public
-file includes resolving only in `SynthEdit2`". **That is one of four things, and
it is the one this box does not own** — it is C13 (in review, windows) and C14
(filed, windows). The other three had never been named:

| # | The dependency | Measured size |
|---|---|---|
| 1 | TIDE's CMake listed five private include paths | **four were DEAD** — C7a |
| 2 | TIDE's own source lives in the private repo | 16 + 10 files, **blast radius 3 files** — C7b |
| 3 | TIDE links `EditorScreenshot` = `SE16/EditorScreenshot/` | on **neither** STEP 5 list — C7c |
| 4 | **`SynthEditLib` cannot configure standalone** | needs a root CMakeLists here — C7d |

**(4) is the one nobody had named and it is the structural gap.**
`SynthEditLib/CMakeLists.txt` consumes `${GMPI_SDK}`, `${GMPI_UI_SDK}` and
`${VST3_SDK}` and **never sets them** — `SE16/CMakeLists.txt:78-161` does, via
the override-or-fetch pattern. So "a stranger can build TIDE" needs a
superproject root `CMakeLists.txt` in *this* repo playing SE16's role for TIDE's
subset. **It is also free CI:** `build.yml`'s guard job (B1) gates the
three-platform matrix on a root `CMakeLists.txt` existing, so C7d turns the
matrix from *skipped* to *running* **with no `.github/workflows/**` edit** — which
matters, because the fleet's token deliberately has no `workflow` scope.

### C7a — four of five private include paths were dead

Each measured separately, not swept:

| Entry | Verdict | Evidence |
|---|---|---|
| `EDITOR_DIR` (`../SynthEdit`) | dead | `set()` once, referenced nowhere in the file |
| `EDITOR2_DIR` (`../SynthEdit2`) | dead | same — the identical pair C6 deleted from `EditorLib/CMakeLists.txt` |
| `../Shared` | dead | **the directory does not exist in the tree** |
| `../SynthEdit` | dead | icons, skins, `.chm`; no headers. `SynthEdit.rc` includes `resource.h` + `windows.h` only |
| `../SynthEdit2` | **REAL** | one consumer: `TideAppStubs.cpp:31` → `SynthEditApp.h` |

Also dropped a stray `PRIVATE` keyword — `include_directories()` is the
*directory*-scoped command and takes paths only, so CMake was adding a relative
directory literally named `PRIVATE`. Harmless, but it made the block read as if
it were `target_include_directories()`.

**Result — fresh Ninja/GCC/Release tree, and a baseline taken in the same tree
immediately before the change so the comparison means something:**

```
baseline   configure RC=0   928/928 RC=0   ctest 67/67
after C7a  configure RC=0   928/928 RC=0   ctest 67/67
```

Zero `error:`, zero `undefined reference`, both runs. `TIDE.gmpi`,
`TIDE_VST3.vst3`, `SynthEditCL` and `SynthEditWayland` all built — so the
standing "leave SynthEdit, SynthEditCL and TIDE building" rule holds on this
platform. **`SynthEdit2` (WinUI3) was not built; it is Windows-only.**

### Two measurements the next stage should not re-derive

**TIDE's own sources have exactly TWO private includes.** Every `#include "..."`
in `SynthEditSem/*.{cpp,h,mm}` resolved against the public repo first, then
`SE16`:

```
SynthEditGui.cpp   "ContainerThumbnail.h"  ->  SE16/EditorScreenshot/   (C7c)
TideAppStubs.cpp   "SynthEditApp.h"        ->  SE16/SynthEdit2/         (C14)
```

**Moving TIDE's source out of `SE16` has a blast radius of three files.**
`grep -rl 'SynthEditSem\|TideModules'` over `SE16`'s build files returns
`CMakeLists.txt` (one `add_subdirectory` at `:409`), `SynthEditSem/CMakeLists.txt`
itself, and `se_gmpi/vst3/CMakeLists.txt` where **both hits are comments**. No
`.vcxproj`, no `.pbxproj`, no `.sln`. That is why C7b is sized at one session.

### A correction to C14's framing, measured rather than inherited

[#165](https://github.com/JeffMcClintock/TideSynth/pull/165) files C14 on the grounds that `SynthEditApp.h` "pulls in
**`moonbasepp_Licensing.h`**". It does — **inside `#ifdef SE_MOONBASE_SUPPORT`**
(`SE16/SynthEdit2/SynthEditApp.h:6-11`), and **that header is not tracked by git
at all**: its own comment says to copy `moonbase_lib/` in from `SynthEdit_Azure`,
and `find` over `~/SE` returns only two workflow files. So "it drags the
licensing surface into the public repo" holds only for a moonbase build. **That
narrows C14 rather than blocking it.** The header is still a private one
declaring a private app class, and that is the real reason it cannot simply move.

### S18 — and why the committed script could not have found it

Three public files include `soundpipe.h`, which resolves only in
`SE16/SDKs/Soundpipe/`: `modules/SoundPipe/ReverbChowning.cpp:4`,
`ReverbSp.cpp:4`, `ReverbZita.cpp:4`. **Not a C7 blocker** — `modules/` is added
by `SE16`'s root (`:416`), never by `SynthEditLib`'s own, and TIDE links none of
it. It is a defect in the public repo *as a public repo*.

`scripts/dangling_private_includes.py` skips `SDKs` by design (`SKIP_DIRS`,
`:57-63`) because vendored SDK headers are not carve-out edges. **That rule is
correct for what the script measures and is exactly why this was invisible.**
Cross-checked both ways this run: script and hand scan agree exactly on the
**seven** carve-out edges across four headers, and differ only here. Use the
script — it is right, and a naive grep is wrong by ~3x for the reason its
docstring gives.

### STEP 1 / STEP 1.5

Two open `platform:linux` issues, both authored by `tide-rack-bot`, **neither
actionable and neither a build break** — so no STEP 1 override:
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88)'s remaining half is `SynthEditJuce`, which its own
CMakeLists calls deprecated and which nothing adds to the build, and
[#156](https://github.com/JeffMcClintock/TideSynth/issues/156) is the ctest path default. Both are GATED-by-default paths
(`SE16/SynthEditJuce/`, `SE16/tests/`), so A17's exception does not reach them.
No open `tide/linux/**` PRs. All six working copies were clean and on their
default branches at start.

**Next:**

1. **C7b** — move `SynthEditSem/` and `TideModules/` into this repo, `SE16`
   consuming them via `TIDESYNTH_FOLDER_OVERRIDE` + `FetchContent`. Both are
   ALLOWED paths, so **no ruling needed**, and it does not depend on C13/C14.
2. **C7c is NEEDS-JEFF** — `EditorScreenshot` is on neither STEP 5 list. G3 is
   the precedent; the answer took one day last time.
3. **C7d after C7b**, and do not expect it to *pass* until C13 and C14 land — a
   clean clone is precisely what those two dangling headers fail.
4. **C13 and [#165](https://github.com/JeffMcClintock/TideSynth/pull/165) merged while this run was working**, which is why this entry's
   branch needed a merge from `main`. Re-measured afterwards rather than assumed:
   `scripts/dangling_private_includes.py` on the merged defaults reports **1**
   dangling private include, down from **7** — and the survivor,
   `ApplySynthEditConfig.cpp:2` → `SynthEditApp.h`, is **the same header TIDE's
   own `TideAppStubs.cpp:31` includes**. So **C14 is now the entire remaining
   private-include debt, and it closes two consumers at once**, one of them
   TIDE's. C7e needs it; C7b does not.

   Note the C7a build numbers above were measured against `SE16` `5ae7fbc67`,
   i.e. **before** C13 merged. C13 moved three headers into `SynthEditLib` and
   did not touch `SynthEditSem/`, so the two changes are orthogonal — but that is
   reasoning, and the re-verification against the post-C13 tip is recorded
   separately on [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59).

**Branch/PR:** [SynthEdit#59](https://github.com/JeffMcClintock/SynthEdit/pull/59) + [#167](https://github.com/JeffMcClintock/TideSynth/pull/167).
Both working copies returned to their default branches.
