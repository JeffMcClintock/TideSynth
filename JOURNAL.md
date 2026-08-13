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

## 2026-08-14 — linux — S2 (plus a platform:linux build break found and filed)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Did:** Produced [docs/sandbox-audit.md](docs/sandbox-audit.md) — the audit
PLAN constraint 4 cannot be verified without. **The answer is that constraint 4
is violated, and it is not a code-reading inference: the files are on disk on
this machine.** Also found, while establishing TIDE's source set, that **SE16
does not configure on linux at all any more** — filed as
[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) rather than fixed,
per STEP 3.

**Neither of this box's usual entry points was takeable, and the NEXT row said so
itself.** No open `platform:linux` issue at STEP 1; no open PRs in any of the
five fleet repos and only `main` on TideSynth's remote, so nothing to resume at
STEP 1.5. The `linux` NEXT row was the stale P7c pointer, explicitly left as a
question by the 2026-08-14 windows interactive session — *"picking the wrong
replacement here is worse than an honest gap"*. So this run did the screening it
asked for, took the item that survived it, and re-pointed the row. That screening
is the second most useful thing here after the audit itself, because **the queue
is systematically misleading about what a scheduled run can take:**

- **The GATED wall is nowhere recorded in the rows it applies to.** `S1b`, `S5`,
  `S7` and `S8` are all `TODO`/`any`/unblocked and read as available. All four
  are **entirely** work in `SE16/EditorLib/`, `SE16/SynthEdit2/` or the
  `SynthEditLib` repo — GATED to C1-C7 only, per the 2026-08-11 C8 ruling that
  deliberately declined to widen the exception. S8's row actively misleads:
  *"~~GATED on C0~~ unblocked 2026-08-08, C0 approved"* reads as permission, but
  C0 gates the carve-out **stages**, not any item that wants to touch a shared
  file. Each of these will burn a session on discovery exactly as A4 did last run.
- **The workflow wall** (A4, A10, A12, and **B1**, which nobody had named
  before — it is `.github/workflows/build.yml`).
- Screened out for their own reasons: **C9** (its remaining work *is* C5, a `win`
  row — `Application.cpp`), **C11** (needs Jeff's ruling on the `SynthEditApp.h`
  licence gate), **A9** (open NEEDS-JEFF prerequisite), **N1** ("do it after
  C7"), **P7d** (scope question for Jeff, and macOS-only to verify), **D1/D2**
  (mac-shaped), **U1** (needs a fresh post-pivot audit first).

That left **S2**, which is `any`, needs no GATED edit (it *reads* gated code and
writes only `docs/` in this repo), and has no open question that would change
what gets written.

**Result — the audit, and why its file set is trustworthy.**

A grep of SE16 returns hundreds of hits in code TIDE never compiles, which
buries the real ones. The row says *"reachable from a TIDE build"*, so the file
set is derived:

| Step | Result |
|---|---|
| link closure from `build.ninja` | `VST3_Wrapper` + `SynthEditLib` + `EditorLib` — and **no** `.sem` module libraries |
| TUs from `compile_commands.json` | **264** |
| first-party after dropping VST3 SDK + generated Wayland C | **230**, **zero unresolved** |
| categorised hits | **202 across 46 files** |
| DWARF compile-unit list from the unstripped `.so` | **235 CUs linked**; of the 46 hit files, **37 are in the binary, 9 are not** |

That last row is the one that makes the audit worth reading: a static archive
contributes an object only when something references it, so "compiled" and
"linked" are different questions and only the second one matters. Re-runnable
with `python3 tools/sandbox_audit.py --build <tree>`.

**The violation, measured on disk rather than argued from source — ~12.8 MB
across 139 files:**

| Location | Size | Files |
|---|---|---|
| `~/SynthEdit Projects/skins/` | 724 KB | 74 |
| `~/SynthEdit Projects/Prefabs/` | 1.1 MB | 35 |
| `~/.local/share/SynthEdit/` | 11 MB | 30 (six ~330 KB `Plugin-Cache-16-override-*.xml`) |

**Rulings: 7 remove, 4 stub, rest keep.** Full reasoning per finding in the doc.

**Most of it attaches to rows that already exist rather than creating new ones** —
which is the point, and is why no new BACKLOG items came out of it:

- **S7** gets its gated remainder named to the line: `SynthEditLib/SkinMgr.cpp:28-32`
  (the *constructor* calls `setSkinFolder`) and `:48-100` (the recursive copy).
  Confirmed by measurement, not just the static chain S7 asserted — **27 `SkinMgr`
  symbols defined in `TIDE_VST3.so`**, and the literal `SynthEdit Projects` is
  **in the TIDE binary** (`default3` 4×).
- **S1b (b)/(c) reproduced on a second platform** (S1b measured macOS):
  `ScanFolder`, `LoadModuleData`, `LoadOrScanModuleData`, `RegisterExternalPluginsXml`
  all defined, `Module_Info3` 65 symbols, and `dlopen`/`dlsym`/`dlclose` genuinely
  imported per `nm -D --undefined-only`.
- **S1b (a) confirmed genuinely done** by the same measurement: `FileWatcher` has
  **zero** symbols and `FileWatcher.cpp` is **not** a linked CU. Replacing
  `SynthEditApp.cpp` with `TideAppStubs.cpp` did remove the watcher thread.
- **S8 reproduced on Linux** — `ug_soundcard_in` 28, `ug_soundcard_out` 33,
  `ug_midi_out` 24, **`OscillatorNaive` 0**. That last one independently confirms
  the blocker **E2a**'s row warns about, now on both measured platforms: there is
  no modern oscillator primitive registered. S8 also gains a fourth module —
  `ug_wave_recorder.cpp:216` `fopen(…,"wb")` writes a WAV to an arbitrary path.
- **S3 gains its evidence.** Its `assert(false)` stubs have live write sites
  behind them (`CUG.cpp:2833-2968` — `create_directory` ×2, `copy_file`,
  `fopen(…,"w")`), so "silently falls through in release" is measured now.
- **C5** inherits three findings (`Application.cpp` prefab copy,
  `SynthEditAppBase.cpp` module staging, `UG2.cpp`'s write-to-`%TEMP%`-and-
  `LoadLibrary`).

**One genuinely new finding, and it is a ruling rather than a bug.**
`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:343-352` prefers
`getpwuid(getuid())` over `getenv("HOME")`, and the comment says why in as many
words: *"getpwuid returns the real home directory even when sandboxed, whereas
getenv("HOME") returns the container path in a sandbox."* **A deliberate
sandbox escape** — correct for SynthEdit, which is a desktop app, and directly
contrary to constraint 3 for TIDE. It sits **upstream** of both folder-copy
findings, because `getCommonDocumentFolder` falls through to it on every
non-Windows platform, so **S7 cannot be fixed properly without ruling on it**.
GATED (`SynthEditLib`), so flagged, not touched.

**Learned:**

- **The GATED wall is the `any` lane's real filter, and it is invisible in the
  rows.** Last run found the workflow wall and re-marked A4/A10/A12 for it; this
  is the same shape one level over, and it disqualifies four more rows that all
  read as available. Worth a Status-cell pass by Jeff, the same way A4 needs one
  — S1b/S5/S7/S8 are not `TODO` for a scheduled run in any useful sense.
- **A build tree you did not make is a first-class measuring instrument.** The
  whole audit rests on reading `build.ninja`, `compile_commands.json` and the
  DWARF out of `/home/jef/SE/build` — Jeff's own tree, read-only, never
  reconfigured or built into. It answered "what does TIDE actually compile and
  link" exactly, which no amount of CMake-reading would have.
- **`readelf --debug-dump=info --dwarf-depth=1` is the cheap way to get the
  linked compile-unit list** from an unstripped `.so`. `--dwarf-depth=1` is what
  makes it tractable on a 91 MB binary; without it the dump is unusable.
- **The skin-version stamp becomes a thrash bug at C7, and does not look like
  one today.** `SkinMgr` invalidates on `SE_APP_BUILD_NUMBER`, which
  `EditorLib/CMakeLists.txt` injects (`183` today) and `se_version.h` defaults to
  `0`. TIDE links that same EditorLib, so **today TIDE and SynthEdit agree and
  nothing thrashes.** From C7 — clean clone, no private repo — TIDE takes `0`
  while SynthEdit keeps `183`, so each would re-copy 724 KB of skins over the
  other on **every** launch. Argues for removing the mechanism from TIDE *before*
  C7, not after.
- **`std::remove` and `Processor::open(phost)` are the two false positives any
  filesystem grep of this codebase will hit** — the `<algorithm>` overload and
  the GMPI lifecycle call. Both are listed in the doc's "keep" section so the
  next audit does not re-flag them.

**The build break, filed not fixed** ([#53](https://github.com/JeffMcClintock/TideSynth/issues/53),
`platform:linux`): `GMPI_Wrappers` `e707482` (*"feat(standalone): Linux/Wayland
standalone host"*, Jeff, 2026-08-13, current tip of `origin/main`) added
`pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)` at
`wrapper/Standalone/CMakeLists.txt:32`. `wrapper/CMakeLists.txt:14` adds that
subdirectory unconditionally, and the file's own early-out is
`if(NOT UNIX OR APPLE) return()` — **so Windows and macOS skip it and only linux
takes the hard dependency**, which is why it can sit on `main` looking green.
`libpipewire-0.3-dev` is not installed here. Reproduced both with a local
`GMPI_WRAPPER_FOLDER_OVERRIDE` and with it blank so CPM fetches `origin/main`.
Net effect: **the linux box cannot configure, so it cannot build or verify
anything in SE16, TIDE included.** `GMPI_Wrappers` is an ALLOWED path, so it is
ordinary takeable work once someone picks between "make Standalone opt-in" and
"probe without `REQUIRED` and skip".

Because of it, the audit is measured from the pre-existing `/home/jef/SE/build`
tree (configured 2026-08-10). **Drift was measured, not hoped for:**
`SynthEditLib`'s CMake source list is **unchanged** since then (267 entries, none
added, none removed), and `EditorLib`'s differs by exactly `browseto.mm` and
`openurl.mm`, both `if(APPLE)` and not compiled on linux. C4 changed those files'
**paths**, not the set of code compiled, so the audit is current in content; the
doc states this as a limitation rather than burying it.

**STEP 4 chores done as part of this run:** **E1a** flipped `IN-REVIEW` → `DONE`
([#52](https://github.com/JeffMcClintock/TideSynth/pull/52) confirmed merged
2026-08-13) and moved verbatim to `BACKLOG-DONE.md`; **A11** moved likewise (it
was already `DONE` and marked *"ready to rotate"*). Also re-verified A11's own
fix still holds on this box: all nine local repos are `https://`, and STEP 0.7's
second assertion prints `git@github.com:`.

**Next:** **[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) is
STEP 1 work for the next linux run** and outranks the backlog — nothing else on
this box can be built or verified until it lands. **Then S3**, which is the one
remaining `any` row a linux box can genuinely do: its target
`SE16/SynthEditSem/TideApp.cpp` is **ALLOWED**, and S2 just supplied its
evidence. The `linux` NEXT row now says both, with the screening written out.

For Jeff, two Status-cell judgements that are his and not a run's: **S1b, S5, S7
and S8 are GATED-in-full and should not read as `TODO`** to a scheduled run, and
**A2's sandbox-escape question has to be answered before S7 can be done
properly**.

- **`gh pr checks` reports the `build` workflow red on every PR, and `gh run
  list` reports the same runs green — both are correct, and the discrepancy will
  waste someone's time.** `build.yml` sets `continue-on-error` at **job** level,
  so the *run* concludes `success` while all three *jobs* conclude `failure`.
  `gh pr checks` surfaces the jobs; `gh run list` surfaces the run. Checked
  against `main` before assuming this PR caused it: run `31749003642` on
  `3ff987b5` has `failure` for linux, macos and windows too, with the identical
  `CMake Error: The source directory … does not appear to contain CMakeLists.txt`
  — TideSynth has no root `CMakeLists.txt`, on `main` or anywhere. That is the
  C7 failure `build.yml`'s own header says is the point, and **B1 is the row for
  it** (its comment already says "green here still means nothing"). **`lint` is
  the only check that currently gates anything, and it passes.**

**Machine state, for the record:** all nine local repos were clean and on their
default branches at the start and are again at the end; **only `TideSynth` was
committed in**, so STEP 5's two-end-states rule has exactly one repo to satisfy.
Four repos were **fast-forwarded** to `origin` before measuring — `SE16` (7
commits behind), `SynthEditLib` (2), `gmpi_ui` (3), `GMPI_Wrappers` (1) — because
an audit of stale source would have been wrong about what TIDE compiles. All four
were clean beforehand, so these were true fast-forwards on the default branch with
nothing to stash; noted because it is a change to Jeff's trees, small and
reversible though it is. **Pulling `GMPI_Wrappers` is also what surfaced #53** —
its 1 commit was `e707482`, the one that breaks the configure.

**Branch/PR:** `tide/linux/S2-sandbox-audit` →
[#54](https://github.com/JeffMcClintock/TideSynth/pull/54)

---

## 2026-08-14 — macos — E1a

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Desktop 1.26832.0 · as `tide-rack-bot`

**Did:** Ran the audio harness on a second platform for the first time in its
life — macOS against the Linux goldens — and set the null-test tolerances from
what came back. **The headline is that finding (e) was closed by measurement
rather than by widening anything**, and that a *different* class of residual,
which nobody had modelled, is what actually needed a fix.

**Result:** both cases render on macOS; 2/2 pass after the change; every number
below is measured, not modelled unless said so.

*Three engines, so the platform axis is isolated rather than assumed.* An
engine-version or build-config difference would otherwise be indistinguishable
from a platform one:

| Engine | Origin |
|---|---|
| `SynthEditCL V1.6.175` | local Release build, 2026-08-08 |
| `SynthEditCL V1.6.182` | local **Debug** build, 2026-08-13 |
| `SynthEditCL V1.6.183` | published `SynthEditCL_mac.zip`, Azure CI, signed |

*The measurements, identical on all three:*

| Case | RMS residual | Peak residual | Class |
|---|---|---|---|
| `voice_midi_note` | −123.1 dBFS | −90.3 dBFS = **exactly 1 LSB**, 51/95,999 samples (0.053%) | pure rounding |
| `osc_naive_sine` | −73.5 dBFS | −68.7 dBFS = 12 LSB | **not rounding** |

**(1) The RMS gate did not need widening, and the reasoning that said it might
was resting on a premise the data does not support.** E1a's arithmetic was
right — the −100 dBFS gate tolerates 1-LSB error on at most ~10.7% of samples —
but it assumed the worst case, 1 LSB on *every* sample. Real cross-platform
rounding touched **0.053%** of samples: a 200× margin, 22.9 dB of headroom.
Two builds agree on nearly every sample and disagree only where a value sits on
a quantisation boundary. Gates stay at −100/−86.

The peak gate's 4.3 dB is structural rather than lucky: **1 LSB is a hard
per-sample ceiling** for this class. Two builds that agree on the underlying
float can only disagree about which way it rounds — drift of this class can
affect more samples, never make one sample wrong by more than 1 LSB.

**(2) `osc_naive_sine` fails for a reason no fixed dBFS number can express.**
The residual **grows monotonically through the render** — −96 dBFS in the first
0.1 s block, −69 dBFS in the last — with a best-fit time lag of exactly zero,
so it is neither rounding nor a delay. Fitting `dphi = k·t` gives a **frequency
offset of 0.15 ppm ≈ 2.5 ULP at single precision** (2⁻²⁴ = 5.96e-8) on a
440 Hz tone. `OscillatorNaive`'s table and increment are both `double`, but
`OscillatorNaive.h:66` derives the table index as a **`float` from a `float`
pitch**, so the pitch→increment path carries single-precision resolution — the
right order for what was measured. Named as the plausible locus, **not proven**;
the measured quantity is the 2.5 ULP.

A frequency offset *integrates*, so the residual is proportional to elapsed
time. Modelled from the fitted rate (the 2 s row matches measurement to 1–2 dB):

| Duration | Peak | RMS |
|---|---|---|
| 0.5 s | −79.6 | −87.4 |
| **2.0 s** (this case) | **−67.6** | **−75.4** |
| 8 s | −55.6 | −63.3 |
| 60 s | −38.1 | −45.8 |

So widening the global gates to admit it would have to reach −67 dBFS — past
finding (b)'s reference defect (3 LSB × 200 samples, caught at peak −80.8 dBFS)
— **and would fail again the moment a case renders for 4 s.** Instead: the
globals stay as the rounding-class budget, and a case whose residual is a
different class declares its own with `null_tolerance_dbfs`,
`peak_diff_tolerance_dbfs` and a mandatory `tolerance_reason` that the harness
**prints on every run** and records per case in the report (schema `/2` → `/3`).
`osc_naive_sine` is set to **−67.0 / −62.0 dBFS**: 6 dB above measurement, i.e.
sized for twice the observed drift (~5 ULP), on the grounds that there is no
reason to think mac-vs-linux is the widest pair. If Windows exceeds that, the
case fails and someone re-measures — the correct outcome, not a defect.

*The cost, stated rather than buried:* that case keeps full sensitivity to
level, waveform and tuning (0.3 ppm of detuning still fails it) and loses it for
localized damage below ~12 LSB. `voice_midi_note` still covers the same
oscillator at the −86 dBFS default in a fuller chain.

**Verification artifact — five controls through the harness's own `null_test`
and `Case` loader, not a re-implementation:**

```
  C0a osc_naive_sine unmodified              rms= -73.5 peak= -68.7  gates -67/-62   -> PASS
  C0b voice_midi_note unmodified             rms=-123.1 peak= -90.3  gates -100/-86  -> PASS
  C1  osc_naive_sine, same drift 4x larger   rms= -61.5 peak= -56.7  gates -67/-62   -> FAIL
  C2  osc_naive_sine, finding-(b) glitch     rms= -73.5 peak= -68.7  gates -67/-62   -> PASS
  C2  voice_midi_note, finding-(b) glitch    rms=-107.5 peak= -80.8  gates -100/-86  -> FAIL
```

C1 is the one that matters — **a widened gate is still a gate**. C2 on
`voice_midi_note` independently reproduces finding (b)'s numbers (−107.6 /
−80.8 dBFS) on a second platform. C2 on `osc_naive_sine` is the accepted
sensitivity cost, demonstrated rather than asserted. Also asserted and checked:
the override does **not** leak to `voice_midi_note`.

Added `render_harness.py --selftest` — synthesises its audio in memory, needs
no engine or fixtures, and bakes in findings (b) and (e)'s numbers plus the
override plumbing. **Confirmed discriminating**: reverting `null_test` to ignore
its tolerance arguments fails it (RC=1), and the 10.7% boundary is straddled
(1 LSB on 10% of samples passes at −100.3 dBFS, on 12% fails at −99.5 dBFS).

**Learned:**

- **`SynthEditCL_mac.zip` at `https://www.synthedit.com/release_1_6/` is a
  working download for this harness**, and the E1a row's "the engine is a
  download, not a build" is true on mac. It is an `.app`; strip the quarantine
  xattr, then `Contents/MacOS/SynthEditCL` and `Contents/PlugIns` are the
  `--cli`/`--modules` pair. That is the closest thing to what CI would run.
- **Finding (c) extends and slightly retracts.** Extends: the published Azure-CI
  Release build and the local Release build are **byte-identical** on both
  cases, so same-platform bit-exactness spans build *machines* too. Retracts:
  the mac **Debug** build differs from mac Release by 1 LSB on 40/95,999
  samples in `voice_midi_note`. Finding (c) claimed Release-vs-Debug
  bit-exactness from a *Linux* measurement; it does not hold on macOS. Renders
  are still run-to-run bit-exact (checked explicitly).
- **A residual that grows through the render is diagnostic on its own.** Two
  cheap measurements separate the three plausible causes before any theorising:
  per-block RMS (flat = rounding, rising = integrating error) and a small
  lag sweep (a minimum away from zero = a delay). Both were computed here in
  about a minute and turned "the sine case fails" into "the phase increment
  differs by 2.5 ULP", which is what made the fix a mechanism change instead of
  a bigger number.
- **The app version STEP 0.5 asks for IS discoverable on the mac desktop app**,
  unlike Windows (see the 2026-08-14 win entry):
  `/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" /Applications/Claude.app/Contents/Info.plist`
  → `1.26832.0`. `claude` is not on `PATH` here either, so the CLI-version
  route the older mac entries used no longer applies on this box.

**Queue hygiene done in passing, both status-cell-only:**

- **C5 flipped `BLOCKED(C4)` → `TODO`.** C4's three PRs all merged 2026-08-14,
  which is the condition its own blocker names. The `win` NEXT row already said
  "C5, if C4's three PRs have merged"; that "if" has resolved, so the row now
  names C5 outright instead of leaving the next win run to re-derive it.
- **The `mac` NEXT row is re-pointed at D1**, since both it and its stated
  fallback (A13) are now done. D1 by the same argument that made E1a a mac
  item — its own row says whether an AUv3 can open a URL at all "is a factual
  question the macOS box can answer and the other two cannot". Design note
  only, no GATED path, no `.github/workflows/**`, and PLAN's "Price and
  funding" already settles the policy it designs against. Fallback named as S2.

**Not done, deliberately:** the `linux` NEXT row is still the flagged question
the 2026-08-14 win entry left. Nothing measured here bears on it, and guessing
at another platform's lane is the mistake that row exists to avoid.

**Next:** the tolerances are now set from *one* cross-platform pair. Windows is
the third lane and has still never rendered — if it lands inside −67/−62 the
6 dB margin was right, and if it does not, the right response is to re-measure
the drift rate rather than to widen again. Nothing here needs Windows to
proceed. `E1b` (installing `docs/ci/verify.yml`) is still Jeff-only and still
the thing that would make any of this run automatically.

**On the PR's three red checks — checked before leaving them alone, not
assumed.** `windows`/`macos`/`linux` are red on #52, and they are red on #49,
#50 and #51 too, for the same pre-existing reason: `build.yml` configures CMake
at the repo root and **TideSynth has no root `CMakeLists.txt`** (`CMake Error:
The source directory ... does not appear to contain CMakeLists.txt`). That is
**B1** verbatim — the skeleton CI "expected to fail until C7". Job-level
`continue-on-error: true` is why the same workflow reports *success* on `main`
while the individual checks read fail on a PR; do not read that difference as a
regression. `lint` **passes** on this PR, which #49 and #50 could not say.

**Branch/PR:** `tide/mac/E1a-null-tolerances` →
[#52](https://github.com/JeffMcClintock/TideSynth/pull/52)

---

## 2026-08-14 — windows — merge cleanup for A13/P6/C4 (interactive session, Jeff directing)

**Did:** After the scheduled A13 run finished, Jeff took over interactively
and asked to fix merge conflicts and merge the queue's open PRs oldest first.
Four PRs were outstanding across three repos, all touching the same shared
docs (`BACKLOG.md`, `JOURNAL.md`, `JOURNAL-2026-08.md`), so #49 merging first
put #50 and #51 into real conflict. Resolved both, then merged all four in
creation order: [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6)
(07:24) → [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15)
(07:24) → [#50](https://github.com/JeffMcClintock/TideSynth/pull/50) (18:14,
P6) → [#51](https://github.com/JeffMcClintock/TideSynth/pull/51) (20:48, A13).

**Result:**

- **SynthEdit#15 needed a CI re-run, not a fix.** It was failing on
  `CMake Error: Cannot find source file` because it pulls `SynthEditLib` from
  `origin/main`, where the twelve C4 files didn't exist yet. Merged
  `SynthEditLib#6` first, then `gh run rerun --failed` on the same run —
  both `windows-latest` and `windows-2025-vs2026` jobs went from fail to pass
  with no code change, confirming the dependency was exactly what the PR body
  said it was.
- **#50 and #51 both had real git conflicts** in `BACKLOG.md`/`JOURNAL.md`/
  `JOURNAL-2026-08.md`, all from independent same-day rotations racing against
  each other rather than genuine content disagreement. Each resolved the same
  way: whichever side's rotation work was **already accepted on `origin/main`**
  won; a branch's own independent rotation of an entry `origin/main` had
  already archived was dropped as a duplicate rather than merged in twice.
  Confirmed no duplication by grepping each archive for the entry heading
  before and after.
- **The A13 conflict resolution accidentally became A13's own best test.**
  `docs/carve-out.md`, `JOURNAL.md` and `JOURNAL-2026-08.md` all use em-dash
  headings, and the S6 part-1/part-2 cross-reference — the link A13's own row
  was written about — ended up with both halves landing in
  `JOURNAL-2026-08.md` together as a direct result of this session's
  rotations. `check-links.py` (with A13's fix) reports 0 broken on the
  resulting tree; the pre-fix slugger would have flagged that exact link.
- **STEP 4 chore, done live rather than left for a later run:** C4, P6 and
  A13 were all `IN-REVIEW` with every linked PR now observed merged in this
  same session, so flipped all three to `DONE` and moved the rows verbatim
  into `BACKLOG-DONE.md`, newest first. Also fixed a stray blank line left
  inside the carve-out table by the row removal.
- **Found, not fixed: the `linux` NEXT row was already stale on `origin/main`
  before this session started.** It still pointed at P7c, which C4's own PR
  (#49) had already flipped to `DONE` and archived — the PR that archived it
  never updated the pointer that named it. Every other `linux`-platform row
  (X1, X2, R4) is `BLOCKED`, so a scheduled linux run today falls through to
  the `any` fallback, and nobody has run that fallback through the same
  NEEDS-JEFF/workflow-wall screening the `any` row itself just needed for A4.
  Flagged in the row rather than guessed at — the queue already has one
  instance this session of a wrong guess (the original E1a/linux pointer)
  costing a run its whole session on discovery, and a second wrong guess here
  would cost another.

**Learned:**

- **A same-day multi-branch queue racing the same rotation files will always
  produce this shape of conflict**, and it resolves the same way every time:
  trust whichever side is already on `origin/main`, treat the other branch's
  independent rotation of the same entry as a duplicate, and grep the archive
  before/after to prove no entry was dropped or doubled. Doing this by hand
  three times in one session is exactly the kind of load A4 (the auto-merge
  tier) was filed to remove — its row is more urgent than its own text says.
- **`gh run rerun --failed` is the right tool when a cross-repo CI failure's
  cause has already been fixed by merging the other repo** — cheaper and more
  informative than pushing an empty commit to retrigger, since the log shows
  the exact same job going from fail to pass with nothing else different.

**Next:** the `linux` NEXT row genuinely needs someone to work out what a
linux-eligible scheduled run should take, not just notice it's wrong — left
as a flagged question rather than a guess, on purpose. `E1a` still needs a mac
or win run to do its render half.

**Branch/PR:** none — merge-conflict fixes were pushed to the branches being
merged (`tide/mac/P6-cl-codesign-xcode`, `tide/win/A13-check-links-slugger`),
which then merged into `main` via their own PRs. This entry and the
DONE-row/NEXT-row cleanup are committed directly to `main`, interactive
session.

---

## 2026-08-14 — windows — A13 (C4 not re-taken, A4 not takeable — see below)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code Desktop, app version not
discoverable on this box (see **Learned**) · as `tide-rack-bot`

**Did:** Fixed `scripts/check-links.py`'s slug function, which disagreed with
GitHub's on every em-dash heading, and fixed a second defect of the same
character sitting a few lines below it. Verified both against GitHub's own
renderer rather than against my reading of the algorithm.

**Neither NEXT pointer was takeable, and one of them is a standing trap.**

- **`win` → C4 is already done.** The 2026-08-13 windows run completed it; it is
  IN-REVIEW across three PRs ([#49](https://github.com/JeffMcClintock/TideSynth/pull/49),
  [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15),
  [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6)). The
  resume rule makes an open PR from my own platform mine to *continue*, but
  there is nothing to continue — the work is complete and waiting on Jeff.
  I checked both red PRs before leaving them alone, and **neither red is a
  defect in the C4 work**: #49's `lint` is red only on `links`, which is this
  very row and is red on `origin/main` too; SynthEdit#15's `WASDK build check`
  dies at `CMake Error at EditorLib/CMakeLists.txt:10 (add_library): Cannot
  find source file`, which is the cross-repo condition the PR body already
  states — CI fetches `SynthEditLib` from `origin/main`, where the twelve moved
  files do not exist until SynthEditLib#6 merges. C2 and C3 failed that same
  check for that same reason, and C3 merged anyway. Structural; not fixable
  from a branch.
- **`any` → A4 cannot be done by a scheduled run at all, and its row does not
  say so.** A4 is an auto-merge *action*, i.e. a file under
  `.github/workflows/**`, and the bot token is deliberately `repo` scope with
  no `workflow` — measured this run, not assumed: `X-OAuth-Scopes: repo` from
  the API response headers. The push is rejected however correct the work is.
  **A10 and A12 hit the identical wall** (both name `.github/workflows/` in
  their own scope lines), and **A9 has an open NEEDS-JEFF prerequisite** —
  TIDE's product philosophy as the auto-reject filter — which would change what
  gets built, so STEP 2 rules it out. That leaves **A13** as the topmost `any`
  item a scheduled run can actually finish, and A13's own row had already
  argued it should precede A4 anyway ("a noisy lint erodes trust in the other
  three checks fast"). Updated the `any` NEXT row to record all of this.

**Result:** Both bugs reproduced, fixed, and verified.

*The bug, exactly.* `slugify()` did `re.sub(r'\s+', '-', s)` — one hyphen per
*run* of whitespace. GitHub emits one per *space*. The line above had already
deleted the em-dash, leaving the two spaces that surrounded it, so
`## 2026-08-13 — macos — S6 (part 2 of 2)` produced
`2026-08-13-macos-s6-part-2-of-2` here and `2026-08-13--macos--s6-part-2-of-2`
on GitHub. The fix is `s.replace(' ', '-')`, plus deleting tabs and other
non-space whitespace outright, which is what GitHub does with them.

*The second defect, which was not in the row.* `anchors_of()` never tracked
code fences — `main()` did, but `anchors_of()` did not — so **nine fenced lines
across four files were registered as real anchors**, among them `JOURNAL.md`'s
own entry template `## YYYY-MM-DD — <machine> — <BACKLOG id>`,
`#include "it_empty.h"` in `docs/c8-it-empty-header.md`, and three `#0`/`#1`/`#2`
gdb backtrace frames. That is a false *negative* — a link to
`#include-it_emptyh` passed the check. Same both-directions character as the
slug bug, same function, so fixed in the same pass.

*The verification artifact — an A/B against GitHub's own renderer.* Fetch each
file through the contents API with `Accept: application/vnd.github.html`, which
returns it rendered by GitHub's real markdown pipeline with anchors intact
(`<a id="user-content-…" class="anchor">`), and compare every anchor to what
this script generates for the same bytes:

```
files 28 | headings compared 254 | mismatching files 0
```

**254 of 254, zero mismatches.** Two negative controls, so the A/B is not
passing vacuously:

```
CONTROL 1  old slugify (collapsing run)        headings wrong: 111   files affected: 23
CONTROL 2  new slugify, fences NOT skipped     headings wrong:  34   files affected:  4
FIXED      new slugify, fences skipped         headings wrong:   0   files affected:  0
```

*Deliberate breaks, all three as specified in the row's Accept clause:* a
nonexistent anchor fails (RC=1); a link written in the **old collapsed form**
now fails (RC=1) — that is the false negative closing, and it is the half that
matters; the correct GitHub form passes (RC=0).

*Tree state:* 186 relative links, **0 broken**. The one previously-flagged line
(`JOURNAL.md:376`) was read and is exactly the false positive A13 predicted —
the first intra-journal anchor anyone wrote.

*Added `--selftest`* to the same script: six golden slugs **read off GitHub's
renderer**, plus a duplicate/fence case, baked in and offline so the regression
is permanent rather than something a future run must remember to re-measure.
Confirmed discriminating — reverting `slugify` to the old algorithm fails 5 of
its 6 cases, RC=1. Also implemented GitHub's duplicate-heading suffixes
(`-1`, `-2`), which no heading in the tree exercises today but `anchors_of()`
was silently collapsing into one.

**Learned:**

- **`/markdown` is not an oracle.** The obvious endpoint for "what would GitHub
  render this as" emits headings with **no `id` attribute at all**, so the
  first A/B came back 0-for-254 and looked like a catastrophic failure of the
  fix rather than of the measurement. The endpoint that works is
  `GET /repos/{owner}/{repo}/contents/{path}` with
  `Accept: application/vnd.github.html`. Anyone verifying anchor behaviour
  again should start there and skip the hour.
- **The em-dash convention and the checker were on a collision course from the
  start.** Every journal entry heading in this repo uses em-dashes, so the
  moment anyone wrote the first intra-journal anchor link the check went red —
  which is exactly what happened, and A3 could honestly claim zero false
  positives when it landed only because nobody had written one yet.
- **The app version STEP 0.5 asks for is not discoverable on this box.** `claude`
  is not on `PATH` under the desktop app, and there is no `app-*` directory or
  `package.json` under `%LOCALAPPDATA%\Claude` carrying a version. The linux and
  mac entries record `Claude Code CLI 2.1.220` because those boxes run the CLI.
  So the provenance line's `app <version>` field is silently unfillable on
  Windows-under-desktop, and `check-prompt-provenance.py` cannot catch that —
  it only looks for the literal `**Prompt:**` marker, not for the fields after
  it. Recording it in prose here rather than inventing a number.
- **Two lint scripts cannot be fed a process substitution on this box.**
  `python scripts/check-backlog-diff.py <(git show origin/main:BACKLOG.md) …`
  fails with `FileNotFoundError: '/proc/1398/fd/63'` — Git Bash creates the fd,
  Windows Python cannot open it. Write the base version to a real temp file.

**Next:** A13's PR is [#51](https://github.com/JeffMcClintock/TideSynth/pull/51)
and should merge **after** [#49](https://github.com/JeffMcClintock/TideSynth/pull/49)
— both prepend to `JOURNAL.md` and both rotate `JOURNAL-2026-08.md`, so
whichever lands second needs a rebase, and #49 is the older and larger of the
two. `BACKLOG.md` does not collide: #49 touches the `win` NEXT row and the
C4/C9/C11/P7c rows, this one touches the `any` NEXT row and A13.

The `any` lane needs a decision, not a run: **A4, A10 and A12 are all
`.github/workflows/**` work that the bot token structurally cannot push**, and
all three sit in the queue marked TODO as though a scheduled run could take
them. Each will burn a session on discovery until someone re-marks them.
That is the same shape as **A12's own finding** — a box that cannot proceed and
nothing escalating it — one level up, applied to the queue instead of to a box.

**Branch/PR:** `tide/win/A13-check-links-slugger` →
[#51](https://github.com/JeffMcClintock/TideSynth/pull/51)

---

