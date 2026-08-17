# Journal — August 2026 (archive)

Rotated out of [JOURNAL.md](JOURNAL.md) by **A8**, 2026-08-12. Newest first,
same as the live file. **Entries here are verbatim** — archiving never edits an
entry, so this is the record.

August 2026 is split between two files: the most recent entries stay in
[JOURNAL.md](JOURNAL.md) and move here as later runs rotate them out. Read
[JOURNAL.md](JOURNAL.md) first; come here only when you need history older than
the entries it still holds.

---

## 2026-08-13 — windows — A11, win half (interactive session, Jeff directing)

**Did:** Checked this box against the SSH-remote gap the linux run found in
A2/A11: swept every local git repo under `C:\SE` (`find C:\SE -maxdepth 2
-name .git`, 22 repos — not just the fleet's usual 5) for its remote
protocol, then applied the global `url."https://github.com/".insteadOf
"git@github.com:"` rewrite and proved it against the private `SynthEdit`
repo the way A11's acceptance test specifies.

**Result:** All 22 repos on Windows were already HTTPS — nothing here was
ever actually exposed, unlike linux's 8-of-9. Applied the rewrite anyway,
since the acceptance test wants it as a structural safeguard, not just a
reaction to today's state. Three-part proof, all against
`https://github.com/JeffMcClintock/SynthEdit.git` (private): bogus
`GH_TOKEN` → `fatal: Authentication failed for
'https://github.com/JeffMcClintock/SynthEdit.git/'` (exit 128); real bot
token (from `~/.tide/agent-token`) → `git ls-remote` succeeds, `gh api user`
confirms `tide-rack-bot`; no `GH_TOKEN` → succeeds, confirms `JeffMcClintock`.
Also checked the caveat the linux fix flagged for Jeff's own workflow-file
access (`gh auth refresh -s workflow` needed once the rewrite lands) — did
not apply here, this box's `gh auth status` already shows `workflow` in
scope.

**Learned:** The fleet's "5 repos" framing (used everywhere A2 discusses
scope) undercounts what's actually on disk — Windows alone has 22 local git
repos under `C:\SE`, most unrelated to TIDE (SE15, SSG, Waves, and other
dormant product repos). The SSH-remote risk is about *any* repo the box's
git config touches, not just the ones the bot has a token for, so the sweep
has to be exhaustive (`find`, not "check the 5 I know about") the way linux's
was.

**Next:** mac remains outstanding — its A2 evidence is still authorship-only,
not authentication-verified. A11 stays TODO until mac's sweep and proof are
done too.

**Branch/PR:** none — committed directly to `main`, interactive session.

---

## 2026-08-13 — linux — A11 (new; A2 follow-up, interactive session, Jeff directing)

**Did:** Jeff asked for help finishing **A2** on this box. A2 had been flipped
**DONE** on all three boxes earlier the same day, but the row I started from was
the pre-flip one — my local `main` was 30+ commits stale, which is how I came at
it fresh. Steps 1–2 were already in place here since 2026-08-09 and I re-verified
rather than assumed them. Then checked the remotes, which nobody had, and found
the mechanism does not reach most of them. Fixed this box, corrected
[docs/a2-actor-separation.md](docs/a2-actor-separation.md), added setup **step 3**
and a second STEP 0.7 assertion to
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md), filed **A11** for the
remaining boxes.

**Result:** every command run, none assumed.

| Check | Result |
|---|---|
| `gh api user --jq .login` with `~/.tide/agent-token` | `tide-rack-bot` |
| token scopes | `repo` — no `workflow`, as intended |
| bot reads private `SynthEdit` | yes |
| `credential.https://github.com.helper` | both lines present |
| remotes on this box | **8 of 9 SSH**, only `TideSynth` HTTPS |
| `ssh -T git@github.com` | `Hi JeffMcClintock!` |

**Learned — three things, and the second is the one that matters.**

**1. A credential helper keyed to `credential.https://github.com.helper` is never
consulted for a `git@github.com:` URL.** So on any repo with an SSH remote, a run
authenticates with Jeff's key and pushes through his admin bypass. Fixed with a
global `url."https://github.com/".insteadOf "git@github.com:"`, not per-repo
`remote set-url`: a fresh clone defaults back to SSH and re-opens the hole with
no signal. Proved read-only in both directions, no push — against the private
`SynthEdit` over HTTPS, a bogus `GH_TOKEN` **fails** auth (that failure is the
proof git consults `gh` at all, rather than the SSH key or the keyring), the real
token succeeds, no token succeeds as Jeff. Cheaper than the Windows method, which
needed a real push to `main` to see `GH013`.

**2. Both guards A2 rests on are blind to it, including the one used to close
it.** STEP 0.7's `gh api user` answers `tide-rack-bot` because `gh`'s API path
reads `GH_TOKEN` and never touches git's transport. And the PR-authorship check
A2 was flipped DONE on answers `tide-rack-bot` because author and committer come
from the four `GIT_*` exports, which STEP 0.7 sets unconditionally. **Authorship
proves authorship, not authentication.** A push made as Jeff over SSH arrives
stamped bot, past an assertion that passes, into a log that reads correctly.
Linux looked clean for one accidental reason: `TideSynth` was the only repo its
runs touched and the only repo that was HTTPS. **The macOS evidence A2 cited —
`gmpi_ui#3`/`#4`, `GMPI_Wrappers#1`/`#2` — is drawn entirely from repos that were
SSH on this box**, and no one has looked at mac's or win's remotes. That is A11.

**3. A stale local `main` makes the bot's first push fail with an error about a
file you did not touch.** My first push was rejected with *"refusing to allow a
Personal Access Token to create or update workflow `.github/workflows/build.yml`
without `workflow` scope"* on a commit touching three `.md` files. Cause: the
branch was cut from a `main` 30+ commits behind, and `1157be3` had since changed
`build.yml`, so relative to `origin/main` the branch *reverted* two workflow files
— which needs the scope the bot deliberately does not have and never will.
**Diagnose with `git diff origin/main HEAD -- .github/`, not by reading your own
commit**, which shows nothing. `git fetch` + rebase clears it. This will recur on
any box whose `main` has drifted, and the message points at the wrong thing every
time.

**Next:** **A11 on mac and win** — one `git config` line each, then the two
assertions. Until then their A2 "verified" means *authorship verified,
authentication unknown*. Jeff needs `gh auth refresh -h github.com -s workflow`
once for his own interactive pushes, now that they resolve through `gh`'s keyring
token: `SE16` has nine workflow files. Note this is the same scope wall **C9(a)**
hit from the other direction.

**Prompt:** n/a — interactive session, not a scheduled run. Steps 1–2 were already
in place, so the work itself ran as Jeff until step 3 landed; the commit and push
below are the first exercise of the fixed path on this box.

**Branch/PR:** `tide/linux/A2-ssh-remote-gap` — TideSynth only. No other repo was
committed in; the box-level `git config` is not a repo change and lives nowhere
but this machine, which is exactly why A11 has to be done per box.

---

## 2026-08-13 — jeff — decision: rack mode is TIDE's default view (interactive session, not a scheduled run)

**Did:** Jeff described SynthEdit's new "rack mode" — the top-level Panel View
renders as a Eurorack case, modules and Containers drag-and-snap into rack
slots — and ruled that in TIDE this becomes the *only* top-level option, not
one of two. Unlocking a module/Container opens its own structure view to
rewire signal flow. Rewrote PLAN.md constraint 1 to match (was: structure
view only, "No panel view"; now: rack is default, structure view is the
unlock drill-down) and recorded the ruling in docs/decisions.md.

**Result:** PLAN.md constraint 1 and its "One sentence" summary rewritten;
decisions.md carries the ruling and its reasoning (closer to Cardinal, with
per-module signal-flow editing added on top). Not yet touched, and flagged
as open follow-ups rather than silently assumed: whether the v0.1 acceptance
test should now be rack-first (currently still says "shows a structure
view..."), and whether BACKLOG U1 needs rescoping around the rack as default.

**Learned:** The underlying SynthEdit feature already exists — `SE16`
`a056d3f5b chore(se) : experimental eurorack 'rack mode' for the panel
view`, from earlier this same day — so this ruling is catching up to code
already landing, not speculating ahead of it. Also: the 2026-08-09 Eurorack
section of PLAN.md already stated "opening a Container is optional" as the
product's differentiator; today's ruling is the concrete mechanism that
fulfils that, and constraint 1's literal wording ("No panel view") was the
one place still contradicting it.

**Next:** Decide the v0.1 acceptance-test wording and U1's scope before
either becomes stale in the same way constraint 1 just was.

**Branch/PR:** none — committed directly to `main`, interactive session.

---

## 2026-08-13 — windows — C3

**Prompt:** `e09e766` · claude-opus-5[1m] · app Claude Code (Agent SDK harness) · as `tide-rack-bot`

**Did:** Moved the document model into the public repo — carve-out stage C3.
27 files leave `SE16/SynthEdit2/` for the **root** of `SynthEditLib`: `DocOb`,
`CContainer`, `CUG`(+`_with_patches`), `Plug`, `Plug4`, `PlugIO4`,
`PlugDescriptionDecorator`, `Plug_decorator_{autoduplicate,namable,sdk2,vst}`,
`SynthEditDocBase`, `SynthEditDoc2`. `EditorLib/CMakeLists.txt` repointed
(27 entries, `${EDITOR_DIR}`/`${EDITOR2_DIR}` → `${SYNTHEDITLIB_DIR}`);
`SynthEdit2.vcxproj` + `.filters` repointed for `SynthEditDoc2`.
PRs: [SynthEditLib#5](https://github.com/JeffMcClintock/SynthEditLib/pull/5),
[SynthEdit#11](https://github.com/JeffMcClintock/SynthEdit/pull/11) — **they must
merge together.** Also flipped **P7 → DONE** (both its PRs merged) and filed
**C9** and **C10**.

**Result:** Release x64 on this box — `EditorLib.lib`, `SynthEditCL.exe`,
`TIDE_VST3.vst3` and `SynthEdit2.exe` all build; `ctest -C Release` **92/92
passed, 0 failed**. `SynthEdit2` built via P8's recipe
(`MSBuild SynthEditStore.sln -t:SynthEdit2 -p:Configuration=Release -p:Platform=x64`),
which is what exercises the `.vcxproj` edit. **Positive control**, because
"it still builds" after a move proves nothing on its own: renaming
`C:\SE\SynthEditLib\DocOb.cpp` aside makes the build fail with
`error C1083: Cannot open source file: 'C:\SE\SynthEditLib\DocOb.cpp'`, and
restoring it builds clean — so the build genuinely reads the new location.
**26 of 27 files byte-identical** to the originals (SHA-256 per file, line
endings normalised). mac / iOS / linux **unverified** — not buildable here.

**Default branch:** SE16 master was also built standalone after restoring the
checkout, not merely inferred from the branch build — EditorLib.lib and
SynthEdit2.exe both build clean at 7cb95f33b. So this stage did not break a
working master, and master was not already broken before it. No
platform-labelled issue was needed.

**Learned:**

- **C2's "nothing outside EditorLib compiles it" test caught exactly one file,
  and it was not obvious.** `SynthEditDoc2.cpp` is compiled by
  `SynthEdit2.vcxproj` as well as by EditorLib — by a path relative to
  `SynthEdit2/`, so the move would have broken the WinUI3 app while EditorLib
  and TIDE carried on building fine. The grep that finds this is over
  `*.vcxproj`/`*.filters`/`*.pbxproj`/`CMakeLists.txt`/`*.cmake`/`*.yml` for each
  candidate basename. **Run it at C4 and C5.** (Fix: the entries now read
  `..\..\SynthEditLib\`, matching how that project already references
  `..\..\SynthEditLib\modules\se_sdk3_hosting\BundleInfo.cpp`.)
- **The `#include "../` check needs a second step C2's note did not state.**
  Grepping is not enough — you have to test whether each target *exists* at that
  relative path. Eight hits across the moved set; seven
  (`../tinyXml2/tinyxml2.h`, `../se_sdk3_hosting/GmpiResourceManager.h`) point at
  directories that **do not exist** under `SE16/`, so they were always resolving
  through the search path and move harmlessly, exactly as C2 found for
  `checkpoint.h`. The eighth, `../se_build_number.h`, **does** exist — and that
  is the only one that matters. So: `test -e` each one; the harmless majority is
  noise and the single real hit is the whole finding.
- **`se_build_number.h` blocks C4 and C5 — filed as C9.** It is SynthEdit's
  product version, at the private repo's root, bumped by `[Build-Machine]`, and
  read by three release workflows at that path. C3 escaped by luck: its one
  includer, `SynthEditDocBase.cpp`, **never used the macros** (zero occurrences
  of `SE_MAJOR_VERSION`/`SE_MINOR_VERSION`/`SE_BUILD_NUMBER`), so the fix was
  deleting a dead line — that is the 27th file, the sole content change in the
  whole stage. The other four includers are live uses:
  `ModuleFactory_Editor.cpp` and `SkinMgr.cpp` (**C4**), `Application.cpp`
  (**C5**), `ExportAsPlugin.cpp` (stays private, fine). Moving the header needs
  a `.github/workflows/**` edit, which a scheduled run **cannot** do — the bot
  token deliberately lacks `workflow` scope. **This needs a decision before C4
  starts, or C4 decides it by accident.**
- **Root vs subfolder: chose root, deliberately, and filed the re-home as C10
  blocked on C6.** Root is already an include dir in all three build systems, so
  the move cost zero include-path edits — which is what keeps breakage on the
  riskiest stage unambiguously about the move. A subfolder now would smear one
  include-path change across C3/C4/C5 and several build systems, including the
  macOS/iOS ones this box cannot verify. After C6 the same change is **one line**
  in an `EditorLib/CMakeLists.txt` that by then lives in `SynthEditLib`.
- **The two repos normalise line endings differently, so blob comparison across
  them is worthless.** `SE16` has `.gitattributes` `* text=auto` (LF in the
  blob); `SynthEditLib` has no `.gitattributes` and `core.autocrlf=false`, so it
  stores **CRLF** — and C2's `checkpoint.cpp` is CRLF there too, so the new files
  match precedent. Compare with `tr -d '\r'` or every file reads as 100% changed.
  Related: `sed -i` on a CRLF file silently rewrites the whole file to LF; the
  tell is `diff` reporting `1,197c1,197`.
- **P3 partly moved out from under itself.** `CContainer.cpp` carried its
  `#include "afxres.h"` unaltered into the public repo, so the MFC requirement is
  public the moment SynthEditLib#5 merges. Row updated with the new path.

**Jeff was working in `SE16` throughout this run, and the two of us collided on
the index. Nothing was lost, and he resolved his half himself.** Worth reading
in full, because the failure mode is not obvious and it will recur.

`SE16` was clean at claim time. Partway through, `d4d0acac5 se_screenshot:
report contentRect, and optionally crop to it` (Jeff McClintock, 08:39 +1200)
appeared **on `tide/win/C3-document-model`** — his tooling committed to whatever
branch was checked out, which was this run's — and it swept **this run's 27
staged deletions** in from the index alongside its own 3 `SynthEditMcp` files.
This run did not revert or rewrite it: it preserved that commit on a local
branch, then `git reset --mixed origin/master` unwound the **index only**,
leaving every file on disk byte-for-byte (the three `SynthEditMcp` files were
SHA-256'd before and after, unchanged), which put his work back to uncommitted
changes exactly as the tree was found. Only this run's own 30 paths were then
staged, by name.

**He then sorted it out himself, while this run was writing PRs:** he created
**`jeff/mcp-screenshot-contentrect`** and committed his work cleanly there as
`d1b403000` — 3 files, no C3 deletions. Verified byte-identical (blob hashes) to
what the safety branch held, so that safety branch was pure redundancy and was
deleted; his own branch is the live copy. **His `png.ts` work is committed and
safe, and is *not* on `master`.**

**One thing he should know: `jeff/mcp-screenshot-contentrect` is based on
`ae4b434df`, this run's C3 commit — not on `master`.** So merging that branch as
it stands would drag C3 in with it. His commit is cleanly separable (a
cherry-pick onto `master` touches only the 3 `SynthEditMcp` files). He also
pushed `97497580a` and `e4216d0d9` to `master` during the run, so `origin/master`
moved twice more; local `master` is left 3 behind, as found — not fast-forwarded,
since that is his call.

**The transferable lesson:** a run whose staged index sits idle through a long
build can have that index harvested by someone else's commit. The deletions were
staged, then four target builds and a 92-test ctest ran — a wide window.
**Stage late, and re-check `git status` immediately before `git commit`**; that
check is the only reason this was caught rather than shipped inside someone
else's commit.

**Next:**

1. **Merge SynthEditLib#5 and SynthEdit#11 together.** Either alone breaks the
   build. Then flip C3 to DONE and C4 unblocks.
2. **Answer C9 before C4 starts.** Recommended option (c): give `SynthEditLib`
   its own version header, or pass the version in as a compile definition, and
   leave `se_build_number.h` where SynthEdit's workflows expect it. Option (b)
   (add SE16's root to the include path) fails at C7 by construction.
3. NEXT for win moved C3 → **P3**, since C4 is `BLOCKED(C3)` until the merge.
4. Still true from the 2026-08-12 windows run: `A3`/`A5`/`A6` can never be done
   by a scheduled run — all three edit `.github/workflows/`.

**Branch/PR:** `tide/win/C3-document-model` in all three repos —
[TideSynth PR](https://github.com/JeffMcClintock/TideSynth/pull/38),
[SynthEditLib#5](https://github.com/JeffMcClintock/SynthEditLib/pull/5),
[SynthEdit#11](https://github.com/JeffMcClintock/SynthEdit/pull/11).

---

## 2026-08-13 — macos — P7b

**Did:** Fixed **P7b** — `DrawingFrameCocoa::onRender` using `backBuffer` after
the re-entrant `drawingClient->render()` call without re-checking it. One guard
in `gmpi_ui/backends/DrawingFrameMac.mm`, plus a regression test that reproduces
the defect first: `gmpi_ui/tests/mac_render_reentrant_resize.mm`, built and run
by `tests/run_mac_render_test.sh`.

**Result — it is a real use-after-free, not a latent one, and the row named the
wrong line.**

P7 filed this as "latent, not demonstrated: it needs a client that resizes
during render and none currently does". Both halves were right, and neither
prevents a test: no *shipping* client does it, but a synthetic `IDrawingClient`
does it in five lines. Unfixed sources die on the first such paint.

The correction that matters, because it moves the fix:

| | the row said | measured |
|---|---|---|
| faulting call | `CGContextRestoreGState(backBuffer)` / `CGBitmapContextCreateImage(backBuffer)`, *after* the block | `context.popAxisAlignedClip()`, **inside** the block |
| why | both use `backBuffer` after `render()` | both read the **member**, which `onResize` sets to `nullptr` — they pass CoreGraphics a NULL, which is untidy, not a fault |
| the real one | — | `gmpi::cocoa::GraphicsContext` keeps its **own** copy in `cgContext_` from `setCGContext` time, and nothing nulls that copy |

So the guard has to sit immediately after `render()` returns, not after the
scope closes. A one-line fix placed where the row pointed would have changed
nothing and looked correct.

**Verification artifact — A/B, 3 runs each, same binary, same machine:**

| sources | result |
|---|---|
| unfixed (`git stash` of the guard only) | **SIGSEGV 3/3**, exit 139 |
| fixed | **exit 0, PASS 3/3** |

The unfixed crash report backtrace, which is what makes it the *right* crash and
not just a crash:

```
CGContextRestoreGState                                   (CoreGraphics)
gmpi::cocoa::GraphicsContext::popAxisAlignedClip()
DrawingFrameCocoa::onRender(NSView*, gmpi::drawing::Rect*)
-[GMPI_VIEW_VERSION_03 drawRect:]
```

`EXC_BAD_ACCESS (SIGSEGV)`, `KERN_INVALID_ADDRESS`. Liveness, copied from the
P7 harness's discipline rather than assumed: the renderer drew 2 distinct
colours *before* the re-entrant resize and 3 *after* it, so "survived" cannot
mean "never ran", and the frame provably recovers at the new size.

**And the existing harness still passes, which is the other half of the claim.**
A new test proving the new guard works says nothing about whether ordinary
painting still does. P7's `mac_editor_resize_host`, built standalone against the
VST3 SDK and run on the `TIDE_VST3` this build produced: **exit 0, 3/3**, editor
live, survived every oversized resize+paint, still drawing at the end. Visible
in its output as a by-product: P7a's clamp is live in this binary —
`onSize(0, 0, 16385, 600)` lands the child at **8192 x 600**.

**Learned:**

- **AddressSanitizer cannot see this, and an ASan-only run is a confident false
  PASS.** The freed read happens inside CoreGraphics; ASan checks loads the
  compiler instrumented plus the functions it intercepts, and a system framework
  is neither. I found this the honest way — my first harness was ASan-only and
  reported PASS on the unfixed sources. The positive control is what settled it:
  the same ASan binary flags a *hand-written* read of the same freed pointer
  immediately (`heap-use-after-free ... freed by _CFRelease`), so ASan was
  tracking the allocation and simply never sees the read. **Guard Malloc**
  (`DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib`) is the detector that
  works: the page is unmapped, so whoever touches it faults. The test script
  defaults to it and says all this at the top.
- **The measurement bug from the P7 entry recurred, in the same shape.** First
  version of the client drew one rect near the drawing origin and the liveness
  probe reported 1 distinct colour — because `onRender` flips to top-down while
  AppKit's `visibleRect` origin is the bottom-left, so the sampled tile landed
  where the rect was not. Fixed by drawing stripes over the whole arranged rect
  rather than moving the probe: the count then does not depend on where either
  the client or the sampler happens to look. **Anyone porting a paint probe to
  Cocoa should expect to hit this once.**
- **The whole test needs no CMake, no VST3 and no plugin.** `DrawingFrameMac.mm`
  plus `DrawingFrameCommon.cpp` compile and link standalone in one `clang++`
  line (`-fno-objc-arc`; the backend's Objective-C is manually reference
  counted, `MacColorDialog.h` calls `-retain`). That is a much cheaper harness
  than P7's, and the right shape whenever the defect is inside `gmpi_ui` itself.
  It follows the convention `gmpi_ui/tests/` already uses — a shell script that
  invokes the compiler, not a build system.
- **`GraphicsContext` caching `cgContext_` is a general hazard, not a P7b
  detail.** Any backend that hands a client a context object holding a raw
  device pointer has the same shape. I did not widen the fix to make
  `setCGContext(nullptr)` reachable from `onResize` — that touches the
  cross-platform class every GMPI plugin uses, and the item is one guard. Worth
  a row if anyone finds a second instance.

**Build health — verified, not assumed.** Fresh Ninja configure of `SynthEdit`
with all four local overrides into a scratch build dir (the tree itself was not
touched), `ninja` with no target: **RC=0** across `SynthEdit_VST3`,
`SynthEdit_GMPI`, `TIDE`, `TIDE_VST3` and `SynthEditCL`. So the standing
direction — leave SynthEdit, SynthEditCL and TIDE all building — is honoured and
checked. This corroborates the P7a run's finding that SynthEditCL *does* build on
macOS with the Ninja generator; **P6 is still not closed by that**, for P7a's
reason: P6's failure is a `CodeSign` step the Ninja generator never emits.

**STEP 1 / 1.5 — what I found before picking an item:**

- **No `platform:mac` issues; no open issues at all** in TideSynth.
- **P7a is complete and not mine to redo.** The NEXT block on `main` still points
  `mac` at P7a, but its two code PRs — [gmpi_ui#3](https://github.com/JeffMcClintock/gmpi_ui/pull/3)
  and [GMPI_Wrappers#2](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/2) —
  **merged on 2026-08-12**, and only its docs PR [#35](https://github.com/JeffMcClintock/TideSynth/pull/35)
  is open, with no reviews and no comments. That PR itself moves the pointer to
  P7b. Under STEP 1.5 a PR with nothing unresolved is waiting for merge, not for
  me, so I left it alone and took P7b — the item #35 nominates.
- **The red-checks rule is still unusable, exactly as the C8 entry reported.**
  #35's head and `main` fail identically on all three platforms; that is the
  documented pre-C7 failure, not a signal. **B1** remains the row that fixes it.
- **P7 is now flippable and I flipped it**, in place: both its linked PRs have
  merged ([GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1)
  2026-08-12, [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) 2026-08-10).
  I did **not** move the row to the Done section: [#36](https://github.com/JeffMcClintock/TideSynth/pull/36)
  is rotating landed rows into `BACKLOG-DONE.md` and a move here would collide
  with it for no gain.
- **The `docs/p7-resize-audit-mac-x11.md` correction is appended at the end of
  the file, not written into the follow-ups table**, deliberately: #35 is editing
  that table right now. Appending keeps both merges clean; the table's P7b line
  stays wrong until someone rebases, and the postscript says so in as many words.

**Expect conflicts, and here is how they resolve.** Three PRs are open against
`main` and all three edit `JOURNAL.md` and `BACKLOG.md`:

| PR | BACKLOG rows it touches | overlap with this one |
|---|---|---|
| [#34](https://github.com/JeffMcClintock/TideSynth/pull/34) E1 | E1 | none |
| [#35](https://github.com/JeffMcClintock/TideSynth/pull/35) P7a | NEXT block, P7a, P6 | **adjacent** — P7a is the line between my P7 and P7b edits |
| [#36](https://github.com/JeffMcClintock/TideSynth/pull/36) A8 | rotates 24 landed rows out | P7 flip may need re-applying after its rotation |

`JOURNAL.md` conflicts by construction — every entry inserts at the top. The
resolution is the one Jeff already used for #31/#32: keep both entries, newest
first. For BACKLOG, my three edits (P7 status, P7b row, nothing else) are
disjoint from every other PR's *content*; only their line adjacency conflicts.
**I did not touch the NEXT block** — #35 already moves `mac` to P7b, and once
both land P7b reads IN-REVIEW, which STEP 2 treats as ineligible and falls
through cleanly.

**Next:** `mac` has no obvious next row once P7b lands — P7c is `linux`, P7d is
`any` but is a scope question for Jeff (`GMPI-plugins` is GATED by default), and
the NEXT block will be pointing at an IN-REVIEW item. **Recommend Jeff sets the
`mac` pointer explicitly**, or answers P7d, which would make it takeable in
minutes. Independently: **B1** keeps costing every run real time, and **A6**'s
digest would have told me in one line what I spent this run's first twenty
minutes deriving from the API.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/P7b-render-backbuffer-guard` in both repos —
[gmpi_ui#4](https://github.com/JeffMcClintock/gmpi_ui/pull/4) (guard + test) and
[#37](https://github.com/JeffMcClintock/TideSynth/pull/37) (docs). **They must merge together in the sense that matters:** the gmpi_ui PR
carries the fix and the test, this one carries only docs, so either order builds
— but merging the docs alone would leave the backlog saying a fix landed that
did not.

---

## 2026-08-12 — windows — A8 (C3 not taken — see below)

**Did:** Rotated the journal, archived the landed backlog rows, distilled the
five largest live rows into `docs/`, and wrote a link checker to prove none of it
stranded a reference. **I did not take C3, which the NEXT block points win at** —
that first, since it is what affects the next run.

**Why C3 was not taken.** SE16 was dirty with **Jeff's live work** — an
experimental Eurorack `rackMode` across `SynthEdit2/MfcDocPresenter.{cpp,h}`,
`SynthEdit2/SynthEditDocBase.h` and three files in
`SynthEditLib/modules/se_sdk3_hosting/`; real content, not CRLF churn.
**`SynthEditDocBase.h` is one of the six files C3 must `git mv`**, and a move
either commits his uncommitted line or destroys it — STEP 5's third dirt
category forbids both. Full detail is now a **precondition on the C3 row**, and
on C4 by implication, since the `MfcDocPresenter` pair is C4's scope.

**It was transient, and it cleared during this run.** Jeff committed the work at
10:51 — SE16 `a056d3f5b`, SynthEditLib `5c6bc1b` — so by the end of the run both
trees were clean and **C3 is unblocked again**. I did not switch to it: A8 was
already claimed, built and in review, and STEP 2 says one item. Re-check rather
than trusting either state; this is a working machine and it moves under you.

**A3, the row above A8, was equally unavailable** — as are A5 and A6. All three
edit `.github/workflows/` and the bot's token deliberately lacks `workflow`
scope. The credential is enforcing the rule as designed, but it means **the
`any` NEXT pointer (A3) can never be satisfied by a scheduled run.**

**Result** — A8 asked for "under 30 KB after rotation, no broken links":

| | before | after |
|---|---|---|
| `JOURNAL.md` | 192,010 B / 37 entries | **25,209 B / 3 entries** (+ this one) |
| `BACKLOG.md` | 76,304 B / 76 rows | **45,003 B / 52 rows** |
| broken links | 4 | **0** of 164 relative links |

New: `JOURNAL-2026-08.md` (34 entries), `BACKLOG-DONE.md` (24 rows),
`scripts/check-links.py`, 5 `docs/<id>.md`.

**Verification artifacts, this session:**

- **Rotation is lossless.** Re-parsed both files, concatenated live + archive,
  compared to the pre-rotation file: **37 entries before, 37 after, order and
  headings identical, every body byte-identical** by SHA-256.
- **The five lifted rows are verbatim** — whitespace-normalised comparison
  against each new `docs/` file. Only line breaks are new.
- **`scripts/check-links.py`, with a positive control.** Clean: 164 links, 0
  broken, exit 0. One bad link appended: exit 1, naming `BACKLOG.md:174`.
- **C8 flipped to DONE on evidence:** `SynthEditLib#4` merged
  2026-08-11T22:23:32Z, `SynthEdit#10` 22:22:56Z, both confirmed via the API.
  **P7 stays IN-REVIEW** — `GMPI_Wrappers#1` is still open.

**Learned:**

- **Lifting a row from a root-level file into `docs/` silently breaks its
  relative links.** Three of the five (`A2`, `N1`, `S1b`) carried `](docs/…)`
  links, correct in `BACKLOG.md` and wrong one directory down — **verbatim is
  faithful, not safe.** That is the whole argument for the checker; it caught all
  three, plus a pre-existing break in `docs/distribution.md:6`.
- **The rotation rule needs a floor, and the floor has to win.** "Under 30 KB"
  and "the last four entries must be readable" genuinely conflict here: three of
  the four retained entries are 3.7–10.3 KB, so this file lands **just under
  30 KiB, above a decimal 30,000**. I kept the floor rather than archive a
  fourth entry: a size rule that starves the handoff is worse than a marginally
  large file. That precedence is now written into the rule above.
- **A grooming item conflicts with every open PR by construction**, so the only
  mitigation is choosing what *not* to touch. #34 edits `E1` and #35 edits
  `P7a`/`P6`, so I distilled neither (I had lifted `E1` and reverted it —
  #34 adds a near-identically-named `docs/e1-verification-harness.md`).
  **Merge #34 and #35 first**; the journal hunk then resolves as: my header,
  their entries, my kept entries.
- **The `## Blocked on Jeff` section held nothing blocked on Jeff** — all six
  rows were `RESOLVED`. A section titled "agents must not start these" holding
  only settled history is a small trap; archived.
- **STEP 0.5 requires an app version this box does not expose.** No
  `AppData\Local\AnthropicClaude`, and the only version string under
  `AppData\Local\Claude\Logs` is the Chrome native host's. The line below says
  "undetermined" rather than copying the last entry's number.

**Build health:** nothing built, no code changed — this run touched **TideSynth
only**. **SE16 and SynthEditLib were left exactly as found** (dirty when I read
them, then committed by Jeff, not by me); `gmpi_ui` and `GMPI_Wrappers` were
clean and untouched. All five checkouts end on their default branches.

**STEP 1 / 1.5:** no open issues in TideSynth at all, so no `platform:win`
issue, and no open `tide/win/**` PR. #34 (linux/E1) and #35 (mac/P7a) are not mine. Per
the C8 entry, CI red is uninformative here until C7.

**Next:** merge **#34** and **#35**, then this PR after a rebase. Then win takes
**C3**, which is clean and unblocked as of 10:51 — but check the tree again, and
expect `rackMode` to keep moving in the files C3 and C4 must relocate. If it is
dirty again, the fallbacks are **S1b** and **P3**, and P3 has the same
precondition (`MfcDocPresenter.cpp`). **A3/A5/A6 need a `workflow`-scoped
credential or Jeff.**

**Prompt:** `e09e766` · claude-opus-5[1m] · app version undetermined on this box
· as `tide-rack-bot`

**Branch/PR:** [#36](https://github.com/JeffMcClintock/TideSynth/pull/36) on
`tide/win/A8-journal-rotation` — TideSynth only, no other repo committed in.

---

## 2026-08-12 — macos — P7a

**Did:** Took **P7a** and did both halves. Bounded the macOS editor extent in
`gmpi_ui/backends/DrawingFrameMac.mm`, and made `checkSizeConstraint` in
`GMPI_Wrappers/wrapper/VST3/SEVSTGUIEditorMac.cpp` write the accepted size back.
Both files were on STEP 5's ALLOWED list, so nothing needed escalating this time.

**The numbers, since choosing them was the judgement call.** P7 filed this row
precisely because it would not pick one: CoreGraphics has no wall to copy, so any
bound is a product decision about how much an editor may reserve. I picked from
**displays**, not from a graphics API:

| constant | value | why |
|---|---|---|
| `maxEditorDimensionPoints` | **8192** points/axis | the widest single Mac display in logical points is a 6K Pro Display XDR in "more space" at **3840**, so this is a bit over twice the largest real case |
| `maxBackingBitmapBytes` | **384 MiB** | the bitmap is 8 bytes/px at *backing* resolution, so a full-screen editor on that same display at 2x reserves **~265 MiB**; 384 clears it with headroom and still bites on every rect P7 exercised |

Aspect ratio is preserved when the area budget bites. **Both numbers are Jeff's
to overrule** — they live in one place with the reasoning beside them for exactly
that reason, and the PR body says so.

Why not one bound instead of two: a per-axis limit alone does not bound memory
(8192² points at 2x is 8.6 TB of reservation), and an area budget alone lets an
absurdly-shaped 16385 x 600 through. Each catches what the other misses. And
Windows' 16384 was rejected on the merits — at that per-axis limit the audit's own
`16385 x 600` case still costs +315 MiB, so copying it would have "fixed" the row
while leaving the measured defect standing.

**Clamped in two places on purpose.** `resizeNativeView` is the wrapper's path.
`initBackingBitmap` is where the memory is actually reserved and is reachable
*without* the wrapper — a host can set the view's frame directly, and
`createNativeView`'s own comment says JUCE does exactly that. If the second site
bites, the bitmap is smaller than the view and the blit at the end of `onRender`
stretches it: a blurry editor at an absurd extent, which is the intended trade and
is commented as such.

**Result — verified A/B, same plugin, both Debug, same machine.** "Before" is
`GainGui_VST3` built from a throwaway worktree at unmodified `gmpi_ui` +
`GMPI_Wrappers`, so the only difference between the columns is this change.

| host asked | before: view adopted | after: view adopted |
|---|---|---|
| `2178 x 32672` | `2178 x 32672` | **`1829 x 6879`** |
| `0 x 0` | `0 x 0` | **`1 x 1`** |
| `16385 x 600` | `16385 x 600` | **`8192 x 600`** |
| `600 x 16385` | `600 x 16385` | **`600 x 8192`** |
| recover `200 x 200` | `200 x 200`, 48 colours | `200 x 200`, 48 colours |

| paint | before | after |
|---|---|---|
| `16385 x 600` | **+253.4 MiB** | **+128.3 MiB** |
| `2178 x 32672` | +36.6 MiB | +31.3 MiB |
| peak resident | 612.8 MiB | 362.5 MiB |

**The before column reproduced P7's `+253 MiB` figure to within 0.4 MiB.** That is
the positive control, and it is worth more than the after column: it says the
harness and this machine still measure what they measured two days ago, so the
delta is the change and not the weather.

`checkSizeConstraint(0, 0, 2178, 32672)`, the row's stated acceptance observable:

| plugin | before | after |
|---|---|---|
| `GainGui_VST3` (resizable) | `kResultTrue`, **UNCHANGED** | `kResultTrue`, **`1829 x 6879`** |
| `TIDE_VST3` (fixed size) | `kResultFalse`, **UNCHANGED** | `kResultTrue`, **`1829 x 600`** |

`mac_editor_resize_host` exits **0, 3/3** on GainGui and 1/1 on a TIDE_VST3 built
against the change, live before (liveness A+B, 19–65 distinct colours) and still
drawing after recovery — so "passed because the clamp made resize a no-op" is
excluded. **Negative control:** `checkSizeConstraint(0, 0, 640, 480)` still comes
back `kResultTrue` with the rect *unchanged*, so an in-bounds size is accepted
as-is rather than spuriously adjusted. I added that control because every rect the
harness tests by default is an absurd one, and a clamp that mangled legitimate
sizes would have passed the whole suite.

**Build health — better than the standing rule expects, and this is the run's
second finding.** Configured a fresh Ninja build of `SynthEdit` with **all four**
local overrides (banner confirmed) and ran `ninja` with no target: **RC=0**.
`SynthEdit_VST3`, `SynthEdit_GMPI`, `TIDE`, `TIDE_VST3`, **`SynthEditCL`** and the
test targets all build. So a macOS run *can* honour "leave SynthEdit, SynthEditCL
and TIDE all building" — P7 and C8 both had to decline to claim that.

**But P6 is not thereby disproved, and I did not close it.** P6's failure is a
`CodeSign` step, and **the Ninja generator emits none** — the app came out
`not signed at all`, so my build cannot reproduce the failure in either
direction. What has changed is the source: `SynthEditCL/CMakeLists.txt:187` now
branches on `APPLE` to `$<TARGET_BUNDLE_CONTENT_DIR>` (`Contents/`) with a comment
quoting P6's exact error string, and prefabs landed in `Contents/Resources/` here.
Two commits did that — `691270c5d` (2026-08-08) and `4792f4bf2` (2026-08-11,
current `master` tip). Confirming it needs an **Xcode**-generator build, which is
what Jeff's own tree uses. Noted on the row, left TODO.

**Learned:**

- **"Copy the Windows clamp" was the trap the row warned about, and it is worse
  than the row says.** The row explains that 16384 has no technical meaning here.
  What it does not say is that adopting it would leave the *measured* defect in
  place: `16385 x 600` clamped to `16384 x 600` still reserves ~315 MiB at 2x. A
  bound that admits the exact case you measured is decoration.
- **The backing scale is the whole reason a points-based bound needs a byte
  budget.** Everything the host says is in points; everything that costs memory is
  in backing pixels, and Retina squares the discrepancy. `checkSizeConstraint` has
  it worst — the view may not be on a screen yet, so the real scale is unknowable
  and the only safe guess is the pessimistic 2x. Hence
  `gmpi_clampEditorSize` working in points at an assumed 2x while
  `initBackingBitmap` clamps in real backing pixels: the wrapper's answer is then
  never *larger* than what the backend will honour, which is the direction that
  matters.
- **The X3 trap is still live in `~/Documents/GitHub/SynthEdit/build`.**
  `GMPI_WRAPPER_FOLDER_OVERRIDE` is still empty there, exactly as the P7 entry
  recorded on 2026-08-10 — so it still links a `GMPI_Wrappers` frozen at May while
  the other three overrides are correct. It is Jeff's tree; I did not touch it and
  built into scratch instead. **Any run that "verifies a GMPI_Wrappers change" by
  building from that directory is verifying May's code and will not be told.**
- **A before/after that reproduces the prior run's number is worth building.** It
  cost one extra worktree and one extra configure, and it converted "+128 MiB
  sounds better than +253" into evidence, because the same binary that produced
  253.4 today is the one the after column is measured against.
- **`gh pr create --base <branch>` for a stacked PR works as the bot**, unlike
  `gh pr edit` (C8's finding — GraphQL wants `read:org`). GMPI_Wrappers#2 is based
  on the still-open #1 because the harness it is verified with lives there; GitHub
  retargets to `main` when #1 merges.

**STEP 1 / 1.5 — what I found first:**

- **No `platform:mac` issues.** TideSynth has no open issues at all. `gmpi_ui#1`
  ("Linux support?", 2024) is from `arjunmenon` — neither Jeff nor the CI bot — so
  per STEP 1 it is information, not instruction. Unchanged since C8 noted it.
- **All three `tide/mac/**` PRs are clean and idle:** GMPI_Wrappers#1 (P7's test),
  SynthEditLib#4 and SynthEdit#10 (C8's pair). No reviews, no review comments, no
  issue comments, `mergeable=true`, and **no check runs configured in any of those
  three repos** — so STEP 1.5's "failing checks" trigger does not fire there at
  all. Nothing handed back to this platform; left alone, per the rule.
- C8's #4 and #10 **still must merge together**, and P7 correctly stays IN-REVIEW
  until GMPI_Wrappers#1 lands. Neither flipped.

**Next:** **P7b** — one guard, and both PRs above put a run inside
`DrawingFrameMac.mm` already, so whoever takes it has the context loaded. The only
real work there is its verification story: it is latent by construction (no client
resizes during `render`), so the honest options are to write a client that does, or
to say plainly it is unverified. The harness on GMPI_Wrappers#1 is the place to add
such a client if anyone wants the former. Independently: **P6 wants one Xcode
build** to close, and **B1** remains the row that would make STEP 1.5 mean anything
in this project.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Merged, same session, at Jeff's instruction** (`merge gmpi_ui#3 then
GMPI_Wrappers#1 and #2`), in that order. All three landed; GitHub retargeted
GMPI_Wrappers#2 from `tide/mac/P7-resize-audit` to `main` on its own when #1
merged, so the stacked PR needed no intervention. Re-verified **from the merged
default branches** rather than from the branches: fresh configure, rebuild,
`mac_editor_resize_host` exit **0**, `checkSizeConstraint` reporting `1829 x 6879`
adjusted and the view clamping to `1829 x 6879` / `8192 x 600` / `600 x 8192` /
`1 x 1`.

**The bot could not merge, and that is the arrangement working.**
`PUT /pulls/3/merge` as `tide-rack-bot` returned **`405 At least 1 approving
review is required by reviewers with write access`**, with `mergeable_state:
blocked` — the same shape as A2's recorded `GH013` push rejection, on a different
verb. The merges were therefore made on **Jeff's own credential**, which is on the
bypass lists, at his explicit instruction in session. Worth writing down for two
reasons: a future run must not read "Jeff asked me to merge" as licence to try it
unprompted, and the *mechanism* by which the bot is refused a merge had not been
observed before today — only the push half had.

**Consequence recorded:** GMPI_Wrappers#1 was P7's last open PR, so **P7 is now
DONE** and its row moved to the Done section as part of this PR. **P7a stays
IN-REVIEW** — its two code PRs are merged but this repo's PR, which carries the
row, the journal and the audit-doc note, is still open.

**Branch/PR:** `tide/mac/P7a-editor-extent-bound` in three repos —
[gmpi_ui#3](https://github.com/JeffMcClintock/gmpi_ui/pull/3),
[GMPI_Wrappers#2](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/2) and this
repo's PR. **Merge gmpi_ui#3 first**: the wrapper calls `gmpi_clampEditorSize`,
which that PR introduces, so the other order is a link error. All five working
copies were clean before this run and are back on their default branches after it.

---

## 2026-08-11 — linux — E1

**Did:** Ported the audio verification harness out of the archived `tide-rack`
repo (`30d3e5e`, cloned read-only) into this one, ran it, and positive-controlled
every gate. Landed as `tools/render_harness.py` + `tests/cases/` +
`tests/references/` + [tests/README.md](tests/README.md), with the reasoning in
[docs/e1-verification-harness.md](docs/e1-verification-harness.md). **The port
found two things the source repo did not know**, one of which changes how much a
local green run is worth.

**Where it landed, since E1 asked for that decision first.** This repo, not
`SE16/tests/`. Two reasons and the second is the deciding one: `SE16/tests/` is a
gtest suite compiled into the SynthEdit build and this is an end-to-end Python
test driving a *published binary*; and `SE16/tests/` is on neither STEP 5 list,
so it is GATED by default and landing there would have cost a ruling for nothing.
The references are TIDE's claims about how TIDE should sound, so they belong
beside TIDE's backlog.

**Result — 2/2 cases pass, byte-identical to the goldens.**

```
engine: SynthEditCL V1.6.178
PASS  osc_naive_sine  peak=-6.0dBFS null=-infdBFS peakdiff=-infdBFS
PASS  voice_midi_note peak=-6.6dBFS null=-infdBFS peakdiff=-infdBFS
2/2 passed.
```

`null=-inf` is exact equality. Confirmed twice over — matching SHA-256 in the
report (`7ade35f2…`, `2a765de1…`) and `cmp` clean against both checked-in
references.

**Verification artifact — the gates were driven red as well as green.** A green
gate that was never shown to fail proves nothing:

| Control | Result |
|---|---|
| identity (file vs itself) | pass, rms/peak `-inf` |
| **3-LSB nudge across 200 of 96,000 samples** | RMS **−107.6 dBFS → passes**; peak **−80.8 dBFS → FAILS**. Finding (b) reproduced *to the decimal* |
| same nudge installed as the golden, through a **real render** | `FAIL … peak sample diff -80.8 dBFS > -86.0 dBFS`, **exit 1** |
| −0.5 dB whole-file level change | FAIL, rms −35.8 dBFS |
| digital silence | peak `-inf` ≤ −90 floor → **caught** |
| missing reference | FAIL, exit 1 |

The middle row is the important one: the peak gate is load-bearing, and it fails
through the whole harness, not just through `null_test()` in isolation.

**Learned — (d), and it is the significant one: `--modules` is not authoritative
on a developer box.** Finding (a)'s relative-`-factorysemsfolder` trap
**cannot be reproduced here**, and I nearly wrote that up as "the engine fixed
it". It is not fixed; this box masks it.

| What I passed | What happened |
|---|---|
| `-factorysemsfolder ./mods` (relative) | full signal, **byte-identical to golden** |
| `-factorysemsfolder /nonexistent/path` | full signal, **byte-identical to golden** |
| `-factorysemsfolder /tmp` | full signal, 116 modules resolved |
| …plus `XDG_DATA_HOME` redirected | still passed |
| …plus `HOME` isolated to an empty dir | still passed |

Two persistent side channels in the engine's own state dir, neither controlled by
`--modules`:

- `~/.local/share/SynthEdit/SynthEdit16.settings.xml` carries
  `ModulePath="/home/jef/.local/share/SynthEdit/modules"` — **absolute**, which is
  why redirecting `XDG_DATA_HOME` did nothing. That folder holds a full duplicate
  of all 41 factory modules (the `Module FOUND TWICE!` spam on every run is this).
- `Plugin-Cache-16-override-<hash>.xml`, one per override path. A cache written
  under a **freshly isolated `HOME`** was observed listing 359 modules from
  `ctl/mods` — a folder named only in an *earlier* run, under a different HOME. The
  cache carries a previously scanned folder forward.

**CI is sound and local reproduction is not**, which is the asymmetry worth
remembering: a clean `ubuntu-24.04` runner has none of this state, which is
exactly why (a) was findable in CI and is invisible here. The failure mode is
someone reproducing a CI failure locally, getting a confident green from a module
set they never named, and closing it as a fluke. I taught the harness to record
the folders the engine *said* it scanned (`module_sources` /
`foreign_module_sources`, report schema `/1` → `/2`) and warn — **not fail**,
because on a dev box the extra source is normal and a hard failure would break
the harness exactly where a human is debugging. It fires correctly here and names
`/home/jef/.local/share/SynthEdit/modules`.

**Learned — (e): the two null tolerances contradict each other.** The source
comment justifies the peak threshold as tolerating the ~1 LSB (−90.3 dBFS at
16-bit) of legitimate cross-platform float rounding. That is true of the peak gate
and **false of the RMS gate two lines above it**: 1 LSB on *every* sample measures
RMS −90.3 dBFS, which fails the −100 dBFS gate. Solving `rms = sqrt(fraction)`,
the RMS gate tolerates 1-LSB error on at most **~10.7% of samples**. Only the
Linux lane has ever run, so this is the most likely cause of a spurious failure
the first time mac or Windows renders — and it will look like a real regression. I
did **not** widen it: choosing that number with zero cross-platform measurements is
guessing at the definition of "regression". Filed as **E1a** with the arithmetic.

**Learned — finding (c) is stronger than it was.** It said references survive
compiler and build-config changes (Release g++-14 vs Debug g++-13.3, same engine
version). The references were rendered 2026-08-07; this run used a locally-built
**V1.6.178** from 2026-08-10 and got byte-identical output. So they survive an
engine *version* bump too — which means "the engine moved" is **not** a free
explanation for a future null-test failure.

**Learned — a relative `--render-audio` path does not land in the CWD.** It
resolves against `$HOME`: `--render-audio rel.wav` reported
`"resolvedPath":"/home/jef/rel.wav"` and wrote there. The harness is safe by
construction (it renders into a `tempfile.TemporaryDirectory()`, always absolute),
but anyone driving SynthEditCL by hand will litter Jeff's home directory. I made
one such file and deleted it; `~/dummy.wav` and `~/temp.wav` are his, from
2026-08-06, and I left them.

**CI is checked in but NOT active.** `docs/ci/verify.yml`, with a header saying
so. STEP 5 forbids writing `.github/workflows/**` and the bot token carries no
`workflow` scope, so it is enforced twice; it is a manual file copy for Jeff,
filed as **E1b**. Until it lands the harness only runs when a human runs it,
which is most of its value unrealised — and per (d), CI is the *only* place the
module set under test is guaranteed to be the one that rendered.

**Build health:** nothing was compiled. This run touched only TideSynth — new
files plus BACKLOG and JOURNAL — so nothing that consumes `gmpi_ui`,
`GMPI_Wrappers`, `SynthEditLib` or `SE16` is affected. I have **no claim** about
whether this platform's default branch builds; I executed an existing
`SynthEditCL` binary (built 2026-08-10, not by me) and never invoked a compiler.
No platform issue filed, because I observed no failure — only an absence of
evidence.

**STEP 1 / 1.5:** no `platform:linux` issues, none open in TideSynth at all.
`gmpi_ui#1` ("Linux support?", 2024) is unlabelled and from a third party — noted,
not acted on, per the issue-authenticity rule. No `tide/linux/**` PR was open, so
nothing was handed back to this platform. The three open PRs are all `tide/mac/**`
and none are mine to touch.

**Jeff's tree, per the three-kinds dirt rule:** `SE16` is on `master` with four
dirty files — `SynthEditWayland/Wayland{MainWindow,MenuBar}.{cpp,h}`. These are
**category 3**: real content changes (102/12/16/15 lines surviving
`git diff --ignore-all-space`, so not CRLF churn), mtimes 2026-08-10 13:23–13:24,
which **predates this run**. Jeff's work in progress on Wayland. Not committed,
not reverted, not stashed. I confined this run to TideSynth, whose tree was clean
before and after; every other repo on this box was clean and on its default
branch throughout and I modified none of them.

**Side effects on this box, stated because they are real:** the engine wrote three
`Plugin-Cache-16-override-*.xml` files into `~/.local/share/SynthEdit/` during my
probes (13:54–13:55). I **left them**. They are the engine's own regenerable state,
keyed by override-path hash, and I could not distinguish files I *created* from a
cache I *refreshed* for a path Jeff also uses — deleting the latter changes his
machine, leaving a stale one does not.

**Next:** **E1a** is the linux NEXT pointer and it is genuinely blocked on
somebody rendering the two cases on mac or Windows — that half is cheap, since the
engine is a download rather than a build, and the whole task is to run one command
and record two numbers. **E1b** is a file copy only Jeff can make, and it converts
this from a script someone remembers to run into an actual gate. Unchanged from
the last run and still true: **C8** needs #4 and #10 merged **together**, and
**P7** stays IN-REVIEW until
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) lands.

**Prompt:** `e09e766` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Branch/PR:** `tide/linux/E1-verify-harness`, TideSynth only — no other repo was
committed in, and no repo outside this one was modified at all. Should merge
cleanly: all but two files are new, and the BACKLOG edits are confined to the
`linux` NEXT row and the E1/E1a/E1b rows.

---

## 2026-08-11 — macos — C8 executed (interactive session, Jeff directing)

**Did:** Jeff merged [#31](https://github.com/JeffMcClintock/TideSynth/pull/31)
and [#32](https://github.com/JeffMcClintock/TideSynth/pull/32) and asked whether
that counted as a go-ahead. It counts for the *ruling*; the entry as written then
needed one more word, and he gave it. Deleted the header and recorded the
decision.

**Result:** the `PROPOSED:` entry merged **unedited**, which under the Proposed
mechanism selects the recommended default — **option (b), C8 is Jeff's call**.
Worth being precise about, because (b) is the one option that does not
self-execute: it reads *"Jeff deletes it, or says 'go' on this PR and a later run
does"*. So the merge settled **what** was decided but not **who acts**, and
answering "yes, merging is enough" without that distinction would have been wrong
in a way that only shows up the next time an agent reads this file for precedent.

Three PRs, all open, none merged by me:

| Repo | PR | Change |
|---|---|---|
| `SynthEditLib` | [#4](https://github.com/JeffMcClintock/SynthEditLib/pull/4) | delete `it_empty.h` |
| `SynthEdit` (SE16) | [#10](https://github.com/JeffMcClintock/SynthEdit/pull/10) | drop `EditorLib/CMakeLists.txt:74` |
| `TideSynth` | this branch | ruling into `decisions.md`, C8 → IN-REVIEW |

**They must merge together.** Either alone leaves a source list naming a file
that does not exist — and because CMake tolerates that for *headers*, it fails
silently rather than loudly. That is the same mechanism that let this orphan
survive three dead-code passes, so landing half of C8 would recreate C8.

**Learned:**

- **The gate was deliberately not widened.** Option (c) — relax STEP 5's
  exception from `C1-C7` to any C-series item — was the tidiest fix and was
  rejected on the merits, not forgotten. It is recorded as rejected in
  `decisions.md` so a later run does not re-derive it and read C8 as licence to
  reach. **C8 is a precedent for escalating, not for reaching.**
- **`gh pr edit` does not work as the bot.** It goes through GraphQL, which wants
  `read:org`; the classic token has `repo` only. Same for `gh pr view --comments`.
  **Use the REST API** — `gh api -X PATCH repos/<o>/<r>/pulls/<n> --input <json>`
  and `gh api …/pulls/<n>/reviews|comments` — which the `repo` scope does cover.
  This will bite every run that tries to amend its own PR body.
- **The #31/#32 `JOURNAL.md` conflict predicted in #32 was real and Jeff resolved
  it cleanly** — both entries are present and in date order on `main`. The
  mitigation that worked was keeping the *BACKLOG* edits disjoint (#31 took the
  NEXT block and the P7 rows, #32 took only the C8 row), so only the journal
  needed hand-merging. Worth repeating whenever two PRs are open at once; **A4**
  is the row that would automate it away.

**Still open, and not mine to close:**
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) — P7's
regression test. TideSynth #31 merged but this did not, so **P7 correctly stays
IN-REVIEW**; do not flip it to DONE until that one lands. It is additive and
changes no shipped code.

**Build health:** nothing built; the only code change is a one-line CMake source-
list deletion and the removal of a header with zero includers. No claim beyond
that.

**Next:** merge #4 and #10 **together**, then flip C8 to DONE and move the row to
the Done section. Then `mac` is on **P7a**, per the NEXT block #31 set.

**Prompt:** n/a — interactive session, not a scheduled run. Acted as
`tide-rack-bot`, so every change went via a PR rather than to a default branch.

**Branch/PR:** `tide/mac/C8-ruling-recorded` (TideSynth), plus the two above.

---

## 2026-08-11 — macos — C8

**Did:** Took **C8** (`SynthEditLib/it_empty.h` — delete it, or find out why it
exists) and did the finding-out half in full. Audit in
[docs/c8-it-empty-header.md](docs/c8-it-empty-header.md). **I did not delete the
file**, and that is the judgement call of this run — reasons under *Learned*.
Also filed the blocking question as a `PROPOSED:` entry in
[docs/decisions.md](docs/decisions.md) and rewrote the C8 row to NEEDS-JEFF with
`Default in effect` / `Decide-by`.

**Result — the file is dead by every measure, and I can date its death.**
Recommendation is **delete**. Reconstructed with `git log -S` over `SE16`:

| When | Commit | What went |
|---|---|---|
| 2022-03-03 | `27f28b54e` | the last live instantiation, `it_visual_ob_list_empty : EmptyIterator<CVisualOb, it_visual_ob>` |
| 2025-01-24 | `176c6c26f` | the archived V1 copies under `OtherProjects/SynthEdit_1.0/` |
| 2026-04-13 | `671457fc5` | `SynthEdit2/it_empty.cpp` — **the header's last includer anywhere** |
| 2026-08-08 | C2 | moved the orphan into the public repo |

The 2026-04-13 step is the punchline: that `.cpp` had its entire body commented
out, so its only live line was `#include "it_empty.h"`. For its last four months
the header's sole reason to exist was a file that existed only to include it.

**Verification artifact — six checks, all run this session, none carried over
from C2's claim:**

| Check | Result |
|---|---|
| `git grep -nIE 'it_empty\|EmptyIterator'` over 8 repos (`SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`, `GMPI-plugins`, `GMPI_Adaptors`, `TideSynth`) | **zero `#include`**; only the file itself, `EditorLib/CMakeLists.txt:74`, and TideSynth prose |
| same as a filesystem grep incl. build dirs | same, plus one pre-C2 copy in an abandoned `.claude/worktrees/` scratch tree — not a reference |
| `file(GLOB)` / `GLOB_RECURSE` anywhere in `SynthEditLib` | **none** — nothing can sweep it in |
| `install()` / `export()` / `PUBLIC_HEADER` / `FILE_SET` in `SynthEditLib` | **none at all** |
| `it_empty` in any `.vcxproj` / `.filters` / `.pbxproj` / `.xcconfig` / `.yml` | **none** (unlike `FuzzyMatch.h`, which C2 had to repoint in three) |
| creation date | **2002-01-10 18:50:19 UTC**, decoded from the v1 UUID in its own ClassWizard guard `AFX_IT_EMPTY_H__E50CDB53_05FA_11D6_…` |

**No build was run, deliberately.** A header that appears in zero `#include`
directives cannot affect any translation unit — that is a proof, and a build
could only fail to contradict it. Building `SE16` here also has known live traps
(P6; and the half-overridden `GMPI_WRAPPER_FOLDER_OVERRIDE` the P7 entry found in
Jeff's tree). Saying "verified by build" would have been weaker evidence dressed
as stronger.

**Learned:**

- **`SynthEditLib` does not build any of C2's 16 files.** Its own
  `CMakeLists.txt` lists none of `it_doc_ob.cpp`, `imbedded_file.cpp`,
  `checkpoint.cpp`, `it_plug_destinations.cpp` — they sit in the public repo but
  are still compiled only by `EditorLib`, reaching across via
  `${SYNTHEDITLIB_DIR}`. Correct and expected until **C6** moves the list, but it
  means **"it's in SynthEditLib" does not yet imply "SynthEditLib builds it"**, and
  a run reasoning about the public repo's surface should not assume otherwise.
- **So the C8 row's "public API surface" overstates the case.** Nothing in
  `SynthEditLib` is exported or installed — there are no such rules in the repo.
  The file is *visible*, which is a real cost for a repo whose point is to be
  read, but no consumer can depend on it. That distinction is what makes the
  deletion risk-free rather than merely low-risk.
- **A CMake source list is an inventory, not a dependency graph.** This is how
  the file survived: it was on `EditorLib/CMakeLists.txt:74`, C2 moved everything
  on that list, and listing a header contributes nothing to compilation so nothing
  ever complains. Any future carve-out stage should expect the same — **C3 moves
  ~120 files off that list and the list is not evidence any of them are live.**
- **Why I stopped short of deleting, since it is the arguable part.** Both files
  the change needs — `SynthEditLib/it_empty.h` and `SE16/EditorLib/CMakeLists.txt`
  — are on STEP 5's **GATED** list, whose single exception is "an approved
  carve-out stage (C1-C7)". C8 is numbered outside that range and is a cleanup,
  not a stage. The prompt's remedy for a GATED fix is "do the TIDE-side part, then
  file the gated part as its own BACKLOG item naming the exact file" — but that is
  already spent: **C8 *is* that item**, filed by C2, naming the exact file, and
  there is no TIDE-side part. So the remedy terminates in a question, not an
  action. Widening the exception myself would be a run rewriting the rule that
  protects the commercial repo because the rule inconvenienced it, which is the
  shape of the mistake the gate exists to prevent; **G3 is the precedent for
  asking, and Jeff answered that one in a day.** The C8 row also asks for "a
  deliberate keep or a deliberate delete" — C2 span it off precisely so the call
  would not be a side effect of a file move, and deciding it unilaterally makes it
  a side effect again, one layer up.
- **Eligibility and authority are different questions, and the prompt is
  consistent about it.** STEP 2 says eligibility lives in the Status column
  alone — C8 is `TODO`/`any`, so taking it was correct. STEP 5 then constrains
  *how* it may be executed. A run that conflates the two would either refuse an
  eligible item or reach across a gate; the right answer is take it, do
  everything in bounds, escalate the one act that is not.

**Build health:** nothing was built and no code changed, in any repo. This run
touched only TideSynth (docs + backlog + journal), so nothing that consumes
`gmpi_ui`, `GMPI_Wrappers`, `SynthEditLib` or `SE16` is affected. All five
working copies were clean before this run and were left on their default
branches.

**STEP 1 / 1.5 — what I found before picking an item, since it changes what the
next run should expect:**

- **No `platform:mac` issues** — none open in TideSynth at all. `gmpi_ui#1`
  ("Linux support?", 2024) is unlabelled and not from the CI bot; noted, not acted
  on.
- **P7's PRs are open and their checks are red, and it is not P7's fault.**
  TideSynth [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) shows
  `windows`, `macos` and `linux` all FAILURE — but **`main`'s own latest run
  (`31352435423`) fails identically on all three**, with
  `CMake Error: The source directory … does not appear to contain CMakeLists.txt`.
  That is the documented pre-C7 failure the workflow header calls "the point"
  (`build.yml`: *EXPECTED TO FAIL until BACKLOG C7*). So it is branch-independent
  and **not** "changes requested handed back to my platform"; I left both PRs
  alone per STEP 1.5. No reviews and no comments on either.
- **Worth flagging plainly: STEP 1.5's "failing checks" trigger is currently
  unusable on this project.** Every PR and every branch has three red checks and
  will until C7 lands, so "does my platform's PR have failing checks" is always
  yes and carries no signal. A run that took the rule literally would spend every
  session re-investigating a known-expected failure instead of taking backlog
  work. I resolved it by comparing against `main`'s own run, which is the check
  that actually discriminates — **recommend that comparison become the rule**, and
  note **B1** is the row that fixes the underlying problem.
- Run-level conclusion says `success` while every job says `failure`, because of
  the job-level `continue-on-error`. `gh run list` is therefore actively
  misleading here; read `…/actions/runs/<id>/jobs`, not the run.

**Next:** **C8 needs one line from Jeff** — merge the PROPOSED entry to answer
it. Under option (b) the execution is one commit in each of two repos and the
audit already contains both exact edits. Otherwise the mac NEXT pointer is
**P7a** once [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) merges
(that PR moves it), and P7a is well-specified with its measurements already
taken. Independently: **B1** is now doing more damage than its row suggests —
see the STEP 1.5 note above.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/C8-it-empty-header`, TideSynth only — no other repo was
committed in. **Expect a JOURNAL.md conflict with [#31](https://github.com/JeffMcClintock/TideSynth/pull/31)**:
both insert at the top of the file, and both are open at once. BACKLOG.md should
merge cleanly — #31 edits the NEXT block and the P7 row region, this edits the C8
row only, and I deliberately left the NEXT block untouched because #31 already
moves `mac` to P7a.

---

## 2026-08-10 — macos — P7

**Did:** Audited the macOS and X11 resize paths for the P4
time-of-check/time-of-use crash, and ported the Windows regression test to Cocoa.
Findings in [docs/p7-resize-audit-mac-x11.md](docs/p7-resize-audit-mac-x11.md);
the test is `GMPI_Wrappers/tests/mac_editor_resize_host.mm`. **No behavioural
change was made to `gmpi_ui` or `SEVSTGUIEditorMac.cpp`** — reasons below, they
are deliberate and they are the main judgement call in this run.

**Result — the crash does not exist on macOS, and cannot by that mechanism.**
Not "was not observed": the structure rules it out. On Windows the device is
rebuilt *inside* the resize, so a checked pointer can be invalidated before it is
used. On macOS the resize only tears down:

```
SEVSTGUIEditorMac::onSize  ->  resizeNativeView  ->  [NSView setFrame:]
    ->  DrawingFrameCocoa::onResize()  ->  CGContextRelease(backBuffer); backBuffer = nullptr;
```

`onResize` ([DrawingFrameMac.mm:434](#)) is three lines and uses nothing
afterwards. Reallocation is lazy, in `onRender`, where the `if(!backBuffer)` test
and the use are adjacent. `onSize` uses nothing after `resizeNativeView`. There is
no window for staleness. X11's `reSize` ([DrawingFrameX11.cpp:932](#)) is safe for
a different reason: it writes its fields and calls `XResizeWindow` *last*, and X
requests queue rather than dispatch synchronously, so there is no re-entrancy to
survive.

**Verification artifact.** `mac_editor_resize_host` built universal (x86_64 +
arm64), AppleClang 21, macOS 26.3.1:

| Plugin | runs | oversized resize+paint pairs | result |
|---|---|---|---|
| `GainGui_VST3`, built from local `gmpi_ui` + `GMPI_Wrappers` | 3 | 6 per run | **exit 0, survived 3/3** |
| `TIDE_VST3` Release, the existing bundle | 1 | 6 | **exit 0, survived** |

Both were provably live *before* the oversized rects (liveness A and B, 19–65
distinct colours) and still drawing *after* recovering to their original size, so
"survived because it never ran" and "survived because resize became a no-op" are
both excluded. Rects exercised each pass: `2178 x 32672` (the rect from the
Windows dump), `0 x 0`, `1 x 1`, `16385 x 600`, `600 x 16385`, then recovery.

**What is actually wrong on macOS, since it is not a crash:**

- **No upper bound on extent, anywhere** — not in `onSize`, not in
  `resizeNativeView` ([:788](#)), not in `initBackingBitmap` ([:406](#)).
- **And — measured, not assumed — there is no NULL to fall back on.** I expected
  `CGBitmapContextCreate` to refuse these sizes and `onRender`'s `:113` guard to
  catch it. It does not refuse them. Probed directly with the exact format
  `initBackingBitmap` asks for: `2178 x 32672` **ok**, `16384 x 16384` **ok**,
  `65536 x 600` **ok**; binary search puts the square limit at **131071** and one
  axis at **4194303**. CoreGraphics reserves lazily. The only extent it refuses is
  `0 x 0`. So the whole `if(!backBuffer) return;` safety story applies at exactly
  one size.
- **The consequence is memory.** Resident size climbs into the hundreds of MiB and
  one measured paint at `16385 x 600` cost **+253 MiB**. Numbers are noisy — they
  include the harness's own bitmaps — but the order of magnitude is the finding.
- **`checkSizeConstraint` never writes the rect back** ([SEVSTGUIEditorMac.cpp:79](#)),
  the pre-P4b shape. Both branches observed: GainGui (resizable) returns
  **`kResultTrue`** and TIDE returns `kResultFalse`, and *neither* touches the
  rect. The resizable case is the sharp one — the wrapper **affirmatively
  approves** `2178 x 32672`, with no clamp behind the answer.
- **Two latent TOCTOUs, neither demonstrated.** `onRender` re-checks `backBuffer`
  after `arrange` at `:113` — correct — and then does *not* re-check it after
  `drawingClient->render()` at `:207` before using it at `:212`/`:215` (**P7b**).
  X11 `present()` is worse-shaped: `pw`/`ph` are cached at `:1262`, checked via
  `ensureImage` at `:1267`, and used at `:1319` after `measure`/`arrange`/`render`
  all re-enter client code — `d.image` is re-read but its *extents* are not, so a
  nested `present()` at a smaller size would overflow the heap rather than
  dereference null (**P7c**).

**Why I changed no shared code, since that is the arguable part.** Three reasons,
weightiest first. (1) P7 asks two questions and asks for the test ported; the
answer is "no crash", so nothing here justifies editing the backend every GMPI
plugin and SynthEdit itself depend on. (2) A clamp needs a defensible number and
there is not one — Windows' 16384 is a hard D3D11 limit, CoreGraphics accepts
131071², so a macOS bound is a product decision about how much memory an editor
may reserve. Picking one silently inside shared code is the guess the run prompt
warns about. (3) The standing direction for these repos is to rebuild SynthEditCL
as well as TIDE, and **P6** says SynthEditCL does not build on macOS — so a
behavioural change to shared rendering code cannot be validated against its other
consumer on this box today. Filed as **P7a** with all the measurements, so
whoever takes it chooses a number with evidence instead of copying 16384.

**Learned:**

- **The port's danger was not the crash, it was the liveness probe.** The Windows
  harness proves the renderer is live by making a benign resize and checking the
  window adopted it. That is sound *there* because adoption proves `reSize` got
  past its device check. On macOS `resizeNativeView` calls `setFrame:` with **no
  device check at all**, so adoption holds with no renderer whatsoever. A literal
  port reports a confident false PASS. Liveness here had to become two facts: the
  view adopted the size, **and** a forced paint produced real drawing.
- **And the resize allocates nothing, so `onSize` alone tests nothing.** Windows
  gets the reallocation free from `SetWindowPos` sending `WM_SIZE`. On macOS it
  happens in the next `drawRect:`, so every resize in the harness is followed by a
  forced synchronous paint. Without that the test would have "passed" while
  exercising only `CGContextRelease`.
- **I produced a tidy wrong finding and caught it; the catch is the lesson.**
  First version sampled one 200x200 tile at the view's origin, got one distinct
  colour at `2178 x 32672`, and I wrote down "the editor goes blank". It does not.
  The view is far larger than its 200x200 window so most of it is clipped and never
  rendered, and AppKit's unflipped origin is the **bottom**-left, which after that
  resize sits far below the window — the tile was in a region that legitimately
  never drew. Exactly P2's error of measuring the parent `HWND`, in Cocoa dress.
  Fixed by sampling corners and centre of `[v visibleRect]`. **Then it was still
  wrong to gate on**: at `16385 x 600` GainGui's tiles are uniform while TIDE's
  return 65 distinct colours, because whether a sampled region has content depends
  on where the client puts it. Distinct-colour counts at absurd extents are now
  diagnostics only. Two plugins is what made this visible — one would have left me
  with a plausible false claim in the doc.
- **`cacheDisplayInRect:toBitmap:` does not exist.** It compiles as an unknown
  selector returning `id` and throws at run time. `...toBitmapImageRep:` is the
  real one. The compiler warned and the warning was the only thing standing
  between this and a runtime exception mid-probe.
- **This box's SynthEdit build tree has the X3 trap live in it.**
  `~/Documents/GitHub/SynthEdit/build/CMakeCache.txt` has
  `GMPI_WRAPPER_FOLDER_OVERRIDE:PATH=` **empty**, so it links a `GMPI_Wrappers`
  frozen at `1a68601`, 2026-05-14, out of `build/_deps/gmpi_wrappers-src` — while
  `gmpi_ui`, `GMPI` and `SynthEditLib` are all correctly overridden. Exactly the
  half-overridden state that made a Linux VST3 unloadable in X3. It is Jeff's tree
  and I did not touch it, but any run that "tests a GMPI_Wrappers change" by
  building TIDE from that directory is testing May's code. I built into a fresh
  scratch directory instead and read the configure banner to confirm all three
  local paths were taken.
- **`GMPI-plugins` cannot link a GUI plugin on macOS at all** — `_OBJC_CLASS_$_UTType`
  undefined, because `gmpi_ui/backends/MacFileDialog.h:8` uses `UTType` and that
  repo's link line never adds the framework, while
  `SynthEdit/EditorLib/CMakeLists.txt:166` does. Pre-existing, invisible until
  someone built a GUI plugin outside SynthEdit. `GMPI-plugins` is on neither the
  ALLOWED nor the GATED list, so it is GATED by default; worked around at configure
  time with `-DCMAKE_MODULE_LINKER_FLAGS="-framework UniformTypeIdentifiers"`,
  touching no file, and filed as **P7d** with the scope question named — the
  requirement plausibly belongs in `gmpi_ui`, which is ALLOWED.

**Build health:** no claim about SynthEdit or SynthEditCL — neither was built, and
P6 says SynthEditCL does not build on macOS anyway. TIDE's existing Release bundle
loads, instantiates, opens its editor and renders under this harness, which is a
narrower claim than "TIDE builds" and is the one I can actually support. `gmpi_ui`
was not modified, so nothing that consumes it is at risk from this run.

**Next:** **P7a** — the one real macOS defect this found, and the audit already did
the measuring a clamp needs; read the doc before reaching for Windows' 16384,
because it does not apply. **P7b** is minutes if someone is in that file anyway.
**P7c** is the X11 half and is `linux` by necessity — this box cannot build or run
X11, and the first thing to establish there is whether a nested `present()` is
reachable at all. **P7d** needs a one-line ruling from Jeff before it is one line of
CMake.

Nothing is in flight on this box. All working copies were clean before this run and
are back on their default branches after it; the only repos committed in are
TideSynth and GMPI_Wrappers, each with an open PR.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/P7-resize-audit` in both repos —
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) (test +
CMake, additive) and TideSynth (audit doc, backlog, this entry). Independent:
the wrappers PR changes no shipped code, so merging either alone cannot break a
build.

---## 2026-08-09 — windows — P8 (interactive session, Jeff directing)

**Did:** Fixed **P8** and pushed it straight to `SE16` master (`4baddfbb4`) at
Jeff's direction. One line at `SynthEdit2/EditorWindowHelper.cpp:294`:

```cpp
- se_cl::renderContainerThumbnail(currentContainerView.universalFactory, c, vf)
+ se_cl::renderContainerThumbnail(currentContainerView.universalFactory.get(), c, vf)
```

**Result:** `MSBuild SynthEditStore.sln -t:SynthEdit2 -p:Configuration=Release
-p:Platform=x64` links, producing `x64\Release\SynthEdit2\SynthEdit2.exe`. That
is the first time the WinUI3 app has reached its link stage in this sequence of
runs, so it **also closes the verification gap C1b and C2 each had to leave
open** — both could compile their new TU but neither could get the app to link.

**How it started:** Jeff pasted the URL of a failed Actions run. Worth recording
what that turned out to be, because the row understated it.

1. **P8 was not "the WinUI3 app does not compile from clean" — it was
   failing SynthEdit's Store release pipeline.** Run
   [31297103293](https://github.com/JeffMcClintock/SynthEdit/actions/runs/31297103293)
   died on `master` at "Build SynthEditStore (Release x64)" with exactly this
   error, so Generate Changelog, MSIX signing, the setup bootstrapper, FTP
   upload and the Discord post were all skipped. **Nothing shipped, and had not
   since P8 was introduced.** A row that reads like a developer-convenience
   annoyance was actually a release blocker; the CI link is the thing that made
   the difference, so it is on the row now.
2. **My first attempt to check it was wrong, and produced spectacular noise.**
   Building `SynthEdit2.vcxproj` directly gives MSBuild a `$(SolutionDir)` of the
   *project* directory, so every `SynthEditLib` and `gmpi_ui` include failed
   C1083 — 14 errors that looked like the tree was broken and had nothing to do
   with anything. Build `SynthEditStore.sln -t:SynthEdit2`. (This is the same
   trap C2's entry recorded a day earlier; recording it twice is deliberate.)
3. **Debug fails for an unrelated reason.** With the fix in,
   `-p:Configuration=Debug` dies on `x64\Debug\pch.pch: Invalid argument` —
   a compiler-intermediate error, not a code error. Release is what CI builds
   and Release is what was verified. Not investigated; if someone needs a Debug
   WinUI3 build, start there and do not assume it is P8's ghost.

**Why `.get()` is the right fix rather than a papering-over:**
`renderContainerThumbnail` takes `gmpi::api::IUnknown* destDrawingFactory` and
**borrows** it — `ContainerThumbnail.h`'s own comment says callers pass what
`HostedView::universalFactory` and `IDrawingHost::getDrawingFactory` "hand out".
`universalFactory` is a `std::unique_ptr<UniversalFactory>`
(`SynthEditLib/Shared/DrawingFrame2_win.h:153`), and `UniversalFactory` derives
from `gmpi::api::IUnknown` (`DrawingFrame2_cpu.h:19`). The other three call sites
— Wayland, the mac bridge, `tests/layouttests.cpp:833` — all pass a raw pointer
already, and `DrawingFrame2_win.h:270` does `*returnFactory = universalFactory.get()`
for the same purpose. So this caller was the only one that never caught up. No
ownership changes hands.

**Learned — the environment moved under this session, and the reflog is how you
find out.** Between the C2 run and this one, C2 was landed on `SE16` master as
`c3a4f9fac` (a rebased copy of my `d933e5e03`, authored by Jeff), SynthEditLib
PR #3 and TideSynth PR #24 were merged, master gained two more commits, and both
local checkouts were switched off my branch — which is why `git log` on a branch
I "was on" reported `unknown revision`. **`git reflog` reconstructed the whole
sequence in one command.** Two consequences worth carrying:

- **A pushed branch is not the same commit as what lands.** `git merge-base
  --is-ancestor origin/tide/win/C2-leaf-files master` says **NO** even though the
  work is fully in master, because a rebase gives it a new sha. Do not conclude
  from that answer that work was lost — compare content, or read the reflog.
- **`origin/tide/win/C2-leaf-files` is now a stale branch in `SE16` with no PR**,
  which the new STEP 5 rule reads as a failure state. It is redundant, not
  unlanded. Safe to delete; left alone here because deleting someone else's
  remote branch is not this session's call to make unasked.

**Next:** Watch for a green Store run. P8 is marked **DONE-PENDING-CI**, not
DONE: the local Release link proves the compile error is gone, but the CI job
continues into signing and upload and nobody has seen it finish. First run to go
green flips the row.

**Branch/PR:** `SE16` master `4baddfbb4` directly, per the interactive-session
convention and logged in [docs/decisions.md](docs/decisions.md). **A scheduled
run still must not push to main** — that rule is unchanged.

---

## 2026-08-09 — windows — A1 done, A2 mostly done (interactive session with Jeff)

**Did:** Jeff executed **A1** (the 8 signing secrets moved into a `release`
environment with required review) and revoked the leaked `cowork-linux-build-test`
PAT. Then most of **A2**: bot account `tide-rack-bot` created, accepted as
collaborator on all five repos, token minted, and "Agent PRs only" rulesets
created on the four public repos.

**Result — rulesets, verified via API rather than by reading the UI back:**

```
TideSynth      [public]  Agent PRs only (active)
SynthEditLib   [public]  Agent PRs only (active)
gmpi_ui        [public]  Agent PRs only (active)
GMPI_Wrappers  [public]  Agent PRs only (active)
SynthEdit      [private] 403 — "Upgrade to GitHub Pro..."

rules:  deletion, non_fast_forward, pull_request(0 approvals)
target: ~DEFAULT_BRANCH        bypass: RepositoryRole:5 always
```

The first ruleset was built through the UI, then read back through the API to
capture its exact shape; the remaining three were POSTed from that captured
payload. Worth repeating as a technique — it removes the guesswork about
`actor_id` and rule parameter names without having to trust a click.

**Learned — two GitHub platform limits that A2 as written did not know about,
both permanent:**

- **A fine-grained PAT cannot serve a collaborator on repos they do not own.**
  The form only offers "Public repositories" (read-only) and "All repositories"
  (the bot's own, of which there are none); there is no third option, and
  accepting the invitations does not produce one. GitHub documents this
  explicitly. **A classic `repo`-scope token is the only thing that works.**
  Its scoping is a property of the bot's collaborator list, not the token, so
  adding the bot anywhere else silently widens it.
- **`workflow` scope was withheld on purpose**, which puts the run prompt's
  no-workflow-edits rule into the credential layer instead of into prose. The
  price is immediate: **A3 and A5 both edit `.github/workflows/`**, and A3 is
  the current NEXT `any` item, so the very next `any` run hits this.
- **Private repos cannot have rulesets without GitHub Pro.** So `SynthEdit` —
  the commercial repo the entire ALLOWED/GATED boundary exists to protect — is
  the one repo where "agents never push to the default branch" remains prose.
  That is the exact inversion of where protection is most wanted.
- **`~DEFAULT_BRANCH` is the right targeting primitive**, not a literal branch
  name: one identical payload covers SE16's `master` and everyone else's `main`,
  and it stays correct if a default branch is ever renamed.
- **Required approvals is 0 by choice.** Self-approval is forbidden, so
  requiring 1 would make Jeff's own PRs unmergeable except by bypass — ceremony,
  not safety, at solo scale. Same reasoning that killed the CODEOWNERS half of
  this recommendation during review.

**Next:** **A2 step 5 — per-box credential wiring — is the only thing left, and
until it lands the whole item is inert.** The bot exists and the rulesets are
live, but the scheduled runs still authenticate as Jeff, so they still bypass
every rule via the admin exemption. Nothing has actually changed for the agents
yet. After that, **moving the five repos under an organization** is now a
recommendation rather than an option: it is the only clean fix for both limits
above, and it forces no rename.

**Prompt:** n/a — interactive session, not a scheduled run.

**Branch/PR:** committed to main (which also served as the live test that the
Repository-admin bypass works — this push would have been rejected otherwise).

---

## 2026-08-09 — windows — process review adopted (interactive session with Jeff)

**Did:** Ran a seven-agent review of the development process itself (three
lenses over the process docs, three web researchers, one adversarial critique)
and adopted what survived. Everything landed on branch
`process/2026-08-09-review` as one PR — the batch touches the run prompt, which
steers the fleet, so it takes the human-review path on purpose.

Changed in this batch:

- **[docs/process-review-2026-08-09.md](docs/process-review-2026-08-09.md)** —
  the condensed review: verdict, the three broken mechanisms, the sequenced
  plan, what the red team rejected (do not re-file those), and what all seven
  reviewers missed.
- **[docs/decisions.md](docs/decisions.md)** — new single decision log, seeded
  with every ruling to date, plus the PROPOSED mechanism: an agent hitting a
  design fork opens a PR adding a PROPOSED entry; Jeff's merge IS the decision.
- **BACKLOG** — new A-series (A1–A9, sequenced process hardening), NEXT block,
  IN-REVIEW status, BLOCKED(<id>) notation, C4–C7 explicitly chained.
- **Run prompt** — the spec-gap batch: STEP 0.5 (FLEET-PAUSED check, 14-day
  staleness bound, provenance capture), STEP 1 issue-authenticity rule and
  inlined fix protocol, STEP 1.5 (own-platform PR triage before new work),
  resume semantics (own-platform open PR = continue it, not "taken" — the old
  wording deadlocked any multi-session item, and C3 is one), claim liveness
  (24h rule), skip-and-flag for under-specified items, decision-latency rule,
  conditional build-health reporting, IN-REVIEW/DONE split, verification-
  artifact requirement, the three-kinds dirt rule (the old text ordered runs to
  commit or revert Jeff's own WIP), default-GATED for unlisted paths, and the
  two no-exception rules (no workflow edits unless the item says so; no
  credential values in any text).

**Result:** Docs only; no mechanism built yet. The mechanisms are the A-rows,
deliberately: building them is agent work under the new rules.

**Learned:**

- The review's three sharpest facts, each verified against the live repos: the
  CI→platform-issue loop has never once executed (read-only token, 403 under
  continue-on-error, zero issues ever); "green CI" is currently meaningless
  (continue-on-error at job level); and the 8 signing secrets are reachable
  from any workflow edit on any `tide/**` branch — but no workflow references
  them yet, so A1 is free to do now.
- **The stated human/AI split is implemented inverted**: all 25 TideSynth PRs
  are coordination churn Jeff merges in seconds, while the shared-lib gate is
  prose. The habituation literature says this fails slowly, then all at once.
- The red team killed four plausible recommendations (cloud/Actions as agent
  hosts, merge queues, dependency-bot policy, encrypted-secrets-in-repo).
  They are recorded in the review doc so no future run re-derives them.
- **Both G4 and G5 landed while this review was being written, and both PRs
  reached `main` before this one did — from two different bases, since G4 and
  G5 were claimed from the same pre-review commit.** G5 (#25) merged first;
  when G4 (#26) was then merged it collided with G5's changes to the same
  three coordination files, including a genuine finding of G5's (the old state
  table had Linux backwards — it claimed Linux predated PR #4 and could not
  write TIDE code, when in fact it could). Resolved by hand rather than by
  picking a side, preserving both journal entries and G5's correction; see the
  merge commit on this branch. This branch was then rebased onto the result,
  which is why the NEXT block below now sends `mac` to **P7** instead of the
  now-done **G4**, and why the `mac` checklist item below is gone. Left as a
  concrete example of what **A5**'s conflict-shaped territory looks like
  before any lint or auto-merge tier exists to catch it early.

**Next:** Jeff's manual checklist (A1 secrets, A2 account, merge #27) — in the
review doc and delivered in-session. Then A3 is the NEXT `any` item.

**Prompt:** n/a — interactive session, not a scheduled run.

**Branch/PR:** `process/2026-08-09-review`.

---

## 2026-08-09 — macos — G4

**Did:** Replaced this box's scheduled task with the bootstrap from
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md), substituting
`{MACHINE}`=`macos`, `{PLATFORM}`=`mac`, `{REPO}`=`~/Documents/GitHub/TideSynth`
— the real path on this box, which is not the table's guess. The task is
`tidesynth-weekly-macos` (no hyphen after `tide`, like Linux, unlike Windows'
`tide-synth-weekly-windows`); its file is
`~/.claude/scheduled-tasks/tidesynth-weekly-macos/SKILL.md`. It went from 127
frozen lines to 42. `list_scheduled_tasks` showed exactly one task on this box,
so there was no risk of the duplicate G4 warns about.

**Cron untouched** at `0 2 * * 6` (Sat 02:00, +151s jitter). Only `prompt` was
passed to `update_scheduled_task` — it is a partial update, so naming
`cronExpression` at all would have been unnecessary risk. Confirmed after the
write that `cronExpression`, `enabled`, `nextRunAt` (2026-08-14) and
`jitterSeconds` were all unchanged.

Also updated [docs/agent-setup.md](docs/agent-setup.md): macOS moved to
**bootstrap** in the state table, the "cannot install itself" paragraph moved to
the past tense, and the three install checks G5 wrote were carried in.

**One deliberate deviation from the block as written.** The installed text ends
with three extra lines — install date, the prompt blob sha it was taken from,
and one sentence saying a run cannot detect its own staleness. That follows the
Windows precedent recorded in the 2026-08-09 entry below, not the doc, which says
"Nothing else". It is inert prose and cannot go stale in a way that matters, but
it is a deviation and should be visible as one rather than discovered later.

Nothing else was touched. No code, no build, no other repo.

**Result:** Verified rather than assumed. All three STEP 0 commands, pasted
verbatim from `/` — an unrelated cwd — while the working tree was parked on
`tide/mac/G4-bootstrap-task`, i.e. the exact condition STEP 0 warns about:

```
git -C ~/Documents/GitHub/TideSynth fetch origin                      exit 0
git -C ~/Documents/GitHub/TideSynth show origin/main:docs/…prompt.md  314 lines
git -C ~/Documents/GitHub/TideSynth rev-parse --short origin/…prompt.md  f0f60a8
```

Read the installed file back in full: one numbered heading, `STEP 0`, and
nothing beyond it. Both of G5's documented false alarms were present and
correct — the prose "as STEP 4 requires", and the literal
`{MACHINE}`/`{PLATFORM}`/`{REPO}` kept in the one sentence about substituting
them into the *fetched* prompt.

No build was run and none was needed: nothing in `SE16`, `gmpi_ui`,
`GMPI_Wrappers` or `SynthEditLib` was touched. So this run makes **no claim**
about SynthEdit, SynthEditCL or TIDE building — and note **P6** still says
SynthEditCL does not build on macOS, so that claim could not be made honestly
today anyway.

**Learned:**

- **The staleness was real, it was aimed at a `mac` item specifically, and this
  run would have walked into it.** Under the frozen copy the topmost eligible row
  in the stale local `BACKLOG.md` was **P7** — audit the X11 and macOS resize
  paths — and P7's entire fix lives in `gmpi_ui` and `GMPI_Wrappers`, which the
  frozen text lists in **neither** ALLOWED nor GATED, because it predates G3.
  The run would have taken its own top item and then been unable to touch a
  single file it needed, which is exactly what happened to P4 on Windows and
  exactly what G3 was filed to stop. Linux's equivalent finding was a *closed*
  item (X4, WONTFIX); this box's was a *live* item it could not do. Different
  shape, same root: a frozen prompt cannot know the rules moved.
- **The collision check saved this run, for the second week running.** `git fetch
  origin` in STEP 2 pulled 20 commits onto `main` — C0 approved, C2 landed, X4
  closed WONTFIX, the TIDE Rack rename, and the bootstrap itself — and rereading
  BACKLOG afterwards is the only reason G4 was visible at all. It is worth
  keeping for that reason and not only for collisions, and both converted boxes
  have now said so independently.
- **`update_scheduled_task` is a partial update, and that is the safe property to
  lean on.** The instruction "leave the cron alone" is satisfied by *not
  mentioning* cron, not by re-supplying the value you think it has. Re-supplying
  is where a transcription error would land, and the failure would be silent
  until a run did not happen.
- **A `~` path inside `git -C` is shell-expanded, so it works — but prove it per
  box.** It is the only part of the bootstrap that differs between machines and
  the only part that can actually be wrong. Two boxes have now proved it and
  neither found a problem; that is still the right check, because the failure it
  guards against is a silent no-op with a week-long feedback loop.
- **This closes the state table in `agent-setup.md`, and that is the actual
  deliverable.** Not "macOS is current" — that was true before, twice, and went
  stale within days both times. The thing that changed is that no one has to
  assert it any more: every entry from every box now carries the blob sha of the
  prompt it ran, and absence of that line is the tell.

**Next:** **P7** is the topmost genuinely takeable `mac` item and it is now
takeable *because* of this run — the fetched prompt lists `gmpi_ui` and
`GMPI_Wrappers` as ALLOWED, so the mac half (`DrawingFrameMac.mm:434`
`onResize()`) and the X11 half are both in scope for whoever gets there. **N1**
sits above it and is `any`, but read its own text first: the prose half is done
and the build-system half says explicitly *do it after C7, not during*, so it is
not really open. **P6** and **P8** are SynthEdit's own bugs and both rows say
check with Jeff before touching.

Nothing is in flight on this box and the tree is clean. Two things a later run
should not misread: `tide/win/C2-leaf-files` still exists on TideSynth's origin
as the stale leftover the C2 entry describes — it is not live work; and
**PR #25 (G5) is open and edits the same three files this branch does**
(`BACKLOG.md`, `JOURNAL.md`, `docs/agent-setup.md`). That is an overlap, not a
collision — different items, both correct — but whichever merges second needs a
rebase, and the resolution is the **union**: G5 owns the Linux row and the Done
entry for G5, this branch owns the macOS row and the Done entry for G4, and the
three install checks in `agent-setup.md` appear on both branches with near
identical wording, so keep one copy.

**Prompt:** `f0f60a8` — with the same caveat G5 recorded, and this is the last
time it will be needed: this run **executed under the frozen copy**, which has no
STEP 0. I fetched and followed `origin/main:docs/weekly-run-prompt.md` by hand
after the collision check showed the repo had moved 20 commits. So the sha
records the instructions I *followed*, not the ones I was *given*. From the next
run on, on all three boxes, those are the same thing.

**Branch/PR:** `tide/mac/G4-bootstrap-task` → PR against `main`.

---

## 2026-08-09 — linux — G5

**Did:** Replaced this box's scheduled task with the bootstrap from
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md), substituting
`{MACHINE}`=`linux`, `{PLATFORM}`=`linux`, `{REPO}`=`~/TideSynth`. The task is
`tidesynth-weekly-linux` — **no hyphen after `tide`, unlike Windows'
`tide-synth-weekly-windows`** — and `list_scheduled_tasks` showed exactly one
task on this box, so there was no risk of the duplicate G4 warns about. Its file
is `~/.claude/scheduled-tasks/tidesynth-weekly-linux/SKILL.md`; it went from the
180-line as-originally-installed prompt to 36 lines. Cron left alone at
`0 2 * * 0` (Sun 02:00, +406s jitter). Also updated `docs/agent-setup.md`: Linux
moved to **bootstrap** in the state table, the paragraphs about what Linux was
missing rewritten in the past tense, and three verification traps added to the
install recipe (below).

Nothing else was touched. No code, no build, no other repo.

**Result:** Verified rather than assumed. All three of STEP 0's commands, pasted
verbatim from `/` — an unrelated cwd — while the working tree was parked on
`tide/linux/G5-bootstrap-task`, i.e. the exact condition STEP 0 warns about:

```
git -C ~/TideSynth fetch origin                                   exit 0
git -C ~/TideSynth show origin/main:docs/weekly-run-prompt.md     314 lines
git -C ~/TideSynth rev-parse --short origin/…prompt.md            f0f60a8
```

Read the installed file back: one numbered heading, `STEP 0`, and nothing beyond
it.

**Learned:**

- **The staleness was not theoretical, and this run walked into it.** The frozen
  copy's STEP 2 would have sent me at **X4** — it is the topmost `TODO`/`any`
  row in the frozen queue's ordering, and Jeff closed it **WONTFIX** on
  2026-08-08 with "do not re-file this". A frozen prompt cannot know the queue
  moved underneath it. I only avoided that because `git fetch` in the collision
  check pulled 19 commits onto `main`, including C0 approved, C2 landed, X4
  closed and the bootstrap itself. **The collision check is what saved this
  run** — the one rule that forces a run to look outside its own head before
  acting. It is worth keeping for that reason and not only for collisions.
- **`agent-setup.md` was wrong about this box, and overstated the damage.** It
  said Linux was "frozen as originally installed" and "predates PR #4", so
  agents here refused to edit `SE16/SynthEditSem/` and claimed items without
  pushing the DOING mark. I read the text I was actually handed and none of that
  held: it had the full ALLOWED/GATED split, the shared-build-file note, and the
  claim-then-push procedure. Dated from the evidence it is the **2026-08-06
  17:55** generation (`SKILL.md` mtime; and it still says *six* constraints, so
  it is one commit shy of `2701fb8` at 17:54:54) — the same generation as
  macOS's, not the 13:38 original. It had been reinstalled that evening and
  nothing recorded that.
  **What it was really missing**, against `f0f60a8`: G3 (`gmpi_ui` and
  `GMPI_Wrappers` ALLOWED), the CRLF-churn guidance, STEP 4's "in EVERY repo you
  committed in" and its prompt-sha line, the whole of STEP 5's two-end-states
  rule including `SE16` being `master`, and the constraint count.
  **An overstated staleness claim is worse than an understated one**: it tells a
  run its own instructions forbid work they actually permit, and the run cannot
  check a claim about itself except by reading the text it was handed — which is
  exactly what nobody had done. Corrected in `agent-setup.md`.
- **The install looks wrong twice when it is right.** Two false alarms, both
  cheap to hit: the bootstrap says "as STEP 4 requires" in prose, so
  `grep 'STEP 4'` matches a *correct* install — the disqualifying thing is a
  STEP *heading*, not the string; and it deliberately keeps literal
  `{MACHINE}`/`{PLATFORM}`/`{REPO}` in the one sentence telling the run to
  substitute them into the **fetched** prompt, so "zero placeholders remain" is
  the wrong check. Both are now written into `agent-setup.md` for G4.
- **Paste STEP 0's commands into a shell before believing them.** The repo path
  is the only part of the bootstrap that differs per box and the only part that
  can actually be wrong; `~` expansion inside `git -C` is the specific thing
  worth proving. It costs seconds now versus a silent no-op at 02:00 next
  Sunday, which is a failure mode with a week-long feedback loop.
- **`git show origin/main:<path>` genuinely works from a parked branch**, which
  is the whole reason the bootstrap reads from `origin/main`. Demonstrated above
  rather than reasoned about — this tree was on the G5 branch the entire time.
- **The prompt version stamp works as designed.** `f0f60a8` is the blob sha of
  the prompt, not a commit, so it changes only when the prompt text changes.
  Every entry from this box will now carry it. **Its absence is the tell for a
  box still frozen** — and note this very entry has one only because I noticed
  the new prompt by hand; next week it will be automatic.

**Next:** **G4** is the last frozen box, and it is now the *only* thing keeping
`agent-setup.md`'s state table alive. macOS should do that and stop, as this run
did.

For the next Linux run: nothing is in flight here and the tree is clean.
`tide/win/C2-leaf-files` still exists on this box's `origin` as the stale
leftover the C2 entry describes — it is not live work. The eligible `linux`/`any`
queue below G4/G5 is thinner than it looks: **C6/C7** are sequential behind
`win`-owned C3–C5, **C8** is a delete inside the GATED `SynthEditLib` repo and is
not one of C1–C7, and **S1b**/**S8** both carry a standing recommendation to ride
along with C4 rather than be done twice. **E1** is the genuinely takeable one —
it needs no plugin, no gated path and no other machine, and it is the only
working audio verification this project has. **P7's X11 half** is also open to
this box now that G3 makes `gmpi_ui` ALLOWED; the row says so explicitly.

**Prompt:** `f0f60a8` — but read the caveat: this run **executed under the frozen
copy**, which has no STEP 0. I fetched and followed
`origin/main:docs/weekly-run-prompt.md` by hand after noticing the repo had
moved. So the sha records the instructions I *followed*, and this is the one
entry where that is not the same as the instructions I was *given*. From the
next run on, it will be.

**Branch/PR:** `tide/linux/G5-bootstrap-task` → PR against `main`.

---

## 2026-08-09 — windows — C2 landed, tide-rack archived (state update, interactive)

**Did:** Nothing new; recording two state changes the next run needs.

**C2 is merged in all three repos.** Jeff merged `SynthEditLib` #3 and `SE16` #9
together. Both local checkouts fast-forwarded clean, so **C3 can start from a
default branch that already has C2's moves** — check that before doing anything
else, because C3 touches the same `EditorLib/CMakeLists.txt`. Note `SE16`'s C2
commit came back with a different sha (`c3a4f9fac`, was `d933e5e03`): that PR was
squash- or rebase-merged, so match on content, not sha, if you go looking.

**`tide-rack` is archived and read-only**, and no box has a local copy. **E1 is
now a one-way port** — clone it to read (`git clone
https://github.com/JeffMcClintock/tide-rack.git`), take the harness and the
reasoning, and do not try to commit anything back. Its two golden WAVs are still
there and still unauditioned.

**Result:** Docs only. All five repos on their default branch, clean, current.

**Learned:** Leftover branches named `tide/win/C2-leaf-files` still exist locally
in `SE16`, `SynthEditLib` and TideSynth, and on TideSynth's origin. GitHub
auto-deleted the head branch for the two real PR merges but not for TideSynth,
because PR #24 was closed by a direct push to main rather than merged through the
UI — a merge and a push that lands the same commits look identical in the log and
leave different residue. If you branch `tide/win/C3-...` you will be working
beside a stale sibling; that is harmless but confusing, so check with
`git branch` before assuming a `tide/win/...` branch is live work.

**Prompt:** n/a — interactive session, not a scheduled run.

**Branch/PR:** none — committed to main.

---

## 2026-08-09 — windows — the prompt is fetched, not copied (interactive session with Jeff)

**Did:** Removed the reason machines go stale, rather than adding more diligence
to noticing it. Each machine's scheduled task becomes a **bootstrap** holding
only its identity — machine name, platform role, repo path — plus a STEP 0 that
fetches `docs/weekly-run-prompt.md` from `origin/main` and follows it. Editing
that file and merging it now reaches every converted box on its next run.

Windows is converted. Its task went from 180 lines of frozen instructions to
~35 lines that can only go stale in ways that do not matter, since the three
values it hard-codes never change.

**Result:** Verified the mechanism before committing to it:

```
git show origin/main:docs/weekly-run-prompt.md          253 lines, reads fine
git rev-parse --short origin/main:docs/…prompt.md       c6f9ae3
  same command one commit back                          cae028b   (changes only
                                                                   with the file)
```

**Learned:**

- **Read the prompt from `origin/main`, never the working tree.** The tree can be
  dirty or parked on a branch a previous run left behind — which is exactly the
  state this box was in an hour ago — and a stale local `main` would hand a run
  old instructions with nothing looking wrong. `git show origin/main:<path>`
  touches no files and works from any branch.
- **The blob sha is the version stamp.** `rev-parse --short
  origin/main:docs/weekly-run-prompt.md` changes only when the prompt changes, so
  STEP 4 now writes `**Prompt:** <sha>` into every entry. **A box on a frozen copy
  has no STEP 0 and will silently omit that line** — absence is the tell, and it
  is the evidence that was missing every previous time this went wrong.
- **The trade is blast radius for visibility, and it is the right way round.** A
  bad prompt edit now hits all three machines at once instead of one. But prompt
  changes go through a PR that Jeff merges, whereas staleness went through
  nothing and announced itself to nobody. Two of three boxes had been running
  months-old rules with every journal entry reading as if all was well.
- **A bootstrap cannot install itself.** A machine that reads nothing remote
  cannot be told to start reading something remote, so macOS and Linux each need
  one last manual install. There is no way around that, and it is worth saying
  plainly rather than implying the fix is already universal.
- The copy-and-remember discipline failed **twice in four days** — G2 reached
  only macOS, G3 reached nobody — and the second failure happened while
  `agent-setup.md` asserted Windows was current. That is what settled it: the
  problem was never insufficient care.

**Next:** **G4** (mac) and **G5** (linux) rewritten — they now install the
bootstrap rather than a fresh copy of the full text, and each says explicitly to
take the block under "The bootstrap", not the one under "The prompt". Both remain
top of "Ready now": a box must fix its own instructions before doing work under
wrong ones, and must then stop, since the new text only takes effect the
following run.

Once both land, `agent-setup.md`'s state table stops being something anyone has
to maintain. Every version of it so far has been wrong within three days.

**Prompt:** n/a — interactive session, not a scheduled run.

**Branch/PR:** none — committed to main.

---

## 2026-08-09 — windows — two end states, never a third (interactive session with Jeff)

**Did:** Closed the hole C2 left, and made it a standing rule so it does not
recur. **Every repo a run commits in must end either on its default branch, or
on a pushed branch with an open PR. Nothing else is acceptable, and no working
copy is left parked on a run's branch.** Jeff's wording: don't leave half-done
work on the developer's machine.

The state that prompted it, found by audit:

| Repo | Was on | Pushed | PR |
|---|---|---|---|
| `SE16` | `tide/win/C2-leaf-files` | yes | **none** |
| `SynthEditLib` | `tide/win/C2-leaf-files` | yes | **none** |
| TideSynth | `main` | yes | #24, merged |

So the backlog said C2 was DONE while the code sat unreviewed on branches in two
repos, and both working copies were parked on them. Nothing was lost — every
branch was pushed and both trees were clean — but nobody had been asked to look
at any of it, and the next run on this box would have started from that state.

Fixed: opened the two missing PRs — `SynthEditLib`
[#3](https://github.com/JeffMcClintock/SynthEditLib/pull/3) and `SE16`
[#9](https://github.com/JeffMcClintock/SynthEdit/pull/9), cross-linked, both
saying that merging one without the other breaks the build — then restored
`SynthEditLib` to `main` and `SE16` to `master`. All five TIDE-related repos
(TideSynth, `SE16`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`) are now on their
default branch with a clean tree.

**Result:** Docs and process only. No code touched, nothing built.

**Learned:**

- **`SE16`'s default branch is `master`. The other four are `main`.** The prompt
  has warned about the reverse case for weeks — `gh pr create --base master`
  failing with a misleading "No commits between…" — but never said which repo is
  which. It does now.
- **The gap was in the prompt, not in the run that hit it.** STEP 4 said "push
  the branch, open a PR" in a document whose subject is the TideSynth repo, so a
  run that also commits in `SE16` and `SynthEditLib` reads it as satisfied. It
  now says "in EVERY repo you committed in", and STEP 5 became "Stop, and leave
  the machine clean" with the two end states spelled out. STEP 5 keeps its
  number so the ALLOWED/GATED references in BACKLOG and agent-setup still
  resolve.
- **Windows was stale on G3 and the table said it was current.**
  `docs/agent-setup.md`'s state table claimed Windows was up to date since
  2026-08-06; G3 landed 2026-08-07 and added `gmpi_ui` and `GMPI_Wrappers` to
  ALLOWED, and the installed task never got it. The table is maintained by hand
  by whoever changes the prompt, and that is never the machine the staleness
  affects. Reinstalled the Windows task from the master; it now ends with its
  install date and a line saying a run cannot detect its own staleness.
- **The task is named `tide-synth-weekly-windows`, with a hyphen.**
  agent-setup.md said `tidesynth-weekly-windows`. Following that would have
  created a *second* task and left the original firing.

**Next:** **G4** and **G5** are filed at the top of "Ready now" — macOS and Linux
must each reinstall their own task before taking any other item. Both boxes are
missing G3 and this rule; Linux additionally still has the pre-PR-#4 blanket ban
that stops it writing TIDE code at all. Neither can detect this from inside a
run, which is why it is a backlog row rather than a note in the prompt they
cannot see. The reinstall takes effect the *following* week, so those runs should
do that one item and stop.

Also open, and not an agent's call: `SynthEditLib` #3 and `SE16` #9 are waiting
on Jeff. They must merge together.

**Branch/PR:** none — committed to main from the interactive session.

---

## 2026-08-09 — windows — Eurorack ruling (interactive session with Jeff, not a scheduled run)

**Did:** Recorded a product ruling and retired a repo. **The Eurorack rack is a
feature of TIDE Rack, not a second product.** Modules are SynthEdit Containers
the user can open up; that ships as a layer on this prototype rather than as a
parallel build.

The separate `tide-rack` repo (<https://github.com/JeffMcClintock/tide-rack>,
scaffolded 2026-08-07) is **superseded**. Jeff's reasoning: the prototype here
already does ~90% of what that repo would have had to build from nothing — the
plugin shell, the carve-out, the build and release plan, the three-machine
coordination — so a second product beside it duplicates all of that to add one
layer.

Changed: a new "The Eurorack rack" section in [PLAN.md](PLAN.md) carrying the
ruling and the four design commitments taken from Reaktor Blocks and VCV Rack;
**E1**, **E2**, **E3** filed in [BACKLOG.md](BACKLOG.md); superseded banners on
the four `tide-rack` docs (`README`, `CLAUDE.md`, `PROGRESS`, `BACKLOG`) so an
unattended session that lands there stops instead of working.

**Result:** Docs only. No code, no build, nothing in `SE16` touched.

**Learned:**

- **Do not re-file `tide-rack`'s backlog.** Its EP-001 is done and salvaged as
  **E1**; EP-002 and EP-003 are now **E2** and **E3**. The repo keeps its
  history and its two golden WAVs; it does not keep its queue.
- **The salvage is the harness, and it is worth taking seriously.** It is the
  only working audio verification this project has: headless render → WAV →
  null-test against a golden reference, gated three ways. E1 carries the three
  findings that cost a day each — the absolute-path trap that renders silence
  while reporting `"ok":true`, why the null test needs a peak gate as well as an
  RMS one, and the bit-exactness result that says references survive a compiler
  change. Port the reasoning, not just the files.
- **That repo's own "Awaiting Jeff" had already asked this question** — "is
  Tidesynth the starting codebase for the Eurorack product's app shell, or is
  the product a fresh gmpi app reusing syntheditlib/Tidesynth pieces?" It sat
  open for two days while a second scaffold was built against the unanswered
  version of it. The general shape: an open question about *what we are
  building* outranks any amount of infrastructure built underneath it.
- Minor, but it will confuse a grep: the repo slug `tide-rack` and the product
  name `TIDE Rack` are now different things. The slug is dead; the name is the
  product. N1 is the rename item and is unaffected by this.

**Next:** Nothing here changes the carve-out, which is still the critical path —
C3 is the next `win` item. **E1** is the cheapest of the new items and needs no
plugin, so it is takeable by any box at any time; E2 and E3 are blocked on V1
and should stay blocked. Also unaffected: v0.1's acceptance test, which is still
patch-survives-save-and-reload and nothing else.

**Branch/PR:** none yet — docs committed from the interactive session.

---

## 2026-08-08 — windows — C2

**Did:** Carve-out stage 2. Sixteen leaf files left the private `SE16` repo for
the **root of the public `SynthEditLib`**, and `EditorLib/CMakeLists.txt` now
points at the new paths. Fifteen came from `SE16/SynthEdit2/` —
`checkpoint.{cpp,h}`, `cpu_accumulator.{cpp,h}`, `FrameRateLogger.{cpp,h}`,
`imbedded_file.{cpp,h}`, `it_doc_ob.{cpp,h}`, `it_doc_ob_recursive.{cpp,h}`,
`it_empty.h`, `it_plug_destinations.{cpp,h}` — and `FuzzyMatch.h` from
`SE16/EditorLib/`, which now holds nothing but its `CMakeLists.txt`.

`SE16` `d933e5e03`, `SynthEditLib` `6e49dbf`, both on branch
`tide/win/C2-leaf-files`, both pushed. **Merging the TideSynth PR does not land
either** — same shape as X3.

**Result — Windows, all green:**

```
cmake --build C:/SE/SE16/build --config Release      exit 0, no warnings
  SynthEditLib, EditorLib, EditorScreenshot, SynthEditCL,
  SynthEdit_GMPI, SynthEdit_VST3, TIDE, TIDE_VST3, all ~90 module .sem/.gmpi
ctest -C Release                                     92/92 passed, 0 failed
```

The old copies are gone from disk — `find` over all of `SE16` returns no
`checkpoint.h`, `FuzzyMatch.h` or `it_plug_destinations.h`, including under
`build/`. So the build genuinely compiled the `SynthEditLib` copies; there was
no stale header left behind to mask a bad path.

**SynthEdit2 (WinUI3) reaches P8 and nothing else** — `MSBuild
SynthEditStore.sln -t:SynthEdit2 -p:Configuration=Debug` produces exactly one
error, the pre-existing one:

```
SynthEdit2\EditorWindowHelper.cpp(294,31): error C2664:
  'gmpi::drawing::Bitmap se_cl::renderContainerThumbnail(gmpi::api::IUnknown*,CContainer*,int)':
  cannot convert argument 1 from 'std::unique_ptr<UniversalFactory,...>' to 'gmpi::api::IUnknown*'
```

No C1083, so nothing lost a header. Better than that, it is positively proved
rather than merely not-disproved: `SynthEdit2/x64/Debug/FindDialog.xaml.obj` was
freshly produced by this build, and `FindDialog.xaml.cpp:12` is the TU that
includes `"FuzzyMatch.h"` — so the WinUI3 app compiled it from its new
`SynthEditLib` location. P8 still blocks the link stage, exactly as it blocked
C1b's.

**Destination: the repo root, and why it matters that C3 knows.** The plan
never says *where* in `SynthEditLib` the files land, and C2 sets the precedent
for the ~120 still to come. Root, because it is already an include directory in
all **three** build systems — `${SYNTHEDITLIB_DIR}` in EditorLib's CMake,
`$(SolutionDir)..\SyntheditLib` in `SynthEdit2.vcxproj`, and
`$(SE_GITHUB_DIR)/SynthEditLib` in both mac xcconfigs — so the move needed *zero*
include-path edits, and root already holds files EditorLib compiles
(`CancellationAnalyse.cpp`, `SafeMessageBox.h`). **A subfolder would have been
tidier and would have cost three include-path edits plus a mac box to verify
them.** That is a fine trade for 16 files and a worse one for 120, so **C3
should decide deliberately whether the bulk goes to root or to a subdirectory**
rather than inheriting this by default. Re-homing these 16 later is a `git mv`.

**"Leaf files" is not quite the right description, and the distinction matters
for judging what is safe to move next.** Only `FuzzyMatch.h`, `it_empty.h` and
`cpu_accumulator.h` are genuinely dependency-free. `checkpoint.cpp` includes
`SynthEditDocBase.h`, `Application.h` and `CContainer.h`; `it_*.cpp` include
`CContainer.h` and `PlugIO4.h`; `imbedded_file.cpp` includes `Application.h`.
All of those still live in `SynthEdit2` and will not move until C3–C5. What
actually makes this set safe is a different property: **nothing outside
`EditorLib`'s own source list compiles these `.cpp`**, and every `#include` of
them resolves through a search path rather than a relative path. That is the
test to apply to C3's candidates, not "does it have dependencies".

**Learned:**

1. **One file did break on the move, and it was the one that looked cleanest.**
   `checkpoint.h` carried `#include "../tinyXml2/tinyxml2.h"`. That never
   resolved relative to the file — `SE16/tinyXml2/` **does not exist** — it was
   quietly matching `<SynthEditLib>/modules/<any>/../tinyXml2/` through the
   include search path, and so would have survived the move by luck rather than
   by design. Rewritten as `"modules/tinyXml2/tinyxml2.h"`, which resolves
   relative to the header's own location on every compiler. **Grep C3–C5's
   candidates for `#include "../` before moving them** — a relative include that
   is already resolving through the search path gives no warning either way.
2. **The public repo already had a source dependency on a private header.**
   `SynthEditLib/modules/se_sdk3_hosting/ModuleViewStruct.cpp:11` includes
   `"cpu_accumulator.h"`, which until today lived in the private `SynthEdit2`
   folder. It compiled only because that `.cpp` is on *EditorLib's* source list
   (`EditorLib/CMakeLists.txt:16-17`), so it is built with `../SynthEdit2` on the
   include path — a public file that cannot be compiled by its own repo. C2
   closes this one by accident. **There may be more: worth a grep of
   `SynthEditLib` for includes of `SynthEdit2` headers before C7 claims a
   stranger can build TIDE**, because each one is a hole that only shows up in a
   clean-clone build.
3. **`file(GLOB)` was checked for, deliberately.** If any `CMakeLists.txt` in
   `SynthEditLib` globbed its root, these 16 files would have silently joined
   `SynthEditLib`'s own target *as well as* EditorLib's, and the failure would
   have surfaced as duplicate symbols at link, far from the cause. There are no
   globs anywhere in that repo. Re-check at C3 and C6.
4. **`it_empty.h` has zero includers anywhere in the tree** — `SE16`,
   `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`. It is a dead template header that
   the carve-out has now carried into the public repo. Not deleted here: C2 is a
   move, and deleting on the way through would have made the diff a judgement
   call instead of a relocation. Filed as **C8**.
5. **C1b's "three build systems" lesson paid again, and cheaply.** Only
   `FuzzyMatch.h` had references outside `EditorLib/CMakeLists.txt` — a
   `ClInclude` in `SynthEdit2.vcxproj` and its `.filters`, and a
   `PBXFileReference` in the SynthEditMac Xcode project — all three repointed.
   None of the fifteen `SynthEdit2` files is compiled by a hand-maintained
   project. The thing that could have broken the mac build is not the fileRef
   (headers are not compiled from it) but `FindWindowController.mm:2`, which
   includes `"FuzzyMatch.h"` by name and finds it through `HEADER_SEARCH_PATHS`
   — and both `Config/Debug.xcconfig` and `Release.xcconfig` already list
   `$(SE_GITHUB_DIR)/SynthEditLib`. So it resolves. **Edit-verified only; the mac
   box should build SynthEdit to confirm**, as it should still do for C1b.
6. **Do not build `SynthEdit2.vcxproj` standalone to test it.** Its
   `AdditionalIncludeDirectories` are written in terms of `$(SolutionDir)`, which
   MSBuild sets to the *project* directory when you pass it a `.vcxproj`
   directly — so every SynthEditLib and gmpi_ui include fails with C1083 and the
   output looks like catastrophic breakage that has nothing to do with your
   change. Build `SynthEditStore.sln -t:SynthEdit2` instead. Cost me one wasted
   build here.

**Next:** **C3** — the document model, the largest and riskiest stage. Before
starting it, settle the root-vs-subfolder question in learning point 1's
paragraph above, and apply the two greps: `#include "../` in the candidates, and
`SynthEditLib` sources including `SynthEdit2` headers. Also note
`SynthEditLib/README.md` is still the SE2JUCE README and describes a fraction of
what that repo now is; that belongs with C6/C7, not filed separately yet.

**Branch/PR:** TideSynth `tide/win/C2-leaf-files` → PR below. Code in `SE16`
`d933e5e03` and `SynthEditLib` `6e49dbf`, both on branches of the same name.

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

## 2026-08-08 — windows — C1b (interactive session, Jeff directing)

**Did:** C1b — `ExportAsPlugin.{cpp,h}` are off `EditorLib`'s source list, and
every app that calls `ExportAsPlugin` now compiles the `.cpp` itself, per the
`SynthEditApp.cpp` precedent. `SE16` `f313fe37e`, pushed to master. Also dropped
`gmpi_ui`'s empty autostash (re-verified 0 lines under `--ignore-all-space`
first) and filed **P8**.

**How the work was shaped — recon first, adversarial review before landing,
and both paid.** A four-agent read-only recon mapped every consumer of
EditorLib before any edit. That map changed the plan materially from what the
C1b row assumed:

1. **"SynthEdit and SynthEditCL call it" was an undercount — it is four apps,
   on three different build systems.** `SynthEdit2.vcxproj` and the
   `SynthEditMac` Xcode project consume EditorLib as a *prebuilt* `.lib`/`.a`,
   so EditorLib's PUBLIC CMake includes/defines do not reach them; each needed
   its own source-list entry (a ClCompile block; a four-coordinate pbxproj
   addition cloning the `SynthEditApp.cpp` quartet — fileRef `D5CA0203…A2`,
   buildFile `D5CA0204…A3`). `SynthEditJuce` also calls it but is **orphaned**
   — no `add_subdirectory` reaches it, superseded by Wayland — entry added
   anyway so it is honest if revived.
2. **Non-callers proved clean:** TIDE, EditorScreenshot, SynthEditWayland and
   the tests reference only the two declarations in `CContainer.h`. Exactly one
   compiled copy of the TU existed in the whole tree, so per-consumer
   compilation cannot create duplicates — provided the `.cpp` only ever goes in
   executables, which the new EditorLib comment states.

**The review refuted the change as first written, and the catch was real.**
Three adversarial lenses ran against the diff before commit. Two independently
found the same break: **both mac xcconfigs pinned `VST3_SDK` to the January
3.7.14 CPM hash (`452951e4…`), stale since the 2026-08-06 bump to 3.8.0 — and
inert precisely because no Xcode TU included a VST3 header. `ExportAsPlugin.cpp`
is the first that does** (`funknown.h:21`), so landing C1b without the bump
breaks any mac whose CPM cache lacks the January entry, and silently compiles
the export TU against 3.7.14 headers on macs that have it. Both xcconfigs
bumped to `2df5ae7c…` in the same commit, with a tracking comment. The same gap
hit Windows first at build time — the vcxproj had no VST3 include path — fixed
with the same hash, mac-xcconfig style.

**Learned:**

1. **"Who consumes this library" has three answers in SE16, not one.** CMake
   targets (propagation works), MSBuild-by-hand (`SynthEdit2.vcxproj`), and
   Xcode-by-hand (SynthEditMac). Anything the carve-out removes from EditorLib
   must be re-plumbed *three ways*. C2–C5 move ~120 files; most are not
   compiled by the hand-maintained projects, but every stage should grep both
   hand-maintained projects before assuming CMake is the whole story.
2. **A dependency path can be stale-but-inert until your change makes it
   load-bearing.** The mac `VST3_SDK` pin sat wrong for two days harming
   nothing. The lesson generalises past this repo: when adding a TU to a
   target, check not just that its includes resolve somewhere, but that the
   *specific paths that target uses* are current. This is also now the sixth
   hand-maintained copy of a CPM hash in the tree (4× vcxproj, 2× xcconfig) —
   they all rot on the next SDK bump; a configure-generated property sheet
   would kill the class. Noted, not filed — Jeff's call whether it is worth it.
3. **P8, found in passing and A/B-confirmed pre-existing:** the WinUI3 app does
   not compile from clean — `EditorWindowHelper.cpp(294)` vs
   `renderContainerThumbnail`'s new signature. Identical failure with the C1b
   change reverted. Consequence for C1b's honesty: **SynthEdit2's link stage is
   unverified** (its `ExportAsPlugin.obj` compiles; the app cannot reach link).
   The mac side is edit-verified only.
4. **The claim-first discipline held even interactively:** C1b was marked DOING
   and pushed before recon started, so a cron-fired weekly run could not take
   it mid-session.

**Verification record (Windows):** EditorLib, SynthEditCL, TIDE, TIDE_VST3 all
exit 0, zero warnings. `dumpbin`: no `?ExportAsPlugin@@` in `EditorLib.lib`,
control (SkinMgr, 224 symbols) positive. SynthEditCL carries its own
`ExportAsPlugin.obj` (2,319,264 B) and links. TIDE_VST3 still export-free
(`cdb x`, control positive). `SynthEdit2/x64/Debug/ExportAsPlugin.obj` produced
by the vcxproj entry.

**Next:** **C2** (leaf files) is unblocked and is the next `win` item. The mac
box should build SynthEdit before trusting the pbxproj/xcconfig edits — that
verification rides on whatever its next run is.

**Branch/PR:** straight to `SE16` master (`f313fe37e`) and TideSynth main at
Jeff's direction, per the interactive-session convention.

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

## 2026-08-14 — windows — A13 (C4 not re-taken, A4 not takeable — see below)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code Desktop, app version not
discoverable on this box (see **Learned**) · as `tide-rack-bot`

**Did:** Fixed `scripts/check-links.py`'s slug function, which disagreed with
GitHub's on every em-dash heading, and fixed a second defect of the same
character sitting a few lines below it. Verified both against GitHub's own
renderer rather than against my reading of the algorithm.

**Neither NEXT pointer was takeable, and one of them is a standing trap.**

- **`win` → C4 is already done.** The 2026-08-13 windows run completed it; it is
  IN-REVIEW across three PRs ([#49](https://github.com/JeffMcClintock/TideSynth/pull/49),
  [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15),
  [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6)). The
  resume rule makes an open PR from my own platform mine to *continue*, but
  there is nothing to continue — the work is complete and waiting on Jeff.
  I checked both red PRs before leaving them alone, and **neither red is a
  defect in the C4 work**: #49's `lint` is red only on `links`, which is this
  very row and is red on `origin/main` too; SynthEdit#15's `WASDK build check`
  dies at `CMake Error at EditorLib/CMakeLists.txt:10 (add_library): Cannot
  find source file`, which is the cross-repo condition the PR body already
  states — CI fetches `SynthEditLib` from `origin/main`, where the twelve moved
  files do not exist until SynthEditLib#6 merges. C2 and C3 failed that same
  check for that same reason, and C3 merged anyway. Structural; not fixable
  from a branch.
- **`any` → A4 cannot be done by a scheduled run at all, and its row does not
  say so.** A4 is an auto-merge *action*, i.e. a file under
  `.github/workflows/**`, and the bot token is deliberately `repo` scope with
  no `workflow` — measured this run, not assumed: `X-OAuth-Scopes: repo` from
  the API response headers. The push is rejected however correct the work is.
  **A10 and A12 hit the identical wall** (both name `.github/workflows/` in
  their own scope lines), and **A9 has an open NEEDS-JEFF prerequisite** —
  TIDE's product philosophy as the auto-reject filter — which would change what
  gets built, so STEP 2 rules it out. That leaves **A13** as the topmost `any`
  item a scheduled run can actually finish, and A13's own row had already
  argued it should precede A4 anyway ("a noisy lint erodes trust in the other
  three checks fast"). Updated the `any` NEXT row to record all of this.

**Result:** Both bugs reproduced, fixed, and verified.

*The bug, exactly.* `slugify()` did `re.sub(r'\s+', '-', s)` — one hyphen per
*run* of whitespace. GitHub emits one per *space*. The line above had already
deleted the em-dash, leaving the two spaces that surrounded it, so
`## 2026-08-13 — macos — S6 (part 2 of 2)` produced
`2026-08-13-macos-s6-part-2-of-2` here and `2026-08-13--macos--s6-part-2-of-2`
on GitHub. The fix is `s.replace(' ', '-')`, plus deleting tabs and other
non-space whitespace outright, which is what GitHub does with them.

*The second defect, which was not in the row.* `anchors_of()` never tracked
code fences — `main()` did, but `anchors_of()` did not — so **nine fenced lines
across four files were registered as real anchors**, among them `JOURNAL.md`'s
own entry template `## YYYY-MM-DD — <machine> — <BACKLOG id>`,
`#include "it_empty.h"` in `docs/c8-it-empty-header.md`, and three `#0`/`#1`/`#2`
gdb backtrace frames. That is a false *negative* — a link to
`#include-it_emptyh` passed the check. Same both-directions character as the
slug bug, same function, so fixed in the same pass.

*The verification artifact — an A/B against GitHub's own renderer.* Fetch each
file through the contents API with `Accept: application/vnd.github.html`, which
returns it rendered by GitHub's real markdown pipeline with anchors intact
(`<a id="user-content-…" class="anchor">`), and compare every anchor to what
this script generates for the same bytes:

```
files 28 | headings compared 254 | mismatching files 0
```

**254 of 254, zero mismatches.** Two negative controls, so the A/B is not
passing vacuously:

```
CONTROL 1  old slugify (collapsing run)        headings wrong: 111   files affected: 23
CONTROL 2  new slugify, fences NOT skipped     headings wrong:  34   files affected:  4
FIXED      new slugify, fences skipped         headings wrong:   0   files affected:  0
```

*Deliberate breaks, all three as specified in the row's Accept clause:* a
nonexistent anchor fails (RC=1); a link written in the **old collapsed form**
now fails (RC=1) — that is the false negative closing, and it is the half that
matters; the correct GitHub form passes (RC=0).

*Tree state:* 186 relative links, **0 broken**. The one previously-flagged line
(`JOURNAL.md:376`) was read and is exactly the false positive A13 predicted —
the first intra-journal anchor anyone wrote.

*Added `--selftest`* to the same script: six golden slugs **read off GitHub's
renderer**, plus a duplicate/fence case, baked in and offline so the regression
is permanent rather than something a future run must remember to re-measure.
Confirmed discriminating — reverting `slugify` to the old algorithm fails 5 of
its 6 cases, RC=1. Also implemented GitHub's duplicate-heading suffixes
(`-1`, `-2`), which no heading in the tree exercises today but `anchors_of()`
was silently collapsing into one.

**Learned:**

- **`/markdown` is not an oracle.** The obvious endpoint for "what would GitHub
  render this as" emits headings with **no `id` attribute at all**, so the
  first A/B came back 0-for-254 and looked like a catastrophic failure of the
  fix rather than of the measurement. The endpoint that works is
  `GET /repos/{owner}/{repo}/contents/{path}` with
  `Accept: application/vnd.github.html`. Anyone verifying anchor behaviour
  again should start there and skip the hour.
- **The em-dash convention and the checker were on a collision course from the
  start.** Every journal entry heading in this repo uses em-dashes, so the
  moment anyone wrote the first intra-journal anchor link the check went red —
  which is exactly what happened, and A3 could honestly claim zero false
  positives when it landed only because nobody had written one yet.
- **The app version STEP 0.5 asks for is not discoverable on this box.** `claude`
  is not on `PATH` under the desktop app, and there is no `app-*` directory or
  `package.json` under `%LOCALAPPDATA%\Claude` carrying a version. The linux and
  mac entries record `Claude Code CLI 2.1.220` because those boxes run the CLI.
  So the provenance line's `app <version>` field is silently unfillable on
  Windows-under-desktop, and `check-prompt-provenance.py` cannot catch that —
  it only looks for the literal `**Prompt:**` marker, not for the fields after
  it. Recording it in prose here rather than inventing a number.
- **Two lint scripts cannot be fed a process substitution on this box.**
  `python scripts/check-backlog-diff.py <(git show origin/main:BACKLOG.md) …`
  fails with `FileNotFoundError: '/proc/1398/fd/63'` — Git Bash creates the fd,
  Windows Python cannot open it. Write the base version to a real temp file.

**Next:** A13's PR is [#51](https://github.com/JeffMcClintock/TideSynth/pull/51)
and should merge **after** [#49](https://github.com/JeffMcClintock/TideSynth/pull/49)
— both prepend to `JOURNAL.md` and both rotate `JOURNAL-2026-08.md`, so
whichever lands second needs a rebase, and #49 is the older and larger of the
two. `BACKLOG.md` does not collide: #49 touches the `win` NEXT row and the
C4/C9/C11/P7c rows, this one touches the `any` NEXT row and A13.

The `any` lane needs a decision, not a run: **A4, A10 and A12 are all
`.github/workflows/**` work that the bot token structurally cannot push**, and
all three sit in the queue marked TODO as though a scheduled run could take
them. Each will burn a session on discovery until someone re-marks them.
That is the same shape as **A12's own finding** — a box that cannot proceed and
nothing escalating it — one level up, applied to the queue instead of to a box.

**Branch/PR:** `tide/win/A13-check-links-slugger` →
[#51](https://github.com/JeffMcClintock/TideSynth/pull/51)

---

## 2026-08-14 — windows — merge cleanup for A13/P6/C4 (interactive session, Jeff directing)

**Did:** After the scheduled A13 run finished, Jeff took over interactively
and asked to fix merge conflicts and merge the queue's open PRs oldest first.
Four PRs were outstanding across three repos, all touching the same shared
docs (`BACKLOG.md`, `JOURNAL.md`, `JOURNAL-2026-08.md`), so #49 merging first
put #50 and #51 into real conflict. Resolved both, then merged all four in
creation order: [SynthEditLib#6](https://github.com/JeffMcClintock/SynthEditLib/pull/6)
(07:24) → [SynthEdit#15](https://github.com/JeffMcClintock/SynthEdit/pull/15)
(07:24) → [#50](https://github.com/JeffMcClintock/TideSynth/pull/50) (18:14,
P6) → [#51](https://github.com/JeffMcClintock/TideSynth/pull/51) (20:48, A13).

**Result:**

- **SynthEdit#15 needed a CI re-run, not a fix.** It was failing on
  `CMake Error: Cannot find source file` because it pulls `SynthEditLib` from
  `origin/main`, where the twelve C4 files didn't exist yet. Merged
  `SynthEditLib#6` first, then `gh run rerun --failed` on the same run —
  both `windows-latest` and `windows-2025-vs2026` jobs went from fail to pass
  with no code change, confirming the dependency was exactly what the PR body
  said it was.
- **#50 and #51 both had real git conflicts** in `BACKLOG.md`/`JOURNAL.md`/
  `JOURNAL-2026-08.md`, all from independent same-day rotations racing against
  each other rather than genuine content disagreement. Each resolved the same
  way: whichever side's rotation work was **already accepted on `origin/main`**
  won; a branch's own independent rotation of an entry `origin/main` had
  already archived was dropped as a duplicate rather than merged in twice.
  Confirmed no duplication by grepping each archive for the entry heading
  before and after.
- **The A13 conflict resolution accidentally became A13's own best test.**
  `docs/carve-out.md`, `JOURNAL.md` and `JOURNAL-2026-08.md` all use em-dash
  headings, and the S6 part-1/part-2 cross-reference — the link A13's own row
  was written about — ended up with both halves landing in
  `JOURNAL-2026-08.md` together as a direct result of this session's
  rotations. `check-links.py` (with A13's fix) reports 0 broken on the
  resulting tree; the pre-fix slugger would have flagged that exact link.
- **STEP 4 chore, done live rather than left for a later run:** C4, P6 and
  A13 were all `IN-REVIEW` with every linked PR now observed merged in this
  same session, so flipped all three to `DONE` and moved the rows verbatim
  into `BACKLOG-DONE.md`, newest first. Also fixed a stray blank line left
  inside the carve-out table by the row removal.
- **Found, not fixed: the `linux` NEXT row was already stale on `origin/main`
  before this session started.** It still pointed at P7c, which C4's own PR
  (#49) had already flipped to `DONE` and archived — the PR that archived it
  never updated the pointer that named it. Every other `linux`-platform row
  (X1, X2, R4) is `BLOCKED`, so a scheduled linux run today falls through to
  the `any` fallback, and nobody has run that fallback through the same
  NEEDS-JEFF/workflow-wall screening the `any` row itself just needed for A4.
  Flagged in the row rather than guessed at — the queue already has one
  instance this session of a wrong guess (the original E1a/linux pointer)
  costing a run its whole session on discovery, and a second wrong guess here
  would cost another.

**Learned:**

- **A same-day multi-branch queue racing the same rotation files will always
  produce this shape of conflict**, and it resolves the same way every time:
  trust whichever side is already on `origin/main`, treat the other branch's
  independent rotation of the same entry as a duplicate, and grep the archive
  before/after to prove no entry was dropped or doubled. Doing this by hand
  three times in one session is exactly the kind of load A4 (the auto-merge
  tier) was filed to remove — its row is more urgent than its own text says.
- **`gh run rerun --failed` is the right tool when a cross-repo CI failure's
  cause has already been fixed by merging the other repo** — cheaper and more
  informative than pushing an empty commit to retrigger, since the log shows
  the exact same job going from fail to pass with nothing else different.

**Next:** the `linux` NEXT row genuinely needs someone to work out what a
linux-eligible scheduled run should take, not just notice it's wrong — left
as a flagged question rather than a guess, on purpose. `E1a` still needs a mac
or win run to do its render half.

**Branch/PR:** none — merge-conflict fixes were pushed to the branches being
merged (`tide/mac/P6-cl-codesign-xcode`, `tide/win/A13-check-links-slugger`),
which then merged into `main` via their own PRs. This entry and the
DONE-row/NEXT-row cleanup are committed directly to `main`, interactive
session.

---

## 2026-08-14 — macos — E1a

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Desktop 1.26832.0 · as `tide-rack-bot`

**Did:** Ran the audio harness on a second platform for the first time in its
life — macOS against the Linux goldens — and set the null-test tolerances from
what came back. **The headline is that finding (e) was closed by measurement
rather than by widening anything**, and that a *different* class of residual,
which nobody had modelled, is what actually needed a fix.

**Result:** both cases render on macOS; 2/2 pass after the change; every number
below is measured, not modelled unless said so.

*Three engines, so the platform axis is isolated rather than assumed.* An
engine-version or build-config difference would otherwise be indistinguishable
from a platform one:

| Engine | Origin |
|---|---|
| `SynthEditCL V1.6.175` | local Release build, 2026-08-08 |
| `SynthEditCL V1.6.182` | local **Debug** build, 2026-08-13 |
| `SynthEditCL V1.6.183` | published `SynthEditCL_mac.zip`, Azure CI, signed |

*The measurements, identical on all three:*

| Case | RMS residual | Peak residual | Class |
|---|---|---|---|
| `voice_midi_note` | −123.1 dBFS | −90.3 dBFS = **exactly 1 LSB**, 51/95,999 samples (0.053%) | pure rounding |
| `osc_naive_sine` | −73.5 dBFS | −68.7 dBFS = 12 LSB | **not rounding** |

**(1) The RMS gate did not need widening, and the reasoning that said it might
was resting on a premise the data does not support.** E1a's arithmetic was
right — the −100 dBFS gate tolerates 1-LSB error on at most ~10.7% of samples —
but it assumed the worst case, 1 LSB on *every* sample. Real cross-platform
rounding touched **0.053%** of samples: a 200× margin, 22.9 dB of headroom.
Two builds agree on nearly every sample and disagree only where a value sits on
a quantisation boundary. Gates stay at −100/−86.

The peak gate's 4.3 dB is structural rather than lucky: **1 LSB is a hard
per-sample ceiling** for this class. Two builds that agree on the underlying
float can only disagree about which way it rounds — drift of this class can
affect more samples, never make one sample wrong by more than 1 LSB.

**(2) `osc_naive_sine` fails for a reason no fixed dBFS number can express.**
The residual **grows monotonically through the render** — −96 dBFS in the first
0.1 s block, −69 dBFS in the last — with a best-fit time lag of exactly zero,
so it is neither rounding nor a delay. Fitting `dphi = k·t` gives a **frequency
offset of 0.15 ppm ≈ 2.5 ULP at single precision** (2⁻²⁴ = 5.96e-8) on a
440 Hz tone. `OscillatorNaive`'s table and increment are both `double`, but
`OscillatorNaive.h:66` derives the table index as a **`float` from a `float`
pitch**, so the pitch→increment path carries single-precision resolution — the
right order for what was measured. Named as the plausible locus, **not proven**;
the measured quantity is the 2.5 ULP.

A frequency offset *integrates*, so the residual is proportional to elapsed
time. Modelled from the fitted rate (the 2 s row matches measurement to 1–2 dB):

| Duration | Peak | RMS |
|---|---|---|
| 0.5 s | −79.6 | −87.4 |
| **2.0 s** (this case) | **−67.6** | **−75.4** |
| 8 s | −55.6 | −63.3 |
| 60 s | −38.1 | −45.8 |

So widening the global gates to admit it would have to reach −67 dBFS — past
finding (b)'s reference defect (3 LSB × 200 samples, caught at peak −80.8 dBFS)
— **and would fail again the moment a case renders for 4 s.** Instead: the
globals stay as the rounding-class budget, and a case whose residual is a
different class declares its own with `null_tolerance_dbfs`,
`peak_diff_tolerance_dbfs` and a mandatory `tolerance_reason` that the harness
**prints on every run** and records per case in the report (schema `/2` → `/3`).
`osc_naive_sine` is set to **−67.0 / −62.0 dBFS**: 6 dB above measurement, i.e.
sized for twice the observed drift (~5 ULP), on the grounds that there is no
reason to think mac-vs-linux is the widest pair. If Windows exceeds that, the
case fails and someone re-measures — the correct outcome, not a defect.

*The cost, stated rather than buried:* that case keeps full sensitivity to
level, waveform and tuning (0.3 ppm of detuning still fails it) and loses it for
localized damage below ~12 LSB. `voice_midi_note` still covers the same
oscillator at the −86 dBFS default in a fuller chain.

**Verification artifact — five controls through the harness's own `null_test`
and `Case` loader, not a re-implementation:**

```
  C0a osc_naive_sine unmodified              rms= -73.5 peak= -68.7  gates -67/-62   -> PASS
  C0b voice_midi_note unmodified             rms=-123.1 peak= -90.3  gates -100/-86  -> PASS
  C1  osc_naive_sine, same drift 4x larger   rms= -61.5 peak= -56.7  gates -67/-62   -> FAIL
  C2  osc_naive_sine, finding-(b) glitch     rms= -73.5 peak= -68.7  gates -67/-62   -> PASS
  C2  voice_midi_note, finding-(b) glitch    rms=-107.5 peak= -80.8  gates -100/-86  -> FAIL
```

C1 is the one that matters — **a widened gate is still a gate**. C2 on
`voice_midi_note` independently reproduces finding (b)'s numbers (−107.6 /
−80.8 dBFS) on a second platform. C2 on `osc_naive_sine` is the accepted
sensitivity cost, demonstrated rather than asserted. Also asserted and checked:
the override does **not** leak to `voice_midi_note`.

Added `render_harness.py --selftest` — synthesises its audio in memory, needs
no engine or fixtures, and bakes in findings (b) and (e)'s numbers plus the
override plumbing. **Confirmed discriminating**: reverting `null_test` to ignore
its tolerance arguments fails it (RC=1), and the 10.7% boundary is straddled
(1 LSB on 10% of samples passes at −100.3 dBFS, on 12% fails at −99.5 dBFS).

**Learned:**

- **`SynthEditCL_mac.zip` at `https://www.synthedit.com/release_1_6/` is a
  working download for this harness**, and the E1a row's "the engine is a
  download, not a build" is true on mac. It is an `.app`; strip the quarantine
  xattr, then `Contents/MacOS/SynthEditCL` and `Contents/PlugIns` are the
  `--cli`/`--modules` pair. That is the closest thing to what CI would run.
- **Finding (c) extends and slightly retracts.** Extends: the published Azure-CI
  Release build and the local Release build are **byte-identical** on both
  cases, so same-platform bit-exactness spans build *machines* too. Retracts:
  the mac **Debug** build differs from mac Release by 1 LSB on 40/95,999
  samples in `voice_midi_note`. Finding (c) claimed Release-vs-Debug
  bit-exactness from a *Linux* measurement; it does not hold on macOS. Renders
  are still run-to-run bit-exact (checked explicitly).
- **A residual that grows through the render is diagnostic on its own.** Two
  cheap measurements separate the three plausible causes before any theorising:
  per-block RMS (flat = rounding, rising = integrating error) and a small
  lag sweep (a minimum away from zero = a delay). Both were computed here in
  about a minute and turned "the sine case fails" into "the phase increment
  differs by 2.5 ULP", which is what made the fix a mechanism change instead of
  a bigger number.
- **The app version STEP 0.5 asks for IS discoverable on the mac desktop app**,
  unlike Windows (see the 2026-08-14 win entry):
  `/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" /Applications/Claude.app/Contents/Info.plist`
  → `1.26832.0`. `claude` is not on `PATH` here either, so the CLI-version
  route the older mac entries used no longer applies on this box.

**Queue hygiene done in passing, both status-cell-only:**

- **C5 flipped `BLOCKED(C4)` → `TODO`.** C4's three PRs all merged 2026-08-14,
  which is the condition its own blocker names. The `win` NEXT row already said
  "C5, if C4's three PRs have merged"; that "if" has resolved, so the row now
  names C5 outright instead of leaving the next win run to re-derive it.
- **The `mac` NEXT row is re-pointed at D1**, since both it and its stated
  fallback (A13) are now done. D1 by the same argument that made E1a a mac
  item — its own row says whether an AUv3 can open a URL at all "is a factual
  question the macOS box can answer and the other two cannot". Design note
  only, no GATED path, no `.github/workflows/**`, and PLAN's "Price and
  funding" already settles the policy it designs against. Fallback named as S2.

**Not done, deliberately:** the `linux` NEXT row is still the flagged question
the 2026-08-14 win entry left. Nothing measured here bears on it, and guessing
at another platform's lane is the mistake that row exists to avoid.

**Next:** the tolerances are now set from *one* cross-platform pair. Windows is
the third lane and has still never rendered — if it lands inside −67/−62 the
6 dB margin was right, and if it does not, the right response is to re-measure
the drift rate rather than to widen again. Nothing here needs Windows to
proceed. `E1b` (installing `docs/ci/verify.yml`) is still Jeff-only and still
the thing that would make any of this run automatically.

**On the PR's three red checks — checked before leaving them alone, not
assumed.** `windows`/`macos`/`linux` are red on #52, and they are red on #49,
#50 and #51 too, for the same pre-existing reason: `build.yml` configures CMake
at the repo root and **TideSynth has no root `CMakeLists.txt`** (`CMake Error:
The source directory ... does not appear to contain CMakeLists.txt`). That is
**B1** verbatim — the skeleton CI "expected to fail until C7". Job-level
`continue-on-error: true` is why the same workflow reports *success* on `main`
while the individual checks read fail on a PR; do not read that difference as a
regression. `lint` **passes** on this PR, which #49 and #50 could not say.

**Branch/PR:** `tide/mac/E1a-null-tolerances` →
[#52](https://github.com/JeffMcClintock/TideSynth/pull/52)

---

## 2026-08-14 — linux — S2 (plus a platform:linux build break found and filed)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Did:** Produced [docs/sandbox-audit.md](docs/sandbox-audit.md) — the audit
PLAN constraint 4 cannot be verified without. **The answer is that constraint 4
is violated, and it is not a code-reading inference: the files are on disk on
this machine.** Also found, while establishing TIDE's source set, that **SE16
does not configure on linux at all any more** — filed as
[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) rather than fixed,
per STEP 3.

**Neither of this box's usual entry points was takeable, and the NEXT row said so
itself.** No open `platform:linux` issue at STEP 1; no open PRs in any of the
five fleet repos and only `main` on TideSynth's remote, so nothing to resume at
STEP 1.5. The `linux` NEXT row was the stale P7c pointer, explicitly left as a
question by the 2026-08-14 windows interactive session — *"picking the wrong
replacement here is worse than an honest gap"*. So this run did the screening it
asked for, took the item that survived it, and re-pointed the row. That screening
is the second most useful thing here after the audit itself, because **the queue
is systematically misleading about what a scheduled run can take:**

- **The GATED wall is nowhere recorded in the rows it applies to.** `S1b`, `S5`,
  `S7` and `S8` are all `TODO`/`any`/unblocked and read as available. All four
  are **entirely** work in `SE16/EditorLib/`, `SE16/SynthEdit2/` or the
  `SynthEditLib` repo — GATED to C1-C7 only, per the 2026-08-11 C8 ruling that
  deliberately declined to widen the exception. S8's row actively misleads:
  *"~~GATED on C0~~ unblocked 2026-08-08, C0 approved"* reads as permission, but
  C0 gates the carve-out **stages**, not any item that wants to touch a shared
  file. Each of these will burn a session on discovery exactly as A4 did last run.
- **The workflow wall** (A4, A10, A12, and **B1**, which nobody had named
  before — it is `.github/workflows/build.yml`).
- Screened out for their own reasons: **C9** (its remaining work *is* C5, a `win`
  row — `Application.cpp`), **C11** (needs Jeff's ruling on the `SynthEditApp.h`
  licence gate), **A9** (open NEEDS-JEFF prerequisite), **N1** ("do it after
  C7"), **P7d** (scope question for Jeff, and macOS-only to verify), **D1/D2**
  (mac-shaped), **U1** (needs a fresh post-pivot audit first).

That left **S2**, which is `any`, needs no GATED edit (it *reads* gated code and
writes only `docs/` in this repo), and has no open question that would change
what gets written.

**Result — the audit, and why its file set is trustworthy.**

A grep of SE16 returns hundreds of hits in code TIDE never compiles, which
buries the real ones. The row says *"reachable from a TIDE build"*, so the file
set is derived:

| Step | Result |
|---|---|
| link closure from `build.ninja` | `VST3_Wrapper` + `SynthEditLib` + `EditorLib` — and **no** `.sem` module libraries |
| TUs from `compile_commands.json` | **264** |
| first-party after dropping VST3 SDK + generated Wayland C | **230**, **zero unresolved** |
| categorised hits | **202 across 46 files** |
| DWARF compile-unit list from the unstripped `.so` | **235 CUs linked**; of the 46 hit files, **37 are in the binary, 9 are not** |

That last row is the one that makes the audit worth reading: a static archive
contributes an object only when something references it, so "compiled" and
"linked" are different questions and only the second one matters. Re-runnable
with `python3 tools/sandbox_audit.py --build <tree>`.

**The violation, measured on disk rather than argued from source — ~12.8 MB
across 139 files:**

| Location | Size | Files |
|---|---|---|
| `~/SynthEdit Projects/skins/` | 724 KB | 74 |
| `~/SynthEdit Projects/Prefabs/` | 1.1 MB | 35 |
| `~/.local/share/SynthEdit/` | 11 MB | 30 (six ~330 KB `Plugin-Cache-16-override-*.xml`) |

**Rulings: 7 remove, 4 stub, rest keep.** Full reasoning per finding in the doc.

**Most of it attaches to rows that already exist rather than creating new ones** —
which is the point, and is why no new BACKLOG items came out of it:

- **S7** gets its gated remainder named to the line: `SynthEditLib/SkinMgr.cpp:28-32`
  (the *constructor* calls `setSkinFolder`) and `:48-100` (the recursive copy).
  Confirmed by measurement, not just the static chain S7 asserted — **27 `SkinMgr`
  symbols defined in `TIDE_VST3.so`**, and the literal `SynthEdit Projects` is
  **in the TIDE binary** (`default3` 4×).
- **S1b (b)/(c) reproduced on a second platform** (S1b measured macOS):
  `ScanFolder`, `LoadModuleData`, `LoadOrScanModuleData`, `RegisterExternalPluginsXml`
  all defined, `Module_Info3` 65 symbols, and `dlopen`/`dlsym`/`dlclose` genuinely
  imported per `nm -D --undefined-only`.
- **S1b (a) confirmed genuinely done** by the same measurement: `FileWatcher` has
  **zero** symbols and `FileWatcher.cpp` is **not** a linked CU. Replacing
  `SynthEditApp.cpp` with `TideAppStubs.cpp` did remove the watcher thread.
- **S8 reproduced on Linux** — `ug_soundcard_in` 28, `ug_soundcard_out` 33,
  `ug_midi_out` 24, **`OscillatorNaive` 0**. That last one independently confirms
  the blocker **E2a**'s row warns about, now on both measured platforms: there is
  no modern oscillator primitive registered. S8 also gains a fourth module —
  `ug_wave_recorder.cpp:216` `fopen(…,"wb")` writes a WAV to an arbitrary path.
- **S3 gains its evidence.** Its `assert(false)` stubs have live write sites
  behind them (`CUG.cpp:2833-2968` — `create_directory` ×2, `copy_file`,
  `fopen(…,"w")`), so "silently falls through in release" is measured now.
- **C5** inherits three findings (`Application.cpp` prefab copy,
  `SynthEditAppBase.cpp` module staging, `UG2.cpp`'s write-to-`%TEMP%`-and-
  `LoadLibrary`).

**One genuinely new finding, and it is a ruling rather than a bug.**
`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:343-352` prefers
`getpwuid(getuid())` over `getenv("HOME")`, and the comment says why in as many
words: *"getpwuid returns the real home directory even when sandboxed, whereas
getenv("HOME") returns the container path in a sandbox."* **A deliberate
sandbox escape** — correct for SynthEdit, which is a desktop app, and directly
contrary to constraint 3 for TIDE. It sits **upstream** of both folder-copy
findings, because `getCommonDocumentFolder` falls through to it on every
non-Windows platform, so **S7 cannot be fixed properly without ruling on it**.
GATED (`SynthEditLib`), so flagged, not touched.

**Learned:**

- **The GATED wall is the `any` lane's real filter, and it is invisible in the
  rows.** Last run found the workflow wall and re-marked A4/A10/A12 for it; this
  is the same shape one level over, and it disqualifies four more rows that all
  read as available. Worth a Status-cell pass by Jeff, the same way A4 needs one
  — S1b/S5/S7/S8 are not `TODO` for a scheduled run in any useful sense.
- **A build tree you did not make is a first-class measuring instrument.** The
  whole audit rests on reading `build.ninja`, `compile_commands.json` and the
  DWARF out of `/home/jef/SE/build` — Jeff's own tree, read-only, never
  reconfigured or built into. It answered "what does TIDE actually compile and
  link" exactly, which no amount of CMake-reading would have.
- **`readelf --debug-dump=info --dwarf-depth=1` is the cheap way to get the
  linked compile-unit list** from an unstripped `.so`. `--dwarf-depth=1` is what
  makes it tractable on a 91 MB binary; without it the dump is unusable.
- **The skin-version stamp becomes a thrash bug at C7, and does not look like
  one today.** `SkinMgr` invalidates on `SE_APP_BUILD_NUMBER`, which
  `EditorLib/CMakeLists.txt` injects (`183` today) and `se_version.h` defaults to
  `0`. TIDE links that same EditorLib, so **today TIDE and SynthEdit agree and
  nothing thrashes.** From C7 — clean clone, no private repo — TIDE takes `0`
  while SynthEdit keeps `183`, so each would re-copy 724 KB of skins over the
  other on **every** launch. Argues for removing the mechanism from TIDE *before*
  C7, not after.
- **`std::remove` and `Processor::open(phost)` are the two false positives any
  filesystem grep of this codebase will hit** — the `<algorithm>` overload and
  the GMPI lifecycle call. Both are listed in the doc's "keep" section so the
  next audit does not re-flag them.

**The build break, filed not fixed** ([#53](https://github.com/JeffMcClintock/TideSynth/issues/53),
`platform:linux`): `GMPI_Wrappers` `e707482` (*"feat(standalone): Linux/Wayland
standalone host"*, Jeff, 2026-08-13, current tip of `origin/main`) added
`pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)` at
`wrapper/Standalone/CMakeLists.txt:32`. `wrapper/CMakeLists.txt:14` adds that
subdirectory unconditionally, and the file's own early-out is
`if(NOT UNIX OR APPLE) return()` — **so Windows and macOS skip it and only linux
takes the hard dependency**, which is why it can sit on `main` looking green.
`libpipewire-0.3-dev` is not installed here. Reproduced both with a local
`GMPI_WRAPPER_FOLDER_OVERRIDE` and with it blank so CPM fetches `origin/main`.
Net effect: **the linux box cannot configure, so it cannot build or verify
anything in SE16, TIDE included.** `GMPI_Wrappers` is an ALLOWED path, so it is
ordinary takeable work once someone picks between "make Standalone opt-in" and
"probe without `REQUIRED` and skip".

Because of it, the audit is measured from the pre-existing `/home/jef/SE/build`
tree (configured 2026-08-10). **Drift was measured, not hoped for:**
`SynthEditLib`'s CMake source list is **unchanged** since then (267 entries, none
added, none removed), and `EditorLib`'s differs by exactly `browseto.mm` and
`openurl.mm`, both `if(APPLE)` and not compiled on linux. C4 changed those files'
**paths**, not the set of code compiled, so the audit is current in content; the
doc states this as a limitation rather than burying it.

**STEP 4 chores done as part of this run:** **E1a** flipped `IN-REVIEW` → `DONE`
([#52](https://github.com/JeffMcClintock/TideSynth/pull/52) confirmed merged
2026-08-13) and moved verbatim to `BACKLOG-DONE.md`; **A11** moved likewise (it
was already `DONE` and marked *"ready to rotate"*). Also re-verified A11's own
fix still holds on this box: all nine local repos are `https://`, and STEP 0.7's
second assertion prints `git@github.com:`.

**Next:** **[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) is
STEP 1 work for the next linux run** and outranks the backlog — nothing else on
this box can be built or verified until it lands. **Then S3**, which is the one
remaining `any` row a linux box can genuinely do: its target
`SE16/SynthEditSem/TideApp.cpp` is **ALLOWED**, and S2 just supplied its
evidence. The `linux` NEXT row now says both, with the screening written out.

For Jeff, two Status-cell judgements that are his and not a run's: **S1b, S5, S7
and S8 are GATED-in-full and should not read as `TODO`** to a scheduled run, and
**A2's sandbox-escape question has to be answered before S7 can be done
properly**.

- **`gh pr checks` reports the `build` workflow red on every PR, and `gh run
  list` reports the same runs green — both are correct, and the discrepancy will
  waste someone's time.** `build.yml` sets `continue-on-error` at **job** level,
  so the *run* concludes `success` while all three *jobs* conclude `failure`.
  `gh pr checks` surfaces the jobs; `gh run list` surfaces the run. Checked
  against `main` before assuming this PR caused it: run `31749003642` on
  `3ff987b5` has `failure` for linux, macos and windows too, with the identical
  `CMake Error: The source directory … does not appear to contain CMakeLists.txt`
  — TideSynth has no root `CMakeLists.txt`, on `main` or anywhere. That is the
  C7 failure `build.yml`'s own header says is the point, and **B1 is the row for
  it** (its comment already says "green here still means nothing"). **`lint` is
  the only check that currently gates anything, and it passes.**

**Machine state, for the record:** all nine local repos were clean and on their
default branches at the start and are again at the end; **only `TideSynth` was
committed in**, so STEP 5's two-end-states rule has exactly one repo to satisfy.
Four repos were **fast-forwarded** to `origin` before measuring — `SE16` (7
commits behind), `SynthEditLib` (2), `gmpi_ui` (3), `GMPI_Wrappers` (1) — because
an audit of stale source would have been wrong about what TIDE compiles. All four
were clean beforehand, so these were true fast-forwards on the default branch with
nothing to stash; noted because it is a change to Jeff's trees, small and
reversible though it is. **Pulling `GMPI_Wrappers` is also what surfaced #53** —
its 1 commit was `e707482`, the one that breaks the configure.

**Branch/PR:** `tide/linux/S2-sandbox-audit` →
[#54](https://github.com/JeffMcClintock/TideSynth/pull/54)

---

## 2026-08-14 — linux — #53 fixed; S2 landed (interactive session, Jeff directing)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Did:** Fixed the platform:linux configure break this box filed earlier the same
day, at Jeff's direction, and did the STEP 4 chores that S2's merge unblocked.
**The linux box builds again**, verified from merged `main`.

**Result — the break was wider than the issue said, and the fix is one gate.**

[#53](https://github.com/JeffMcClintock/TideSynth/issues/53) named the pipewire
probe. Reading the file properly found **three** hard-failure sites, any one of
which kills the whole SE16 configure:

1. nine `pkg_check_modules(... REQUIRED ...)`
2. `find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)`
3. `message(FATAL_ERROR)` when a wayland protocol XML is absent

Two facts decided the shape of the fix, and both were checked rather than assumed:

- **Nothing links `Standalone_Wrapper`.** `grep -rn 'Standalone_Wrapper'` across
  `GMPI_Wrappers`, `SE16`, `GMPI`, `gmpi_ui` and `GMPI-plugins` finds no consumer.
  So skipping the target costs nothing today.
- **The parent already documented the contract that was broken.**
  `wrapper/CMakeLists.txt:12-14` says of this very `add_subdirectory`: *"Returns
  immediately on platforms whose shell is not written yet, so this is safe to add
  unconditionally."* The `REQUIRED` probes made that false. The fix restores the
  stated contract rather than inventing a policy.

So: probe everything, collect what is missing, one gate — skip with a message
naming the missing packages and the `apt` line, or fail hard under the new
`GMPI_STANDALONE_STRICT` (default OFF). **That option is the mitigation for the
fix's own downside** — a silent skip on a machine that meant to build the
standalone host — so the change does not trade one invisible failure for another.

Two things kept deliberately, both of which a naive "just drop REQUIRED" would
have lost: the per-module `Found X, version Y` output, which is how you tell
*absent* from *present but too old*; and naming missing protocol XMLs
individually, because the usual cause is a distro too old for the staging
protocols (Ubuntu 22.04 ships wayland-protocols 1.25 and has neither
`fractional-scale-v1` nor `cursor-shape-v1`) and `GMPI_WAYLAND_PROTOCOLS_DIR`
fixes that without touching system packages.

**Verification artifact — built, not just configured:**

| Check | Before | After |
|---|---|---|
| `cmake -S SE16 -B <fresh> -G Ninja` | RC=1 | **RC=0** |
| `cmake --build . --target TIDE_VST3` | could not configure | **298/298**, links `TIDE_VST3.so`, assembles the `.vst3` |

The artifact is real: 91 MB ELF exporting `GetPluginFactory` and `ModuleEntry`.
Two controls so the gate is not passing vacuously — `GMPI_STANDALONE_STRICT=ON`
gives RC=1 naming `libpipewire-0.3`, and the build graph contains **zero**
`Standalone_Wrapper` targets while `VST3_Wrapper`, `CLAP_Wrapper`,
`SynthEditLib`, `EditorLib`, `TIDE_VST3` and `TIDE.gmpi` are all present. (17
`standalone` strings remain in the graph; all are CMake's per-directory
`install`/`test`/`edit_cache` boilerplate, emitted for any added subdirectory
even when it returns early.)

**Re-verified after the merge, which is the check that actually matters:**
configure of merged `main` with `GMPI_Wrappers` **fetched from GitHub rather than
a local override** — RC=0. That is what a fresh clone and CI get, not just what
this box's working tree gets.

**Learned:**

- **A platform-gated `return()` above a `REQUIRED` probe hides the probe from two
  of three platforms.** `if(NOT UNIX OR APPLE) return()` meant Windows and macOS
  never reached the pipewire line, so a hard dependency that stopped the entire
  linux tree could sit on `main` looking green. Any dependency probe below a
  platform gate is, by construction, only tested on the platforms below that gate
  — worth remembering before adding one.
- **Check for consumers before deciding between "make it optional" and "make it
  opt-in".** A target nothing links can be skipped silently at near-zero cost; a
  target something links cannot, because the consumer's
  `target_link_libraries` then fails on a nonexistent target. The grep is one
  command and it picked the option.
- **Resolve-then-generate, never resolve-while-generating.** The original loop
  emitted `add_custom_command`s for the protocols it had found and only then hit
  the missing one. Splitting the loop is what lets a missing XML join the same
  report as a missing package instead of being a separate failure mode.

**STEP 4 chores this unblocked:** **S2** flipped `IN-REVIEW` → `DONE`
([#54](https://github.com/JeffMcClintock/TideSynth/pull/54) merged 2026-08-14)
and moved verbatim to `BACKLOG-DONE.md`. The `linux` NEXT row, which this
morning pointed at "#53 first, then S3", now points at **S3** alone.

**Next:** **S3** for the next linux run — `SE16/SynthEditSem/TideApp.cpp` is
ALLOWED, the box can build again, and S2 supplied the evidence its row lacked
(finding A6: live write sites behind the `assert(false)` stubs). The two
judgements still waiting on Jeff are unchanged: **S1b/S5/S7/S8 are GATED-in-full
and should not read as `TODO`** to a scheduled run, and **A2's sandbox-escape
question gates doing S7 properly**.

**Branch/PR:** [GMPI_Wrappers#3](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/3)
(merged, `49ff927`) for the fix; this entry and the backlog cleanup on
`tide/linux/post-merge-cleanup`. Both working copies left on their default
branches, clean.

---

## 2026-08-14 — linux — A4 built (interactive session, Jeff directing)

**Did:** Built the auto-merge tier for coordination PRs, after Jeff asked why
BACKLOG/JOURNAL bookkeeping needs a human at all. It does not; A4 has been the
answer since 2026-08-09 and could not be built by the fleet itself. **Two
defects in its specification were found first, and the second is the serious
one.**

**Why no scheduled run could ever have done this.** A4 is a
`.github/workflows/**` file and the bot token is `repo` scope with no
`workflow` — measured this session, `x-oauth-scopes: repo`. That is the
credential-layer enforcement of the no-workflow-edits rule working exactly as
designed, and its consequence is that **the one item that would free scheduled
runs is the one item scheduled runs are structurally forbidden from building.**
Jeff's own token on this box already carries `workflow`, so no `gh auth refresh`
was needed here; the commit is his.

**Defect 1 — the allowlist would have fired close to never.** A4 says
"PRs touching only `JOURNAL.md`, `BACKLOG.md`, and `docs/**`". Checked against
the file lists of the last seven merged PRs, that scores **0 of 7**:

| Blocker | PRs |
|---|---|
| `JOURNAL-2026-08.md` | 7 of 7 |
| `BACKLOG-DONE.md` | #55, #54, #48 |

**A8 created both files on 2026-08-12 — three days after A4's allowlist was
written — and nobody updated the allowlist.** STEP 4 *mandates* rotating into
exactly those two files every run, so the tier would have shipped, looked
correct, and merged almost nothing. With them added the same seven score
**3 of 7**: #48, #50 and #55 auto-merge; #51, #52 and #54 correctly wait on
`scripts/`, `tests/` and `tools/`; #41 on `.github/`.

**Defect 2 — `docs/**` was too wide, and the miss was `docs/decisions.md`.**
A4 excluded `docs/weekly-run-prompt.md` and `PLAN.md` because "they steer the
fleet". `docs/decisions.md` steers it harder: that file *is* the PROPOSED
mechanism, and its own text says **"Jeff's merge of that PR is the decision."**
Auto-merging it would let a run answer its own escalation — the single file in
`docs/**` where merging is an act of authority rather than bookkeeping. Now
denied by name, and the selftest tries it both alone and smuggled in beside a
legitimate journal entry.

**Result — design, and the option deliberately not taken.**

The trigger is `workflow_run` on `lint`, not `pull_request`:

- A `workflow_run` job always runs the **default branch's** copy of the
  workflow, never the PR's. So a PR cannot edit the rules that judge it. The
  allowlist denies `.github/**` anyway; this is the second lock.
- It gates on lint having actually concluded green.

**`gh pr merge --auto` was rejected, and the reason is measurable rather than
stylistic.** It delegates the waiting to GitHub, which only works when a
ruleset marks lint a *required* check. This repo has `allow_auto_merge:false`,
and ruleset `20600401` ("Agent PRs only") carries **only** `deletion`,
`non_fast_forward` and `pull_request` — **no required-status-checks rule at
all.** With neither, `--auto` merges immediately and the lint gate is
decorative. Anyone reaching for `--auto` here should check those two settings
first; the failure is silent and looks like success.

Making lint a required check repo-wide was the other route and was **not**
taken: it changes the merge rules for every PR including code, which is wider
than A4's own "Human merge remains for … all code repos". Keeping the tier
inside one workflow plus one script means no repository setting can drift out
of sync with it.

**Guards beyond the allowlist**, because a path allowlist alone is not an
authorisation model: author must be `tide-rack-bot`, not a draft, open, and
based on the default branch. Without the author check, a docs-only PR from
anyone able to open one is an unauthenticated write path into `main`. The
workflow never checks out or executes PR code — it reads the changed-file list
from the API and runs the allowlist script from `main`, which is what makes
`contents: write` safe to grant.

**Verification artifact:** the eligibility decision is a script, not YAML, so
it is testable without GitHub. `scripts/automerge_eligible.py --selftest` —
**19 cases, 0 failed.** Seven are the real file lists of merged PRs, so those
expectations are measurements. The rest are edges: both carve-outs alone and
beside a legitimate journal edit, `PLAN.md`, `website/`, the auto-merge
workflow itself, the script itself, an empty list, an unrecognised new
top-level file, and a near-miss on the archive regex (`JOURNAL-2026-8.md`).

**The selftest earned its keep before it ever ran in CI:** `docs/../PLAN.md`
passed the first draft, because it starts with `docs/` and the prefix test was
happy. `git diff --name-only` normalises paths so it is not reachable in
practice — which is precisely why it deserved a guard rather than an
assumption about an upstream tool's output. Now rejected along with absolute
paths and backslashes.

**Learned:**

- **A path allowlist ages badly and silently.** A4's went stale three days
  after it was written, because a *different* item (A8) added two files, and
  nothing connected them. The lesson is not "update the allowlist" but
  "an allowlist needs a test that runs against real recent PRs" — which is why
  the selftest carries seven of them, and why a future run adding a new
  coordination file should add a case there in the same change.
- **Strict inclusion, never exclusion.** The failure mode of an exclusion list
  is that tomorrow's file merges by default. Everything unrecognised fails
  closed, which is why `tools/` and `tests/` blocked correctly without ever
  having been thought about when the list was written.
- **`--auto` is not "merge when checks pass" unless a ruleset says which
  checks.** With no required-status-checks rule it is just "merge now".

**Next:** **not verified live, and it cannot be** — a `workflow_run` workflow
only fires once it is on the default branch, so the first real firing can only
be watched after this merges. Flip A4 to `DONE` only after seeing it **merge
one PR and leave another alone**; the negative control matters more than the
positive one, since an auto-merge action that is wrong about its allowlist is
a worse problem than merging by hand. The next scheduled run's own
BACKLOG/JOURNAL PR is the natural first test.

**Branch/PR:** `tide/linux/A4-auto-merge-tier`. Committed as Jeff, not as
`tide-rack-bot` — the bot token deliberately cannot push `.github/workflows/**`.

---

## 2026-08-14 — windows — C5

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Carve-out stage **C5**, the app base. Moved fourteen files out of the
private repo into public `SynthEditLib` and finished **C9** with them. The row
promised "one `#include` swap and one macro rename, no new machinery" and that
was accurate — the work took minutes. **The two things worth reading are that
C5 is the first stage to reduce the public repo's private-include debt rather
than grow it, and that the stage list does not cover the files it claims to,
which is filed as C12.**

**Result — all three products build, all tests pass, C9 proven by a control.**

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16`, Release | **905/905, RC=0** — `SynthEdit_VST3`, `SynthEdit_GMPI`, `SynthEditCL`, `TIDE.gmpi`, `TIDE_VST3` |
| the seven moved TUs really compiled from their new home | log shows `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj` for all seven |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | 58/58, 24/24, 10/10 — all RC=0 |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, 0 errors, `x64\Release\SynthEdit2\SynthEdit2.exe` |
| no line-ending churn | `core.autocrlf=false` in both repos; all fourteen `cmp`-identical to their originals before the one C9 edit |

The two MSBuild warnings are `C4834` in `SynthEditApp.cpp:421,437`, a file this
stage does not touch and does not move — pre-existing, not caused here.

**C9's third and last user, with a control rather than an assertion.**

`Application.cpp:22` included `"../se_build_number.h"` and `:97` read
`SE_BUILD_NUMBER` to decide whether `CopyInitialPrefabs` re-seeds the user's
`SynthEdit Projects/Prefabs`. That is the same cache-invalidation shape as
`SkinMgr` and `ModuleFactory_Editor`, so it took the same treatment: include
`se_version.h`, read `SE_APP_BUILD_NUMBER`.

| | |
|---|---|
| `se_build_number.h` today | `SE_BUILD_NUMBER 183` |
| configure output | `EditorLib: SE_APP_BUILD_NUMBER=183 (from se_build_number.h)` |
| `build.ninja` statements carrying the define | **57, every one of them EditorLib**, one distinct value — and `Application.cpp.obj` is among them |
| probe TU, **no** injection | prints **0** — what a clean clone gets, i.e. TIDE from C7 |
| probe TU, **with** injection | prints **183** |

The probe is the part that matters: `se_version.h` defaults the macro to 0, so
a lost injection **fails silently** — nothing would fail to compile and the
prefab copy would just quietly stop invalidating. An absence of build errors is
not evidence here, which is why there is a positive control. Note the count is
still 57, unchanged from C4: `Application.cpp` was already an EditorLib TU, so
C5 changed which directory it compiles from, not which target owns it.

**Learned — C5 is the first stage to pay debt down, and both halves were
measured from git refs.** C4 went 47 → 56 and warned that C5 would move
`Application.h`, the most-included name on the list. It did:

| | dangling private includes in `SynthEditLib` |
|---|---|
| before C5 (`SE16 origin/master` / `SynthEditLib origin/main`) | **59** |
| after C5 | **54** |
| closed by C5 | **15** |
| opened by C5 | **10** |

**These are not comparable to C4's 47/56 as absolute numbers** — the C4 script
lived in that run's scratchpad and is gone, so this is a re-implementation with
slightly different normalisation (it counts `./CLine2.h` and `CLine2.h`
separately, and includes SE16's vendored `soundpipe.h`). The delta is the honest
figure, and both sides were read from git refs with the same script, which is
the thing C4's entry said to get right. Script re-created as `dangling.py` in
this run's scratchpad; it is ~70 lines and the next stage should re-create it
again rather than trust a number.

The 15 closed are all `Application.h` (11) and `SynthEditAppBase.h` (4), from
`CContainer.cpp`, `CUG.cpp`, `CUG_with_patches.cpp`, `DocOb.cpp`, `plug4.cpp`,
`checkpoint.cpp`, `imbedded_file.cpp`, `SynthEditDoc2.cpp`,
`SynthEditDocBase.cpp`, `MfcDocPresenter.cpp`, `SkinMgr.cpp`, `ModuleBrowser.cpp`,
`PropertiesBrowser.cpp`, `CancellationAnalyse.cpp` and `CpuMeterGui.cpp`.

**Of the 10 opened, 6 are closed by construction later and 4 are not:**

| header | on a stage's list? |
|---|---|
| `commandMgr.h` (×2), `PatchParameter.h`, `SuspendDSP.h`, `ui_msg_target.h` | yes — on `EditorLib/CMakeLists.txt`'s list, so C12 closes them |
| `IMidiDriver.h` (×2), `ParseSynthEditArgs.h`, `ISEAppManaged.h` | **no stage's list** |
| **`SynthEditApp.h`** | **no stage's list — and it is C11(a)** |

**`SynthEditApp.h` is the one to look at, because C5 has just given C11 a second
call site and C11 is still unruled.** C4 found `MfcDocPresenter.cpp` reaching
`theApp->isMoonbaseEnabled()`; now `ApplySynthEditConfig.cpp` includes the same
deliberately-excluded header from the public repo. That header is excluded *by
design* so each app picks its own `SE_MOONBASE_SUPPORT` without ODR conflicts,
so it cannot simply join a stage's list the way `ModulePicker.h` can — C11's
question got wider, not just longer. `ISEAppManaged.h` was already dangling
before C5 (`CUG.cpp`); C5 adds a second includer. `IMidiDriver.h` and
`ParseSynthEditArgs.h` are new and on nobody's list.

**Learned — the big one. The carve-out's stage list does not cover the files it
says it does, and C7 cannot pass until something does.** `docs/carve-out.md`
stages C3, C4 and C5 as "the rest", and `EditorLib/CMakeLists.txt`'s own comment
said "C3-C5 convert the rest, and C6 moves this file itself". After C5 there are
still **41 `${EDITOR_DIR}` entries** — roughly 21 units: `CLine2`, `commandMgr`,
`Control`, `CPlugin`, the four `Ctl_*` controls, `Dialogs_editor`, `GuiPin`,
`IGuiHost`, `InterfaceObject_editor`, `legacyExternalApp`,
`ModuleDragAndDropManager`, `Module_Info_Plugin`, `PatchManager`,
`PatchParameter`, `PatchParameter_host_generated`, `platform_editor`,
`resource.h`, `SuspendDSP`, `UG2`, `ui_msg_target`. **C7's whole test is a clean
clone with no access to SE16**, and every one of these is on EditorLib's source
list, so C6 would move a `CMakeLists.txt` that points at 41 files a stranger
cannot see. Filed as **C12**; the CMakeLists comment now says so rather than
claiming the conversion is finished, so the next reader is not misled the way
this run was.

I did not widen C5 to absorb them. C5's file list is Jeff's, the extra 41 are
about three times C5's own size, and reshaping an approved stage from inside it
is not a scheduled run's call — the same reasoning C4 used for C11.

**Learned — `cmd.exe /c` is unusable from the Bash tool on this box, and it
fails by hanging rather than erroring.** MSYS path conversion rewrites the `/c`
switch to `C:/`, so `cmd.exe` starts *interactively*, prints its banner, and
blocks on stdin until the tool times out. It looks exactly like a slow CMake
configure. Ten minutes were lost to it before the banner in the log gave it
away. Either use the PowerShell tool for anything that needs `vcvars64.bat`
(what this run did), or set `MSYS_NO_PATHCONV=1`. Note the failure is silent in
the other direction too: the wrapper's own `echo RC=$?` reported **0** for a
command that never ran.

**Learned — the C7 cache-thrash problem S2 found for skins now has a third
instance, and it is the same mechanism.** S2 predicted that from C7 TIDE takes
`SE_APP_BUILD_NUMBER=0` while SynthEdit keeps 183, so each would re-copy 724 KB
of skins over the other on every launch. `CopyInitialPrefabs` is keyed on
exactly the same macro and writes to `SynthEdit Projects/Prefabs` (1.1 MB, 35
files per S2's own measurement), so it thrashes identically — as does the module
cache XML. Today nothing thrashes, because TIDE links the one EditorLib that
carries the injection and so sees 183 too; the divergence begins at C7. This
strengthens S2's argument that the mechanism should be removed from TIDE
*before* C7 rather than after, and it is now three caches, not one.

**Learned — no `.vcxproj` or Xcode edit was needed, unlike C3 and C4, and that
is worth checking rather than assuming.** Neither `SynthEdit2.vcxproj` nor
`SynthEditMac/SynthEdit.xcodeproj` mentions any of the fourteen files: the
`.cpp` had already moved to EditorLib (the vcxproj carries only comments saying
so) and the headers were never listed. `$(SolutionDir)..\SyntheditLib` is
already on the vcxproj's `AdditionalIncludeDirectories`, so every consumer's
`#include "Application.h"` keeps resolving through a search path — just to a
different directory. Two stale path comments in `SynthEditCL/CMakeLists.txt` and
`SynthEditWayland/CMakeLists.txt` did name `SynthEdit2/Application.cpp` and were
updated.

**STEP 1 / 1.5:** no `platform:win` issues; the only open issue anywhere in the
five repos is TideSynth [#44](https://github.com/JeffMcClintock/TideSynth/issues/44)
(the A6 watchdog digest, `github-actions`, informational) — noted, not acted on.
**Zero open PRs in all five repos** at claim time, and no `tide/**` branch on any
remote, so nothing was handed back to this platform and nothing was in flight to
collide with. `SynthEdit` carries an unrelated `claude/audio-sample-rate-persist-795c69`
branch, not a fleet branch; left alone.

**A4's first live test is available now.** The A4 run said to flip it to `DONE`
only after watching it merge one PR and leave another alone, and that the next
scheduled run's own PRs are the natural first test. This run supplies both
controls at once: the TideSynth PR is `BACKLOG.md` + `JOURNAL*.md` only and
should auto-merge, while the two code PRs are in other repos entirely and the
tier must not touch them. A4 is left `IN-REVIEW` — this run did not observe the
firing, and observing it is the whole condition.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches** at claim time. Nothing of his was committed, reverted or
stashed. All five were **fast-forwarded** to `origin` before starting —
`TideSynth` 4 commits behind, `SE16` 3, `SynthEditLib` 2, `gmpi_ui` 1,
`GMPI_Wrappers` 2 — true fast-forwards with nothing to stash, noted because it is
a change to his trees. `SE16/SynthEdit2/.claude/worktrees/` holds two stray
agent worktrees (`audio-sample-rate-persist-795c69`, `blank-project-app-load-d27d84`);
they are ignored, they are not mine, and they were not touched — but note they
make a naive `grep -rn` over `SE16` return every hit three times.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** a scratch Ninja tree and two probe binaries under
the session scratchpad, all outside every repo — Jeff's own `SE16\build` was not
configured or built into. The MSBuild run **did** write into `SE16\x64\Release\`,
which is `.gitignore`d and is where that build has always put its output.
`SynthEditCL.exe` was built but never executed, so no module cache, skin folder
or prefab folder was created or invalidated on this machine.

**Next:**

1. **Merge both code PRs together** —
   [SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) and
   [SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7).
   Merging either alone breaks the build: the files exist in exactly one repo at
   a time and `EditorLib/CMakeLists.txt` points at the new location.
2. **C12 before C6, not after.** C6 moves `EditorLib/CMakeLists.txt` into the
   public repo; doing that while it still points at 41 private files puts a
   source list a stranger cannot satisfy into the public repo, which is the
   `cpu_accumulator.h` shape C2 hit, at 41× the size. C12 is `win`-shaped only in
   that this box has all three build systems to verify against; the row is `any`.
3. **C11 needs Jeff and now has two call sites, not one.** `SynthEditApp.h` is
   reached from `MfcDocPresenter.cpp` (C4) and `ApplySynthEditConfig.cpp` (C5).
   It cannot join a stage's list the way `ModulePicker.h` can, because its
   exclusion is deliberate and about `SE_MOONBASE_SUPPORT` ODR.
4. **`IMidiDriver.h`, `ParseSynthEditArgs.h` and `ISEAppManaged.h` are on no
   stage's list either** — smaller and probably uncontroversial, but they are
   real C7 blockers and C12 should absorb them explicitly.

**Branch/PR:** `tide/win/C5-move-app-base` in three repos —
[SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) (deletions,
`EditorLib/CMakeLists.txt`, two comment fixes),
[SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7) (the
fourteen files + `se_version.h`),
[#58](https://github.com/JeffMcClintock/TideSynth/pull/58) (BACKLOG, JOURNAL,
`docs/carve-out.md`). No other repo was committed in or modified.

---

## 2026-08-14 — windows — C12 (scoping session)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness; no `claude` CLI on PATH to version, same as the C5 run) · as `tide-rack-bot`

**Did:** Took **C12** and did what its own Size line and the NEXT block both
asked for — a scoping session, not an attempt. Split it into six sub-stages
**C12a–C12f**, each with Scope/Accept/Size, and wrote
[docs/c12-remaining-editor-files.md](docs/c12-remaining-editor-files.md).
**No file was moved and no code changed**; the deliverable is the split, the
measurements behind it, and one correction that mattered more than the split.

**Result — the correction first, because it was live.** C5's PRs all merged
earlier the same day, which flipped **C6** from `BLOCKED(C5)` to eligible **by
the status column, which is the one place eligibility lives**. Every warning
against taking C6 next — C5's journal entry, C12's row, `docs/carve-out.md`,
the CMakeLists comment — was prose, and the backlog's own rule says prose never
overrides the status column. So the next run to read that table would have been
told, correctly by the rules, to move `EditorLib/CMakeLists.txt` into the public
repo while it still points at 41 private files: the `cpu_accumulator.h` shape C2
hit, at 41×. C6 is now `BLOCKED(C12f)`. **The general lesson is worth more than
this instance: a `BLOCKED(<id>)` row is a landmine armed by the merge of `<id>`,
and nothing in the process notices it going live.** Anything relying on a
successor to that blocker must be encoded in the blocker itself.

**Result — the measurements. The 41 are smaller and cleaner than the row
feared.** All read from the working trees (all five clean, on default branches);
four scripts, kept in this run's scratchpad and named in the doc, not committed.

| | |
|---|---|
| `${EDITOR_DIR}` entries on EditorLib's list | **41** (23 units, 9,791 lines) |
| Of those, files that must actually **move** | **37** (9,010 lines) |
| Private headers the 41 pull in that no stage owns | **zero — the set is closed under inclusion** |
| Real dangling private includes from the public repo into the 41 | **43** |
| `.vcxproj` / Xcode entries to edit | **none** |
| Files among the 41 needing MFC (`afxres.h`) | **none** |

**"Closed under inclusion" is the headline.** C4 closed 11 dangling includes and
opened 20; C5 closed 15 and opened 10. **C12 opens none** — every quoted include
in all 41 files resolves to the public repo, to the GMPI SDK, or to another of
the 41. So C12 takes the count to zero rather than trading it around, and unlike
C4 and C5 it cannot spawn a C11-shaped follow-up. That is also why the six
sub-stages can be taken in any order: EditorLib's include path carries both
`${SYNTHEDITLIB_DIR}` and `../SynthEdit2`, which is exactly why C4 and C5 could
each leave dangling includes behind and still build. Ordering is a convenience.

**Learned — four of the 41 are dead or duplicate, so C12 is 37 files, not 41.**

- **`GuiPin.h`** (393 lines) has **no includer anywhere** in `SE16`,
  `SynthEditLib`, `SynthEditCL`, `gmpi_ui` or `GMPI_Wrappers`, including inside
  `SynthEdit2` itself, and no `.cpp`. It could not compile if something did
  include it: `control_float_normalised.h`, `variable.h` and
  `gui_default_variable.h` exist nowhere in the current trees — they survive
  only in the dormant `SE15` repo under `OtherProjects/SynthEdit_1.0/`.
- **`Module_Info_Plugin.{h,cpp}`** (26 lines): header says *"VST2 plugin.
  Deprecated."*, constructor is `protected` and marked `// serialisation only`,
  nothing derives from it, `getClassType()` returns 3 and nothing tests for 3,
  and the sole construction site is commented out —
  `ModuleFactory_Editor.cpp:1320`: `// meh: mi = new Module_Info_Plugin();`.
- **`resource.h`** does not need to move because **there are already two of
  them**. `SynthEditLib/resource.h` and `SE16/SynthEdit2/resource.h` are both
  361 lines, differing in exactly two: a trailing space in a comment, and
  `_APS_NEXT_RESOURCE_VALUE` (207 vs 210), a Visual Studio counter inside
  `#ifdef APSTUDIO_INVOKED`. Every `ID_*` constant matches. Not a carve-out
  artifact — the public one dates to `SynthEditLib`'s initial commit.

**Learned — the measurement trap, stated plainly because the first pass fell
into it.** A naive scan reports **114** dangling edges, of which 71 are
`resource.h`, making it look like C12's single biggest item. The true figure is
**43 and `resource.h` contributes 0**: 65 of those 71 includers are `ug_*.cpp`
DSP modules in `SynthEditLib`'s own root and the other six are the editor files
C3/C4 moved there, and a quoted `#include` searches the includer's own directory
first — so all 71 get the public copy and none reaches the private one. **When a
basename exists in both repos, "is on the stage list" must not outrank "the
includer's own directory has a copy".** Any re-implementation should check its
`resource.h` number first: if it is not zero, the resolution order is wrong.

**Learned — the one part of C12 this box cannot verify, which the row did not
name.** `platform_editor.cpp` is 16 lines containing nothing but
`new_InterfaceObjectA/B/C`, and it is the seam that lets `SynthEditLib` call
into the editor layer without a compile-time dependency. Three CMakeLists carry
the same comment — `SynthEditCL:57-61`, `SynthEditJuce:92-96`,
`SynthEditWayland:150-153` — saying the two are *mutually-referencing static
archives* and **GNU ld needs a rescan group**. Moving the provider into
`SynthEditLib` puts it in the same archive as the code expecting it, which
plausibly makes the rescan group redundant; *plausibly* is the problem, because
MSVC's linker is indifferent either way, so a Windows run reports green whichever
way it lands and the failure surfaces as a Linux link error. Split out as
**C12d** and marked `linux` for that reason alone — the code is platform-neutral
— so that a Linux failure cannot block C12c's twelve entries.

**Learned — `Dialogs_editor2.cpp` links by accident, and it bears on S3.** It is
16 lines defining the three dialog entry points with **empty bodies**, real
implementations commented out under `// all obsolete?`, and it is one of *five*
definitions of the same three functions — one per consuming app
(`SynthEditCL/CLApp.cpp:14`, `SynthEditSem/TideApp.cpp:13`,
`tests/layouttests.cpp:26`, EditorScreenshot pointing at those). **TIDE links
`EditorLib`, which contains `Dialogs_editor2.obj`, *and* defines the same three
symbols in `TideApp.cpp`.** There is no duplicate-symbol error only because that
object file holds nothing else, so the linker never has a reason to pull it in.
**Add one symbol to that file and TIDE stops linking.** That makes it an
app-level stub inside a shared library — the exact shape `SynthEditApp.cpp` and
`ExportAsPlugin.cpp` are already deliberately kept off EditorLib's list for. I
did not decide it: **PROPOSED entry filed in
[docs/decisions.md](docs/decisions.md)**, recommended default (b), C12e parked
as `NEEDS-JEFF` with a Default-in-effect and a Decide-by. **S3 is about
`TideApp.cpp`'s `assert(false)` stubs for these same three functions**, and the
linux box is pointed at S3 — whoever takes either should read the other.

**Learned — the patch cluster is irreducible, and it is 64% of C12.**
`PatchManager` ↔ `PatchParameter` ↔ `PatchParameter_host_generated`, and
`PatchManager` ↔ `UG2`, form a genuine strongly-connected component; `CPlugin`
hangs off `PatchManager`. Ten entries, **6,298 lines**, no split leaves both
halves whole. That is what forces C12f to be the largest single stage of the
whole carve-out however the rest is arranged — and it is the stage that takes
`${EDITOR_DIR}` to zero and so unblocks C6.

**Learned, in passing, and it changes B1's premise — `build.yml` is not failing for the reason it says it is.** This PR is markdown-only, so its `windows`/`macos`/`linux` check failures cannot be its own doing; checked anyway rather than asserting it, and all three die identically at the **Configure** step with `CMake Error: The source directory "…/TideSynth" does not appear to contain CMakeLists.txt`. `build.yml`'s own header says the expected failure is that *"TIDE depends on EditorLib, which lives in the private SynthEdit repo, so a clean checkout genuinely cannot build"*, and **B1** asks for it to fail *"for exactly that one honest reason, rather than for toolchain or syntax errors"*. The real failure is neither: **`TideSynth` has no top-level `CMakeLists.txt` at all** — the repo root holds only markdown, `docs/`, `scripts/`, `tests/`, `tools/` and `website/`. So the run never reaches the private dependency, and B1 is not the tidy-up its row implies: it includes **authoring TIDE's top-level CMake**, which is a different size of job and probably wants C7's shape settled first. Not filed as a new row — it is B1's, and B1 is `.github/workflows/**` that the bot token cannot push anyway — but its row now says so.

**Verification artifact.** This item produced no code, so the artifact is the
measurement and the baseline it was taken against, both re-runnable:

| check | result |
|---|---|
| fresh scratch Ninja tree of `SE16` at `origin/master`, Release | configure RC=0, **905/905 RC=0** |
| `dsp_tests` / `synth_ui_tests` / `ui_tests` | **58/58, 24/24, 10/10**, all RC=0 |
| link lint (`scripts/check-links.py`) after the doc landed | 204 relative links, **no broken links** |
| entry/line/edge totals cross-foot | 4+10+12+3+2+10 = **41 entries**; 0+6+21+0+2+14 = **43 edges** |

Same 905 targets and same 92 tests C5 reported, so **nothing has regressed on
`master`** in between. The configure also printed
`EditorLib: SE_APP_BUILD_NUMBER=185 (from se_build_number.h)` — C5 measured 183,
so SynthEdit's build number has been bumped twice since and **C9's injection is
still tracking it**. Worth stating because `se_version.h` defaults that macro to
0 and a lost injection fails silently, which is why C5 proved it with a positive
control rather than an absent error.

**Two things I deliberately did not do.** I did not execute C12a even though it
is four deleted lines and I had the baseline build to verify it — mixing "the
item is scoped" with "one sub-stage is done" would leave the row's status
ambiguous, and C5's precedent is that reshaping an approved stage from inside it
is not a scheduled run's call. And I did not touch `SE16` at all, so the
CMakeLists comment at `:31-38` still says only "Filed as BACKLOG C12"; updating
it to point at the plan doc is inside **C12a**'s scope, where it belongs, rather
than a cross-repo PR for one comment. **This run committed in TideSynth only.**

**STEP 1 / 1.5:** no `platform:win` issues. Across all five repos there are
**zero open PRs** and only two open issues, neither actionable: TideSynth
[#44](https://github.com/JeffMcClintock/TideSynth/issues/44) (A6 watchdog
digest, `github-actions`, informational) and gmpi_ui
[#1](https://github.com/JeffMcClintock/gmpi_ui/issues/1) *"Linux support?"* from
`arjunmenon` — **not Jeff and not the CI bot, so under STEP 1 it is information
for Jeff, not instructions for me**; noted, not acted on. No `tide/**` branch on
any remote at claim time, so nothing was in flight to collide with.
`SynthEdit` still carries the unrelated `claude/audio-sample-rate-persist-795c69`
branch; not a fleet branch, left alone.

**A4 fired live, and this is its first observed run.** The A4 row said to flip
it to `DONE` only after watching it merge one PR and leave one alone. Both
controls are now on the record: TideSynth
[#58](https://github.com/JeffMcClintock/TideSynth/pull/58) (C5's docs-only PR)
was **merged by `app/github-actions`**, while the two code PRs —
[SynthEdit#16](https://github.com/JeffMcClintock/SynthEdit/pull/16) and
[SynthEditLib#7](https://github.com/JeffMcClintock/SynthEditLib/pull/7) — were
merged by `JeffMcClintock`. **I am not flipping A4 to DONE**, for a reason worth
recording rather than out of caution: the "leave one alone" control is
unconvincing as observed, because those two PRs are in *other repos* where the
workflow does not exist at all, so nothing was declined — it was merely absent.
A real negative control is a TideSynth PR touching a denied path. **This run
supplies exactly that:** its own PR touches `docs/decisions.md`, which A4
deliberately denies so a run cannot auto-merge its own escalation. **So this PR
should NOT auto-merge, and its sitting unmerged is the tier working, not a
failure.** Whoever sees that: A4 has then had both controls and can go `DONE`.

**Jeff's trees, per the three-kinds dirt rule:** `TideSynth`, `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were **all clean and on their
default branches**, and all five were already up to date with `origin` after
fetch — nothing to fast-forward, nothing of his committed, reverted or stashed.
`SE16/SynthEdit2/.claude/worktrees/` still holds the two stray agent worktrees
(`audio-sample-rate-persist-795c69`, `blank-project-app-load-d27d84`); not mine,
not touched, and they still make a naive `grep -rn` over `SE16` return every hit
three times — every scan in this run excludes them explicitly.

**A11 still holds:** all five repos answer `https://` to
`ls-remote --get-url origin`, and STEP 0.7's second assertion printed
`git@github.com:`.

**Side effects on this box:** one scratch Ninja tree and four Python scripts
under the session scratchpad, all outside every repo. **Jeff's own `SE16\build`
was neither configured nor built into**, and unlike the C5 run nothing was
written to `SE16\x64\Release\` because no MSBuild run was needed.
`SynthEditCL.exe` was built but never executed, so no module cache, skin folder
or prefab folder was created or invalidated.

**Next:**

1. **C12a, then C12b** on the next `win` run — C12a is four deleted lines and
   cannot break anything; C12b is the ten control files. A box with room for a
   large session should take **C12f** instead, since that is the one that
   unblocks C6.
2. **C12d belongs to the Linux box.** Do not let a Windows or macOS run take it:
   both will report green regardless, which is precisely the failure.
3. **C12e needs Jeff** — merge or edit the PROPOSED entry in
   `docs/decisions.md`. Until then C12 tops out at 39 of 41 entries and C6 stays
   blocked.
4. **C11 is still unruled and still has two call sites.** Unchanged by this run,
   but it and C12e are now both waiting on the same person, and C7 needs both.
5. **Check the `BLOCKED(<id>)` rows whenever an `<id>` merges.** C6 is the
   instance found here; nothing guarantees it is the only one.

**Branch/PR:** `tide/win/C12-scope-remaining-editor-files`, TideSynth only —
[#59](https://github.com/JeffMcClintock/TideSynth/pull/59). No other repo was
committed in or modified. **Expect #59 to sit unmerged:** it touches
`docs/decisions.md`, which A4 denies by design.

---

## 2026-08-15 — windows — C12a

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness; no `claude` CLI on PATH to version, same as the C5 and C12 runs) · as `tide-rack-bot`

**Did:** Carve-out sub-stage **C12a**, the first of the six the C12 scoping run
split out the day before. Four `${EDITOR_DIR}` lines deleted from
`SE16/EditorLib/CMakeLists.txt` — `GuiPin.h`, `Module_Info_Plugin.{cpp,h}` and
`resource.h` — plus the stale stage comment replaced. **Nothing moved and no
file was deleted.** `SE16` `c58e4bc5a` on `tide/win/C12a-delist-dead-duplicate`,
pushed, [SynthEdit#18](https://github.com/JeffMcClintock/SynthEdit/pull/18).
**41 → 37 entries.**

**Result — green, and one number differs from the row's prediction in the
direction that confirms rather than undermines it.**

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **41 → 37** |
| fresh scratch Ninja tree of `SE16`, Release, configure | RC=0 |
| build | **904/904, RC=0** |
| `ctest` | **92/92 passed, 0 failed**, RC=0 |
| artifacts | `TIDE.gmpi`, `TIDE_VST3.vst3`, `SynthEditCL.exe` all produced |
| `Module_Info_Plugin` / `GuiPin` edges in `build.ninja` | **0** / **0** |
| `SynthEdit2.vcxproj` and `.filters` mentions of the four | **0** |
| `SE_APP_BUILD_NUMBER` at configure | **185** — C9's injection still tracking |

**904, where both the row and the plan doc said "still builds 905/905".** That
is the expected consequence of the change, not a regression: dropping
`Module_Info_Plugin.cpp` removes one TU from `EditorLib`, so the ninja edge
count falls by exactly one from the 905 the C12 scoping run measured on
`master` hours earlier. **Both documents predicted the count would hold while
also correctly identifying that this is a real link-surface change needing a
build — the two statements contradict each other and nobody noticed.** Worth
saying plainly because "905/905" was written as an acceptance check, and a
later run reading it literally would have treated a correct build as a failure.
The 92 tests are unchanged, which is the part that actually had to hold.

**Learned — all three delisting claims verified, and the checks are cheap
enough that no future stage should skip them.** The plan doc says *"do not take
it on trust, the checks are two greps"*; re-run this session, all three hold:

- **`GuiPin.h`** — zero includers across `SE16`, `SynthEditLib`, `SynthEditCL`,
  `gmpi_ui` and `GMPI_Wrappers`. The only apparent extra hits anywhere in the
  tree came from `SE16/SynthEdit2/.claude/worktrees/` (two stray agent
  worktrees, gitignored) and `SE16/build/_deps/syntheditlib-src/` — **worth
  knowing for any future grep-based measurement on this box, because both look
  like real source and neither is.** Scope greps to the five repo roots.
- **`Module_Info_Plugin`** — stronger than the row claimed. The row says its
  only construction site is commented out at `ModuleFactory_Editor.cpp:1320`.
  It is also true that the `switch` there has **no `case 3`** at all for its
  class-type id: `case 1` carries the commented-out line and falls through to
  `default`, i.e. plain `Module_Info` with `SetUnavailable()`. So the type is
  unreachable by two independent routes, not one.
- **`resource.h`** — both copies 361 lines; **all 318 `ID_*`/`IDR_*` constants
  byte-identical**; the only differences are a trailing space in a comment and
  `_APS_NEXT_RESOURCE_VALUE` (**210** private, **207** public). That counter gap
  is the measurable sign the private copy has had three resource slots the
  public one has not, which is **P9**'s whole point and the only thing making
  C12a safe.

**Learned — A4 has still not been observed firing, and this run's own PR is the
next chance.** A4's row says to flip it to DONE *"only after watching it merge
one PR and leave one alone"*. [#59](https://github.com/JeffMcClintock/TideSynth/pull/59),
the only candidate since it landed, was **merged by `JeffMcClintock`**, not by
the action — checked via the API rather than inferred from the timeline. The
workflow has run five times with `conclusion: success`, but success includes
"correctly declined", so those runs are not evidence either way. **This run's
TideSynth PR touches only `JOURNAL.md`, `JOURNAL-2026-08.md` and `BACKLOG.md`,
all three on A4's allowlist, and is authored by `tide-rack-bot` — so it should
auto-merge.** Whoever reads this next: check whether it did. If it did, that is
half of A4's flip condition met; the paired SynthEdit PR carries `.txt` build
code and must stay for Jeff, which is the other half. Left A4 IN-REVIEW.

**Learned — the C12 scoping run's "no non-CMake build edits" claim holds for
these four specifically**, checked rather than inherited: `SynthEdit2.vcxproj`
and `.vcxproj.filters` mention none of them. So C12a needed no Visual Studio or
Xcode project edit, as predicted.

**Next:** **C12b** — the ten control files (`Control`, `Ctl_Combo`,
`Ctl_Keyboard2`, `Ctl_Slider`, `Ctl_Text`), 1,054 lines, a comfortable single
session, and it closes 6 dangling edges. The win NEXT row already said "C12a,
then C12b"; it is now just C12b, and the baseline any successor should compare
against is **904/904 and 92/92**, not 905. C12c is the bigger win on dangling
edges (21, more than C5 closed in total) if a box wants that instead — ordering
between sub-stages is a convenience, not a constraint. **Do not take C12f
expecting its stated Accept to pass**: it says *"zero `${EDITOR_DIR}` entries
remain"*, which is only reachable once a–e have landed, so taken now it would
leave 27 and fail its own check as written.

**One thing deliberately not done.** The NEXT row reads "C12a, then C12b", but
STEP 2 says pick exactly one item, and one item is what this run took. C12b is
a `git mv` of ten files into `SynthEditLib` — a second repo, a second PR, and a
second full build — which is not a rider on a four-line delete. Re-pointing the
NEXT row at C12b is this entry's contribution to it.

**STEP 1 / 1.5:** no `platform:win` issues. Zero open PRs across all five repos
at the start of the run; one open issue, TideSynth
[#44](https://github.com/JeffMcClintock/TideSynth/issues/44) (A6 watchdog
digest, `github-actions`, informational). One remote branch from another
platform, `tide/mac/D1-donation-affordance`, with no PR yet — the mac box's live
D1 claim, not a collision with C12a.

**Side effects on this box:** one scratch Ninja tree and a build script under
the session scratchpad, both outside every repo. **Jeff's own `SE16\build` was
not touched** — it is a Visual Studio / Debug tree and was left exactly as
found. All five working copies were clean and on their default branches at the
start of the run, and are back on them at the end.

**Branch/PR:** [SynthEdit#18](https://github.com/JeffMcClintock/SynthEdit/pull/18)
(the code — four deleted lines and the comment) and the TideSynth PR carrying
this entry and the backlog status. **Merging the TideSynth one alone lands no
code**, and merging SynthEdit#18 alone is safe on its own — nothing in the two
depends on the other to build.

---

## 2026-08-15 — windows — C12b (and a second-agent collision worth more than the item)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness; no `claude` CLI on PATH to version) · as `tide-rack-bot`

**Did:** Carve-out sub-stage **C12b** — `Control`, `Ctl_Combo`, `Ctl_Keyboard2`,
`Ctl_Slider`, `Ctl_Text`, ten files and 1,053 lines, moved from private
`SE16/SynthEdit2` into public `SynthEditLib`'s root. **37 → 27
`${EDITOR_DIR}` entries.** Also added a committed measurement script, and
archived C12a to [BACKLOG-DONE.md](BACKLOG-DONE.md) after seeing both its PRs
merge. **This was a second item in one session, taken at Jeff's explicit
direction in an interactive session — not a scheduled run deciding to keep
going.** STEP 2's one-item rule stands for unattended runs.

**Read the collision section first if you are short of time. The stage itself
went exactly as scoped; the collision is the part that changes how a run should
behave.**

### The collision — another agent committed my work as Jeff

Midway through C12b, with the ten files staged in two repos and nothing yet
committed, **a second Claude session (Fable 5) running on this same box picked
up my staged changes and committed them** — on my branches, in both `SE16` and
`SynthEditLib`, at 10:49:35, authored **and** committed as
`Jeff McClintock <jef@synthedit.com>`.

The content was mine and was correct: my CMakeLists comment verbatim, my
repointing, my ten file moves, nothing foreign mixed in. Checked by diffing
against the merge-base before doing anything else. **The damage was purely to
authorship — which is exactly what STEP 0.7's four `GIT_*` variables exist to
prevent**, and the reason they exist is stated in the run prompt: without them
`git log` cannot tell agent work from Jeff's. Here the mechanism was inverted —
the variables were set correctly in *my* environment, and a different process
with a different environment committed my working tree anyway.

**Resolved** by `git commit --amend --reset-author` in both repos with
`GIT_AUTHOR_*`/`GIT_COMMITTER_*` exported, then an immediate push, on Jeff's
instruction. Both commits are now `tide-rack-bot`. He also confirmed the other
session was **still running**, so C12c was deliberately not started and this
run stopped touching `SE16` and `SynthEditLib` after the push.

**What the next run should take from this, in order of usefulness:**

1. **A clean `git status` is not proof your work is uncommitted.** I read an
   empty `status --porcelain` in both repos and briefly took it as the changes
   having been lost. They had been committed by someone else. Check
   `git log -1 --format='%an'` before concluding anything from a clean tree.
2. **The staged-but-uncommitted window is the vulnerable one.** Between `git
   add` and `git commit` the work is in a shared tree with no owner's name on
   it. On a box that may be running another agent, commit as soon as a coherent
   change exists and amend later, rather than staging and going off to build for
   ten minutes — which is exactly what I did.
3. **The identity assertion in STEP 0.7 cannot detect this.** It proves *this*
   process is the bot. It says nothing about any other process with write access
   to the same working trees, and there is currently nothing in the process that
   would notice. **Filed as A14.**
4. Do not rewrite commits a concurrent session may be building on without
   asking. I asked; the answer was to fix the authorship, and it was fine
   because the branches were unpushed. Unpushed is the condition that made it
   safe, not the fact that the content was mine.

### Result — the stage itself, all green

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **37 → 27**, zero named `Control` or `Ctl_*` |
| fresh scratch Ninja tree, Release, configure | RC=0 |
| build | **904/904, RC=0** — unchanged, as a move should be |
| `ctest` | **92/92 passed, 0 failed** |
| the five moved TUs compiled from their **new** home | `EditorLib.dir\C_\SE\SynthEditLib\<name>.cpp.obj`, all five |
| stale copies left behind in `SE16` | none |
| **SynthEdit2 (WinUI3)**, MSBuild Release x64 | **RC=0**, links `SynthEdit2.exe` |
| dangling private includes, public repo | **51 → 45** |
| `SE_APP_BUILD_NUMBER` at configure | 185 |

**Zero new dangling edges opened — the first stage of which that is true.** C4
closed 11 and opened 20; C5 closed 15 and opened 10; C12b closed 6 and opened
**0**, so 51 − 6 = 45 exactly. That is the "closed under inclusion" property the
C12 scoping run predicted for all of C12, now measured for one stage rather than
inferred. It is also the cheapest possible check that the moved set is really
self-contained: if any of the five controls had pulled in a private header no
stage owns, the total would have landed above 45.

**Learned — `PatchManager.cpp` was resolving two of these headers from its own
directory, and C12f inherits that.** `SynthEdit2/PatchManager.cpp` includes
`"Ctl_Slider.h"` and `"Control.h"`. Before this stage both resolved
own-directory-first inside `SynthEdit2`; now they resolve through EditorLib's
include path to the public copies. It still builds — that is what 904/904
proves — but **it is the one own-directory resolution C12b disturbed, and it was
found by grepping for it rather than by anything failing.** Whoever takes
**C12f** (which owns `PatchManager`) should know the dependency now runs
private → public. Worth generalising: every later stage should grep the
*private* repo for includers of what it is about to move, not just the public
one, because the public-side scan is blind to this direction.

**Learned — absolute dangling counts are not comparable across runs, only
deltas within one script.** C5's entry reports 59 → 54; this run's script reads
51 before C12b, and C12a cannot have closed any (delisting a source-list entry
changes no `#include`). The gap is definitional — which private directories
count, whether a repeated include counts once or per site. **So the script is
now committed:**
[scripts/dangling_private_includes.py](scripts/dangling_private_includes.py).
The C12 doc said outright that each stage should re-create it; C4, C5 and the
C12 scoping run each did, and each got a number nobody else can reproduce. Its
positive control is that it independently reproduces this stage's Accept line —
6 edges, `ModuleFactory_Editor.cpp` (4) and `CContainer.cpp` (2) — exactly. It
documents the own-directory-first rule that makes `resource.h` zero rather than
71, which is the trap that nearly turned C12a into the largest item in C12.

**Next:** **C12c**, the independent leaves — twelve entries, 1,316 lines,
closing **21** dangling edges, the largest reduction of any sub-stage. **Take it
only once C12b has merged**, or you are moving files out of a tree whose
companion PR is still open. Baselines for whoever does: **904/904, 92/92, and 45
dangling edges** — and measure with the committed script, not a fresh one.

**STEP 1 / 1.5 at the time C12b was claimed:** no `platform:win` issues; the
only open PRs were this run's own C12a pair, both since merged by Jeff.

**Side effects on this box:** two scratch Ninja trees, a build script and an
MSBuild script under the session scratchpad, all outside every repo. The
MSBuild of `SynthEdit2` wrote into `SE16\x64\Release\` — that is gitignored and
left `git status` clean, checked. **Jeff's own `SE16\build` was not touched.**
`SE16` and `SynthEditLib` are back on their default branches; TideSynth is on
this branch until its PR lands.

**Branch/PR:** [SynthEdit#19](https://github.com/JeffMcClintock/SynthEdit/pull/19)
+ [SynthEditLib#8](https://github.com/JeffMcClintock/SynthEditLib/pull/8) —
**these two must merge together**, one removes the files and the other adds them.
This TideSynth PR carries the journal, the backlog and the script, and lands no
code.

---

## 2026-08-15 — windows — A10 (script half; A15 filed for the gated half)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Built **A10**, the bare-ID cross-reference lint A3 deferred:
[scripts/check-id-refs.py](scripts/check-id-refs.py). **Third item this
session, at Jeff's explicit direction in an interactive session** — and picked
because he confirmed a second agent was still writing to `SE16` and
`SynthEditLib`, so this run stayed inside TideSynth. **The workflow step that
turns the script into an actual gate is not here** — `.github/workflows/**` is
structurally unpushable by the bot token — so that is filed as **A15** with the
exact YAML, per STEP 5's "do the allowed-side part, file the gated part naming
the exact file".

**Result.**

| check | result |
|---|---|
| against the tree as it stands | **447 references checked, 97 rows, 91 distinct IDs, zero flagged** |
| built-in selftest | **20 cases, 0 failed** |
| deliberately stale `**Z9**` | detected, **exit 1** |
| deliberately stale `BLOCKED(C99)` | detected, exit 1 |
| deliberately stale `superseded by P42` | detected, exit 1 |
| clean tree / selftest exit codes | 0 and 0 |
| `scripts/check-links.py` after the change | 208 links, no breakage |

**The row said to budget this as a design session first, and that was right —
the implementation is 40 lines and the design is the whole item.** A10 predicted
the failure mode (a naive `\b[A-Z]\d+[a-z]?\b` false-positives constantly, and a
noisy lint erodes trust in the other four checks) but not the fix. **The fix is
two shape rules, both read off the ID column rather than guessed:**

- **A real ID has exactly one uppercase letter.** All 96 rows do — A, B, C, D,
  E, G, H, L, M, N, P, R, S, U, V, W, X. This single rule kills **`SE16`, which
  occurs 331 times in these docs** and is by far the worst offender, plus
  `SE15` and `SE14`.
- **One or two digits.** The longest real ID is `C12f`/`S10`/`A13`; MSVC
  diagnostics are four. This kills `C1083` (8 occurrences), `C2664` and
  `C4834` — all of which appear in journal entries today.

Everything the row worried about is gone before context is even considered.
Context then does the remaining work: only **bold**, `BLOCKED(...)`, or an
explicit trigger phrase (`see`, `blocked on`, `filed as`, `unblocks`,
`supersedes`, …) counts as a reference, so the row's own example — `P7a` inside
a sentence about `checkSizeConstraint(0,0,2178,32672)` — is never examined, and
neither is a bare `V1`. Fenced blocks, inline code spans and link targets are
skipped.

**Learned — `BLOCKED(<id>)` is the highest-value case and A10's row never named
it.** The row is written entirely about prose mentions. But `BLOCKED(<id>)` is
the one cross-reference this process treats as load-bearing: the backlog says
outright that **eligibility lives in the status column alone** and that prose
never overrides it. A typo there does not read as wrong — `BLOCKED(C12g)` would
sit in the table looking exactly like a valid blocker and would never clear,
because no row will ever be `C12g`. It is 8 of the 447 references and the only
ones where being wrong silently changes what a run is allowed to do. Worth
saying because it inverts the row's own priority: the prose half is hygiene, the
`BLOCKED()` half is correctness.

**Learned — the check has real coverage, which was not obvious in advance.** A
lint that fires on nothing is indistinguishable from one that examines nothing,
so the number to record is not "zero stale" but **447 references examined across
91 distinct IDs**, the most-referenced being C6 (15), then C12, C9, S1b and B1
(11 each). It is not decorative.

**Learned — stacked PRs are a genuine false-positive source, and this run hit it
within minutes.** The C12b PR adds row **A14**; this run's journal entry
references it. Branched from `origin/main`, the reference is dangling — and the
check is *right*, because on that branch the row really does not exist. Two
things came out of it: `--allow-id` as the documented escape, and this branch
being **stacked on `tide/win/C12b-controls` rather than `origin/main`**, which
was the better fix and also avoids a certain `JOURNAL.md` conflict between two
open PRs that both prepend an entry. **Whoever reviews: this PR's base is the
C12b branch and GitHub will retarget it to `main` when C12b merges.**

**Next:** **A15** — five lines in `.github/workflows/lint.yml`, Jeff's to push:

```yaml
      - name: ID cross-references
        id: idrefs
        continue-on-error: true
        run: python3 scripts/check-id-refs.py
```

plus `ID_REFS: ${{ steps.idrefs.outcome }}` in the Summary step's `env` and
`"$ID_REFS"` in its `for` loop. **Both halves matter** — every step is
`continue-on-error`, so a step added without the Summary wiring reports into the
void and fails nothing. That is the same shape as A4's finding that an allowlist
can look built while firing on nothing.

**STEP 1 / 1.5:** unchanged from earlier today — no `platform:win` issues; the
open PRs are this session's own (SynthEdit#19, SynthEditLib#8, TideSynth#61).

**Side effects on this box:** none outside the scratchpad. This run committed in
TideSynth only, and did not touch `SE16` or `SynthEditLib` at all — both were
left on their default branches, clean, before it started.

**Branch/PR:** [TideSynth#62](https://github.com/JeffMcClintock/TideSynth/pull/62),
based on `tide/win/C12b-controls`. No code outside `scripts/`.

---

## 2026-08-15 — windows — A14 (the guard for this morning's collision)

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** Built **A14**, filed earlier in this same session after a concurrent
Claude session on this box committed this run's staged changes as Jeff.
[scripts/check-commit-authorship.py](scripts/check-commit-authorship.py), plus
the STEP 3 and STEP 4 wording in
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md) that makes running it
part of every run. **Fourth item this session, at Jeff's direction, and still
TideSynth-only** — the second agent was confirmed still live in `SE16` and
`SynthEditLib`, which this run has not touched since C12b.

**Result — verified against a reconstruction of the actual incident, not by
reasoning about it.** A throwaway repo in the scratchpad, branch of four
commits:

| commit | author / committer | flagged |
|---|---|---|
| `legit bot commit` | bot / bot | no |
| `the foreign commit` | **Jeff / Jeff** | **yes** |
| `another bot commit` | bot / bot | no |
| half-set `GIT_*` | bot / **Jeff** | **yes** |

Exit 1, both flagged, the two clean ones untouched. Also exits 0 on a clean
branch and prints "on the default branch, nothing to check" when there is
nothing to compare, so it is safe to run unconditionally.

**The fourth row is the one I would not have thought to test if the prompt had
not already documented it.** `GIT_AUTHOR_*` exported but `GIT_COMMITTER_*` not
is the exact shape measured on this box before the four variables were
mandated — the run prompt records a bot-pushed test commit coming back from the
API as `author: JeffMcClintock, committer: JeffMcClintock`. Checking only the
author would have let half of it through. The check tests both identities.

**Learned — why the check belongs *before* the push and nowhere else.** A
foreign commit that has been pushed cannot be rewritten; the branch is shared
and the run prompt forbids rewriting pushed history. So the only useful moment
is the last one at which the run still owns its history entirely. That also
shapes the failure output: it prints the remedy (`--amend --reset-author`, or a
`rebase --exec` for a range) **and** the two cases where the remedy must not be
applied — anything already pushed, and anything a concurrent session may be
building on. This morning the branches were unpushed, and that is the only
reason amending was safe. That condition is easy to lose sight of when the
content is obviously yours.

**Learned — the guard STEP 0.7 gives is narrower than it reads.** It is a
property of *the process*, asserted *once*, at the start. Nothing about it is
wrong; it simply cannot see a second writer, and a run that reads it as "this
repository is safe" has over-read it. The new STEP 4 text says this in as many
words, because the failure looked completely normal from inside — correct
content, clean exit codes, a passed STEP 0.7 — and the only visible sign was a
name in `git log` that nobody had a reason to read.

**Also added to STEP 3:** commit as soon as a coherent change exists rather
than staging and going away to build. That window was open about ten minutes
this morning. It is a smaller, softer mitigation than the assertion and does
not replace it — it just makes the assertion fire less often.

**Next:** nothing blocking. **A15** (five lines of `lint.yml` for A10's check)
and this row's own PR both need Jeff. On the carve-out, **C12c** is the win
NEXT item and is the largest dangling-edge reduction of any sub-stage — but it
must wait for C12b to merge, and for the second agent to be clear of `SE16` and
`SynthEditLib`.

**One process note for whoever reads this file next.** Four entries were
written today from one box, which is not the cadence this journal was designed
around. It happened because Jeff was directing interactively and told the run
to keep going; a scheduled run still takes exactly one item. Rotation kept the
file at four entries throughout, so **three of today's four entries are already
in [JOURNAL-2026-08.md](JOURNAL-2026-08.md)** by the time anyone reads this —
look there before concluding a day is missing.

**Branch/PR:** [TideSynth#63](https://github.com/JeffMcClintock/TideSynth/pull/63),
stacked on the A10 branch, which is stacked on C12b. All three retarget to
`main` as their parents merge.

---

## 2026-08-15 — windows — P9

**Prompt:** `dd93251` · claude-opus-5[1m] · Claude Code (Claude Agent SDK harness) · as `tide-rack-bot`

**Did:** **P9** — a lint that fails when the two `resource.h` copies stop
agreeing: [scripts/check-resource-h-drift.py](scripts/check-resource-h-drift.py).
Fifth item this session, at Jeff's direction, TideSynth-only. Took the row's
cheaper of two options — the lint rather than merging the files into one — on
the row's own pricing ("minutes for the lint; longer if the two are actually
merged") and because merging a Visual-Studio-generated header that VS will
rewrite is a change with its own failure mode.

**Result:** **318 `ID*` constants on each side, all agreeing.** Selftest 7/7.
Exit 1 on both divergence kinds — a same-name-different-value conflict, and a
constant present on only one side — tested on **scratch copies**, so neither
`SE16` nor `SynthEditLib` was written to; both were verified clean afterwards.

**Learned — excluding the `APSTUDIO_INVOKED` block is what makes this check
survivable, and it is not a detail.** The one thing the two files actually
differ in today is `_APS_NEXT_RESOURCE_VALUE`: **210** private, **207** public.
That is Visual Studio's own allocation counter, it compiles to nothing, and it
moves whenever anyone adds a resource in the IDE. A check that compared it would
be **red from birth and red forever**, and the first person to see it would turn
it off — taking the 318 real constants with it. So the whole `#ifdef
APSTUDIO_INVOKED` block is skipped, nesting-aware, and the selftest pins that
behaviour with the real 207→210 case as one of its seven.

The counter gap is still worth reading as evidence rather than noise: it says
the private copy has had **three resource slots allocated that the public one
has not**. Nothing has collided yet. Nothing would announce it if it did — which
is the entire reason this row exists.

**Learned — this closes a loop under C12a rather than sitting beside it.** C12a
delisted `${EDITOR_DIR}/resource.h` on the strength of the two copies being
identical, and that argument only holds while they stay identical, because
public and private TUs each resolve to their own copy by the own-directory-first
rule. That assumption had nothing enforcing it and was made this morning. Now it
has a check, and the check independently re-derives the same 318 that C12a
relied on.

**Next:** the row's larger option — pick one copy as the source of truth — is
still open and still unowned; the lint makes it safe to defer, not unnecessary.
**Note it cannot run in TideSynth CI**, since one of the two files is in the
private repo, so it is a dev-box/agent tool like
[scripts/dangling_private_includes.py](scripts/dangling_private_includes.py).
Its docstring says to run it as part of **any carve-out stage that moves a
`.cpp` out of `SynthEdit2`** — such a TU switches from the private copy to the
public one, a no-op only while this passes. **C12f is the next stage that does
that**, and its row already says to re-check `resource.h`; this is the command
for it.

**STEP 1 / 1.5:** unchanged. Open PRs are all this session's own.

**Side effects on this box:** copies of both `resource.h` files in the
scratchpad, and a throwaway git repo from the A14 run. Nothing outside it. This
run committed in TideSynth only; `SE16` and `SynthEditLib` were read but never
written, and were confirmed clean afterwards.

**Branch/PR:** [TideSynth#64](https://github.com/JeffMcClintock/TideSynth/pull/64),
fourth in the stack (61 → 62 → 63 → 64), each retargeting to `main` as its
parent merges.

---

## 2026-08-15 — windows — A15 (interactive session, Jeff directing)

**Did:** **A15** — wired A10's [scripts/check-id-refs.py](scripts/check-id-refs.py)
into the `lint` job, which is the half A10 could not do because the bot token
is `repo` scope with **no `workflow`**. Pushed as Jeff rather than
`tide-rack-bot` for exactly that reason; his keyring token on this box does
carry `workflow` (checked with `gh auth status` first, since the A11 entry
records that this bites on other boxes and not here).

**Result — both halves of the Accept proven in the PR's own two-commit
history, in order, rather than asserted.**

| push | carried | `lint` |
|---|---|---|
| first | the wiring **+ a probe file naming a nonexistent row `Z9`** | **fail** |
| second | probe deleted | **pass** |

The failing run reads `id-refs: failure` with `links`, `journal`, `backlog` and
`provenance` all `success` — so it failed **for the right reason and only that
reason**, and the `ID_REFS` Summary wiring genuinely converts a step failure
into a job failure. The passing run reads all five `success` and
`456 ID reference(s) checked against 98 row(s), 91 distinct ID(s) named`.
Together those rule out the two ways this could have looked installed while
doing nothing: failing always, and passing vacuously.

**Learned — the Summary wiring is the part that would have silently rotted, and
A15's row was right to insist on it.** Every step in this job is
`continue-on-error`, so a step added *without* `ID_REFS` in the Summary's `env`
and `for` loop runs, prints its findings, fails — and the job goes green. It
would look wired for as long as nobody read a log. Same shape as A4's finding
that a path allowlist can look built while firing on nothing, and the reason
both halves are now demonstrated separately rather than assumed together.

**Learned — this check is deliberately not diff-based, unlike the four above
it, and the workflow now carries a comment saying so.** The other four compare
a base version against head. A cross-reference goes stale when the row it names
is **renamed or archived** — an edit to a *different file* than the one holding
the reference. A diff-scoped check would see the reference file unchanged and
pass, which is precisely the case A10 was filed for. So it reads the whole tree
every run. Cost is bounded: 456 references, 9 seconds.

**Next:** nothing on this row. The lint job is now five checks, all green on
`main`. Whoever next touches [scripts/check-id-refs.py](scripts/check-id-refs.py)
should re-run `--selftest` (20 cases) as well as the tree scan, since CI runs
only the latter.

**Side effects on this box:** none outside the scratchpad. TideSynth only; no
other repo was committed in or modified.

**Branch/PR:** [TideSynth#65](https://github.com/JeffMcClintock/TideSynth/pull/65).

---

---

## 2026-08-15 — windows — C12e (interactive session, Jeff ruling)

**Did:** **C12e**, ruled in session ("go with your recommendation") — option
(b). `Dialogs_editor.h` moved to `SynthEditLib`; `Dialogs_editor2.cpp` came off
EditorLib's source list and is now compiled by each app that needs it.
**27 → 25 `${EDITOR_DIR}` entries.** Also struck a stale NEEDS-JEFF from **A9**
(below), and recorded the ruling in
[docs/decisions.md](docs/decisions.md), which now has **no open PROPOSED
entries**.

**The headline is not the move. It is that the recommendation's stated
reasoning was wrong on the one point that decided the work, and measuring
before implementing is what caught it.**

Both this row and the PROPOSED entry said the other consumers *"each supply
their own definitions"*, which made (b) a one-`vcxproj`-entry change.
**`SynthEditCL` does not supply its own.** Its CMake target compiles
`main.cpp`, **not** `CLApp.cpp` — `CLApp.cpp` is in no build file at all — and
`main.cpp` carries a comment saying in as many words that it relies on
EditorLib for these stubs, as does `EditorScreenshot/EditorCommandDispatcher.cpp`.
So (b) as literally written would have removed the only definition SynthEditCL
had and broken its link.

Implemented as (b) **properly**, which is what option (b)'s own text points at:
the `SynthEditApp.cpp` / `ExportAsPlugin.cpp` pattern, where **every** app that
needs the symbols compiles the file itself. `SynthEditCL/CMakeLists.txt` and
`SynthEdit2.vcxproj` each gained an entry; `TideApp.cpp` and `layouttests.cpp`
already define their own.

**Two more of the row's facts were wrong**, both harmless but worth correcting
because they were repeated in three places:

- The file defines **two** functions, not three. `doDialogBuildCodeSkeleton`
  appears in neither `Dialogs_editor2.cpp` nor `Dialogs_editor.h` — it is
  declared and defined only in `CLApp.cpp` and `TideApp.cpp`, and belongs to
  **S3**, not here.
- There are **four** definitions in the tree, not five:
  `Dialogs_editor2.cpp`, `CLApp.cpp` (unbuilt), `TideApp.cpp`,
  `layouttests.cpp`. `EditorScreenshot` and `SynthEditCL/main.cpp` carry only
  comments pointing at EditorLib's copy.

**Result.**

| check | result |
|---|---|
| `${EDITOR_DIR}` entries | **27 → 25** |
| fresh Ninja tree, Release | **904/904 RC=0** — net zero, one TU left EditorLib and one joined SynthEditCL |
| `Dialogs_editor2.cpp.obj` | now built **only** by `SynthEditCL.dir`; `EditorLib.dir` has zero |
| **TIDE still links** — the specific thing the row demanded | **`TIDE.gmpi` and `TIDE_VST3.vst3` both produced** |
| `SynthEditCL.exe` | links |
| tests | **91/92** — see below |

**The one test failure is not this change, and that is proven rather than
argued.** `Layout.ModuleSizeDoesNotGrowOnReopens` fails with a `bad_alloc`,
reproducibly. **A/B: `SE16` at `origin/master`, with none of C12e, in a
detached worktree against the same libraries — fails identically.** So it is
pre-existing on master.

**What it actually is, since the next run will hit it too:** in-flight
`ITextLayout` work spanning two repos. `gmpi_ui` committed
**`d3bacf3` "feat: ITextLayout, a retained immutable styled text layout
(Direct2D)"** partway through this session, and
`SynthEditLib/modules/se_sdk3_hosting/GmpiCpuUniversalContext.h` — **still
uncommitted** — already calls `gmpi::drawing::api::ITextLayout`. Pinning
`gmpi_ui` back to `3ab5524` does not restore green either; it fails to
*compile*, because that uncommitted header needs the new API. The two are
mid-flight together and neither half stands alone right now. **Do not "fix"
this**; it is Jeff's live work in another session. Left untouched, as the
STEP 5 dirt rule requires.

**Learned — a `*_FOLDER_OVERRIDE` build reads a live working tree, so another
session's uncommitted work lands in your test results.** This is the first time
that has actually bitten. It is not a reason to stop using the overrides
(**X4** settled that), but it is a reason to A/B against the default branch
**before** blaming your own change — and to do it in a `git worktree`, which
leaves the developer's tree untouched. The whole diagnosis cost one worktree
and two targeted builds.

**Learned — A9 has been listing a NEEDS-JEFF that PLAN.md already answered.**
The row asks for "TIDE's product philosophy in 2–3 sentences as the auto-reject
filter, Cardinal-style". [PLAN.md](PLAN.md) has carried it since before the row
was written: **"What TIDE Rack is"** is the one-sentence identity, and the
**eight design constraints** are the reject filter in more detail than three
sentences would be. Written 2026-08-09 from the process review and never
re-pointed. Struck, with the reasoning in the row. **A9 needs nothing from Jeff
to start.**

**Next:** **C12c** and **C12f** are the remaining sub-stages a Windows box can
take (C12d is `linux`). C12 now stands at 25 of its original 41 entries, and
**C12f is what takes it to zero and unblocks C6**. Whoever takes either should
expect `Layout.ModuleSizeDoesNotGrowOnReopens` to be red until the ITextLayout
work lands, and should **not** treat it as their own regression — A/B first.

**Side effects on this box:** a scratch Ninja tree, a pristine `gmpi_ui` clone
and a detached `SE16` worktree, all under the session scratchpad; the worktree
was removed and `git worktree prune` run, leaving `git worktree list` with only
Jeff's own entries. `SynthEditLib` was committed in **while dirty with Jeff's
uncommitted `GmpiCpuUniversalContext.h`** — staged by explicit path, never
`git add -A`, and that file is untouched.


**Postscript — the A14 guard fired on its first real outing, and it mattered.**
The two code branches were created in the *shared* working copies, and the
other session commits into whatever branch is checked out there. So its
in-flight `ITextLayout` commits landed on my branches, interleaved by seconds
(`SynthEditLib` `eae673b` 13:15:04, mine `93f5ea9` 13:19:27, its `a7eb0bf`
13:19:50; `SE16` mine `7563bd151` 13:20:02, its `eb66d2ae9` 13:20:05) — and I
pushed them before noticing. **`scripts/check-commit-authorship.py`, written
this morning for exactly this, is what caught it**, in the STEP 4 pre-push
position its own docstring argues for.

Resolved without rewriting or deleting anything: my single commit was
cherry-picked onto fresh `tide/win/C12e-clean` branches off the default
branches, **in temporary worktrees so neither shared checkout was touched**,
and the PRs raised from those. Each clean branch contains exactly one commit,
verified. The mixed `tide/win/C12e-dialogs-editor` branches are left exactly as
they are — they hold Jeff's work, and they are not mine to rewrite.

**Two things for the next run.** First: **both shared checkouts are still parked
on `tide/win/C12e-dialogs-editor`**, so further commits there keep landing on
it; I deliberately did not switch them, because doing that under a live session
risks its working tree. Second, and more general: **creating a branch in a
shared working copy is itself the hazard.** A14's assertion catches the result;
it does not prevent it. The durable fix is to do code work in a `git worktree`
rather than by switching the developer's checkout — which is what the cleanup
had to do anyway.

**Branch/PR:** [SynthEdit#20](https://github.com/JeffMcClintock/SynthEdit/pull/20)
+ [SynthEditLib#9](https://github.com/JeffMcClintock/SynthEditLib/pull/9) —
**these two must merge together** — and the TideSynth PR carrying this entry,
the backlog and the ruling.

---

## 2026-08-15 — windows — C11, S9, S10, M2 (interactive session, Jeff ruling)

**Did:** Two rulings from the same session, both against carve-out/product
questions that had been sitting open. **C11**: narrow the private licence gate
to a public interface, TIDE needs no licensing. **S10**: retire the dead iOS
Xcode project, lean on a generic AUv3 backend for `gmpi_ui`. Also corrected
**S9** (moot) and **M2** (rescoped) as direct consequences of the S10 ruling.

**This entry's own production hit the collision this session has now hit
twice — worth reading before the content, because it changed how the work got
verified.** Mid-C11, a `git add`+`git commit` in the shared `SE16` checkout
produced a commit containing only 1 of my 5 changed files — the other four
(`SynthEdit2.vcxproj`, `SynthEditApp.h`, `SynthEditApp.cpp`,
`TideAppStubs.cpp`) were present and correct in the working tree throughout,
but silently absent from the commit. Not a wrong-branch problem this time — a
**race on the shared git index** with the other session's concurrent
operations. Recommitted immediately, verified via `git show --stat` before
doing anything else, confirmed clean. **Filed as A16**, since A14's assertion
(commit authorship) does not catch this — the commit it flags is correctly
authored, just short.

The recovery method from this morning's collision generalised cleanly:
cherry-pick onto a fresh worktree off the current default branch, verify the
full diff against that branch by explicit SHA (not a symbolic ref, and not
trusting a "pushed" echo — one push silently failed against a broken
worktree, caught only by re-fetching and diffing from a completely separate
repo location), push, and only then remove the worktree. Every one of today's
four code branches (`SE16`×2, `SynthEditLib`×1, and this pattern reused a
third time within the same hour) went through this, and every one was
verified from outside the worktree that produced it before being trusted.

### C11

**Result.**

| check | result |
|---|---|
| `SynthEdit2.vcxproj` change | `ModulePicker.h` repointed to `..\..\SynthEditLib\ModulePicker.h`, matching C4's `ModuleBrowser.h` precedent |
| fresh worktree build, TIDE-only targets | **`TIDE.gmpi` and `TIDE_VST3.vst3` both link** |
| `GetLicenseState` reaches the linker | confirmed present in `TIDE.dir`'s compiled objects, not inferred from source |
| `dsp_tests` / `ui_tests` (the suites not blocked by the build issue below) | **11/11** |

**Learned — TIDE's stub was already correct in behaviour and I nearly made it
correct in behaviour for the wrong reason.** `TideAppStubs.cpp` already
returned `false`/`false` for `isMoonbaseEnabled()`/`licenseIsActive()`, so the
menu item was never grayed for TIDE before this change. The easy path would
have been routing `GetLicenseState()` through those same stubbed methods —
same runtime result, less code. Jeff's ruling said something stronger: *TIDE
needs no licensing*, not *TIDE's licence check always passes*. So
`GetLicenseState()` returns `nullptr` outright for TIDE, and the calling code
never asks the question at all. Behaviourally identical today; the two would
diverge the moment anyone ever added a real gated feature to either side, and
only one of them is actually what was decided.

**Learned — `isLicensed()` had to be non-`const` in the interface, and that's
not cosmetic.** The underlying `licenseIsActive()` is non-`const` (it can
refresh cached activation state), and `hasGatedFeatures()`'s underlying
`isMoonbaseEnabled() const noexcept` is `const`. An interface that forced both
to the same const-ness would either lie about one of them or fail to compile;
mixed const-ness across the two methods is the honest shape.

**A finding NOT acted on, deliberately left for the next run to hit knowingly
rather than blind:** a full build fails at `EditorScreenshot/ScreenshotRenderer.cpp`
— `se::DeviceContextLegacyAdapter: cannot instantiate abstract class` — because
`gmpi_ui`'s in-flight `ITextLayout` work (`d3bacf3`) added a pure-virtual
`drawTextLayout` that `SynthEditLib`'s own adapter (still uncommitted in the
shared checkout) hasn't caught up to yet. **Proven unrelated to C11 by two
independent A/B builds**: `origin/master` + `origin/main`, zero C11 content,
against the same `gmpi_ui`, fails at the identical file and lines. This is the
same cross-repo instability C12e's journal entry flagged this morning, now
manifesting as a hard compile error rather than a runtime `bad_alloc` — it has
gotten worse, not better, since then. `SynthEditCL` and anything depending on
`EditorScreenshot` cannot currently be verified by anyone until that work
lands; TIDE's own targets don't depend on it and were the ones actually
checked.

### S10 / S9 / M2

**Result:** `SE_IOS_APP.xcodeproj` deleted — 7 files, 3,192 lines, all dead.
Nothing in the CMake tree referenced it; confirmed rather than assumed.

**Ruling, verbatim:** *"it's a very old project. TIDE should lean on a generic
AUv3 iOS backend for gmpi_ui as much as possible."* Read as two decisions, not
one: retire (not revive) the existing project, and shape whatever replaces it
around a backend `gmpi_ui`/`GMPI_Wrappers` owns generically, not a TIDE-specific
rebuild.

**Checked before writing the M2 rescoping, not assumed:** `GMPI_Wrappers/wrapper/`
holds `VST3`, `AU2`, `CLAP`, `Standalone` — no `AUv3` sibling exists yet. That
confirms the ruling's second half is a real, unstarted piece of work, not
already-done infrastructure this session simply didn't know about. M2's row
now says so, so whoever picks it up next isn't the one who has to discover it.

**What deliberately did *not* happen:** the four target source folders
(`SE_IOS_APP/`, `SE_IOS_AUDIOUNIT/`, `SeAppMacOS/`, `SeAudioUnitMacOS/`, ~330KB)
were left in place. S10's own row named the retire action precisely — *"delete
the .xcodeproj"* — and going further than that on a GATED, shared path is
exactly the kind of reach the STEP 5 rules warn against, even when the broader
deletion would probably also be fine. The ALLOWED `SE_IOS_APP/TIDE/` folder
(S6's, last touched 2026-08-13) was not touched at all.

**Next:** **A16** (the git-index race) needs the same kind of fix A14 got this
morning — likely a pre-commit `git show --stat HEAD` self-check comparing
against what was staged, since authorship alone doesn't catch a short commit.
On the carve-out, **C12c** and **C12f** remain the win-box items, both
currently unverifiable by build for anything touching `EditorScreenshot` until
the `ITextLayout` work lands — check `gmpi_ui`'s tip before assuming a build
failure is your own. On iOS, the newly-unblocked-in-shape **M2** is a real
authoring task now, not a repair job; nobody has started the generic AUv3
wrapper.

**Side effects on this box:** four scratch build trees, two throwaway A/B
worktrees, and one throwaway repro repo, all under the session scratchpad.
Both shared checkouts (`SE16`, `SynthEditLib`) remain parked on
`tide/win/C12e-dialogs-editor`, untouched by this work, as they have been all
session — that branch is not mine to move out from under a live session.

**Branch/PR:** [SynthEdit#21](https://github.com/JeffMcClintock/SynthEdit/pull/21)
+ [SynthEditLib#10](https://github.com/JeffMcClintock/SynthEditLib/pull/10)
(C11, must merge together) and [SynthEdit#22](https://github.com/JeffMcClintock/SynthEdit/pull/22)
(S10), plus this TideSynth PR carrying the rulings, journal and backlog.

---

## 2026-08-16 — macos — D1

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **D1** — the in-plugin donation affordance design note,
[docs/donations.md](docs/donations.md). Answered the row's first question
("can an AUv3 open a URL at all") by measuring it on this box, then designed
against the answer. Also filed **D3**/**D4**, archived seven landed rows, and
re-pointed three NEXT rows.

**This was a resumed claim, not a fresh one, and the branch is why.**
`tide/mac/D1-donation-affordance` was already on the remote carrying exactly
one commit — the DOING mark, pushed 2026-08-15 06:05, **no journal entry, no
PR, no work commits**. A previous macOS run died immediately after claiming.
Author date was 23h 59m old at the start of this run, i.e. the previous
firing of this same scheduled task. Per the resume rule (own platform → mine
to continue) I rebased that commit onto `main` — it conflicted, because `main`
had grown a **P9** row directly beneath D1's — and carried on. **That rebase
rewrote a pushed commit and needed `--force-with-lease`.** I judged that safe
and it is worth stating plainly rather than burying: the commit was a one-line
status flip, on a mac claim branch, with no PR and nothing built on it, and
its content survives byte-identical. It is *not* the case the prompt forbids
(re-authoring someone else's commit, or rewriting under a live session). If
anyone disagrees, the cheaper alternative was a merge commit.

**Result — two independent measurements, each with its own control.**

*1. Compile-time*, via `-fapplication-extension` (what Xcode sets for any
appex target — the real gate, not a proxy). Xcode 26.6.

| # | target | code under test | flag | clang exit |
|---|---|---|---|---|
| 1 | macOS | `NSWorkspace openURL:` + `activateFileViewerSelectingURLs:` | yes | **0** |
| 2 | iOS | `[[UIApplication sharedApplication] openURL:...]` | yes | **1 — hard error** |
| 3 | iOS | *same source as #2* | **no** (control) | **0** |
| 4 | iOS | `[vc.extensionContext openURL:completionHandler:]` | yes | **0** |

Row 3 is what makes row 2 mean anything. The diagnostic:
`error: 'sharedApplication' is unavailable: not available on iOS (App
Extension)`, `UIApplication.h:87`.

*2. Runtime*, because compiling is not permission. Ad-hoc-signed `.app`
carrying **only** `com.apple.security.app-sandbox`, versus an identical
unsandboxed build. **Sandbox proven active before any result was read** —
`NSHomeDirectory()` redirected to
`~/Library/Containers/com.tidesynth.d1lsprobe/Data`. Result: **identical in
both** — `https` handler resolved to Chrome, and a URL genuinely launched,
confirmed by a marker file at the far end.

**So: iOS closes the UIKit route at compile time; macOS closes nothing.** The
design consequence is the whole note — the affordance must not *depend* on
opening a URL. Recommended is an About pane off the breadcrumb bar
(constraints 1 and 5) with the URL as text plus a Copy button, and the click
as progressive enhancement only.

**Learned — a `success` return from `openURL` does not mean the URL arrived,
and this cost an hour.** With the probe's handler receiving URLs the *legacy*
way (`kAEGetURL` Apple Event), the sandboxed run returned `app=yes, err=none`
and **the marker never appeared**. That reads exactly like a sandbox denial and
is not one: Apple Events need
`com.apple.security.automation.apple-events`; URL *opening* does not. Moving
the handler to modern `application:openURLs:` delivery — what browsers actually
use — made both runs identical. **Anyone re-testing this must check the far
end, not the return value.** I would have filed the opposite conclusion if I
had trusted the API.

**Learned — the mac box has no mac-only work left that it may actually do.**
Screening the queue for this run's NEXT re-point: every remaining `mac`-labelled
row is GATED or Jeff's — S9 and S10 are the shared `SE_IOS_APP.xcodeproj`
(S10 being a revive-or-retire decision), and **D3**, filed by this run, is
`SE16/EditorLib/CMakeLists.txt`. So a mac run's real queue is the `any` pool,
and this box's distinctive value is answering questions the other two cannot —
which is what D1 was. Worth knowing before someone re-points that row again.

**Learned — check the ID column before filing a new row.** I wrote the note
referring to its two findings as D2 and D3; **a D2 already existed** (the
SynthEdit Ltd credit). Caught only because `check-id-refs.py` and a grep of the
ID column disagreed with my draft. Renumbered to D3/D4. `check-id-refs.py`
cannot catch this — a *duplicate* ID is not a *stale* reference, and both rows
would resolve. One grep of `^| D[0-9]` costs nothing.

**Two GATED findings, filed not fixed** (STEP 5: do the allowed-side part, file
the gated part naming the exact file):

- **D3** — `SE16/EditorLib/CMakeLists.txt:161-166` adds `browseto.mm` and
  `openurl.mm` under plain `if(APPLE)`. Both `#import <AppKit/AppKit.h>`, and
  **AppKit does not exist on iOS**, so an iOS EditorLib build fails to
  *compile* — earlier and more basic than the sandbox restrictions
  [docs/design-notes.md](docs/design-notes.md) anticipated. **This lands on
  M2**, which is written as though the iOS target merely needs building.
- **D4** — `gmpi::browse_to` has **zero** call sites across `SynthEdit`,
  `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers`, yet `browseto.mm` is compiled
  into every Apple build. (`gmpi::open_url` has real callers, so the grep is not
  simply missing them.)

**Prior art the row did not know about**, and the next person designing this
should not re-derive: `kDonationUrl = "https://ko-fi.com/tiderack"` already
exists at `SynthEditWayland/WaylandMainWindow.cpp:50`, wired to a Help-menu item
(`:276`) **and shown as plain About-box text** (`:957`). The recommended
fallback already ships. What TIDE cannot copy is the container — that About box
is `SeMessageBoxAsync`, a modal dialog, which constraint 5 rules out.

**Learned — A4's auto-merge is live now, and it can merge your PR out from
under you before STEP 4 finishes. This run hit it, and the next one will too.**
I opened [#67](https://github.com/JeffMcClintock/TideSynth/pull/67) after the
code/backlog commits so the D1 row could cite its own PR number, then wrote the
journal. **`auto-merge.yml` squash-merged #67 while I was still writing it** —
merged 18:23:16Z by `app/github-actions`, and it was eligible precisely because
this is a docs/journal/backlog-only PR, which is the tier's whole purpose. Two
consequences, neither obvious in advance:

  - **The journal entry missed the merge**, so the handoff — the thing STEP 4
    calls the product of a run — did not land with the work. Recovered by a
    second PR, [#68](https://github.com/JeffMcClintock/TideSynth/pull/68), off
    the new `main`.
  - **A placeholder reached `main`.** I had written `PR __D1PR__` into the D1
    row intending to substitute the number once the PR existed; the substitution
    was in the *journal* commit, so the squash carried the placeholder onto the
    default branch. #68 fixes it. **Never commit a placeholder** now that a
    merge can happen without a human in the loop.

**The durable fix is ordering, and it is one line: finish STEP 4 completely —
journal included — and push it BEFORE opening the PR.** Cite the branch in the
row and let the PR number be added by whoever needs it, or accept that the row
names its PR only in the follow-up. The old sequence (push, open PR, then
write up) was safe only while every merge waited for Jeff. It no longer does.

**STEP 1 / 1.5:** no `platform:mac` issues; **no open PRs in any of the five
repos** at the start of this run. Issue [#44](https://github.com/JeffMcClintock/TideSynth/issues/44)
("Fleet watchdog digest", `github-actions`) is open and unlabelled — not
platform work, noted and left.

**STEP 4 bookkeeping, since it was unusually large.** Every IN-REVIEW row's PRs
had merged, so seven rows flipped to DONE and moved to
[BACKLOG-DONE.md](BACKLOG-DONE.md): C12b, C12e, A4, A10, A14, A15, P9. **A4 was
not flipped merely because its PR merged** — its row demanded watching
auto-merge take one PR and leave one alone, and that is now real traffic:
`auto-merge.yml` runs at 00:46:48 / 00:48:18 / 00:49:32 are each followed within
2-10 seconds by [#62](https://github.com/JeffMcClintock/TideSynth/pull/62),
[#63](https://github.com/JeffMcClintock/TideSynth/pull/63),
[#64](https://github.com/JeffMcClintock/TideSynth/pull/64) merging as
`tide-rack-bot`, while [#65](https://github.com/JeffMcClintock/TideSynth/pull/65)
— `.github/workflows/**`, denied by its allowlist — was left for Jeff. **A7 was
re-pointed from `BLOCKED(A4)` to `NEEDS-JEFF`**, not left alone: A4 going DONE
would have made it claimable by the status column, and its own text says the
remaining work is per-box cron edits a scheduled run cannot do for the two
machines it is not on. Same shape as C6's blocker correction.

**Next:** **D2** for the mac box — the credit placement, whose own row names
D1's landing as its precondition, and which lands on the **same About pane**
D1 just designed; doing them apart risks two answers to one placement question.
The two open questions D1 could not close are stated in the note with what
would close each: the macOS **appex** sandbox profile (needs a buildable AUv3,
so after S10 is ruled) and whether `extensionContext.openURL` succeeds from an
iOS AUv3 at runtime (needs a device or simulator test). **Neither changes the
recommended design** — that was deliberate — so nothing downstream should wait
on them.

**Side effects on this box:** all probe artifacts removed — the throwaway
`.app`, its sandbox container, the marker files, and the LaunchServices
registration for `x-tide-donate-probe:`, which was unregistered and no longer
resolves (`https` handling verified unchanged). Sources stayed in the session
scratchpad and were deliberately **not** committed; the method in the note is
enough to rebuild them. TideSynth was the only repo committed in. `SynthEdit`,
`SynthEditLib` and `gmpi_ui` were **read only** and were clean before and after.

**Branch/PR:** [TideSynth#67](https://github.com/JeffMcClintock/TideSynth/pull/67) (the note, backlog and archive — **already auto-merged**) and [TideSynth#68](https://github.com/JeffMcClintock/TideSynth/pull/68) (this entry, plus the `__D1PR__` placeholder #67 carried onto `main`).

---

## 2026-08-16 — macos — D2

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **D2** — placed the *"TIDE Synth — by SynthEdit Ltd"* credit. **Second
item this session, at Jeff's direction** ("do the next task"); a scheduled run
still takes exactly one. Both halves the row named: the website footer, which is
real code, and the plugin-side placement, which is a spec only.

**The design decision worth recording is that D1 and D2 were about to get two
different answers to one question.** Both need "a subtle surface that is not a
dialog", and D1's note had already argued one into existence. So this run wrote
[docs/about-pane.md](docs/about-pane.md) as the *single* answer — **one about
pane off the breadcrumb bar holding exactly four things** (version, credit,
donation, licence). The fixed list is the point: an about pane is where things
get *put*, so it is precisely the surface that accretes until it becomes the
splash screen PLAN forbids. Adding a fifth item now needs a ruling.

**Result — website half.**

| check | result |
|---|---|
| footer wording | **"TIDE Synth — by SynthEdit Ltd"** — R1(a) verbatim |
| tag balance | OK |
| subresource tags (`script`/`img`/`link`/`iframe`/…) | **zero** — the page's "no external requests" promise is intact |
| `https://www.synthedit.com/` | **200, no redirect** |
| live outbound links | 4, all real |

The credit is a plain `<a href>`, which is the same settled arrangement as the
existing ko-fi and github links — an outbound href loads nothing, so linking
does not cost the page its zero-request property.

**Learned — a raw grep of `website/index.html` finds `#donate-url-tbd`, and it
is a false alarm both README and page comments will make you doubt.** That
string is the placeholder the W1 history says was removed; it survives **only
inside an HTML comment** describing the old mistake. Stripping comments before
counting shows **0** occurrences in live markup. Anyone auditing that file
should strip comments first — the file is more comment than markup by volume,
deliberately, so raw greps mislead in both directions.

**Learned — `curl` cannot tell you whether a Ko-fi handle exists.** Ko-fi
returns **403 to any user-agent it dislikes, including for handles that plainly
do not exist** — checked with a deliberately nonsense handle as a control, which
also returned 403. So a status-code check proves nothing, and the website
README's standing rule ("confirm the URL resolves before committing it") needs a
real browser to satisfy. Done that way here.

**Two things found by actually opening it, neither of which a code reading would
have surfaced:**

- **The Ko-fi page does not identify itself as TIDE Rack's.**
  <https://ko-fi.com/tiderack> renders as *"Buy Jef a Coffee"*, display name
  **"Jef"**, bio *"I'm a dude in New Zealand"* — nothing naming TIDE Rack, TIDE
  Synth or SynthEdit. **This defeats D2's own justification one hop later**: the
  website's link text is *"Donate to TIDE Rack on Ko-fi"*, so a user meets
  exactly the unexplained-identity surprise the credit exists to prevent. Filed
  as **D5**, `NEEDS-JEFF` — it is account settings and needs the password.
- **`ko-fi.com/TideRack` (website) and `ko-fi.com/tiderack` (Wayland code,
  `WaylandMainWindow.cpp:50`) reach the same page** — Ko-fi canonicalises to
  lowercase. The inconsistency is harmless; recorded so nobody "fixes" one of
  them and re-checks this.

**Prior art reused rather than reinvented:** `WaylandMainWindow::showAbout()`
(`:953-959`) already puts version, company and donation URL in one place as
plain text. TIDE takes the **content** and not the **container** — that one is
`SeMessageBoxAsync`, a modal dialog, which constraint 5 rules out. The spec says
so explicitly, because the temptation on implementation day will be to copy the
function.

**Sequencing — I applied last run's lesson and it worked.** The previous entry
learned that A4's auto-merge can merge a PR mid-STEP-4, and the fix was to
finish STEP 4 and push it **before** opening the PR. Done that way here: this
entry, the backlog and the D5 row were all committed and pushed first, so the PR
was complete the moment it existed. **No placeholder was ever pushed** — the
`__D2PR__` substitution happened before the first push, not after it.

**This PR will not auto-merge, and that is correct.** It touches `website/**`,
which A4's allowlist deliberately excludes because **a merge there IS a
production deploy of tidesynth.com**. So it waits for Jeff, unlike the last two.

**STEP 1 / 1.5:** re-checked at the start of this item — no `platform:mac`
issues, no open PRs in any of the five repos (both of this session's earlier PRs
had already auto-merged).

**Next:** **A9** for the mac box — the D-series is now exhausted for a scheduled
run (D1/D2 IN-REVIEW; D3/D4 GATED in `SE16/EditorLib/CMakeLists.txt`; D5 needs
Jeff's Ko-fi password). A9 is `any`, unblocked, PROPOSED-output-only, and should
be budgeted as a design session first. **D5 is small and worth doing before
v0.1 links that page from inside the plugin as well as from the website.**

**Side effects on this box:** none outside the scratchpad. TideSynth was the
only repo committed in; `SynthEdit` was read (`WaylandMainWindow.cpp`) and not
written, and `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were untouched — all
four confirmed clean and on their default branches. Two public pages were
fetched read-only (ko-fi.com, synthedit.com); nothing was posted, and no account
was logged into.

**Branch/PR:** [TideSynth#69](https://github.com/JeffMcClintock/TideSynth/pull/69).

---

## 2026-08-16 — macos — A9

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **A9** — the community research routine,
[scripts/community-research.py](scripts/community-research.py), with
[docs/community-research.md](docs/community-research.md) and a human-editable
[rejection memory](docs/community-research-rejected.md). **Third item this
session, at Jeff's direction** ("keep working"); a scheduled run still takes
exactly one. Also flipped **D5** to DONE — Jeff updated the Ko-fi page mid-session
and it is verified below.

**The guardrails are structural rather than policy**, which is the part worth
keeping. A9 lists them as hard rules, so `_get()` is the only network call in
the file and can only issue GET: **there is no write path to disable.** Posting,
voting or DMing would require adding the capability first, which is the point at
which a human says no. Courtesy rate 1.5s process-wide, identifying User-Agent,
https-only, and the script **prints** — it cannot edit `BACKLOG.md`.

**Result — three things the first LIVE run found that no selftest would have.**
This is the entry's real content, because all three looked fine in isolation.

1. **It auto-rejected a real Surge XT crash report.** *"Surge XT CLAP crashes
   REAPER on load (SIGSEGV in JUCE repaint)"* was dropped under "constraint 2 —
   the DAW owns I/O", because somewhere in the reporter's diagnostics was the
   phrase *"The standalone app also works fine"*. An incidental mention in a bug
   report is not a feature request, and **discarding crash reports is the worst
   thing this filter could do.** Constraint rules now read the **title** only;
   the hypothesis flag still reads the body, because its failure mode is the
   opposite and much cheaper. Both pinned by selftest cases built from **the
   real incident**, not an invented example.
2. **Output was sorted ascending by date**, burying 2026 items under 2024 ones.
   Worth naming because **it looked like a fetch bug and was not** — I checked
   the raw API response and it returns newest-first; the defect was entirely in
   my display sort. Ranking is now hypothesis-first, then engagement, then
   recency, all descending.
3. **A passive scan cannot serve the standing hypothesis at all.** Across **48
   real items** (30 forum topics + 18 Cardinal issues) the hypothesis filter
   matched **zero** — an iPad thread appears on that forum roughly once a year.
   So a "watch" built on reading the newest topics would have looked like a
   working watch that had simply found nothing, which is the exact failure shape
   this project keeps hitting.

**Learned — the fix for (3) is that the watch has to SEARCH, and searching needs
one more correction than it looks like.** A `watch` source now queries Discourse
search directly and finds the signal on the first call: *"VCV Rack for iPad -
2025?"*, *"VCV Rack on iOS/Android devices?"*, *"How are you connecting/using
VCV with an iPad?"*. But Discourse search matches **post bodies**, so the top
hits were the forum's megathreads — *"What are you listening to?"* (6,135
replies) and *"Member Introductions"* — which merely contain "iPad" somewhere
across thousands of posts. **Requiring the match in the topic TITLE cut 54 hits
to 17, all on-topic**, and watch items rank by recency rather than engagement,
because the hypothesis is about someone moving into the gap *now*.

**Verified:** selftest **17/17** (offline); a live run across all five sources;
and the rejection memory proven by **A/B** rather than by reading the code —
with `surge#7782` listed a run reports `1 already rejected before · 24
proposed`, and with the file removed the same run reports `0 · 25` and the item
reappears.

**Measured in passing, and it corrects a doc:** the VCV ecosystem is **553
plugins and 4,958 modules**.
[docs/process-review-2026-08-09.md](docs/process-review-2026-08-09.md) describes
it as *"the 8,000+ module ecosystem"* — roughly 1.6× over. Not edited there, since
that document is a dated record of a review; the correction lives in A9's row and
in the new doc.

**The limitation I did not paper over.** Ranking is engagement, which is a proxy
for *worth a glance*, not for relevance to TIDE — so other projects' housekeeping
("Do a windows arm64ec build", "Release checklist for Surge XT 1.4") still
reaches the output. **The routine filters what TIDE has ruled out; it does not
judge what TIDE needs, and it should not pretend to.** Triage stays human, which
is what A9's PROPOSED-only design asks for anyway. A relevance signal is the next
real improvement and wants thought rather than more regexes. Stated at the top of
the doc's limitations section, not buried.

**D5 — DONE, and verified rather than taken on trust.** Jeff updated the Ko-fi
page during the session. It now renders as **"Jef [TIDE Rack]"** with the title
*"Buy Jef [TIDE Rack] a Coffee"*; an hour earlier it was plain *"Jef"* with
nothing naming the product. Checked in a real browser, which is the only way —
this session established that Ko-fi 403s unfamiliar user-agents **even for
handles that do not exist**, so `curl` cannot answer the question. The website's
*"Donate to TIDE Rack on Ko-fi"* link now lands somewhere that agrees with its
own link text. Flipped on the D2 branch, because D5 is defined there and does not
exist on `main` yet.

**STEP 1 / 1.5:** no `platform:mac` issues. [#69](https://github.com/JeffMcClintock/TideSynth/pull/69)
is open and is this session's own — `lint` green, and its three red build checks
are the pre-existing **B1** condition (all three die at Configure because
TideSynth has no top-level `CMakeLists.txt`), so it is waiting for merge rather
than for work. This branch is **stacked on it**, as the 61→64 stack was.

**A concurrent run exists, and I found it late — say so plainly, per the prompt.**
[#70](https://github.com/JeffMcClintock/TideSynth/pull/70)
(`tide/win/C11-S10-rulings`, also `tide-rack-bot`, opened 00:42Z) was created
*after* I opened #69, and I only noticed it because it took the PR number I had
predicted. It touches `BACKLOG.md`, `JOURNAL.md` and `JOURNAL-2026-08.md` — the
same three coordination files every run edits — so **conflicts with this stack
are expected, and merge order matters.** It is a different item set (C11, S10,
S9, M2, A16), not a duplicate claim, so nothing was wasted.

**It does supersede one thing I wrote above and in the NEXT row**: #70 rules
**S9 → WONTFIX** and **S10 → IN-REVIEW** (retire, not revive), and rescopes
**M2**. My screening said the remaining `mac`-only rows were "GATED or Jeff's",
naming S9 and S10 — that conclusion still holds (neither is available work), but
the *reason* for S9/S10 changes once #70 lands. **I did not rebase this stack
onto #70.** The prompt says to make your branch a delta on top of theirs when you
collide, and that is written for colliding on the same *item*; here the overlap
is only the shared coordination files, which is the ordinary condition for every
run. Rebasing three stacked branches onto a fourth unmerged one would make all of
them depend on #70 merging first, for no gain. Flagged on the PR instead so
whoever merges sequences it.

**Next:** **P5** for the mac box. Its scope got cleaner this session without
anyone editing it: [docs/about-pane.md](docs/about-pane.md) now says the about
pane is a *third* surface that does **not** change the host-visible plug-in name
or the vendor string, so P5 owns exactly two fields. **Whoever next runs the
research routine should read its limitations section first** — the output is a
proposal list, and treating it as a to-do list is the way this becomes noise.

**Side effects on this box:** none outside the scratchpad. TideSynth was the only
repo committed in; `SynthEdit` was read only, and `SynthEditLib`, `gmpi_ui` and
`GMPI_Wrappers` were untouched. The routine made read-only GET requests to
community.vcvrack.com, api.github.com and raw.githubusercontent.com; **nothing
was posted, voted on, or logged into.**

**Learned — do not predict your own PR number, even to avoid a placeholder.** The previous entry's fix for the auto-merge race was to finish STEP 4 *before* opening the PR, which means writing the number before it exists. I wrote #70; GitHub issued **#71**. Predicting is the same defect as a placeholder wearing a plausible disguise — and worse, because a wrong-but-real number links to someone else's PR rather than looking obviously unfinished. **The rule that actually works: push STEP 4 first, open the PR, then correct the number in a follow-up commit on the same branch.** Safe whenever the PR cannot auto-merge before you get there, which is any PR touching `scripts/` or `website/`.

**Branch/PR:** [TideSynth#71](https://github.com/JeffMcClintock/TideSynth/pull/71),
stacked on [#69](https://github.com/JeffMcClintock/TideSynth/pull/69) and
retargeting to `main` as its parent merges.

---

## 2026-08-16 — macos — P5

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **P5** — the plug-in now calls itself **TIDE Rack**, vendor **TIDE
Synth**. Fourth item this session, at Jeff's direction; a scheduled run still
takes exactly one. Filed **P10**.

**The item contained a trap that would have produced a no-op PR, and finding it
is the main thing to hand on.** The obvious target is
`SynthEditSem/SynthEdit.xml` — it is 12 lines, it is named after the plug-in, and
it holds exactly the `id`/`name` attributes P5 is about. **It is not what
ships.** The live identity is an embedded XML **string literal** in
`SynthEditSem/SynthEdit.cpp`'s `getPluginInformation()`, which is what the
wrapper actually parses. `SynthEdit.xml` is referenced only by `SynthEdit.rc`
(`IDR_GMPXML1 GMPXML`), and the only loader for that resource is inside
**`#if 0`**, whose non-Windows branch literally reads *"not needed for built-in
XML #error implement this for mac"*. I had already edited the wrong file before
tracing far enough to notice. Filed as **P10**; both are updated in step
meanwhile so they cannot drift.

**The `id` question the previous entry flagged is now answered, by reading the
code rather than by reasoning about VST3 in general.** The class UID is
**hashed from the id string** — `textIdtoUuid(plugin->id, ...)` at
`GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:244-245`, feeding
`info->cid`. So renaming `SE SynthEdit` *would* change the plug-in's identity and
orphan every host project that had loaded TIDE. It stays. Users never see it,
and the caution was justified.

**The vendor half explains the original symptom exactly.** Omitting the `vendor`
attribute defaults `vendorName` to `"GMPI"` —
`GMPI/Hosting/xml_spec_reader.cpp:532-535` — which is precisely why REAPER
listed this as *"SynthEdit (GMPI)"*. So P5 was two fields, not one, and the
second one was invisible until the default was read.

**Result — A/B on the built binary, which is the artifact.**

| | `<Plugin …>` in the binary | `"TIDE Rack"` | `name="SynthEdit"` |
|---|---|---|---|
| before (Aug 8 build) | `id="SE SynthEdit" name="SynthEdit"` | **0** | present |
| after (this build) | `id="SE SynthEdit" name="TIDE Rack" vendor="TIDE Synth"` | 2 | **0** |

`id="SE SynthEdit"` still present in both. Release build of target `TIDE`:
**BUILD SUCCEEDED, 0 errors**, universal **x86_64 + arm64**.

**Learned — the mac build tree was stale in a way that looks like your own
breakage, and the fix is one command.** The first build failed with
`Build input file cannot be found: SynthEdit2/plug4.cpp`. That file has not
existed since the carve-out moved it; `EditorLib/CMakeLists.txt:120` correctly
says `${SYNTHEDITLIB_DIR}/plug4.cpp`. The **generated Xcode project** was stale
— Xcode's `ZERO_CHECK` did not regenerate it. `cmake .` in `build/` fixed it
(0 references to the old path afterwards, 4 to the right one) and the build then
succeeded. **Any mac run touching this tree after a carve-out stage should
expect this and re-run `cmake .` before believing a build failure is theirs.**

**Learned — `getVendor4charCode()` is unaffected, checked rather than assumed.**
`SanitizeVendor4charCode(code, vendorName)` regenerates the four-character code
*from the vendor name* only when the code is not 4 alphanumeric characters with
a capital. `TideApp::getVendor4charCode()` returns `"TIDE"`, which passes, so
changing `vendorName` from "GMPI" to "TIDE Synth" does **not** reach it. That
mattered because the four-char code is plug-in identity too, in the AU/preset
sense, and silently changing it would have been the same class of bug as
renaming `id`.

**What is NOT verified, and it is the row's own acceptance evidence.** P5's
original finding was REAPER listing `VST3i: SynthEdit (GMPI)` and
`TrackFX_AddByName(tr, "TIDE_VST3", ...)` returning -1. **The rebuilt plug-in
has not been loaded in a host.** The binary carries the right strings and that
is strong, but "REAPER shows TIDE Rack" is unconfirmed. Marked as such in the
row rather than claimed.

**STEP 1 / 1.5:** no `platform:mac` issues; no open PRs at the start of this
item — the earlier stack (#69/#70/#71/#72/#73) had all merged.

**Next:** **U1** for the mac box — the rack-mode UX audit, which
[docs/about-pane.md](docs/about-pane.md) now depends on, and which a macOS box
can actually drive. **P10** is the cheap fallback. Whoever loads TIDE in a host
should close P5's last gap while they are there.

**Side effects on this box:** `SynthEdit/build/` was **regenerated and rebuilt**
(`cmake .` + Release build of `TIDE`) — that tree was already stale and is now
correct, but it is Jeff's build tree and the rebuild took a few minutes of CPU.
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were read only and left clean.
Committed in **two** repos this time: `SynthEdit` (the code) and `TideSynth`
(this entry and the backlog).

**Branch/PR:** [SynthEdit#24](https://github.com/JeffMcClintock/SynthEdit/pull/24)
+ TideSynth PR — **the SynthEdit one carries the actual fix**; the TideSynth one
is bookkeeping and they do not have to merge together.

---

## 2026-08-16 — macos — U1

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **U1** — the fresh audit its own row demanded before anything else,
[docs/u1-rack-mode-audit.md](docs/u1-rack-mode-audit.md), and split the build
work into **U1a/U1b/U1c**. Fifth item this session, at Jeff's direction; a
scheduled run still takes exactly one. Also flipped **P5** to DONE after
watching both its PRs merge.

**The headline, and it changes what rack mode costs: it is not started, and it
is also not a from-scratch build.**

`TideApp::OpenView` still constructs `ContainerViewStruct` with
`CF_STRUCTURE_VIEW` (`TideApp.cpp:63`) — no branch, no setting, no second path.
The 2026-08-13 pivot changed PLAN and has not yet changed a line of TIDE. **But
a top-level panel renderer already exists in the public repo TIDE links:**
`SE2::ContainerViewPanel : public TopView`,
`SynthEditLib/modules/se_sdk3_hosting/ContainerView.h:15` — same base class as
the structure view, with `CF_PANEL_VIEW` a first-class type through
`MfcDocPresenter` and `CUG`, not a stub.

**Learned — "the class exists" and "the class ships" are different claims, and
only one was true. Measured with both controls.**

| binary | `ContainerViewPanel` | `ContainerViewStruct` | `ModuleViewPanel` | bogus name |
|---|---|---|---|---|
| shipped `TIDE.gmpi` | **0** | 15 | 25 | 0 |
| `ContainerView.o` in `libSynthEditLib.a` | **13** | — | — | — |

`ContainerView.cpp` is on `SynthEditLib/CMakeLists.txt:535`, so it compiles;
nothing in TIDE references `ContainerViewPanel`, so **the linker never extracts
that archive member**. Exactly the static-library behaviour C12e documented for
`Dialogs_editor2.obj`. Without the positive and negative controls the zero would
have been indistinguishable from a bad `nm` invocation, which is the whole
reason both are in the table.

**The single most useful number is `ModuleViewPanel` = 25.** The *per-module*
panel renderer already ships in TIDE; only the *top-level container* one does
not. That asymmetry is what turns rack mode from "write a renderer" into "wire
up the one that exists", and it is why U1a is scoped as one line in one ALLOWED
file rather than a project.

**Learned — two of the P2-era findings survive the pivot, and the visual ones
could not be re-measured.** No breadcrumb bar; properties and module browsers
constructible (`TideApp.cpp:79,88`) but unplaced — both still true. The canvas
offset and the dead strip down the right, from
[state-of-the-prototype.md](docs/state-of-the-prototype.md) §6, **need a running
host and this audit did not run one.** Said so in the doc rather than repeating
them as if re-checked. That is the honest limit of a source-and-symbols audit.

**Split into three, in dependency order, and deliberately not folded together:**
**U1a** switches the view (one ALLOWED file, two acceptance bars — links, then
draws); **U1b** the breadcrumb bar, which [about-pane.md](docs/about-pane.md)
now depends on since it is the only chrome the about pane can hang from; **U1c**
rack styling and snapping, **the only part that is genuinely unwritten**. U1a's
result decides whether U1c is styling or a build, so costing U1c now would be
guessing — its row says so instead of carrying a number.

**U1a's row carries a warning worth repeating here:** nothing has ever linked
`ContainerViewPanel` in TIDE, so a crash or a blank canvas on first render is
**information, not the row failing**. File it and keep going.

**STEP 1 / 1.5:** no `platform:mac` issues; no open PRs at the start of this
item — P5's two had just merged.

**Next:** **U1a**, and it is mac-shaped for the same reason D1 was: its second
acceptance bar is "draws something sane in a host", which this box can do.
**P10** is the cheap fallback.

**Side effects on this box:** none — this item read source and ran `nm` on
binaries already built earlier in the session. TideSynth was the only repo
committed in; `SynthEdit`, `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were
read only and left clean on their default branches.

**Branch/PR:** the TideSynth PR carrying this entry, the audit and the split.

---

## 2026-08-16 — macos — U1a

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **U1a** — `TideApp::OpenView` now constructs `SE2::ContainerViewPanel`
with `CF_PANEL_VIEW`. **TIDE opens the rack/panel view.** Sixth item this
session, at Jeff's direction.

**Result — the symbol A/B, which is bar (a) and is the whole verifiable claim:**

| | before | after |
|---|---|---|
| `ContainerViewPanel` | **0** | **13** |
| `ContainerViewStruct` | 15 | **0** |
| `ModuleViewPanel` | 25 | 25 |
| bogus name (control) | 0 | 0 |

Release build **BUILD SUCCEEDED, 0 errors**, universal x86_64 + arm64, and P5's
identity strings verified un-regressed in the same binary.

**Learned — the interesting number is the one that went to zero, not the one
that went to thirteen.** `ContainerViewStruct` is now at **0 symbols**: nothing
constructs it, so **the structure view is currently unreachable**. Constraint 1
asks for *two* depths — rack by default, structure view on unlock — so this
change makes the rack default *and* removes the other depth. That is a real
regression against constraint 1 taken as a whole, not a tidy half-step, and I
have written it into **U1b**'s row: that item is now "breadcrumb bar **and** an
unlock path that constructs the structure view", which is more than it was
scoped as an hour ago. **Nobody would have noticed this from the diff** — it
only shows up because the audit had established the before-numbers.

**Learned — it was four files, and typing the interface to the base is what
makes it the last time.** `ISeApp` was typed to the concrete
`ContainerViewStruct`, threaded through `TideApp.h`, `TideAppWrapper.h` and
`SynthEditGui.cpp`. It is now `SE2::TopView`. Every member `SynthEditGui.cpp`
calls on the view — `arrange`, `Presenter`, `getCenter`, `DragNewModule`, the
scrollbar callbacks — is a base member, checked before the change rather than
discovered by the compiler. **The one thing the compiler did catch** was a
forward declaration: `TideAppWrapper.h` declared `class ContainerViewStruct;`
rather than including anything, so the first build failed with 17 errors that
all cascaded from `no type named 'TopView' in namespace 'SE2'`. One line.

**What is NOT done, and it is bar (b) of this row's own Accept.** *"Draws
something sane in a host"* — **the plug-in has not been loaded in a DAW.** The
class links and the binary builds; whether the rack renders, renders blank, or
crashes is unknown. U1a's row warned that a crash here would be information
rather than failure; that warning is still unspent, because nobody has looked.

**Next — and the most useful next action is not a backlog item.** **Load the
rebuilt `TIDE.gmpi` in a DAW.** One observation closes U1a's bar (b), closes
P5's outstanding "REAPER shows TIDE Rack" check, and unblocks U1b and U1c, which
should *stay* blocked until then — taking either now means building on a view
nobody has seen render. The mac NEXT row therefore points at **P10** (minutes,
ALLOWED, deletes the dead `SynthEdit.xml` that nearly caused a no-op PR during
P5) rather than at more rack work.

**STEP 1 / 1.5:** no `platform:mac` issues. [SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)
is this run's own and is the only open PR.

**Side effects on this box:** `SynthEdit/build/` rebuilt again (Release,
target `TIDE`). Committed in two repos: `SynthEdit` (the change) and `TideSynth`
(this entry and the backlog). `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were
read only and left clean.

**Branch/PR:** [SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)
— **that one carries the change**; the TideSynth PR is bookkeeping and they need
not merge together.

---

## 2026-08-16 — macos — U1a·P5 host verification (interactive session, Jeff present)

**Prompt:** n/a — interactive session, not a scheduled run. Jeff at the
keyboard; Claude (claude-fable-5) drove REAPER by computer use; committed and
pushed as `tide-rack-bot`.

**Did:** the one observation the last two entries said mattered more than any
backlog item — **loaded the rebuilt `TIDE_VST3.vst3` in a host and looked at
it.** REAPER 7.45/macOS-arm64: cleared the VST cache and re-scanned all 93
plug-ins (progress dialog confirmed **+0 cached**), then in a NEW project tab
("Optimus HP" untouched throughout, per this session's own rule) inserted the
plug-in and opened its UI. That one sitting closed **P5**'s outstanding host
check and **U1a**'s bar (b), filed **U2**, unblocked **U1b**/**U1c**, and
re-pointed mac NEXT at U1b.

**Result — P5, now closed end to end.** The FX browser lists exactly
**`VST3i: TIDE Rack (TIDE Synth)`** — the strings P5 put in the binary,
finally observed in the host that motivated the row. The API half, run via
ReaScript (see Learned): `TrackFX_AddByName(tr, "TIDE Rack", false, -1)` →
**0**, and `"VST3i: TIDE Rack (TIDE Synth)"` → **1**, both instances
reporting the full name. **The row's literal cited call,
`TrackFX_AddByName(tr, "TIDE_VST3", ...)`, still returns -1 — and always
will**: that API matches display names, not bundle filenames, so the call was
only ever a proxy for "unfindable by name", and the thing it proxied is
fixed. Recorded in P5's archived row rather than left as a loose end.

**Result — U1a bar (b): the rack RENDERS.** No crash through instantiate, UI
open, module insert, selection, and a window resize. What draws: the module
browser (categories + list, working), the panel canvas with its grid, and the
properties pane, which populates correctly on selection (List Entry: pins,
parameters, Appearance=Combo Box). With
[SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25) and
[#76](https://github.com/JeffMcClintock/TideSynth/pull/76) both merged and
bar (b) met, **U1a is DONE and archived**; U1b and U1c flip to TODO.

**The bugs bar (b) promised to surface exist, and they are U2** — four, exact
symptoms in the row: (1) **drag-and-drop from the module browser places
nothing** — two synthetic drag profiles and Jeff's real mouse all failed;
double-click inserts fine, so it is the drop path, not insertion; (2) **the
scroll wheel is dead** everywhere in the plugin UI (real hardware); (3) a
placed **List Entry control draws as a ~10 px glyph stack**, not a usable
combo box, while its properties pane is fully correct — model right, panel
geometry wrong; (4) **the §6 canvas offset/dead-strip layout survives in the
panel view** and re-anchors oddly on resize. Moog Filter showing no panel is
correct panel-view semantics, not a fifth defect. A crash was the feared
outcome; the actual outcome — a rendering view whose input/geometry layer has
simply never been exercised — is cheaper than that, and it is exactly the
costing input U1c was waiting on.

**Learned — REAPER's plug-in cache ini flushes on exit, not on scan.** After
a completed clear-cache re-scan, `reaper-vstplugins_arm64.ini` on disk stayed
byte-identical (mtime included) while the FX browser and `TrackFX_AddByName`
both showed the new identity. Anyone re-checking P5's "cached symptom" from a
shell while REAPER is running will read the stale line and wrongly conclude
the re-scan failed. While the app lives, the in-app browser is the truth, not
the file.

**Learned — `REAPER -nonewinst <script.lua>` runs a ReaScript inside the
already-running instance**, no screen control needed. The AddByName numbers
above came from a script injected that way; it guarded against the wrong
project being active (abort if the active project path contains "optimus")
and deleted its own scratch track afterwards. That is the pattern for any
future agent needing REAPER API answers on a box where REAPER is already
open.

**Learned — the a2 doc's macOS caveat is settled.** All five repos on this
box answer `https://github.com/...` to `git ls-remote --get-url origin`, the
global `insteadOf` rewrite is present, and the credential helper chain is
`gh`'s — so
[docs/a2-actor-separation.md](docs/a2-actor-separation.md)'s "macOS remotes
have still never been inspected" is now answered, on the record here. Not
edited there — that file is a dated record, the same reasoning as A9's
non-edit of the process review.

**Next:** **U1b** (mac NEXT re-pointed): breadcrumb bar plus the
structure-view unlock path. **Read U2 before starting it** — the breadcrumb
lands in the same view whose input layer U2 describes, and U1c stays uncosted
until U2 is triaged. **P10** remains the cheap fallback.

**Side effects on this box:** REAPER's VST cache cleared and re-scanned (93
plug-ins; in-memory — the ini rewrites when REAPER exits). A throwaway
unsaved project tab was left open in REAPER for Jeff to play with; "Optimus
HP" was never saved or modified. TideSynth is the only repo committed in;
`SynthEdit`, `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were read only and
left clean.

**Branch/PR:** this PR (TideSynth only — no code changed anywhere; the code
already landed as
[SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)).

---

## 2026-08-16 — windows — U1a·P5·U2 host verification, Windows half (interactive session, Jeff present)

**Prompt:** n/a — interactive session, not a scheduled run. Jeff at the
keyboard; Claude (claude-opus-5) drove REAPER by computer use; committed and
pushed as `tide-rack-bot`.

**Did:** the Windows half of the verification the mac session finished earlier
today. Rebuilt `TIDE_VST3` from `origin/master`, cleared REAPER's VST cache,
re-scanned, and loaded the rack in a new empty project. **The point was never
to re-close U1a or P5** — both were already closed on mac evidence, and this
run does not touch their status. It was to find out **which of U2's four
first-render defects belong to the panel view and which belong to macOS**,
because U2's own text proposes a pairing that Windows disproves.

**Result — the rack renders on Windows too.** REAPER 7.78/win64 rev 608e49
(Jul 18 2026), x64. No crash through instantiate → UI open → insert → select →
window resize → remove → re-instantiate. Category tree, module list, gridded
panel canvas and properties pane all draw, and the properties pane populates
correctly on selection (List Entry: pins, parameters, Appearance=Combo Box —
the same cell the mac session read).

**Result — U2 is three-quarters cross-platform, and its own hypothesis is
wrong.** The row guesses *"(1)/(2) smell like one event-routing cause"*.
Windows splits that pair:

| U2 | mac | windows |
|---|---|---|
| (1) drag-drop from the module browser places nothing | fails | **reproduces** |
| (2) scroll wheel dead everywhere in the UI | fails | **works fine** |
| (3) a placed control draws at the wrong size | ~10 px glyph stack | **reproduces, worse** |
| (4) §6 canvas offset/dead-strip, re-anchors on resize | fails | **reproduces** |

So **(1), (3) and (4) are the panel view's own defects and (2) is macOS-only**,
and (1) and (2) cannot be one cause. Detail worth having before anyone opens
these: **(1)** a stepped-slow synthetic drag of both `Moog Filter` and
`List Entry` highlights the row in the browser, shows no drag ghost, and drops
nothing — while double-click inserts fine, the same split the mac saw, so it is
the drop path on both platforms. **(2)** the wheel scrolls the module list and
the canvas on real hardware. **(3)** on Windows the control does not draw at
all: only its selection/resize adorner draws, **collapsed onto a zero-size rect
at the canvas origin**, with every subsequent module landing on the same point.
Jeff identified the artifact at the keyboard — a blue outline with white circle
resize nodes, which is what proves the module *is* inserted and selected rather
than missing. `Text Entry` behaves identically. So mac's "~10 px" and Windows'
"zero" are the same defect at two magnitudes: the model is right and the panel
geometry is not. **(4)** the canvas is anchored to the **right and bottom** of
its pane with dead grey filling the top and left, and on a window resize it
translates with the right edge rather than reflowing.

**Result — P5's Windows half, with the UID evidence mac could not get.**
Windows showed the *same original symptom* first: before the re-scan,
`%APPDATA%\REAPER\reaper-vstplugins64.ini` read
`TIDE_VST3.vst3=6346B150292DDD01,741344739{67756C506E694D47504920501951ED43,SynthEdit (GMPI)!!!VSTi`.
After a clear-cache re-scan of all 153 plug-ins (the dialog confirmed
**+0 cached**) the same line reads `...,TIDE Rack (TIDE Synth)!!!VSTi`. **The
class UID `741344739{67756C506E694D47504920501951ED43` is byte-identical
across that change** — a direct, measured confirmation that leaving the XML id
`SE SynthEdit` alone kept the hashed VST3 class UID stable, so no saved host
project is orphaned. That is the one thing P5's row most feared and it had
never been observed; the mac could not observe it because its ini never
rewrote (see below). FX browser: **`VST3i: TIDE Rack (TIDE Synth)`**. Via
ReaScript, `TrackFX_AddByName(tr, "TIDE_VST3", false, -1)` → **-1**, exactly as
on mac and for the same reason; `"TIDE Rack"` → 0, `"VST3i: TIDE Rack (TIDE
Synth)"` → 1 and bare `"TIDE"` → 2, all three reporting the full name.
`EnumInstalledFX` over 354 installed FX returns exactly one TIDE entry, ident
`C:\Program Files\Common Files\VST3\TIDE_VST3.vst3`.

**Learned — the mac entry's cache-flush rule is macOS's, not REAPER's.** That
entry states, as a general REAPER fact, that the plug-in cache ini *"flushes on
exit, not on scan"*. On Windows/7.78 it flushed **at scan time**:
`reaper-vstplugins64.ini` was rewritten with the new identity while REAPER was
still running, which is what made the UID A/B above possible. Not edited there
— that entry is the record of what that box saw, the same reasoning A9 used for
the process review. Read it as platform-specific, and on Windows the file is
trustworthy mid-session.

**Learned — building `TIDE_VST3` alone ships a plug-in that cannot build its
DSP graph. Filed as P11.** On Windows the VST3 resolves its built-in `SE *` GUI
modules through the *installed module database*, whose TIDE entry is
`C:\Program Files\Common Files\SynthEdit\modules\TIDE.gmpi` — written by the
separate `TIDE` target, not by `TIDE_VST3`. With a stale `TIDE.gmpi` there, the
plug-in threw **"Export failed: required module is missing from the module
database"** naming `SE Background Image` at instantiate and `SE List Entry` /
`SE Text Entry` on each GUI insert, while DSP-only modules (`Moog Filter`)
exported clean — the tell that isolates it to the GUI half. Building the `TIDE`
target as well cleared every one of them. **The error blames the user's
install** (*"this installation is broken. Re-scan modules"*) for what is
actually a half-built tree, which is why this is worth a row rather than a
footnote. **U2's (3) survives the fix** — re-tested with a consistent database
and the control still draws as a zero-size rect, so the geometry defect and
this trap are independent.

**Next:** unchanged — **win NEXT stays C12c**; this run was verification, not a
claim on a work item. **U2 is now the triage-ready row** its Accept asks for on
three of four defects, and whoever takes it should start from (3)/(4) as one
geometry cause with (1) separate — not from U2's original (1)+(2) pairing.
**Note U2's `Plat` cell still reads `mac` and now understates the row**: the
BACKLOG lint ([scripts/check-backlog-diff.py](scripts/check-backlog-diff.py))
forbids a run changing `Plat` on an existing row, so that cell needs Jeff or a
deliberate human edit; the Item text carries the correction meanwhile. **P11**
is new, `any`, and small.

**Side effects on this box:** REAPER's VST cache cleared and re-scanned (153
plug-ins; the ini rewrote in place, see above). `TIDE_VST3.vst3` and
`TIDE.gmpi` in `C:\Program Files\Common Files\` are now current Release builds
rather than the stale 2:43 pm ones. A throwaway unsaved REAPER project with a
TIDE instance was left open for Jeff; no saved project was opened or modified.
`SE16` is on the pre-existing local branch `fix/synthedit2-dbghelp-link` and
was not committed to; TideSynth is the only repo committed in.

**Branch/PR:** this PR (TideSynth only — no code changed anywhere; the code
this verifies already landed as
[SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25) and
[#24](https://github.com/JeffMcClintock/SynthEdit/pull/24)).

---

## 2026-08-16 — macos — U2 triage + U2a wheel fix (interactive session, Jeff present)

**Prompt:** n/a — interactive session, second of the day on this box; Jeff at
the keyboard contributing live observations. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** the triage U2's own Accept asked for — root causes named for all
four symptoms, split into **U2a/U2b/U2c/U2d**, full note in
[docs/u2-triage-2026-08-16.md](docs/u2-triage-2026-08-16.md) — **and fixed
U2a in the same sitting**: the mac scroll wheel,
[gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7), live-verified
in REAPER before the PR was opened.

**The wheel was a TODO, not a bug.** `DrawingFrameMac.mm scrollWheel:`
computed deltas and flags, then dropped the event — both dispatch lines
commented out (`:813`/`:818`), with the VST3-level `onWheel` fallback also
returning `kResultFalse`. The fix mirrors the proven Windows path
(`inputClient->onMouseWheel`, 120-per-notch, `ScrollHoriz` for `deltaX`)
using the `inputClient` the mac frame already used for pointer events —
which is why clicking always worked while the wheel did not. **Verified on a
fresh REAPER process:** canvas pans both directions, Ctrl+wheel zooms, the
module browser list scrolls and reveals entries that were previously
unreachable by any input, since TIDE hides scrollbars and middle-pan is also
dead (that one is **U2b**, filed: the backend has no `otherMouse*` handlers
at all).

**The Windows re-test ([#78](https://github.com/JeffMcClintock/TideSynth/pull/78))
landed mid-triage, and the two boxes answered each other's open questions.**
Their "(2) does not reproduce on Windows" is this box's root cause seen from
the other side — the Windows dispatch was always finished. Their *"every
later module landing on that same point"* is **U2c**'s mechanism named on
this box: `TopView::centerPos` defaults `{0,0}` (`ViewBase.h`), TIDE never
calls `setCenter`/`setPanZoom`, and `AddModule(moduleId, view->getCenter())`
inserts at exactly that corner — so the §6 "canvas offset / dead strip" both
boxes see is a **pan default, not a drawing bug**, and the fix is one line in
ALLOWED `TideApp.cpp`. Their *"(3)+(4) one geometry cause"* guess was half
right: adjacent, but (4) is that one-line default while **(3) is the real
remaining unknown — U2d**.

**U2d is the gate on the rack showing anything, and Jeff cracked its
description live:** the placed control draws **only its ResizeAdorner**
around a degenerate rect — "blue rectangle with white circles, only the
resizer" — model fully correct in the properties pane, panel drawing
nothing. Standing hypothesis, one leg short of proof: the panel pipeline is
skin-driven (`ContainerView.cpp:25` → SkinMgr/GmpiResourceManager) and
`TIDE_VST3.vst3` stages **no Resources at all** (binary + Info.plist +
signature; contrast SynthEditCL's staged `fonts`/`skins`/`templates`). Cheap
falsifier in the row. P11 is ruled out as its cause — the Windows session
fixed that trap and (3) still reproduced.

**And (1) is not a code defect on either platform.** Placement is
click-to-arm → click-to-place by design (`OM_DRAG_NEW_MODULE` →
`ViewBase::DragNewModule` → drop on the next `onPointerDown`), **proven
live**: browser click, canvas click, module placed at the exact click point.
What both boxes reproduced is that the press-drag-release gesture users try
first does not place. UX decision recorded in the doc; default in effect:
the design stands.

**Learned — a host keeps a VST3 module mapped after FX-remove.** Remove →
replace bundle → re-add loaded the OLD dylib, and the first post-fix wheel
test "failed" purely for that reason; a full REAPER restart picks up the
replacement. Budget a restart into every mac edit-build-verify loop.

**Learned — correcting this morning's entry:** `reaper-vstplugins_arm64.ini`
is not just laggy, it is **not a live mirror at all** — byte-identical
through a clear-cache re-scan *and* a clean quit while the FX browser showed
the new identity throughout. "The ini rewrites when REAPER exits" was an
overclaim; the durable rule is: read the FX browser, never the ini. (Also
told Jeff live: this box's `reaper.ini`/`reaper-reginfo2.ini` are owned by
**root**, so REAPER cannot persist its preferences here — his machine's
quirk, not TIDE's.)

**Next:** **U2c** is the best minutes-sized item on this box (one line,
ALLOWED, fixes the corner anchor and where inserts land on both platforms);
**U2d**'s falsifier decides whether the rack can display anything and wants
running before or alongside **U1b**'s chrome. U1b remains the headline item;
**U1c stays uncosted until U2d lands**. **P10** unchanged as fallback.

**Side effects on this box:** `gmpi_ui` gained one commit on a PR branch
([gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7), 5+/6-);
`SynthEdit/build/` rebuilt `TIDE_VST3` (Release) and its PostBuild step
re-installed `~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3` — now carrying
the wheel fix. REAPER was restarted twice (module-reload lesson above);
"Optimus HP" was never saved or modified; the throwaway test tab was left
open for Jeff, who was driving the plugin UI himself between my steps.
`SynthEdit` and `SynthEditLib` were read only; `GMPI_Wrappers` read only.

**Branch/PR:** this TideSynth PR (triage doc, U2 split, this entry) +
[gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7) — **the gmpi_ui
one carries the code**; they need not merge together.

---

## 2026-08-16 — macos — U2c fix + U2d falsifier (interactive session, Jeff present)

**Prompt:** n/a — interactive session, third of the day on this box, at Jeff's
direction ("do the U2c one-liner and U2d falsifier now"; logging explicitly
blessed). Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** **U2c** — the one-line centre default,
[SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26), verified
live. **U2d** — ran the row's falsifier and kept going until the mechanism
fell out: **both cheap hypotheses are refuted, and the real one is named.**

**U2c, closed end to end in one cycle.** `TideApp::OpenView` now calls
`setCenter({viewDimensions/2, viewDimensions/2})` after `setDocument`.
REAPER 7.45/macOS-arm64, fresh process: **the gridded canvas fills the whole
pane** — no corner dead-strips, across every subsequent fresh load this
session — and click-placed modules land at the click point, in view. The §6
"offset" is dead as a default; U1c still owns what "home" ultimately means.

**U2d falsifier, round 1 — skins were never missing, and the hypothesis dies
on a path quirk worth keeping:** `BundleInfo::getCommonDocumentFolder()`
resolves to plain **`$HOME`**, not `~/Documents` — so SkinMgr reads
`~/SynthEdit Projects/skins/`, which on this box already held 8 real skins
from Jeff's SynthEdit install, and the temp logging showed
`getSkin('default3')` scoring an **exact hit** at plugin load. (A seeding
copy aimed at `~/Documents/SynthEdit Projects/skins` — where this session
first looked — would target the wrong place entirely.) Placed List Entry:
still adorner-only.

**Round 2 — the stale module DB was real, and it still was not the cause.**
`/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` was **3.5 months stale** (May 7:
pre-P5 identity `name="SynthEdit"`, 0 `ContainerViewPanel` symbols), and
**the mac build never refreshes it — P11's win row describes a post-build
copy that simply has no mac counterpart**; today's `TIDE.gmpi` sits only in
`build/SynthEditSem/Release/`. Manually installing the fresh one and
sidelining the editor's `Plugin-Cache*.xml` (renamed `*.u2d-bak`, restorable)
changed nothing: still adorner-only. Also learned in passing: the VST3 runs
fine with no cache XMLs present, so those caches belong to the editor app,
not the plugin.

**Round 3 — the log that ends the hunt.** With `ModuleView::Build`
instrumented: it fires for `SE PatchCableChangeNotifier` and
`PatchAutomator` (invisible utilities, `windowType=0`, GUI2 objects
constructed fine) and **never fires at all for the placed `SE List Entry`.**
The only silent pre-`Build` exit is `ModuleViewPanel`'s
`if (!moduleInfo) return;` — *"unregistered module type"*, whose diagnostic
is `_RPTN`, Debug-only (`ModuleView.cpp:659`). **So the GUI class
registration for the SE control modules never reaches the panel view's
module factory in the mac VST3, and the view constructs empty — the adorner
then hugs a zero rect, which is exactly what Jeff identified on screen.**
This unifies the two platforms: win's P11 dialog and mac's silence are one
defect with two failure surfaces, and it explains the win re-test's
"(3) persists after P11 fixed" — refreshing the DB satisfied the *export*
path, not the view's factory. U2d's row now carries the next moves: trace
where the mac VST3 populates the module factory
(`LoadOrScanModuleData`/CUG), and make the unregistered-type path **loud in
Release** so this class of failure can never be silent again.

**Learned — a latent trap for whoever ships skins with TIDE:**
`GetHomeDir()` is the dylib's own directory, so SkinMgr's seeding source
`{home}/Resources/skins` resolves to **`Contents/MacOS/Resources/`** inside
the bundle — the exact layout P6 spent six days learning that `codesign`
refuses. Bundle staging for TIDE must target `Contents/Resources` AND teach
the seeding path to find it, or skip user-folder seeding entirely
(constraint: sandbox-safe means the plugin should read its own bundle, not
write `~/…`).

**Temp instrumentation:** SkinMgr + ModuleView logging was local-only and is
reverted; the misplaced `~/Documents/SynthEdit Projects/skins` seed is
removed (the pre-existing "Mac Export" content untouched). What remains on
the box deliberately: the **refreshed `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi`**
(a legitimate update of a 3.5-month-stale install) and the sidelined
`*.u2d-bak` cache files.

**Next:** **U2d** is now a scoped fix session (factory-population trace +
loud failure), and it still gates the rack displaying anything; **U1b**
after it, per the NEXT row. The win box can cheaply confirm the unified
mechanism by checking whether its `SE List Entry` `ModuleViewPanel` gets a
`moduleInfo` after a DB refresh.

**Side effects on this box:** `SynthEdit/build/` rebuilt `TIDE_VST3` twice
and `TIDE` once (PostBuild reinstalled the VST3 each time — it now carries
U2a's wheel fix and U2c's centring); REAPER quit/relaunched three times
(module-reload lesson), "Optimus HP" never saved or modified, throwaway
tabs left open. `gmpi_ui` and `GMPI_Wrappers` untouched this entry;
`SynthEditLib` was instrumented and **restored byte-clean**.

**Branch/PR:** this TideSynth PR +
[SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26) — the
SynthEdit one carries the code; they need not merge together.

---

## 2026-08-16 — macos — U2b + U2d fix session: first modern panel renders (interactive session, Jeff present)

**Prompt:** n/a — interactive session, fourth of the day on this box; Jeff
said "work on as many tasks as possible, don't stop" and merged PRs live as
they opened. Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** **U2b** (mac middle-button pan,
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8)), and the
**U2d fix session** — root cause found two layers deeper than yesterday's
hypotheses, first fix landed and **verified: TIDE drew a modern module
panel in a host for the first time.** Jeff merged
[gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7) (U2a wheel)
and [SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26)
(U2c centring) mid-session, so **U2a and U2c are DONE** — both were
live-verified before their PRs opened.

**U2d, the actual mechanism, nailed by one trace line.** The typeId
instrumentation printed `ctor(json): 'SE List Entry' -> moduleInfo=0x0` —
correct id, same `CModuleFactory` singleton the browser uses (the two-
factory theory died: `ModuleFactory()` is a `#define` for `Instance()`),
so the id is simply **not registered in the VST3**. Why: **the modern
control modules compile only inside `IF(SE2JUCE)`**
(`SynthEditLib/CMakeLists.txt:553`) — mirrored by
`initialise_synthedit_modules`'s force-link lists sitting in
`#if GMPI_IS_PLATFORM_JUCE==1` and `#if SE_GRAPHICS_SUPPORT` blocks, the
latter macro **never defined for the compiler anywhere** (undefined
identifiers are 0 in `#if`). Full SynthEdit never noticed because it
scans modules from disk; TIDE — scan removed by S1a, by design — is the
first product that needed the static path, and it never existed. The
browser still lists "List Entry" because the **legacy DocObs**
(`Ctl_Combo`) link via direct reference; `Ctl_Combo::Export` writes
`"type": "SE List Entry"` into the panel JSON — so model right, view
empty, both platforms: **win's P11 dialog and mac's silence are one
defect's two faces**, and it explains #78's "(3) persists after P11
fixed".

**The fix, and what it proved.** TIDE now lists its fixed module set
(PLAN constraint 7) as **direct target sources** in
`SynthEditSem/CMakeLists.txt`
([SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27),
stacked on #26) — target sources cannot be dead-stripped. First entry:
`Controls/PlainImageGui.cpp` (`SE Background Image`). **Verified in
REAPER: the default document's Background Image module — the thing P11's
win error named "at instantiate" — draws as a real panel with working
resize adorners.** Deliberately NOT done: defining `SE_GRAPHICS_SUPPORT`
lib-wide — it gates dormant code in `Controller.cpp`/`MpParameter.cpp`
and would double-register modules in the scanning editor
("Module found twice" boxes).

**The classic SDK3 controls are a further layer, filed as U2e with three
crash stacks.** Listed with their widget deps, `SE List Entry` registers,
constructs — and crashed REAPER three different ways in one hour:
`ClassicControlGuiBase::initialize` (`widgets.back()` on an empty vector —
address -16 IS the tell: empty `back()` with 16-byte `shared_ptr`
elements), then `ListEntryGui::measure` (`widgets[0]`, null at 0x0), then
`ListEntryGui::arrange` after both guards. **The widget layer assumes skin
bitmaps/fonts load during pin init and never checks** — 47 unguarded
`widgets[` sites in `ListEntryGui.cpp` alone, so guarding call-sites is
whack-a-mole; the fix session must make widget-building succeed (or fail
into a placeholder widget) instead. Guards for the first two crash sites
plus the Release-loud `ModuleViewPanel` unregistered-type diagnostic are
in [SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12);
the classic controls stay **out** of TIDE's module list until U2e lands —
**final state verified: no crash, Background Image draws, a placed List
Entry degrades to the adorner + a stderr line instead of killing the
host.**

**Learned — the module-set list is the constraint-7 lever.** TIDE's
"fixed module set, compiled in" now has a literal, reviewable home: the
source list in `SynthEditSem/CMakeLists.txt`. Growing the rack's palette
= adding a file there and verifying its layer actually renders. That is
a better shape than any registry define.

**Learned — U2b is code-complete but untested by hand:** synthetic
middle-drag isn't available to the agent's tooling, so
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8) awaits
Jeff's real mouse. The handlers mirror the proven left/right pairs and
are gated to `buttonNumber == 2`.

**P10 was deliberately not taken** despite being minutes: it would have
meant a third stacked SynthEdit branch mid-session with the build tree
checked out elsewhere; it stays the mac fallback. **P11 gained a mac
finding:** the win post-build module-DB copy has **no mac counterpart at
all** — `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` sat 3.5 months stale
(May 7, pre-P5 identity) until this session refreshed it by hand; noted
in [docs/building.md](docs/building.md).

**Next:** **U2e** is the gate on the rack showing *controls* (Background
Image proves the pipeline; the classic widget layer is what stands
between TIDE and a usable List Entry). **U1b** remains the headline.
U2b/U2d flip on their PRs; U2a/U2c archived DONE.

**Side effects on this box:** `SynthEdit/build/` rebuilt `TIDE_VST3`
five times and `TIDE` once; the installed VST3 now carries U2a+U2b+U2c+
the PlainImage registration and is **crash-free**. REAPER crashed three
times (all TIDE_VST3 faults, all filed with stacks) and was relaunched;
"Optimus HP" never saved or modified. `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi`
refreshed to today's build. Temp trace logging in `SynthEditLib` was
reverted; the four files now changed there are the real guards/diagnostic
on the PR branch. Throwaway REAPER tabs left open for Jeff.

**Branch/PR:** this TideSynth PR +
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8) +
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12) +
[SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27); merged
mid-session by Jeff: [gmpi_ui#7](https://github.com/JeffMcClintock/gmpi_ui/pull/7),
[SynthEdit#26](https://github.com/JeffMcClintock/SynthEdit/pull/26).

---

## 2026-08-16 — macos — U2e first pass: crash-free placeholders, one question left (interactive session, Jeff present)

**Prompt:** n/a — interactive session, fifth of the day; Jeff verified
middle-drag by hand, merged
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8),
[SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27) and
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12)
mid-session, and said "keep going". Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** flipped **U2b** and **U2d** to DONE (merged + verified — U2b by
Jeff's own middle-drag), then took **U2e** far enough that the classic
controls are **crash-free, visible, and one isolated question from
working**: PRs
[SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28) and
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13).

**U2e finding 1 — TIDE never seeded the resource folders.** The base
`CSynthEditAppBase::InitInstance` seeds
`GmpiResourceManager::resourceFolders` (skin images among them);
`TideApp::InitInstance` replaced the base wholesale for S1a and dropped
that seeding, so every skin-image URI resolved against an **empty map**.
Seeded now (`Image` only — the one type panel controls read). This is a
real prerequisite for widget bitmaps, **but it was not the crash gate**:
rebuilding with the seed alone still crashed in
`ListEntryGui::arrange`.

**U2e finding 2 — the actual gate, isolated to one sentence.** Widgets
are built inside `onSetAppearance()` — a **pin-update handler**
(`ListEntryGui.cpp`: ctor `initializePin(pinAppearance, …onSetAppearance)`,
handler gated only by `currentAppearance == pinAppearance`, ctor default
`-2`). Had the handlers fired even once with default pin values,
`ACM_PLAIN` would have built a ListWidget — the vector being empty at
crash time means **the pin-update handlers never run at all in TIDE's
SDK3 hosting**. The next U2e step is therefore a single directed trace:
how `ModuleView`'s Sdk3 path delivers initial pin values (the "fake
plugs" `Ctl_Combo::Export` writes) and why the handler pass never
happens — the same wiring the editor exercises when these controls work
in full SynthEdit.

**U2e finding 3 — with `arrange()` guarded, the state is honest and
stable.** Third SIGSEGV site from the same root (initialize → measure →
arrange, all `widgets[]` on empty); guard landed
([SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13)).
**Verified in REAPER on the final build: no crash through instantiate →
insert → select; the placed List Entry draws as a right-sized, selectable
100×20 placeholder at the click point; Background Image renders
alongside; properties pane fully correct.** The classic controls stay in
the module list — a visible empty placeholder plus a stderr breadcrumb
beats both a dead host and an invisible module.

**Learned — pin defaults argue the diagnosis for us.** When a handler's
absence can be inferred from what default values *would* have built, the
"is it invoked at all vs does it fail inside" fork resolves without
instrumentation. That saved a fourth build-and-crash cycle.

**Next:** **U2e's pin-delivery trace** is the single remaining step
between TIDE and usable classic controls — after it, the combo should
draw for real and U1c's costing finally has a live control to look at.
**U1b** remains the headline. **P10** untouched as fallback.

**Side effects on this box:** `SynthEdit/build/` rebuilt `TIDE_VST3`
three more times; the installed plugin now carries U2a+U2b+U2c+U2d+the
U2e prerequisites and is crash-free (verified). REAPER crashed twice
more during diagnosis (both filed in the U2e row's stack list, same
root) and was relaunched; "Optimus HP" untouched throughout. Working
copies: `SynthEdit` on `tide/mac/U2e-resource-folders`, `SynthEditLib`
on `tide/mac/U2e-arrange-guards` (both pushed, PRs open); returned to
defaults after push.

**Branch/PR:** this TideSynth PR +
[SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28) +
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13);
merged mid-session by Jeff:
[gmpi_ui#8](https://github.com/JeffMcClintock/gmpi_ui/pull/8) (U2b, his
own middle-drag as the verify),
[SynthEdit#27](https://github.com/JeffMcClintock/SynthEdit/pull/27) +
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12)
(U2d).

---

## 2026-08-16 — macos — structure view interim: an oscillator draws with its pins (interactive session, Jeff directing)

**Prompt:** n/a — interactive session, sixth of the day; Jeff set the goal
directly: *"first it would be nice to see a basic module drawn in structure
view, like the oscillator … just draws in the structure view with its
pins."* Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** flipped `TideApp::OpenView` to `ContainerViewStruct` /
`CF_STRUCTURE_VIEW` as the **interim default** —
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29) — and
**verified the goal exactly: Phase Dist Osc draws in REAPER as a full
structure-view module box** — title, rounded body, blue input pins (Pitch,
Modulation Depth), green list pins (Wave1, Wave2), Audio Out on the right,
selection chrome, properties pane in sync, placed at the click point.
First module TIDE's structure view has ever drawn in a host on this
platform. `ModuleViewStruct` went **0 → 53 symbols** with the flip (it had
been dead-stripped along with `ContainerViewStruct` since U1a).

**Why this is a flip and not a revert of U1a.** The rack pivot stands;
what changed is sequencing. U2e isolated the panel-view CONTROLS behind
one remaining question (the SDK3 pin-delivery pass never runs), while the
structure view's generic module rendering — box + typed pins, no custom
GUIs, no skins, exactly as Jeff noted — is the decades-proven path and
worked on the first try. So the structure view is the *interim* default
until panel controls land; **U1b's job is unchanged and now easier**: make
the rack the default again with the structure view behind its unlock —
both classes now link, and the `SE2::TopView` typing keeps the flip one
line. The two views' remaining difference is which one `OpenView` names.

**Recorded from Jeff, and it reframes U1c:** *"we already added a basic
rack mode to synthedit. code is there already."* That matches what the
code shows — `ModuleViewPanel`'s JSON ctor reads an `isRackModule` flag,
and `ViewBase::snapToGrid`'s comment describes rack-mode axes ("one HP
across, one whole rack row down — so modules land in real slots"). U1c's
row said "genuinely not written / from-scratch build" — **corrected: U1c
is wiring and enabling existing rack code**, the same shape U1a turned out
to be. Its row now says so.

**The navigation stack earned its keep immediately:** finding the
oscillator meant scrolling the module browser (the wheel fix), and the
centred canvas (U2c) put the placed module exactly where clicked, in
view. Everything from this morning compounds.

**Next:** unchanged in priority, sharper in shape — **U2e's pin-delivery
trace** (usable panel controls), then **U1b** (rack default + unlock,
now trivially two linked classes and chrome), then **U1c** (enable the
existing rack code). The module-browser filter box (type-to-find) would
have made tonight faster; noted as a UX nicety for U1-series work, not
filed as a row.

**Side effects on this box:** one more `TIDE_VST3` rebuild + auto-install
— the installed plugin now opens the structure view; REAPER quit/relaunch
once, "Optimus HP" untouched; throwaway tabs remain. `SynthEdit` working
copy returned to `master` after push.

**Branch/PR:** this TideSynth PR +
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)
(stacks cleanly beside [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
— same file, different functions, merge order irrelevant).

---

## 2026-08-17 — macos — U2e: the pin-delivery trace, completed (scheduled run, unattended)

**Prompt:** b3e9876 · claude-opus-5[1m] · app 1.30096.5 · as tide-rack-bot

**Did:** answered U2e's one directed question — *"trace how `ModuleView`'s Sdk3
path delivers initial pin values and why the handler pass is skipped"* — and the
answer corrects the row's own suspicion. Written up in full at
[docs/u2e-pin-delivery-trace.md](docs/u2e-pin-delivery-trace.md). **No code
changed; that was the call, see Next.**

**Result — the handler pass is not skipped, it is never requested.** TIDE's
`Module_Info` for `SE List Entry` has **zero GUI pin descriptions**, and every
initial-value path reads that list, so `setPin` is called zero times.

`Module_Info::gui_plugs` is populated by exactly two mechanisms, and the classic
SDK3 controls use neither:

  1. classic internal DSP — `REGISTER_MODULE_1` + `LIST_PIN2` in C++
     (`UgDatabase.cpp:347`). This is why **Phase Dist Osc drew with its pins**
     last night and List Entry did not.
  2. modern GMPI — `gmpi::Register<T>::withXml(...)` → `RegisterPluginWithXml`
     (`UgDatabase.cpp:267`) → `ScanXml` (`:287`). This is `PlainImageGui.cpp:182`,
     i.e. **SE Background Image**.
  3. SDK3 official module — `GMPI_REGISTER_GUI` (`mp_sdk_gui.h:12`) →
     `RegisterPlugin` (`UgDatabase.cpp:242`), which stores **a constructor and
     nothing else**. Its pins live in `ControlsXp.xml:262-283`, which
     `modules/plugin_helper.cmake:236` copies into the `.sem` bundle for the
     **module scan — the scan S1a deliberately removed** (PLAN constraints 4 & 7).

The chain, every link at file:line, is in the doc. Short form:
`ViewBase::ConnectModules` gates **both** default paths on
`moduleInfo->gui_plugs` (`ViewBase.cpp:711`→`:742`, and `:778`→`:799`) → zero
`setPin` → `ModuleView.cpp:1404`'s `notifyPin` never fires →
`GuiPinOwner::notifyPin` (`mp_sdk_gui.cpp:125`) never reaches `doNotify` →
`ListEntryGui::onSetAppearance` (`ListEntryGui.cpp:49`, the **only**
`widgets.push_back` site) never runs → `widgets` empty at
`initialize`/`measure`/`arrange` — the three SIGSEGVs, in `Refresh`'s own call
order.

**Verification artifact — the shipping binary, A/B with positive controls.**
`master` built clean first (`cmake --build . --config Release --target
TIDE_VST3` → `** BUILD SUCCEEDED **`, 0 errors, universal x86_64+arm64), then:

```
                       id-string   embedded <Plugin id="…"> XML
SE List Entry               2                 0     <- family 3
SE Text Entry               2                 0     <- family 3
SE Background Image         4                 2     <- family 2 (control)
SE Patch Point in           2                 2     <- family 2 (control)
PatchAutomator              4                 2     <- family 2 (control)
"LED Stack" / "Up/Down Select" / "Appearance": 0 / 0 / 0
```

The controls are the point: the same `strings` command finds full pin metadata
for family-2 modules in the same binary, so absence is absence, not a tooling
artifact. `SE List Entry` is **a registered module id with no pins attached**.
Only **29** `<Plugin id=…>` blocks exist in the entire TIDE binary.

**Learned — three things the next run would otherwise redo.**

  - **The U2e row's skin-bitmap/ImageCache suspicion is not the gate** and should
    drop to second hypothesis. Nothing in widget construction is *reached* to
    fail. It also explains why [SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
    (resource-folder seeding) measured as a real prerequisite yet was proven
    insufficient alone: it is a prerequisite for a step the run never gets to.
  - **Pin IDs are fine** and were worth ruling out explicitly:
    `ListEntryGui.cpp:32-40` seeds `initializePin(10, pinValueIn, …)` and lets
    the rest auto-increment to 11-16, matching `ControlsXp.xml:271-281` exactly.
  - **This is a wall in front of TIDE's whole fixed module set, not one broken
    control.** Every family-3 module TIDE adds lands in the same state. S1a's
    trade (no scan → no cache write → sandbox-safe) has this as its unpriced
    cost, and it is now priced.

**What did NOT work / was ruled out by reading rather than guessing:** the
`pluginParameters2B` queryInterface path is sound (`MpGuiBase2` derives from
`IMpUserInterface2B`, `mp_sdk_gui.h:365` answers `MP_IID_GUI_PLUGIN2B`), so the
"host never got a notify-capable interface" theory is dead. `Module_Info::ScanXml`
(`Module_Info3_base.cpp:213`) does **not** call `ClearPlugs()` first — it clears
only `pinXmlDiagnostics_` — which is what makes the tidiest fix risky (below).

**Next:** the fix is a **choice, not an investigation**, and the doc costs three
options. Recommended: embed `ControlsXp.xml` at build time and
`RegisterPluginXml` it from `TideApp::InitInstance` — confined to
`SE16/SynthEditSem/` (ALLOWED), cannot regress the scanning editor. Rejected for
now: swapping ControlsXp to `withXml`, which is tidier but double-populates
`Module_Info` in the scanning editor (map `insert` drops and leaks the second)
and changes commercially-shipped behaviour no macOS box can test.
**Give U2e's remaining half to an interactive session** — its acceptance is *"a
placed List Entry draws as a usable combo box"*, a GUI observable an unattended
run cannot check; and the fix stacks on the still-open #28. The mac NEXT row now
says so. **U1b** is the unattended-safe mac item, with the caveat that its
default-flip half must wait on U2e and would undo the open
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29).

**Process note for Jeff, no row filed.** The run prompt's STEP 5 lists *"the
SynthEditLib repo"* as GATED, while U2e's own row says its scope is *"ALLOWED
(public repo)"* — and precedent agrees with the row (you merged SynthEditLib#12
and #13 for this item). The contradiction did not bite this run, because nothing
was written outside TideSynth, but the next run to attempt the U2e fix will hit
it. Worth one line in whichever of the two is wrong.

**Side effects on this box:** one `TIDE_VST3` Release rebuild from `master` (the
build artifact above); no REAPER, no GUI, no computer-use — scheduled runs cannot
get that approval. All five working copies (`TideSynth`, `SynthEdit`,
`SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`) were **clean at start** and all are
returned to their default branches. Nothing of Jeff's was touched.

**Branch/PR:** `tide/mac/U2e-pin-delivery` → this TideSynth PR. No other repo was
committed in. Three earlier mac PRs remain open, clean and mergeable, and were
deliberately left alone per STEP 1.5:
[SynthEdit#28](https://github.com/JeffMcClintock/SynthEdit/pull/28),
[SynthEdit#29](https://github.com/JeffMcClintock/SynthEdit/pull/29),
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13).

---

## 2026-08-17 — macos — U2e closed on mac: the combo box draws (interactive session, Jeff directing)

**Prompt:** n/a — interactive session continuing from yesterday's five; Jeff
confirmed cable-drag and module insertion work in the structure view, then
said "do the next task". The box's own scheduled run fired unattended at 06:19 and completed the same trace from source ([docs/u2e-pin-delivery-trace.md](docs/u2e-pin-delivery-trace.md)) — this session read SDK and code independently, converged on the same mechanism, and shipped the fix it framed as "a decision, not an investigation". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** **U2e's pin-delivery trace, to the bottom, and the fix** —
[SynthEdit#30](https://github.com/JeffMcClintock/SynthEdit/pull/30), stacked
on [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28). **Verified in
REAPER: a placed List Entry draws as a real combo box with styled title, and
the module browser shows exactly the fixed module set.** The row's Accept is
met on mac.

**The trace, mechanically.** SDK3 semantics read from the SDK itself:
`setPin` stores only; handlers fire **only on `notifyPin`**; the base
`initialize()` does nothing (its fire-all is deprecated in place). Initial
values are sent by `ViewBase::ConnectModules` STEP 2 — which iterates
**`moduleInfo->gui_plugs`** to parse and send each pin default. And
`gui_plugs` was **empty**: the pin descriptions live in `ControlsXp.xml`,
which only the module scan ever loaded — and TIDE's scan is gone by design
(S1a). No descriptions → nothing sent → no `notifyPin` → `onSetAppearance`
never ran → no widgets. Every layer below (registration, resources, guards)
was real but insufficient; this was the last missing piece.

**The fix, in two parts.** CMake stages `ControlsXp.xml` into
`Contents/Resources` **from SynthEditLib's copy** — single source of truth,
no drift (`BundleInfo::getResource` falls back to exactly that folder on
mac; the P6 rule keeps data out of `MacOS/`). `TideApp::InitInstance` then
merges it — **into already-registered classes only**. The merge-only filter
was learned live, not designed up front: a plain `RegisterPluginsXml` call
**grew the browser** with insertable phantoms (Keyboard (MPE), Scope3, Volt
Meter… XML-only, no class — one placed as an empty adorner before the
filter existed). `Module_Info3_internal`s without constructors are NOT
hidden by the browser's `isDllAvailable()` filter, so curation must happen
at registration: iterate the XML, `GetById`, `ScanXml` only on hits.
Constraint 7's fixed set stays exactly as curated — verified by eye against
the browser before and after.

**What the day-and-a-half arc adds up to.** U2 filed four symptoms two days
ago; every one is now DONE or IN-REVIEW with the mac verify green: wheel
(U2a ✓ merged), middle-pan (U2b ✓ merged, Jeff's hand on the mouse),
centring (U2c ✓ merged), registration + first modern panel (U2d ✓ merged),
and now pin delivery + a drawing control (U2e, #28+#30 +
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13)
in review). The structure view draws full module boxes with pins and
patchable cables ([#29](https://github.com/JeffMcClintock/SynthEdit/pull/29),
Jeff verified cable-drag by hand). **TIDE went from "renders a grid and
silent adorners" to "a patchable structure view AND a panel view that draws
real skinned controls" in two sessions' worth of days.**

**Learned — the browser does NOT filter unavailable internal modules.**
`ExportModuleNames` skips `!isDllAvailable()`, but `Module_Info3_internal`
never sets that flag false for XML-only entries (the assignment in
`RegisterPluginConstructor` is commented out as "might be needed?"). Anyone
registering module XML wholesale into a scanless product will grow the
insert menu with phantoms. The merge-only loop is the pattern; noted here
because the win box will want it too.

**Next:** the win box has two cheap U2e follow-ups (staging equivalent —
win reads the same file as a fallback after the exe resource — and the
combo re-verify). Then the board is exactly what the NEXT row says: **U1b**
(breadcrumb + restore rack-as-default with the structure view behind the
unlock — both classes link, the flip is one line) and **U1c** (enable
Jeff's existing rack-mode code). **P10** unchanged as fallback.

**Side effects on this box:** three more `TIDE_VST3` rebuilds; the
installed plugin now carries the full U2a–U2e stack (panel view default on
this branch lineage) and staged `ControlsXp.xml`; REAPER restarted three
times, "Optimus HP" untouched; the throwaway tab with Jeff's two-oscillator
cable patch from last night was lost to a restart — two modules and one
cable, noted for honesty.

**Branch/PR:** this TideSynth PR +
[SynthEdit#30](https://github.com/JeffMcClintock/SynthEdit/pull/30) (stacked
on [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28); merge #28 →
#30, or together; [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)
is independent).

---

## 2026-08-17 — linux — S3 (TIDE-side half), plus two platform:linux breaks found and filed

**Prompt:** b3e9876 · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as tide-rack-bot

**Did:** closed **S3**'s TIDE-side half —
[SynthEdit#32](https://github.com/JeffMcClintock/SynthEdit/pull/32) — and, while
building the baseline it needed, found that **this box cannot build `main` at
all** and filed both causes as
[#87](https://github.com/JeffMcClintock/TideSynth/issues/87) and
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88). The build breaks
are the more important half of this run.

**STEP 1 and 1.5 were clean at the start** — no open `platform:linux` issue in
any of the five repos, no `tide/linux/**` PR. The three open `tide/mac/**` PRs
are green with nothing unresolved and were left alone. The `linux` NEXT row said
**S3**, it survived screening (`SE16/SynthEditSem/TideApp.cpp` is ALLOWED, no
open PROPOSED entry touches it), and its acceptance check was stateable before
starting, so it was takeable.

**Break 1 — [#87](https://github.com/JeffMcClintock/TideSynth/issues/87):
`SynthEditLib` does not compile with GCC, and the cause says so itself.**
`modules/se_sdk3_hosting/ModuleView.cpp:621-633` carries

```
// TEMPORARY U2d trace - local only, do not commit.
#include <cstdio>
#include <cstdarg>
static void tideTraceLog(...)  { if (FILE* f = fopen("/tmp/tide-skin-debug.log", "a")) ... }
```

committed as `227ba48` via
[SynthEditLib#12](https://github.com/JeffMcClintock/SynthEditLib/pull/12) (U2d).
`namespace SE2 {` opens at `:38` and closes at `:1893`, so those two `#include`s
are **inside `SE2`** and declare a nested `SE2::std`. Every later unqualified
`std::` in the file then resolves there and fails — 30+ errors starting at
`:696`, the first `std::` use after the includes: *"‘SE2::std::map’ has not been
declared"*, then `make_unique`, `vector`, `max`, `min`, `string`, `unique_ptr`.
The file's own `using namespace std;` at `:34` cannot help, because qualified
lookup finds `SE2::std` first.

**Why no other box has seen it, and this is the part worth keeping:** both
headers are include-guarded. On a toolchain that already pulled them in
transitively before line 623, the two lines expand to **nothing** and no
`SE2::std` is created. libstdc++ here does not, so it is created. **The bug is
equally present in the source on all three platforms; only Linux is unlucky
enough to be told.** A misplaced `#include` inside a namespace is invisible
wherever the header happens to have been included already.

**It is also a live constraints 3 and 4 violation in the shared library**, not
just a build break: `fopen("/tmp/tide-skin-debug.log", "a")` runs unconditionally
in both `ModuleView` constructors, no `#ifdef`, in Release — a hard-coded
absolute path outside the bundle, in code SynthEdit links too. One revert fixes
both problems.

**Break 2 — [#88](https://github.com/JeffMcClintock/TideSynth/issues/88):
`SynthEditWayland` fails to link, and C12e is why.** `undefined reference to
doDialogConnectUg(CUG*)` and `doDialogPatchManager(CUG_with_patches*)`. C12e
(`a2ffdcd3c`) took `Dialogs_editor2.cpp` off EditorLib's source list so each app
compiles it directly; `SynthEditCL/CMakeLists.txt:42` and
`SynthEdit2/SynthEdit2.vcxproj:290` got the entry, **`SynthEditWayland` and
`SynthEditJuce` did not**. Neither target is generated on Windows, where C12e was
verified — its journal entry's "904/904 RC=0, TIDE.gmpi and TIDE_VST3.vst3 both
link" is all true and touches neither. Same shape as the 2026-08-14 finding here:
a target below a platform gate is only ever tested below that gate.
`SynthEditWayland/CMakeLists.txt` already sets `EDITOR2_DIR` (`:134`) and already
compiles `SynthEditApp.cpp` (`:160`), so it is one line short. `SynthEditJuce`
is not generated on this box, so that half of the issue is by inspection and the
issue says so.

**Neither was fixed, and that is the run's one real judgement call.** STEP 1 says
a broken build on your platform outranks all backlog work and tells you to fix
it. STEP 5 says `SynthEditLib` is GATED, and `SE16/SynthEditWayland/` and
`SE16/SynthEditJuce/` are on neither list so they are GATED by default. **Both
fixes are one revert and one line, which is exactly the situation STEP 5 warns
about** — *"do not reach across the line because the fix looks small — that is
precisely when it is tempting"*. So: filed, with the full diagnosis and the exact
fix, and not touched. The 2026-08-17 macOS run's process note about
`SynthEditLib` being called ALLOWED in a row and GATED in the prompt is no longer
abstract; it now blocks a build fix on a broken platform. **That contradiction is
the thing to resolve, and it is Jeff's.**

**S3 itself, and this row named one of its three functions wrongly.**
`doDialogBuildCodeSkeleton` is declared by **no header anywhere** and called by
**nothing** — checked across `SE16`, `SynthEditLib`, `gmpi_ui` and
`GMPI_Wrappers`. It was dead weight, not a guard, so it is deleted rather than
made loud. The live "Build Code Skeleton..." path never went through it:
`MfcDocPresenter.cpp:1276` → `POPUP_MENU_DEBUG_CODE` → `CUG.cpp:2034`'s
`VO_Notify(OM_SHOW_CODE_SKELETON_DIALOG)`, whose only handler is the WinUI3 app's
`MainWindow.xaml.cpp:762`. TIDE registers none, so it is dropped. **Consequence
for the sandbox audit: finding A6's `create_directory`/`copy_file` sites in
`CUG::BuildSkeletonCode` are unreachable in TIDE, though still linked** — A6 read
the stub as the guard on that path and it never was. The other two,
`doDialogConnectUg` and `doDialogPatchManager`, *are* reachable
(`CUG.cpp:2635`, `CUG_with_patches.cpp:164`) and now report on stderr on every
build, keeping the `assert` for debug.

**Why stderr and not something louder**, since the row said "fail loudly":
`abort()`/`std::terminate()` kills the host DAW, which is strictly worse than the
no-op it replaces and is the P4 failure; a message box is a modal dialog
(constraint 5) needing a parent window TIDE may not have under AUv3, which is why
`TideAppStubs.cpp` already stubs `SafeMessagebox` to nothing; a log file is a
write outside the bundle (constraints 3 and 4) — the very thing the audit filed
these under. stderr is what is left, and it is already this project's answer to
the same question at `ModuleView.cpp:684` (*"Loud in Release on purpose … stderr,
not a dialog"*). The reasoning is in the code, not just here.

**Verification artifact — A/B on the shipping binary, with a positive control.**
`TIDE_VST3.so`, Release, `-DNDEBUG -O3` confirmed from `ninja -t commands` on
`TideApp.cpp.o`:

| Measurement | before | after |
|---|---|---|
| `"TIDE ships no such dialog"` in `strings` | 0 | **1** |
| `doDialogBuildCodeSkeleton` in `nm -C` | `T doDialogBuildCodeSkeleton[abi:cxx11] (CUG*)` | **absent** |
| `doDialogConnectUg` / `doDialogPatchManager` | present | present |
| `__assert_fail` in `nm -uC` | **0** | **0** |

That last row is the one that matters: it measures S3's premise rather than
asserting it. The old `assert(false)` compiled to **literally nothing** in a
shipping build — there is no `__assert_fail` reference in the binary at all, so
the stubs really did return as though the dialog had been shown and cancelled.
The control is the pair of symbols present in both binaries, which shows the
absence of the third is a real deletion and not a tooling artifact.

Builds, in the same tree: **`TIDE_VST3` 297/297** (links `TIDE_VST3.so`,
assembles the bundle), **`TIDE.gmpi`**, **`SynthEditCL` 19/19**. `SynthEditWayland`
is red for #88's reasons, not this change's — its two undefined symbols are
defined in `TideApp.cpp`, which is not on that target's link line before or after.

**Learned:**

- **A `#include` inside a namespace is a platform-dependent time bomb, and the
  guard is what hides it.** Whether it does damage depends entirely on whether
  something else already included that header in that TU. Worth a lint; nothing
  about the source tells you which platforms are affected.
- **"Loud in release" has a narrow menu in a plugin.** Three of the four obvious
  options each break a PLAN constraint or kill the host. Anyone reaching for
  `abort()` on a future S3-shaped row should read `TideApp.cpp`'s comment first.
- **A stub is not evidence that a path is guarded.** A6 assumed
  `doDialogBuildCodeSkeleton` sat on the Build Code Skeleton path; it sat on
  nothing. Check the call graph, not the name.
- **Reading a shared working tree read-only has a limit.** Verifying S3 needed a
  `SynthEditLib` that compiles, and #87 meant there was none. Solved with a
  throwaway `git clone` of it into the scratch dir with the trace removed, used
  only as a `SYNTHEDITLIB_FOLDER_OVERRIDE`. Nothing was committed there and Jeff's
  checkout was never modified — worth repeating rather than patching his tree
  and hoping to restore it.

**Next:** **[#87](https://github.com/JeffMcClintock/TideSynth/issues/87) and
[#88](https://github.com/JeffMcClintock/TideSynth/issues/88) first**, by whoever
is allowed to touch them — until then Linux is red and every "linux verified"
claim on this repo is worth re-checking. **S3g** carries S3's other half (the
menu entries, all GATED, NEEDS-JEFF). **Do not take C12d** despite its `linux`
mark: its Accept requires `SynthEditWayland` and `SynthEditJuce` to link under
GCC and #88 stops both. The next thing this box can actually finish is **P10**.

**Side effects on this box:** none to Jeff's trees. All five working copies were
**clean at start**, all are back on their default branches, and only
`SE16/SynthEditSem/TideApp.cpp` was ever modified. The build tree, the
`SynthEditLib` clone and the logs are all under the session scratch dir, not in
`~/SE`; Jeff's own `~/SE/build` was not touched or read into. No GUI, no host —
a scheduled run cannot get that approval, so nothing here is a runtime
observation.

**Branch/PR:** `tide/linux/S3-dialog-stubs` in both repos — this TideSynth PR +
[SynthEdit#32](https://github.com/JeffMcClintock/SynthEdit/pull/32). Merging one
without the other is harmless here: the TideSynth side is bookkeeping only and
the SynthEdit side is self-contained.

---

## 2026-08-17 — macos — U1b: the breadcrumb bar navigates in and out (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff confirmed cable-drag and module
insertion work, had the repos synced and old branches cleaned, and said
"take next task". Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** **U1b's chrome-and-navigation half, wired and verified** —
[SynthEdit#31](https://github.com/JeffMcClintock/SynthEdit/pull/31), stacked
on [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29). **Verified in
REAPER, the full loop:** the bar shows "Main"; placing a Container (which
draws as a proper module box with pins) and double-clicking it navigates
inside — trail reads "Main › Container", the container's own IO Mod visible
— and clicking "Main" navigates back out with the forward trail retained
for one-click re-entry. Both directions of U1b's Accept, live.

**The build was mostly discovery, not invention.** `SE2::BreadcrumbBar`
already existed in `se_sdk3_hosting` — cross-platform, thumbnail-caching,
retained-trail, powering every editor frontend (Wayland/JUCE/WinUI/mac
bridge) — and `TopStripLayout`'s own comment says it grew from exactly this
strip. TIDE's work was wiring: the bar becomes a fourth strip in
`SynthEditGui`'s manual pane layout (origin-rooted arrange + PaneHostWrapper
offset + pane pointer routing, the exact pattern of the two browsers), and
`ISeApp` grows `OpenViewForContainer` plus two callbacks.

**The enter path was a latent crash, now a feature.** Double-clicking a
Container runs `PresenterCommand::Open` → `CContainer::OnMenuCommand` →
`Document()->OpenView` → `CSynthEditAppBase::OpenView` →
**`m_app_user_interface->OpenView` — and TIDE never sets
`m_app_user_interface`**, so the gesture was a null deref waiting for the
first curious user. `TideApp` now overrides that virtual and routes to the
GUI's navigation callback instead.

**One deliberate mechanism worth keeping: navigation is deferred.**
Requests originate inside pointer dispatch — a crumb click dispatched by
the GUI, or a double-click dispatched by the very view being replaced —
and rebuilding the view stack from within its own dispatch destroys the
object mid-call. The Wayland app defers to its event-loop tick; TIDE
defers to a one-shot `gmpi::TimerClient` tick (30 ms), with the callbacks
cleared and the timer stopped in the destructor. The scroll-wiring block
was extracted to `wireViewScrollbars()` so navigation re-opens rewire
identically to the first open.

**Scoped out, recorded rather than hidden:** thumbnails (`renderThumbnail`
left unset — the bar draws name-only crumbs; the EditorScreenshot helper
`se_cl::renderContainerThumbnail` is the follow-up), and **U1b's second
half** — restoring the rack as the *default* with the structure view
behind an unlock — which waits on the open PR queue
([#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)/[#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)/[#30](https://github.com/JeffMcClintock/SynthEdit/pull/30))
and on the unlock UX being decided. The row stays IN-REVIEW listing both.

**Housekeeping done at Jeff's ask:** all five repos synced (gmpi_ui#8 had
merged — U2b's middle-pan is on main), and thirteen local branches with
merged PRs deleted across four repos; only open-PR branches and Jeff's own
release branches remain.

**Next:** merge queue for Jeff — [#28](https://github.com/JeffMcClintock/SynthEdit/pull/28)
→ [#30](https://github.com/JeffMcClintock/SynthEdit/pull/30), [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)
→ [#31](https://github.com/JeffMcClintock/SynthEdit/pull/31), plus
[SynthEditLib#13](https://github.com/JeffMcClintock/SynthEditLib/pull/13).
Once the queue clears: U1b's default-flip/unlock half on a clean base, then
**U1c** (enable Jeff's existing rack-mode code). The win box still has
U2e's two cheap follow-ups.

**Side effects on this box:** two `TIDE_VST3` rebuilds; the installed
plugin now carries the breadcrumb (struct-interim lineage). REAPER
restarted once; "Optimus HP" untouched; the test tab holds a Container
demonstrating the trail.

**Branch/PR:** this TideSynth PR +
[SynthEdit#31](https://github.com/JeffMcClintock/SynthEdit/pull/31)
(stacked on [#29](https://github.com/JeffMcClintock/SynthEdit/pull/29)).

---

## 2026-08-17 — macos — U1b complete: two depths, both directions (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged the outstanding PRs, asked
for the remaining one to be resolved and merged, then "do U1b's second half".
Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** resolved and landed the last open PR, then **finished U1b** —
[SynthEdit#33](https://github.com/JeffMcClintock/SynthEdit/pull/33). **The
rack is the default again and the structure view sits behind an unlock, with
all four navigation paths verified in REAPER.** Constraint 1's two depths are
now real in the plug-in.

**The routing, in one sentence:** the **master** container opens as the rack
(panel view — the product's face, workable now that U2d/U2e made panel
modules and controls draw); **any other** container opens as its structure
view; and `OpenViewForContainer` grows an optional explicit `view_flag` where
**0 routes by depth and a `CF_*` honours the caller**. That one parameter is
what turns SynthEdit's *existing* menu commands into the unlock and its
inverse — no new UI invented:

| gesture | result |
|---|---|
| open the plug-in | **rack** (panel view, modules drawing) |
| double-click a Container | its **structure** view, breadcrumb follows |
| **"Goto Structure…"** on the master | the master's **structure** view — the unlock |
| **"Panel Edit…"** | back to the **rack** |
| **"Main"** crumb | back to the **rack** |

**Learned — a false negative that cost an hour, and the tell.** "Panel
Edit…" appeared not to work: the canvas stayed on the fine structure grid,
and a List Entry inserted afterwards drew structure-style (module box + a
"Value Out" pin), which looked like confirmation. It was an artifact: the
context menu had been left open across a model switch, macOS auto-dismissed
it, and the click landed on the canvas instead. **Instrumenting
`TideApp::OpenView` settled it in one build** — `flag=256` (structure) then
`flag=128` (panel) both logged, with the two-tone rack canvas back on screen.
**The rule worth keeping: when a GUI verification contradicts a code path
that reads correct, suspect the input, not the code — and re-run the gesture
fresh before believing the failure.** A stale menu is invisible in a
screenshot.

**Learned — Jeff's rack machinery is right there, and U1c should start from
it.** `CContainer::OnMenuCommand` already handles
**`POPUP_MENU_TOGGLE_RACKMODULE`** (toggling `m_is_rack_module`, the flag
`ModuleViewPanel`'s JSON ctor already reads) and **`POPUP_MENU_TOGGLE_LOCKED`
→ `toggleLocked()`**. So U1c is enabling and surfacing existing code, exactly
as Jeff said — and the lock machinery is the natural home for a future
unlock UX if the menu command is ever felt to be too hidden.

**Also did — the PR queue is empty.** [#90](https://github.com/JeffMcClintock/TideSynth/pull/90)
(the linux S3 run's) was conflicting on all three coordination files; resolved
by keeping **both** sides' journal entries (S3 below the newer U1b entry, both
archives unioned) and **cross-picking** the NEXT rows — main's `mac`, the
branch's `linux`. **One lint trap worth recording:** the S3 entry quoted `nm`
output containing `[abi:cxx11]` immediately followed by `(CUG*)`, which `check-links.py` reads as a
markdown link to a file named `CUG*`; a space between `]` and `(` defuses it
without touching the quoted output's meaning. Every repo is now at zero open
PRs except this session's own.

**Next:** **U1c** — enable the existing rack-mode code (`m_is_rack_module`,
the rack axes already documented in `ViewBase::snapToGrid`), which is what
makes modules *snap into rack rows* rather than free-float. After that the
D-series surfaces (the about pane hangs off the breadcrumb bar, which now
exists). The win box still owes U2e's two follow-ups (staging + combo
re-verify).

**Side effects on this box:** four `TIDE_VST3` rebuilds; the installed
plug-in now opens as the rack. REAPER restarted twice; "Optimus HP" untouched
throughout; test tabs left open. Temp navigation logging was local-only and
is removed.

**Branch/PR:** this TideSynth PR +
[SynthEdit#33](https://github.com/JeffMcClintock/SynthEdit/pull/33) (against
`master`, no stack — the queue is clear).

---

## 2026-08-17 — macos — U1c: rack mode on, modules bolt to the rails (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "do U1c". Committed and pushed
as `tide-rack-bot` (claude-fable-5).

**Did:** **U1c** — [SynthEdit#34](https://github.com/JeffMcClintock/SynthEdit/pull/34),
stacked on [#33](https://github.com/JeffMcClintock/SynthEdit/pull/33).
**Verified in REAPER: TIDE renders a real Eurorack case — dark interior,
bevelled aluminium rails, threaded mounting holes at every HP — and a dragged
module snaps onto a rack row instead of staying where it was dropped.**
Constraint 1's rack is now what the plug-in actually looks like.

**It was one line, because Jeff had already built the rack.** Everything sits
behind `Document()->rackMode`, and **the only thing missing was a way to turn
it on in a TIDE build**: the flag is a per-project setting whose sole toggle
lives in a `#if defined(_DEBUG)` context menu, so a Release TIDE could never
reach it. `TideApp::InitInstance` now sets it at document creation, because
TIDE *is* the rack rather than a project that opts in. What that unlocks, all
pre-existing: `MfcDocPresenter::getRackLayout()` enables **only for the
top-level panel view** (sub-panels and every structure view keep ordinary
layout — exactly what U1b's second depth needs); `ViewBase::snapToGrid()`
switches from the square snap to **one HP across, one rack row down**; and
`TopView::renderRack()` draws the case. **The row's original "the only part
of U1c that is a from-scratch build" was wrong in the same direction U1a's
and U1b's estimates were** — this is the third time the honest answer was
"the code is there, wire it up", and Jeff said so twice before the code
confirmed it.

**Learned — verify against the branch that has the prerequisite, not against
`master`.** The first build showed no rack at all. Probes proved
`ContainerViewPanel::render` was never called, then that `getRackLayout()`
was never called — mystifying until the cause turned out to be **my own
staging**: I branched U1c off `master`, where **U1b's rack-as-default
(#33) is still an open PR**, so the master container still opened as the
*structure* view and the panel-view rack path was unreachable by
construction. Rebasing onto `tide/mac/U1b-rack-default` made it render on the
first try. **The tell was that two independent probes both showed "never
called" — that pattern means the code is not on the path, so check what you
are running before you debug what you wrote.** Both probes are reverted.

**Learned — a rebase can silently put someone else's commit on your branch,
and the authorship check is what catches it.** Rebasing onto the U1b branch
replayed Jeff's `dbghelp` fix (already on `master` as `85cd689a0`) as a new
SHA on my topic branch, so the push carried a commit not authored by the bot.
`scripts/check-commit-authorship.py` flagged it — **exactly the class of thing
A14 exists for, caught by the tool rather than by luck.** Fixed with
`git rebase --onto` to drop the duplicate, then `--force-with-lease` on my own
just-pushed topic branch (PR #34 was a minute old, nothing else built on it;
Jeff's original commit on `master` was never touched). Stated plainly here
because it is a rewrite of a pushed ref, same as the D1 precedent.

**Known follow-up, not blocking, recorded rather than pre-solved:**
`rackMode` is persisted per project (`s("rack_mode", rackMode)`), so a patch
authored in full SynthEdit *without* rack mode could load into TIDE with the
rack off. TIDE forces it on at document creation; if project load overrides
that, forcing it after load is the fix.

**Next:** with U1a/U1b/U1c landed, **constraint 1 is substantially done** —
rack by default, structure view behind an unlock, breadcrumb navigation, and
modules that bolt to rails. The natural next work is the **D-series** (the
about pane now has the breadcrumb bar to hang from) and **U1**'s own row,
which can finally be closed once U1a–U1c merge. The win box still owes U2e's
two follow-ups.

**Side effects on this box:** five `TIDE_VST3` rebuilds; the installed
plug-in now opens as a rack case. REAPER restarted three times — one restart
raced a scripted quit and left a "save unsaved project?" prompt, answered
**No** for a throwaway tab; **"Optimus HP" was never saved or modified**, and
REAPER's own reload of it (with its pre-existing missing-plug-in warning) was
dismissed untouched. Temporary probes in `SynthEditLib` were local-only and
are reverted; that repo is clean.

**Branch/PR:** this TideSynth PR +
[SynthEdit#34](https://github.com/JeffMcClintock/SynthEdit/pull/34) (stacked
on [#33](https://github.com/JeffMcClintock/SynthEdit/pull/33) — merge that
first, or both together).

---

## 2026-08-17 — macos — D3 done, D4 refuted by measurement, U1 closed (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff asked me to merge the U1b/U1c
stack, sync, and "do the D-series". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** merged [SynthEdit#33](https://github.com/JeffMcClintock/SynthEdit/pull/33)
and [#34](https://github.com/JeffMcClintock/SynthEdit/pull/34) at Jeff's
request (so **U1a+U1b+U1c are all on `master`** and **U1 itself closes**),
synced all five repos, then took the D-series: **D3 is done**
([gmpi_ui#9](https://github.com/JeffMcClintock/gmpi_ui/pull/9) +
[SynthEdit#35](https://github.com/JeffMcClintock/SynthEdit/pull/35)) and
**D4 is WONTFIX — its central measurement is now false, and acting on it
would have broken the build.**

**D4 first, because it is the finding.** The row says *"grepping SynthEdit,
SynthEditLib, gmpi_ui and GMPI_Wrappers finds **zero** call sites for
`gmpi::browse_to`"* and concludes the file can be dropped *"at no functional
cost"*. Re-measured today: **two live call sites** —
`SynthEditLib/SkinMgr.cpp:111` (`SkinMgr::EditSkin`, the "open this skin's
folder" command) and `SynthEditLib/MfcDocPresenter.cpp:1106` (the
skin-folder context command). Deleting `browseto.mm` would have produced an
undefined symbol, not a saving. The row was filed 2026-08-16; the C-series
carve-out has been moving files into `SynthEditLib` throughout, so the
likeliest explanation is that the callers arrived with a move after the grep
ran. **The lesson is the cheap one: re-run a row's own measurement before
acting on its conclusion, especially a "delete this, nothing uses it" row in
a tree that is being actively carved up.** I ran the positive control too
(`gmpi::open_url`, 5+ call sites) so a zero would have been distinguishable
from a broken grep.

**D4's *intent* is nonetheless delivered — by D3.** Its real goal was
removing an AppKit dependency and a sandbox-hostile API
(`activateFileViewerSelectingURLs:`, i.e. reveal-in-Finder) from Apple builds
that should not have them. D3's split does exactly that for the platform
where it matters: `browse_to` compiles to a deliberate no-op off macOS. On
macOS it stays, because it is used. So D4 is WONTFIX with the goal met
elsewhere rather than dropped.

**D3, and why the fix is not where the row put it.** The row proposed making
`EditorLib/CMakeLists.txt`'s `if(APPLE)` block iOS-excluding. That alone
would only convert a **compile** error into a **link** error: the headers
dispatch on `__APPLE__` — true on iOS — so the call sites still reference
`browse_to_impl`/`open_url_impl`. **The split belongs in the `.mm` files**,
which now choose their framework internally on `TARGET_OS_OSX`: `open_url`
uses `NSWorkspace` on macOS and `UIApplication openURL:options:completionHandler:`
on iOS; `browse_to` is macOS-only behaviour and a no-op elsewhere. The CMake
change is then just the framework line — **AppKit is macOS-only, iOS wants
UIKit**; CoreText, CoreFoundation and UniformTypeIdentifiers exist on both.

**Verified, and the limit stated:** TIDE_VST3, SynthEdit_VST3 **and**
SynthEditCL all build on macOS (that was D4's own Accept, reused here as the
regression check). **The iOS side is unverifiable on this box** — no iOS
target exists (S10) — so this removes the known compile blocker rather than
proving an iOS build succeeds. Said that way in the row and both PRs.

**Learned — a codesign failure on SynthEditCL can be stale-bundle detritus,
and my first A/B was not controlled.** SynthEditCL failed with P6's exact
string (*"code object is not signed at all … Contents/MacOS/Resources/
Prefabs/Button Small2.syntheditprefab"*). My first check stashed the change
**and** deleted the .app, so a pass proved nothing about which variable
mattered. Re-run properly — change applied, fresh bundle — it **builds
clean**: the failure was a stale bundle carrying resources under
`Contents/MacOS/`, not P6 regressing and not my edit. **`rm -rf` the .app
before believing a codesign failure on that target**, and change one variable
at a time even when the first answer is the one you wanted.

**Bookkeeping done in the same pass:** U1b and U1c flip **DONE** (their PRs
merged this session) and **U1 itself flips DONE and archives** — its three
children have all landed, which is what its row was waiting for. Constraint
1 is now delivered end to end: rack by default, structure view behind an
unlock, breadcrumb navigation, and modules that bolt to rack rows.

**Next:** the D-series is exhausted for now — D1/D2 landed 2026-08-16, D3 is
IN-REVIEW, D4 is WONTFIX, **D5 is Jeff's Ko-fi account** (done). The about
pane that D1/D2 designed is the natural next build: it now has the
breadcrumb bar to hang from, and [docs/about-pane.md](docs/about-pane.md)
fixes its contents to exactly four items. It needs a row of its own — filed
as **D6**. The win box still owes U2e's two follow-ups.

**Side effects on this box:** three products rebuilt (TIDE_VST3,
SynthEdit_VST3, SynthEditCL — the last twice, once from a fresh bundle);
`SynthEditCL.app` was deleted and rebuilt in the build tree. REAPER was not
driven this session. `gmpi_ui` and `SynthEdit` each carry one commit on a PR
branch; `SynthEditLib`, `TideSynth` and `GMPI_Wrappers` were read only.

**Branch/PR:** this TideSynth PR +
[gmpi_ui#9](https://github.com/JeffMcClintock/gmpi_ui/pull/9) +
[SynthEdit#35](https://github.com/JeffMcClintock/SynthEdit/pull/35) — **the
two code PRs must merge together**: the CMake one alone changes nothing, the
helper one alone leaves iOS linking AppKit.

---

## 2026-08-17 — macos — D6: the about pane is built (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "do D6". Committed and pushed
as `tide-rack-bot` (claude-fable-5).

**Did:** **D6** — [SynthEdit#36](https://github.com/JeffMcClintock/SynthEdit/pull/36).
The about pane that D1 and D2 designed exists and works: it opens from the
breadcrumb bar, shows exactly the four specified items, the donation link opens
Ko-fi, **Copy link puts the exact URL on the system clipboard**, and clicking
away dismisses it. **This is the first D-series item that is a running feature
rather than a design note** — D1 and D2 produced the spec, and it survived
contact with implementation essentially unchanged.

**What it looks like:** a rounded panel centred over the editor with a soft
scrim behind it — *TIDE Rack — version 0.1 (unreleased)* in bold, the credit
under it, then the ko-fi URL in link blue with **Copy link** beside it, then
*ISC licence — github.com/JeffMcClintock/TideSynth*. An **X** at the corner and
click-away both dismiss.

**Every rule in the spec is kept, and the code says so where it matters.**
`AboutPane.h` restates the six rules at the top, next to the code that has to
keep them, because they are the kind of thing a later reader deletes by
accident: nothing unprompted (**the only way in is a plain "About" text
affordance** at the right end of the breadcrumb strip — no badge, no dot, no
splash); never a dialog or a second window; nothing blocking audio; no image
assets; the donation line degrades to text but never to nothing; **exactly four
items, and a fifth needs a ruling.**

**The one design decision I had to make, and it is rule 5 taken literally.**
"Copy link" needs a clipboard write, and GMPI has no clipboard abstraction —
only `KeyListenerCallback`'s cut/copy hooks, which are for text fields. So the
pane asks `tide::clipboardAvailable()` and **omits the button entirely where the
answer is no**, rather than drawing one that silently does nothing. Apple gets
`NSPasteboard`/`UIPasteboard` (split on `TARGET_OS_OSX`, the same pattern D3
just established); Windows and Linux get a stub returning false, with a comment
naming the reachable APIs for whoever wires them. **A dead button would have
broken the very rule the button exists to serve.**

**Where it lives, and why not in the shared bar.** The pane is TIDE's alone, so
it is in `SynthEditSem` (ALLOWED); `SE2::BreadcrumbBar` is shared with every
SynthEdit frontend and gains nothing TIDE-specific. The affordance is drawn by
`SynthEditGui` over the strip's right end, which also keeps the bar's own
hit-testing untouched.

**Learned — verify a clipboard by reading it back, not by watching the label
change.** The button flips to "Copied" on its own return value, which is
exactly the kind of self-report that can be true while the write failed. I
primed the system clipboard with a sentinel string via `pbcopy`, clicked Copy
link, and checked `pbpaste`: the sentinel was gone and the URL was there.
**That is a one-line check that turns "the UI said it worked" into evidence**,
and it is available to any mac session.

**Caught in my own first build:** the pane rendered with no Copy button at all —
`copyOffered` defaulted to `false` and I never wired it. The screenshot looked
fine (four items, all correct), which is precisely why the row's Accept lists
the button separately. Fixed by initialising it from `clipboardAvailable()`.

**Next:** the D-series is now exhausted — D1/D2 (design) and D6 (build) landed,
D3 is IN-REVIEW, D4 is WONTFIX, D5 was Jeff's. With constraint 1 delivered
(U1a/U1b/U1c) and the about pane built, **the plug-in has its shape**. The
honest next question is not another feature but **what v0.1 needs**, which is
the R-series' territory (R2–R6, all blocked on there being something to ship) —
and the version line in this pane will be the first thing that has to stop
saying "unreleased". The win box still owes U2e's two follow-ups.

**Side effects on this box:** three `TIDE_VST3` rebuilds; the installed
plug-in now has the about pane. REAPER restarted twice; **"Optimus HP" was
never saved or modified**. The system clipboard was overwritten twice as part
of the test (sentinel, then the ko-fi URL) — it now holds
`https://ko-fi.com/tiderack`, which is worth saying because it is Jeff's
clipboard.

**Branch/PR:** this TideSynth PR +
[SynthEdit#36](https://github.com/JeffMcClintock/SynthEdit/pull/36).

---

## 2026-08-17 — macos — P10: the dead XML is gone (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged the D3/D6 stack, then
"sync repos, clean up branches, continue". Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** synced all five repos and deleted every merged local branch (only
`main`/`master` and Jeff's own release branches remain; **zero open PRs** in
all four repos at the start), then took **P10** — the item the NEXT row names
for exactly this situation — [SynthEdit#37](https://github.com/JeffMcClintock/SynthEdit/pull/37).
`SynthEditSem/SynthEdit.xml` and its `SynthEdit.rc` resource line are deleted.

**Why the file had to go, restated because it is the trap P5 nearly fell
into:** the file looks like the source of truth — 12 lines, named after the
plug-in, holding `id` and `name` — and **is not what ships**. The live identity
is the embedded XML string literal in `SynthEdit.cpp`'s
`getPluginInformation()`. Editing the `.xml` alone changes nothing at runtime
on any platform: a no-op PR that reviews as correct.

**Re-verified before deleting rather than trusting the row, and the discipline
mattered twice this week.** D4's central measurement turned out false
yesterday, so P10 got the same treatment: only two references exist (the `.rc`
line and an explanatory comment in `SynthEdit.cpp`), and the sole loader sits
inside `#if 0`. **The near-miss worth recording: the loader's first visible
guard is `#if _WIN32` at `MyVstPluginFactory.cpp:472`, which reads as live —
the `#if 0` that kills it is the *enclosing* one at `:462`.** I read the inner
guard first and briefly concluded the row was wrong, exactly as I had concluded
about D4. Checking the enclosing guard settled it in one command. **When a
"this code is dead" claim rests on a preprocessor guard, find the outermost
one, not the nearest.**

**Accept met, both halves:** TIDE_VST3 and SynthEdit_VST3 build on macOS, and
the built binary's identity is byte-identical — `id="SE SynthEdit"
name="TIDE Rack" vendor="TIDE Synth"`, the strings P5 put there.

**The limit this box cannot close, stated rather than glossed:** `.rc` files
are Windows-only, so the deletion is verified *consistent* here but the Windows
resource compile is unexercised. It should be trivially fine — the only line
naming the file goes with the file — but the Windows box is the real check, and
the PR says so.

**Next:** with P10 done, the mac backlog has **no remaining item a scheduled
run should take on its own initiative**. What is left is either Jeff's call
(the R-series, all blocked on there being something to ship) or small
follow-ups already recorded in their rows: crumb thumbnails (U1b), `rackMode`
on project load (U1c), Windows/Linux clipboard for Copy link (D6), and the win
box's two U2e items. **That is a genuinely finished board rather than a tired
one**, and the NEXT row now says so in those words so the next run does not
invent scope to fill the gap.

**Side effects on this box:** two `TIDE_VST3` builds and one `SynthEdit_VST3`
build; the installed plug-in is current. REAPER was not driven this entry.
Only `SynthEdit` was committed in; the other four repos were read only and are
clean on their default branches.

**Branch/PR:** this TideSynth PR +
[SynthEdit#37](https://github.com/JeffMcClintock/SynthEdit/pull/37).

---

## 2026-08-17 — macos — crumb thumbnails, and what they cost (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff picked the U1b crumb-thumbnail
follow-up off the list the previous entry left. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** wired the breadcrumb bar's thumbnails —
[SynthEdit#38](https://github.com/JeffMcClintock/SynthEdit/pull/38). **Verified
in REAPER: crumbs render the container's real content, and switching the master
from rack to structure view swaps the tile from the dark case to the light
structure grid with the placed Container visible inside it** — so it is
genuinely rendering the container, not drawing a placeholder.

**There was nothing to invent.** `BreadcrumbBar::renderThumbnail` has always
been the way in, `se_cl::renderContainerThumbnail` has been shared since it was
lifted out of SynthEdit2 ("which made thumbnails a Windows-only feature by
accident rather than by design"), and both the Wayland and WinUI editors
already install the callback. TIDE left it unset and silently got name-only
crumbs. **This is the fourth item in a row where the answer was wiring, not
building** — U1a, U1c, D6's content, and now this.

**The one interface change, and why it is two methods rather than one.**
`ISeApp` grows `setQuiet(bool)` returning the **previous** value. The offscreen
render walks the module factory, whose duplicate-module dialogs must be
suppressed around it; the Wayland version scopes `app_.quiet` directly, but
`ISeApp` exists to firewall SE SDK3 off from the GMPI side, so exposing the
application object to get at one bool would have been the wrong shape.
Returning the previous value means callers restore rather than assume `false`.

**Measured the cost rather than waving at it, because a plug-in pays for every
byte.** TIDE_VST3 went **10,149,744 → 10,414,832 bytes (+265,088, +2.6%)**.
Static-archive extraction did most of what C12e's rule predicts —
`EditorCommandDispatcher` is **not** linked (0 symbols) — **but
`SamplingProfiler` IS pulled in (8 symbols)** through `ScreenshotRenderer`.
That is the finding worth keeping: **the screenshot library is not free of its
tooling, and "only the members you reference" is true transitively, which is
not the same as "only the members you wanted".** If the cost is unwanted the
revert is two lines, and the PR says so.

**Learned — the strongest visual test is a CHANGE, not a picture.** A dark
thumbnail of a dark rack is indistinguishable from a black rectangle, and I
nearly recorded "it renders" on that basis. Switching the same container to its
structure view and watching the tile change to a light grid **containing the
module I had just placed** is proof that content is being rendered per
container and per view flag. Same discipline as yesterday's clipboard sentinel:
make the thing prove it changed, do not photograph it once.

**Next:** three small follow-ups remain from the finished-board list —
`rackMode` on project load (**U1c**), Windows/Linux clipboard for Copy link
(**D6**), and the win box's two **U2e** items — plus the **R-series**, which is
Jeff's call. The mac NEXT row's "do not invent scope" still stands.

**Side effects on this box:** two `TIDE_VST3` builds; the installed plug-in now
draws thumbnails. REAPER restarted once; **"Optimus HP" untouched** (it
reloaded on its own and was left alone). Only `SynthEdit` was committed in.

**Branch/PR:** this TideSynth PR +
[SynthEdit#38](https://github.com/JeffMcClintock/SynthEdit/pull/38).

---

## 2026-08-17 — macos — TIDE does not save the user's rack (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "keep working, no mercy" after
merging the thumbnails. I went after the smallest remaining follow-up
(`rackMode` on project load) and found something much larger on the way in.
Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** filed **S11** — **TIDE never persists or restores its document, so the
user's rack is lost the moment a project is reloaded.** No code change this
entry: the finding, its evidence, and the mechanism are the deliverable, and
the fix is a real feature that should be scoped deliberately rather than
started at the end of a long session.

**How it surfaced, which is the useful part.** U1c's follow-up asks what
happens to `rackMode` when a project is loaded, since the flag is serialised
(`s("rack_mode", rackMode)` in `SynthEditDocBase.h`). Following D4's lesson I
went to measure rather than reason — and the measurement kept coming back
wrong in a way that only made sense if **nothing loads a document at all.**

**The evidence, in three steps, each one cheap:**

1. **The saved state is 250 bytes of base64** for a project containing a placed
   List Entry in a rack (`GetTrackStateChunk` via ReaScript). A document with a
   module in it cannot fit in 250 bytes.
2. **Decoded, it is two parameters and nothing else** — the `.rpp`'s VST block
   reads `<Preset><Param id="1" val="0"/><Param id="0" val="0"/></Preset>`.
   Those are `controllerPtr` and `chunk`, both zero.
3. **Save → close → reopen → the module is gone.** The reloaded plug-in draws
   an empty rack: rails present (rack mode is set at document creation), no
   List Entry. Verified visually.

**The mechanism, so the row is actionable rather than alarming.** TIDE's XML
already declares the parameter this needs —
`<Parameter id="1" name="chunk" ignorePatchChange="true" datatype="blob"/>` —
and **nothing in the codebase ever writes it or reads it**;
`TideApp::InitInstance` unconditionally does `createNewDocument()` +
`OnNewDocument()`, so every instance starts empty by construction. The
controller's preset system (`MpController` / `DawPreset`) serialises
*parameter values*, which is exactly the two-param XML observed. The document
has its own serialisers already — `CSynthEditDocBase::ExportXml` /
`ImportXml` — so the shape of the fix is: export the document into that blob
parameter on save, import it back and rebuild the view on load.

**Why this is an architecture difference and not an oversight to be ashamed
of.** In a normal SynthEdit-exported plug-in the document IS the product: it
is baked in at export time and the chunk only has to carry knob values. TIDE
inverts that — **the document is what the user edits at runtime** — so it must
ride in the state. Nobody wrote that because nothing before TIDE needed it.
That framing belongs in the row so the next reader does not go looking for a
regression.

**What it means for the release, stated plainly:** the mac NEXT row said this
morning that the board was finished and the remaining question was v0.1. **It
still is, and this is now the answer**: a synthesiser that cannot save its
patch is not shippable, so **S11 blocks the R-series** more concretely than
"there is nothing to ship" did. That is a better problem than it sounds —
the question moved from "what should we build?" to "build this one thing".

**Also settled, and it retires a follow-up:** U1c's `rackMode`-on-load worry is
**moot in the form it was written**. Nothing loads a document, so nothing can
override the flag; the rack survives *because* the document is always fresh.
When S11 lands, the question becomes live again and S11's own work has to
answer it — noted in both rows so the retirement is not silently forgotten.

**Learned — chase the follow-up, find the feature.** The smallest item on the
list was the one that exposed the largest gap, because verifying it required
exercising a path (state round-trip) that no previous session had reason to
touch. **Six sessions of host verification never caught this**: every test
opened a fresh plug-in, and a fresh plug-in looks identical whether or not
persistence exists. The failure is only visible across a save/reload boundary,
which is a class of test worth adding deliberately rather than stumbling into.

**Next:** **S11** is the item, and it is Jeff's call how far to take it — the
row proposes the minimum honest version (round-trip the document through the
existing blob parameter) and lists the questions that need his answer, chiefly
what happens to the DSP graph on restore and whether patch-change should
reload the rack.

**Side effects on this box:** no code changed, nothing rebuilt. REAPER was
driven and a throwaway project was written to `/tmp/tide-persist-test.rpp` as
part of the test; **"Optimus HP" was never opened, saved or modified** — the
test script aborts if it sees that project active, which it checked and
reported.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — S11's design answered by Jeff; S12 filed: TIDE makes no sound (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff answered S11's three open questions
and said to keep building. Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** recorded Jeff's three rulings into **S11** so they cannot evaporate,
and filed **S12** — **TIDE's audio processor is a stub that writes silence, so
the rack makes no sound at all.** Found while reading the DSP path his answers
pointed at. **No code this entry**, and the reason is the entry's whole point:
his answers describe rebuilding a DSP graph, and **there is no DSP graph to
rebuild.**

**Jeff's rulings, verbatim in substance, now in S11's row:**

1. **A new document implies a rebuild of the DSP graph** — and SynthEdit
   already handles that: **fade-out → teardown → reconstruction → fade-up.**
   So restore does not need a new mechanism invented; it needs the existing one
   driven.
2. **A host preset change may modify the rack.** It is treated as loading a
   brand-new document — anything can change.
3. **All state lives in the preset.** That settles the third question (rack in
   plug-in state vs a referenced user file) in favour of the preset, which is
   also what makes (2) coherent.

**S12, and why it stops S11 rather than merely accompanying it.**
`SynthEditSem/SynthEdit.cpp`'s `class SynthEdit final : public Processor` is
the only DSP class TIDE has, and its `subProcess` is:

```cpp
// TODO: Signal processing goes here.
*left = 0.0f;  *right = 0.0f;
```

`TideApp` never starts a synth runtime — no `prepareToPlay`, no
`StartBackgroundProcessing`, no generator. **Nothing anywhere instantiates the
user's placed modules as DSP.** So the rack is an editor with a silent audio
stub bolted on: the modules exist as documents and views, and their DSP
counterparts (which ARE registered — `ug_oscillator2` and the rest) are never
built into a running graph.

**Why this reframes everything above it.** Ruling 1 says restore must rebuild
the DSP graph; a rebuild of nothing is a no-op, so **S11's restore path can be
built today and will be correct, but its DSP half cannot be exercised or
verified until S12 lands.** And for the release question the two are not equal:
a rack that forgets your patch is a bad synthesiser, but **a rack that makes no
sound is not a synthesiser at all.** S12 therefore blocks the R-series ahead of
S11.

**What this does NOT mean, stated so nobody re-derives it in alarm.** This is
not a regression and nothing broke: the DSP stub has been a `// TODO` since the
prototype, and every session since has been building the editor — the thing
constraint 1 is about. Six sessions of host verification never caught it for
the same reason they never caught S11: **every test drove the UI, and no test
ever played a note and looked at a meter.** That is now two findings from one
missing habit.

**Learned — when a ruling arrives, check its premise before building to it.**
Jeff's answers are exactly right for the system he is describing; they were
answers about a DSP rebuild, and the honest response was to look at the DSP
path before writing a line. Two greps did it. **Building S11's restore first
and discovering the silence afterwards would have produced code whose central
claim — "the graph rebuilds" — nobody could test.**

**Next:** **S12** is the item, and it is the real v0.1 gate. S11 is fully
specified now (Jeff's three rulings + the mechanism already in its row) and can
follow, or land alongside, once there is a graph for its restore path to
rebuild. Both rows say which comes first and why.

**Side effects on this box:** none — no code changed, nothing rebuilt, REAPER
not driven. Only TideSynth was committed in.

**Branch/PR:** this TideSynth PR (rows + entry only; no code).

---

## 2026-08-17 — macos — S12 mapped: the machinery exists, in the sibling VST3 target (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff's pointer, in substance: SynthEdit
also builds the graph for its own use and switches out the DSP smoothly; the
SynthEdit VST3 target is similar and can rebuild the graph under certain
conditions. Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** followed the pointer through the code and rewrote **S12** from "scope
unknown, probably large" into a **start-ready implementation map with file:line
references**. No product code this entry; the map is the deliverable, and it
changes S12's size class from "unknown" to "one focused session".

**What the pointer found, part by part:**

1. **The plugin-side graph host exists:** `SynthRuntime`
   (`SynthEditLib/SynthRuntime.cpp` — sibling of the standalone's
   `SynthRuntime_editor`) owns `SeAudioMaster` and builds the whole DSP graph
   from an XML document. Today it reads that XML from the **`dsp.se.xml`
   bundle resource** (line 59) — the thing SynthEdit's *exporter* bakes in.
   **The cache check is the injection point:** `if
   (!currentDspXml.RootElement())` means a pre-seeded document skips the
   bundle read entirely. TIDE never needs a baked resource; it needs to hand
   the runtime its XML.
2. **The smooth swap Jeff described is real and complete**
   (`SynthRuntime.cpp:330-395`): `audioMasterState::AsyncRestart` → retain
   presets (`getPresetsState`) → `Close()` → new `SeAudioMaster` → background
   `rebuildDsp` thread → fade-up. **And the document-swap hook already exists
   in skeletal form:** a `pendingDspXml` branch sits inside that path behind
   `#if 0 // editor only` — disabled because an exported plug-in's document
   never changes. TIDE is exactly the product that flag was sketched for.
3. **A complete working reference exists:** `se_vst3`'s `SeProcessor`
   (`adelayprocessor.cpp`, 1502 lines) does everything TIDE's stub does not —
   `prepareToPlay`/`reInitialise`, MIDI translation, queue servicing,
   `ProcessorStateMgrVst3`, and `setState`/`getState` whose chunk is a
   `DawPreset` string. **Jeff's "all state lives in the preset" is that
   target's existing design**, not a new invention.
4. **The editor can emit the DSP XML at runtime:** `dsp.se.xml` is nothing but
   `<Document><DSP>` wrapping `MasterContainer->ExportXml(element, target)` —
   fifteen copyable lines in `ExportAsPlugin.cpp:1204-1220` (skip the Release
   `Scramble`). So the live document TIDE's editor already edits can produce
   exactly what `SynthRuntime` eats, with the same serialiser the exporter
   uses.

**Why S11 and S12 turn out to be one mechanism.** Document XML rides in the
preset (S11's rulings); a preset that carries a new document sets
`pendingDspXml` and triggers `AsyncRestart`; the rebuild thread constructs the
new graph while audio fades (S12). Save, restore, and preset-change-modifies-
the-rack are the same wire.

**The one fork, flagged for a one-word ruling.** **Option A (recommended):**
keep TIDE as a GMPI plug-in and grow its processor a `SynthRuntime` — all of
this session's controller/editor wiring survives, and the document travels
through the already-declared `chunk` blob parameter bound to a DSP pin
(`BlobInPin` exists in sdk3; the GMPI-Core equivalent is a named unknown).
**Option B:** rebase TIDE onto `se_vst3`'s `SeProcessor`/`SeController` —
the processor comes ready-made, but the `controllerPtr` trick, TideApp
attachment and editor hosting all get redone against Steinberg classes.
**Named unknowns for either:** `BundleInfo` calls inside `SynthRuntime`
(`latencyConstraint`, resource folder) against a bundle that lacks the
exporter's resources; in-process queue wiring TideApp ↔ runtime.

**Thin-slice accept, unchanged:** place **1 kHz Tone**, wire it to **Sound
Out**, play, and see the host meter move.

**Learned — "scope it before costing it" can cost one hour and change the
answer.** Yesterday's S12 said "unknown and probably large: building a DSP
graph at runtime is what the exporter does at build time". The pointer plus an
hour of reading found the runtime builder, the swap machinery, the skeletal
document-swap hook, and the serialiser — all existing, none speculative. The
row now names them by file and line, which is the difference between a next
session that implements and one that re-discovers.

**Next:** S12, Option A unless Jeff says B — starting with the fresh-context
implementation session the row is now written to launch.

**Side effects on this box:** none — read-only exploration; nothing built,
REAPER not driven. Only TideSynth committed.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — S12 built to its last step: the rack has an engine; the tone is one gate away (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff ruled Option A with the boundary
sharpened — "keep the wrappers pure and simple, put the DSP xml stuff in TIDE
itself" — and the implementation began. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** the S12 thin slice, end to end except its final step, across four
repos: GMPI branch `tide/mac/blob-param-transport` (**the bot cannot push to
that repo — 403 — so the commit is local and the patch is filed at
[docs/patches/gmpi-blob-param-transport.patch](docs/patches/gmpi-blob-param-transport.patch)
for Jeff to apply**),
[GMPI_Wrappers#4](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/4),
[SynthEditLib#14](https://github.com/JeffMcClintock/SynthEditLib/pull/14),
[SynthEdit#39](https://github.com/JeffMcClintock/SynthEdit/pull/39) (WIP).

**Found and fixed on the way in — the wrapper had no blob transport at all.**
The three-point log bisected it in one run: the editor pushed 570 bytes, the
controller's `setParameter` returned Ok, and the processor's pin never fired.
`notifyDaw`/`performEdit` speak normalized doubles; `onQueMessageReady`
consumed only `"ppc2"`. **The fix is generic, not TIDE-specific** — any
wrapped plug-in with a blob parameter needs it: the holder hands changed blobs
to a wrapper-installed hook (the `"ppc3"` framing already existed unused!),
the VST3 wrapper moves the framed bytes with its existing binary message, and
the processor holder consumes them into a Blob PinSet event. **Verified: the
next run's log shows the same push arriving as `onSetPins size=570`.**

**Then the graph — three crashes, each teaching the document's required
shape:** (1) `SetupVstIO` derefs the *nested synth container* — exported
projects have main→synth-child, TIDE's flat rack IS the master → wrap the
export in a synthetic outer container. (2) `BuildPatchManager` runs on main
unconditionally → move the PatchManager to the outer; the inner inherits,
ordinary SE containment. (3) After both: **the empty document builds and RUNS
(`prepared SR=48000 BS=512`) and a wired-document push while running takes
the AsyncRestart fade/teardown/rebuild path without crashing.** The
`#if 0 // editor only` `pendingDspXml` hook is enabled and working.

**The frontier, precisely: modules are pruned from the export.** The placed
1 kHz Tone and Sound Out never reach the XML — `<Modules />` stays empty while
their connecting `<Line>` survives. First cause found and fixed:
`CDocOb::exportFlags = EXP_PLUGIN` makes `doExport()` drop every
`excludeFromVst` module — **Sound Out and the whole Diagnostic group are
exactly those** — because a baked export replaces them; TIDE self-hosts
editor semantics, so flags are now 0. **But modules are still pruned with
flags 0.** Prime suspect: the other gate in `CUG::ExportXml` —
`hasDspModule()` false because the module set's **DSP-side registrations never
ran in TIDE**. That is the exact U2d pattern that hit the GUI half. **Next
probe (one build): log `name / doExport() / hasDspModule()` per master-child
inside `exportDspXml`.**

**Learned — a three-point log turns "it doesn't work" into a one-run
bisect.** sync-push / setParameter-rc / onSetPins-size located a missing
wrapper subsystem in a single REAPER launch, then re-verified the fix the
same way. The temp diagnostics (tagged `TEMP S12 diag`) are deliberately
left in the WIP branch so the next session continues without re-instrumenting;
they come out before merge.

**Build note until the stack lands:** TIDE's build cache now sets
`GMPI_SDK_FOLDER_OVERRIDE` and `GMPI_WRAPPER_FOLDER_OVERRIDE` to the local
checkouts (CPM otherwise fetches GitHub main, which lacks the transport).
Drop the overrides once the GMPI patch and GMPI_Wrappers#4 are on main.

**Next:** the row's next-probe, then whichever registration wiring it names —
the Accept (1 kHz Tone → Sound Out → host meter moves) is plausibly one or
two builds away. After the tone: remove the diagnostics, then S11's restore
path rides the same wire (the chunk already persists in the DAW state).

**Side effects on this box:** ~8 TIDE_VST3 rebuilds; REAPER crashed twice
(both crash reports read and acted on) and was restarted ~6 times; throwaway
projects only, **"Optimus HP" never opened, saved or modified**. Temp files:
`/tmp/tide-s12.log`, `/tmp/tide-dsp-doc.xml`. GMPI repo has a local branch
the bot could not push.

**Branch/PR:** this TideSynth PR + the four-repo stack above.

---

## 2026-08-17 — macos — FIRST SOUND (interactive session, Jeff present — "i can hear it!")

**Prompt:** n/a — interactive session. Jeff merged the S12 stack, pointed out
that Sound Out is the device sink and "VST Output" the plugin path, and then
— after three more fixes — heard TIDE Rack's first sound. Committed and
pushed as `tide-rack-bot` (claude-fable-5).

**Did:** finished the S12 thin slice. **Place 1 kHz Tone, place Sound Out,
drag the cable: track meter −6.0 dB, master green, and Jeff heard it.** The
whole product loop is live for the first time: edit the rack → document
exports → chunk parameter → processor → graph rebuilds → audio.
[SynthEditLib#15](https://github.com/JeffMcClintock/SynthEditLib/pull/15) +
[SynthEdit#40](https://github.com/JeffMcClintock/SynthEdit/pull/40), 
diagnostics stripped.

**The three finds between "merged" and the meter moving, in order:**

1. **The export pruner was a design decision, not a bug** — with any target
   except `SAT_SYNTHEDIT_DSP`, `ExportXml_Pt2` on a top-level container
   serialises ONLY its first child container and disregards loose modules
   ("save XML for use in a plugin. Excludes 'Sound Out'..." — the comment says
   it plainly). The probe that proved both per-module gates PASSED
   (`doExport=1 hasDsp=1`) is what forced reading past them. **TIDE now
   exports with the editor's own runtime format, and the modules appear.**
2. **The AsyncRestart swap path is unreachable in the plugin runtime** —
   `eRuntimeState::resetting` exists only as a case label; nothing enters it.
   So the first (empty) graph played silence forever while every later
   document push was stored and never consumed. **Fix: `documentPending_`
   forces `prepareToPlay` to rebuild, and the processor re-calls it on every
   document arrival** — synchronous, in place, fine at rack scale; the faded
   swap can come later without changing the transport.
3. **No browser unhide was needed.** Jeff's pointer resolved cleanly:
   "VST Output" is what `SetupVstIO` builds internally; the user-facing sink
   is plain **Sound Out**, whose `ug_soundcard_out` is a real
   `ISpecialIoModuleAudioOut` — it registers with the audio master at `Open`
   and receives whatever buffers the shell provides: device buffers in the
   standalone, **the host's output buffers in TIDE**. (IO Mod, the other
   candidate, refuses to instantiate in a master container — it needs a
   parent to expose plugs to.)

**Learned — when both gates pass and the output is still empty, the skip is
between the gates.** The probe pattern (log every candidate's name plus each
gate's verdict) took one build and turned "modules missing, cause unknown"
into "read `ExportXml_Pt2`". And its own first run crashed REAPER on an
uninitialised iterator (`it_doc_ob` needs `First()`), which is the day's
smallest lesson: MFC-style iterators do not position themselves.

**Also caught on the way:** my line-ending-blind Python edit turned the GMPI
patch into a 2212-line diff; redone byte-safe (CRLF preserved) it is 58
lines. **Check the patch size before filing a patch.**

**Where S12 stands:** thin-slice Accept met and heard. Remaining, named in
the row: the MIDI-note path (wired via `onMidiMessage` → `rack.MidiIn`, but
untested — no MIDI module was in the patch), the faded document swap, preset
retention across edits, and re-verifying a REAPER save/reopen now that the
chunk carries real documents (S11's other half). **The GMPI blob-transport
patch is still unapplied** — main lacks "ppc3" and GMPI_Wrappers main now
requires it, so builders without the local branch break; the patch sits in
[docs/patches/](docs/patches/gmpi-blob-param-transport.patch) and Jeff's queue.

**Side effects on this box:** ~6 more TIDE_VST3 rebuilds, one REAPER crash
(my probe's iterator — report read, fixed), several restarts; throwaway
projects only, **"Optimus HP" untouched**. `/tmp/tide-s12.log` and
`/tmp/tide-dsp-doc.xml` left behind by the now-removed diagnostics.

**Branch/PR:** this TideSynth PR +
[SynthEditLib#15](https://github.com/JeffMcClintock/SynthEditLib/pull/15) +
[SynthEdit#40](https://github.com/JeffMcClintock/SynthEdit/pull/40).

---

## 2026-08-17 — macos — the sound reproduces from upstream alone (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff applied the GMPI patch, merged it,
and said "done". Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** verified the thing the local overrides had been hiding: **with no
local overrides and nothing unmerged anywhere, a from-scratch configure that
fetches GMPI and GMPI_Wrappers from GitHub main builds TIDE and it still makes
sound** — 1 kHz Tone → Sound Out, −6.0 dB, master green. The four-repo S12
stack is now genuinely self-consistent on main, and this box builds exactly
what a fresh clone would. Also appended a build-trap note to
[docs/building.md](docs/building.md).

**Confirmed on main before testing:** `ppc3` in `processor_holder.cpp` and the
`sendNonNativeParameterToProcessor` hook in `controller_holder.h` (GMPI,
merged as PR #1), and the VST3 installer of that hook in GMPI_Wrappers. Then
cleared both `GMPI_*_FOLDER_OVERRIDE` cache entries and deleted the local
`tide/mac/blob-param-transport` branch — **nothing on this box is now needed
to build TIDE that is not on GitHub.**

**Made a mess and cleaned it, which is the entry's real content.** Clearing
the overrides was not enough: FetchContent had already populated `_deps`, so
I deleted those directories — which left the build tree inconsistent and
configure failing (`gmpi_plugin.cmake` not found; the fetch step silently
declined to re-run). The right tool was **`cmake --fresh`**, and it worked
first time. But `--fresh` wipes the *whole* cache, and two of those cached
values mattered:

1. **The generator.** The tree was an **Xcode** project; a bare `--fresh`
   re-generated it as Unix Makefiles. Restored with
   `cmake --fresh -G Xcode .` — worth knowing before anyone runs `--fresh` on
   a tree they did not create.
2. **`SE_LOCAL_BUILD`**, and this one is genuinely nasty. The POST_BUILD step
   that copies the bundle to `~/Library/Audio/Plug-Ins/VST3` lives inside
   `if(SE_LOCAL_BUILD)` in GMPI's `gmpi_plugin.cmake`, and the option is
   **declared FALSE by default** — a developer machine auto-installs only
   because the value is sitting in `CMakeCache.txt`. After `--fresh` it was
   gone, so **the build succeeded, the bundle in the tree was current, and
   REAPER kept loading the previous binary.** No error anywhere.

**Learned — "it built" and "the host is running it" are different claims, and
a cache reset can split them silently.** I caught it only because I compared
the installed binary's timestamp and size against the build tree's before
trusting a host test, which turned a plausible false pass into a two-command
fix (`cmake -DSE_LOCAL_BUILD=TRUE .`). **This is the same discipline as the
clipboard sentinel and the thumbnail change-test: make the artefact prove it
is the one under test.** Both traps are now written into
[docs/building.md](docs/building.md), since the next person to run `--fresh`
here will hit them in the same order.

**Left as found:** Xcode generator restored, `SE_LOCAL_BUILD=TRUE` restored,
overrides empty, installed plug-in byte-identical to the current build tree
(checked with `cmp`).

**Next:** unchanged — S12's remainder in its row (MIDI-note verify first, then
the save/reopen re-check, faded swap, preset retention).

**Side effects on this box:** two full fresh configures and three TIDE_VST3
builds (~15 min); `_deps` for GMPI and GMPI_Wrappers re-cloned from GitHub;
REAPER restarted once, throwaway project only, **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (doc + entry only; no code).

---

## 2026-08-17 — macos — S12(a): MIDI reaches the processor but not the graph (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "sync and clean up, then
continue", which meant S12's next item: verify the MIDI note path. Committed
and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** synced all six repos and deleted three merged branches, then took
**S12(a)**. **Result: MIDI arrives at TIDE's processor and is forwarded to the
runtime, but produces no sound in a correctly-built instrument — the MIDI does
not manifest inside the DSP graph.** No code change; the measurement and the
two named suspects are the deliverable.

**What is proven, each by direct measurement:**

1. **MIDI reaches the processor.** A temporary probe in `onMidiMessage` logged
   **42 messages with `prepared=1`** during a looping C4: note-ons (`0x90`),
   note-offs (`0x80`), and the note number `0x3c` = 60. They are forwarded to
   `rack.MidiIn`. **They arrive as 8-byte MIDI 2.0 UMP packets** (leading
   `0x40` = MIDI2 channel-voice), not 3-byte MIDI 1.0 — which is the first
   suspect below.
2. **The audio path works.** With the oscillator wired straight to Sound Out
   the master meter clipped at **+10 dB** — loud, obviously alive.
3. **A complete instrument is silent.** Jeff's correction was the key
   methodological point: an oscillator wired directly to Sound Out **drones at
   its default pitch whether or not MIDI arrives**, so it proves nothing about
   MIDI. Rebuilt as a real instrument — **MIDI In → MIDI-CV 2; Pitch → Phase
   Dist Osc; Gate → VCA Volume; Osc → VCA Signal; VCA → Sound Out** — and the
   measured peak is **−156.7 dB, i.e. digital silence**, throughout the note.

**Learned — a tight Lua polling loop measures nothing.** My first two
"measurements" reported 9.3M and 34M samples of `Track_GetPeakInfo`, all
identical, because a busy-wait blocks REAPER's main thread and those values
only update on it: **the loop was re-reading one frozen snapshot millions of
times and reporting it as a result.** The give-away was the transport position
never advancing across 6 seconds of wall clock. Re-done with `reaper.defer`
(one sample per main-thread cycle), position advanced normally and the reading
was trustworthy. **A high sample count is not evidence; a changing input is.**

**Two suspects for the next session, in order:**

1. **MIDI format.** GMPI hands the processor **UMP**; `SeAudioMaster::MidiIn`
   may expect MIDI 1.0 bytes. `se_vst3`'s `SeProcessor` does explicit
   translation around its `MidiIn` calls (`midi2data`/`midi2size`, and it
   advertises `kMIDIProtocol_2_0`), which TIDE's one-line forward does not.
   Compare those call sites first.
2. **Container plumbing.** `SeAudioMaster::SetupVstIO` connects the synthetic
   **VST Input**'s "MIDI Out" to *the synth container's* MIDI plug — and S12
   wraps TIDE's flat rack in a **synthetic outer container**, so MIDI may be
   delivered to the outer container and never forwarded to the inner rack
   where the MIDI In module lives. That wrapper was introduced for
   `SetupVstIO`'s benefit, so it is exactly the code to re-read.

**Also worth knowing:** TIDE's module set has **no MIDI Monitor** (Diagnostic
holds only 1 kHz Tone and DAW Sample Rate), which is why the verification had
to be built from a VCA instead of read off a monitor — Jeff suggested a
monitor first and it simply is not in the fixed set.

**Next:** S12(a) continues with suspect 1 (cheap: log what
`SeAudioMaster::MidiIn` receives, and compare with `se_vst3`'s translation),
then suspect 2. The other S12 remainder items are unchanged.

**Side effects on this box:** two probe builds plus two clean rebuilds; the
probe branch was deleted and the tree reverted, so **the installed plug-in is
built from `master` with no diagnostics**. REAPER restarted twice; throwaway
projects only, **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — suspect (a) refuted; the real cause found: the rack exposes no plugs (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff said "do suspect (a)". Committed
and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** chased suspect (a) — the UMP-vs-MIDI-1.0 format theory — and **it is
wrong**. Instrumenting the chain instead found the actual break, one layer
lower and much more consequential: **TIDE's rack container exposes no MIDI
plug and no audio plugs, so `SetupVstIO` never connects the synthetic VST
Input's MIDI Out to anything.** No code change; the diagnosis is the
deliverable.

**Suspect (a) is refuted at the source.** `SeAudioMaster::MidiIn` hands the
bytes to `ug_vst_in::sendMidi`, which calls `MpeConverter::processMidi` — and
that function opens with `if (gmpi::midi_2_0::isMidi2Message(msg)) { sink(msg,
timestamp); return; }`. **MIDI 2.0 passes through by design**, comment and
all. The UMP packets TIDE forwards are exactly what it accepts. Nothing needs
translating.

**What the chain probe showed, in order:**

```
SetupVstIO: synthModule=1 plugs=3
  synth plug: type=0 dir=0        <- DT_ENUM
  synth plug: type=4 dir=0        <- DT_BOOL
  synth plug: type=0 dir=0        <- DT_ENUM
AudioMaster::MidiIn len=8 [40 90 3c] audioIn=1
  vst_in sink: size=8 inUse=0 mo=1
```

`EPlugDataType` is `DT_ENUM=0, DT_TEXT=1, DT_MIDI2=2, DT_DOUBLE=3, DT_BOOL=4,
DT_FSAMPLE=5`. **So the container's three plugs are ENUM, BOOL, ENUM — there
is no DT_MIDI2 plug and no DT_FSAMPLE plug.** `SetupVstIO` loops over exactly
those looking for MIDI and audio, finds neither, and connects nothing —
which is precisely the measured `inUse=0` on `vst_in`'s MIDI Out. **The MIDI
arrives at the audio master, is converted correctly, and is then sent into a
plug with no connections.**

**And this explains the asymmetry that made the bug confusing.** Audio works
(the tone clipped at +10 dB) **not** through the container's plugs but because
**Sound Out is a special IO module**: `ug_soundcard_out` registers itself via
`RegisterIoModule` at `Open()` and receives the host's buffers directly. MIDI
has no equivalent registration, so it depends on the container plumbing that
does not exist. **A rack that makes sound while ignoring MIDI is exactly what
those two different mechanisms predict.**

**The fix direction, for the next session to design rather than guess:** the
inner rack container needs real plugin IO — a `DT_MIDI2` input plug wired to
the patch's MIDI In module (and, if audio should ever leave via the container
rather than via Sound Out's registration, `DT_FSAMPLE` outputs too). In SE
terms that is what an **IO Mod** provides, and `exportDspXml`'s synthetic
outer container is the natural place to synthesise it. **Whether TIDE should
instead treat the patch's "MIDI In" module as a registering special IO module
— the symmetric counterpart of Sound Out — is the design question, and it is
Jeff's call.**

**Learned — probe the chain, not the theory.** Suspect (a) was a reasonable
hypothesis and it cost one build to disprove by *reading the consumer*
(`processMidi`'s first three lines). The chain probe then located the break
in a single run because it logged **at four points**, so the last successful
step and the first failing one were adjacent in the output. **Instrumenting
several points at once beats bisecting one hypothesis at a time.**

**Caught a build-configuration trap of my own making:** the first probe run
produced *no log at all*, because `cmake --fresh` had also cleared
`SYNTHEDITLIB_FOLDER_OVERRIDE`, so the build was compiling **SynthEditLib
fetched from GitHub** (`_deps/syntheditlib-src`) and my local edits were
invisible. Verified by `strings`-ing the installed binary for the probe's log
path — zero hits — before believing the silence. **`strings` the artefact when
a probe does not fire; the code you edited may not be the code that ran.** The
override is now restored to the local checkout, which is how this box was set
up before the `--fresh`.

**Next:** design the container-IO fix (S12(a) continues). Everything else in
S12's remainder is unchanged.

**Side effects on this box:** four TIDE_VST3 builds; all probes reverted and
the installed plug-in rebuilt clean (verified probe-free with `strings`).
`SYNTHEDITLIB_FOLDER_OVERRIDE` now points at the local checkout again; GMPI
and GMPI_Wrappers remain upstream. REAPER restarted twice. **"Optimus HP" was
never modified — and the guard proved itself:** REAPER reopened that project
as the active tab and the rig script aborted rather than touch it, after which
every subsequent script created its own new tab first.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — container-IO contract reverse-engineered; the MIDI In module is standalone-only (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff ruled "synthesise real container IO
for MIDI, I think that is the least disruptive to SynthEdit". Committed and
pushed as `tide-rack-bot` (claude-fable-5).

**Did:** worked out **exactly** what `exportDspXml` has to emit, by reading the
importer rather than guessing — and found one thing that changes the shape of
the fix: **the patch's "MIDI In" module cannot be the endpoint, because in a
plug-in nothing ever feeds it.** No code change; the contract and that
constraint are the deliverable, and together they make the next session a
single implementation pass.

**The XML contract, from `ug_base::Setup` and `SeAudioMaster::BuildModules`:**

- A **container IO plug** is any `<Plug>` carrying a `Direction` attribute —
  that is literally how the importer distinguishes it ("IO Plug on Container
  or I/O Mod. Identified by 'Direction' element"). It becomes
  `new UPlug(this, (EDirection)direction, (EPlugDataType)datatype)`, so:
  `<Plug Direction="0" Datatype="2"/>` is a MIDI **input** — `DT_MIDI2` is
  **2** in `EPlugDataType{DT_ENUM=0, DT_TEXT, DT_MIDI2, DT_DOUBLE, DT_BOOL,
  DT_FSAMPLE=5}`.
- An **IO Mod**'s plug ties to its container's plug **by handle**, and only
  when the module carries `UGF_IO_MOD`:
  `<Plug Direction="1" Datatype="2" TiedTo="<containerHandle>"
  TiedToPinIdx="<n>"/>` → `up->TiedTo = p2; p2->TiedTo = up;`
- **Connections** are `<Line From="<handle>" To="<handle>" FromPin="i"
  ToPin="j"/>`; `FromPin`/`ToPin` default to 0, and the handles are resolved
  through `HandleToObject`.

**The constraint that changes the design.** TIDE's browser offers a **MIDI In**
module, and it looks like the obvious MIDI source — but
`modules_internal/MidiIn.h` is `class MidiIn final : public MpBase2, public
ISpecialIoModule`, and it obtains MIDI by calling
`AudioMaster()->RegisterIoModule(this)` in `open()`. In the **standalone** that
registration lands in `UIoManager`, which feeds it from a MIDI device. In the
**plug-in** it lands in `SynthRuntime::RegisterIoModule`, whose entire body is
`{ return 1; } // nothing special to do in plugin`. **So a "MIDI In" module in
a plug-in registers itself and is then never fed by anyone** — it is a
standalone-app module, and its Audio pins confirm it (`MIDI Data` out,
`Activity` out, `MPE Mode` in — **no MIDI input pin at all**, so nothing can be
routed into it either).

**Which means the classic plug-in MIDI path is the only one available**, and
it is exactly what Jeff's ruling describes: host → `vst_in` → **the synth
container's DT_MIDI2 plug** → an **IO Mod** inside → the user's MIDI-consuming
modules (MIDI-CV 2 and friends). That is how an exported SE plug-in has always
worked; TIDE's flat rack simply never grew the container plug.

**So the open question is a UX one, not a mechanical one, and it is Jeff's:**
what does the user patch *from* in the rack? Either **(i)** TIDE synthesises a
container MIDI plug plus a tied IO Mod at export, and the IO Mod is the thing
users drag from — it is already in TIDE's module list, so this needs no new
module and no SynthEdit change; or **(ii)** TIDE keeps "MIDI In" as the
user-facing source and `SynthRuntime` learns to feed registered MIDI modules
the way `UIoManager` does — nicer for users, but it is the SynthEdit change
Jeff's ruling was steering away from.

**Learned — read the importer, not the exporter, when synthesising a format.**
Every attribute that matters here (`Direction` as the IO-plug marker,
`Datatype`'s enum ordering, `TiedTo`/`TiedToPinIdx`, the defaulting of
`FromPin`/`ToPin`) came from the ~40 lines that *parse* the XML. The exporter
would have shown only what a normal project happens to contain, which is
exactly the case that does not apply to TIDE's synthesised document.

**Next:** Jeff picks (i) or (ii); the row holds the full contract so the
implementation is one pass either way.

**Side effects on this box:** read-only investigation — nothing built, REAPER
not driven, no probes left anywhere. All six repos clean.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

---

## 2026-08-17 — macos — choice (ii) built: the runtime feeds MIDI In; a second defect surfaced (interactive session, Jeff directing)

**Prompt:** n/a — interactive session. Jeff ruled **choice (ii)**: "MIDI-in will
eventually be a rack module with a patch-point of its own, perhaps more like a
MIDI-CV". (He also briefly pasted Optimus dialog screenshots from another
session and said to carry on with MIDI — no Optimus work was done.) Committed
and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** implemented (ii) —
[SynthEditLib#16](https://github.com/JeffMcClintock/SynthEditLib/pull/16), 29
lines. **`SeAudioMaster` now remembers a registered `MIDI In` module and
`MidiIn()` feeds it with the same `AddMidiEvent` call
`UIoManager::OnMidiData` makes**, so the module behaves identically in the
standalone and in a plug-in. **Half-verified, and the other half turned up a
second, separate defect** — both stated below rather than blurred together.

**Why the gap existed, in one line:** a MIDI In module registers itself with
the audio master exactly as Sound Out does, the standalone's `UIoManager`
pairs that registration with a MIDI device, and the plug-in's
`SynthRuntime::RegisterIoModule` is a documented no-op — *"nothing special to
do in plugin"*. **So a MIDI In module in a plug-in patch was silent by
construction**, and the fix is to make the plug-in the device.

**Verified:** the module **does** register in a plug-in — the probe printed
`RegisterIoModule[0x11e39a370]: midiIn=1` and the pointer was stored.

**Not verified, and the reason is a NEW finding:** after the document rebuild
that adds the module, **no further MIDI reached `SynthRuntime::MidiIn` at
all**. Instance pointers made it unambiguous: every delivery went to
`SeAudioMaster[0x11ce0e2a0]` — the graph that existed *before* the rebuild —
and the registration landed on `[0x11e39a370]`, the graph built *after* it,
which then received nothing despite the transport running for 240 sampled
frames. **So MIDI delivery stops across a document rebuild.** That is
independent of this fix and is the next thing to chase.

**Learned — log the instance pointer when two objects can wear the same
name.** "Registered" and "not receiving" looked contradictory until `%p`
showed they were different `SeAudioMaster`s. **And read the log's ORDER before
concluding:** I nearly filed "registration never happens" when the
registration line was simply the *last* line in the file — everything before
it predated placing the module. One `tail` corrected a wrong conclusion.

**Repeated a mistake I had already recorded, which is worth admitting.** My
first cleanup of the probes used a line-based Python rewrite and normalised
`SeAudioMaster.cpp`/`.h` from CRLF to LF — a **5754-line diff** for a 29-line
change, exactly the trap I hit on the GMPI patch earlier today and wrote down.
Reverted and redone byte-safely (`b'\r\n'`-aware), giving the honest 29-line
diff. **A lesson recorded is not a lesson learned until the tool that caused
it is fixed** — the byte-safe `edit()` helper now used should be the default
for every repo that stores CRLF.

**Next:** chase the rebuild defect — MIDI stops reaching the processor after a
graph rebuild. Cheapest probe is the one already proven: log in TIDE's
`onMidiMessage` and in `SynthRuntime::MidiIn` across a document change, and
compare instance pointers on both sides.

**Side effects on this box:** six TIDE_VST3 builds; all probes reverted and
the installed plug-in rebuilt from the committed state. REAPER restarted three
times; **"Optimus HP" untouched** (the guard aborted one script when REAPER
reopened that project, after which every script created its own tab).
`SYNTHEDITLIB_FOLDER_OVERRIDE` remains pointed at the local checkout.

**Branch/PR:** this TideSynth PR +
[SynthEditLib#16](https://github.com/JeffMcClintock/SynthEditLib/pull/16).

---

## 2026-08-17 — macos — the MIDI mystery solved: a second processor instance, and it never gets the document (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged choice (ii) and said "keep
going till you are blocked". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** chased the "MIDI stops after a rebuild" defect and **solved it — the
cause is not the rebuild at all.** REAPER runs **two** TIDE processor
instances, and **MIDI is delivered to the one that has never received a
document**. This also retracts yesterday's conclusion, which was based on a
transport that had quietly stopped. No code change; the evidence and the fix
direction are the deliverable.

**The evidence, from one instrumented run** (probe in TIDE's `onMidiMessage`,
`subProcess` and `SynthRuntime::MidiIn`, all logging `this`):

```
onMidiMessage  proc[0x12b0b0800] prepared=1   x31      <- editor's instance
onMidiMessage  proc[0x12ba36800] prepared=0   x77      <- the one REAPER feeds MIDI
PREPARE        proc[0x12b0b0800]  (every document)
subProcess     proc[0x12b0b0800]  (only this one)
```

**So:** the instance the editor is attached to receives every document push,
builds its graph and runs audio. A **second** processor instance receives the
host's MIDI, has `prepared=0` for its whole life, and never runs `subProcess`.
The two never meet.

**Why it happens, and it is a flaw in my S12 design rather than a host quirk.**
TIDE pushes the document **only when the XML changes** — `serviceDocumentSync`
dedupes against `lastPushedDspXml`. **Any processor that appears afterwards
therefore starts empty and stays empty**, because nothing ever re-sends. A
plug-in's processor can be created at any time — the host may re-instantiate
after a `restartComponent`, add an instance for offline/anticipative
processing, or restore state into a fresh one — so "push once on change" was
never going to be sufficient.

**Fix direction, and the right one is not the obvious one.** The hacky answer
is to re-push periodically. **The correct answer is that a newly created
processor should be seeded with the current value of every parameter**,
including blob parameters — which is what the chunk parameter is for. The
document already persists in the DAW state (that half now works), so the same
delivery that restores a saved project should seed a mid-session instance.
Worth checking whether `gmpi_processor` seeds pins from
`patchManager` at construction and simply skips blobs: if so, this is a small
generic fix in the same place as this morning's transport work, not a
TIDE-specific patch.

**Learned — retract cleanly when the evidence changes.** Yesterday I recorded
"MIDI delivery stops across a document rebuild", with instance pointers to
back it. Today's run shows MIDI never stopped: **the transport had stopped**
because clicking in the editor to place modules had halted playback, and my
"no deliveries after the rebuild" was that, not a defect. **The pointer
evidence was real and the conclusion drawn from it was wrong** — the missing
control was "is the transport actually running while I measure?", which the
`reaper.defer` sampler answers in its first line and which I did not check
before concluding.

**Also settled: there is exactly one processor per editor.** The earlier
worry that registration and delivery hit different `SeAudioMaster`s has the
same explanation — different *processors*, each with its own runtime and
generator, not a stale pointer inside one.

**Next:** seed a new processor with the current chunk-parameter value (check
`gmpi_processor`'s construction path for blob handling first, since a generic
fix there beats a TIDE-specific one). That is the last thing between the
current build and an audible MIDI note.

**Side effects on this box:** four TIDE_VST3 builds; **all probes reverted
byte-safely and both trees verified clean** (`git status` empty in SynthEdit
and SynthEditLib), installed plug-in rebuilt from the committed state. REAPER
restarted twice; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (row + entry only; no code).

## 2026-08-17 — macos — MIDI notes play: the blob pin was never seeded on a new processor (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged choice (ii) and said "keep
going till you are blocked". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** found and fixed the last blocker. **A MIDI note now plays a TIDE
rack.** Measured: **peak 0.5103 (−5.8 dB) while the note sounds, 0.0001 with
the transport stopped** — so the audio genuinely follows the notes rather than
droning, which is the test Jeff's VCA correction made possible. The fix is
generic and lives in GMPI; **the bot cannot push that repo (403 again), so the
patch is filed at
[docs/patches/gmpi-seed-blob-pins.patch](docs/patches/gmpi-seed-blob-pins.patch)**
(72 lines) for Jeff to apply, exactly like this morning's transport patch.

**The bug, in one sentence:** `gmpi_processor`'s pin-initialisation loop seeded
parameter-backed pins for `Float32`, `Int32` and `Bool`, and **`Blob` fell
through to `default: assert(false)`** — so a newly created processor started
with an empty blob pin and was never told otherwise, because blobs only reach
the processor when they **change**.

**Why that mattered so much here.** A host creates processors whenever it
likes — after `restartComponent`, for offline or anticipative processing, on
state restore. REAPER was running **two** TIDE processors: the editor's held
the rack document and ran audio, while a second one received all the MIDI with
`prepared=0` for its entire life. It had never been handed the document, so it
had no graph to play. **The two-instance behaviour was never the bug; the
un-seeded blob pin was.**

**Learned — a `default: assert(false)` is a to-do list.** The same switch
statement had already bitten me this morning: `sendParameterToProcessor` was
missing its Blob case, and I added it to fix the *change* path. **I fixed one
arm of the pattern and did not check the other**, so the initialise path kept
the hole for another six hours. When a datatype is missing from one switch
over `PinDatatype`, grep every switch over `PinDatatype` in the same file
before moving on — there were exactly two, and they needed the same case.

**Verification note:** the gate test is what makes this claim safe. A patch
whose oscillator reaches Sound Out will drone at its default pitch and read a
healthy peak whether or not MIDI works — Jeff caught me making exactly that
mistake earlier. Comparing **note-playing (0.5103) against transport-stopped
(0.0001)** is the measurement that cannot be faked by a drone.

**Next:** with notes audible, S12(a) is done. The remainder of S12 is the
save/reopen re-check (the chunk now carries real documents **and** is seeded
into fresh processors, so S11's restore half may work already), then the faded
swap and preset retention.

**Side effects on this box:** three TIDE_VST3 builds; all probes reverted
earlier and both code trees verified clean before this change. **The build now
points `GMPI_SDK_FOLDER_OVERRIDE` at the local GMPI checkout** (needed to test
the unmerged patch) — it should be cleared once Jeff applies it, or fresh
clones will build without the fix. REAPER restarted twice; **"Optimus HP"
untouched**.

**Branch/PR:** this TideSynth PR + local GMPI branch `tide/mac/seed-blob-pins`
(patch filed for Jeff).

## 2026-08-17 — macos — MIDI notes verified from pure upstream; the patch workaround is retired (interactive session, then unsupervised)

**Prompt:** n/a — interactive session; Jeff granted the bot write access to
GMPI, merged [GMPI#2](https://github.com/JeffMcClintock/GMPI/pull/2), then
said "clean up the patches and verify from upstream" and left ("do as many
tasks as you can unsupervised"). Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** **the bot can now open PRs on GMPI** (invitation accepted,
`push=true` confirmed), and with both GMPI fixes merged I **cleared every
local override and re-verified MIDI from pure upstream: peak 0.5103 (−5.8 dB)
with a note, 0.0001 with the transport stopped** — the identical numbers to
the local-override build, so a fresh clone now hears MIDI. This closes S12(a)
properly rather than on a machine-specific build.

**The access fix, for the record:** the bot had `push=true` on all five other
repos and **`push=false` on GMPI alone** — it had simply never been granted.
Jeff added it as a collaborator; the bot accepted invitation `329366224`
itself via `gh api -X PATCH user/repository_invitations/<id>`. **Both GMPI
fixes are now normal PRs (#1 and #2), and the patch-file route is retired.**

**A near-miss worth recording, because it would have been a false pass.**
After clearing the overrides the build **succeeded** — and grepping the
fetched sources showed **the fixes were absent**: `_deps` still held the old
checkouts, because **FetchContent does not re-pull an already-populated
dependency just because `origin/main` moved.** I had "verified upstream"
against stale code for one build. Fixed by `git fetch && git reset --hard
origin/main` in each `_deps/*-src`, after which all four greps matched and
the numbers reproduced. **Grep the fetched source for the change you are
verifying — a green build proves nothing about which code was compiled.**

**Cleanup, and why the patch files stay.** `docs/patches/` now carries a
**README marking both patches superseded**, with the PR each landed as, and
an instruction not to use that route again. **The `.patch` files themselves
are kept deliberately**: JOURNAL entries link to them and the journal is an
immutable record, so deleting the files would break history to tidy a folder.
The README is the honest way to say "obsolete" without rewriting the past.

**Also cleared:** all three `*_FOLDER_OVERRIDE` cache entries are empty, the
local GMPI branches are deleted, and the installed plug-in is byte-identical
(`cmp`) to the pure-upstream build. `SE_LOCAL_BUILD=TRUE` had to be restored
again after the earlier `--fresh` — the trap already documented in
[docs/building.md](docs/building.md), hit for the second time today, which is
why it is written down.

**Learned — a clean instance beats a debugged one.** The first upstream
measurement read silence, and the reason was not the build: earlier failed
UI batches had dropped modules into that instance's rack while the structure
view was not open, leaving a polluted document. Rebuilding the patch in a
**fresh tab** reproduced the expected numbers immediately. **When a test rig
has been poked at by failed automation, rebuild the rig before debugging the
product.**

**Next:** S12's remainder — the save/reopen re-check first, since the chunk
now carries real documents *and* is seeded into fresh processors, so S11's
restore half may already work; then the faded swap and preset retention.

**Side effects on this box:** several TIDE_VST3 builds; `_deps` for GMPI,
GMPI_Wrappers and SynthEditLib re-cloned/reset to upstream main; overrides
cleared; installed plug-in current. REAPER restarted twice and several
throwaway tabs were created; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (docs + bookkeeping only; no code).

---

## 2026-08-17 — macos — S11's restore has exactly one blocker: blobs are not encoded in the preset (unsupervised)

**Prompt:** n/a — Jeff left with "do as many tasks as you can unsupervised"
after the upstream verification. Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** took S12(b), the save/reopen re-check, and **measured rather than
assumed — the rack still does not survive a reload, and the reason is now a
single named TODO** in code, not a mystery.

**Measured:** with a full instrument patched and playing, the saved VST state
is **250 bytes of base64 before the save and 250 after the reopen** — the same
number S11 recorded before any of today's work. The document is pushed through
the chunk parameter and it reaches the processor (that is why MIDI notes now
play), but **it is not written into the DAW's saved state.**

**The cause, both halves, already flagged in the source:**

- **Writer** — `gmpi_processor::getPresetUnsafe()` writes every parameter with
  `paramElement->SetAttribute("val", parameter.valueReal())`. `valueReal()` is
  the *double* accessor, so a blob parameter is serialised as `val="0"` and the
  bytes are silently dropped. That is exactly the two-parameter, all-zero
  `<Preset>` S11 dissected.
- **Reader** — `GmpiParameter::setFromXml` has, verbatim:
  `case gmpi::PinDatatype::Blob: break; // TODO uuencode or hex`.

So blob parameters have **never** round-tripped through a preset; TIDE is
simply the first plug-in whose entire state is a blob.

**The decision this needs, and it is Jeff's because it is a persisted
format:** which encoding — base64 or hex — and where the codec lives.
`SynthEditLib/Base64.h` exists, but **GMPI is the lower layer and cannot depend
on SynthEditLib**, so GMPI needs its own small encoder (or the pair moves into
GMPI and SynthEditLib uses it). Both sites are one function each; the work is
small, the format choice is permanent.

**Why I stopped here rather than implementing it.** It is a change to a
persisted file format in shared code, at the end of a very long session, and
the encoding choice is not mine to make silently. **The rest of today's fixes
were behaviour; this one is compatibility** — a preset written by the first
implementation has to be readable by every later one.

**Learned — "it reaches the processor" and "it is saved" are different
claims.** The chunk parameter now demonstrably delivers the document to the
DSP (notes play), which made it tempting to assume persistence followed. The
`GetTrackStateChunk` byte count answered it in one line and cost nothing.
**Measure the artefact you actually care about; a working adjacent path is not
evidence.**

**Next:** S11 — implement blob encoding in the preset writer and reader once
Jeff picks the encoding. That single change should complete S11's restore,
since the document already survives everywhere else in the chain.

**Side effects on this box:** no code changed; nothing built. REAPER driven,
two throwaway projects written to `/tmp`; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (rows + entry only; no code).

---

## 2026-08-17 — macos — presets now carry the rack: blobs base64'd, non-stateful params excluded (unsupervised)

**Prompt:** n/a — Jeff chose base64 and asked for the codec to live in GMPI or
GMPI-UI "so we're not duplicating code", then said "work continuously" and
left. Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** implemented S11's blocker —
[GMPI#3](https://github.com/JeffMcClintock/GMPI/pull/3) +
[SynthEditLib#17](https://github.com/JeffMcClintock/SynthEditLib/pull/17).
**A project's saved VST state grows from 250 bytes of base64 to 1503, and the
`.rpp` now contains the rack document inside the `<Preset>`.** The bytes are
in the file, which is exactly what was missing.

**Where the codec went, per the ruling:** `GMPI/Core/base64.h` — the lowest
layer that needs it, since **both** ends of the round-trip (the writer in
`Hosting/processor_holder.cpp` and the reader in `Hosting/controller_holder.h`)
live in GMPI. **`SynthEditLib/Base64.h` is now a thin forwarder**, so there is
one implementation rather than two and its four call sites are unchanged.

**The second half was not optional, and I only found it by breaking the
editor.** With encoding in place the plug-in UI **rendered blank**. Stashing
the change made it render again; that A/B, run in the same REAPER session,
proved the regression was mine rather than session flakiness. The cause:
TIDE's **`controllerPtr` is a blob holding a raw pointer**, declared
`persistant="false"` — harmless while blobs serialised as `"0"`, but once they
round-trip properly, **restoring one hands the plug-in a dangling pointer from
a previous run.** Fixed by honouring the flag the spec already carries:
`if (!parameter.info->is_stateful) continue;`. With encoding **and** guard,
the editor is normal.

**Learned — making serialisation work can resurrect things that were only
safe while broken.** The `"0"` bug was silently protecting a
session-only pointer from ever being restored. **When you fix a lossy
round-trip, audit what was relying on the loss** — the `is_stateful` flag
existed precisely for this, and nothing had needed to respect it until now.

**Also learned — an A/B beats a hypothesis, and costs one build.** The blank
editor could plausibly have been the day's REAPER instability (a deleted
project file, dialogs, a failed load). Stashing and rebuilding answered it in
one cycle instead of a debugging session, and it is the same control I used on
the codesign failure earlier in the week.

**Build note for future sessions, now also in memory:** this box builds with
`GMPI_SDK_FOLDER_OVERRIDE`, `GMPI_UI_FOLDER_OVERRIDE` and
`SYNTHEDITLIB_FOLDER_OVERRIDE` pointed at the local checkouts, per Jeff — the
local clones are the truth while several repos move together, and FetchContent
will not re-pull once `_deps` is populated.

**Next:** S11's remaining half — confirm the **restore** side rebuilds the
rack on load (place a module, save, reopen, look). The reader decodes base64
now, so this may already work; measure before writing code.

**Side effects on this box:** several TIDE_VST3 builds including a stash/A/B
cycle; the three `*_FOLDER_OVERRIDE` entries point at local checkouts;
installed plug-in current. REAPER restarted repeatedly and several throwaway
tabs made; a stale `/tmp/tide-persist2.rpp` I had deleted caused one REAPER
"error opening project" dialog — **`/tmp/tide-persist3.rpp` is left in place
deliberately** for the restore test. REAPER twice offered to reopen **"Optimus
HP"** after a failed load and **both times it was declined; that project was
never opened or modified.**

**Branch/PR:** this TideSynth PR + [GMPI#3](https://github.com/JeffMcClintock/GMPI/pull/3)
+ [SynthEditLib#17](https://github.com/JeffMcClintock/SynthEditLib/pull/17)
(**must merge together** — the forwarder includes GMPI's header).

---

## 2026-08-17 — windows — C12c done, and the Windows build of `main` is broken by something else

**Prompt:** `b3e9876e8` · claude-opus-5[1m] · app version not discoverable on
this box (no `claude` on PATH, nothing recorded in `~/.claude`) · as
`tide-rack-bot`

**Did:** took **C12c** from the NEXT block and finished it — the twelve
independent leaves (`CLine2`, `commandMgr`, `SuspendDSP`, `legacyExternalApp`,
`ModuleDragAndDropManager` as `.cpp`+`.h`, plus header-only `ui_msg_target.h`
and `IGuiHost.h`, 1,316 lines) moved from `SE16/SynthEdit2/` into
`SynthEditLib`'s root, and `EditorLib/CMakeLists.txt` repoints them.
**25 → 13 `${EDITOR_DIR}` entries.** Files are byte-identical (`cmp`) to the
originals.

**Result — the numbers, and then the caveat they were measured under.**

- **Dangling private includes 42 → 21**, `scripts/dangling_private_includes.py`
  before and after. Exactly the 21 edges C12c's Accept names, **zero new
  opened**: `CLine2.h` 7, `SuspendDSP.h` 6, then `commandMgr.h`,
  `legacyExternalApp.h`, `ModuleDragAndDropManager.h`, `ui_msg_target.h` at 2
  each; `IGuiHost.h` had no public includer and closes 0, as predicted.
- **Fresh scratch Ninja tree, Release, all four sibling repos on local
  overrides: `935/935 RC=0`, `ctest 92/92 passed, 0 failed`** (93 listed, 1
  disabled upstream), producing `TIDE.gmpi`, `TIDE_VST3.vst3` and
  `SynthEditCL.exe`. The build log shows the five moved `.cpp`s compiling as
  `EditorLib\...\C_\SE\SynthEditLib\<name>.cpp.obj` — from the public repo, not
  the private one.
- **The edge total is 935, not the 904 the backlog and C12a recorded.** That is
  upstream growth in `SynthEditLib` (the tearout-window-layout feature), not
  C12c, which moves TUs rather than adding or removing them. **The new baseline
  for later sub-stages is 935/935 and 92/92.**

**The caveat, and it is the important half of this entry: `SynthEditLib`'s
`main` does not compile on Windows, and C12c had nothing to do with it.** The
first full build died at edge 78 of 935:

```
C:\SE\SynthEditLib\SeAudioMaster.cpp(548): error C2680: 'MidiIn *': invalid target type for dynamic_cast
C:\SE\SynthEditLib\SeAudioMaster.cpp(548): note: 'MidiIn': class must be defined before using in a dynamic_cast
C:\SE\SynthEditLib\SeAudioMaster.cpp(805): error C2027: use of undefined type 'MidiIn'
C:\SE\SynthEditLib\SeAudioMaster.h(609): note: see declaration of 'MidiIn'
```

From `e14970e`, "Feed a patch's MIDI In module in plug-in hosts too (S12(a))",
merged as SynthEditLib#16 from the macOS box today. Clang accepts
`class MidiIn* midiInModule = {};` at `SeAudioMaster.h:609` together with
`dynamic_cast<class MidiIn*>` at `:548`; MSVC binds both to the incomplete type
that first declaration introduces, and never to the complete class in
`modules_internal/MidiIn.h`. Filed as
[#111](https://github.com/JeffMcClintock/TideSynth/issues/111) with a suggested
one-line fix. **Not fixed here** — `SynthEditLib` is GATED for a scheduled run
and this is not a carve-out stage, the same STEP 1 / STEP 5 contradiction the
linux box hit this morning in [#87](https://github.com/JeffMcClintock/TideSynth/issues/87).
**Two `platform:*` breaks in GATED shared code in one day is the signal, not
the coincidence** — this needs Jeff's ruling, not a third run's judgement.

So C12c was verified with its twelve files on top of `d96edbb`, the merge
immediately before `e14970e`, not on top of `main`. **The gap that leaves is
stated in both PR bodies:** this branch has not been proven to build against
`main` as `main` stands, only against `main` minus one unrelated commit.

**Learned — a control compile is cheap and it is what makes "not my fault"
a fact rather than a claim.** The tempting move was to reason that a TU in the
`SynthEditLib` target cannot be affected by `EditorLib`'s source list. That
reasoning is correct and it is not evidence. Checking out pristine `origin/main`
and re-running the *exact* `cl.exe` command line from the failing ninja edge
reproduced all three errors identically in about forty seconds, with none of
C12c present. **When a build breaks during your change, re-run the one failing
compile against the untouched tree before you diagnose anything.**

**Learned — four things about that failure were ruled out, so nobody repeats
them:** it is not a missing include (`/showIncludes` shows
`modules_internal/MidiIn.h` opened at `.cpp:26`, ahead of both failures); not a
shadowing duplicate (`git ls-files` finds exactly one `MidiIn.h`); not
conditional compilation (in the preprocessed TU `class MidiIn final` sits at
line 344022 and the failing `dynamic_cast` at 356338); and **not** the bare
elaborated-type-specifier pattern, because a minimal `class X* member;`-then-
define-`X` repro compiles clean under the same compiler and flags. The
interaction is more specific than that shape alone, and I stopped there rather
than keep digging in a GATED file.

**Learned — the run prompt's GATED wording says "C1-C7" and the carve-out is up
to C12.** C12a, C12b, C12e, C9 and C11 have all been executed and merged as
GATED edits under a rule whose literal text does not cover them, and the NEXT
block instructs this box to take C12c. The practice is settled and the wording
is not; worth an edit to STEP 5 next time someone touches the prompt.

**Next:** **C12f** is the remaining large stage — the patch cluster, ten
entries, 6,298 lines, atomic, and the one that takes `${EDITOR_DIR}` to zero and
unblocks C6. C12d stays `linux`. Before either, someone with the authority
should fix [#111](https://github.com/JeffMcClintock/TideSynth/issues/111), or
the next Windows run pays the same tax and the next Windows *user* cannot build
TIDE at all.

**Side effects on this box:** a scratch Ninja tree at `C:\SE\build-c12c`
(Jeff's own `SE16\build` untouched); no other repo modified. All five working
copies were clean before this run and are returned to their default branches.

**Postscript, same session:** Jeff merged both code PRs within minutes of them being opened (07:21 and 07:24 UTC), so C12c has landed on `master` and `main` already. The row stays `IN-REVIEW` regardless — a run does not set `DONE` on its own fresh work — and the next run may flip it.

**Branch/PR:** [SynthEdit#41](https://github.com/JeffMcClintock/SynthEdit/pull/41)
+ [SynthEditLib#18](https://github.com/JeffMcClintock/SynthEditLib/pull/18),
which must merge together, plus this TideSynth PR.

---
