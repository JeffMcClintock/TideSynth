# U2e — the pin-delivery trace, completed

**Run:** 2026-08-17, macos, scheduled (unattended).
**Question this answers,** posed verbatim by the 2026-08-16 U2e run: *"trace how
`ModuleView`'s Sdk3 path delivers initial pin values (the fake plugs
`Ctl_Combo::Export` writes) and why the handler pass is skipped."*

**Answer in one sentence:** the handler pass is not skipped — **it is never
requested**, because TIDE's `Module_Info` for `SE List Entry` has **zero GUI pin
descriptions**, and every initial-value path in `ViewBase::ConnectModules`
iterates that list, so `setPin`/`notifyPin` are called zero times for the module.

The missing pin descriptions are not a bug in the view layer at all. They are a
direct, predictable consequence of **S1a removing the module scan** (PLAN
constraints 4 and 7): the classic SDK3 controls keep their pin metadata in a
separate `.xml` file that only the disk scan reads.

---

## 1. Three registration families, and only one of them loses its pins

`Module_Info::gui_plugs` — the map every default-delivery path reads — is
populated by exactly two mechanisms, and TIDE's classic controls use neither.

| Family | Registration | Where pins come from | Populated in TIDE? |
|---|---|---|---|
| Classic internal DSP | `REGISTER_MODULE_1` + `LIST_PIN2` (e.g. [`ug_oscillator_pd.cpp:18,45-47`](https://github.com/JeffMcClintock/SynthEditLib/blob/main/ug_oscillator_pd.cpp)) | in C++, via `CModuleFactory::RegisterModule(mod_func, pin_func)` → `UgDatabase.cpp:347` `gui_plugs.insert(...)` | **yes** |
| Modern GMPI | `gmpi::Register<T>::withXml(R"XML(...)")` (e.g. `modules/Controls/PlainImageGui.cpp:182-191`) | embedded XML string → `CModuleFactory::RegisterPluginWithXml` (`UgDatabase.cpp:267`) → `mi3->ScanXml(...)` (`UgDatabase.cpp:287`) | **yes** |
| SDK3 "official module" | `GMPI_REGISTER_GUI` / `REGISTER_PLUGIN2` — **id only** (`mp_sdk_gui.h:12`, `mp_sdk_audio.h:26`) | the module bundle's `Contents/Resources/<Project>.xml`, read **only by the module scan** | **NO** |

`ListEntryGui.cpp:21` is family 3:

```cpp
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, ListEntryGui, L"SE List Entry");
```

which expands to `CModuleFactory::RegisterPlugin(subType, uniqueId, create)` —
`UgDatabase.cpp:242`:

```cpp
auto mi3 = FindOrCreateModuleInfo3(uniqueId);
return mi3->RegisterPluginConstructor( subType, create );   // no ScanXml
```

A constructor, and nothing else. `ScanXml` — the only thing that would fill
`gui_plugs` for this family — is never reached. Its pin metadata lives in
`SynthEditLib/modules/ControlsXp/ControlsXp.xml:262-283`, which
`modules/plugin_helper.cmake:236` copies into the built `.sem` bundle's
`Contents/Resources/` for the **scanning** editor to pick up. TIDE compiles the
`.cpp` straight into the plugin and never scans, so that file is never read.

The same distinction is already documented in the tree, from the other end —
`ModuleFactory_Editor.cpp:2054-2058`, on a different symptom of the same root:

> *"Internal modules (SE Patch Point in/out) hide the bug — they re-register
> from their literal XML on every startup … Only an external `.sem`/`.gmpi`
> module can hit it."*

---

## 2. Verification artifact — the shipping binary, A/B with positive controls

Measured on `build/SynthEditSem/Release/TIDE_VST3.vst3/Contents/MacOS/TIDE_VST3`,
built from `master` (`SynthEdit`) on 2026-08-17, `** BUILD SUCCEEDED **`:

```
                       id-string   embedded <Plugin id="…"> XML
SE List Entry               2                 0        <- family 3
SE Text Entry               2                 0        <- family 3
SE Background Image         4                 2        <- family 2 (control)
SE Patch Point in           2                 2        <- family 2 (control)
PatchAutomator              4                 2        <- family 2 (control)

List Entry pin metadata strings from ControlsXp.xml:
  "LED Stack"        0
  "Up/Down Select"   0
  "Appearance"       0
```

Reproduce with:

```bash
BIN=~/Documents/GitHub/SynthEdit/build/SynthEditSem/Release/TIDE_VST3.vst3/Contents/MacOS/TIDE_VST3
strings -a "$BIN" | grep -c 'SE List Entry'                 # 2  -> id IS registered
strings -a "$BIN" | grep -c '<Plugin id="SE List Entry"'    # 0  -> pins are NOT
strings -a "$BIN" | grep -c '<Plugin id="SE Background Image"'  # 2 -> positive control
```

The point of the positive controls is that they rule out "`strings` just can't
see the XML": the same command finds the family-2 modules' full pin metadata in
the same binary. `SE List Entry` is a **registered module id with no pin
descriptions attached** — which is exactly the state that produces the observed
crash.

Only **29** distinct `<Plugin id="…">` XML blocks are embedded in the whole TIDE
binary. Every family-3 module TIDE ever adds to its list will land in this same
state; this is a wall in front of TIDE's fixed module set (constraint 7), not a
one-off defect in List Entry.

---

## 3. The chain, end to end, at file:line

Everything below is on the current `main`/`master` of each repo.

1. `MfcDocPresenter::RefreshView` (`SynthEditLib/MfcDocPresenter.cpp:389`)
   exports the document and calls `view->Refresh(&gui_json, guiObjectMap_)`
   (`:427`).
2. `ViewBase::Refresh` (`modules/se_sdk3_hosting/ViewBase.cpp:2058`) runs, in
   order: `BuildModules` → `ConnectModules` → `InitializeGuiObjects` →
   `measure` → `arrange` (`:2081-2096`).
3. `ViewBase::ConnectModules` (`:593`) looks up the module description at `:682`
   `moduleInfo = CModuleFactory::Instance()->GetById(...)` — **non-null, but
   with an empty `gui_plugs`**.
4. There are **two** paths that deliver an initial GUI pin value, and **both**
   gate on `moduleInfo->gui_plugs`:
   - the JSON `"default"` path, `:711` `for(auto& pd : moduleInfo->gui_plugs)` →
     `:742` `wrapper->setPin(...)` — no match is ever found, so no call;
   - the module-info defaults path, `:778`
     `for(auto& plugInfoPair : moduleInfo->gui_plugs)` → `:799`
     `wrapper->setPin(...)` — the loop body never executes.

   With `gui_plugs` empty, **zero** `setPin` calls are made for this module.
5. `ModuleView::setPin` (`ModuleView.cpp:1374`) is therefore never entered, so
   `pluginParameters2B->notifyPin(pinIndex, voice)` (`:1404`) is never called.
6. `GuiPinOwner::notifyPin` (`modules/se_sdk3/mp_sdk_gui.cpp:125`) → `doNotify`
   (`:130`) is the only thing that invokes a registered pin handler. Never
   called ⇒ no handler runs.
7. `ListEntryGui::onSetAppearance` (`modules/ControlsXp/ListEntryGui.cpp:49`) is
   the **only** place `widgets.push_back` happens (`:88,95,112,122,126,131,141,
   151,157`). It never runs, so `widgets` stays empty.
8. `ClassicControlGuiBase::initialize`, `ListEntryGui::measure` and
   `ListEntryGui::arrange` then index `widgets[…]` — the three SIGSEGV stacks the
   2026-08-16 run recorded, in the exact order `Refresh` calls them.

### What this confirms and what it corrects

- **Confirms** the previous run's defaults-based argument. It reasoned "had the
  handlers fired even once with default values, `ACM_PLAIN` would have built a
  ListWidget; the vector is empty, so the handler pass never happens." That is
  right, and now has a mechanism.
- **Corrects** the framing in the U2e row. The row suspected
  "skin-bitmap/font loading (`BitmapWidget::Load` → ImageCache →
  GmpiResourceManager) failing in the VST3". That is **not** the gate — nothing
  in the widget-construction path is ever reached to fail. The pin IDs are also
  fine, and were worth ruling out: `ListEntryGui.cpp:32-40` seeds
  `initializePin(10, pinValueIn, …)` and lets the rest auto-increment to 11-16,
  matching `ControlsXp.xml:271-281` exactly.
- **Explains** why U2e finding 1 (resource-folder seeding,
  [SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)) was
  measured as a real prerequisite but proven insufficient alone. It is a
  prerequisite for step 7 succeeding; it cannot help when step 4 never fires.

---

## 4. The fix, and why this run did not write it

The fix is "make `Module_Info` for TIDE's family-3 modules carry their pin
descriptions". Three ways, none free:

**(a) Embed the XML in the module source**, the shape `PlainImageGui.cpp:182`
already uses — swap `GMPI_REGISTER_GUI` for a `withXml` registration in
`ListEntryGui.cpp` / `TextEntryGui.cpp`. One place, matches existing design
intent in the same repo. **Risk:** ControlsXp also ships as a scanned `.sem` for
full SynthEdit, so the same `Module_Info` would be populated twice — once at
static-init from the embedded string, once from the scanned
`Contents/Resources/ControlsXp.xml`. `Module_Info::ScanXml`
(`Module_Info3_base.cpp:213`) does **not** call `ClearPlugs()` first (it clears
only `pinXmlDiagnostics_`), and `gui_plugs` is a `std::map` whose `insert` drops
— and leaks — the second registration. Not a crash, but not clean, and it is a
change to the scanning editor's behaviour that no macOS box can test.

**(b) Register the XML from TIDE.** `TideApp::InitInstance` calls
`RegisterPluginXml(...)` (`UgDatabase.cpp:212`) with the `<Plugin>` blocks for
the family-3 modules TIDE ships. Confined to `SE16/SynthEditSem/` (ALLOWED),
**cannot** regress full SynthEdit, and is constraint-7-shaped: TIDE's fixed
module set gets declared in TIDE. Cost: the XML text is duplicated from
`ControlsXp.xml`, so it can drift.

**(c) (b), without the duplication.** A build step embeds
`ControlsXp.xml` as a string TIDE registers at startup. Same isolation as (b),
no drift, at the cost of new CMake machinery.

**Recommendation: (c), falling back to (b).** Both stay inside ALLOWED paths and
neither can affect the scanning editor. (a) is the tidiest-looking and the only
one that changes shared, commercially-shipped behaviour — Jeff's call, not a
scheduled run's.

**Why no code landed this run.** U2e's acceptance is *"a placed List Entry draws
as a usable combo box in a host on both platforms"* — a GUI observable. This run
was unattended and cannot open REAPER, so it could not have told a working fix
from a plausible one. Two further reasons to stop at the trace:

- the fix stacks on [SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
  (resource-folder seeding), which is open and unmerged — step 7 needs it;
- re-adding the ControlsXp TUs to `SynthEditSem/CMakeLists.txt` (the repro the
  U2e row specifies) reinstates three known SIGSEGV sites, and shipping that
  without being able to observe the result is exactly the "plausible-looking
  wrong PR" the run prompt warns about.

**What the next session should do,** now mechanical: apply (c) or (b), re-add
the ControlsXp TUs + deps per the U2d comment in
`SE16/SynthEditSem/CMakeLists.txt:60-79`, rebuild, and check the same binary
artifact flips (`<Plugin id="SE List Entry"` count 0 → 1) before opening REAPER.
If the combo still does not draw with pins delivered, the skin-bitmap suspicion
in the U2e row becomes the live hypothesis again — but it is second in line, not
first.
