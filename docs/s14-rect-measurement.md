# S14 — why placed modules render nothing: the measurement

Measured 2026-08-18 (macos, scheduled run, no GUI available). This is the
"cheap first measurement" the S14 row asked for, and it answers the row's
question. **No code was changed** — every fix site is in GATED `SynthEditLib`,
filed as **S15**.

## The question S14 asked

> Unknown and worth measuring first: whether the zero rect comes from the
> placement path not assigning one, or from rack mode expecting a *panel* rect
> rather than `structRect`.

**Answer: both halves describe the same bug, and they are the same event.**
Rack mode addresses the *panel* rect; a plain DSP module has no panel rect;
so the assignment lands on an empty base-class no-op and is silently
discarded. `structRect` — the only rect such a module persists — is never
written, so it stays `0,0,0,0`.

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

## Why TIDE cannot fix this on its own side

Every one of the four sites is in **`SynthEditLib`**, which is GATED, and S14
is not a carve-out stage. A17's 2026-08-18 ruling permits a run to repair a
**build break** in a GATED path; this is a functional defect, not a build
break, so that ruling does not reach it. `TideApp.cpp`'s only involvement is
setting `Document()->rackMode = true`, which is correct and is not the bug.

Filed as **S15**, with the two candidate fixes and the reason it wants a
ruling rather than a guess.

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

The **acceptance** in S14 ("a module placed in the rack is visible, and still
visible after reload") cannot be observed without a GUI, so confirming any fix
needs an interactive session. But the *pre*-check is headless and worth doing
first: after a candidate fix, re-export the chunk and assert the modules'
rects are non-zero and distinct — that separates "geometry is now stored" from
"geometry is stored and the rack draws it", which are different failures.
