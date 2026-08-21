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

## 2026-08-22 — windows — R2: the Windows installer, and the payload it must carry is not the file the build emits

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** took **R2**, the Windows installer. It is the first `win`-marked
takeable row this platform has had — the release track unblocked yesterday
([#270](https://github.com/JeffMcClintock/TideSynth/pull/270)) and no windows
box had run since. Shipped `installer/windows/TIDE-Rack.iss` +
`scripts/package-windows.ps1`, which produce **`TIDE-Rack-Windows.exe`
(3.0 MB)** and **`TIDE-Rack-Windows.zip` (1.4 MB)**, the two assets
[docs/distribution.md](docs/distribution.md) names.

### Two things had to be fixed before anything could be packaged

**1. `cmake -S . -B build` does not build on this box, and the error names the
wrong thing.** CMake picks the Visual Studio **BuildTools** instance, which has
no MFC, and the build dies:

```
syntheditlib-src\EditorLib\MfcDocPresenter.cpp(4,10): error C1083:
  Cannot open include file: 'afxres.h': No such file or directory
syntheditlib-src\EditorLib\CContainer.cpp(8,10): error C1083: ...
```

Those are the two files **P3** exists to de-MFC, so the failure reads as a known
problem in the code rather than an unknown one in the environment.
`18\Community` has `atlmfc`; `18\BuildTools` does not. Fixed with
`-DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community"`
— a flag `docs/building.md` records **only for the old SE16 path**, not for
C7d's root `CMakeLists.txt`, which is the one a stranger now uses. With it:
configure rc=0, build rc=0, **zero `error C` / `error LNK` lines**, producing
`TIDE-Rack.gmpi`, `TIDE-Rack.vst3` and `TIDE-Rack.exe`. Built with **no
`*_FOLDER_OVERRIDE`s**, the way CI does — all eight dependencies fetched fresh,
so nothing here is contaminated by the concurrent session's dirty trees.

Incidentally this is the first Windows confirmation of **N1a's PDB fix**: the
three PDBs are `TIDE_Rack.pdb` / `TIDE_Rack_VST3.pdb` /
`TIDE_Rack_STANDALONE.pdb`, distinct, and the parallel link that produced
`LNK1201` does not recur.

**2. The plug-in the build produces does not work.** That is **S36**, filed
below, and it is the more important of the two.

### The decision that is actually R2's: the payload is a VST3 BUNDLE

`gmpi_plugin.cmake`'s VST3 block gives Windows a bare `SUFFIX ".vst3"` — macOS
gets a real bundle, Linux gets one assembled by a POST_BUILD copy, Windows gets
neither. A bare DLL is a legal VST3 and hosts load it. But TIDE has data it
cannot work without — four pin XMLs and six rack prefabs — and **a bare DLL has
nowhere to keep it**. Its only sibling directory is
`C:\Program Files\Common Files\VST3`, shared with every other vendor's
plug-ins, where a folder called `Prefabs` and a file called `Converters.xml`
have no business being.

So the asset ships:

```
TIDE-Rack.vst3\Contents\x86_64-win\TIDE-Rack.vst3
TIDE-Rack.vst3\Contents\Resources\{ControlsXp,Converters,MidiPlayer2,VaFilters}.xml
TIDE-Rack.vst3\Contents\Resources\Prefabs\*.synthedit
```

and the runtime already reads exactly that: `BundleInfo.cpp:670-684` sets
`pluginIsBundle` from *"a path element with an extension, followed by
`Contents`"*, and `getResourceFolder()` (`:266-271`) then returns
`<bundle>\Contents\Resources\`.

### Verified in three conditions, and the third is the shipped asset itself

Every one is the **same binary**, differing only in where it sits:

| # | condition | what the plug-in printed |
|---|---|---|
| **a** | flat DLL, exactly as the build leaves it | all four XMLs `missing from bundle resources`; `no Prefabs folder in bundle resources - the rack module browser will be empty` |
| **b** | flat, with the staged resources copied **beside** the binary | `ControlsXp.xml enriched 2 of 18` (+3 more); **`6 rack prefab(s) seeded from the bundle`** |
| **c** | **unzipped from `TIDE-Rack-Windows.zip`**, run from inside the bundle | the same four `enriched` lines; **`6 rack prefab(s) seeded from the bundle`**; `rack built for 48000 Hz` |

**(b) proves the runtime rule, (c) proves the shipped asset obeys it, and (a) is
the control that makes either mean anything.** (c) is run by dropping the
STANDALONE `TIDE-Rack.exe` *inside* the shipped bundle beside the DLL: its
module path is then the same shape the host sees, so the same `BundleInfo`
branch runs, and unlike the VST3 it prints to a stderr I can read. That
substitution is the one thing about (c) worth distrusting, and it is why (b)
exists.

### The installer is proven, not merely compiled

Windows has no counterpart to macOS's `installer -target <sandbox volume>`: the
destination is a fixed machine path under Program Files, so a real run needs
elevation and this session has none (`IsInRole(Administrator)` = **False**).

So `-SelfTest` compiles **the same `.iss` a second time** with two `#define`s
overridden — `Vst3Dir` at a scratch folder, `PrivilegesLevel` at `lowest` — and
runs that copy silently. **11 files installed, every one SHA-256 identical to
the staged payload; none missing, none differing, none extra; and the
uninstaller removed the bundle whole.** The two overrides are the *only*
difference between the two compilations: same `[Files]`, same
`[UninstallDelete]`, same `[Code]`. They are compile-time defines rather than
runtime hooks precisely so the shipped installer cannot be talked into using
them.

Inno Setup was not on this box. Installed **user-scope**
(`winget install --id JRSoftware.InnoSetup --scope user`) so it landed in
`%LOCALAPPDATA%\Programs`, needing no administrator and no change to
`Program Files`.

### Signing: not done, not claimed, and the block is unverified

It runs only when `AZURE_TENANT_ID` / `AZURE_CLIENT_ID` / `AZURE_CLIENT_SECRET`
are set, under the `SynthEdit Limited` identity R1(a) settled, with endpoint /
account / profile defaults taken from `SE16/SynthEdit_store_win.yml:199-211`.
**It has never run.** `ArtifactSigning@1` is an Azure Pipelines task with no
local or GitHub Actions equivalent, so the script drives `signtool.exe` with the
Azure Code Signing dlib instead — the documented stand-in, whose dlib is not
installed here. It **throws** rather than silently skipping if credentials are
present without `TRUSTED_SIGNING_DLIB`, so it cannot claim a signature that did
not happen. **R5 owns the secret store and should treat that block as a starting
point to test, not as working code.**

### What did NOT work, so nobody repeats it

**A portable REAPER did not run unattended.** Copying
`C:\Program Files\REAPER (x64)` to a scratch folder with a `reaper.ini` beside
`reaper.exe` and calling `-renderproject` produced **zero bytes of output and no
wav in 240 s** — it stalls on a first-run modal. It was also **not isolated**:
Jeff's `%APPDATA%\REAPER\REAPER.ini` and `reaper-fxtags.ini` carry that
attempt's timestamp. Nothing of his was deleted or edited by hand, but the run
touched them, and this entry is where that is recorded rather than left to be
noticed. The scratch copy is gone. Separately, **`render-and-measure.py` is
mac-only as written** — `REAPER = "/Applications/REAPER.app/Contents/MacOS/REAPER"`,
hardcoded at line 51 — so the five audio fixtures cannot be measured from this
box without changing that script, which was outside R2 and is not changed here.

**Learned:**

1. **A packaging script's real job is deciding what the shipped layout IS, not
   copying a build tree into a zip.** Half of R2 turned out to be the discovery
   that the artifact the build emits cannot be installed correctly, and no
   amount of installer scripting would have surfaced that — running the thing
   did.
2. **`afxres.h` names a missing header and means a wrong Visual Studio
   instance.** Two VS 18 instances on one box, only one carrying MFC, and CMake
   picks by its own rule. The two failing files are exactly the two P3 exists to
   de-MFC, which is what makes the error read as a code problem.
3. **Windows has no sandboxed installer run, so the way to prove one is to
   compile it twice.** Two `#define`s and a SHA-256 tree comparison is a stronger
   claim than a real elevated install would have been anyway, because it
   compares against the payload rather than against expectations.
4. **The app version STEP 0.5 asks for IS discoverable on this box**, contrary to
   the standing lesson from A13 (2026-08-14): `appVersion: '1.34493.1'` sits in
   `%LOCALAPPDATA%\Claude\Logs\main.log`. One grep.
5. **A "portable" REAPER on Windows is neither portable nor unattended.** It
   stalled on a modal and still wrote to the user's roaming profile. If a windows
   run needs a host, the honest options are Jeff's own REAPER — saying what was
   touched — or nothing.

**Next:**

1. **S36 is the row to take next, and its (a) is minutes** —
   `SynthEditSem/CMakeLists.txt` is TIDE's own file. Its (b) is the better fix
   and is PR-GATED in GMPI; it would also bear on
   [#271](https://github.com/JeffMcClintock/TideSynth/issues/271). **Any Windows
   user of a developer build has an empty module browser today.**
2. **R5 must test the signing block before relying on it** — see above. Until
   then `TIDE-Rack-Windows.exe` draws a SmartScreen warning and a UAC prompt
   naming an unknown publisher, and the script says so on every run.
3. **`docs/building.md` should carry `CMAKE_GENERATOR_INSTANCE` for the root
   `CMakeLists.txt` path**, not only for the SE16 one. Not done here — it is
   N1b's neighbourhood and this run had one item.
4. **This platform's queue is empty again once R2 merges.** P3 is still the only
   other `win` row and is still GATED; the NEXT cell now says so without naming a
   dead row.

**Machine left clean.** TideSynth's checkout was never switched — all work
happened in a **separate worktree** at `C:\SE\_r2`, because a concurrent session
is live in `C:\SE\TideSynth`: `modules/common/TidePathTracer.cpp` was dirty with
**227 lines of real content** (Kulla-Conty multiple-scattering compensation;
`git diff --ignore-all-space` non-empty) and local `main` carried **two unpushed
commits**, both `tide_render`. Neither was committed, reverted nor stashed, and
both have since landed on `origin/main` as somebody else's work. The worktree is
removed. Nothing was written to `C:\Program Files`; Inno Setup went to
`%LOCALAPPDATA%\Programs`. `%APPDATA%\TIDE Rack\` did not exist before this run,
was created by the standalone launches, and was removed. `%APPDATA%\REAPER` is
as noted above. No other repo was touched — this item is entirely inside
TideSynth.

**`C:\SE\TideSynth` ends this run parked on `review/renderer`, and that is NOT a dead agent run — do not reset it.** The concurrent session created `review/renderer-base` and `review/renderer` at 08:52–08:53 and committed to them **as Jeff McClintock** (`10247d3ba`, *"The renderer's arc since its last review, as one commit"*); the reflog shows the checkout moving `main` → `review/renderer-base` → `main` → `review/renderer` while this run was pushing. It was on `main` when this run started and this run never switched it. Putting it back would yank a live session off its own branch, so STEP 5's "return every working copy to its default branch" is deliberately not applied here — the third dirt category's reasoning (never touch work in progress that is not yours) governs the branch as much as the files.

**Filed as S36, not S35.** The macos box filed a different **S35** (P11's mac half)
hours after this row was written and landed first, so mine was renumbered on
rebase -- **A23's duplicate-id hazard, live**, caught by reading the rebased file
rather than by the lint, which sees one id per row and cannot see two rows
racing from different branches.

**Branch/PR:** `tide/win/R2-windows-installer` — TideSynth only. Two new files,
plus the R2 and S36 rows.

---

---

## 2026-08-22 — macos — S27: four suspects eliminated, and the reference box turns out to be x86_64

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **S27** — `tide_render`'s image references failing on mac — and
eliminated four candidate causes by measurement. No fix; the row wants a
decision, and the decision is better made knowing what it is not.

### The experiment only a Mac can run

S27's hypothesis is *"libm transcendental divergence ... or an x86-vs-arm64
divergence — labelled a hypothesis, not a diagnosis"*. Those two are separable on
exactly one kind of machine: an Apple Silicon Mac, which builds and runs **both**
architectures against **one** OS, one libm, one compiler.

`modules/common` is a self-contained CMake project with no GMPI or SDK
dependency, so this cost two configures.

**arm64 render vs x86_64 render, same machine:**

```
ok  knob        0.168%   worst delta 9        ok  knob (fast)        0.000%  delta 1
ok  materials   0.125%   worst delta 16       ok  materials (fast)   0.000%  delta 1
ok  shapes      0.396%   worst delta 14       ok  shapes (fast)      0.000%  delta 1
ok  glass       0.146%   worst delta 10       ok  glass (fast)       0.000%  delta 1
ok  glow        0.000%   worst delta 1        ok  glow (fast)        0.000%  delta 1
```

**All ten pass**, inside the existing 0.400% / delta-40 limits. Against the
committed references, both architectures fail at **35–67%**, worst delta 46–142.

Two orders of magnitude apart. **ISA is not what moves the image.**

### Three more suspects, three more eliminations

**Renderer drift.** Eight commits touched the tracer after `37d65d5`, and the
current tip `4128291` (*telecentric depth of field*) landed without re-baking —
so "the references are just stale" was the obvious reading. Built at `25e0bf6`,
the last commit that *did* update them: **identical figures to three decimals**
(35.301 / 35.007 / 66.917 / 54.465 / 61.535), the same numbers `main` gives. The
references never matched a mac build at any commit.

**The RNG.** A hand-rolled PCG over `uint32`/`uint64` shifts
(`TidePathTracer.cpp:84`). No `std::uniform_real_distribution`, no
`std::mt19937` — the usual cross-stdlib trap is absent, so the sample sequence is
bit-identical everywhere.

**Inherent nondeterminism.** The five `(fast)` variants pass at **0.000%** on both
architectures, and the code explains itself: fast mode uses *"a fixed sub-pixel
GRID, not jittered draws ... bit-deterministic without the RNG being involved at
all"* (`:2539`).

### What survives, with an argument instead of a guess

The divergence is confined to the transcendental-heavy Monte Carlo path.
`shadeFast()` contains **1** transcendental call; the tracer overall contains
**19**. `sqrt` is excluded from that count — IEEE-754 requires it correctly
rounded, so it cannot diverge. Few transcendentals → bit-stable. Many → 35–67%.

### And the reference box is x86_64, but not a Mac

The x86_64 build reproduces all five fast references **bit-exactly, worst delta
0**, while arm64 is off by one — yet x86_64-on-macOS still fails the full scenes
identically to arm64. So the references came from an **x86_64 machine running a
different libm**: Windows or Linux.

### Two corrections to the row, and one new problem

The row says *"5 of 5 scenes fail"*. It is **5 of 10 checks** — every fast variant
passes, and that asymmetry is the most useful fact available.

I also misread the exit status once: piping the tool into `tail` and echoing `$?`
reports **`tail`'s** status, so the test looked like it passed while printing five
failures. It exits **1**, correctly.

**New, latent:** `shapes` cross-ISA is **0.396% against a 0.400% limit** — one
percent of margin. Even with correct references that scene is borderline flaky.

**Learned:**

- **An Apple Silicon Mac separates ISA from OS/libm in a way no other box can** —
  two architectures, one operating system. When a cross-platform difference is
  suspected, that is the cheapest possible discriminator, and it costs one extra
  `-DCMAKE_OSX_ARCHITECTURES`.
- **`$?` after a pipeline is the LAST command's status.** `tool | tail` then
  `echo $?` reports `tail`. Use `${PIPESTATUS[0]}`, or don't pipe when the status
  is what you came for — I briefly recorded a failing test as passing.
- **Check whether a hand-rolled RNG is actually the portable kind before blaming
  it.** `std::uniform_real_distribution` differs between libc++ and libstdc++ and
  is the classic cause; a PCG over integer shifts is not, and ruling it out took
  one grep.
- **`sqrt` is not a cross-platform divergence source.** IEEE-754 requires it
  correctly rounded. `pow`/`exp`/`log`/`sin`/`cos` are not required to be, and are
  where libm implementations actually differ — so count those separately.
- **A passing subset is a control, not noise.** The fast-mode scenes passing at
  0.000% is what turns "the renderer is nondeterministic" into "the renderer is
  deterministic except in the path that calls transcendentals".
- **Rebuild at the commit that produced the artifact before assuming drift.** It
  cost one build to kill the most plausible explanation, and believing it would
  have sent someone to re-bake references that were never right.

**Next:** the decision is still **Jeff's** — per-platform references, a tolerance
derived from measured cross-platform residual, or pinning the math — but it can
now be made knowing ISA and the RNG are irrelevant, that a bit-stable subset
already exists to build on, and that `shapes` needs headroom regardless.
**Whoever owns it should also decide where the test runs**, since TideSynth's root
force-disables `TIDE_RENDER_PREVIEW` and no CI has ever executed it.

**Branch/PR:** `tide/mac/S27-isa-vs-libm` — TideSynth, backlog and journal only.

---

## 2026-08-22 — macos — E1c: the hypothesis was already refuted by a table in this repo

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **E1c**, could not run its harness here, and found that the
experiment it asks for is unnecessary — the hypothesis it names is contradicted
by numbers already written down. Then fixed the thing that made establishing
that take an afternoon.

### The row asks the wrong question

E1c says to test whether *"the core oscillator's phase increment is
cross-platform stable where the naive one's is not"*. E1a's own table settles it:

| case | oscillator | pitch input | residual | E1a's class |
|---|---|---|---|---|
| `osc_naive_sine` | `SE Oscillator (naive)` | **not connected** | **−73.5 dBFS** | 12 LSB, *"not rounding"* |
| `voice_midi_note` | `SE Oscillator (naive)` | keyboard | −123.1 dBFS | 1 LSB, *"pure rounding"* |
| `prefab_oscillator` | `Oscillator` (core) | patch point, 5 V | −131.1 dBFS | rounding |
| `prefab_filter` | `Oscillator` (core) | patch point, 5 V | −121.4 dBFS | rounding |

The top two are **the same module**, measured macOS-vs-Linux-goldens in one run,
identical on three independent engines — and they are **50 dB apart**. The module
cannot be the variable.

What co-varies instead is visible in the scripts: `osc_naive_sine` is the only
case whose **pitch input is connected to nothing**, so it free-runs on the module
default. Every case that drives pitch — from a keyboard or from a pinned patch
point, naive oscillator or core — is rounding class.

So `prefab_oscillator` and `prefab_filter` are rounding-class cases carrying
phase-drift-class gates, and their `tolerance_reason` cites a mechanism their own
scripts do not exhibit. And the settling experiment is now **one** case, not two:
the naive oscillator *with* pitch pinned. Rounding class confirms the pitch
reading; −73 dB class restores the module reading.

### The part that cost the time, and the part I fixed

Every number above needed its platform pair established before it meant
anything, and **nothing in `tests/references/` recorded that.** It took a journal
entry from nine days earlier plus a sentence buried in a case description
(*"REFERENCE SEEDED ON macOS, 2026-08-18 — unlike the other two, which came from
Linux"*) to work out which WAV came from where.

The four were comparable at all only because one run happened to produce them —
luck, not method. A null-test residual means *"rounding, ignore it"* if both
sides ran on one platform and *"cross-platform drift, size your gates for it"* if
they did not, and those are opposite conclusions from the same number.

So `--update-refs` now writes `tests/references/<case>.provenance.json` recording
system, release, machine, engine build and the reference hash. Six existing
references backfilled, honestly graded: three `reconstructed` with the evidence
quoted, and **three `unknown`** — `prefab_envelope`, `prefab_filter`,
`prefab_midi` — where nobody wrote it down and I could not establish it.

`prefab_filter` is the interesting one. E1c calls its −121.4 figure a Linux
verify *"against macOS-seeded references"*, which implies Darwin — but the
reference file was added on 2026-08-20 and the measurement is dated 2026-08-19.
Those do not line up, so recording Darwin would have been inventing a fact that
merely sounded right. It is `unknown`.

### Verification

`--selftest` needs no engine, which is the whole point given the harness needs
`SynthEditCL` and a Linux box. Six new cases, and both negative controls bite:
hardcode the platform → the platform check fails; write the sidecar under the
wrong name → the path check fails. Restored, the suite passes.

### What I did NOT do

**The gates are unchanged.** E1c's Accept requires a positive control —
tightened gates passing on both platforms while failing a deliberate regression —
and `verify.yml` is `ubuntu-24.04` only and needs `SynthEditCL` from the private
repo. **Changing a gate without that control is exactly what created this row.**

**Learned:**

- **Before designing an experiment, check whether the repo already ran it.** E1c
  named a hypothesis two existing measurements refute. The table was in
  `JOURNAL-2026-08.md` and the module names were in the case files; nothing
  needed to be rendered.
- **When two cases differ by 50 dB, list every way they differ before believing
  the first explanation.** "Naive vs core oscillator" was the obvious reading and
  it was wrong — the same module appears on both sides. The undriven pitch input
  was the only variable that actually tracked the split.
- **A measurement without its provenance is not a measurement.** A null-test
  residual supports opposite conclusions depending on whether the two sides ran
  on the same platform. Record the platform pair *with the artifact*, at the
  moment it is produced — reconstructing it later is archaeology and sometimes
  impossible.
- **Grade backfilled facts explicitly.** `measured` / `reconstructed` / `unknown`
  keeps a later reader from treating a plausible inference as a record. Marking
  `prefab_filter` unknown was more useful than recording the Darwin the row
  implies, because the dates do not support it.
- **A harness that needs an engine should still have a mode that does not.**
  `--selftest` is why this change is verified at all from a box that cannot run
  the real suite.

**Next:** the one-case experiment (naive oscillator, pitch pinned to 5 V) settles
the mechanism and wants a **Linux** box, since that is where `verify.yml` runs.
Then the gates can be justified rather than inherited. **Re-seeding
`prefab_envelope`, `prefab_filter` or `prefab_midi` fixes its `unknown` record as
a side effect** — worth doing on whichever platform is going to own them.

**Branch/PR:** `tide/mac/E1c-reference-provenance` — TideSynth.

---

## 2026-08-22 — macos — S31: the trap only exists on Linux, and that is why writing it down four times did not work

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **S31** — `pkill -f <pattern>` killing the shell that runs it —
and shipped `scripts/kill-named.sh` with a 7-case suite. The useful finding is
not the script. It is *why* the lesson had failed to stick through two journals,
one doc and three repeats.

### The negative control refused to reproduce

First move was to reproduce the bug, since a fix for a bug you have not seen is
a guess. Backgrounded a probe, then ran `pkill -f $PAT` from a shell whose own
command line contained `$PAT` — the exact shape of all three recorded hits.

The shell survived. Exit 0.

That is either a broken test or a wrong premise, and the way to tell is to ask
whether the signal was even sent. Trapping `TERM` in the calling shell answers
it:

```
  trap TERM; kill -TERM $$      -> ">>> got TERM", rc=143     (it CAN die)
  trap TERM; pkill -f $PAT      -> ">>> finished normally"    (nothing arrived)
```

Not ignored — **never delivered**. Then `man pkill`:

> **-a** Include process ancestors in the match list. By default, the current
> pgrep or pkill process **and all of its ancestors are excluded**.

**BSD `pkill` excludes ancestors by default. GNU procps excludes only itself.**
So the trap is Linux-only. All three recorded hits were on the linux box. A mac
or windows run cannot reproduce it however carefully it tries — *"it worked when
I tested it"* was true, and useless.

That is the actual reason four retellings failed: two of the three boxes reading
the lesson could never see the behaviour it described.

### The platform split forced the test design

If cases only check behaviour — *does it kill the target, does the shell live* —
then on macOS **they pass whether or not the filter works**, because the OS is
already doing the filtering. I did not reason my way to that; I broke the
ancestor walk on purpose (`p=1` before the loop, which is precisely the Linux
bug) and re-ran:

```
  PASS  kills the named process
  PASS  the calling shell survives (rc=0)      <- the bug is LIVE and invisible
  FAIL  ancestor list contains the calling shell
  FAIL  ancestor list contains the grandparent
```

So the suite asserts the ancestor list **directly**, through a
`--print-ancestors` mode, instead of inferring it from behaviour the OS would
mask. Second control, `is_ancestor` forced true: case 1 fails — it spares
everything and kills nothing. Both breaks are caught; the restored script is 7/7.

### What I did not verify

**The suite has never run on Linux or Windows** — which is where the bug lives.
It is POSIX `sh` with a `ps`-based ancestor walk and no `/proc` dependency, but
that is an argument, not a measurement. One `sh tests/s31_kill_named_test.sh` on
the linux box closes the row.

**Learned:**

- **When a negative control refuses to reproduce a documented bug, that is a
  result, not a broken harness.** Chasing "why didn't it fire" turned a
  three-line script into the actual explanation for why the lesson never stuck.
- **`pkill -f` self-kill is a Linux-only trap.** BSD (macOS) excludes the caller
  and all ancestors by default; GNU procps excludes only the caller. Check
  `man pkill` for `-a` before assuming a `pkill` behaviour is portable.
- **A lesson that two of three boxes cannot reproduce will not stick by being
  written down again.** The fix is a mechanism that behaves identically
  everywhere, or the platform caveat stated up front so the boxes that cannot
  see it know they are not the audience.
- **Test what the OS might be doing for you, directly.** If a platform makes
  your safeguard redundant, your behavioural tests pass with the safeguard
  removed — so they are not testing it. Assert the internal state instead, and
  prove it by breaking the code and watching the right case fail.
- **Ask whether the signal was delivered, not whether the process died.**
  Trapping the signal separates "not sent" from "sent and ignored", which are
  different bugs with different fixes.
- **Silence expected noise in test output.** Every passing case printed
  `Terminated: 15` from job control; starting probes in a detached subshell
  removes it. Output that always appears is output nobody reads, so a real
  failure hides in it.

**Next:** **the linux box should run `sh tests/s31_kill_named_test.sh` once** —
that is the only outstanding evidence, and it is the platform the bug is real on.
Nothing else blocks the row.

**Branch/PR:** `tide/mac/S31-kill-by-pid` — TideSynth.

---

## 2026-08-22 — macos — R4a: CLAP was in nobody's build, and my own Linux fix was a half-fix

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **R4**, split it, shipped the half that is not Linux-bound as
**R4a**, and spent most of the item repairing a GMPI fix I had already opened —
which is the part worth reading.

### R4's Accept named a format nothing built

R4 wants a Linux tarball carrying *"the VST3 and CLAP bundles"*.
[docs/distribution.md](docs/distribution.md) lists CLAP as shipped on all three
platforms, and R2 and R3 say the same thing. But `FORMATS_LIST` in
[SynthEditSem/CMakeLists.txt](SynthEditSem/CMakeLists.txt) read
`GMPI VST3 STANDALONE`, so **no platform was building a CLAP at all.** Three
installer rows had been waiting on a one-word change nobody had noticed was
missing, because a missing format produces no error — only an absent file.

It cost one word. GMPI already had the wrapper wired, and the format picked up
N1a's `OUTPUT_NAME` for free.

**Measured on the artefact, not the build log** — the build succeeding says
nothing about whether the thing it made is loadable:

```
TIDE-Rack.clap/Contents/MacOS/TIDE-Rack        arm64
  nm -gU | grep clap_entry   ->  _clap_entry        (exactly 1 entry point)
  Contents/Resources/Prefabs/{Oscillator,Envelope,MidiCv,Filter}.synthedit
configure rc=0, build rc=0, 0 CMake errors, 0 compiler errors
```

### A suspect eliminated is still progress

The CLAP bundle's `CFBundleIdentifier` is **present and empty**
(`<string></string>`) — I first wrote "missing", because `PlistBuddy -c Print`
returns nothing either way, and the distinction turned out to name the cause.
It looked like a CLAP-specific plist gap until I checked the other formats as a
control: `TIDE-Rack.vst3` and `TIDE-Rack.gmpi` are identical, and only the
`.app` has a real one. **So CLAP is eliminated — this is pre-existing and
belongs to the signing track, not here.**

**One cause explains both symptoms.** The plugin formats fall through to CMake's
stock `MacOSXBundleInfo.plist.in`, and `MACOSX_BUNDLE_GUI_IDENTIFIER` is set
only on the STANDALONE branch (`GMPI/gmpi_plugin.cmake:775`), so the
substitution yields an empty string — and that same stock template is why all
four bundles also declare `CFBundlePackageType=APPL` rather than `BNDL`. The fix
is one property per format, not a new plist. Filed as **R8** rather than fixed
in passing.

What `codesign` actually does with it is worth recording, because I tested it
instead of assuming: it **succeeds**, inventing
`TIDE-Rack-555549444341029c5cd537c59001e63d20c200d3` from the executable name
and a hash. So it is not a signing blocker, but that string is what Gatekeeper
and notarization tickets key on. R5 is where it bites.

### The part I got wrong: I fixed the lines my grep returned

Earlier this session I opened [GMPI#6](https://github.com/JeffMcClintock/GMPI/pull/6)
for the Linux VST3 bundle-name mismatch N1a introduced. The linux box had found
the same defect independently, **measured on Linux** rather than reasoned from
the CMake, and filed it as [#271](https://github.com/JeffMcClintock/TideSynth/issues/271).
It closed its own [GMPI#7](https://github.com/JeffMcClintock/GMPI/pull/7) under
the first-filed rule and left me a review. All three of its points were correct
and all three were mine to fix.

**1. It was a half-fix that would have made things worse.**
`SynthEditSem/CMakeLists.txt` stages the VST3's `Resources/` into a path spelled
to match GMPI's expression character-for-character. Merging GMPI#6 alone gives
Linux *two* bundles:

```
TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so    <- loadable, NO resources
TIDE_Rack_VST3.vst3/Contents/Resources/Prefabs/…     <- resources, never loaded
```

A loadable plugin with no prefabs is the exact failure **S21** exists to prevent.
[#274](https://github.com/JeffMcClintock/TideSynth/pull/274) is the companion and
the two must land together.

**2. I missed a second site.** `copy_plugin()` ships the same mismatched pair into
`~/.vst3`. I had grepped for uses of the `vst3_bundle` *variable* and declared the
class closed; that site spells `${TARGET_NAME}.vst3` directly, so my search could
not have found it. **The grep was a proxy for the class and I mistook it for the
class.**

**3. My edit rewrote 1,274 lines.** `gmpi_plugin.cmake` is pure CRLF. I edited it
with Python `open(p, 'w')`, whose text mode wrote LF, so a three-line change
arrived as 1,288 insertions / 1,274 deletions — unreviewable, and `git blame`
destroyed. Redone in binary mode: **10 insertions / 4 deletions**, matching the
linux box's own diff exactly.

### One I deliberately did not fix

`${SUB_PROJECT_NAME}.appex` at `gmpi_plugin.cmake:1057`/`:1067`/`:1076`/`:1095`
is structurally identical — `$<TARGET_BUNDLE_DIR:...>` copied to a target-named
destination. I flagged it in the PR body and left it: an appex resolves through
its `Info.plist` rather than by name-globbing, so the consequence is probably
cosmetic, and TIDE builds no AU3, so **neither box can test it either way.** A
speculative edit in a PR-GATED repo is worse than a note.

**Learned:**

- **A format missing from a build list produces no error, only an absent file.**
  CLAP was named in `docs/distribution.md` and in three rows' Accept clauses
  while being in nobody's `FORMATS_LIST`. Documents that describe an artifact
  are not evidence the artifact is built; `ls` the build tree.
- **Grepping for a variable name closes the uses of that variable, not the
  defect class.** `copy_plugin()` had the identical bug and spelled the path
  literally. Before claiming a class is fixed, search for the *shape* of the
  defect — a hand-built path next to a generator expression — not the identifier
  that happened to appear in the first instance.
- **Never edit a CRLF file with Python text mode.** `open(p, 'w')` silently
  normalises every line ending in the file. `open(p, 'rb')` / `'wb'` with
  explicit `\r\n` keeps the diff to the lines actually changed. Check with
  `d.count(b'\r\n')` against `d.count(b'\n')` before and after.
- **Checking a control turns a bug report into an elimination.** The missing
  `CFBundleIdentifier` looked like a CLAP defect until the other three bundles
  were checked and had it too. One extra command moved it from R4a's blocker to
  R8's finding.
- **A duplicate found from two boxes is not waste** — the second box's review is
  what caught two of the three defects in the first box's fix.
- **Check a lint's EXIT CODE, never grep its output.** I ran
  `check-id-refs.py 2>&1 | grep -viE "advisory|umbrella|E2|…"` to skip a known
  advisory, and the filter swallowed a real `SHARED LOCATION` failure — CI
  caught it on #275 instead. A check that distinguishes advisory from fatal
  *in its return code* is telling you something a grep cannot. There is now a
  helper that runs all seven the way `lint.yml` does and reports rc.
- **Invoke a lint exactly as CI does or the local run means nothing.**
  `check-prompt-provenance.py` and `check-journal-prepend.py` take **file
  paths** to the base copies, not git refs. Passed refs, they fail on branches
  CI has already marked green — a false alarm that trains you to ignore the
  tool. `grep -nE "run: python3 scripts/" .github/workflows/lint.yml` is the
  source of truth.
- **Filing a row out of another row duplicates its citations.** Splitting R8
  out of R4a, and S35 out of P11, each collided on a `file:line` a live row
  already cited. Decide which row *owns* each line: the measurement row keeps
  the evidence, the fix row keeps the line a patch would touch. R8's real
  subject was `:775` (`MACOSX_BUNDLE_GUI_IDENTIFIER`) rather than N1's `:774`
  (`MACOSX_BUNDLE_BUNDLE_NAME`) — the collision was also a sloppy citation.
- **`gh pr edit` needs `read:org` and the agent token has only `repo`.** Use
  `gh api -X PATCH repos/OWNER/REPO/pulls/N --input file.json` to set a body.
  `gh pr comment` works fine.

**Next:** **R4 stays on the `linux` box** — the tarball and `install.sh` cannot
be built or verified here — and it should not be packaged until GMPI#6 and #274
both land, or the tarball ships a bundle no host loads. **R4a** is in review.
**R8** is R5's to hit and needs a naming decision from Jeff (publisher is
*SynthEdit Limited* per R1). GMPI#6 is PR-GATED and complete; it needs Jeff, and
it must merge with #274.

**Branch/PR:** `tide/mac/R4-clap-and-tarball` — TideSynth. Upstream:
[GMPI#6](https://github.com/JeffMcClintock/GMPI/pull/6) (rebuilt, PR-GATED).

---

## 2026-08-22 — linux — #271: fixing the bundle name alone would have emptied the bundle

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** STEP 1 outranked the backlog — [#271](https://github.com/JeffMcClintock/TideSynth/issues/271),
the packaging break I filed earlier today, is my platform's product break. Fixed
it as two PRs that must land together: [GMPI#7](https://github.com/JeffMcClintock/GMPI/pull/7)
(PR-GATED, raised as a proposal, not merged) and
[#274](https://github.com/JeffMcClintock/TideSynth/pull/274).

Also resolved #273's conflict first, per STEP 1.5, and corrected an
overstatement in my own issue on the way.

### The one-line fix was a trap

`gmpi_plugin.cmake` builds the Linux VST3 bundle directory from the **target**
name while the `.so` inside takes `OUTPUT_NAME`. Obvious fix: make the directory
follow `OUTPUT_NAME` too. It builds, and it produces **two** bundles:

```
TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so    <- loadable, NO resources
TIDE_Rack_VST3.vst3/Contents/Resources/Prefabs/…     <- resources, never loaded
```

`SynthEditSem/CMakeLists.txt` stages `Resources/` into a path it spells out to
match GMPI's expression **character for character** — and its comment says so, in
as many words. So the GMPI-only fix leaves the loadable bundle with no prefabs
and no pin XMLs: **exactly the failure S21 was filed to fix.** I only saw it
because I built and listed the tree instead of trusting a green rc=0.

With both halves: one bundle, `TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so`,
Resources and all six prefabs intact — and `TIDE-Rack.vst3` is the name the five
`tests/hosts/*.rpp` fixtures already expect after N1a.

### Blast radius, probed rather than argued

The PR-GATED rules want to know what else this touches. With `OUTPUT_NAME` unset,
`$<TARGET_FILE_BASE_NAME:t>` **is** the target name — measured with a two-target
CMake probe:

```
plain:   target=plain    base=plain
renamed: target=renamed  base=Some-Name
```

The only other in-tree `gmpi_plugin()` consumer, `SE16/se_gmpi/vst3`, does not set
`OUTPUT_NAME`, so its output is unchanged. That is the argument for a GMPI change
being safe, and it is checkable rather than rhetorical.

### I overstated my own issue, and corrected it

I had written that `~/.vst3/TIDE_Rack_VST3.vst3/…` "is what a Linux user ends up
with". Reasoned from the code, not observed. The whole `copy_plugin()` block is
gated on **`SE_LOCAL_BUILD`** (`gmpi_plugin.cmake:1139`), which this build sets
`FALSE`; the generated `build.make` has no `~/.vst3` reference and TIDE is
correctly absent from that folder. Local developer builds do propagate it;
standalone and CI builds never run the copy. Corrected on the issue.

Corroboration for the convention itself, since I was asserting one: every other
VST3 installed here keeps bundle name == payload name — `Gain_VST3.vst3` →
`Gain_VST3.so`, `Container.vst3` → `Container.so`, `FinalCheckSynth.vst3` →
`FinalCheckSynth.so`.

**Learned:**

1. **When two files are documented as mirroring each other, changing one is a
   half-fix by construction.** The comment in `SynthEditSem/CMakeLists.txt` named
   the GMPI line it copies. Reading the *other* side of a documented pairing
   before editing either is the cheap move.
2. **A build that succeeds can still package nothing.** rc=0 with an empty
   loadable bundle is a worse outcome than a compile error, and only `find` on
   the output tree distinguishes them.
3. **`GMPI_SDK_FOLDER_OVERRIDE` makes a PR-GATED change testable** without
   touching the developer's tree: clone GMPI to scratch, point a scratch build
   at it, and the whole proposal is verifiable before it is proposed.
4. **Write to a CRLF file with Python and you get a 1,280-line diff.** Caught it
   on the first `git diff --stat` — read the byte mode and re-encode. STEP 5
   warns about CRLF churn for stashes; it applies to your own edits too.
5. **Prove a no-op instead of claiming one.** A five-line CMake probe turned "this
   should not affect other consumers" into a printed before/after.

**Next:**

1. **The two PRs must merge together** — GMPI#7 first or simultaneously; either
   alone leaves the bundle split or the names mismatched. Both bodies say so.
2. **#271 stays OPEN** — nothing here loaded the plugin in a host. It is a layout
   check against the rule GMPI's own comment states. Closing it wants the v0.1
   harness against the fixed bundle, which needs REAPER (win or mac).
3. **#273 (N1b)** is conflict-free again and waiting on review.

**Machine left clean.** All builds ran in scratch trees against a scratch GMPI
clone; Jeff's `~/TideSynth/build` and his `~/.vst3` were not written to. TideSynth
back on `main` after this branch.

**Branch/PR:** `tide/linux/issue-271` → [#274](https://github.com/JeffMcClintock/TideSynth/pull/274), with [GMPI#7](https://github.com/JeffMcClintock/GMPI/pull/7).
## 2026-08-22 — macos — P11's mac half had the right symptom and the wrong mechanism

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **P11**, re-measured its mac half, and corrected it. The row said
the mac build has *no* module-database install step. It has one. It installs
into a folder the scanner never reads — which produces the identical symptom and
sends you somewhere completely different to fix it.

### The row set the wrong expected difficulty

> *"the mac build has NO counterpart to the win post-build module-DB copy at all"*

Read that and the job is "add an install step". The job is not that.

| | path | who |
|---|---|---|
| `SE_LOCAL_BUILD` installs to | `~/Library/Audio/Plug-Ins/GMPI` | `GMPI/gmpi_plugin.cmake:1225` |
| the scanner reads | `/Library/Audio/Plug-Ins/GMPI` | hard-coded |

`getPlatformPluginsFolder()` returns the string literal `"/Library/Audio/Plug-Ins/"`
(`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:152-164`); `"GMPI"` is
appended in `SynthEdit/SynthEdit2/SynthEditApp.cpp:155-164`; that one path is
everything `RefreshModuleData` scans. **There is no
`NSSearchPathForDirectoriesInDomains` anywhere in the scan path** — so unlike
VST3 and AU, which search user *and* system by convention, the user domain is
never consulted. Filed as **S35**.

### Two independent measurements, because one would not have settled it

I started from the cache, not the code. `~/Library/Application Support/SynthEdit/`
holds the `Plugin-Cache-16-override-*.xml` files the scan writes, and the newest
recorded exactly one TIDE bundle: the **stale, system-domain, pre-rename**
`/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi`.

**That alone proves nothing** — that cache was written at 08:09 and the current
`TIDE-Rack.gmpi` was installed at 21:57, so age explains it. The question that
does settle it is *"does the scanner **ever** record a user-domain path?"*:

```
all Plugin-Cache-16-override-*.xml, every date:
  602  /Library/Audio/Plug-Ins/GMPI
    0  ~/Library/Audio/Plug-Ins/...        <- any user-domain path, of any kind
```

Zero, ever. Then the code confirmed the mechanism the cache implied.

### TIDE is not affected — and that is the useful half

TIDE does **no** module scan. S1a removed it; the browser reads a force-linked
in-memory list (`SynthEditSem/TideApp.cpp:434-442`). So this never touches TIDE's
own runtime. It bites **SynthEdit the editor** consuming TIDE as a third-party
module, which is the configuration P11's Windows symptom was found in.

### One thing N1a made permanent

Any `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` predating the rename is now an
orphan: the build emits `TIDE-Rack.gmpi`, so nothing will ever update the old
name again, and the scanner keeps serving whatever was last copied there. Two
were sitting on this box — system-domain 2026-08-16, user-domain 2026-05-07.

### An assumption of mine, caught by testing it

I wrote that the hand-copy needs `sudo`, because the folder is system-wide.
`[ -w /Library/Audio/Plug-Ins/GMPI ]` says otherwise **on this machine** — it is
owned by the developer, presumably from an installer or an old `chmod`. On a
fresh machine it is root-owned, which is why SynthEdit's CI runs `sudo mkdir -p`
and `sudo chmod 777` on it
(`SynthEdit/.github/workflows/Export_Tests_mac.yml:34-35`). The doc now says
both and tells you to check, instead of asserting either.

**Learned:**

- **A stale row is most expensive when its symptom is right and its mechanism is
  wrong.** "No install step" and "install step pointing at the wrong domain"
  look identical from the outside and lead to opposite work. When a row's
  mechanism claim is older than a few weeks, re-derive it before costing the
  job — the symptom surviving is not evidence the explanation did.
- **"The cache doesn't list X" is not evidence X is ignored** — it may just
  predate X. The question that settles it is whether the artifact *ever* records
  that class of thing, across every copy you have. One `grep` over all cache
  files was worth more than reading the newest one carefully.
- **`SE_LOCAL_BUILD` on macOS does not do what its name implies.** It installs,
  and the install is invisible to the scanner. Anyone debugging "my rebuilt
  module didn't take effect" on mac is looking at this.
- **Check `[ -w ]` before telling someone to use `sudo`.** Folder ownership under
  `/Library` is not uniform across machines; asserting it wastes the reader's
  time in whichever direction you got it wrong.
- **The shared-citation lint (A31) earns its keep on rows you split.** Filing S35
  out of P11 duplicated two `file:line` citations across both. The right fix was
  not to delete one at random but to decide which row *owns* each line: P11 owns
  the evidence, S35 owns the line a fix would change.

**Next:** **S35** is the real fix and it is **GATED** —
`SynthEditLib/EditorLib/Application.cpp` needs Jeff. Its first task is not the
extra `ScanFolder` call but the question that call raises: whether a bundle
present in **both** domains produces duplicate module IDs. Both copies existed on
this box, so that is testable rather than theoretical. **P11 itself stays open**
for its Windows half and the misleading diagnostic, neither of which this touched.
**Windows and Linux are unexamined** — `getPlatformPluginsFolder()` branches per
platform and I measured only mac.

**Branch/PR:** `tide/mac/P11-mac-module-visibility` — TideSynth, docs and backlog only.

---

## 2026-08-22 — linux — N1b: the rename's live docs, and a Linux-only gap N1a could not have seen

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** took **N1b** (unblocked once N1a's merged PR let me flip it DONE), and
found a packaging regression on the way to the ground truth I needed.

### The finding is worth more than the item: #271

Building `main` here — the **first Linux build since N1a landed** — is green
(rc=0, 0 errors), but the VST3 bundle is not:

```
TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE-Rack.so
^^^^^^^^^^^^^^^                           ^^^^^^^^^
```

The directory kept the **target** name; the payload took `OUTPUT_NAME`.
`gmpi_plugin.cmake`'s own comment says a host scanning `~/.vst3` looks for
`<name>.vst3/Contents/<arch>-linux/<name>.so` — the two must match, and before
N1a they did (`TIDE_VST3.vst3/…/TIDE_VST3.so`).

**Why mac and Windows verification could not catch it:** they never build that
path by hand. macOS gets it from `BUNDLE_EXTENSION` and Windows from `SUFFIX`,
and both resolve through `OUTPUT_NAME` for free. Linux is the only platform
where the bundle path is spelled out (`:853`), and it is the one platform N1a
could not be checked on. **The same shape as N1a's own commit title** — *"a
rename that skipped work silently"* — happening once more, one platform along.

It reaches the user, not just the build tree: `copy_plugin()`'s Linux VST3 branch
copies by target name on both sides, so `~/.vst3` gets the same mismatch. The
`.gmpi`/`.clap` branch uses `$<TARGET_FILE:…>` and is already correct, which
narrows the fix to one hand-spelled path in two places. Filed as
[#271](https://github.com/JeffMcClintock/TideSynth/issues/271) with the suggested
one-liner; **not fixed** — `gmpi_plugin.cmake` is in GMPI, which is PR-GATED, and
I could not find a TIDE-side fix because TIDE does not control that variable.

### N1b itself: the triage corrected N1's own bucketing

I wrote N1's cost model yesterday and put `docs/state-of-the-prototype.md` in
"live reference docs" **on the strength of its filename**. Reading it says
otherwise — *"Observation only. … Everything below was seen"*, dated 2026-08-06.
Its REAPER Lua transcript, its crash-report text and its P5 finding about the FX
browser would all be **falsified** by a rename. Same for `p4-resize-crash.md`,
whose PDB names sit inside a measured before/after byte table.

So both are **annotated, not rewritten** — against what the N1b row expected of
me. I have said so in the row rather than doing it silently, because it is a
judgement Jeff may want to overrule.

Genuinely live, and updated:

| doc | what was stale |
|---|---|
| `docs/building.md` | the build command, the two-target trap, the macOS copy line |
| `docs/ci/linux-build-deps.md` | build-and-run instructions |
| **`docs/ci/headless-gui-verification.md`** | **wrong within a day of my writing it** |
| `docs/n1-tide-rack-rename.md` | its status line still said TODO |

The headless doc is the one worth noting: I wrote it yesterday and it told the
next run to launch `./TIDE_STANDALONE`, which no longer exists. I corrected it
and **re-ran the recipe end to end** rather than assuming — the renamed binary
comes up under headless weston, prints its command channel, and `pgrep -x
TIDE-Rack` matches, so the shutdown line still works too.

Rather than stamp eleven banners on eleven dated records, `docs/building.md`
gains one **Current target and artifact names** table, so there is a single
authoritative place to check and the records stay untouched.

**Learned:**

1. **Classify a doc by reading its opening, not its filename.** "state-of-the-
   prototype" sounds like current state; it is a dated observation report. My own
   cost model got this wrong 24 hours earlier, and only reading fixed it.
2. **A doc you wrote yesterday is not exempt from going stale.** The headless
   recipe was obsolete within a day, by someone else's merge. Grep your own
   output when a rename lands.
3. **The first build on a platform after a cross-platform rename is a real
   test.** Nothing failed, exit code 0 throughout — the defect is a *name*, and
   only comparing two names caught it.
4. **When two things must agree, check them against each other, not against
   spec.** Both halves of the bundle path were individually defensible; only
   putting them side by side showed the mismatch.
5. **"Verified on two platforms" is not "verified".** N1a was checked on mac and
   Windows and was correct on both, by two different mechanisms — neither of
   which is the mechanism Linux uses.

**Next:**

1. **#271** — one-line fix in GMPI (`$<TARGET_FILE_BASE_NAME:…>` for the bundle
   dir, in both the assembly and the copy). PR-GATED: happy to raise the GMPI PR
   on request, but not unilaterally.
2. **The v0.1 fixtures now name `TIDE-Rack.vst3`** while Linux emits
   `TIDE_Rack_VST3.vst3`. Undetectable here (no REAPER); it resolves itself when
   #271 lands.
3. **N1b's annotate-don't-rewrite call** is Jeff's to overrule if he wanted those
   two files edited.

**Machine left clean.** TideSynth back on `main` after the PR; weston and the
standalone both stopped by pid (**S31**), scratch `XDG_CONFIG_HOME` throughout so
`~/.config/TIDE Rack/` is untouched. The gmpi_ui working tree is now clean — its
2026-08-19 edit went out as gmpi_ui#10 earlier today.

**Branch/PR:** `tide/linux/N1b-live-docs` — TideSynth only. No code change.
## 2026-08-22 — macos — N1a: OUTPUT_NAME renamed three things, and only one of them had an extension

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1.5, not a backlog item. [#268](https://github.com/JeffMcClintock/TideSynth/pull/268) is this platform's own
open PR and its `windows` check was red, which outranks new work. Fixed the
cause, on the same branch, and re-ran N1a's full Accept against a binary built
from the tree being pushed.

### The failure, and why two of three platforms said nothing

```
LINK : fatal error LNK1201: error writing to program database
  'D:\a\TideSynth\TideSynth\build\SynthEditSem\Release\TIDE-Rack.pdb';
  check for insufficient disk space, invalid path, or insufficient privilege
  [...\TIDE_Rack_VST3.vcxproj]
   Creating library ...\Release\TIDE-Rack.lib and object ...\Release\TIDE-Rack.exp
   TIDE_Rack.vcxproj -> ...\Release\TIDE-Rack.gmpi
```

Read those three lines together and the message is a lie about its own cause.
`TIDE_Rack.vcxproj` is writing `TIDE-Rack.lib` at the same moment
`TIDE_Rack_VST3.vcxproj` fails to write `TIDE-Rack.pdb`. It is not disk space.
**`OUTPUT_NAME "TIDE-Rack"` renames three artifacts per target, not one** — the
module, the linker PDB, and the import library — and all three format targets
(`TIDE_Rack`, `TIDE_Rack_VST3`, `TIDE_Rack_STANDALONE`) build into the SAME
directory on Windows. Only the module is disambiguated, by its extension
(`.gmpi` / `.vst3` / `.exe`). The PDB and the `.lib` are not, so three targets
raced for one `TIDE-Rack.pdb` under MSBuild's parallel link.

**macOS and Linux were green on the identical commit**, and that is the whole
trap: neither emits a PDB, neither emits an import library for a MODULE, and on
macOS each artifact is a bundle so even the paths differ. A rename verified
end-to-end on this box could not have shown it here.

Fixed by keying the two side-channel names off the TARGET name, which is unique
by construction (`${PROJECT_NAME}_${kind}`):

```cmake
set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "TIDE-Rack"
    PDB_NAME "${SUB_PROJECT_NAME}"
    COMPILE_PDB_NAME "${SUB_PROJECT_NAME}"
    ARCHIVE_OUTPUT_NAME "${SUB_PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_NAME "TIDE Rack")
```

That also restores the pre-rename convention rather than inventing one: the
PDBs were `TIDE.pdb` / `TIDE_VST3.pdb`, i.e. target-named, which is what
`docs/state-of-the-prototype.md:106` and `docs/p4-resize-crash.md` still record.
Those two are live reference docs and belong to **N1b**, so they are untouched.

### The macOS half is proven inert, by construction rather than by re-rendering

Configured the branch twice — once with the fix, once with it stashed — and
diffed the ENTIRE generated build system under `build/SynthEditSem`, with the
build-directory name normalised away. **Three files differ, and every difference
is a PDB filename:**

| target | before | after |
|---|---|---|
| `TIDE_Rack` | `TIDE-Rack.gmpi/Contents/MacOS/TIDE-Rack.pdb` | `…/TIDE_Rack.pdb` |
| `TIDE_Rack_VST3` | `TIDE-Rack.vst3/Contents/MacOS/TIDE-Rack.pdb` | `…/TIDE_Rack_VST3.pdb` |
| `TIDE_Rack_STANDALONE` | `TIDE-Rack.pdb` | `TIDE_Rack_STANDALONE.pdb` |

All three `link.txt` files are byte-identical. **The baseline column is also the
proof of the diagnosis** — three targets, one PDB name — obtained without a
Windows machine.

### N1a's Accept, re-run against this tree's own binary

Configure rc=0, build rc=0, zero `error` lines. `TIDE-Rack.gmpi`,
`TIDE-Rack.vst3` and `TIDE-Rack.app` all emitted; targets still `TIDE_Rack*`;
`lipo -archs` = `arm64`, as ruled 2026-08-21.

The build does **not** copy the VST3 into `~/Library/Audio/Plug-Ins`, so
rendering straight away would have measured the previous session's bundle. It
was installed deliberately and the identity checked rather than assumed —
installed and built binaries both
`009060be0f4852280bd89e4cabfc3277df0f3040f36d1e15af9232950c5fe816`. Jeff's stale
`TIDE_VST3.vst3` (16 Aug, same plugin ID) was parked for the duration, so
`TIDE-Rack.vst3` was the only candidate, and **restored afterwards**.

| fixture | this run | 2026-08-21 |
|---|---|---|
| `--control` | PASS, −6.0 / −9.0 | PASS |
| `v1-rack` | −6.3 / −17.0, 2 cables | −6.3 / −17.0 |
| `v1-rack-midi` | −6.3 / −17.0, 4 cables | −6.3 / −17.0 |
| `v3-midi-pitch` | −6.2 / −21.1, 4 cables | −6.2 / −21.1 |
| `v3-midi-gate` | −6.3 / −21.2, 3 cables | −6.3 / −21.2 |
| `v1-rack-uncabled` | **silence**, 0 cables | silence |

### The Windows link, which this box cannot compile — verified by CI, then waited for

There is no MSVC here, and STEP 3 forbids fixing a platform blind. This is not
that: the break is this platform's own PR, STEP 1.5 makes it mine, and **the
PR's own `windows` job is the verification** — which is why the fix went to the
same branch, and why the run stayed up for it rather than declaring victory on
a mechanism.

[Run 32492249466](https://github.com/JeffMcClintock/TideSynth/actions/runs/32492249466), on `276e150`:

| job | before (`ea6a1e1`) | after (`276e150`) |
|---|---|---|
| `guard` | success | success |
| `linux` | success | success |
| **`windows`** | **failure — `LNK1201`** | **success** |
| `macos` | success | queued (S30) |

**`windows` went red → green on a one-block change, with `linux` green on both
ends as the control.** That is the A/B this platform could not run locally.

`macos` is still queued and **carries no information about this change** — it
was green on the previous head, the change sets Windows-only properties, and the
local build here was rc=0 with the artifacts checked. S30 (the mac runner
completes ~5% of runs) is why it is still sitting there, and waiting longer would
be waiting on a 5%-likely event to confirm something already measured — the same
call the 2026-08-21 C7e entry made, for the same reason.

**Learned:**

1. **`OUTPUT_NAME` is three renames, and only the one with an extension is
   collision-proof.** Targets sharing an output directory can carry the same
   `OUTPUT_NAME` safely only for the artifact whose suffix differs; `PDB_NAME`
   and `ARCHIVE_OUTPUT_NAME` have no suffix to save them. Any future format
   added to `FORMATS_LIST` inherits this for free now, because both derive from
   the target name.
2. **`LNK1201` names disk space, privilege and path, and means none of them.**
   The line above it in the log — a sibling project writing the same base name —
   is the actual evidence, and it is easy to skim past as ordinary progress.
3. **Configure twice and diff the generated build system.** It answered "can
   this change affect macOS?" exactly, in about a minute, and produced the
   diagnosis of the Windows failure as a by-product. Cheaper and stronger than
   re-rendering, which could only have shown that nothing broke.
4. **A build that does not install is a measurement trap.** `cmake --build`
   leaves the previous bundle in the plugin folder, so a render "after the
   change" silently measures the artifact from before it. Hashing the installed
   binary against the built one is one command and converts a plausible result
   into a proof — the same trap the 2026-08-21 entry hit from the other side,
   with a same-ID bundle rather than a stale one.

**Next:**

1. **[#268](https://github.com/JeffMcClintock/TideSynth/pull/268) is green where it matters and wants only a merge** — `lint`,
   `guard`, `windows` and `linux` all pass on `276e150`; `macos` is queued
   behind S30 and cannot say anything about a Windows-only property. No
   `platform:win` issue was needed.
2. **N1b unblocks when N1a merges** — and it now has two more references to
   carry, both PDB names: `docs/state-of-the-prototype.md:106` and
   `docs/p4-resize-crash.md:461-462`. Noted on its row.
3. **Two stale `platform:mac` issues were closed** — see below; they were never
   this platform's break.

**Also this run, STEP 1 bookkeeping.** [#264](https://github.com/JeffMcClintock/TideSynth/issues/264) and [#260](https://github.com/JeffMcClintock/TideSynth/issues/260) were the only open
`platform:mac` issues and **both were re-verified before being touched**, per
STEP 1's rule about not fixing what you cannot observe. Neither is a macOS
break: each names a branch that no longer exists (both merged), and in each run
**all three platforms failed the Build step together**, which is `main`'s state
at that hour rather than anything about this platform. `main` went green on all
three at `5ef0bf29`, and macOS passes on #268's current head. Closed with that
evidence, and the linux box's own lesson is what made the check cheap — *"a
branch's CI platform issue can be reporting `main`'s break."*

**Machine left clean.** TideSynth returned to `main`, tree clean; every other
repo on this box (`SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`,
`GMPI`) was clean at the start of the run, untouched during it, and clean at the
end — this item is one file in TideSynth. Builds went to the scratchpad, not to
Jeff's trees. `~/Library/Audio/Plug-Ins/VST3/` ends the run as it began, except
that `TIDE-Rack.vst3` is now this branch's build rather than yesterday's;
`TIDE_VST3.vst3` is back and byte-untouched, and deleting it remains Jeff's
call, exactly as the previous entry left it.

**Branch/PR:** `tide/mac/N1a-rename` — [#268](https://github.com/JeffMcClintock/TideSynth/pull/268), TideSynth only. One CMake block.

---

## 2026-08-21 — macos — R3: the pkg builds, and productbuild would have shipped it to the wrong hardware

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** built the macOS pkg — [scripts/package-macos.sh](scripts/package-macos.sh)
produces `TIDE-Rack-macOS.pkg` — and split the half of R3 that cannot be done.

### Half the row was unbuildable, and checking first is what caught it

R3 says *"AU → `Components`, VST3 → `VST3`"*. `SynthEditSem/CMakeLists.txt` sets
`FORMATS_LIST GMPI VST3 STANDALONE`: **there is no AU target**, and **M1**, the
row that would add one, is BLOCKED. Filed as **R3a**, `BLOCKED(M1)`.

The script **fails** if the AU is missing rather than quietly packaging one
plug-in where the docs promise two — a pkg that silently omits half its payload
is worse than one that refuses to build.

### The real find: productbuild lies about hardware

`productbuild` writes `hostArchitectures="x86_64,arm64"` into the synthesized
Distribution **regardless of what the payload actually contains**. TIDE is now
arm64-only, so the pkg would have **installed happily on an Intel Mac** and the
plug-in would then have failed to load with nothing explaining why.

That is precisely the consequence R3's own row predicted this morning — *"the
pkg will not run on an Intel Mac and nothing tells the user why"* — and it turns
out macOS will tell them, if the pkg is honest. The script now derives
`hostArchitectures` from `lipo` on the built binary and verifies the
substitution landed; the shipped pkg reads `hostArchitectures="arm64"`, so the
installer itself refuses the wrong hardware. Derived rather than hardcoded, so
it stays correct if the ARM ruling is revisited.

### Verified against the artefact, not the tool's own output

- payload installs to `./Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3`, matching
  distribution.md
- a real `installer` run into a sandbox target: *"The install was successful"*,
  placing a binary **byte-identical** to the build (same sha), and leaving no
  stray receipt
- `hostArchitectures="arm64"` read back out of the expanded pkg

**Not signed, not notarized, and that is stated rather than implied.** Signing
runs only when the two identity variables are in the environment; notarization
(`notarytool` + `stapler`, modelled on `SynthEdit_cmake_mac.yml:223-244`)
belongs to **R5**, which owns the secret store. The script prints which of the
two artefacts it produced and says plainly that an unsigned pkg is not
shippable.

**Learned:**

1. **A packaging tool's defaults describe the tool, not your payload.**
   `productbuild` had no idea the binary was single-arch and cheerfully said it
   would run anywhere. The check that caught it was reading the generated
   Distribution rather than trusting "Wrote product to …".
2. **When a row names two payloads, confirm both exist before starting.** Half
   of R3 was blocked by a row nobody had connected to it, and the connection was
   one grep of `FORMATS_LIST`.

**Next:** R3a waits on M1. R5 wires notarization and is a workflow file, so
Jeff pushes it. R2 (`win`) and R4 (`linux`) are now takeable on their boxes.

**Branch/PR:** `tide/mac/R3-macos-pkg` — TideSynth only.

---

## 2026-08-21 — macos — S29's coverage-hole fix, rebuilt clean after the branch went stale

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** re-created the `startsWith(github.head_ref, 'tide/')` correction on a
fresh branch off current `main`, because the original never got pushed and had
drifted 17 files behind.

**#258 merged the guard without the correction.** Checked directly rather than
assumed: `main`'s `build.yml` has the bare `if:` with no `tide/` test, so every
branch Jeff names by hand — none of which start with `tide/` — currently gets
**zero build coverage**: no push run (outside `on: push: branches:`), and now no
PR run either, because the guard skips all same-repo PRs unconditionally.

**The old local branch was the wrong base to push.** `git diff origin/main
s29-close-coverage-hole --stat` showed 17 files and 1263 deletions — journal
rotation, a deleted doc, N1a's rename, all landed separately since. Force-pushing
that would have reverted merged work. Deleted the stale branch and rebuilt the
one-line fix directly on today's `main`: one file, 13 lines.

**Learned:**

1. **An unpushed branch decays the moment other agents keep merging.** The fix
   was correct when written; by the time I went to push it, rebasing would have
   cost more than re-deriving it. Re-creating small, mechanical diffs from a
   current base is cheaper than reconciling a stale one.

**Next:** Jeff pushes `s29-close-coverage-hole` — one file, the fleet token
still has no `workflow` scope.

**Branch/PR:** `s29-close-coverage-hole` — workflow + row + this entry.

---

## 2026-08-21 — macos — the release track was free for three days and the backlog said otherwise

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** unblocked **R2, R3, R4, R5**; gave **R6** a *named* blocker instead of a
blanket one; corrected the section header that caused all of it.

### The stale gate

`## Release & distribution — blocked on V1 (nothing to ship yet)`. **V1 closed
2026-08-18.** One row also said *"Needs C7"* — **C7 closed 2026-08-21**. Neither
had been revisited, so five rows advertised a shut door that had been open for
three days, on the one track that turns a working plugin into something a user
can install.

This is A32's failure with the polarity reversed: A32 catches rows that look
*live* and are finished; this is rows that look *blocked* and are free. Nothing
detects it, because a `BLOCKED` status is never wrong-looking on its own.

### Four are free, and one is not — which is why I did not flip all five

**R2 / R3 / R4** need an artefact and a signing identity: all three platforms
build from a clean public clone, N1a gave the artefacts their shipped names
(`TIDE-Rack.vst3` / `.gmpi` / `.app`), and R1 settled signing. Free.

**R5**'s own named blocker was C7, now gone — but it is a
`.github/workflows/**` file, so a run can author and verify it and **cannot push
it**. That constraint is recorded on the row rather than discovered by the next
taker.

**R6 is genuinely not free**, and flipping it with its siblings would have been
the lazy read. It replaces the honest *"nothing to download yet"* card with
`releases/latest/download/<asset>` permalinks, and **those 404 until a release
exists**. So it moves from `BLOCKED` to **`BLOCKED(R5)`** — same status, real
information, and eligibility now lives in the status column where STEP 2 reads
it.

**Learned:**

1. **A blocked row is never obviously wrong, so nothing ever re-reads it.** The
   fleet has a lint for stale-live rows (A32) and none for stale-blocked ones,
   and the second kind is more expensive: it hides work that could have started.
2. **"Unblock the section" is not the same as "unblock every row in it."** Four
   of five were free; the fifth had a real dependency the blanket status was
   concealing. Naming the blocker is worth more than clearing it.

**Next:** R3 is `mac` and now takeable here. R2 is `win`, R4 is `linux`, R5
wants Jeff's push.

**Branch/PR:** `tide/mac/R-unblock` — TideSynth only, statuses and header.

---

## 2026-08-21 — macos — N1a: the rename shipped, and it silently unlinked half the build first

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing · linux + renderer agents also active

**Did:** carried the TIDE Rack rename through the build. Shipped artifacts are
now `TIDE-Rack.vst3` / `.gmpi` / `.app`; targets stay `TIDE_Rack` /
`TIDE_Rack_VST3` / `TIDE_Rack_STANDALONE`.

### The finding: a rename that skips work instead of failing

Two loops built target names by hand — `set(_tide_target TIDE)` and
`TIDE_${_fmt}` — instead of deriving from `PROJECT_NAME`. **Every use of
`_tide_target` sits behind `if(TARGET ...)`**, so renaming the project did not
break those loops, it made them **no-ops**: no `tide_render` link, no resource
staging, no diagnostic.

It surfaced only as `fatal error: 'TidePathTracer.h' file not found`, and only
because `TiDEPanelGui.cpp` happens to include that header. **Without that
include the build would have gone green and shipped bundles containing zero
prefabs** — an empty module browser, which is exactly S21's failure wearing a
different hat. All four sites now derive from `${PROJECT_NAME}`.

N1a's scope list was careful and still missed these, because it searched for
`TIDE_VST3`-shaped strings and these are the bare `TIDE`.

### Checked before renaming, not after

- **Identity does not move.** `id`, `name`, `vendor` come from the `<Plugin>`
  element in `SynthEdit.cpp`, not `PROJECT_NAME` — so saved host sessions still
  resolve. That was the one thing worth knowing before touching anything.
- **`${PROJECT_NAME}.xml` is not in play** (no `HAS_XML`, no `TIDE.xml`), so
  distribution.md's second warning does not apply here.
- **The STANDALONE's bundle id does follow `PROJECT_NAME`**
  (`com.gmpi.standalone.${PROJECT_NAME}`), so that dev tool gets a fresh
  preferences container. Stated rather than discovered later.

### The measurement, and why the first pass of it was worthless

Baseline `v1-rack.rpp`: peak **−6.3 dBFS**, rms **−17.0**, 2 patch cables.
After the rename: identical. **That proved nothing**, because the old
`TIDE_VST3.vst3` was still installed and carries the same plugin ID — REAPER
could have loaded either. So the old bundle was **moved aside** and the render
repeated: same numbers with only `TIDE-Rack.vst3` present. That is the
difference between "the numbers match" and "the artifact under test produced
them".

All five fixtures then pass in isolation — `v1-rack` −6.3/−17.0, `v1-rack-midi`
−6.3/−17.0, `v3-midi-pitch` −6.2/−21.1, `v3-midi-gate` −6.3/−21.2, and
`v1-rack-uncabled` **silence**, the negative control. `--control` PASSes.
**Jeff's original bundle was restored immediately afterwards.**

**Left for Jeff, deliberately:** `~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3`
(16 Aug) is now stale — nothing produces that name any more — and sits beside
the new bundle, so a DAW scan lists both. Deleting from his plugin folder is
his call, not a run's.

**Learned:**

1. **A guard that makes missing work silent turns a rename into a downgrade.**
   `if(TARGET x)` is the right shape for an optional format and the wrong shape
   for a name that must exist; the same line cannot tell the two apart. Deriving
   the name from `PROJECT_NAME` removes the question.
2. **When the old artifact is still installed and shares an ID, matching
   numbers are not evidence.** Moving it aside cost one minute and converted a
   plausible result into a proof.

**Next:**

1. N1's remaining buckets (B and C) are untouched — this was bucket A only.
2. The stale installed bundle wants Jeff's decision.

**Branch/PR:** `tide/mac/N1a-rename` — TideSynth only.

---

