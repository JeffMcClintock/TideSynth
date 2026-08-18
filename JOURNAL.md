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

## 2026-08-18 — macos — V3 attempted: MIDI reaches the rack, but does not cross a patch cable (interactive session, Jeff directing)

**Did:** took **V3** — PLAN's last unproven v0.1 clause, "play it from the DAW's
MIDI". Built the rack module it needed, built the fixture, and measured. **The
MIDI half works. The rack half does not**, and the finding is precisely located
rather than "it didn't play".

**Result.**

```
v1-rack-midi.rpp  (4 cables, middle-C note on 0.500 s, off 1.200 s)
   peak=  -6.3 dBFS  rms= -17.0 dBFS   440.0 Hz, CONSTANT -- identical to the same rack with NO note

PROBE A  MIDI In -> MIDI-CV 2 -> Sound Out INSIDE the container
   Gate mean/100ms:  0 0 0 0 0  1 1 1 1 1 1 1  0 0 0 0 0 0 0 0     <-- tracks the note exactly
PROBE B  same, MIDI In module removed (redirector only)
   Gate mean/100ms:  0 0 0 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 0 0     <-- never opens
```

**The check Jeff asked for, and its answer.** He drilled into the MIDI In module,
found it structurally correct but apparently inactive, and asked for a check on
whether it was even receiving MIDI. `SynthEditSem/SynthEdit.cpp` now has one, and
it says **yes**:

```
TIDE: host MIDI reaching the rack - first message 8 byte(s), status 0x40
```

Status `0x40`, 8 bytes, is a **MIDI 2.0 UMP** channel-voice packet — GMPI's
`DT_MIDI2`, not legacy MIDI bytes. Worth knowing before anyone debugs the
downstream path expecting three-byte messages. A second line covers the case
`onMidiMessage` was already dropping in silence: MIDI arriving before the editor
has pushed the document (`if (rackPrepared)`). One print per instance per
condition, because that method is the audio thread.

**Why the check mattered more than it looks.** A rack wired to MIDI has FOUR
failure sites — host doesn't send, the wrapper declares no event input,
`onMidiMessage` never runs, the rack drops it — and from the outside all four are
one indistinguishable silence. Two ruled out by code reading (TIDE declares
`<Pin name="MIDI" datatype="midi"/>` at `SynthEdit.cpp:199`, so
`Processor_VST3.cpp:315` adds the event input bus), two by that line.

**A theory of mine that was WRONG, killed by its own negative control.** I
expected the `MIDI In` module to be the culprit: it models a hardware MIDI port
for the SynthEdit app, and `keyboard2/keyboard.xml:21` says a module needs its
DT_MIDI2 pin **unconnected** for `CreateMidiRedirector` to feed it, or "voices
never get allocated". PROBE B tests exactly that and its gate never opens.
**Jeff's design is right: the MIDI In module is required.** Both probes are kept
behind `build-prefabs.py --diagnostics` so the comparison re-runs in one command
instead of being retold.

**So what actually fails.** MIDI-CV 2's Gate is correct *inside its container*
and worth nothing *outside* it. Filed as **E7**, with the boundary drawn honestly:

- **Proven:** a two-module probe, MIDI PITCH → Output L — a jack with **no**
  stored default, so it would have passed anything that arrived — renders
  **−inf**. And the four-cable rack sits at 440.0 Hz, which is the *Oscillator's
  own* 5 V default, so both MIDI cables contributed exactly nothing.
- **Inferred, not measured:** that polyphony is the reason.
  `SE MIDI to CV 2` is `polyphonicSource="true"`/`cloned="true"`
  (`MidiToCv2.cpp:18`), so its container is a voice container and its outputs are
  polyphonic — the one structural difference from V1's cables, which do carry
  audio. **The row says this is a hypothesis and names the experiment that
  settles it** (cable a NON-polyphonic source out of a container: if that
  crosses, polyphony is the variable; if it does not, patch cables out of
  containers are broken more generally and V1's chain works for a reason nobody
  understands yet). S14's lesson — do not measure carefully against an assumed
  architecture — is why that is a test to run and not a conclusion to write.

**Learned — four things.**

1. **`MIDI In` is `modules_internal/MidiInGui.cpp`, id `MIDI In`, and its audio
   output `MIDI Data` is pin 1, not pin 0.** Pin 0 is the GUI `Activity` **input**.
   The combined plug list interleaves GUI and Audio pins, so the `<Audio>` block's
   declaration order is NOT the saved pin index. Verified by making SynthEditCL
   resolve the name and print the index rather than by counting the XML.
2. **A jack's hit-area is a few pixels, and TIDE has no undo.** A drag starting
   3 px off centre grabs the module BODY and moves it — that happened, put the
   Oscillator half off-canvas, and cost a full re-place of the rack. Cable in an
   order that grabs each jack *before* any cable is drawn near it.
3. **The prefab staging step copies but never prunes.** Deleting a prefab from
   `TideModules/prefabs/` leaves it in the bundle, so the two probes kept showing
   up in the browser after they were removed from the generator. Delete by hand,
   or a diagnostic ships.
4. **Every generator run rewrites all the prefab handles** (SynthEditCL assigns
   fresh ones), so any regeneration dirties files it did not mean to change.
   Harmless — a placed prefab is copied into the host document — and V1's
   fixtures were re-measured at −6.3/−17.0 and −inf after the rebuild to confirm.

**Also, in passing:** reopening `v1-rack.rpp` showed both patch cables drawn and
the mixer at −6.3 / RMS −13.5 with the transport stopped — so V1's result holds
for **live playback**, not only offline render. That was never explicitly
observed before.

**Next:** **E7**, and it starts with the one experiment named in its row, not with
the prefabs or the MIDI path — both are measured good. **V3** is `BLOCKED(E7)`
with everything it built already landed, so the day E7 clears, V3 is a re-measure
of a fixture that already exists.

**Side effects on this box:** REAPER launched five times interactively and several
times headlessly by the render scripts; it is not running. One accidental
keystroke opened REAPER's "Dynamic split items" dialog (cancelled, undone, MIDI
item verified intact afterwards) — the startup nag had been covering the FX
button, which is what sent those clicks astray. The two PROBE prefabs were
deleted from the installed bundle and from the build tree. The installed VST3 is a
**Release** build of this branch. No repo but TideSynth and SynthEdit changed.

**Branch/PR:** `tide/mac/V3-midi-findings`; SynthEdit
[#46](https://github.com/JeffMcClintock/SynthEdit/pull/46).

## 2026-08-18 — macos — E2a and V1 flipped DONE and archived (state update, interactive)

**Did:** observed that [#140](https://github.com/JeffMcClintock/TideSynth/pull/140)
merged and did the STEP 4 chore its rows were waiting on — no code, rows only.

**Result.** **E2a → DONE**, **V1 → DONE**, both archived verbatim into
[BACKLOG-DONE.md](BACKLOG-DONE.md) with the landing note and the measured numbers
prepended (peak −6.3 dBFS, rms −17.0 dBFS, 440.0 Hz left-only). Every linked PR
on E2a has now merged: SynthEdit#45, GMPI_Wrappers#8, #139, #140. **E2 →
`TODO`** in the same edit: its `BLOCKED(V1)` premise — "no point authoring a
fuller module library for a plugin that cannot yet keep its patch across a host
save" — is retired by measurement, and the rule in this file is that
`BLOCKED(<id>)` lifts when `<id>` is DONE. The `mac` NEXT row now points at
**V3** with E2a/V1 redirected to the archive. All repo checks green:
`check-backlog-diff` reports *2 row(s) archived, verified verbatim*,
`check-id-refs`, `check-links` and `check-next-block` clean.

**Learned:** archiving a row is **prepend-only**. `check-backlog-diff.py`
requires the base version's Item text to survive *verbatim* inside the new one,
so the landing note goes in front and the old text stays — including clauses that
have gone stale, like E2a's "held at IN-REVIEW because #140 is open". That reads
oddly out of context, which is why both archived rows label the tail "Row as it
stood at IN-REVIEW follows, kept for the record". Do not tidy it; the check
treats a shrink as a rewrite and fails it.

**Next:** **V3** — "play it from the DAW's MIDI", the one clause of PLAN's v0.1
acceptance test nothing tracked. Needs no GUI session: a MIDI item is plain text
inside a `.rpp`, so a fixture can be hand-edited from
`tests/hosts/v1-rack.rpp` and measured with `scripts/render-and-measure.py`.
**E6** is still open and nothing here depended on it.

**Side effects on this box:** all fourteen local repos fetched and
fast-forwarded at Jeff's request before this edit — only `JUCE`
(`620b879ef7` → `076454f21e`) and `synthedit-website` (`74b0ded` → `e3c7f4f`)
moved; everything else was current. No merges, no rebases, no tracked file
touched by the sync. `AlphaBlender` is parked on branch `DrawOnImage` and
`VST_SDK` is detached at the `VST SDK 3.7.12` tag — both left alone
deliberately. `GMPI` has a stale local `release_1_5` fully merged into `main`,
noted but not deleted. Only TideSynth changed.

**Branch/PR:** `tide/mac/e2a-v1-done`.

