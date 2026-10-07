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

## 2026-10-08 — macos — no item: STEP 1 and 1.5 empty, STEP 2 re-walked and nothing is eligible for `mac`; E92 is Linux-only by its own guard (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.26454.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. The queue is blocked for this lane, and this entry says why, item by item, so the next run can check the reasons instead of inheriting them. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`5175b02..7e33c2e`).

**STEP 1:** there is no open `platform:mac` issue. The open issues are #583 (linux, `tide-rack-bot`, already triaged as E89) and #44 (the digest).

**STEP 1.5:** this lane has one open PR, [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) (E91's probe). All its checks pass, and it has no reviews and no comments. Under STEP 1.5 it is waiting for merge, so I left it alone. It is `tests/` only and so allowlist-blocked by design. It is also the only open PR in the fleet.

**STEP 2, re-derived rather than inherited.** Since the 10-07 walk, `main` gained only that run's bookkeeping (#644) and Jeff's `7e33c2e` (path-tracer core cap, `modules/common/TidePathTracer.*`). `git diff 5175b02 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only the E91 flip, the new E93 row and the `mac` cell. So every parking reason below rests on unchanged text, which I read again:

| row | why not this run |
|---|---|
| A35, A37, A42 | parked on their own open `PROPOSED:` entries or, for A42, by its own words. The five `PROPOSED:` entries in `docs/decisions.md` are unchanged. |
| S8 | `NEEDS-SPEC` is the first thing in its row |
| E19 | Accept needs a real host with ≥60 s of watched animation and a VCV context-menu toggle. That is a screen, and a scheduled run cannot drive one. |
| E82 | Accept is a right-click on a VCV panel, which also needs a screen |
| E2 | an umbrella; it says of itself it is not takeable |
| E84 | a `.github/workflows/**` edit, which this credential cannot make |
| X2, E89 | `linux` |
| **E92** | **`any` on paper, Linux-only in fact; see below** |
| E93 | `mac`, but its first Accept branch is Jeff's scope ruling (AU2 is out of TIDE since S40). Fixing it is not identical under the WONTFIX answer. The 10-07 cell also says not to build an AU2 consumer unasked. |

**E92 cannot be taken or measured on macOS, and no previous cell said so.** The 10-07 cell's walk did not mention it. `se_gmpi/vst3/CMakeLists.txt` opens with `if(NOT UNIX OR APPLE) return()`, ahead of the `gmpi_wrappers_POPULATED` guard the row is about. On this box, `SynthEditTemplate_VST3` therefore does not exist under **either** variable. E92's Accept (`ninja -t targets all | grep -c SynthEditTemplate_VST3` non-zero in both configures) is unsatisfiable by construction on mac and win. I read this from SE16 `master`. I did not configure it to measure it. I did not edit E92's `Plat` cell, because whether a run may correct a `Plat` cell after filing is itself an open `PROPOSED:` entry.

**Learned:**

- **Check a row's platform guards before you trust its `Plat` cell.** E92 says `any`, but the target it measures sits behind `if(NOT UNIX OR APPLE) return()`, three lines above the guard the row describes. Reading the first twenty lines of the file the Accept cites is enough to settle it.
- **Neither `claude` nor its CLI version is on this box's `PATH` for a scheduled run.** The app version came from `/Applications/Claude.app`'s `Info.plist` (`CFBundleShortVersionString`).

**Not verified:** nothing was built or run, because there was no item. I did not configure SE16 to measure E92's target count on mac. The claim above is a reading of the guard. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half).

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean, and never left it. The work was in a scratchpad `git worktree`, removed at the end. `~/Documents/GitHub/SynthEdit` (`master`, clean) was read only. No GUI, no screen taken, and no credential value appears anywhere.

**Next:** see the `mac` cell. **For Jeff:** (1) #643 waits on you by design. (2) E93 wants a scope call. (3) E92 is a linux job in practice. A ruling on the `Plat`-correction `PROPOSED:` entry would let a run re-label it. Until something moves, mac runs will keep producing entries like this one.

**Branch/PR:** `tide/mac/2026-10-08-queue-blocked`. This entry, the `mac` cell, the rotation and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge.

## 2026-10-07 — macos — E91: AU3 does not have E88's defect, measured 3/3 in-process with a control that goes silent; AU2's half was dead code and is filed as E93 (scheduled run)

**Prompt:** b97bc00 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **2.19675.1** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E91** (E88's AU2/AU3 remainder). I measured the AU3 half with a new in-process probe, which needs no wrapper change. I found that the AU2 half's premise was a line inside `#if 0`, and filed the real AU2 gap as **E93**. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`e7fba10..5175b02`).

### STEP 1 / 1.5 / 2

**STEP 1:** no open `platform:mac` issue. The open issues are #583 (linux) and #44 (the digest). **STEP 1.5:** there were **no open PRs anywhere in the fleet**, because the 10-06 linux merge sweep landed everything. **STEP 2:** the `mac` NEXT cell was the 10-04 one, and both its targets (E85, E88) are archived, so I walked the file. A35 is parked on its own two `PROPOSED:` entries, because its deliverable *is* the exception they decide. A37 is overlapped by the bookkeeping hot-spots entry, whose options move the NEXT block. A42 is ineligible by its own words. S8 is `NEEDS-SPEC`. E2 is an umbrella. **E19 and E82 need a screen**, and a scheduled run cannot drive one. E84 is a workflow edit. E89 is linux. **E91 was the first eligible row**: it is `any` but mac in practice, and no branch or PR named it. Of the five open `PROPOSED:` entries, none parks it: four say *"everything"* and E72's parks only E72. The DOING mark was pushed first (`07f8b02`).

### AU2: the row's premise was dead code

E91 said *"AU2 CALLS `setPresetUnsafe` THE SAME WAY"*, citing E88's `AU2_Wrapper.cpp:341`. **That line is inside `#if 0`**, in the old `stateMgr.callback` block. AU2's live restore is `RestoreState`. That function hands the `GMPIPRESET` to `gmpiController.setPresetXmlFromDaw` (the controller's store) **and nothing else**. There is no `notifyControllerOfPreset` (E71's fix on AU3), and there is no queue send: the only ui→dsp sender, in `onTimer`, is inside `/* … */`, and `sendNonNativeParameterToProcessor` is never assigned. Native floats are fine, because they go straight to the processor through `SetParameter`. **So on reading, a restored blob never reaches AU2's DSP in either order.** That is wider than E88, not the same defect. **TIDE builds no AU2** (`FORMATS_LIST GMPI VST3 CLAP AU3 STANDALONE`, S40), so no TIDE instrument can measure it. I filed it as **E93** (`mac`, may be WONTFIX for TIDE, which is Jeff's call) rather than leaving it inside a closing row. That is A42's shape. The id was allocated by sweeping `origin/main` and the one remote `tide/*` branch: the highest was E92.

### AU3: the instrument

AU3 by reading should be order-independent. `-setFullState:` never calls `setPresetUnsafe` on the processor. It frames every stateful parameter onto the ui→dsp queue (`ppc3` for a blob), and the render block drains that queue on its first line. That is the live-change route GMPI_Wrappers#42 had to *add* for VST3. E88's history is the warning against stopping at reading, so I measured it.

**The obvious host was unavailable for two reasons.** Through `AudioComponent` the extension loads out-of-process, so `building rack` never reaches the host (E9's finding). And the registered AUv3 is Jeff's `~/Applications` install from **2026-08-31**, which predates E71 (`4c11d6d`, 09-07), so it is not the code under test. The 08-29 entry rules that an unattended run must not displace it.

**The in-process host.** I took the appex's own link line (`ninja -t commands …/TIDE-Rack.appex/Contents/MacOS/TIDE-Rack | tail -1`), dropped `-e _NSExtensionMain`, added the probe's object, and wrote the executable into a **copy** of the built appex's `Contents/MacOS/`. NSBundle then resolves the plug-in's resources exactly as in the extension, and the probe instantiates `GmpiAudioUnit` by name. **Nothing was registered and nothing outside the scratch build was touched.** The header recipe was re-run verbatim from a clean directory, link rc=0.

### Evidence

TIDE from `origin/main` `5175b02`: `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release`, **659/659, rc=0**, GMPI_Wrappers fetched at `0a791ad` (includes E71 and E88). Preset: `v1-rack.rpp`'s 18,893-byte `<Preset>` (sha256 `5d4aec4f…`). 400 blocks of 512 at 44.1 kHz. Three runs per arm, interleaved:

| arm | `building rack` | peak |
|---|---|---|
| state-first `--no-readback` | ×1 | **0.482431 (−6.3 dBFS)**, 3/3 |
| **activate-first `--no-readback`** | ×1, **after** `restore of a 14136 byte document -> imported`, following a 50-block pre-window at −inf | **0.482431 (−6.3 dBFS)**, 3/3 |
| `--no-preset` (negative control) | none | **0 (−inf)**, 3/3 |
| **control build**: `setFullState`'s `sendParameterToProcessorQueue(&param)` deleted, `--no-readback` | **none**, both orders | **0 (−inf)**, both orders |

**0.482431 is the same six digits as VST3 and CLAP** for this document. **The control is what makes the pass mean something**: delete one line and both orders play silence, so the probe can see E88's shape when it is there, and that line is the route. After restoring the line, the rebuilt appex is **byte-identical** to the first build (sha256 `51601bcf2c0d47f7`), so the A/B's only variable was the one line.

### The trap: reading `fullState` back is not passive

My first control run *still played*, in both orders. The probe read `fullState` back after setting it, which is S33's round-trip check. **`-fullState` runs `syncState()` (E68)**, and TIDE's `syncState` re-exports the document through the parameter path, which is a second delivery route to the DSP. The log showed it (`syncState exporting … (host asked for state)`, then a rack built from a "Sync chunk"). The probe now has `--no-readback`, and every figure above uses it. With the readback, both builds play in both orders (3/3 each, also recorded), and that measures nothing about `setFullState`.

**Learned:**

- **Check that a row's cited line is live code before porting a defect's shape to it.** E91's AU2 premise was a `setPresetUnsafe` call inside `#if 0`. The real AU2 restore has a *different* and wider gap. One `grep -n '#if 0\|#endif'` around the citation would have shown it on 10-06.
- **A "round-trip" readback can be a write.** `-fullState` calls `syncState()`, which re-delivers the document to the DSP. A probe that reads state back to prove it took can deliver the state itself, and that hides exactly the defect under test. It showed up only because the control build refused to go silent.
- **A control that will not fail is a finding about the probe, not about the code.** I expected the deleted-line build to go silent. It did not, and the cause was my own readback. Run the control *before* believing the pass.
- **An appex's own link line, minus `-e _NSExtensionMain`, is an in-process AUv3 host.** Running it from a copy of the appex bundle keeps resource lookup identical. The plug-in's stderr becomes visible and nothing is registered. That removes the 08-29 displacement problem for any AU3 question that does not need a real DAW.
- **The installed AUv3 on this box is pre-E71** (2026-08-31). Any measurement through `AudioComponent` here is of old code, not `main`. Check the appex binary's mtime against the wrapper's history before trusting one.
- **`timeout` is not on this box**; `perl -e 'alarm N; exec @ARGV' cmd …` is.

**Not verified:** **AU2**, by construction: TIDE has no AU2 target. E93's claims are a reading of `AU2_Wrapper.cpp`. **No real DAW and no out-of-process load**: the in-process host shares the appex's code but not the XPC boundary, so a host that restores across that boundary is not measured. iOS AUv3 was not tested. SynthEdit and SynthEditCL were not rebuilt, because nothing they compile changed (no wrapper edit landed; the control edit lived only in the scratch build's `_deps` and was reverted, and byte-identity proves it). **Observed, not investigated:** every run prints `SynthEdit: could not read factory.se.xml -- this plugin has no identity` (`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp`) because the built `TIDE-Rack.appex/Contents/Resources` has no `factory.se.xml`, and Jeff's installed appex has none either. It did not stop the rack building or playing. I filed no row, because I cannot yet say it is a defect.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean, and never left it. All work was in scratchpad `git worktree`s (TideSynth ×2, GMPI_Wrappers ×1, read-only) and a scratchpad build tree, all removed at the end. `SynthEdit` (`master`), `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers` and `GMPI` (`main`) were clean at the start and untouched. **Jeff's `~/Applications/TIDE-Rack-AUv3.app` and its registration were not touched.** No GUI and no screen taken. No credential value appears anywhere.

**Next:** see the `mac` NEXT cell. **For Jeff, two things:** (1) [#643](https://github.com/JeffMcClintock/TideSynth/pull/643) is `tests/` only and so allowlist-blocked by design. It closes E91's AU3 half. (2) **E93 wants a scope call**: AU2 is out of TIDE since S40, so it is WONTFIX unless GMPI_Wrappers' AU2 matters to another plug-in.

**Branch/PR:** the probe is on `tide/mac/E91-au-activate-first`, [#643](https://github.com/JeffMcClintock/TideSynth/pull/643), with `BACKLOG.md` byte-identical to `main`. This entry, the E91 flip, the E93 row, the `mac` cell, the rotation and the regenerated `docs/lessons.md` are on `tide/mac/2026-10-07-e91-bookkeeping`, which is bookkeeping-only and should auto-merge.

## 2026-10-06 — linux — the merge sweep Jeff asked for: all three PRs landed, five rows archived, and CI closed half of E88's own stated gap (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.32885.1** (CLI `2.1.220`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:` · **interactive continuation of this box's scheduled run, Jeff directing**

**Did:** Jeff read the scheduled run's report and said ***"resolve and merge outstanding PRs"***. All three open PRs across both repos are now MERGED, the five IN-REVIEW rows whose PRs had all merged are DONE and archived, and the `linux` NEXT cell is re-pointed.

**This is an interactive session, not the scheduled run, and the distinction is load-bearing twice over.** STEP 5 says *"Do NOT merge the PR"* and STEP 1.5 scopes a run to its own platform's PRs — I did both of the things those rules forbid, because Jeff asked for them in session. That is the same convention the 2026-09-07 merge sweep and the 2026-10-06 E90 entry record (*"Jeff read the run's report and said 'push to main branches'"*). **A scheduled run reading this entry must not copy it.**

### What merged, and in what order and why

| # | repo | what | note |
|---|---|---|---|
| [#42](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/42) | GMPI_Wrappers | E88's VST3 restore fix | merged FIRST, deliberately |
| [#640](https://github.com/JeffMcClintock/TideSynth/pull/640) | TideSynth | the probe's Linux loader | 13 pass + 2 skipping |
| [#639](https://github.com/JeffMcClintock/TideSynth/pull/639) | TideSynth | E86's ruling | conflicted; resolved, then 15 checks green |

**#42 first was the right order by luck as much as design, and it is the most useful thing in this entry.** TideSynth fetches GMPI_Wrappers at `GIT_TAG origin/main`, so once #42 landed, **the next CI run on any TideSynth branch built against the fixed wrapper.** #639's re-synced head was pushed at 02:06 UTC, one minute after #42 merged at 02:05, and its configure step ran at 02:08–02:09. Read out of the run log rather than inferred from the timestamps:

```
linux    Configure   --   GMPI_Wrappers <- .../build/_deps/gmpi_wrappers-src [fetched]
macos    Configure   --   GMPI_Wrappers <- .../build/_deps/gmpi_wrappers-src [fetched]
windows  Configure   --   GMPI_Wrappers <- D:/a/.../build/_deps/gmpi_wrappers-src [fetched]
linux    Build       [  8%] Building CXX object ... VST3_Wrapper.dir/Processor_VST3.cpp.o
macos    Build       [  6%] Building CXX object ... VST3_Wrapper.dir/Processor_VST3.cpp.o
windows  Build         Processor_VST3.cpp
```

`windows` **pass 10m44s**, `macos` **pass 3m41s**, `linux` **pass 1m58s**.

**So the largest gap the scheduled run declared — *"not compiled on Windows or macOS, and it is `#ifdef`-free shared code"* — is now closed on its compile half, by a CI run belonging to someone else's PR.** State the limit precisely, because it is easy to overclaim: **CI builds, it does not measure.** Nothing on Windows or macOS has run the probe's `--activate-first` arm, so the *behaviour* of the fix on those platforms is still unverified. Confirming it is a small mac/win job — the probe already loads a bundle on macOS.

### #639's conflict was one file and the two sides were disjoint

`BACKLOG.md` alone. #639 edits the **E86** row; `main`'s 10-06 linux bookkeeping flipped **E88** and added E91/E92. Git saw one hunk only because E86 and E88 sit two rows apart. **`tests/e80_vst3_feedback_probe.cpp` auto-merged** — #639 edits the `--no-controller` documentation, I had added the Linux loader, different regions — which is what the scheduled run's `git merge-tree` check between the two branch heads had predicted.

Resolved by keeping each row from the side that edited it. **Each side's copy of the other's row was the stale one**, which is the general shape of every conflict this fleet gets in `BACKLOG.md`.

### The archive sweep, and one lint-driven correction

Five rows were IN-REVIEW with every linked PR merged, so STEP 4's own rule applies (*"If you see an IN-REVIEW row whose PRs have all merged, flip it"*): **A41, E76, E85, E86, E88** → DONE, moved to `BACKLOG-DONE.md`. **I applied the mechanical rule and did not re-judge other lanes' evidence**, which stands as their entries record it. Two worth a note:

- **E76's Accept is a FORK**, and it is met on its second branch. The first branch (a linux `render-and-measure.py` figure) was never this row's outstanding debt — it was the other option — and the ruling that branch wanted became E90, which has itself landed and been archived.
- **E88 is DONE on its VST3 half only, and that is exactly why the scheduled run split E91 out of it** hours earlier. A row closing with an open question inside it is how a question gets archived; E91 carries AU2/AU3.

**`check-next-block.py` then caught the archive's blast radius, as it is built to:** archiving A41 and E76 turned *"(1) take A41"* in the `win` cell and *"then **E76**"* in the `linux` cell into instructions pointing at dead rows. Both are preserved history, so both were corrected in place with an explicit negation rather than rewritten — the precedent being the `linux` cell's own earlier *"E75 has since been archived DONE, so the preserved wording here is corrected in place"*.

**Reading that lint's source was quicker than guessing at it**, and the rule is worth knowing: a sentence is disarmed only by `do not | don't | never | rather than | instead of | not to | nor to`, and sentences split on `.` `;` ` -- ` ` — `. My first fix to the `win` cell passed by accident, because the words *"rather than an instruction"* happened to land in the same segment. The `linux` one failed because the offending phrase was a **different** E76 mention from the one I had patched — `then **E76**.` ending its own sentence. **Find every mention of an ID you archive, not the one the error names**: the lint reports the first failure, not all of them.

**Learned:**

- **Merge order decided what got verified, and nobody planned it.** Landing the wrapper fix before the next TideSynth CI run meant a PR from another lane compiled my change on two platforms I cannot build on. With a `GIT_TAG origin/main` dependency, *"which PR merges first"* is a question about test coverage, not just about conflicts.
- **"Compiled on three platforms" and "verified on three platforms" are different claims**, and CI only ever offers the first. The temptation to write the second is strongest exactly when the first arrives unexpectedly.
- **When two lints disagree, read the source of the one that is still failing.** `check-next-block`'s negation list and sentence splitter are eleven lines; two rounds of guessing cost more than reading them.
- **A lint reports its first failure, not its last.** I patched the E76 mention the error pointed at and the error did not move, because there were three more take-shaped mentions of E76 in the same cell. Grep for every occurrence of an ID you archive.
- **Archiving N rows has a blast radius of N NEXT-cell clauses**, and they are all in preserved text you may not rewrite. Budget for it; the fix is an inserted negation, not an edit.

**Not verified:** **the behaviour of E88's fix on Windows and macOS** — compiled there now, measured nowhere but Linux. I ran no probe in this session at all; every figure above is either CI's or quoted from the scheduled run's own measurement. I did not re-judge A41's, E76's or E85's evidence before flipping them. I did not delete the two stale remote branches (`tide/mac/issue-599`, `tide/win/E86-vst3-processor-factory`) — deleting a branch is destructive and was not asked for, and the second is genuinely orphaned now that #634 is closed unmerged. **#639 landed code and a ruling with no journal entry of its own**, which is the windows lane's STEP 4 debt, not something this entry can discharge for it.

**Machine state:** `~/TideSynth` stayed on `main`, clean, never left it; all work in a scratchpad `git worktree`, removed at the end. **`~/TideSynth`'s local `main` is 68 commits behind `origin/main`** and I deliberately left it there — fast-forwarding a sibling tree out from under another process is the second half of #583's collision. Every build in this session's scheduled half was in the scratchpad. No credential value appears anywhere.

**Next:** see the `linux` cell. **For Jeff:** nothing is waiting on you — no open PRs and no IN-REVIEW rows in either repo. The two stale branches want a yes/no before anyone deletes them.

## 2026-10-06 — linux — E88: the VST3 half is FIXED and measured 3/3, and the SDK's own annotation is what settles that the order is legal (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.32885.1** (CLI `2.1.220`) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E88** from the `linux` NEXT cell, ported the VST3 probe's loader to Linux, reproduced the defect here, and **fixed it** — the half the 10-05 macos run deliberately left unchosen. Filed **E91** (AU2/AU3, E88's unmet half) and **E92** (a build-system skip found on the way). `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`8876c9a..50633f8`).

### STEP 1 / 1.5 / 2

**STEP 1:** one open `platform:linux` issue, [#583](https://github.com/JeffMcClintock/TideSynth/issues/583), authored by `tide-rack-bot` — so it is evidence rather than unauthenticated input, and STEP 1 permits acting on it. **Its own first line says not to**: *"This is a PROCESS defect, not a build break … STEP 1's 'a broken build outranks all backlog work' tier does NOT apply — triage it into a BACKLOG row and take it in normal order."* It is already triaged as **E89**. So STEP 1 had no work. The other open issue is #44, the digest.

**STEP 1.5:** **no `tide/linux/**` branch and no linux PR existed.** Jeff merged almost everything since the 10-06 windows entry was written: #629, #631, #633, #634, #635, #636, #637 and #638 are all `MERGED`. The fleet's only open PR at the start of this run was [#639](https://github.com/JeffMcClintock/TideSynth/pull/639) (`tide/win/E86-controller-required`), which is windows'. Nothing for this lane to address.

**STEP 2.** The `linux` cell's named take-target is **E88**, and it was still eligible: `TODO`, `any`, not blocked, and **no branch or PR named it** — #633 had merged, which is what released it. I re-derived the rest of the walk rather than inheriting it: **X2** is `linux` and `TODO` and is **topmost**, but its own text says the remaining takeable half *"is Windows and macOS"* and the other half is a `SynthEditLib` decision, so there is nothing on it for this box; **E89** is this box's own process wound and is real work, but the NEXT cell binds first and E88 was eligible. None of the five open `PROPOSED:` entries reaches E88 — I read all five, and four say *"May proceed meanwhile: everything/EVERYTHING"* while E72's parks only E72.

### The row said the fix was a three-way choice. One reading of the SDK header collapses it

The 10-05 macos entry stopped here, and correctly by its own lights: *"I could not state the fix in one sentence."* Its three options were (a) `restartComponent(kReloadComponent)`, (b) `reInitialise()` from `setState`, (c) a flag consumed in `process()`. What unlocked it was **not** a cleverer option, it was reading the two annotations the VST3 SDK puts above the calls:

```
/** Activates / deactivates the component.
 * \note [UI-thread & Setup Done] */
setActive

/** Sets complete state of component.
 * \note [UI-thread & (Initialized | Connected | Setup Done | Activated | Processing)] */
setState
```
`pluginterfaces/vst/ivstcomponent.h:194-200`

**`Activated` and `Processing` are named as legal states for `setState`.** That settles two things at once, and they point in opposite directions:

1. **The order is not a host misbehaving.** This was a live question — the row says *"REAPER happens to call setState before setActive, and VST3 hosts conventionally restore state during setup"*, which left open whether the probe was exercising something no host may do. It may. So the defect is real and is TIDE's, not a probe artifact.
2. **It kills option (b) outright, rather than on a judgement.** `setState` is legal *during* `Processing`, so `reInitialise()` there is a guaranteed race, not a possible one. And it is worse than the row knew: **`start_processor` calls `factory->createInstance` and CONSTRUCTS A NEW PROCESSOR** (`gmpi-src/Hosting/processor_holder.cpp:48-80`), so (b) would swap the processor object out from under a `process()` call in flight.

### The cause, in GMPI's own words

`setPresetUnsafe` writes the parameter **stores** and nothing else — I read it to the end to be sure, and the only write is `param.setFromXml(v)` plus a reset pass. A store reaches a live DSP graph by exactly one route, a `PinSet` event in `gmpi_processor::events`, and the comment on `start_processor`'s seeding block says why that is a problem:

> *"Seed the pin with the parameter's CURRENT bytes, not a default. A processor can be created at any time … and without this it would start with an empty blob and never be told otherwise, **since blobs only reach it when they CHANGE**."*

**A restore is a change that nothing announced.** TIDE's whole patch is one blob parameter, so "nothing announced it" is the entire bug. That sentence was already in the tree and is a better statement of E88 than either row.

### The fix, and why it is option (c) with the objection removed

The row's objection to (c) was *"puts the rack build on the audio thread"*. **That is where TIDE already builds the rack**, and it says so:

> *"The rack asked for a rebuild … Consume it at a block BOUNDARY, never mid-process … **Same synchronous audio-thread cost as a chunk arrival**, and far rarer."* — `SynthEditSem/SynthEdit.cpp:588-603`

So (c) adds no new thread hazard; it reuses the one TIDE ships. And the shape is not even new to GMPI: **`gmpi_processor::onQueMessageReady`'s `"ppc3"` arm — "Patch parameter change, blob payload" — already does exactly this on the audio thread**, ending in `sendParameterToProcessor`. `process()` polls that queue on its first line.

So: `setState` sets an atomic flag; `process()` test-and-clears it at the top and calls `sendParameterToProcessor` for each parameter. **This makes a state restore look to the DSP like the live parameter change it already knows how to receive**, and introduces no threading that was not already there. One sentence, which is what the row asked for.

Two details that are not decoration:

- **Empty blobs are skipped**, exactly as `start_processor`'s seeding skips them (*"nothing stored yet; a later change will deliver it"*). This also keeps the loop off a real hazard: `sendParameterToProcessor`'s Blob arm uses the **throwing** `std::get`, where the startup path uses `std::get_if`. An exception there would be on the audio thread.
- **Every parameter, not only the ones the preset mentioned**, because `setPresetUnsafe` also resets absent parameters to their defaults — those stores moved too and are just as unannounced.

`gmpi_processor` is a `struct`, so `sendParameterToProcessor` and `patchManager` are public and **the whole fix fits in GMPI_Wrappers (ALLOWED)**. No GMPI change, which matters: GMPI is PR-GATED and the first shape I reached for — having the controller announce the restore — would have landed there.

### Verification

One build tree, **one TU apart**: only `wrapper/VST3/Processor_VST3.{cpp,h}` differ between the two binaries. `v1-rack.rpp`'s 18,893-byte preset via `scripts/decode_rpp.py --preset-out`, 400 blocks of 512 at 44.1 kHz, **3 runs per arm, interleaved**:

| arm | baseline `f43ce2d6` | fix `f5923fd3` |
|---|---|---|
| state-then-activate | −6.3 dBFS, `building rack` ×1 | −6.3 dBFS, `building rack` ×1 |
| **activate-then-state** | **−inf, NO `building rack` line** | **−6.3 dBFS, `building rack` ×1** |
| activate-then-state `--no-pump` | **−inf, NO `building rack` line** | **−6.3 dBFS, `building rack` ×1** |
| `--no-preset` (negative control) | −inf | −inf |

Peak is `0.482431` in every passing arm — **the same six digits macOS measured on 10-05**, and the same as the CLAP reference. `restartComponent` is called **0** times in every arm before and after, which is the practical argument against option (a): this needs no host cooperation at all.

**The negative control is the arm that makes the rest mean anything.** A fix that simply always built a rack would turn `--no-preset` green too; it stays silent. That is the same discriminator the 10-06 windows run built its eight-arm probe around, and it is worth imitating every time.

**The rebuild is byte-identical.** Restoring the patch and rebuilding reproduced `f5923fd3d7beac02` exactly, so the A/B's only variable really was the one TU — the cheapest available proof, per the 2026-09-01 lesson.

**CLAP regression control, same build:** `e79_clap_headless_probe.c --activate-first` still renders −6.3 dBFS with one rack build. The change is VST3-only and E79's CLAP fix is unaffected.

**Consumers built, because GMPI_Wrappers is shared and STEP 5 asks:**

| consumer | result |
|---|---|
| TIDE — `TIDE-Rack.vst3`, `.clap`, standalone | **562/562, rc=0**, 0 `error:` lines |
| **SynthEdit's Linux VST3 export template** (`SynthEditTemplate_VST3` → `se_vst3_linux.dat`) | **238/238, rc=0** |
| **SynthEditCL** | **76/76, rc=0** |

`main` **builds on Linux**: the baseline arm is `origin/main`'s wrappers exactly, and it configured and built rc=0. No platform issue needed filing.

### E92: the one variable you set to test a wrapper change is the one that deletes its SynthEdit-side consumer

Found while trying to satisfy *"rebuild SynthEditCL as well as TIDE"*, and it is a silent skip of the shape this project keeps paying for.

`se_gmpi/vst3/CMakeLists.txt:21` guards SynthEdit's Linux VST3 export template with `if(NOT gmpi_wrappers_POPULATED) … return()`. `gmpi_wrappers_POPULATED` is set only by `FetchContent_MakeAvailable(gmpi_wrappers)` — and **the `GMPI_WRAPPER_FOLDER_OVERRIDE` branch of `SynthEditSem/CMakeLists.txt:44-50` deliberately does not call it**, it just sets `GMPI_ADAPTORS`. Measured, counting ninja targets:

| configure | `SynthEditTemplate_VST3` targets |
|---|---|
| no override | present |
| `-DGMPI_WRAPPER_FOLDER_OVERRIDE=<tree>` | **0** |
| `-DFETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS=<tree>` | 10 |

It prints `SynthEditTemplate_VST3: skipped (gmpi_wrappers not populated)` and exits 0, so a configure log scanned for errors shows nothing. **`FETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS` is the variable that works** — it keeps `_POPULATED` true *and* redirects the source — which is what the 10-02 macos lesson already said and I now know the reason for. Filed as **E92** rather than fixed: `se_gmpi/` is in `SE16` and on neither STEP 5 list, so GATED by default, and this is not a build break.

**This is why my SynthEditCL and template builds used `FETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS` and my TIDE A/B used `GMPI_WRAPPER_FOLDER_OVERRIDE`** — TIDE's own targets carry no such guard, so the override is sound there; SE16's are not, and had I used the override for both I would have reported "SynthEditCL builds" on a configure that had silently dropped the only target that compiles my change.

### Bookkeeping

- **E88 → IN-REVIEW for the VST3 half, and the AU2/AU3 half filed as E91.** The row's Accept says *"for each wrapper"*, and AU2/AU3 are neither fixed nor measured, so leaving the question inside a closing row would archive it with the row — **A42's shape**, which the windows lane applied to E76/E90 on 10-06. Ids allocated by A42's guard: highest `E` is **90** across `origin/main` and all four remote `tide/*` branches, so E91 and E92 were free.
- **Three PRs, because the work spans two repos and the lane's allowlist splits the third.** `automerge_eligible.py` on `tests/e80_vst3_feedback_probe.cpp` is **rc=1** (*"not on the auto-merge allowlist"*) and on `BACKLOG.md`/`JOURNAL.md`/`docs/lessons.md`/`JOURNAL-2026-10.md` is **rc=0**, so code and bookkeeping cannot ride together without parking the bookkeeping behind a human. `BACKLOG.md` on the code branch is **byte-identical to `origin/main`**.
- **The DOING mark was pushed first** (`bf9851d`, before any work) and removed at the end, so the claim was visible for the whole of the run.
- **STEP 3's grep before filing:** no row on `origin/main` names `GMPI_WRAPPER_FOLDER_OVERRIDE`, `se_gmpi/vst3` or `SynthEditTemplate_VST3`. E85 mentions `FETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS` only as its own A/B artifact, which is not this job.
- **`docs/lessons.md` was stale on `origin/main` again**, exactly as the 10-06 macos entry predicted it would be once one of its two PRs landed (*"extract-lessons.py --check is not a lint-workflow step, so nothing will flag that"*). `--check` said *"stale -- run --write"* before I touched it. Regenerated here, so that debt is cleared as a by-product rather than left for another run to find.
- `gh pr edit` **fails for this credential** — it asks for `read:org` to resolve reviewer logins, and the fleet token is `repo`-only by design. `gh api -X PATCH repos/.../pulls/<n> --input <json>` does the same job. Worth knowing before someone treats it as a broken token.

**Learned:**

- **When a row calls a fix a three-way choice, read the SDK's own annotation before picking.** `[UI-thread & (… | Activated | Processing)]` on `IComponent::setState` both proved the defect real and eliminated one option by making the race certain rather than arguable. Two days of "mechanism choice" was one header comment.
- **The best statement of this bug was already a comment in the tree.** GMPI's *"blobs only reach it when they CHANGE"* is the whole of E88, written by whoever added blob seeding. **Grep the code you are about to change for a comment that already describes your bug** — it is faster than reasoning and it is evidence.
- **An objection of the form "that would put X on the audio thread" needs checking against where X already runs.** TIDE already rebuilds its rack on the audio thread at a block boundary and documents the cost as accepted. The row's objection to option (c) was true of the words and false of the program.
- **`start_processor` does not restart a processor, it constructs a new one.** Any fix phrased as "just call reInitialise again" is swapping a live object out from under `process()`. The name actively misleads here.
- **`sendParameterToProcessor`'s Blob arm uses throwing `std::get` where the startup seeding uses `std::get_if`.** Two code paths that seed the same pins disagree about whether an unset blob is an error. A new caller that iterates ALL parameters meets the throwing one on the audio thread.
- **`GMPI_WRAPPER_FOLDER_OVERRIDE` silently deletes SynthEdit's VST3 export template from the build**, because the guard tests `gmpi_wrappers_POPULATED` and the override path never populates. The variable you set in order to test a wrapper change removes the consumer you most need to compile. Use `FETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS`.
- **`check-id-refs.py` and `check-backlog-diff.py` pull in OPPOSITE directions on a row split, and only one edit satisfies both.** Filing E91 out of E88 made two live rows cite `AU2_Wrapper.cpp:341`, which `check-id-refs` fails. Dropping it from E88 then failed `check-backlog-diff`, whose rule is that a flipped row's base Item text must still be present **verbatim** — so the row I was allowed to edit was the one I was not allowed to shorten. **The new row is the one that has to yield**: E91 names the file without a line number and says why. Worth knowing before the next split, because the first fix looks obviously right and turns the other check red.
- **`nohup cmd &` inside the Bash tool reports the SHELL's exit, not the command's, and the command keeps running.** I read "exit code 0" as "build finished", restarted it, and had **two ninja processes in one build tree**. Use the tool's own backgrounding; a second builder in one tree is unrecoverable by inspection, so I cleaned 168 objects and rebuilt rather than trust them.
- **`pkill -f '<pattern>'` matched my own shell and killed it with exit 144 — the fifth time this fleet has paid for that**, and `scripts/kill-named.sh` has existed since the third. The pattern was `ninja -j 6`, which appears in my own `bash -c` command line. **`pkill -f` sees the command that is running it.**
- **A command ending in `grep -c` reports the TASK as failed when the count is zero.** Three of my background steps came back "failed with exit code 1" on builds that had exited 0. The journal already carries this lesson; it is cheap to re-learn and cheaper to avoid by putting `; true` or the real check last.

**Not verified:** **no Windows or macOS compile of either change**, and the wrapper fix is `#ifdef`-free shared code, so it reaches both platforms unmeasured — that is the largest gap in this run and the reason the PR says so in its own words. **AU2 and AU3 are not fixed and not measured** (E91); this build has no `.component`. No DAW was launched, no window opened and no screen taken — the probe needs none. `--editor` is still unimplemented on Linux, so nothing here speaks to the editor-attached case. SynthEdit's own GUI application has no Linux target, so *"SynthEdit builds"* is attested only through `SynthEditCL` and the export template. I did not re-run any committed fixture through `render-and-measure.py`, so this run says nothing about E76's or E90's recent changes. I did not verify the fleet PAT's expiry (A40's `NEEDS-JEFF` half, still open).

**Machine state:** `~/TideSynth` started and ended on `main`, clean, and **never left it** — all work in three scratchpad `git worktree`s (two in TideSynth, one in GMPI_Wrappers), all removed at the end, with every build tree in the scratchpad. **I fast-forwarded nothing and touched none of Jeff's checkouts**, which is deliberate given #583: `~/SE/GMPI_Wrappers` was clean on `main` at `4c11d6d` (behind `origin/main`) and I **left it there**, working from a worktree at `origin/main` instead — the 09-09 collision's second half was a run fast-forwarding five sibling repos out from under another run. `~/SE/SE16` was clean on `master` and was read only; its two scratchpad configures wrote nothing into it. **One concurrent-run check at the start** (`ps` for a second `claude` CLI): exactly one, this run — so #583's duplicate firing did not recur today. No credential value appears in any commit, PR, journal entry or row.

**Next:** see the `linux` NEXT cell. **For Jeff, three things:** (1) **[GMPI_Wrappers#42](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/42) is the fix and [#640](https://github.com/JeffMcClintock/TideSynth/pull/640) is the instrument** — they are independent, and #42 is the one that changes shipped behaviour in every VST3 host, so it wants a Windows or macOS build before it lands. (2) **E91** is E88's AU2/AU3 remainder and **E92** is the build-system skip above; neither needs a ruling. (3) [#639](https://github.com/JeffMcClintock/TideSynth/pull/639) (windows, E86's ruling) is still open and is a decision rather than a review.

**Branch/PR:** `tide/linux/E88-vst3-activate-first` in **two** repos — [GMPI_Wrappers#42](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/42) (the fix) and [#640](https://github.com/JeffMcClintock/TideSynth/pull/640) (the probe's Linux loader, `BACKLOG.md` byte-identical to `main`). This entry, the E88 flip, the E91 and E92 rows, the refreshed `linux` cell, the rotation and the regenerated `docs/lessons.md` are on `tide/linux/2026-10-06-e88-bookkeeping`, which is bookkeeping-only and should auto-merge.

