# `e83-vcv-scope-cabled.xml` — BACKLOG E83's fixture

**What it is:** [`e75-vcv-visible-rack.xml`](e75-vcv-visible-rack.README.md) **with one
line of its decoded document changed and nothing else**. Same 19 modules, same view
centre and zoom, same everything E75 is cited for — its **LFO→Scope cable now exists in
the audio graph as well as on the panel.**

| | |
|---|---|
| preset file | **51,806 bytes**, md5 `80d7b85841c707df3f93ac3244990182` |
| document inside it | **38,745 bytes**, md5 `7887ea95aeb71f2b0f9fd03135de57fd` |
| derived from | `e75-vcv-visible-rack.xml` (51,694 / 38,661 bytes) |
| DSP `HC_PATCH_CABLES` | **2 cables** — was 1 |
| Editor `HC_PATCH_CABLES` | 2 cables — unchanged |

Regenerate it, and check the claim above, with one command each:

```bash
python3 scripts/patch-cables.py tests/fixtures/e75-vcv-visible-rack.xml \
        --sync-dsp -o tests/fixtures/e83-vcv-scope-cabled.xml
python3 scripts/patch-cables.py tests/fixtures/e83-vcv-scope-cabled.xml --show
```

`--show` exits **0** here and **1** on `e75-vcv-visible-rack.xml`, which is the whole
difference between the two fixtures stated as an exit code.

**A byte diff of the two committed files is useless**, for the same reason it is useless
for E75: the document is base64 inside one `<Param>`, so any change rewrites the whole
blob. Diff the *decoded* documents instead — 798 lines each, differing on **one**.

## What was wrong with the parent, and why it is not the parent's fault

A TiDE document stores its rack cabling **twice**. `HC_PATCH_CABLES` (host control 49,
`SynthEditLib/HostControls.h`) appears once in the `<DSP>` half and once in the
`<Editor>` half, each holding its own base64'd `<Cables>` list. The panel draws from the
editor's copy; **the audio graph is built from the DSP's**.

In `e53-vcv-rack-segv.xml` — and therefore in `e75-vcv-visible-rack.xml`, which is two
view fields away from it — those two lists **disagree**:

| half | cables |
|---|---|
| Editor | Pulses→SHASR, **LFO→Scope** |
| DSP | Pulses→SHASR |

So the LFO→Scope cable is drawn and carries nothing. That is why E19's pixel-diff clause
kept measuring a Scope whose trace never moved, and why **E83 looked like a broken
display-state capture for a week**: the capture was working perfectly and faithfully
reporting an unconnected input.

`e53-vcv-rack-segv.xml` is a **session file TiDE itself wrote** on 2026-08-26, before
E68's 2026-08-31 ruling made a cable edit push the document. Loading it into a build of
current `main` and saving produces halves that **agree** — so this is a stale stored
document, not a defect a run can still provoke through this path. It is still a live trap
for any fixture committed before that date.

## The measurement this fixture exists to make possible

Same build, same probe, the two fixtures one document line apart — the Scope's own
`RackProcessor` trace, with `RACK_ADAPTOR_TRACE=1`:

| | `e75-vcv-visible-rack` | `e83-vcv-scope-cabled` |
|---|---|---|
| `'LFO' connections` | `outs=0000` | **`outs=1000`** |
| `'Scope' connections` | `ins=000` | **`ins=100`** |
| `'Scope' first NONZERO INPUT` | *never printed* | **`pin 0 (1.000000)`** |
| `'Pulses' / 'SHASR'` (the control) | `outs=…1000` / `ins=…1…` | **identical** |

The last row is what makes the first three mean something: the cable that was already in
*both* halves connects in both arms, so the change is the one cable, not the harness.

## What this fixture does NOT establish

**That the Scope's picture now animates.** The display-state payload is 65,548 bytes and
reaches the editor on VST3 and the standalone; on CLAP it does not — **E80** caps what
the plug-in packs at 200 bytes, measured again here as identical traffic in both arms. So
the headless probe can prove the input arrives and cannot see the capture that follows.
Confirming the trace moves is one launch of the standalone or one hosted VST3 session,
and it is **E19**'s clause rather than this fixture's.

**That any other committed fixture is consistent.** `scripts/patch-cables.py --show`
answers that per file in one command; wiring it into `lint` is **E84**.
