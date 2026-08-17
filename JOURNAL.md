# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

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
2. Stop when this file is **under 30 KB**, or when the **four most recent
   entries** remain — whichever comes first. **The floor of four wins**: the run
   prompt tells every run to read the last four entries, so a verbose month
   pushing this file over 30 KB is correct, not a rotation failure.
3. Never edit an entry while archiving it. The archive is the record.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-08-17 — windows — E2a planned, S8 corrected, E4 filed (interactive session, Jeff directing)

**Prompt:** n/a — interactive; Jeff asked for the E2a plan, the S8 row fix, and
an answer on user-authored prefabs under AUv3. Committed and pushed as
`tide-rack-bot` (claude-fable-5).

**Did:** wrote [docs/e2a-prefabs.md](docs/e2a-prefabs.md) — the implementation
plan E2a's row now points at — corrected S8's premise in place, and filed the
user-prefab question as **E4** (NEEDS-JEFF) with the analysis in the doc's §7.

**The three findings under the plan, all measured today:**

- **The prefab format is forced, not chosen.** `CContainer::LoadPrefab`
  parses only modern `.synthedit`/`.syntheditprefab`; the `.seprefab` branch
  (`CContainer.cpp:2996`) launches an installed SynthEdit 1.5 to upgrade the
  file and is `_WIN32`-only — unusable from any sandboxed plugin. The 2024
  prototype prefabs are references, not inputs.
- **`Output.seprefab` contains no Sound Out** (decoded the UTF-16 payload:
  Container + `SE Patch Point in` + `IO Mod`). The Output prefab is authored
  from scratch; `TIDE.se1` is where the working Sound Out example lives.
- **S8's "delete the forbidden modules" premise would have silenced TIDE.**
  All three modules it names are `RegisterIoModule` seams, and Sound Out is
  the plugin's audio egress — `SeAudioMaster` hands it the host's output
  buffers (`SeAudioMaster.cpp:560-562`, `:640-642`), the same seam S12 used
  for MIDI input. The row now says relabel-not-delete, with cites.

**On user prefabs (E4):** yes under AUv3 — prefabs are data, and an extension
may write inside its own container, which constraint 4's wording permits. The
open ruling is desktop, where no OS-enforced container exists and the natural
folder is the one constraint 4 names as banned. Default in effect: not v0.1.

**Ruling, later the same sitting — constraint 9.** Presented with E4's
"bless a desktop folder?" question, Jeff declined the shape of the question:
rather than per-platform blessed locations, **TIDE Rack only implements
features implementable on the lowest-common-denominator target (today AUv3)**.
Added as PLAN.md constraint 9, recorded in docs/decisions.md, and applied to
E4 — which drops from NEEDS-JEFF to BLOCKED(E2): the per-device library is
allowed in principle (AUv3 can write in its own container), and desktop gets
the same container semantics or nothing. Note for future rows: questions of
the form "may platform X do Y?" now start from "can AUv3 do Y?".

**Next:** E2a is takeable with a concrete first step — author the Output
prefab as `.synthedit`, then module-enumeration stage 4 to ship it. The
oscillator prefab stays gated on S8's oscillator finding.

**Side effects on this box:** none — docs and rows only; nothing built.

**Branch/PR:** this TideSynth PR.

---

## 2026-08-17 — windows — C12c done, and the Windows build of `main` is broken by something else

**Prompt:** `b3e9876e8` · claude-opus-5[1m] · app version not discoverable on
this box (no `claude` on PATH, nothing recorded in `~/.claude`) · as
`tide-rack-bot`

**Did:** took **C12c** from the NEXT block and finished it — the twelve
independent leaves (`CLine2`, `commandMgr`, `SuspendDSP`, `legacyExternalApp`,
`ModuleDragAndDropManager` as `.cpp`+`.h`, plus header-only `ui_msg_target.h`
and `IGuiHost.h`, 1,316 lines) moved from `SE16/SynthEdit2/` into
`SynthEditLib`'s root, and `EditorLib/CMakeLists.txt` repoints them.
**25 → 13 `${EDITOR_DIR}` entries.** Files are byte-identical (`cmp`) to the
originals.

**Result — the numbers, and then the caveat they were measured under.**

- **Dangling private includes 42 → 21**, `scripts/dangling_private_includes.py`
  before and after. Exactly the 21 edges C12c's Accept names, **zero new
  opened**: `CLine2.h` 7, `SuspendDSP.h` 6, then `commandMgr.h`,
  `legacyExternalApp.h`, `ModuleDragAndDropManager.h`, `ui_msg_target.h` at 2
  each; `IGuiHost.h` had no public includer and closes 0, as predicted.
- **Fresh scratch Ninja tree, Release, all four sibling repos on local
  overrides: `935/935 RC=0`, `ctest 92/92 passed, 0 failed`** (93 listed, 1
  disabled upstream), producing `TIDE.gmpi`, `TIDE_VST3.vst3` and
  `SynthEditCL.exe`. The build log shows the five moved `.cpp`s compiling as
  `EditorLib\...\C_\SE\SynthEditLib\<name>.cpp.obj` — from the public repo, not
  the private one.
- **The edge total is 935, not the 904 the backlog and C12a recorded.** That is
  upstream growth in `SynthEditLib` (the tearout-window-layout feature), not
  C12c, which moves TUs rather than adding or removing them. **The new baseline
  for later sub-stages is 935/935 and 92/92.**

**The caveat, and it is the important half of this entry: `SynthEditLib`'s
`main` does not compile on Windows, and C12c had nothing to do with it.** The
first full build died at edge 78 of 935:

```
C:\SE\SynthEditLib\SeAudioMaster.cpp(548): error C2680: 'MidiIn *': invalid target type for dynamic_cast
C:\SE\SynthEditLib\SeAudioMaster.cpp(548): note: 'MidiIn': class must be defined before using in a dynamic_cast
C:\SE\SynthEditLib\SeAudioMaster.cpp(805): error C2027: use of undefined type 'MidiIn'
C:\SE\SynthEditLib\SeAudioMaster.h(609): note: see declaration of 'MidiIn'
```

From `e14970e`, "Feed a patch's MIDI In module in plug-in hosts too (S12(a))",
merged as SynthEditLib#16 from the macOS box today. Clang accepts
`class MidiIn* midiInModule = {};` at `SeAudioMaster.h:609` together with
`dynamic_cast<class MidiIn*>` at `:548`; MSVC binds both to the incomplete type
that first declaration introduces, and never to the complete class in
`modules_internal/MidiIn.h`. Filed as
[#111](https://github.com/JeffMcClintock/TideSynth/issues/111) with a suggested
one-line fix. **Not fixed here** — `SynthEditLib` is GATED for a scheduled run
and this is not a carve-out stage, the same STEP 1 / STEP 5 contradiction the
linux box hit this morning in [#87](https://github.com/JeffMcClintock/TideSynth/issues/87).
**Two `platform:*` breaks in GATED shared code in one day is the signal, not
the coincidence** — this needs Jeff's ruling, not a third run's judgement.

So C12c was verified with its twelve files on top of `d96edbb`, the merge
immediately before `e14970e`, not on top of `main`. **The gap that leaves is
stated in both PR bodies:** this branch has not been proven to build against
`main` as `main` stands, only against `main` minus one unrelated commit.

**Learned — a control compile is cheap and it is what makes "not my fault"
a fact rather than a claim.** The tempting move was to reason that a TU in the
`SynthEditLib` target cannot be affected by `EditorLib`'s source list. That
reasoning is correct and it is not evidence. Checking out pristine `origin/main`
and re-running the *exact* `cl.exe` command line from the failing ninja edge
reproduced all three errors identically in about forty seconds, with none of
C12c present. **When a build breaks during your change, re-run the one failing
compile against the untouched tree before you diagnose anything.**

**Learned — four things about that failure were ruled out, so nobody repeats
them:** it is not a missing include (`/showIncludes` shows
`modules_internal/MidiIn.h` opened at `.cpp:26`, ahead of both failures); not a
shadowing duplicate (`git ls-files` finds exactly one `MidiIn.h`); not
conditional compilation (in the preprocessed TU `class MidiIn final` sits at
line 344022 and the failing `dynamic_cast` at 356338); and **not** the bare
elaborated-type-specifier pattern, because a minimal `class X* member;`-then-
define-`X` repro compiles clean under the same compiler and flags. The
interaction is more specific than that shape alone, and I stopped there rather
than keep digging in a GATED file.

**Learned — the run prompt's GATED wording says "C1-C7" and the carve-out is up
to C12.** C12a, C12b, C12e, C9 and C11 have all been executed and merged as
GATED edits under a rule whose literal text does not cover them, and the NEXT
block instructs this box to take C12c. The practice is settled and the wording
is not; worth an edit to STEP 5 next time someone touches the prompt.

**Next:** **C12f** is the remaining large stage — the patch cluster, ten
entries, 6,298 lines, atomic, and the one that takes `${EDITOR_DIR}` to zero and
unblocks C6. C12d stays `linux`. Before either, someone with the authority
should fix [#111](https://github.com/JeffMcClintock/TideSynth/issues/111), or
the next Windows run pays the same tax and the next Windows *user* cannot build
TIDE at all.

**Side effects on this box:** a scratch Ninja tree at `C:\SE\build-c12c`
(Jeff's own `SE16\build` untouched); no other repo modified. All five working
copies were clean before this run and are returned to their default branches.

**Postscript, same session:** Jeff merged both code PRs within minutes of them being opened (07:21 and 07:24 UTC), so C12c has landed on `master` and `main` already. The row stays `IN-REVIEW` regardless — a run does not set `DONE` on its own fresh work — and the next run may flip it.

**Branch/PR:** [SynthEdit#41](https://github.com/JeffMcClintock/SynthEdit/pull/41)
+ [SynthEditLib#18](https://github.com/JeffMcClintock/SynthEditLib/pull/18),
which must merge together, plus this TideSynth PR.

---

## 2026-08-17 — macos — presets now carry the rack: blobs base64'd, non-stateful params excluded (unsupervised)

**Prompt:** n/a — Jeff chose base64 and asked for the codec to live in GMPI or
GMPI-UI "so we're not duplicating code", then said "work continuously" and
left. Committed and pushed as `tide-rack-bot` (claude-fable-5).

**Did:** implemented S11's blocker —
[GMPI#3](https://github.com/JeffMcClintock/GMPI/pull/3) +
[SynthEditLib#17](https://github.com/JeffMcClintock/SynthEditLib/pull/17).
**A project's saved VST state grows from 250 bytes of base64 to 1503, and the
`.rpp` now contains the rack document inside the `<Preset>`.** The bytes are
in the file, which is exactly what was missing.

**Where the codec went, per the ruling:** `GMPI/Core/base64.h` — the lowest
layer that needs it, since **both** ends of the round-trip (the writer in
`Hosting/processor_holder.cpp` and the reader in `Hosting/controller_holder.h`)
live in GMPI. **`SynthEditLib/Base64.h` is now a thin forwarder**, so there is
one implementation rather than two and its four call sites are unchanged.

**The second half was not optional, and I only found it by breaking the
editor.** With encoding in place the plug-in UI **rendered blank**. Stashing
the change made it render again; that A/B, run in the same REAPER session,
proved the regression was mine rather than session flakiness. The cause:
TIDE's **`controllerPtr` is a blob holding a raw pointer**, declared
`persistant="false"` — harmless while blobs serialised as `"0"`, but once they
round-trip properly, **restoring one hands the plug-in a dangling pointer from
a previous run.** Fixed by honouring the flag the spec already carries:
`if (!parameter.info->is_stateful) continue;`. With encoding **and** guard,
the editor is normal.

**Learned — making serialisation work can resurrect things that were only
safe while broken.** The `"0"` bug was silently protecting a
session-only pointer from ever being restored. **When you fix a lossy
round-trip, audit what was relying on the loss** — the `is_stateful` flag
existed precisely for this, and nothing had needed to respect it until now.

**Also learned — an A/B beats a hypothesis, and costs one build.** The blank
editor could plausibly have been the day's REAPER instability (a deleted
project file, dialogs, a failed load). Stashing and rebuilding answered it in
one cycle instead of a debugging session, and it is the same control I used on
the codesign failure earlier in the week.

**Build note for future sessions, now also in memory:** this box builds with
`GMPI_SDK_FOLDER_OVERRIDE`, `GMPI_UI_FOLDER_OVERRIDE` and
`SYNTHEDITLIB_FOLDER_OVERRIDE` pointed at the local checkouts, per Jeff — the
local clones are the truth while several repos move together, and FetchContent
will not re-pull once `_deps` is populated.

**Next:** S11's remaining half — confirm the **restore** side rebuilds the
rack on load (place a module, save, reopen, look). The reader decodes base64
now, so this may already work; measure before writing code.

**Side effects on this box:** several TIDE_VST3 builds including a stash/A/B
cycle; the three `*_FOLDER_OVERRIDE` entries point at local checkouts;
installed plug-in current. REAPER restarted repeatedly and several throwaway
tabs made; a stale `/tmp/tide-persist2.rpp` I had deleted caused one REAPER
"error opening project" dialog — **`/tmp/tide-persist3.rpp` is left in place
deliberately** for the restore test. REAPER twice offered to reopen **"Optimus
HP"** after a failed load and **both times it was declined; that project was
never opened or modified.**

**Branch/PR:** this TideSynth PR + [GMPI#3](https://github.com/JeffMcClintock/GMPI/pull/3)
+ [SynthEditLib#17](https://github.com/JeffMcClintock/SynthEditLib/pull/17)
(**must merge together** — the forwarder includes GMPI's header).

---

## 2026-08-17 — macos — S11's restore has exactly one blocker: blobs are not encoded in the preset (unsupervised)

**Prompt:** n/a — Jeff left with "do as many tasks as you can unsupervised"
after the upstream verification. Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** took S12(b), the save/reopen re-check, and **measured rather than
assumed — the rack still does not survive a reload, and the reason is now a
single named TODO** in code, not a mystery.

**Measured:** with a full instrument patched and playing, the saved VST state
is **250 bytes of base64 before the save and 250 after the reopen** — the same
number S11 recorded before any of today's work. The document is pushed through
the chunk parameter and it reaches the processor (that is why MIDI notes now
play), but **it is not written into the DAW's saved state.**

**The cause, both halves, already flagged in the source:**

- **Writer** — `gmpi_processor::getPresetUnsafe()` writes every parameter with
  `paramElement->SetAttribute("val", parameter.valueReal())`. `valueReal()` is
  the *double* accessor, so a blob parameter is serialised as `val="0"` and the
  bytes are silently dropped. That is exactly the two-parameter, all-zero
  `<Preset>` S11 dissected.
- **Reader** — `GmpiParameter::setFromXml` has, verbatim:
  `case gmpi::PinDatatype::Blob: break; // TODO uuencode or hex`.

So blob parameters have **never** round-tripped through a preset; TIDE is
simply the first plug-in whose entire state is a blob.

**The decision this needs, and it is Jeff's because it is a persisted
format:** which encoding — base64 or hex — and where the codec lives.
`SynthEditLib/Base64.h` exists, but **GMPI is the lower layer and cannot depend
on SynthEditLib**, so GMPI needs its own small encoder (or the pair moves into
GMPI and SynthEditLib uses it). Both sites are one function each; the work is
small, the format choice is permanent.

**Why I stopped here rather than implementing it.** It is a change to a
persisted file format in shared code, at the end of a very long session, and
the encoding choice is not mine to make silently. **The rest of today's fixes
were behaviour; this one is compatibility** — a preset written by the first
implementation has to be readable by every later one.

**Learned — "it reaches the processor" and "it is saved" are different
claims.** The chunk parameter now demonstrably delivers the document to the
DSP (notes play), which made it tempting to assume persistence followed. The
`GetTrackStateChunk` byte count answered it in one line and cost nothing.
**Measure the artefact you actually care about; a working adjacent path is not
evidence.**

**Next:** S11 — implement blob encoding in the preset writer and reader once
Jeff picks the encoding. That single change should complete S11's restore,
since the document already survives everywhere else in the chain.

**Side effects on this box:** no code changed; nothing built. REAPER driven,
two throwaway projects written to `/tmp`; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (rows + entry only; no code).

---

## 2026-08-17 — macos — MIDI notes verified from pure upstream; the patch workaround is retired (interactive session, then unsupervised)

**Prompt:** n/a — interactive session; Jeff granted the bot write access to
GMPI, merged [GMPI#2](https://github.com/JeffMcClintock/GMPI/pull/2), then
said "clean up the patches and verify from upstream" and left ("do as many
tasks as you can unsupervised"). Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** **the bot can now open PRs on GMPI** (invitation accepted,
`push=true` confirmed), and with both GMPI fixes merged I **cleared every
local override and re-verified MIDI from pure upstream: peak 0.5103 (−5.8 dB)
with a note, 0.0001 with the transport stopped** — the identical numbers to
the local-override build, so a fresh clone now hears MIDI. This closes S12(a)
properly rather than on a machine-specific build.

**The access fix, for the record:** the bot had `push=true` on all five other
repos and **`push=false` on GMPI alone** — it had simply never been granted.
Jeff added it as a collaborator; the bot accepted invitation `329366224`
itself via `gh api -X PATCH user/repository_invitations/<id>`. **Both GMPI
fixes are now normal PRs (#1 and #2), and the patch-file route is retired.**

**A near-miss worth recording, because it would have been a false pass.**
After clearing the overrides the build **succeeded** — and grepping the
fetched sources showed **the fixes were absent**: `_deps` still held the old
checkouts, because **FetchContent does not re-pull an already-populated
dependency just because `origin/main` moved.** I had "verified upstream"
against stale code for one build. Fixed by `git fetch && git reset --hard
origin/main` in each `_deps/*-src`, after which all four greps matched and
the numbers reproduced. **Grep the fetched source for the change you are
verifying — a green build proves nothing about which code was compiled.**

**Cleanup, and why the patch files stay.** `docs/patches/` now carries a
**README marking both patches superseded**, with the PR each landed as, and
an instruction not to use that route again. **The `.patch` files themselves
are kept deliberately**: JOURNAL entries link to them and the journal is an
immutable record, so deleting the files would break history to tidy a folder.
The README is the honest way to say "obsolete" without rewriting the past.

**Also cleared:** all three `*_FOLDER_OVERRIDE` cache entries are empty, the
local GMPI branches are deleted, and the installed plug-in is byte-identical
(`cmp`) to the pure-upstream build. `SE_LOCAL_BUILD=TRUE` had to be restored
again after the earlier `--fresh` — the trap already documented in
[docs/building.md](docs/building.md), hit for the second time today, which is
why it is written down.

**Learned — a clean instance beats a debugged one.** The first upstream
measurement read silence, and the reason was not the build: earlier failed
UI batches had dropped modules into that instance's rack while the structure
view was not open, leaving a polluted document. Rebuilding the patch in a
**fresh tab** reproduced the expected numbers immediately. **When a test rig
has been poked at by failed automation, rebuild the rig before debugging the
product.**

**Next:** S12's remainder — the save/reopen re-check first, since the chunk
now carries real documents *and* is seeded into fresh processors, so S11's
restore half may already work; then the faded swap and preset retention.

**Side effects on this box:** several TIDE_VST3 builds; `_deps` for GMPI,
GMPI_Wrappers and SynthEditLib re-cloned/reset to upstream main; overrides
cleared; installed plug-in current. REAPER restarted twice and several
throwaway tabs were created; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (docs + bookkeeping only; no code).

---

## 2026-08-17 — macos — MIDI notes play: the blob pin was never seeded on a new processor (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged choice (ii) and said "keep
going till you are blocked". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** found and fixed the last blocker. **A MIDI note now plays a TIDE
rack.** Measured: **peak 0.5103 (−5.8 dB) while the note sounds, 0.0001 with
the transport stopped** — so the audio genuinely follows the notes rather than
droning, which is the test Jeff's VCA correction made possible. The fix is
generic and lives in GMPI; **the bot cannot push that repo (403 again), so the
patch is filed at
[docs/patches/gmpi-seed-blob-pins.patch](docs/patches/gmpi-seed-blob-pins.patch)**
(72 lines) for Jeff to apply, exactly like this morning's transport patch.

**The bug, in one sentence:** `gmpi_processor`'s pin-initialisation loop seeded
parameter-backed pins for `Float32`, `Int32` and `Bool`, and **`Blob` fell
through to `default: assert(false)`** — so a newly created processor started
with an empty blob pin and was never told otherwise, because blobs only reach
the processor when they **change**.

**Why that mattered so much here.** A host creates processors whenever it
likes — after `restartComponent`, for offline or anticipative processing, on
state restore. REAPER was running **two** TIDE processors: the editor's held
the rack document and ran audio, while a second one received all the MIDI with
`prepared=0` for its entire life. It had never been handed the document, so it
had no graph to play. **The two-instance behaviour was never the bug; the
un-seeded blob pin was.**

**Learned — a `default: assert(false)` is a to-do list.** The same switch
statement had already bitten me this morning: `sendParameterToProcessor` was
missing its Blob case, and I added it to fix the *change* path. **I fixed one
arm of the pattern and did not check the other**, so the initialise path kept
the hole for another six hours. When a datatype is missing from one switch
over `PinDatatype`, grep every switch over `PinDatatype` in the same file
before moving on — there were exactly two, and they needed the same case.

**Verification note:** the gate test is what makes this claim safe. A patch
whose oscillator reaches Sound Out will drone at its default pitch and read a
healthy peak whether or not MIDI works — Jeff caught me making exactly that
mistake earlier. Comparing **note-playing (0.5103) against transport-stopped
(0.0001)** is the measurement that cannot be faked by a drone.

**Next:** with notes audible, S12(a) is done. The remainder of S12 is the
save/reopen re-check (the chunk now carries real documents **and** is seeded
into fresh processors, so S11's restore half may work already), then the faded
swap and preset retention.

**Side effects on this box:** three TIDE_VST3 builds; all probes reverted
earlier and both code trees verified clean before this change. **The build now
points `GMPI_SDK_FOLDER_OVERRIDE` at the local GMPI checkout** (needed to test
the unmerged patch) — it should be cleared once Jeff applies it, or fresh
clones will build without the fix. REAPER restarted twice; **"Optimus HP"
untouched**.

**Branch/PR:** this TideSynth PR + local GMPI branch `tide/mac/seed-blob-pins`
(patch filed for Jeff).

---

## 2026-08-17 — macos — the MIDI mystery solved: a second processor instance, and it never gets the document (interactive session, Jeff directing)

**Prompt:** n/a — interactive session; Jeff merged choice (ii) and said "keep
going till you are blocked". Committed and pushed as `tide-rack-bot`
(claude-fable-5).

**Did:** chased the "MIDI stops after a rebuild" defect and **solved it — the
cause is not the rebuild at all.** REAPER runs **two** TIDE processor
instances, and **MIDI is delivered to the one that has never received a
document**. This also retracts yesterday's conclusion, which was based on a
transport that had quietly stopped. No code change; the evidence and the fix
direction are the deliverable.

**The evidence, from one instrumented run** (probe in TIDE's `onMidiMessage`,
`subProcess` and `SynthRuntime::MidiIn`, all logging `this`):

```
onMidiMessage  proc[0x12b0b0800] prepared=1   x31      <- editor's instance
onMidiMessage  proc[0x12ba36800] prepared=0   x77      <- the one REAPER feeds MIDI
PREPARE        proc[0x12b0b0800]  (every document)
subProcess     proc[0x12b0b0800]  (only this one)
```

**So:** the instance the editor is attached to receives every document push,
builds its graph and runs audio. A **second** processor instance receives the
host's MIDI, has `prepared=0` for its whole life, and never runs `subProcess`.
The two never meet.

**Why it happens, and it is a flaw in my S12 design rather than a host quirk.**
TIDE pushes the document **only when the XML changes** — `serviceDocumentSync`
dedupes against `lastPushedDspXml`. **Any processor that appears afterwards
therefore starts empty and stays empty**, because nothing ever re-sends. A
plug-in's processor can be created at any time — the host may re-instantiate
after a `restartComponent`, add an instance for offline/anticipative
processing, or restore state into a fresh one — so "push once on change" was
never going to be sufficient.

**Fix direction, and the right one is not the obvious one.** The hacky answer
is to re-push periodically. **The correct answer is that a newly created
processor should be seeded with the current value of every parameter**,
including blob parameters — which is what the chunk parameter is for. The
document already persists in the DAW state (that half now works), so the same
delivery that restores a saved project should seed a mid-session instance.
Worth checking whether `gmpi_processor` seeds pins from
`patchManager` at construction and simply skips blobs: if so, this is a small
generic fix in the same place as this morning's transport work, not a
TIDE-specific patch.

**Learned — retract cleanly when the evidence changes.** Yesterday I recorded
"MIDI delivery stops across a document rebuild", with instance pointers to
back it. Today's run shows MIDI never stopped: **the transport had stopped**
because clicking in the editor to place modules had halted playback, and my
"no deliveries after the rebuild" was that, not a defect. **The pointer
evidence was real and the conclusion drawn from it was wrong** — the missing
control was "is the transport actually running while I measure?", which the
`reaper.defer` sampler answers in its first line and which I did not check
before concluding.

**Also settled: there is exactly one processor per editor.** The earlier
worry that registration and delivery hit different `SeAudioMaster`s has the
same explanation — different *processors*, each with its own runtime and
generator, not a stale pointer inside one.

**Next:** seed a new processor with the current chunk-parameter value (check
`gmpi_processor`'s construction path for blob handling first, since a generic
fix there beats a TIDE-specific one). That is the last thing between the
current build and an audible MIDI note.

**Side effects on this box:** four TIDE_VST3 builds; **all probes reverted
byte-safely and both trees verified clean** (`git status` empty in SynthEdit
and SynthEditLib), installed plug-in rebuilt from the committed state. REAPER
restarted twice; **"Optimus HP" untouched**.

**Branch/PR:** this TideSynth PR (row + entry only; no code).
