# TIDE Synth

A modular synthesiser that lives inside your DAW.

TIDE is a cut-down [SynthEdit](https://synthedit.com): one structure view, a
breadcrumb bar, and nothing else. You patch modules together in the plugin
window while the host feeds it audio and MIDI. There is no separate application
and no export step — the patch *is* the plugin.

**Status: early. Not usable yet.** There is a working prototype, but it is not
yet buildable outside the author's machine. See [BACKLOG.md](BACKLOG.md).

## Design in six lines

1. One view — structure only. No panels, no tabs.
2. The DAW owns audio and MIDI I/O.
3. Runs under an iOS AUv3 sandbox. No filesystem access.
4. Self-contained. Nothing scattered across your disk.
5. Minimal dialogs.
6. Shares its core with SynthEdit rather than forking it.

Inspired in spirit by [RNBO](https://rnbo.cycling74.com/) — a deliberately
reduced patcher whose subset is chosen so that everything in it runs everywhere.
TIDE's architecture is different (it interprets rather than compiles); the
discipline is the same. See [docs/design-notes.md](docs/design-notes.md).

## Licence

**Not yet chosen.** Until a LICENSE file exists here and in
[SynthEditLib](https://github.com/JeffMcClintock/SynthEditLib), default
copyright applies and this code is not open source, regardless of the repo
being public. This is tracked as BACKLOG item L1 and is the author's decision.

## Repository layout

| Path | Purpose |
|---|---|
| [PLAN.md](PLAN.md) | Goal, constraints, target formats. Stable. |
| [BACKLOG.md](BACKLOG.md) | Ordered work queue. |
| [JOURNAL.md](JOURNAL.md) | Append-only log of every work session. |
| [docs/carve-out.md](docs/carve-out.md) | Plan for making the shared core public. |
| [docs/design-notes.md](docs/design-notes.md) | UX model and sandbox rules. |
| [docs/agent-setup.md](docs/agent-setup.md) | How the Windows/macOS/Linux boxes coordinate. |

## Development

Much of the work is done by scheduled weekly sessions on three machines — one
per platform — coordinating through this repo's backlog, journal and issues.
[docs/agent-setup.md](docs/agent-setup.md) explains the arrangement, including
the rules those sessions operate under (never merge, never publish, never guess
at another platform's build errors).
