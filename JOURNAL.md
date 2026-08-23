# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-23 — windows — S34: two guards in SynthEditLib, and a stale row flipped on the way in (interactive, Jeff directing)

**Did:** S36 confirmed merged and flipped to DONE — [#339](https://github.com/JeffMcClintock/TideSynth/pull/339)
landed while this session's sync ran. Then took **S34** off the `any` queue: two
unguarded `plugs.back()` calls on `std::vector<UPlug*>`, GATED (`SynthEditLib`),
takeable here because this is interactive with Jeff directing.

### S34

Both sites fault at exactly **-8** when `plugs` is empty — `data[-1]` on an
8-byte pointer element, not a null-pointer read, so no null check catches it.
Same class the fleet already fixed once at -16 for `ClassicControlGuiBase.cpp`'s
16-byte `widgets` elements (**U2d**). The row named both sites, the exact
mechanism, and a third sibling (`ug_oversampler.cpp:337`) that already guards
the identical pattern — nothing here needed re-deriving, only applying.

`ug_adder2.cpp:81` — first line of `NewConnection()`, TIDE's automatic input
summing, reachable whenever a graph is built from a restored patch with an
empty pin list, which is exactly what a missing bundle resource causes.
`ug_feedback_delays.cpp:72` — `BypassFeedbackModule()`, identical shape.

**Fix:** guard, one loud stderr line naming what will not work, return rather
than crash — `ClassicControlGuiBase.cpp`'s own established pattern, and its
comment states the rule this copies: *"a host where those don't fire must not
bring the whole process down. Loud, not silent."*

**Verified by building, per the row's own Accept** — `SynthEditCL` (this
repo's Release config is a shared library, so TIDE building alone would not be
evidence): a scratch Ninja tree, `SYNTHEDITLIB_FOLDER_OVERRIDE` on the fix
branch, `262/262` targets, rc=0, zero errors. Both new stderr strings read back
out of the built `SynthEditCL.exe` verbatim. Smoke-ran the exe: scans modules,
exits cleanly on an unrecognised verb, no crash.

**Not verified:** neither path was exercised at runtime with a genuinely empty
`plugs` vector — the row itself frames this as latent UB surfaced by
investigation (S23), not a currently-reproducing crash, so the fix is
defensive against a reachable condition rather than a reproduction of a live
symptom.

### The stale-row catch

The win NEXT cell (written by this box a session ago) still said *"S36 is
IN-REVIEW, no PR link yet — check whether it has one and whether it merged
before doing anything else with it."* It had, ten minutes before this run
started. Confirmed via `gh pr view` before touching anything else — flipping a
row on verified PR state, not on memory of having pushed it, per the standing
lesson this backlog's own history keeps recording.

**Learned:**

- **A row that names its own precedent site is most of the fix.** S34 named
  both defect locations, the exact fault address to expect, and a working
  sibling to copy. The work was verifying and applying, not investigating.
- **Check the cell's own "before doing anything else" instruction before doing
  anything else.** It was there specifically so this wouldn't be skipped.

**Next:** the `any` queue still carries several other GATED rows (S5, S3g,
S22, S18) that want either Jeff or another interactive session; none was
sized as tightly as S34 was.

**Branch/PR:** `tide/win/S34-guard-record` — TideSynth, bookkeeping only.
Product change is [SynthEditLib#34](https://github.com/JeffMcClintock/SynthEditLib/pull/34), not merged.

---

## 2026-08-23 — windows — S36: the Windows resources move beside the binary, and my first attempt at "beside" was wrong (interactive, Jeff directing)

**Did:** took S36's option (a) off the `any` queue — its own row records that #314 retired
the objection against it (dropping the race meant the destination could finally
move without reintroducing the collision). Windows resources now land where the
runtime actually looks, and packaging was updated to match.

### The mechanism, read rather than assumed

`BundleInfo::getResourceFolder()` for a non-bundle Windows plug-in
(`pluginIsBundle == false`, which every unpackaged dev-tree binary is) returns
`getImbeddedFileFolder()` verbatim — the binary's own directory, **no subfolder
appended**. `getResource()` then does `getResourceFolder() + resourceId`, a
plain concatenation, and `seedPrefabsFromBundle()` does `resourceFolder /
"Prefabs"`. So the four pin XMLs and `Prefabs/` belong **loose in `Release\`**,
mixed in with the binaries — not in a `Resources` subfolder at all, on this one
platform.

### My first attempt got that wrong, and testing the row's own Accept caught it

Read "point the Windows arm at `$<TARGET_FILE_DIR>`, drop the `/..`" and
implemented it as `$<TARGET_FILE_DIR>/Resources` — dropping the `/..` but
keeping a `/Resources` suffix, by analogy with the bundle-platform arms right
above it in the same file. Built, ran the row's own Accept command on the
freshly built standalone, and it printed the exact same four `missing from
bundle resources` lines and `no Prefabs folder` as before the fix — a clean
compile and a clean build log said nothing about this being wrong. Only running
the binary caught it. Reading `getImbeddedFileFolder()`'s actual return value —
the bare directory, not a subfolder of it — is what gave the real answer:
`$<TARGET_FILE_DIR>` alone, no suffix.

### Packaging had to change with it, not just the CMake line

`package-windows.ps1` previously copied the whole staged `Resources` directory
into the bundle's `Contents\Resources\`. With the dev-tree destination now the
bare `Release\` folder, doing the same thing would have copied every target's
binaries, PDBs, `.lib`s and `.exp`s into the shipped bundle too. Rewrote it to
pick the four known XMLs and `Prefabs\` out of `Release\` by name — the same
list `SynthEditSem/CMakeLists.txt`'s `_tide_xmls` already enumerates, and the
same "these two lists must move together" rule `TideApp.cpp:496` already states
for its own read of the identical set.

**The packaged bundle's own layout is unchanged** — still `Contents\x86_64-win\
TIDE-Rack.vst3` + `Contents\Resources\{4 xmls, Prefabs\}`, which is what makes
`pluginIsBundle` true for the installed copy and routes it through the
bundle-aware code path this row never touches.

### Verified

Row's own Accept, on the freshly built standalone, nothing copied by hand:

```
TIDE: ControlsXp.xml enriched 2 of 18 described class(es)
TIDE: MidiPlayer2.xml enriched 2 of 7 described class(es)
TIDE: Converters.xml enriched 26 of 70 described class(es)
TIDE: VaFilters.xml enriched 2 of 7 described class(es)
TIDE: 6 rack prefab(s) seeded from the bundle
```

Zero `missing from bundle resources` or `no Prefabs folder` lines (`grep -c`
against the run log: 0).

**The #314 race fix, re-checked because I edited the same block:** 20 parallel
relinks (`cmake --build --parallel`, deleting the binaries and the loose
resources between each), **0 failures**.

**Packaging, end to end:** `package-windows.ps1` against this build, unsigned
(no Azure credentials on this box, same as every prior run) — assembled bundle
contains exactly 10 files, the four XMLs and six `.synthedit` prefabs, nothing
else. No binaries, PDBs, `.lib`s or `.exp`s leaked into `Resources\` — checked
by listing the assembled tree, not assumed from the script logic.

**Not verified:** the packaged bundle was not loaded in a real VST3 host this
run — the bundle-aware code path and the shipped layout are unchanged by this
fix (same files, same place), and R2's own prior verification already covers
that path; re-proving it would be re-verifying something this row does not
touch. macOS and Linux are untouched — the edited arm is `if(WIN32)` /
`if(UNIX AND ...)`-gated and neither ran.

**Learned:**

- **A clean build and a clean log are not evidence the destination is right.**
  My first attempt compiled, linked, and staged files into *a* folder without
  any error — it was simply the wrong folder, and nothing short of running the
  Accept command surfaced that.
- **"Drop the `/..`" meant drop it entirely, not shorten it by one segment.**
  The row's own wording was correct; I filled in the wrong generator expression
  from pattern-matching the bundle arms beside it rather than reading what
  `getImbeddedFileFolder()` actually returns.
- **A destination change to a build-tree path can force a packaging-script
  change even when the shipped layout doesn't move.** The dev-tree consumer and
  the packaging consumer read the same CMake output through two different
  assumptions (loose files vs. a clean `Resources` subtree), and moving the
  first broke the second's "copy the whole folder" shortcut.

**Next:** S36 is the last of the resource-staging defects this cluster of rows
(#314, S21, S36) named; nothing else on the `any`/`win` queue currently touches
this file. **P3** remains this platform's only own-boxed row and is GATED,
needing Jeff.

**Branch/PR:** `tide/win/S36-resource-destination` — TideSynth, [#339](https://github.com/JeffMcClintock/TideSynth/pull/339).

---

## 2026-08-23 — linux — A34: the scan warning now reports a finding instead of a probe, and S37's inference is corrected

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Fifth item this session, at Jeff's direction. **A34** — the row this box filed
an hour earlier while running E1c.

**Did:** `tools/render_harness.py` warned about every folder the engine *looked
at*. It now warns about folders that actually **contain** modules.

### The change

`folder_has_modules()` classifies a scanned folder, and the warning fires only
on the populated subset. **Conservative by construction, because the two
mistakes do not cost the same:** a missed warning silently attributes a
measurement to the wrong module set; a spurious one costs a reader a moment. So
it searches recursively, matches `.sem` and `.gmpi` on the suffix (a `.gmpi` may
be a file *or* a bundle directory), and returns true on anything it cannot rule
out — an unreadable folder, or one too large to walk inside a 20,000-entry cap.

`.xml` does not count: the engine package pairs each `.sem` with an `.xml` pin
descriptor, and a folder holding only descriptors can load nothing.

**The report keeps both lists** — `foreign_module_sources` is still the raw
probe list, and `populated_module_sources` is the new finding — so nothing that
consumed the old field loses anything.

### Demonstrated three ways, which is one more than the row asked for

Same engine, same case, only `XDG_DATA_HOME` differing:

| scan folder | probed | populated | warning |
|---|---:|---:|---|
| **absent** | 1 | 0 | **silent** |
| **exists, empty** | 1 | 0 | **silent** |
| **one `.gmpi` planted** | 1 | **1** | **fires, and names it** |

The row's Accept asks for absent-or-empty silent and populated loud. Splitting
absent from empty is the case that matters most, because **absent is what CI
does** — `/home/runner/.local/share/SynthEdit/modules` on a runner that has no
such directory is precisely the false alarm that made this row.

Full suite unchanged at **8/8**, and silent under isolation.

**Seven engine-free selftest cases** cover the classifier both directions —
absent, empty, descriptors-only, `.sem`, `.gmpi`-as-directory, a module nested
below the folder, and the filter itself. A one-sided test would pass on a
classifier that only ever says "no".

### CORRECTION, and it is the more important half of this entry

**Jeff: *"CLAP does ship a GUI. I suspect DAWs support only X11."* He is right
and my S37/S43 conclusion was wrong.**

I wrote that the Linux CLAP *"has no GUI backend linked"* and therefore
*"`is_api_supported` returning false is honest"*. The measurement was sound; the
inference was not. **The honest reading is that the CLAP wrapper is unfinished
on Linux.** Four artifacts, one build:

| artifact | links libX11/xcb | links wayland |
|---|---:|---:|
| `TIDE-Rack` (standalone) | 0 | **1** |
| `TIDE-Rack.so` (**VST3**) | **2** | **1** |
| `TIDE-Rack.clap` | 0 | 0 |
| `TIDE-Rack.gmpi` | 0 | 0 |

**The VST3 links both; the CLAP links neither**, and the source lists say why
outright: `wrapper/CLAP/CMakeLists.txt` has a `WIN32` block and an `APPLE` block
and **no Linux arm at all**, while `wrapper/VST3/` ships
`SEVSTGUIEditorLinux.{h,cpp}` (X11) *and* `SEVSTGUIEditorWayland.{h,cpp}`.

**Jeff's X11 suspicion is backed by that wrapper's own design:** its CMake calls
X11 *"the only Linux embedding"* before VST3 3.8.0 and makes Wayland
conditional, falling back to *"X11 editor only"*.

**And my instrument was wrong in a way worth recording.** I grepped `nm` for
`DrawingFrameX11` and `DrawingFrameWayland` and got zero — but the VST3 uses its
own `SEVSTGUIEditor*` classes, so that grep returns zero on the binary that
**does** have an X11 editor. It was never a test of the thing I claimed.
**`ldd` was the reliable indicator and I had it in front of me the whole time.**

**Ruling, from Jeff, in session:** *"if CLAP wrapper lacks GUI support, we need
to add it."* So S43's option (ii) is authorised and is no longer an open product
question — it is the next job, with `SEVSTGUIEditorLinux` as the pattern.

**Learned:**

- **A negative grep is only evidence if you know the symbol would be there.**
  Zero hits for `DrawingFrameX11` felt like proof and was not — the working
  sibling scores zero on the same test. Confirm the instrument fires on a known
  positive before quoting its silence.
- **"Compare the artifact against a sibling that works" beats any amount of
  reading.** One `ldd` table across four formats said in four lines what three
  code-reading sessions had got backwards.
- **A conservative classifier needs its false branch tested hardest.** The
  interesting selftest cases here are the ones that must stay silent, because a
  classifier that always warns is exactly the bug being fixed.
- **Splitting "absent" from "empty" was worth the extra case**, because absent
  is the case CI actually hits and the one the old wording denied could happen.

**Not verified:**

- **The CI half of A34's Accept** — *"the `verify` job green with no warning"* —
  needs this merged and a run on `main`. It is silent locally under both the
  absent and empty layouts, and CI's is the absent one, so it should hold;
  should is not measured.
- **Windows and macOS** never run this harness (`verify.yml` is `ubuntu-24.04`),
  so the classifier's behaviour on their path conventions is untested.
- **Nothing about S43 (ii) is started.** This entry only corrects the record and
  records the ruling.

**Next:**

1. **S43 (ii) is now the job** — give the CLAP wrapper a Linux arm mirroring
   `wrapper/VST3/SEVSTGUIEditorLinux`, restore the commented-out X11 arm in
   `guiIsApiSupported`, and let `CLAP_WINDOW_API_X11` do what hosts use. Jeff has
   ruled it in.
2. **S37 is unblocked by the same ruling** — once the Linux CLAP has an editor,
   the editor reads `getBundleContentsFolder() / "Resources"` and S37's collision
   becomes real and sizable for the first time.

**Machine left clean.** One throwaway worktree under the session scratchpad, with
the engine package extracted inside it. All six repos synced to their default
branches at Jeff's request and clean. Nothing installed.

**Branch/PR:** `tide/linux/A34-scan-warning` — TideSynth, one tool plus backlog
and journal. No product code change.

## 2026-08-23 — linux — S43: the CLAP now refuses an API it just said it did not support

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Fourth item this session, at Jeff's direction. Took **S43** from the `linux`
NEXT cell — the row this box filed an hour earlier out of S37's first step —
and shipped **option (i) only**.

**Did:** `Processor_CLAP::guiCreate()` now returns false when
`guiIsApiSupported()` says no, instead of building an editor anyway.

### Why option (i) was eligible while option (ii) is not

S43's option (ii) — link a Linux windowing backend — needs Jeff to choose X11 or
Wayland, and until he does, no run may build against either answer. **Option (i)
is identical under every answer to that question:** whether or not a backend is
linked later, a function that answers "no api is supported" must not then report
a 1100x600 editor. So it is takeable now and does not pre-empt the decision.

### The change is five lines and the comment is longer than the code

    if (!guiIsApiSupported(api, isFloating))
        return false;

The assumption it enforces was **already written at that exact spot**, in the
function's own comment: *"we ignore the API and isFloating here, because we
handled them above and assume our host follows the protocol that it only calls
us with values which are supported."* Documented, and never enforced. On Linux
the combination is live rather than theoretical, because `guiIsApiSupported()`
has no X11 arm — it is commented out — so it answers false to **every** api.

### A/B, by reverting the file rather than editing the fix out

Same tree, same probe, three builds:

| build | `is_api_supported` | `guiCreate` |
|---|---|---|
| **BEFORE** — plain `origin/main` wrappers | all four **false** | **true, 1100x600** |
| **AFTER** — this fix | all four **false** | **false** |
| **CONTROL** — this fix + x11 forced true | x11 **true** | **true, 1100x600** |

**The control is the one that matters, and it is why this is a fix rather than a
regression.** Without it, "create now fails" is indistinguishable from "I
disabled the editor". Forcing `guiIsApiSupported` to answer true for X11 — with
the new guard still in place — puts the 1100x600 editor straight back, so the
guard passes through exactly when it should. That temporary line was marked
`TIDEDIAG`, removed afterwards, and `grep TIDEDIAG` is clean in all seven trees.

**A conformant host sees no change at all.** It asks first, gets the same answer
it always got, and never reaches the guarded line. This only changes what a host
that skips the query is told — and "no" is the honest answer, because `ldd` on
the built `.clap` links no X11, xcb or Wayland library to attach anything to.

### Blast radius, measured rather than asserted

`Editor_CLAP.cpp` appears in exactly one build file — `wrapper/CLAP/CMakeLists.txt`
lines 26-27 — and **`grep -rl Editor_CLAP` across `SE16` returns nothing**, so
`SynthEditCL` does not compile it on any platform. The standing rule for the
shared wrapper repos is *"rebuild SynthEditCL as well as TIDE"*; here that target
is structurally untouched, and saying so with the grep is better than a
forty-minute build that could only confirm it. **Every TIDE format target was
rebuilt** and the whole tree is rc=0.

**Cocoa and Win32 are unaffected by construction** — their arms in
`guiIsApiSupported` are live, so the guard returns true on those platforms
exactly as before. That is an argument, not a measurement; neither was built
here.

**Learned:**

- **I hit the CRLF trap the journal documents, on the first try.** Editing
  `Editor_CLAP.cpp` with plain Python text mode turned a 5-line change into
  **897 insertions / 875 deletions**. `newline=''` on *both* the read and the
  write gives the 25-line diff it should always have been. The lesson was
  already in `docs/lessons.md` twice — *"never edit a CRLF file with Python text
  mode"* — and reading it is evidently not the same as remembering it at the
  keyboard. `git diff --stat` immediately after the edit is what caught it.
- **A guard needs the passing case tested, not just the failing one.** The
  interesting measurement here was not "create fails now" but "create still
  succeeds when the answer is yes".
- **An unenforced assumption written in a comment is a defect with
  documentation.** This one named its own precondition and shipped without
  checking it; the fix is to make the comment true rather than to rewrite it.

**Not verified:**

- **No real CLAP host, still.** `clap_probe` is mine. What the fix does in
  Bitwig, Reaper or Ardour is unmeasured — though the change only makes the
  plugin agree with an answer those hosts already query.
- **macOS and Windows CLAPs** were not built. Their arms are live and the guard
  is a pass-through for them, which is reasoning rather than measurement.
- **Option (ii) is untouched and is still Jeff's call:** does the Linux CLAP
  ship a GUI, and over X11 or Wayland? Until then the honest state is a CLAP that
  loads, processes and says plainly that it has no editor.

**Next:**

1. **S43 is IN-REVIEW on option (i); the row stays open for (ii)**, which is a
   decision before it is a change.
2. **S37 remains unsizable until (ii) lands** — its collision needs an editor to
   exist before there is anything to collide.

**Machine left clean.** Two throwaway worktrees under the session scratchpad, one
per repo. Jeff's `~/SE/GMPI_Wrappers` was never checked out onto my branch — the
work is in a worktree — and all six repos are on their default branches and
clean. Nothing installed; `~/.clap` untouched.

**Branch/PR:** `tide/linux/S43-clap-gui-honesty` in **both** TideSynth and
GMPI_Wrappers. The GMPI_Wrappers PR is the change; the TideSynth PR is backlog
and journal. **Merging one without the other is harmless** — TIDE fetches
`GMPI_Wrappers` at `origin/main`, so the behaviour lands when the wrappers PR
does, and the TideSynth PR only records it.

## 2026-08-23 — linux — S37: the collision is unreachable, because the Linux CLAP has no GUI at all

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Third item this session, at Jeff's direction: the next Linux-specific task.
**S37** is the only genuinely Linux-specific `TODO` — `X1` and `X2` are the only
`platform: linux` rows and both are `BLOCKED`, `S32` and `R4` are `DONE`.

**Did:** took the first step S37 names for itself — *"whoever takes this needs a
CLAP host first, and that may be the real first step"* — and it turned the row
over.

### A CLAP host, without downloading one

`clap-validator` and `clap-info` are not installed here and Ardour 8.4 has no
CLAP support at all, which is why nobody had ever loaded a TIDE CLAP anywhere.
**The CLAP entry ABI is a C struct in a header the build already fetches**
(`free-audio/clap`, already a FetchContent dependency), so the host is 190 lines
and needs no third-party binary:
[tools/clap_probe.c](tools/clap_probe.c). dlopen → `clap_entry->init` → factory
→ create → init → activate, plus optional `--gui` and `--state`.

**TIDE Rack's CLAP loads, instantiates and activates.** First time on any
machine:

    plugins: 1
      [0] id=TIDE Synth: TIDE Rack  name=TIDE Rack  vendor=TIDE Synth  version=0.1.1
          init OK
          activate OK (48k, 1..512)

### And the row's premise does not survive contact

S37 says the Linux CLAP *"reads its module data from `~/.clap/Resources`"* and
collides with every other CLAP installed the same way. **It does not read it.
Measured, not inferred:** `strace -e trace=file` across **every** path a host can
reach headlessly — init, activate, `gui->create`, `state->save`, `state->load` —
shows **zero** accesses to `Resources`, `PlugIns`, `Prefabs` or any `.se.xml`.
The only syscalls naming the install directory are the `readlink` and `dlopen` of
the `.clap` itself.

**A/B, because absence of a syscall is a weak claim on its own:** the same probe
against an install with the `Resources` folder **deleted** behaves identically —
init OK, activate OK, gui created, and `state->save` produces **86 bytes both
times, byte-identical**. A plugin that needed those resources could not be
indifferent to their absence.

### Why: the Linux CLAP has no windowing backend linked into it

    $ ldd TIDE-Rack.clap
      libstdc++  libm  libgcc_s  libc          <- and nothing else

No X11, no xcb, no Wayland. `nm -D --undefined-only` finds no `XOpenDisplay`,
`XCreateWindow`, `wl_display_connect` or `wl_surface_commit`, and neither
`DrawingFrameX11` nor `DrawingFrameWayland` appears in the binary — though
gmpi_ui ships both.

So the editor — and `TideApp`, which is what actually reads the bundle's
`Resources` (`SynthEditSem/TideApp.cpp:506` and `:616`, the two *"missing from
bundle resources"* strings the row read off the binary) — is never constructed.
**The strings are in the binary because the translation unit is compiled in, not
because that code can run.**

**`GMPI_Wrappers/wrapper/CLAP/Editor_CLAP.cpp:34` agrees, and says so in a
comment block:** `guiIsApiSupported()` has arms for Cocoa and Win32, and its
Linux X11 arm is **commented out**. The prose four lines above it reads *"pretty
obviously, mac supports cocoa, windows supports win32, and linux supports X11"*.
Measured against the built artifact, the commented-out code is the honest one:
there is no X11 backend in there to support.

### The defect that IS live, filed as S43

`guiIsApiSupported` returning false for every API is correct given the above.
**`guiCreate` returning TRUE is not.** My probe asked about x11, wayland, win32
and cocoa — all `0` — and `get_preferred_api` declined; then it called
`create` anyway and got:

    gui created, size 1100x600

A conformant host consults `is_api_supported` first and would simply report that
TIDE Rack has no editor. A host that does not — and `guiCreate`'s own comment
says it *"assume[s] our host follows the protocol that it only calls us with
values which are supported"* — is told an editor exists, and will then hand
`guiSetParent` a window the plugin has no code to use.

**So the Linux CLAP's GUI story is: honest at the query, dishonest at the
create.** Filed as **S43**; `GMPI_Wrappers` is ALLOWED, but which way to fix it
is a product question (does the Linux CLAP ship a GUI at all, and over which
API?) rather than a one-line uncomment, so it is filed rather than fixed.

### What this does to S37

**Back to TODO, with its premise corrected rather than its options costed.** All
three options — (a) bundle directory, (b) namespaced folder, (c) embed — solve a
collision that cannot currently happen. **The prior question is S43's:** if the
Linux CLAP gains a GUI, the editor starts reading `getBundleContentsFolder() /
"Resources"`, and *then* S37 is real and its options matter. Sizing them before
that is guessing at a shape.

**One thing the row understates and it is worth keeping:** the shared folder is
not only `Resources`. `BundleInfo.cpp:699` derives `semFolder` the same way —
`getBundleContentsFolder() / "PlugIns"` — so a GMPI CLAP that scanned for modules
would share **that** directory too. Moot today for the same reason, and it
widens the blast radius whenever S43 is answered.

**And R4 ships a `Resources` folder beside the Linux CLAP that nothing reads.**
That is not a bug to fix today — it is exactly right the moment S43 is — but
its `README.txt` currently tells a user to install a folder for no present
benefit, which is worth knowing before anyone treats it as evidence the CLAP
works.

**Learned:**

- **A probe that stops before the code under test reports a healthy subject and
  proves nothing.** My first version ended at `activate` and returned rc=0 with
  the `Resources` folder deleted. The resource read is on the controller, not the
  processor, so the interesting question was two extensions further on. I nearly
  wrote "the CLAP is fine".
- **`ldd` answers "can this code path exist" faster than any amount of reading.**
  Six lines of output settled what the commented-out X11 arm, the `TideApp`
  strings and the row's own mechanism paragraph left ambiguous.
- **I nearly filed a bug inside `#if 1`'s dead `#else`.** `BundleInfo.cpp:693`
  has `path.find(L"TIDE") == 0` where all five siblings use `!= npos` — a real
  defect, in an arm that never compiles because the `#if` above it is literally
  `#if 1`. **This is S33's lesson twice in one week**, and the only reason it
  cost a minute instead of a session is checking the preprocessor before the
  logic.
- **A strings match is evidence the file was compiled, not that the code
  runs.** The row inferred the CLAP needs resources from `strings` finding the
  same diagnostics the VST3 has. Both binaries compile `TideApp.cpp`; only one
  can reach it.
- **Absence of a syscall wants an A/B.** "strace shows nothing" is much weaker
  than "strace shows nothing AND deleting the folder changes nothing AND the
  saved state is byte-identical".

- **A23's duplicate-id race is real and I hit it inside one session.** I read the
  next free id as S42, worked for an hour, and by commit time the mac box had
  landed its own S42 from a branch cut off the same `main`. `check-id-refs.py`
  caught it; renumbered to S43 before pushing. The row's own mitigation — re-check
  the id against freshly-fetched `origin/main` **at commit time, not at read
  time** — is the whole lesson, and it cost one command.
- **The shared-citation check (A31) fired on my own two rows**, because S37's
  re-framing and S43 both cited `Editor_CLAP.cpp:34`. They are genuinely
  different jobs, so S43 keeps the line and S37 now names the file and the
  function without it. That is the check working as designed on a real split.

**Not verified:**

- **No real CLAP host.** `clap_probe` is mine and deliberately minimal — it
  returns NULL for every host extension, so a plugin that needs `timer-support`
  or `posix-fd-support` to build its editor would fail here for a reason a real
  host would not have. That is precisely the condition the commented-out X11 arm
  tested for, so it is the obvious confound and I cannot rule it out from here.
  **What makes the conclusion survive it anyway is `ldd`:** an X11 backend that is
  not linked cannot be enabled by a host extension.
- **Windows and macOS CLAPs** were not probed. `guiIsApiSupported` has live arms
  for both, so this is Linux-specific by construction, but "by construction" is
  not a measurement.
- **The 86-byte state.** That looks small for a rack, and an empty default
  document may legitimately be that size — `TideApp` creates a blank container by
  default. I did not establish which, and it is not S37's question. Recorded
  because a CLAP that cannot persist a patch would fail V1's acceptance clause,
  and nobody has checked.

**Next:**

1. **S43 is the prior question** and it is a product decision before it is a
   code change.
2. **S37 stays filed and unsizable until S43 lands.** Do not cost its options.
3. **`tools/clap_probe.c` is now the box's CLAP host** — the thing S37 said was
   the real first step. It is 190 lines with no dependencies beyond the fetched
   headers, so the next CLAP question on any platform is cheap to answer.

**Machine left clean.** All work in a throwaway worktree under the session
scratchpad; the probe and both scratch installs live there. **Jeff's `~/.clap`
was read and never written** — it holds two GMPI CLAPs (`FreqAnalyser_CLAP`,
`SawDemo_CLAP`), neither of which carries bundle resources, which is why no
`~/.clap/Resources` exists on this box and the collision has never been observed
in the wild either. No plug-in was installed. All six repos are on their default
branches and clean.

**Branch/PR:** `tide/linux/S37-clap-resources` — TideSynth: one new tool, the
backlog and the journal. S43 filed. No product code change.

## 2026-08-23 — windows — P11: the diagnostic can't name the file, so it stops naming the cause (interactive, Jeff directing)

**Did:** took P11's Windows-only piece off the win NEXT cell — option (b), fix
the export diagnostic that blames the user's installation for what is actually a
partial multi-target build.

### What the NEXT cell asked for, and what it turned out to need

The cell's own words: *"make the export diagnostic name the stale `TIDE.gmpi`
instead of telling the user their installation is broken."* The row's own Accept
is looser and is what I actually built to: *"...or produces a message that names
the mismatch **it can actually detect**."*

**Naming the specific file honestly isn't achievable without either of two
things I didn't do.** `FlagRequiredModuleForExport` (`SynthEditLib/EditorLib/
DocOb.cpp`) gets a module type id string and nothing else when the lookup
misses — there is no live `Module_Info` to ask for a `Filename()`, because the
whole point is that nothing registered that id. Naming `TIDE-Rack.gmpi`
specifically would need either hardcoding a product name into a shared,
general-purpose diagnostic — the header comment already frames the mechanism
generically, `libControls.gmpi` is its own worked example, and `SynthEditLib`
has no business knowing `TIDE` exists — or new provenance-tracking
infrastructure (recording which file last supplied which id, persisted across
scans) that nobody sized and this row didn't ask for.

**So the message now says what it can prove instead of guessing what it
can't.** It used to assert one cause — *"this installation is broken"* — and
that assertion was **wrong** in the case P11 measured: `TIDE_VST3` built alone
leaves `TIDE`'s own module-database entry stale, which looks, from
`GetById()`'s perspective, identical to a genuinely incomplete scan. The new
text names both mechanisms and gives an action for either: re-scan, and if a
build produced this, rebuild every target of the plugin in question.

### Verified by building, not by reading the diff

Built `TIDE_Rack_VST3` alone from `TideSynth`, against
[SynthEditLib#33](https://github.com/JeffMcClintock/SynthEditLib/pull/33) —
the exact reproduction the row names, via the four `_FOLDER_OVERRIDE` variables
pointed at local checkouts:

```
configure rc=0  (-- Using local SynthEditLib folder)
build --target TIDE_Rack_VST3  rc=0, 0 errors
```

**Read the string back out of the built binary rather than trusting the source
edit** — this project has been burned by exactly that gap before (verify the
artifact, not the command line). `TIDE-Rack.vst3` contains the new text
verbatim (1 UTF-16LE hit on `"rebuild every one of its"`), and the old
`"this installation is broken"` string is **absent** (0 hits) — not merely
unreached, genuinely gone from the compiled output.

Full tree also builds clean afterward: `cmake --build build --config Release
--parallel`, rc=0, 0 errors, all four Windows targets.

### The accident this session nearly compounded

Made the edit directly in Jeff's live `SynthEditLib` checkout (which was clean)
instead of a worktree — caught before committing anything, by `git status`
showing modified files on `main`. Fixed by creating a branch **in place** and
switching to it (`git checkout -b`, which carries uncommitted changes with it),
rather than stashing or discarding. Then, still cd'd into that live checkout
rather than the intended `TideSynth` worktree, two `git worktree add` calls
landed against `SynthEditLib` instead — one reused the branch name and errored
harmlessly, the second silently created a second worktree and branch
(`tide/win/P11-diagnostic-build`) in the wrong repo. Both cleaned up (`git
worktree remove --force`, `git branch -D`) before anything was pushed. **The
common cause was assuming cwd carried across calls the way I intended rather
than checking it** — `pwd` before the first `worktree add` would have caught
this before it needed cleaning up at all.

**Not verified:** `SynthEdit2` (the WinUI3 desktop editor, `FlagRequiredModule
ForExport`'s other real caller) was not built — its `.vcxproj` links out of
Jeff's own Debug tree, so verifying it means writing into a tree this session
must not touch, per [[se16-scratch-ninja-build]]. macOS, Linux and iOS were not
exercised — this touches no platform-specific code, but nobody ran them.

**Learned:**

- **A NEXT cell's paraphrase and a row's own Accept can disagree, and the row
  wins.** "Name the stale `TIDE.gmpi`" is a previous run's gloss; "names the
  mismatch it can actually detect" is what P11 itself commits to, and it is
  the honest, buildable version of the same intent.
- **A diagnostic that can't tell two causes apart should say so, not guess.**
  The false specific claim ("this installation is broken") was worse than a
  correct general one, because it sent the reader to repair the wrong thing.
- **`cd` inside a command chain does not substitute for checking cwd before
  the NEXT command that assumes it.** Two accidental worktrees in the wrong
  repo, from the same root cause, in one session.

**Next:** **S35** is the mac-side companion — the actual scan-domain mismatch
this row measured but didn't fix, and it is GATED (`SynthEditLib`) plus
PR-GATED (`GMPI`), so it needs Jeff either way. **P3** remains this platform's
only own-boxed row and is GATED for the same reason P11 was.

**Branch/PR:** `tide/win/P11-diagnostic-build` — TideSynth, backlog and journal
only. Product change is [SynthEditLib#33](https://github.com/JeffMcClintock/SynthEditLib/pull/33),
not merged.

---

## 2026-08-23 — macos — S16: the missing-binary defect is gone, and it was hiding another (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Took S16 — the mac suite 44/57 red for a hardcoded build path. Fixed in
[SynthEdit#72](https://github.com/JeffMcClintock/SynthEdit/pull/72).

**The row under-counted: FOUR hardcoded sites, not two.** It names the `build/`
pair; the `UnitTest/` pair (`projecttests.cpp:78`, `layouttests.cpp:45`) had the
identical defect and would have been left behind by a fix that trusted the row.

Both now derive from `tests/CMakeLists.txt` — `CMAKE_BINARY_DIR` and the repo
root — the same trick `synth_ui_tests` already used for `REFERENCE_IMAGES_DIR`.
**A missing define is a compile error, not a silent fallback.** A default
pointing at someone else's home directory is precisely how this survived long
enough for the C-stage rows to quote "92 tests all RC=0", a Windows number.

**The path fix alone changed NOTHING measurable, and saying so matters.**
`dsp_tests` shells out to `SynthEditCL` and `cancellation`; building the test
target does not build them. Deriving the path only moved the failure from "wrong
directory" to "right directory, still empty". `add_dependencies` was the other
half. Necessary but not sufficient — if I had stopped at the count I would have
reported a fix that fixed nothing.

**Measured, same checkout, before and after:**

| | before | after |
|---|---|---|
| failed | 44 | 40 |
| passed | 13 | 17 |
| "No such file or directory" | **82** | **0** |
| references to the dead checkout | **0** | **536** |

**82 to 0 is the row's defect, gone.** The four-test swing is small because the
other 40 were never blocked on the binary.

**And the last column is a second defect this one was masking.** 46 `.synthedit`
fixtures under `UnitTest/` bake in absolute paths to
`/Users/jeffmcclintock/SynthEdit/`, a checkout that no longer exists. Those
references appear **only after** the fix, because until the harness stopped
dying on a missing binary the tests never got far enough to load a fixture.
Filed as **S42**. A further 89 files carry absolute paths to the CURRENT
checkout — correct today, wrong on any other machine, so the same defect not yet
triggered.

**Not verified:** Windows and Linux. The change is symmetric — both platforms'
hardcodes are replaced by the same derived defines — but only macOS was run.

**Separately, and Jeff should know: his `SynthEdit/build` tree cannot
reconfigure at all.** S17 reports `gmpi_wrappers` has both a local override and
a fetched copy. It predates this change — a clean tree fails identically, which
I checked by stashing before blaming myself — so I built in a scratch directory
rather than delete 3.4 GB of his.

## 2026-08-23 — linux — E1c: the deciding render lands at −140 dBFS, and neither the module nor the pitch is the variable on its own

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Second item this session, at Jeff's direction after #332 merged. Took **E1c**
from the `linux` NEXT cell, which this box had re-pointed there hours earlier.

**Did:** ran the deciding render the row has been waiting two days for, then
used it to retire the gates the row exists to complain about.

### The answer, and it is the pre-committed branch

`osc_naive_note64`, Linux x86_64 against the macOS-seeded reference:

    null −140.1 dBFS    peakdiff −90.3 dBFS    peak −6.0 dBFS

The case's own `tolerance_reason` pre-committed the reading before anyone could
rationalise it: *"near −123 dBFS (rounding class, ~1 LSB) and the discriminator
is the PITCH VALUE"*. **−140.1 is that branch with 17 dB to spare**, and
−90.3 dBFS is exactly 1 LSB at 16 bits.

### But the binary was too coarse, and the same run says why

The row's two options were *the pitch value* or *the voice chain*. The full
table, all measured in one run, supports neither cleanly:

| case | oscillator | pitch | reference from | residual | class |
|---|---|---|---:|---:|---|
| `osc_naive_sine` | naive | 440.0 Hz, undriven | Linux | −73.5 (E1a) | **drift** |
| `osc_naive_pitched` | naive | 440.0 Hz, 5 V | Linux | −73.5 (mac half) | **drift** |
| **`osc_naive_note64`** | **naive** | **329.63 Hz, 4.583 V** | **macOS** | **−140.1** | **rounding** |
| `voice_midi_note` | naive | 329.63 Hz, MIDI 64 | Linux | −123.1 (mac half) | rounding |
| `prefab_oscillator` | **core** | 440.0 Hz, 5 V | macOS | **−131.1** | rounding |
| `prefab_filter` | **core** | 440.0 Hz, 5 V | unknown | **−121.4** | rounding |

**The naive oscillator at note 64 is clean, and the CORE oscillator at 440 Hz is
also clean.** So the module alone is not the variable and the pitch value alone
is not the variable. **It is the interaction: the naive oscillator at 440.0 Hz
specifically.** Every case on the board is now explained, and the explanation is
narrower than either option the row offered.

I am not claiming a mechanism beyond that. Which of the two code paths computes
its phase increment differently, and why 440.0 Hz in particular, is unmeasured.

### Two controls, because the engine was not the one that seeded the references

The published Linux package is **SynthEditCL V1.6.192**; every reference was
seeded on **V1.6.186**. That is a second variable, and rendering across it
without checking would have confounded the whole experiment.

**The three Linux-seeded references all came back bit-identical — `null=−inf`.**
`osc_naive_sine`, `osc_naive_pitched` and `voice_midi_note`, same platform,
different engine build, zero residual. **So the version bump is not a variable**,
and the macOS-seeded comparisons are single-variable after all.

**And the pitch was verified rather than assumed**, as the mac half did when it
seeded: zero-crossing count over the 2.0 s render gives **329.50 Hz** against
note 64's 329.63 — 1318 crossings, where ±1 is ±0.25 Hz. **The same figure the
macOS box measured.** `osc_naive_sine` gives 439.75 and `prefab_oscillator`
440.00, so the 440 cases really are at 440.

**The developer-box module-scan warning was eliminated rather than tolerated.**
The harness warned it had scanned `~/.local/share/SynthEdit/modules` — Jeff's
demo `.gmpi` files. Re-running under `XDG_DATA_HOME` pointed at an empty scratch
directory moves the scan there and it holds **zero files**, so the render
provably used only `--modules`. **Both runs are numerically identical**, so the
stray folder was not a confound — demonstrated, not assumed. That is stronger
than CI's own evidence, where the folder simply does not exist.

### The gates: E1c's actual Accept, and how far it got

The row's complaint is that `prefab_oscillator` and `prefab_filter` carry
−67/−62 gates against residuals of −131.1 and −121.4 — *"~55 dB of margin for a
real regression to hide in"*. Their justification was inherited by analogy from
E1a's measurement of a **different** module. **That justification is now not
merely unmeasured but refuted:** the core `Oscillator` at 5 V does not drift.

All three rounding-class cases are moved to the project **defaults (−100/−86)**,
with the measurement and the platform pair written into `tolerance_reason`.

**THE POSITIVE CONTROL, which is what the Accept actually asks for, and it turns
the "55 dB of margin" into a number.** A 3-sample localized glitch injected into
the real Linux render, compared against the same reference through the harness's
own `null_test`:

| glitch | rms | peak | old −67/−62 | new −100/−86 |
|---:|---:|---:|---|---|
| none | −131.1 | −90.3 | PASS | **PASS** |
| 2 LSB | −127.1 | −84.3 | **PASS** | FAIL |
| 6 LSB | −119.5 | −74.7 | **PASS** | FAIL |
| 12 LSB | −113.7 | −68.7 | **PASS** | FAIL |
| 26 LSB | −107.0 | −62.0 | **PASS** | FAIL |
| 40 LSB | −103.3 | −58.3 | FAIL | FAIL |

**The old gates are blind to localized damage up to 26 LSB; the defaults catch it
from 2 LSB; and the undamaged render still passes.** Identical for all three
cases. 26 LSB is not a coincidence — `10^(−62/20) × 32768` is exactly 26, so the
blind spot is the peak gate read back as amplitude.

**My first attempt at this control was worthless and I nearly shipped it.** I
detuned the pitch pin by 1e-4 V and got a resounding failure — −16.6 dBFS, all
three cases. But the *old* gates catch that too, so it demonstrates nothing
about tightening. A control has to separate the two gates, not merely fail.

### What is NOT done, and it is not work this box can do

E1c's Accept says gates justified *"across mac and Linux (**both directions**,
since a reference seeded on one platform is the asymmetric case)"* and a positive
control *"on both platforms"*. **I have one direction and one platform.** The
reverse — a macOS render against a Linux-seeded reference — needs the mac box,
and [verify.yml](.github/workflows/verify.yml) is `ubuntu-24.04` only, so nothing
runs it there today. The row goes IN-REVIEW, not DONE.

**Tightening cannot break CI in the meantime**, because only Linux runs the
harness and Linux is what I measured. If a mac run is ever added and these fail,
that is the missing measurement being made, not a regression.

**`prefab_filter` carries a caveat `prefab_oscillator` does not.** Its reference
provenance is `recorded: "unknown"` — nobody wrote down where it was seeded. I
did not tighten it on faith: the three references known to be Linux-seeded all
rendered `−inf` in this same run, so a −121.4 dBFS residual means this one was
almost certainly seeded elsewhere. **That is strong evidence and not proof** — an
older engine on Linux is not excluded, though V1.6.192 reproduced V1.6.186's
Linux renders exactly. It is in the case file as an inference, labelled as one.

**Learned:**

- **A pre-committed binary outcome is worth the setup, and worth distrusting
  when it lands.** The row pre-committed two readings; the measurement matched
  one of them and the matching reading was still wrong, because `prefab_*` sit at
  440 Hz and are rounding class. Pre-commitment stops you rationalising the
  number — it does not stop the dichotomy being false.
- **A regression control must separate the two gates, not merely fail.** A 1e-4 V
  detune fails everything by 50 dB and proves nothing about a tightening. The
  useful control is the one sized to land *between* old and new.
- **Check the engine version against the reference's before believing a
  cross-platform residual.** V1.6.192 against references seeded on V1.6.186 is a
  second variable, and it cost one extra look to retire — three same-platform
  references at `−inf`.
- **A scan warning can be eliminated instead of noted.** Three runs of this
  fleet have recorded "the engine scanned folders outside `--modules`" as a
  caveat. `XDG_DATA_HOME` at an empty directory removes it, and the identical
  numbers prove the caveat was harmless *here* rather than assuming it.
- **A warning that fires on every run is not a caveat, it is noise — and the way
  to find out is to check its own claim.** The harness annotates this one
  *"never on a clean CI runner"*; the `verify` job on this very branch emits it,
  naming `/home/runner/.local/share/SynthEdit/modules`. The engine probes the
  XDG path whether or not it exists, so the warning reports the probe rather
  than a finding. Filed as **A34**.
- **`recorded: "unknown"` provenance can sometimes be settled by measurement.**
  If same-platform renders come back bit-identical, a non-zero residual is itself
  evidence about which platform seeded the reference.

**Not verified:**

- **The reverse direction and the macOS positive control** — the Accept's
  remaining half, above.
- **Which transcendental, and why 440.0 Hz.** Bounded now to the naive
  oscillator at that pitch, which is much tighter than the row started with, but
  nothing here opens the phase-increment computation.
- **Windows.** Never rendered against any of these references.
- **`prefab_envelope` and `prefab_midi`** still have `recorded: "unknown"`
  provenance and render `−inf` here, so they are same-platform or trivially
  stable and this run cannot tell which. Untouched.

**Next:**

1. **A macOS render of these three cases closes E1c.** It is one harness run with
   the published mac engine package, and it is the whole remaining Accept.
2. **`verify.yml` runs on Linux only**, so the gates I just tightened are checked
   on exactly one platform. Extending that matrix is `.github/workflows/**` and
   needs Jeff's credential — worth doing while the render-regression CI question
   from #331 is open, since it is the same argument.

**Machine left clean.** All work in a throwaway worktree under the session
scratchpad; the engine package was downloaded there and is not installed. Jeff's
`~/.local/share/SynthEdit/modules` was read but never written. All six repos were
synced to their default branches at the start of this item at his request, were
clean then, and are clean now.

**Branch/PR:** `tide/linux/E1c-note64` — TideSynth, [#333](https://github.com/JeffMcClintock/TideSynth/pull/333); three test cases plus the
journal and backlog. A34 filed. No product code change.

## 2026-08-23 — linux — the render references fail on Linux too, and the tolerance that merged four hours ago does not reach it

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** STEP 1. Four open `platform:linux` issues; one was actionable, and
working it turned into the measurement [#291](https://github.com/JeffMcClintock/TideSynth/issues/291)
asked an x86_64 non-Mac box for: **`tide_render_regression` run on Linux, where
it fails 5 of 10.**

**OVERTAKEN WHILE I WAS WRITING IT UP, AND I AM LEAVING THE ORDER VISIBLE.**
[#331](https://github.com/JeffMcClintock/TideSynth/pull/331) merged mid-run and
its first CI run measured all three platforms — so "nobody has ever run this on
Linux" was true when I started and false by the time I pushed. **CI's Linux
column is my local figures to three decimals**, from a different machine, which
is worth more as confirmation than my run was as news. What survives as new is
below and none of it is the pass/fail: the unit-test control, two alpha defects
in the comparator, and a measured third option for the metric that the row's
conclusion excludes.

### The four issues, and why only one was mine to act on

| issue | disposition |
|---|---|
| [#306](https://github.com/JeffMcClintock/TideSynth/issues/306) build failure, `tide/mac/M2-ios-configure` | **closed** — cause resolved, verified by building here |
| [#291](https://github.com/JeffMcClintock/TideSynth/issues/291) render references re-baked on arm64 | **measured** — its item 2 is this box's, see below |
| [#88](https://github.com/JeffMcClintock/TideSynth/issues/88) `SynthEditJuce` misses `Dialogs_editor2.cpp` | left open — `SE16/SynthEditJuce/` is GATED by default, the target is deprecated and generated by nothing, so it is not a build break and A17's exception does not reach it. Unchanged from the 2026-08-18 assessment |
| [#156](https://github.com/JeffMcClintock/TideSynth/issues/156) ctest defaults to a macOS home | left open — `SE16/tests/` is GATED by default and the suite passes once pointed at the right folders, so not a build break either |

### #306: closed on a build, not on CI being green

Its own text says *"close this only after verifying the fix by building on that
platform"*. Clean scratch worktree at `d276be0`, no overrides, the exact CI
recipe (`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, then
`--parallel 6`): **configure rc=0, build rc=0, zero compiler errors** (166
warnings), and all four Linux artifacts emitted — `TIDE-Rack`, `TIDE-Rack.gmpi`,
`TIDE-Rack.clap`, `TIDE-Rack.vst3/`. **So this platform's default branch builds
and there is no new platform issue to file.**

### The real finding: Linux fails the render references, and it fails them big

`modules/common` is a dependency-free CMake project, so this costs one small
configure. Built at current `main` `805b7e6`, Ubuntu 24.04, GCC 13.3.0, glibc
2.39, x86_64:

| scene | pixels moved | limit | worst Δ | limit |
|---|---:|---:|---:|---:|
| knob | **35.359%** | 0.800% | **142** | 40 |
| materials | **34.847%** | 0.800% | **63** | 40 |
| shapes | **67.028%** | 0.800% | **46** | 40 |
| glass | **54.562%** | 0.800% | **53** | 40 |
| glow | **61.528%** | 0.800% | **62** | 40 |
| all five `(fast)` | 0.000% | — | 0–2 | — |

**Deterministic:** three consecutive runs are byte-identical in every figure.

**THE TOLERANCE THAT MERGED IN [#330](https://github.com/JeffMcClintock/TideSynth/pull/330) FOUR HOURS AGO DOES NOT REACH THIS, AND IT IS NOT CLOSE.**
`kMaxChangedFraction` went 0.004 → 0.008 on a cross-ISA worst of 0.444%. Linux
is **43× to 84× over the new limit**, and it also trips `kMaxChannelDelta`,
which that PR left untouched on the measured grounds that *"worst delta anywhere
cross-ISA is 20"* — true on that Mac, where both builds share one libm. All five
Linux worst deltas are 46–142. **#330 is not wrong; it is calibrated to a
different variable.** arm64-vs-x86_64 on one Mac holds libm fixed and moves the
ISA; Linux-vs-macOS moves libm, which S27's own earlier work had already
identified as the one that matters.

**So S27's Accept clause is not met.** It asks for *"`tide_render_regression`
passes on at least two platforms from one set of committed references"*. It
passes on one. **CI now says the same and adds Windows**, which fails the same
five to within 0.014 on `shapes` and identically on the other four — so Linux
and Windows agree with each other and macOS is the outlier.

### The renderer is fine. It is the metric that cannot say so

Three controls, and they matter more than the failure:

1. **`tide_render_unit` passes 20/20 on Linux** — also a first run here. White
   furnace, bloom symmetry (1.5e-09 against a 1e-06 limit), fast-vs-full
   coverage agreement, quality-rung ordering. Nothing about the tracer is broken
   on this platform.
2. **All five `(fast)` variants match at 0.000%, worst delta 0–2.** Fast mode
   uses a fixed sub-pixel grid and one transcendental against the tracer's 19,
   so the bit-stable subset is bit-stable here too.
3. **The pictures are indistinguishable.** I rendered reference, actual, and an
   8×-amplified absolute difference side by side for all five. The difference
   images are structureless high-frequency noise over the lit surfaces — no
   moved silhouette, no lost highlight, no shifted geometry. Jeff's bar for
   #291 is *"they have to look the same to a human"*, and by that bar these pass.

**This cross-validates #291's macOS table to two decimal places**, from the
opposite direction — that table is a Mac render against x86_64 references, this
is a Linux render against Mac references:

| | knob | materials | shapes | glass | glow |
|---|---:|---:|---:|---:|---:|
| #291 mean Δ (mac) | 1.68 | 1.77 | 2.88 | 2.48 | 3.14 |
| **this run (linux)** | **1.68** | **1.76** | **2.88** | **2.48** | **3.15** |
| #291 worst Δ (mac) | 142 | 63 | 46 | 53 | 62 |
| **this run (linux)** | **142** | **63** | **46** | **53** | **62** |

Two boxes that have never met, measuring different pairs, agree to the digit.
The divergence is one property of libm, not a per-machine accident.

### Two things #291's proposed metric needs, found by trying it

**`knob`'s worst delta of 142 — the largest number in #291's whole table — is in
a pixel that is 99.2% transparent.** Reference RGBA at (24,39) is
`(255,255,255,2)`, actual is `(117,113,114,2)`. The comparator takes the max
across all four channels and never consults alpha, so it is scoring RGB in
pixels nobody can see. **Restricted to pixels with α≥128, knob's worst delta is
28 — under the existing limit of 40.** The other four scenes are fully opaque
and unaffected (63, 46, 53, 62 either way).

**And the downsampling lead #291 offers breaks on that same scene unless the
comparator premultiplies.** Box-downsampling RGBA without premultiplying mixes
the arbitrary RGB of invisible pixels into visible ones: knob's worst delta at
2× and 4× comes out **255**, worse than the native 142, which is nonsense.
Premultiplied, it behaves:

| | knob | materials | shapes | glass | glow | **glow vs shapes** |
|---|---:|---:|---:|---:|---:|---:|
| native worst Δ | 142 | 63 | 46 | 53 | 62 | 243 |
| 4× premultiplied worst Δ | **15** | **10** | **5** | **10** | **10** | **203** |

`glow vs shapes` is the negative control — two genuinely different pictures.
**This is the deliverable, and it contradicts the conclusion S27 now carries.**
That row's newest text reads the CI result as leaving *"per-platform references
or pinning the math ... the only ones left"*. Those are the only two left **for
the metric as written**. The measurement says: to pass Linux on the native metric you would need `kMaxChangedFraction`
≈ 0.68 against a negative control of 0.99, a separation of 1.5×, which is
indistinguishable from deleting the test. On the 4× premultiplied worst-delta
metric the same pass needs a limit of ~20 against a negative control of 203 —
**13.5×**. One reference set can survive every platform on the second and cannot
on the first.

**I did not re-bake the references**, which is #291's item 2 and is the thing
this box is nominated for. Two reasons, and the second is the real one: it would
have inverted the failure onto macOS exactly as `246399a` inverted it onto us,
and #291's item 1 — *decide the metric* — is unanswered, so any re-bake now is
work thrown away the moment the metric changes. **That decision is Jeff's**; the
measurement it needs is above.

### And STEP 4's own rotation could not pass the lint that polices it

`check-journal-prepend.py` failed this change, naming two entries as missing
that are both plainly present — one still in `JOURNAL.md`, one in
`JOURNAL-2026-08.md`. **The check was wrong, and the defect fires on every
rotation, which STEP 4 mandates every run.**

`split_entries()` bounded each entry by the next **dated** heading, so the
oldest entry's block ran past the end of the entries and swallowed the two
undated sections below them — the rotation rule and the entry template, about
4.5 KB that belongs to no entry. Rotating changes *which* entry is last, so that
trailer moves from one block to another and two comparisons break at once: the
entry newly promoted to last looks edited (its text grew by the trailer), and
the entry rotated out cannot be matched against its archived copy (the archive
holds the entry, not the trailer). Measured here at 9,696 bytes against 14,287
for the same entry.

Fixed by bounding an entry at the next `## ` of **any** kind. `canonical()`
already exists for exactly this class of bug at the entry/entry boundary — it
strips a trailing `---` — and cannot reach this one, because a whole trailing
section is not a separator.

**Verified both directions, because a lint that stops failing is not obviously
fixed:**

| case | rc | wanted |
|---|---:|---:|
| this run's actual change | **0** | 0 |
| an entry edited in place | **1** | 1 |
| an entry dropped, not archived | **1** | 1 |
| new entry appended at the bottom instead of prepended | **1** | 1 |
| `--selftest` | **0**, 7 cases | 0 |
| **replay of [#321](https://github.com/JeffMcClintock/TideSynth/pull/321)**, a past rotation that passed | **0** | 0 |

The replay is the one that matters — the fix must not change a verdict that was
already right. **And I verified the preservation claim without the tool at all**
before touching it: all 18 base entries appear verbatim in `JOURNAL.md` or the
archive, and exactly one entry is new.

**Why the bug survived this long:** the base of the last rotation to pass
(`c39ab26`) had **no trailer at all** — the sections had been rotated away
themselves at some point and were restored by that commit. So the check has been
green over rotations that could not trip it, and the first rotation against a
file with a trailer at the bottom is this one.

### Incidental, one command, and the linux NEXT cell asks for it

`sh tests/s31_kill_named_test.sh` — **7 passed, 0 failed.** S31's row says the
suite has never run on Linux, *"the only platform the bug is real on"*, and that
if it fails there the row should be reopened. It does not. Recorded on the row;
nothing else about S31 changed.

**Learned:**

- **A tolerance is calibrated against a variable, and the variable is not in the
  number.** #330 measured 0.444% cross-ISA and set 0.8%. Both figures are
  correct and the conclusion does not transfer, because holding libm fixed while
  moving the ISA measures a different thing from moving libm. A tolerance should
  record which variable it was fitted to, or the next platform reads it as a
  general bound.
- **The largest number in a table is worth checking before it drives a
  decision.** `knob`'s 142 has been the headline figure in this row, its issue
  and two PRs, and it is a pixel at 0.8% opacity. One `getpixel` settles it.
- **A comparator that ignores alpha is measuring pixels the product never
  shows** — and it is the same defect as the downsampling artifact, seen twice:
  once in the metric that exists and once in the metric proposed to replace it.
- **Two boxes agreeing to two decimals from opposite directions is worth more
  than either measurement.** Neither run could have known the other's figures;
  matching worst deltas of 142/63/46/53/62 is what turns "a difference" into "a
  property of libm".
- **Run the cheap control before the expensive fix.** 20/20 unit checks and five
  byte-stable fast renders took eleven seconds and are what license the sentence
  "the renderer is fine". Without them this is a Linux bug report.
- **A check that has only ever seen the easy case is untested, not proven.**
  `check-journal-prepend.py` has policed rotations for eleven days and the
  last rotation before this one ran against a file with no trailer to
  swallow. Green over inputs that cannot trip it is not evidence.
- **When a lint fails, prove the artefact by hand before believing either of
  you.** Eighteen entries checked against two files settled who was wrong in
  about a minute, and it is what made patching the checker a fix rather than
  a workaround.
- **A dependency-free subproject is a gift.** `modules/common` configures and
  builds in seconds with no SDK fetches, which is why this measurement was
  affordable at all — and it is why [#331](https://github.com/JeffMcClintock/TideSynth/pull/331)
  can run it as a job independent of `guard`.

**Not verified:**

- **Windows, by me.** CI answered it during this run — same five failures,
  agreeing with Linux to three decimals — but I did not measure it, and the
  alpha and downsampling findings above have not been re-derived there. They are
  properties of the comparator rather than of a platform, so they should hold;
  "should" is not "measured".
- **Whether any single tolerance is honest.** I measured that the native metric
  cannot carry one and that a 4× premultiplied worst-delta metric plausibly can.
  Nobody has implemented the second, and my numbers come from five scenes and
  one negative control.
- **The alpha finding is about `knob` only** — it is the one scene with
  transparency (10,728 of 25,600 pixels at α=0). It changes knob's headline and
  nothing else's.
- **The full build was at `d276be0`, not `805b7e6`.** The only delta is
  `RenderRegression.cpp`, which the plugin build excludes (`TIDE_RENDER_PREVIEW`
  is force-OFF at `CMakeLists.txt:380`), so it cannot affect that result — but I
  did not rebuild to prove it.

**Next:**

1. **#331 merged and is already red on two of three platforms**, exactly as its
   body predicted and asked for. The open question it leaves is whether a job
   that is red-by-design should gate PRs while the metric is undecided — every
   PR from now on carries two red checks that mean "unanswered question", and
   the journal's own lesson is that a check which is always red trains people to
   skim the check list.
2. **S27 goes back to needing a decision, not a tolerance.** Recorded on the row
   rather than changing its status: the row is mac's and its PR merged, so
   flipping it is not mine to do on the strength of a measurement it has not
   seen.
3. **Windows should run the same two commands** — configure `modules/common`,
   run `tide_render_regression` and `tide_render_unit`. It is minutes, and it is
   the last unknown in the table.

**Machine left clean.** All work in three throwaway worktrees under the session
scratchpad, removed afterwards; nothing was built in `~/TideSynth`, and no
plug-in was installed. All six repos (`TideSynth`, `SE16`, `SynthEditLib`,
`gmpi_ui`, `GMPI_Wrappers`, `GMPI`) were clean and on their default branches at
the start, were not written to, and still are.

**Branch/PR:** `tide/linux/issue-291` — TideSynth: journal, backlog, and one
lint script. No product code change. Issues: #306 closed, #291 commented; #331 had already merged, so its thread got
the measurement as a comment rather than a review.

## 2026-08-23 — macos — the render CI job's first run answers S27 (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

The job I added an hour ago ran, and its first run settles a question two
sessions had been circling.

**macOS passes 10/10. Linux and Windows each fail the same five.** Pixels moved
against the arm64-baked references, limit 0.800%:

| scene | linux | windows |
|---|---|---|
| knob | 35.359% | 35.359% |
| materials | 34.847% | 34.847% |
| shapes | 67.028% | 67.014% |
| glass | 54.562% | 54.562% |
| glow | 61.528% | 61.528% |
| all five (fast) | **0.000%** | **0.000%** |

**Two facts decide the row.**

**Linux and Windows agree with each other to three decimals.** Four scenes are
identical; `shapes` differs by 0.014. Both differ from macOS by 35-67%. So
**macOS is the outlier**, not "every platform differs" — which means two
reference sets at most, not three. I had assumed three; the data says otherwise.

**All five `(fast)` variants are bit-identical on all three platforms.** That is
the row's surviving hypothesis confirmed from the other side: fast mode uses a
fixed sub-pixel grid and one transcendental, the full path uses nineteen, and
the divergence lives entirely in the transcendental-heavy path.

**A tolerance cannot fix this.** Raising 0.004 -> 0.008 earlier today was right
for cross-ISA noise at 0.444% and is irrelevant at 67%. Two of the row's three
options survive — per-platform references, or pinning the math — and the numbers
say per-platform needs only two sets.

**The figures also identify where the ORIGINAL references came from.** They
match this row's first measurement (34.262 / 34.528 / 66.410 / 54.674 / 61.312)
almost exactly. So the references were baked on a Windows-or-Linux box, and the
2026-08-22 re-bake moved them to macOS — flipping which platforms fail rather
than fixing anything. Nobody could have known, because nothing ran the test
anywhere.

**The job is red on two of three, and that is what it was merged to find out.**
I said in the PR to expect that. It is a measurement, not a breakage — but it
does mean this cannot sit on `main` as a gating job until Jeff picks an option,
and that is his call, not mine to pre-empt.

**Not verified: which transcendental**, still. It is now bounded to the 19-call
full path against the 1-call fast path, on macOS against a Windows/Linux
consensus — a much smaller box than this morning.

## 2026-08-23 — macos — the render test finally runs somewhere (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Added a `render` job to `build.yml` running `tide_render_regression` on all
three platforms. Nothing had ever run it: the test appeared in no workflow, and
TIDE's root force-disables `TIDE_RENDER_PREVIEW`, so S27's cross-platform
question was unanswerable except by a human building a second architecture by
hand — which is exactly how its "1% of margin" warning finally surfaced, two
sessions late.

**Independent of `guard` and `build` on purpose.** `modules/common` is a
dependency-free CMake project: it needs neither a root CMakeLists nor the SDK
fetches, and it should keep reporting when those are broken.

**`fail-fast: false`**, because with a divergence question the platforms that
PASS are half the answer. On failure the `-actual` PNGs upload as an artifact —
the log cannot tell "the maths moved by a bit" from "a light went out".

**No `continue-on-error`.** B1 spent effort removing that from the matrix
because it made the whole thing decorative; adding a new job with it would undo
that on the same day.

**I expect the first run to be informative rather than green, and that is the
point.** The references are baked on macOS arm64. Whether Windows and Linux
match has never been measured. A red Windows or Linux here is the measurement
S27 has been waiting for, not a broken build — and the row already lists the
three ways to resolve it.

**Verified:** the exact step bodies were rehearsed locally — configure, build,
`all 10 references match`, rc=0. The executable lookup drops the `-perm -u+x`
test and matches `.exe` too, because on the Windows runner the executable bit is
not meaningful and the binary is named differently. YAML parses.

**Not verified: the job has never run on Windows or Linux.** That is what
merging it is for.

## 2026-08-23 — macos — S27: the symptom is gone, the row's own prediction came true (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Took S27 intending to answer its open question — *which transcendental dominates*
— and the first measurement made that question moot.

**The 5-of-10 failures at 35-67% do not reproduce.** On current `main`, arm64
passes **all ten checks**, worst 0.023%. Jeff re-baked the references at
`246399a` on 2026-08-22, after this row was measured. The row's headline was
stale and I would have spent the day chasing a fixed bug if I had started from
its hypothesis instead of from a run.

**But the row predicted its own successor, and it was right.** It warned that
`shapes` cross-ISA sat at 0.396% against a 0.400% limit — *"1% of margin ...
borderline flaky"*. It has crossed. Building the same dependency-free project
**x86_64 on this same Mac** — same OS, same libm, same compiler, only the ISA
differing — `shapes` fails at **0.444%**. Three consecutive runs give 0.444% and
worst delta 14 at (122,42), byte for byte: deterministic, not flaky.

**Fixed by taking one of the three options the row offers Jeff — the measured
tolerance.** `kMaxChangedFraction` 0.004 -> 0.008. Per-scene worst cross-ISA:

    glow 0.014%   glass 0.111%   materials 0.181%   knob 0.191%   shapes 0.444%

so 0.008 is ~1.8x the worst. **`kMaxChannelDelta` (40) is untouched**, and never
came close — worst delta anywhere cross-ISA is 20.

**Loosening a tolerance without proving the test still bites is just weakening
it, so I measured that too.** Perturbing the key light and re-rendering:

    kFastKey 0.90 (baseline)   0.023% moved   0 FAILs
    kFastKey 0.85 (-5.6%)     24.820% moved   4 FAILs
    kFastKey 0.80 (-11%)      98.799% moved   5 FAILs
    kFastKey 0.70 (-22%)     100.000% moved   5 FAILs

There is no band between 0.4% and 0.8% for a real regression to hide in — the
metric goes from 0.02% to 25% with nothing in between. My first probe was a 1%
light change, which the test did NOT catch; that is not a hole, it is 1% of a
~230 pixel landing at the 2-level channel floor. Worth recording because it
briefly looked like a hole.

**The finding that matters most is none of the above: NOTHING RUNS THIS TEST.**
`tide_render_regression` appears in no workflow, and TIDE's root still
force-disables it (`CMakeLists.txt:380`, `FORCE`). No CI on any platform has
ever run it. The cross-platform divergence this row exists to worry about would
be invisible, and the 1%-of-margin warning sat in the backlog until someone
happened to build the other architecture by hand.

**Correction to the row's hypothesis:** it names *"exp/pow"*. The tracer contains
**no `exp` at all** — its 19 transcendentals are cos 7, sin 6, log 4, pow 2.

**Not verified:** anything about Windows or Linux. This box cannot render there,
which is exactly why the CI gap matters.

## 2026-08-23 — macos — S33 is WONTFIX: the stub is inside a comment (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Took S33 — *"`GmpiParameter::setBlob` is a STUB that returns `true`, so every
blob a processor is told to store is silently discarded"* — and it is not true.

**`Hosting/processor_holder.h` lines 71-158 are ONE COMMENT BLOCK**, and both
`struct GmpiParameter` (line 72) and its `setBlob` (line 135) are inside it.
Line 11 of the same file includes `controller_holder.h`, whose `setBlob` is a
correct implementation. Nothing is discarded.

**I did not spot this by reading, and that is the point.** I read the same code
twice, on two different days, and both times saw a live stub. What settled it
was a log line **inside** the commented-out `setBlob`: it never fires, while
blobs reach the processor normally in the same run — **13232 bytes on parameter
1**, TIDE's S12 document-sync XML.

Before that I had already run an A/B: "stub" against "properly implemented",
built and measured. **13216 bytes versus 13227** — no difference, because both
builds compiled the identical comment. I had a working hypothesis, a mechanism,
two call sites and a fix; the only thing missing was the code being real.

**This block has now cost two investigations.** It was filed as S33, and chased
again during M4 as a candidate cause of the blank AUv3 editor — where I wrote
the fix, built it, tested it, and discarded it when the symptom did not move. It
did not move because I had edited a comment.

Marked as dead in [GMPI#14](https://github.com/JeffMcClintock/GMPI/pull/14),
comment-only, TIDE builds against it rc=0. **Not deleted** — 87 lines of
reference code is not mine to remove on my own judgement, and the PR asks
whether it was left deliberately.

**Two process notes worth keeping.** A `git worktree add` succeeded while the
following `git checkout -B` failed on a branch already held by another
worktree, so a heredoc ran in a tree I had not meant to be in; I checked the
main checkout was clean rather than assuming. And `check-backlog-diff.py`
reported row **A33** missing, which looked like damage and was not: A33 had
landed in `main` from another box after my branch was cut. Rebasing fixed it.
Reading the failure before reacting to it was worth the minute.

**Not verified:** whether the dead block is deliberate reference or an accident.
That is Jeff's call.

## 2026-08-23 — windows — S41: nothing ever closed a platform issue, and the option the row favoured would not have helped (interactive, Jeff directing)

**Did:** S41 — the CI mechanism that files a `platform:<p>` issue on a red build
had no other half. Nothing closed one. A break that fixes itself therefore leaves
a live STEP 1 item, and STEP 1 outranks every backlog row on every box.

### The row named three options and (a) does not work

S41 proposed *"(a) have the issue-filing step re-check the branch's latest run
before filing, so a transient that has already gone green files nothing"*, and
said (a) was the one that removes the cost for all three platforms. **It would
have filed #310 anyway**, which is the issue the row was written about:

```
08:02:56Z   run 32561242609  main @38bc3068   FAILURE   <- the run that filed #310
08:04:14Z   #310 filed                                     it WAS the newest run for main
09:08:46Z   run 32564149915  main @9806401b   success   <- 64 minutes later
```

At filing time the failing run is the latest run, and the green one does not
exist yet. There is nothing to re-check. **The row's own Accept describes the
right thing and (a) was a wrong guess at how to get it** — *"no issue **once the
following run is green**"* is a statement about a later run, so the step has to be
on the later run.

### Two more things the issue history says, which the row did not

- **31 issues filed on 2026-08-20 alone**, all `linux`, all closed in a single
  manual sweep the next day. The de-duplication key is branch+platform, so one
  persistently broken dependency files a fresh issue for **every branch anyone
  pushes**. The row frames the cost as "three boxes re-verify one break"; the
  measured worst case is thirty-one issues in a day.
- **#306 is still open, for a branch that no longer exists.** `git ls-remote`
  returns nothing for `tide/mac/M2-ios-configure`. Nothing will ever close it,
  because the only thing that could is a run on a branch that is gone.

### What shipped

One step, `Close the platform issue on success`, on the same matrix job, keyed on
the same title the filing step builds. No new permission — the job already has
`issues: write`.

**Only issues this mechanism could have filed are touched:** exact title match,
the platform label, and `author.is_bot`. Checked against real data rather than
assumed —

```
#310  is_bot=true   app/github-actions     <- auto-filed, may be auto-closed
#306  is_bot=true   app/github-actions     <- same
#314  is_bot=false  tide-rack-bot          <- hand-filed by an agent, never touched
```

`is_bot` rather than a login string because `gh` has changed the form it renders
for an app before, and because it is the property that actually matters.

### Demonstrated on real runs, which is what the Accept asks for

Throwaway branch `tide/_test/s41-close-verification`, a deliberate
`message(FATAL_ERROR)` in the root `CMakeLists.txt`, pushed:

```
00:52:57Z  #322 filed  Build failure on linux — tide/_test/s41-close-verification
00:53:20Z  #323 filed  Build failure on macos — tide/_test/s41-close-verification
```

Break reverted, pushed again:

```
00:55:07Z  #323 CLOSED  reason=COMPLETED   <- macos job, as it finished
00:57:05Z  #322 CLOSED  reason=COMPLETED   <- linux job, ~2 min later
```

**Each platform closed its own issue, independently, while the run was still in
progress** — windows was still building. That is the shape you want: no
cross-platform coupling, and no waiting on the slowest job.

The closing comment carries the evidence and the caveat:

> Green on `linux` as of `fd4ccad6e…`, so closing this automatically (BACKLOG S41).
> **This is evidence about that commit, not proof the reported defect is gone.**
> A race's normal outcome is success, so if this issue described something
> intermittent, reopen it.

**Third control, unplanned and better than the two I designed:** the S41 work
branch's own first run was green on macos with no matching issue, and the step
logged *"No open CI-filed issue for macos + … — nothing to close"* rather than
erroring. A step that only ever runs when there is something to close has never
been tested for the case where there is not.

### What this costs, stated rather than discovered later

The filing step de-duplicates on **open** issues only. A branch that flaps
red/green/red now files a **second** issue where it used to add a comment to the
first. That is the right trade — each red is a real occurrence, and one issue per
occurrence is cheaper than one issue that outlives its cause — but it is a
behaviour change for an intermittently failing branch, and reopen-instead-of-file
was considered and dropped: telling "auto-closed by CI" from "closed deliberately
by a person" needs a marker label to exist, and fighting a human who closed an
issue on purpose is worse than one extra issue.

**Learned:**

- **An option written into a row is a hypothesis, not a plan.** (a) had been sat
  there unexamined since the row was filed; ten minutes of `gh api` on the run
  timeline killed it. The row even contained the timestamps that kill it.
- **Read the Accept clause as the specification and the options as guesses.**
  S41's Accept — "no issue once the following run is green" — describes the
  correct mechanism precisely, and none of its three options implement it.
- **A mechanism with one half is worse than none, because it looks complete.**
  Filing worked from the day it was fixed; nobody asked what closed them, and the
  answer was a person, by hand, in a sweep, once.
- **Test the boring branch.** The "nothing to close" path is the one that runs on
  every green build forever, and it was the path I had not thought to exercise.

**Not verified:** the deleted-branch case (#306's class) is untouched — no run
will ever happen on a branch that is gone, so nothing closes those. That wants a
sweep in `watchdog.yml` and is a different item; **filed as A33**. And windows
never files or closes, by the same `matrix.platform != 'win'` condition the
filing step already carried, which is unchanged.

**Branch/PR:** `tide/win/S41-close-issue-on-green` — TideSynth.

---
## 2026-08-23 — macos — STEP 4: five rows land, and the mac NEXT cell is re-pointed (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Bookkeeping, done by checking rather than by memory. Every PR linked from every
IN-REVIEW row was queried for its state; all five rows had **all** their PRs
merged, so all five flip to DONE:

| row | PRs |
|---|---|
| M1 | GMPI#8, GMPI#13, GMPI_Wrappers#11, TideSynth#318 |
| M2 | GMPI#12, GMPI#13, SynthEditLib#32, TideSynth#308, #312 |
| M4 | GMPI_Wrappers#14 |
| S38 | GMPI#11, GMPI_Wrappers#13, gmpi_ui#11, gmpi_ui#13, TideSynth#281, #309 |
| R10 | GMPI#12 |

Status only — the rows stay in place, which is this file's convention: 36 DONE
rows already live in `BACKLOG.md` rather than being moved out.

**The mac NEXT cell said "take M1", and M1 is now DONE** — the same staleness
the cell's own history complains about, where a box empties its target and the
cell keeps naming it. Re-pointed at **S33**, with the reason spelled out so the
next reader does not have to re-derive it.

**Two open PRs are the Windows box's** (`tide/win/S41-close-issue-on-green`,
`tide/win/issue-314`) and were left alone.

**Not verified:** DONE here means every linked PR merged, which is what the
status means. It does not mean each row's Accept was re-run today; M2 in
particular still carries its own "installed, not launched" caveat in its text.

## 2026-08-23 — macos — the AUv3 editor draws (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Wrote M4's fix and tested it. **The editor renders in GarageBand.**

The AU3 wrapper now creates the plug-in's own `<Controller/>` and calls
`initialize()` on it, as VST3 and CLAP already did. Two details that are not
incidental: `AU3Core` **holds** the controller rather than merely initialising
it — it publishes state through the holder and must outlive
`initWithComponentDescription` — and the call goes **after** the parameter tree
is built, because `initialize()` may set parameters and `notifyDaw` needs the
tree to route them.

**Before:** a correctly-sized 1100x600 window painting nothing.
**After:** category tree, module list and rack rails, drawn.

**The teardown crash went with it, which I did not expect.** Closing the editor
used to produce *"An Audio Unit plug-in reported a problem which might cause the
system to become unstable"* with the appex gone and no crash report. It was
recorded in M4 as a separate, undiagnosed symptom. It is gone: the extension
survives a close, no crash report, and reopening draws again. Same broken
initialisation all along — a half-constructed editor being torn down.

**Verified:** editor draws; close/reopen clean; extension alive afterwards; no
crash reports; `auval` still AU VALIDATION SUCCEEDED. **Negative control:** the
standalone, run with an isolated HOME, is unaffected — still drawing its browser
and MIDI-CV module.

**Not verified:** no host other than GarageBand. **AU2 was not changed or
tested** — it has the same zero `initialize(` count and is *predicted* to share
the defect. Predicted is not measured, and I am not going to write it up as
though it were.

**What this cost, and what actually moved it.** Six suspects died on the way:
the empty rack measuring tiny (mine), the wiring not completing, silence meaning
broken DSP, the timer not being wired (Jeff's, and the best of them), AppKit
refusing to draw, and S33's stub `setBlob` — which looked certain, since the
pointer really does travel as a blob, and which I implemented and tested before
discarding. Two of Jeff's corrections were load-bearing: *"does tide load a
playable patch by default?"* and *"standalone loads the last document
automatically. plugin should not."* Both times I was about to build on something
false, and neither was something measurement alone would have caught, because I
was measuring the wrong thing confidently.

## 2026-08-23 — macos — M4 root cause: the AU3 wrapper never initialises the controller (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

The blank AUv3 editor is one cause, and every symptom follows from it.

**`GMPI_Wrappers/wrapper/AU3` contains ZERO calls to a controller/editor
`initialize(`.** VST3 has six. CLAP has one. **AU2 also has zero**, which
predicts the AU2 editor is blank for the same reason — untestable here, since
S40 dropped AU2.

**The chain, each link measured rather than reasoned:**

1. AU3 never calls it, so `SynthEditController.cpp`'s
   `initialize(gmpi::api::IUnknown* phost, int32_t phandle)` never runs.
   Instrumented both sides: the standalone logs
   `publish seApp: host=0x16ce64400 seApp=0x736cc8658 size=8`; **the AUv3 logs
   no publish line at all.** The pointer is never SENT — it is not lost in
   transit, which is what I had assumed twice.
2. So the editor's `notifyPin(0)` arrives empty — `ctrlPtrSize=0 expect=8`
   against `ctrlPtrSize=8` in the standalone.
3. So `SynthEditGui.cpp:694`'s guard
   `if (pinId == 0 && controllerPtr.value.size() == sizeof(seApp))` fails, and
   the entire GUI-construction block is skipped — host wrappers,
   `onOpenContainerView`, `startTimer(500)`.
4. So no GUI timer clients register. The AUv3 registers only
   `(anonymous namespace)::AU3Core period=15`; the standalone registers
   `SynthEditGui` (500), `TiDEPanelGui` (100) and **three** `gmpi::ui::Form`
   (16). That is the 3-vs-1 asymmetry from yesterday, explained.
5. So `invalidateRect` is called **zero** times against six.
6. So `drawRect` — which IS called, twice, at the full 1100x600, on a view
   genuinely in a window — paints an empty scene.

**S33 was eliminated, and it deserved better than a guess.** The pointer travels
as a blob, and `processor_holder.h`'s `setBlob` is a stub with its store
commented out — so it looked certain. **I implemented `setBlob` properly, built,
and re-ran: still `ctrlPtrSize=0`.** The value is never published, so no
transport fix can help. S33 is a real latent defect and is **not** this one. The
speculative fix was discarded rather than shipped on a hunch.

**What made this tractable was Jeff's two corrections**, neither of which I would
have found alone: *"does tide load a playable patch by default?"* killed the
silence theory, and *"standalone loads the last document automatically, plugin
should not"* killed a control I was leaning on. Both times I was about to build
on something false.

**THE FIX IS NOT WRITTEN AND NOT TESTED.** The obvious shape is for AU3 to call
the controller's `initialize(host, handle)` as VST3 and CLAP do — but nothing was
built or run to confirm it, and the ordering against
`AU3_ViewController::loadView`, which creates the editor BEFORE the audio unit
exists, is unexamined. Saying "the fix is one call" without running it would be
exactly the kind of claim this whole investigation kept disproving.

**Housekeeping:** every diagnostic branch (`tmp/pin-diagnosis`,
`s33/processor-setblob`, and the earlier timer/draw ones) is deleted;
`grep TIDEDIAG` returns nothing in any repo. Dev build removed from
`/Applications`. GarageBand was force-quit at the end while holding only my own
scratch project — Jeff's own project was closed unsaved much earlier and never
reopened for writing.
## 2026-08-23 — windows — #314 reproduced here: 5 failures in 25 builds of plain `main`, then 0 in 40

**Prompt:** 5146a612b · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1. The only open `platform:win` issue is
[#314](https://github.com/JeffMcClintock/TideSynth/issues/314), filed from the
macos box, which said plainly that it could not run any of it and that whoever
took it should *"reproduce locally under `--parallel` ... and verify by
construction, not by exit code."* That is what this run is.

### The issue is evidence, not instruction, so it got reproduced before it got fixed

**It reproduces on the unmodified tree, at a rate nothing in CI would suggest.**
A full Release build of `origin/main` (rc=0, all four Windows artifacts), then 25
iterations of: delete the four binaries and `build/SynthEditSem/Resources`, then
`cmake --build --config Release --parallel`.

```
ITER  8  Error copying file (if different) ... MidiPlayer2/MidiPlayer2.xml -> .../Release/../Resources/MidiPlayer2.xml
ITER 14  Error copying file (if different) ... ControlsXp/ControlsXp.xml   -> .../Release/../Resources/ControlsXp.xml
ITER 19  Error copying file (if different) ... VaFilters/VaFilters.xml     -> .../Release/../Resources/VaFilters.xml
ITER 20  Error copying directory ... TideModules/prefabs -> .../Release/../Resources/Prefabs
ITER 23  Error copying file (if different) ... ControlsXp/ControlsXp.xml   -> .../Release/../Resources/ControlsXp.xml
RESULT: 5 failures out of 25
```

Iteration 20 is the CI failure verbatim, down to the unnormalised
`Release/../Resources` the issue read off the log. **20% here against roughly one
Windows job in twelve in CI** — the mechanism is the same and the exposure is
not, because a local relink puts all four targets in the staging window at once
while a CI build spreads them across a cold compile.

**Four racers, confirmed rather than inferred.** The issue could only infer
`TIDE_Rack_STANDALONE`'s participation from `FORMATS_LIST`. The control build's
log has all four: `Staging rack prefabs (TIDE_Rack)`, `(TIDE_Rack_VST3)`,
`(TIDE_Rack_CLAP)`, `(TIDE_Rack_STANDALONE)`. On Windows `AU3` builds no target,
so it is four and not five.

### The fix, and why it is a shared target rather than a moved directory

`SynthEditSem/CMakeLists.txt` now stages Windows resources **once**, in a
`TIDE_Rack_stage_resources` custom target that every format target
`add_dependencies` on, instead of hanging the identical five copies off four
targets' `POST_BUILD`. `_tide_xmls` was hoisted out of the per-format loop so the
new target and the surviving per-target commands read the same list — the file's
own comment already forbids two copies of it.

**Every other platform is untouched and keeps the per-target commands**, because
there each format target owns its own bundle directory and there is nothing
shared to collide over. The Windows arm sets `_tide_resources` to the empty
string, which is what skips the per-target copies; the arm's comment says so,
because "no resources on Windows" is exactly the wrong reading of that line.

**The A/B, on the real tree, same box, same recipe:**

| tree | parallel builds | failures |
|---|---|---|
| `origin/main` (`2d15e13f3`) | 25 | **5** |
| the same tree + this fix | 40 | **0** |

`0.8^40` is about `1.3e-4`, so the fixed tree is not a lucky run of the broken
one.

**And a synthetic control, because 5-in-25 is a rate and not a mechanism.** Four
executables, one output directory, the identical five copies into one
`$<TARGET_FILE_DIR>/../Resources`, and a 200-file prefabs directory to widen the
copy window: **8 failures in 20** with the copies per-target, **0 in 30** behind
one shared target. Same shape, same error text, no TIDE in it at all.

**The staged output is byte-identical to what `main` produces** — `diff -r`
between the control tree's `Resources` and the fixed tree's is clean: four XMLs
and six prefabs, same place, same content. This changes when the copies run, not
what they write or where.

### Two things I checked because they are how a dedup like this goes wrong

- **Building one format target still stages.** `cmake --build --target
  TIDE_Rack_VST3` on a tree with `Resources` deleted brings the staging target in
  via `add_dependencies` and produces all four XMLs and six prefabs. A
  build-everything-only fix would have looked identical in every measurement
  above.
- **A no-op build costs 0.9 s and rewrites nothing.** A custom target has no
  outputs, so its commands run on every build where a `POST_BUILD` ran only on a
  relink. All five are `*_if_different`, so the staged mtimes are unchanged; the
  cost is five stat-and-skip invocations.

### Not fixed, deliberately

**S36's destination is untouched.** The resources still land one directory above
the binary, so a Windows developer build still has an empty module browser —
that is S36's defect, not this one, and the two are independent. Recorded on that
row: dropping the `/..` would not have helped here, and now that the staging is
de-duplicated, S36's (a) is a one-expression change with no race left attached to
it.

`scripts/package-windows.ps1` reads the same destination and needed no change;
its comment naming `POST_BUILD` as the writer did, and got one.

**The default branch builds on Windows.** `origin/main` at `2d15e13f3`: configure
rc=0, build rc=0, `TIDE-Rack.gmpi` / `.vst3` / `.clap` / `.exe` all emitted. No
new platform issue to file.

**Learned:**

- **A race whose CI rate is about 8% was 20% on a developer's box, and the
  difference is the compile.** CI spreads four link steps across a cold build; a
  local relink puts them in the same 6 ms. Anyone judging "how often does this
  really fire" from the CI history alone would have under-rated it by more than
  twofold.
- **Reproduce in the real tree even when a synthetic repro is easier.** I wrote
  the four-target synthetic first because it was cheap and gave a loud signal. It
  was the real tree that produced the CI error *verbatim*, including the
  `Release/../Resources` spelling — which is what makes this the same bug rather
  than a bug of the same shape.
- **Delete the outputs, not the build tree, to re-run a POST_BUILD race.**
  Twenty-five iterations took 48 seconds because only the four link steps and the
  staging re-ran. A fresh configure per iteration would have made the same
  experiment cost an hour, and I would have run five iterations instead of
  twenty-five.
- **A dedup has a second failure mode the first one hides: building one target.**
  Moving work out of `POST_BUILD` into a shared target is only correct if the
  dependency edges are there, and every measurement of the race passes whether
  they are or not.
- **The platform that differs is not the platform with the bug.** Linux has a
  bespoke arm in this block, with eleven lines of comment about why VST3 needs
  its own destination. I read that, concluded Linux was handled, and never asked
  what the `else()` beside it resolved to. The conspicuous branch drew the
  attention; the unremarkable one shared a directory three ways.
- **"Nothing shared to collide over" is a claim about a generator expression, and
  one command settles it.** `$<TARGET_FILE_DIR>` is the same for every target
  that links into the same directory, which is knowable from a build log I had
  already read on Windows and had not thought to read on Linux.

**Not verified:** the four sibling repos on this box are 2, 3, 3 and 8 commits
behind their `origin/main` and were **not** pulled (they are Jeff's checkouts,
they were clean, and this change is TIDE-only CMake), so CI builds the same file
against slightly newer siblings than I did. macOS was not built at all.

**CORRECTED FOUR HOURS LATER, AND THE CORRECTION IS THE BEST PART OF THIS ENTRY
— see the section below.** This paragraph originally ended *"macOS and Linux were
not built — the change is inside `if(WIN32)` and an `else()` arm those platforms
never reach, but nobody ran them."* The second half of that sentence is false, it
was load-bearing, and CI falsified it before this entry ever merged. It is
rewritten rather than annotated because it has not landed; what it said is quoted
here so the mistake is still in the record.

**Machine left clean.** All work in two throwaway worktrees, `C:/SE/wt314` and
`C:/SE/wt314c`, removed afterwards; nothing was built in `C:/SE/TideSynth`, and
no plug-in was installed. `C:/SE/TideSynth` had one pre-existing dirty file,
`tools/tidepanel-screenshot.synthedit` — a real content diff, not CRLF churn
(`git diff --ignore-all-space` shows it) — so it is Jeff's work in progress and
was left exactly as found. `SE16`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers` and
`GMPI` were clean and on their default branches at the start, were not written
to, and still are.

### CORRECTION — the same race is on Linux, and "Windows only" was my invention

**Four hours after the above, CI failed on a branch whose only change was a
comment in `build.yml`:**

```
00:54:47.5299  Staging rack prefabs (TIDE_Rack_CLAP)
00:54:47.5300  Staging rack prefabs (TIDE_Rack)
00:54:47.5331  Error copying directory ... build/SynthEditSem/Resources/Prefabs
gmake[2]: *** [SynthEditSem/CMakeFiles/TIDE_Rack_CLAP.dir/build.make:657] Error 1
```

**0.07 ms apart, on Linux.** The Linux arm gives the VST3 its own
`.vst3/Contents/Resources` — which is what I looked at — but `GMPI`, `CLAP` and
`STANDALONE` all link into `build/SynthEditSem/`, so
`$<TARGET_FILE_DIR>/Resources` is one directory for the three of them. I read the
branch that differed and generalised from it; the branch that did not differ is
where the bug was.

**So the rule is not "Windows is special". It is "two targets, one directory."**
The block now builds the set of targets that share a destination and stages once
for that set: four on Windows, three on Linux, none on Apple.

**Verified on Linux, on this box, in WSL Ubuntu 24.04** — which also retires the
"do not fix a platform you cannot compile on" objection for this item, because I
can compile on it:

| tree | prefabs in the copy | `--parallel 8` builds | failures |
|---|---|---|---|
| `main` (`d007f34ac`) | 6, as shipped | 20 | **1** |
| + this fix | 6, as shipped | 40 | **0** |
| `main`, window widened | 206 | 25 | **7** |
| + this fix, same widening | 206 | 35 | **0** |

**8 failures in 45 against 0 in 75.** `diff -r` is clean between control and fix
for *both* Linux destinations — the shared one and the VST3 bundle — so the VST3
arm really is untouched. Windows re-measured after the generalisation: build
rc=0, all four artifacts, **0 failures in 30**.

**What made this findable was not diligence.** It was that the failure landed on
an unrelated branch of mine, in a job I only opened because the run was red. Had
I not been looking, the first version of this fix would have merged with the
Linux half of the bug intact and a comment in the file asserting it could not
exist there.

**Next:**

1. **S36 is the natural follow-on and it is now smaller than its row says** — its
   (a) is one expression, the objection that it would not fix the race is spent,
   and it is `any` platform so it does not have to wait for this box.
2. **#314 stays open until the PR merges**, which closes it. A green CI run on
   this branch is not evidence either way, for the reason the issue gives.
3. **The win NEXT cell still points at P11**, untouched by this run.

**Branch/PR:** `tide/win/issue-314` — TideSynth, [#321](https://github.com/JeffMcClintock/TideSynth/pull/321).

---

## 2026-08-23 — macos — the AUv3 view is asked to paint and paints nothing (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

M4 narrowed by one measurement, and a confound of mine found and removed on the
way — by Jeff, not by me.

**Instrumented the draw path** — `drawRect`, `invalidateRect`,
`viewDidMoveToWindow`, view init — in `gmpi_ui/backends/DrawingFrameMac.mm`:

| | standalone | AUv3 |
|---|---|---|
| view init | 1100x626 | 1100x600 |
| in a window | yes | yes |
| `drawRect` | 3 | **2** |
| `invalidateRect` | 6 | **0** |

**`drawRect` IS CALLED in the AUv3**, twice, with the full 1100x600 dirty rect,
on a view that is genuinely in a window. AppKit asks the view to paint; it
paints nothing. And **`invalidateRect` never fires at all** — nothing upstream
ever asks for a repaint.

**THE CONFOUND, AND IT WAS A REAL ONE.** Jeff: *"standalone loads the last
document automatically. plugin should not."* So my comparison was a restored
patch against an empty rack, and the entire 6-vs-0 asymmetry could have been
nothing but content-vs-no-content. I had been treating the standalone as a
clean control and it was not one.

**Removed it by giving the standalone an isolated `HOME`**, so it had no last
document to restore. It still shows the module browser and a **MIDI-CV module**,
and still logs invalidateRect x6, drawRect x3. **So a default TIDE Rack is not
blank**, and the AUv3 showing nothing is a real defect rather than an empty rack
correctly drawn. The finding survives the control that could have killed it,
which is the only reason it is worth anything.

**Eliminated so far:** the view, its size (1100x600, measured), its window, the
wiring (`createNativeView` valid, `initUi` returns, `subviews=1`), the timer
(main thread, main run loop, fires), and now AppKit's willingness to draw.

**Points at:** the editor CONTENT never initialising, or never signalling a
change, inside the extension. Consistent with the earlier timer-client
asymmetry — 3 timers in the standalone, 1 in the AUv3.

**Unconfirmed and not investigated:** the standalone's File > Revert to Plugin
Defaults appeared to change nothing on screen, and Jeff independently said he is
unsure it works. Recorded, not chased.

**Housekeeping:** the draw diagnostics lived on a throwaway `gmpi_ui` branch,
now deleted; `grep TIDEDIAG` is clean. Dev build removed from `/Applications`.
I also broke the backlog lint once here by EDITING M4's Item column in place
after #319 merged, instead of prepending — `check-backlog-diff.py` caught it,
which is exactly its job.

## 2026-08-23 — macos — TIDE in GarageBand: the editor is blank, and four suspects are dead (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Jeff offered to try it in GarageBand, which closed the gap M1 shipped with —
*"the extension was never opened in a real DAW; auval is the evidence, not a
host."* It should have been closed before M1 merged, not after.

**IT LOADS AND IT DOES NOT DRAW.** GarageBand lists it under AU Instruments >
TIDE Synth > TIDE Rack, instantiates it, exposes its parameters in Smart
Controls, and runs it out-of-process with no crash report. Its editor opens a
1100x600 window containing **nothing** but a ~28x26pt fragment top-left.

**Four suspects eliminated, each by measurement, none by reading code.**

1. **"The empty rack measures tiny."** My theory, and wrong. Jeff authorised
   temporary logging; `measure()` returns **1100.0 x 600.0** and
   `preferredContentSize` is set to exactly that. The window really is that big.
2. **"The wiring never completes."** It completes. `connectEditorToUnit`
   early-returns once while `_audioUnit` is still null — by design — is
   re-entered when the unit arrives, and then `createNativeView` returns a
   **valid view at 1100x600** and `initUi` returns with `subviews=1`.
3. **"It is silent, so the DSP is broken."** Jeff heard the metronome and no
   plug-in audio. But `TideApp.cpp:546` creates a blank document — *"creates an
   empty main container"* — so **an empty rack is silent by design.** Jeff asked
   the question that killed this one: *"does tide load a playable patch by
   default?"* It does not. Silence was never the bug; the bug is that you cannot
   build a patch in an editor that will not draw.
4. **"The timer is not wired up."** Jeff's hypothesis, and the best of the four
   because it is where the drawing actually comes from. Instrumented: in the
   AUv3 `Timer::start` runs on the **main thread**, with
   `CFRunLoopGetCurrent() == CFRunLoopGetMain()`, and the callback **fires
   repeatedly**. The timer is fine.

**NEGATIVE CONTROL, and it is what makes all of this a real bug rather than a
misconfiguration: the STANDALONE renders perfectly** — same build, same engine,
full module browser and rack.

**THE LIVE LEAD came out of eliminating suspect 4, not out of confirming it.**
The standalone starts **THREE** timers; the AUv3 starts **ONE**. Two
timer-driven subsystems never start inside the extension.
`SynthEditLib/ModulePicker.h` declares two `gmpi::TimerClient` subclasses — the
module browser, which the standalone shows and the AUv3 does not. That is where
I would look next.

**Filed as M4.** All diagnostic logging lived on throwaway branches in `gmpi_ui`
and `GMPI_Wrappers`, both now deleted; `grep TIDEDIAG` is clean in every repo.
The dev build was removed from `/Applications`, and both GarageBand projects
touched — Jeff's own "Test Preset Change" and my scratch "Untitled" — were
closed **without saving**.

**Also seen and NOT diagnosed:** closing the editor made GarageBand report *"An
Audio Unit plug-in reported a problem which might cause the system to become
unstable"*, with the appex process gone and **no crash report written**. The
teardown path (`dealloc` -> `gmpi_onCloseNativeView`) is the obvious suspect and
is unmeasured. It is in M4's row so it is not lost.

**Not verified:** why the view does not paint. Everything upstream of the draw
is now eliminated, which is progress, but the draw path itself was never
instrumented.

## 2026-08-23 — macos — M1 closes, and it takes two of my own claims down with it (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

M1 was never blocked on work — its row says so: *"BLOCKED ON A RULING, NOT ON
WORK"*. AU2 and AU3 share fourCCs, so `AU3` stayed out of `FORMATS_LIST` until
Jeff chose. **He chose: S40 dropped AU2**, so the clash is gone by construction
and the row only needed evidence.

**The evidence, from plain `main`:** `AU3` is in `FORMATS_LIST`, the build is
rc=0 with zero errors and produces all five artifacts, the extension registers,
and `auval -v aumu Drck Dsyh` reports *"version 3 implementation"*, *"Loaded
AudioUnit out-of-process: true"*, **AU VALIDATION SUCCEEDED**, rc=0.

**Then two things in the row turned out to be wrong, and both were mine.**

**The identifier it cites is stale.** `pluginkit -m -i
com.gmpi.au3.TIDE_Rack.extension -v` returns "(no matches)" — GMPI#13 made the
appex id derive from the containing app, so it is now
`com.tidesynth.tiderack.au3app.extension`. Anyone following the row's own
command would conclude registration is broken. That is precisely the false
negative this row already records me making once, preserved in amber and waiting
for the next person.

**"macOS registers the AUv3 automatically, with no launch required" is wrong.**
I wrote that during S40, in `SynthEditSem/CMakeLists.txt` and — much worse — in
user-facing `docs/distribution.md`. Re-measured today, deliberately, because the
first `pluginkit` query came back empty and I wanted to know whether that was
the stale id or a real gap:

  - `~/Applications`, 30s after the copy: **(no matches)**
  - `/Applications`, at 5s, 15s and 30s: **(no matches)**
  - within **12 seconds of `open`ing the app**: registered, and reporting
    version 0.1.1, which is R10's wiring showing up in a third place

So the install story is a copy **plus a first launch**. A user who follows the
docs as they stood installs the pkg, opens their DAW, and finds nothing. The pkg
puts the app in place and cannot open it for them, so whatever tells the user how
to install this has to tell them to open it once. Both places corrected.

**What made the difference was not being satisfied by the first "(no matches)".**
The tempting read was "stale id, mystery solved" — the id WAS stale, and fixing
it alone would have left the doc bug shipping.

**Not verified:** the extension was never opened in a real DAW; `auval` is the
evidence, not a host. Nothing was tested on a clean machine, so whether the first
launch is needed once per install or once per user is unmeasured. The dev build
was removed from `/Applications` and `~/Applications` afterwards, so this box is
back to where it started.

## 2026-08-23 — macos — I broke the Linux release with the class I spent all day fixing (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

v0.1.1 failed. Windows succeeded, macOS failed at notarization (Apple, below),
and **`build-linux` failed because of me**:

    ##[error]missing or empty asset: TIDE-Rack-Linux.tar.gz

R10 renamed the tarball to `TIDE-Rack-Linux-0.1.1.tar.gz` while `release.yml`'s
asset check still expected the old name. **Two places that must agree and I
changed one** — the exact class I fixed four times today: the AU2 class name
beside its string, the MIDI menu beside its handler, the appex name beside its
executable, the XML chooser beside its consumer. I wrote the commit messages for
all four and then introduced a fresh instance.

**And the rename was wrong on its own terms, which one line of the file I was
editing would have told me.** `release.yml`'s header says: *"Asset names carry NO
version (docs/distribution.md): the version lives in the tag, so
`releases/latest/download/TIDE-Rack-macOS.pkg` is a permalink R6 can promise
never to change."* Two identically-named files across releases is the POINT.
I had justified the rename in a comment as fixing a problem that was a
deliberate design decision, stated in the file, which I had not read.

Reverted the name. The rest of R10 stands and is what actually matters: the
version lives in the tag and INSIDE the artifacts — `<Plugin version="0.1.1">`
drives every bundle, `--version "$VERSION"` drives the pkg metadata — not in the
filenames.

**The notarization retry worked exactly as designed and did not help.** The log
shows `notarytool submit (attempt 4 of 4)` and then *"Apple's notary service
still failing after 4 attempts"* — so the code from #311 ran, correctly
identified four consecutive 500s as transient, and backed off through all of
them. **Apple returned HTTP 500 at 07:22, 10:40 and 20:58 UTC**, which is not a
blip; it is a service that has been failing all day. The retry covers about
fifteen minutes. Nothing in TIDE can fix that, and the right response is to wait
rather than burn another build.

**Verified:** the Linux tarball name matches `release.yml`'s expectation again
(`TIDE-Rack-Linux.tar.gz`), and the version is still carried inside the
artifacts. **Not verified:** the Linux packager was not executed — this is a
name match by inspection, and the proof is the next release run.

## 2026-08-23 — macos — "bump to 0.1.1" was not a one-number change (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Jeff asked to increment the version. Before touching anything I looked for where
the version lives, and found **four consumers giving four different answers,
none of them the tag**:

  - the BUNDLES said `1.0.0` — GMPI's documented default for a `<Plugin>` tag
    with no `version` attribute, and TIDE's carried none;
  - `package-macos.sh` fell back to a hardcoded `0.1.0`;
  - `package-windows.ps1` read `$env:TIDE_RACK_VERSION`, which **nothing ever
    set**, so it got an empty string;
  - `package-linux.sh` put no version in the tarball name at all, so two
    releases would have produced two identically-named files.

`release.yml` mentioned `TIDE_RACK_VERSION` **zero times**. Tagging `v0.1.1`
would have shipped artifacts labelled 1.0.0, 0.1.0, nothing, and nothing.

**Fixed by giving it one source.** `SynthEdit.cpp`'s `<Plugin>` tag now carries
`version="0.1.1"`, which every bundle derives from; `release.yml` derives
`TIDE_RACK_VERSION` from the tag; the two shell packagers' fallbacks match it.

**The release.yml half is a shell step, not a job-level `env:`, and I got that
wrong first.** The leading `v` has to come off, and I wrote
`substring(github.ref_name, 1)` — **GitHub's expression syntax has no substring
function.** The YAML parsed happily, because it is just a string until Actions
evaluates it; nothing would have complained until a tagged release ran. Replaced
with `${GITHUB_REF_NAME#v}` in a `shell: bash` step, so the one script also runs
on the Windows runner.

**Verified by building:** all four macOS bundles — `.vst3`, `.clap`, `.gmpi` and
the AUv3 app — report **0.1.1** where the same build previously reported
`1.0.0`. The tag-strip was exercised directly (`v0.1.1` → `0.1.1`).

**And the first build after the change still said 1.0.0**, which was worth the
five minutes it cost. The local GMPI checkout was **14 commits stale**, so
`gmpi_find_plugin_element` still preferred a MODULE source for the plugin XML
and never saw the new attribute. GMPI#12's `<PluginList>` preference is what
makes this work, and the stale build was an accidental demonstration of the very
bug that PR fixed. `git pull` in the local checkouts, rebuild, 0.1.1 everywhere.
[[cmake-local-repo-overrides]] warns about exactly this and I still hit it.

**Not verified:** no packager was executed — they need signing identities and a
release environment — so the version reaching the installer FILENAMES is by
inspection, not by run. Windows and Linux packaging were not exercised at all.

## 2026-08-23 — macos — TIDE Rack installs on iOS. The blocker was a folder (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Two fixes, measured after each as Jeff asked. `simctl install` now returns **0**
on the unmodified build output.

**FIRST, A CORRECTION I OWE THE RECORD.** I told Jeff this needed "a GMPI-wide
identity decision", then corrected that to "TIDE's own file, ALLOWED". Both were
wrong. The appex's `CFBundleIdentifier` is written by `plist_util` from a value
baked into its COMMAND LINE at GMPI configure time, so no TIDE target property
could ever have changed it. Reading `plist_util.cpp`'s usage string settled in
one command what I had twice asserted from memory.

**Fix 1 — the extension id (GMPI).** An extension's id must be PREFIXED by its
containing app's. GMPI's comment already said so, and its code then wrote both
halves from one string a plugin may replace. TIDE replaces it: S40 renamed the
containing app to `com.tidesynth.tiderack.au3app` — **and renamed only the app.**
The extension kept `com.gmpi.au3.TIDE_Rack.extension`, the two shared no prefix,
and macOS never complained because macOS does not enforce the rule. Now DERIVED
from the app target as a generator expression. **Measured:** the extension came
out `com.tidesynth.tiderack.au3app.extension` with TIDE setting nothing.
Install still failed, which is why there was a second fix.

**Fix 2 — and it was not the plist at all.** I had assumed missing iOS plist
keys. Only bisection showed otherwise. Eliminated in turn, each by experiment:
`MinimumOSVersion`, `LSRequiresIPhoneOS`, `CFBundleSupportedPlatforms`,
`DTPlatformName`, `UIDeviceFamily` — added all five by hand, still rc=2. Version
match with the container — already matched. Malformed plist — read the whole
file, well-formed. `NSExtension*` keys — all correct.

**Then: deleting `Resources/` from the appex made it install.** But removing
only the XMLs did not, and removing only `Prefabs` did not. **It was the
subdirectory itself.** An iOS bundle is FLAT — resources belong at the bundle
ROOT — and TIDE staged them into `Resources/`, the macOS layout, because the
destination was `$<TARGET_BUNDLE_CONTENT_DIR>/Resources` on all Apple platforms.
Moving the contents to the root: **rc=0**.

That is the same root cause as the codesign defect earlier today, where a flat
bundle takes its executable name from the bundle name. **Third time iOS's flat
layout has broken something written against macOS**, and the error named it in
none of the three cases — this one said *"Missing bundle ID"*, about a bundle
whose id was present and correct.

**A process note worth keeping.** I pushed the GMPI half onto GMPI#12's branch
after Jeff had already merged it, leaving an orphaned commit — the one end state
STEP 5 forbids. Caught by checking, moved to a fresh branch off `main`, stale
branch deleted. The lesson is STEP 4's, and it is cheap: **check the PR is still
open before pushing a follow-up**, because on an actively-merging day it may not
be.

**Verified.** iOS: clean build rc=0, `simctl install` **rc=0** on the build
output with nothing edited by hand, app registered as
`com.tidesynth.tiderack.au3app`. macOS negative control: rc=0, all five formats,
and the appex still uses `Contents/Resources` — the flat layout is iOS-only.

**Not verified.** The app was installed, **not launched**, and the Audio Unit was
never opened in an iOS host — so nothing here says TIDE RUNS on iOS, only that it
installs. Device (non-simulator) builds were never configured. The `__APPLE__`
population in SynthEditLib is still 31 files against 2 using `TARGET_OS_*`.

## 2026-08-23 — macos — three red signals, three different mechanisms, and none of them a defect in this repo

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.34493.1** (agent 2.1.237) · as **tide-rack-bot** (both paths)

**Did:** STEP 1 and STEP 1.5 only — no backlog item. Two `platform:mac` issues and
one red check on this platform's own PR, and the interesting result is that the
three have **three unrelated causes and zero overlap**, where the obvious reading
was one bad merge.

| signal | cause | where it lives |
|---|---|---|
| [#310](https://github.com/JeffMcClintock/TideSynth/issues/310) `main` red on macOS | a **26-second** window between two sibling repos' merges | nobody's commit — **S41** |
| [#307](https://github.com/JeffMcClintock/TideSynth/issues/307) red on a branch | 14 build-tree **gitlinks** committed by accident | fixed before I looked |
| [#313](https://github.com/JeffMcClintock/TideSynth/pull/313) `windows` FAILURE | a **parallel-build race** in the Windows resource staging | real, latent, **#314** |

### #310 — the failure was 26 seconds wide, and CI fell in it

```
build/_deps/gmpi_wrappers-src/wrapper/AU2/AU2_Wrapper.cpp:8:10:
  fatal error: 'backends/GmpiObjCNames.h' file not found
```

S38 shipped as two PRs in two repos. TIDE fetches both by `GIT_TAG origin/main`:

```
08:03:41Z   GMPI_Wrappers#13 merged   (adds the #include)
08:04:07Z   gmpi_ui 6aa8871 merged    (adds the header)      <- 26 s later
```

and the failing run's own configure log straddles the gap:

```
08:03:42.10Z   Fetching gmpi_ui from github     <- 25 s BEFORE the header existed
08:03:59.35Z   dep GMPI_Wrappers  [fetched]     <- 18 s AFTER the #include did
```

The build got the consumer without the provider. **Nothing in TideSynth was
wrong, and nothing needed fixing** — `9806401b` (09:08) and `318d2748` (09:41)
went green on macOS with no change in between.

**Re-verified here rather than inferred from those green runs**, because the
current head `736e75b8` had no macOS verdict at all (its run was cancelled):
clean scratch worktree, no overrides, the exact CI recipe — configure rc=0,
**build rc=0, 0 errors**, all five formats emitted, `lipo -archs` = `arm64`.
Closed on that.

**The part worth keeping is not the header.** X4 (WONTFIX, *"do not re-file"*)
reasons that *"CI clones fresh, so `origin/main` there is real"*. That is true
**per repo** and false **across** repos: CI resolves each sibling at a different
instant, so there is no snapshot consistent between them, and any two-repo change
merged non-atomically breaks every platform for the width of the interval. Filed
as **S41**, proposing no change to the pins — the fix worth having is in the
issue-filing step, not in the fetching.

**And the real cost is not the red build**, which nobody was waiting on. It is
that `build.yml` files a `platform:*` issue, that issue is STEP 1 work outranking
all backlog items, and three boxes then spend a session each re-verifying a break
that already fixed itself. This run is that cost, paid.

### #307 — the error names `.gitmodules` and means "a build tree got committed"

Not a compiler failure at all; the job died in `actions/checkout`:

```
fatal: No url found for submodule path 'build-ios/_deps/au2-src' in .gitmodules
```

`git ls-tree -r 1e2956b4 | awk '$2=="commit"'` → **14 gitlinks**: the fetched
dependency checkouts of `build-ios/` *and* `build-mac/`, staged as submodules
with no `.gitmodules` to resolve them. That breaks checkout on every platform,
which is why one run produced both this and #306 (linux).

Already resolved, and I checked each half rather than assuming: the merged result
`38bc306` has **0** gitlinks, `main` has no `.gitmodules` and no `build*` entry,
the branch is deleted — **and `38bc306` added `build-*/` to `.gitignore`**, with a
comment saying plain `build/` did not catch a per-platform tree and one "was very
nearly committed because of it". Not *nearly*: on that branch it was. Closed.
Commented on #306 with the same evidence and left it for the linux box, since
closing another platform's issue is not mine.

### #313 — the windows check, and the log proves the mechanism unaided

STEP 1.5 made this mine. The PR's own change is **iOS-only** (`if(CMAKE_SYSTEM_NAME
STREQUAL "iOS")`), so it cannot reach a Windows build — and the log says what did:

```
10:11:07.3390324  TIDE_Rack.vcxproj      -> build\SynthEditSem\Release\TIDE-Rack.gmpi
10:11:07.3593501  TIDE_Rack_VST3.vcxproj -> build\SynthEditSem\Release\TIDE-Rack.vst3
10:11:07.3934793  TIDE_Rack_CLAP.vcxproj -> build\SynthEditSem\Release\TIDE-Rack.clap
10:11:07.3983502  Staging rack prefabs (TIDE_Rack)
10:11:07.3985881  Staging rack prefabs (TIDE_Rack_VST3)
10:11:07.4048826  Staging rack prefabs (TIDE_Rack_CLAP)
10:11:07.5854032  Error copying directory ... Release/../Resources/Prefabs: Permission denied
```

**Three targets started staging the same directory 6.5 ms apart**, and one lost
180 ms later. On Windows every format target links into one directory — the three
`->` lines are the proof — so `$<TARGET_FILE_DIR:...>/../Resources` is a single
shared destination and all four targets run the identical five POST_BUILD copies
into it under `--parallel`. **N1a's class exactly**, which fixed `PDB_NAME` and
`ARCHIVE_OUTPUT_NAME` for the same underlying reason and never touched the
resource destination.

**Not fixed here.** STEP 3 forbids fixing a platform I cannot compile on, and
this one deserves the rule even more than usual — see the lesson below. Filed as
[#314](https://github.com/JeffMcClintock/TideSynth/issues/314) with the mechanism
and the frequency, and recorded on **S36**, which already owns this arm and whose
open (a)/(b) choice it settles: **(a) does not remove the race** (dropping the
`/..` still leaves one destination shared by four targets), **(b) does**.

**Re-ran the job on the identical commit and it passed.** #313 is now fully green
— `lint`, `guard`, `linux`, `macos`, `windows` — and nothing about it was
changed, which is the whole point.

**Learned:**

- **A race cannot be verified fixed by watching CI go green.** The 11 Windows
  jobs before this one all passed, and the same commit passed on re-run. Success
  is what the *broken* code produces most of the time, so a green run after a fix
  carries almost no information. This is the strongest argument I have met for
  STEP 3's "do not fix a platform you cannot compile on" — the usual objection,
  *"but CI is my instrument"*, is exactly what fails here.
- **`origin/main` is not a snapshot when you fetch more than one repo.** Two
  correctly-ordered merges 26 seconds apart broke every platform, because the
  consumer and the provider were resolved 17 seconds apart. The journal already
  says a two-PR fix has an order; what it lacked is that the **interval** is also
  load-bearing, and nothing in CI makes it atomic.
- **A self-healing break still costs three sessions.** The transient was over in
  under a minute and cost nothing to the build, but the issue it filed is STEP 1
  work, and STEP 1 outranks everything. The expensive part of a false alarm is
  the priority it inherits, not the alarm.
- **Three red signals on one platform in one hour is not evidence of one cause.**
  I opened this expecting the M2 merge to be behind all of them. They shared a
  branch name and nothing else — a merge-window transient, an accidental `git add`,
  and a latent race that had been there since CLAP joined `FORMATS_LIST`.
- **Read the first error, not the loudest.** The Windows failure printed ~45
  `MSB3073` lines; every one was cascade. The single `Error copying directory ...
  Permission denied` above them carried the whole diagnosis, including the
  unnormalised `Release/../Resources` that names the CMake expression verbatim.
- **A checkout error that names `.gitmodules` means someone committed a
  directory.** Nothing in the message resembles the actual mistake, and it fails
  identically on all three platforms — so it files a platform issue per box for
  what is one staging slip. The prompt's *"never `git add -A`"* is the rule; this
  is what breaking it looks like from the outside.

**Next:**

1. **M1 is closeable on evidence** and the mac NEXT cell now says so. Its only
   blocker was the AU2/AU3 fourCC clash awaiting a ruling; **S40 is DONE** — AUv3
   only, AU2 dropped — so the clash is gone by construction, and its Accept was
   demonstrated on plain `main` by this run's build.
2. **M2 is IN-REVIEW, not takeable** — #313 is open and green. A later run flips
   it once merged. Its **"Not verified"** list is the real remaining mac-only
   work: the iOS app was installed but **never launched**, the Audio Unit has
   never been opened in an iOS host, and device builds were never configured.
3. **#314 wants the Windows box**, and wants it to reproduce locally under
   `--parallel` rather than trusting a green run.
4. **S41 needs Jeff** — the fix lives in `.github/workflows/build.yml`, which the
   fleet token deliberately cannot touch.

**Machine left clean.** All work in a scratch worktree under the session
scratchpad, removed afterwards; Jeff's `~/Documents/GitHub/TideSynth/build` was
never written to, and no plug-in was installed into `~/Library/Audio/Plug-Ins`.
All six repos (`TideSynth`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`,
`SynthEdit`) were clean and on their default branches at the start, were not
touched during, and are back on them at the end. This item is two files in
TideSynth.

**Branch/PR:** `tide/mac/issue-310` — TideSynth, backlog and journal only. No
product code change. Issues: #310 and #307 closed, #314 filed, #306 commented.

---
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
### Correction: Ardour IS a host here, and it settles the question

**Jeff asked "don't we have Ardour host?" — yes, and that makes three separate
claims of mine wrong.** I wrote in the row, both PR bodies and the issue that
closing this needed REAPER on a win/mac box. **Ardour 8.4 is installed on this
box**, `ardour-vst3-scanner` answers precisely this question, and **my own memory
note from 2026-08-19 records using it**, including the
`LD_LIBRARY_PATH=/usr/lib/ardour8` quirk it needs.

```
BROKEN (main):  VST3 not a valid bundle:
                  '.../TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE_Rack_VST3.so'
FIXED  (both):  [Info]: Found Plugin: TIDE Rack
                  uid=506C7567696E474D504920501951ED43 category="Instrument|Synth"
                  n_outputs=2 n_midi_inputs=1
```

Ardour derives the payload name from the bundle name — exactly the rule GMPI's
own comment states — so **the Linux VST3 is unloadable today, not merely oddly
named**, and the fix is host-verified on the platform that has the bug. The
scanned UID also matches the one in all five `.rpp` fixtures.

**The lesson is not "use Ardour".** It is that I asserted an environment limit
three times without testing it, while holding a note that contradicted it.
"Not verifiable here" is a claim about the machine, and it deserves one command
before it goes into a row, two PR bodies and an issue.

Ardour's cache entry from the scan pointed into a scratch tree and was removed;
Jeff's other nine cached plugins were left alone.


**Learned:** anything the next run would otherwise rediscover the hard way.

0. **"Not verifiable on this box" is a measurable claim, and I shipped it three
   times unmeasured.** Ardour was installed the whole time and my own memory note
   named the command. Check the machine before writing a limit into a row.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---
