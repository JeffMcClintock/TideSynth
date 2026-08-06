# Module enumeration without filesystem scanning

**BACKLOG item:** S1. **Status:** design note only — nothing implemented.
**Written by:** the linux weekly run, 2026-08-06.

Analysed against `SE16` at `8a7b1ef7b` and `SynthEditLib` at `53f0979`
(the working copies on the Linux box, at `~/SE/SE16` and `~/SE/SynthEditLib`).
Line numbers below are from those revisions.

---

## 1. What TIDE does today

`TideApp::InitInstance` (`SE16/SynthEditSem/TideApp.cpp:107`) does two relevant
things:

```cpp
BundleInfo::instance()->semFolder = GetHomeDir() + L"modules\\";   // :109
static bool oneTimeOnly = LoadOrScanModuleData();                  // :114
```

`ApplicationBase::LoadOrScanModuleData` (`SE16/SynthEdit2/Application.cpp:469`)
expands to this, in order:

| Step | Where | What it touches |
|---|---|---|
| `CopyInitialPrefabs()` | `Application.cpp:80` | **Reads** `GetHomeDir()/Resources/Prefabs`, **writes** a recursive copy into `getCommonDocumentFolder()/SynthEdit Projects/Prefabs`, **writes** `.resource_version`. |
| `LoadModuleData()` | `ModuleFactory_Editor.cpp:1178` | **Reads** `getSettingsFolder()/SynthEdit/Plugin-Cache-16.xml`. |
| `RefreshModuleData()` — only if the cache miss | `Application.cpp:485` | Three `ScanFolder` sweeps (below), each `std::filesystem::directory_iterator` + `dlopen`. |
| `StoreModuleData()` — at the end of a refresh | `ModuleFactory_Editor.cpp:1120` | `CreateFolderRecursive` then **writes** the XML cache back to `getSettingsFolder()`. |

The three scan roots (`Application.cpp:501`, `:510`, `:525`):

1. `getDefaultPath(L"syntheditprefab")` — the user's Prefabs folder.
2. `getSettingString(L"ModulePath")` — the user's 3rd-party SEM folder.
3. `BundleInfo::instance()->getSemFolder()` — factory SEMs.

`ScanFolder` (`ModuleFactory_Editor.cpp:1007`) recurses directories, and for each
`.sem`/`.gmpi` hit calls `ScanBundle` (`:751`) or `ScanStandaloneSem` (`:631`
Windows, `:967` Linux, `:960` no-op on macOS). Where a bundle ships a discrete
`Contents/Resources/*.xml`, the scan reads the XML and skips loading the binary;
otherwise it `PluginHolder::load()`s the module and asks its factory.

`getSettingsFolder()` (`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:166`)
resolves to `CSIDL_COMMON_APPDATA` on Windows, `~/Library/Application Support` on
macOS, `$XDG_DATA_HOME` (default `~/.local/share`) on Linux.

## 2. Which constraint each piece breaks

| Behaviour | Constraint broken | Note |
|---|---|---|
| Writing `Plugin-Cache-16.xml` under `getSettingsFolder()` | 4 (self-contained) | Also 3 — the container is not writable on iOS. |
| `CopyInitialPrefabs()` writing into the user's Documents | 3 and 4 | Copies files a plugin has no business copying. |
| Scanning `ModulePath` (arbitrary user folder) | 3 (sandbox) | Path is outside the bundle by definition. |
| `dlopen` of a discovered `.sem` | 3 (sandbox) | Code not shipped and signed inside the bundle. |
| Scanning the prefabs folder, then loading a prefab by path at drop time | 3 | See §4 — prefabs are *data*, so this one is solvable differently. |

## 3. The key structural fact: the scan is redundant for internal modules

This is the finding that makes S1 tractable.

`CModuleFactory`'s constructor (`SynthEditLib/UgDatabase.cpp:86`) already calls
`initialise_synthedit_modules()` (`:1054`), which force-links ~157 self-registering
translation units (the `INIT_STATIC_FILE` list, `:1064`–`:1243`). Each module
registers itself, with its **full XML description**, entirely in memory:

- `internalSdk::RegisterPlugin(create, xml)` — `UgDatabase.cpp:236`
- `CModuleFactory::RegisterPluginWithXml(subType, xml, create)` — `UgDatabase.cpp:266`
- `CModuleFactory::RegisterPluginsXml(const char* xml_data)` — `UgDatabase.cpp:494`

`Module_Info3_base::RegisterPluginConstructor` sets `m_module_dll_available = true`
(`Module_Info3_base.cpp:1298`), and the default is `true` anyway
(`module_info.h:216`).

Now follow the editor UI's data path:

```
ModuleBrowser::Init()                       ModuleBrowser.cpp:99
  → CSynthEditAppBase::ExportModules()      SynthEditAppBase.cpp:1329
    → ExportModuleNames()                   ModuleFactory_Editor.cpp:2193
      → CModuleFactory::Instance()->module_list      (filtered on isDllAvailable())
```

**The module browser never touches the filesystem.** It reads `module_list`, which
the factory constructor has already populated before `LoadOrScanModuleData()` is
reached. `ExportModules` builds its menu map lazily
(`if (m_menu_to_module_map.empty())`, `SynthEditAppBase.cpp:1331`), so it does not
depend on `ReloadMenu()` having been called either.

So for the ~157 built-in modules, scanning contributes **nothing**. The scan exists
solely to add (a) external `.sem`/`.gmpi` bundles and (b) prefab *files*.

There is even an existing build configuration for "no external modules":
`SE_EXTERNAL_SEM_SUPPORT` (`SynthEditLib/modules/shared/xplatform.h:34`), used by
the JUCE target and consulted at `UgDatabase.cpp:595`.

## 4. What iOS AUv3 actually forbids — code vs data

Two separate prohibitions get conflated, and separating them decides the design:

- **Writing outside the container** — hard rule, applies to everything. Kills the
  cache write, `CopyInitialPrefabs`, and any scan root outside the bundle.
- **Loading code not shipped and signed in the bundle** — kills `dlopen` of a
  `.sem`, and therefore kills third-party modules on iOS *permanently*, not just
  until we find a clever path.

But **reading inside its own bundle is allowed.** A sandboxed extension can
`directory_iterator` its own `Contents/`. That means:

- **Modules (code):** must be fixed at link time. No enumeration will help.
- **Prefabs (XML data):** *can* legitimately be enumerated from inside the bundle,
  or embedded as string resources. Either is sandbox-safe.

That split is why the recommendation below is not simply "compile in a list".

> To confirm on the Mac (M2): whether an AUv3 extension may load a `.sem` bundle
> embedded inside its own `.appex` and co-signed. Even if the OS permits it,
> App Store review of downloadable modules almost certainly does not, so TIDE
> should not depend on the answer. Recorded here so a future run does not
> re-derive it.

## 5. Options weighed

### Option A — compile-in static registry

Rely on what §3 shows already happens: the factory constructor registers every
built-in module. TIDE simply stops calling `LoadOrScanModuleData()`.

- **For:** zero filesystem access; already the mechanism SynthEdit uses to build
  exported plugins and JUCE targets, so it is proven; no new file format; no
  startup scan cost; the module set is reproducible and reviewable.
- **Against:** the module set is frozen at build time. No third-party modules,
  ever, on any platform — not just iOS.
- **Cost:** small. The work is *removal*, plus making `SE_EXTERNAL_SEM_SUPPORT`
  settable independently of `GMPI_IS_PLATFORM_JUCE`.

### Option B — enumerate from inside the plugin bundle

Keep `ScanFolder`, but point every root at `getSemFolder()`, which for a bundle
already resolves to `<bundle>/Contents/Plugins/` (`BundleInfo.cpp:699`), and drop
the cache entirely (scan every launch, nothing persisted).

- **For:** third-party modules stay possible on desktop; no new registry format.
- **Against:** still `dlopen`s, so it does not work on iOS — the target that
  drives the whole design. Scanning on every plugin instantiation is a real
  startup cost inside a DAW (a scan loads every module binary). Two code paths to
  maintain, and the iOS one is the untested one.
- **Cost:** medium, and it does not discharge constraint 3.

### Option C — hybrid: static registry for code, bundle enumeration for prefabs

Option A for modules. For prefabs, either embed the `.seprefab` XML as string
resources registered at static-init the same way module XML is, or enumerate
`<bundle>/Contents/Resources/Prefabs/` read-only and load by path from there.

- **For:** discharges constraint 3 completely; keeps the demo patches
  (`SE16/TideModules/{AR,Output,Sine}.seprefab`) working, which Option A alone
  does not; prefabs stay editable as files during development.
- **Against:** needs a small amount of new code on the prefab path
  (`CContainer::LoadPrefab`, `CContainer.cpp:2981`, currently does
  `doc.LoadFile(...)` on a resolved absolute path).
- **Cost:** Option A plus a bounded prefab change.

## 6. Recommendation

**Option C**, staged. Option A is the right answer for modules and is nearly free;
prefabs are the only part that needs design work, and they are data, so they have a
sandbox-legal answer.

Suggested staging — each stage leaves TIDE building:

1. **Stop scanning.** In `TideApp::InitInstance`, delete the `semFolder` assignment
   and the `LoadOrScanModuleData()` call. Per §3 the browser is unaffected. This is
   entirely within TIDE's own file — **no EditorLib or SynthEditLib change**, so it
   does not wait on the carve-out.
2. **Prove it.** Assert/log the size of `module_list` before and after, and confirm
   the module browser is populated and a module can be dropped into the view.
   (Needs P1/P2 — a machine that can run the prototype.)
3. **Make the absence structural.** *Required, not optional — see §7.1.* Introduce a
   `TIDE_NO_EXTERNAL_MODULES` (or decouple `SE_EXTERNAL_SEM_SUPPORT` from
   `GMPI_IS_PLATFORM_JUCE` at `xplatform.h:34`) so the scan/cache code is not merely
   un-called but not compiled. Otherwise it remains reachable via any other entry
   point and S2's audit can never come back clean. This touches shared code →
   carve-out order. Backlog **S1b**.
4. **Prefabs.** Embed the three TIDE prefabs, or ship them under
   `Contents/Resources/Prefabs/` and resolve `*P=` entries relative to
   `BundleInfo::getResourceFolder()` instead of the user's Documents folder.

**Trap for whoever implements stage 3:** the two arms of
`initialise_synthedit_modules` register *different* module sets. The
`GMPI_IS_PLATFORM_JUCE==1` arm (`UgDatabase.cpp:1063`–`1138`) and the `#else` arm
(`:1140`–`1155`) are not the same list — the non-JUCE arm registers
`ug_soundcard_in`, `ug_soundcard_out` and `ug_midi_out`, which TIDE must *not*
have (constraint 2, the DAW owns I/O), while the JUCE arm omits e.g.
`ug_filter_sv`. Flipping a build flag to get "no external SEMs" silently changes
which modules exist. TIDE most likely wants a third, explicit list rather than
either existing arm.

## 7. Open questions

1. ~~Does TIDE ever want third-party modules on desktop, or is a fixed module set
   the product?~~ **Answered by Jeff, 2026-08-06: fixed module set, compiled in.
   No third-party module loading on any platform.** This is now
   [PLAN.md](../PLAN.md) constraint 7 — a non-negotiable, not a preference. Do not
   re-open it. Consequences for this note: Option B (§5) is dead; stage 3 (§6) is
   definite rather than optional — the scan and cache code must be compiled *out*,
   not merely left un-called; and Option A's stated "against" (module set frozen at
   build time) is now an accepted property rather than a cost.
2. Which modules belong in TIDE's list? Neither existing arm is right (§6 trap).
   Depends on P2 (what the prototype actually shows) and constraint 2.

## 8. Adjacent findings — noted, not fixed

Found while tracing the above. Filed as backlog items rather than fixed here,
per the one-item rule.

1. **`TideApp.cpp:109` hard-codes a Windows separator:**
   `GetHomeDir() + L"modules\\"`. On macOS/Linux this names a directory whose name
   literally ends in a backslash. It is silent today only because `ScanFolder`
   swallows the failure through `std::error_code` (`ModuleFactory_Editor.cpp:1009`).
   Moot if stage 1 deletes the line, which is why it is listed here rather than
   fixed.

2. **TIDE can clobber the installed SynthEdit's module cache.** `TideApp` sets
   `BundleInfo::semFolder` but never sets `isSemFolderOverridden`
   (`BundleInfo.h:63`). `SemCacheName()` (`ModuleFactory_Editor.cpp:174`) only
   appends its `-override-<hash>` suffix when that flag is set, so TIDE reads and
   **writes** the same `Plugin-Cache-16.xml` as the desktop SynthEdit application.
   A TIDE instance running in a DAW can overwrite the desktop app's module cache
   with TIDE's different, smaller module set. This is a live Windows/macOS hazard
   today, independent of iOS, and it is a second reason stage 1 is worth doing
   early. → **BACKLOG S4**.

3. **`getFolderInfo` is UB in TIDE.** `TideApp::InitInstance` (`TideApp.cpp:107`)
   overrides the base and never calls `CSynthEditAppBase::InitInstance`, so
   `refreshFolderLocations()` (`SynthEditAppBase.cpp:96` → `Application.cpp:145`)
   never runs and `m_folder_settings` stays empty. `getFolderInfo`
   (`Application.cpp:167`) falls through its loop and then evaluates
   `m_folder_settings[0]->current_folder` on an empty vector. Reachable from
   `CSynthEditAppBase::ShortenFilename` (`SynthEditAppBase.cpp:238`).
   `ResolveFilename` is *not* affected — it uses `getDefaultPath`, which has a safe
   fallback (`Application.cpp:200`). → **BACKLOG S5**.

## 9. How to verify the recommendation cheaply

Before implementing anything, on a machine that can build and run TIDE (P1/P2):

1. Rename or delete `<settings>/SynthEdit/Plugin-Cache-16.xml`, and point
   `ModulePath` at an empty directory.
2. Launch TIDE and open the module browser.

If the browser is still fully populated, §3 is confirmed empirically and stage 1
is a safe deletion. If it is empty or short, something outside `module_list` is
feeding it and this note needs revisiting — say so in the journal.

---

# Addendum — macOS run, 2026-08-06

The macOS and Linux runs both fired on 2026-08-06 (both boxes were set up that
day, so the Fri/Sat/Sun stagger in [agent-setup.md](agent-setup.md) had not
taken effect yet) and both took S1. Rather than land a second competing note,
the macOS findings are folded in here.

Verified against `SynthEdit` @ `e6b50de2b` and `SynthEditLib` @ current `master`
on the macOS box, at `~/Documents/GitHub/SynthEdit` and
`~/Documents/GitHub/SynthEditLib` — **siblings, not nested**, so `SE16/…` in the
sections above maps to `~/Documents/GitHub/SynthEdit/…` here. Line numbers agree
with the Linux revisions everywhere they were cross-checked.

Everything above stands. Four things to add, one of which changes what §9's
verification will actually show.

## A1. §9 will not give a clean yes/no — expect "populated but wrong"

§3 is right that `module_list` is pre-populated and the browser never reads the
filesystem. But the ~157 `INIT_STATIC_FILE` entries are **three** regions, not
two, and which modules TIDE gets depends on which region they sit in:

| Region | Lines | Count | In a non-JUCE TIDE build? |
|---|---|---|---|
| `#if GMPI_IS_PLATFORM_JUCE==1` | 1063–1139 | ~70 | **No** |
| `#else` | 1140–1155 | 14 | Yes |
| unconditional tail | 1156–1243 | ~66 | Yes |

Spot-checked placements:

| Module | Line | Region |
|---|---|---|
| `ug_adsr` | 1168 | unconditional |
| `ug_oscillator2` | 1201 | unconditional |
| `ug_vca` | 1215 | unconditional |
| `ug_filter_biquad` | 1144 | `#else` only |
| `ug_filter_sv` | 1145 | `#else` only |
| `ADSR` | 1064 | JUCE only |
| `Converters` | 1070 | JUCE only |
| `OscillatorNaive` | 1082 | JUCE only |
| `Slider` | 1089 | JUCE only |

So after stage 1 the browser will be **neither empty nor correct**. Predict:

- The legacy `ug_*` palette present — enough for v0.1's oscillator → envelope →
  output, which is genuinely good news for stage 1.
- The modern SEM-derived modules (`EnvelopeAdsr`, `Filters`, `OscillatorHD`,
  `ControlsXp`, `Converters` — the 54 bundles built into
  `build/modules/Debug/` on this Mac) **absent**, because they reach the factory
  only via the scan or the JUCE arm.
- `ug_soundcard_in`, `ug_soundcard_out`, `ug_midi_out` **present**, which
  constraint 2 forbids.

This does not invalidate stage 1 — it sharpens it. Whoever runs §9 should
record *which* modules appear, not just whether the list is non-empty, and
should not read "soundcard modules are showing" as a failure of the deletion.

## A2. The "third, explicit list" §6 asks for already has a hook

§6's trap says TIDE probably needs its own module list rather than either
existing arm. That mechanism exists — `UgDatabase.cpp:1239`:

```cpp
// You can include extra plugin-specific modules by placing this define in projucer 'Extra Preprocessor Definitions'
// e.g. SE_EXTRA_STATIC_FILE_CPP="../PROJECT_NAME/Resources/module_static_link.cpp"
#ifdef SE_EXTRA_STATIC_FILE_CPP
#include SE_EXTRA_STATIC_FILE_CPP
#endif
```

plus an `initialise_synthedit_extra_modules()` hook at `:1243` (the editor's
implementation is `ModuleFactory_Editor.cpp:170`). A TIDE list can therefore
live in the TIDE target as its own `.cpp`, with **no edit to the shared
`initialise_synthedit_modules`**. That matters for carve-out ordering: stage 3
gets smaller than §6 assumes.

## A3. Getting the modern SEM modules in — the metadata half

A1 leaves a gap: the SEM-derived modules TIDE actually wants are not in any
non-JUCE arm. Two pieces are needed, and both already exist.

**Code.** Each module needs `SE_DECLARE_INIT_STATIC_FILE`
(`SynthEditLib/module_register.h:30`) and an `INIT_STATIC_FILE` entry — via the
A2 hook. Each must also build as a static library: today both
`build_sem_plugin` and `build_gmpi_plugin` in
`SynthEditLib/modules/plugin_helper.cmake` emit `add_library(… MODULE)` (`:70`
and `:186`), a loadable bundle, so a static variant of those macros is new work.
The JUCE arm links ~70 of them this way already, so the scale is proven.

**Metadata.** `CModuleFactory::RegisterExternalPluginsXmlOnce`
(`UgDatabase.cpp:526`) reads `database.se.xml` from bundle resources (`:543`)
and feeds it to `RegisterPluginsXml` (`:563`). Exported plugins already ship it
— a live example is checked in at `SE16/SE_IOS_APP/TIDE/Resources/database.se.xml`
(16,458 bytes). The switch is the `imbeddedFilename` attribute
(`UgDatabase.cpp:587`): absent → `Module_Info3_internal`, compiled-in; present →
the `dlopen` path guarded by `SE_EXTERNAL_SEM_SUPPORT` (`:595`).

And the source content is already in the tree: every module ships its own
descriptor beside its source — e.g.
`SynthEditLib/modules/OscillatorNaive/OscillatorNaive.xml` is a complete
`<PluginList>` with all six pins. **So `database.se.xml` can be generated at
build time by concatenating those files with the `imbeddedFilename` attribute
omitted** — no editor, no scan, no binary loaded. Generating it from the same
list that drives the A2 static-link file means code and metadata cannot drift.

## A4. `-DSE_EXTERNAL_SEM_SUPPORT=0` will not work as written

§5 and §6 both want `SE_EXTERNAL_SEM_SUPPORT` settable independently of
`GMPI_IS_PLATFORM_JUCE`. Note that a CMake `-D` on the TIDE target does **not**
achieve this today: `xplatform.h:34` defines the macro unconditionally, so a
command-line definition collides.

The header already solves this one symbol earlier — `GMPI_IS_PLATFORM_JUCE` is
wrapped in `#if !defined(…)` at `:25` precisely so CMake can set it. The minimal
shared-header edit is to give `SE_EXTERNAL_SEM_SUPPORT` the same guard, which
changes no existing target's value. The fuller fix is the feature test the
header's own `todo` at `:10` already asks for ("BETTER TO SUPPORT FEATURES, e.g.
'SEM support' NOT 'JUCE' or 'VST3'").

## A5. Partial answer to §4's question for the Mac

§4 asks whether an AUv3 extension may load a `.sem` embedded inside its own
`.appex` and co-signed. **Not answered** — that needs M2 and a real device, and
the note is right that App Store review makes the answer unsafe to depend on
either way.

What *can* be reported now is that nothing in the tree currently exercises it,
and the thing that looks like it does is a decoy:

`SE16/SE_IOS_APP/TIDE/Plugins/` holds six checked-in `.sem` bundles (ControlsXp,
Converters, EnveloperAdsr, Oscillator, PatchMemory, SubControlsXp). `file` on
each binary reports `Mach-O 64-bit bundle x86_64` — not arm64, so none can load
on an iOS device or simulator. They are macOS bundle layout
(`Contents/MacOS/…`), and the Xcode Run Script that installs them
(`SE_IOS_APP.xcodeproj/project.pbxproj:2064`) copies the folder to
`${BUILT_PRODUCTS_DIR}/${FULL_PRODUCT_NAME}/Contents/`, a macOS-only
destination. **It is a macOS-era artifact, not an iOS module story.** Filed as
BACKLOG **S6** so a future run does not sink time into making it load.

## A6. One more scan root to remove, outside `TideApp`

§6 stage 1 deletes the `semFolder` assignment in `TideApp.cpp:109`. Worth
knowing that the default it overrides is *fine* — `BundleInfo.cpp:699` sets
`semFolder` to `<bundle>/Contents/PlugIns/`, inside the bundle.

But `BundleInfo.cpp:712` then adds a dev-tree fallback: if that folder does not
exist, it walks **parent directories** looking for a sibling
`SynthEdit2/PlugIns`. Convenient in a build tree, and exactly the kind of
reach-outside-the-bundle behaviour that must not survive into a shipping AUv3.
It is not in `TideApp`, so stage 1 will not remove it and it is easy to miss.
One for S2's audit.
