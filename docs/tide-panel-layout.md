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

## Verified, 2026-08-24

Panel `panelRect` minus module `panelRect`, centre to centre, across the
hand-tweaked prefabs in `RackModules/`. Every entry lines up exactly:

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

## Upgrade status — most prefabs are not on this pattern yet

The pairing above is the target, not the current state. Surveyed 2026-08-24:

| prefab | TiDE Panel | patch points | knobs | state |
|---|---|---|---|---|
| `AR_jef` | yes | 4 TiDE | 2 | **upgraded** |
| `Output_jef` | yes | 2 TiDE | - | **upgraded** |
| `Sine_jef` | yes | 2 TiDE | - | **upgraded** |
| `MidiCv` | yes | 4 SE | - | panel, stock patch points |
| `Envelope` | - | 3 SE | - | not started |
| `Filter` | - | 3 SE | - | not started |
| `Midi` | - | 2 SE | - | not started |
| `Oscillator` | - | 2 SE | - | not started |
| `Output` | - | 2 SE | - | not started |

The five with no panel are consistent as they stand: stock `SE Patch Point`
modules draw their own jacks, which is correct when there is no TiDE panel
underneath to draw them instead.

`MidiCv` is the one part-way case, and it is worth knowing what to expect from
it while it waits its turn. It has a panel, 96 DIPs wide (two units), still
carrying the pin's untouched default `Layout` — which assumes one 48-wide unit
and puts jacks at `24 297` and `24 337`. Its four real patch points are the SE
kind, at x=80. So the panel paints two jack rings with nothing connectable under
them, and the stock patch points paint their own jacks elsewhere. Nothing is
broken; it is simply mid-upgrade.

## Labels: use the label module, not text-entry

`SE Label` is the convenient way to put text on a panel — a text-entry module is
the wrong tool for a caption. This is already the practice everywhere labels
exist: **13 `SE Label` instances across the nine prefabs, and zero text-entry
modules**. The five not-started prefabs simply have no labels yet, so adding
them is part of upgrading each one.

| prefab | labels |
|---|---|
| `AR_jef` | 6x `SE Label` |
| `MidiCv` | 5x `SE Label` |
| `Output_jef` | 1x `SE Label` |
| `Sine_jef` | 1x `SE Label` |
| the other five | none yet |

## If you move something

Move both. The `Layout` entry and the module's `panelRect` are the same point
expressed twice, and nothing checks that they agree — the panel cannot see the
modules above it, and the modules cannot see the panel below.
