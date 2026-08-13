# E1 — the audio verification harness

Ported 2026-08-11 from the archived [`tide-rack`](https://github.com/JeffMcClintock/tide-rack)
repo (`30d3e5e`), which PLAN's "The Eurorack rack" ruling superseded on
2026-08-09. The harness was the one thing in that repo worth keeping.

This document is the half that does not survive a file copy. Findings (a)–(c)
came with the code and cost about a day each to learn originally; (d) and (e)
were measured during the port and are new.

## Where it landed, and why not `SE16/tests/`

**In this repo:** `tools/render_harness.py`, `tests/cases/`, `tests/references/`.

The E1 row asked for this decision before copying anything. `SE16/tests/` is a
gtest suite of C++ unit tests compiled as part of the SynthEdit build. This is a
different tier of test and has a different lifecycle: end-to-end, Python, and it
drives a *published engine binary* rather than anything it built. Putting it in
`SE16/tests/` would couple TIDE's audio claims to SynthEdit's build.

The deciding argument is narrower, though. `SE16/tests/` is on neither the
ALLOWED nor the GATED list in the run prompt's STEP 5, which makes it GATED by
default — landing there needs a ruling first, and there is no reason to spend
one. This repo is fully ALLOWED, and the references are *TIDE's* claims about
how TIDE should sound, so they belong beside TIDE's backlog and journal.

## What it does

Drives SynthEditCL over `--script`, renders each `tests/cases/*.json` to a WAV,
and gates three ways: the render succeeded, **signal is present**, and the audio
null-tests against a checked-in golden. One JSON report, nonzero exit on any
failure. See [`tests/README.md`](../tests/README.md) to run it.

## The findings

### (a) `-factorysemsfolder` must be an absolute path

Relative works for the editor factory and **silently fails for the DSP loader**.
Every command still reports `"ok":true`, `==== MISSING MODULES ====` goes to
stdout, and you get a well-formed WAV of digital silence.

Hence the harness's founding rule: **assert on the signal, never on the exit
code alone.** It is why `SILENCE_FLOOR_DBFS` exists as a gate rather than a
diagnostic, and why `render()` greps for `MISSING MODULES` explicitly. The code
defends against this by calling `.resolve()` on `--modules`.

### (b) The null test needs both an RMS gate and a peak gate

RMS alone is nearly blind to localized damage. **Re-measured during this port,
and it reproduces exactly:** a 3-LSB nudge across 200 of 96,000 samples yields
an RMS residual of **−107.6 dBFS** — comfortably inside the −100 dBFS RMS
tolerance — while the peak-sample-difference gate catches it at **−80.8 dBFS**
against its −86 dBFS threshold. An obvious audible-class defect that ships green
without the peak gate. **Do not drop the peak gate.**

### (c) Same-platform renders are bit-exact, across compilers and now versions

The original finding: bit-exact across runs *and* between a Release g++-14 build
and a Debug g++-13.3 build of the same engine version, so references survive
compiler and build-config changes.

**This port extends it by one axis.** The references were rendered 2026-08-07;
this run used **SynthEditCL V1.6.178**, a locally-built engine from 2026-08-10,
and both cases came back **byte-identical** — matching SHA-256, `cmp` clean. So
references also survive an engine *version* bump, at least across these two.
That is better than the harness's own docstring assumes, and it means an
engine-version change is not a free explanation for a future null-test failure.

~~Cross-platform drift remains **untested** — only the Linux lane has ever run.~~
**Tested 2026-08-14 on macOS — see findings (f) and (g).** Both halves of the
original claim survive with one retraction: Release-to-Release is bit-exact
across engine versions *and* build machines, but the macOS **Debug** build is
not bit-exact with the macOS Release builds.

### (d) `--modules` is not authoritative on a developer box — NEW

The trap that finding (a) describes **cannot be reproduced on a machine that has
run SynthEdit before**, and the reason is worth knowing because it cuts the
other way too.

Measured on the Linux box, all against the local V1.6.178 engine:

| What was passed | Result |
|---|---|
| `-factorysemsfolder ./mods` (relative) | full signal, **byte-identical to golden** |
| `-factorysemsfolder /nonexistent/path` | full signal, **byte-identical to golden** |
| `-factorysemsfolder /tmp` | full signal, 116 modules resolved |
| ...plus `XDG_DATA_HOME` redirected | still passed |
| ...plus `HOME` isolated to an empty dir | still passed |

The engine reaches modules through at least two persistent side channels in its
own state directory, neither of which `--modules` controls:

- **`~/.local/share/SynthEdit/SynthEdit16.settings.xml`** holds
  `ModulePath="/home/jef/.local/share/SynthEdit/modules"` — an **absolute** path,
  which is why redirecting `XDG_DATA_HOME` changed nothing. That folder holds a
  complete duplicate of all 41 factory modules, so the graph resolves from it
  regardless of what you named.
- **`Plugin-Cache-16-override-<hash>.xml`**, one per override path. A cache
  written under a *freshly isolated* `HOME` was observed listing 359 modules from
  `ctl/mods` — a folder named only in an *earlier* run under a different HOME. So
  the cache carries a previously scanned folder's contents forward.

**CI is sound; local reproduction is not.** A clean `ubuntu-24.04` runner has
none of this state, which is exactly why finding (a) was discoverable in CI and
is invisible here. The danger runs the other way: someone reproducing a CI
failure locally can get a confident green from a module set they did not name,
and conclude the CI failure was a fluke.

**What the port does about it.** `render_harness.py` now records the folders the
engine reported scanning into the report as `module_sources`, flags any outside
`--modules` as `foreign_module_sources`, and prints a warning. It is
deliberately **not fatal** — on a developer box the extra source is normal, and
a hard failure would break the harness precisely where a human is debugging.
Report schema went `tide-rack.audio-verify/1` → `/2` for the two new fields.

This is the same principle as finding (a), one level up: assert on what the
engine *did*, not on the argument you handed it.

### (e) The two tolerances disagree about cross-platform drift — NEW

The source comment justifies the peak threshold as sitting "just above the ~1 LSB
(−90.3 dBFS at 16-bit) that cross-platform float rounding can legitimately
produce." That is true of the peak gate and **false of the RMS gate beside it**.

Measured: 1 LSB of error on *every* sample gives RMS = **−90.3 dBFS**, which
fails the −100 dBFS RMS gate. Solving `rms = sqrt(fraction)` against that
threshold, the RMS gate tolerates 1-LSB error on at most **~10.7% of samples**.
Beyond that it fails, however benign the cause.

So the stated tolerance for legitimate float drift is only half-implemented. Since
the cross-platform lane has never run, this is a plausible cause of a spurious
failure the first time mac or Windows renders a case — and it would look like a
real regression.

**Deliberately not fixed here.** Picking a new RMS threshold without a single
cross-platform measurement would be guessing at exactly the number that decides
what counts as a regression. Filed as **E1a** with the arithmetic above, to be
settled with data from the first mac or Windows render.

> **RESOLVED 2026-08-14 by E1a — and the answer was "do not widen it".** The
> feared spurious failure did not happen, and the reason is that the worst case
> above is not what cross-platform drift looks like. See finding (f).

### (f) The first cross-platform render — E1a, macOS, 2026-08-14

The harness's second lane finally ran: macOS against the Linux goldens. Three
independent engines, so an engine-version or build-config difference could not
be mistaken for a platform one:

| Engine | Origin |
|---|---|
| `SynthEditCL V1.6.175` | local Release build, 2026-08-08 |
| `SynthEditCL V1.6.182` | local **Debug** build, 2026-08-13 |
| `SynthEditCL V1.6.183` | **published** `SynthEditCL_mac.zip`, Azure CI build, signed |

Results, both cases, all three engines:

| Case | RMS residual | Peak residual | Verdict |
|---|---|---|---|
| `voice_midi_note` | −123.1 dBFS | −90.3 dBFS (**exactly 1 LSB**, 51 of 95,999 samples = 0.053%) | pure rounding |
| `osc_naive_sine` | −73.5 dBFS | −68.7 dBFS (12 LSB) | **not rounding — see (g)** |

**The RMS gate was not widened, because the measurement says it does not need
to be.** Finding (e)'s arithmetic is correct but its premise is not: it assumed
the worst case, 1 LSB on *every* sample. Real cross-platform drift touched
**0.053%** of samples against the ~10.7% the gate allows — a 200× margin, and
22.9 dB of headroom on the number itself. Two builds agree on almost every
sample and disagree only where a value lands on a quantisation boundary. The
number was never the problem; the missing measurement was.

**The peak gate's 4.3 dB of headroom is structural, not luck.** 1 LSB is a hard
per-sample ceiling for rounding-class drift: two builds that agree on the
underlying float value can only disagree about which way it rounds. Drift of
this class can affect *more samples*; it cannot make any one sample wrong by
more than 1 LSB. So −86 dBFS sits above a ceiling, not above a sample.

**Finding (c) extends and slightly retracts.** Extends: the published Azure-CI
Release build and the local Release build are **byte-identical** on both cases,
so same-platform bit-exactness now spans build machines as well as compilers
and engine versions. Retracts: the mac **Debug** build differs from the mac
Release builds by 1 LSB on 40 of 95,999 samples in `voice_midi_note`. Finding
(c) claimed Release-vs-Debug bit-exactness from a Linux measurement; it does not
hold on macOS. Same-platform renders are **run-to-run** bit-exact (checked), and
Release-to-Release bit-exact — not Debug-to-Release.

### (g) An oscillator's residual is unbounded in render duration — NEW

`osc_naive_sine` failed by five orders of magnitude, identically on all three
macOS engines. It is not rounding, and treating it as a bigger version of
rounding would have produced the wrong fix.

The residual **grows monotonically through the render** — −96 dBFS in the first
0.1 s block, −69 dBFS in the last — with a best-fit time lag of zero, so it is
not a delay either. Fitting the implied phase error `dphi = k·t` gives a
frequency offset of **0.15 ppm, ≈2.5 ULP at single precision** (2⁻²⁴ = 5.96e-8).
The reference tone is A440; the offset is 6.6e-5 Hz.

`OscillatorNaive`'s pitch table and phase increment are both `double`, but the
path between them is not: `OscillatorNaive.h:66` derives the table index as a
`float` from a `float` pitch, so the interpolation carries single-precision
resolution. A few ULP there is the right order for what was measured. Named as
the plausible locus, **not proven** — the measured quantity is the 2.5 ULP.

**Why this changes the shape of the fix and not just the number.** A frequency
offset integrates. The residual is proportional to elapsed time, so:

| Render duration | Peak residual | RMS residual |
|---|---|---|
| 0.5 s | −79.6 dBFS | −87.4 dBFS |
| **2.0 s** (this case) | **−67.6 dBFS** | **−75.4 dBFS** |
| 8 s | −55.6 dBFS | −63.3 dBFS |
| 60 s | −38.1 dBFS | −45.8 dBFS |

(Modelled from the fitted drift rate; the 2 s row matches the measured −68.7 /
−73.5 dBFS to within 1–2 dB.)

So **no fixed dBFS number is a duration-independent statement about a
free-running oscillator.** Widening the global gates to admit this case would
have to reach −67 dBFS — which sails straight past finding (b)'s reference
defect (3 LSB over 200 samples, caught at peak −80.8 dBFS) — and would fail
again the moment a case renders for 4 s.

**What was done instead.** The global gates stay at −100/−86 as the
rounding-class budget every case is held to by default, and a case whose
residual is a *different class of thing* declares its own budget with a written
reason (`null_tolerance_dbfs`, `peak_diff_tolerance_dbfs`, `tolerance_reason`).
The harness prints the relaxed gates and the reason on every run and records
them per case in the report, so a widened gate cannot be invisible. Report
schema `/2` → `/3`.

`osc_naive_sine` is set to **−67.0 dBFS RMS / −62.0 dBFS peak**: 6 dB above the
measurement, i.e. sized for a drift rate twice the observed one (~5 ULP), on
the grounds that there is no reason to think mac-vs-linux is the widest pair.
If Windows exceeds that, the case fails and someone re-measures — which is the
correct outcome, not a defect.

**The cost, stated rather than buried.** That case keeps full sensitivity to
level, waveform and tuning regressions — 0.3 ppm of detuning still fails it —
and loses sensitivity to localized damage below ~12 LSB. `voice_midi_note`
still covers the same oscillator at the −86 dBFS default in a fuller chain, so
the coverage is not lost, only moved.

**Verification artifact.** Five controls, run through the harness's own
`null_test` and `Case` loader rather than a re-implementation:

```
  C0a osc_naive_sine unmodified                   rms= -73.5 peak= -68.7  gates -67/-62   -> PASS
  C0b voice_midi_note unmodified                  rms=-123.1 peak= -90.3  gates -100/-86  -> PASS
  C1  osc_naive_sine, same drift 4x larger        rms= -61.5 peak= -56.7  gates -67/-62   -> FAIL
  C2  osc_naive_sine, finding-(b) glitch          rms= -73.5 peak= -68.7  gates -67/-62   -> PASS
  C2  voice_midi_note, finding-(b) glitch         rms=-107.5 peak= -80.8  gates -100/-86  -> FAIL
```

C1 is the one that matters: a widened gate is still a gate. C2 on
`voice_midi_note` reproduces finding (b)'s numbers (−107.6 / −80.8 dBFS)
independently on a second platform; C2 on `osc_naive_sine` is the accepted
sensitivity cost, demonstrated rather than asserted.

`render_harness.py --selftest` makes the arithmetic permanent — it synthesises
its audio in memory, needs no engine, and bakes in findings (b) and (e)'s
numbers plus the override plumbing. Confirmed discriminating: reverting
`null_test` to ignore its tolerance arguments fails it (RC=1).

## Open ends carried over

- **The harness has still never executed on a real GitHub runner.** See below.
- **`--update-refs` stays manual, and stays out of CI.** A reference is a claim
  about how something should sound; seeding one without listening locks in a bug
  behind a green check.

## CI — not installed, and cannot be by an agent

The workflow is checked in at [`docs/ci/verify.yml`](ci/verify.yml) and **is not
active**. It has to be copied to `.github/workflows/verify.yml` by hand.

A scheduled run cannot do it. The run prompt's STEP 5 forbids touching
`.github/workflows/**` — a workflow file executes with repository-secret access
on the branch that pushes it, so an edit there is a credential-scope change, not
a code change — and the bot's token deliberately carries no `workflow` scope, so
the restriction is enforced by the credential as well as the prose.

It needs no secrets: it downloads the published engine from
`synthedit.com/release_1_6/` rather than building it, so it costs about a minute
rather than the ~45 the engine build takes. Filed as **E1b**.

Note it also runs on a weekly `schedule:`, which is the right call — it surfaces
an engine regression as a harness failure instead of letting a new engine
silently invalidate every reference.
