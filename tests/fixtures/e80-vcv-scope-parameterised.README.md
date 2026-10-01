# `e80-vcv-scope-parameterised.xml` — the rack with the Scope's parameters present

Added 2026-09-16 (windows, scheduled run) for **BACKLOG E80**.

**Use this fixture, not `e75-vcv-visible-rack.xml`, for anything that measures a
rack module's DSP→GUI traffic.** They are the same five-VCV rack and they differ
in one thing: this one's Scope has its eleven patch parameters and `e75`'s Scope
has **none**.

## Why the difference matters more than it sounds

A rack module's lights and its display-state blob are not side channels — the
adaptor declares each one as a private, non-persistent **parameter** with a
`direction="out"` pin bound to it
(`SynthEdit_Rack_Adaptor/RackAdaptor.h`). With no parameter in the document,
`ug_patch_param_setter::ConnectParameter` leaves the pin unconnected,
`UPlug::Transmit` finds an empty connection list, and everything the module
sends is discarded. Nothing errors: the module still constructs, still
processes, still captures its display state, and still reports success.

`e75` (and its ancestor `e53`, and its sibling `e83-vcv-scope-cabled`) all carry
the same Scope, handle `987654321`, with zero parameters. Every E80, E19 and E83
display-state measurement was taken through one of them.

## The A/B, same binary, one variable — the document

VST3, 800 blocks of 512 at 44.1 kHz, `--editor`, `TIDE_FEEDBACK_TRACE_EVERY=1`:

| | `e75-vcv-visible-rack.xml` | **this fixture** |
|---|---|---|
| `no patch parameter for module` lines | **9** | **0** |
| feedback sends | 569 | 573 |
| **largest send** | **325 B** | **65,873 B** |
| lifetime queue traffic | 59,878 B | **17,502,646 B** |
| `display-state capture` | `#200 (65548 B)` | `#200 (65548 B)` — unchanged |
| far end, the editor | `update #1 arrived (0 bytes)`, frozen | **`update #260 arrived (65548 bytes)`, advancing** |

The capture row is the control: the DSP was doing the same work in both arms, so
what changed is what happened to the picture afterwards. CLAP reproduces it from
the same build tree — `#1 arrived (0 bytes)` → `#260 arrived (65548 bytes)`.

## How it was made, and what "regenerate" can and cannot promise

```
e80vst3probe.exe <build>/SynthEditSem/Release/TIDE-Rack.vst3 \
    tests/fixtures/e75-vcv-visible-rack.xml --blocks 1 --save out.xml
```

That is `e75` loaded and saved by a build of `main`, nothing else — the product
supplies the missing parameters itself, which is what says they are missing
rather than unnecessary.

**It does NOT regenerate byte-identically, and the reason is a known row.**
`Handle="…"` on a patch parameter is re-minted from a `time(nullptr)`-seeded RNG
on every load (**E77**, mechanism at `SynthEditLib/UniqueSnowflake.cpp:176`;
**E81** is the open question about it), and `<Parameter>` is sorted by handle, so
one changed handle moves whole blocks. Measured here: two saves a second apart
differ in **592 of 892 decoded lines** — and are **identical** once handles are
masked and order ignored, which is E77's own normalisation.

So: regenerate it when the document's *content* needs to change; do not expect a
`diff` to be empty, and do not treat a non-empty one as a defect on its own.

## What it inherits, and what it does not

- **The cables agree.** `scripts/patch-cables.py --show` reports both halves
  listing `Pulses→SHASR` and `LFO→Scope`, so this fixture carries **E83**'s fix
  as well — the save wrote the editor's cable list into both halves.
- **The view still opens on the rack.** `PanelLocationZoom` survives at
  `0.64999998` against `e75`'s `0.65`, with the same 17 `panelRect`s, so
  **E75**'s work — panels actually on screen at the default view — is intact.
- **It is not a fix for the three stale fixtures.** `e53`, `e75` and
  `e83-vcv-scope-cabled` are deliberately left exactly as they are: three rows
  name them as reproductions, and rewriting a fixture other rows cite loses the
  thing they reproduce. Check any of them with
  `scripts/patch-parameters.py <fixture> --compare` before quoting a
  display-state number from it.
