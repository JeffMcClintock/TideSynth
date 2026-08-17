# Patches

**Everything in this folder is already merged upstream. Do not apply anything here.**

These files exist only because the `tide-rack-bot` account could not push to
`JeffMcClintock/GMPI` at the time (HTTP 403), so a fix that belonged in a pull
request had to be handed over as a patch file instead. That access was granted
on 2026-08-17 and the bot now opens PRs there like any other repo, so **this
route should not be used again** — open a PR.

The files are kept rather than deleted because JOURNAL entries link to them,
and the journal is an immutable record.

| File | Now upstream as | What it did |
|---|---|---|
| `gmpi-blob-param-transport.patch` | GMPI PR #1 | Gave blob parameters a controller→processor transport; they had none, so a changed blob was stored and dropped. |
| `gmpi-seed-blob-pins.patch` | [GMPI PR #2](https://github.com/JeffMcClintock/GMPI/pull/2) | Seeded blob pins on processor creation; `Blob` fell through to `default: assert(false)`, so a newly created processor never received the value. |

Both were found while making TIDE Rack play a MIDI note (TideSynth BACKLOG
S12), and both are generic GMPI fixes rather than TIDE-specific ones.
