# Design notes

## RNBO as reference — what to take, what not to

[RNBO](https://rnbo.cycling74.com/) (Cycling '74) is the stated inspiration.
It is worth being precise about *which* part is being borrowed, because RNBO's
headline architecture is the opposite of TIDE's.

### Where the analogy breaks

RNBO **compiles**. A patch is sent to a code-generation server, which emits
portable C++, which a cloud compiler turns into a VST3/AU/wasm/Raspberry Pi
binary. The patch does not run interpreted at the end; generated code does.

TIDE **interprets**. The patch runs live inside the plugin, hosted by the
SynthEdit engine, edited in place. There is no codegen step, no server, no cloud
compiler. That is also precisely the capability SynthEdit sells and TIDE must
not include (see [carve-out.md](carve-out.md)).

So: do not copy RNBO's architecture.

### Where the analogy holds — and this is the valuable part

**1. A deliberately reduced patcher.** RNBO is not "Max in a plugin"; it is a
chosen subset of Max, and the subset was chosen so that everything in it can run
everywhere. TIDE should be the same relationship to SynthEdit: not a stripped
build of the app, but a subset chosen so that everything in it survives an iOS
AUv3 sandbox. When deciding whether a feature belongs, the question is "does
this work on iOS?" — not "can we hide the button?".

**2. The patch lives inside the host.** `rnbo~` puts a patcher inside a Max
session; TIDE puts the structure view inside a DAW plugin window. In both cases
the user is editing in the place where the sound is happening, rather than
editing in an app and exporting to somewhere else. This is the core product
idea and the reason TIDE is worth building.

**3. Self-containment as a hard rule, not an aspiration.** RNBO's generated
code has no external dependencies because it has to run on a Raspberry Pi and
in a browser. TIDE has the same discipline forced on it by the AUv3 sandbox.
Both benefit: no scanned folders, no global caches, no "where did my modules
go?" support burden.

**4. Parameters are the contract with the host.** RNBO surfaces patch
parameters to the DAW automatically via its export description. TIDE needs the
same: a patch's parameters should appear as host-automatable parameters without
the user building a panel first. This matters more in TIDE than in SynthEdit,
because TIDE has no panel view where a user could wire up controls manually.

## Cardinal as reference — the fixed-module-set precedent

[Cardinal](https://cardinal.kx.studio/) is a self-contained plugin build of
[VCV Rack](https://vcvrack.com/) — the same patcher, repackaged as ordinary
plugin software (AU, CLAP, LV2, VST2, VST3, plus a standalone and a web build)
instead of a standalone app that hosts scanned third-party modules.

It matters here for one specific reason: **Cardinal already made TIDE's
constraint 7 decision, independently, for the same underlying reason.** Its own
description is direct about it — *"Cardinal does not load any external modules,
everything is built-in."* All ~1,400 modules from ~84 authors are compiled into
one binary. No module folder, no scanning, no runtime loading of anyone else's
code. That is not a limitation Cardinal apologises for; it is the design,
because a plugin that scans and loads arbitrary third-party binaries is not
something every host and OS will accept — precisely the AUv3-sandbox argument
constraint 7 makes in [PLAN.md](../PLAN.md).

**What to take:**

1. **Proof the model works at scale.** Cardinal is not a toy demonstration of
   "fixed module set" — it ships ~1,400 modules this way and is in real use.
   TIDE's module count will be far smaller, but the architecture question ("can
   a compiled-in registry feel as open as a scanned folder?") already has a
   working answer to point at.
2. **The rack *is* the interface, and everything is visible at once.** Cardinal
   inherits VCV's layout rather than a paged or tabbed one — the same "whole
   patch visible, any output patches to any input" property PLAN.md's Eurorack
   section already takes from VCV Rack directly (see there for the full list;
   Cardinal is the proof that this layout survives being embedded in a plugin
   rather than staying a standalone app).
3. **"Plugin, not app" was the whole point of the exercise.** Cardinal exists
   because VCV Rack itself is a standalone application with its own audio/MIDI
   I/O, and Cardinal's contribution was removing exactly that — turning a rack
   into something a DAW can host and own I/O for. That is constraint 2, reached
   by the same route.

**What not to take:** Cardinal is a derivative of VCV Rack's GPL-licensed code
and module ecosystem. TIDE borrows the *architecture decision*, not any of
Cardinal's code or modules — see [carve-out.md](carve-out.md) and the licence
section of [PLAN.md](../PLAN.md) for why TIDE's own licensing stays independent
of that lineage.

## The one-view UX

The whole interface is the structure view plus a breadcrumb bar.

```
┌──────────────────────────────────────────────────────┐
│  TIDE  ›  Main  ›  Voice  ›  Filter          [ ? ]   │   breadcrumb bar
├────────────┬─────────────────────────────┬───────────┤
│  Modules   │                             │ Properties│
│  (pane)    │      structure view         │  (pane)   │
│  search…   │      — the only view —      │           │
│            │                             │           │
└────────────┴─────────────────────────────┴───────────┘
```

- **Breadcrumb** is the only navigation. Click a crumb to go up; double-click a
  container in the view to go down. No tabs, no window list, no back button.
- **Module browser** and **properties** are panes, not dialogs. They can be
  collapsed. They never float.
- **No modal dialogs** except genuinely modal things (a destructive confirm).
  The existing prototype already asserts-out `doDialogConnectUg`,
  `doDialogPatchManager` and `doDialogBuildCodeSkeleton` — those assertions are
  the right instinct, but they need to become "the feature does not exist"
  rather than "the feature crashes in debug".

## Sandbox rules — what gets removed

Driven by constraint 3 in [PLAN.md](../PLAN.md).

| Removed | Why |
|---|---|
| Sampling / wave-file modules | Need arbitrary filesystem paths |
| "Browse for file..." anywhere | Same |
| Scanned modules folder | `TideApp::InitInstance` currently sets `BundleInfo::semFolder = GetHomeDir() + "modules\\"` and calls `LoadOrScanModuleData()`. On iOS there is no such folder and no scanning. Modules must be enumerated from a compiled-in registry or the plugin bundle. See BACKLOG S1. |
| Audio/MIDI device selection | Host owns I/O (constraint 2) |
| Plugin export | Commercial boundary, and pointless in a plugin |
| External editor / help launching | `browseto.mm` / `openurl.mm` are unavailable or restricted in-sandbox |

## Known self-containment violations in the prototype

These are real, found by reading the current code. They are backlog items, not
speculation.

1. `TideApp::InitInstance` (`SE16/SynthEditSem/TideApp.cpp:109`) writes
   `BundleInfo::instance()->semFolder = GetHomeDir() + L"modules\\"` — a path
   outside the plugin bundle, and a Windows-only path separator.
2. The same function calls `LoadOrScanModuleData()`, which scans a folder and
   caches results. Both the scan and the cache violate constraint 4.
3. `EditorLib` links `browseto.mm` and `openurl.mm` on Apple platforms —
   these open external apps and will need to be excluded or stubbed for AUv3.

## Sources

- [RNBO — Cycling '74](https://rnbo.cycling74.com/)
- [RNBO Architecture](https://rnbo.cycling74.com/learn/architecture)
- [Exporting to the Audio Plugin Target](https://rnbo.cycling74.com/learn/audio-plugin-target-export-overview)
- [Cardinal](https://cardinal.kx.studio/)
- [VCV Rack](https://vcvrack.com/)
