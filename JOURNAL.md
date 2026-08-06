# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

Template:

```
## YYYY-MM-DD — <machine> — <BACKLOG id>

**Did:** what actually changed.
**Result:** built / tested / failed, with the real output.
**Learned:** anything the next run would otherwise rediscover the hard way.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---

## 2026-08-06 — jeff — decision: fixed module set (manual, not a scheduled run)

**Did:** Answered the open question raised by the same day's linux run (S1,
§7.1 of [docs/module-enumeration.md](docs/module-enumeration.md)):

> **TIDE ships a fixed module set, compiled in. No third-party module loading on
> any platform — not just iOS.**

Recorded as [PLAN.md](PLAN.md) **constraint 7**, so it is checked against every
future backlog item like the other six. Marked the question answered in the design
note, made stage 3 of that note a requirement rather than an option, and filed
stage 3 as BACKLOG **S1b**.

**Why it went in PLAN.md rather than a PR comment:** the weekly prompt has each run
read PLAN, BACKLOG, JOURNAL, carve-out, and open issues labelled for its own
platform. **PR comments, PR descriptions and review threads are read by nobody.**
An answer left on [PR #1](https://github.com/JeffMcClintock/TideSynth/pull/1) would
have been invisible to every future run. A GitHub issue would have been read, but
the prompt frames issues as broken builds, so a product decision filed as one gets
picked up as if it were a compile failure. The same trap applies to any new doc:
it only gets read if PLAN or BACKLOG links it, which is why
`docs/module-enumeration.md` is now in PLAN's companion-documents list.

**Learned:** the durable channels into a memoryless run are PLAN.md (rulings),
BACKLOG.md (queue) and JOURNAL.md (reasoning). Everything else on GitHub is
human-to-human only.

**Next:** unchanged — S1a still wants the §9 verification on a machine that can run
TIDE, and realistically P1/P2 first. S1b is queued behind it.

**Branch/PR:** committed to the S1 branch so the ruling lands with the note that
prompted it — `tide/linux/s1-module-enumeration-design`, PR #1.

---

## 2026-08-06 — macos — S1 (duplicate run — see "The collision" below)

**Did:** Took S1, independently analysed it, then discovered the Linux run had
taken the same item hours earlier and already had a PR open. Folded the macOS
findings into the Linux note as **Addendum A1–A6** rather than landing a
competing document at the same path. Filed G2 and S6. No code changed, no build
run, nothing in `SynthEdit` or `SynthEditLib` modified.

STEP 1 was clear — `gh issue list` on `JeffMcClintock/TideSynth` returns nothing
at all, there are no issues yet of any label. P1 and P2 are `win`, so S1 was the
topmost eligible item, exactly as it was for Linux.

**Result:** Addendum delivered. Independent analysis reached the same
recommendation as the Linux note (static registry, not scanning), by a different
route, which is worth something as corroboration. Four findings are additive and
one materially changes what the note's §9 verification will show.

**The collision — read this before assuming the stagger works.** Both boxes ran
on 2026-08-06. [agent-setup.md](docs/agent-setup.md) staggers Windows Fri /
macOS Sat / Linux Sun precisely so two machines cannot take the same backlog
item — but both were *set up* on the 6th, so both fired immediately and the
stagger had not taken effect yet. The mechanism is fine; the first week is the
hole. Nothing in the process caught it: I only noticed at `gh pr create` time,
when the push succeeded and the PR failed. Two cheap fixes, neither of which is
mine to make:

1. Have STEP 2 check open PRs and remote branches for `<backlog-id>` before
   marking an item DOING. `git ls-remote --heads origin` would have caught this
   in one call, before any work.
2. Mark DOING and **push that commit** before starting, not just commit it
   locally. The DOING mark is only useful as a claim if other machines can see
   it.

Also note the remote default branch is **`main`**, but a fresh clone here left
me on `master` — `gh pr create --base master` fails with a confusing "No commits
between…" rather than "no such branch". Use `main`.

**Learned — additive to the Linux entry, not repeating it:**

- **The `INIT_STATIC_FILE` list is three regions, not two, and this changes the
  §9 prediction.** The Linux note's §6 trap correctly spots that the JUCE arm
  (`UgDatabase.cpp:1063`–1139, ~70 entries) and the `#else` arm (`:1140`–1155,
  14) differ. But ~66 more entries sit *after* the `#endif` at `:1156` and are
  **unconditional**. Verified placements: `ug_adsr` `:1168`, `ug_oscillator2`
  `:1201`, `ug_vca` `:1215` are unconditional; `ug_filter_sv` `:1145` and
  `ug_filter_biquad` `:1144` are `#else`-only; `ADSR` `:1064`, `Converters`
  `:1070`, `OscillatorNaive` `:1082`, `Slider` `:1089` are JUCE-only.
  **Consequence:** after stage 1 the module browser will be *populated but
  wrong* — legacy `ug_*` modules present (enough for v0.1), modern SEM modules
  absent, and `ug_soundcard_in/out` + `ug_midi_out` present in violation of
  constraint 2. Whoever runs §9 must record *which* modules appear, not just
  whether the list is non-empty, or they will misread the result in either
  direction.

- **The "third, explicit list" the Linux note asks for already has a hook.**
  `SE_EXTRA_STATIC_FILE_CPP` at `UgDatabase.cpp:1239`, plus
  `initialise_synthedit_extra_modules()` at `:1243` (editor implementation at
  `ModuleFactory_Editor.cpp:170`). A TIDE module list can live in the TIDE
  target with **no edit to the shared function** — which makes stage 3 smaller
  than the note assumes. Four lines at the bottom of a 190-line function; easy
  to miss, and I nearly did.

- **The metadata half has a shipping mechanism too.**
  `RegisterExternalPluginsXmlOnce` (`UgDatabase.cpp:526`) reads
  `database.se.xml` from bundle resources (`:543`); the `imbeddedFilename`
  attribute (`:587`) is the switch between compiled-in and `dlopen`. And every
  module already ships its own descriptor beside its source (e.g.
  `SynthEditLib/modules/OscillatorNaive/OscillatorNaive.xml`), so the database
  can be **generated at build time by concatenation** with the attribute
  omitted. That is how the modern SEM modules get their pin metadata in without
  a scan. Blocker: `plugin_helper.cmake` emits `add_library(… MODULE)` at both
  `:70` and `:186` — there is no static-library variant of either macro today.

- **`-DSE_EXTERNAL_SEM_SUPPORT=0` will not work.** Both the Linux note's stage 3
  and its §5 want that macro settable independently. `xplatform.h:34` defines it
  unconditionally, so a CMake `-D` collides. `GMPI_IS_PLATFORM_JUCE` at `:25` is
  wrapped in `#if !defined(…)` for exactly this reason — giving
  `SE_EXTERNAL_SEM_SUPPORT` the same guard is a one-line change that alters no
  existing target's value.

- **`SE16/SE_IOS_APP/TIDE/Plugins/` is a decoy — do not try to make it load.**
  Six checked-in `.sem` bundles that look like an iOS module story. `file` says
  every binary is `Mach-O 64-bit bundle x86_64`; they are macOS bundle layout;
  and the Run Script that installs them
  (`SE_IOS_APP.xcodeproj/project.pbxproj:2064`) copies to
  `${BUILT_PRODUCTS_DIR}/${FULL_PRODUCT_NAME}/Contents/`, a macOS-only path.
  Nothing there can load on arm64. Filed as **S6**. This is the one finding only
  the Mac could have made, and it is a partial answer to the question the Linux
  note explicitly addressed to the Mac in its §4 — the full answer still needs
  M2 and a real device.

- **One scan root that stage 1 will not remove.** `TideApp.cpp:109` overrides a
  *good* default: `BundleInfo.cpp:699` already points `semFolder` inside the
  bundle. But `BundleInfo.cpp:712` adds a dev-tree fallback that walks **parent
  directories** hunting for a sibling `SynthEdit2/PlugIns`. It is not in
  `TideApp`, so deleting line 109 leaves it. One for S2.

- **Where the trees are on the Mac:** `~/Documents/GitHub/SynthEdit` (= `SE16`)
  and `~/Documents/GitHub/SynthEditLib`, **siblings, not nested** — same shape
  as the Linux box, different paths from PLAN.md's `C:\SE\…` table. `SynthEdit`
  was at `e6b50de2b`. `build/modules/Debug/` has 54 built `.sem`/`.gmpi`
  bundles, so the module set does build here.

**The process problem — Jeff should read this one.** STEP 5 of the run prompt
says agents must not modify `SE16` or `SynthEditLib` unless the item is an
approved carve-out stage. But S1a, S3, S4 and S5 all edit
`SE16/SynthEditSem/TideApp.cpp` or `SE16/SynthEdit2/`. As written, **no agent
can ever write TIDE code — only design notes.** Both today's runs happened to
draw design items so neither was blocked, but the next machine to pick up S1a
will either stop or quietly break the rule. Filed as **G2** (NEEDS-JEFF). I did
not downgrade S1a to BLOCKED: that is someone else's item and the reading is
ambiguous enough that the call should be Jeff's, not mine.

**Next:** G2 first — it gates everything with code in it, including S1a which is
otherwise the obvious next step. Then P1/P2 on Windows, since §9's check and
S1a both need a machine that can build and run TIDE. S6 is small and mac-owned
but also gated by G2.

**Branch/PR:** `tide/mac/s1-module-enumeration`, branched from
`tide/linux/s1-module-enumeration-design` rather than `main` so it is a clean
delta on PR #1 with no conflict. **If PR #1 is closed rather than merged, this
work goes with it** — the addendum lives in the Linux note's file.

---

## 2026-08-06 — linux — S1

**Did:** Wrote [docs/module-enumeration.md](docs/module-enumeration.md) — the design
note S1 asked for. No code changed; S1 explicitly said design only. Split three
follow-ups out of it into BACKLOG (S1a, S4, S5).

No open `platform:linux` issues, so STEP 1 was clear. S1 was the topmost TODO
matching `linux`/`any` (P1 and P2 are `win`).

**Result:** Note delivered. Nothing built — S1 required no build. The
recommendation has an explicit cheap empirical check (§9 of the note) that a
machine which can *run* TIDE must do before S1a is implemented; the Linux box
cannot do that yet (X1 is BLOCKED).

**Learned — things the next run should not have to rediscover:**

- **The Linux box has a full copy of the source tree**, at `~/SE/SE16` and
  `~/SE/SynthEditLib` — not just `C:\SE` as PLAN.md's table implies. SE16 was at
  `8a7b1ef7b`, SynthEditLib at `53f0979`. So `any` items that only need to *read*
  the source can be done on Linux, not just Windows. `~/SE` also has `build`,
  `build-vst3sdk`, `GMPI`, `GMPI-plugins`, `synthedit-website` and a
  `wayland-spike`. Whether SynthEditSem *builds* here is untested (that is X1).

- **The static-registration mechanism S1 was asked to design already exists.**
  `CModuleFactory`'s constructor (`SynthEditLib/UgDatabase.cpp:86`) calls
  `initialise_synthedit_modules()` (`:1054`), which force-links ~157
  self-registering modules; each registers with its full XML description in
  memory via `internalSdk::RegisterPlugin` (`UgDatabase.cpp:236`) or
  `RegisterPluginWithXml` (`:266`). No filesystem involved.

- **The module browser never touches the filesystem.**
  `ModuleBrowser::Init()` (`SE16/SynthEdit2/ModuleBrowser.cpp:99`) →
  `CSynthEditAppBase::ExportModules` (`SynthEditAppBase.cpp:1329`) →
  `ExportModuleNames()` (`ModuleFactory_Editor.cpp:2193`) → reads
  `CModuleFactory::Instance()->module_list`. That list is already populated by the
  constructor *before* `LoadOrScanModuleData()` runs, and the menu map is built
  lazily (`SynthEditAppBase.cpp:1331`), so it does not need `ReloadMenu()` either.
  **Therefore the scan contributes nothing for built-in modules** — stage 1 of the
  recommendation is a deletion, not a rewrite. This is the single most useful fact
  in the note.

- **Separate the two iOS prohibitions or you will over-scope.** Writing outside the
  container is banned for everything; loading code not signed into the bundle is
  banned for `dlopen`. But *reading inside the plugin's own bundle is allowed*. So
  modules (code) must be fixed at link time, while prefabs (XML data) can legally
  be enumerated from `Contents/Resources/`. Hence the hybrid recommendation rather
  than "compile everything in".

- **Trap for S1a/stage 3:** the two arms of `initialise_synthedit_modules` register
  *different* module sets. The `GMPI_IS_PLATFORM_JUCE==1` arm
  (`UgDatabase.cpp:1063`–`1138`) vs the `#else` arm (`:1140`–`1155`): the non-JUCE
  arm registers `ug_soundcard_in`, `ug_soundcard_out`, `ug_midi_out` — which TIDE
  must **not** have (constraint 2, the DAW owns I/O) — while the JUCE arm omits
  e.g. `ug_filter_sv`. Flipping `SE_EXTERNAL_SEM_SUPPORT`
  (`SynthEditLib/modules/shared/xplatform.h:34`, currently derived from
  `GMPI_IS_PLATFORM_JUCE` and not independently settable) silently changes which
  modules exist. TIDE probably needs a third, explicit list.

- **Two real bugs found in passing, filed not fixed** (S4, S5):
  - S4: `TideApp` sets `BundleInfo::semFolder` without `isSemFolderOverridden`
    (`BundleInfo.h:63`), so `SemCacheName()` (`ModuleFactory_Editor.cpp:174`) drops
    its `-override-<hash>` suffix and TIDE **writes** the desktop SynthEdit's
    `Plugin-Cache-16.xml`. A TIDE instance in a DAW can clobber the desktop app's
    module cache. Not an iOS issue — happens on Windows/macOS today.
  - S5: `TideApp::InitInstance` never calls `CSynthEditAppBase::InitInstance`, so
    `refreshFolderLocations()` never runs, `m_folder_settings` is empty, and
    `getFolderInfo` (`Application.cpp:167`) indexes `[0]` on an empty vector.
    Reachable from `ShortenFilename` (`SynthEditAppBase.cpp:238`).
    `ResolveFilename` is *not* affected — it uses `getDefaultPath`, which has a
    safe fallback (`Application.cpp:200`).
  - A third, cosmetic: `TideApp.cpp:109` hard-codes `L"modules\\"`, which on
    macOS/Linux names a directory ending in a literal backslash. Silent because
    `ScanFolder` swallows the error via `std::error_code`
    (`ModuleFactory_Editor.cpp:1009`). Not filed separately — stage 1 deletes the
    line.

- **Process note:** the run prompt says to commit the `DOING` mark before starting,
  but also never to work on `main`. I branched first, then committed the `DOING`
  mark on the branch (`4187556`). A crash after that point is still diagnosable,
  just from the branch rather than `main`. Suggest the prompt say so explicitly.

**Next:** S1a — stage 1 of the note — is `win` because it needs a machine that can
build and run TIDE. **Do the §9 check before touching code:** delete
`<settings>/SynthEdit/Plugin-Cache-16.xml`, point `ModulePath` at an empty folder,
launch TIDE, open the module browser. Full browser ⇒ the deletion is safe. Empty or
short browser ⇒ something outside `module_list` feeds it, and the note is wrong —
say so in the journal rather than pressing on. Realistically P1 and P2 should land
first anyway, since both S1a and that check need a working build.

Open question that is Jeff's, not an agent's: does TIDE ever want third-party
modules on desktop, or is a fixed module set the product? The note works either
way; only stage 3's shape depends on it.

**Branch/PR:** `tide/linux/s1-module-enumeration-design`

---

## 2026-08-06 — windows — project setup (manual session, not a scheduled run)

**Did:** Created this repo as the coordination point for TIDE Synth. Wrote
PLAN.md, BACKLOG.md, docs/carve-out.md, docs/design-notes.md,
docs/agent-setup.md, and a CI skeleton. No code was written and nothing in
`C:\SE\SE16` or `C:\SE\SynthEditLib` was modified.

**Learned:**

- TIDE is not a greenfield project. A working prototype exists at
  `C:\SE\SE16\SynthEditSem` — `TideApp` implements `ISeApp`, opens a
  `ContainerViewStruct` in `CF_STRUCTURE_VIEW` mode, and builds as VST3 + GMPI.
  There is also an existing iOS target at `SE16/SE_IOS_APP/TIDE/` and demo
  patches at `SE16/TideModules/`.
- The blocker for open-sourcing is `EditorLib`, which lives in the private
  `SynthEdit` repo. It has only 2 files of its own; the other ~120 come from
  `SE16/SynthEdit2/` via `EditorLib/CMakeLists.txt`. That file is the
  authoritative scope of the carve-out.
- `SynthEditLib` is already a **public** repo — but it has **no LICENSE file**,
  so it is not open source yet. This surprised the setup session and is now
  BACKLOG L1.
- The commercial boundary is cleaner than expected. `ExportAsPlugin` is one
  free function in one 2,470-line file. Its only callers are the private WinUI3
  IDE and `SynthEditCL`. `CContainer.h` references it solely through a `friend`
  declaration, which is legal C++ even when the function is never defined — so
  that header can go public unchanged. No shimming required.
- Moonbase licensing is already outside `EditorLib` by deliberate design
  (see the comment at `EditorLib/CMakeLists.txt:179`).
- "RNBW" in the original spec was a misreading; the reference is **RNBO** by
  Cycling '74. Note that a `getUniqueId() == id_to_long("RNBW")` special case
  does exist in `SE14/SynthEdit/VST_Wrapper.cpp` for a plugin called "rainbow" —
  unrelated, and a trap for a future agent grepping for it.

**Next:** L1, C0 and G1 need Jeff. P1 (verify the prototype builds) is the
first thing an agent can do unaided, and everything else depends on knowing
that baseline.

**Branch/PR:** none — scaffolding committed directly.
