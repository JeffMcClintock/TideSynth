# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-28 — windows — E48 built: the three prefab modules join the compiled-in set, and the round trip surfaces a serialization oscillation (interactive, Jeff directing)

**Prompt:** *"so we need to ship these extra modules with TiDE i guess"* · Fable 5, `claude-fable-5` · as **tide-rack-bot**

**Did:** implemented Jeff's E48 ruling — `SE Scope3 XP`, `SE Oscillator` and
`SynthEdit ADSR` now compile into TIDE. Ruling recorded in
[docs/decisions.md](docs/decisions.md). E48 → IN-REVIEW; **E56** filed for what
the Accept test found underneath. Two branches, and **the SynthEditLib one must
merge first** or TIDE's link fails.

(Housekeeping: the session crossed midnight — entries above dated 2026-08-27
from roughly `E19's VST3 cell` onward ran into the early hours of 08-28.)

### Why linking the lib was never enough

The three modules sit behind `IF(SE2JUCE)` in SynthEditLib's own source list and
behind `#if GMPI_IS_PLATFORM_JUCE==1` in `UgDatabase.cpp`'s `INIT_STATIC_FILE`
pull list — SynthEdit-product builds only, both ways. TIDE's route is the
E7/MidiToGate pattern: compile the TU into `SynthEditSem`, where its static
registration cannot be discarded. `EnvelopeAdsr.xml` and `Oscillator.xml` joined
`_tide_xmls` **and** TideApp's read loop in the same edit — the two lists that
must move together.

### The collision only TIDE could ever see

First link: **LNK2005 — `Oscillator::onSetPins` already defined.**
`modules/Oscillator/Oscillator.cpp` and `modules/OscillatorHD/Oscillator.cpp`
both declare a global `class Oscillator` with out-of-line virtual members.
SynthEdit never links the two into one image — OscillatorHD ships there as a
loadable — so the ODR violation was invisible until TIDE became the first
consumer to compile both. Fixed in the lib (gated, own PR): the whole TU goes
into an anonymous namespace, which is the linkage it always effectively had —
`Oscillator.h` reads, in full, `// not used`.

The same two files are also why the offline screen failed twice: OscillatorHD's
id `SE Oscillator4` **contains** `SE Oscillator`, so a substring scan scored the
missing module present.

### Measured — a fresh five-prefab round trip on the new binary

```
Module not found:      0        (was 3 modules)
could not be restored: 0        (was 8 connectors)
--dialogs:             count=1  (the quiet-mode announcement only)
ControlsXp.xml         enriched 4 of 18   (was 2)
EnvelopeAdsr.xml       enriched 1 of 1
Oscillator.xml         enriched 1 of 1
```

The old degraded fixture also reloads clean — with two stale placeholder cable
lines reconciled away silently, the E42 lesson again: a fixture authored through
the bug does not survive the fix byte-for-byte.

### What the Accept test found underneath: E56

E48's Accept asks for a **byte-identical** document, and three consecutive
save/reload cycles give **49,421 → 49,419 → 49,421** — the serialization
*oscillates between two forms*. The diff is pure `<Parameter>` reordering plus
handle assignment; the same values, the same `0.494999`, different order. The
default rack round-trips byte-identical, so the churn needs the prefab
parameter population. Filed as **E56** with the likely fix named (a
deterministic sort in `ExportGetSortedParameters` — GATED, so filed): until
then, every "byte-identical" Accept in this backlog is unachievable by
construction, and size-based instruments carry ±2 bytes of noise per cycle.

**E48 should be judged on its loss clauses, which are met.**

### The check's second screen retired the day its prediction came true

`check-prefab-modules.py`'s described-only screen carried a note: a module both
registered in C++ and described in a staged XML would be flagged wrongly, and
"a maintainer who hits it should fix the rule rather than the prefab." Jeff's
ruling made described-AND-registered the *normal* state — all three new modules
tripped it. Screen retired, one absence screen kept, and the docstring now says
what three revisions in one day converged on: **the authoritative census is a
`-quiet` launch reading `Module not found` from the app itself.**

**Learned:**

- **A module set curated by conditional compilation has invisible seams.** Two
  files named Oscillator.cpp with two global classes named Oscillator coexisted
  for years because no build linked both. The first new consumer paid.
- **An Accept can fail on a defect older than the row.** Byte-identical was the
  right bar; the serialization order was never stable underneath it, and only a
  fresh A/B/C cycle separated "my change lost data" from "the order oscillates".
- **When the product becomes the instrument, retire the scaffolding honestly.**
  The described-only screen was correct for exactly one day and wrong forever
  after the ruling; keeping it would have failed every future legitimate module.

**Next:** merge order — SynthEditLib `tide/win/E48-oscillator-tu-local` first,
then TideSynth `tide/win/E48-ship-prefab-modules`. **E56** wants a ruling on a
deterministic parameter sort. macOS/Linux have not built these three modules
(Scope3Gui's `sys/time.h` path is guarded; expected clean, unverified).

**Machine state.** `RackModules/AR_jef.synthedit` still dirty — Jeff's, left
alone. `%APPDATA%\TIDE Rack\` restored and md5-verified; no TIDE process
running. SynthEdit itself rebuilt against the namespaced Oscillator: rc=0.

**Branch/PR:** TideSynth `tide/win/E48-ship-prefab-modules` (CMake, TideApp
loop, check, rows, decisions.md, this entry) + SynthEditLib
`tide/win/E48-oscillator-tu-local` (the anonymous namespace). Cross-linked;
lib first.
## 2026-08-28 — macos — E53 reproduced 3/3 with a faulting address, and the cause is a guard #64 missed

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell* · as tide-rack-bot (both)

**Did:** took **E53**, reproduced it on macOS **3/3**, and produced the faulting address and named cause the row's first stage asked for. **Row back to TODO** — the Accept (*"the fixture loads and runs without faulting"*) is **not** met and the fix is GATED. Branch `tide/mac/E53-mac-repro`.

### Why this box could take a row the windows box could not

E53 could not be re-measured on 2026-08-28 because the repro needs a config folder only one process can own, and `configRoot()` has **no environment override on Windows** — that asymmetry is **E55**. On macOS the same function reads `$HOME`, so the fixture ran in a fully isolated config root, three times, touching nothing of Jeff's. **The row's blocker was platform-specific, not intrinsic**, which is the kind of thing worth checking before recording a row as stuck.

### The fault — 3/3 identical

```
EXC_BAD_ACCESS / SIGSEGV   KERN_INVALID_ADDRESS at 0x51
faulting thread 11 = the CoreAudio render thread

  dsp_patch_parameter_base::UpdateOutputParameter(int, UPlug*) +24
  ug_base::HandleEvent(SynthEditEvent*) +124
  ug_base::DoProcess(int, int) +260
  SeAudioMaster::DoProcess_plugin(...)
  SynthRuntime::process(...)
  gmpi::standalone::StandaloneHost::processAudio(...)
  gmpi::standalone::AudioDriverCoreAudio::renderProc(...)
```

Three `.ips` reports, same address, same top frame. **This confirms the row's "after the graph is live" as a thread fact, not an impression:** E49 died *during graph build*; this dies inside `process()`.

### The cause: a fourth assert-only lookup that #64 missed

`ug_patch_param_watcher::onSetPin`, `SynthEditLib/ug_patch_param_watcher.cpp:160-161`:

```cpp
assert(patchParams[paramNumber]);                        // compiles out under NDEBUG
patchParams[ paramNumber ]->UpdateOutputParameter( ... ); // unguarded deref
```

`:145` has no assert at all. And [#64](https://github.com/JeffMcClintock/SynthEditLib/pull/64) — *"Guard three assert-only lookups that compile out under NDEBUG"* — touched `PatchManager.cpp`, `PropertiesBrowser.cpp` and `ug_patch_param_setter.cpp`. **`ug_patch_param_watcher.cpp` was not among them**, verified with `git show --stat 796bbc2`.

### And that links E49 and E53 mechanically, which is the durable part

E49's guard (`ug_patch_param_setter.cpp:203`) leaves the pin *"unconnected rather than dereferenced"* — the eight `no patch parameter for module 987654321 parameter id 0..7` lines print on every run of this fixture, and `987654321` is the `VCV: Scope`. Those eight entries stay **null** in `patchParams[]`. The watcher then dereferences one of them on the audio thread.

**So #64 did not fix the null — it moved where the null is dereferenced, from graph build into `process()`. It made the document loadable but not runnable.** That is not a criticism of #64, which did exactly what it claimed; it is the reason a guard that converts a crash into a degraded state needs someone to ask what consumes the degraded state.

### Not done, deliberately

**The fix is two guards of the same shape as #64, and it is GATED.** Not written: Jeff authorised the **E39** GATED edit specifically, not a standing exception, and this is not a build break so STEP 5's exception does not reach it.

**One trap for whoever reproduces it:** the fixture holds five VCV modules, and `TIDE_VCV_FUNDAMENTAL` defaults **OFF** — with it off they do not resolve and you get an E48-shaped missing-module failure instead of this one. I nearly measured the wrong bug; the giveaway was `strings` finding no VCV symbols in the first binary I reached for.

Fixture verified byte-exact before use: 38,658 B, md5 `9248a7ee283cf8a4c1dfaaeb811f32b4`, `grep -ci compare` = 0 — which independently confirms the README's correction to E49's module list.

**Learned:**

- **A row blocked by a platform's tooling is not blocked everywhere.** E53 sat unmeasured because Windows cannot redirect its config root; macOS can, and the whole reproduction took three runs. Check whether a stated blocker is intrinsic or local before recording a row as stuck.
- **A guard that turns a crash into a degraded state has moved the failure, not removed it.** #64 was correct and its own Accept was met; the null it stopped dereferencing was still null, and the next consumer was on the audio thread.
- **Fixing three of a kind invites checking for a fourth.** #64's own title says *three* assert-only lookups; the same pattern in a fourth file is what E53 turned out to be.

**Next:** E53 needs two guards in `ug_patch_param_watcher.cpp` — GATED, and the same shape Jeff already merged once as #64. **E55** would remove the reason this row stalled on Windows at all.

**Branch/PR:** `tide/mac/E53-mac-repro`.

## 2026-08-28 — macos — E39 built: the rack is a whole number of rows, and the two measurements this row called contradictory are both right (interactive, Jeff directing)

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell* · as tide-rack-bot (both)

**Did:** built **E39** — a GATED change, at Jeff's explicit instruction (*"yes, do E39"*). [SynthEditLib#66](https://github.com/JeffMcClintock/SynthEditLib/pull/66), branch `tide/mac/E39-rack-whole-rows`. Row is IN-REVIEW.

### The cause, and why the fill is the part that was wrong

`TopView::renderRack` lays rails out from `rack.origin` — which `MfcDocPresenter::getRackLayout` sets to the **panel rect's top-left**, an arbitrary document coordinate — while filling the **whole cliprect** with case interior. The canvas edges at `0` and `viewDimensions` almost never land on a row boundary, so the canvas top carried case interior with a rail on **one side only**.

The fix computes the row-aligned band inside the canvas and clamps both the fill and the row loop to it. **Against the canvas, not the cliprect** — the cliprect is whatever dirty region is repainting, so aligning to it would make the rack's extent depend on what happened to need redrawing.

### The A/B

One build tree, the commit the only variable, 1100×1500 window, one column sampled:

```
BEFORE                          AFTER
  case  253..643  (391 px)  <-- gone
  rail  644..663            <-- gone (the partial row's OWN bottom rail)
  rail  666..686                rail  666..686
  case  690..1219 (530 px)      case  690..1219 (530 px)
  rail 1220..1262               rail 1220..1262
  case 1266..1795 (530 px)      case 1266..1795 (530 px)
  rail 1796..1838               rail 1796..1838
  case 1842..2207               case 1842..2207
```

**Every interior row is byte-identical.** Only the canvas-edge partial row moved, which is the entire claim.

### It reproduces the original Windows report exactly, and that is the good part

The original report measured the strip at *"391 px = 261 DIP"*, 0.68 of a row. This measures **391 px**; at the effective 1.5 px/DIP (2.0 rasterization × 0.75 zoom, itself confirmed by the 576 px row pitch = 384 DIP = E5's row height) that is **261 DIP**. The arithmetic closes independently: `panelRect.top` is 3732, `3732 mod 384 = 276`, and `276 − 15` for the rail = **261**.

**So the two measurements this row treats as contradictory are BOTH RIGHT.** The 2026-08-26 run got 0.14, 0.27, 0.29 and 2.16 of a row at four viewports and concluded that no fixed canvas quantity could explain it — and drew the correct conclusion, *"do not spend a session on `kRackViewDips`"*, because the canvas height genuinely is not the cause. But the strip is **not** variable: it is a fixed **261 DIP** set by `panelRect.top mod rowHeight`. What varies is **how much of it the viewport can see**. A row can be right about the refutation and wrong about what the refutation implies.

### Not verified, and one of these is a new defect

- **macOS renderer only**, and only at two window sizes.
- **`--scroll` reports `ok` and moves nothing.** Three scroll positions produced **byte-identical** screenshots (same md5). I had intended to vary the viewport that way and it proved nothing; window height was used instead. **That verb looks broken and wants its own row** — filed as a note here rather than chased, per STEP 3's scope rule.

### One judgement call left with Jeff

The Accept is an either/or: *"every rack row is a full 384 DIP with rails above and below, **or** the partial region is not drawn as rack at all."* This is the **second** limb, because the first is unreachable at a canvas edge without moving `rack.origin` — and origin anchors row 0 to the panel, so moving it would shift every saved module relative to the rails. The visible consequence is that the leftover now paints as canvas background (black) rather than case interior: honestly *not rack*, but a starker band than before. Raised on the PR.

**Learned:**

- **A refutation and its implication are separate claims, and a row can carry the first correctly while the second is wrong.** "It varies, so it is not the canvas constant" was right; "therefore the strip is not a fixed quantity" did not follow. The fixed thing was one modulo away.
- **Reproducing an earlier report's exact number is worth chasing.** 391 px on two platforms, and `panelRect.top mod rowHeight` arriving at the same 261 DIP from the source, is far stronger evidence than a screenshot that merely looks better.
- **A verb that returns `ok` can still do nothing.** `--scroll` answered `{"ok":true,"delta":-720}` three times and moved zero pixels; only comparing image hashes caught it. An `ok` is a claim about the call, not about the effect.

**Next:** #66 is Jeff's to merge, with the black-band question on it. `--scroll` needs a row.

**Branch/PR:** `tide/mac/E39-row-in-review` (this row + entry) and `tide/mac/E39-rack-whole-rows` → [SynthEditLib#66](https://github.com/JeffMcClintock/SynthEditLib/pull/66).

## 2026-08-28 — macos — the queue is blocked and this run did nothing to it, deliberately

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell; recorded as unknown rather than guessed* · as tide-rack-bot (both)

**Did:** nothing to the queue. STEP 1 clear, STEP 1.5 has nothing of mine to fix, and **no mac/any row is takeable**. Recording that and stopping, per STEP 2's own instruction, rather than finding something to do.

### The walk

Extracted with the line regex, not `awk -F'|'` — the previous entry records why that distinction matters.

| row | blocker |
|---|---|
| S1b, S8, E7, E39 | GATED |
| E38 | `NEEDS-SPEC` + GATED |
| E2 | "not takeable" by its own row |
| E48 | `NEEDS-JEFF` — a PLAN constraint 7 product decision |
| E51 | `NEEDS-SPEC` — now one thing, a re-stated Accept; its census half was answered yesterday |
| E19 | no marker, but needs E48/E49/E50 understood first, then a human at a DAW |
| E53 | no marker, but held by windows on [#516](https://github.com/JeffMcClintock/TideSynth/pull/516) |

**STEP 1.5:** only [GMPI_Wrappers#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28) — `MERGEABLE`, no failing checks, nothing unresolved. That is the "waiting for merge, leave it alone" case in the rule's own words.

**E52 remains correctly `IN-REVIEW`**, holding on #28 exactly as annotated on 2026-08-27.

### One thing I checked and decided NOT to do, which is the substance of this entry

The watchdog digest ([#44](https://github.com/JeffMcClintock/TideSynth/issues/44)) was last regenerated **2026-08-27 09:12 UTC**, so it predates E25/E45/E54 flipping, five merges, and the E51 census. Its *"IN-REVIEW rows — PR merge status"* and *"Open PRs"* sections are materially wrong right now, and its own header says *"regenerated every run"*. That reads like a standing chore this run was skipping.

**It is not.** A6's row records that the digest is regenerated by **`.github/workflows/watchdog.yml`** on `schedule: 0 6 * * *` plus `workflow_dispatch` — *"every run"* there means every **workflow** run, not every agent run. So regenerating it by hand is not this run's job, it would post publicly on the strength of my own misreading, and it self-heals at 06:00 UTC daily.

**Worth writing down because the temptation was real and the wording invited it.** A blocked queue makes any plausible-looking chore attractive, and "the header says every run" is exactly the kind of half-read justification that gets one done. One check of the row that owns it settled it.

**Learned:**

- **A blocked queue is when invented work is most likely, so the bar for "is this actually my job?" should go up, not down.** The digest looked overdue, said so in its own header, and belonged to a workflow.
- **"Every run" is ambiguous in a fleet that has both agent runs and workflow runs.** The owning row disambiguated it; the artefact's own header did not.

**Next:** unchanged and all with Jeff — merge `GMPI_Wrappers#28` (frees E52), rule on E48, or open a GATED session for S1b/S8/E39. A run has now confirmed the same wall three iterations running; the fleet is not short of capacity, it is short of decisions.

**Branch/PR:** `tide/mac/queue-blocked-2026-08-28` — journal only, no row touched.

## 2026-08-28 — macos — the queue is blocked, so I answered the one question a ruling said was open: which call site consumes a dialog answer

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell; recorded as unknown rather than guessed* · as tide-rack-bot (both)

**Did:** **no backlog item taken.** The mac/any queue is blocked, verified row by row. Instead: the **answer-consuming call site census** that `docs/decisions.md` names as unresolved and E51's row names as still open. **E51 stays TODO** — this supplies one of the two things its `NEEDS-SPEC` is missing, and changes no code.

### First, a tooling error of mine that made three previous walks unreliable

I had been extracting rows with `awk -F'|'` and reading `$5` as the Item. **Rows contain `|` characters**, so `$5` is a fragment. E51 read as **1,989 bytes** that way; it is **10,094**. The truncated read hid its `NEEDS-SPEC` marker entirely, and I nearly took the row on the strength of it.

Re-walked with a proper regex (`^\| (id) \| ([^|]+) \| ([^|]+) \| (.*) \|$`). The corrected walk:

| row | real blocker |
|---|---|
| S1b, S8, E39, E7 | GATED |
| E38 | `NEEDS-SPEC` + GATED |
| E2 | "not takeable" by its own row |
| E48 | `NEEDS-JEFF` (product decision) |
| E51 | `NEEDS-SPEC` |
| E19 | needs E48/E49/E50 first, then a human at a DAW |
| E53 | taken by windows, [#516](https://github.com/JeffMcClintock/TideSynth/pull/516) |

**Also checked and it is fine:** E51's original text says it is *"BLOCKED on that ruling"* and points at a `PROPOSED:` entry. That entry was **answered and removed by the windows box on 2026-08-27** ([#495](https://github.com/JeffMcClintock/TideSynth/pull/495)) — the decisions entry says so in as many words. I checked because the "None." now in that section is text **I** wrote on 08-27, and I needed to know I had not deleted a live question. I had not.

### The census

The ruling: *"of the ~58 call sites only one consumes the answer — but that one site has not been identified."*

**It is `CContainer::ReplaceModuleAsync`, `SynthEditLib/EditorLib/CContainer.cpp:3990`:**

```cpp
Document()->Application()->SeMessageBoxAsync(
    L"Want to just set destination pins default instead?", L"", MB_YESNO | MB_ICONWARNING,
    [...](int32_t answer)
    {
        const auto action = (answer == IDYES) ? ReplaceModuleAction::SetPinDefaults
                                              : ReplaceModuleAction::Replace;
```

A second consumer exists — `ExportAsPlugin.cpp:840`, `MB_YESNOCANCEL`, *"Save Changes First?"* — so the ruling's "only one" is one too few, though both are outside TIDE's reach.

**In TIDE the site is linked but UNREACHABLE, checked three ways:**

1. **No caller.** Its only caller is `SynthEditApp::ReplaceModuleAsync` (`SynthEdit/SynthEdit2/SynthEditApp.cpp:680`), and TIDE does not build SynthEdit2 — `TideAppStubs.cpp` replaces it and never mentions `ReplaceModule`.
2. **The menu route is inert.** `POPUP_MENU_REPLACE` (`CUG.cpp:2023`) only calls `VO_Notify(OM_WPF_REPLACE_DIALOG, handle)`, and EditorLib's handler for that message is an explicit **`break;`** (`MfcDocPresenter.h:392`, grouped with `OM_SCREENSHOT` as deliberately ignored). Its one real handler is SE16's WinUI3 `MainWindow.xaml.cpp:790`.
3. **Symbols, with both controls:**

```
ReplaceModuleAsync   16 symbols   linked, as expected
ExportAsPlugin        0 symbols   absent      <- negative control
divertPrompt          1 symbol    present     <- positive control
```

**So quiet mode's blanket answer is safe in TIDE today, and safe for the right reason** — not because nothing consumes an answer, but because the one thing that does cannot be reached.

### The latent trap, which is the part worth acting on

`divertPrompt` returns `constexpr int32_t answer = MB_OK;` (`Application.cpp:475`).

**`MB_OK` is a FLAGS constant, `0x0`. It is not a response constant at all.** The responses are `IDOK`=1, `IDCANCEL`=2, `IDYES`=6, `IDNO`=7. A diverted prompt therefore answers a value equal to *none* of them.

It is correct here only by arithmetic accident: the sole consumer tests `== IDYES`, gets false, and falls to `Replace` — which is exactly the branch taken when the offer is not available at all, so the degradation is conservative. But **a consumer testing `== IDOK` would also read false**, which is the opposite of what "answered OK" implies. If the replace dialog is ever implemented TIDE-side, this should return `IDOK`, not `MB_OK`.

**Learned:**

- **`awk -F'|'` cannot read this backlog.** Rows contain pipes; field-splitting silently truncates them, and a blocker marker past the cut is invisible. Three of my walks were unreliable and one nearly took a `NEEDS-SPEC` row.
- **"Linked" is not "reachable", and `nm` alone would have said the wrong thing.** The symbol is in the binary; the call graph is what decides, and here it takes a caller check, a message-dispatch check and a symbol check to be sure.
- **A safe default reached by accident is still worth naming.** `MB_OK` works only because the one consumer asks a question it happens to answer correctly by being unequal to everything.

**Next:** E51's remaining `NEEDS-SPEC` is now **one thing, not two** — a re-stated Accept. The census half is answered. Still blocked for a run otherwise: `GMPI_Wrappers#28` unmerged (holds E52), E48 needing its product decision, and S1b/S8/E39 GATED.

**Branch/PR:** `tide/mac/E51-answer-consumer-census` — one row annotation, no code.

## 2026-08-28 — macos — the queue is blocked for a run, so this was STEP 4: three rows flipped, one deliberately not, and a false claim corrected at its source

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell; recorded as unknown rather than guessed* · as tide-rack-bot (both)

**Did:** no backlog item — **the mac/any queue is genuinely blocked for a scheduled run**, re-walked and verified rather than inherited. Did STEP 4's mandated bookkeeping instead: **E25, E45 and E54 flipped DONE and archived; E52 deliberately NOT; the NEXT block's "two branches with no PR" claim corrected.**

### The walk, verified rather than inherited

The mac cell already said the queue was blocked. STEP 2 says eligibility lives in the status column and not in section prose, so each row was checked:

| row | why not takeable | how established |
|---|---|---|
| S1b, S8 | GATED | `EditorLib`/`SynthEditLib` CMake |
| E38, E51 | `NEEDS-SPEC` in the row itself | read |
| E19 | mac cell wants AU3 in a real host | needs a human at a DAW |
| E7 | a ruling that reframes, not a code task | read |
| E2 | umbrella; its own row says the Accept cannot be stated | read |
| **E39** | **GATED** | **located `TopView::renderRack` at `SynthEditLib/modules/se_sdk3_hosting/ViewBase.cpp:1518` — the cell asserted this, I checked it** |
| E48 | `NEEDS-JEFF` on the fix (a PLAN constraint 7 product decision) | read |
| E53 | taken by windows | [#516](https://github.com/JeffMcClintock/TideSynth/pull/516), `tide/win/E53-fixture-segv` |

STEP 1.5 found only [GMPI_Wrappers#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28) — green, mergeable, nothing unresolved, so left alone per the rule's own words.

### E52 is the row that must NOT be flipped, and the reason generalises

All four `IN-REVIEW` rows had their PRs checked with `gh pr view --json state` rather than read off the row text. Three were complete. E52 was not:

```
E52   GMPI_Wrappers#23  MERGED
      TideSynth#515     MERGED      <- docs-only, A4 auto-merged
      GMPI_Wrappers#28  OPEN        <- the entire fix
```

**#515 changed `BACKLOG`, `JOURNAL`, `JOURNAL-2026-08` and `lessons` and nothing else.** A4 auto-merged it because every changed file is on the allowlist. So from inside TideSynth the row looks satisfied while **the whole of E52's code sits unmerged in another repo.**

This is the 2026-08-27 stale-row family (E32, X2, R5) **running the other way**: those rows asserted something a merged PR had made false; this one would mark done something that has not landed. The common cause is the same — *a row's status was inferred from a PR merging, rather than from the PR's contents.* Annotated on the row so the next STEP 4 cannot make the mistake, and it stays `IN-REVIEW` until #28 merges.

### The false claim, and why correcting it at the source matters

The mac cell said two mac branches sit on the remote with **no PR** — "the one end state STEP 5 forbids". **Both have PRs:**

```
tide/mac/icon-tide-app              #435  MERGED
tide/mac/E36-renumber-duplicate-e34 #445  CLOSED, not merged
```

One command (`gh pr list --head <branch> --state all`) settles it, and **it had not been run by anyone.** The claim originated on the windows box 2026-08-27, was copied into this cell on 08-28, and **I repeated it to Jeff twice** before checking it myself.

It also matters practically, because the two branches need opposite treatment: the merged one is redundant and safe to delete; deleting the closed-not-merged one **discards whatever was on it**, so it wants a human who knows why #445 was closed.

**A NEXT-block cell is read far more often than it is verified**, so an unchecked assertion there propagates further than the same sentence in a journal entry. Corrected in the cell, not just here.

**Verification artifact:**

```
check-backlog-archived  44 row(s), none DONE, all terminated, OK (224 KB)   (was 47 / 247 KB)
check-next-block / check-id-refs / check-links / backlog-diff / journal-prepend / prompt-provenance   rc=0
```

Every PR state above came from `gh pr view --json state`, not from row text.

**Learned:**

- **A merged PR does not mean a row is done — read what the PR changed.** E52's TideSynth half was docs-only and auto-merged; its code is still open elsewhere. "All PRs merged" is only a safe flip test when every PR is checked for *contents*, not just state.
- **The auto-merge allowlist makes docs-only halves land silently, which is exactly what makes this trap likely.** A cross-repo change whose bookkeeping half is allowlisted will always land before its code half.
- **Verify a NEXT-block claim before repeating it, because that cell is the fleet's most-copied text.** A wrong sentence there was reproduced by two runs and reported to Jeff twice on the strength of nobody running one command.

**Next:** the queue stays blocked for a run until one of: **#28 merges** (frees E52), **E48 gets its product decision** (it is fully diagnosed and waiting only on that), or a GATED session opens S1b/S8/E39. Nothing on `tide/mac/**` needs a run.

**Branch/PR:** `tide/mac/step4-flip-e25-e45-e54` — bookkeeping only, no code.
## 2026-08-28 — windows — E53: the box was already taken, so the fixture got committed and the lead got measured instead (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3.0** (no `claude` CLI on this box's PATH; this is the Appx package version, which is what A13 records as the discoverable one on Windows) · as **tide-rack-bot** (both paths)

**Did:** took **E53**. Did **not** get a faulting address — a concurrent Claude session owned the one folder its reproduction needs, for the whole session. Landed the fixture into the repo, measured the document E49/E48/E53 all reason about, filed **E55**, and re-pointed the `win` NEXT cell off **E48**, which has not been takeable since its own diagnosis landed. Row back to **TODO**.

### E48 is not takeable, and the NEXT cell has been aiming this box at it for two days

The `win` cell says `TAKE E48`. E48's row says the only step left is *"re-author `AR_jef.synthedit` to use a module TIDE ships, **or** add `Adsr.cpp` + `EnvelopeAdsr.xml` to the compiled-in set — a PLAN constraint 7 decision. A run must not pick."*

That is an open NEEDS-JEFF question that decides what would be built, and STEP 2's rule is explicit: *"You may only do work that is identical under every open answer."* Nothing under E48 is. The row's status column says TODO and the eligibility rule that actually binds is the other one. **It needs one sentence from Jeff, not a session**, and the cell now says so.

The walk after that, so the next run does not repeat it: **S1b**/**S8** wholly GATED, **E38** and **E51** already `NEEDS-SPEC`, **E19**'s windows cell blocked behind E48 by its own text, **E7** ruled a non-blocker, **E2** not takeable by its own row, **E39** GATED, **X2** marked `linux`. **E53** was the eligible row, and it is a Windows fault on the box with the debugger.

### The blocker, measured rather than asserted

E53's reproduction is *"copy the fixture over the standalone's `session.xml` in `%APPDATA%\TIDE Rack\`"*. Throughout this session that folder belonged to somebody else:

| observed | value |
|---|---|
| second Claude session, same box | transcript `c7a20ba8…` written continuously alongside mine |
| `TIDE-Rack.exe` | **running**, pid 33368, started 07:56:10 |
| `SynthEdit2.exe` | running, `SE16\x64\Debug\…\AppX`, started 07:47:26 |
| `%APPDATA%\TIDE Rack\` | `session.previous.xml` quarantined 07:49:44; a fresh 36,386-byte `session.xml` at 07:56:19 — **neither written by this run** |

**And on Windows there is nowhere else to put it.** `configRoot()` honours `$HOME` on mac and `$XDG_CONFIG_HOME` on linux; on Windows it calls `SHGetKnownFolderPath(FOLDERID_RoamingAppData)` and nothing else (`GMPI_Wrappers/wrapper/Standalone/StandaloneSettings.cpp:31-54`). So there is exactly one config folder per box and two sessions wanting it. Running anyway would have corrupted their measurements and mine. **Filed as E55.**

**I did not build E55 either, and the reason is the same reason.** The other session was compiling from this same `GMPI_Wrappers` checkout; editing shared source under somebody's running build is the mistake I had just declined to make with the folder. The row carries the fix shape instead.

### The fixture is in the repo now, and that is the durable half

`tests/fixtures/e53-vcv-rack-segv.xml` — 51,690 B, md5 `a8c1493a373da00b01c0d4a74735994a` — plus a README. It existed **only** as `_scratch/e19-fixture-preset.xml` on this one box: untracked, outside every repo, nothing regenerating it, and **named as the reproduction by three separate rows** (E19, E49, E53). Finding it took a slice of this session and it was luck that it was still there.

**Verification artifact — the committed preset really does carry the document those rows cite:**

```
preset bytes: 51690  md5 a8c1493a373da00b01c0d4a74735994a
  param 1: decoded 38658 bytes, starts b'<?xml version="1.0" ?>\n<Document>\n  <DSP'
    md5 9248a7ee283cf8a4c1dfaaeb811f32b4
doc md5 9248a7ee283cf8a4c1dfaaeb811f32b4 38658
```

**One correction found on the way:** the wrapper attribute is `standalonePlugin="TIDE Rack"`, not `TiDE Rack` as E49's row writes it. `SessionState` compares that string (`kAttrPlugin`, `SessionState.cpp:43`), so following the row literally would produce a file that silently does not restore.

### The measurement I could make without the app, and it moves the lead

E49 handed E48 and E53 the same lead: *"987654321 is the `VCV: Scope`, and the document DEFINES it — that module reached the DSP graph with no patch-manager parameters at all."* That is checkable in the file.

**(1) Zero dangling handles.** 58 `<Parameter>` tags in the DSP `<PatchManager>`, 56 carry `Module=`, and every one of those handles is a module the document defines.

**(2) The Scope's eight really are absent, and the FILE is where they are missing.** Module `987654321` has zero parameters in the document, and the class declares **exactly eight** — `X_SCALE, X_POS, Y_SCALE, Y_POS, TIME, LISSAJOUS, THRESH, TRIG` (`VCV_Fundamental_gmpi/modules/Scope/vcv/Scope.cpp:9-17`), ids **0..7**, precisely what E49's runtime diagnostic prints. **So the diagnostic is explained by the saved document, not by anything the loader drops** — which is worth knowing, because "the loader loses them" and "they were never written" want different fixes.

**(3) But the Scope is not singled out, and that half of the lead does not survive.**

```
module handle  params  type            module handle  params  type
   987654321        0  VCV: Scope         249916321       20  VCV: Pulses
  2064520790        0  VCA                529566147       12  VCV: LFO
   356931408        0  VCA                 13300239       12  VCV: LFO2
   174695687        0  VCA               1242924866        6  VCV: SHASR
  (+ 4 TiDE Patch Point Out, 2 IO Mod, SE MIDI to CV 2, 2 Container — all 0)
```

**13 of 19 modules carry no patch parameters**, and `VCA` declares 3 of its own, so it is missing them too. Having none is the common case in this document, not the anomaly.

**(4) The module that actually faults has twelve.** E53's last line before the fault is always `LFO2`. So *"a parameter-less module faults later"* is not what the evidence says, and the next run should not start from it.

### Also checked, since I built anyway

`main` builds green on Windows: `cmake --build build --config Release --target TIDE_Rack_STANDALONE`, **rc=0**, producing `TIDE-Rack.exe`. No open `platform:win` issue — though per this cell's standing note, `build.yml:409` excludes the windows leg from filing them, so that emptiness verifies nothing on its own.

**Two CI runs on `main` have been `pending`/`in_progress` since 2026-08-27T19:38Z** (`33109385595`, `33109201343`), with zero jobs materialised. Not a build break — a queued or gated workflow — but nothing has reported on `7c74dd5` and nobody has said so.

**Learned:**

- **A reproduction that lives in `_scratch/` is not a reproduction.** Three rows named the same untracked path on one machine as the way to reproduce them. It survived by luck this time; committing it costs 51 KB.
- **Two sessions on one Windows box cannot both test the standalone, and nothing tells you until you look.** The config root is redirectable on the other two platforms, so this is a Windows-only tax nobody had filed — and the fleet's per-platform results are less comparable than they look because of it.
- **The rule that made E48 ineligible is not in the status column.** A row can read `TODO` and be blocked by an open product decision recorded inside its own text. Two NEXT cells and two days pointed here before anyone read the row that far.
- **"That module has no parameters" is only a finding once you count the others.** It was true of the Scope, and equally true of twelve other modules in the same document — and the module that faults has a full set. One census turned a lead into a refutation.
- **Declining to contend has to apply to source as well as to state.** Having refused the config folder, editing the `GMPI_Wrappers` checkout the other session was compiling from would have been the identical mistake with a longer fuse.

**Next:** **E55** is the row for this box — small, ALLOWED repo, and it is what makes **E53** measurable here again; E53's fixture is now committed and its first stage is still a faulting address, with the parameter lead corrected above. **E48 needs one sentence from Jeff** and nothing else. **E52's Windows `GMPI_STANDALONE_COMMAND_CHANNEL=OFF` build is still unverified** and is one configure away on this box.

**Machine state.** All six repos were on their default branches and clean at the start; `TideSynth` is on the branch below and returned to `main` at the end. **`RackModules/AR_jef.synthedit` went dirty at 07:55 with real content changes** (view zoom, panel window rects) — not mine, not CRLF churn (`git diff --ignore-all-space` still shows content), written by the `SynthEdit2.exe` the other session has open on E48's prefab. Left exactly as found, per STEP 5's third kind of dirt. **`%APPDATA%\TIDE Rack\` was never written by this run** — I copied it aside at 07:50 and then never restored it, deliberately, because by then it was the other session's live experiment and not a state of Jeff's to put back. No TIDE or cdb process left running by me. **At the end, `GMPI_Wrappers` is parked on `tide/win/E51-argv-win-linux` with `windows/MainWin32.cpp` and `linux/MainWayland.cpp` modified** -- it was on `main` and clean when I checked at 07:45, so that is the other session's live work, not mine and not stranded as far as I can tell. Recorded so the next run here does not read it as its own leftovers. The other five repos are on their default branches.

### STEP 4 bookkeeping: two rows flipped, two deliberately not

Every linked PR checked with `gh pr view`, not inferred:

| row | linked PRs | state | action |
|---|---|---|---|
| **E25** | `SynthEditLib#58`, `#513` | both MERGED | **DONE**, archived |
| **E54** | `#514` | MERGED | **DONE**, archived |
| E52 | `GMPI_Wrappers#23`, **`#28`**, `#515` | **#28 still OPEN** | stays IN-REVIEW |
| E45 | `#503`, `#504` | both MERGED | **left alone on purpose** |

Both flips are on the **Accept** as well as on the merge, which is the E49/E50 lesson: E25's own first line is *"THE ACCEPT IS MET"*, and E54's gate was re-run against its shipped fixture by the 2026-08-28 macos run after merging.

**E45 is the interesting one, and I did not re-decide it.** Its PRs have merged, so the literal STEP 4 rule would flip it — but the 2026-08-28 macos run considered exactly that and declined, because the row says the check *"exists and enforces nothing"* until one line lands in `lint.yml`, which the bot token cannot write. That is an Accept-not-met argument and it is still true. Overturning another run's stated, reasoned call on a row I have not worked is the *"do not half-apply another run's deliberate bookkeeping"* mistake from two entries below.

**Branch/PR:** `tide/win/E53-fixture-segv`, [#516](https://github.com/JeffMcClintock/TideSynth/pull/516) — the fixture, its README, E53's annotation, E55, the `win` NEXT cell, the E25/E54 archive, and this entry.

## 2026-08-28 — macos — #514 broke because I fixed #513, and a stacked pair will keep doing that

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell; recorded as unknown rather than guessed* · as tide-rack-bot (both)

**Did:** no backlog item, second iteration running. STEP 1.5 again: [#514](https://github.com/JeffMcClintock/TideSynth/pull/514) had gone `CONFLICTING` since the previous iteration. Resolved, pushed to the same branch. **Both #513 and #514 are now `MERGEABLE` with 12/12 checks.** No product code touched.

### The conflict was self-inflicted, and that is the point

#514's base is not `main` — it is `tide/mac/E25-document-driven-repro`, #513's branch. The previous iteration pushed two commits to that base to clear #513's own conflict, **and that is what made #514 conflict.** Nothing drifted from `main`; `origin/main` has not moved in 20 minutes (`9e64b00` both times). A reader looking for an external cause would not find one.

**So the pair is a small treadmill, and a run "helping" is what turns it.** Clearing the base breaks the stacked PR; clearing the stacked PR is another push to a branch nobody has merged. Neither PR is waiting on a run — both are green and waiting on Jeff.

**What the next run should expect:** when #513 merges, GitHub retargets #514 to `main` automatically, and it may conflict *again* at that moment, on the same coordination files. That is not a new problem and does not need pre-empting — **it needs #513 merged first, then one resolution, not two.** A run that finds only #514 conflicting and #513 already merged is in the normal case, not a broken one.

### The ordering rule is "whichever is newer", not a side

One conflict, `JOURNAL.md`, and it resolved the **opposite way** to the previous iteration: the base now carries two 2026-08-28 entries and this branch's is 2026-08-27, so the base's go above. Last time this branch's entry was the newer one and went first.

`check-journal-prepend.py` enforces newest-first *as well as* prepend-only — which the previous iteration learned by failing it. **There is no standing "ours first" or "theirs first" answer; it has to be read off the dates each time.**

### Checked for the previous iteration's failure mode

`BACKLOG.md` auto-merged with no conflict. Last iteration a **deletion outside every conflict hunk merged silently** and cost R6 its row, so this time the auto-merged file was checked rather than trusted: R6 is still in `BACKLOG-DONE.md`, E54 is still `IN-REVIEW`, and `check-backlog-diff` reports no dropped rows.

**Verification artifact — E54's own gate, untouched by this merge and still firing on the fixture it ships:**

```
$ python3 scripts/check-rack-populated.py --log-file tests/rack-content/lost-module-handle.log
  ok   default rack loaded, 25109 byte document
  FAIL parameter names module handle 999999999, which the document does not
       contain -- the rack loaded DEGRADED, missing whatever that module was.
1 assertion(s) failed -- the rack did NOT come up populated.
```

Eight lints green: `backlog-diff`, `journal-prepend`, `prompt-provenance`, `id-refs`, `backlog-archived`, `links`, `next-block`, and E54's gate.

**Learned:**

- **Resolving a base branch's conflict breaks every PR stacked on it.** Worth predicting before the push rather than discovering next iteration; the cost is one extra resolution per stacked PR, every time.
- **"Ours or theirs" is never the rule for the journal — the dates are.** Two consecutive merges on the same pair of branches resolved in opposite directions, both correctly.
- **An auto-merged file is a changed file.** "I only touched the conflicts" describes what git showed you, not what git did.

**Next:** nothing on `tide/mac/**` needs a run — both PRs are green, mergeable, and waiting on Jeff. **Merge #513 first, then #514**, so its retarget to `main` costs one resolution instead of two. Still unaddressed from the windows box's 08-27 note: `tide/mac/E36-renumber-duplicate-e34` and `tide/mac/icon-tide-app` sit on the remote with **no PR**, the one end state STEP 5 forbids.

**Branch/PR:** `tide/mac/E54-gate-lost-module`, [#514](https://github.com/JeffMcClintock/TideSynth/pull/514) — same branch per STEP 1.5, no second PR.

## 2026-08-28 — macos — STEP 1.5 was the whole run: #513 had gone CONFLICTING, and my first resolution of it was wrong

**Prompt:** b97bc00 · claude-opus-5 · app *unavailable — `claude --version` does not answer in this shell, recorded as unknown rather than guessed* · as tide-rack-bot (both)

**Did:** no backlog item. STEP 1.5 found [#513](https://github.com/JeffMcClintock/TideSynth/pull/513) (E25) `CONFLICTING`, resolved it, and pushed to the same branch. It is `MERGEABLE` again. No product code, no fixture change.

### Why this outranked a backlog row

STEP 1.5 names "failing checks, requested changes, or unresolved review comments". **#513 had none of those — 12/12 checks pass, zero reviews — and still could not merge.** A conflict is not on that list, but the intent plainly reaches it: it is this platform's PR, it is stuck, and no other box will touch a `tide/mac/**` branch. The neighbouring rule settles it the other way round too — "green with nothing unresolved is just waiting for merge, leave it alone" — and #513 was *not* that. [#514](https://github.com/JeffMcClintock/TideSynth/pull/514) **was** exactly that, so it was left alone; it is stacked on #513's branch and clears when #513 lands.

Third merge from main on this branch. E52 ([#515](https://github.com/JeffMcClintock/TideSynth/pull/515)) and the 2026-08-28 NEXT block landed since the last one.

### The resolutions, on the merits

| conflict | taken | why |
|---|---|---|
| `BACKLOG.md` NEXT block | origin/main | main's cells are dated 08-28 and already name this PR — *"#513 and #514 are both green … leave them alone"*. The branch's are 08-27 copies. |
| `BACKLOG.md` E52 | origin/main | `IN-REVIEW` beats the branch's stale `TODO`; #515 merged. |
| `BACKLOG-DONE.md` E50 | origin/main | archived on **both** sides with different dates. Kept main's 08-28 row (#508), dropped the branch's 08-27 duplicate, so E50 survives exactly once. |
| `JOURNAL.md` | both | prepend-only file, both sides added entries. |

### I GOT R6 WRONG, AND THREE LINTS CAUGHT IT INDEPENDENTLY

I dropped the branch's R6 archive row, reasoning that main still carries R6 as `IN-REVIEW` so leaving main's state alone was conservative, and that promoting a row was not this PR's job.

**It was not conservative, it was incoherent.** The branch had also **deleted** R6 from `BACKLOG.md` as the other half of the same deliberate act (`e626a0e`, *"E50 and R6 DONE and archived"*), and **that deletion sat outside every conflict hunk, so git auto-merged it silently.** I never saw it. Dropping the archive row on top of it made R6 vanish from both files:

```
check-backlog-diff     R6 missing from head, no verbatim copy in any other file
check-id-refs          12 STALE references to R6
check-journal-prepend  (separately) entries are not newest-first
```

Restored the branch's R6 row verbatim, keeping **both halves** of that run's bookkeeping. R6 is genuinely done — [#505](https://github.com/JeffMcClintock/TideSynth/pull/505) merged and `tidesynth.com` serves all five `releases/latest/download/` permalinks — so archived is the correct state, and it was that run's call to make, not mine to half-undo.

The journal failure was separate and also mine: I put the branch's 08-27 entry above main's 08-28 one, having assumed `check-journal-prepend.py` only enforced the prepend-suffix property. **It enforces newest-first as well.** Moved below.

**Verification artifact — all seven lints, after:**

```
check-id-refs           no stale ID references, no duplicate IDs, no shared live citations
check-backlog-diff      status/date cells and new rows only, OK
check-journal-prepend   prepend-only, OK
check-backlog-archived  47 row(s), none DONE, all terminated, OK (244 KB)
check-links / check-next-block / check-prompt-provenance   rc=0
```

`gh pr view 513` → `mergeable=MERGEABLE`, from `CONFLICTING`.

**Learned:**

- **A conflict is not on STEP 1.5's list of three, and should be.** No failing check, no requested change, no review comment — and unmergeable. The list reads as exhaustive and is not; the "leave a green PR alone" sentence next to it is what disambiguates.
- **A deletion outside a conflict hunk merges silently, so "I only touched the conflicts" is not a description of what you changed.** Half of the branch's R6 act was invisible to me while I was deciding the other half.
- **Do not half-apply another run's deliberate bookkeeping.** Taking one side of a two-part act produced a state neither run intended and no lint would have predicted from either input alone. Either keep it whole or leave it whole.
- **The lints are load-bearing, not ceremony.** Three of them independently caught one wrong judgement call, each from a different direction, in a diff that looked entirely reasonable.

**Next:** #513 is mergeable and waiting on Jeff; #514 clears with it. Nothing else on `tide/mac/**`. Two mac branches still sit on the remote with **no PR** — `tide/mac/E36-renumber-duplicate-e34` and `tide/mac/icon-tide-app` — flagged by the windows box on 08-27 and still true; that is the one end state STEP 5 forbids.

**Branch/PR:** `tide/mac/E25-document-driven-repro`, [#513](https://github.com/JeffMcClintock/TideSynth/pull/513) — pushed to the same branch per STEP 1.5, no second PR.

## 2026-08-28 — macos — E52: a shipping build option that did not compile, and the control that proves the fix is not a deletion (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** (there is no `claude` CLI on this box's PATH, so this is the desktop app's `CFBundleShortVersionString`, the version A13 recorded as the discoverable one on a mac) · as **tide-rack-bot** (both paths)

**Did:** took **E52**. Both Accept clauses met.
[GMPI_Wrappers#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28) off branch
`tide/mac/E52-geometry-seam-outside-channel` carries the code, PR-gated so proposed and
not merged; TideSynth's `tide/mac/E52-standalone-channel-off-build` carries the row, the
E51 annotation, E50's archive and this entry. Row is **IN-REVIEW**.

### Why E52 and not one of the six TODO rows above it

The next run will walk the same list, so here is the walk rather than the conclusion. The
`mac` NEXT cell is dated **2026-08-25** and says nothing is takeable; it predates E32–E54
and I read it as history. In file order: **S1b** and **S8** are wholly GATED
(`EditorLib/CMakeLists.txt`, `SynthEditLib/CMakeLists.txt`) and S8 is additionally MOOT
since the Oscillator HD ruling; **E38** already carries `NEEDS-SPEC`; **E19**'s mac cell
wants AU3 in a real host, which needs a human at a DAW; **E7** is an engine fact Jeff has
ruled is not a blocker; **E2**'s own row says it is not takeable; **E25** is done and
waiting on [#513](https://github.com/JeffMcClintock/TideSynth/pull/513); **E39**'s fix is
GATED and its own row says re-write the Accept before taking it; **E48**'s remaining step
is a PLAN constraint 7 product decision, and it is the `win` cell's pick.

**E51 is the one I annotated rather than skipped silently**, because it is the first row
where the reason is the row's own text: its Accept requires *"grep finds no direct
`MessageBox`/`NSAlert`"* and the row then records two such calls that are **correct as they
are** (`MainWin32.cpp:218`, `mac/MainMac.mm`'s `showFatalAlert`), concluding *"re-state it
before using it"*. Of the two things left, the `--dialogs` verb is design work the row
itself defers, and identifying the one answer-consuming call site is a grep that does not
close the row. `NEEDS-SPEC` added, naming what is missing, per the E38 precedent.

### The break, reproduced before anything was changed

`cmake -DGMPI_STANDALONE_COMMAND_CHANNEL=OFF` on **unmodified `origin/main`**, building
TIDE's standalone:

```
StandaloneApp.cpp:240:19: error: no member named 'setWindowPosition' in 'gmpi::standalone::PlatformShell'
StandaloneApp.cpp:598:15: error: no member named 'logicalSize'       in 'gmpi::standalone::PlatformShell'
StandaloneApp.cpp:621:19: error: no member named 'windowPosition'    in 'gmpi::standalone::PlatformShell'
3 errors generated.
```

Three errors, exactly those three calls, nothing else — which is the row's own account.
**One correction: the row's `:201`/`:555`/`:578` are stale line numbers.** They moved when
E32's position half landed. The calls did not.

### The fix, and the one that would have passed while deleting the feature

`logicalSize`, `windowPosition` and `setWindowPosition` move **out** of
`#if GMPI_STANDALONE_COMMAND_CHANNEL`, with all three shells' overrides. `framePixels` and
`canvasSize` stay **in**: with the channel off nothing calls them and `mcp/` is not
compiled at all.

**Guarding the three CALL SITES was the alternative and E52 called it right.** It compiles,
and silently removes window restoration from every OFF build — the same class of mistake as
the break. The row is worth quoting to itself here: *"these are window GEOMETRY, and
reopening where the user left the window is a SHIPPING FEATURE, not a test affordance."*

**For an ON build this is a pure move.** Every line was already compiled, because the guard
it sat in was true. That is the whole reason it was safe to move all three shells at once
from a box that can build one of them.

### Verification, and the control is the part worth keeping

| build (Release, Ninja, macOS, all four siblings local) | result |
|---|---|
| **OFF**, all targets | **rc=0** — GMPI, VST3, CLAP, AU3, STANDALONE all link (was 3 errors) |
| **ON**, all targets | **rc=0**, 314/314 |
| the OFF binary really is OFF | `gmpi-standalone` occurs **0** times in it, **3** in the ON one |

Clause 2 — prepare `standalone.conf`, launch, `SIGTERM` (the normal teardown, which saves),
read the file back. E32's technique, because `--info` reports no position and there is no
move verb — and on an OFF build there is no channel at all, so it is the only technique:

| saved | OFF reads back | ON reads back |
|---|---|---|
| `x=300 y=200 900x700` | **300, 200, 900x700** | **300, 200, 900x700** |
| `x=740 y=415 1020x760` | **740, 386, 1020x760** | **740, 386, 1020x760** |
| *(empty config)* | **570, 153, 1100x626** | — |

**The empty-config row is the whole reason the other two mean anything.** A build with the
feature deleted also produces a `standalone.conf` full of plausible numbers — the ones the
window happened to open at. Knowing that an unconfigured launch lands at `570,153
1100x626` is what turns "it wrote a position" into "it read mine". Two saved positions
rather than one, for the same reason: one value can be a coincidence.

**`y=415 → 386` is not a defect and is not new.** AppKit's `constrainFrameRect:toScreen:`
pulling the window fully onto a 2240x1260-point display, `386 + 760 = 1146`. Both arms show
it identically, and it is the platform behaviour Jeff ruled on for E32 on 2026-08-27.

### What was NOT verified, stated rather than implied

**Windows and Linux were not compiled.** This box builds neither, which E52 predicted
(*"it touches all three shells and wants a box that can build each — or three runs"*). What
was done instead is a read: every member the moved bodies touch is declared outside the
guard in both shells — `ToplevelWindow window_` and `window_.frame()` in `MainWin32.cpp`,
`WaylandToplevel frame_` in `MainWayland.cpp`. `FrameCapture capture_` is the only guarded
member in either, and no moved body names it.

**The bound on that risk is structural, and it is why one box was enough.** Their ON builds
cannot change, because the moved text was already inside a TRUE guard; their OFF builds
cannot regress, because they do not compile today. So the worst case is that an OFF build
stays broken somewhere, which is the state before this change.

`SE16` does not compile `wrapper/Standalone/**` at all — checked, not assumed — so
SynthEditCL and SynthEdit are not consumers of this and did not need rebuilding.

### E50 archived, and the lint that caught my own note

STEP 4 bookkeeping: **E50** was IN-REVIEW with
[#508](https://github.com/JeffMcClintock/TideSynth/pull/508) merged. Flipped **DONE** and
moved to [BACKLOG-DONE.md](BACKLOG-DONE.md) — **on the Accept, not on the merge**, which is
yesterday's E49/E46/E47 lesson: its Accept is an either/or (*"either the Compare is
accounted for … or it stops being constructed"*) and the row's own first line records the
second limb as met by measurement.

**E45 and R6 were NOT flipped, deliberately.** E45's PRs both merged and its row says the
check *"exists and enforces nothing"* until one line lands in `lint.yml`, which the bot
token cannot write. R6's row states no Accept at all, so there is nothing to check it
against; both left alone and named here instead.

**Archiving E50 turned the `win` NEXT cell red**, and the failure is worth writing down.
`check-next-block.py` reads a take-phrase inside its own SENTENCE, and the cell preserves
its previous re-pointing verbatim — including a `TAKE` clause naming E50 from two days ago.
It was true when written and is now an instruction to take archived work. Defused in place
with a visible marker, because a correction appended afterwards is a different sentence and
the lint cannot see it. **Then my own explanatory note re-armed it**, by quoting the
defused phrase: the quote is itself a take-phrase in a fresh sentence with no negation in
it. Reworded to describe the phrase rather than reproduce it.

**Learned:**

- **An absent control makes a passing round-trip worthless.** A build with window
  restoration deleted still writes a full `standalone.conf`, because it saves whatever the
  window opened at. Only the empty-config launch — `570,153 1100x626` — separates "it
  restored mine" from "it reported its default", and it costs one extra launch.
- **A "pure move" is the strongest argument available for editing code you cannot build.**
  The Windows and Wayland edits are unverifiable from here, and they are still safe,
  because moving text out of a guard that was TRUE cannot change what that build compiles.
  Say the invariant, not "it should be fine".
- **Guarding the call site is the fix that passes and deletes the feature.** Three `#if`s
  would have turned this red build green in five minutes, with window restoration silently
  gone from every OFF build and nothing to notice it.
- **Quoting a lint's trigger re-arms it.** I defused a stale `TAKE` phrase and then
  reproduced it verbatim in the note explaining the defusal, which is a fresh armed
  sentence. Describe the pattern; do not paste it.
- **Check a lint by its exit code, not by the tail of its output.** I very nearly recorded
  `check-next-block.py` as green because I piped it through `tail` and read `$?` from the
  pipe. The archive already carries this lesson from 2026-08-27 (windows) and I repeated it
  the same day I read it.
- **A NEXT cell three days old is history, not a queue.** The `mac` cell said nothing was
  takeable and predated eighteen filed rows. Reading it as current would have ended the
  session with no work done.

**Next:** **E51**'s two remaining pieces need the spec named on its row before a run can
take it — that is one decision, not a task. **E52's Windows and Linux OFF builds are
unverified** and are one command each on the boxes that can run them: if either box has a
spare moment, `cmake -DGMPI_STANDALONE_COMMAND_CHANNEL=OFF` against
[#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28) is the whole check.
**E53** still wants a faulting address; **E48** still wants a product decision.

**Machine state.** All six repos on their default branches at the start and clean;
`TideSynth` and `GMPI_Wrappers` now on the branches above, returned to their defaults at
the end. Two scratch build trees (`e52-off`, `e52-on`) in the session scratchpad, outside
every repo. `~/Library/Application Support/TIDE Rack/` was copied out before the first
launch and **restored byte-for-byte, md5-verified** — the three files are Jeff's, not this
run's. No TIDE process left running.

**Branch/PR:** `tide/mac/E52-geometry-seam-outside-channel` in GMPI_Wrappers
([#28](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/28), the code) and
`tide/mac/E52-standalone-channel-off-build` in TideSynth
([#515](https://github.com/JeffMcClintock/TideSynth/pull/515)) (E52's row, E51's `NEEDS-SPEC`,
E50's archive, the `win` NEXT cell's defused phrase, the `mac` NEXT cell, and this entry).
**Merging TideSynth's side alone changes no behaviour**; merging GMPI_Wrappers' alone
leaves the backlog saying the work is open.
## 2026-08-27 — windows — The second gate was __argv itself, the whole quiet chain now measures working, and E46's guard fired in production (interactive, Jeff directing)

**Prompt:** *"find the second gate"* · Fable 5, `claude-fable-5` · as **tide-rack-bot**

**Did:** found it, fixed it on
[GMPI_Wrappers#29](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/29)'s
branch (`ed03305`), and measured the whole chain end to end. **E46 goes DONE and
is archived** — its Accept was met by observation on the way. E48 gains a third
missing module. E51's mechanism is now real on Windows, pending #29's merge.

### The gate, and why the first fix could not have worked

**In a Unicode build entered through `wWinMain`, the CRT populates only
`__wargv`; `__argv` stays NULL.** The first commit on #29's branch passed
`__argc, __argv` — a count without its strings — and `applyCommandLineConfig`
correctly refused the pair. Exactly as inert as the `(0, nullptr)` it replaced,
one line later.

**The trace found it in one launch.** On the previously-rebuilt exe:

```
TIDE: command line: argc=2, argv=NULL
```

`argc` arrived; `argv` did not. No amount of stack reading had produced that —
three offline analyses ruled things *out*, and the one line of instrumentation
ruled the answer *in*. That is what "instrument the app" bought.

The fix converts `__wargv` to UTF-8 once, into function-local statics, because
`gArgv` aliases the pointers for the life of the process. Still the CRT's own
parse — same splitter that fed `wWinMain`, nothing to `LocalFree`, no second
parser to disagree with the first.

### The chain, measured end to end on the fixture that blocked every run today

```
TIDE: command line: argc=2 -quiet
TIDE: parsed quiet=1 rescan=0
Logging dialogs to stderr, and keeping them for --dialogs.
...
Module not found in factory: SE Scope3 XP
Module not found in factory: SE Oscillator
Module not found in factory: SynthEdit ADSR
3 connector(s) in "Sine" could not be restored ...
5 connector(s) in "AR" could not be restored ...
command channel: \\.\pipe\gmpi-standalone.30644
```

**The restore completes. The channel opens. `--dialogs` answers count=6** with
every title and text intact — the two `Connectors lost while loading` reports
carry their captions, the factory prompts their empty ones. Jeff's question from
this morning — *"is the dialog fixed so it reports to MCP?"* — is now **yes**,
measured, on this branch.

### Three modules, not two — and the best census was never the check

The unblocked restore enumerates **`SE Oscillator`** as missing too.
**It passed BOTH of the check's screens**: its string occurs in the binary by
coincidence and no staged XML describes it, so offline screening scored it
registered. Third correction to that count today (one → two → three), and the
lesson has stabilised: **the authoritative census is one `-quiet` launch of the
fixture and a read of stderr or `--dialogs`** — the app names exactly what it
cannot resolve, per container, connectors included. The offline check stays as a
cheap screen; its pass proves nothing.

### E46's Accept, met by an event nobody staged

Three times in that log:

```
SynthEdit: parameter names module handle 481794193, which this document does not contain -- parameter left with no module.
```

That is the guard from
[SynthEditLib#64](https://github.com/JeffMcClintock/SynthEditLib/pull/64) firing
on a **real saved document** in a Release build — the exact condition E46 was
filed on, reached by the route the row predicted second (a parameter whose
module was dropped) via E48's missing-prefab-module class. The document loaded
degraded, the rack built, the app ran on. Every crafted document had failed to
reach the condition; a genuinely broken prefab reached it naturally. **E46 is
DONE and archived.**

**Learned:**

- **`__argv` is NULL under `wWinMain` in a Unicode build.** A count forwarded
  without its strings is as inert as no count, and the compiler is happy with
  both.
- **One line of instrumentation beat three offline analyses.** The stack, the
  vcxproj and the source each ruled something out; only the app saying
  `argc=2, argv=NULL` said what was true.
- **A guard's best verification can arrive as a side effect.** E46's Accept was
  unreachable by construction for two days of crafted documents, then a real
  defective prefab met it in passing.
- **When the product can report on itself, prefer that to screening it from
  outside.** Three rounds of check-corrections converged on: launch it quiet and
  read what it says.

**Next:** #29 is ready for Jeff's review — with it, every "save and reload"
Accept on Windows becomes drivable headlessly, which is the population E44 was
counting. E48's remaining work is unchanged and now three modules wide. E53
still wants its faulting address.

**Machine state.** `RackModules/AR_jef.synthedit` still dirty — Jeff's, left
untouched. One regret owned in the moment: an unconditional pre-sweep killed the
TIDE instance Jeff had the dialog open in (PID 25328). All repos on expected
branches; `%APPDATA%\TIDE Rack\` restored and md5-verified; nothing of ours
running.

**Branch/PR:** rows and this entry on `tide/win/E51-argv-trace`
([#518](https://github.com/JeffMcClintock/TideSynth/pull/518)); the code is the
second commit on [GMPI_Wrappers#29](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/29).

## 2026-08-27 — windows — The instrument said argc=0; then Jeff photographed the dialog and it named a different module than I had (interactive, Jeff directing)

**Prompt:** *"is the dialog fixed so it reports to MCP?"* → *"chase it"* → *"instrument the app to find out"* · Opus 5 (1M context), `claude-opus-5[1m]` · as **tide-rack-bot**

**Did:** answered the question — **no** — found the measured cause of quiet mode
being inert on Windows, proposed the fix as
[GMPI_Wrappers#29](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/29)
(PR-GATED, not merged), landed a permanent command-line trace, and **corrected
E48 and the check I had shipped hours earlier**.

### Answer: no, and here is why, in one line per shell

```
int runStandaloneApp(PlatformShell& shell, int argc = 0, char** argv = nullptr);
```

**Defaulted parameters, and only the mac shell passes them.** Windows and Linux
both called `runStandaloneApp(shell)`, so `gArgc`/`gArgv` stay `0`/`nullptr`,
`applyCommandLineConfig` bails at `argc <= 1 || !argv`, `SetQuiet()` is never
called, and **`-quiet` has never done anything on two of three platforms.**

The defaults are what made it silent — without them the shells would not have
compiled.

**The instrument settled it in one launch:** `TIDE: command line: argc=0,
argv=NULL`. That trace is now permanent (`SynthEditController.cpp`,
standalone-only), because this function failing open is otherwise invisible:
every flag simply does nothing and the symptom looks like the default.

**The corroboration was already in every log I had.**
`CSynthEditAppBase::SetQuiet()` announces itself — it sets `quiet` then calls
`SeMessageBox("Logging dialogs to stderr, and keeping them for --dialogs.")`.
Across every `-quiet` launch of a long session that line appeared **zero** times.
I had the positive control the whole time and did not think to look for it.

**Necessary but not sufficient, measured:** with argv forwarded and the exe
relinked, `SetQuiet()` **still** does not fire and E48's modal still blocks. A
second gate remains and I have not found it. Ruled out: the `GMPI_STANDALONE`
guard (target-wide on the standalone), ordering (`applyCommandLineConfig` runs in
`initialize`, before `setParameter`), and the divert itself.

### And then the photograph

Jeff sent a screenshot of the actual box:
**`Module not found in factory: SE Scope3 XP`** — from `Sine_jef.synthedit`, not
the `SynthEdit ADSR` in `AR_jef.synthedit` that I had named as *the* cause hours
earlier and merged into E48.

**Both are real. Neither is "the" cause. The defect is a class:** shipped prefabs
using modules TIDE does not register.

**Why my check missed the one the user actually sees.** It tested only *absence
from the binary*. `SE Scope3 XP` **is** a string in the exe and **is** described
by the staged `ControlsXp.xml`, so it scored as present. But **staging an XML
enriches the pins of a module TIDE already registers; it registers nothing.**

And the measurement of exactly that has been printed at every startup all
session:

```
TIDE: ControlsXp.xml enriched 2 of 18 described class(es)
```

**Sixteen described classes TIDE does not have.** I read past that line in a
dozen logs, including in the entry where I quoted the surrounding lines verbatim.

The check now runs two screens and catches both — with the false-positive
direction stated, because (b) would wrongly flag a module both registered in C++
and described in a staged XML.

**Learned:**

- **A screen that catches one instance is not a diagnosis of the class.** I found
  one missing module, stopped, and wrote "the cause" into a merged row. The
  second one was in a different prefab and was the one on screen.
- **Present-in-the-binary does not mean registered.** I wrote in the script's own
  docstring that presence proves nothing and absence is decisive — and then
  treated a single absence hit as the complete answer. Being right about the
  logic did not stop me misusing it.
- **The instrument you need is often already printing.** `enriched 2 of 18` and
  `SetQuiet`'s announcement were both in every log; each would have saved hours,
  and I went looking for new instruments instead of reading the ones running.
- **Default arguments can turn a missing call into a silent no-op.** Two of three
  shells threw the command line away and nothing failed anywhere.
- **A photograph from a human beat three offline analyses.** The stack told me
  which call site; only the screen told me which module.

**Next:** the second gate on quiet mode. The trace now landed is the tool for it —
the next step is one line reporting whether `SetQuiet()` was reached. And **E48's
fix is Jeff's, now twice over**: two prefabs to re-author, or two modules to add
to the compiled-in set.

**Machine state.** `RackModules/AR_jef.synthedit` is dirty in the working tree —
**Jeff's, being edited in SynthEdit 1.6 while this ran, left untouched.** A
TIDE-Rack he launched is showing the dialog above; not mine and not swept. All
other repos on their default branches and clean; `%APPDATA%\TIDE Rack\` restored
and md5-verified.

**Branch/PR:** `tide/win/E51-argv-trace` — the corrected check, the command-line
trace, E48's and E51's corrections, and this entry. The wrapper fix is
[GMPI_Wrappers#29](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/29),
proposed only.

## 2026-08-27 — macos — E54: the gate reads the library's diagnostic now, and the obvious place to put it would have matched nothing (scheduled run, continued)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude Code (no `claude` on this box's PATH) · as **tide-rack-bot** (both paths) · continued from the E25 entry below at Jeff's *"fix E54"*

**Did:** built **E54**, the row this run filed an hour earlier. Its Accept ran live on a real standalone, both arms. Branch `tide/mac/E54-gate-lost-module`, stacked on `tide/mac/E25-document-driven-repro` because E54's row exists only there until [#513](https://github.com/JeffMcClintock/TideSynth/pull/513) merges.

### The one-line fix was in the wrong place, and it would have failed silently

E54's own row sized this as *"one entry in `FATAL_LINES` plus the negative control"*. **That entry would have matched nothing.** The loop is

```python
for needle, why in FATAL_LINES:
    for line in text.splitlines():
        if needle in line and "TIDE:" in line:
```

and the message is `SynthEdit: parameter names module handle N, which this document does not contain` — prefixed **`SynthEdit:`**, because it comes from `CPatchManager::InitModulePointers` in **SynthEditLib**, not from `TideApp.cpp`. The constant's own comment says *"Each is a real message in TideApp.cpp"*, and that sentence is the guard rail; I only read it because I was about to add a line underneath it.

So the check would have been **added, committed, reviewed and green, while asserting nothing** — the silently-disarmed check that file warns about, arrived at from a different direction. It is a separate `LOST_MODULE` regex instead, which also lets the failure **name the handle**: "a module is missing" sends the reader to the wrong repo, and the number is what they grep the document for.

**I wrote the sizing in that row myself, three hours earlier, from reading the same file.** A row's size estimate is a claim about code the estimator did not open.

### The Accept, run live rather than from a log

Same binary in both arms; the document is the only variable.

| arm | result |
|---|---|
| default rack, one `<param module=>` → `999999999` | **exit 1**, `FAIL parameter names module handle 999999999 …` |
| stock default rack | **exit 0**, `rack is populated.` |

And the four negative controls in `tests/rack-content/` all still exit 1, so nothing was disarmed on the way in.

### The new fixture is load-bearing in the opposite direction to the old one

`lost-module-handle.log` is a real capture, and what makes it worth keeping is that **every positive assertion the gate makes is present and healthy** — four XMLs enriched, five prefabs seeded, `default rack loaded, 25109 byte document` — on a rack that is missing a module.

`silent-empty-rack.log` exists because a negative-line scan passes an ABSENT line. This one exists because a positive-line scan passes a PRESENT one. M8's note says the positive assertions are the load-bearing half; that is true of the case it was written about and **not true in general**, and this folder now holds the counterexample.

### What I did not do, stated rather than implied

**This covers `--standalone` and `--log-file` only, not `--au3`.** That arm reads os_log; `TideApp` mirrors its own diagnostics there precisely because an app extension's stderr reaches nothing, but this message is `std::cerr` inside SynthEditLib, which knows nothing about os_log. **So an AUv3 that lost a module is still invisible to this gate.** Checked by reading both files rather than assumed. Closing it means routing the library's diagnostics through the same channel — another repo, and not E54's job.

**Learned:**

- **"Add it to the existing list" is a claim about the list's matching rule, not just its contents.** The obvious entry here would have been inert, and every test that mattered would have been green.
- **A row's Size estimate is a claim about code the estimator did not open.** I wrote this one's "one entry in FATAL_LINES" from the same file three hours earlier and it was wrong about the only detail that mattered.
- **A guard that makes a crash survivable can blind the gate that caught it.** E46's fix was right and it cost this gate its coverage of that case; nothing would have reported the loss. Worth asking, whenever a silent-failure fix lands, what used to notice.
- **A fixture folder can hold two load-bearing cases that argue opposite ways** — absent-line and present-line — and a note explaining one of them will be read as a general rule unless the other is there too.
- **Say which arms a check covers when the channels differ.** os_log and stderr are different pipes, and a check that reads one is not a check on the other.

**Next:** nothing outstanding on E54. The AUv3 gap above is real, is not filed, and is a SynthEditLib change — worth a row only if someone wants the gate to cover the extension.

**Machine state.** All six repos on their default branches and clean at the end; TideSynth on this run's branch until STEP 5. The scratch build tree is under the session scratchpad, outside every repo, and removed. Every standalone ran under an isolated `HOME`; no TIDE process left running; Jeff's `~/Library/Application Support/TIDE Rack/` untouched, verified by mtime and size. No new crash reports — nothing in this entry faults.

**Branch/PR:** `tide/mac/E54-gate-lost-module`, **based on `tide/mac/E25-document-driven-repro` rather than `main`**, because E54's row is only on that branch. Kept separate from #513 deliberately: E45's own argument is that a check and a bulk change are not reviewable together, and #513 is a fixture plus five rows. If #513 merges first GitHub retargets this to `main`.

## 2026-08-27 — macos — E25 reproduced from a document, and STEP 1's stale issue turned out to be E46 crashing in the wild (scheduled run)

**Prompt:** b97bc00 · Opus 5, `claude-opus-5` · app Claude Code (no `claude` on this box's PATH, and `claude --version` reports `command not found`, so the version A13 calls discoverable is not available here — recorded rather than guessed) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, which matches the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** STEP 1 on [#491](https://github.com/JeffMcClintock/TideSynth/issues/491) — diagnosed, closed. Then took **E25**, the `platform: mac` row the NEXT cell said did not exist, and **met its Accept**. Filed **E54**. Annotated **E46** with the reproduction it says it lacks. No product code changed in any repo.

### The NEXT cell was five days stale and said this box had nothing

The `mac` cell asserted *"There are still NO `platform: mac` TODO rows"*. The Status column said **E25 | TODO | mac**. STEP 2 is explicit that eligibility lives in the Status column alone, and it is the only reason this run had an item. Walking the rows above it in file order, with the reason each was skipped on its own row rather than only here: **S1b** and **S8** wholly GATED, **E38** carries `NEEDS-SPEC`, **E19**'s mac cell wants AU3 in a real host, **E7** and **E2** are product decisions.

### STEP 1: #491 was real, reproduces at its own commit, and is not a macOS defect

The issue named a branch that had been **deleted ten minutes before the issue was filed** — `build.yml`'s close-on-success step can never fire on such a branch, which is A33's case, and `watchdog.yml` (which closes exactly these) had not run since 2026-08-26T06:14Z.

It was not a compile error. The build succeeded and `check-rack-populated.py` fired: *"no 'default rack loaded' line"*.

Rebuilt the pair CI actually saw — TideSynth `0876c3ac` with `SYNTHEDITLIB_FOLDER_OVERRIDE` pinned to SynthEditLib `4f334b9` — **and it reproduces**. Two things ruled out first, so nobody re-checks them: a stale `session.xml` in the runner's `$HOME` (gate passes with and without one) and E42's `PanelLocationCenter` change to the same file (passes with either version staged).

**The trigger is a cross-repo straddle.** `0876c3ac` pointed `DefaultRack.synthedit` at `<module type="MIDI In NL">`; `MIDI In NL` had **zero** occurrences in SynthEditLib at `4f334b9` and one in `main` today. TideSynth's half merged 01:08Z, SynthEditLib's (`b031ec6`) at 02:29Z. **81 minutes, and the run started at 01:06Z, inside the gap.**

### I called it a silent failure, and it was a crash. Reading the exit status is the fix

I wrote *"the app said nothing — no error, no warning"* into a comment on the issue, from a stderr that ends after `5 rack prefab(s) seeded`. **It ends because the process was gone.** `~/Library/Logs/DiagnosticReports` had two identical reports:

```
EXC_BAD_ACCESS  KERN_INVALID_ADDRESS at 0x8b890660a94c2680 (possible pointer authentication failure)
  CUG::GetPlug(int) <- CContainer::getIgnoreProgramChange() <- PatchParameter_base::ExportXml
  <- CPatchManager::ExportXml <- CContainer::ExportXml <- TideApp::exportChunkXml <- importChunkXml
```

**That is E46, reproduced in the wild** — and E46's row says in its own words that it was read off the source with no repro. The module never existed, so its handle never entered `uniqueIds`, and `InitModulePointers` at `4f334b9` was `assert(it != uniqueIds.end())` followed by a bare deref. The parameter took a garbage module pointer.

**E46 and E25 are the same stack reached two ways, and the faulting address separates them:** `0x50` is a NULL container (E25); a PAC-failing address is a WILD one (E46). Both guards are on `main` now — `f85cf73` and `796bbc2` — and `796bbc2` landed at **08:05Z, seven hours after the crash it explains**. Verified the guard works rather than assuming: a document naming handle `999999999` now prints *"parameter names module handle 999999999, which this document does not contain"* and the app loads.

Corrected on the issue rather than left standing.

### E25: the three earlier attempts failed on one word of spelling

The row records three crafted documents that did not reproduce, and recommends driving the Properties pane toggle instead. **The document route works; it was being spelled wrong.** Those attempts set `ignoreProgramChange="0"` — the attribute `PatchParameter_base::ExportXml` writes for an **exported plugin** (`PatchParameter.cpp:435`). The attribute a **document** carries is `ignorePC`, from the `SerialiseB` reflection list at `PatchParameter.h:101`, and any shipped prefab shows it (`modules/Filters/Lookahead.synthedit:143`). One field, two serialisations, two names.

The fixture is `DefaultRack.synthedit` plus **one attribute pair**:

```
<param type="10" handle="1100194740" private="true" hostControl="49" module="1996595734" ignorePC="false">
```

Both halves are load-bearing, and neither alone reaches the deref: `m_ignoreProgramChange` **defaults to `true`** and `true` short-circuits before `module()` is read; and `module="1996595734"` is the `<master_container name="Main">`, the one object whose `Container()` is null (`DocOb.cpp:40`, *"special case for 'Main' container"*). `CPatchManager::Import` reads the attribute and `InitModulePointers` binds it out of `uniqueIds`, which every object joins via `uniqueIds[Handle()] = this` (`CUG.cpp:1216`) — master container included.

**Measured. Same TideSynth `main` (`2612a2d`) in every cell; the guard is the only variable:**

| | stock `DefaultRack.synthedit` | the fixture |
|---|---|---|
| **`f85cf73` reverted** | no crash, `default rack loaded, 25110 byte document` | **SIGSEGV, exit 139**, `KERN_INVALID_ADDRESS at 0x50` |
| **`main`** | no crash, `25110 byte document` | no crash, `default rack loaded, 25147 byte document` |

`0x50` is the address the original report named, which is the whole evidence this row turns on. The left column is the **control** — it is what makes the fixture the variable rather than the build. Crash reports: **one per faulting run, zero in the other three cells**, so the Accept's *"crash-report count before and after"* has a before again after macOS rotated the originals away.

**The A/B arm had to be built by reverting the guard, not by checking out a pre-`f85cf73` `SynthEditLib`.** That was tried first and does not compile: TideSynth `main` calls `takeDivertedPrompts`, which E51 added *after* the guard. The same cross-repo coupling that caused #491, hit twice in one session.

### E54: E46's fix opened a hole in the shipping gate

Before the guard, a straddle crashed and `check-rack-populated.py` caught it by the absent line. **After the guard it loads degraded and the gate passes** — `rack is populated.`, exit 0 — on a rack missing a module, while the app printed the reason two lines earlier. Measured, not inferred. That is the M5 shape the script was written to stop, reintroduced by a fix that was right to make.

### STEP 4 bookkeeping

**E50** and **R6** had every linked PR merged and no clause left open in their own words, so both are DONE and moved to `BACKLOG-DONE.md` verbatim. PR state read with `gh pr view`, not inferred from a merge commit; R6 names a branch rather than a number (A22), so its PR was resolved from the head ref — [#505](https://github.com/JeffMcClintock/TideSynth/pull/505).

**E45 was NOT flipped, deliberately.** Both its PRs merged, but its own row says the check is not wired into `lint.yml` and *"until it is, the check exists and enforces nothing"* — that line needs Jeff, because the bot token has no `workflow` scope. DONE would be false, which is the E32 precedent exactly.

Archiving E50 then failed `check-next-block`: the `win` cell still carried a literal `TAKE **E50**` in its "previous cell follows" history, which the lint correctly reads as a live directive. Reworded as history rather than deleted — **my archive broke it, so my branch fixes it**.

**Learned:**

- **A truncated stderr and a crashed process look identical from the log.** Check the exit status before writing "it said nothing" — I put that sentence in a public comment and had to correct it.
- **One field can have two serialised names, and a row can spend three attempts on the wrong one.** `ignoreProgramChange` is the export attribute; `ignorePC` is the document attribute. Grepping a shipped file for the attribute settles it in one command.
- **When a row recommends a GUI route, check whether the document reaches the same state.** E25 asked for the Properties toggle, which the command channel cannot click; two attributes did it with no gesture at all.
- **Revert the fix rather than checking out the tree that predates it.** A months-old sibling will not compile against today's consumer, and reverting keeps the fix as the only variable — which is the whole point of the A/B.
- **A guard that stops a crash can blind the gate that caught it.** Worth asking, every time a silent-failure fix lands, what used to notice and whether it still does.
- **A stale NEXT cell is more dangerous than an empty one**, because it reads as a measurement. Five days old, and wrong about the one row this box could take.

**Next:** **E54** is small and this run filed it with its Accept as a command. **E52** is this box's own find and is ALLOWED code. **E51's** remaining grep — the one call site that consumes a dialog answer — has still not been run by anybody. E25's fixture covers the standalone only; no wrapper and no host was involved.

**Machine state.** `main` green on macOS, verified locally by a CI-equivalent build (all four siblings `[fetched]`, no overrides) of `2612a2d`: configure rc=0, build rc=0, 0 errors, five artifacts. Zero open `platform:mac` issues. All six repos on their default branches and clean; TideSynth on this run's branch until STEP 5. Four scratch build trees and three worktrees, all under the session scratchpad and outside every repo, removed at the end. Every standalone ran under an isolated `HOME` in the scratchpad and all were killed; **Four `.ips` crash reports from this run's deliberate faults are in `~/Library/Logs/DiagnosticReports` and were left there** — they are OS diagnostic logs, not ours to delete, but a later run counting reports for E25 or E46 should know they are mine (two from the #491 repro build, two from the E25 no-guard arm) and not a live defect. **Jeff's `~/Library/Application Support/TIDE Rack/` was never written to** — the one file copied out of it was read-only, and the isolation was verified rather than assumed (the app loaded the default rack rather than his 46,890-byte session). Two mac branches still sit on the remote with no open PR — `tide/mac/E36-renumber-duplicate-e34` (its PR #445 was CLOSED unmerged) and `tide/mac/icon-tide-app` (PR #435 merged, branch not deleted); noted by the windows box yesterday, still not mine to unwind.

**`main` moved under this branch mid-session** — [#511](https://github.com/JeffMcClintock/TideSynth/pull/511) archived E32/E34/E42 and added a note to E25 saying this box was working on it, which was true and is now superseded. Merged rather than rebased, because the claim commit was already pushed and STEP 4 forbids rewriting a pushed commit. Both conflicted files were reset to `origin/main`'s version and my edits re-applied on top, so their archive work is intact rather than resolved around; their journal entry is byte-for-byte unchanged below mine.

**Branch/PR:** `tide/mac/E25-document-driven-repro` — TideSynth only: the fixture and its README, E25's row, E46's annotation, E54, the `mac` NEXT cell, and this entry. **No product code in any repo**, so there is nothing here that can break a build.
## 2026-08-27 — windows — E48: a shipped prefab uses a module TIDE does not ship, and that one fact explains both dialogs and the 3,577 bytes (interactive, Jeff directing)

**Prompt:** *"take next windows task"* · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** · as **tide-rack-bot**

**Did:** took **E48** (the `win` NEXT pick). Diagnosed it end to end with a stack.
Landed `scripts/check-prefab-modules.py`, which fails on the defect. **Row stays
TODO**: the remaining step is a product decision, not a task. Annotated **E51**
with a measured instance of its own gap.

### The cause, in one sentence

`RackModules/AR_jef.synthedit` — a **shipped prefab** — contains
`<module type="SynthEdit ADSR">`, and TIDE neither compiles that module nor
stages its XML. Measured: the string occurs **0 times** in `TIDE-Rack.exe` in
either UTF-8 or UTF-16, while all 32 other module types across the five shipped
prefabs are present.

### The chain, each step measured

1. Inserting the prefab **appears to work** and the rack looks right.
2. The saved document therefore carries `type="SynthEdit ADSR"`.
3. On reload `CContainer::ImportChildren` cannot resolve it and raises
   `SeMessageBoxAsync("Module not found in factory: …", L"", MB_OK)`
   (`CContainer.cpp:1089`).
4. That is a **blocking `MessageBoxW`** running a nested `SoftModalMessageBox`
   pump on the main thread, so the restore never finishes — no further stderr,
   no `building rack from`, no command channel, **~0.08 s of CPU**.
5. The module is then `continue`d past, so its connectors are dropped and the
   *"Connectors lost while loading"* dialog this row was filed on follows at
   `:1143`.

**One cause, both dialogs, and the 3,577 bytes.**

### The stack, because reading the source would not have settled it

```
wWinMain → runStandaloneApp → SessionState::restore → StandaloneHost::restoreState
 → notifyControllerOfPreset → SynthEditController::setParameter
  → TideApp::importChunkXml → CSynthEditDocBase::ImportModules
   → CContainer::Import → CContainer::ImportChildren
    → ApplicationBase::SeMessageBoxAsync → USER32!MessageBoxW
     → SoftModalMessageBox → IsDialogMessageA → PeekMessageW
```

**Attach, do not launch.** An earlier attempt launched the app *under* `cdb` and
hung for seven minutes with nothing to show, because the modal pumps inside the
debuggee before the debugger has anything to report. Attaching to an
already-blocked process answers in seconds.

### Two corrections to the row, and the second cost real time

- **The blocking dialog is not the one the row names.** *"Connectors lost while
  loading"* is the **sync** `SeMessageBox` at `:1143`; the blocker is the
  **async** one at `:1089`, which fires first.
- **`MainWindowTitle` is not a usable handle on it.** E48 and E51 both lean on
  the title as *"the only evidence available from outside"*. `:1089` passes
  **`L""`** as the caption, so the title reads **empty** for the entire block and
  `EnumWindows` lists no visible window — while a dialog is demonstrably on
  screen, because Jeff saw it twice and told me.

That second one is the expensive part of this session, and it was my error
rather than the row's. I built a detector keyed to the window title, **told Jeff
it made driving the app safe, and it could not see this dialog at all** — so the
early-bail never fired and each probe left a modal up for its full timeout. The
control I needed was one I already had: enumerate windows *while blocked* and
notice that the count is zero when a dialog is plainly there.

### The control is what makes the reproduction mean anything

The **default** rack round-trips **byte-identical** — 18,169 → 18,169, no dialog,
no unresolved parameters. So the loss genuinely needs the inserted modules and is
not a property of save-and-reload as such. Fixture kept:
`_scratch/e48-rack-session.xml`, 65,878 B holding a 49,295 B document.

Also worth writing down: **`session.xml` is written only on quit.** It is absent
through the whole editing session, which is why an earlier measurement read
49,297 bytes and then found 18,106 on disk moments later — two different writes,
not one file changing under me.

### What landed, and what it replaces

`scripts/check-prefab-modules.py` fails when a shipped prefab names a module
absent from the built binary. It catches this defect (1 of 32 types) and
**skips rather than passing when it cannot find a binary**.

**`check-prefab-layout.py` could never have caught this**, and the reason is
worth keeping: its check #2 is *every `<module type=X>` has a `<Plugin id=X>`* —
but a prefab carries its **own** `PluginList`, so that asks whether the FILE
describes its modules, not whether the PRODUCT has them. `AR_jef.synthedit`
passed it for weeks.

The test is **absence**, deliberately: a registered id must exist as a literal in
the binary, so a type string appearing nowhere cannot be registered — while a
string that *is* present proves nothing. Only the direction it can be sure of is
reported. **Not wired into CI**: the bot token has no `workflow` scope.

### What is left, and it is not mine

Re-author `AR_jef.synthedit` to use a module TIDE ships, **or** add `Adsr.cpp` +
`EnvelopeAdsr.xml` to the compiled-in set — a **PLAN constraint 7** decision
about the fixed module set. A run must not pick, so the row stays TODO.

**Learned:**

- **A dialog with an empty caption is invisible to every handle we have.**
  `MainWindowTitle` empty, `EnumWindows` listing nothing, stderr silent, CPU
  idle — four instruments agreeing on "nothing here" while a modal is on screen.
- **Do not promise a mitigation you have not seen fire.** I claimed a bounded,
  title-detecting harness made GUI driving safe, on the strength of a detector
  that had never been tested against the dialog it existed for.
- **Attach to a hung process; do not launch it under the debugger.** Seven
  minutes versus seconds, for the same question.
- **A checker that validates a file against itself is not validating the
  product.** The prefab's own `PluginList` made it self-consistent and
  unloadable at the same time.
- **`session.xml` is written on quit only.** Reading it mid-session tells you
  about the *previous* run.

**Next:** the product decision above. Once it is made, E48's Accept is one
re-run of the fixture. **E53** still wants a faulting address and now has a
working technique for it — attach, do not launch.

**Machine state.** All eight repos on their default branches, clean.
`%APPDATA%\TIDE Rack\` holds a scratch session from this work, not Jeff's — his
was restored earlier and this run replaced it again; **restored once more at the
end and md5-verified.** No TIDE-Rack or cdb process running, checked by
enumerating every visible top-level window as well as by name.

**Branch/PR:** `tide/win/E48-connectors-lost` — the check, E48's row, E51's
annotation, and this entry.

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
2. Stop when this file is **under 60 KB**, or when the floor is reached —
   whichever comes first. **The floor is the LATER of: the four most recent
   entries, or every entry carrying the most recent date.** The floor always
   wins; a busy day pushing this file over 60 KB is correct, not a rotation
   failure.
3. Never edit an entry while archiving it. The archive is the record.

**Why a date and not a duration (A24, 2026-08-20).** A24 asked for a time-based
floor — *"retain everything from the last 7 days"* — and measuring what that
costs is what killed it. Entries per day, counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it, so 7 days is **3.4× the 192 KB that
triggered A8 in the first place** — the remedy would have been twenty times more
expensive than the problem. Even two days is worse than the state A8 was created
to fix.

So the floor is **one date**, which bounds the cost at roughly a day's work while
guaranteeing a run can always see everything that happened most recently — the
failure A24 correctly identified, where a 4-entry floor at ten entries a day
bought under half a day. On a quiet week the four-entry floor still binds and
nothing changes.

**What this does NOT fix, filed as A30:** the durable lessons still age out.
Rotation moves an entry's *"Learned"* bullets into the archive with it, and no
run reads the archive. The cheap answer is a standing digest that never rotates;
the expensive one is reading 651 KB.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

Template:

```
## YYYY-MM-DD — <machine> — <BACKLOG id>

**Did:** what actually changed.
**Result:** built / tested / failed, with the real output.
### Correction: Ardour IS a host here, and it settles the question

**Jeff asked "don't we have Ardour host?" — yes, and that makes three separate
claims of mine wrong.** I wrote in the row, both PR bodies and the issue that
closing this needed REAPER on a win/mac box. **Ardour 8.4 is installed on this
box**, `ardour-vst3-scanner` answers precisely this question, and **my own memory
note from 2026-08-19 records using it**, including the
`LD_LIBRARY_PATH=/usr/lib/ardour8` quirk it needs.

```
BROKEN (main):  VST3 not a valid bundle:
                  '.../TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE_Rack_VST3.so'
FIXED  (both):  [Info]: Found Plugin: TIDE Rack
                  uid=506C7567696E474D504920501951ED43 category="Instrument|Synth"
                  n_outputs=2 n_midi_inputs=1
```

Ardour derives the payload name from the bundle name — exactly the rule GMPI's
own comment states — so **the Linux VST3 is unloadable today, not merely oddly
named**, and the fix is host-verified on the platform that has the bug. The
scanned UID also matches the one in all five `.rpp` fixtures.

**The lesson is not "use Ardour".** It is that I asserted an environment limit
three times without testing it, while holding a note that contradicted it.
"Not verifiable here" is a claim about the machine, and it deserves one command
before it goes into a row, two PR bodies and an issue.

Ardour's cache entry from the scan pointed into a scratch tree and was removed;
Jeff's other nine cached plugins were left alone.


**Learned:** anything the next run would otherwise rediscover the hard way.

0. **"Not verifiable on this box" is a measurable claim, and I shipped it three
   times unmeasured.** Ardour was installed the whole time and my own memory note
   named the command. Check the machine before writing a limit into a row.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---
