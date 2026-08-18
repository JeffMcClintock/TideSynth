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

