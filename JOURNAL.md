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
2. Stop when this file is **under 60 KB**, or when the floor is reached —
   whichever comes first. **The floor is the LATER of: the four most recent
   entries, or every entry carrying the most recent date.** The floor always
   wins; a busy day pushing this file over 60 KB is correct, not a rotation
   failure.
3. Never edit an entry while archiving it. The archive is the record.

**Why a date and not a duration (A24, 2026-08-20).** A24 asked for a time-based
floor — *"retain everything from the last 7 days"* — and measuring what that
costs is what killed it. Entries per day, counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it, so 7 days is **3.4× the 192 KB that
triggered A8 in the first place** — the remedy would have been twenty times more
expensive than the problem. Even two days is worse than the state A8 was created
to fix.

So the floor is **one date**, which bounds the cost at roughly a day's work while
guaranteeing a run can always see everything that happened most recently — the
failure A24 correctly identified, where a 4-entry floor at ten entries a day
bought under half a day. On a quiet week the four-entry floor still binds and
nothing changes.

**What this does NOT fix, filed as A30:** the durable lessons still age out.
Rotation moves an entry's *"Learned"* bullets into the archive with it, and no
run reads the archive. The cheap answer is a standing digest that never rotates;
the expensive one is reading 651 KB.

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

## 2026-08-22 — linux — N1b: the rename's live docs, and a Linux-only gap N1a could not have seen

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** took **N1b** (unblocked once N1a's merged PR let me flip it DONE), and
found a packaging regression on the way to the ground truth I needed.

### The finding is worth more than the item: #271

Building `main` here — the **first Linux build since N1a landed** — is green
(rc=0, 0 errors), but the VST3 bundle is not:

```
TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE-Rack.so
^^^^^^^^^^^^^^^                           ^^^^^^^^^
```

The directory kept the **target** name; the payload took `OUTPUT_NAME`.
`gmpi_plugin.cmake`'s own comment says a host scanning `~/.vst3` looks for
`<name>.vst3/Contents/<arch>-linux/<name>.so` — the two must match, and before
N1a they did (`TIDE_VST3.vst3/…/TIDE_VST3.so`).

**Why mac and Windows verification could not catch it:** they never build that
path by hand. macOS gets it from `BUNDLE_EXTENSION` and Windows from `SUFFIX`,
and both resolve through `OUTPUT_NAME` for free. Linux is the only platform
where the bundle path is spelled out (`:853`), and it is the one platform N1a
could not be checked on. **The same shape as N1a's own commit title** — *"a
rename that skipped work silently"* — happening once more, one platform along.

It reaches the user, not just the build tree: `copy_plugin()`'s Linux VST3 branch
copies by target name on both sides, so `~/.vst3` gets the same mismatch. The
`.gmpi`/`.clap` branch uses `$<TARGET_FILE:…>` and is already correct, which
narrows the fix to one hand-spelled path in two places. Filed as
[#271](https://github.com/JeffMcClintock/TideSynth/issues/271) with the suggested
one-liner; **not fixed** — `gmpi_plugin.cmake` is in GMPI, which is PR-GATED, and
I could not find a TIDE-side fix because TIDE does not control that variable.

### N1b itself: the triage corrected N1's own bucketing

I wrote N1's cost model yesterday and put `docs/state-of-the-prototype.md` in
"live reference docs" **on the strength of its filename**. Reading it says
otherwise — *"Observation only. … Everything below was seen"*, dated 2026-08-06.
Its REAPER Lua transcript, its crash-report text and its P5 finding about the FX
browser would all be **falsified** by a rename. Same for `p4-resize-crash.md`,
whose PDB names sit inside a measured before/after byte table.

So both are **annotated, not rewritten** — against what the N1b row expected of
me. I have said so in the row rather than doing it silently, because it is a
judgement Jeff may want to overrule.

Genuinely live, and updated:

| doc | what was stale |
|---|---|
| `docs/building.md` | the build command, the two-target trap, the macOS copy line |
| `docs/ci/linux-build-deps.md` | build-and-run instructions |
| **`docs/ci/headless-gui-verification.md`** | **wrong within a day of my writing it** |
| `docs/n1-tide-rack-rename.md` | its status line still said TODO |

The headless doc is the one worth noting: I wrote it yesterday and it told the
next run to launch `./TIDE_STANDALONE`, which no longer exists. I corrected it
and **re-ran the recipe end to end** rather than assuming — the renamed binary
comes up under headless weston, prints its command channel, and `pgrep -x
TIDE-Rack` matches, so the shutdown line still works too.

Rather than stamp eleven banners on eleven dated records, `docs/building.md`
gains one **Current target and artifact names** table, so there is a single
authoritative place to check and the records stay untouched.

**Learned:**

1. **Classify a doc by reading its opening, not its filename.** "state-of-the-
   prototype" sounds like current state; it is a dated observation report. My own
   cost model got this wrong 24 hours earlier, and only reading fixed it.
2. **A doc you wrote yesterday is not exempt from going stale.** The headless
   recipe was obsolete within a day, by someone else's merge. Grep your own
   output when a rename lands.
3. **The first build on a platform after a cross-platform rename is a real
   test.** Nothing failed, exit code 0 throughout — the defect is a *name*, and
   only comparing two names caught it.
4. **When two things must agree, check them against each other, not against
   spec.** Both halves of the bundle path were individually defensible; only
   putting them side by side showed the mismatch.
5. **"Verified on two platforms" is not "verified".** N1a was checked on mac and
   Windows and was correct on both, by two different mechanisms — neither of
   which is the mechanism Linux uses.

**Next:**

1. **#271** — one-line fix in GMPI (`$<TARGET_FILE_BASE_NAME:…>` for the bundle
   dir, in both the assembly and the copy). PR-GATED: happy to raise the GMPI PR
   on request, but not unilaterally.
2. **The v0.1 fixtures now name `TIDE-Rack.vst3`** while Linux emits
   `TIDE_Rack_VST3.vst3`. Undetectable here (no REAPER); it resolves itself when
   #271 lands.
3. **N1b's annotate-don't-rewrite call** is Jeff's to overrule if he wanted those
   two files edited.

**Machine left clean.** TideSynth back on `main` after the PR; weston and the
standalone both stopped by pid (**S31**), scratch `XDG_CONFIG_HOME` throughout so
`~/.config/TIDE Rack/` is untouched. The gmpi_ui working tree is now clean — its
2026-08-19 edit went out as gmpi_ui#10 earlier today.

**Branch/PR:** `tide/linux/N1b-live-docs` — TideSynth only. No code change.
## 2026-08-22 — macos — N1a: OUTPUT_NAME renamed three things, and only one of them had an extension

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1.5, not a backlog item. [#268](https://github.com/JeffMcClintock/TideSynth/pull/268) is this platform's own
open PR and its `windows` check was red, which outranks new work. Fixed the
cause, on the same branch, and re-ran N1a's full Accept against a binary built
from the tree being pushed.

### The failure, and why two of three platforms said nothing

```
LINK : fatal error LNK1201: error writing to program database
  'D:\a\TideSynth\TideSynth\build\SynthEditSem\Release\TIDE-Rack.pdb';
  check for insufficient disk space, invalid path, or insufficient privilege
  [...\TIDE_Rack_VST3.vcxproj]
   Creating library ...\Release\TIDE-Rack.lib and object ...\Release\TIDE-Rack.exp
   TIDE_Rack.vcxproj -> ...\Release\TIDE-Rack.gmpi
```

Read those three lines together and the message is a lie about its own cause.
`TIDE_Rack.vcxproj` is writing `TIDE-Rack.lib` at the same moment
`TIDE_Rack_VST3.vcxproj` fails to write `TIDE-Rack.pdb`. It is not disk space.
**`OUTPUT_NAME "TIDE-Rack"` renames three artifacts per target, not one** — the
module, the linker PDB, and the import library — and all three format targets
(`TIDE_Rack`, `TIDE_Rack_VST3`, `TIDE_Rack_STANDALONE`) build into the SAME
directory on Windows. Only the module is disambiguated, by its extension
(`.gmpi` / `.vst3` / `.exe`). The PDB and the `.lib` are not, so three targets
raced for one `TIDE-Rack.pdb` under MSBuild's parallel link.

**macOS and Linux were green on the identical commit**, and that is the whole
trap: neither emits a PDB, neither emits an import library for a MODULE, and on
macOS each artifact is a bundle so even the paths differ. A rename verified
end-to-end on this box could not have shown it here.

Fixed by keying the two side-channel names off the TARGET name, which is unique
by construction (`${PROJECT_NAME}_${kind}`):

```cmake
set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "TIDE-Rack"
    PDB_NAME "${SUB_PROJECT_NAME}"
    COMPILE_PDB_NAME "${SUB_PROJECT_NAME}"
    ARCHIVE_OUTPUT_NAME "${SUB_PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_NAME "TIDE Rack")
```

That also restores the pre-rename convention rather than inventing one: the
PDBs were `TIDE.pdb` / `TIDE_VST3.pdb`, i.e. target-named, which is what
`docs/state-of-the-prototype.md:106` and `docs/p4-resize-crash.md` still record.
Those two are live reference docs and belong to **N1b**, so they are untouched.

### The macOS half is proven inert, by construction rather than by re-rendering

Configured the branch twice — once with the fix, once with it stashed — and
diffed the ENTIRE generated build system under `build/SynthEditSem`, with the
build-directory name normalised away. **Three files differ, and every difference
is a PDB filename:**

| target | before | after |
|---|---|---|
| `TIDE_Rack` | `TIDE-Rack.gmpi/Contents/MacOS/TIDE-Rack.pdb` | `…/TIDE_Rack.pdb` |
| `TIDE_Rack_VST3` | `TIDE-Rack.vst3/Contents/MacOS/TIDE-Rack.pdb` | `…/TIDE_Rack_VST3.pdb` |
| `TIDE_Rack_STANDALONE` | `TIDE-Rack.pdb` | `TIDE_Rack_STANDALONE.pdb` |

All three `link.txt` files are byte-identical. **The baseline column is also the
proof of the diagnosis** — three targets, one PDB name — obtained without a
Windows machine.

### N1a's Accept, re-run against this tree's own binary

Configure rc=0, build rc=0, zero `error` lines. `TIDE-Rack.gmpi`,
`TIDE-Rack.vst3` and `TIDE-Rack.app` all emitted; targets still `TIDE_Rack*`;
`lipo -archs` = `arm64`, as ruled 2026-08-21.

The build does **not** copy the VST3 into `~/Library/Audio/Plug-Ins`, so
rendering straight away would have measured the previous session's bundle. It
was installed deliberately and the identity checked rather than assumed —
installed and built binaries both
`009060be0f4852280bd89e4cabfc3277df0f3040f36d1e15af9232950c5fe816`. Jeff's stale
`TIDE_VST3.vst3` (16 Aug, same plugin ID) was parked for the duration, so
`TIDE-Rack.vst3` was the only candidate, and **restored afterwards**.

| fixture | this run | 2026-08-21 |
|---|---|---|
| `--control` | PASS, −6.0 / −9.0 | PASS |
| `v1-rack` | −6.3 / −17.0, 2 cables | −6.3 / −17.0 |
| `v1-rack-midi` | −6.3 / −17.0, 4 cables | −6.3 / −17.0 |
| `v3-midi-pitch` | −6.2 / −21.1, 4 cables | −6.2 / −21.1 |
| `v3-midi-gate` | −6.3 / −21.2, 3 cables | −6.3 / −21.2 |
| `v1-rack-uncabled` | **silence**, 0 cables | silence |

### The Windows link, which this box cannot compile — verified by CI, then waited for

There is no MSVC here, and STEP 3 forbids fixing a platform blind. This is not
that: the break is this platform's own PR, STEP 1.5 makes it mine, and **the
PR's own `windows` job is the verification** — which is why the fix went to the
same branch, and why the run stayed up for it rather than declaring victory on
a mechanism.

[Run 32492249466](https://github.com/JeffMcClintock/TideSynth/actions/runs/32492249466), on `276e150`:

| job | before (`ea6a1e1`) | after (`276e150`) |
|---|---|---|
| `guard` | success | success |
| `linux` | success | success |
| **`windows`** | **failure — `LNK1201`** | **success** |
| `macos` | success | queued (S30) |

**`windows` went red → green on a one-block change, with `linux` green on both
ends as the control.** That is the A/B this platform could not run locally.

`macos` is still queued and **carries no information about this change** — it
was green on the previous head, the change sets Windows-only properties, and the
local build here was rc=0 with the artifacts checked. S30 (the mac runner
completes ~5% of runs) is why it is still sitting there, and waiting longer would
be waiting on a 5%-likely event to confirm something already measured — the same
call the 2026-08-21 C7e entry made, for the same reason.

**Learned:**

1. **`OUTPUT_NAME` is three renames, and only the one with an extension is
   collision-proof.** Targets sharing an output directory can carry the same
   `OUTPUT_NAME` safely only for the artifact whose suffix differs; `PDB_NAME`
   and `ARCHIVE_OUTPUT_NAME` have no suffix to save them. Any future format
   added to `FORMATS_LIST` inherits this for free now, because both derive from
   the target name.
2. **`LNK1201` names disk space, privilege and path, and means none of them.**
   The line above it in the log — a sibling project writing the same base name —
   is the actual evidence, and it is easy to skim past as ordinary progress.
3. **Configure twice and diff the generated build system.** It answered "can
   this change affect macOS?" exactly, in about a minute, and produced the
   diagnosis of the Windows failure as a by-product. Cheaper and stronger than
   re-rendering, which could only have shown that nothing broke.
4. **A build that does not install is a measurement trap.** `cmake --build`
   leaves the previous bundle in the plugin folder, so a render "after the
   change" silently measures the artifact from before it. Hashing the installed
   binary against the built one is one command and converts a plausible result
   into a proof — the same trap the 2026-08-21 entry hit from the other side,
   with a same-ID bundle rather than a stale one.

**Next:**

1. **[#268](https://github.com/JeffMcClintock/TideSynth/pull/268) is green where it matters and wants only a merge** — `lint`,
   `guard`, `windows` and `linux` all pass on `276e150`; `macos` is queued
   behind S30 and cannot say anything about a Windows-only property. No
   `platform:win` issue was needed.
2. **N1b unblocks when N1a merges** — and it now has two more references to
   carry, both PDB names: `docs/state-of-the-prototype.md:106` and
   `docs/p4-resize-crash.md:461-462`. Noted on its row.
3. **Two stale `platform:mac` issues were closed** — see below; they were never
   this platform's break.

**Also this run, STEP 1 bookkeeping.** [#264](https://github.com/JeffMcClintock/TideSynth/issues/264) and [#260](https://github.com/JeffMcClintock/TideSynth/issues/260) were the only open
`platform:mac` issues and **both were re-verified before being touched**, per
STEP 1's rule about not fixing what you cannot observe. Neither is a macOS
break: each names a branch that no longer exists (both merged), and in each run
**all three platforms failed the Build step together**, which is `main`'s state
at that hour rather than anything about this platform. `main` went green on all
three at `5ef0bf29`, and macOS passes on #268's current head. Closed with that
evidence, and the linux box's own lesson is what made the check cheap — *"a
branch's CI platform issue can be reporting `main`'s break."*

**Machine left clean.** TideSynth returned to `main`, tree clean; every other
repo on this box (`SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`,
`GMPI`) was clean at the start of the run, untouched during it, and clean at the
end — this item is one file in TideSynth. Builds went to the scratchpad, not to
Jeff's trees. `~/Library/Audio/Plug-Ins/VST3/` ends the run as it began, except
that `TIDE-Rack.vst3` is now this branch's build rather than yesterday's;
`TIDE_VST3.vst3` is back and byte-untouched, and deleting it remains Jeff's
call, exactly as the previous entry left it.

**Branch/PR:** `tide/mac/N1a-rename` — [#268](https://github.com/JeffMcClintock/TideSynth/pull/268), TideSynth only. One CMake block.

---

## 2026-08-21 — macos — S29's coverage-hole fix, rebuilt clean after the branch went stale

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** re-created the `startsWith(github.head_ref, 'tide/')` correction on a
fresh branch off current `main`, because the original never got pushed and had
drifted 17 files behind.

**#258 merged the guard without the correction.** Checked directly rather than
assumed: `main`'s `build.yml` has the bare `if:` with no `tide/` test, so every
branch Jeff names by hand — none of which start with `tide/` — currently gets
**zero build coverage**: no push run (outside `on: push: branches:`), and now no
PR run either, because the guard skips all same-repo PRs unconditionally.

**The old local branch was the wrong base to push.** `git diff origin/main
s29-close-coverage-hole --stat` showed 17 files and 1263 deletions — journal
rotation, a deleted doc, N1a's rename, all landed separately since. Force-pushing
that would have reverted merged work. Deleted the stale branch and rebuilt the
one-line fix directly on today's `main`: one file, 13 lines.

**Learned:**

1. **An unpushed branch decays the moment other agents keep merging.** The fix
   was correct when written; by the time I went to push it, rebasing would have
   cost more than re-deriving it. Re-creating small, mechanical diffs from a
   current base is cheaper than reconciling a stale one.

**Next:** Jeff pushes `s29-close-coverage-hole` — one file, the fleet token
still has no `workflow` scope.

**Branch/PR:** `s29-close-coverage-hole` — workflow + row + this entry.

---

## 2026-08-21 — macos — the release track was free for three days and the backlog said otherwise

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** unblocked **R2, R3, R4, R5**; gave **R6** a *named* blocker instead of a
blanket one; corrected the section header that caused all of it.

### The stale gate

`## Release & distribution — blocked on V1 (nothing to ship yet)`. **V1 closed
2026-08-18.** One row also said *"Needs C7"* — **C7 closed 2026-08-21**. Neither
had been revisited, so five rows advertised a shut door that had been open for
three days, on the one track that turns a working plugin into something a user
can install.

This is A32's failure with the polarity reversed: A32 catches rows that look
*live* and are finished; this is rows that look *blocked* and are free. Nothing
detects it, because a `BLOCKED` status is never wrong-looking on its own.

### Four are free, and one is not — which is why I did not flip all five

**R2 / R3 / R4** need an artefact and a signing identity: all three platforms
build from a clean public clone, N1a gave the artefacts their shipped names
(`TIDE-Rack.vst3` / `.gmpi` / `.app`), and R1 settled signing. Free.

**R5**'s own named blocker was C7, now gone — but it is a
`.github/workflows/**` file, so a run can author and verify it and **cannot push
it**. That constraint is recorded on the row rather than discovered by the next
taker.

**R6 is genuinely not free**, and flipping it with its siblings would have been
the lazy read. It replaces the honest *"nothing to download yet"* card with
`releases/latest/download/<asset>` permalinks, and **those 404 until a release
exists**. So it moves from `BLOCKED` to **`BLOCKED(R5)`** — same status, real
information, and eligibility now lives in the status column where STEP 2 reads
it.

**Learned:**

1. **A blocked row is never obviously wrong, so nothing ever re-reads it.** The
   fleet has a lint for stale-live rows (A32) and none for stale-blocked ones,
   and the second kind is more expensive: it hides work that could have started.
2. **"Unblock the section" is not the same as "unblock every row in it."** Four
   of five were free; the fifth had a real dependency the blanket status was
   concealing. Naming the blocker is worth more than clearing it.

**Next:** R3 is `mac` and now takeable here. R2 is `win`, R4 is `linux`, R5
wants Jeff's push.

**Branch/PR:** `tide/mac/R-unblock` — TideSynth only, statuses and header.

---

## 2026-08-21 — macos — N1a: the rename shipped, and it silently unlinked half the build first

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing · linux + renderer agents also active

**Did:** carried the TIDE Rack rename through the build. Shipped artifacts are
now `TIDE-Rack.vst3` / `.gmpi` / `.app`; targets stay `TIDE_Rack` /
`TIDE_Rack_VST3` / `TIDE_Rack_STANDALONE`.

### The finding: a rename that skips work instead of failing

Two loops built target names by hand — `set(_tide_target TIDE)` and
`TIDE_${_fmt}` — instead of deriving from `PROJECT_NAME`. **Every use of
`_tide_target` sits behind `if(TARGET ...)`**, so renaming the project did not
break those loops, it made them **no-ops**: no `tide_render` link, no resource
staging, no diagnostic.

It surfaced only as `fatal error: 'TidePathTracer.h' file not found`, and only
because `TiDEPanelGui.cpp` happens to include that header. **Without that
include the build would have gone green and shipped bundles containing zero
prefabs** — an empty module browser, which is exactly S21's failure wearing a
different hat. All four sites now derive from `${PROJECT_NAME}`.

N1a's scope list was careful and still missed these, because it searched for
`TIDE_VST3`-shaped strings and these are the bare `TIDE`.

### Checked before renaming, not after

- **Identity does not move.** `id`, `name`, `vendor` come from the `<Plugin>`
  element in `SynthEdit.cpp`, not `PROJECT_NAME` — so saved host sessions still
  resolve. That was the one thing worth knowing before touching anything.
- **`${PROJECT_NAME}.xml` is not in play** (no `HAS_XML`, no `TIDE.xml`), so
  distribution.md's second warning does not apply here.
- **The STANDALONE's bundle id does follow `PROJECT_NAME`**
  (`com.gmpi.standalone.${PROJECT_NAME}`), so that dev tool gets a fresh
  preferences container. Stated rather than discovered later.

### The measurement, and why the first pass of it was worthless

Baseline `v1-rack.rpp`: peak **−6.3 dBFS**, rms **−17.0**, 2 patch cables.
After the rename: identical. **That proved nothing**, because the old
`TIDE_VST3.vst3` was still installed and carries the same plugin ID — REAPER
could have loaded either. So the old bundle was **moved aside** and the render
repeated: same numbers with only `TIDE-Rack.vst3` present. That is the
difference between "the numbers match" and "the artifact under test produced
them".

All five fixtures then pass in isolation — `v1-rack` −6.3/−17.0, `v1-rack-midi`
−6.3/−17.0, `v3-midi-pitch` −6.2/−21.1, `v3-midi-gate` −6.3/−21.2, and
`v1-rack-uncabled` **silence**, the negative control. `--control` PASSes.
**Jeff's original bundle was restored immediately afterwards.**

**Left for Jeff, deliberately:** `~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3`
(16 Aug) is now stale — nothing produces that name any more — and sits beside
the new bundle, so a DAW scan lists both. Deleting from his plugin folder is
his call, not a run's.

**Learned:**

1. **A guard that makes missing work silent turns a rename into a downgrade.**
   `if(TARGET x)` is the right shape for an optional format and the wrong shape
   for a name that must exist; the same line cannot tell the two apart. Deriving
   the name from `PROJECT_NAME` removes the question.
2. **When the old artifact is still installed and shares an ID, matching
   numbers are not evidence.** Moving it aside cost one minute and converted a
   plausible result into a proof.

**Next:**

1. N1's remaining buckets (B and C) are untouched — this was bucket A only.
2. The stale installed bundle wants Jeff's decision.

**Branch/PR:** `tide/mac/N1a-rename` — TideSynth only.

---

## 2026-08-21 — linux — the compositor problem is solved, and S23 does not reproduce once you can safely look

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Fifth item this run, at Jeff's direction.** He installed `weston` in response to
the previous entry's *"blocked on one `apt install`"* and said *"use it if you
can"*. Two rows moved as a result, and one of them only because the other was
fixed first.

### S32: headless weston works, and costs nothing

```bash
weston --backend=headless --socket=tide-test --width=1400 --height=900 &
WAYLAND_DISPLAY=tide-test XDG_CONFIG_HOME=<scratch> ./TIDE_STANDALONE
```

The headless backend needs no display, no seat and no GPU, and **nothing it does
reaches `gnome-shell`** — so mutter's client-teardown bug cannot be triggered by
anything a run does.

**What I expected to lose and did not:** the MCP command channel works unchanged,
and `gmpi_screenshot` returned a correct 1100x626 PNG with the rack drawn and the
module browser fully populated. It renders from the app's **own** buffer rather
than asking the compositor, so a headless session gives up the *view* and keeps
every bit of the *verification*.

**Roughly ten launches this session** — control, resources-absent, mismatch, four
shutdown trials, plus a continuous **635 s (10 m 35 s)** run still answering the command
channel at the ten-minute mark — with **zero `gnome-shell` crashes** and Jeff's login session (`loginctl` session 10) unchanged throughout.
Against four crashes in the two days before, on a box where two runs lost their
work to it.

Recipe written up as [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md),
including the one cosmetic wart (`Gdk-CRITICAL … gdk_seat_get_keyboard`, because
headless advertises no keyboard) so the next run does not chase it, and the
warning that nested-but-visible weston is only **partial** isolation — that
compositor is itself a mutter client and its own exit still runs the bad path.

### S23: the repro finally ran, and came back negative

Only possible because S32 was fixed first. Three conditions, seven launches:

| condition | result |
|---|---|
| control — resources present | 60 s, no fault |
| **resources ABSENT** (the layout both crashes were in) | 60 s, plus **4 × 10 s runs, every one exit 0** |
| **resources absent + the quarantined `session.previous.xml`** | 45 s, no fault |

The third is the case nobody had tried: a patch saved when the modules existed,
replayed when they do not. It was the best remaining hypothesis and it is dead.

**The startup state was confirmed identical to the crash runs**, not merely
similar — all four `missing from bundle resources`, `no Prefabs folder`, and
`MidiCv.synthedit did not insert a container`. Zero `TIDE_STANDALONE` segfaults
in `journalctl` throughout. A clean SIGTERM exit really is 0 here, so a 139 would
have been unambiguous.

**So the reproduction attempt is exhausted and should not be repeated.** Most
likely it was fixed in passing by the churn since 2026-08-20 — E14, E15, S26 and
Jeff's TiDEPanel/tide_render work all landed after it. The alternative is an
ingredient nobody has named. Either way, more stress runs will not settle it.

### What survives: S34

The signature decode stands on its own, and it points at two **real** latent
defects whether or not they caused S23 — two unguarded `back()` calls on
`std::vector<UPlug*>`, which read `data[-1]` at address -8 when the vector is
empty. Not a null-pointer fault, so no null check catches it. A third sibling is
already guarded and is the model; `ClassicControlGuiBase.cpp:22-33` fixed the
identical thing at -16 for **U2d** and set the convention (guard, log loudly,
return). Both files are GATED and this is not a build break, so filed, not fixed.

**Learned:**

1. **Fixing the tooling blocker was worth more than any single item it
   unblocked.** Two runs burned their runtime work on this box, and the previous
   entry's honest answer was "blocked on Jeff". One `apt install` converted an
   unrunnable experiment into a seven-launch A/B in twenty minutes.
2. **A headless compositor loses the view and keeps the verification**, because
   the screenshot path reads the app's own buffer. I assumed I would be trading
   capability for safety and was wrong — worth knowing before anyone declines the
   safe route to keep the pictures.
3. **A negative result is only worth what its control is worth.** The
   resources-absent run is meaningful *because* the log lines match the crash
   runs exactly; without that, "it didn't crash" would just mean "I ran something
   else."
4. **`check-id-refs.py` caught me filing A31's exact hazard.** S34 and S23 both
   cited `ug_adder2.cpp:81`, and the lint flagged the shared location within a
   minute of my writing it — the check A31 shipped for precisely this, doing its
   job on the run that had just re-read A31. Fixed by giving S34 the citations
   and having S23 point at it.
5. **A crash row that no longer describes anything observable should be closed,
   not left open.** S23 has now consumed four runs. The useful residue is S34;
   keeping the crash row open would keep drawing runs toward a repro that has
   failed on ~35 launches.

**Next:**

1. **S34** — two `empty()` guards in `SynthEditLib`, minutes for Jeff or an
   interactive session. Accept is in the row, including "build `SynthEditCL`
   too", since it is shared and TIDE building is not evidence.
2. **S23 should be closed as not-reproducible**, once S34 is on the board. Jeff's
   call, not a run's — I have not set it DONE.
3. **The mutter bug itself is untouched** and remains option (a) — Jeff's
   decision about the VM's graphics stack, not TIDE's problem to fix.
4. **Every future linux GUI item can now be verified here.** That includes the
   **E14 clause-2 gap** (the rack placement gesture), which older rows call
   unmeasurable on this box.

**Machine left clean.** TideSynth back on `main`, tree clean. All experiments ran
on a **copy** of the binary and resources in the scratchpad — Jeff's build tree
was never modified. `~/.config/TIDE Rack/` is byte-identical (every run used a
scratch `XDG_CONFIG_HOME`). Weston stopped by pid, not `pkill -f` (**S31**).
`~/SE/gmpi_ui/TEXT_LAYOUT_PLAN.md` is still dirty from 2026-08-19 and is Jeff's.

**Branch/PR:** `tide/linux/S32-headless-compositor` — TideSynth only. No product code change.
## 2026-08-21 — macos — S33 filed: a live defect was sitting on a closed row

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing · two other agents active (linux, renderer)

**Did:** gave the `setBlob` stub its own row. It was found while answering E11
and recorded on **E6** — which I had already flipped to DONE hours earlier. **A
live defect on a closed row is invisible**, which is precisely the failure A32
exists to name, so this is me walking into the trap I filed a lint for.

The finding itself is unchanged and is restated on S33: `setBlob`
(`processor_holder.h:135`) has its one assignment commented out and returns
`true`; `value_` is written only at construction; two callers branch on that
`true`, and `onQueMessageReady` reads a payload into a local vector, calls the
stub, then sends a **zero-length** blob event — discarding the bytes it just
read.

**A31's habit ran first and earned its keep again:** the backlog already had
**S31 and S32** filed by the linux box while I was working, so the next free id
was not the one I would have guessed, and a live row (**E9**) already cites
`processor_holder` — different lines, different subject, so not a duplicate.
Re-checked against a freshly fetched `origin/main` immediately before
committing, per the id-collision rule.

**Learned:**

1. **Recording a finding on a row you are about to close loses it.** The
   sequence was innocent — flip E6 DONE in a bookkeeping pass, discover the
   root cause hours later, add it to the row that already described the
   symptom. Nothing warns you, and A32's advisory only looks at umbrella rows
   with closed children, not at closed rows carrying new text.
2. **Two other agents were filing ids concurrently.** The gap between reading
   the highest id and committing is where collisions live; fetching again
   immediately before the commit is the whole mitigation.

**Next:** S33 is PR-GATED and wants Jeff. E11 stays WONTFIX with its reopen
trigger pointing at exactly this row.

**Branch/PR:** `tide/mac/S33-setblob-stub` — TideSynth only, one row and this entry.

---

## 2026-08-21 — linux — N1 costed: 91% of what a grep finds must not be touched

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Fourth item this run, at Jeff's direction** (*"take next task"*, three times).
STEP 2's one-item rule is overridden by that each time.

**Also at his direction: I merged [#262](https://github.com/JeffMcClintock/TideSynth/pull/262) myself** (*"then merge 262"*), which
overrides STEP 5's *"Do NOT merge the PR"* for that one PR. Recording it plainly
because that rule exists so Jeff reviews before anything lands, and him asking
for the merge **is** that review — but a later run reading `git log` would
otherwise see a bot-merged PR and have no way to tell which. All three platform
builds were green on the head before the merge-of-main; `lint` and `guard` passed
on the merged commit; the three matrix jobs were still re-running, and the PR
contains no compiled code.

**Did:** took **N1** and did the thing its own row asked for — *"probably wants
splitting once someone costs it"* — rather than starting the rename. Costing it
is what showed the rename must not happen on this box.

### The count, and the finding is bucket C

| bucket | what | files | refs |
|---|---|---|---|
| **A — live build, tooling, fixtures** | the actual rename, one commit | 11 | **21** |
| **B — live reference docs** | instructions somebody follows today | 4 | ~28 |
| **C — the historical record** | `JOURNAL*`, `BACKLOG-DONE.md`, `docs/lessons.md` | 4 | **220** |

**Bucket C is ten times bucket A and none of it may be rewritten.** Those files
record measurements of a binary that really was called `TIDE_VST3.so` on the day
they were taken, and this project's convention is that superseded text is
preserved verbatim. So the row's standing warning — *"do not start with a global
search-and-replace"* — is stronger than it reads: **a global replace is not
merely risky, it is wrong on 91% of its hits**, and it would falsify the record
rather than just churn it.

### Two flagged unknowns answered, two flagged hazards dismissed

- **`OUTPUT_NAME` alone is not enough**, exactly as the row suspected.
  `gmpi_plugin.cmake:774` sets `MACOSX_BUNDLE_BUNDLE_NAME "${GMPI_PLUGIN_PROJECT_NAME}"`,
  so the project name reaches the macOS bundle name directly.
- **No `OUTPUT_NAME` is set anywhere today** — artifacts are named straight off
  the target names, so the dashed convention is an addition, not an edit.
- **`TIDE.xml` / `TIDE.rc` do not exist**, so `gmpi_plugin()`'s
  `${PROJECT_NAME}.xml` / `.rc` paths are not in play.
- **`build.yml` names no TIDE target or artifact** — comments only. So the rename
  needs no workflow change and no `workflow` token scope, which is the wall this
  fleet keeps hitting and which does **not** apply here.

### The prose half is done — verified rather than assumed

`SynthEditSem/SynthEdit.cpp:396` ships `name="TIDE Rack" vendor="TIDE Synth"`,
and the host sees it: every fixture records `"VST3i: TIDE Rack (TIDE Synth)"`.
`getVendor4charCode()` still returns the fixed-width `"TIDE"`, correct and not to
be changed. **Nothing user-visible is waiting on N1** — what remains is internal
naming only, which is worth knowing before anyone prioritises it.

### Why this box must not do the rename

The five fixtures name the artifact by **filename**:

```
<VST "VST3i: TIDE Rack (TIDE Synth)" TIDE_VST3.vst3 0 "" 1386065673{...}
```

Renaming the artifact invalidates all five at once, and they are **v0.1's
acceptance evidence** — the thing PLAN.md points at to say the product works.
Re-verifying them needs `render-and-measure.py` and **REAPER**, which is not
installed here. A run on this box could make the change and could not tell
whether it had broken the proof.

**Split accordingly:** **N1a** (bucket A, one commit, on a box with REAPER; N1
becomes the umbrella) and **N1b** (bucket B, `BLOCKED(N1a)` — doing the docs
first would make them lie). Bucket C is out of scope permanently. Full working
in [docs/n1-tide-rack-rename.md](docs/n1-tide-rack-rename.md).

**Also, bookkeeping:** **A12 → DONE** on its merged PR, and the NEXT linux cell
re-pointed — it still told the next run to *"rebuild the 2026-08-20 tree and
`addr2line 0x3b4627`"*, which this run proved impossible three hours ago.

**Learned:**

1. **Counting a rename by bucket, not by total, changes the decision.** "143
   references" reads as a large scary job. "21 live, 220 that must not be
   touched" is a small job with a trap beside it, and only the second framing
   tells you what to do.
2. **A grep total is not a work estimate when the repo keeps a historical
   record.** Every append-only file inflates the count with hits that are
   correct as they stand.
3. **Ask which box can VERIFY a change before asking which box can make it.**
   The rename is minutes of editing anywhere; it is only finishable where the
   acceptance harness runs. That constraint lives in the fixtures, not in the
   code being renamed.
4. **A row that says "needs decisions rather than edits" is worth re-reading
   after its blocker clears.** N1's decisions were all settled — the forms, the
   repo name, the asset names. Only the *timing* was open, gated on C7, and C7
   went DONE earlier in this same run.
5. **When the developer overrides a standing rule, write down which rule and
   which instance.** A bot-merged PR is indistinguishable from a bot that decided
   to merge, and the difference is the whole point of the rule.

**Next:**

1. **N1a on a box with REAPER** (win or mac). Everything it needs is in its row;
   nobody should have to re-derive the file list.
2. **This platform's runtime work is blocked on one `apt install`** — S32 has no
   workaround a run can apply, because `weston`, `cage`, `sway` and `Xvfb` are
   all absent and `sudo` needs a password. Until then S23's targeted repro cannot
   be run safely here.
3. **S23** otherwise needs only that repro; the signature and two candidate sites
   are already in the row.

**Machine left clean.** TideSynth back on `main`, tree clean. Nothing was run
against the desktop for this item — greps, CMake reading and counting only.
`~/.config/TIDE Rack/` untouched. `~/SE/gmpi_ui/TEXT_LAYOUT_PLAN.md` remains
dirty from 2026-08-19 and is Jeff's.

**Branch/PR:** `tide/linux/N1-cost-and-split` — TideSynth only. No code change.

---

## 2026-08-21 — linux — S23: what -8 means, measured — and the fleet has been bitten by this exact class before

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Third item this run, at Jeff's direction** (*"take next task"*, twice). STEP 2's
one-item rule is overridden by that; recording it so this does not read as a run
helping itself.

**Did:** took S23 back to finish it. The step I had left — rebuild the 2026-08-20
tree and `addr2line 0x3b4627` — turned out to be **impossible**, and closing that
off properly was worth more than the guess it would have produced. Then decoded
the fault signature by measurement instead, which named a class of site the
project has already been bitten by once.

### The offset route is closed, and here is the proof rather than the excuse

`/var/log/apport.log.1` names the binary that faulted:

```
2026-08-20 17:33:00: executable: /tmp/claude-1000/-home-jef-SE/22760dc3-.../scratchpad/s17/TideSynth/build-sa/SynthEditSem/TIDE_STANDALONE
2026-08-20 17:33:00: ERROR: executable does not belong to a package, ignoring
```

Three things follow, and each kills a route:

1. **It was a clone in the S21 run's own scratchpad**, built into `build-sa` —
   **not** `~/TideSynth/build`, which is the tree the previous entry resolved
   `0x3b4627` against. Different tree, different link, different layout.
2. **No core was ever written.** `core limit 0`, and apport declined an
   unpackaged executable. Nothing to open.
3. **The binary is gone** — the scratchpad did not survive the reboots.

And an exact rebuild is not reproducible either: at `21f9c80` (main's code state
at crash time) there was **no `cpm-package-lock.cmake`**, so every dependency
resolved to whatever its branch head was that afternoon.

**The previous entry's `CContainer::OnEditContain` lead was not merely weak, it
was invalid**, and one command shows it — `0x3b4627` lands **mid-instruction** in
today's binary:

```
3b4626: 0f 84 92 00 00 00    je  3b46be      <-- 6 bytes, 3b4626..3b462b
```

I labelled that lead "probably a coincidence" for the wrong reason (I argued from
the -8 offset, see below) and it turns out to be right for a better one.

### What -8 actually means, measured

Three candidates, each in its own forked child so one crash could not mask the
others, with the fault addresses read back from the kernel log:

| candidate | fault address | verdict |
|---|---|---|
| `dynamic_cast` on an object with a zeroed vptr | **-16** | not ours |
| `back()` on an empty `std::vector<int>` | **-4** | right class, wrong element size |
| `null->member` | **+12**, and `error 4` | not ours |

Ours is **-8 with error 5**. So the crash is **`back()` / `rbegin()` /
`end()[-1]` on an empty vector of 8-byte elements — a vector of raw pointers.**

**This kills two guesses, one of which I had made about forty minutes earlier.**
I had started to favour `dynamic_cast` on a dangling handle, on the reasoning
that the Itanium ABI puts typeinfo at vptr-8. The measurement says -16. Reasoning
about ABI offsets from memory is exactly the move that produces a confident wrong
answer, and it cost one 30-line program to avoid.

### The precedent was already in the tree

`SynthEditLib/modules/ControlsXp/ClassicControlGuiBase.cpp:22`:

> `widgets.back()` on an empty vector is UB (crashed TIDE at address **-16** =
> empty `back()` with **16-byte elements**; TideSynth BACKLOG **U2d**). Widgets
> are built by pin-init callbacks above; **a host where those don't fire must not
> bring the whole process down.** Loud, not silent, per U2d's rule.

Same class, same arithmetic done the same way, and **the same trigger shape**:
U2d's empty collection came from missing font/skin resources; **S23's two crashes
were both in the layout with the bundle's `Resources` missing**. The fleet has
solved this once and written down how.

### Two unguarded candidate sites

Both are `std::vector<UPlug*>` (`ug_base.h:245`), so both fault at exactly -8:

| site | code | guard |
|---|---|---|
| `SynthEditLib/ug_adder2.cpp:81` | `auto p = plugs.back();` — first line of `ug_adder2::NewConnection()` | **none** |
| `SynthEditLib/ug_feedback_delays.cpp:72` | `auto dummyPin = u->plugs.back();` | **none** |
| `SynthEditLib/ug_oversampler.cpp:337` | `connections.back()` | `while(!…empty())` — what the other two should look like |

The adder is the interesting one: it is what implements TIDE's automatic summing
when patch cables fan into one input, and `NewConnection` runs while the DSP
graph is built from a restored patch — at startup, which is when both crashes
happened.

**Not proven, and the tidiness of the story is exactly why it should not be
trusted yet.** Nothing here observes the fault at either line. "Resources missing
→ pin list empty → `back()`" fits every measured fact and remains a hypothesis.

Both files are **GATED** (`SynthEditLib`) and this is not a build break, so A17's
exception does not reach it. Filed, not fixed.

**Learned:**

1. **`/var/log/apport.log` names the executable path for crashes apport
   declined to report.** Two runs assumed the crashing binary was the one in the
   obvious build tree. It was a clone in a scratchpad, which is *why* the offset
   resolved to nonsense — and one grep would have said so on day one.
2. **An address that lands mid-instruction is proof the binary is wrong**, and it
   is a one-command check. Worth doing before any reasoning about what a resolved
   symbol means.
3. **Negative fault addresses are arithmetic, and the arithmetic is worth
   measuring rather than recalling:** -4, -8, -16 are `back()` on empty vectors of
   4-, 8-, and 16-byte elements. I had the ABI story for -8 confidently wrong.
4. **`error 4` vs `error 5` separates a null dereference from a wild read**, and
   both crashes were `error 5`, consistent with the negative-address reading.
5. **Grep the tree for your own crash signature before theorising.** The comment
   at `ClassicControlGuiBase.cpp:22` had already done the same decode, in the same
   codebase, four days earlier — including the element-size arithmetic.
6. **A dead end closed with evidence is worth more than a lead kept alive on
   hope.** The rebuild would have produced a symbol nobody could trust, and the
   next run would have spent on it.

**Next:**

1. **A targeted repro, not archaeology:** launch `TIDE_STANDALONE` with the
   bundle's `Resources` absent — the layout both crashes were in — under `gdb`,
   and see whether it stops in either candidate. Minutes, and it either names the
   frame or clears both sites.
2. **Read S32 first.** Launching the standalone on this box has taken the
   developer's desktop down; a nested compositor is the safe way, and none is
   installed (`weston`, `cage`, `sway`, `Xvfb` all absent; no `sudo`).
3. **Incidental, noted in the row rather than filed** so as not to make two ids
   for one job: `ClassicControlGuiBase.cpp:9-11` `dynamic_cast`s and then calls
   `header->SetText` with no null check, while its sibling at `:31` checks.

**Machine left clean.** TideSynth back on `main`, tree clean. Nothing was run
against Jeff's desktop this item — all of it was log reading, disassembly, and a
30-line test program in the scratchpad. `~/.config/TIDE Rack/` untouched.
`~/SE/gmpi_ui/TEXT_LAYOUT_PLAN.md` is still dirty from 2026-08-19 and is Jeff's.

**Branch/PR:** `tide/linux/S23-addr2line` — TideSynth only, row and journal. No code change.

---

## 2026-08-21 — linux — A12: the wall this row recorded was not there, and the check it wanted had a false alarm in it

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Second item this run, at Jeff's direction.** He interrupted with *"take next
task"* after S23's PR was open, which overrides STEP 2's one-item rule. Recording
that here because otherwise this entry reads as a run that helped itself to a
second item.

**Did:** took **A12** (a halted box is invisible to the fleet), built the
detection half in `scripts/watchdog-digest.py`, and left one clause of its Accept
explicitly unmet rather than pretending otherwise.

### The row's stated blocker was not real

A12 ends: *"Note the digest workflow lives in `.github/workflows/`, so a
scheduled run cannot edit it — same wall as A3/A5/A6 and C9(a). Needs Jeff or an
interactive session."*

`.github/workflows/watchdog.yml:39` is:

```yaml
run: python3 scripts/watchdog-digest.py --repo-root .
```

**That is the whole of the workflow's involvement.** Every check lives in a
script under `scripts/`, which a run may edit freely. The wall is real for
A3/A5/A6 — those change *when and how* the workflow runs — but A12 only changes
*what the digest says*, and nothing about that is gated. The row had inherited
the constraint from its neighbours.

### What was built

`check_halted_boxes()` classifies each box and says, in the digest itself, what
separates the two ways a box produces no work:

| classification | rule |
|---|---|
| alive | an entry newer than 3 days |
| **QUIET** | silent >= 3 days — ordinary if the machine was off |
| **LIKELY HALTED** | silent >= 7 days — past any ordinary explanation |
| **NO ENTRY EVER FOUND** | nothing in the journal or any archive |

The discriminator is **the presence of an entry, not its content**: a box that
ran and found nothing eligible still writes and pushes an entry saying so, and
reads as alive. A box halted at STEP 0.7 cannot push anything at all, so silence
is the only symptom it is capable of emitting.

Plus `check_credential_expiry()` — a countdown to the bot token's **2026-11-07**
expiry. That is the one fleet-wide halt known in advance, and without a countdown
it arrives as three boxes going silent on the same day with nothing saying why.

### The false alarm that was already shipping

The old `check_journal_freshness()` read `JOURNAL.md` only. Rotation (A8/A24)
moves entries out **by age**, so a box running normally but less often than the
others has its last entry carried into the archive by somebody else's busy day —
and the live file then reports `no entry found`, which is the same output as a
box that has never run.

It was doing exactly that, today, on the real repo:

```
before:  - windows: no entry found
after:   - windows: alive -- last entry 2026-08-20 (1 day ago).
```

**The windows box was fine the whole time.** Its 2026-08-20 entry (`C14: the last
private include was never needed`) had rotated into `JOURNAL-2026-08.md`. A
watchdog whose most alarming output is its own artifact is worse than no
watchdog, because the first real halt looks identical to the noise.

Fixed by scanning `JOURNAL.md` **and** every `JOURNAL-<YYYY>-<MM>.md`, taking the
max date per box rather than the first heading seen — the archive is not reliably
ordered, so "first heading" was also wrong.

**Result:** `python3 scripts/watchdog-digest.py --selftest` — 4 fixtures, all
pass: alive / quiet / likely-halted / rotated-but-alive. Full digest builds
end-to-end (`--dry-run`, rc=0, 8 sections, real GitHub data).

### The clause I did not meet, and why it cannot be met here

Accept asks that a halted box appear *"with the failing assertion"*. **No code in
this repository can deliver that.** A run that fails STEP 0.7 holds no credential
it is permitted to use — filing an issue or pushing a branch to report the
failure is precisely what the step forbids, and using Jeff's keyring credential
to do it is the bypass the whole assertion exists to prevent. So the assertion
physically cannot leave that machine through any channel the fleet reads.

The digest now says this in place and prints the three commands to run at that
keyboard. Surfacing the assertion itself needs a channel outside GitHub auth, and
that is a different item.

Accept also asks for *"a deliberately induced halt... not by reasoning"*. The
self-test is a **fixture**, not an induced halt on a real box, and the row says
so. I could not induce a real one without breaking another machine's credentials.

**Also, as STEP 4 bookkeeping:** flipped **C7** and **C7e** IN-REVIEW → DONE.
Both were flagged by the digest's own IN-REVIEW check and confirmed independently
via the API — [#165](https://github.com/JeffMcClintock/TideSynth/pull/165) and
[#250](https://github.com/JeffMcClintock/TideSynth/pull/250), both `merged=true`.

**Learned:**

1. **A row can inherit a blocker from the rows filed beside it, and nobody
   re-checks.** A12 said a scheduled run could not do it, citing A3/A5/A6. One
   `grep` of the workflow showed it runs a single script and nothing else. **When
   a row names the obstacle rather than showing it, look at the obstacle first —
   it costs one command and it was the whole item here.**
2. **A watchdog's own false alarms are the expensive kind.** `no entry found` for
   a healthy box is not merely noise: it trains whoever reads the digest to
   discount the exact line that will report the first real halt.
3. **Rotation is a hazard for anything that reads the journal, not just for
   readers of it.** Any check computing "how recently did X happen" from
   `JOURNAL.md` alone silently inherits the rotation policy as its time window.
4. **The archive is not reliably ordered**, so "first heading wins" is wrong
   there; take the max. My own first pass at this used `head -1` and got
   2026-08-18 for windows when the answer was 2026-08-20.
5. **Some Accept clauses are unsatisfiable by construction, and saying so beats
   half-meeting them.** "Report the failing assertion" cannot work when the
   failure being reported is the loss of the only credential permitted to report
   anything. That is worth writing down as a property, not logged as a shortfall.

**Next:**

1. **Jeff's call on the unmet clause** — if the failing assertion needs to reach
   the fleet, it needs a channel that does not depend on the credential that just
   failed. Worth its own row if he wants it.
2. **The thresholds (3 / 7 days) are a first guess** from the fleet's roughly
   daily cadence. If they prove noisy, they are two constants at the top of the
   check.
3. **S23** remains one `addr2line` from closed; **S32** before any further GUI
   work on this box.

**Machine left clean.** TideSynth is back on `main`, tree clean; both PRs are the
only place this run's work lives. **One dirty file elsewhere, and it is not mine:**
`~/SE/gmpi_ui/TEXT_LAYOUT_PLAN.md` carries a real content change (not CRLF churn —
`git diff --ignore-all-space` is non-empty) dated **2026-08-19 17:41**, two days
before this run started. That is Jeff's work in progress: not committed, not
reverted, not stashed. The three CPM `_deps` checkouts I read from
(`gmpi_ui-src`, `gmpi_wrappers-src`, `syntheditlib-src`) are all clean — this run
only read them.

**Branch/PR:** `tide/linux/A12-halted-box-digest` — TideSynth only.
## 2026-08-21 — macos — S29 fixed, after measuring that S29's own recommendation was wrong

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** prepared the one-run-per-commit fix for `build.yml`, and corrected the
row I wrote yesterday, whose recommended fix does not work.

### The duplication is real and exact

Every `build.yml` run for `tide/mac/macos-arm64`: **four shas, eight runs**,
each an exact `push`/`pull_request` pair about four seconds apart. Cross-sha
cancellation already works — older runs show `cancelled` — so it is only the
same-sha pair that escapes.

### My own fix was wrong, and one query showed it

S29 recommended dropping `event_name` from the concurrency group. **That
changes nothing**, because the group also keys on `github.ref`, and that is
`refs/heads/<branch>` for push but **`refs/pull/<n>/merge`** for pull_request.
Different refs, different groups, with or without `event_name`. I had reasoned
about the key without checking what its components evaluate to.

### What shipped instead, and why not the tidier variant

An `if:` on the `guard` job: run on push, and on `pull_request` only when the
PR head is a **fork**. Same-repo PRs are already covered by their push run,
whose checks attach to the same sha and therefore show on the PR; `build`
inherits the skip through `needs: guard`.

The tidier-looking alternative is a concurrency group keyed on the head sha
(`github.event.pull_request.head.sha || github.sha`), which really would unify
the two events. **Rejected on the strength of S30:** that lets both runs QUEUE
and then cancels one, and the scarce resource here is the macOS runner at ~5%
completion — a run that queues and dies has already taken the slot. The `if:`
never starts it.

### Checked before handing it over

YAML parses with `guard` and `build` intact and triggers unchanged; and
**`main` has no required status checks**, so a skipped job cannot block a
merge — which was the real risk of gating a job that everything else `needs:`.

**Stated cost:** same-repo PRs stop being tested as a merge result and are
tested as the branch tip. Fine while this repo squash-merges quickly.

**Learned:**

1. **A concurrency group is only as good as what its expressions evaluate to,
   and `github.ref` is not the branch on a `pull_request` event.** I wrote a
   recommendation from the shape of the key rather than its values, and it
   would have shipped a no-op that looked like a fix — the worst kind, because
   the duplication would have continued under a closed row.
2. **The second-best fix won on a constraint from a different row.** Both
   candidates halve the runs; only one avoids consuming a macOS slot before
   cancelling, and that mattered only because S30 had measured the scarcity.

**Next:** Jeff pushes it — the fleet token is `repo`-scope only by design.

**Branch/PR:** `s29-one-run-per-commit` — workflow + row + journal.

---

