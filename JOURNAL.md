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

## 2026-08-13 — macos — A11, mac half — halted at STEP 0.7, then resolved in session (part 1 of 2)

**Prompt:** `dd93251` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Outcome, up front: A11 is DONE on all three boxes.** This run halted on STEP
0.7's second assertion; Jeff applied the missing `git config` line while the
session was still live; the assertion and the full acceptance test then passed
and the run continued to S6. **The resolution is at the bottom of this entry;
S6 is [part 2](#2026-08-13--macos--s6-part-2-of-2), its own entry.** The halt
record below is kept unedited, because the deadlock it documents is real and
survives the fix.

**Did:** Nothing. This run stopped at STEP 0.7's second assertion, as the prompt
requires, before selecting or claiming any backlog item. **S6 was not started**
and remains `TODO`. What follows is the halt record plus the read-only
diagnostics needed to make it actionable.

**Result — assertion 1 passed, assertion 2 printed nothing:**

| STEP 0.7 command | required | actual |
|---|---|---|
| `gh api user --jq .login` | `tide-rack-bot` | `tide-rack-bot` ✅ |
| `git config --global --get url."https://github.com/".insteadOf` | `git@github.com:` | **empty, exit 1** ❌ |

That is the A11 gap the linux run found on 2026-08-13, in the one box A11 still
lists as outstanding. `git config --global --get-regexp 'url\.'` returns nothing
at all — setup step 3 has never been applied here. Setup steps 1–2 *are* in
place: `credential.https://github.com.helper` is
`!/opt/homebrew/bin/gh auth git-credential` (Homebrew path, not `gh`).

**The exposure on this box is nil today — and that is a measurement, not an
assumption.** Exhaustive sweep, `find ~ -maxdepth 5 -name .git` (excluding
`Library/`, `node_modules/`, `build/`, `_deps/`, `.Trash/`), **28 repos**:

- **Zero SSH GitHub remotes.** Every GitHub repo is `https://` — including all
  nine fleet repos (`TideSynth`, `SynthEdit`, `SynthEditLib`, `gmpi_ui`,
  `GMPI_Wrappers`, `GMPI`, `GMPI_Adaptors`, `GMPI-plugins`, `gimpi_ui_tests`)
  and the eight `VST_SDK` submodules.
- Non-GitHub and therefore out of scope: `~/SynthEdit` (Azure DevOps),
  `~/Plugins` + four `~/myagent/_work/*/s` build-agent checkouts (Azure DevOps,
  remote named `SSG` not `origin`), `~/MacSIMD` (**no remotes at all**).

So this box matches Windows (22/22 already HTTPS), not linux (8 of 9 SSH). **The
macOS A2 evidence A11 called "void until checked" is not void** — `gmpi_ui#3`/`#4`
and `GMPI_Wrappers#1`/`#2` were pushed from HTTPS remotes here, so those pushes
did authenticate as the bot, not as Jeff via SSH.

**A11's read-only three-way proof, run in full** against the private
`https://github.com/JeffMcClintock/SynthEdit.git`. No push, no write:

| GH_TOKEN | `git ls-remote` | `gh api user` |
|---|---|---|
| bogus | **`fatal: Authentication failed`** — `remote: Invalid username or token.` | — |
| real bot token | succeeds (`b3c1efb07…` HEAD) | `tide-rack-bot` |
| unset | succeeds | `JeffMcClintock` |

Leg 1 is the load-bearing one: git genuinely consults `gh` for HTTPS GitHub URLs
on this box rather than a keychain entry or an SSH key. **So for every remote
that actually exists here, the credential wiring is correct and now proven.**

**What is missing is only the structural safeguard**, which is exactly what A11
says it is for: a future `git clone git@github.com:…` would silently re-open the
hole with no signal, because nothing rewrites it.

**Learned — the finding that matters, and it is new:**

**This box cannot heal itself, and every future scheduled mac run will halt
here.** STEP 0.7 fires before STEP 1 and STEP 2, so a run can never reach the
point of claiming A11 — the assertion that detects the problem also forbids
fixing it ("do not 'fix' it by carrying on as whoever you are"). A11's mac half
is therefore **not takeable by a scheduled run at all**; it needs Jeff or an
interactive session on this machine. Until then the macOS box is a no-op in the
fleet: it will burn a run every week producing this same entry. Linux and
Windows are unaffected — both applied step 3 on 2026-08-13.

That deadlock is not a flaw in the halt rule, which did its job. It is a gap in
A11's framing: the row is written as ordinary backlog work with plat `any`, and
one third of it structurally cannot be done that way.

**Second, smaller:** `ls-remote --get-url origin` echoes the literal string
`origin` when no such remote exists, rather than erroring. A sweep that greps for
`git@` will read those as clean; they need checking with `git remote -v` before
being called clean. Two repos here hit that, both benign.

**I deliberately did NOT apply the one-line fix**, though it is the whole
remedy and I had the evidence for its acceptance test in hand. STEP 0.7 says
stop and do nothing else, and a run that reasons its way past its own failed
safety assertion is the precise failure mode the rule exists to prevent. It is
Jeff's to run, on this box:

```
git config --global url."https://github.com/".insteadOf "git@github.com:"
```

Acceptance is already half-established above: after that command, assertion 2
prints `git@github.com:`, all 28 remotes still read `https://`, and the
three-way proof is recorded here. **A11 can then be flipped DONE across all
three boxes.**

**Also checked, and clear:** no `platform:mac` issues; the only open issue is
[#44 "Fleet watchdog digest"](https://github.com/JeffMcClintock/TideSynth/issues/44)
(author `app/github-actions`, unlabelled — A6's digest, informational). No open
PRs at all in TideSynth, so STEP 1.5 had nothing either. Tree was clean and on
`main`, in sync with `origin/main`; no dirt of Jeff's was touched.

**Journal rotation was skipped on purpose.** `JOURNAL.md` is 78 KB / 11 entries
and is over the 30 KB target, but rotation is STEP 4 work and this run never
reached STEP 4. The next win or linux run should do it.

**One caveat for Jeff before he runs the command**, carried over from the linux
entry: his interactive pushes then resolve through `gh`'s keyring token. If that
token lacks `workflow` scope, a commit touching `.github/workflows/**` is
rejected until `gh auth refresh -h github.com -s workflow` is run once. This did
not bite on Windows (its token already had the scope); **unverified here** — I
did not inspect Jeff's keyring scopes, since doing so is outside a halted run.

**Next:**

1. **Jeff: run the one `git config` line above on this box.** Until then macOS
   contributes nothing and S6 stays untouched.
2. **Rewrite A11's mac line** to say it needs an interactive session, not a
   scheduled run, and record the deadlock above so the next person does not
   re-file it as agent work. Consider whether STEP 0.7 should let a run apply
   *this specific* config repair — I think not, but it should be a decision
   rather than an accident.
3. **`mac`'s NEXT stays S6**, untouched and still eligible, for the first mac
   run after the fix lands.

### Resolution — same session, Jeff applied the fix

Jeff ran the one line on this box while the session was still open. Everything
above stands as written; this is what changed after it.

**STEP 0.7 re-run, both assertions:**

| command | required | actual |
|---|---|---|
| `gh api user --jq .login` | `tide-rack-bot` | `tide-rack-bot` ✅ |
| `git config --global --get url."https://github.com/".insteadOf` | `git@github.com:` | `git@github.com:` ✅ |

**A11's acceptance test, in full, on mac:**

- All **nine fleet repos** still resolve `https://` (`ls-remote --get-url origin`
  — note this applies `insteadOf` rewriting, so it is testing the post-fix path).
- **The safeguard itself demonstrably works**, which is the part the sweep alone
  cannot show: feeding git an explicit `git@github.com:JeffMcClintock/TideSynth.git`
  now resolves to `https://github.com/JeffMcClintock/TideSynth.git`. That is the
  future-SSH-clone hole closed, not merely absent.
- **Three-way proof re-run post-fix, unchanged:** bogus token → auth fails; real
  bot token → succeeds as `tide-rack-bot`; no token → succeeds as
  `JeffMcClintock`.

**So A11 is DONE — linux 2026-08-13, win 2026-08-13, mac 2026-08-13.** Row
flipped in this PR.

**The deadlock finding is not retired by this.** It was resolved by a human
happening to be at the keyboard, which is exactly the circumstance a *scheduled*
run does not have. Had this fired unattended at 03:00, the box would have sat
halted for a week and every subsequent mac run would have halted identically.
The general shape is worth keeping in view: **STEP 0.7 can put a box into a
state that only an interactive session can clear, and nothing in the fleet
notices or escalates.** A6's watchdog digest is the natural place to surface a
box that halted, and does not do so today. Filed as **A12**.

**Branch/PR:** `tide/mac/A11-step07-halt` — TideSynth only, docs only, no code.
The branch is named for the halt that produced it; **S6, the item this run went
on to take, is on its own branch** (see part 2). A later mac run should not treat
this branch as work-in-progress to resume.

---
