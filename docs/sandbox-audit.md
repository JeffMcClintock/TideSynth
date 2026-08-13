# Sandbox audit — every filesystem, cache and registry write reachable from a TIDE build

BACKLOG **S2**. Produced 2026-08-14 on the linux box.

[PLAN.md](../PLAN.md) constraint 3 (*sandbox-safe*) and constraint 4
(*self-contained*) are the two this measures:

> **3. Sandbox-safe.** Must run under iOS AUv3 sandbox rules. No arbitrary
> filesystem access.
>
> **4. Self-contained.** No scattered caches, no writes outside the plugin's own
> sandboxed container. State lives in the plugin's saved state, not in
> `%APPDATA%`, `~/Library`, or a scanned modules folder.

**The headline: constraint 4 is violated today, and it is not a code-reading
inference — the files are on disk on this machine.** A TIDE-family build writes
**≈12.8 MB across 139 files** into three locations outside any plugin bundle:

| Location | Size | Files | Written by |
|---|---|---|---|
| `~/SynthEdit Projects/skins/` | 724 KB | 74 | `SkinMgr::setSkinFolder`, from `SkinMgr`'s **constructor** |
| `~/SynthEdit Projects/Prefabs/` | 1.1 MB | 35 | `Application.cpp:82-113` |
| `~/.local/share/SynthEdit/` | 11 MB | 30 | `ModuleFactory_Editor.cpp:1126-1182`, the module cache |

Constraint 3 has one clean violation of its own, in a code comment that says
outright what it is doing — see [finding A2](#a2--getpwuid-is-used-specifically-to-escape-the-sandbox--stub).

---

## Method, and why it is not a grep of SE16

A grep across `SE16` + `SynthEditLib` returns hundreds of hits in code TIDE
never compiles, which is worse than useless: it buries the real ones. The row
asks for writes *"reachable from a TIDE build"*, so the file set here is
derived, not chosen.

1. **Link closure, from `build.ninja`.** `TIDE_VST3.so`'s `LINK_LIBRARIES` is
   `VST3_Wrapper`, `SynthEditLib`, `EditorLib` — and, notably, **none of the
   `.sem` module libraries** (`ControlsXp`, `PatchMemory`, `Converters`, …).
   Those are separate SynthEdit plugins; the modules that *are* in TIDE live
   inside `SynthEditLib` itself.
2. **Translation units, from `compile_commands.json`.** Those targets plus
   TIDE's own sources are **264 TUs**; 34 are the VST3 SDK and generated
   Wayland protocol C, leaving **230 first-party files**, all resolving to a
   real current path (zero unresolved).
3. **Categorised scan** of exactly those 230 files — **202 hits**.
4. **Linked, not merely compiled.** A static archive contributes an object only
   when something references it. Reading the DWARF compile-unit list out of the
   unstripped `TIDE_VST3.so` gives the **235 CUs that actually survived the
   link**: of the 46 files with hits, **37 are in the binary and 9 are not.**

Reproduce all four steps:

```bash
python3 tools/sandbox_audit.py --build /home/jef/SE/build
```

### Drift, stated rather than assumed

The measurements come from `/home/jef/SE/build`, configured **2026-08-10** —
before C4 landed. That is deliberate: **SE16 does not configure on this box
today** ([#53](https://github.com/JeffMcClintock/TideSynth/issues/53), filed by
this run — `Standalone_Wrapper` hard-requires `libpipewire-0.3`), so a fresh
configure was not available. Both attempts are in that issue.

So the drift was measured instead of hoped for:

- **`EditorLib`** — the Aug-10 build compiled **57** TUs; today's
  `EditorLib/CMakeLists.txt` lists **59** `.cpp`/`.mm`. The difference is
  exactly `browseto.mm` and `openurl.mm`, which are inside `if(APPLE)` and are
  not compiled on Linux at all. **Set difference otherwise: empty, both ways.**
- **`SynthEditLib`** — its `CMakeLists.txt` source list is **unchanged since
  Aug 10**: 267 entries, zero added, zero removed.

C4 moved twelve files from `SE16/SynthEdit2/` into `SynthEditLib/`. That
changed their **paths**, not the set of code compiled, so the audit is current
in content. Paths below are today's.

### What this audit does not cover

- **Static reachability, not dynamic.** "Linked into the binary" is not "runs
  on every instantiation". Where the call chain matters, it is named in the
  finding — otherwise assume linked-and-plausibly-reachable, not proven-hot.
- **Headers.** The scan is of translation units. Logic in a header is caught
  only via the `.cpp` that includes it.
- **Linux/Windows paths.** No macOS or iOS box ran this. The macOS-specific
  branches are read from source and marked as such; **`browseto.mm` and
  `openurl.mm` are not compiled on Linux and are therefore unmeasured here**,
  which matters because they are exactly what D1 turns on.
- **Third-party.** The 34 VST3-SDK and generated Wayland TUs are audited as a
  group, not line by line. The VST3 SDK's `vstpresetfile.cpp` does file I/O by
  design, driven by the host's stream, and is not a TIDE-side decision.

---

## Classification

- **keep** — legitimate under constraints 3 and 4: reads inside the plugin
  bundle, or writes the host asked for through its own stream.
- **stub** — the code path must become unreachable or a no-op in TIDE. The code
  stays for SynthEdit, which is a desktop application and is entitled to it.
- **remove** — must not be in the TIDE binary at all. Constraint 7 (no module
  scanning) and constraint 8 (no user skins) are compile-out rulings, not
  runtime ones, so leaving these in and merely not calling them fails the
  constraint as written.

Every row is a real, linked site. The 9 files with hits that are **not** linked
are listed at the end so nobody re-investigates them.

---

## A. Writes outside the bundle — constraint 4

### A1 — `SkinMgr` copies the built-in skins to the user's home, from its constructor · **remove**

`SynthEditLib/SkinMgr.cpp:28-32`

```cpp
SkinMgr::SkinMgr()
{
    const auto commonDocuments = BundleInfo::instance()->getCommonDocumentFolder();
    setSkinFolder( (commonDocuments / L"SynthEdit Projects" / "skins" / "").wstring() );
    ScanFiles();
}
```

`setSkinFolder` (`:48-100`) then creates directories and **recursively copies**
`{app}/Resources/skins` over the destination, and writes a `.resource_version`
stamp beside it:

- `:74`, `:82`, `:96` — `std::filesystem::create_directories`
- `:84-88` — `std::filesystem::copy(..., recursive | overwrite_existing)`
- `:98` — `std::wofstream outFile(versionFile)`
- `:126` — `directory_iterator` over the user's skin folder (a scan)

**This is BACKLOG S7's static chain, now confirmed by measurement on Linux**,
where S7 could only assert it:

- `SkinMgr::SkinMgr()`, `SkinMgr::setSkinFolder`, `SkinMgr::ScanFiles`,
  `SkinMgr::Instance` are all **defined in `TIDE_VST3.so`** (27 `SkinMgr`
  symbols).
- The literal `SynthEdit Projects` is **in the TIDE binary** (2 occurrences);
  `default3` appears 4 times.
- On disk: `~/SynthEdit Projects/skins/` holds `default`, `default2`,
  `default3`, `_fallback`, `PD303` — **74 files, 724 KB** — with
  `.resource_version` containing `178`, last written 2026-08-10 11:30.

**Attribution, stated honestly:** SynthEdit's own applications run on this box
and write the same folder, so the *existence* of these files does not by itself
prove TIDE wrote them. What is proven is the whole chain bar the last step —
the code is linked into TIDE, the path literal is in TIDE's binary, and the
same code demonstrably produces exactly this folder when it runs. Closing the
last step needs TIDE loaded in a host, which needs [#53](https://github.com/JeffMcClintock/TideSynth/issues/53)
fixed first.

Constraint 8 says no user skins, permanently — so the ruling is **remove**, not
stub: TIDE loads its look from plugin resources and no skin folder exists to
copy to. `SkinMgr.cpp` is in `SynthEditLib` (GATED), so this is S7's gated
remainder, named exactly: **`SynthEditLib/SkinMgr.cpp:28-32` and `:48-100`**.

**A trap for whoever takes S7 after C7.** The version stamp is keyed on
`SE_APP_BUILD_NUMBER`, which `SynthEditLib/se_version.h` defaults to `0` and
`EditorLib/CMakeLists.txt` injects from SynthEdit's `se_build_number.h`
(currently `183`). TIDE links that same EditorLib, so **today TIDE and SynthEdit
agree on the number and nothing thrashes.** From C7 — clean clone, no access to
the private repo — TIDE takes the `0` default while SynthEdit keeps `183`, and
`versionChanged` becomes true on *every* launch of *both*. They would then
re-copy 724 KB of skins over each other on every single instantiation. That
argues for removing the mechanism from TIDE before C7 lands, not after.

### A2 — `getpwuid` is used specifically to escape the sandbox · **stub**

`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:343-352`

```cpp
#else // Mac.
    // getpwuid returns the real home directory even when sandboxed,
    // whereas getenv("HOME") returns the container path in a sandbox.
    const struct passwd* pwd = getpwuid(getuid());
```

**The comment states the intent plainly: this is a deliberate sandbox-escape.**
Under AUv3, `getenv("HOME")` returns the container — which is the correct,
sandbox-respecting answer and exactly what constraint 3 wants. This code
prefers `getpwuid` *because* it defeats that.

Correct for SynthEdit, which is a desktop application. Directly contrary to
constraint 3 for TIDE, and it sits upstream of A1 and A3 — `getCommonDocumentFolder`
(`:371-385`) falls through to `getUserDocumentFolder` on every non-Windows
platform, so this one function chooses the destination for both the skin copy
and the prefab copy on macOS, iOS **and Linux**.

The commented-out block immediately below it (`:355-366`) even records the
sandboxed path it was avoiding:
`/Users/…/Library/Containers/com.synthedit.SynthEditMac/Data/SynthEdit Projects/skins/`.

**Stub**, not remove: TIDE needs *a* document folder concept for the host to
hand it paths. It must be the container, i.e. the `getenv("HOME")` answer, or
nothing at all. GATED (`SynthEditLib`); flagged here, not edited.

### A3 — Prefabs copied to the user's documents folder · **remove**

`SE16/SynthEdit2/Application.cpp:82-113` — same shape as A1, different payload:

- `:82` — destination `getCommonDocumentFolder() / "SynthEdit Projects" / "Prefabs"`
- `:103` — source `GetHomeDir() / "Resources" / "Prefabs"`
- `:108` — `create_directories`
- `:110-114` — `copy(..., recursive | overwrite_existing)`

On disk: `~/SynthEdit Projects/Prefabs/`, **35 files, 1.1 MB**.

Prefabs are content TIDE genuinely wants (E2a builds three of them), but
constraint 4 says they ship *in* the plugin and load from there. Copying them
out to a shared user folder is the thing being ruled out, not the prefabs
themselves. `Application.cpp` moves under **C5**; this is a TIDE-side behaviour
change to make at that point, not a reason to touch a GATED file now.

### A4 — The module cache: 11 MB in `~/.local/share/SynthEdit/` · **remove**

`SynthEditLib/ModuleFactory_Editor.cpp`

- `:1126` — `std::filesystem::path settingsPath(getSettingsFolder());`
- `:1168`, `:1182` — `getSettingsFolder() / L"SynthEdit" / SemCacheName()`
- `:1172` — `std::filesystem::remove(cacheFilename, ec)`

`getSettingsFolder()` resolves through `BundleInfo::settingsPath()`
(`BundleInfo.cpp:126-138`) to `$XDG_DATA_HOME`, defaulting to
`~/.local/share` — the Linux analogue of the `~/Library` constraint 4 names
explicitly.

On disk: `~/.local/share/SynthEdit/` holds **six `Plugin-Cache-16-override-*.xml`
files at ~330 KB each, plus a `modules/` directory — 30 files, 11 MB.**

This is the *cache* half of constraint 7, and it is the largest single
violation by size. **Remove** — a plugin with a fixed, statically-registered
module set has nothing to cache. Same file and same ruling as **S1b**, which
this independently confirms; see B1.

### A5 — Live module update staging · **remove**

`SE16/SynthEdit2/SynthEditAppBase.cpp:529-589` — a developer feature that
copies `.sem` binaries into a staging folder and deletes the originals:

- `:531` — `getPlatformPluginsFolder() / L"SynthEdit" / L"modules-staged"`
- `:547` — `directory_iterator` over the staging dir
- `:580` — destination from the `ModulePath` setting
- `:586` — `fs::copy_file(..., overwrite_existing)`
- `:589` — `fs::remove(sourceFilename, copyError)`
- `:638` — `MonitorFileSystem(std::filesystem::path modulesFolder)`

Writes *and deletes* outside the bundle, in the service of loading third-party
modules — constraints 3, 4 and 7 at once. `SynthEditAppBase.cpp` moves under
**C5**.

### A6 — `new_module` project scaffolding · **stub**

`SynthEditLib/CUG.cpp:2833-2968` — "Build Code Skeleton": creates
`getUserDocumentFolder() / "new_module"` and a project subfolder under it
(`:2967`, `:2968`), copies template files (`:2842` `fopen(...,"w")`, `:2891`,
`:2901` `copy_file`).

`TideApp.cpp` already stubs `doDialogBuildCodeSkeleton` with `assert(false)` —
which is **BACKLOG S3's exact complaint**: in a release build that assert
compiles out and the path falls through silently. So this is S3's concrete
consequence, measured: the write sites are live in the binary behind a stub
that does nothing in release. **Stub properly**, per S3.

### A7 — DSP debug dumps · **stub**

`SynthEditLib/SeAudioMaster.cpp:1567-1592` — `se_fs::create_directories(outputFolder)`
then two `fopen(..., "wb"|"w")` writing audio and pin-name dumps. A debugging
aid; needs to be unreachable in a shipping plugin. Cheap to gate.

### A8 — `ug_wave_recorder` writes a WAV to an arbitrary path · **remove**

`SynthEditLib/ug_wave_recorder.cpp:216` — `fopen(utf8Path.c_str(), "wb")`.

A module that writes a user-chosen file is precisely the "browse for…" class
constraint 3 removes. It is compiled and registered in the TIDE binary today.
**Same shape and same fix as S8** — an unwanted module statically registered in
`SynthEditLib/UgDatabase.cpp`; it belongs on S8's list, which currently names
only `ug_soundcard_in`, `ug_soundcard_out` and `ug_midi_out`.

### A9 — `conversion.cpp` directory creation · **stub**

`SynthEditLib/conversion.cpp:1197`, `:1224`, `:1231` — a portable
`create_directories` helper (`_wmkdir` / `mkdir(…, 0775)`), plus `:470`
`fopen(…,"r")`. Utility code, harmless in itself; it is the callers (A1, A3,
A4, A5) that matter. **Stub** only in the sense that it should have no
remaining callers in TIDE once those are fixed — nothing to change here
directly.

---

## B. Module scanning and dynamic loading — constraint 7

### B1 — The scan is still in the binary, on Linux too · **remove**

Constraint 7 is a compile-time ruling: *"statically registered at link time. No
module scanning, no module cache, no loading of third-party modules — on any
platform."* Measured in `TIDE_VST3.so`:

| Symbol | Present |
|---|---|
| `ScanFolder(std::filesystem::path const&, …)` | yes (+ `.cold`, `.localalias`) |
| `LoadModuleData()` | yes |
| `LoadOrScanModuleData` | yes |
| `CModuleFactory::RegisterExternalPluginsXmlOnce(TiXmlNode*)` | yes |
| `RegisterExternalPluginsXml(…)` | yes |
| `Module_Info3` | 65 symbols |

And the loader is genuinely imported from libc, not merely referenced —
`nm -D --undefined-only` lists **`dlopen`, `dlsym`, `dlclose`**.

`SynthEditLib/modules/shared/xp_dynamic_linking.cpp:30-56` is the site
(`dlopen(…, RTLD_LAZY)`, `dlsym`), with the scan itself in
`ModuleFactory_Editor.cpp:1007-1092` (`ScanFolder`, `ScanFile`, `ReloadDll`,
`directory_iterator`).

**This is BACKLOG S1b (b) and (c), independently reproduced on Linux** — S1b's
measurements were taken from the macOS Release binary. Two platforms, same
result.

**S1b (a) is confirmed genuinely done, by the same measurement.** `FileWatcher`
has **zero** symbols in the binary, and `SynthEditLib/modules/shared/FileWatcher.cpp`
is **not** among the linked compile units — so replacing `SynthEditApp.cpp` with
`TideAppStubs.cpp` did remove the filesystem-watcher thread, exactly as that
row claims.

### B2 — `UG2.cpp` writes a temp file and `LoadLibrary`s it · **remove**

`SE16/SynthEdit2/UG2.cpp:455-561` — `GetTempPath` (`:470`), `CreateFile`
(`:455`, `:494`), then `LoadLibrary(szTempName)` (`:561`): write a DLL to
`%TEMP%` and load it. Windows-only, and the sharpest single expression of what
constraint 3 forbids. Linked into the binary. `UG2.cpp` moves under **C5**.

### B3 — S8 reproduces on Linux · *(cross-check, no new ruling)*

Not a new finding — recorded because it is a second platform confirming S8,
whose measurements were macOS-only:

| Symbol | Count in Linux `TIDE_VST3.so` |
|---|---|
| `ug_soundcard_in` | 28 |
| `ug_soundcard_out` | 33 |
| `ug_midi_out` | 24 |
| `OscillatorNaive` | **0** |

Soundcard and MIDI-out modules compiled in and registered — in a plugin whose
premise is that the DAW owns I/O (constraint 2) — and the modern SEM oscillator
absent. **`OscillatorNaive` being 0 on Linux as well confirms the blocker E2a's
row warns about**: there is no modern oscillator primitive registered, on either
measured platform.

---

## C. Reads, and things that are fine

### C1 — Resource loading inside the bundle · **keep**

`SynthEditLib/modules/se_sdk3_hosting/GmpiResourceManager.cpp:84-314` — 13
`fs::` sites, all **read-only path resolution** against
`BundleInfo::getResourceFolder()` / `getImbeddedFileFolder()`, i.e. inside the
plugin bundle. `BundleInfo.cpp:606` `fopen(filePath, "rb")` is the matching
read. This is what constraint 3 permits and what TIDE should be doing *more* of
once A1 and A3 are removed.

One caveat for a later pass: `GmpiResourceManager.cpp:264-303` resolves against
a *skins* folder. Once A1 is removed that folder will not exist, so the skin
search path needs repointing at bundle resources in the same change — the two
are coupled, and fixing A1 alone would break image loading.

### C2 — Host-driven preset streams · **keep**

`SynthEditLib/Shared/VstPreset.cpp:36-318` (6 sites) and `Shared/AuPreset.cpp:80`
— open files by a path the **host** supplies through the VST3/AU preset API.
The host owns that path and has already cleared it with the user; this is the
DAW's I/O, not the plugin reaching into the filesystem. Constraint 2's spirit,
correctly implemented.

### C3 — `tinyxml`, `tinyxml2`, `wav_file`, `Processor.cpp`, `mp_sdk_audio.cpp` · **keep**

One `fopen`/`ofstream` each, in generic library code that opens whatever path
it is handed. No policy of their own; they inherit the caller's. `Processor.cpp:109`
and `SeAudioMaster.cpp:109` are `gmpi::Processor::open(phost)` — the GMPI
lifecycle call, matched only because the pattern looks for `open(`. **False
positives**, recorded so the next audit does not re-flag them.

### C4 — `std::remove` the algorithm, not the syscall · **keep**

`SeAudioMaster.cpp:2733`, `TimerManager.cpp`, `Timer.cpp`,
`gmpi_ui/experimental/builders.cpp` (2) — `std::remove` on a container, the
`<algorithm>` overload. **False positives**, same reason.

### C5 — `freopen("CON", …)` · **keep**

`SynthEditAppBase.cpp:61` — reattaches stdout to a Windows console. Not a
filesystem write in any meaningful sense; Windows-only; harmless.

---

## D. Compiled but not linked — do not re-investigate

These 9 files matched the scan but their objects **did not survive the link**
into `TIDE_VST3.so`, so they are dead for TIDE on Linux. Listed so the next
audit does not spend time on them:

| File | Hits | Note |
|---|---|---|
| `SynthEditLib/modules/se_sdk3_hosting/Controller.cpp` | 7 | |
| `SynthEditLib/modules/shared/FileWatcher.cpp` | 3 | **S1b (a) removed this — see B1** |
| `SynthEditLib/ThemeManager.cpp` | 2 | the only `HKEY_`/`RegOpenKeyExW` sites in the whole scan; Windows-only, and **not linked**, so TIDE touches no registry on any platform measured |
| `SynthEditLib/Shared/se_logger.cpp` | 1 | `fopen(…, "wb")` — not linked |
| `SynthEditLib/imbedded_file.cpp` | 1 | |
| `SE16/SynthEdit2/legacyExternalApp.cpp` | 1 | |
| `gmpi_ui/helpers/ImageMetadata.cpp` | 1 | |
| `gmpi_ui/backends/DirectXGfx.cpp` | 1 | Windows backend |
| `gmpi_ui/backends/DrawingFrameWin.cpp` | 1 | Windows backend |

**Caveat:** "not linked" is measured on the **Linux VST3** build. A Windows or
macOS build has different backends and may pull some of these in —
`ThemeManager.cpp` and the two Windows `gmpi_ui` backends especially. Re-run
`tools/sandbox_audit.py` on those platforms before treating this table as
cross-platform.

---

## Summary

| Ruling | Findings |
|---|---|
| **remove** | A1 skins copy · A3 prefabs copy · A4 module cache · A5 module staging · A8 `ug_wave_recorder` · B1 scan + `dlopen` · B2 temp-DLL load |
| **stub** | A2 `getpwuid` sandbox escape · A6 `new_module` scaffolding · A7 DSP dumps · A9 (no direct change) |
| **keep** | C1 bundle resources · C2 host preset streams · C3–C5 library code and false positives |

**Constraint 4 cannot be marked verified.** It is measurably violated, in three
places, totalling ≈12.8 MB on this machine.

**Constraint 3** has one deliberate escape (A2) plus the write set above; the
AUv3-specific half is unmeasured because no macOS/iOS box has run this.

### Where each finding already has a home

Most of this is not new work — it is measurement attaching to rows that already
exist. Nothing below is filed as a new item:

- **S7** ← A1, and the gated remainder is now named to the line.
- **S1b** ← A4, B1. Confirmed on a second platform; **(a) confirmed genuinely
  done.**
- **S8** ← A8 (`ug_wave_recorder` belongs on its list), B3 (reproduced on Linux).
- **S3** ← A6. The `assert(false)` stub has live write sites behind it.
- **C5** ← A3, A5, B2 — all in `Application.cpp` / `SynthEditAppBase.cpp` /
  `UG2.cpp`, which that stage moves.
- **E2a** ← B3: `OscillatorNaive` is absent on Linux too, so the blocker its row
  warns about is real on both measured platforms.
- **D1** ← the note in "What this audit does not cover": `browseto.mm` /
  `openurl.mm` are `if(APPLE)` and were **not** measured here.

The one genuinely new thing is **A2**, and it is a `SynthEditLib` (GATED)
behaviour question rather than a bug: a sandbox escape that is correct for
SynthEdit and wrong for TIDE, sitting upstream of two other findings. Whoever
takes S7 will have to rule on it, because A1 cannot be fixed properly without
deciding what `getCommonDocumentFolder` should return in a plugin.
