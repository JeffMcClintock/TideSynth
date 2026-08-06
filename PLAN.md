# TIDE Synth — Plan

The stable document. Describes *what* and *why*. Changes rarely — a weekly agent
should treat this as given, and propose amendments rather than edit it silently.

Companion documents:
- [BACKLOG.md](BACKLOG.md) — ordered work items. Changes every run.
- [JOURNAL.md](JOURNAL.md) — append-only log of what each run did. Changes every run.
- [docs/carve-out.md](docs/carve-out.md) — the code-sharing migration plan.
- [docs/design-notes.md](docs/design-notes.md) — UX model, RNBO as reference.
- [docs/agent-setup.md](docs/agent-setup.md) — how the three machines coordinate.

## What TIDE Synth is

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
