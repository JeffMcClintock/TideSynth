# `e53-vcv-rack-segv.xml` — BACKLOG E53's reproduction

**What it is:** a standalone **session file** — a `<Preset standalonePlugin="TIDE Rack">`
wrapper holding one base64 `<Param id="1">` — written by the 2026-08-26 windows run and
used since by **E19**, **E49** and **E53**.

| | |
|---|---|
| preset file | **51,690 bytes**, md5 `a8c1493a373da00b01c0d4a74735994a` |
| document inside it | **38,658 bytes**, md5 `9248a7ee283cf8a4c1dfaaeb811f32b4` |
| `standalonePlugin` attribute | `TIDE Rack` — **not** `TiDE Rack`, which E49's row misquotes |

**Why it is committed rather than described.** Until 2026-08-28 this file existed only as
`_scratch/e19-fixture-preset.xml` on the windows box — untracked, outside every repo, and
named by three separate BACKLOG rows as the way to reproduce them. `_scratch/` is not in
any tree and nothing regenerates it, so the rows pointed at a path that any tidy-up
deletes. Committing it costs 51 KB and makes E53 reproducible from a clean clone on any
box.

## What it contains

19 modules, and the five VCV ones are the point:

| count | type |
|---|---|
| 1 | `VCV: LFO` |
| 1 | `VCV: LFO2` |
| 1 | `VCV: Pulses` |
| 1 | `VCV: SHASR` |
| 1 | `VCV: Scope` |
| 3 | `VCA` |
| 4 | `TiDE Patch Point Out` |
| 3 | `Container` |
| 2 | `IO Mod` |
| 1 | `MIDI In` |
| 1 | `SE MIDI to CV 2` |

**`VCV: Compare` is NOT in this document** — `grep -ci compare` is 0. E49's "never
reached" list names it and that name is an error; E50 carries the measurement.

## How to run it

Copy it over the standalone's `session.xml` and launch:

```bash
cp tests/fixtures/e53-vcv-rack-segv.xml "$APPDATA/TIDE Rack/session.xml"
```

**Delete any `session.loading` sitting beside it first** — `SessionState` treats that
sentinel as "the last load died here" and quarantines the file instead of restoring it
(`SessionState.cpp:32`, `:312`).

**On mac and linux, isolate instead of copying over the real folder.** `configRoot()`
honours `$HOME` on mac and `$XDG_CONFIG_HOME` on linux
(`GMPI_Wrappers/wrapper/Standalone/StandaloneSettings.cpp:31-54`), so point those at a
scratch dir and Jeff's config is never touched — S23 measured that isolation working,
byte-identical before and after. **On Windows there is no such override**: the same
function calls `SHGetKnownFolderPath(FOLDERID_RoamingAppData)` and nothing else, so a
Windows repro must back the real folder up and restore it. That gap is **E55**, and it is
what stopped E53 being re-measured on 2026-08-28.

## Extracting the document

The document is base64 inside the single `<Param id="1">`; there is no separate copy in
the tree, deliberately, so the two cannot drift:

```bash
python3 -c "import base64,re,sys; s=open(sys.argv[1],encoding='utf-8').read(); sys.stdout.buffer.write(base64.b64decode(re.search(r'<Param id=\"1\" val=\"([^\"]*)\"',s).group(1)))" tests/fixtures/e53-vcv-rack-segv.xml > e53-doc.xml
```

That must produce 38,658 bytes with the md5 in the table above. If it does not, the
fixture has been re-saved and the rows citing those numbers no longer describe it.

## What it does

**E49's half — fixed.** Against a `SynthEditLib` predating `796bbc2` it faulted *during*
graph build, at `ug_patch_param_setter::ConnectParameter+0x5e` reading `0x64` off a null
`parameter`. With that guard in, all five rack modules construct, connections are made and
the DSP runs.

**E53's half — open.** The process still exits `0xC0000005` **after** the graph is live —
`RackProcessor: 'LFO2' processing (block 96)`, `first NONZERO OUTPUT pin 0 (0.500131)` and
the command channel all appear first, and the last line is always
`RackProcessor: 'LFO2' first nonzero light: id 0 brightness 0.996`. Measured 3/3 on
2026-08-27. **No faulting address yet** — that is E53's first stage.

The guard also prints `SynthEdit: no patch parameter for module 987654321 parameter id
0..7` against this document. `987654321` is the `VCV: Scope`, which the document *does*
define — so eight parameters are absent from the patch manager for a module that exists.
Whether that is E53's cause is **untested**.
