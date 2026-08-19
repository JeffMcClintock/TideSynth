# Reaktor Blocks' unified connection scheme

Extracted from the **REAKTOR Blocks manual, software version 1.1 (12/2015)**,
section 4 "Connections and Signals", authored by Jan Ola Korte — the PDF was
downloaded and the text extracted directly, not recalled.

**Why this matters to TIDE:** BACKLOG **E2** says its first job is to *"define
the naming and I/O conventions for a module Container"*. This is a shipped,
proven answer to exactly that problem, from the product Jeff names as a
favourite. It is worth copying almost wholesale.

## The core rule

> There is no distinction between different types of signals. All connections
> are made with signals in the range **-1 to +1**, so any output can connect to
> any input without worrying about signal type or value range.

Everything below is a *convention layered on one uniform signal*, not a type
system enforced by the patcher. That is the trick: universal connectability,
with predictable meaning supplied by the port's declared role.

## The six port roles

| Role | Ports | Semantics |
|---|---|---|
| **General** | `In`, `Out` (numbered `In 1`, `In 2`… when several; or function-labelled) | anything; audio rate; -1..+1 |
| **Modulation** | `Mod A`, `Mod B` (input only), `FM` (input only) | general modulation buses, full -1..+1, any rate. `FM` is the dedicated pitch/cutoff modulation input, with a panel control setting depth |
| **Pitch** | `Pitch` | 0..1 maps to **MIDI note 0..120** — so 0.5 = MIDI 60, and the range is exactly **10 octaves**. This is **0.1 per octave**, Blocks' analogue of the 1V/Oct standard. Accepts values outside -1..+1 for pitches beyond the range |
| **Gate** | `Gate` | triggers on a **positive zero crossing**; gate-off when it falls below 0. **Velocity is encoded as how far above 0 the initial rise goes** — a 0→1 leap is full velocity, 0→0.5 is half |
| **Reset** | `Reset` | positive zero crossing only; negative crossings ignored. Sequencers and counters |
| **Sync** | `Sync` | positive zero crossing only. Oscillator phase sync; a dedicated high-quality `Osc Sync` output exists for osc-to-osc use |
| **Pluck** | `Pluck` | excites an optocoupler model for percussive response (e.g. the West Coast LPG's `LEVEL`). Wants sharp rising edges |

## Function-labelled outputs — the naming convention

Rather than one output with a mode switch, Blocks exposes **every mode at once**
as separate labelled outputs, *and* keeps a switchable main `Out`:

- Filter (Bento Box SVF): `Out` (switchable mode), `LP Out`, `BP Out`, `HP Out`
- Envelope (Bento Box Env): `Gate A`, `Gate D`, `Gate S`, `Gate R` — each high
  during the corresponding envelope stage

## Feedback

No constraint on feedback across any number of blocks. **A block cannot feed
back into itself directly** — one block must be patched in between.

## What TIDE should take, and the one thing to question

**Take:**
1. **One uniform signal range, roles as convention.** It is why Blocks can
   promise "any output to any input, with predictable results" — the property
   PLAN.md wants when it says the whole patch is visible and anything patches
   into anything.
2. **`Mod A` / `Mod B` as standard modulation buses on every module**, with
   per-parameter routing on the panel. It removes the attenuverter-and-mixer
   tax that Eurorack charges for basic modulation, which is a large part of why
   Blocks is friendly.
3. **Velocity encoded in gate amplitude.** One port, two pieces of information,
   no extra cable.
4. **All filter modes exposed simultaneously as extra outputs.** Costs nothing
   and removes a mode switch from the common case.
5. **Positive-zero-crossing triggering everywhere**, so any signal can drive any
   trigger input — an audio oscillator can clock a sequencer.

**Question:** the **0.1/octave** pitch convention. It is internally consistent
and maps neatly onto a normalised -1..+1 world, but every piece of Eurorack
literature, every tutorial and every user's hardware intuition is **1V/octave**.
SynthEdit's own convention should be checked before choosing — inventing a third
convention would be the worst outcome. Flag as a decision, not a default.
