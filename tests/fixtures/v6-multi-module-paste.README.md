# `v6-multi-module-paste.synthedit`

A deliberately minimal prefab for **BACKLOG V6**: two top-level modules and one
top-level connection between them. Nothing else.

## What it is for

V6 proposes replacing `TideApp::seedRootMidiCv()` — which builds the root
MIDI-CV assembly in C++ — with a single prefab. The row named the risk:

> the risk is the root-level paste, which should be verified before the C++ is deleted

That risk was real rather than theoretical. **All nine shipped prefabs in
`RackModules/` have exactly one top-level module and zero top-level lines**, so
the case V6 depends on — several top-level modules, plus connections *between*
them, pasted at the root — had never been exercised in this product.

This fixture exercises exactly that case and nothing else, so a failure has one
possible cause.

## How to run it

Copy it into a build's `SynthEditSem/Resources/Prefabs/`, launch
`TIDE_Rack_STANDALONE` with an isolated `HOME` (the session file is restored on
launch, so a relaunch is NOT a clean rack), arm `Prefabs > V6TestPaste` in the
module browser and click the rack — insertion is **arm-then-click**, not a drag.
Quit, then decode `~/.config/TIDE Rack/session.xml`: it is a `<Preset>` whose
`Param id="1"` is base64.

## What it proved, 2026-08-24 (linux)

Both modules landed at the root with their handles intact, and the connection
survived — byte-equivalent to what the C++ produces for the seeded pair:

    seeded  <line ... fMod="1521837852" tMod="1620974935" fPlg="1" />
    pasted  <line ... fMod="811000001"  tMod="811000002"  fPlg="1" />

and in the DSP half:

    seeded  <Line From="1521837852" To="1620974935" />
    pasted  <Line From="811000001"  To="811000002"  />

The seeded pair is the positive control: it is the same wiring built the way
TIDE builds it today, in the same document, so "the prefab produced the right
thing" is a comparison rather than a judgement.
