# Audio verification

Renders each case in `cases/` through a headless SynthEditCL and null-tests the
result against the golden WAV of the same name in `references/`. The harness is
[`tools/render_harness.py`](../tools/render_harness.py); the reasoning behind
its gates and its traps is [`docs/e1-verification-harness.md`](../docs/e1-verification-harness.md),
and you should read that before changing a tolerance or seeding a reference.

Stdlib Python only — there is no install step.

**This harness measures the ENGINE, not the plugin inside a host.** It builds a
graph with SynthEditCL verbs, so it can prove a module set still sounds the way
it did, but it cannot say whether a patch survives a DAW's save and reload. That
question has its own fixtures and its own script — see
[hosts/README.md](hosts/README.md) and
[`scripts/render-and-measure.py`](../scripts/render-and-measure.py).

## Run it

```bash
python3 tools/render_harness.py --cli <path>/SynthEditCL --modules <abs path>/PlugIns
```

`--modules` **must be absolute**, and the harness resolves it for you. See
finding (a) in the doc for what a relative path does — it is worse than an
error.

On the Linux box, against the local engine build:

```bash
python3 tools/render_harness.py --cli ~/SE/build/SynthEditCL/SynthEditCL --modules ~/SE/build/modules
```

Exit codes: `0` all passed · `1` a case failed · `2` bad configuration.

## Reading the result

Three gates, and all three must pass:

| Gate | Threshold | Catches |
|---|---|---|
| signal present | peak > −90 dBFS | a well-formed WAV of digital silence |
| null RMS | ≤ −100 dBFS | broad regressions — level, waveform, tuning, filters |
| null peak | ≤ −86 dBFS | localized damage — a click, a dropout, one bad block |

Those two null thresholds are the **rounding-class budget**, and E1a measured
them against real cross-platform data on 2026-08-14 rather than arithmetic:
macOS against the Linux goldens leaves 22.9 dB of RMS headroom and 4.3 dB of
peak headroom, the latter sitting above a hard 1-LSB-per-sample ceiling.

**A case may widen its own gates**, and one does. `null_tolerance_dbfs`,
`peak_diff_tolerance_dbfs` and `tolerance_reason` in a case file override the
defaults for that case alone. Use them only for a residual that is a different
*class* of thing, not a bigger version of rounding — `osc_naive_sine` is the
founding example, because a free-running oscillator's cross-platform residual
grows linearly with render duration and no fixed number describes it. The
harness prints the relaxed gates and the reason on every run, and records them
per case in the report; read the case's own entry, not `config`, when asking
what a result was judged against. Finding (g) in the doc is the reasoning.

## Check it without an engine

```bash
python3 tools/render_harness.py --selftest
```

Synthesises its audio in memory — no engine, no fixtures, runs anywhere. It
guards the gate arithmetic the docs quote as settled fact and the per-case
override plumbing (in particular that an override does not leak to other cases).

`--out report.json` carries every measurement plus `module_sources`. **Check
that field on any local run.** If `foreign_module_sources` is non-empty the
engine scanned a folder you did not name, and the run does not prove the module
set under test is what rendered — finding (d). The harness prints a warning
rather than failing, because on a developer box that state is normal and
failing on it would break the harness exactly where someone is debugging.

## Adding a case

A case is one JSON file naming the SynthEditCL verbs to build the graph, the
`$alias:pin` to record from, and a duration and rate. Then:

```bash
python3 tools/render_harness.py --cli ... --modules ... --filter <name> --update-refs
```

**`--update-refs` is deliberately manual and deliberately never run in CI.** A
reference is a claim about how something *should* sound. Seed one without
listening to it and you have locked in whatever bug was present, with a green
check on top.

### `<case>.provenance.json` — which platform seeded the reference

Seeding also writes a sidecar next to the WAV (BACKLOG **E1c**). **A null-test
number is uninterpretable without it.** The same residual means *"rounding,
ignore it"* when both sides ran on one platform and *"cross-platform drift, size
your gates for it"* when they did not — and until 2026-08-22 nothing here
recorded which had happened.

That is not hypothetical. E1c exists because `prefab_oscillator`'s wide gates
were inherited from a measurement taken on a **different oscillator**, and
establishing what the four cases' numbers were even comparable to meant reading
a journal entry from nine days earlier. They *were* comparable — but only
because one run happened to produce them all, which is luck, not method.

The `recorded` field says how much the record is worth:

| `recorded` | means |
|---|---|
| `measured` | written by the run that seeded the reference. First-hand. |
| `reconstructed` | backfilled later from journal or case-file evidence, quoted in `evidence`. Says what someone wrote down afterwards. |
| `unknown` | nobody recorded it and it could not be established; `seeded_on` is `null`. **A cross-platform claim about this case is unsupported.** |

Three of the six existing references are `unknown` — `prefab_envelope`,
`prefab_filter` and `prefab_midi`. Re-seeding any of them fixes its record as a
side effect.

## Prefab coverage, and the two prefabs that cannot have a case

TIDE ships rack prefabs from `SE16/RackModules/` (one flat folder, all
`.synthedit`). They were originally generated by
`build-prefabs.py`). Four have a case here; two cannot, and the reasons are
measured rather than assumed — recorded so nobody spends a session rediscovering
them (BACKLOG **E13**, 2026-08-19, macos; Filter added by **E2b** the same day).

**Adding a module to the rack? Check it against this table first.** A prefab is
worth a case exactly when the harness can record something downstream of its
DSP — which is why Output and MIDI-CV have none, and why every module added
under E2 should be checked against that question before a case is written for it.

| Prefab | Case | Why |
|---|---|---|
| TIDE Oscillator | `prefab_oscillator` | — |
| TIDE Envelope | `prefab_envelope` | — |
| TIDE MIDI | `prefab_midi` | — |
| TIDE Filter | `prefab_filter` | needs a SOURCE as well — a filter fed DC measures nothing, so the oscillator drives it |
| TIDE Output | **none** | the harness cannot observe a sink, see below |
| TIDE MIDI-CV | **none** | it is a facade with no DSP of its own, see below |

**TIDE Output — the harness structurally cannot record it.** The prefab ends in
`Sound Out`, whose `Out` pin is an *input*, so the recorder has nothing to
attach to and `--render-audio --from $snd:Out` fails outright:

```
{"cmd":"render-audio","ok":false,"error":"could not connect --from source to recorder"}
```

Recording the patch points that feed it instead would test the patch points, not
the prefab. What the prefab actually promises — that L and R become **two**
channels rather than one stealing the other, because `Sound Out`'s input is
`IO_AUTODUPLICATE` (`ug_soundcard_out.cpp:20`) — is visible in the build
commands, not in any audio:

```
{"cmd":"connect","ok":true,"from":[...,1],"to":[963908723,0]}   <- L, pin 0
{"cmd":"connect","ok":true,"from":[...,1],"to":[963908723,1]}   <- R, pin 1, a NEW pin
```

That is a graph-shape assertion, and this harness is an audio harness. It would
want a different check, not a case file. The end-to-end proof that the Output
prefab works is the v0.1 fixture pair in [hosts/](hosts/README.md): a rack cabled
through it renders −6.3 dBFS, and the uncabled negative control renders silence.

**TIDE MIDI-CV — there is nothing to render.** It is deliberately a *facade*: it
holds four jacks and a faceplate and nothing else, and every jack is fed from
**outside** the container by the `SE MIDI to CV 2` that TIDE places at the
document root (the reasoning is in `build-prefabs.py`, and the underlying
limitation is **E7** — polyphony cannot escape a container). Rendered on its own
it reproduces whatever its scaffolding was set to, which measures the scaffolding.
What is worth guarding about it is the outer-pin mapping TideApp hard-codes
(pin 7 = PITCH, 8 = GATE, 9 = VEL, 10 = TRIG), and that is again graph shape.
