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

## 2026-08-18 — macos — V1's audio half: THE RACK SOUNDS (interactive session, Jeff directing)

**Did:** measured the audio half of PLAN's v0.1 acceptance test — "have the patch
survive save-and-reload of the host project" — and it **passes**. Built the one
input that was missing: a saved REAPER project whose TIDE instance carries a
wired rack. Then answered the two questions the job hinged on, added the
per-prefab E1 cases, and corrected `render-and-measure.py`'s now-wrong
diagnostic.

**Result.**

```
control (known -6 dBFS 1 kHz sine)   peak=  -6.0 dBFS  rms=  -9.0 dBFS  AUDIO PRESENT
v1-rack.rpp                          peak=  -6.3 dBFS  rms= -17.0 dBFS  AUDIO PRESENT
   rack: 2 patch cable(s) (HC_PATCH_CABLES); 8 <Line>(s) inside prefab containers
v1-rack-uncabled.rpp                 peak=  -inf       rms=  -inf       SILENCE
   rack: 0 patch cable(s) (HC_PATCH_CABLES); 8 <Line>(s) inside prefab containers
```

Characterising the render rather than only gating it: **440.0 Hz, left channel
only, right channel digital silence** — which is precisely the wiring. 5 V at
1 V/octave is middle A, only the L jack is cabled, and the ADSR sits at its
sustain after a 3 dB step in the first 100 ms with the gate at its open default.
Nothing was sequenced; the rack sounds with no MIDI, as designed.

**Question 1 — does the patch-cable wiring survive the save? YES, and here is
where it lives.** A patch cable is **not** a `<Line>` in the saved document. It
is an entry in a serialised `<Cables>` list held in the patch manager as the
**`HC_PATCH_CABLES`** host control — **49**, counted off the enum at
`SynthEditLib/HostControls.h:14` — written by `MfcDocPresenter::AddPatchCable`
and turned back into DSP connections at load time by
`ug_container::ConnectPatchCables` (`SynthEditLib/ug_container.cpp:433`). Decoded
out of the saved `.rpp`, both cables are there, with their endpoints resolving to
the right jacks:

```
cable 1: 'TIDE Oscillator' / SE Patch Point out (panel top=86)  -> 'TIDE Envelope' / SE Patch Point in (top=40)
cable 2: 'TIDE Envelope'   / SE Patch Point out (panel top=132) -> 'TIDE Output'   / SE Patch Point in (top=40)
```

So the S11 round-trip inference held. It is now measured twice over — once by
reading the parameter back out of the file, once by the audio coming back.

**Question 2 — the `<Lines>` message was misleading, and fixing it needed a
negative control, not an argument.** The old text said a zero `<Lines>` count
made silence certain, which implied a non-zero count meant a project could
sound. That was true only while saved projects held bare modules. **A rack of
three prefabs reports EIGHT `<Line>`s whether or not it is wired**, because
those belong to the containers' insides. Rather than assert that, I built
`tests/hosts/v1-rack-uncabled.rpp` — the same three prefabs placed, deliberately
uncabled — which is exactly the case the old message got wrong: eight `<Line>`s,
zero cables, silent. Both branches of the rewritten diagnostic are now exercised
by real artifacts in the repo. The script reports both counts and says which one
it judged on.

**Per-prefab E1 cases: two of the three, and the third cannot exist.**
`tests/cases/prefab_oscillator.json` locks 440.0 Hz from the pitch jack's 5 V
default and the statically-registered `Oscillator` primitive (not the absent
`OscillatorNaive`, S8). `tests/cases/prefab_envelope.json` locks the audio path
the prefab exists for — ADSR at Overall Level 1 into `Multiply`, peak **exactly
0.5** from a 5 V input, sustaining at **0.7**, so the VCA is unity gain and the
gate is open. Suite is **4/4**. **Output gets no standalone case:** it is a
`Sound Out`, whose entire job is handing audio to the host, and the harness
records from a pin with `--render-audio --from`, which is upstream of that. The
`.rpp` render *is* Output's test — audio appearing in the host's own render is
the only observation that can prove a Sound Out works.

**Learned — five things, two of them corrections to me.**

1. **A patch point carries VOLTS, and 10 V is full scale.** Setting the envelope
   case's IN jack to `1` gave a −20.0 dBFS render, which looks like a 14 dB gain
   bug and is not one: 1 V is 0.1. At 5 V — what the Oscillator prefab actually
   delivers, measured at 0.49 — the peak is exactly 0.5. This is also why the
   generator's `--set-pin $gate:Input 10` means "gate fully open". Written into
   the case description so the number cannot be misread later.
2. **A DAW lists TIDE under its product name `TIDE Rack`, not its filename.**
   Filtering REAPER's FX browser for "TIDE" finds
   `VST3i: TIDE Rack (TIDE Synth)`. REAPER's cache can also still serve a name
   from an older build — this box's `reaper-vstplugins_arm64.ini` still said
   `SynthEdit (GMPI)` — in which case a re-scan (Preferences → Plug-ins → VST →
   Re-scan) is what makes the current name appear.
3. **Corrected in-session, by Jeff, twice.** I first searched the browser for
   "SynthEdit" because that is what the stale cache entry said; Jeff pointed out
   the plugin is TIDE Rack, which is what sent me to the product name at all.
   Later I reported an empty result list and theorised that the installed
   bundle's directory mtime does not change when a POST_BUILD copies a new binary
   into it, so REAPER's startup scan skips TIDE. **That theory was wrong and is
   not evidence of anything** — Jeff spotted that the filter field actually read
   "TDE", a dropped keystroke of mine. Recorded because the wrong theory is
   exactly the sort of thing that gets quoted later as a build-system fact.
4. **A dropped prefab does not keep the generator's slot size.**
   `build-prefabs.py` writes `PanelWndPosition` as 100x160, but the properties
   pane reports **W 20, H 66** for the Oscillator and Output and **H 112** for
   the three-jack Envelope — the container is sized to its jacks on drop. The
   size in the file still matters (it is what stops a prefab drawing nothing, the
   `docs/e2a-prefabs.md` 9.1 trap); it is just not what survives placement.
5. **The "places but does not draw" intermittency did not appear once**, across
   six placements in two REAPER sessions. Not a fix, and not evidence it is gone
   — but worth recording as a data point beside the times it did happen.

**No exception, no crash.** Both REAPER sessions were launched from a shell so an
uncaught C++ exception would name itself; stderr carried one line of swell
metal-context noise and TIDE's own
`TIDE: 3 rack prefab(s) seeded from the bundle` — which incidentally confirms the
installed bundle's `Resources/Prefabs/` is what supplied the prefabs **in the
real host**, not only in the standalone. Both instances exited 0.

**Next:** **V3**, filed this run — "play it from the DAW's MIDI" is the one clause
of PLAN's v0.1 acceptance test that no row in BACKLOG covered, and V1 is
specifically the save/reload clause. It needs **no GUI session**: a MIDI item is
plain text inside a `.rpp`, so the fixture can be edited by hand and measured
with the script that already exists. **E2**'s `BLOCKED(V1)` premise — "no point
authoring a fuller module library for a plugin that cannot yet keep its patch
across a host save" — is retired by this measurement. **E6** is untouched and
still open; nothing here depended on it.

**Side effects on this box:** REAPER was launched twice interactively and four
times headlessly by the render script, and exited on its own each time; it is not
running. Its VST plugin cache was re-scanned (Preferences → VST → Re-scan), which
is a settings change to REAPER, not to any repo. Renders went to a temp dir the
script cleans up. `tests/hosts/Backups/` — REAPER's own `.rpp-bak` folder,
created beside the fixtures — was deleted and is now gitignored, along with
`report.json`, which the E1 harness writes into the repo root on every run. Only
TideSynth changed.

**Branch/PR:** `tide/mac/v1-audio-half`.

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

## 2026-08-18 — macos — the audio measurement: harness built, answer is "not yet, and here is exactly why"

**Prompt:** 397330d · claude-opus-5[1m] · app 2.1.229 · as tide-rack-bot

**Did:** built and proved a headless audio-measurement harness
(`scripts/render-and-measure.py`), ran it on every saved TIDE project, and
established that the audio half of V1 **cannot be answered by any artefact that
currently exists** — for a specific, measured reason rather than for want of
trying. V1 stays `BLOCKED(E2a)`, now confirmed empirically instead of by
argument.

**The GUI assumption was wrong twice in one day.** S13 established that running
a program is not driving a GUI. The same applies here and nobody had tried it:
`REAPER -renderproject file.rpp` **renders and exits**. So an unattended run can
measure audio, and the "audio needs a GUI session" line — which I wrote into a
handoff prompt this morning — was never true.

**Result — the harness, with a positive control, because silence proves nothing
on its own.**

```
control (known -6 dBFS 1 kHz sine)  peak=  -6.0 dBFS  rms=  -9.0 dBFS  AUDIO PRESENT
tide-s11-final.RPP                  peak=  -inf       rms=  -inf       SILENCE
tide-s11-verify.rpp                 peak=  -inf       rms=  -inf       SILENCE
tide-persist3.rpp                   peak=  -inf       rms=  -inf       SILENCE
```

The control lands at exactly -6.0 peak / -9.0 rms, which is what a sine should
give, so the render-and-measure chain demonstrably detects audio. **The three
silences are still not evidence about TIDE:** every saved project has **zero
`<Line>` elements** in its DSP graph, so silence is arithmetically certain
whatever the plugin does. The script detects and prints that itself rather than
leaving a future reader to infer it — an -inf with no explanation is exactly the
kind of result that gets quoted later as "audio is broken".

**What is actually missing is a patch that could sound at all.** TIDE's rack
prefabs exist — `SE16/TideModules/Sine.seprefab`, `AR.seprefab`,
`Output.seprefab` — but they are **binary MFC serialisations** like `TIDE.se1`,
so they can only be placed and wired by the editor. That is E2a, precisely as
V1's row already said: *"the acceptance test itself needs the
oscillator/envelope/output prefabs E2a builds before it can even be attempted."*
**The row was right; this makes it measured rather than reasoned, and leaves the
tooling ready for the day E2a lands.**

**Considered and rejected: hand-authoring a sounding patch.** The DSP format is
simple enough — `<Line From="id" To="id" FromPin="n" ToPin="n"/>`, parsed at
`SynthEditLib/SeAudioMaster.cpp:1198` — and I could have written one. I did not,
because I do not know the container IO plug conventions, and **a wrong guess
produces silence that is indistinguishable from a real failure.** That is the
S14 mistake exactly: measuring carefully against an assumed architecture. The
format is recorded in V1's row so the next person starts ahead of where I did.

**Learned — three layers of encoding in a `.rpp`, each of which cost a wrong
guess, written down so nobody re-derives them.** (1) The `<VST` body is one
base64 stream split over lines, **but the first line is REAPER's own header
block with its own `=` padding**, so concatenating everything and decoding
truncates at 44 bytes. (2) The decoded state is preset XML whose blob attribute
is **`val=`, not `value=`** — my regex was right about everything except the
attribute name and silently matched nothing. (3) That is base64 again, and
yields the `<Document>`. All three are in the script's docstring.

**Next:** **E2a** is now the single thing standing between the fleet and V1's
acceptance test, and it needs the editor, so it is an interactive job. The
moment those prefabs can be placed and saved, `render-and-measure.py` answers
the audio question in one command with no GUI. Remaining unattended-friendly
work is **S16** (make `dsp_tests` a real signal) and **A25** (four lines of lint
wiring); both are small and both need Jeff.

**Side effects on this box:** REAPER was launched four times headlessly by me
(three project renders plus the control) and exited on its own each time; it is
not running. Renders went to a temp dir the script cleans up. No repo but
TideSynth changed.

**Branch/PR:** `tide/mac/audio-measurement`.

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

