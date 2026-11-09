# `e94-mixer-invert.xml` — BACKLOG E94's fixture

**What it is:** [`e83-vcv-scope-cabled.xml`](e83-vcv-scope-cabled.README.md) **with a
`VCV: Mixer` added and the LFO→Scope cable routed through it**, so that for the first
time a committed fixture carries a module with a **bool** context-menu option on the
visible rack page. Every e83 module keeps its handle and its `panelRect`, so E19's
existing coordinates and its `Wave points` arm on the WT LFO are untouched.

| | |
|---|---|
| preset file | **52,330 bytes**, md5 `ee68d059c0dbf52b54c378fbdf1e2d4f` |
| document inside it | **39,138 bytes**, md5 `4f2ded97ec3d84917347a7f44bb2f8f3` |
| derived from | `e83-vcv-scope-cabled.xml` (51,806 / 38,745 bytes) |
| modules | **20 rack-level**, was 19 — one `VCV: Mixer`, handle `94000094`, panel `1125,276 → 1170,660` |
| `HC_PATCH_CABLES` | **3 cables** in both halves — was 2 |

Regenerate it, and check the claim above, with one command each:

```bash
python3 scripts/place-rack-module.py tests/fixtures/e83-vcv-scope-cabled.xml \
        -o tests/fixtures/e94-mixer-invert.xml \
        --type "VCV: Mixer" --handle 94000094 --panel 1125,276,1170,660 \
        --cable 529566147:5 --cable-out 987654321:0:6 \
        --drop-cable 529566147:5:987654321:0
python3 scripts/patch-cables.py tests/fixtures/e94-mixer-invert.xml --show
```

`--show` exits **0** — the two halves agree, which is E83's check and the reason the
derivation writes both.

**A byte diff of the two committed files is useless**, for e83's and e75's reason: the
document is base64 inside one `<Param>`, so any change rewrites the whole blob. Diff the
*decoded* documents instead — 799 lines against 804, **differing on 9**: one `<Module>`
line in `<DSP>`, four `<module>`/rect lines in `<Editor>`, and the two base64 cable
strings, one per half.

## Why a Mixer, and why `Invert output`

E19's `int/bool/enum` clause has an **enum** producer — the WT LFO's `Wave points`, which
passed on linux/VST3 (2026-10-26) and win/VST3 (2026-11-02) — and had **no bool producer
in any committed fixture**, so both lanes ended their cell with *"bool not measured"*.
That was never a platform problem; it was one missing input file.

Measured over all 39 compiled-in models with
[`tests/e82_rack_menu_probe.sh`](../e82_rack_menu_probe.sh), which reads the exact data
`RackEditor::populateContextMenu` is built from (`readPanelLayout()`), rather than
clicking anything:

| | |
|---|---|
| models offering at least one option | **12 of 39** — agreeing with the 2026-09-16 datatype census's `bool=11` plus `enum` |
| `Mixer` | `options=2 entries=3` — `SEP`, **`BOOL Invert output` default 0**, `BOOL Average voltages` default 0 |
| `Scope` (the module E82 first probed) | `options=0 entries=0` — the control: 27 of 39 offer nothing |

The Mixer is the row's recommended module because **its readout is a sign flip rather
than a magnitude change**: upstream's `process()` is `out = Σinputs × gain` with
`gain = Level` and `if (invert) gain *= -1` (`modules/Mixer/vcv/Mixer.cpp:46-55`), and
`Level` defaults to 1. One input carrying 1.0 therefore gives exactly ±1.0, which cannot
be confused with drift or with a different wave position.

## Why the Mixer is SPLICED INTO the chain rather than hung off it

The adaptor's `first NONZERO` trace is **one latch per instance, and it checks inputs
before outputs** (`SynthEdit_Rack_Adaptor/RackAdaptor.h:629-643`). So a module with a
connected input never traces its own output. Measured on the first cut of this fixture,
where the Mixer was fed from the LFO and left unconnected downstream:

```
RackProcessor: 'Mixer' connections ins=100000 outs=0
RackProcessor: 'Mixer' first NONZERO INPUT pin 0 (1.000000)        <- and no OUTPUT line
```

E94's row recommends reading `'Mixer' first NONZERO OUTPUT`; **that line cannot exist for
a Mixer**, by construction. The readable arrangement is the one committed here — the
Mixer between the LFO and the Scope, so the Scope's own input trace reports the Mixer's
output, with the Mixer's input trace as the in-frame control.

## The measurement this fixture exists to make possible

Measured 2026-11-10 (windows, scheduled run). `main` at `f6551b4d8`, Release,
`-DTIDE_VCV_FUNDAMENTAL=ON -DSE_LOCAL_BUILD=OFF
-DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`, 0 build errors; the bare VST3 host
[`tests/e80_vst3_feedback_probe.cpp`](../e80_vst3_feedback_probe.cpp), **no DAW, no
window, no screen**, five fresh instances, 19 of 19 `PASS` and 0 `FAIL` in each.
**Run twice end to end** -- derivation, roundtrip, option edit and all five arms --
**with identical numbers.**

| arm | `Invert output` | `'Mixer' first NONZERO INPUT` | `'Scope' first NONZERO INPUT` = the Mixer's output |
|---|---|---|---|
| A1 | 0 | 1.000000 | **+1.000000** |
| B1 | **1** | 1.000000 | **−1.000000** |
| A2 | 0 | 1.000000 | **+1.000000** |
| B2 | **1** | 1.000000 | **−1.000000** |
| control, option never written | (default 0) | 1.000000 | **+1.000000** |

**The controls are what make the flip attributable.** The Mixer's own input is
`1.000000` in all five arms and so is `'LFO' first NONZERO OUTPUT`, so the source is
identical; `'Mixer' params: 1` in all five, so `Level` is 1 throughout and the magnitude
is unchanged; and the unedited arm repeats the A value, so a difference needs the option
to have been written.

Re-run it:

```bash
python3 scripts/place-rack-module.py tests/fixtures/e83-vcv-scope-cabled.xml \
        -o tests/fixtures/e94-mixer-invert.xml --type "VCV: Mixer" \
        --handle 94000094 --panel 1125,276,1170,660 --cable 529566147:5 \
        --cable-out 987654321:0:6 --drop-cable 529566147:5:987654321:0
e80vst3probe TIDE-Rack.vst3 tests/fixtures/e94-mixer-invert.xml --blocks 50 \
        --no-editor --save rt.xml
python3 scripts/place-rack-module.py rt.xml -o rt-invert.xml \
        --handle 94000094 --set-option 1:1
e80vst3probe TIDE-Rack.vst3 rt-invert.xml --blocks 200 --no-editor 2>&1 \
        | grep "'Scope' first NONZERO"
```

**The `--save` step is not a detour, and it is the fixture's one surprise.** A freshly
placed module has **no `<Parameter>` entries at all** — this fixture's Mixer is two
elements and nothing else, exactly as e83's Scope is — and that is not a defect: the
plug-in creates them on load from the module's declared defaults. Measured, from the
save of this very fixture:

```
<param type="4" ... shortName="Invert output"     vstParameterNumber="30" module="94000094" moduleParamId="1">
<param type="6" ... shortName="Level"             vstParameterNumber="31" module="94000094" moduleParamId="0">
<param type="4" ... shortName="Average voltages"  vstParameterNumber="27" module="94000094" moduleParamId="2">
```

So the option's stored value only exists once the plug-in has written one, and the
toggled arm is that save with one value changed.

## What this fixture does NOT establish

**That a right-click on the Mixer's panel reaches the option.** The arms above set the
stored parameter the menu writes to (`RackAdaptor.h`'s `menuBoolPins`), one click short
of the editor path. The offer side is measured from `readPanelLayout()` above, and the
click itself is **E19**'s clause, in a host or in the standalone, and needs a screen.

**That `Invert output` is automatable by the host — it is not, and this is measured
rather than assumed.** `--list-params` on this rack reports the controller exposing
**2,080 parameters, every one of them a MIDI CC, Channel Pressure or Pitch Bend**, and
none of the rack's own. `setParamNormalized` on the id a save calls
`vstParameterNumber="30"` is refused, and a `IParameterChanges` queue carrying it changes
nothing — the four `--set-param` arms all read `+1.000000`. That `vstParameterNumber` is
an index into TIDE's own patch list and **not** a VST3 `ParamID`. This is **V2**'s
territory (*host-automatable parameters without a panel view*), which is `BLOCKED`, and
nothing here is a reason to unblock it.

**That any other datatype has a producer.** `string` still has none (the 2026-09-16
census), and `blob` and `float` are E19's other clauses.
