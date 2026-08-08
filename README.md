# TIDE Rack

A modular synthesiser that lives inside your DAW.

**TIDE Rack is the product. TIDE Synth is the organisation that makes it**, and
this repository keeps the organisation's name — as does the domain,
[tidesynth.com](https://tidesynth.com). TIDE Rack is its first plugin, and
currently its only one.

TIDE Rack is a cut-down [SynthEdit](https://synthedit.com): one structure view, a
breadcrumb bar, and nothing else. You patch modules together in the plugin
window while the host feeds it audio and MIDI. There is no separate application
and no export step — the patch *is* the plugin.

What you patch together is a rack of **Eurorack-style modules, each built as a
SynthEdit Container** — so any module can be opened up, edited and extended by
anyone curious enough to look inside. Opening one is optional: the closed
surface is meant to be complete on its own, and curated ready-to-play racks ship
with the plugin. Decided 2026-08-09; see [PLAN.md](PLAN.md).

**Status: early. Not usable yet.** There is a working prototype, but it is not
yet buildable outside the author's machine. The Eurorack module set is not in
v0.1. See [BACKLOG.md](BACKLOG.md).

## The design, in short

- One view — structure only. No panels, no tabs.
- The DAW owns audio and MIDI I/O.
- Runs under an iOS AUv3 sandbox. No filesystem access.
- Self-contained. Nothing scattered across your disk.
- Minimal dialogs.
- Shares its core with SynthEdit rather than forking it.
- A fixed module set, compiled in. No module scanning, no third-party modules.
- No user skins. TIDE ships its own look and writes nothing to your disk.

[PLAN.md](PLAN.md) has these as the full design constraints and is the
authority; this is the summary. Deliberately not numbered here — the list grows,
and a stale count is worse than no count.

Inspired in spirit by [RNBO](https://rnbo.cycling74.com/) — a deliberately
reduced patcher whose subset is chosen so that everything in it runs everywhere.
TIDE's architecture is different (it interprets rather than compiles); the
discipline is the same. See [docs/design-notes.md](docs/design-notes.md).

## Licence

**[ISC](LICENSE)** — chosen 2026-08-07, the same licence as the
[GMPI](https://github.com/JeffMcClintock/GMPI) and
[gmpi_ui](https://github.com/JeffMcClintock/gmpi_ui) repos. A `LICENSE` file is
committed here and in
[SynthEditLib](https://github.com/JeffMcClintock/SynthEditLib), so both are
genuinely open source rather than merely public.

TIDE Rack is also **free** — no paid tier, no trial, no licence key, nothing held
back. It runs on [donations](https://ko-fi.com/TideRack).

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
