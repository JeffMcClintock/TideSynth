# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-31 — linux — X1 closed by Jeff's ruling: the blocker was never written down (state update, interactive)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths) · interactive continuation, Jeff directing (*"what is x1 about?"*, then *"mark it DONE"*)

**Did:** **X1 → DONE and archived.** Bookkeeping only; no code, no measurement this entry is claiming credit for.

### The row, in full, from the repo's first commit

```
| X1 | BLOCKED | linux | VST3 + CLAP on Linux, GCC 13+. See the Linux toolchain memory for WSL specifics. |
```

That is `b2b1466`, *"Scaffold TIDE Synth coordination repo"*, and it is the entire original specification. It carried a **bare `BLOCKED` with no `(id)`**, under **After the carve-out** — so the implicit blocker was **C7**, C7 went DONE, and nothing anywhere said so.

### Why it took a human, and both refusals were correct

Its Accept has been met since at least 2026-08-27, measured rather than assumed on both occasions: GCC 13.3.0, 492/492 rc=0 then; **553/553, 0 errors** today on a fresh `TIDE_VCV_FUNDAMENTAL=ON` tree, producing both artifacts — **and both were driven, not merely linked.** The VST3 was hosted in REAPER 7.43 under headless weston with the transport rolling 75 s ([#566](https://github.com/JeffMcClintock/TideSynth/pull/566)); the CLAP went through `clap_plugin_state` load/save via `tests/e60_clap_state_probe.cpp` ([#550](https://github.com/JeffMcClintock/TideSynth/pull/550)).

Three linux runs in a row noticed and none flipped it. STEP 2: *"NEVER start a BLOCKED item, even if you think the blocker is stale... say so in the journal and stop."* The 2026-08-27 run added a second reason of its own — it was claiming X2, and *"a status change on a row it did not take is exactly the kind of drive-by edit that makes a queue untrustworthy."* Both are the rules working, and together they made the deadlock structural: **the only actor permitted to break it was Jeff.**

**Learned:**

- **A bare `BLOCKED` is unfalsifiable by construction, and the queue has no way to notice.** `BLOCKED(<id>)` can be re-checked by any run in one command; `BLOCKED` can only be re-checked by the person who wrote it, and after a while not even by them. Prefer the parameterised form, and a row whose blocker cannot be named probably wants `NEEDS-JEFF` — which at least says *who* is owed.
- **Two individually correct rules can compose into a deadlock that neither one describes.** "Never start a BLOCKED row" and "never edit a row you did not take" are both right and both worth keeping; their intersection is a row no agent may ever touch. Worth knowing that the fleet can manufacture these, because nothing in the process detects one.
- **Ask what the row is FOR before proposing a status.** The answer here was one line from the repo's first commit, and reading it is what turned "the blocker looks stale" into "the blocker was never written down" — a different claim, and the one that got a ruling.

**Not verified:** nothing new — this entry measures nothing. The build and host evidence it cites belongs to the two entries below it.

**Machine state.** All six repos on their default branches, clean; nothing running.

**Next:** **E74** remains the top of the linux lane, and **the linux CLAP cell of E19 is newly measurable** now the 32 KB cliff is off `main`.

**Branch/PR:** `tide/linux/X1-done` — the flip, its archive row, the `linux` NEXT cell, and this entry.

## 2026-08-31 — linux — the merges, and E60's fix measured after it had already landed (interactive continuation, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths) · interactive continuation of the scheduled run below, Jeff directing (*"resolve conflicts and merge"*)

**Did:** merged this box's three open PRs, resolved the one conflict the entry below predicted, and **flipped E60 to DONE**. Scope was deliberately my own PRs: [#565](https://github.com/JeffMcClintock/TideSynth/pull/565) is the mac box's E73 work and was left alone.

### Both E60 PRs had already auto-merged, within a minute of becoming eligible

[#550](https://github.com/JeffMcClintock/TideSynth/pull/550) merged at **04:42:02Z** and [GMPI_Wrappers#32](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/32) at **04:41:43Z** — both while I was still building #32 to check it. #550 had been CONFLICTING for three days; resolving it made it eligible and the docs-only allowlist took it, and #32 went with it.

**So nothing gated #32 on a build, and that is worth saying rather than presenting what follows as if it were a gate.** It is a product-code change in a repo with no CI. The measurement is post-hoc, and it passes.

`tests/e60_clap_state_probe.cpp` — which #550 itself had just landed — against two CLAP binaries differing by exactly #32, same commit of everything else:

| build | 51,690-byte `e53-vcv-rack-segv.xml` | 18,893-byte preset |
|---|---|---|
| `main` without #32 | **FAIL** — `load` false, **32,512 of 51,690** consumed, save falls back to the 86-byte default | PASS, 18,662 back |
| with #32 | **PASS** — 51,690 of 51,690, saves **51,630** | — |

**32,512 is the old `maxSize - chunkSize - 1` cliff to the byte.** The small preset passing on the *same pre-fix binary* is the positive control that stops the FAIL reading as a broken probe. The BEFORE binary was free: it was the copy installed into the scratch `HOME` an hour earlier, before the rebuild.

**Consumers:** TIDE **553/553** then **36/36**, 0 errors on linux. SynthEdit consumes only `se_gmpi/vst3` from GMPI_Wrappers and this change is confined to `wrapper/CLAP/`, so the SynthEditCL rule is discharged by scope, not by a build.

### The predicted conflict, and a near-miss resolving it

#566 went CONFLICTING the moment #550 landed, on exactly the one line the entry below said it would — the `linux` NEXT cell — plus `docs/lessons.md`, which is generated and was regenerated rather than merged.

**The near-miss is the part worth writing down.** My first archive attempt put a markdown TABLE inside E60's row, i.e. newlines inside a table cell, and `check-backlog-diff.py` correctly refused: a row that is no longer one line cannot be matched verbatim against its source. Reaching for `git checkout ORIG_HEAD -- BACKLOG.md` to start over then **silently reverted #550's own E60 row**, because ORIG_HEAD is the pre-merge branch tip and that row only exists on main. Caught by grepping for the row rather than by any lint. `git checkout --merge -- <file>` re-creates the conflict markers and is the right way back — and note it writes `<<<<<<< ours` / `>>>>>>> theirs`, not `HEAD` / `origin/main`, so a resolver script that pattern-matches the marker text silently matches nothing.

**Learned:**

- **A PR you resolved may merge before you finish checking it.** Auto-merge fires on eligibility, not on your intent, and a docs-only allowlist can pull a sibling repo's code PR along in the same minute. If a build is meant to gate a merge, it has to happen before the resolution, not after.
- **Say "post-hoc" out loud when verification arrives after the merge.** The numbers are just as true and mean something different; a row that presents them as a gate is lying about its own process.
- **Keep the superseded binary — it is the A/B for free.** The pre-fix CLAP was sitting in a scratch install directory from an earlier step, so the control cost one command instead of a second build tree.
- **A markdown table cannot go inside a table cell, and the archive lint is what catches it.** The row stops being one line and no longer matches its source verbatim, which is exactly the property the lint exists to protect.
- **`git checkout <ref> -- <file>` during a merge is not "undo".** It resolves the path to that ref's content, discarding the *other* side's changes outside the conflict hunk — here, another PR's row. `git checkout --merge -- <file>` is the undo.
- **Conflict marker text depends on how the conflict was produced.** `--merge` writes `ours`/`theirs` where the original merge wrote `HEAD`/`origin/main`; my resolver script matched neither and raised `NoneType has no attribute 'group'` rather than doing something wrong quietly, which is the only reason this is a footnote.

**Not verified:** #32 in a real CLAP host — the probe is deliberately the C ABI with no DAW, and the linux CLAP cell of E19 is now measurable and unmeasured; whether #32's larger loads behave on Windows or macOS.

**Machine state.** `GMPI_Wrappers` was briefly on a `verify-32` branch for the A/B build and is back on `main`, fast-forwarded, clean; the branch is deleted. All six repos on their default branches, clean. Nothing running. `build-e19/` is gitignored and now carries #32.

**Next:** **E74** is still the top of the linux lane. **The linux CLAP cell of E19 is newly measurable** now that the cliff is gone, and the harness in [tests/e19-host-feedback/](tests/e19-host-feedback/) mints its own project. **X1 still wants Jeff** — its `BLOCKED` mark has been stale since 2026-08-27 and no run may start it.

**Branch/PR:** `tide/linux/E19-vst3-linux-cell`, [#566](https://github.com/JeffMcClintock/TideSynth/pull/566) — the merge commit, E60's flip to DONE, the refreshed `linux` NEXT cell, and this entry.

## 2026-08-31 — linux — STEP 1.5 unblocked #550, then E19's linux VST3 cell: the DSP now runs the right rack, and the editor is bound to the wrong processor (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required

**Did:** two things, in the order the prompt puts them. **STEP 1.5:** this platform's only open PR, [#550](https://github.com/JeffMcClintock/TideSynth/pull/550), had gone **CONFLICTING** while it sat for three days; resolved and pushed, it is `MERGEABLE` again. **STEP 2:** took **E19**'s linux VST3 cell, whose own text said *"do not re-take this cell until E59 closes"* and E59 closed on 2026-08-29. Branch `tide/linux/E19-vst3-linux-cell`. **Cell back to TODO; E74, E75 and E76 filed.** No product code changed on either branch.

### STEP 1.5 first, because a conflicted PR is not "waiting for merge"

STEP 1.5 lists failing checks, requested changes and unresolved comments. #550 had none of those — all 13 checks green, no reviews — and could not merge anyway. The 2026-08-28 macos entry already recorded that *"a conflict is not on STEP 1.5's list of three, and should be"*; this is the second time it has been the whole first half of a run.

All three conflicts were in the fleet's bookkeeping files, and the resolution is by date and ownership rather than by side:

- **`JOURNAL.md` — main's copy verbatim.** Main rotated every 2026-08-28 entry into the archive while #550 sat open, so the branch's own E60 entry was the only thing missing. It moved into `JOURNAL-2026-08.md` between the two 08-28 windows entries it sat between on the branch. Checked rather than assumed: of the branch's 35 entries, **exactly one** was absent from both main's `JOURNAL.md` and the archive.
- **`docs/lessons.md` — regenerated**, not hand-merged. `scripts/extract-lessons.py` reads both journal files, so the correct content is a function of the other two resolutions.
- **`BACKLOG.md`** — mac NEXT cell from main (2026-08-31, three days newer), linux NEXT cell from the branch (same day, "later"); E59 stays archived as main has it; E60 takes the branch's IN-REVIEW row.

`GMPI_Wrappers`[#32](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/32), E60's other half, is `MERGEABLE`/`CLEAN` with nothing unresolved, so STEP 1.5 says leave it — **and it still matters**: `Processor_CLAP::stateLoad` on `main` still has the `maxSize = 4096 * 8` cliff, unchanged by #36 and #37, which touched the same function.

### Then E19, and the first half of the result is that two of this row's claims are now false

**The DSP runs the PREPARED rack.** `TIDE: instance #3 building rack from 43191 byte document`, twice, and the 17,957-byte default never appears after the restore. On 2026-08-28 the same path gave `29101` then `17957`, which is the observation E59 was filed for.

**And `tests/hosts/v1-rack.rpp` makes sound on Linux for the first time.** PLAN cites that fixture for *the patch plays after reload*; it was **peak −inf, digital silence** on this box on 2026-08-28.

| fixture | linux, 2026-08-31 |
|---|---|
| `--control` (no plug-in at all) | −6.0 / −9.0 dBFS — the chain detects audio |
| `v1-rack.rpp` | **−6.3 / −17.0 dBFS**, 2 patch cables — the macOS reference to the decimal |
| `v1-rack-uncabled.rpp` | **−inf**, 0 patch cables — the negative control |
| `v3-midi-pitch.rpp` | −6.2 / −21.1 dBFS |
| `v3-midi-gate.rpp` | −6.3 / −21.2 dBFS |

### The FAIL that is left, and its mechanism is ordering rather than any single line

75 s, transport rolling the whole time (`playstate=1`, position 0 → 74.919 — the control that separates "the plug-in is frozen" from "nothing is being processed").

| | hosted VST3, REAPER 7.43 | STANDALONE (control) |
|---|---|---|
| `Scope display-state capture` | **#2100**, 65,548 bytes, still climbing | #1800 |
| `display-state update … arrived` | **frozen at #1, 0 bytes** | **#1820, 65,548 bytes** |
| `light … update` | **frozen at #2, value 0.000** | **#18100**, values varying |

Same build, same document, same box, same compositor. The log order says why:

```
TIDE: instance #3 building rack from 43191 byte document
RackProcessor: 'Scope' display-state capture #0 (65548 bytes)
RackEditor: light 0 update #0 value 0.000          <- the editors' initial defaults
RackEditor: display-state update #1 arrived (0 bytes)
TIDE: instance #4 building rack from 43191 byte document    <- a SECOND processor
RackProcessor: 'Scope' display-state capture #0 … #2100      <- and it runs alone
                                                   (no RackEditor line ever again)
```

**The standalone builds twice as well** — `Legacy chunk`, then `Build chunk, rack already prepared` — **but as `instance #1` both times.** So a double build is not the defect; the changing instance is, and the editor's feedback pins are left attached to a processor the host has retired. Filed as **E74**. It is not E59 (the document is right, both times) and not the ui→dsp direction E64 fixed.

### The number I will not let anyone quote, and the control that disarmed it

The hosted pixel diff is **0 of 690,800**, which is exactly what this row's Accept calls a FAIL. **It is not evidence, because the standalone control over the same interval gives byte-identical screenshots** — while its counters are at #18100.

The reason is the fixture. All five VCV editors construct with panel art in **both** arms (`RackEditor: 'Scope' model=yes art=yes(res/Scope.svg) art-size=195x380`), and none of them is on the visible rack page; vertical and horizontal scrolling did not reach them. The negative control that makes this the fixture's layout rather than a rendering fault: the **DEFAULT** rack in the same build draws its `Out` panel on the rails. So E19's pixel-diff and int/bool/enum clauses are unmeasured and want a fixture with a visible Scope — **E75**.

### Two traps that each cost a wrong provisional conclusion

**`render-and-measure.py` segfaults REAPER on Linux from a scheduled run's shell** — rc **−11**, the documented inherited-`WAYLAND_DISPLAY` crash — and the downstream symptom is an `EOFError` in Python's `wave` module on a zero-length render. I read that as the committed fixture's macOS token being rejected, wrote it down, and it was wrong: with `env -u WAYLAND_DISPLAY … GDK_BACKEND=x11` the same file renders −6.3/−17.0. **E29's divergence is real for what a host WRITES and did not stop a fixture being READ here.** Filed as **E76**.

**The standalone's config folder is `TiDE Rack`, lower-case `i`**, and `tests/fixtures/e53-vcv-rack-segv.README.md` said `TIDE Rack`. Following it loads the DEFAULT rack and says nothing — measured as `building rack from 17961 byte document` against `38658` once the file moved one directory. Corrected at its source.

### The harness is in the tree this time

[tests/e19-host-feedback/](tests/e19-host-feedback/) — the 2026-08-28 run built the REAPER-on-weston recipe and left its drivers in a session scratch that did not survive, which is this repo's own lesson arriving for the second time. The piece worth having beyond E19 is `frame_chunk.py`: the `vst_chunk` framing **measured off a default instance** (140 base64 chars, 105 bytes, `int32 len+4 | int32 1 | int32 len | XML | 8 zero bytes` — no 44-byte header, no trailer), and a mint route that adds the plug-in **by name** and then sets the parm, so REAPER writes its own token. It wrote `1013510754{506C7567696E474D50492050A2A07287}` unprompted, and **E29 cannot be got wrong by construction** that way.

**Build:** `TIDE_VCV_FUNDAMENTAL=ON`, `-DRACK_ADAPTOR_TRACE=1`, Release, `SE_LOCAL_BUILD=OFF` — **553/553, 0 errors**, all four artifacts, against `main` in all five sibling repos. Verified to contain what this run needed before believing any of it: `display-state update #` and E59's `declined to publish the startup default` are each present once in the standalone, the VST3 `.so` and the CLAP.

**Learned:**

- **A CONFLICTING PR is not "green and waiting for merge", and STEP 1.5's list of three does not name it.** Second run in four days where that was the entire first half. `mergeStateStatus` costs one field on a `gh pr view` that STEP 1.5 already makes you run.
- **Resolve a rotated `JOURNAL.md` by taking main whole and re-placing your own entry in the archive.** The merge conflict looks like a text problem and is a bookkeeping one; the check that makes it safe is set arithmetic — which of the branch's entries are in neither of main's two files — and it printed exactly one.
- **A generated file is not merged, it is regenerated.** `docs/lessons.md` conflicted in two places and `extract-lessons.py --write` settled both, because its content is a function of the files the other resolutions produced.
- **A frozen readout and an unattached listener look identical, and only the ORDER of the log separates them.** Every counter in this run was correct about what it could see. The finding is in which line comes after which, and no single line carries it.
- **When a control gives the same "failing" number as the experiment, the number is not about the experiment.** A 0-pixel diff was E19's own FAIL condition; the standalone's byte-identical screenshots turned it into a statement about the fixture. Run the control even when — especially when — the result already looks like the answer you expected.
- **A crash can present as a corrupt output file two layers away.** REAPER's rc −11 reached me as `EOFError` inside `wave.py`, and I had already written down "the token is rejected on linux" before reading the render log. The log was two lines from the exception.
- **Read a verb's usage before reporting that it ignores its arguments.** `--scroll 500,300 0,-5` reported `delta 120, horiz false` three times and I was one sentence from filing a harness gap; the real syntax is `--scroll <x,y> [--notches N] [--delta N] [--horiz]` and it works.
- **A folder name that differs by one letter's case fails silently and looks like a broken fixture.** `TIDE Rack` vs `TiDE Rack`: the app loads its default, logs nothing unusual, and the fixture sits one directory away. The document's own `standalonePlugin` attribute is still the OTHER spelling, and both are correct in their own place.
- **A NEXT cell has to live on the branch its targets live on, and two lints enforce that.** The `linux` cell also belongs to #550's diff, so I tried to update it there — and `check-next-block.py` and `check-id-refs.py` both refused, because E74/E75/E76 and `tests/e19-host-feedback/` exist only on #566. They were right, and it settles the question the 2026-08-31 macos entry raised as a judgement call: the cell goes where its targets are, the conflict is one line, and the cell says in its own text which side to take.
- **Two of these lints passed on a NEXT table I had just destroyed.** My first edit ate the `linux` row's Take column entirely; `check-next-block.py` and `check-backlog-diff.py` both said OK, and the only tell was the row COUNT dropping from 4 to 3 in the lint's own summary line. Read the count, not the verdict.

**Not verified:** the linux **CLAP** cell — E60 owns it and its fix is [GMPI_Wrappers#32](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/32), unmerged, so `main` still carries the 32 KB `stateLoad` cliff; whether **E74** reproduces on Windows or macOS (nothing about a processor recreation is platform-specific, but neither box has read these counters in a host); the E74 **fix**, entirely — the row names where to look and does not guess; E19's **int/bool/enum**, **pixel-diff** and **string** clauses, all three for reasons recorded above; whether the e53 fixture's modules are reachable by any view gesture at all, which is half of E75.

**Machine state.** All six repos were clean and on their default branches at the start; the five siblings were fast-forwarded to `origin/main` (GMPI 1 commit, gmpi_ui 1, GMPI_Wrappers 6, SynthEditLib 9, SE16 2) and **none was committed to**. TideSynth is on this run's branch until STEP 5 returns it. **REAPER 7.43 was downloaded fresh** — the 2026-08-28 copy lived in that session's scratch and is gone — and ran only against a scratch `HOME`, so `~/.vst3`, `~/.clap` and `~/.config/REAPER` were never written; `~/.config/REAPER` still does not exist, and `~/.vst3` and `~/.clap` compare identical to the pre-run listing, TIDE absent from both. The standalone ran under a scratch `XDG_CONFIG_HOME`; `~/.config/TiDE Rack/` is untouched. `build-e19/` is a scratch build tree and is gitignored; Jeff's `build/` was not touched. `decode_rpp.py` wrote `tests/hosts/v1-rack.rpp.block0.param1.xml` and `v1-rack-uncabled.rpp.block0.param1.xml` as side effects; both removed. Headless weston, REAPER and the standalone all stopped via `scripts/kill-named.sh` — 0 of each left running.

**Next:** **E74 is the whole of E19's linux VST3 cell now**, and its harness needs no authoring — it mints its own project. **E75 is cheap and unblocks two more of E19's clauses.** **#550 and GMPI_Wrappers#32 want Jeff's merge**; until #32 lands the linux CLAP cell cannot be measured at all. And **the same 553-target build is sitting in `build-e19/`**, so any further linux measurement is minutes rather than an hour.

**Branch/PR:** `tide/linux/E19-vst3-linux-cell`, [#566](https://github.com/JeffMcClintock/TideSynth/pull/566) — the harness in [tests/e19-host-feedback/](tests/e19-host-feedback/), the E19 row, E74/E75/E76, the linux sections of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), the audio table in [tests/hosts/README.md](tests/hosts/README.md), the folder-name correction in [tests/fixtures/e53-vcv-rack-segv.README.md](tests/fixtures/e53-vcv-rack-segv.README.md), and this entry. Plus the merge commit on `tide/linux/E60-clap-state-trace` ([#550](https://github.com/JeffMcClintock/TideSynth/pull/550)), which is the STEP 1.5 half.

## 2026-08-31 — macos — E19's mac AU3 cell: a DAW has now hosted TIDE's AUv3, and the half that is still unmeasured has a structural cause (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** · as **tide-rack-bot** (both paths) · interactive continuation of the same session, Jeff directing (*"merge your PRs. sync all tide related repos"*, then *"then take the next task"*)

**Did:** took **E19**'s mac AU3 cell — the topmost eligible row, and the one this box alone can measure — after both of its blockers lifted in the same minute: the screen was unlocked, and Jeff was present to authorise the one step an unattended run must not take. **A DAW has now hosted TIDE's AUv3, the first time on any box.** Branch `tide/mac/E19-au3-registered`. No product code changed.

### The registration wall came down exactly where 2026-08-29 said it would

That run measured five ways to register a current build **beside** the developer's — launching the built app, `pluginkit -a`, a clone with a distinct `CFBundleIdentifier` *and* subtype, an inside-out ad-hoc re-sign, `lsregister -f` — and all five left `pluginkit -m -i <id> -v` answering `(no matches)`. It was right, and it was right to stop: displacement was the only route and an unattended run must not take it.

**A `ditto` backup taken first is what makes it safe**, and it answers that run's stated objection directly — the risk was dying mid-way and leaving his registration pointing at a build tree that later gets deleted; a 5 MB copy makes that one command to undo.

| | before | after |
|---|---|---|
| `pluginkit -mv` UUID | `DBE224FD…` | **`793D00A0…`** |
| its date | 2026-08-25 | **2026-08-31** |

Read it by UUID and date, not by presence: the stale registration is present too and differs in nothing else.

### Then Apple's validator, before any DAW

```
auval -a          ->  aumu Drck Dsyh  -  TiDE Synth:TiDE Rack
auval -v aumu Drck Dsyh   ->  AU VALIDATION SUCCEEDED   rc=0
```

**The first Apple-validated AU result this project has.** M2 and E9 both record that TIDE's AU evidence was *our own probe, never a DAW*; `auval` is neither ours nor a DAW, and it is stricter than the first and cheaper than the second.

### REAPER hosts it — and two traps cost a launch each

REAPER 7.45 scanned the registered extension into its AU cache as `TiDE Synth:TiDE Rack` (his own cache had **never** held a TIDE entry — 0 matches, checked before starting), instantiated it as **`AUi: TiDE Rack (TiDE Synth)`**, floated the editor, and rolled the transport **43 s at `playstate=1`** with the position advancing to 43.14 — so `process()` ran and nothing wedged.

- **A seeded portable config reloads the developer's last project**, whose missing plug-ins raise a modal, and the modal blocks `Scripts/__startup.lua` from ever running. The symptom is a startup script that writes **no log at all**, which reads as "my script is wrong" — I spent a launch there. `loadlastproj=0` plus an explicit empty `.rpp`.
- **The AU cache must be deleted from the PORTABLE copy** to force a rescan; seeded from his, it has no TIDE entry, so REAPER never looks.

Useful by-product: the blocking modal is where REAPER's own naming convention is printed — `AUi: <name> (<manufacturer>)`. Take the spelling from REAPER rather than guessing it.

### The screenshot settles what a symbol check could not

The floated editor **drew**, and its module browser lists `LFO`, `LFO2`, `Scope`, `SEQ3`, `SHASR`, `Quantizer`, `RandomValues` and the rest under a **`Rack-VCV Fundamental`** heading, with the five prefabs above them.

That is a picture of VCV Fundamental linked and **enumerated inside the hosted extension**. The 2026-08-29 run reached for `strings … "VCV: Scope"`, got 0, read it as "VCV did not link", and then confirmed its own error with a second bad reading — the ids are composed at runtime so the literal never appears. No symbol check could have answered this; one screenshot did.

### The wall a human does NOT remove, and it is the reason the rest is unmeasured

**An audio-unit extension runs out-of-process, so everything this project traces to `stderr` is invisible when the plug-in is hosted.** `RACK_ADAPTOR_TRACE`'s counters and TIDE's own `syncState`/`building rack from` lines are all `fprintf(stderr, …)`. Measured, not assumed: the strings are in the appex binary, the plug-in loads and runs under the host, and grepping REAPER's stderr for `TIDE:` or `RackProcessor` returns **nothing**.

So the linux box's whole instrument set is unavailable here, and E19's animation, int/bool/enum and pixel-diff clauses cannot be read on macOS AU3 however long anybody watches. **Filed as E73**, whose fix already exists one layer up: E65's `TIDE_PANEL_LOG_PATH` + `-DTIDE_PANEL_TRACE_LOG`, which routes a trace to a file and defaults into `TMPDIR` so it survives the sandbox.

### One measurement that belongs to V2, recorded in passing

REAPER sees **3** parameters on the instance: `Bypass`, `Wet`, `Delta` — all REAPER's own AU wrapper params. **None of TIDE's parameters are visible to the host**, so there is nothing for a DAW to automate today. That is V2's problem and this is a datum for it, not a new row.

**Learned:**

- **"Needs a human" is a claim with an expiry, and it expired the minute one showed up.** Two of E19's blockers were properties of an *unattended* run — a locked screen and a registration nobody may displace — not of the platform. The row had said so since 2026-08-29; what changed was availability, and a run should check that before re-inheriting a blocker.
- **Take the backup and the objection disappears with it.** The 2026-08-29 refusal was reasoned from irreversibility ("if the run died in between"). A `ditto` first converts the whole argument into a one-command undo — the blocker was recoverability, not permission.
- **`auval` before any DAW.** It is Apple's, stricter than our probes, needs no host config, and had never been run against this plug-in. A DAW failure after `auval` passes means something about the DAW; before it, you do not know what it means.
- **A no-output startup script is more often a modal than a bug.** REAPER wrote nothing at all, and the cause was a dialog about a *different* project's missing plug-ins. Screenshot before debugging the script.
- **When a symbol check is ambiguous and the thing is on screen, screenshot it.** Third time this project has been misled by `strings` on runtime-composed ids; the picture cost one command and is unarguable.
- **Out-of-process changes what an instrument IS, not just where it prints.** Every counter this fleet added for the linux box is a `stderr` write, and that design choice silently excludes the AUv3 target entirely. Worth knowing before adding the next one.

**Not verified:** E19's animation window, int/bool/enum toggle and pixel diff — blocked on E73 and on getting a PREPARED rack into a hosted AUv3, which is the same shape as E60's CLAP blocker; audio out of the hosted AU (the default rack with no MIDI is silence, so the test would have proved nothing); whether the same holds in Logic or Live, neither of which was opened.

**Machine state.** **One deliberate change to the developer's machine, and it is the point of the exercise:** `~/Applications/TIDE-Rack-AUv3.app` is now the current build (Release/arm64, `TIDE_VCV_FUNDAMENTAL=ON`, `RACK_ADAPTOR_TRACE=1`, 395/395 0 errors) and is the registered AUv3. **The 2026-08-26 app it replaced is backed up** in the session scratchpad; restoring it is `rm -rf` + `ditto` + one `open -g`. Everything else was isolated and verified afterwards: his `~/Library/Application Support/REAPER` has **0 files** modified in the last two hours across 2052, and his installed `VST3/TIDE-Rack.vst3` (Aug 28) and `CLAP/TIDE-Rack.clap` (Aug 22) are untouched — every build ran `SE_LOCAL_BUILD=OFF`. The portable REAPER, its config and all captures are in the scratchpad. No REAPER, appex or TIDE process left running by this run; a `e38_context_menu_probe.py` and a standalone TIDE belonging to Jeff's own live session were running throughout and were left alone. **A macOS permission dialog is on his screen** — *"Claude is requesting to bypass the system private window picker"*, raised by `screencapture`; I did not answer it, because system security settings are his, and screen capture worked without it.

**Next:** **E73 unblocks three of E19's clauses** and is one session. **E19's remaining mac clauses also want a prepared rack in a hosted AUv3** — worth solving once, since E60 needs the same thing for CLAP. **E72** wants a ruling, not a session. And the AUv3 is registered *now*, so any further AU3 measurement is cheap until somebody rebuilds over it.

**Branch/PR:** `tide/mac/E19-au3-registered` — the E19 row, E73, the macOS AUv3 section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry.

## 2026-08-31 — macos — E69: the CLAP save was fixed into an EMPTY save, and a 200-line bare host found it in one command (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** (no `claude` CLI on this box's PATH; the app's version, which A13 records as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** took **E69**, filed this morning from E68's leftovers, and found that its part (1) had **already been implemented and merged 20 minutes after the row was filed** — and that the implementation is a **regression that loses the entire patch on a CLAP save**. Built the instrument that shows it, measured a three-way A/B, and fixed it: [GMPI_Wrappers#37](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/37). **E69 → IN-REVIEW; E70 and E71 filed.**

### Why E69 and not "the queue is blocked"

The `mac` NEXT cell was written twice today: Jeff's correction about E65/#559, over yesterday's *"THE mac/any QUEUE IS BLOCKED FOR A SCHEDULED RUN"*. Both predate **E69**, which was filed at 10:27 NZST in the same commit that archived E68. Re-walking in file order: **S8** GATED, **E19**'s mac AU3 cell needs a human, **E7** turns on an unruled question, **E2** not takeable by its own row, **E60** is linux's on [#550](https://github.com/JeffMcClintock/TideSynth/pull/550), **E63** is `win`, **X1**/**X2** are `linux` — and **E69** is `any`, TODO, unblocked and unclaimed (no remote branch, no open PR). The blocked-queue cell was **19 hours old and already wrong**.

### The thing worth carrying: a row's work can be consumed before the row is read

[GMPI_Wrappers#36](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/36) — *"CLAP + AU3: saves serialise the controller's store"* — merged at **22:47Z**, and E69 was filed at **22:27Z**. Twenty minutes. Nothing in the row said so, and nothing could: the row is in this repo and the code is in another. **One command changes what the item IS:**

```bash
gh pr list --repo JeffMcClintock/GMPI_Wrappers --state merged --limit 5
```

And #36 shipped with a line this box has now met twice in three days: *"CLAP compiled and installed on Windows; **AU3 is mac-only — CI will say**."* CI says a thing compiles. **This defect compiles perfectly.**

### The instrument, and why it needed no DAW at all

Every route this fleet had for reading a plug-in's saved bytes went through REAPER: a GUI launch, a `__startup.lua`, a project file to decode. That route is shut for CLAP anyway — REAPER's CLAP state path does not restore (**E60**).

`tests/e69_clap_state_probe.c` is ~200 lines against `dlfcn` and the CLAP headers: `dlopen` → `clap_entry` → `create_plugin` → `plugin->init` → `get_extension(CLAP_EXT_STATE)` → `save`, `load`, `save`. It runs in about a second.

**It is a real test and not a stub because of one line in the wrapper:** `Processor_CLAP`'s constructor creates and `initialize()`s the plug-in's own `<Controller/>` unconditionally (`Processor_CLAP.cpp:88`, added for S43(ii)), asking the host for no extension to do it. So a bare host whose `get_extension` returns `NULL` for everything still drives the same controller/processor pair a DAW drives — which is exactly where save/restore bugs live.

### Measured — three builds, one commit apart, load an 18,893-byte four-cable rack and save

| CLAP built at | save after load | `<Cable>` | modules |
|---|---|---|---|
| `bb155b1` (#35 — save echoes the processor) | 18,933 bytes | **4** | 22 |
| `379d5c1` (#36 — save reads the controller) | **85 bytes** | **0** | — |
| [#37](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/37) | 18,661 bytes | **4** | 22 |

#36's 85 bytes are the *whole document*: `<Preset><Param id="1" val=""/></Preset>`.

The plug-in's own log is the mechanism, printed by the probe:

```
#36          syncState declined to publish the startup default (17959 bytes)
#37          syncState exporting 13926 byte document (host asked for state)
```

### The cause: a census, not a hypothesis

#36 was right about *which* store a save must read. What it could not see is that **CLAP's `stateLoad` writes only the PROCESSOR's store** (`plugin.setPresetUnsafe`), so after #36 load wrote one store and save read another, and the other was never written. One grep across the four wrappers settles it:

| wrapper | load feeds the controller's store? | save reads |
|---|---|---|
| VST3 | ✔ `Controller_VST3.cpp:514,526` | controller |
| Standalone | ✔ `StandaloneHost.cpp:311,318` | controller |
| AU3 | ✔ `AU3_Wrapper.mm:539` | controller |
| **CLAP** | **✘ — processor only** | controller (after #36) |

**No async hop explains the 85 bytes**, and that distinction is what made this safe to call a defect rather than a probe artifact: `setPresetXmlFromDaw` is never called on that path, so nothing can arrive later and no run loop would rescue it. TIDE's controller then *correctly* declines to publish — E59's refusal, doing its job — and the save writes nothing. #37 adds the standalone's two calls verbatim.

### The controls, because "4 cables" alone would not have been enough

- **Structural, not just counted:** the minted document holds the **same 22 modules and same 8 module types** as the input, with all four `<Cable>` endpoints (`fm`/`tm`/`fp`/`tp`/`c`) byte-identical. `scripts/dump_preset.py` (new) is the census.
- **Round-tripped twice:** feeding the minted document back in gives **byte-identical** output. That is what separates "the save re-serialises once" from "the save rewrites the patch a little more every time", and without it the 14,136 → 13,930 shrink reads as loss.
- **Fresh-instance save is 85 bytes on all three builds** — unchanged, and E59's refusal still fires there, which is correct.
- **The A/B never touched the developer's trees:** `-DFETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS=<private clone>` points each build at its own clone at its own commit. Two clones, three build dirs, nothing to restore.

### Consumers built

Fresh Release/arm64 Ninja over `main`, `SE_LOCAL_BUILD=OFF`, `TIDE_VCV_FUNDAMENTAL=OFF`: **TIDE 598/598, 0 errors** — VST3, AU, AUv3 appex, AU3 app, CLAP, standalone. **`SynthEditCL` 779/779, 0 errors** against #37's branch. The change is confined to `wrapper/CLAP/`, and SynthEdit consumes only the **VST3** wrapper from this repo (`se_gmpi/vst3`, the export template) — so the 779 is the rule discharged, not the risk.

**The 598 also discharges #36's own "CI will say" for AU3**: `AU3_Wrapper.mm`'s added `syncState()` compiles on this platform, and AU3's load already feeds the controller, so its one-line half of part (1) is correct by construction and does **not** share CLAP's asymmetry.

### Filed rather than fixed

- **E70** — E69's part (2). `MfcDocPresenter::AddPatchCable`/`RemovePatchCable` (`SynthEditLib/EditorLib/MfcDocPresenter.cpp:280,363`) set no dirty flag; confirmed by reading — a bare `setParameterValue` with no `SuspendDSP` guard, so `invalidateDsp()` is never reached. **`SynthEditLib/EditorLib/` is GATED and this is not a build break**, so the STEP 5 exception does not reach it. It is also shared with SynthEdit proper, which is the ruling the row asks for.
- **E71** — found in passing: AU3's `setFullState` never calls `notifyControllerOfPreset`, which both other wrappers do and which is the only call that tells a plug-in's own `<Controller/>` its state was restored. A code reading, deliberately not fixed by a run that cannot drive an AUv3 host.

**Learned:**

- **A row filed this morning may have had its code landed by an interactive session before any scheduled run reads it** — twenty minutes, here. The row still said TODO, and it had to: the code was in a sibling repo. One `gh pr list --state merged` on the repo the row's Scope names, before starting, and the item's whole shape changes.
- **"CI will say" about a platform you cannot test is an assignment with no addressee, and CI answers a different question.** This is the same lesson this box wrote two days ago, arriving as a *stronger* case: three merges deferred to CI and CI had already run; here CI ran, passed, and was never asked anything that could have caught an empty save.
- **A fix that corrects one half of a pair is a regression until you check the other half.** #36 moved the save to the right store without asking who writes it. The check is a four-row table — for each wrapper, which store does load write and which does save read — and it fits in one grep.
- **A "no host at all" harness is cheaper than isolating a host, and stricter.** Days of this document's macOS section are about isolating REAPER; the defect that mattered needed no host, ran in a second, and produced files you can diff. Reach for the plug-in's own C ABI before the DAW whenever the question is about bytes.
- **A size MATCH proves nothing and a size DIFFERENCE proves one thing.** E68's `14,136 → 14,494` discriminator works in one direction only: the mint is a **fixed point**, so a re-saved document matches exactly. My own probe printed "consistent with an echo" over a correct mint before I fixed its wording.
- **Distinguish "the store is stale" from "nothing ever writes the store" before calling a probe result a race.** A missing call cannot be won by waiting, and the grep that shows it is absent is faster than any run-loop pump would have been to write.
- **Reading the machine costs one command and I nearly skipped it.** `CGSSessionScreenIsLocked` is *absent* from `IOConsoleUsers` when the screen is unlocked — cheaper and less ambiguous than `screencapture`, whose all-black frame is the trap this document already names.

**Not verified:** audio through a CLAP host — E60 has REAPER's CLAP state route open, and this probe measures bytes deliberately, not sound; the AU3 save through a real AUv3 host — the registration wall E19's mac cell measured on 2026-08-29 is unchanged, and the developer was working at the machine; **whether a cable edit made LAST is captured** — E69's Accept says "cables as the last edit" and that needs a driven editor gesture, so what is proven here is the round trip, not the ordering; linux and Windows builds of #37.

**Machine state.** All six repos were clean and on their default branches at the start; `GMPI_Wrappers` was 2 commits behind and was fast-forwarded. TideSynth is on this run's branch and GMPI_Wrappers on its own until STEP 5 returns them. **An accidental `REAPER --help` launched the developer's REAPER for a few seconds early in the session** — recorded because it should not have happened, and because the check afterwards is the point: **nothing under `~/Library/Application Support/REAPER` was modified** (0 files newer than 30 minutes, 2052 files total as before, `reaper.ini` mtime still Sep 2025), and no REAPER was launched deliberately at any point. His installed plug-ins were never touched, measured rather than assumed: `~/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3` is sha256 `f3b09c3c…` with mtime **Aug 28 17:45:54** — the same two values the earlier run today recorded — and his installed `CLAP/TIDE-Rack.clap` is still dated **Aug 22 09:40:08**, nine days old, despite three CLAP builds this session. All three ran `SE_LOCAL_BUILD=OFF` in gitignored trees, and `SynthEditCL`'s tree is under the session scratchpad, outside every repo. A `tests/e51_dialog_divert_probe.py` and a TIDE standalone belonging to the developer's own live session were running throughout and were left alone. `decode_rpp.py` wrote `tests/hosts/v1-rack.rpp.block0.param1.xml` as a side effect; removed. Build trees `build-e69-after/`, `build-e69-before/`, `build-e69-fixed/` are gitignored; the two GMPI_Wrappers clones, the probe binary and all presets are in the scratchpad. No REAPER or TIDE process left running by this run.

**Next:** **#37 is the one that matters** — without it every CLAP save is empty, and it is one file. **E71 and E19's mac AU3 cell want the same single unlocked interactive session**, which is now the second row waiting on it. **E70 wants a ruling, not a session.** And the probe generalises: an AU3 and a VST3 equivalent would each be an afternoon, and would let a scheduled run answer "did the patch survive" on every format without a DAW.

**Branch/PR:** [GMPI_Wrappers#37](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/37) (the fix — the substance) + `tide/mac/E69-clap-au3-pull-state` (the probe, `scripts/dump_preset.py`, the E69/E70/E71 rows, the mac NEXT cell, the C-ABI section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry). The TideSynth side lands alone harmlessly; #37 is what stops the data loss.

## 2026-08-31 — windows — E68: the save was an echo of the wrong store, and Jeff's three questions redesigned it into a pull (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"use the computer to figure out why no sound"*, *"what is serviceDocumentSync about?"*, *"we have doubled-up… skip the complex document comparison yeah?"*, *"won't the parameter path handle the patch cables already?"*, *"we should be able to save the state without races?"*, *"why not save/load the Controller's state instead?"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** diagnosed Jeff's silent-patch report down to the byte, then built the design his questions converged on: **delete the document-shape machinery** (this repo) and **make the VST3 save pull fresh state from the controller** ([GMPI_Wrappers#35](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/35)). Proven by a scripted save round-trip. **E68 filed and IN-REVIEW.**

### The diagnosis: the .als itself was the witness

Driving Ableton from a shell with his MRU project: restore healthy (29,475 bytes imported, rack built, feedback flowing), MIDI reaching the rack, silence. Decoding TIDE's chunk out of the saved `.als`: **every module present, `HC_PATCH_CABLES` an empty patch-list** — a rack with no wiring. When Jeff re-cabled live, instant sound. The saved state, not the engine, was the defect.

Two mechanisms stacked, both already confessed in comments:

1. `serviceDocumentSync` pushed only on document-**shape** change, and `documentShape()` strips every `<patch-list>` — cables live inside HC49's patch-list, i.e. inside the one thing the comparison was built to ignore. The 2026-08-25 ruling's premise — *"the DSP rebuilds from the document it ALREADY HAS"* — is true of the running rack and **false of the store a save reads**.
2. The save serialised the **processor's** store, fresh only up to the last async IMessage the audio thread applied — and `Controller_VST3::getState`'s own "HONEST CAVEAT" documents the race, including the case no flush can fix: a host reading the component's state before ever calling the controller's `getState`.

### Jeff's questions did the design work, in order

- *"We have doubled-up — skip the comparison?"* → Almost: the flag (`dspDirty`) is already structural-only (SuspendDSP's sites; knobs never touch it), and the compare's one surviving job was suppressing exactly the push that would have saved his patch. **Deleted** — `documentShape()` + `lastPushedShape`, −37 lines. Verified in the standalone: 1 push at startup, **0 in idle** (the "holy fuck" economics intact), 1 per module insert, 0 per layout nudge.
- *"Won't the parameter path handle cables?"* → For the running DSP, yes — but the wrapper serialises only its four parameters, and cables live *inside* parameter 1's document; a `ppc` updates the live rack and touches nothing a save reads.
- *"We should be able to save without races?"* / *"Why not save the Controller's state — are we expecting the Processor to know the state when it simply does not?"* → Exactly, and the API's frame allows it: the component stream is canonical (hosts hand it back to *both* sides on load — restore was always controller-authoritative via `setComponentState`), so the fix is that **the processor stops answering from its own knowledge**: `getState` pulls from the paired controller, which mints the preset synchronously from its live store (`syncState()` + the shared `getPresetXml()` — the drift-unified `writePresetXml` meant no new serialiser). No hop, no ordering, nothing to race. Pairing is a one-time pointer over the connection-point **message** channel ("GmpiCtlPtr") — a message, not a cast of the peer, because hosts interpose connection proxies; cleared on `disconnect` so a dead controller cannot be pulled through (E66 one layer up, avoided).

### Proven

Scripted REAPER load→save→quit of `v1-rack-win.rpp` (a temporary `__startup.lua`, backed up and restored):

| | chunk in the project file |
|---|---|
| before | 14,136 bytes — the seeded store copy |
| after the save | **14,494 bytes — the freshly minted document**, `syncState exporting … (host asked for state)` in the log, **both cables present** in HC49 |

The size mismatch *is* the discriminator: an echo of the store would have written 14,136 back.

**Learned:**

- **The user's design questions were the diagnosis.** Four questions in sequence — each one eliminated a layer I would have patched — and the end state deletes code net. "Are we expecting the Processor to know the state when it simply does not?" is the whole fix in one sentence.
- **A save path that echoes a store is only as correct as the store's freshest writer** — and every writer between editor and store was asynchronous. Mint at the moment of asking, from the side that cannot be stale.
- **A comment's premise can be measured.** *"The DSP rebuilds from the document it already has"* was written about the running rack and silently extended to the saved one; one decoded `.als` separated the two claims.
- **Send pointers as messages, not casts, across a host-owned connection** — proxies forward the one and defeat the other.
- **The fixture's saved chunk size is a free discriminator** between "echoed the store" and "minted fresh" — no instrumentation needed, the number differs by construction.

**Not verified:** mac/linux builds; Ableton itself (REAPER proved the mechanism — Jeff's own `TIDE Test.als` re-cable→save→reload is the live Accept once merged); CLAP/AU3 still echo their processor store (named in the row); `AddPatchCable` still sets no dirty flag (harmless for saves now; named in the row for the live-restore path).

**Machine state.** The E68 stderr probe in `SynthEdit.cpp` reverted — diagnosis complete, the permanent E59-era lines suffice. `modules/TiDEPanel/TiDEPanelGui.cpp` briefly carried a foreign `TIDE_PANEL_TRACE_LOG 1` edit (Jeff's E65 diagnosis, marked REVERT) — left alone and since reverted by its owner. TideSynth on `tide/win/E68-push-on-dirty`, GMPI_Wrappers on `tide/win/E68-pull-state-at-save`, both with PRs. REAPER's `__startup.lua` restored. Jeff's Ableton closed by him; the installed VST3 carries both halves.

**Branch/PR:** `tide/win/E68-push-on-dirty` (this repo: the shape deletion, this row and entry) + [GMPI_Wrappers#35](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/35) (the pull). Either lands alone; together a patch can no longer be saved incompletely from either direction.
## 2026-08-31 — macos — the queue is blocked, so this run closed three "not verified: mac builds" lines and flipped the rows they sat on (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** (no `claude` CLI on this box's PATH; the app's `CFBundleShortVersionString`, which A13 records as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** re-walked the queue in file order and found nothing takeable, then did the thing that was actually owed instead: **E64, E66 and E67 → DONE and archived**, with the macOS half of each row's own *"not verified"* line measured rather than inherited. Branch `tide/mac/step4-e64-e66-e67-mac-verified`. TideSynth only; no product code changed, no sibling repo committed to.

### Why nothing was takeable, walked rather than inherited

The `mac` NEXT cell was 2026-08-28 and predates all four of E64–E67, so I re-walked. **S8** GATED — its live half is `SynthEditLib/UgDatabase.cpp`, and the row itself says changing the CMake gating *"needs a ruling this row does not ask for"*. **E19**'s mac AU3 cell needs a human, both reasons re-confirmed today rather than assumed: `ioreg -n Root -d1 -a` reports `CGSSessionScreenIsLocked true`, and the 2026-08-29 run measured five separate ways to register a current AUv3 beside the developer's, all failing. **E7** turns on Jeff's unruled *"where do the jacks live"*. **E2** is not takeable by its own row. **E60** is linux's. **E63**/**X1**/**X2** are other platforms'.

**E65 is this platform's and is already finished.** [#559](https://github.com/JeffMcClintock/TideSynth/pull/559) is green, unreviewed, nothing unresolved — STEP 1.5 says explicitly that is waiting on Jeff and not mine to touch. Its branch already flips the row to IN-REVIEW, **so I deliberately did NOT flip E65 on `main`**: doing so would hand #559 a conflict in the one file this fleet conflicts in most.

### The work that was owed, and nothing pointed at it

E64, E66 and E67 merged from the windows box on 2026-08-28 and **every one shipped saying "Not verified: mac/linux builds (no platform code; CI will say)"**. CI did not say. `gh run list --branch main` shows the last `build` on `main` at **9af5ea5a, 21:41Z** — E64's own merge — while `SynthEditLib#75` and `GMPI_Wrappers#34` landed at **23:57Z**. So no macOS build has ever compiled E67's change to `ViewBase.cpp`, and the row that says CI will answer named a run that had already finished.

### Measured, on a fresh tree with the trees themselves as the control

All six repos were clean (`git status --porcelain` empty) and byte-equal to `origin/main` before configuring, recorded per repo, because a `FETCHCONTENT_SOURCE_DIR_*` build reads a live working tree and another session's uncommitted work would land in the result. Fresh Release/arm64 Ninja tree, `SE_LOCAL_BUILD=OFF`, `TIDE_VCV_FUNDAMENTAL=OFF` — the shipped configuration, and OFF is also what stops POST_BUILD replacing the developer's installed plug-ins.

| | result |
|---|---|
| TIDE, every target | **598/598, 0 errors**, 97 s — standalone, VST3, AU, AUv3 appex, AU3 app, CLAP |
| `SynthEditCL` | **779/779, 0 errors** against the same merged `SynthEditLib` |
| E64's drain diagnostic in fresh artifacts | **1** each in VST3, CLAP, standalone |
| same string in the two INSTALLED pre-merge bundles | **0** and **0** |

`SynthEditCL` is the row that mattered: `ViewBase.cpp` ships in SynthEdit as well as TIDE, so *"TIDE builds"* is not evidence the commercial product is safe, and E67's own verification built SynthEditCL on Windows only.

**`nm` finds no `canvasCenter` and that proves nothing** — it is an inline member declared at `ViewBase.h:316`. The evidence that E67 is in this build is that both corrected call sites are in the compiled `ViewBase.cpp` (`:1237`, `:1388`) and the tree was fresh, so no stale object could have been reused. This is the 2026-08-29 `VCV: Scope` lesson arriving in a second shape, and I checked the declaration before reading the miss as a result this time.

### The audio half, and the control that makes the number mean anything

Through an isolated portable REAPER (copy `REAPER.app`, `touch reaper.ini`, seed the developer's `reaper.ini`):

| fixture | measured |
|---|---|
| `--control` (no plug-in at all) | **peak −6.0 / rms −9.0 dBFS** — the chain detects audio |
| `tests/hosts/v1-rack.rpp` | **peak −6.3 / rms −17.0 dBFS** — the 2026-08-18 macOS reference to the decimal |
| `tests/hosts/v1-rack-uncabled.rpp` | **−inf** — negative control, 0 patch cables |
| `tests/hosts/v3-midi-pitch.rpp` | −6.2 / −21.1 dBFS |
| `tests/hosts/v1-rack-midi.rpp` | −6.3 / −17.0 dBFS |

**The last row is NOT an E7 measurement and must not be read as one.** E7's Accept is about envelope TIMING, and peak/rms is precisely the number the 2026-08-28 run showed can make E7's failure look like a pass — identical figures to the no-MIDI fixture are the failing signature, not a passing one. Recorded as a by-product only.

### The provenance control, which is the reusable part

E19's windows leg cost a measurement to *"a local build does not shadow the installed plug-in and REAPER will silently load either one."* So this run did not merely narrow `vstpath_arm64` to a staging folder holding one bundle and delete `reaper-vstplugins*.ini` to force a rescan — **it then removed the bundle and re-rendered.** REAPER hung on an unresolvable-plug-in modal and wrote no TIDE cache entry; restoring the bundle reproduced −6.3 / −17.0 exactly. That round trip is what turns "the number came from my build" from a hope into a fact, and it cost one 300-second timeout.

It also settled a discrepancy I would otherwise have hand-waved: the rescanned cache keys the entry `TIDE_Rack.vst3` with an **underscore** while the staged bundle is `TIDE-Rack.vst3` with a **dash**. No underscore-named bundle exists anywhere on this box — REAPER sanitises `-` to `_` in an ini key.

**Learned:**

- **A "not verified on your platform" line is an assignment with no addressee, and "CI will say" can be false at the moment it is written.** All three rows deferred to CI; the last `build` on `main` predated two of the three merges, so the deferral pointed at a run that had already finished. One `gh run list` compared against the merge timestamps is the whole check.
- **Prove which binary answered, by taking it away.** Narrowing the scan path and clearing the cache are *arrangements*; removing the bundle and watching the render fail is a *measurement*, and only the second survives someone asking whether the installed copy got loaded.
- **A `strings` hit is worth what its miss is worth.** Two pre-merge bundles were already sitting on this box and gave the negative control for free — 0 occurrences against 1 in every fresh artifact.
- **Check how a symbol is declared before reading its absence from `nm`.** Inline members never appear; this is the runtime-composed-id lesson from two days ago wearing different clothes, and the same box wrote both.
- **"Which consumers did you build" is a different question from "does it compile".** The shared file compiled into TIDE at 598/598 and that says nothing about SynthEdit; `SynthEditCL` 779/779 is the claim worth having, and it is one target.
- **Do not flip a row on `main` when another platform's open PR already flips it.** E65 was one edit away from handing #559 a conflict in `BACKLOG.md`, which is exactly the file this fleet conflicts in.
- **A blocked queue is not the same as an idle run.** Three rows were sitting IN-REVIEW with every PR merged, and the mac verification each of them asked for was cheap once somebody read the sentence.

**Not verified:** E67's zoom BEHAVIOUR on mac — its drift figure needs a driven `--scroll --ctrl` gesture with the editor on screen, and the screen is locked; E66's 5/5 close/reopen reproduction — a GUI gesture whose assert half is `_DEBUG`-only and this is Release, and it was never reproduced on this box; E64's hosted processor-recreate lifecycle on mac — the row's reproduction was Ableton on Windows and nobody has re-run it here; **linux builds of all three, which are still unverified by anybody** and are one build on that box.

**Machine state.** All six repos were clean and on their default branches at the start, and were fast-forwarded to `origin/main` (TideSynth 8 commits, SynthEditLib 1, gmpi_ui 1, GMPI_Wrappers 2, GMPI 1; SynthEdit already current). TideSynth is on this run's branch until STEP 5 returns it; nothing else was committed to. **The developer's `~/Library/Application Support/REAPER` compares identical to its pre-run snapshot across 2052 files including sizes and mtimes**, and his installed `~/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3` is sha256 `f3b09c3c…`, mtime still Aug 28 17:45:54 — the isolation held, as the 2026-08-29 run measured it would. Build trees `build-macverify/` (gitignored) and a scratch `build-secl/` for SynthEditCL, the latter outside every repo so Jeff's Xcode `build/` tree was never touched. Portable REAPER, its seeded config and all renders are under the session scratchpad. No REAPER or TIDE process left running.

**Next:** **the same three rows want a linux build**, which is the one leg nobody has run and is a single command on that box. **E65 wants Jeff's merge** — [#559](https://github.com/JeffMcClintock/TideSynth/pull/559) is green and has been since 2026-08-29. **E19's mac AU3 cell still wants one unlocked interactive session**, unchanged: build `TIDE_Rack_AU3_assemble` with `-DTIDE_VCV_FUNDAMENTAL=ON -DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`, copy the app over `~/Applications`, launch it once.

**Branch/PR:** `tide/mac/step4-e64-e66-e67-mac-verified` — the E64/E66/E67 flips and their archive rows, the mac NEXT cell, the macOS plug-in-provenance recipe in [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry.

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
### Correction: Ardour IS a host here, and it settles the question

**Jeff asked "don't we have Ardour host?" — yes, and that makes three separate
claims of mine wrong.** I wrote in the row, both PR bodies and the issue that
closing this needed REAPER on a win/mac box. **Ardour 8.4 is installed on this
box**, `ardour-vst3-scanner` answers precisely this question, and **my own memory
note from 2026-08-19 records using it**, including the
`LD_LIBRARY_PATH=/usr/lib/ardour8` quirk it needs.

```
BROKEN (main):  VST3 not a valid bundle:
                  '.../TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE_Rack_VST3.so'
FIXED  (both):  [Info]: Found Plugin: TIDE Rack
                  uid=506C7567696E474D504920501951ED43 category="Instrument|Synth"
                  n_outputs=2 n_midi_inputs=1
```

Ardour derives the payload name from the bundle name — exactly the rule GMPI's
own comment states — so **the Linux VST3 is unloadable today, not merely oddly
named**, and the fix is host-verified on the platform that has the bug. The
scanned UID also matches the one in all five `.rpp` fixtures.

**The lesson is not "use Ardour".** It is that I asserted an environment limit
three times without testing it, while holding a note that contradicted it.
"Not verifiable here" is a claim about the machine, and it deserves one command
before it goes into a row, two PR bodies and an issue.

Ardour's cache entry from the scan pointed into a scratch tree and was removed;
Jeff's other nine cached plugins were left alone.


**Learned:** anything the next run would otherwise rediscover the hard way.

0. **"Not verifiable on this box" is a measurable claim, and I shipped it three
   times unmeasured.** Ardour was installed the whole time and my own memory note
   named the command. Check the machine before writing a limit into a row.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---
