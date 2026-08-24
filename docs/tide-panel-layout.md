# The TiDE Panel `Layout` pin, and how it pairs with patch points

Two layers make one rack module, and they have to agree on coordinates.

- **`SE TiDE:Panel`** draws the faceplate and everything on it — jack rings, knob
  bodies, vents, slots, LEDs. It is **inert eye-candy**: path-traced, and wired to
  nothing.
- **`TiDE Patch Point In` / `Out`** sits **layered above** it and provides the
  **functionality** — the actual pin a cable connects to. It deliberately draws
  nothing, *because* the panel already drew a better jack than it could.

`SE Patch Point in` / `out` — the stock SynthEdit ones — do the opposite: they
draw their own Eurorack-style jack picture. Use them on a TiDE panel and you get
two jacks, one real and one painted.

## The contract

The panel's **`Layout`** pin is a text list of what to draw:

```
grill 24 20; knob big 24 163; knob small 24 216; switch 24 247;
led 14 272; led 34 272; jack 24 297; jack 24 337; slots 24 371
```

Grammar (`parsePanelLayout`, `TiDEPanelGui.cpp:1185`):

| part | meaning |
|---|---|
| separator | `;` or newline; `#` starts a comment |
| `tok[0]` | `knob`, `jack`, `switch`, `grill`, `slots`, `led` |
| optional size | `knob big`, `knob small` — shifts the coordinates one token right |
| `x y` | **DIPs from the panel's top-left**, and the **CENTRE** of the feature |
| extras | `grill` takes `cols rows`; `slots` takes `rows` |

Coordinates are snapped to **0.5 DIP** (`snapToWidgetGrid`, `:1151`), applied to
every kind without exception.

**The rule: every `jack` and `knob` entry must sit at the same point as the
centre of the functional module layered over it.** `grill` and `slots` have no
module — they are decoration only.

## Verified, 2026-08-24 — and now checked by script

`scripts/check-prefab-layout.py` measures this contract for every file in
`RackModules/`: jack/knob Layout entries must match the TiDE patch point / knob
module centres exactly (both directions), the panel must be RackUnits x 48 by
384, stock `SE Patch Point` modules must not share a document with a TiDE
panel, and every visible child must sit on the plate. Run it after moving
anything; it is the automated form of "move one, move both".

The original hand measurement, 2026-08-24 — panel `panelRect` minus module
`panelRect`, centre to centre, across the hand-tweaked prefabs in
`RackModules/`. Every entry lines up exactly:

| prefab | panel | `Layout` entry | module centre |
|---|---|---|---|
| `AR_jef` | 48x384 | `knob big 24 200` / `24 240` | `SE TiDE:knob` (24,200) / (24,240) |
| | | `jack 24 80` / `120` / `160` | `TiDE Patch Point In` x3, same |
| | | `jack 24 337` | `TiDE Patch Point Out` (24,337) |
| `Output_jef` | 48x384 | `jack 24 297` / `24 337` | `TiDE Patch Point In` x2, same |
| `Sine_jef` | 48x384 | `jack 24 80` / `24 337` | `In` (24,80), `Out` (24,337) |

A patch point is 20x20 DIPs, so its `panelRect` top-left is its centre minus 10.
The panels are 48 DIPs wide and 384 tall — one rack unit, matching E5's ruling
(*"snap 3, row 384, standard width 48"*).

## Upgrade status — all nine are on this pattern

Upgraded 2026-08-25 (windows, interactive, Jeff directing). Every prefab in
`RackModules/` now carries a TiDE panel with an explicit `Layout`, TiDE patch
points, and labels; `scripts/check-prefab-layout.py` passes on all nine, and
each upgraded file was loaded in the editor and its finalised `panelRect`s
dumped to confirm the view measures exactly what was authored.

| prefab | TiDE Panel | patch points | knobs | jacks on panel |
|---|---|---|---|---|
| `AR_jef` | yes | 4 TiDE | 2 | ins 80/120/160, out 337 |
| `Output_jef` | yes | 2 TiDE | - | 297, 337 |
| `Sine_jef` | yes | 2 TiDE | - | in 80, out 337 |
| `Envelope` | yes | 3 TiDE | - | ins 80/120, out 337 |
| `Filter` | yes | 3 TiDE | - | ins 80/120, out 337 |
| `Midi` | yes | 2 TiDE | - | outs 80/120 (no ins; MidiCv's precedent) |
| `Oscillator` | yes | 2 TiDE | - | in 80, out 337 |
| `Output` | yes | 2 TiDE | - | 297, 337 (matches `Output_jef`) |
| `MidiCv` | yes | 4 TiDE | - | outs at (80, 54/124/194/264), 2 units wide |

The single-unit upgrades follow the hand-tweaked reference geometry exactly:
`grill 24 20`, title label centred at (24, 50), input jacks from y=80 at a
40-DIP pitch with their label 20 below each centre, output jacks at 337 (and
297 for a stereo pair), `slots 24 371`. `MidiCv` kept its own 96-wide layout —
jacks at x=80 with labels to the left — and its `Layout` pin now paints jack
rings under those real positions instead of the pin's one-unit demo default; it
skips the grill because its title label occupies that band.

## Labels: use the label module, not text-entry

`SE Label` is the convenient way to put text on a panel — a text-entry module is
the wrong tool for a caption. This is the practice everywhere labels exist:
**25 `SE Label` instances across the nine prefabs, and zero text-entry
modules**. Convention from the references: a short title label near y=50, a
label 20 DIPs under each *input* jack's centre, and no label on an output jack
whose bottom-of-panel position already says what it is (`AR_jef` and
`Output_jef` both leave theirs bare — `Midi`'s two outputs are labelled because
Gate and Trig are otherwise indistinguishable).

| prefab | labels |
|---|---|
| `AR_jef` | 6x `SE Label` |
| `MidiCv` | 5x `SE Label` |
| `Envelope`, `Filter`, `Midi` | 3x each |
| `Oscillator` | 2x |
| `Output`, `Output_jef`, `Sine_jef` | 1x each |

## The duplication is deliberate — do not "fix" it yet

The `Layout` entry and the module's `panelRect` are the same point written twice,
and nothing checks that they agree: the panel cannot see the modules above it,
and the modules cannot see the panel below.

**That is accepted, on purpose.** Jeff's ruling, 2026-08-24: *"one day we will
make it more sophisticated (automatic tracking) but for now the double-up is
acceptable in the interest of getting this shipped."*

So automatic tracking — the panel deriving its jack and knob positions from the
modules layered over it, or the reverse — is the eventual intent, not a gap
somebody should close on the way past. Until it exists, the rule is simply:

**move one, move both.**
