# The TIDE Rack module set — MVP and beyond

Produced 2026-08-18 (windows, interactive) alongside
[competitive-review.md](competitive-review.md).

**This answers a question the tree has had open since 2026-08-06.**
[module-enumeration.md](module-enumeration.md) §7.2 asks *"Which modules belong
in TIDE's list? Neither existing arm is right"*, and BACKLOG **E2**'s first job
is *"define the naming and I/O conventions for a module Container, then build
the rest of a first curated set"*. This document proposes both. **It decides
nothing** — E2 and E16 are where the work is agreed.

---

## 1. The constraints that shape the list, before any taste is applied

| Constraint | What it removes from a normal module set |
|---|---|
| **2** — DAW owns I/O | audio-device and MIDI-device modules. Note the S8 correction: `ug_soundcard_out` **stays**, because in a plugin it is the `ISpecialIoModuleAudioOut` seam the host writes through. Relabel, do not delete. |
| **3** — sandbox-safe, no filesystem | **samplers, wave players, file-loaded wavetables, IR loaders, "browse for…"**. `ug_wave_player`, `Sample Oscillator2`, `ug_wave_recorder`, `WaveRecorder2` are out. |
| **7** — fixed set, compiled in | no third-party modules, so **the shipped set is the whole product**. **This is the constraint that most changes the answer — see §4.** |
| **9** — lowest common denominator | the AUv3 memory ceiling (~300 MB 32-bit / ~360 MB 64-bit per extension instance) applies on *every* platform, so large embedded wavetable/IR banks are constrained everywhere. |

**The measured good news:** the DSP for nearly all of this already exists.
`C:\SE\SynthEditLib` carries **75 `ug_*` DSP modules** plus a modern SEM set
(`EnvelopeAdsr`, `SVFilter2`, `VaFilters`, `Delay3`, `Reverb`, `StepSequencer`,
`Waveshapers`, `BPMClock3`, `Arpeggiator`, `Unison`, …). **The module set is
predominantly a Container-authoring and panel-art job, not a DSP-writing job.**

**The measured bad news:** what is *compiled into the shipped binary* is a
different question, and the two arms of `initialise_synthedit_modules`
(`UgDatabase.cpp:1063-1155`) register different sets. S8 measured
`OscillatorNaive` at **zero symbols** in the Release binary. **Verify presence in
the binary before assuming any row below is free.**

---

## 2. The conventions first — because they are load-bearing

A module set without an I/O convention is 40 incompatible modules. Reaktor
Blocks solved this; the full extraction is in
[blocks-connection-scheme.md](blocks-connection-scheme.md). Proposed for E2:

1. **One signal range, roles by convention.** Every port carries the same range
   (Blocks uses audio-rate −1..+1 for *everything*, pitch and gate included);
   meaning comes from the port's declared role. This buys "any output into any
   input, with predictable results", which PLAN.md already commits to.
2. **`Mod A` / `Mod B` on every module**, with per-parameter depth on the panel.
   **Highest-leverage decision in the set** — it removes the
   attenuverter-and-mixer tax Eurorack charges for ordinary modulation. Bitwig's
   equivalent is "pre-cords".
3. **Gate on positive zero-crossing, velocity encoded as rise height.**
4. **Expose every filter mode as its own output** (`Out`, `LP`, `BP`, `HP`).
5. **Pitch convention — DECIDE, do not default.** Blocks uses 0.1/octave over a
   normalised range; Eurorack says **1V/octave**. Check what SynthEdit's existing
   modules do and match *that*. Inventing a third convention is the only
   genuinely bad outcome.
6. **Make the CV mixer arithmetically exact.** Hardware needs a "precision adder"
   because mixing pitch CV inaccurately detunes sequences; in software exactness
   is free, so take it.

### The host boundary — copy Voltage Modular, not VCV

VCV puts audio and MIDI I/O in *placeable modules* (the separate **VCV Core**
plugin: Audio, MIDI-CV, MIDI-CC, MIDI-Gate, CV-MIDI…). **Cherry Audio's Voltage
Modular instead has a fixed, non-removable I/O Panel at the top of the cabinet.**

**For TIDE the fixed strip is clearly right.** Constraint 2 means host I/O is not
optional and cannot be misconfigured, so making it a module the user can delete
or forget to add is a trap. Bitwig's naming is the cleanest model to copy:
`Pitch In`, `Gate In`, `Velocity In`, `CC In`, `Audio Out`.

*(This also removes "the user deleted the output module" as a support case.)*

### Two hardware conventions TIDE should deliberately **not** copy

- **Multiples.** Eurorack needs them for electrical fan-out. **Two of the five
  reference sets — Bitwig Grid and Reaktor Blocks — ship no mult at all**,
  because outputs fan out freely. Buffered mults rank #19 and #20 on ModularGrid
  purely as a hardware fix. Ship none, or one cosmetic one. *Confirm SynthEdit
  permits one-output-to-many-inputs first.*
- **Ubiquitous standalone attenuverters** — unnecessary if convention 2 lands.

---

## 3. The evidence — what the reference sets actually contain

Measured across **VCV Fundamental (37 modules)**, **Reaktor Blocks Base (24)**,
**Bitwig The Grid (~180–245 across 16 categories)**, **Voltage Modular Nucleus
(22)** and **Softube Modular (7 Doepfer + ~30 stock)**, plus three semi-modulars
(Moog Mother-32, Behringer Neutron, Make Noise 0-Coast).

**The hard intersection — present in all five software sets *and* the hardware:**

> VCO · VCF · VCA · ADSR · LFO · noise · sample & hold · audio mixer ·
> CV attenuverter/offset/mixer · step sequencer · slew/glide · host I/O

**Twelve functions.** That is the irreducible core, and it is the MVP.

**Near-intersection (4/5):** crossfader, oscilloscope, delay, clock/divider.

**Explicitly NOT in the intersection:** reverb (1/5), compressor (0/5),
chorus/phaser (1/5), wavetable (2/5), envelope follower (2/5), wavefolder (2/5),
sampler/granular (1/5).

Three findings worth carrying forward:

1. **Nobody complains about missing oscillators or filters.** Every recorded
   complaint is about (a) time-based FX and (b) gate/clock/comparator plumbing.
2. **VCV's own conclusion was that the gap was utilities.** Roughly 15 utility
   modules were added in VCV Rack 2.2.0 after years of user pressure —
   Rescale, Logic, Compare, Gates, Process, Mult, Random Values, Push…
3. **Six of ModularGrid's top twenty modules are pure utilities** — Maths (#2),
   Pamela's NEW Workout (#4), Quadratt (#7), Quad VCA (#8), Buff Mult (#19/#20).

---

## 4. The finding that changes the answer

**Every reference set fills its gaps with a paid tier or a store. TIDE has
neither.**

- VCV Fundamental has no reverb, chorus, flanger or compressor — those are
  **VCV Pro** or third-party library modules.
- **Blocks Base ships no delay and no reverb at all** — they are in the paid
  **Blocks Primes** ($99).
- Voltage Nucleus omits quantizer, logic, comparator and sequential switch —
  the Cherry Audio store sells them.
- Softube's base set has no reverb and no scope; the store sells the rest.

Constraint 7 means TIDE cannot punt. **So "ship the intersection" is the wrong
rule for this product.** TIDE's fixed set must be closer to the *useful union*
than to any single starter set — specifically, it must include the FX layer that
every competitor defers to a paid tier.

**This is the single most important conclusion in this document.**

---

## 5. List A — the MVP set

### Tier 0 — v0.1 acceptance test (3 modules, already scoped as E2a)

| # | Module | Function | Primitive | Notes |
|---|---|---|---|---|
| 1 | **Oscillator** | the sound source | **`SE Oscillator4` ("Oscillator HD")** | **RULED 2026-08-21: Oscillator HD is the one TIDE ships.** Already compiled into the plugin by E2c (`SynthEditSem/CMakeLists.txt`), so this is **not blocked** — S8's zero-symbol finding is about `OscillatorNaive`, a different module, and does not gate the MVP |
| 2 | **Envelope (ADSR)** | shapes it | `ug_adsr`, `EnvelopeAdsr` | exists |
| 3 | **Output** | reaches the host | `ug_soundcard_out` seam | authored from scratch — existing `Output.seprefab` has **no Sound Out** |

### Tier 1 — the hard intersection (+9 → 12 total)

Everything here appears in all five reference sets.

**CORRECTED 2026-08-21 by Jeff.** This paragraph used to say that below this
line a user *"can make a sound but not music"*. That is wrong, and it was the
sentence arguing for Tier 2: *"Tier 1 absolutely can make music. Plenty of real
hardware products ship with only this type of functionality."* **Tier 1 is the
ruled MVP set** — see [decisions.md](decisions.md).

| # | Module | Function | Primitive | Note |
|---|---|---|---|---|
| 4 | **I/O modules** | Trigger/Gate/Pitch/Velocity in, audio in/out | `MidiToCv2`, soundcard seams | **RULED 2026-08-21: mandatory and not deletable, but they ARE rack modules** — placed and movable like any other. Supersedes this row's old "not a placeable module". Four separate signals, not a combined gate — see the gate convention in [decisions.md](decisions.md) |
| 5 | **Filter (SVF)** | multimode, all outputs exposed | `ug_filter_sv`, `SVFilter2`, `VaFilters` | |
| 6 | **VCA** | level by CV | `ug_vca` | **must have a lin/exp switch** — see §7 |
| 7 | **LFO** | cyclic modulation | `ug_oscillator2` at low rate | host-tempo sync + reset input |
| 8? | **Mixer** (audio + CV) | sum several signals | `ug_adder2` | **UNDECIDED 2026-08-21** — Jeff: *"not critical as a distinct module since SE support multiple cables to one destination with automatic summing. Quite useful though i guess."* TIDE's patch cables **fan in with automatic summing**, so a mixer is convenience rather than capability. In or out is the one open question in the MVP list |
| 9 | **Noise** | white/pink | `ug_random` | pairs with S&H — a canonical compound |
| ~~10~~ | ~~Sample & Hold~~ | stepped random | `ug_sample_hold` | **CUT from the MVP 2026-08-21** — *"not critical"*. Second-wave expansion |
| 11 | **Attenuverter + Offset** | scale, invert, offset CV | `ug_multiplier`, `ug_adder2` | **one module, not two** — every set bundles them |
| ~~12~~ | ~~Slew / Glide~~ | portamento, smooth CV | `ug_filter_1pole_lp` | **CUT from the MVP 2026-08-21 — already built into MIDI-CV2**, verified: `CVoiceList.cpp` drives its constant-rate glide via `HC_GLIDE_START_PITCH` into `MidiToCv2`'s `pitchInterpolator`. Not needed standalone |

### Tier 2 — the credible first release (+10 → 22 total)

Tier 2 is where §4 bites: several of these are *not* in the intersection and are
here anyway, because TIDE has no store to defer them to.

| # | Module | Function | Primitive | Why |
|---|---|---|---|---|
| 13 | **Step sequencer** | the melodic engine | `StepSequencer` | 5/5 sets |
| 14 | **Clock** | host-tempo pulses, divide/multiply | `BpmClock3`, `ug_logic_decade` | 4/5. **VCV's lack of a standalone clock is a documented complaint** — do not repeat it. Host transport makes this cheap |
| 15 | **Quantizer** | snap CV to a scale | `ug_quantiser` | turns random into musical; highest value per line of code |
| 16 | **Scope** | *see* the signal | `Scope3XP` | 4/5. **Teaching device** — with no manual, this is how a beginner learns what CV is |
| 17 | **Crossfader** | blend two signals | `ug_cross_fade` | 4/5 |
| 18 | **Delay** | time effect, tempo-synced | `ug_delay`, `Delay3` | 4/5 |
| 19 | **Reverb** | space | `Reverb`, `Spring2` | **only 1/5 — included anyway per §4.** Algorithmic only, never convolution |
| 20 | **Waveshaper / Wavefolder** | west-coast timbre, distortion | `Waveshapers`, `SoftDistortion`, `ug_clipper` | 2/5, but the 0-Coast proves a folder can *substitute* for a filter |
| 21 | **Logic + Comparator** | gate arithmetic, thresholds | `ug_logic_gate`, `ug_comparator` | **one module with a selectable operation, not eight.** Add **window and hysteresis/Schmitt modes** — these are the most-cited hand-built gaps |
| 22 | **Sequential switch** | route between signals | `ug_switch`, `Switches` | **must hard-switch. VCV's crossfades, and users complain** |

**So: 12 modules is the floor for playable, 22 for credible.** For calibration
Voltage gives away 22 and Blocks Base 24 — but both have a store behind them.

---

## 6. List B — nice-to-haves, ranked for the backlog

Ranked by ModularGrid popularity, presence in reference sets, and recorded
demand.

| Rank | Module | Function | Effort | Note |
|---|---|---|---|---|
| 1 | **Function generator (Maths-style)** | slope/AD/LFO/attenuverter/mixer in one | medium | **#2 on ModularGrid.** The single most-used module in Eurorack; self-cycling makes it three modules at once |
| 2 | **Macro / host-automation control** | expose a knob to the DAW | medium | **this is BACKLOG V2** and a differentiator — see §8 |
| 3 | **Polyphony / voice allocator** | polyphonic racks | medium | `ug_voice_splitter`, `PolyphonyControl`. **Decide the model before freezing the list** — see §8 |
| 4 | **Clock modulator (Pamela-style)** | divisions, euclidean, per-output patterns | medium | **#4 on ModularGrid** |
| 5 | **Granular / buffer processor** | Clouds/Beads-style textures | high | **#1 and #5 on ModularGrid.** Record the *host input* into RAM — fully sandbox-safe, and a genuine differentiator rather than a workaround |
| 6 | **Multi-model macro-oscillator (Plaits-style)** | many synthesis models in one | high | **#3 on ModularGrid.** More variety per byte than a user-wavetable system, and no files |
| 7 | **Quad VCA / quad envelope** | multi-channel level and shaping | low–medium | #8 and #13 on ModularGrid |
| 8 | **Chorus / phaser / flanger** | movement | low–medium | 1/5 sets — included per §4 reasoning |
| 9 | **Low-pass gate** | west-coast dynamics | low | with the wavefolder, substitutes for VCF+VCA (0-Coast) |
| 10 | **Ring modulator** | classic timbre | low | `ug_multiplier` — nearly free |
| 11 | **Envelope follower** | audio → CV | low | `ug_peak_det`. **Worth promoting** — the host feeds TIDE audio, so a follower is unusually relevant here |
| 12 | **Stepped random / counter** | generative CV | low | `ug_random`, `ug_logic_shift`. Clep Diaz is #10 |
| 13 | **Compressor / limiter** | dynamics | medium | **0/5 sets** — pure gap-fill |
| 14 | **Wavetable oscillator** | modern timbre | medium | **constrained** — `WavetableOsc` loads by URI, so tables must ship in-bundle and count against the memory ceiling |
| 15 | **Arpeggiator** | pattern from held notes | low | `Arpeggiator` exists |
| 16 | **Octave / transpose** | pitch utility | low | |
| 17 | **Manual gate/trigger buttons** | performance | low | |
| 18 | **Unison / super-saw** | thickness | low | `Unison`, `ThickOsc` |
| 19 | **Drum voices** | percussion | medium | **synthesised only** — no samples |
| 20 | **Spectrum analyser** | visualisation | low | `FreqAnalyser2/3` |

### Explicitly excluded, with the reason

| Wanted | Why not |
|---|---|
| Sampler / wave player | constraint 3 — arbitrary file paths |
| Convolution reverb with user IRs | constraint 3. *`ImpulseResponse` takes a **blob**, not a path, so built-in IRs are legal* — but the memory ceiling limits how many |
| Audio/MIDI device selection | constraint 2 |
| Multiples | software fan-out makes them redundant (§2) |
| User-installable third-party modules | constraint 7 |

---

## 7. Three implementation rulings the evidence already settles

1. **VCA response: linear for CV, exponential for audio level — ship a switch.**
   Ears are logarithmic, so exactly one stage in the chain should be
   exponential. Linear is correct for modulating CV; exponential for volume
   (otherwise there is very little resolution across much of the range).
   Softube's Doepfer A-132-3 is explicitly a "Dual Linear/Exponential VCA". This
   is not a luxury.
2. **Sequential switches must hard-switch, not crossfade.** VCV's Sequential
   Switch crossfades, and users substitute other modules for clean CV switching.
   A documented mistake, free to avoid.
3. **Bundle logic, don't scatter it.** One module with a selectable operation
   (AND/OR/XOR/NOT) beats eight modules, and adding **window comparator** and
   **Schmitt-trigger/hysteresis** modes to the Compare module closes the single
   most-cited "I had to build this by hand" gap.

---

## 8. What is not a module, and matters more than most of them

**Host automation (V2).** [competitive-review.md](competitive-review.md) §3 shows
the field has settled: RNBO, plugdata and Bitwig's Grid all expose parameters
**declaratively from the patch, with no panel required**, while Reaktor and Max
for Live require a UI control first — and Reaktor's automation-ID holes are the
predictable consequence. TIDE should follow the declarative camp. For a product
whose premise is *living inside the host*, this outranks most of List B.

**Polyphony model — decide before freezing the list.** VCV solved it with
poly-cables plus four utility modules (Split, Merge, Sum, Viz); Bitwig solved it
with per-voice instantiation. **Reaktor Blocks is architecturally monophonic
between Instruments**, which is a real weakness TIDE could beat outright. The
choice changes whether four utility modules exist at all, so it belongs before
E2 authors anything.

---

## 9. What this document does not settle

1. ~~The pitch convention~~ — **ANSWERED 2026-08-21**: the signal is SynthEdit
   pitch (0.5 = 440 Hz, 0.6 = 880 Hz) and volts are a **display convention
   only**. Verified in `ug_oscillator2.cpp:375`. See [decisions.md](decisions.md).
2. **Whether `Mod A`/`Mod B` is adoptable** in SynthEdit's Container model at
   acceptable cost. The MVP list's shape depends on the answer.
3. ~~The polyphony model~~ — **ANSWERED 2026-08-21**: SynthEdit's existing one.
   Modules are always monophonic and the runtime clones them per voice on the
   DSP graph only; the user sets a voice count on the MIDI-CV rack module.
   **One live consequence**: whether cloning crosses a rack Container boundary
   is E7's measured problem, and the I/O-modules ruling puts it on the MVP's
   critical path.
4. **Which primitives are actually linked** per module in the shipped binary.
   S8 has one measurement (`OscillatorNaive`: zero); nobody has done the rest.
5. **Panel authoring cost per module** — unknown until the first few are built,
   and it is the real schedule risk, not the DSP.

---

## 10. Sources

Reference-set contents: [VCV Fundamental](https://vcvrack.com/Fundamental) ·
[VCV Core manual](https://vcvrack.com/manual/Core) ·
[VCV Fundamental Constructs thread](https://community.vcvrack.com/t/vcv-fundamental-constructs/15895) ·
[Bitwig Grid modules](https://www.bitwig.com/userguide/latest/grid_modules/) ·
[CDM on Blocks Base](https://cdm.link/2019/04/reaktor-modular-patching-free/) ·
[Voltage Modular Nucleus](https://store.cherryaudio.com/bundles/voltage-modular-nucleus) ·
[Softube Modular manual](https://www.softube.com/user-manuals/modular)

Hardware evidence: [SOS Mother-32](https://www.soundonsound.com/reviews/moog-mother-32) ·
[Synthtopia Neutron guide](https://www.synthtopia.com/content/2022/05/23/a-complete-guide-to-the-behringer-neutron-synthesizer/) ·
[SOS 0-Coast](https://www.soundonsound.com/reviews/make-noise-0-coast)

Utility and popularity evidence:
[ModularGrid evaluation lists](https://modulargrid.net/e/modules/evaluationlists) ·
[Noise Engineering — The Utility of Utilities](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/the-utility-of-utilities/) ·
[Learning Modular — Linear versus Exponential](https://learningmodular.com/linear-versus-exponential/) ·
[MOD WIGGLER — 'Essential' utilities](https://www.modwiggler.com/forum/viewtopic.php?t=246095)

**Confidence flags:** VCV Fundamental (37), VCV Core (10), Voltage Nucleus (22)
and Softube's base list are verified against vendor sources. Bitwig Grid's
taxonomy is solid but per-category counts vary by version. **Reaktor Blocks
Base's count (24) is confirmed but ~13 of its individual block names are
UNVERIFIED** — NI's pages refuse automated fetch and no third party enumerates
them; the *functions* are well attested. ModularGrid ordering is reliable;
individual rack counts are approximate.
