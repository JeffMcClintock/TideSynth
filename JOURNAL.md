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

## 2026-08-18 — macos — V3 PASSES: the rack plays the DAW's MIDI, and the bug was missing type converters (interactive session, Jeff directing)

**Did:** found why the MIDI-gated rack was silent, and it was not MIDI. **TIDE had
no type converters linked**, so a whole class of connections was silently dead.
Jeff's one-line observation is what located it: *"the library will automatically
insert converters when needed. So long as the converter is linked in of course."*

**Result — V3's Accept is met.** `tests/hosts/v3-midi-gate.rpp`, one middle-C note
on at 0.500 s and off at 1.200 s, gate patched from MIDI rather than left at its
open default:

```
peak per 100 ms   0.000 x5   0.484  0.345 0.341 ... 0.340   0.044   0.000 x6
0.05-0.45 s       silent
0.60-1.10 s       440.0 Hz
1.35-1.95 s       silent
```

Silence, note, silence.

**The mechanism, and why it was invisible.** `ug_base.cpp:1751` builds a converter
id like `SE <From>To<To>` whenever two connected pins differ in datatype, then
calls `ModuleFactory()->GetById()`. On a miss it does:

```cpp
assert(false); // invalid connection.
return;
```

In a **Release** build the assert compiles out, so the function just returns and
**the connection is silently abandoned**. The editor still draws the cable. The
DSP never carries it. There is no warning, no log, and nothing in the saved
document to distinguish it from a working cable.

`Converters.cpp` builds into a separate `Converters` target as a loadable
`Converters.sem`, and TIDE links statically with no scan (S1a) — so **`SE
BoolToVolts` did not exist in TIDE**, and `SE MIDItoGate2`'s `BoolOutPin` gate
could never reach an audio-rate jack. Fixed the way `MidiToGate` was: the `.cpp`
joins `SynthEditSem`'s source list and `Converters.xml` is staged.
`my_type_convert.cpp` has to come with it — `Converters.cpp` instantiates
`SimpleConverter<From,To>` for every pair and each calls
`myTypeConvert<From,To>()`, whose specialisations live in that other translation
unit; without it the link fails on a wall of undefined symbols.

**This is worth more than the row it unblocked.** It was not one module's bug: any
mixed-datatype cable a user drew in TIDE went quietly dead. Bool to volts, float
to volts, volts to float — all of it.

**Three of my own hypotheses died on the way, and the pattern in them is the
lesson.** I blamed, in order: the ADSR's gate threshold (measured false — it opens
identically at 1 V and 10 V); `MidiToGate2` not being MIDI-2 aware (false — it
receives the UMP, decodes NoteOn, reads note 60 and sets `pinGate = true`, traced);
and `setSleep(true)` truncating the gate (plausible, still untested, and now
irrelevant). **Every one of those put the fault in a module because the silence was
measured at the end of a chain.** The actual fault was in the *wire*, which is the
one place I never instrumented. Third time this session that a downstream silence
got attributed upstream — after `gmpi_render_audio` (E6, the tool) and
"MIDItoGate2 emits no gate" (the wire again).

**What is still missing: PITCH.** The shipped MIDI prefab is now the monophonic
`SE MIDItoGate2`, which has no pitch output, so every note sounds at the
Oscillator's own 5 V default of 440 Hz. Pitch needs `SE MIDI to CV 2`, which is
`polyphonicSource`/`cloned` and so still blocked by **E7** — kept runnable as
`PROBE C`. **So "notes start and stop with the DAW's MIDI" is done; "play a tune"
is not.** V3 is `IN-REVIEW` rather than claimed DONE because whether that satisfies
PLAN's clause is Jeff's call, not mine.

**No regressions:** all three earlier host fixtures unchanged (−6.3/−17.0, −inf,
−6.3/−17.0) and E1 4/4 after linking the converters.

**Side effects on this box:** REAPER was not launched — the fixture was already
saved from the previous round, so every measurement here was a headless render.
The temporary `MidiToGate2` trace from the previous entry stayed reverted;
SynthEditLib is untouched and clean. TIDE_VST3 rebuilt Release, universal
(arm64 + x86_64) — the converter link error surfaced on x86_64 first, so a
single-arch build would have hidden half of it.

**Next:** **E7**, now purely the polyphony question, with Jeff's rulings already
reducing it to "where do the jacks live".

**Branch/PR:** `tide/mac/E7-miditogate2-finding`; SynthEdit
[#48](https://github.com/JeffMcClintock/SynthEdit/pull/48).

## 2026-08-18 — macos — MIDItoGate2 traced: it IS MIDI-2 compatible and DOES set the gate (interactive session, Jeff directing)

**Did:** Jeff asked whether `SE MIDItoGate2` is MIDI 2.0 aware. Answering it
properly overturned a claim I had already written into **E7**, so this entry is
mostly a correction.

**E7 said `SE MIDItoGate2` "emits no gate at all". That was wrong** — an inference
from a silent rack, never a direct measurement. It is fully MIDI 2.0 compatible
and it does set the gate. Two independent proofs.

**By code.** `MidiConverter2::processMidi` opens with an explicit pass-through for
MIDI 2.0 input (`modules/se_sdk3/mp_midi.h:966` — *"MIDI 2.0 messages need no
conversion — pass through and return"*), and `MidiToGate2::onMidi2Message`
requires `ChannelVoice64`, which is exactly the message type TIDE forwards
(status `0x40`). So there is no 1.0-versus-2.0 mismatch anywhere in the path.

**By trace**, added to `modules/MidiPlayer2/MidiToGate.cpp` at Jeff's request and
**reverted unbuilt afterwards** — a temporary trace that got committed is issue
[#87](https://github.com/JeffMcClintock/TideSynth/issues/87), and this one was
not going to repeat it:

```
MTG2: onMidiMessage pin=0 size=8 bytes=40 90 3c 00 isMidi2=1
MTG2: onMidi2Message msgType=4 status=9 (ChannelVoice64=4 NoteOn=9)
MTG2: NOTE ON note=60 -> pinGate=true triggerCounter=22
MTG2: trigger pulse ended at sample 6 -> setSleep(true)
MTG2: onMidiMessage pin=0 size=8 bytes=40 80 3c 00 isMidi2=1   <- NoteOff, status=8
```

It receives the UMP, decodes it, reads **note 60** — the fixture's middle C — and
sets `pinGate = true`. `triggerCounter=22` is the 0.5 ms pulse at 44.1 kHz,
correct. The intervening `status=6` and `status=0` messages are other MIDI 2.0
types it correctly ignores.

**So where is the break?** The rack downstream measured **zero for the whole
render**, peak −inf, including the samples while the gate was set. The gate is set
on the pin and never arrives at the Envelope's ADSR.

**This does NOT overturn "a monophonic module's cable is live."** The cable
demonstrably drove the GATE jack from its default of 10 down to 0 — that is why
the rack went silent rather than droning. **The cable crosses; the VALUE does
not.** Two concrete suspects, neither tested:

1. `pinGate` is a `BoolOutPin` and the jack is an audio-rate `SE Patch Point in`,
   so a static/event bool may not drive an audio-rate signal at all. That would
   also explain why `--connect $mtg:Gate $pp:Input` is *accepted* by SynthEditCL
   while carrying nothing — the connection is legal and inert.
2. `setSleep(true)` fires 22 samples after note-on while the gate is still
   logically HIGH, so a sleeping module may stop driving its output.

**Test (1) first:** it is a datatype question answerable with SynthEditCL alone,
no GUI — put a bool-to-volts conversion or a `Multiply` between the two and see
whether the gate arrives. If it does, the interim prefab just needs that module
inside it.

**Learned — the shape of my own error, because it repeated.** Twice now I have
turned a *silent downstream* into a claim about an *upstream module*: first
"`gmpi_render_audio` says the rack is silent" (which was the tool), now
"MIDItoGate2 emits no gate" (which was the wire). Silence measured at the end of a
chain names the chain, not a link in it. The fix both times was to measure at the
suspected link instead — a `Sound Out` inside the container, or a trace in the
module. **`build-prefabs.py --diagnostics` exists for the first; a throwaway
`fprintf` is fine for the second, as long as it is reverted.**

**Side effects on this box:** the SynthEditLib trace was reverted and that repo is
clean on `main`; TIDE_VST3 was rebuilt from the reverted source so the installed
plugin matches the tree. REAPER was not launched — the fixture was already saved,
so this was a headless render. GMPI (`9541b1d -> b9e4f92`) and GMPI_Wrappers
(`e2eeedc -> ea2e357`) arrived in the routine sync, both from
`tide/win/standalone-correctness-fixes`; TIDE was rebuilt against them and all
three host fixtures plus E1 4/4 re-measured unchanged, so they disturb nothing
here.

**Next:** **E7**, unchanged in scope — Jeff's rulings still reduce the design to
"where do the jacks live". The interim's remaining question is now suspect (1)
above, which is a 30-second SynthEditCL test rather than an investigation.

**Branch/PR:** `tide/mac/E7-miditogate2-finding`.

## 2026-08-18 — macos — PROBE D: MIDI does exit the MIDI In module (interactive session, Jeff directing)

**Did:** answered one question Jeff asked of the previous entry's evidence — *did
we prove that MIDI exits the `MIDI In` module?* — and the honest answer was **no**.
Fixed that with one more probe, then tidied the fleet's stale branches.

**First, a bookkeeping failure worth more than the fix.** Several commits from the
previous session never reached either default branch: I kept pushing to branches
whose PRs had **already merged**, so `TideSynth/main` stopped at "record the
confirmed cause" and `SynthEdit/master` stopped at the first commit of the V3
series. Everything after that — the `MidiToGate` linking, the one-list staging
fix, `TIDE_STATIC_EXTRAS`, Jeff's rulings, and my own correction of a wrong
explanation — sat on deleted branches. Recovered here from the branch tips
(`34503ea21`, `25216c1`, still in the object store). **The lesson: `git push`
succeeding says nothing about whether a PR is still open to carry it.** Check the
PR state before pushing a follow-up, or push to a fresh branch.

**One silver lining:** the wrong explanation never reached the repo of record
either, so `main` was never publishing it. It is re-landed here in corrected form
only, rather than as a wrong commit followed by a fix.

**The wrong explanation, since it is a plausible trap.** I blamed
`#if GMPI_IS_PLATFORM_JUCE==1` around `INIT_STATIC_FILE(MIDItoGate)` for
`SE MIDItoGate2` being absent from TIDE. Wrong twice: `INIT_STATIC_FILE(ADSR)` is
inside that same block while ADSR works fine (TIDE's ADSR is the legacy
`ug_adsr.cpp` `REGISTER_MODULE_1` one, reached through the legacy table), and
adding an unconditional `INIT_STATIC_FILE(MIDItoGate)` **fails to link** —
`se_static_library_init_MIDItoGate()` undefined, the linker saying the object is
not in the library. **The real reason:** `modules/MidiPlayer2/MidiToGate.cpp` is
in a `SynthEditLib/CMakeLists.txt` source list belonging to a **separate
`MidiPlayer2` target**, so it builds as a loadable module.  `MidiToGate.o` lands
under `MidiPlayer2.build/`; `MidiToCv2.o`, genuinely in the library, lands under
`SynthEditLib.build/`. TIDE links statically and has no scan (S1a). Fixed with no
GATED change by adding the `.cpp` to `SynthEditSem`'s source list — E2a's
`RectangleGui.cpp` pattern — and **that same layout is what S8 measured**, so S8's
own "separately-loaded module" suspicion was right all along and its row now says
so.

**Why the earlier pair was not enough.** PROBE A (MIDI In present **and** cabled
to MIDI-CV 2) gates on the note; PROBE B (MIDI In absent) never does. That reads
like proof but changes **two** variables at once, so all it supports is "MIDI In
plus its cable delivers MIDI". The live alternative was that the module's mere
**presence** makes the container a MIDI destination and `ug_container`'s
redirector feeds MIDI-CV 2 directly — `ug_base.cpp:2859` scans for the first
DT_MIDI2 input pin, so that is not a fanciful reading.

**Result — PROBE D holds the module present and leaves the pin UNCONNECTED.**

```
A  present + cabled    Gate  0 0 0 0 0 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0
D  present, uncabled   Gate  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
B  absent              Gate  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
```

A and D differ in **only** the cable, and only A gates. **So MIDI travels through
the patch cable and does exit the `MIDI Data` output.** The redirector alternative
is dead.

**What that buys beyond bookkeeping.** `SE MIDItoGate2` is wired from `MIDI In`
in exactly the way MIDI-CV 2 is, so it **is** being fed MIDI — its silence is
internal to the module rather than a delivery problem. That removes the whole
delivery half of the suspect list and leaves two lines: the `setSleep(true)` /
`subProcessNothing` path in `MidiToGate2::subProcess`, and the cross-class
`setSubProcess(&MidiToGate::subProcessNothing)` sitting inside `MidiToGate2` —
both at `modules/MidiPlayer2/MidiToGate.cpp:222`.

**Learned:** an A/B that moves two variables is worth exactly as much as its
weaker leg. Both earlier probes were real measurements and the conclusion drawn
from them was still unsupported; it took a third arrangement to make the claim
true. Cheap, too — one prefab, one placement, one render, no cabling.

**Side effects on this box:** REAPER launched once, exited on its own; not
running. The diagnostic prefabs were removed from `TideModules/prefabs/` and from
the installed bundle again, which the `Probe*.synthedit` gitignore rule now makes
harder to get wrong. Stale merged local branches deleted at Jeff's instruction:
`TideSynth/tide/mac/e2a-v1-done` and `GMPI/release_1_5`. Everything else on those
repos was left alone — `SynthEdit`'s `Release_V14`/`Release_V15`, `gmpi_ui`'s
`release_1_5`, `AlphaBlender`'s `offscreen` and `JUCE`'s `master` are release or
unmerged branches, not stale.

**Next:** unchanged — **E7**, which Jeff's rulings have already reduced to "where
do the jacks live". The `MidiToGate2` thread is now a two-line code question
rather than an investigation.

**Branch/PR:** `tide/mac/E7-probe-d`; SynthEdit
[#47](https://github.com/JeffMcClintock/SynthEdit/pull/47).

