# TIDE Synth — Plan

The stable document. Describes *what* and *why*. Changes rarely — a weekly agent
should treat this as given, and propose amendments rather than edit it silently.

Companion documents:
- [BACKLOG.md](BACKLOG.md) — ordered work items. Changes every run.
- [JOURNAL.md](JOURNAL.md) — append-only log of what each run did. Changes every run.
- [docs/carve-out.md](docs/carve-out.md) — the code-sharing migration plan.
- [docs/design-notes.md](docs/design-notes.md) — UX model, RNBO and Cardinal as
  reference.
- [docs/agent-setup.md](docs/agent-setup.md) — how the three machines coordinate.
- [docs/module-enumeration.md](docs/module-enumeration.md) — how modules get into
  the factory without a filesystem scan. Basis for constraint 7.

## Naming — decided 2026-08-08 by Jeff

**The product is TIDE Rack.** Named in the same vein as VCV Rack. **TIDE Synth
is the organisation**, which may release more than one plugin; TIDE Rack is the
first.

**The domain stays `tidesynth.com`** — it is already paid for, and it now reads
as the organisation's site rather than the product's, which is consistent.

This renames the thing this document has called "TIDE Synth" throughout.
Everything below still describes the product, which is now TIDE Rack. Enforcement
— the rename across docs, binaries, targets and release assets — is BACKLOG
**N1**; it is not done, so expect both names in the tree until it is.

Two consequences worth naming up front, because they change existing items:

- **P5 changes meaning, not just its target string.** It was "the plugin does
  not call itself TIDE"; the host-visible name is now TIDE Rack, and the vendor
  string is arguably TIDE Synth — those are two different fields and the
  organisation/product split is exactly the distinction VST3 draws between
  vendor and plug-in name.
- **An organisation that ships more than one plugin changes what some
  constraints mean.** Constraint 7's "fixed module set, compiled in" is a
  per-plugin statement; a second TIDE plugin gets its own set, not a shared
  scanned one. Nothing in the constraints needs rewriting today, but do not read
  them as describing a single perpetual binary.

## The Eurorack rack — decided 2026-08-09 by Jeff

**TIDE Rack's module story is a set of Eurorack-style modules, each built as a
SynthEdit Container**, so a curious user can open one up, edit it and extend it.
That is the product's point of difference — and it is a *feature of TIDE Rack*,
not a second product.

This closes a question that was open for two days. A separate repo,
[`tide-rack`](https://github.com/JeffMcClintock/tide-rack), was scaffolded on
2026-08-07 to build the Eurorack idea in parallel, with its own backlog, agent
primer and CI. **That repo is superseded, and was archived on 2026-08-09 — it is
read-only and no further work happens there.**
The prototype described below already does most of what it would have had to
build from nothing: a plugin shell that hosts the structure view, the carve-out
that makes it public, a build and release plan, and three machines coordinating
through this backlog. A second product beside it would have duplicated all of
that in order to add one layer.

The one thing that repo built and this one lacks is a working **audio
verification harness** — render a Container headlessly to a WAV, null-test it
against a checked-in golden reference. That is worth having whichever repo the
product lives in, and is filed as **E1**.

A close precedent already exists: [Cardinal](https://cardinal.kx.studio/) is a
self-contained plugin build of VCV Rack — same rack UI, but packaged as an
ordinary AU/CLAP/LV2/VST plugin with every module compiled in and none loaded
externally, rather than a standalone app scanning third-party modules. That is
constraint 7 and constraint 2, both reached independently, at real scale
(~1,400 modules). See [docs/design-notes.md](docs/design-notes.md) for what
TIDE takes from it and what it deliberately does not (Cardinal's code and
module ecosystem, which are GPL-licensed and not TIDE's lineage).

What the Eurorack framing commits to, taken from Reaktor Blocks and VCV Rack:

- **Curation is a deliverable.** Polished, ready-to-play preset racks ship with
  the product. They *are* the friendly surface, not an afterthought. Blocks'
  best trait.
- **The whole patch is visible**, and any output patches to any input. VCV's
  layout, not Blocks' cramped navigation.
- **QWERTY-as-MIDI-input**, so someone with no hardware makes sound in seconds.
  Stolen from VCV.
- **Opening a Container is optional.** The closed surface must be complete and
  satisfying on its own — a delight for the curious, never a requirement for
  value.

Being a plugin already buys Blocks' other good trait for free: no standalone-app
audio routing for the user to solve. That is constraint 2, and it was there
first.

**None of this is v0.1.** The acceptance test at the bottom of this document is
unchanged; the modules are **E2** and **E3**, blocked on **V1**. **(V1 is DONE as
of 2026-08-18 and v0.1 passes — see "v0.1 PASSES" below. E2 is unblocked; E3 is
not, and neither is in v0.1.)** Constraint 7
governs how they reach the factory: statically registered at link time, no
scanning. A Container-based module set is compatible with that — a Container is
composed of other modules rather than being a scanned binary of its own — but
the primitives it is built from have to be in the compiled-in set, which is what
**S8** is about.

## What TIDE Rack is

An open-source modular synth plugin: a cut-down SynthEdit that lives *inside*
the DAW as a plugin, rather than being a standalone application that exports
plugins.

Website: tidesynth.com. Source: GitHub (see "Open-source status" below).

**One sentence:** A Eurorack-style rack, built from SynthEdit modules and
Containers, with the host supplying audio and MIDI — unlock any module to
rewire it in SynthEdit's structure view.

## Design constraints

These are the non-negotiables. Every backlog item is checked against them.

1. **One view, two depths — amended 2026-08-13 by Jeff's rack-mode pivot.**
   The default, top-level view is the **rack**: SynthEdit's Panel View
   rendered as a Eurorack case, with every module and Container appearing as
   a rack-mounted panel. Modules and Containers can be dragged around as in
   any SynthEdit panel view, but they snap to fit neatly into the rack —
   there is no free-floating panel layout at the top level, only the rack.
   The user can **unlock** (edit) any module or Container, which takes them
   into *that* Container's own structure view to rewire its signal flow;
   unlocking is still navigation via the breadcrumb, in and out of
   Containers, exactly as before — only the *default* rendering at each
   level has changed, from structure view to rack view. No tabs, no
   dockable windows, no separate editor window alongside the rack.
   **This reverses the original constraint's "no panel view" exclusion** —
   panel view (rack-rendered) is now the default, and structure view is the
   drill-down, not the other way around. Ruling and reasoning:
   [docs/decisions.md](docs/decisions.md). The original wording, for the
   record: *"One view. The structure view, and the breadcrumb bar for
   navigating in and out of containers. No panel view. No tabs. No
   dockable windows."*
2. **The DAW owns I/O.** No audio device selection, no MIDI device selection,
   no ASIO, no driver settings. Audio and MIDI arrive from the host.
3. **Sandbox-safe.** Must run under iOS AUv3 sandbox rules. No arbitrary
   filesystem access. Anything requiring a file path off the plugin bundle is
   out of scope — this removes sampling modules, file-based wavetables, and
   the "browse for..." class of dialog entirely.
4. **Self-contained.** No scattered caches, no writes outside the plugin's own
   sandboxed container. State lives in the plugin's saved state, not in
   `%APPDATA%`, `~/Library`, or a scanned modules folder.
5. **Minimal dialogs.** Properties and module browsing are panes in whichever
   view is showing — rack or structure — not modal dialogs. If something
   needs a dialog, question whether it needs to exist.
6. **Share code with SynthEdit.** TIDE is a second consumer of the same core,
   not a fork. A fix in the shared core benefits both. See
   [docs/carve-out.md](docs/carve-out.md).
7. **Fixed module set, compiled in.** The modules TIDE ships are statically
   registered at link time. No module scanning, no module cache, no loading of
   third-party modules — on *any* platform, not just iOS. On iOS this follows
   from constraint 3; on desktop it is a deliberate product choice. Decided by
   Jeff, 2026-08-06. See [docs/module-enumeration.md](docs/module-enumeration.md).

8. **No user skins.** TIDE ships its default appearance in the plugin's own
   resources, and that is the whole story: no user-installable skins, no skin
   folder, no scanning for one, and nothing skin-related written to the user's
   disk. On iOS this follows from constraints 3 and 4; on desktop it is a
   deliberate product choice, like constraint 7. Decided by Jeff, 2026-08-07.
   Note this is stricter than the v0.1 list below, which merely *defers*
   skinning — user skins are out of TIDE permanently. Enforcement is BACKLOG
   **S7**: the shared `SkinMgr` copies skins into
   `<CommonDocuments>\SynthEdit Projects\skins\` from its constructor, and a
   TIDE build can reach it.

9. **Lowest common denominator, everywhere.** TIDE Rack only implements
   features that can be implemented on its most restricted target — today
   that is iOS AUv3, whose sandbox is the strictest set of rules any TIDE
   binary runs under. If a feature cannot be built within those rules, it is
   not built for the other platforms either: no desktop-only features, and no
   per-platform blessed exceptions (a folder that exists only on Windows, a
   capability that exists only outside the sandbox). This elevates the
   existing observation below — "if it runs there, it runs anywhere" — from a
   design habit to a rule, and it is why questions of the form "where may
   platform X write?" are answered by the AUv3 container model, not by
   platform-specific carve-outs. Decided by Jeff, 2026-08-17. First applied
   ruling: BACKLOG **E4** (user prefabs). See
   [docs/decisions.md](docs/decisions.md).

## Where the code currently lives

TIDE is not starting from zero. A working prototype exists:

| Thing | Location | State |
|---|---|---|
| Plugin shell (VST3 + GMPI) | `C:\SE\SE16\SynthEditSem\` | Builds. `TideApp` hosts the structure view. |
| `TideApp` | `SE16/SynthEditSem/TideApp.{h,cpp}` | Implements `ISeApp`; opens view, module browser, properties browser. |
| Editor core | `SE16/EditorLib/CMakeLists.txt` | ~120 files pulled from `SE16/SynthEdit2/`. **Private repo.** |
| Shared core | `C:\SE\SynthEditLib` | Already a public repo — but see licensing gap. |
| iOS app shell | `SE16/SE_IOS_APP/TIDE/` | Existing iOS/AUv3 target with a TIDE folder. |
| Demo patches | `SE16/TideModules/` | `TIDE.se1`, plus AR/Output/Sine prefabs. |

The prototype links `SynthEditLib` (public) **and** `EditorLib` (private). That
split is the central problem the carve-out solves.

## Open-source status — RESOLVED 2026-08-07

**Both repos are ISC.** `JeffMcClintock/SynthEditLib` carries an ISC `LICENSE`
(*Copyright (c) 2007-2026 Jeff McClintock*), added by `a2143a4`, *"Switch LICENSE
to ISC, matching GMPI and gmpi_ui"*; TideSynth carries the same licence. That is
the same licence as GMPI and gmpi_ui, so the whole stack is consistent. L1 is
resolved and archived in [BACKLOG-DONE.md](BACKLOG-DONE.md); the ruling is in
[docs/decisions.md](docs/decisions.md).

**Corrected 2026-08-19 (windows, Jeff directing).** This section said, in the
present tense, that `SynthEditLib` was *"a public repo with **no LICENSE file**"*
and that *"nobody may legally use or redistribute it"* — twelve days after the
licence landed. It is the most consequential thing in this document to have
wrong, because it is the sentence a prospective contributor or packager would
read first, and it told them the project was legally untouchable.

The one part of the old wording still worth keeping is the rule it existed to
enforce, which has not changed and is not made moot by L1 being answered: **a
weekly agent must not pick or change a licence.** That is Jeff's alone.

The commercial boundary to preserve: **plugin export stays private.** SynthEdit
sells the ability to export patches as plugins. TIDE embeds patches instead of
exporting them, so it does not need that code. See
[docs/carve-out.md](docs/carve-out.md) for how the seam works.

## Price and funding — decided

**TIDE is free.** No paid tier, no trial period, no licence key, no unlock, no
feature held back for a paid version. Decided by Jeff, 2026-08-06.

**Funding is by donation.** Both the plugin and tidesynth.com should make
donating possible. Neither should nag: no splash screen, no countdown, no modal
reminder, nothing that interrupts making sound. A donation route that a user has
to go looking for is the intended outcome, not a failure of the design.

Free is **not** the same as open source, and the distinction is still worth
keeping: this section settles the **price**, and the licence is a separate
question that stays Jeff's alone.

**Corrected 2026-08-19 (windows, Jeff directing):** the licence question is no
longer open. It closed on 2026-08-07 — **ISC, both repos** — so the old closing
sentence here, *"a free binary with no LICENSE file is exactly the state
`SynthEditLib` is in today"*, describes a state that ended twelve days ago. TIDE
is a free binary under an ISC licence, and is open source. See
[Open-source status](#open-source-status--resolved-2026-08-07) above.

Two constraints above already narrow what a donation affordance can be, and they
narrow it a lot:

- **Constraints 1 and 5** — one view, minimal dialogs. Whatever this is, it is
  not a dialog and not a second window. It has to live in the breadcrumb bar or
  an about pane, or it does not exist.
- **Constraint 3** — sandbox-safe. Opening an external URL is the obvious
  implementation and it is the one most at risk: `browseto.mm` / `openurl.mm`
  are already listed as removed-or-restricted under AUv3 in
  [docs/design-notes.md](docs/design-notes.md). On iOS a "Donate" button may
  simply not be able to open a browser. Design this before building it — see
  BACKLOG **D1**.

Timing: none of this is v0.1. v0.1 is the acceptance test below and nothing else.
**That test passed on 2026-08-18** — see "v0.1 PASSES" below.
The website side (**W1**) can carry a donation link immediately, because a static
page has none of the above problems.

## Target formats

| Platform | Formats | Owning machine |
|---|---|---|
| Windows | VST3, GMPI | Windows box |
| macOS | AU, AUv3, VST3 | macOS box |
| iOS | AUv3 (sandboxed — the strictest target) | macOS box |
| Linux | VST3, CLAP | Linux box |

iOS AUv3 is the constraint that drives the design. If it runs there, it runs
anywhere.

## What "done" looks like for v0.1

**Amended 2026-08-13 for the rack-mode pivot (constraint 1).** A plugin that
loads in a DAW, shows the rack, lets the user drop in an oscillator and an
envelope — each a rack module: a prefab Container with the actual DSP module
inside and its patch points exposed on the panel — cable them to an output,
play it from the DAW's MIDI, and have the patch survive save-and-reload of
the host project. Nothing more.

Explicitly *not* in v0.1: presets browser, skinning, custom panels, undo,
plugin export, module authoring.

## v0.1 PASSES — measured 2026-08-18

Every clause of the test above now holds, and each one is a number rather than a
judgement. All of it is re-runnable from the repo in one command:

```bash
python3 scripts/render-and-measure.py --control            # proves the chain detects audio
python3 scripts/render-and-measure.py tests/hosts/v3-midi-pitch.rpp
```

| clause | evidence |
|---|---|
| loads in a DAW, shows the rack | `TIDE: 5 rack prefab(s) seeded from the bundle`, editor opens, rack draws |
| drop in an oscillator and an envelope, each a prefab Container | **E2a** — three prefabs that place and cable with real mouse drags |
| cable them to an output | 4 patch cables, endpoints verified in `HC_PATCH_CABLES` |
| play it from the DAW's MIDI | **261.6257 Hz** for a middle C — **+0.001 cents** |
| patch survives save-and-reload | **peak −6.3 dBFS, 440.0 Hz**, wiring intact |

Fixtures live in [tests/hosts/](tests/hosts/README.md) — five of them, including a
deliberately **failing** negative control (the same rack with no patch cables,
which renders digital silence). The pair matters more than either alone: silence
only means something about the plugin once you can show the harness detects audio
when it is there.

Rows closed: **V1**, **E2a**, **V3**, **E8** — all in
[BACKLOG-DONE.md](BACKLOG-DONE.md).

**Three findings from getting here are worth more than the tick**, because each was
a silent failure that produced plausible-looking output:

1. **TIDE had no type converters linked.** The DSP graph auto-inserts one for any
   mixed-datatype connection and, on a miss, `assert(false); return;` — which in a
   Release build compiles out, so **the connection was silently abandoned**. The
   editor still drew the cable. Any mixed-datatype cable a user drew was dead.
2. **A centred MIDI 2.0 per-note pitch bend detuned every note by exactly three
   semitones** — a minor third out while staying perfectly in tune with itself, so
   octaves and intervals all measured correct. Fixed in SynthEditLib; it affected
   MPE controllers generally, not just TIDE.
3. **Polyphony cannot escape a container.** A polyphonic MIDI-CV inside a rack
   module is correct internally and worth nothing outside it. v0.1 side-steps this
   by keeping one MIDI-CV at the document root and making the rack module a facade
   of jacks fed inward; the underlying limitation is **E7**, still open.

**The dependency this document had backwards is resolved.** The original note
follows, kept for the record.

**Read the next paragraph as dated, not current — checked 2026-08-19 (windows,
Jeff directing).** It says *"E2 is currently `BLOCKED` on V1"*, and that
"currently" is **2026-08-13's**, not today's. As of now **V1 is `DONE`**
(2026-08-18, its acceptance test measured above) and **E2 is `TODO`, not
blocked** — its own row records it as unblocked on 2026-08-18 precisely because
V1 closed. Nothing in the paragraph is true of the present tree.

It is kept verbatim anyway, because this file's convention is that a superseded
note is preserved rather than rewritten, and because the reasoning error it
records — a document asserting a cycle between its own acceptance test and the
work that satisfies it — is the useful part. The hazard is only the word
"currently" in preserved text, which is why this warning sits above it rather
than inside it.

**This created a dependency this document had backwards — RESOLVED 2026-08-13 by
splitting E2a (the three V1-critical prefabs) out of E2, and closed out entirely on
2026-08-18 when the test above passed.** The acceptance test now
requires at least an oscillator, envelope and output *prefab* to exist, which
is E2's job ("the first Eurorack modules, as SynthEdit Containers") — but E2
is currently `BLOCKED` on V1 ("no point authoring modules for a plugin that
cannot yet keep its patch across a host save"). Both cannot be true at once.
