# Host-project fixtures

Saved DAW projects that exercise TIDE **through a real host**, which is the only
way to measure the thing PLAN's v0.1 acceptance test actually asks about: does a
patch still play after the host saves and reloads it?

Measure one with [`scripts/render-and-measure.py`](../../scripts/render-and-measure.py):

```bash
python3 scripts/render-and-measure.py --control
python3 scripts/render-and-measure.py tests/hosts/v1-rack.rpp
```

Run `--control` first. It renders a known −6 dBFS 1 kHz sine and must report
exactly −6.0 peak / −9.0 rms; that proves the render-and-measure chain detects
audio at all, so a subsequent silence is a fact about the patch rather than
about the script.

| Fixture | What it holds | Measured 2026-08-18 |
|---|---|---|
| `v1-rack.rpp` | Oscillator → Envelope → Output, cabled jack-to-jack | **peak −6.3 dBFS, rms −17.0 dBFS** — 440.0 Hz, left channel only |
| `v1-rack-uncabled.rpp` | the same three prefabs, **no patch cables** | **−inf, silent** — the negative control |

The pair matters more than either one alone. `v1-rack-uncabled.rpp` is what a
saved rack looks like when nothing joins one module to another, and it reports
**eight `<Line>` elements** — the cables *inside* the three prefab containers.
So a non-zero `<Line>` count is not evidence that a rack is wired, and the
uncabled fixture is what keeps that from being re-learned the hard way. The
count that decides it is the patch-cable list in `HC_PATCH_CABLES`; see the
docstring of `render-and-measure.py`.

## Regenerating one

These are GUI artifacts — there is no script that writes them, because placing
and cabling rack prefabs is an editor operation. To rebuild one:

1. Launch REAPER **from a shell**, not `open -a`, so an uncaught C++ exception
   in the plugin names itself on stderr. TIDE also prints
   `TIDE: 3 rack prefab(s) seeded from the bundle` there, which confirms the
   installed bundle's `Resources/Prefabs/` is what supplied them.
2. Insert **TIDE Rack** on a track. It is listed under its product name, not
   its filename — filtering the FX browser for "TIDE" finds it, and REAPER's
   plugin cache may still hold an older name from a previous build, in which
   case re-scan (Preferences → Plug-ins → VST → Re-scan).
3. Open its editor, pick **Prefabs** in the browser, and click a prefab then
   click the rack to place it. Drag jack to jack to cable. **Cable each jack
   once** — dragging a second cable from a jack that already has one grabs the
   existing cable instead of making a new one.
4. Save. The rack needs no MIDI: the oscillator's PITCH jack defaults to 5 V
   (440 Hz) and the envelope's GATE jack defaults open, so three cabled modules
   emit a continuous tone with nothing sequenced.
