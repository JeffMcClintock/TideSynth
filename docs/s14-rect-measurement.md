# S14 — why placed modules render nothing: the measurement

Measured 2026-08-18 (macos, scheduled run, no GUI available). This is the
"cheap first measurement" the S14 row asked for, and it answers the row's
question. **No code was changed** — every fix site is in GATED `SynthEditLib`,
filed as **S15**.

## The question S14 asked

> Unknown and worth measuring first: whether the zero rect comes from the
> placement path not assigning one, or from rack mode expecting a *panel* rect
> rather than `structRect`.

**Answer: the mechanism below is real, but it is not a defect — and this
document originally drew the wrong conclusion from it.** Corrected 2026-08-18
by Jeff, the same day, before anything was built on it.

Rack mode addresses the *panel* rect; a plain DSP module has no panel rect; so
the assignment lands on an empty base-class no-op and is discarded, and
`structRect` stays `0,0,0,0`. All of that is accurately measured. **What was
wrong was the inference that `CUG` therefore needs geometry it does not have.**

**A bare DSP module is not a rack citizen and was never meant to be one.** Rack
modules are Containers, designed in advance and shipped as **prefabs**, added
to the rack at runtime: the container carries the panel, with patch-points and
knobs/sliders on it wired internally to non-GUI modules like an oscillator. The
container is what the rack draws. A module that has its own GUI — a scope, say
— can also sit on the rack directly, and **that already works today**.

So the three bare `1 KHz Tone` modules in the measured document are not a
product composition at all. They are what audio testing looks like: drop plain
modules in, switch to the structure view, check basic audio functionality. That
is a developer workflow, not something an end user does on the main screen, and
it is why they have no rack geometry — nothing was supposed to give them any.

## What was compared

Two documents, both read from disk, no host and no build:

| | document |
|---|---|
| TIDE | `/tmp/tide-s11-final.rpp.block0.param1.xml` — the 2516-byte chunk S11's fix made round-trip byte-identically |
| full SynthEdit | `SynthEdit2/Resources/prefabs/Controls/Knob.syntheditprefab` — a shipped prefab, i.e. modules placed by full SynthEdit |

### TIDE — every rect zero

```
master_container  type=Container        structRect=0,0,0,0 (ZERO)   panelRect=ABSENT
module            type=1 KHz Tone       structRect=0,0,0,0 (ZERO)   panelRect=ABSENT
module            type=1 KHz Tone       structRect=0,0,0,0 (ZERO)   panelRect=ABSENT
module            type=1 KHz Tone       structRect=0,0,0,0 (ZERO)   panelRect=ABSENT
```

### full SynthEdit — every `structRect` non-zero

```
master_container  type=Container            structRect=0,16,20,36        panelRect=ABSENT
module            type=Container            structRect=180,129,264,244   panelRect=ABSENT
module            type=SE Text Entry4       structRect=572,276,680,436   panelRect=32,76,97,99
module            type=SE PatchMemory Float structRect=12,84,132,168     panelRect=0,0,0,0 (ZERO)
module            type=Image3               structRect=428,240,548,393   panelRect=32,24,97,89
module            type=Image3               structRect=428,60,548,213    panelRect=32,24,97,89
module            type=SE ImageTinted XP    structRect=560,48,680,236    panelRect=32,24,96,88
module            type=FloatToVolts         structRect=176,264,260,300   panelRect=ABSENT
module            type=IO Mod               structRect=296,288,356,312   panelRect=ABSENT
```

## Two things this rules out

**An absent `panelRect` is not the defect.** `FloatToVolts` and `IO Mod` are
plain DSP modules placed by full SynthEdit: they carry **no `panelRect` at
all** and a perfectly good non-zero `structRect`. TIDE's `1 KHz Tone` matches
them on the absent `panelRect` and differs only in the zero `structRect`. So
"TIDE is missing panelRect" is a red herring — the same red herring shape as
the `.ips` thread reasoning in S11.

**It is not persistence.** S11 landed; the document round-trips
byte-identically. The modules, their types and their handles are all correctly
saved and restored. The state is right and only the geometry is wrong.

## The mechanism, from sources

Four facts, each checkable independently:

1. **`CDocOb`'s rect accessors are a no-op and a zero.**
   `SynthEditLib/DocOb.h:89-93` — `getViewObRect` returns `{}`, and
   `setViewObRect` has an **empty body**.
2. **A plain module only implements the *structure* view.**
   `SynthEditLib/CUG.cpp:2557` — `CUG::getViewObRect` returns `m_struct_rect`
   for `CF_STRUCTURE_VIEW` and otherwise delegates to `CDocOb`, i.e. to the
   zero above. `CUG::setViewObRect` (`CUG.cpp:2566`) likewise writes
   `m_struct_rect` **only** for `CF_STRUCTURE_VIEW`. `structRect` is the only
   rect it serialises (`CUG.h:32`).
3. **Only a GUI control has a panel rect at all.**
   `SynthEditLib/Control.h:23` — `s("panelRect", PanelWndPosition)` is on
   `CControl`, not on `CUG`. That is exactly why the prefab's DSP modules show
   `panelRect=ABSENT` and its GUI modules do not.
4. **Rack mode places through the panel view.**
   `SynthEditLib/MfcDocPresenter.cpp:811` —
   `m->setViewObRect(view ? view->getViewType() : CF_PANEL_VIEW, rl)`, and
   `getRackLayout` enables rack mode **only** on the top-level panel view
   (`MfcDocPresenter.cpp:1421`, and the comment at
   `SynthEdit/SynthEditSem/TideApp.cpp:497-500` says so in prose).

Compose them: in TIDE every placement calls
`CUG::setViewObRect(CF_PANEL_VIEW, …)` → `CDocOb::setViewObRect` → **`{}`**.
The rect is dropped on the floor, nothing is marked modified, and
`m_struct_rect` keeps its constructed zero. In full SynthEdit the same modules
are placed in the *structure* view, which is the one branch `CUG` implements —
hence non-zero `structRect` in every prefab.

This also explains why the container itself is zero: `master_container` is a
`CUG` too, placed the same way.

## What the measurement is still good for

The four facts above stand, and one of them now reads as *confirmation* rather
than a complaint. In the full-SynthEdit prefab, the modules carrying a
`panelRect` are exactly the ones with a GUI, and the plain DSP modules
(`FloatToVolts`, `IO Mod`) carry none. That is the same split at code level
that Jeff describes at product level: GUI-bearing modules have a panel presence
and can go on the rack; plain modules cannot, and are composed inside a
container instead.

The container half is already provided for, under a name this document did not
think to look for. `CContainer` is not a `CControl` — it is
`CContainer : CUG_with_patches : CUG` — but it overrides the rect accessors
itself (`SynthEditLib/CContainer.h:60-62`) and serialises its own panel
geometry as **`PanelWndPosition`** (`:214`). That element is already present in
the measured chunk, on the `master_container`. **The rack's geometry mechanism
exists, on the object that is meant to have it.**

**Neither fix this document originally proposed should be built:**

- *Give `CUG` a panel rect* — adds geometry to the object that is specifically
  not supposed to appear on the rack, and duplicates what `CContainer` has.
- *Route rack placement to `structRect`* — pushes rack geometry into the
  schematic view's rect and fights the container's existing `PanelWndPosition`.

**The GATED framing was wrong with them.** This document said the fix was
entirely in `SynthEditLib` and needed a ruling because option (b) changed the
on-disk schema for every SynthEdit module. No shared-format change is needed,
so that claim on Jeff's attention goes away too.

## Reproducing this measurement

No host, no build, ~2 seconds:

```
python3 - <<'EOF'
import xml.etree.ElementTree as ET
for path in ("/tmp/tide-s11-final.rpp.block0.param1.xml",
             "SynthEdit2/Resources/prefabs/Controls/Knob.syntheditprefab"):
    print("==", path)
    for el in ET.parse(path).getroot().iter():
        if el.tag in ("module", "master_container"):
            def f(x):
                if x is None: return "ABSENT"
                v = (x.get("l"), x.get("t"), x.get("r"), x.get("b"))
                return "ZERO" if all(a == "0" for a in v) else ",".join(v)
            print(f'  {el.get("type"):24} struct={f(el.find("structRect")):16} panel={f(el.find("panelRect"))}')
EOF
```

## What to measure next

Nothing, for S14 — closed as not-a-defect. The live work moved elsewhere: the
prefab rack modules themselves are **E2a**/**E2**, and rack-shaped styling for
GUI-bearing modules is **E5**.

**The lesson, since it cost a row and a request for a ruling:** the measurement
was sound and the architecture behind it was assumed. Three bare DSP modules in
a test patch were read as the product's intended composition, and a two-option
design fork was built on that reading and sent to Jeff to decide between. One
question first — *what is a rack module supposed to be?* — would have replaced
all of it. Measure the artefact, but establish what the artefact is supposed to
be before concluding the code is wrong.
