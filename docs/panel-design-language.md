# Panel design language

*Written 2026-08-20. Status: **adopted**, subject to tweaks and refinements.
Ruled in session by Jeff, after building it: see
[decisions.md](decisions.md), 2026-08-20. This resolves BACKLOG E17 in a
direction neither of the two proposals on the table had named.*

TIDE Rack's panels are **rendered, not drawn**. A module's faceplate and its
fittings — knobs, jacks, switch slots, vents, LED holes — are produced by a
path tracer ([modules/common](../modules/common), `tide_render`) as a cached
bitmap, and the moving parts are drawn over that in vector.

This document is the authority for the **traced layer**. The vector layer on
top of it is still governed by [ui-design-language.md](ui-design-language.md),
which is now scoped to that overlay rather than to the whole UI — see
[Two layers](#two-layers) below.

The reference implementation is [modules/PanelTest](../modules/PanelTest),
which carries every part described here behind a textual layout spec.

---

## Why a renderer

The look TIDE wants is **lighting, not drawing**. Brushed aluminium reads as
metal because the surface is a picture of the room around it; a gradient can
imitate a photograph of that, but it cannot behave like it. Faking it means
hand-tuning every highlight for every shape and getting it wrong the moment
the shape changes.

Three things follow from simulating instead, and they are the practical
argument:

1. **Highlights land where the geometry puts them.** The chamfer on a panel
   edge, the bright arc on a jack bezel, the shadow under a knob — none of
   these are authored. Move the part and they move with it.
2. **It is affordable because it is cached.** Every output is rendered once
   into a bitmap and blitted from then on, so the code optimises for
   correctness and for looking right, never for speed.
3. **No bitmap assets.** The renderer is two source files with no
   dependencies; panels are generated at runtime from a text description. The
   no-shipped-artwork property that made a pure 2D language attractive is kept.

The cost is honest and worth stating: a full trace takes **seconds**. That is
handled by rendering progressively (below), not by pretending it is fast.

---

## Sizes and the grid

Panels are **real Eurorack sizes**, and the unit system is **VCV Rack's**,
because matching it costs nothing and buys interchange.

| | |
|---|---|
| 1 DIP | 1 VCV Rack pixel = 1/75 inch |
| **1 HP** | 5.08 mm = **exactly 15 DIPs** (`RACK_GRID_WIDTH`) |
| **3U panel height** | 128.5 mm = **380 DIPs** (`RACK_GRID_HEIGHT`) |
| millimetres | `mm x 75/25.4`, i.e. `mm x 2.9528` |

Verified against `rack.hpp` rather than recalled. One DIP being one Rack pixel
means a VCV panel layout ports across unchanged, and both convert to real
millimetres exactly.

**A round 3 DIPs/mm was tried first and rejected.** It is only 1.6% away and
looks identical, but it puts 1 HP at 15.24 DIPs — a panel width that can never
be a whole number of DIPs, in a system where panel widths are counted in HP.

### Parts are specified as the real part

Every hardware dimension is a measurement, not an eyeball. This is what caught
the LED holes: they were `1.6` **DIPs** radius, a 1.07 mm hole, when a 3 mm LED
needs a 3.2 mm cut-out — so the radius wanted to be 1.6 **millimetres**, three
times bigger. They had rendered as specks for exactly that reason.

Current parts, and how they sit against VCV's standard component library:

| Part | TIDE | VCV equivalent | |
|---|---|---|---|
| LED hole | 3.2 mm | `MediumLight` 3.18 mm | matches |
| Jack bezel | 7.0 mm | `PJ301MPort` 8.03 mm | **13% small** |
| Big knob | 7.07 mm | `RoundSmallBlackKnob` 7.0 mm | ours is VCV's *small* |
| Small knob | 4.13 mm | (below `Trimpot`, 7.0 mm) | **no VCV equivalent** |
| Switch slot | 2.87 x 5.87 mm | `CKSS` 3.5 x 7.0 mm | slightly small |

**Two known discrepancies, recorded rather than fixed:**

- **The knobs are undersized against both VCV and real hardware.** What TIDE
  calls a big knob is VCV's *small* one, and TIDE's small knob is below
  anything in VCV's library. They look right on the test panel only because
  that panel is unusually narrow.
- **One rack unit is 48 DIPs = 16.26 mm = 3.2 HP**, which is not a real module
  width. Making the Rack Units pin mean HP is one constant (`kHpDips`), but it
  also rescales every coordinate in every layout string *and* the module's own
  width in the host, so it wants to be one deliberate edit.

---

## Materials

Three panel finishes, selected per module:

- **Brushed aluminium** (default) — horizontal grain, anisotropic.
- **Flat aluminium** — isotropic, slightly duller. Note it picks up coloured
  room sources far more strongly than brushed does: smooth and flat means
  near-axis lights reflect coherently instead of smearing.
- **Powder-coated steel** — a satin dielectric skin over a per-module colour.

Hardware finishes are fixed, not per-module: black satin plastic for knobs and
jack bodies, plated (roughness 0.14) for jack collars, unbrushed aluminium for
switch plates.

### The one thing the renderer cannot do unaided

**A flat face seen head-on under an orthographic camera reflects exactly one
colour.** Every pixel has the same normal and the same view direction, so the
result is a flat grey rectangle — and statistical anisotropy does *not* rescue
it, because the tangent is constant too.

So the visible brushing has to be **geometry**: the grooves go into the
distance field and the normal falls out of its gradient for free. Amplitude is
derived from a peak *slope* rather than a depth, because what you see is the
normal tilt.

This is the single most load-bearing fact in the whole renderer, and it
generalises. **Every part that reads as hardware does so because some surface
of it sweeps through angles:**

| Part | What catches the light |
|---|---|
| Panel face | the brushed grooves |
| Panel edge | the chamfer |
| Knob | the bevelled rim |
| Jack collar | it is a **torus** — a flat chrome annulus rendered nearly black |
| Jack bezel | the rounded outer shoulder |
| Pocket edge | the fillet where it meets the face |

A flat mirror facing the camera reflects one direction — straight back past the
camera at whatever happens to be behind it. Being chrome does not help.

### Grain direction is not free

The two brush directions do **not** want the same groove density. An
anisotropic highlight is a smear *along* the grain, so on a panel much taller
than it is wide a horizontal smear runs out of panel almost immediately and
fine grain collapses into what looks like noise. Horizontal wants about half
the density that reads as crisp brushing vertically.

---

## The room

Panels are lit by a procedural studio, invisible to the camera so the panel
keeps a transparent background while still being lit by — and reflecting — all
of it.

- **Sky above, warm floor below.** A blue cast from above and a warm one from
  below is most of what separates *outdoors under a sky, standing on a wooden
  floor* from *inside a grey box*. The ceiling **emits** as a broad soft
  skylight; painted, it only ever showed what the room's lamps bounced onto it.
- **The key is a four-pane window.** One rectangle smears into a single soft
  band; four with a mullion gap put a cross into every reflection, which is
  what reads as a window rather than a softbox.
- **Two coloured gear lights**, red and green, suggesting racked equipment.
  They sit **near the camera axis** — a flat, fairly smooth metal face viewed
  head-on reflects what is behind the *camera*, so a light out to the side at
  sixty degrees never lands in the reflected cone however bright it is.
- **A room is not free to enlarge.** Lights are fixed flux, so doubling the
  room in two directions quadruples the wall area, dims the walls, and takes
  the bounce that fills the panel's midtones with it.

---

## Rendering

**Progressive.** A preview is traced synchronously at a fraction of the size
(cost is linear in pixels, so 1/6 on a side is ~1/36th of the work and does not
show) and the full trace runs on a worker, swapping in when it lands. Until
then the preview is stretched over the panel: soft, but already the right
material under the right lights, which a flat grey placeholder is not.

**Cached** by pixel size and panel configuration, shared across instances, with
eviction — a live-edited layout would otherwise accumulate a multi-megabyte
render per keystroke.

**Orthographic, always.** A perspective knob only looks right from the exact
spot the camera was pointed, and panel art has to tile. A diagnostic flag
swings the camera off-axis for judging part heights and for promotional
renders; it is still orthographic, just rotated, which is what a technical
drawing uses for the same reason.

---

## The layout spec

A panel is **described, not hard-coded**. One component per statement:

```
knob [big|small] X Y
jack X Y
switch X Y
grill X Y [COLS ROWS]
slots X Y [ROWS]
led X Y
```

`X` and `Y` are DIPs from the panel's **top-left, y downward** — GUI
coordinates, so the vector overlays drawn later reuse the same numbers
unchanged. Statements separate on newline *or semicolon*, and unparseable
statements are **skipped, never errors**: the text is edited live and a
half-typed line must not blank the panel.

### The indent is automatic

Jacks cluster by proximity and each cluster gets one milled pocket, padded out
from the group's extent. The pocket then **shrinks away** from any non-jack
component it would otherwise swallow, down to a minimum pad. A pocket is a jack
feature; a knob half-in and half-out of one is a drawing error on a real panel
too.

---

## Two layers

The traced panel is the **background**. Everything that moves or carries
meaning is drawn over it in vector, at runtime:

| Traced (this document) | Vector overlay ([ui-design-language.md](ui-design-language.md)) |
|---|---|
| faceplate, material, grain | labels and legends |
| knob bodies | knob pointers |
| switch slots and plates | switch levers |
| jack collars and bezels | — |
| LED holes | LED lenses and their light |
| vents, pockets, chamfers | cables |

**This is what supersedes the "banned outright" list.** That ban — gradients,
drop shadows, blur, glow, bevels — was written for a language in which the
whole UI was flat 2D, and it does not apply to the traced layer, where shadows
and bevels are not decoration but the *output of a light simulation*. It
continues to apply, unchanged, to the vector overlay: a drawn drop shadow under
a label is still banned, and drawing a fake highlight onto a traced knob would
be fighting the renderer.

The distinction the old document drew between *material* and *decoration* — "material
is felt and not seen" — survives intact and is worth keeping in mind for the
overlay. The traced layer simply answers it differently: the surface really is
modelled, so there is nothing to fake.

---

## Status and open questions

Adopted, **subject to tweaks and refinements**. Known open items:

1. **Knob sizes** are below both VCV's library and real hardware (above).
2. **The rack unit is 3.2 HP**, not a whole HP (above).
3. **Shadow definition is set by light size, not part height.** The key is a
   large softbox, so its penumbra is about as wide as the shadow is long.
   Height gives a shadow its *length*; only a smaller or more distant key gives
   it *definition* — and the emissive sky works against that, since it is
   another large soft source filling shadows in. The two pull opposite ways.
4. **First-render cost.** Seconds per full trace, mitigated by the progressive
   preview and the cache, but not eliminated.
5. **The studio is local to PanelTest**, not in `tide_render`'s shared
   `addStudio`, because that function's look is pinned by committed reference
   images for every demo scene. Promoting it means regenerating and
   re-approving those in the same commit.
