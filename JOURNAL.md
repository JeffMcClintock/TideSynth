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

## 2026-08-08 — linux — H1 (interactive session with Jeff, not the scheduled run)

**Did:** Took tidesynth.com live on GitHub Pages. Enabled Pages, deployed,
repointed DNS at the registrar, set the custom domain, enabled Enforce HTTPS.
Also merged PR #21 (`website/CNAME`) at Jeff's instruction. All of it driven
from this session — the DNS half through Jeff's own logged-in Chrome, the
GitHub half through `gh`.

**Result — live and verified end to end:**

```
https://tidesynth.com/       200,  cert verifies clean
https://www.tidesynth.com/   301 -> https://tidesynth.com/
http://tidesynth.com/        301 -> https://tidesynth.com/   (all 4 edge IPs)
cert: CN=tidesynth.com, SAN={tidesynth.com, www.tidesynth.com}
      Let's Encrypt YR2, Aug 7 -> Nov 5 2026, auto-renews
```

**The old site was already dead before we touched anything.** The apex pointed
at `202.124.241.178` = `redirector.servers.netregistry.net`, which served an
HTML *frameset* embedding `tidesynthstaticwebsite.z13.web.core.windows.net`.
That Azure endpoint is **NXDOMAIN** — gone. So tidesynth.com was serving a frame
around nothing, and `www` did not resolve at all. This was a repair, not a
migration, which is why there was no cutover window to protect.

**Learned — things that will cost the next person an hour each:**

1. **The Domainz DNS editor is at `/~/dns` and nothing links to it.** The
   domain's own product page offers only Lock/Unlock, Update Nameservers,
   Update Registrant and EPP code. Its Settings tab is labels and delegate
   access. The separate "tidesynth.com (Domain Manager)" product — filed under
   *Email & Office Tools*, of all places — is metadata only. I searched all
   three and concluded the portal had no zone editor and that the records must
   live in a legacy Netregistry console; I was about to recommend moving DNS to
   Cloudflare. **Jeff produced the URL.** Corrected in
   [docs/hosting.md](docs/hosting.md).

2. **"Export Zone" is server-side broken — 500 every time.** There is no working
   zone export, so the backup at
   [docs/dns-zone-tidesynth.com.txt](docs/dns-zone-tidesynth.com.txt) was
   transcribed by hand from the table and cross-checked with `dig`. Worse, a
   failed Export leaves a dead modal behind so the *next* dialog you open also
   renders as a 500 — which made DNS editing look broken when it is not. Reload
   the page and the editor works. I lost time treating the second 500 as real.

3. **The domain has live email, and that was the real risk.** MX plus
   smtp/imap/pop/pop3/webmail CNAMEs on `nsserver.net.nz`. Six of the nine
   records in the zone are mail. Nothing about pointing a website at Pages
   touches them, but a careless "repoint the domain" would have taken them out.
   Verified intact on all three nameservers after the change.

4. **Their nameservers propagate inconsistently, and briefly lie.** After the
   edit, `ns1` served the new records while `ns2`/`ns3` still served the old IP
   — and twelve repeated queries to `ns1` alone came back **6 old / 6 new**, so
   ns1 is several backends syncing independently. A single `dig` was worthless
   here. Convergence took roughly 10 minutes; `www` propagated fully well before
   the apex `A` did. **Any check on this registrar must poll all three
   nameservers repeatedly before believing the answer.**

5. **Local resolver cache made the finished site look broken.** After everything
   was working I ran a plain `curl https://tidesynth.com/` and got *"Failed to
   connect on port 443"*, plus `http://` returning `200`. Both were this
   machine's resolver still holding the old IP and hitting the dead redirector.
   Cloudflare's `1.1.1.1` was stale for ~30 min after Google and Quad9 had
   updated. **Verify with `--resolve` against a known edge IP**, or you will
   diagnose a phantom.

6. **`https_enforced` gets set back to `false` when you save a custom domain,**
   and the redirect then lags the setting. The API read `https_enforced: true`
   while all four edge IPs still answered `200` instead of `301` for several
   minutes. Not a misconfiguration — just edge propagation. The certificate
   itself arrived in **4 minutes**, far quicker than the hour the doc budgeted.

7. **The `website/CNAME` file is load-bearing, and now proven.** With the
   *GitHub Actions* source the published artifact defines the served domain, so
   without that file a later deploy can clear the custom domain silently — green
   deploy, dead domain. PR #21 added it; merging it triggered a deploy, and
   `cname`, `https_enforced` and the certificate all survived. That was the test.

**Next:** H1 is done and marked RESOLVED. The remaining website item is **R6**
(Downloads section), still blocked on there being something to download. Note
`build.yml` fails on every PR — three platforms, all *"source directory does not
appear to contain CMakeLists.txt"* — which is **B1**'s known pre-carve-out state,
not a regression; it made PR #21 show as `UNSTABLE` and it is safe to merge past
until C7.

**Branch/PR:** `docs/h1-golive-writeup`; `website/CNAME` merged as PR #21.

---

## 2026-08-08 — linux — X3

**Did:** Fixed X3 — one line of substance in `SE16/SynthEditSem/CMakeLists.txt`
(ALLOWED, TIDE's own file), plus a comment explaining the trap. The
`FetchContent_Declare` for `gmpi_wrappers` now pins an explicit sha:

```
-  GIT_TAG origin/main
+  GIT_TAG e6a454156c3505483a9bd1dca4f74f0511e7efa5
```

`SE16` commit `23bee68ce` on branch `tide/linux/X3-vst3-moduleentry`, pushed,
**not merged** — see the warning at the bottom.

**Result: fixed and verified, both directions.**

Before (baseline reproduced first, on the tree as it stood):

```
$ nm -D --defined-only TIDE_VST3.so | grep -E 'Module|Dll|Factory'
00000000001c8650 T ExitDll
00000000001c8620 T GetPluginFactory
00000000001c8630 T InitDll
```

After:

```
00000000001fd910 T ExitDll
00000000001fd8e0 T GetPluginFactory
00000000001fd8f0 T InitDll
00000000001fd930 T ModuleEntry
00000000001fd940 T ModuleExit
```

And Steinberg's own validator — the tool that rejected the module outright with
*"The shared library does not export the required 'ModuleEntry' function"* —
now loads it and runs the suite:

```
$ /home/jef/SE/build-vst3sdk/bin/Release/validator TIDE_VST3.vst3
Result: 47 tests passed, 0 tests failed
```

Builds (gcc 13.3.0, RelWithDebInfo, existing `~/SE/build` tree):

| Target | Exit | Warnings |
|---|---|---|
| `TIDE_VST3` | 0 | 4, all in Steinberg's `fstring.cpp` — none in TIDE |
| `TIDE` (GMPI) | 0 | 0 |
| `SynthEditCL` | 0 | 0 |
| `SynthEditWayland` | 0 | 0 |
| `SynthEdit_VST3` | 0 | 2, both in Steinberg's `ustring.cpp` |

**Learned:**

1. **Why `GIT_TAG origin/main` freezes, precisely.** It is not that CMake
   ignores updates — it is that `origin/main` is a *remote-tracking ref that
   already resolves inside the cached clone*. CMake's git-update step asks
   "does `origin/main` name a commit I already have, and is HEAD there?", gets
   yes on both, and never runs `git fetch`. So the checkout pins itself to
   whatever `main` pointed at on the **first** configure, permanently and
   silently. An explicit sha does **not** have this problem: the sha is absent
   from the cached clone, resolution fails, and CMake fetches. Verified —
   `_deps/gmpi_wrappers-src` moved `032b4d5` → `e6a4541` on the first
   reconfigure after the change. **The shared `SE16/CMakeLists.txt` has the
   identical `GIT_TAG origin/main` in six places** — `SynthEditLib` (:120),
   `GMPI` (:137), `gmpi_ui` (:154), `AudioUnitSDK` (:189), `clap` (:205),
   `clap-helpers` (:214). All frozen the same way; the boxes just don't notice
   for the first three because they set `*_FOLDER_OVERRIDE`, which is exactly
   the mask that hid this one. The CLAP pair is the one to watch — nobody
   overrides those, and X1 needs CLAP. That file is a shared build file
   (GATED) — filed as **X4** rather than edited.

2. **Chose the pin over the two alternatives, for a reason.** X3 offered pin /
   force-refresh / `GMPI_WRAPPER_FOLDER_OVERRIDE`. The override is a
   per-developer local path — it fixes this machine and leaves the default
   build, and therefore CI, broken. Force-refresh (`GIT_TAG main`) restores the
   *intent* but keeps the property that made this bug invisible: what you build
   depends on when you configured. A sha is reproducible, is what B1 and C7
   will need from public CI, and turns a silent staleness into a line of text
   somebody can read.

3. **The bump carries a Windows fix, not a Windows risk.** `032b4d5..e6a4541`
   is 15 commits and touches `SEVSTGUIEditorWin.cpp` and `SEVSTGUIEditorMac.cpp`,
   so I checked what. The Windows delta *is* **P4b** — the `checkSizeConstraint`
   fix the Windows box wrote — plus the shared `measurePreferredSize` refactor.
   The rest is Linux X11/Wayland editor work and CLAP PIC.

4. **`/usr/bin/cmake` on this box is 3.28.3 and cannot configure this tree**
   (`cmake_minimum_required(VERSION 3.30)`, declared in ten CMakeLists across
   `SE16` — the root, `EditorLib`, `se_vst3`, `se_au`, `SynthEditCL`,
   `SynthEditWayland`, `SynthEditJuce`, `tests`, `EditorScreenshot`,
   `SynthEditSem`). It fails with a bare *"CMake 3.30 or higher is required"*
   that reads like a broken checkout. Ubuntu 24.04 is pinned at 3.28.3 for the
   life of the LTS, so this does not resolve itself. **Do not lower
   `cmake_minimum_required` to suit it** — most of those files are shared/GATED,
   and the version also sets policy scope (CMP0168/CMP0169, the FetchContent
   policies, landed in exactly 3.30 and this tree leans on FetchContent hard).
   That would change Windows and macOS build semantics to accommodate one Linux
   box.

   The working cmake during this run was
   `/home/jef/.cache/cmake-3.31.6-linux-x86_64/bin/cmake`. **Two traps around
   it.** First, `CMakeCache.txt` recorded `CMAKE_COMMAND=/usr/bin/cmake` — the
   3.28 one — which is misleading, because the tree plainly had been configured
   by something newer. Second, and worse: reconfiguring with the `~/.cache`
   binary *rewrites* `CMAKE_COMMAND` to point into `~/.cache`, and ninja invokes
   that path whenever a `CMakeLists.txt` changes and the tree needs
   regenerating. `~/.cache` is by contract a disposable directory. If anything
   cleans it, `~/SE/build` breaks with an error that looks nothing like its
   cause.

   **Resolved the same day.** Jeff installed Kitware's APT repo for noble, which
   supplies **cmake 4.4.2** (`4.4.2-0kitware1ubuntu24.04.1`), replacing apt's
   3.28.3 at `/usr/bin/cmake`. `~/.cache` is now out of the dependency path —
   `CMAKE_COMMAND` reads `/usr/bin/cmake` again, and this time it means a cmake
   that can configure the tree. **The 3.x → 4.x jump is the part to know about,
   because it is a major version and CMake 4 drops compatibility with
   `cmake_minimum_required(VERSION < 3.5)` — the usual way a fetched
   third-party dependency breaks.** Nothing here does: configure is clean, and
   `TIDE_VST3`, `TIDE`, `SynthEditCL`, `SynthEditWayland` and `SynthEdit_VST3`
   all rebuild at exit 0 with no warnings. X3 re-verified on the new toolchain
   from a real recompile of `wrapperVst3.cpp` — `ModuleEntry`/`ModuleExit` still
   exported, validator still 47/47. The `GIT_TAG` pin survived the toolchain
   change too (`_deps` still at `e6a4541`), which is the reproducibility the pin
   was chosen for.

5. **A prebuilt Steinberg `validator` is on this box** at
   `/home/jef/SE/build-vst3sdk/bin/Release/validator`. The S4 run concluded the
   validator route "died immediately" and fell back to a hand-linked probe —
   the binary was there the whole time. It takes a `.vst3` bundle directory,
   runs in seconds, and is the cheapest real-host check available on Linux.

6. **P5 confirmed on Linux, in passing.** The validator prints
   `name = SynthEdit` for both classes and `vendor = GMPI`. Same finding P2 made
   in REAPER on Windows; it is not Windows-specific and not a host quirk.

**Next:** **X1's empirical half is now unblocked** — the Linux VST3 loads, so
someone can put TIDE in a Linux host for the first time. CLAP is still genuinely
absent: `SynthEditSem/CMakeLists.txt:41` reads `set(FORMATS_LIST GMPI VST3)`
with no CLAP entry, so that half of X1 is real work. Also **X4** (the same
`origin/main` freeze in the shared `SE16/CMakeLists.txt`) is filed and wants a
Windows or macOS box, since those are the ones that would notice a GMPI or
gmpi_ui bump.

**Merge warning — the same trap S1a hit.** This run's actual fix is in `SE16`,
a *different repo* from this one. The TideSynth PR below contains only the
journal and backlog; merging it does **not** land the code. `SE16` branch
`tide/linux/X3-vst3-moduleentry` (`23bee68ce`) has to be merged into `SE16`
master separately. S1a found the S4 branch stranded exactly this way a day
later. **Check `SE16` for unmerged `tide/*` branches.**

**Branch/PR:** TideSynth `tide/linux/X3-vst3-moduleentry`; code in `SE16`
`23bee68ce` on the branch of the same name.

---

## 2026-08-08 — macos — S1b (partial: the ALLOWED part; the rest is gated)

**Did:** Took S1b, did the whole of the part that is TIDE's to do, measured the
part that is not, and put S1b back in the queue as BLOCKED on C0 rather than
claiming it done. Landed `SE16` `40b6008ee` on branch
`tide/mac/S1b-compile-out-scan`: TIDE's `SynthEditSem/CMakeLists.txt` no longer
compiles `SynthEdit2/SynthEditApp.cpp`, and a new
`SynthEditSem/TideAppStubs.cpp` supplies the three symbols that were the only
reason it was there. Wrote addendum **B0–B5** to
[docs/module-enumeration.md](docs/module-enumeration.md). Filed **S8** and
**P6**.

**Result — TIDE builds on macOS, and that had never been recorded.** `TIDE` and
`TIDE_VST3`, Debug and Release, universal `x86_64;arm64`, Xcode generator,
deployment target 13.3 — all four exit 0 with no warnings. BACKLOG **M1** is
about the AU/AUv3 targets and is still blocked; the GMPI and VST3 plugins build
here today. Nothing needed configuring — the checked-out `build/` tree already
had `SynthEditSem` in it.

**Result — the headline: §6 stage 3 was wrong, and I nearly implemented the
wrong thing.** Stage 3 offers two routes, `TIDE_NO_EXTERNAL_MODULES` **or**
decoupling `SE_EXTERNAL_SEM_SUPPORT` from `GMPI_IS_PLATFORM_JUCE`. The second
is not an alternative. It removes nothing:

- `SE_EXTERNAL_SEM_SUPPORT` appears in exactly **two** places in the whole
  codebase outside comments — `SynthEditLib/UgDatabase.cpp:28` (an `#include`)
  and `:595` (one `new Module_Info3(imbeddedFilename)` branch). `ScanFolder`,
  `LoadOrScanModuleData`, `SemCacheName`, `LoadModuleData` and
  `ClearModuleDataCache` have **no feature guard at all** — only `_WIN32` /
  `__APPLE__`. Flipping the flag changes one object file.
- `Module_Info3.cpp` is unconditional in `SynthEditLib/CMakeLists.txt:295` and
  `Module_Info3` is constructed at `ModuleFactory_Editor.cpp:500` and `:1309`
  and `dynamic_cast`ed in ~13 more places across `CUG.cpp`, `DocOb.cpp`,
  `ExportAsPlugin.cpp`. So `dlopen` stays linked regardless.

A4 is confirmed exactly, including that it fails *silently*:
`-DSE_EXTERNAL_SEM_SUPPORT=0` gives
`xplatform.h:35:10: warning: 'SE_EXTERNAL_SEM_SUPPORT' macro redefined
[-Wmacro-redefined]` — a warning, not an error — and the value ends up **1**.

**Result — what is actually in the shipping binary.** Release, `arm64` slice,
`nm | c++filt`, built from `SE16` master `a6f6d82c9` (S1a's deletion already
in). `ScanFolder`, `SemCacheName`, `LoadModuleData`, `ClearModuleDataCache`,
`ApplicationBase::LoadOrScanModuleData`, `Module_Info3::LoadDllOnDemand` and
`gmpi_dynamic_linking::MP_DllLoad` are all present, and `nm -u` shows `_dlopen`,
`_dlsym`, `_dlclose` imported. **There is no dead-stripping** — the Release link
passes no `-dead_strip`, so nothing falls out by itself. S1a removed the call;
the code is all still there, which is exactly what S1b exists to fix and is what
an AUv3 reviewer would see.

**Result — the one piece that was TIDE-side, and it was worth having.** TIDE's
own CMakeLists compiled the whole of `SynthEditApp.cpp` to satisfy three symbols
EditorLib references. That put `SynthEditApp::InitInstance()` in the plugin —
**a second `LoadOrScanModuleData()` call site** that S1a did not know about,
plus a detached `MonitorFileSystem()` thread on the live-modules folder, the
`SynthEdit16.settings.xml` read/write path, and `licenseIsTrial` /
`startActivationPolling` / `checkActivationStatusNow` /
`licenseStatusDescription`.

Scoping it was one experiment: comment the file out, build, read the linker's
complaint. Exactly three symbols — `theApp`, `SafeMessagebox`,
`SynthEditApp::licenseIsActive()` and `::isMoonbaseEnabled()`. A 60-line
`TideAppStubs.cpp` supplies them, and **behaviour is unchanged, not merely
similar**: `SynthEditApp`'s constructor is what assigns `theApp`, and `TideApp`
derives from `CSynthEditAppBase`, so `theApp` was always null in a TIDE process.
`SafeMessagebox` was already a no-op on its own `if (theApp)` guard; the sole
TIDE-reachable caller of the other two, `MfcDocPresenter.cpp:1244`, reads
`theApp && theApp->isMoonbaseEnabled() && !theApp->licenseIsActive()` and was
already dead on the first term; and both predicates are `return false` without
`SE_MOONBASE_SUPPORT`, which TIDE never defines.

| Release, arm64 | before | after |
|---|---|---|
| `SynthEditApp::` symbols | 48 | 2 (the stubs) |
| `SynthEditApp::InitInstance` | present | gone |
| the four licensing/activation symbols | present | gone |
| binary | 8,627,808 B | 8,586,976 B |

**Learned:**

1. **`nm | c++filt` on the built plugin is the right verifier for this class of
   item, and it is nearly free.** Every claim above is one command. It settled
   in minutes questions the note had been reasoning about for two runs — and it
   caught that stage 3's proposed route was a no-op *before* I spent a run
   implementing it. S1a's screenshot-hash trick and this are the same idea:
   measure the artifact, not the source.

2. **"Do the TIDE-side part" is a real instruction, but you have to go looking
   for the TIDE-side part.** My first read said S1b was 100% gated, same as P4.
   It wasn't: TIDE's *own* CMakeLists was importing a gated file, and that is
   TIDE's decision to unmake. Before concluding an item has no TIDE side, check
   what `SE16/SynthEditSem/CMakeLists.txt` is pulling in from `../SynthEdit2/`.

3. **A1's soundcard prediction is confirmed, and it is worse than predicted.**
   The trio is not merely compiled in, it is *registered* — each has its
   `se_static_library_init_*` and `__GLOBAL__sub_I_*`. `ug_soundcard_out.cpp:10`
   registers "Sound Out" under Input-Output with help text about your speakers
   and about non-registered SynthEdit being limited to 2 channels. Also
   confirmed TIDE uses the **non-JUCE** arm (`ug_filter_sv`, `ug_test_tone`
   present; `OscillatorNaive` zero symbols). Filed **S8** — no TIDE-side fix
   exists, both arms are in `UgDatabase.cpp`.

4. **`CSynthEditAppBase::MonitorFileSystem` survives my change** and is still in
   the binary, with `UpdateLiveModules` and `getLiveModuleUpdateStagingFolder`.
   Removing the `SynthEditApp` caller removed the only thing that *started* the
   thread, not the thread function — it is a `CSynthEditAppBase` member and
   `TideApp` derives from that. Don't read the shrunken symbol count as "the
   watcher is gone".

5. **SynthEditCL does not build on macOS**, and has nothing to do with TIDE. It
   compiles and links, then `CodeSign` fails on
   `Contents/MacOS/Resources/Prefabs/Controls/Button Small2.syntheditprefab` —
   `stage_prefabs_SynthEditCL` puts data under `MacOS/`, where `codesign`
   insists everything is code. Verified pre-existing by stashing my change and
   rebuilding: identical failure. So this run **cannot** honour "leave
   SynthEdit, SynthEditCL and TIDE all building" for SynthEditCL, and is saying
   so rather than assuming. Filed **P6**. I did not fix it — it is SynthEdit's
   own build config, not TIDE's and not on either list.

6. **My working copy was 48 commits stale and sitting on an already-merged
   branch** (`tide/mac/g2-run-prompt-permissions`, PR #4, merged 2026-08-06).
   `git fetch` alone would not have shown it — I only caught it because
   BACKLOG.md still listed items that PRs #5–#17 had closed. **Start every run
   with `git checkout main && git reset --hard origin/main` in TideSynth and
   `git merge --ff-only origin/master` in SE16**, then read BACKLOG. Reading a
   stale BACKLOG is how two machines take the same item.

**Next:** S1b's remainder is genuinely blocked on **C0** and should ride along
with **C4** rather than be attempted standalone — (b) and (c) touch exactly the
files C3 and C4 move, so doing them first means doing them twice. **S8** is the
most valuable unblocked-by-nothing-but-C0 finding and pairs with it. For a
macOS run with nothing gated available: **S6** (delete the dead iOS `.sem`
artifact) is mac-platform, ALLOWED, and small; **D1** wants the
"can an AUv3 open a URL" question answered, and the note says only a macOS box
can answer it. **P6** is mac but is SynthEdit's, not TIDE's — ask Jeff first.

**Branch/PR:** TideSynth `tide/mac/S1b-compile-out-scan`; code in `SE16`
`40b6008ee` on the same branch name, **pushed but not merged** — per S1a's
warning, a weekly run's SE16-side work does not land itself, so someone must
merge `SE16` `tide/mac/S1b-compile-out-scan`.

---

## 2026-08-08 — jeff — decisions: queue order, artifact naming, X4 closed (interactive session, not a scheduled run)

**Did:** Three rulings and the website copy, all straight to `main`.

**1. The carve-out section now sits ABOVE "Ready now" in BACKLOG.md.** This is a
behaviour change, not tidying. The run prompt says take *the topmost unblocked
item matching your platform*, reading the file top to bottom — so with "Ready
now" first, the next machine to wake would have taken **N1** (the rename, `any`)
while **C1b** sat `win`-only below sixteen rows. The critical path is
C1b→C2→…→C7→V1 and nothing else unblocks macOS, iOS, Linux and the acceptance
test. **If you reorder sections in this file, you are reprioritising the whole
fleet** — that is worth knowing before someone tidies it back.

**2. Artifact naming, settled in two passes the same day. The final answer is
three forms, not two** — the first version of this entry said two, and said the
underscore was load-bearing. It was not; see below.

| | Form | Where |
|---|---|---|
| Display | `TIDE Rack` (space) | plug-in name in a DAW, installer titles, website |
| Shipped files | `TIDE-Rack` (dash) | binaries, bundles, release assets |
| CMake targets | `TIDE_Rack` (underscore) | internal only, never shipped |

So `TIDE-Rack-Windows.exe`, `TIDE-Rack-macOS.pkg`, `TIDE-Rack-Linux.tar.gz`,
`TIDE-Rack.vst3`. Applied to [docs/distribution.md](docs/distribution.md) now
rather than at R2 time, because **a space in a shipped filename cannot be fixed
later**: R6's design is permanent `releases/latest/download/<asset>` permalinks,
and a space would be `%20` in every one of them forever. Also fixes the P2
annoyance where `TrackFX_AddByName` needed the exact filename.

**Why it changed within the hour, because the reasoning generalises.** The first
pass chose `TIDE_Rack` and justified it as "less brittle for scripting". Jeff
asked whether all-dashes was cleaner, and it is: **the underscore was defending
against *spaces*, and a dash defends identically.** Once spaces are out, `_` vs
`-` is pure style — and the mixed form `TIDE_Rack-macOS.pkg` is the one genuinely
bad option, because it implies `_` means "inside the name" and `-` means "field
boundary", a distinction nothing in the toolchain consumes. Two things then
decide it: these filenames become **visible link text** on the no-JS page (R6)
and underscores get swallowed by link underlining where dashes never do, and
all-dash is the near-universal release-asset convention
(`surge-xt-win64.exe`). The generalisable bit: **when a constraint is satisfied
by two options, stop calling one of them load-bearing.**

**CMake targets are the exception, and it is a real one rather than a
compromise.** `SynthEditSem/CMakeLists.txt:90` builds them as
`${PROJECT_NAME}_${kind}`, so a dashed project name produces `TIDE-Rack_VST3` —
the mixed form, in the one place you cannot avoid it. Targets are never shipped,
so they stay `TIDE_Rack` / `TIDE_Rack_VST3` and `OUTPUT_NAME` carries the dashed
artifact name. Loose end for whoever does N1(a): `:45` also passes
`PROJECT_NAME` into `gmpi_plugin.cmake`, which probably feeds the bundle name
and possibly an Info.plist identifier, so `OUTPUT_NAME` alone may not cover it.

This closes N1(b). N1(a) is explicitly deferred to **after C7**: C2–C7 are
already rewriting the same build files, and nothing has shipped under either
name, so renaming mid-carve-out doubles the conflict surface for nothing.

**3. X4 closed WONTFIX** — leave the six `GIT_TAG origin/main`s alone. Jeff's
reasoning: CI builds from a fresh download, so `origin/main` resolves to real
current `main` there; the freeze only bites a cached `_deps` tree, and pinning
six shared dependencies is a bigger change than the problem. Added `WONTFIX` to
the file's status legend, since this is the first one and a bare "closed" row
invites re-filing.

**I had this wrong, and the correction is worth more than the original
finding.** I argued the residual risk was the developer boxes, since X3's
failure happened on one. Jeff's reasoning, added the same day: **`FetchContent`
is not how these dependencies are meant to be developed against at all.** The
SynthEdit-family repos are not third-party libraries needing occasional version
bumps — they implement a large part of the application's own functionality and
change daily, so the intended local workflow is to bypass `FetchContent`
entirely and point CMake at working copies you can edit, push and pull as you
go. The `*_FOLDER_OVERRIDE` variables are **the normal path on a dev box, not an
optimisation**.

Checking that against `SE16/CMakeLists.txt` makes it airtight — the
dependencies split exactly along that line:

| | Override? | Changes |
|---|---|---|
| `SynthEditLib`, `GMPI`, `gmpi_ui`, `GMPI_Wrappers` | **yes** | daily |
| `AudioUnitSDK` (`:184`), `clap` (`:200`), `clap-helpers` | **no** | rarely |

Everything that changes often has an escape hatch; everything without one is a
stable third-party SDK where a frozen checkout is harmless. So there is no gap
worth pinning six shared dependencies to close.

**Which means X3 was not a freeze bug — it was a missing override.**
`SynthEditSem/CMakeLists.txt` had `GMPI_UI_FOLDER_OVERRIDE` pointed at the local
repo while `GMPI_WRAPPER_FOLDER_OVERRIDE` was blank, so one sibling came from
disk and the other from a stale clone. The asymmetry was the defect; the freeze
just made it permanent.

**And the detection already exists — nobody was reading it.** Every one of these
prints `Using local <X> folder` or `Fetching <X> from github` at configure time.
An unexpected `Fetching` on a SynthEdit-family repo means a `-D` was forgotten
and the build is against stale code. Written into
[docs/building.md](docs/building.md), which is where someone actually configures
a tree, rather than left in a closed backlog row nobody will read. Fallback if
it still looks impossible: `git log -1` in `build/_deps/<dep>-src` before
believing your own source tree.

**4. Website copy carries the rename.** Leads with TIDE Rack, states the
relationship once and early — *"TIDE Rack is the first plugin from TIDE Synth —
hence the address"* — because a reader who typed `tidesynth.com` and landed on
something called TIDE Rack needs that resolved on the first screen. Not
restructured into an org page with a product list: there is one product, and
doing it now would make it a page about nothing. Deployed and verified live.

**Learned:** **no bare "TIDE" is left in the visible copy, and that is a rule
now, not a tidy-up.** "TIDE" alone is ambiguous once it prefixes both a product
and an organisation. Checked by script (7 × "TIDE Rack", 2 × "TIDE Synth", zero
bare) rather than by eye, and written into `website/README.md` so the next
editor does not reintroduce it. The repo links still say `TideSynth` and should
— that is the organisation's repo, the same answer the carve-out gave for
`SynthEditLib` — and both the HTML comment and the README say so, because it
looks exactly like an oversight someone would helpfully "fix".

**Next:** **C1b**, `win`, and it is now genuinely the topmost item the Windows
box will pick up.

**Branch/PR:** straight to `main` at Jeff's direction.

---

## 2026-08-08 — jeff — decision: the product is TIDE Rack; donation link live (interactive session, not a scheduled run)

**Did:** Two things. Shipped the donation link — the last `TODO(jeff)` on the
website — and recorded a product naming ruling:

> **The product is TIDE Rack**, named in the vein of VCV Rack. **TIDE Synth is
> the organisation**, which may release more than one plugin; TIDE Rack is the
> first. **The domain stays `tidesynth.com`** — already paid for, and it now
> reads as the organisation's site, which is consistent.

Landed in [PLAN.md](PLAN.md) as a "Naming" section ahead of "What TIDE Rack is",
following the channel rule the fixed-module-set and no-user-skins entries set:
rulings go in PLAN, enforcement goes in BACKLOG. Enforcement is **N1**.

**Donation link, done properly this time.** `https://ko-fi.com/TideRack`, a plain
`<a href>`, plus `.github/FUNDING.yml` (`ko_fi: TideRack`) for the repo Sponsor
button. Ko-fi was chosen over GitHub Sponsors because this page's audience is DAW
users, not developers, and **GitHub Sponsors requires the donor to have a GitHub
account**; `ko_fi:` in FUNDING.yml still earns the repo button, so both surfaces
came without GitHub Sponsors enrolment.

**Learned:**

1. **Verify the handle resolves before committing the href — every time.** This
   link failed twice before on exactly that: `<a href="#donate-url-tbd">` shipped
   as a real link that went nowhere (`453721e`), and the platform-choice commit
   had to keep the page as plain text because the account did not exist yet.
   Third time it was fetched first: HTTP 200, landed on `/TideRack` rather than
   redirecting to `/`. **The redirect *is* the test** — Ko-fi bounces unclaimed
   handles to its homepage, so `landedOn === "/"` means free. Confirmed against
   two known-existing creators as a control, the same discipline P4c's liveness
   probe used: an availability check that cannot fail proves nothing.

2. **The Ko-fi page title is "Support Jef", not TIDE Rack.** The signup display
   name is separate from the page URL slug, and only the slug was set. A donor
   clicking "Donate to TIDE on Ko-fi" currently lands on a page with no mention
   of TIDE. Jeff's to fix in Ko-fi settings; nothing in this repo can.

3. **`.github/FUNDING.yml` was deliberately withheld until the handle existed.**
   A FUNDING.yml naming an unclaimed handle renders a Sponsor button that 404s —
   the same dead-link failure, relocated from the website to the repo page.

4. **The rename is not a search-and-replace, and N1 says so in bold.** "TIDE"
   is currently a product name, an organisation name, two CMake targets, a
   **fixed four-character** vendor code (`TideApp::getVendor4charCode()`), and a
   filename prefix. They do not all move together, and one of them cannot grow.

5. **Settle the release asset names before R2–R6 ship anything.** R6's whole
   design is static `releases/latest/download/<asset>` permalinks, which is what
   lets the no-JS page avoid a version bump. Renaming a published asset breaks
   that permalink. The rename is free today and expensive after the first
   release — the same shape of argument as the repo-naming question, which went
   the other way.

**Next:** N1 wants costing and probably splitting; its cheap half (docs, README,
website copy) is genuinely cheap, its expensive half (targets, artifacts, asset
names) each needs a decision first. P5 is now a subset of it. Engineering queue
is otherwise unchanged: C1b then C2, both `win`.

**Branch/PR:** straight to `main` at Jeff's direction.

---

## 2026-08-08 — jeff — decision: carve-out APPROVED (interactive session, not a scheduled run)

**Did:** Jeff approved **C0**, the decision ~20 backlog items were waiting behind:

> **The carve-out is approved. Keep as much `ExportAsPlugin` code private as
> practical.**

C2–C7 are now TODO, and so are S1b's remaining stages and S8. Also set the
signing credentials up (see the R1 row) — separate thread, same session.

**Learned — the direction is not a restatement of the existing boundary, and
checking it found a hole in the plan.**

1. **The carve-out plan, followed literally, would have published the export
   implementation.** [docs/carve-out.md](docs/carve-out.md) says
   `ExportAsPlugin.{cpp,h}` stays private *and* says "the ~120 files in
   `EditorLib/CMakeLists.txt`" move public. Those two files are on that list,
   unconditionally, at `EditorLib/CMakeLists.txt:113-114` — and stage 6 moves
   that CMakeLists itself. So the two halves of the document contradicted each
   other, and the half that would have been executed is the wrong one. Filed as
   **C1b**, ordered before C2.

2. **TIDE does not ship the export code today — verified, not assumed.**

   ```
   0:000> x TIDE_VST3!*ExportAsPlugin*
             (nothing)
   0:000> x TIDE_VST3!*SkinMgr*getSkin*
   00000001`8017e5e0 TIDE_VST3!SkinMgr::getSkin
   ```

   The control query is what makes the empty result mean something. The object
   *is* built (`ExportAsPlugin.obj`, 2,319,260 bytes Release) and the symbol is
   `External` in `EditorLib.lib` — but the linker never pulls it, because nothing
   in TIDE references it. The only public-side mentions are declarations, which
   generate no reference. **The commercial boundary already holds at link time,
   by accident.** C1b makes it hold by construction, and costs TIDE nothing.

3. **`SynthEditCL` stays private** — that is the direction applied to the plan's
   second open question. `SynthEditCL/main.cpp:1230` calls `ExportAsPlugin`
   directly, so a public CLI means publishing the export code or splitting the
   tool in two. TIDE embeds patches rather than exporting them, so it needs
   neither.

4. **A doc correction worth not rediscovering:** `CContainer.h` mentions
   `ExportAsPlugin` **twice** — a plain declaration at `:23` as well as the friend
   declaration at `:32`. The plan named only the friend declaration. Both are
   declarations without a definition, so the header still moves unaltered, but a
   grep for one line will mislead.

5. **Stale framing removed rather than left to mislead.** The plan's licence
   question offered "GPLv3 vs MIT/BSD" as a competitive-moat decision; the actual
   answer was ISC, arrived at by matching the sibling repos — a third option the
   question never listed. Struck through with a note saying so, because a reader
   would otherwise assume the original framing was weighed and rejected.

**Repo naming — answered the same day: keep `SynthEditLib`.** No rename when the
editor moves in, no redirect, no follow-up item. Write it as `SynthEditLib` in
build instructions, CI and the README without hedging.

**So every open question on the carve-out plan is now closed** — C0 approved,
licence ISC, `SynthEditCL` private, name unchanged. Nothing on it is waiting on a
decision; only on the work.

**Next:** **C1b**, then C2. Both `win`. C1b is small and is the one stage that
must not be skipped or reordered.

**Branch/PR:** straight to `main` at Jeff's direction.

---

## 2026-08-08 — windows — P4c

**Did:** Reproduced the P4 resize crash on demand, A/B'd the fixes against it,
and left the reproduction behind as a permanent test —
`GMPI_Wrappers/tests/win_editor_resize_host.cpp`. The resize crash is now
**fixed-by-test**. Filed **P7** (the X11/macOS resize paths, still unaudited).

STEP 1 clear: `gh issue list` returns nothing, no labels. STEP 2: `git fetch`
brought two new remote branches, `tide/linux/X3-vst3-moduleentry` (PR #19) and
`tide/mac/S1b-compile-out-scan` (PR #18) — so X3 and S1b are taken, and P4c was
both topmost and unclaimed. Claimed it, pushed the DOING mark, then started. My
working copy was already at `origin/main` this time, so the four documents I read
were current; the Linux run's "fetch before you read" warning still stands for a
box that has been idle longer.

**Result — the A/B, three runs of each configuration:**

| `reSize` fix (P4a) | `checkSizeConstraint` fix (P4b) | `onSize(0,0,2178,32672)` |
|---|---|---|
| off | off | **`0xC0000005`, 3/3** |
| **on** | off | survived, 3/3 |
| **on** | **on** | survived, 3/3 |

P2 saw the original 3/3. This reproduces it 3/3. Frames 0 and 1 match the
minidump exactly — only the host frame differs:

```
TIDE_VST3_prefix!gmpi::hosting::DrawingFrame::reSize+0x7c
00007ffe`4f7658ac 488b01        mov  rax,qword ptr [rcx]  ds:00000000`00000000
00007ffe`4f7658af ff9050020000  call qword ptr [rax+250h]
TIDE_VST3_prefix!wrapper::SEVSTGUIEditorWin::onSize+0x1e
win_editor_resize_host!main
```

`ExceptionCode c0000005`, `Parameter[0] 0` (read), `Parameter[1] 0`. A vtable
load off a null COM pointer then a virtual call through it — that is
`d2dDeviceContext->SetTarget(nullptr)`, the first use after `SetWindowPos`, i.e.
the "use" half of the time-of-check/time-of-use bug P4 described. Verification
section added to [docs/p4-resize-crash.md](docs/p4-resize-crash.md).

**Learned — five things, and the first two are the ones that matter:**

1. **P4c asked the wrong question, and answering it as asked would have failed
   again.** The item said: reproduce the *DAW's* state — try the old REAPER
   config, a different DPI layout, dragging the window edge. But the DAW was
   never the experiment. It is just a thing that once passed a bad rect, and the
   minidump had already recorded *which* rect. So the answer was not to recreate
   REAPER's state, it was to stop needing REAPER: load the plugin, attach the
   editor to an `HWND`, call `onSize({0,0,2178,32672})` directly. That is ~330
   lines and it crashed on the first run. **When a dump has captured the input,
   replay the input, not the environment.** Every guess listed in P4c and in the
   note's "untested guesses" was a dead end, and none of them was needed.

2. **P2's `MoveWindow` lead was a red herring — and it nearly cost this run
   too.** P2 recorded that `MoveWindow` "did not resize" the plugin window and
   crashed anyway; P4 reasoned from that to some unusual window state, and P4c
   inherited it as *the* lead. The truth: **`attached()` creates a child window
   inside the HWND the host hands over, and `DrawingFrame::reSize` calls
   `SetWindowPos` on that child.** The parent's client rect never moves. P2
   measured the parent. There was no strange state and no mystery to hunt. Use
   `GetWindow(hwnd, GW_CHILD)`. My first harness draft made the identical
   mistake and reported `editor live: NO` against a build that was demonstrably
   fine — which is the only reason I caught it.

3. **A "did not crash" result needs a liveness proof, or it is worthless.** The
   D2D device is created lazily on first paint. With no device, the *unfixed*
   `reSize` returns at its own first test — which looks exactly like the fix
   working. Any harness that skips this reports a false pass, and I think this is
   a real candidate for what the earlier REAPER A/B was actually measuring. The
   test therefore does a benign resize first and checks the child window
   **adopted** it: a resize that took effect proves `reSize` got past
   `if (d2dDeviceContext && ...)` and reached `SetWindowPos`, so the device is
   live and the crash path is reachable. If not, it exits **3 INCONCLUSIVE**, not
   0. This is the same discipline as S1a's screenshot hash — build the verifier
   so it can *fail*, not just so it can pass.

4. **The Release PDB from P4 paid for itself immediately.** The whole crash
   symbolised out of a Release build; no Debug repro was needed. `cdb -g ... -c
   ".symopt-0x100; g; .reload /f <module>; .lines -e; .ecxr; u . L3"` run
   *live* on the harness is quicker than hunting a minidump — and note
   `%LOCALAPPDATA%\CrashDumps` did **not** catch my exe (LocalDumps looks to be
   configured for `reaper.exe`, not globally), so do not count on a dump
   appearing.

5. **A/B by reverting the file with git, not by editing the fix out.**
   `git checkout <pre-fix-sha> -- <file>` → rebuild → test → `git checkout HEAD --
   <file>`. Exact, reversible, and it cannot leave a hand-mangled fix behind.
   Both shared repos were clean this time (no CRLF churn), and are clean again.

**Where the code went.** `GMPI_Wrappers` branch
`tide/win/P4c-resize-regression-test`, commit `fd38ee8`, pushed — **not merged**.
Three files: the new test, plus a platform gate moved out of
`wrapper/CMakeLists.txt` into `tests/CMakeLists.txt`. That directory was
Linux-only and `pkg_check_modules`'d unconditionally, so Windows could not
configure it at all; it now picks targets per platform. Still off by default
(`-DGMPI_WRAPPERS_BUILD_TESTS=ON`), so no plugin build changes. The test needs no
SDK sources — `ClassName_iid` is a header-only constant, so pluginterfaces
headers plus `user32` is the entire dependency, one translation unit.

**State I am leaving this machine in, deliberately:** `C:\SE\GMPI_Wrappers` is
left **checked out on that branch** rather than back on `main`, so the test stays
runnable here. That is safe — the only differences from `main` are the new test
file and a CMake gate that is off by default, so nothing SynthEdit or any other
GMPI plugin builds is affected. But it is a shared working copy, so if you are
doing unrelated work in it, `git checkout main` first. The branch is pushed;
nothing is lost either way. As S1a found with the stranded S4 branch, **a weekly
run's shared-repo work does not merge itself** — this one needs merging too.

**Builds** (Release, `C:\SE\build-tide-p1`, now configured with
`GMPI_WRAPPERS_BUILD_TESTS=ON`):

| Target | Exit | Warnings |
|---|---|---|
| `TIDE` | 0 | 0 |
| `TIDE_VST3` | 0 | 0 |
| `SynthEditCL` | 0 | 0 |
| `win_editor_resize_host` | 0 | 0 |

**Next:** **P7** — the X11 and macOS resize paths (`DrawingFrameX11.cpp:920`,
`DrawingFrameMac.mm:434`) have never been audited for the same
check-then-re-enter pattern, and nothing says they are clean. `x11_editor_host.cpp`
already attaches an editor, so porting the probe is small; read the liveness trap
in point 3 first or the Linux result will be a false pass.

The remaining `win` engineering item is **P3** (the MFC/`afxres.h` dependency).
S1b and X3 are in flight on the other two boxes. With the resize crash now
genuinely fixed, **V1 is hand-testable on Windows** — the editor can be resized
without killing the DAW, which was the blocker P4 left behind.

**Branch/PR:** `tide/win/P4c-verify-resize-crash` in this repo; the test itself is
`fd38ee8` on `tide/win/P4c-resize-regression-test` in `JeffMcClintock/GMPI_Wrappers`,
pushed as a branch, not to `main`.

---

## 2026-08-07 — windows — distribution plan (at Jeff's request, interactive)

**Did:** Wrote [docs/distribution.md](docs/distribution.md) — installers on all
four platforms plus website downloads — and filed BACKLOG **R1–R6** as a new
"Release & distribution" section, all blocked on V1 except R1. Also, at Jeff's
direction, removed every SynthEdit mention from the website's rendered text
(commit `d0bf3ef`, straight to main after losing two merge races in a row —
see below).

**Result — the plan in four lines:**

1. Tag `v*` → one GitHub Release per version, constant asset names
   (`TIDE-Windows.exe`, `TIDE-macOS.pkg`, `TIDE-Linux.tar.gz`) + SHA256SUMS.
2. The website links `releases/latest/download/<asset>` — static permalinks
   that always point at the newest release, so the no-JS page never needs a
   version bump.
3. iOS is App Store only, arriving with M2; plain text link, no Apple badge
   image (it would be the page's first external request).
4. CI automation waits on C7 (public runners cannot link private EditorLib);
   until then each box builds locally and `gh release upload`s — same release
   page, same permalinks, working from the first v0.1 build.

**Learned — SynthEdit's shipping infrastructure, located by reading `SE16`:**

- **Windows signing is Azure Trusted Signing and already paid for** — account
  `SynthEditTrustedSigning`, profile `SynthEditCertificateProfile`, endpoint
  in `SE16/SynthEdit_store_win.yml:205-207`. The open question is naming, not
  money: the cert subject is the publisher users see in UAC, so whether TIDE
  ships under SynthEdit's publisher name is R1(a), Jeff's call.
- **Apple identity + DMG/notarization pipeline exist** —
  `SE16/SynthEdit_cmake_mac.yml:185-199`, `create_dmg.sh`,
  `$(APPLE_CERTIFICATE_SIGNING_IDENTITY)`.
- **Inno Setup is the Windows installer precedent** —
  `SE16/SynthEdit2/installer/SynthEdit2.iss` and `SynthEditCL.iss`.
- **All of it runs in Azure Pipelines in the private repo.** The recipes port
  to GitHub Actions; the *secrets do not follow* — recreating them in the
  public repo is R1(d), and signing must never run in PR workflows (tag-push
  only) or fork PRs could reach the secrets.

**Process note — three merge races in one afternoon.** Jeff merges PRs within
seconds of their appearing. Twice, a follow-up commit pushed to an open PR's
branch landed moments *after* the merge, silently recreating the just-deleted
branch instead of joining the PR (git happily resurrects a deleted remote
branch on push; nothing warns). Recovery both times: cherry-pick the stranded
tip onto a fresh base, delete the stray branch. For the second one — a one-file
website edit Jeff had directly ordered — it went straight to main instead, per
his standing sole-developer preference. Rule of thumb for future runs: before
pushing a follow-up to a PR branch, `gh pr view <n> --json state` first;
scheduled runs should keep using PRs regardless.

**Next:** R1 is the only distribution item that can move now and it is Jeff's.
Engineering queue unchanged: P4c, then S1a (and S7 wants its runtime check).

**Branch/PR:** `plan/distribution`

---

## 2026-08-07 — windows — S1a

**Did:** Removed the module scan and cache from TIDE — the `semFolder`
assignment, S4's `isSemFolderOverridden` flag, and `LoadOrScanModuleData()`
are gone from `TideApp::InitInstance`. `SE16` commit `d67bdfbab`, pushed to
master. Before starting, **merged the stranded S4 branch**: the Linux run's
one-line fix sat unmerged on `SE16` branch `tide/linux/S4-sem-cache-clobber`
while BACKLOG showed S4 done — the run obeyed "never push to main" and nobody
merged the branch. Merged as `d28e02007`, branch deleted. **Check `SE16` for
unmerged `tide/*` branches; a weekly run's SE16-side work does not land
itself.**

**Result — §9, adapted, passes.** The recipe says "point `ModulePath` at an
empty directory", which is impossible in TIDE: `Application.cpp:139` returns
the user Documents folder for *every* settings key. So the test became:
build with the scan deleted, delete TIDE's own cache file, run in the portable
REAPER harness, screenshot the module browser, and compare against the same
screenshot from the scanning build taken minutes earlier. Verdict:

- **Pixel-identical browsers** — the before/after PNGs have equal SHA-256
  hashes. The scan contributed nothing the browser shows.
- **Zero filesystem writes** — the baseline (scanning) run recreated TIDE's
  override cache within seconds; the descanned run wrote nothing under
  `ProgramData\SynthEdit` at all.
- TIDE, TIDE_VST3, SynthEditCL all build, exit 0, no warnings.

Category tree observed both times: All, Controls, Conversion, Diagnostic,
Effects, Experimental, Filters, Flow Control, Input-Output, Logic, Math, MIDI,
Modifiers, Old, Special, Waveform. **Not audited:** per-category contents —
A1's prediction (soundcard trio present, modern SEM modules absent) needs the
categories expanded one by one, and that inspection belongs with S1b's module
curation anyway.

**Learned:**

1. **S4 verified at runtime on Windows, in passing.** The baseline run (scan
   still in, S4 flag in) wrote `Plugin-Cache-16-override-a08c134c04a8099a.xml`
   and left `Plugin-Cache-16.xml` untouched — the Linux fix does on Windows
   exactly what its author proved by linking `SemCacheName()` on Linux.

2. **S7's write confirmed at runtime, with a nuance.** The baseline run
   touched `Public Documents\SynthEdit Projects\.resource_version` from inside
   the DAW — the skins machinery does write outside the sandbox (constraint 4).
   Nuance: the *second* run wrote nothing — the version file matched, so the
   copy was skipped. S7's offender writes on version mismatch or first run,
   not every launch. Removing the scan did **not** remove this; S7 stands.

3. **`Plugin-Cache-16.xml` was rewritten today by another agent, mostly.**
   It shrank from ~1 MB (P2's measurement) to 71,621 B at 11:22:04. A second
   Claude session visible on this desktop ("JUCE Linux development environment
   setup") says it *set the cache aside to force a rescan and kept a backup in
   its scratchpad*. So the shrink is explained, and the original cache is
   recoverable from that session — but note 71,621 B is byte-for-byte the size
   TIDE's own cache came out at, so whatever rewrote the shared file was
   running TIDE's module set. If the desktop app's browser looks thin, restore
   from that session's backup.

4. **`Set-Content -NoNewline` on a line array concatenates the lines.** A
   quick sed-style status flip flattened BACKLOG.md to one line, and it was
   committed and pushed before being caught. Use the Edit tool for file edits,
   or `-replace` on the raw string from `Get-Content -Raw`. The claim commit
   was amended; no history damage beyond a force-with-lease on the claim
   branch.

5. **Screenshot comparison is a strong, cheap verifier** — but only because
   the harness pins everything else (same window size, same REAPER, same
   track). Equal SHA-256 on two PNGs taken across a rebuild is much stronger
   than "looks the same to me", and it costs one `Get-FileHash`.

**Next:** **S1b** — now unblocked, and it inherits the per-category audit
(expand Input-Output, confirm the soundcard trio, curate via
`SE_EXTRA_STATIC_FILE_CPP` per A2). P4c remains the other open `win` item.
S7's fix is now the only remaining known write (`.resource_version` / skins
copy) from a TIDE instance.

**Branch/PR:** `tide/win/S1a-stop-scanning`; code in `SE16` `d67bdfbab`
(pushed to master at Jeff's standing direction for interactive sessions —
scheduled runs should still branch).

---

## 2026-08-07 — jeff — decision: no user skins (interactive session, not a scheduled run)

**Did:** Recorded a product ruling:

> **No user skins in TIDE. The default appearance ships in the plugin's
> resources — nothing skin-related written to the user's disk.**

Landed as [PLAN.md](PLAN.md) **constraint 8**, following the channel rule from
the 2026-08-06 fixed-module-set entry: rulings go in PLAN, enforcement goes in
BACKLOG. Filed **S7** for enforcement. Note the constraint is stricter than
PLAN's v0.1 list, which merely *defers* skinning — user skins are now out
permanently.

**Learned — the ruling names a live behaviour, not a hypothetical.** Five
minutes of grep while filing S7 found: `SkinMgr`'s constructor
(`SE16/SynthEdit2/SkinMgr.cpp:27-30`) points at
`<CommonDocuments>\SynthEdit Projects\skins\`, `setSkinFolder` (`:47+`)
**recursively copies the built-in skins there on first use**, and
`CContainer.cpp:97` reaches `SkinMgr::Instance()` — `CContainer` being squarely
in TIDE's document path. So the first container a TIDE instance constructs
probably writes the shared skin set onto the user's drive, in a DAW, on every
machine. Unverified at runtime (S7's first job), but the static chain is
direct. It is also another instance of the S4 pattern: TIDE silently sharing
mutable on-disk state with the desktop SynthEdit app.

**Next:** S7 wants the runtime check before any fix — same discipline as S1a's
§9. The fix may split like S4 did: a TIDE-side part in ALLOWED code, and a
gated `SkinMgr` part to file rather than reach for.

**Branch/PR:** `plan/no-user-skins`

---

## 2026-08-07 — linux — S4

**Did:** Fixed S4 — one line in `SE16/SynthEditSem/TideApp.cpp` (ALLOWED), plus a
comment explaining why it is there. `TideApp::InitInstance` now sets
`BundleInfo::instance()->isSemFolderOverridden = true` immediately after the
existing `semFolder` assignment. Nothing shared was touched. Spun off **X3**.

STEP 1 clear: `gh issue list` returns nothing at all, no labels. STEP 2: `main`
was the only remote branch and there were no open PRs, so nothing was claimed.
P4c and S1a are `win`; S1b's own text blocks it behind S1a; **S4 was the topmost
`any` item**. Claimed it, pushed the DOING mark, then started.

**Read this first: my working copy of TideSynth was five merged PRs stale.**
`git fetch` moved `main` from `a6f1e7f` to `6f3ca8f` — PRs #7 through #11, i.e.
P1, P2, P4, P4a, P4b, the "free + donation-supported" PLAN section and the CRLF
correction. Everything I had read up to that point (PLAN, BACKLOG, JOURNAL) was
the pre-P1 version, and I re-read all of it after fetching. **Fetch before you
read, not after** — the run prompt puts "read the four files" ahead of the
`git fetch` in STEP 2, which is the wrong order on a box that has been idle a
week.

**Result — the fix, and it is verified rather than reasoned.**

The chain, re-checked link by link rather than taken from S1's journal:

| Step | File:line | What |
|---|---|---|
| TIDE sets its factory folder | `SynthEditSem/TideApp.cpp:109` | `semFolder = GetHomeDir() + L"modules\\"` |
| …and leaves the flag false | `SynthEditLib/.../BundleInfo.h:63` | `isSemFolderOverridden = false` |
| so the suffix is dropped | `SynthEdit2/ModuleFactory_Editor.cpp:188` | `if (bi.isSemFolderOverridden)` never taken |
| desktop SynthEdit does the same | `SynthEdit2/SynthEditApp.cpp:133` | sets `semFolder`, never sets the flag |
| both therefore name one file | `ModuleFactory_Editor.cpp:1168,1182,1135` | `<settings>/SynthEdit/Plugin-Cache-16.xml`, read **and** written |
| and TIDE gets there at instantiation | `SynthEditSem/SynthEditController.cpp:63` | `IController::initialize` → `app->InitInstance()` → `LoadOrScanModuleData()` |

That last row matters more than it looks: `InitInstance` runs from
**`IController::initialize`**, so merely *instantiating* TIDE touches the cache.
No editor, no GUI, no user action — a host's plugin scan is enough.

`GetHomeDir()` (`SynthEdit2/Application.cpp:203`) is the directory of the loaded
binary (`MP_GetDllFilename().parent_path()`), so TIDE's folder really is its own
bundle and really is a different folder from the app's `<home>/PlugIns/` — a
different folder writing the *same* cache file, which is exactly the hazard.

**How I verified it, since "it builds" proves nothing here.** The Steinberg
validator route died immediately (see X3 below), so instead I linked a 20-line
probe against the **real** `SemCacheName()` in `build/EditorLib/libEditorLib.a`
and called it with the flag both ways:

```
isSemFolderOverridden=false -> Plugin-Cache-16.xml
isSemFolderOverridden=true  -> Plugin-Cache-16-override-14603581876e07dd.xml
```

`Plugin-Cache-16.xml` is not hypothetical — it is sitting in
`~/.local/share/SynthEdit/`, 329,914 bytes, 491 `<Plugin>` entries and 35
`<Prefab>`s, last written 09:28 the same morning by a real SynthEdit run. Four
`-override-<hash>` siblings sit beside it from `SynthEditCL` runs, which is
independent evidence that the suffix mechanism works in production.

The probe is worth reproducing if you need to test anything in EditorLib without
a host — it took about ten minutes:

```
g++ probe.o stubs.o -o probe -Wl,--start-group \
    build/EditorLib/libEditorLib.a build/SynthEditLib/libSynthEditLib.a -Wl,--end-group
```

`--start-group` is the whole trick: EditorLib and SynthEditLib reference each
other, so a single left-to-right pass leaves `new_InterfaceObjectA/B/C` undefined
even though `platform_editor.cpp.o` is right there in the archive. I wasted a
link cycle stubbing those out with `nullptr`-returning fakes, which linked fine
and then **segfaulted in static init** — the ~157 self-registering modules build
their pin lists at load time and dereference what those factories return. The
only symbol that genuinely needs a stub is `SafeMessagebox`.

**Builds** (gcc 13.3.0, RelWithDebInfo, existing `~/SE/build` tree):

| Target | Exit | Notes |
|---|---|---|
| `TIDE` (GMPI) | 0 | zero warnings |
| `TIDE_VST3` | 0 | zero warnings |
| `SynthEditCL` | 0 | no recompile — confirms the change is TIDE-only |
| `SynthEditWayland` | 0 | ditto |

**Learned — things the next run should not have to rediscover:**

1. **TIDE already builds on Linux, and X1 is stale in one direction.** X1 is
   listed BLOCKED behind the carve-out, but `~/SE/build` is a configured tree
   that builds `TIDE.gmpi` and `TIDE_VST3.so` from a warm cache in **12 seconds**
   on gcc 13.3.0. Both bundle layouts exist
   (`build/SynthEditSem/TIDE_VST3.vst3/Contents/x86_64-linux/`). What is *not*
   done is CLAP — `SynthEditSem/CMakeLists.txt` has `set(FORMATS_LIST GMPI VST3)`
   and no CLAP entry, so half of X1 is real work and half is already sitting on
   disk. Also `~/SE/SE16` has recent Linux commits (`e8d190866` prefabs in the
   module list, `cf2d6de52` thumbnails), so somebody is actively running TIDE
   here.

2. **X3: the Linux VST3 cannot be loaded by any host, and it is a stale
   `FetchContent`, not a bug.** Steinberg's `validator` says *"The shared library
   does not export the required 'ModuleEntry' function"*, and `nm -D` on
   `TIDE_VST3.so` shows only `GetPluginFactory` and `InitDll` — the Windows pair.
   `libSynthEdit_VST3.vst3` in the same build tree exports `ModuleEntry` and
   `ModuleExit` as well. The reason: `SynthEditSem/CMakeLists.txt` leaves
   `GMPI_WRAPPER_FOLDER_OVERRIDE` **blank**, so TIDE fetches GMPI_Wrappers from
   GitHub instead of using `~/SE/GMPI_Wrappers`, and `build/_deps/gmpi_wrappers-src`
   is pinned at `032b4d5` — older than `9a2341d fix(linux) : export
   ModuleEntry/ModuleExit`, which is on that repo's `main`. `GIT_TAG origin/main`
   does not re-fetch on a later configure. Note `GMPI_UI_FOLDER_OVERRIDE` *is*
   set to the local `gmpi_ui` in the same cache, so the two sibling repos are
   sourced differently — that asymmetry is what hides the problem. Filed as X3;
   I did not fix it, because changing where TIDE gets its wrapper from is a
   build-policy call and S4 is one line.

3. **A negative result, so nobody chases it: TIDE does *not* scan the user's
   Documents folder.** I thought it did. `RefreshModuleData`
   (`Application.cpp:507`) calls `ScanFolder(getSettingString(L"ModulePath"))`,
   and `ApplicationBase::getSettingString` (`Application.cpp:139`) is a `// TODO`
   stub returning the user's documents folder — which would be a constraint 3
   violation. But `CSynthEditAppBase::getSettingString`
   (`SynthEditAppBase.cpp:1089`) **overrides** it and returns
   `settings.ModulePath`, and `settings` is a plain `ApplicationSettings` member
   (`SynthEditAppBase.h:118`) that is never loaded for TIDE, because
   `CSynthEditAppBase::InitInstance` is never called — the same omission S5 is
   about. So `ModulePath` is empty and the third-party scan is a no-op.
   **S5's omission is currently masking a worse problem than the one S5
   describes**: fix S5 by calling `InitInstance`, and TIDE starts scanning
   whatever the desktop app's `ModulePath` points at. Whoever takes S5 should
   read this paragraph first.

4. **The cache is only *written* when it is missing or stale.**
   `LoadOrScanModuleData` (`Application.cpp:469`) calls `RefreshModuleData` only
   `if (!LoadModuleData())`. So the damage is asymmetric and easy to miss in
   testing: on a machine that already has a cache, TIDE silently *reads* the
   desktop app's module set (pulling third-party descriptions into TIDE, which
   constraint 7 forbids); the clobbering write only happens after a version bump
   or a cache delete. Either half alone justifies the fix.

5. **`TideApp.cpp:109` still hard-codes `L"modules\\"` and I deliberately left
   it.** S1's journal flagged the trailing backslash as cosmetic. It is not quite
   cosmetic — but "fixing" it on Linux/macOS would turn a path that never exists
   into one that does, and TIDE would start actually scanning it for `.sem`/`.gmpi`,
   which is precisely what constraint 7 forbids. The right move is S1a's deletion,
   not a separator fix. Do not tidy this line.

6. **`isSemFolderOverridden` is named for its caller, not its effect.** Its
   comment (`BundleInfo.h:57-62`) says "set by USER intent (test harness;
   SynthEditCL's `-factorysemsfolder`)", which TIDE's case is not. But the flag's
   only reader is `SemCacheName()` (grep gives four hits total: two setters, one
   reader, one declaration), and what it actually selects is "give this factory
   folder its own cache file". I set it and explained the mismatch in a comment on
   the TIDE side rather than rewording the header, because `BundleInfo.h` is
   GATED. If the carve-out ever renames it, `TideApp.cpp` is a caller.

7. **This machine's installed task is still the pre-G3 prompt.** My STEP 5 has
   G2's ALLOWED/GATED split but says nothing about `gmpi_ui` or `GMPI_Wrappers`,
   which G3 resolved as ALLOWED on 2026-08-07. It did not affect S4 (nothing
   outside `SE16/SynthEditSem/` was needed), but it would have blocked me on X3
   had I tried to fix it. Linux still needs reinstalling from
   [docs/weekly-run-prompt.md](docs/weekly-run-prompt.md); per G2's note, Windows
   may too.

**Next:** **X3** is the highest-value Linux item and it is probably small — set
`GMPI_WRAPPER_FOLDER_OVERRIDE` or re-pin the fetch, rebuild, and confirm the
validator loads the module. It unblocks every runtime question on this box:
`GMPI_Wrappers/tests/x11_editor_host.cpp` is a real VST3 host that attaches the
editor and dumps pixels, so once TIDE loads, U1's measurements, V1's
save/reload and even P4c's resize path become testable on Linux instead of only
on Windows. After that, X1's CLAP half is genuine unstarted work.

S1a still wants its §9 check on a machine that can run TIDE — and after X3 that
could be this one, not just the Windows box.

**Branch/PR:** `tide/linux/S4-sem-cache-clobber` in this repo; the code change is
`8f5650e94` on a branch of the same name in `JeffMcClintock/SynthEdit`, pushed as
a branch, not to `master`.

---

## 2026-08-07 — windows — P4

**Did:** Diagnosed the host-killing resize crash down to file and line, from the
minidumps P2 left behind. Wrote [docs/p4-resize-crash.md](docs/p4-resize-crash.md).
**Did not fix it** — the entire fix is in `gmpi_ui` and `GMPI_Wrappers`, which
are outside my write scope; see "The scope problem" below. The one thing I did
change is `SE16/SynthEditSem/CMakeLists.txt` (ALLOWED), which now emits a PDB
for Release, as P4 itself asked.

STEP 1 was clear: `gh issue list` returns nothing, no issues of any label. STEP 2
— all seven PRs are merged and `main` is the only remote branch, so nothing had
claimed P4. Claimed it, pushed the DOING mark, then started.

**Result — the crash, in one sentence:** `DrawingFrame::reSize` checks that its
Direct2D device context is alive, calls `SetWindowPos`, and then uses the device
context — but `SetWindowPos` dispatches `WM_SIZE` *synchronously*, and that
handler releases the device. Time-of-check/time-of-use across a re-entrant Win32
call.

```
TIDE_VST3!gmpi::hosting::DrawingFrame::reSize+0x139
00007fff`24304ed9  mov  rax,qword ptr [rax]  ds:00000000`00000000   <- rax = 0
[C:\SE\gmpi_ui\backends\DrawingFrameWin.cpp @ 1418]
01  TIDE_VST3!wrapper::SEVSTGUIEditorWin::onSize+0x4c
[C:\SE\GMPI_Wrappers\wrapper\VST3\SEVSTGUIEditorWin.cpp @ 88]
02  reaper+0x405ae
```

Base `0x7fff24180000` → RVA `0x184ED9`, which is exactly the Debug RVA P2
recorded, so this is provably the same crash. The chain, step by step, is in the
note; the short version is `reSize:1406` checks → `SetWindowPos:1408` →
`WindowProc:592` `WM_SIZE` → `OnSize:1371` → `ResizeBuffers` fails →
`ReleaseDevice()` nulls both pointers (`DrawingFrameWin.h:376-377`) →
`reSize:1418` dereferences the null. Two defects, both needed:
**P4a** the stale check, **P4b** nothing clamps the size.

**Learned:**

1. **This machine *does* have `cdb.exe`, and P2's journal is wrong about that.**
   It is in the Store WinDbg package:
   `C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2603.20001.0_x64__8wekyb3d8bbwe\amd64\cdb.exe`.
   P2 concluded there was none after searching only `Windows Kits\10\Debuggers`
   (which holds just `dbghelp`/`dbgcore`/`srcsrv`/`symsrv` DLLs) and then lost
   time P/Invoking `dbghelp` from PowerShell. Search `WindowsApps` too. Nothing
   needed installing and the whole symbolisation took about five minutes.

2. **Two cdb flags are the difference between an answer and a wall of noise.**
   `.symopt-0x100` (clear `SYMOPT_NO_UNQUALIFIED_LOADS`) — without it `!analyze`
   prints the "you specified an unqualified symbol" boilerplate three times and
   resolves nothing. And `.reload /f TIDE_VST3.vst3` — the module loads
   *deferred*, so `lm` shows it with no symbols until you force it. My first run
   produced 340 lines of nothing because of these two.

3. **The Debug artifacts from P1 are still on disk and still match.**
   `C:\SE\build-tide-p1\SynthEditSem\Debug\TIDE_VST3.{vst3,pdb}`, timestamped
   14:07:29 on 2026-08-06 — before the 16:52 crash. So does
   `%LOCALAPPDATA%\CrashDumps\reaper.exe.44464.dmp`. Nothing had been cleaned up
   in the day between runs, but do not count on that indefinitely.

4. **`dv` in the Debug dump gives the host's actual arguments, and they are
   absurd.** `right = 2178`, `bottom = 32672`. 32672 is past the D3D11 maximum
   texture dimension of 16384, so `ResizeBuffers` *cannot* succeed — which is
   why this reproduced 3/3 rather than intermittently. Where REAPER got that
   number I could not establish; the note records the bit patterns as a loose
   end rather than guessing.

5. **The bug is visible in the source once you know where to look, and the
   codebase already knows about it.** `OnSize`, twenty lines above `reSize`,
   checks `!swapChain || !d2dDeviceContext` and carries a comment explaining
   that the device can legitimately be gone. `reSize` checks one of the two,
   before the re-entrant call instead of after, and never checks `swapChain` at
   all — line 1419 is a second latent null deref on the same path.

6. **A minidump gives you locals but not the whole object.** `dt -r1 this` died
   with "Memory read error" partway through — a minidump keeps stack and
   registers, not the full heap. I could not read `swapChain`/`d2dDeviceContext`
   out of the object directly and had to pin the null pointer from the
   disassembly instead: the fault at `+0x139` falls between the
   `ComPtr<ID2D1DeviceContext>::operator->` call at `+0x12a` and the indirect
   call at `+0x14f`, while the `swapChain` accessor is not reached until
   `+0x174`. `uf /c` on the function is what makes that legible.

**The scope problem — this one is Jeff's, and it is the reason P4 is not
fixed.** The run prompt's ALLOWED list is `SE16/SynthEditSem/`,
`SE16/TideModules/`, `SE16/SE_IOS_APP/TIDE/`; the GATED list is `EditorLib`,
`SynthEdit2`, `SynthEditLib`. **`gmpi_ui` and `GMPI_Wrappers` are in neither.**
They are separate public repos that the build consumes via
`GMPI_UI_FOLDER_OVERRIDE` / `GMPI_WRAPPER_FOLDER_OVERRIDE`, and they are where
TIDE's rendering and windowing bugs actually live. The prompt says "do the
TIDE-side part and file the rest" — but I grepped `SE16/SynthEditSem/` for
`DrawingFrame`, `reSize` and `SEVSTGUIEditorWin` and there are **no hits**.
There was no TIDE-side part. Filed as **G3**.

I did not reach across, for three reasons: the prompt explicitly warns that "the
fix looks small" is exactly when not to; `gmpi_ui` is the render backend for
every GMPI plugin *and* SynthEdit, so it is shared code in the sense the GATED
rule means; and **both working copies were dirty** with in-progress Wayland work
(`gmpi_ui` at `11051f1`, `backends/DrawingFrameWayland.h` modified;
`GMPI_Wrappers` at `4a6a733`, `tests/wayland_editor_host.cpp` modified).
Committing into someone's uncommitted branch is how you lose both.

> **Correction, later the same day:** that dirt was **not** in-progress Wayland
> work. It was pure CRLF line-ending churn — `git diff --ignore-all-space`
> returns nothing for all three files (8,424 and 1,019 diff lines respectively,
> zero real content). Nobody's work was at risk. The caution above was still the
> right call for the other two reasons, but do not repeat the mistake of reading
> a dirty tree as work-in-progress without testing it. See the entry for the
> push/cleanup below.

**What I did change, and it builds.** `SE16/SynthEditSem/CMakeLists.txt` now
adds `/Zi` + `/DEBUG` for Release on the TIDE targets, with `/OPT:REF` and
`/OPT:ICF` restored explicitly because `/DEBUG` silently turns both off. Scoped
to `${SUB_PROJECT_NAME}` inside the existing `FORMATS_LIST` loop, so it reaches
`TIDE` and `TIDE_VST3` and nothing else.

| Target | Exit | Result |
|---|---|---|
| `TIDE`, `TIDE_VST3` Release | 0 | `TIDE_VST3.pdb` 10,031,104 B and `TIDE.pdb` 8,998,912 B now exist |
| `SynthEditCL` Release | 0 | unaffected, built to confirm rather than assumed |

Binary cost is 11,776 bytes each (`TIDE_VST3.vst3` 2,969,600 → 2,981,376), which
is the debug directory entry — if it had grown by megabytes, `/OPT:REF` would
have been lost. The next Release crash report symbolises without needing a Debug
repro, which is what P4 asked for.

**Next:** **G3 first** — it is one ruling and it unblocks a crash that kills the
host. P4a is a few lines (re-check both pointers after `SetWindowPos`, mirroring
`OnSize:1376`) and P4b is a clamp in `checkSizeConstraint`; both are written up
with exact line numbers in the note, so whoever is allowed to touch those repos
can land them quickly — including Jeff directly, which may be the fastest route.
Until then **V1 stays untestable by hand**, since the editor still cannot be
resized without killing the DAW.

After that the queue is S1a (win, and the §9 check still wants doing first),
then S1b/S4/S5. Note S4 is still open and still worth closing as a side effect
of S1a rather than fixing twice.

**Branch/PR:** `tide/win/P4-editor-resize-crash`

---

## 2026-08-07 — windows — L1 + H1 resolved, C1 done (Jeff's decisions, executed same run)

**Did:** Jeff made two rulings in quick succession and this run executed both.

**L1 — licence: ISC**, the same licence as GMPI and gmpi_ui. One stumble worth
recording: the instruction arrived as "MIT", an MIT LICENSE was pushed to
`SynthEditLib`, and Jeff corrected to "same as gmpi_ui" — which is **ISC** —
minutes later. Both commits are in that repo's history (`42ce33d` MIT,
`a2143a4` ISC replacing it); no force-push, the correction is a plain follow-up
commit. TideSynth got ISC directly (`a58a6f1`, copyright 2026 alone — nothing
in this repo predates the project). GitHub now detects both repos as ISC. This
also closes **C1** — the one carve-out stage that moves no code — and C2–C7 now
wait on C0 alone.

**H1 — hosting: GitHub Pages.** The deploy half is done:
`.github/workflows/pages.yml` publishes `website/` on any push to `main`
touching it. No build step — the artifact *is* the folder. The go-live half
(enable Pages with Source "GitHub Actions", custom domain, four apex `A`
records + `www` CNAME, Enforce HTTPS) needs repo settings and the registrar,
so it stays NEEDS-JEFF with the exact checklist in
[docs/hosting.md](docs/hosting.md).

**The page now says "open source".** The Source section links the LICENSE and
names ISC. Until today that phrase would have been false; the wording history
is in `website/README.md`, along with the one distinction still worth keeping:
open source (true — licence and repos) is not "buildable from public code
alone" (false until C7 — `EditorLib` is still private).

**Learned:**

1. **"MIT" and "same as my other repos" are different answers; ask which one is
   meant when they conflict.** The sibling repos use ISC. Functionally the two
   licences are near-identical, which is exactly why the wrong one sails
   through review — match on the *text*, not the vibe. Byte-identical to
   gmpi_ui's LICENSE is the convention now, and the copyright range
   (`2007-2026` for shared-lineage code, `2026` for TIDE) follows it.

2. **GitHub's licence badge lags.** SynthEditLib showed "ISC License" within
   seconds; TideSynth still showed nothing minutes after the push. Do not read
   the API's `licenseInfo: null` as "file missing" right after a push.

3. **PR #12 appeared mid-run from another machine, claiming S4.** The stagger
   is working as designed — a different box, a different item, no collision.
   S4's row here was deliberately left untouched so their PR can update it
   without conflict. (S4 will also be subsumed if S1a lands first; whoever
   merges should reconcile.)

**Next:** the go-live checklist in H1 is the only thing between the page and
`https://tidesynth.com`. After that the last placeholder is the donation URL.
Engineering queue unchanged: P4c, then S1a.

**Branch/PR:** licences went straight to `main` in both repos at Jeff's
direction; the website/Pages work continues on
`tide/win/W1-website-holding-page` (PR #13).

---

## 2026-08-07 — windows — W1 (same run, at Jeff's request)

**Did:** Built the tidesynth.com holding page at `website/index.html`, wrote
[docs/hosting.md](docs/hosting.md), and filed **H1**. Not deployed — W1 says
deployment is Jeff's.

**Result:** One self-contained file, 5,708 bytes. No build step, no
dependencies, no JavaScript. **Zero external requests**, and that is verified
rather than asserted: loaded it in a browser and the network log is empty, and a
static grep finds no `<script>`, `<link>`, `@import`, `<img>` or `<iframe>`. The
only URLs in the file are outbound `<a href>` links, which load nothing.

**Learned:**

1. **synthedit.com is not on Netlify, despite `netlify.toml` in its repo root.**
   That file is vestigial and will mislead you. The real deploy is GitHub
   Actions → **FTP** (`.github/workflows/deploy.yml`) to an Apache shared host:
   `server-dir: /domains/synthedit.com/public_html/_site/`. The
   `/domains/<domain>/public_html/` layout is the DirectAdmin convention, and
   the useful part is that it is **already per-domain** — the account is
   structured to hold more than one.

2. **Do not put TIDE at `synthedit.com/tide/`.** That site's root `.htaccess`
   (`server/root.htaccess` in the website repo) maps the Astro build onto the
   domain root while falling through to the old SilverStripe CMS for
   `/purchase/`, `/members/`, `/downloads/`. It is hand-maintained and
   explicitly **not** deployed by CI, so it drifts silently. A subdirectory
   would land inside those rules. Its own document root avoids all of it.

3. **G1 resolved mid-item and changed the answer.** The repo was private when I
   started, which meant the page had nothing to link *and* GitHub Pages was
   unavailable (Pages from a private repo needs a paid plan). Jeff made it
   public partway through, so Pages became the recommendation and the "Source"
   section became real. Both were rewritten.

4. **Public is not open source, and TIDE is now the second repo in that trap.**
   PLAN.md criticises `SynthEditLib` for being public with no LICENSE — "default
   copyright applies and nobody may legally use or redistribute it". As of today
   `TideSynth` is in exactly that state too. Readable by anyone, legally usable
   by no one. The page therefore says "developed in the open" and states plainly
   that the licence is unsettled; it does **not** say "open source", and it must
   not until **L1** lands. L1 just went from theoretical to urgent.

**Next:** **H1** — pick the host and point the DNS. Then the donation platform,
which is the last `TODO(jeff)` in the page. **L1** deserves to jump the queue now
that two public repos carry no licence. On the code side nothing changed: **P4c**
is still the top engineering item.

**Branch/PR:** `tide/win/W1-website-holding-page`, branched from
`docs/crlf-churn-correction` rather than `main` so the JOURNAL edits do not
conflict with PR #11, which is still open. Same pattern P2 used over P1.

---

## 2026-08-07 — windows — push + branch cleanup (same run, at Jeff's request)

**Did:** Pushed the two shared-repo fixes, tidied branches, and corrected a
factual error I had put in this journal earlier the same day.

**Result:**

| Repo | Outcome |
|---|---|
| `gmpi_ui` | `9c79f30` pushed to `main` (rebased over 2 new upstream commits) |
| `GMPI_Wrappers` | `e6a4541` pushed to `main` (rebased over 1) |
| `SE16` | nothing to push — Jeff had already re-committed the PDB change as `0e19fdd6a` |
| `TideSynth` | nothing to push — PRs #9 and #10 both merged while the run was still going |

Rebuilt after both rebases (upstream had touched rendering): `TIDE`, `TIDE_VST3`
and `SynthEditCL` all exit 0, zero warnings. Deleted six fully-merged local
branches in TideSynth with `git branch -d`; origin already had only `main`.

**Learned — the correction, and it is the useful part of this entry:**

1. **A dirty tree is not necessarily work.** I twice described `gmpi_ui` and
   `GMPI_Wrappers` as "dirty with in-progress Wayland work" and used that as a
   reason not to touch them. It was **pure CRLF line-ending churn**: 8,424 and
   1,019 lines of raw diff, and `git diff --ignore-all-space` returns *nothing*
   for every file. Nobody's work was ever at risk. The giveaway was visible from
   the start and I did not read it — a diffstat with **equal** insertion and
   deletion counts (`4207 insertions(+), 4207 deletions(-)`).

2. **Revert churn; never stash and restore it.** I stashed it to rebase, pushed
   fine, and then the `git stash pop` **conflicted** — an 8,000-line CRLF rewrite
   against a real upstream commit touching the same file. Git keeps the stash on
   a failed pop, so nothing was lost; I cleared the tree and left `stash@{0}` in
   place rather than resolving someone else's apparent work. Once the churn was
   identified, `git checkout HEAD -- <file>` made the second repo trivial. The
   repo rule already says never commit line-ending-only changes; the corollary is
   never *preserve* them either.

3. **Check the PR you are adding to is still open.** PR #8 was merged while I was
   working, so a follow-up commit pushed to that branch recreated a deleted
   branch and needed a fresh PR. `gh pr view <n> --json state` before pushing a
   follow-up.

4. **Do not delete other sessions' branches.** `claude/*` branches in `gmpi_ui`
   and `SE16` are merged and look stale, but each has a live worktree under
   `.claude/worktrees/` and one is actively checked out. Left alone.

**Next:** unchanged — **P4c** (reproduce the crash, then re-run the A/B) is still
the top item, and the resize fix is still fixed-by-reasoning rather than
fixed-by-test. One loose end: `gmpi_ui stash@{0}` holds the reverted churn; it is
provably zero-content and safe to drop.

**Branch/PR:** `docs/crlf-churn-correction`

---

## 2026-08-07 — windows — P4a + P4b (same run, continued after Jeff lifted the scope block)

**Did:** Jeff answered G3 mid-run — `gmpi_ui` and `GMPI_Wrappers` are ALLOWED —
so I implemented both fixes instead of leaving them queued. Updated
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md) to match, since that is
the file each machine's task is reinstalled from.

**Result — the honest headline: the fixes are landed and build clean, but they
are NOT verified, because I could not reproduce the crash.**

The fixes:

- **P4a**, `gmpi_ui/backends/DrawingFrameWin.cpp`. `reSize` re-reads
  `d2dDeviceContext` and `swapChain` *after* `SetWindowPos` returns rather than
  trusting the check it made before, and rejects degenerate/over-limit extents
  up front (`maxSwapChainDimension = 16384`). Also closes the second latent
  deref — `swapChain` was never checked at all.
- **P4b**, `GMPI_Wrappers/wrapper/VST3/SEVSTGUIEditorWin.cpp`.
  `checkSizeConstraint` writes the nearest acceptable size back into the rect,
  per the VST3 contract, instead of returning `kResultFalse` and leaving it
  untouched.

Builds: `TIDE` + `TIDE_VST3` Release **and** Debug, plus `SynthEditCL`, all exit
0 with zero warnings.

**The verification failure — read this before believing P4 is closed.** I
rebuilt P2's portable-REAPER harness and ran a proper A/B. The crash never
happened:

| Build | Fixes | Resizes | Result |
|---|---|---|---|
| Release | both on | 7 | survived, resized correctly |
| Release | gmpi_ui off | 1 | survived |
| Release | **both off** | 1 | survived |
| Debug | **both off** | 1 | survived |
| Release | both on (final) | 5 | survived, 0 crash dumps |

With both fixes disabled the code is behaviourally identical to what crashed 3/3
in P2, so this is a **harness difference, not evidence the fix works**. I nearly
reported the first green run as proof; the A/B is what stopped me, and it is the
only reason I know the result is meaningless.

**The lead for P4c.** P2 recorded that `MoveWindow` **did not** resize the
window — `GetWindowRect` returned 1672×995 before and after — and that it
crashed anyway. In my harness `MoveWindow` resizes correctly every single time
(1672×995 → 1200×800 → …). So P2's plugin window was in some different state,
and that state is very likely what produced the `{0,0,2178,32672}` rect. Things I
did not try: the `%APPDATA%\REAPER` config as it stood on 2026-08-06 (I copied
today's, which may have changed), five launches with screenshots and
`SetForegroundWindow` interleaved as P2 did, a different monitor/DPI layout, or
dragging the window edge instead of calling `MoveWindow`.

**Learned:**

1. **Do the A/B before claiming a fix works.** Seven clean resizes looked
   conclusive and were not. Disabling the change and re-running is cheap — two
   rebuilds — and it is the difference between "did not crash" and "this change
   stopped it".
2. **Disable the *whole* fix when you A/B.** My first A/B disabled only the
   `gmpi_ui` half and left the wrapper change live, which would have let me
   credit the wrong file. Both halves off is the only meaningful control.
3. **PowerShell tool state does not persist between calls.** An `Add-Type`
   class in one call is gone by the next; my first resize attempt silently did
   nothing while printing six lines of reassuring "alive=True". Build the whole
   experiment — type definition, window lookup, action, polling — into one call.
4. **`reSize` is Windows-only.** X11 has its own `reSize(int,int)`
   (`DrawingFrameX11.cpp:920`), macOS an `onResize()` (`DrawingFrameMac.mm:434`).
   Neither was touched and neither was audited for the same
   check-then-re-entrant-call pattern; worth a look from those machines.
5. **`OnSize` still has the over-limit weakness** `reSize` had — an out-of-range
   `WM_SIZE` from another route would fail `ResizeBuffers` and be misread as
   device loss, tearing down a working device. It cannot *crash* (it checks both
   pointers), and `reSize`'s clamp makes it unreachable from this path, so I left
   it rather than widening a shared-code change. Noted in the doc, not filed.
6. **Staging discipline in the shared repos.** Both had uncommitted changes. I
   staged only my own file in each (`git add <path>`, never `-A`) and committed
   locally without pushing. `git diff --stat` on just that path is the quick check
   that nothing else came along. *(Correction: I called those changes "Wayland
   work". They were CRLF churn — see the push/cleanup entry below.)*

**Next:** **P4c** — reproduce the crash, then re-run the A/B. Until that lands,
P4 is fixed-by-reasoning, not fixed-by-test, and V1 should not be assumed
unblocked. After that, S1a (still wants the §9 check first), then S1b/S4/S5.

Also worth doing at some point: the fixes are committed but **not pushed** in
`gmpi_ui` and `GMPI_Wrappers`, so they exist only on this machine. If the Mac or
Linux box needs them they are not there yet.

**Branch/PR:** `tide/win/P4-editor-resize-crash` (PR #8)

---

## 2026-08-06 — jeff — decision: free, donation-supported (manual, not a scheduled run)

**Did:** Recorded a product decision that had not been written down anywhere:

> **TIDE is free. No paid tier, no trial, no licence key. Funding is by
> donation, and both the plugin and tidesynth.com should support it.**

Added as a "Price and funding" section in [PLAN.md](PLAN.md), next to the
open-source section rather than as a numbered design constraint — it is a
commercial fact like the plugin-export boundary, not something every backlog item
gets checked against. Amended **W1** to carry a donation link on the website, and
filed **D1** for the plugin side.

**Learned — why the plugin side is a design note and not a task:**

1. **Two existing constraints delete most of the obvious answers.** Constraints 1
   and 5 (one view, minimal dialogs) rule out a splash, a nag dialog or a second
   window, which is how almost every donation-funded plugin does this. What is
   left is the breadcrumb bar or an about pane.

2. **The remaining answer may not work on the platform that matters.** A "Donate"
   button that opens a browser is the natural fallback, and
   [docs/design-notes.md](docs/design-notes.md) already lists
   `browseto.mm`/`openurl.mm` as removed-or-restricted under AUv3. So the one
   implementation that survives the UX constraints may not survive constraint 3.
   Whether an AUv3 can open a URL at all is a **factual question only the macOS
   box can answer** — hence D1 says to establish that first and to say so plainly
   if you are running on Windows or Linux instead of guessing.

3. **Free is not open source, and this decision does not touch L1.** Price and
   licence are separate. A free binary with no LICENSE file is exactly what
   `SynthEditLib` is today. L1 stays NEEDS-JEFF.

4. **The website side is the easy half and should not wait.** A static page with
   an `<a href>` has none of the sandbox or one-view problems. The one trap is
   that W1 already says "no trackers", and hosted donate *widgets* ship
   third-party script and cookies — so W1 now says plain link, not embed, and
   leaves the destination as a placeholder because choosing the platform is a
   Jeff decision like L1 and G1.

**Next:** W1 can absorb the website half whenever it is taken. D1 is `any` but is
really a macOS question; if the Mac takes it, it can answer the AUv3 URL question
properly instead of deferring it.

**Branch/PR:** `plan/free-donation-supported`

---

## 2026-08-06 — windows — P2

**Did:** Loaded the P1 build of `TIDE_VST3.vst3` in REAPER 7.78 and watched it.
Wrote [docs/state-of-the-prototype.md](docs/state-of-the-prototype.md) with two
screenshots under `docs/images/`. Observation only — nothing under `SE16`,
`SynthEditLib` or `C:\SE\build-tide-p1` was modified, and no bug was fixed.

Branched from `tide/win/P1-verify-prototype-build`, not `main`: that PR (#2) is
still open, P2 uses the build tree P1 produced, and both runs edit the same
BACKLOG rows. Before claiming P2 I checked `git ls-remote --heads origin` and
`gh pr list` — no branch or PR named it. (There *are* open PRs #1 and #3 from the
Linux and macOS boxes, both for S1; they collided. #4 is the macOS run's fix to
the run prompt, which is what told me to check remotes first. None are merged, so
`main` still shows P1 as TODO.)

**Result:** It loads and the editor opens — the prototype really is a working
plugin in a real DAW. Four findings, in descending order of how much they hurt:

1. **Resizing the editor window crashes the host.** 3/3 reproductions,
   `0xc0000005` inside `TIDE_VST3.vst3`, Release fault RVA `0x44d8c`, Debug
   `0x184ed9`, followed 3–5 s later by `0xc000041d` at the same offset (unhandled
   exception in a user callback — so it is dying in a window proc, not on the
   audio thread). Filed **P4**.
2. **The plugin is not called TIDE anywhere the user can see it** — REAPER shows
   `VST3i: SynthEdit (GMPI)`. Filed **P5**.
3. **Zero host-automatable parameters.** REAPER reports 3 params and all three
   are its own wrapper's (Bypass/Wet/Delta). That is the concrete state of V2.
4. **The module browser is populated from `C:\ProgramData\SynthEdit\Plugin-Cache-16.xml`**,
   written by the *installed SynthEdit app* at 11:30 that morning. TIDE works on
   this machine only because SynthEdit is installed on it. Evidence for S1/S2.
5. **No breadcrumb bar, no properties pane, canvas drawn ~440 px in from the
   top-left.** Filed **U1**.

**Learned:**

1. **A portable REAPER is the right harness for this, and it takes one copy
   command.** Copy `C:\Program Files\REAPER (x64)\*` and then `%APPDATA%\REAPER\*`
   into one scratch directory (152 MB, ~30 s). Because `reaper.ini` now sits next
   to `reaper.exe`, REAPER runs portable: its own config, its own plug-in scan
   cache, its own `Scripts` folder, and the developer's REAPER is untouched. Set
   `vstpath64` to just the build folder so the scan finds exactly one plug-in.
   `-splashlog <file>` gives a timestamped startup trace.

2. **Drive it from `Scripts/__startup.lua`, not from the mouse.** REAPER runs that
   file automatically at startup, so `InsertTrackAtIndex` + `TrackFX_AddByName` +
   `TrackFX_Show(tr, idx, 3)` instantiates the plugin and floats its editor with
   no UI automation at all, and `TrackFX_GetNumParams`/`GetParamName` dump the
   host-visible parameter list to a log file. This is how finding 3 was measured.
   Two traps: `TrackFX_AddByName` needs `"TIDE_VST3.vst3"` (the *filename*) —
   `"TIDE_VST3"` returns -1 because of finding 2. And the startup script does not
   re-run if REAPER restores project tabs from a previous session; strip
   `projecttab*` and `lastproject=` from the portable ini between runs or you will
   test an empty REAPER and think the plugin is stable. I lost one run to exactly
   that and briefly believed the crash was spontaneous.

3. **Screenshots and window control need no MCP.** `Graphics.CopyFromScreen` from
   PowerShell captures the virtual desktop; `EnumWindows` + `GetWindowRect`
   locates the plugin's floating window by title. One gotcha: declare
   `GetWindowTextW` with `CharSet=CharSet.Unicode`, otherwise StringBuilder
   marshals as ANSI and every window title comes back as its first character.

4. **How to prove a crash is caused by what you think it is.** Run 4 sat idle
   2.5 minutes with the editor open, polled every 5 s, `Responding=True`
   throughout, then died 1 second after the resize. An earlier
   `SetForegroundWindow` + screenshot on the same window did not kill it. Without
   that idle control I could not have ruled out a timer or idle callback.

5. **`MoveWindow` on the plugin window does not resize it** — `GetWindowRect`
   returns the same `1672x995` before and after — and it crashes anyway. So the
   fault is in *handling* the size-change message, before any new size is adopted.
   Useful narrowing for whoever takes P4.

6. **The Release configuration produces no PDB.** `build-tide-p1/SynthEditSem/Release`
   has the `.vst3` and nothing else, so the Release fault RVA cannot be
   symbolised. Debug does have `TIDE_VST3.pdb` (57 MB). I tried to symbolise the
   Debug RVA with `dbghelp.dll` from `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64`
   P/Invoked from PowerShell; `SymLoadModuleExW` succeeded but `SymFromAddrW`
   returned `<no symbol>` and I did not chase it further. **There is no `cdb.exe`
   on this machine** — that Debuggers folder holds only `dbghelp/dbgcore/srcsrv/symsrv`
   DLLs. Installing the Debugging Tools for Windows feature, or opening the dump
   in Visual Studio, is the shorter road.

7. **Windows kept full minidumps** (`%LOCALAPPDATA%\CrashDumps\reaper.exe.<pid>.dmp`,
   ~37 MB each) because LocalDumps is enabled on this box. The WER `ReportArchive`
   copies, by contrast, contain only `Report.wer` — the `.tmp.dmp` files it
   references are already deleted by the time you look. Go to `CrashDumps`, not
   `ReportArchive`.

8. **`GetHomeDir()` has no trailing separator.** It ends in
   `std::filesystem::path::parent_path()` (`SynthEdit2/Application.cpp:203-234`),
   so `TideApp.cpp:109`'s `GetHomeDir() + L"modules\\"` composes
   `...\Releasemodules\`, not `...\Release\modules\`. Harmless here because
   neither path exists, but `SynthEditAppBase.cpp:1108` concatenates the same way.
   Not filed separately — it belongs with S1, which rewrites that line anyway.

9. **The module cache filename does not distinguish TIDE from SynthEdit.**
   `SemCacheName()` only adds a per-folder hash when `BundleInfo::isSemFolderOverridden`
   is set (`ModuleFactory_Editor.cpp:175-191`), and `TideApp::InitInstance` assigns
   `semFolder` directly rather than through the setter — so TIDE reads, and on a
   cache miss would *rewrite*, `C:\ProgramData\SynthEdit\Plugin-Cache-16.xml`, the
   installed app's own file. Whoever takes S2 should force the cache-miss path,
   but do it on a machine without SynthEdit installed, or back that file up first.

10. **Reordering note:** P4 is now the topmost Ready-now item, ahead of S1/S2/S3.
    A crash that kills the host blocks V1 and makes every by-hand test impossible,
    so it seemed to belong there. P5 and U1 went to the bottom of the table.

**Next:** P4. It has a one-line repro, two minidumps and a narrowed message path,
and nothing else that touches the editor by hand is testable until it is fixed.
Add `/DEBUG` to the Release link while you are there, so the next crash report is
symbolisable. Whoever takes it: the fix may sit in `SE16/SynthEdit2` (GATED under
the run prompt's ALLOWED/GATED split) — do the TIDE-side part and file the rest.
The portable-REAPER harness in §"How it was observed" is worth rebuilding rather
than clicking; it took about 15 minutes.

**Branch/PR:** `tide/win/P2-daw-load-observation`

---

## 2026-08-06 — windows — P1

**Did:** Verified the prototype builds from a clean CMake configure, in a fresh
build tree at `C:\SE\build-tide-p1` (deliberately *not* the developer's existing
`C:\SE\SE16\build` — that tree is a decade of accumulated cache and would have
hidden the finding below). Wrote `docs/building.md`. Nothing under `C:\SE\SE16`
or `C:\SE\SynthEditLib` was modified; the only writes outside this repo were the
build tree.

**Result:** It builds. `cmake --build ... --target TIDE TIDE_VST3` exits 0 for
both configs, zero compiler warnings at default verbosity:

| Config | Artifacts in `<build>/SynthEditSem/<config>/` |
|---|---|
| Release | `TIDE.gmpi` 2,712,576 B · `TIDE_VST3.vst3` 2,969,600 B |
| Debug | `TIDE.gmpi` 10,235,904 B · `TIDE_VST3.vst3` 11,616,768 B |

Exact commands are in [docs/building.md](docs/building.md). Environment: CMake
4.2.0, VS 18 Community, MSVC 14.51.36231, toolset v145, Windows SDK
10.0.26100.0.

**Learned:**

1. **A clean configure does NOT work with default settings, and the error looks
   like something else entirely.** First attempt failed with exactly two errors:

   ```
   C:\SE\SE16\SynthEdit2\CContainer.cpp(8,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [EditorLib.vcxproj]
   C:\SE\SE16\SynthEdit2\MfcDocPresenter.cpp(4,10): error C1083: Cannot open include file: 'afxres.h': No such file or directory [EditorLib.vcxproj]
   ```

   Root cause: this machine has two VS 18 instances. `...\18\Community` has the
   MFC component (`VC\Tools\MSVC\14.51.36231\atlmfc\include\afxres.h` exists);
   `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` has MSVC
   14.51.36231 but **no `atlmfc` directory at all**. With no
   `CMAKE_GENERATOR_INSTANCE` given, CMake picks BuildTools. Fix: pass
   `-DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community"`.
   `CMAKE_LINKER` in `CMakeCache.txt` is the quickest way to see which instance a
   tree is actually using. The instance cannot be changed in place — delete the
   build tree and reconfigure.

   The developer's `C:\SE\SE16\build` was configured with
   `CMAKE_GENERATOR_INSTANCE:UNINITIALIZED=C:/Program Files/Microsoft Visual Studio/18/Community`,
   i.e. it was passed on the command line at some point. That is the whole reason
   it has always worked and a fresh tree does not.

2. **Do not debug this by building the failing `.vcxproj` directly — it lies.**
   `MSBuild.exe EditorLib.vcxproj /t:...` succeeds on the *same* build tree that
   `cmake --build` fails on, because MSBuild launched by hand resolves its own VS
   instance (Community, MSBuild 18.8.2) while `cmake --build` uses the cached one
   (BuildTools, MSBuild 18.7.8). I lost about half an hour to this: the two
   `.vcxproj` files are byte-identical apart from paths and GUIDs, the
   `IncludePath` property printed by `msbuild -getProperty:IncludePath` contains
   `atlmfc\include`, and `cl.exe` invoked by hand with those include dirs
   compiles `#include "afxres.h"` fine. All of that is true and all of it is
   irrelevant. To get the truth, run `cmake --build ... -- /v:diag` and grep the
   log for `EXTERNAL_INCLUDE=` — the VS install path in that string is the one
   actually in use. It is also not shell-related; it reproduces identically from
   Git Bash, PowerShell and a `.bat` wrapper.

3. **Two files in the carve-out set require MFC on Windows.**
   `SynthEdit2/CContainer.cpp:8` and `SynthEdit2/MfcDocPresenter.cpp:4` both do
   `#include "afxres.h"` inside `#ifdef _WIN32`. Nothing in any CMakeLists
   mentions MFC — it works only because `atlmfc\include` is on the default
   include path when the component happens to be installed. Both files are
   scheduled to move to public `SynthEditLib` (C3 and C4), which would make "you
   must have Visual Studio's MFC component" a build requirement of the open-source
   repo. Filed as **P3**. Not fixed here — out of scope for P1.

4. Building `--target TIDE TIDE_VST3` pulls in only SynthEditLib, EditorLib and
   HarfBuzz, not SynthEditCL or the test suite. Useful: it is a much shorter
   build than the default all-targets one, ~6 min cold.

5. Even with all four `*_FOLDER_OVERRIDE` variables pointed at local clones, a
   fresh configure still hits the network for the VST3 SDK and HarfBuzz (CPM,
   cached in `%USERPROFILE%\.cpm`) and for CLAP + clap-helpers (FetchContent,
   into the build tree, so re-downloaded per build tree).

**Next:** P2 — load TIDE in a DAW and record what happens, observing only. The
build tree at `C:\SE\build-tide-p1` is current and correct as of today, so P2 can
use `SynthEditSem/Debug/TIDE_VST3.vst3` from it without rebuilding. P3 is the
useful item to pair with the carve-out when C0/L1 clear; it is worth doing
*before* C3/C4 move those files, not after.

**Branch/PR:** `tide/win/P1-verify-prototype-build`

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
