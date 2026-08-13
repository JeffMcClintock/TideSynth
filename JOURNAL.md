# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

Every run on three machines reads this file in full, so its size is a cost paid
forever. It hit **192 KB across 37 entries in six days** before the first
rotation (**A8**, 2026-08-12). Nothing is ever deleted or rewritten — old
entries just move to a per-month archive.

**The rule, applied after you append your own entry:**

1. Move the oldest entries out, in order, into `JOURNAL-<YYYY>-<MM>.md` for the
   month each entry belongs to, appending **below** what is already there so the
   archive stays newest-first. Copy the template from
   [JOURNAL-2026-08.md](JOURNAL-2026-08.md) if that month has no file yet.
2. Stop when this file is **under 30 KB**, or when the **four most recent
   entries** remain — whichever comes first. **The floor of four wins**: the run
   prompt tells every run to read the last four entries, so a verbose month
   pushing this file over 30 KB is correct, not a rotation failure.
3. Never edit an entry while archiving it. The archive is the record.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-08-14 — macos — P6

**Prompt:** `dd93251` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Did:** Closed P6 by measurement, not by edit. Built `SynthEditCL` on this box
with the **Xcode** generator — the one thing the P7a and P7b runs could not do,
and the whole reason the row stayed open — and confirmed the `CodeSign` failure
is gone. **Changed no source file:** `SE16/SynthEditCL/CMakeLists.txt` is on
neither the ALLOWED nor the GATED list, so it is GATED by default, and the fix
had already landed in SynthEdit's own commits. The work was verification.

**Result — fixed, and proven on the generator that actually signs.**

| | |
|---|---|
| source | `SE16` `b3c1efb07` — `git diff HEAD origin/master -- SynthEditCL/ SynthEditLib/ CMakeLists.txt` is **empty**, so this is current `master`'s content for everything in scope |
| configure | `cmake -G Xcode`, fresh scratch tree outside both repos, `x86_64;arm64`, deployment 13.3, three local overrides matching Jeff's own `build/CMakeCache.txt` |
| `cmake --build --config Debug --target SynthEditCL` | **RC=0** |
| `codesign --verify --deep --strict --verbose=2` | *"valid on disk"*, *"satisfies its Designated Requirement"* |
| sealed | 336 files, 98 signed `.sem` under `Contents/PlugIns` |
| smoke | the signed binary runs: `SynthEditCL V1.6.182` |

**The Xcode generator does emit the step, and the log proves it** —
`CodeSign …/SynthEditCL.app (in target 'SynthEditCL')`, `Signing Identity:
"Sign to Run Locally"`, `/usr/bin/codesign --force --sign - --entitlements … `.
That is the asymmetry this row was stuck on: **Ninja emits no `codesign`
invocation at all**, so an RC=0 Ninja build is not evidence either way.

**Layout, which is the actual fix (`691270c5d`):** `Contents/MacOS` holds
**only** the executable. `Prefabs`, `fonts`, `skins` and `templates` are all
under `Contents/Resources`, and the exact file the row named is now
`Contents/Resources/Prefabs/Controls/Button Small2.syntheditprefab`.

**Verification artifact — A/B positive control on the same signed binary, no
source edit and no rebuild.** Copied the built bundle, moved the four staged
directories back under `Contents/MacOS/Resources` to recreate the pre-fix
layout, and re-ran the *same* `codesign` command Xcode ran:

| bundle layout, same binary, same codesign command | RC | output |
|---|---|---|
| pre-fix (`Contents/MacOS/Resources/…`) | **1** | `SynthEditCL.app: code object is not signed at all` / `In subcomponent: …/Contents/MacOS/Resources/Prefabs/Controls/Button Small2.syntheditprefab` |
| current (`Contents/Resources/…`) | **0** | — |

That reproduces P6's error string **verbatim, down to the same subcomponent
file**, and shows the staging path is what closes it — not a toolchain or Xcode
version difference, and not luck.

**The second commit (`4792f4bf2`, Finder detritus) also holds:** the built
bundle contains **0** `.DS_Store` and no extended attributes.

**Standing rule — all five products build under the Xcode generator, each
RC=0, each verifying:**

| target | build | `codesign --verify --deep --strict` |
|---|---|---|
| `SynthEditCL` | RC=0 | RC=0 |
| `SynthEdit_VST3` | RC=0 | RC=0 |
| `SynthEdit_GMPI` | RC=0 | RC=0 |
| `TIDE` | RC=0 | RC=0 |
| `TIDE_VST3` | RC=0 | RC=0 |

So SynthEdit, SynthEditCL and TIDE all build on macOS on `master` today, under
the generator Jeff's own tree uses — which is a stronger statement than the
Ninja RC=0 the last three mac runs could make.

**Learned:**

- **Any signing-shaped question on mac must be answered with `-G Xcode`.** Ninja
  never emits `codesign`, so a Ninja build cannot confirm *or* deny a codesign
  bug. P6 sat open for six days because two runs reported RC=0 from a generator
  that structurally could not see the failure. Worth treating as a fleet rule,
  not a P6 detail.
- **A bundle-level A/B is enough to prove a staging-path fix, and it needs no
  source edit.** That matters when the file lives on a GATED-by-default path:
  the positive control was `cp -R` + `mv` + re-run `codesign`, and it produced
  the row's exact error text. No branch in `SE16`, nothing to review there.
- **`GMPI_WRAPPER_FOLDER_OVERRIDE` is empty in Jeff's `build/CMakeCache.txt`**,
  so `GMPI WRAPPERS` is fetched from github rather than taken from the local
  clone — the configure output says `Fetching GMPI WRAPPERS from github` while
  `SynthEditLib`, `GMPI` and `GMPI-UI` all say `Using local … folder`. This is
  exactly the asymmetry **X4** says to watch for; I matched Jeff's cache rather
  than "fixing" it, so this run's result reflects his tree, but anyone debugging
  a wrapper-side problem on this box should know the wrapper is not local.
- **The `any` NEXT pointer is A4, and a scheduled run cannot do it.** A4 is a
  path-allowlisted auto-merge *action* — i.e. a file under
  `.github/workflows/**`, which the bot token deliberately cannot write. Noted,
  not acted on: it is not this run's item, and A12 already covers the general
  shape of "the fleet points a box at work it structurally cannot do". Flagging
  it so the next `any`-eligible run does not burn its session discovering it.
- **My PR's lint will be red, pre-existing.** `check-links.py`'s slugger bug is
  **A13**, found by the P7c run; it is already red on `main`. Nothing here
  caused it and fixing it would be a second item.

**Next:**

1. **Merge [#50](https://github.com/JeffMcClintock/TideSynth/pull/50)** (this
   run — docs only, no code in any repo). P6 then flips IN-REVIEW → DONE.
2. **`mac` NEXT moved P6 → E1a**, taking the P7c run's correction at its word:
   E1a's Accept clause is *"one render of both cases on a second platform"*,
   second to linux, so it is mac-or-win work and the linux box can never satisfy
   it. This box can: the render half is a download and two numbers, and it is
   the topmost item that is genuinely mac's rather than anyone's. S9 and S10,
   the only other `mac` rows, both still need Jeff.
3. **A13 is the fallback** if a mac run finds E1a blocked — it is small, it is on
   `scripts/` (a scheduled run may edit it), and it gates A4's usefulness as a
   merge check.
4. **P6's own text should not be re-filed.** It is closed on evidence, and the
   evidence is a positive control rather than an absence of failure.

**Tree hygiene:** nothing was written into either working copy's source. `SE16`
and `TideSynth` were both clean at claim time (`SE16` 5 commits behind
`origin/master`, `TideSynth` 3 behind `origin/main`) and I left `SE16` untouched
and un-updated — no fetch-into-tree, no checkout, no build inside it. All build
output went to a scratch tree under the session's temp dir; Jeff's own
`SynthEdit/build` Xcode tree was not read from, written to, or invalidated.
`git status` in `SE16` is unchanged from how I found it.

**Branch/PR:** `tide/mac/P6-cl-codesign-xcode` —
[#50](https://github.com/JeffMcClintock/TideSynth/pull/50), TideSynth only.
**No code repo was committed in**, so STEP 5's two-end-states rule has exactly
one repo to satisfy.

---

## 2026-08-13 — windows — C4

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Carve-out stage **C4**. Moved the twelve view/browser files out of the
private repo into public `SynthEditLib`, implemented **C9** with them (Jeff's
option (c)), and rebuilt everything including the SynthEdit2 link step no
previous carve-out stage had been able to reach. Also measured what the move
costs C7, which is the part worth reading.

**Result — all three products build, all tests pass, and C9 is proven rather
than asserted.**

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16`, Release | **905/905, RC=0** — `SynthEdit_VST3`, `SynthEdit_GMPI`, `SynthEditCL`, `TIDE.gmpi`, `TIDE_VST3` |
| the six moved TUs really compiled from their new home | log shows `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj` for all six |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | 58/58, 24/24, 10/10 — all RC=0 |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, `x64\Release\SynthEdit2\SynthEdit2.exe` |
| no line-ending churn | `core.autocrlf=false` in both repos; all twelve files `cmp`-identical to their originals |

The SynthEdit2 row matters beyond "it builds": **C1b and C2 both had to leave
link-stage verification of SynthEdit2 open**, because P8 blocked it. P8 is fixed,
so this is the first carve-out stage whose WinUI3 link step was actually reached.

**C9 — implemented as ruled, and the value is injected rather than owned.**

New `SynthEditLib/se_version.h` defaults `SE_APP_BUILD_NUMBER` to 0;
`EditorLib/CMakeLists.txt` injects SynthEdit's real value with a `file(READ)` +
`string(REGEX MATCH)` on `se_build_number.h`, which **stays exactly where
`SynthEdit_cmake_mac.yml:153`, `SynthEdit_cmake_win.yml:206` and
`SynthEdit_store_win.yml:266` grep for it.** No workflow file touched, and none
needed to be — the bot token's missing `workflow` scope never came into it.

I did not give the library its own constant, and the reason is worth recording
because "SynthEditLib gets its own version header" reads like it means that.
**Both uses are cache invalidation, and both track the *application*, not the
library:** `SkinMgr` re-copies skins out of the *application's* own `Resources`
folder, and `ModuleFactory_Editor` caches the modules the *application* links. A
constant `SynthEditLib` owned would never move on a SynthEdit release, so
SynthEdit would silently stop invalidating both caches on upgrade — a behaviour
regression wearing a decoupling's clothes. Injection keeps SynthEdit's behaviour
bit-identical and still leaves the public repo able to compile with no private
header.

Proof, not assertion:

| | |
|---|---|
| `se_build_number.h` today | `SE_BUILD_NUMBER 183` |
| `build.ninja` | `SE_APP_BUILD_NUMBER=183` |
| probe TU, no injection | prints **0** (what a clean clone gets, i.e. TIDE from C7) |
| probe TU, with injection | prints **183** |
| scope | the define appears on **57 build statements, every one of them EditorLib** — `PRIVATE` holds, nothing leaks to TIDE, SynthEditCL or SynthEditLib's own targets |

**Learned — C3's second check has a false-alarm case and a real one, and telling
them apart took a filesystem test, not a grep.** Two of the twelve files use
`#include "../"`:

| include | resolves relative to the file? |
|---|---|
| `ThemeManager.cpp` → `"../tinyxml2/tinyxml2.h"` | **no.** `SE16/tinyxml2/` **does not exist.** It already resolves through a search path — `SynthEditLib/modules/se_sdk3_hosting/../tinyXml2/tinyxml2.h` — so the file's own directory is irrelevant and a move cannot affect it |
| `ModuleFactory_Editor.cpp`, `SkinMgr.cpp` → `"../se_build_number.h"` | **yes.** `SE16/se_build_number.h` exists. This is the one that breaks |

That is exactly the shape C4's row predicted ("most resolve through the search
path and are harmless, and the one that resolves for real is the one that
breaks") — recording it because the `tinyxml2` line *looks* identical to the
`se_build_number` line and would have been "fixed" by anyone pattern-matching on
the `../`. Note also the case difference: the file says `tinyxml2/`, the
directory on disk is `tinyXml2/`. Harmless on Windows and on the search-path
route; worth knowing before anyone moves that directory.

**Learned — the big one. C4 makes the public repo's private-include problem
worse, not better, and I measured it instead of assuming either way.**

I expected the opposite. `SkinMgr.h`, `ModuleFactory_Editor.h` and
`MfcDocPresenter.h` were being included by 11 files already public — the exact
"a public file its own repo cannot compile" shape C2 hit with
`cpu_accumulator.h`. C4 closes all 11. But the six moved `.cpp` bring their own
private-header dependencies with them:

| | dangling private includes in `SynthEditLib` |
|---|---|
| before C4 (`SE16 origin/master` / `SynthEditLib origin/main`) | **47** |
| after C4 | **56** |
| closed by C4 | 11 |
| opened by C4 | 20 |

**Read both sides from git refs, not working trees.** My first attempt computed
"before" with the private-header list taken from the already-modified `SE16`
working tree, so the eleven closures were invisible and the number was wrong in
a direction that flattered the change. The script that does it properly is
`dangling2.py` in the run scratchpad; it is 60 lines and worth re-creating for
C5, which will move the single most-included name on the list (`Application.h`).

18 of the 20 name headers already on `EditorLib/CMakeLists.txt`'s source list, so
C5 and later stages close them by construction. **Two are on no stage's list at
all, and both are reached from `MfcDocPresenter.cpp`:**

- **`SynthEditApp.h`.** Deliberately excluded from EditorLib so each app picks
  its own `SE_MOONBASE_SUPPORT` without ODR conflicts — the CMakeLists says so
  in a comment. `MfcDocPresenter.cpp:1258-1261` declares
  `extern SynthEditApp* theApp` and calls `theApp->isMoonbaseEnabled()` and
  `theApp->licenseIsActive()` to gray out "Selection to Prefab". **So a
  licence-gate call site is now in the public repo.** No Moonbase implementation
  is published — two method names and the existence of the gate are. This is the
  first time the carve-out has put licensing-adjacent code on the public side,
  and the standing direction is to keep commercial code private as practical.
- **`ModulePicker.h`.** 19 KB, header-only, no `.cpp`, on no stage's list.
  `MfcDocPresenter.cpp:536` does `return new ModulePicker(pparent)`.

**Filed as C11, and deliberately not acted on.** C4's file list is Jeff's and
`MfcDocPresenter` is on it; reshaping an approved stage because one of its files
turned out to be awkward is not a scheduled run's call. It is flagged at the top
of both code PR bodies so it cannot be merged without being seen. Neither breaks
anything today — both resolve through EditorLib's `../SynthEdit2` include path —
they break at **C7**, whose whole test is a clean clone with no access to SE16.

**Learned — `MSBuild SynthEditStore.sln` now needs the VS 2026 (v145) toolset,
and P8's recorded command no longer works on this box.** Under VS 2022 it dies at
`build\ZERO_CHECK.vcxproj` with `MSB8020: The build tools for v145
(Platform Toolset = 'v145') cannot be found`, **before reaching `SynthEdit2` at
all** — so it reads like a project failure and is not one. The solution
references two projects inside `SE16\build`, Jeff's CMake-generated VS tree,
which is now generated for VS 2026. Building through
`Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat` succeeds. Both VS
2022 Community and VS 18 Community are installed here; Ninja + `cl` from either
is fine for the CMake side, it is only the `.sln` that is pinned.

**Also done as STEP 4 chores:** **P7c** flipped IN-REVIEW → DONE and moved
verbatim to `BACKLOG-DONE.md` — [gmpi_ui#5](https://github.com/JeffMcClintock/gmpi_ui/pull/5)
and [#48](https://github.com/JeffMcClintock/TideSynth/pull/48) both merged
2026-08-13. **C9's row grew** to record that its mechanism now exists and is
proven, so C5 reuses it with one `#include` swap and one macro rename rather than
building anything; it stays TODO because `Application.cpp` has not been done.
**The `win` NEXT row** now points at C5-if-merged, else E1a — the linux run
recommended exactly that E1a move on 2026-08-13 and could not make it from its
own row.

**STEP 1 / 1.5:** no `platform:win` issues; no `platform:*` labelled issues at
all across the five repos. The only open issues anywhere are TideSynth
[#44](https://github.com/JeffMcClintock/TideSynth/issues/44) (the A6 watchdog
digest, `github-actions`, informational) and `gmpi_ui#1` ("Linux support?", 2024,
third-party and unlabelled) — noted, not acted on, per the issue-authenticity
rule. **Zero open PRs in all five repos** at claim time, so nothing was handed
back to this platform and nothing was in flight to collide with.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches** at claim time. Nothing of his was committed, reverted or
stashed. Note `SE16` local `master` was two commits behind `origin/master`; I
branched from `origin/master`, as the prompt requires, so the `[Build-Machine]`
bump to build 183 is included — which is why C9's injected value reads 183 and
not the 182 in the stale local tree.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** a scratch Ninja tree and a probe binary under the
session scratchpad, both outside every repo — Jeff's own `SE16\build` was not
configured or built into. The MSBuild run **did** write into `SE16\x64\Release\`,
which is `.gitignore`d (`.gitignore:13`) and is where that build has always put
its output; `git status` in `SE16` is clean apart from my own commit. No engine
state was written: `SynthEditCL.exe` was built but never executed, so no module
cache or skin folder was created or invalidated on this machine.

**Next:**

1. **Merge all three C4 PRs together** —
   [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15),
   [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6),
   [#49](https://github.com/JeffMcClintock/TideSynth/pull/49). Unlike P7c's
   independent pair, **merging any one alone breaks the build**: the files exist
   in exactly one repo at a time and `EditorLib/CMakeLists.txt` points at the new
   location.
2. **Rule C11 before C7, and it needs Jeff.** The `SynthEditApp.h` half is a
   boundary decision, not work — the call is two bools behind a pointer, so a
   small interface hook would do it, but whether licensing-adjacent code may sit
   in the public repo at all is not an agent's call. `ModulePicker.h` is the easy
   half and probably just joins a stage's list.
3. **C5 is cheap now.** C9's mechanism shipped with C4, so `Application.cpp` is
   one `#include` swap and one macro rename. It is `win`, and it is the
   carve-out's critical path the moment C4 lands.
4. **A13 is still open and still red on `main`.** I did not hit it as a blocker
   this run, but the link checker's em-dash slug bug is unchanged and every
   heading in this file has em-dashes.

**Branch/PR:** `tide/win/C4-move-views-browsers` in three repos —
[SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15) (deletions,
`EditorLib/CMakeLists.txt`, vcxproj),
[SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6) (the
twelve files + `se_version.h`),
[#49](https://github.com/JeffMcClintock/TideSynth/pull/49) (BACKLOG, JOURNAL,
`docs/carve-out.md`). No other repo was committed in or modified.

---

## 2026-08-13 — linux — P7c (E1a not taken — see below)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Did:** Closed the X11 half of P7. Reproduced the stale-extent heap overflow in
`X11DrawingFrame::Impl::present()`, guarded it, and wrote the regression test and
the audit. Also settled the reachability question the row was filed with, and
corrected the `linux` NEXT pointer, which named an item no linux box can do.

**E1a was the NEXT pointer and I did not take it. It is not a linux item.** Its
Accept clause is *"one render of both cases **on a second platform**"* — second
to linux, the only lane that has ever run. So this box is the one machine
definitionally unable to satisfy it, and the NEXT row's reasoning ("this is the
box that can actually test cross-platform") was backwards. Checked before
falling through rather than assuming: no wine, no container runtime, no macOS
emulation on this box. **And Wine would have been worse than nothing** — its
`msvcrt` routes `sin`/`cos`/`exp` to glibc's libm, i.e. the exact implementation
the linux golden was rendered with, so a Wine render would report near-zero
drift and license an RMS tolerance a real Windows box then fails. That is the
specific trap E1 refused to walk into by guessing; guessing with a
plausible-looking measurement attached would have been worse than guessing
openly. Put the constraint in the row's own Item text (the Plat column stays
`any` — see the lint finding below), and recommended (not imposed) that E1a
become the mac or win NEXT rather than this one's.

Fell through in file order to **P7c** — TODO, `linux`, ALLOWED path, and the row
always said a Linux run could take it alone. What I skipped on the way, so the
next run need not re-derive it: **C9** (its remaining work is GATED
`SynthEditLib`, and it is not a C1-C7 stage), **A4/A10/A12** (all need
`.github/workflows/**`, which the bot token cannot push), **A9** (its stated
prerequisite is NEEDS-JEFF and would change what gets built), **N1** (the row
itself says "do it after C7, not during", and calls its own remainder decisions
rather than edits), **P7d** (GATED-by-default, explicitly waiting on Jeff's
scope ruling).

**Result — the audit was right in every particular, and the bug is real.**

A/B on the same harness binary, only the backend source differing:

| backend | result |
|---|---|
| `origin/main` as it stood | **SIGSEGV, exit 139, 3/3** |
| with the guard | **exit 0, PASS, 3/3** |

Under gdb, unfixed:

```
Program received signal SIGSEGV
#0  gmpi::cpugfx::encodeDirtyRect (...) at backends/CpuEncode.h:269
#1  X11DrawingFrame::Impl::present (...) at backends/DrawingFrameX11.cpp:1332
#2  X11DrawingFrame::onTimer (...) at backends/DrawingFrameX11.cpp:1220
locals: right = 800, bottom = 600, y = 68
```

`right`/`bottom` are the stale `pw`/`ph` against a 64×48 image — `encodeDirtyRect`
clips to `dst.width`/`dst.height`, so **the clip clips to the lie** and protects
nothing. It faulted at row **68**: rows 48–67 were written outside the surface
first. That number is the whole difference between this and P4 — it corrupts
before it crashes, and on a smaller mismatch it would not crash at all.

**Learned — the reachability question the row asked is answerable, and the
answer is no.** *"Whether any client callback can pump the X event loop and
actually cause a nested `present()`."* It cannot, and by construction rather than
by luck:

| Check | Result |
|---|---|
| `ensureImage()` call sites | **one** — `present()` |
| `Impl::present()` call sites | **two** — `processEvents()`, `onTimer()`, both host-driven |
| `XNextEvent`/`XMaskEvent`/`XIfEvent`/`XPeekEvent` in the backend | **one** `XNextEvent`, in `processEvents()`. No nested or modal loop anywhere — menus, stock dialogs, the colour dialog, the text edit and the tooltip all share that one pump, and the portal file chooser is async over D-Bus |
| what a client can reach | `IDrawingHost`, `IInputHost`, `IDialogHost`. **None expose `processEvents`, `onTimer`, `reSize`, or the `Display*`** — they are non-virtual members of the concrete frame, and a client only ever holds an interface pointer |

`DrawingFrameX11.h`'s rule 1 — *"NO EVENT LOOP OF OUR OWN. The host owns the
loop"* — turns out to be load-bearing rather than a design note. Also worth
recording because it is the near-miss: **`reSize()` alone cannot cause this.** It
moves `d.width`/`d.height` and reallocates nothing, so the image still matches
`pw`/`ph`. The overflow needs the *allocation* to change, and only `present()`
changes it.

**So why guard it at all?** The residual the audit cannot close: during
`measure`/`arrange`/`render` the client may call back into the **host**, and a
host that re-enters its own run loop from there reaches `processEvents()`.
gmpi_ui can neither prevent nor detect that, and cannot prove no host does it. A
heap overflow whose only defence is an audit of code we do not own is a bad trade
against three lines — and "unreachable today" is a property a future backend
change revokes silently, where a guard cannot. I would have recorded
unreachable-by-construction and stopped, as the row permits, if the residual were
empty. It is not.

**Learned — AddressSanitizer is a false-negative machine on this path, for a
brand-new reason.** On a local display (including Xvfb) the image is MIT-SHM, so
`image->data` is a `shmat()` mapping of its own and the overflow walks off its
end into unmapped space: a plain SIGSEGV, no sanitizer needed. **ASan does not
instrument a shm segment.** The `XCreateImage` fallback (remote display) is
`malloc`'d and ASan does cover it, so the script offers `ASAN=1` for that path
only, with the warning in its header. This is the **second** time in P7 that ASan
was blind to the crash it looks purpose-built for, and the reasons are unrelated
— CoreGraphics on mac (P7b), shared memory here. The rule that generalises, and
the one I would want the next audit to have: **work out who owns the memory
before choosing the detector.**

**Learned — two of this row's premises were wrong, and one changed the approach.**

| the row said | what is actually there |
|---|---|
| "`tests/x11_editor_host.cpp` already attaches an editor and is the place for the probe" | that file is **not in `gmpi_ui`** — it is in **`GMPI_Wrappers`**, and it is a full VST3 host. Wrong tool regardless of repo: a real plugin client *cannot reach the nesting at all*, so a test driven through one could only ever exercise the safe path |
| (implicitly) copy the liveness discipline from `mac_editor_resize_host.mm` | the right precedent is **P7b's** `mac_render_reentrant_resize.mm` — a synthetic client driving the backend directly, no CMake, no VST3. That is what reaches this |

So the test hands its synthetic client a pointer to the *concrete* frame, which
a real client never has. That is deliberate and stated at the top of the file:
it is a **positive control for the guard**, not a reproduction of host behaviour.
**Two assertions stop it passing vacuously** — it checks the nested present
actually ran and that the frame really shrank to 64×48. Without those, a guard
that prevented the scenario instead of surviving it would look identical to one
that worked.

**Build health: this platform's default branch builds, and I verified it rather
than assuming.** Fresh Ninja configure into a scratch dir (Jeff's `~/SE/build`
untouched) with `GMPI_UI_FOLDER_OVERRIDE` pointing at this branch: **885/885,
RC=0**, producing `SynthEdit_VST3`, `SynthEdit_GMPI`, `SynthEditCL`, `TIDE.gmpi`
and `TIDE_VST3.so`. The log confirms `DrawingFrameX11.cpp` was compiled into it,
so the build is evidence about *this change* and not about some cached object.
Configure reported `Using local` for SynthEditLib, GMPI and GMPI-UI — per X4, the
thing to watch for is an unexpected `Fetching` on a family repo, and there was
none. **No regression to ordinary painting either:** `tests/x11_menu_test.sh`
passes in full with the guard in — menu, stock dialog, text edit, key listener,
colour picker, tooltip, file dialog, and *"pixels changed by the editor: 18495"*,
which is the line that proves the guard is **not** firing on normal paints.

**Learned — A3's link check is red on `main` right now, and it is the checker
that is wrong, not the link. Filed as A13.** I hit it as a pre-existing failure
(confirmed by stashing my changes and re-running: same single break, same line).
`scripts/check-links.py:44` collapses a **run** of whitespace to one hyphen;
GitHub emits one hyphen **per space**. Line 43 has already deleted the em-dash,
so a heading like `## 2026-08-13 — macos — S6 (part 2 of 2)` leaves two spaces
behind and the script computes `2026-08-13-macos-s6-part-2-of-2` where GitHub
computes `2026-08-13--macos--s6-part-2-of-2`. **Every entry heading in this
journal uses em-dashes.** The reason A3 could honestly claim zero false positives
when it landed is that nobody had yet written an intra-journal anchor link; the
S6 run wrote the first one, and it has been red since. **It is wrong in both
directions**, which is the part worth fixing before A4 makes anything a gate: a
correct link reads BROKEN, and a link written in the script's own form would pass
the check and be broken on GitHub. A scheduled run *can* fix this one — it is a
script, not a workflow.

**Learned — the Platform vocabulary cannot express "any box except linux", and
the backlog lint is right to stop you inventing one.** I first set E1a's Plat to
`mac/win` and `check-backlog-diff.py` rejected it: only the Status cell may
change on an existing row, and that value is outside the documented set
(`any`/`win`/`mac`/`linux`) anyway. Reverted to `any` and put the constraint in
the Item text where a reader hits it. Recording the mechanism because the next
run to find a mis-scoped row will reach for the same edit: **the correction goes
in prose; the column is Jeff's to change.**

**Also done as STEP 4 chores:** **S6** flipped IN-REVIEW → DONE and moved
verbatim to BACKLOG-DONE.md — [SynthEdit#13](https://github.com/JeffMcClintock/SynthEdit/pull/13)
and [#47](https://github.com/JeffMcClintock/TideSynth/pull/47) both merged
2026-08-13. **There are now zero open PRs in all five repos** other than my own
two.

**STEP 1 / 1.5:** no `platform:linux` issues; no `platform:*` labelled issues at
all. Open issues are TideSynth [#44](https://github.com/JeffMcClintock/TideSynth/issues/44)
(the A6 watchdog digest, `github-actions` — informational, not a build failure)
and `gmpi_ui#1` ("Linux support?", 2024, third-party and unlabelled) — noted, not
acted on, per the issue-authenticity rule. No `tide/linux/**` PR was open, so
nothing was handed back to this platform.

**Jeff's trees, per the three-kinds dirt rule:** all nine repos on this box were
**clean and on their default branches** at claim time — including `SE16`, whose
four dirty Wayland files from the E1 run (2026-08-10) are gone. Nothing of his
was committed, reverted or stashed. `TideSynth` was parked on
`tide/linux/A2-ssh-remote-gap` (left by the 2026-08-13 interactive A11 session,
PR #45 merged); I branched from `origin/main` rather than from it, and restored
the checkout in STEP 5. **A11 still holds:** all nine repos are `https://`, and
STEP 0.7's second assertion printed `git@github.com:`.

**Side effects on this box — all cleaned up, and checked rather than assumed:**
a 1.8 GB scratch build tree under the session scratchpad (outside every repo,
deliberately *not* Jeff's `~/SE/build`, so his tree keeps its own artifacts) and
`gmpi_ui/tests/build-x11/`, both **deleted**; four throwaway Xvfb displays
(`:70`–`:73`), all killed. The commit still adds `tests/build-x11/` to
`.gitignore`, alongside the `tests/build-mac/` line P7b added for the same
reason, so the next person to run the script does not have to remember.
**Unlike the E1 run, this one left no engine state**: every file in
`~/.local/share/SynthEdit/` still predates it (newest 2026-08-11 14:00), because
I built `SynthEditCL` but never executed it.

**Next:**

1. **Merge [gmpi_ui#5](https://github.com/JeffMcClintock/gmpi_ui/pull/5) and
   [#48](https://github.com/JeffMcClintock/TideSynth/pull/48).** Independent —
   unlike C8's pair, merging either alone breaks nothing. #5 is the code.
2. **E1a needs a mac or win run.** The render half is genuinely cheap — download
   the engine, run `tools/render_harness.py`, record two numbers. The judgement
   half is setting the RMS gate from that data, and it cannot start until the
   numbers exist.
3. **A13 is small, real, and this box could have done it** — a script fix, no
   workflow edit, so it is not blocked on Jeff like A4/A10/A12 are. Good
   candidate for the next linux run given how thin the rest of the queue is.
4. **P7 is now fully closed** across all three backends — P4 (win), P7b (mac),
   P7c (X11) — and P7a bounded the editor extents. The remaining P7 offcut is
   **P7d**, which is a scope ruling for Jeff, not work.
5. This box's next linux-eligible item in file order is **P7c's neighbour, N1**,
   which its own row defers behind C7 — so realistically the linux queue is thin
   until C7 lands or E1a moves. Worth Jeff's eye when he next sets NEXT.

**Branch/PR:** `tide/linux/P7c-x11-present-extents` in two repos —
[gmpi_ui#5](https://github.com/JeffMcClintock/gmpi_ui/pull/5) (the guard, the
test, `docs/x11-present-extents.md`) and
[#48](https://github.com/JeffMcClintock/TideSynth/pull/48) (BACKLOG, JOURNAL, and
the closing section of `docs/p7-resize-audit-mac-x11.md`). No other repo was
committed in or modified.

---

## 2026-08-13 — macos — S6 (part 2 of 2)

**Prompt:** `dd93251` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Part 1** of this run is the A11 entry (halt at STEP 0.7, cleared by Jeff
mid-session) on branch `tide/mac/A11-step07-halt`, [#46](https://github.com/JeffMcClintock/TideSynth/pull/46).
This is the item the run went on to take once the assertions passed.

**Did:** Deleted `SE16/SE_IOS_APP/TIDE/Plugins/` — six `.sem` bundles, **26
tracked files, 4.4 MB**. Chose *remove* over the row's "or add a README"
alternative: constraint 7 rules out separately-loadable module bundles
entirely, so a README would preserve 4.4 MB of a contradiction and explain it
rather than fix it.

**Result — the deletion is right, and two of the row's premises were wrong.**

What the files are, measured: all six binaries `Mach-O 64-bit bundle x86_64`,
`platform 1` (macOS) or `LC_VERSION_MIN_MACOSX`, in macOS bundle layout
(`Contents/MacOS/`, `Contents/_CodeSignature/`). Added 2021-02-24→2021-03-03,
**untouched since**. Nothing there can load on arm64 iOS. That much the row had
right.

| the row said | measured |
|---|---|
| "dead **iOS** module artifact", installed by a Run Script to a macOS-only destination | consumer is **`SeAudioUnitMacOS`** — a **macOS** AUv3 app-extension. **No iOS target references the folder at all.** It was never wired into an iOS build; the destination is not a mistake, it is correct for that target |
| the Run Script is the wiring; deleting the folder is safe | the Run Script is only *half*. **Individual files inside the bundles are entries in that target's Resources build phase** — real `CpResource` inputs |

The 2021 commit messages agree with the correction: *"chore(ios) : macOS AUV3
runnning (fixed signing by signing TIDE sems)"*. This is macOS AUv3 scaffolding
that happens to live under an iOS-named folder, which is exactly why it reads as
an iOS module story.

**Verification artifact — and the A/B is not clean, so I am not calling it
clean.**

| `SeAudioUnit macOS`, same machine, same command | before | after |
|---|---|---|
| result | **BUILD FAILED**, RC=65 | **BUILD FAILED**, RC=65 |
| errors | 6 × missing `SE_DSP_CORE/*.cpp` compile inputs | 7 × `CpResource` "couldn't be opened" |

Nothing went from working to broken. But the **failure mode changed**, and the
compile errors stop surfacing afterwards only because `xcodebuild` stops
scheduling once the resource phase fails — they are still there underneath. My
first reading of the pbxproj said the bundles were in no build phase at all;
that was wrong, and the A/B is what caught it. Tracing the six `.sem` *folder*
ids was not enough — they are group entries whose *children* are the build
inputs.

**The standing rule is honoured, and structurally rather than by luck.** Fresh
Ninja configure into a scratch dir (this tree untouched), all four local
overrides, full build: **RC=0, 936/936**, producing `SynthEdit_VST3.vst3`,
`SynthEdit_GMPI.gmpi`, `SynthEditCL.app`, `TIDE.gmpi`, `TIDE_VST3.vst3`. And
**no `CMakeLists.txt` or `.cmake` in `SE16` references `SE_IOS_APP`**, so the
CMake build and that Xcode project are fully decoupled — the deletion could not
have reached them.

**Learned — the big one, and it is much larger than S6:**

**`SE_IOS_APP.xcodeproj` is dead, and it bears on M2.** All four targets fail,
each RC=65, on **28 references to `SE_DSP_CORE/`** — the pre-split name of the
DSP core directory, which no longer exists (it became `SynthEditLib`). The
pbxproj was last touched **2022-12-15**. PLAN calls iOS AUv3 "the constraint
that validates the whole design", and **M2 is written as though a working iOS
project exists to build on. It does not.** M2 is really "author an iOS target",
not "get the existing one green" — worth knowing before anyone estimates it.
Filed as **S10**, with the revive-or-retire decision named as Jeff's.

Two smaller ones:

- **`database.se.xml` is the same architecture constraint 7 forbids.**
  `SE_IOS_APP/TIDE/Resources/database.se.xml` is a 31-entry module database
  naming the six now-deleted bundles by `imbeddedFilename`, and it *is* wired
  into two Resources build phases. Left alone deliberately — outside S6's scope
  — but it should not be revived as-is. Folded into S10.
- **`gh pr edit` fails with the bot's token; `gh api ... -X PATCH` does not.**
  `gh pr edit` issues a GraphQL query touching `login`/`name`/`slug`, which
  needs `read:org`; the bot has `repo` only, by design. The REST route has no
  such requirement. This will bite any run that tries to amend a PR body —
  including a run following STEP 1.5's "push fixes to the SAME branch".

**Next:**

1. **Merge [SynthEdit#13](https://github.com/JeffMcClintock/SynthEdit/pull/13)
   and [#47](https://github.com/JeffMcClintock/TideSynth/pull/47).** Order does
   not matter for the build — #13 is the only code change and it cannot break
   anything the CMake build touches — but merging the docs alone would say a
   deletion landed that did not.
2. **Answer S10 before S9.** If the project is retired, S9 is moot and the right
   move is deleting the whole `.xcodeproj`.
3. **`mac` NEXT moved S6 → P6**, and P6 is genuinely this box's to close: it
   needs an **Xcode**-generator build to reproduce the `CodeSign` failure, which
   Ninja never emits. `SynthEditCL` builds clean under Ninja here (re-confirmed
   RC=0 during this run), so the codesign step is the whole remaining question.

**Tree hygiene:** `SE16` was clean at claim time and only the 26 deletions were
staged — re-checked with `git status` immediately before commit, per the C3
run's lesson about an idle index being harvested. No work of Jeff's was touched.

**Branch/PR:** `tide/mac/S6-dead-ios-modules` in both repos —
[SynthEdit#13](https://github.com/JeffMcClintock/SynthEdit/pull/13) (the
deletion) and [#47](https://github.com/JeffMcClintock/TideSynth/pull/47) (docs).

---
