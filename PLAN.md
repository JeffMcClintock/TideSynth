# TIDE Synth — Plan

The stable document. Describes *what* and *why*. Changes rarely — a weekly agent
should treat this as given, and propose amendments rather than edit it silently.

Companion documents:
- [BACKLOG.md](BACKLOG.md) — ordered work items. Changes every run.
- [JOURNAL.md](JOURNAL.md) — append-only log of what each run did. Changes every run.
- [docs/carve-out.md](docs/carve-out.md) — the code-sharing migration plan.
- [docs/design-notes.md](docs/design-notes.md) — UX model, RNBO as reference.
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

## What TIDE Rack is

An open-source modular synth plugin: a cut-down SynthEdit that lives *inside*
the DAW as a plugin, rather than being a standalone application that exports
plugins.

Website: tidesynth.com. Source: GitHub (see "Open-source status" below).

**One sentence:** SynthEdit's structure view, embedded in a plugin, with the
host supplying audio and MIDI.

## Design constraints

These are the non-negotiables. Every backlog item is checked against them.

1. **One view.** The structure view, and the breadcrumb bar for navigating in
   and out of containers. No panel view. No tabs. No dockable windows.
2. **The DAW owns I/O.** No audio device selection, no MIDI device selection,
   no ASIO, no driver settings. Audio and MIDI arrive from the host.
3. **Sandbox-safe.** Must run under iOS AUv3 sandbox rules. No arbitrary
   filesystem access. Anything requiring a file path off the plugin bundle is
   out of scope — this removes sampling modules, file-based wavetables, and
   the "browse for..." class of dialog entirely.
4. **Self-contained.** No scattered caches, no writes outside the plugin's own
   sandboxed container. State lives in the plugin's saved state, not in
   `%APPDATA%`, `~/Library`, or a scanned modules folder.
5. **Minimal dialogs.** Properties and module browsing are panes in the one
   view, not modal dialogs. If something needs a dialog, question whether it
   needs to exist.
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

## Open-source status — unresolved

`JeffMcClintock/SynthEditLib` is a public repo with **no LICENSE file**. Public
is not the same as open source: with no licence, default copyright applies and
nobody may legally use or redistribute it.

TIDE cannot honestly be called open source until a licence is chosen for both
`SynthEditLib` and TIDE itself. This is a decision for Jeff alone — a weekly
agent must **not** pick a licence. See BACKLOG item L1.

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

Free is **not** the same as open source. This section settles the price; it does
not settle the licence, which is still L1 and still Jeff's alone. A free binary
with no LICENSE file is exactly the state `SynthEditLib` is in today.

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

A plugin that loads in a DAW, shows a structure view, lets the user drop in an
oscillator and an envelope, wire them to an output, play it from the DAW's MIDI,
and have the patch survive save-and-reload of the host project. Nothing more.

Explicitly *not* in v0.1: presets browser, skinning, custom panels, undo,
plugin export, module authoring.
