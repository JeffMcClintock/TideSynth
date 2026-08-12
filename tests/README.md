# Audio verification

Renders each case in `cases/` through a headless SynthEditCL and null-tests the
result against the golden WAV of the same name in `references/`. The harness is
[`tools/render_harness.py`](../tools/render_harness.py); the reasoning behind
its gates and its traps is [`docs/e1-verification-harness.md`](../docs/e1-verification-harness.md),
and you should read that before changing a tolerance or seeding a reference.

Stdlib Python only — there is no install step.

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
