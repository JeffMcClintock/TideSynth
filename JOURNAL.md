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

## 2026-08-18 — macos — E2a: the three rack prefabs exist, ship, place and cable (interactive session, Jeff directing)

**Did:** Built BACKLOG **E2a** — the oscillator, envelope and output rack
prefabs — plus module-enumeration **stage 4** that ships them. Took the
STANDALONE option the prompt raised, and it paid for itself several times over.

**The STANDALONE decision, and why the stated risk turned out not to exist.**
Added `STANDALONE` to `SynthEditSem/CMakeLists.txt`'s `FORMATS_LIST`. The
concern was that it puts a local IPC endpoint in the product. It does not, and
that is measured rather than argued: `Standalone_Wrapper` is linked PRIVATE into
the `_STANDALONE` executable only (`GMPI/gmpi_plugin.cmake:373`), and `nm` on
the Release binaries counts **25** IpcServer/CommandDispatcher symbols in
`TIDE_STANDALONE` against **0** in both `TIDE_VST3` and `TIDE.gmpi`. Nothing
copies the app to a Plug-Ins folder either. **The one footgun, written into the
CMake rather than left implicit:** `GMPI_STANDALONE_COMMAND_CHANNEL` defaults
**ON**, so if TIDE ever ships a standalone that release must configure
`-DGMPI_STANDALONE_COMMAND_CHANNEL=OFF`, which removes the code rather than
merely declining to start it.

**It made the rack scriptable, which is the whole reason E2a got as far as it
did:** screenshot, click, drag and render-audio over a unix socket, driving the
real editor. Every visual claim below was verified that way.

**Three real bugs surfaced on the way in, each of which blocked the next step:**

1. **The standalone never instantiated the plugin's Controller subtype.** It
   created only `Audio` and `Editor`; TIDE's entire UI hangs off its controller,
   so TIDE came up as a menu bar, a breadcrumb strip and an empty black canvas —
   no document, no browser, no rack. `TideApp::InitInstance` was never running.
   The VST3 wrapper has always done this
   (`wrapper/VST3/Controller_VST3.cpp:347`); the standalone simply did not.
   Fixed in **GMPI_Wrappers**, its own PR.
2. **TIDE answered a zero-size `measure()` probe with zero.** The standalone
   probes at `{0,0}` to ask "what size do you want?", read the zero as "no
   opinion", and opened a 400x400 window. Below 720 DIPs `recomputeStrips` sets
   `showSidePanes = false`, so **both** the module browser and properties pane
   vanish — which is what made TIDE look like it had no module browser at all.
   400x426 -> 1100x626 with the fix.
3. **POST_BUILD ordering shipped a correct build tree and a wrong plugin.**
   `gmpi_plugin`'s `copy_plugin` copies the bundle to `~/Library/Audio/Plug-Ins`
   as an *earlier* POST_BUILD step than the resource staging added here, so the
   installed VST3 had `ControlsXp.xml` and no `Prefabs`. Invisible until you
   wonder why the standalone lists three prefabs and REAPER lists none.

**The prefabs are generated, not hand-written.** `TideModules/build-prefabs.py`
drives SynthEditCL for the graph (handles, `<lines>`, the `IO Mod`s
`--containerise` synthesises — the half a human gets wrong silently) and does
the panel layout itself, because the CLI has no verb that moves a module.
`TideModules/prefabs/*.synthedit` is its output.

**Two facts that each cost a debugging cycle, now encoded in the generator:**

- **`PanelWndPosition` is what the rack draws** for a Container
  (`CContainer::getViewObRect`, `CContainer.cpp:3332`) — *not* `panel_rect`.
  SynthEditCL saves it as `0,0,0,0`, so the first prefabs dropped into the rack
  **successfully**, reported the right size in the properties pane, and drew
  nothing. Compare `Controls/LED2.syntheditprefab`, which carries a real one.
- **Every module in a prefab must be a class TIDE actually LINKS.** In a saved
  document that is `class="1"`/`class="2"`; an XML-only entry has **no `class`
  attribute at all**. One such module takes the container's *whole* widget layer
  down — a blank rack, not a partial failure. `assert_all_modules_linked()` now
  fails the build on it, with a negative control proving the check fires.
  **A `strings`/`nm` check is a FALSE POSITIVE here** and cost this run an hour:
  `"SE Rectangle XP"` is in TIDE's binary via the legacy rename table at
  `CUG.cpp:301` while having no registration whatsoever.

**The faceplate needs BOTH halves — corrected in-session after Jeff caught it.**
This entry first said the `Sine.seprefab` faceplate idiom
([docs/e2a-prefabs.md](docs/e2a-prefabs.md) §1) was *impossible* in TIDE. Wrong.
The rule, which is the general one for TIDE's fixed module set (constraint 7):
**a module needs its `.cpp` in `SynthEditSem/CMakeLists.txt`'s source list AND
its pin descriptions merged from XML in `TideApp::InitInstance`** — TIDE has no
module scan to supply the latter (S1a). Either alone fails, and differently:
XML-only is an insertable phantom with no class behind it; `.cpp`-only is a
class with no pins, which takes the whole enclosing container's widget layer
down with it — a blank rack, not one missing module. `SE Rectangle XP` had
*neither*. Adding `modules/SubControlsXp/RectangleGui.cpp` **and** staging
`SubControlsXp.xml` makes it real: a **Sub-Controls** category appears in TIDE's
browser and the rectangle draws on the rack as a proper module body. The merge
stays safe because its `GetById()` guard is enrichment-only, so an XML listing
far more modules than TIDE links adds no phantoms.

**How not to test it, since both of my first two methods were wrong:**
`strings`/`nm` on the binary is a false positive — `"SE Rectangle XP"` is there
via the legacy rename table at `CUG.cpp:301` with no registration. The `class=`
attribute in a saved document is better but reflects **SynthEditCL's**
registration, not TIDE's. The authoritative check is placing the prefab in TIDE
and looking at it.

**The shipped prefabs are still jacks-only**, deliberately: the rectangle
covered the jacks on the first attempt and document order did not obviously
control z-order, and a caption still wants a module (`SE Text Entry` is linked
but is a patch-memory text field, pin 0 `patchValue`, not a plain label). Rack
styling as a whole stays **E5**, Jeff's call to set.

**The Envelope is an envelope AND a VCA**, deliberately. A bare ADSR emits
control voltage and has no audio path, so oscillator -> envelope -> output would
render silence however correct each part was. Its Gate jack defaults open so the
minimal three-module rack — which has nothing to patch a gate from — still
sounds.

**Result — what is verified, all of it live in `TIDE_STANDALONE` with the
`~/SynthEdit Projects/Prefabs` copies DELETED, so the bundle path is what was
exercised (the false-pass trap docs/e2a-prefabs.md §5.3 names):**

- `TIDE: 3 rack prefab(s) seeded from the bundle` at startup, with the scan
  still absent — S1a's design intact.
- All three appear under the browser's **Prefabs** group.
- Each **places** on the rack as a Visible container with its jacks drawn.
- Jacks **cable** to each other with real mouse drags: oscillator -> envelope ->
  output, wired and rendered on screen.
- The **installed** VST3 at `~/Library/Audio/Plug-Ins/VST3` carries
  `Resources/Prefabs/`, so this is what REAPER will load.

**So E2a's unaudited question is ANSWERED: placing and cabling a CLOSED prefab
in the rack works today and needs no U1 work.** U1/U1a/U1b/U1c had already
landed; the rack-mode placing surface they left behind is sufficient. What it
needed was correct prefab *data*, not more UX.

**NOT DONE, and the reason is specific: the audio has not been measured.**
`gmpi_render_audio` returns silence for TIDE and **that is an artifact of the
tool, not of the rack**. It primes a fresh processor from current parameter
values and skips every non-scalar parameter —
`if (!is_scalar(param.info->datatype)) continue;`
(`GMPI_Wrappers/wrapper/Standalone/mcp/CommandDispatcher.cpp:858`). **TIDE's
entire patch is a blob parameter** (S12's chunk), so the offline instance is
built with an empty graph and is guaranteed to be silent whatever is on screen.
The tell is `parametersPrimed: 0` in the result, with TIDE's two parameters both
blobs. Fixing it needs a non-scalar setter in GMPI's `processor_holder` (only
`setParameterNormalizedFromDaw` exists) — a third repo, so it was filed rather
than grabbed at the end of a session.

**The V1 measurement therefore still wants the REAPER route the prompt
described** (`scripts/render-and-measure.py` against a saved `.rpp`), which
exercises the product path and does not depend on the standalone's offline
render at all. That is the next step and it is now unblocked in every other
respect: the prefabs exist, ship, place and cable.

**Also not done:** the per-prefab E1 harness cases. The generator's
`assert_all_modules_linked` check is a real regression guard and is green, but
it is a build-time invariant, not an E1 audio case.

**Two smaller traps worth keeping:**
- Dragging a second cable *from a jack that already has one* grabs the existing
  cable rather than making a new one, and it is easy to end up with a cable
  between one module's own two jacks. Cable each jack once.
- Two Output prefabs in a rack means two `Sound Out`s competing for the host's
  buffers. Keep one.

---

## 2026-08-18 — macos — S13 verified by A/B, and a wrong assumption corrected

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** measured S13's Accept instead of leaving it open, after Jeff asked
whether I wanted to try the repro. [SynthEditLib#19](https://github.com/JeffMcClintock/SynthEditLib/pull/19)
is merged and S13 is DONE and archived.

**Learned, and this is the entry's real content — I wrongly believed I could
not test in REAPER.** Two entries today, and the S13 PR body, all state that a
scheduled run cannot verify this because computer-use is refused. **That
conflates two different things.** Computer-use is refused, and it was never
needed: **launching a binary and reading its stderr is a Bash operation.** The
mac NEXT row has said so since this morning — *"launch the DAW from a shell
(`/Applications/REAPER.app/Contents/MacOS/REAPER project.rpp`), not `open -a`
— an uncaught C++ exception then prints its own type and message to stderr"* —
and I quoted that note into a handoff prompt for an interactive session
**while still believing I couldn't use it myself.** The prompt I wrote was the
evidence that I could.

**What that cost, stated plainly:** S13 shipped with its Accept unmet and a row
left IN-REVIEW, an issue (#117) closed on someone else's runtime evidence, and
a handoff prompt asking Jeff to do a check that took me two commands. The
generalisation worth keeping: **"I have no GUI" is not the same as "I cannot
run the program".** Before declaring something unverifiable, ask which of the
two it actually needs — driving a UI, or observing a process.

**Result — the A/B, same project, same Debug config, same machine, only the fix
differing.** `/tmp/tide-s11-final.RPP`, REAPER launched from a shell with a
40-second watchdog:

```
BEFORE (SynthEditLib main, no fix)
  RESULT: exited after ~6s with code 134
  Assertion failed: (false), function RegisterExternalPluginsXmlOnce,
                    file UgDatabase.cpp, line 549.

AFTER (fix branch, Debug TIDE_VST3 rebuilt)
  RESULT: STILL RUNNING after 40s — no abort, project loaded
  SYNTHEDIT PROCESSOR: Intel
  BLOCK SIZE 128, DRIVER BUFFER 512 (4 per buffer, EXACT)
  audioMasterState::Starting
  audioMasterState::Running
  grep -c "Assertion failed" -> 0
```

The process was alive at 40s and killed by the harness, not by an abort. **The
Debug build is usable for debugging again**, which was the row's whole point:
every S11-era investigation had to work from `.ips` reports because this assert
killed the only build with symbols.

**Trap found while setting this up, and it would have wasted someone's
session:** a post-build step copies the built bundle to
`~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3`, so **whichever configuration
you build last is the one REAPER loads.** Build Release after Debug and your
Debug test silently measures the Release plugin — where this assert compiles
out, so it "passes" for the wrong reason. That is exactly the shape of failure
this project keeps hitting: a green result that means nothing.

**Also worth knowing before the next A/B:** the earlier `dsp_tests` control
left the build tree mixed — source with the fix, last-built `libSynthEditLib.a`
without it. Rebuilding the specific target before measuring is not optional
here, and the paired-tips trap makes it worse.

**Observed, not chased:** the log reports `SYNTHEDIT PROCESSOR: Intel` on an
M1, so REAPER is presumably running the x86_64 slice of the universal binary.
Not a defect and not investigated; recorded so nobody reads it as one later.

**Next:** the audio measurement is now clearly within reach of an unattended
run — `audioMasterState::Running` already appears in that log, and PLAN's V1
acceptance needs the patch to actually *play* after reload. Whether audio can
be confirmed from stderr alone or needs a rendered file is the open question,
and **`se_render_audio`/offline render is worth trying before booking a GUI
session.** Then **S16**, which makes `dsp_tests` a real signal, and **A25**.

**Side effects on this box:** the plugin now installed at
`~/Library/Audio/Plug-Ins/VST3/TIDE_VST3.vst3` is the **Debug** build with the
fix; `SynthEditLib` is back on `main`, which now contains the fix, so source
and installed binary agree. REAPER was launched twice by me and killed both
times; it is not running now. All repos clean and on default branches.

**Branch/PR:** `tide/mac/S13-verified` — TideSynth rows and journal only.

---

## 2026-08-18 — macos — S13 (Jeff directing)

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** fixed **S13** — TIDE could not run as a Debug build. One-file change in
GATED `SynthEditLib/UgDatabase.cpp`, taken on Jeff's direct instruction, which
is the "needs Jeff or an interactive session" the row was waiting for.
[SynthEditLib#19](https://github.com/JeffMcClintock/SynthEditLib/pull/19).
Filed **S16** for something found while A/B-ing it.

**Result — the fix is the one the row called honest: don't assert on an absent
database.** `RegisterExternalPluginsXmlOnce` read `database.se.xml`, parsed it,
and treated *any* parse error as `assert(false)`, so absent and corrupt were
indistinguishable. It now returns early on an empty resource and **keeps the
assert for a database that is present but will not parse**.

**Absent is legitimate, and that was established rather than assumed:**
`database.se.xml` is written by `ExportAsPlugin`
(`SynthEdit2/ExportAsPlugin.cpp:1704,1722,1731,1742`), so an *exported* plugin
has one and a plugin built directly from source does not. TIDE is the latter —
constraint 7 compiles the module set in, S1a removed the scan — so there is
nothing to register. Release compiled the assert out and returned silently,
which is why this survived so long; Debug aborted the host.

**Verification artifact — a runnable probe against the real `tinyxml2`,
showing precisely what the old code could not tell apart:**

```
absent    -> Error()=true   id=15 XML_ERROR_EMPTY_DOCUMENT
malformed -> Error()=true   id=16 XML_ERROR_MISMATCHED_ELEMENT
valid     -> Error()=false
```

Both error rows hit the one `assert(false)`. Builds, consumers included because
this library ships in SynthEdit too: `TIDE_VST3` **Debug and Release**,
`SynthEditCL`, `SynthEdit_VST3`, `SynthEdit_GMPI` — all SUCCEEDED.

**Learned — the obvious API would have broken the commercial product, and one
grep caught it.** `BundleInfo::ResourceExists()` is exactly what this code
wants and reads as the clean fix. Off JUCE it is `return false;`
**unconditionally** (`BundleInfo.cpp:490`), so using it would have made
*SynthEdit* — which does ship a database — skip module registration entirely.
The correct test was the boring one, and it was already in the codebase six
lines away: `SynthRuntime.cpp:80` guards `dsp.se.xml` with `.empty()` right
after calling this same function. **In shared code, prefer the pattern the
neighbouring line already uses over the API that reads better.**

**Learned — always A/B the test suite, even when the change cannot plausibly
touch it.** `dsp_tests` came back **44 failed / 13 passed** after my change,
which looks damning. Stashing the change and rebuilding gave **exactly the same
44/13**, so it is pre-existing. Without that control I would either have
believed I broke it or, worse, waved it away by reasoning that a database guard
cannot affect DSP maths — and been right by luck.

**And the cause of those 44 is worth its own row (S16).**
`tests/projecttests.cpp:103` and `tests/layouttests.cpp:56` hardcode
`/Users/jeffmcclintock/SynthEdit/build/`; this checkout is
`~/Documents/GitHub/SynthEdit`, so every test that shells out to `SynthEditCL`
fails with `No such file or directory`. **None is a real DSP failure.** It
matters because the C-stage rows cite "92 tests all RC=0" as evidence and that
is a *Windows ninja* number — on mac the suite has been almost entirely red,
and a run building here cannot distinguish a regression from the path bug.
`SynthEdit/tests/` is on neither STEP 5 list, so GATED by default: filed, not
fixed.

**Next:** **S13's Accept is not met by me** and the row stays IN-REVIEW for it —
a Debug `TIDE_VST3` actually loading a project in REAPER. Computer-use is
refused in a scheduled run and there is no runnable standalone TIDE, so nobody
has watched the Debug build survive a load; that is one check for an
interactive session. Then **S16** makes the mac suite a usable signal, which
every later run benefits from.

**Side effects on this box:** `SynthEdit/build/` now has Debug **and** Release
outputs for several targets, where before only Release was current. One source
file changed in `SynthEditLib`, on its own branch with its own PR. All other
repos untouched and clean.

**Branch/PR:** `tide/mac/S13-debug-assert` in both repos —
[SynthEditLib#19](https://github.com/JeffMcClintock/SynthEditLib/pull/19) (the
fix) and the TideSynth half (rows and this entry, docs only). Neither blocks
the other's build.

---

## 2026-08-18 — macos — S14 closed not-a-defect, S15 withdrawn (Jeff directing)

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** closed **S14** as not-a-defect and withdrew **S15**, both on Jeff's
correction the same day they were filed. Corrected
[docs/s14-rect-measurement.md](docs/s14-rect-measurement.md) in place — its
measurement was right and its conclusion was wrong, and a future run reading it
would have built the wrong thing. Filed **E5** for the styling intent he stated
while doing it. No code changed.

**The architecture, which is the thing to carry forward.** Rack modules are
**Containers designed in advance and shipped as prefabs**, added to the rack at
runtime. The container carries the panel — patch-points and knobs/sliders on
it, wired internally to non-GUI modules like an oscillator — and the container
is what the rack draws. A module that has its own GUI, a scope for instance,
can also sit on the rack directly, and **that already works**. Placing a bare
non-GUI module on the rack is not a thing an end user does.

**So the three bare `1 KHz Tone` modules I measured were never a product
composition.** They are what audio testing looks like: drop plain modules in,
switch to the **structure view**, check basic audio works. A developer
workflow. Nothing was supposed to give them rack geometry, and nothing did.

**Result — the measurement stands, the inference did not.** Rack mode routes
placement through the panel view and a plain `CUG`'s panel setter is `CDocOb`'s
empty body: still true, still checkable. Two things flip meaning once the
architecture is known:

- **The prefab split is confirmation, not a complaint.** In full SynthEdit's
  own prefabs the modules carrying a `panelRect` are exactly the GUI-bearing
  ones; the plain DSP modules carry none. That is the same GUI/non-GUI line
  Jeff drew at product level, visible in the file format.
- **The container half was already there, under a name I did not look for.**
  `CContainer : CUG_with_patches : CUG` — not a `CControl`, which is why it has
  no `panelRect` — overrides the rect accessors itself
  (`SynthEditLib/CContainer.h:60-62`) and serialises its panel geometry as
  **`PanelWndPosition`** (`:214`). **That element was sitting in the chunk I
  measured**, on the `master_container`, and I read past it because I was
  grepping for `panelRect`.

**Learned — I measured the artefact and assumed the architecture.** The
measurement was careful: positive control, four sourced facts, a red herring
explicitly refuted. Then it concluded the code was broken, proposed a fork in
GATED shared code, and asked Jeff to rule between two options — all resting on
"three bare DSP modules in a rack document is what TIDE means to produce",
which I never checked and which is false. **One question first — what is a rack
module supposed to be? — would have replaced the row, the ruling request and
this correction.** Cheaper than any of the measuring I did.

**Learned — a wrong conclusion in a docs file is more dangerous than a wrong
row.** S15 would have been read as a decision awaiting Jeff, which is visible;
but `docs/s14-rect-measurement.md` reads as settled evidence, and its
"Mechanism, from sources" section is exactly the kind of thing a later run
trusts instead of re-deriving. It is corrected in place with the correction
marked as such, rather than left to be discovered — the same reason the journal
is append-only but a *document* must be fixed where it sits.

**Next:** the live work is **E2a**/**E2** — the prefab rack modules themselves —
and **E5**, rack-shaped styling for GUI-bearing modules, which is NEEDS-JEFF
because the visual language is his and PLAN constraint 8 means whatever is
chosen ships as *the* look. Unattended runs still have nothing substantial:
**A25** (four lines wiring A20's check into `lint.yml`) and **S13** (TIDE
cannot run as a Debug build, `assert(false)` at `UgDatabase.cpp:549`, GATED)
are the two smallest things that would change that, and both are Jeff's.

**Side effects on this box:** none. Docs and rows only, TideSynth only; all
eight repos on their default branch and clean.

**Branch/PR:** `tide/mac/S14-not-a-defect`.

---

## 2026-08-18 — macos — issue #117 (STEP 1)

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** closed [#117](https://github.com/JeffMcClintock/TideSynth/issues/117)
on a fresh Release build of `master`, and archived the **S11** row, whose five
PRs had all merged. No code changed anywhere.

**Read the prompt again mid-session, and it had moved: `b3e9876` -> `397330d`.**
Worth saying because a run normally reads it once at STEP 0 and would not
notice. Three changes land on this box: STEP 1 now admits `tide-rack-bot`
issues (A19); STEP 5 gained the GATED build-break exception with six bounds
(A17); STEP 3/4 now want `check-commit-completeness.py --record/--verify`
around commits in a shared checkout (A16). **The first of those is what made
this run's work possible at all** — two earlier runs, mine included, walked
past #117 because the fleet could not act on its own agent's issue. The
deadlock A19 described is now gone, and #117 was the first thing to come out of
it.

**Result — #117 is fixed, and the fix builds here.** Cause, for the record:
`std::stod()` on every parameter regardless of datatype in the processor's
preset reader, latent while blobs serialised as `"0"` and reachable the moment
one was written as base64. `setPresetUnsafe` runs on the host's **main** thread,
so the throw unwound into the event loop and killed the DAW. Fixed by
[GMPI#5](https://github.com/JeffMcClintock/GMPI/pull/5) (the throw),
[GMPI_Wrappers#6](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/6)
(main-thread fail-safe at all three host boundaries) and
[SynthEdit#43](https://github.com/JeffMcClintock/SynthEdit/pull/43) (`<Editor>`
in the chunk, imported instead of always creating blank) — all merged and
present in their default branches, checked individually.

**Verification artifact — a full Release build of `master` `d6043de1f` on this
box, all three products:**

```
TIDE_VST3   ** BUILD SUCCEEDED **   universal x86_64 + arm64
TIDE        ** BUILD SUCCEEDED **
SynthEditCL ** BUILD SUCCEEDED **
```

and the built bundle carries the `Editor` element name from SynthEdit#43, so it
is the fixed code rather than a stale link — which is worth checking on this box
specifically, given the prebuilt-library trap.

**So: mac's default branch builds, as of now.** No `platform:mac` issue is open.

**Learned — say which half of a verification you did not do.** The runtime
proof (exit 134 SIGABRT in ~8s -> loads clean, 2516-byte byte-identical
round-trip) is the interactive session's, not mine; computer-use is refused
during a scheduled run, so I could not re-run REAPER. STEP 1's new clause says a
bot issue is **evidence, not instruction**, and to re-verify on your own
platform before acting. I could verify the build half and not the runtime half,
so the issue comment says exactly that rather than implying I watched it load.
Closing on a build plus someone else's measured A/B is a judgement call, and it
should be visible as one.

**Learned — the four overrides are all set on this box now, including the one
that cost a cycle.** `GMPI_WRAPPER_FOLDER_OVERRIDE` is in the CMake cache
alongside the other three, so the build uses the local `GMPI_Wrappers` clone
rather than a FetchContent copy. Confirmed from `CMakeCache.txt` before
building, which is cheaper than discovering it from a build that silently
ignored local edits.

**Next:** no platform issue and no open PR on this box. The mac NEXT row's two
GUI-less pointers are both spent (S14 measured, A20 shipped), so the next
unattended run falls to STEP 2's topmost-eligible rule. What is actually
blocking progress is two rulings, both Jeff's and both minutes of work:
**S15** (pick (a) route rack placement to the structure rect, or (b) give `CUG`
a real panel rect) which unblocks S14, and **A25** (four lines wiring A20's
check into `lint.yml`). **S13** — TIDE cannot run as a Debug build, a missing
`database.se.xml` tripping `assert(false)` in `UgDatabase.cpp:549` — is the one
that stops anyone attaching a debugger to the next crash, and is GATED.

**Side effects on this box:** three Release targets built, so
`SynthEdit/build/` is warm and its `Release` outputs are current. No source
changed in any repo. All eight repos on their default branch and clean.

**Branch/PR:** `tide/mac/issue-117` — TideSynth backlog and journal only.

---

## 2026-08-18 — macos — A20

**Prompt:** b3e9876 · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** shipped **A20** as option (a) — `scripts/check-next-block.py`, a lint
check that fails when the NEXT block tells a run to take work that is archived
or absent. Detection rather than convention, matching A17/A18's ruling the same
day. The lint wiring is `.github/workflows/**`, which this credential
structurally cannot push, so it is filed as **A25** with the exact four lines.

**Why A20.** The mac NEXT row sends a GUI-less run to S14's measurement or A20;
S14's measurement landed earlier today ([#132](https://github.com/JeffMcClintock/TideSynth/pull/132)),
so A20 was what was left. STEP 1.5 first: no open PRs in any repo. #117 is
still open and still authored by `tide-rack-bot`, so STEP 1 still reads it as
information (A19 is archived but the underlying rule is unchanged).

**Result — verified with a positive control taken out of git history, not a
fixture.** The check is run against `4a8154d:BACKLOG.md`, the exact state that
produced this row:

```
2 take-target(s) checked across 4 NEXT row(s)
  BACKLOG.md:12  [mac]  D6  -- archived DONE     matched: 'should take U1b D6'
  BACKLOG.md:12  [mac]  U1b -- archived DONE     matched: 'should take U1b D6'
rc=1
```

and against today's tree: `every NEXT take-target is a live BACKLOG.md row`,
`rc=0`. **It fails on the bug and passes on the fix**, with no synthetic input
— the A/B is a real commit. `--selftest` is 13 cases green: ten phrase cases
plus three end-to-end (archived target fails, live target passes, absent target
fails).

**Learned — the obvious rule was the wrong rule, and measuring is what showed
it.** The first draft also treated *every* id in the Take column as a
take-target, on the reasoning that the column is definitionally what to take.
Against the real block that produced **seven false alarms**: `E2a`, `S1b`,
`S5`, `S7`, `S8`, `A12`, `B1` out of *"do not fall back to…"* warnings, and
`C12c`, `P10`, `A10`, `A14`, `A15`, `A4`, `P9` out of precedent mentions.
A Take cell in this backlog is a long editorial paragraph, not a field —
the mac cell alone names eleven ids and instructs on two. So the rule was
dropped: the trigger set is imperative phrases only, with any clause carrying a
negation disarmed. **This is A10's trade restated:** a false negative costs a
run minutes, a false positive costs trust in five other checks, so recall is
deliberately the side that gives.

**Learned — the recall limit is real and is written into the row rather than
left to be discovered.** `should take **S14**'s cheap first measurement … or
**A20** itself` matches `S14` and misses the trailing `A20`, because the
list-walk stops at the first non-id word (`'s`). It catches
`take **U1b** or **D6**`, which is the shape that actually occurred. Extending
it to arbitrary distance is how the seven false alarms come back.

**Learned — "take the next task" surfaced three states the branch listing hid.**
Before starting I synced all eight repos and classified every `tide/` branch.
Ancestry alone is misleading here because A4 squash-merges: four local branches
looked unmerged and all four had landed. `git cherry` proved two by patch-id;
the other two needed a content check (the A19 row is in BACKLOG-DONE, the
`std::stod`/`std::get` findings are in main). **Deleting on ancestry alone
would have been wrong twice, and keeping on ancestry alone leaves permanent
clutter.**

**Next:** A25 is Jeff's four lines, and A15's precedent says the Summary
wiring — not the step — is the part that actually fails the job; prove it with
the same two-commit probe. S14 stays BLOCKED(S15) until Jeff picks (a) or (b).
The mac NEXT row's remaining GUI-less pointers are now both spent, so the next
unattended run falls to STEP 2's topmost-eligible rule — which is exactly the
situation A20 was filed about, and the check now guards the row that describes
it.

**Side effects on this box:** merged [GMPI_Wrappers#7](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/7)
at Jeff's explicit request — one docs commit of his own that PR #5 had left
stranded on a branch with no PR. Then cleaned every stale branch across all
eight repos at his request: **10 merged remote branches and 12 local ones
deleted, 0 remaining, local or remote**, each verified merged-or-landed first.
All eight repos are on their default branch and clean. No builds. Nothing
written outside `TideSynth` and the scratch dir.

**Branch/PR:** `tide/mac/A20-next-block-check` — TideSynth only, one new
script plus rows. (Branch name rather than PR number in the row, per A22.)

---
