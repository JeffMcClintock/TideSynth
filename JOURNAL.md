# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

Template:

```
## YYYY-MM-DD — <machine> — <BACKLOG id>

**Did:** what actually changed.
**Result:** built / tested / failed, with the real output.
**Learned:** anything the next run would otherwise rediscover the hard way.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---

## 2026-08-06 — windows — project setup (manual session, not a scheduled run)

**Did:** Created this repo as the coordination point for TIDE Synth. Wrote
PLAN.md, BACKLOG.md, docs/carve-out.md, docs/design-notes.md,
docs/agent-setup.md, and a CI skeleton. No code was written and nothing in
`C:\SE\SE16` or `C:\SE\SynthEditLib` was modified.

**Learned:**

- TIDE is not a greenfield project. A working prototype exists at
  `C:\SE\SE16\SynthEditSem` — `TideApp` implements `ISeApp`, opens a
  `ContainerViewStruct` in `CF_STRUCTURE_VIEW` mode, and builds as VST3 + GMPI.
  There is also an existing iOS target at `SE16/SE_IOS_APP/TIDE/` and demo
  patches at `SE16/TideModules/`.
- The blocker for open-sourcing is `EditorLib`, which lives in the private
  `SynthEdit` repo. It has only 2 files of its own; the other ~120 come from
  `SE16/SynthEdit2/` via `EditorLib/CMakeLists.txt`. That file is the
  authoritative scope of the carve-out.
- `SynthEditLib` is already a **public** repo — but it has **no LICENSE file**,
  so it is not open source yet. This surprised the setup session and is now
  BACKLOG L1.
- The commercial boundary is cleaner than expected. `ExportAsPlugin` is one
  free function in one 2,470-line file. Its only callers are the private WinUI3
  IDE and `SynthEditCL`. `CContainer.h` references it solely through a `friend`
  declaration, which is legal C++ even when the function is never defined — so
  that header can go public unchanged. No shimming required.
- Moonbase licensing is already outside `EditorLib` by deliberate design
  (see the comment at `EditorLib/CMakeLists.txt:179`).
- "RNBW" in the original spec was a misreading; the reference is **RNBO** by
  Cycling '74. Note that a `getUniqueId() == id_to_long("RNBW")` special case
  does exist in `SE14/SynthEdit/VST_Wrapper.cpp` for a plugin called "rainbow" —
  unrelated, and a trap for a future agent grepping for it.

**Next:** L1, C0 and G1 need Jeff. P1 (verify the prototype builds) is the
first thing an agent can do unaided, and everything else depends on knowing
that baseline.

**Branch/PR:** none — scaffolding committed directly.
