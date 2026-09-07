# `e75-vcv-visible-rack.xml` — BACKLOG E75's fixture

**What it is:** [`e53-vcv-rack-segv.xml`](e53-vcv-rack-segv.README.md) **with two view
fields changed and nothing else**. Same 19 modules, same 2 patch cables, same everything
the other fixture is cited for — it opens *looking at the rack* instead of 3,500 DIPs
away from it.

| | |
|---|---|
| preset file | **51,694 bytes**, md5 `d9ba73d76c8c405957c4383ae1894ca2` |
| document inside it | **38,661 bytes**, md5 `2a01b6c70cc9ab683d787a014aa7bb00` |
| derived from | `e53-vcv-rack-segv.xml` (51,690 / 38,658 bytes) |
| `PanelLocationCenter` | **(988.5, 468)** — was (3984, 3984) |
| `PanelLocationZoom` | **0.65** — was 1 |

Regenerate it, and check the claim above, with one command each:

```bash
python3 scripts/set-view-center.py tests/fixtures/e53-vcv-rack-segv.xml \
        --center 988.5,468 --zoom 0.65 -o tests/fixtures/e75-vcv-visible-rack.xml
python3 scripts/set-view-center.py tests/fixtures/e75-vcv-visible-rack.xml --show
```

**A byte diff of the two committed files is useless and that is not this fixture's
fault** — the document is base64 inside one `<Param>`, so any change to it rewrites the
whole blob. Diff the *decoded* documents instead; they are 798 lines each and differ on
**two of them**. `scripts/set-view-center.py`'s docstring carries the recipe.

## Why a second fixture rather than an edit to the first

`e53-vcv-rack-segv.xml` is named by **E19**, **E49** and **E53**, and E53's whole subject
is a segfault during graph build — a thing the view position has nothing to do with, and
a thing nobody should have to re-establish because a different row wanted a different
scroll position. The two fields here are exactly the ones E53 does not care about, so the
cheap move is a sibling rather than a mutation.

## What was actually wrong, which is not what E75 assumed

E75 was filed asking *"whether a rack can hold a module the view cannot reach"*.
**It cannot, and no module here was ever unreachable.** Measured 2026-09-07 (macos):

- `CContainer`'s constructor initialises `PanelLocationCenter` to **(3984, 3984)** —
  `viewDimensions / 2`, the canvas midpoint (`SynthEditLib/EditorLib/CContainer.cpp:82`).
  So that value in a saved document does not mean "centre it"; it means **nobody ever
  panned this view before it was saved**, and it is indistinguishable from a choice.
- TiDE restores the stored centre faithfully — that is BACKLOG **E33**, at
  `SynthEditSem/TideApp.cpp`'s `setPanZoom(targetContainer->GetViewCenter(...))`. The
  restore is not the bug; it is what makes the never-panned default visible.
- `e53`'s modules sit at **x 480..1380, y 276..660**. Its view therefore opened about
  3,000 DIPs right and 3,500 DIPs below them, onto bare rails — which is exactly what
  three separate E19 runs screenshotted and reported as *"no VCV panel on the rack"*.
- **The modules were reachable the whole time.** One wheel detent is 30 document DIPs
  (`ViewBase.cpp`: `pixelsPerDetent = 0.25f`, 120 delta per detent), so the gap is
  **118 vertical notches and 100 horizontal ones**. Driven through the standalone's
  command channel, all five VCV panels and both patch cables appear. A harness that
  nudges the wheel a few notches concludes "unreachable"; the number is why.

**The control that settles it is the shipped default rack.** `DefaultRack.synthedit` puts
its modules in the *same band* — l=36 t=276 r=1377 b=660 — and is visible, because it
carries a centre somebody actually saved:

| | `PanelLocationCenter` | modules | opens on them? |
|---|---|---|---|
| `DefaultRack.synthedit` | (1261.157, 584.740) | l=36 t=276 r=1377 b=660 | **yes** |
| `e53-vcv-rack-segv.xml` | (3984, 3984) — never panned | l=120 t=144 r=1380 b=808 | **no** |
| `e75-vcv-visible-rack.xml` | (988.5, 468) | *(identical to e53)* | **yes** |

Two documents, the same module band, opposite outcomes, one field.

## Why the zoom is here too, and why 0.65

A centre alone is not enough and the first attempt at this fixture proved it. The VCV LFO
starts at x 597 and the VCV Scope ends at x 1380 — **783 DIPs** — while the standalone's
rack canvas is **567.5 x 587.5 DIPs** at its default 1100x626 window (measured off a
screenshot, not assumed: the module browser takes the rest). At zoom 1 the two ends are
clipped no matter where the centre goes.

0.65 puts 873 x 904 document DIPs on screen, so both modules clear the edges by about 45
DIPs. `DefaultRack.synthedit` reaches for the same remedy at 0.745, which is the sanity
check on the number rather than a coincidence.

**This ties the fixture to a window size, and that is a real limit.** A host giving the
editor a much narrower view can still clip it. 0.65 was chosen against the standalone's
default; a hosted VST3 or AUv3 window is not obliged to match.

## What it is for

E19's **pixel-diff** and **int/bool/enum** clauses, which E75 blocks. Both need the Scope
on screen; it now is. **Neither clause passes**, and with the module visible those results
finally mean something instead of nothing:

- **pixel diff over the Scope display: 0 of 52,577 over 15 s.** The control is inside the
  same pair, as this project's own rule requires — 619 pixels of the frame *did* change,
  all inside the WT LFO's phase indicator, so the capture is live. The Scope's
  display-state is arriving in full (`display-state update #1300 arrived (65548 bytes)`)
  and **324 of 327 applies carry one identical checksum**, `sum=19140`. Filed as **E83**.
- **right-click anywhere on a VCV panel gives the rack's own 7-item menu** — *Goto Rack,
  About TIDE..., Cut, Copy, Paste, Delete* — byte-identical to the menu over empty canvas,
  and unchanged after selecting the module. There is no VCV context-menu option to toggle.
  Filed as **E82**.

Both were invisible while the panels were off screen, and neither is this fixture's doing.

## How to run it

Exactly as `e53-vcv-rack-segv.xml` — see [its README](e53-vcv-rack-segv.README.md) for
the `session.xml` route, the `TiDE Rack` folder spelling, the `session.loading` sentinel,
and the per-platform isolation rules. On macOS:

```bash
mkdir -p "$SCRATCH/TiDE Rack" && cp tests/fixtures/e75-vcv-visible-rack.xml "$SCRATCH/TiDE Rack/session.xml"
GMPI_STANDALONE_CONFIG_DIR="$SCRATCH" build-<tree>/SynthEditSem/TIDE-Rack.app/Contents/MacOS/TIDE-Rack
```

then screenshot through the command channel it prints (`command channel: /tmp/…`):

```bash
--screenshot <path>
```

A correct run shows `LFO`, `PULSES`, `S&H ASR`, `WT LFO` and `SCOPE` seated on the rails,
with the orange patch cable running from the LFO to the Scope's `IN 1`. **Build with
`-DTIDE_VCV_FUNDAMENTAL=ON`** or the five VCV modules are dropped on import and the rack
comes up with four modules and a wall of `parameter names module handle N, which this
document does not contain`.
