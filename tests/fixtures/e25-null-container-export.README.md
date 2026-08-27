# `e25-null-container-export.synthedit` — BACKLOG E25's reproduction

**What it is:** `DefaultRack.synthedit` with **one attribute pair added**, and nothing
else. Diff it against the repo root's copy and you get exactly one line:

```
-  <param type="10" handle="1100194740" private="true" hostControl="49">
+  <param type="10" handle="1100194740" private="true" hostControl="49" module="1996595734" ignorePC="false">
```

**What it does:** loading it faults on demand, at
`KERN_INVALID_ADDRESS at 0x50` — the address E25 was filed on — in

```
CUG::GetPlug(int)
CContainer::getIgnoreProgramChange()
PatchParameter_base::ExportXml(TiXmlElement*, ExportFormatType)
CPatchManager::ExportXml(...)
CContainer::ExportXml(...)
TideApp::exportChunkXml()
TideApp::importChunkXml(...)
```

…against a build whose `SynthEditLib` predates `f85cf73`. Against `main` it loads
cleanly. **That pair is the point** — see "How to run it" below.

## Why those two attributes, and why neither alone is enough

`PatchParameter_base::ignoreProgramChange()` reads

```cpp
return m_ignoreProgramChange || (module() && module()->Container()->getIgnoreProgramChange());
```

so reaching the deref needs **both** halves:

| attribute | what it arranges | why it is needed |
|---|---|---|
| `ignorePC="false"` | `m_ignoreProgramChange == false` | it defaults to **`true`** (`PatchParameter.h:307`), and `true` short-circuits before `module()` is ever touched. Almost every parameter takes that exit, which is why this crash is rare. |
| `module="1996595734"` | `module()` is the **master container** | `CPatchManager::Import` reads the `module` attribute and `InitModulePointers` binds it out of `uniqueIds`, which every object joins via `uniqueIds[Handle()] = this` (`CUG.cpp:1216`). `1996595734` is this document's `<master_container … name="Main">`, and the master container is the one object whose `Container()` is **null** — `DocOb.cpp:40` says so in a comment, *"special case for 'Main' container"*. |

`getIgnoreProgramChange` is then **tail-called** with `this == nullptr`, and `CUG::GetPlug`'s
first instruction after the index guard reads the `Plugs` vector at `[this+0x50]`. Hence
`0x50` rather than `0x0`, which is the distinction
[`e25_null_container_probe.py`](../e25_null_container_probe.py) exists to make.

**The handle is document-specific.** If you re-save this fixture, re-read
`<master_container handle="…">` and put that number in the `module` attribute, or the
parameter binds to nothing (`if(moduleHandle)` … `uniqueIds.find` misses) and the fixture
silently stops reproducing.

## How to run it

It needs an A/B, because a fixture that only ever passes proves nothing:

```bash
# arm 1 -- the guard REVERTED (single variable)
git -C <SynthEditLib> worktree add --detach /tmp/wt-noguard origin/main
git -C /tmp/wt-noguard revert --no-commit f85cf73
cmake -S <TideSynth> -B /tmp/b-noguard -DCMAKE_BUILD_TYPE=Release \
      -DSYNTHEDITLIB_FOLDER_OVERRIDE=/tmp/wt-noguard
cmake --build /tmp/b-noguard --parallel 8 --target TIDE_Rack_STANDALONE

# arm 2 -- stock main
cmake -S <TideSynth> -B /tmp/b-main -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/b-main --parallel 8 --target TIDE_Rack_STANDALONE
```

Then, for each, copy this file over
`…/TIDE-Rack.app/Contents/Resources/DefaultRack.synthedit`, launch with an isolated
`HOME` and `-quiet`, and read the exit status.

**Revert the guard rather than checking out a pre-`f85cf73` `SynthEditLib`.** That was tried
first and does not build: TideSynth `main` calls `takeDivertedPrompts`, which E51 added to
`SynthEditLib` *after* the guard, so the old tree fails to compile for an unrelated reason.
Reverting the one commit keeps the guard as the only variable, which is what makes the
result mean anything.

## Measured 2026-08-27 (macos, scheduled run)

Same TideSynth `main` (`2612a2d`) in every cell; the guard is the only variable.

| | stock `DefaultRack.synthedit` | this fixture |
|---|---|---|
| **`f85cf73` reverted** | no crash, `default rack loaded, 25110 byte document` | **SIGSEGV, exit 139**, `KERN_INVALID_ADDRESS at 0x50` |
| **`main` (guard present)** | no crash, `25110 byte document` | no crash, `default rack loaded, 25147 byte document` |

The left column is the control: it is what says the fixture is the variable and not the
build. Crash reports, counted in `~/Library/Logs/DiagnosticReports`: **one per faulting run,
zero in the other three cells.**
