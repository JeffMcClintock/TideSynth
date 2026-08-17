# E2a — the three rack prefabs: implementation plan

Written 2026-08-17 (windows, interactive session, Jeff directing). Everything
cited here was read or measured from the working trees that day — `SynthEditLib`
at `f0e3c92`, `SE16` at `28907334e` — not recalled from documentation.

**The item:** BACKLOG **E2a** — oscillator, envelope, output, each a rack
prefab, the minimal set V1's acceptance test needs. This doc is the plan the
row points at; it moves no file and ships no prefab.

A companion question Jeff asked the same day — can *users* author and save
their own prefabs, under AUv3 — is answered in [§7](#7-user-prefabs--can-users-save-their-own)
and filed as **E4**.

---

## 1. What a rack prefab is, mechanically

A rack module is a **Container set Visible**, with patch points on its panel
and the real DSP module inside:

- "Visible" is the Container's third pin — `GetPlug(2)`, asserted by name at
  `SynthEditLib/CContainer.cpp:3301` (`ug_container.cpp:226` renamed it from
  "Controls on Parent"). Visible-on is what makes the Container render as a
  panel in the parent view — the rack.
- The panel surface carries `SE Patch Point in` / `SE Patch Point out`
  modules, which is how cables reach the module from outside without opening
  it. `Sine.seprefab` already uses exactly this idiom (patch points +
  `SE Rectangle XP` + `SE Text Entry4` for the faceplate), so the visual
  pattern is established, not invented here.
- The DSP module inside can be `CF_STRUCTURE_VIEW`-flagged (Sound Out is,
  `ug_soundcard_out.cpp:11`) — that flag governs where the *module* shows,
  and inside a closed prefab nothing shows but the Container's panel.

## 2. The format is forced: modern `.synthedit` XML, not `.seprefab`

Not a decision to make — the code has made it:

- `CContainer::LoadPrefab` (`CContainer.cpp:2987`) accepts `.synthedit` /
  `.syntheditprefab` and parses them with tinyxml2 (`:3018`).
- For legacy `.seprefab`/`.se1` it does not parse at all: it calls
  `legacyExternalApp::create()` and **shells out to an installed SynthEdit
  1.5** to upgrade the file (`:2996`–`:3008`), and that branch is
  `#ifdef _WIN32`. A sandboxed plugin cannot launch an external editor, and
  two platforms never could. Dead end.

Consequence: the three prototype files in `SE16/TideModules/`
(`AR/Output/Sine.seprefab`, Oct 2024) are **references, not inputs**. They get
replaced by newly authored `.synthedit` files, not upgraded in place.

And one of them is not what its name says: decoding `Output.seprefab`
(UTF-16LE strings) shows a Container holding `SE Patch Point in` and an
`IO Mod` — **no Sound Out**. `TIDE.se1` is where the Sound Out lives. So the
Output prefab is authored from scratch; there is nothing to port.

## 3. The three prefabs, in build order

**Output first.** It is the only one with no dependency on S8's oscillator
problem, and it is the one every E1 test case needs (nothing renders without
an egress).

| Prefab | Inside | Blockers |
|---|---|---|
| **Output** | `SE Patch Point in` (L, R) → **Sound Out** (its input pin is `IO_AUTODUPLICATE`, `ug_soundcard_out.cpp:20`, so two connections make two channels) | none |
| **Envelope** | patch points → the AR/envelope primitive (`ug_envelope_base` family is registered; `AR.seprefab` proves the shape) | none known |
| **Oscillator** | patch points → an oscillator primitive | **S8's finding stands: `OscillatorNaive` has zero symbols in the Release binary.** Verify what oscillator primitives are actually registered before authoring; if none is usable, this prefab waits on the S8 slice that registers one |

Authoring: build each in SynthEdit 1.6 (or via the SynthEditCL MCP tools on
the Windows box) and save in the modern format. Keep the faceplate idiom from
`Sine.seprefab`.

## 4. Where the sources live

`SE16/TideModules/` — already the demo-patch home, already ALLOWED for agent
work under STEP 5. New files land beside the old ones; the `.seprefab` files
stay (journal entries reference them) with this doc marking them superseded.

One forward-looking note, not today's problem: `TideModules` is in the
**private** repo. C7's clean-clone test is about code and will pass without
prefabs, but the day TIDE builds outside SE16 entirely, the prefab sources
must move somewhere public with it.

## 5. How they ship — stage 4 of module-enumeration §6, the last unbuilt stage

Today they ship nowhere: S1a deleted the module scan from
`TideApp::InitInstance` (`TideApp.cpp:312`), and the prefab discovery went
with it — `ModuleFactory()->PrefabFileNames` is populated only by the scan
(`ModuleFactory_Editor.cpp:1027`) or its cache (`:1223`), so in TIDE it is
empty and the browser's Prefabs group (`:2101`–`:2134`) is silent.

Three pieces, each with a working precedent:

1. **Stage into the bundle.** POST_BUILD copy of `TideModules/*.synthedit`
   into `Contents/Resources/Prefabs/`. Precedent: `ControlsXp.xml`
   (`SynthEditSem/CMakeLists.txt:134`, P6-approved home). That block is
   `if(APPLE)` — **the Windows staging equivalent is outstanding, same gap
   U2e's row already flags**; prefabs inherit it and should be fixed with it.
2. **Seed the browser.** At startup, enumerate
   `BundleInfo::getResourceFolder()/Prefabs/` read-only and push names into
   `PrefabFileNames`. Reading inside the bundle is sandbox-legal
   ([module-enumeration §4](module-enumeration.md)) — prefabs are data, and
   the code/data split there is the whole reason this is allowed while module
   scanning is not.
3. **Resolve drops against the bundle.** A browser drop arrives as
   `"*P=<relpath>"` (consumed at `CContainer.cpp:2866`, `MfcDocPresenter.cpp:90`)
   and `LoadPrefab` resolves it via
   `GetApp()->ResolveFilename(filename, L"syntheditprefab")` (`:2989`) — the
   user's Documents folder. TIDE needs that resolve to hit
   `getResourceFolder()/Prefabs/` instead. Small; the alternative (a
   `LoadPrefab` sibling taking XML as a string via `doc.Parse`, fed from
   `BundleInfo::getResource`) avoids paths entirely and is worth choosing if
   the resolve change touches shared code awkwardly.

Enumerate-the-bundle is the recommended shape (files stay editable during
development, no embedding step); the string route is the fallback.

## 6. Verification

- **E1 harness case per prefab** — the standing rule from E2a's row. For
  Output: a test-tone patch terminating in the prefab, rendered headlessly by
  SynthEditCL, null-tested against a golden WAV. Audio leaving the prefab *is*
  the acceptance check.
- Browser check: Prefabs group populated in TIDE with the scan still absent.
- Drop check: prefab lands in the rack as a Visible container with its patch
  points cable-able, per constraint 1.

## 7. User prefabs — can users save their own?

Asked by Jeff 2026-08-17: can a user author a prefab and save it somewhere,
under AUv3? **Yes — with one ruling needed, and it is about desktop, not
iOS.** Filed as BACKLOG **E4** (NEEDS-JEFF).

The reason it is possible at all is the same code/data split that makes §5
legal: a prefab is XML. Constraint 3 forbids loading *code* from outside the
bundle and writing outside the sandbox; constraint 4's own wording permits
"the plugin's own sandboxed container". User prefabs are user *documents* in
that container — not the scattered caches the constraint exists to ban.

Three tiers, independent of each other:

1. **In-project (free once V1 lands).** A user's Container already lives in
   the document and survives host save/reload with it. Copy/paste within and
   between projects costs nothing new and writes nothing outside DAW state.
   This much is not even a feature; it falls out of V1.
2. **A per-device library.** On iOS/AUv3 the extension may write inside its
   own container (`NSHomeDirectory()` of the appex, or an app-group container
   if a companion app ever exists) — sandbox-legal, App-Store-ordinary; it is
   where every AUv3 synth keeps user presets. Saved prefab = the Container
   subtree serialised to the same `.synthedit` XML §2 uses; the library
   folder is enumerated read-mostly exactly like §5's bundle folder, just a
   second root. **The ruling E4 needs is the desktop half:** VST3 has no
   OS-enforced container, and the natural location (`%APPDATA%`,
   `~/Library/Application Support`) is what constraint 4 names as banned.
   Either Jeff blesses one decided folder as "the plugin's container by
   convention" on desktop, or the desktop library lives inside plugin state
   only (tier 1) and the filesystem library is iOS-only. That is a product
   call, not a technical one.
3. **Sharing.** Export/import through the system document picker
   (user-initiated, security-scoped URLs) — the only sanctioned route to
   user-chosen locations on iOS, and inherently user-triggered so it cannot
   violate the sandbox. It is a modal system sheet, which brushes against
   constraint 5; being OS-supplied and user-initiated it is closer to the
   file-open sheet every iOS app has than to the dialogs the constraint bans,
   but that reading is Jeff's to confirm, not this doc's to assume.

Explicitly *not* proposed: abusing the AUv3 user-preset API
(`AUAudioUnit.userPresets`) as prefab storage — its granularity is
whole-plugin state, and a prefab is a rack module, not a preset.

None of this is v0.1. The prerequisite chain is honest: tier 1 needs V1;
tiers 2–3 need E2's naming/IO conventions (a user-authored Container is the
same object as a shipped one, so the conventions must exist first) and the
E4 ruling.

## 8. Traps, named so nobody re-derives them

- **S8 must not delete the three "forbidden" modules.** Its row read
  `ug_soundcard_in`/`ug_soundcard_out`/`ug_midi_out` as constraint-2
  violations on the strength of standalone-flavoured help text. Measured
  2026-08-17: all three are `RegisterIoModule` seams
  (`ug_soundcard_out.cpp:69`, `ug_soundcard_in.cpp:63`, `ug_midi_out.cpp:50`);
  Sound Out implements `ISpecialIoModuleAudioOut` and `SeAudioMaster` hands
  it the **host's** output buffers (`SeAudioMaster.cpp:560-562`, `:640-642`)
  — the identical mechanism S12 used for MIDI input. `TIDE.se1` terminates in
  Sound Out and the Output prefab will too. **Deleting them silences the
  product.** S8 is a relabelling job (name, group, help text) plus the module
  list; the row is corrected accordingly.
- **The legacy `.seprefab` upgrade path is not a porting tool** — §2. It
  launches SynthEdit 1.5, Windows-only.
- **`ResolveFilename` defaults to the user's Documents folder** — §5.3. A
  prefab that loads in a dev tree because the file also sits in Documents is
  a false pass; test with the Documents copy absent.
- **The `if(APPLE)` staging gap** — §5.1. A prefab that ships on mac and
  silently not on Windows would be U2e's gap repeated; fix both together.
