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

## 2026-08-18 — macos — E9 researched: a rate change is absorbed by REPLACING the plugin, not by re-reading the rate (interactive session, Jeff directing)

**Did:** answered Jeff's question — *how do SynthEdit's AU and VST3 targets handle
a host sample-rate change, given it requires rebuilding the DSP graph* — by
reading all four wrappers and then **measuring a live rate change in REAPER**.
Wrote [docs/e9-sample-rate.md](docs/e9-sample-rate.md), corrected E9's row, and
fixed two wrong comments in `SynthEditSem/SynthEdit.cpp`.

**Result: the premise is right, E9's diagnosis was wrong, and the correction is
the finding.** The rebuild Jeff expected already exists and is already
rate-triggered — `SynthRuntime::prepareToPlay` rebuilds when
`generator->SampleRate() != sampleRate` (`SynthRuntime.cpp:51`). What no wrapper
does is *tell a running plugin* about a new rate. `gmpi::api::IProcessor` has
three methods — `open`, `setBuffer`, `process` (`GMPI/Core/GmpiApiAudio.h:50`) —
so there is nowhere to put such a callback. Instead
`gmpi_processor::start_processor` (`GMPI/Hosting/processor_holder.cpp:48`)
**destroys the IProcessor** (`:55`), **creates a new one** (`:69`), calls `open()`
(`:82`), and re-seeds the blob parameter from its retained bytes (`:215`) — so
TIDE's chunk arrives again, `onSetPins` runs again, and the rack is built at the
new rate. Doorbells: VST3 `setActive(true)`, AU `Initialize()`, CLAP `activate()`,
standalone `onAudioFormatChanged`.

**The measurement, since this row had never had one.** REAPER launched from a
shell on `tests/hosts/v3-midi-pitch.rpp`, then **Preferences → Audio → Device →
Request sample rate** driven by hand 48000 → 44100 → 48000 on the loaded project
(the GUI route the row said this needs). Eight `TIDE: rack built for N Hz` lines,
the rate following the device every time, and playback afterwards metering
**−6.2 dBFS peak / −13.4 RMS** — the level the fixture gives at 48 kHz. Device
and preference left exactly as found; REAPER quit cleanly.

**Learned — the thing that reframes the row.** **Not one of those eight lines
carried the `(rate CHANGED)` suffix, and it never can.** `preparedSampleRate` is
a *member* of the object `start_processor` destroys, so it is re-zeroed with each
new instance. The guard cannot outlive the rebuild that handles the change. The
repeated identical `44100` lines are the proof — an instance that survived with an
unchanged rate would print nothing at all. **My earlier comment drew the wrong
conclusion from correct evidence** (it inferred "the rack would keep the stale
rate and everything would be detuned, silently"); the evidence was the absence of
a line that is structurally impossible.

**Second wrong comment, also fixed:** *"the AsyncRestart path is unreachable in
the plugin runtime — nothing enters `eRuntimeState::resetting`"*. `resetting` is
entered via `ug_vst_out.h:65` → `SeAudioMaster::onFadeOutComplete()` (`:1509`) →
`OnFadeOutComplete()` (`iseshelldsp.h:124`), and `ug_vst_out` **is**
`audioOutModule` in a plugin (`SetupVstIO()` runs under `!isEditor()`,
`SeAudioMaster.cpp:502`). `DoAsyncRestart` is reached from
`dsp_patch_parameter.cpp:773` for any host control with `requiresAsyncRestart()`
— a set that **includes `HC_PATCH_CABLES`**, i.e. every rack re-cabling. Nothing
in TIDE calls it *yet*; that is TIDE's wiring, not the runtime's limits.

**What is actually left of E9, and it is smaller:** a fresh instance with **no
chunk stored never prepares at all** — `processor_holder.cpp:225` `continue`s on
an empty blob, and TIDE's only `prepareToPlay` call site is that blob arriving.
The fix has an exact precedent in SynthEdit's own glue: override `open()` like
`se_gmpi/source/SeGmpiProcessor.cpp:151`, and let the blob be a pure document
swap. Two caveats for whoever does it: `DoAsyncRestart()` alone cannot absorb a
rate change (the `resetting` branch rebuilds from the member `sampleRate`,
`SynthRuntime.cpp:388`, which only `prepareToPlay` writes), and `prepareToPlay`
never joins `dspBuilderThread`, so its precondition is no concurrent `process()`.

**Not claimed:** AU and CLAP are read, not run — TIDE builds neither
(`SynthEditSem/CMakeLists.txt:59` is `GMPI VST3 STANDALONE`). Whoever adds AU
should know `reInitialize()` does not update the `AU2_Wrapper::sampleRate` that
`getSampleRate()` returns, and that `offLineRenderMode`'s only consumer is inside
`#if 0`.

**Next:** either take the `open()` latch above, or E2 / the per-prefab E1 cases.

**Branch/PR:** `tide/mac/e9-research` (TideSynth), `tide/mac/e9-comment-fix`
(SynthEdit).

---

## 2026-08-18 — macos — PLAN's v0.1 acceptance test is COMPLETE (interactive session, Jeff directing)

**Did:** merged the last three PRs, synced the fleet, rebuilt against updated
dependencies and re-measured everything. **Every clause of PLAN's v0.1 acceptance
test now passes, measured.** V3 and E8 are DONE and archived.

**The acceptance test, clause by clause, all measured rather than argued:**

| clause | evidence |
|---|---|
| loads in a DAW, shows the rack | `TIDE: 5 rack prefab(s) seeded from the bundle`, editor opens |
| drop in an oscillator and an envelope as prefabs | E2a, DONE |
| cable them to an output | 4 patch cables in `HC_PATCH_CABLES` |
| **play it from the DAW's MIDI** | **261.6257 Hz for a middle C — +0.001 cents** |
| patch survives save-and-reload | −6.3 dBFS, 440.0 Hz, cables intact |

Five host fixtures in `tests/hosts/`, E1 **4/4**, all re-run from merged `main`.

**Where this session started:** nobody had ever heard TIDE make a sound after a
host reload, and V1 had been blocked for weeks behind a circular dependency with
E2a. It ends with the whole v0.1 bar cleared and four checked-in fixtures anyone
can re-run in one command. That last part is the real change — this stopped being
something the project reasons about and became something it measures.

**Rows closed today:** V1, E2a, V3, E8 — all archived. **A25** landed too (the
NEXT-block check now actually gates `lint`, proven by a two-commit probe).

**Still open, and none of them blocking:** **E9** (TIDE latches its sample rate at
document-push time with no rate-change path — the nearest thing to a live defect
left, and the new `mac` NEXT target), **E7** (polyphony cannot escape a container —
V3 side-stepped it by keeping the MIDI-CV at the root, so it is an
engine-limitation row now rather than a blocker), **E6**, **S8**, **E2**, and
**E5**/**A25**-style items needing Jeff.

**Dependency churn checked rather than assumed.** The sync pulled GMPI_Wrappers
`ea2e357 → ebf8cfe`, GMPI-plugins `5c1c6e5 → 79e3f92` and synthedit-website. TIDE
builds against local overrides of GMPI and GMPI_Wrappers, so those land in the
plugin on the next build — which is exactly the kind of thing that silently
invalidates a measurement. So TIDE was rebuilt against them and every fixture
re-measured **unchanged**, including the pitch at +0.001 cents. Both deltas were also audited
and both came back `affects_tide: no` at high confidence, which explains the
unchanged numbers rather than just corroborating them:

* **GMPI-plugins** deletes one stale unused `FreqAnalyser.xml` that was never a
  build input (its CMakeLists never passed `HAS_XML`; the plugin registers inline
  in code under a different id), and GMPI-plugins is not in TIDE's build at all.
* **GMPI_Wrappers** is 20 files, all under `wrapper/Standalone/` or `mcp/` —
  nothing under `wrapper/VST3/`, `wrapper/common/` or the shared
  `GMPI_HOSTING_SRCS`. It teaches the standalone to notice when its audio device
  dies (`isStreamRunning`/`stoppedReason`) and hardens the Windows named-pipe IPC.
  `Processor_VST3.cpp` is not in the changed-file list at all, and no added or
  removed line mentions ump/noteon/pitch/bend — so the class of bug fixed today is
  not in scope. The one CMake risk was real and is clear: `add_subdirectory(Standalone)`
  is unconditional, so a configure error there would break TIDE's configure even
  though the VST3 never links the target; the sole change is one header added to
  `standalone_mcp_srcs`, and the file exists.

**Two caveats from that audit worth keeping, both confined to `TIDE_STANDALONE` —
the developer target this project uses for screenshot/click/render work.** (1) The
MCP `info` reply now gates `sampleRate`/`bufferFrames` on a live driver poll and
adds an `audioStopped` field, **so a harness that reads `sampleRate` out of `info`
will find it ABSENT rather than stale once a stream has died** — a JSON-shape
change in the measurement tooling, not in what TIDE renders. (2) Windows only:
WASAPI's `Start()` moved inside `open()`, so a device that refuses to start now
fails the open instead of returning success and playing nothing. Strictly better,
but a real behaviour change if anyone drives the Windows standalone.

**Learned, and it is the pattern of the whole session.** Six of my own hypotheses
died today, and every single one failed the same way: **I attributed a silence or
an error measured at the END of a chain to a component inside it.** The tool (E6),
the wire (the missing converters), the module (MidiToGate2), the sample rate, the
convention, the JUCE block. Each time the fix was to measure at the suspected link
instead — a `Sound Out` inside the container, a trace in the module, the authors'
own `DMIDI_LOG`. And twice the *instrument* was the thing at fault: the render tap
cannot see inside a container, and a 0.25 s Goertzel window cannot separate two
candidates 1.2 Hz apart. **Validate the instrument on a case whose answer you
already know, before believing what it says about the case you care about.**

**Side effects on this box:** no REAPER this round — every measurement was a
headless render. TIDE_VST3 rebuilt Release from merged `master`. All fourteen repos
on their default branches and clean; the three merged feature branches deleted and
pruned. `AlphaBlender` remains parked on `DrawOnImage`, `VST_SDK` detached at its
SDK tag, both deliberately.

**Next:** **E9**.

**Branch/PR:** `tide/mac/v3-e8-done`.

## 2026-08-18 — macos — V3: the root MIDI-CV design works, gate and pitch (interactive session, Jeff directing)

**Did:** implemented Jeff's design — every fresh document gets `MIDI In` →
`SE MIDI to CV 2` → a **facade** rack module at the ROOT — and measured it. Gate
and pitch both work, and pitch tracking is exact.

**Result.** `tests/hosts/v3-midi-pitch.rpp`:

```
0.05-0.45 s  silent
0.60-1.10 s  311.0 Hz          <- the note, and the pitch TRACKS it
1.35-1.95 s  silent
```

and a two-note octave fixture settles the tracking question properly:

```
C4 (note 60)  311.2 Hz
C5 (note 72)  622.2 Hz
ratio 1.9994  -> 12 MIDI semitones produce 11.99 semitones of output
```

So 1 V/octave is right to within 0.05%.

**Why the design works where the obvious one did not.** `SE MIDI to CV 2` is
`polyphonicSource`/`cloned`, so whatever container holds it becomes a voice
container — and polyphony cannot escape a container (**E7**). Inside a rack module
its Gate is correct internally and worth nothing outside, which is what V3
measured earlier. Jeff's move: keep the real MIDI-CV at the **root**, where the
root itself is the voice context, and make the rack module a **facade** — a
container holding nothing but jacks, each fed *inward* from the root MIDI-CV
through the container's own pins. Carrying CV inward is an ordinary connection,
not a polyphonic escape. **It side-steps E7 rather than needing it fixed.** And
one MIDI-CV per project, created and owned by TIDE, means "what if the user adds a
second" never arises.

**Three implementation facts worth keeping.**

1. **`AddModule` returns −1 for a prefab** because a prefab may hold several
   top-level modules, so there is no handle to hand back. The fix is a handle
   snapshot/diff around the call — and
   `EditorScreenshot/EditorCommandDispatcher.cpp:1399` **already does exactly
   this**, so TIDE now uses the same idiom rather than a second invention.
   `dynamic_cast<CUG*>` is the necessary filter, not decoration: a container's
   child list holds `CLine2` connections alongside modules, and `AddSorted`
   *prepends* modules while appending lines, so iteration order is
   reverse-insertion.
2. **The pin contract.** A container's outer input pin is `7 + jack index` — pins
   0..6 are the Container's own built-ins (2 is Visible), so 7 is the first
   synthesised IO pin. Verified for 1, 2 and 4 jacks. `build-prefabs.py` and
   `TideApp.cpp` both state it; change one and the other changes.
3. **Coordinates are DOCUMENT space and the canvas is centred near 4000.** A
   user-dropped prefab lands around X 4024-4288, Y 3944-4008. Seeding at
   `{40,40}` "works" and puts everything in the far top-left, off the visible
   rack — which looks *exactly* like the insert having failed. It had not; it was
   scrolled out of view. Cost one build cycle.

**A measurement I nearly reported as a finding, and the control that stopped
me.** Before touching TIDE I tried to validate the design headlessly: root
MIDI-CV → containerised patch point, render the inner jack. It read `0.0000`, and
so did a monophonic control — which looked like "signals cannot enter a container"
and would have contradicted the whole design. Then I put source *and* sink both
INSIDE the container, so nothing crossed a boundary at all: still `0.0000`. **The
render tap cannot see inside a container**, so all three results were false
negatives. That is also a live trap for the E1 harness — any future case whose
`--from` pin sits inside a container will read silence regardless of the audio.

**What is left is a tuning constant, not a design problem.** Note 60 sounds
311.0 Hz where 261.6 Hz is wanted: a fixed **+3 semitone** offset, 0.25 V between
`Oscillator`'s V→Hz reference and `SE MIDI to CV 2`'s note→volts (which behaves as
`note/12 − 0.5`). Filed as **E8** with the measurement that proves it is an offset
rather than a scale error. Absolute-voltage readings of MIDI-CV 2's Pitch pin are
NOT in that row, deliberately: I scaled them through a `Multiply` whose Input-2
units I could not pin down, so those numbers were uncalibrated and I dropped them
in favour of the frequency ratio, which needs no calibration.

**Jeff asked two questions that each moved a finding, and both are worth keeping.**

**"Does TIDE have the correct sample rate or some hard-coded default?"** Not
hard-coded — `SynthEdit.cpp` passes `host->getSampleRate()`, and a log confirmed
real values: **48000 Hz then 44100 Hz**, block 512, in one offline render. But the
call site matters more than the value: `prepareToPlay` is reached from **exactly
one place**, the chunk arriving in `onSetPins`, so the rate is latched at
*document-push* time and nothing handles a rate change. Filed as **E9**, with a
`preparedSampleRate` guard that logs `TIDE: rack built for N Hz` on a change.
**And the two rates are two INSTANCES, not one re-preparing** — the guard's "rate
CHANGED" branch did not fire, which is how that is known rather than assumed. I
had written the opposite in a comment first and corrected it.

Forcing a different render rate turned out not to be possible headlessly: REAPER
ignores a project's `SAMPLERATE` line when rendering, and hand-writing
`RENDER_SRATE` into a `.rpp` makes REAPER **stop on a dialog** — Jeff saw it
blocking a render before I did. So a genuine rate-change test needs REAPER's own
dialog, and E9 says so rather than pretending the headless attempt proved
something.

**"Find SynthEdit's pitch calculation, confirm it's what you think."** It was not
what I thought, and **E8 is materially different as a result.** I had written that
`Oscillator`'s V→Hz and `SE MIDI to CV 2`'s note→volts disagreed about the
convention. They do not — both are correct and they agree:

* `ug_oscillator2.cpp:31` — `440 * powf(2, FSampleToVoltage(v) - MAX_VOLTS/2)`,
  and the Pitch pin documents itself at `:66`: default `"5"`, *"1 Volt per Octave,
  5V = Middle A"*. So float 0.5 → 5 V → 440.0 Hz.
* `CVoiceList.cpp:1930` and `dsp_patch_manager.cpp:52` are the same line —
  `volts = GetKeyTune(key) * (1/12) - 0.75` — with the comment *"SE convention is
  Volts, 1V/octave, with MIDI A4 (key 69) = 5.0V"*. Key 60 → 4.25 V → **261.6 Hz,
  the right answer.**

So the +3 semitones is a **bug against the engine's own stated formula**: something
supplies key 63 where 60 was sent. That also moves E8's scope from TIDE's prefab
(ALLOWED) to `SynthEditLib` (GATED), which is the opposite of what I first wrote.

Jeff also supplied the piece that made my voltage readings interpretable and then
worthless: **5 V is float 0.5 on an audio cable**, hence the `0.1f *` scaling
everywhere. My `Multiply`-scaled readings of MIDI-CV 2's Pitch pin could not be
reconciled with that at any assumed factor, which is why E8 rests on the frequency
ratio — which needs no calibration — and not on them.

**One suspect ruled out rather than left hanging:** a MIDI 2.0 note-on carrying
`attribute_type::Pitch` retunes the key table (`ug_container.cpp:1206-1215`), which
would give exactly a wrong-but-musical offset. But the observed packet is
`40 90 3c 00` and byte 3 **is** the attribute type — `0x00`, none — so that branch
never fires.

**E8 IS FIXED, and it was a real bug — one line, in MIDI 2.0 per-note bend.**
[SynthEditLib#20](https://github.com/JeffMcClintock/SynthEditLib/pull/20).

```
before   311.1270 Hz   (+3.000 semitones, 0.00 cents from 2^(3/12))
after    261.6257 Hz   (middle C is 261.6256 -- +0.001 cents)
```

`ug_container.cpp`'s `case PolyBender` passed `decodePolyController`'s value
straight to `HC_VOICE_PITCH_BEND`. That value is **0..1 with 0.5 = centre**, while
`MidiToCv2` consumes `pinVoiceBender` as **bipolar around 0**
(`pinVoiceBender * 0.05f`). The channel-wide `PitchBend` case **twelve lines
below already does the `[-1,+1]` remap, and carries a comment saying why it is
required.** `PolyBender` simply never got it.

**Why it read as a convention disagreement for so long.** REAPER emits a
**centred** per-note bend at note-on, so every note arrived `0.5 * 0.05` = 0.025
normalised = **0.25 V = exactly three semitones sharp** — a minor third out while
staying perfectly in tune with itself. Octaves came back at ratio 1.9994 and
tracking was exact, so everything *except* the reference looked right. That is
what made me write "the two modules disagree about the convention", which was
wrong twice over: they agree, and they are both right.

**Two of Jeff's questions did the actual work here.**

*"Find SynthEdit's pitch calculation, confirm it's what you think."* It was not:
`ug_oscillator2.cpp:31/66` (float 0.5 = 5 V = Middle A) and
`CVoiceList.cpp:1930` / `dsp_patch_manager.cpp:52`
(`volts = GetKeyTune(key)/12 - 0.75`, A4=69=5 V) are both correct and agree. Key
60 → 4.25 V → 261.6 Hz is what the code says it should do. So the +3 semitones had
to be something *added later* — which is what pointed at the bender.

*"Check the ratio between the two samplerates vs the ratio between the two Hz
measurements."* This is what made the finding exact rather than plausible. The
error is 1.189207; 2^(3/12) is 1.189207 — **0.00 cents**; `(48000/44100)²` is
1.184692 — **6.59 cents away**. Those two candidates are only 1.2 Hz apart, and
the 0.25 s Goertzel window I had been using **could not have distinguished them**.
Re-measuring by zero-crossing count over 170 cycles (~0.01 Hz) settled it and
killed the sample-rate explanation outright.

**Found by enabling instrumentation that already existed.** `debug_midi_log.h` has
a `DEBUG_CONTAINER_MIDI` gate feeding 12 `DMIDI_LOG` sites across
`ug_container.cpp`, `CVoiceList.cpp` and `ug_midi_to_cv.cpp`, writing to **stderr**
— which the render harness already captures. Flipping it on printed the whole
chain and showed the engine correct until `hc=31` (`HC_VOICE_PITCH_BEND`) arrived
as `0.5000`. **Look for the authors' own trace before writing your own.** All of
it, plus one extra `DoNoteOn` line I added, is reverted; only the fix remains.

**A trap that nearly shipped a 3,400-line diff.** `ug_container.cpp` is **CRLF**,
and Python's `write_text` silently normalised the whole file to LF — `git diff
--stat` showed 1728 insertions / 1718 deletions for a one-line change. Caught it
before committing; re-applied via `read_bytes`/`write_bytes` with explicit
`\r\n`, giving the correct 12-insertions/1-deletion diff. **Check the diffstat
after any scripted edit to a shared-repo source file.**

**Deliberately not changed:** `pinVoiceBender * 0.05f` yields ±6 semitones at full
scale, while MidiToCv2's own comment says the voice bender is "hard-coded to 48
semitones (for MPE)". Those disagree — but a centred bend is now correctly zero
either way, so it is a separate question and not folded into a one-line tuning fix.

**No golden depended on the old behaviour:** E1 is 4/4 including
`voice_midi_note`, which reaches pitch through `SE Keyboard2` and legacy MIDI
rather than `PolyBender`.

**Side effects on this box:** REAPER launched three times, quit each time; not
running. TIDE_VST3 rebuilt Release from `tide/mac/V3-root-midicv`. A four-agent
recon workflow read SynthEditLib for the presenter/container APIs; its most useful
finding was the existing handle-diff precedent above.

**Next:** **E8**, the tuning constant — decide which module is the reference
(`signals.htm`, cited from `ug_midi_to_cv.cpp:117`), then correct whichever is
wrong; TIDE's own prefab default is ALLOWED, a stock module's mapping is GATED and
would move every SynthEdit patch, so the prefab is almost certainly what changes.
**E7** stays open as the underlying engine limitation, but nothing now waits on it.

**Branch/PR:** `tide/mac/V3-root-midicv`.

## 2026-08-18 — macos — A25: the NEXT-block check now actually runs, proven by probe (interactive session, Jeff directing)

**Did:** wired `scripts/check-next-block.py` into the `lint` job. It shipped with
**A20** and nothing ran it, so a NEXT block could send a run at an archived row
and CI would say nothing.

**All four parts, because three of them is worse than none.** A15's row records
the trap and this run demonstrates it rather than restating it: the Summary step
is what turns a red *step* into a red *job*. Wire only the step and it goes red
while the job still passes — a check that reports and does not gate.

1. the step, after `idrefs` — whole-tree, for the same reason that one is: a
   take-target goes stale when the row it names is **archived**, which is an edit
   to a *different* file than the one citing it, so a base-vs-head diff misses
   exactly the case that matters
2. `NEXTBLOCK: ${{ steps.nextblock.outcome }}` in the Summary's `env`
3. `echo "next-block: $NEXTBLOCK"` beside the other five
4. `"$NEXTBLOCK"` in the `for outcome in …` list that sets `fail=1`

**Result — the two-commit probe A25 asked for, run for real.** Commit 1 pointed
the `mac` NEXT row at **E2a**, which is archived:

```
links:      success
journal:    success
backlog:    success
provenance: success
id-refs:    success
next-block: failure      <- job FAILED, not merely the step
```

[run 32105947035](https://github.com/JeffMcClintock/TideSynth/actions/runs/32105947035).
Commit 2 removed the probe: all six `success`, job green —
[run 32106036402](https://github.com/JeffMcClintock/TideSynth/actions/runs/32106036402).

**Simulated locally before pushing anything**, by running the three whole-tree
checks against a clean tree and against a planted probe. That cost nothing and
meant the CI run confirmed a prediction rather than discovering a surprise.

**The row's access claim is confirmed as well, and it cuts both ways.** A25 said
only Jeff or an interactive session could push this, because the bot token
deliberately lacks `workflow` scope. This was an interactive session and the push
to `.github/workflows/**` was accepted. **So the wall is real and still stands for
a scheduled run** — the same wall that blocks **A12** and **B1**, which remain
un-takeable by any agent.

**Learned:** `gh run view --log` interleaves ANSI escapes and tab-separated
job/step prefixes, so grepping it for a Summary line finds the `echo` command
rather than its output. `sed 's/\x1b\[[0-9;]*m//g' | awk -F'\t' '$2=="Summary"{print $3}'`
gets the actual six lines. Worth keeping — reading a Summary block is the normal
way to check any of these lint steps.

**Next:** unchanged — **E7**, the polyphony question, with Jeff's rulings already
reducing it to "where do the jacks live". **A12** and **B1** stay blocked on the
token, and this run is the evidence for why: the push that worked here worked
*because* a human was driving it.

**Side effects on this box:** no builds, no REAPER, no plugin changes — this was
a CI-wiring run. Two CI runs consumed on the probe, deliberately.

**Branch/PR:** `tide/mac/A25-nextblock-lint` —
[#145](https://github.com/JeffMcClintock/TideSynth/pull/145).

