# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-30 — macos — E65: seven panels asked for a draft render and five were thrown away by the request path (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** (no `claude` CLI on this box's PATH; the app's `CFBundleShortVersionString`, which A13 records as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** took **E65**, reproduced it, diagnosed it to a single member variable, fixed it and measured the fix A/B. **E65 → IN-REVIEW.** Branch `tide/mac/E65-panel-draft-render`. TideSynth only; no sibling repo was committed to, and nothing GATED was touched — `modules/TiDEPanel/` is TIDE's own.

**Why E65.** The `mac` NEXT cell is 2026-08-28 and says the mac/any queue is blocked; it predates E65 being filed on 2026-08-29, so I re-walked in file order rather than trusting it. **S8** GATED (`SynthEditLib/CMakeLists.txt`), **E19** — its mac AU3 cell was measured *yesterday* on this box and its own row now says it needs one unlocked interactive session and a displaced AUv3 registration, which a scheduled run must not do; **E7** turns on Jeff's unruled *"where do the jacks live"*, which STEP 2 forbids; **E2** not takeable by its own row; **E60** held by linux on [#550](https://github.com/JeffMcClintock/TideSynth/pull/550); **E63/E64/E66/E67** `win`; **X1/X2** `linux`. E65 was the topmost eligible row, and its Accept is stated as an observable, which is the test STEP 2 actually applies.

STEP 1 and STEP 1.5 were both empty: no open `platform:mac` issue, no open `tide/mac/**` PR.

### It reproduces on a POPULATED rack, and only on one — which is why nobody had it

The shipped `DefaultRack.synthedit` puts **exactly one panel in view**, and with one panel the contract is met perfectly: `REQUEST 0.1 ms → PREVIEW 68.9 ms → DRAW PREVIEW 104.6 ms → FULL 8806 ms`. I very nearly filed "does not reproduce" on that. What changes with a second panel is everything:

| | REQUEST | DROPPED | panels previewed | on screen at 60 s |
|---|---|---|---|---|
| **before** | 7, inside 1.2 ms | **5** | **2 of 7** | **five flat grey** |
| **after** | 7, inside 1.2 ms | 0 | **7 of 7**, last at 453 ms | seven rendered |

Screenshots of the same fixture either side of the fix are the A/B: five blank grey slabs become seven panels. The fixture is seven copies of the Out module with different layout strings — they must DIFFER, because identical panels share one cache entry by design and N copies of one module is one trace.

### The cause is the request path, not worker starvation — and the log says which

E65's row listed four candidates. The trace log separates them in one line, once it names *which* panel each line is about (the panels are all 72x576, so `WxH` alone cannot tell them apart — I added `cfg=` to REQUEST/PREVIEW/FULL, and a `DROPPED` line, before drawing any conclusion):

```
0.4 ms  REQUEST  72x576 cfg=79e95eb2  cached-stage=0
0.6 ms  REQUEST  72x576 cfg=15578c0d  cached-stage=0
0.6 ms  DROPPED  72x576 cfg=79e95eb2 was wanted and never started; replaced by cfg=15578c0d
```

`FaceRenderer` is a **process-wide singleton with one worker**, and it held **one** `wantedKey` + `haveWanted`. Every panel calls `request()` within the same millisecond of a load, so each asker overwrote the last and the worker only ever learned about the final one. **Nothing re-declares the losers**: `request()` is reached only when `faceDirty` is set, and `faceDirty` is set only by a SIZE or SCALE change — so on a still rack the dropped panels stay grey indefinitely. Not starvation behind the 8-second full trace, which was my first reading too: **the five drops all happen before any trace starts at all.**

### It is not this week's regression — it is 2026-08-20's, made visible this week

`git log -S 'bool haveWanted'` puts both the singleton and the single slot in **`7966408`, 2026-08-20**, *"TiDEPanel: all tracing off the UI thread"*. The row's candidate window named E48, S1b and the E17-era preview rework; **E48 is right but as the TRIGGER, not the cause** — it put more prefab modules on racks, and the defect needs two panels to be visible at all. So the honest statement is that the code has been wrong for ten days and became reportable when the racks got fuller. **This is inference about why Jeff saw it now, not a measurement**; what is measured is the mechanism and its introducing commit.

### The fix: one want per PANEL, and cheap work first

`wanted` becomes `std::map<const void*, Want>` keyed by the calling panel's `this`, so a panel replaces its own want and nobody else's — which also stops a zoom drag piling up obsolete sizes, because a panel only ever holds its current one. The worker then picks the **newest want that still needs a preview**, and only when none is left does it pick one needing a full trace. That ordering is the other half: a preview is ~70 ms and a full is ~8.4 s and cannot be interrupted, so finishing one panel completely before starting the next leaves the rest of the rack grey for however long the queue ahead of them takes. `~TiDEPanelGui` calls `forget(this)` — the editor is rebuilt on every host resize.

### Two smaller things fixed because the instrument had to work here first

- **The trace log wrote to the wrong place on every non-Windows platform.** `%TEMP%` + a literal backslash: POSIX sets `TMPDIR`, not `TEMP`, so a mac run fell through to `"."` and created a file named `.\TiDEPanel.log` in the process's cwd — created, so nothing failed and nothing said so. Now `TMPDIR` with `/`, and `TIDE_PANEL_LOG_PATH` overrides both, which is what lets the probe collect it.
- **Arming it needed a source edit.** `-DTIDE_PANEL_TRACE_LOG=ON` now sets the define on the one file, beside the strict-fp opt-out and for the same reason. The `#define … 0` default is unchanged.

**Verification artifact:** [tests/e65_panel_preview_probe.py](tests/e65_panel_preview_probe.py) — builds the multi-panel fixture from the bundle's own rack, runs it, and checks the trace. **Its control is the real pre-fix trace**, committed as `tests/fixtures/e65-prefix-7panel.log`: `--selftest` runs the checks against it and requires them to FAIL, so the guard has been watched failing on the actual defect rather than a synthetic one. On the fix: 5/5 PASS. On the defect: checks 1, 2 and 4 FAIL — and **check 3 (timing) correctly PASSES on the defect too**, which is the calibration worth having: the failure is *never*, not *late*, and a probe that flagged timing would have been reporting the wrong thing.

**Consumers built:** every TIDE target, **280/280, 0 failures** — standalone, VST3, AU, AUv3 appex and the AU3 app. `SynthEditCL` was NOT built and does not need to be: `modules/TiDEPanel/` is TIDE's own and is not shared with SynthEdit, unlike the gmpi_ui / GMPI_Wrappers changes that carry that obligation. **macOS `main` is green** as a by-product — this tree is `origin/main` plus this change.

**Learned:**

- **A bug that needs two of something will not reproduce on a fixture that ships one.** The default rack has a single panel in view, the contract is met perfectly there, and "does not reproduce" was one measurement away from being the finding. The row's own Accept ("load a patch") is satisfiable by a patch that cannot show the defect.
- **When every instance of a thing logs the same identifier, the log cannot answer a question about which one.** Seven panels, all `72x576`; adding `cfg=` cost one format specifier and turned an ambiguous trace into a named producer/consumer list.
- **Log the DISCARD, not just the work.** The five `DROPPED` lines are the entire diagnosis, and no amount of staring at REQUEST/PREVIEW timings would have produced them — the evidence was a thing that did not happen.
- **A shared worker with a single-slot request queue is a starvation bug wearing a caching bug's clothes.** Every symptom pointed at the expensive stage; the loss was in the bookkeeping, 8 seconds before the expensive stage began.
- **`git log -S` on the member, not the file, dates a structural defect in one command** — and it is what separates "this week's regression" from "ten days old and newly visible", which are different rows with different candidate windows.
- **A control that passes one check is better calibrated than one that fails them all.** The timing check passing on the pre-fix trace is what proves the probe is measuring the right failure.
- **A debug instrument that only works on one platform is a defect in the instrument, and you find it by being on the other platform.** The log path had been Windows-shaped since it was written; nobody had needed it on a mac until this row.

**Not verified:** Windows and Linux (no platform code — the change is portable C++ in TIDE's own module; CI on the PR will say, though note it builds with the log OFF, which is the shipped configuration and exercises the fix but not the probe); the **live host** case — this is measured in the standalone, and a DAW's editor lifecycle rebuilds the panel more often, which the `forget(this)` call is written for but which nobody has watched; whether **8.4 seconds for a 72x576 full trace** is itself reasonable — it is not this row's question and I did not chase it, but a seven-panel rack now takes ~62 s to reach full quality throughout, and that is worth someone's attention; and Jeff's own reproduction, since I never saw his rack.

**Machine state.** All six repos were clean and on their default branches at the start; TideSynth is on this run's branch until STEP 5 returns it. No sibling repo was modified — `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI` and `SynthEdit` are untouched and were only read. Build tree `build-e65/` is gitignored, `SE_LOCAL_BUILD=OFF` (so its POST_BUILD cannot replace Jeff's installed plug-ins) and `TIDE_VCV_FUNDAMENTAL=OFF`, the shipped configuration. Bundle copies, fixtures, logs and screenshots are under the session scratchpad and `/tmp`, outside every repo; Jeff's installed plug-ins and REAPER configuration were never touched. No TIDE process left running.

**Next:** **E65 needs merging**, then one look in a real host — the standalone proves the mechanism, a DAW exercises the editor-rebuild path `forget()` exists for. **E19's mac AU3 cell still wants one unlocked interactive session** (yesterday's entry has the four steps). **E60** is with linux on [#550](https://github.com/JeffMcClintock/TideSynth/pull/550). **Worth a row if anyone agrees:** a full trace costs ~8.4 s per 1U panel here, so a full rack is a minute of tracing on one serial worker.

**Branch/PR:** `tide/mac/E65-panel-draft-render` — the fix (`modules/TiDEPanel/TiDEPanelGui.cpp`), the trace-log switch (`CMakeLists.txt`, `SynthEditSem/CMakeLists.txt`), the probe and its control fixture, the E65 row and this entry.

## 2026-08-29 — windows — E67: ctrl+wheel translated the document under the cursor — E42's defect, one function from where E42 fixed it (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"new bug: ctrl mouse wheel is meant to zoom in/out while keeping same point of document under the mouse"*, then *"the zoom works, but the document moves under the mouse"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** diagnosed by reading, fixed, made the gesture drivable, and measured the A/B. [SynthEditLib#75](https://github.com/JeffMcClintock/SynthEditLib/pull/75) (GATED — proposed for review, never merged by a run) + [GMPI_Wrappers#34](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/34) (`--scroll --ctrl`). **E67 → IN-REVIEW.**

### Jeff's clarification did the triage

The first report read as "broken"; I was half-way into the delivery path (E39's old note that `--scroll` "reports ok and moves nothing" pointed that way) when the clarification landed: **the zoom works, the document translates**. That eliminated delivery entirely — a zoom that works proves the event arrives with the ctrl flag intact — and reduced the search to the anchor arithmetic in one function.

### The defect, and where the answer was already written

`TopView::onMouseWheel` keeps the doc point under the cursor by recomputing the view centre — against `viewWidth * 0.5f`, the pane's half-SIZE. `calcViewTransform`, thirty lines below, anchors the actual transform on the pane's MIDPOINT, `(left+right)/2`, under a long E42 comment explaining **precisely this distinction**, measured to +240 DIP of browser strips. `(left+right)/2 − (right−left)/2 = left`, so every zoom step translated the view by `left/zoom` (and `top/zoom`) while the zoom factor itself was right. Origin-rooted panes hide it — midpoint equals half-size there — which is every other view in the repo, and why only TIDE showed it.

The fix is the same substitution E42 made, term for term against `calcViewTransform`.

### Making the gesture drivable was half the work, and it pays forever

The command channel could not express ctrl+wheel — `--scroll` built its flags from `kHoverFlags` only. [GMPI_Wrappers#34](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/34) adds `--ctrl`, the `--double`/`--right` precedent for the third time: each was one flag, and each turned a verification that "needs a human at the window" into a script. Also resolves E39's dangling note — the verb was never broken; the *gesture* was inexpressible.

### Measured — one variable, self-calibrating

`measure_zoom.py`: anchor one ctrl+wheel notch on the Out module's edge, find the module's two panel edges along the anchor row before and after, derive the ACTUAL zoom ratio from the edge spread (no assumption about snap), and compare each edge's landing spot with the doc-anchored prediction `anchor + (edge − anchor) × ratio`.

| build | result |
|---|---|
| origin/main | module **clean out of the viewport** after one notch; rails jumped rows |
| SynthEditLib#75 | left edge drift **+0.3 px**, right edge drift **+0.3 px**, ratio 1.302 |

The BEFORE build was produced by checking SynthEditLib back to `main` in the second build tree, so both binaries share the `--ctrl` flag and differ by exactly one commit's worth of view math.

**Consumers built:** TIDE standalone Debug and **SynthEditCL 90/90** — `ViewBase.cpp` is shared, so SynthEdit's own ctrl+wheel gets the same correction; origin-rooted panes are unchanged by construction.

**Learned:**

- **"The zoom works but it translates" is a complete triage in one sentence.** It rules out delivery, flags, and the zoom path, and leaves only the anchor arithmetic — the user's second sentence saved the session the delivery investigation the first sentence had started.
- **When a bug is fixed in one function, grep for the same expression in its callers.** E42 fixed midpoint-vs-half-size in `calcViewTransform` and documented it loudly; the identical expression sat in `onMouseWheel` computing the input to that very function. A fix that renames or wraps the corrected quantity (a `canvasCenter()` helper) would have fixed both sites at once.
- **An inexpressible gesture is a class of unverifiable rows.** Third time one flag on the command channel converted "needs a human" into a script — `--double` (E36), `--right` (E38), now `--ctrl`.
- **Self-calibrate the measurement against the artifact, not the spec.** Deriving the zoom ratio from the edge spread made the drift number independent of the snap formula — the measurement cannot be fooled by the very quantity under test.

**Not verified:** mac/linux builds (no platform code; CI on #75 will say); SynthEdit's interactive feel beyond compiling — the correction is mathematically the E42 fix, but nobody has wheel-zoomed the full editor against this branch.

**Machine state.** `SynthEditLib` on `tide/win/E67-zoom-anchor-drift` (PR #75), `GMPI_Wrappers` on `tide/win/E67-scroll-ctrl-flag` (PR #34), TideSynth on `tide/win/E67-filed` — all with open PRs. gmpi_ui still parked on its #16 branch awaiting Jeff's review click. Measurement artifacts under `C:\SE\_scratch\e64\zoom-*\`, script at `measure_zoom.py`.

**Branch/PR:** [SynthEditLib#75](https://github.com/JeffMcClintock/SynthEditLib/pull/75) + [GMPI_Wrappers#34](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/34) + `tide/win/E67-filed` (row and entry). The lib PR is the substance; #34 lands alone harmlessly.

## 2026-08-29 — windows — E66 fixed both halves: reload releases the visuals first, and State's death is now loud at the cause (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"drive the computer... should crash"*, then *"yes, fix E66"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** reproduced Jeff's settings-pane crash under a resident debugger, diagnosed it to a named invariant, and fixed both halves: [GMPI_Wrappers#33](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/33) (the root cause) + [gmpi_ui#16](https://github.com/JeffMcClintock/gmpi_ui/pull/16) (the tripwire). **E66 → IN-REVIEW.** Also filed **E65** earlier in the session ([#555](https://github.com/JeffMcClintock/TideSynth/pull/555), the panel's missing draft render — report only, deliberately undiagnosed).

### The reproduction, driven blind over the command channel

The pane was **already open at launch** — the audio device is held exclusively (Jeff's Ableton), and the pane auto-opens on that failure, which made the repro cheap. Clicking Close survived; **re-opening via `--menu Audio/MIDI Settings...` crashed 2/2**, AV reading `0xdddddddd` — MSVC's freed-heap fill. A resident `cdb -p <pid> -c "g; ~#k; qd"` caught the full stack on the second run:

```
State<bool>::release            <- walking the subscriber vector of a FREED State
~StateRef<bool> <- ~ToggleSwitch <- unique_ptr<View> dtor <- ViewParent::clear
Form::renderVisuals <- SettingsPane::render <- AppLayout <- paintLoop
```

### The invariant was already written down, twice

`ViewParent::clear()`'s own comment: *"release anything pointing to states before releasing states (else crash)"* — and `SettingsPane`'s destructor obeys it, with a comment saying why. **`reload()` is the same teardown happening mid-life and it skipped the first half:** `midiInputs_.clear()` destroys the MIDI tick-boxes' `State<bool>` objects while the old widget tree — only torn down at the *next* `renderVisuals` — still holds `StateRef`s into them. The other `State` members survive reload (they are assigned, not destroyed), which is why only the tick boxes could kill it.

### The fixes, and the E64 ruling applied one layer down

**Root cause** (GMPI_Wrappers#33): `reload()` begins with `clear()` and marks the form dirty. All three call sites (menu action, startup failure, device-death notification) verified to run outside the form's own widget dispatch, so clearing there cannot destroy a widget that is currently on the stack.

**Tripwire** (gmpi_ui#16): `~State()` now **asserts** no watcher remains — Debug stays loud **at the destruction site**, which is the cause, instead of a UAF two frames later in STL iterator machinery — and then **detaches** every survivor by nulling its back-pointer, so a Release build loses a notification instead of corrupting the heap. Drain-and-assert, the exact shape Jeff ruled for E64's queue. `StateRef` grants `State<T>` friendship for the detach.

### Verified

- Scripted repro: **5/5 close/reopen cycles alive** (was 2/2 crash on the first reopen), the pane fully rendered afterwards (screenshot — device combo, rate, buffer, tick boxes, status line all present), zero asserts on stderr.
- TIDE standalone + VST3 Debug rebuilt; the binary contains the new assert string (checked before running, installed-copy as negative control).
- **SynthEditCL 130/130** against the gmpi_ui branch — `observable.h` is shared with SynthEdit, per G3.
- The VST3 POST_BUILD install step failed once mid-session: **Ableton holds the installed bundle's DLL** — benign, left alone, and worth knowing: `SE_LOCAL_BUILD=ON` cannot replace the installed plugin while any host has it loaded.

**Learned:**

- **A settings pane that auto-opens on failure is a free reproduction rig.** The audio device being held exclusively looked like an obstacle and was the enabler: the pane was on screen at launch, every launch.
- **When a class's destructor documents a teardown order, grep for every other place the same members die.** `reload()` was the destructor's own sequence run mid-life, minus the half that made it safe — the invariant was stated in two places and enforced in neither.
- **Put the tripwire at the destruction site, not the use site.** The UAF surfaced in `_Adopt_unlocked` two frames after the cause; `~State()`'s assert fires at the exact line that breaks the contract, which is the difference between a session and a glance.
- **`0xdddddddd` in a Debug AV is a diagnosis in itself** — MSVC's freed-heap fill means use-after-free before any stack is read.
- **A resident `cdb -p <pid> -c "g; ~#k; qd"` costs nothing and catches what a post-mortem cannot** — the first attach without `g` lost the process; the second run's stack was the whole diagnosis.

**Not verified:** mac/linux builds of both changes (no platform code; CI will say); whether any *other* `gmpi_forms` consumer relies on destroying a watched `State` (the new assert will now say so loudly, which is the point).

**Machine state.** `gmpi_ui` on `tide/win/E66-state-outlives-ref-guard` (PR #16), `GMPI_Wrappers` on `tide/win/E66-settingspane-reload-order` (PR #33), TideSynth on `tide/win/E66-fixed` — all with open PRs, returned to defaults once merged. Jeff's Ableton untouched. Repro artifacts under `C:\SE\_scratch\e64\s3\` and `s4\`.

**Branch/PR:** [GMPI_Wrappers#33](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/33) + [gmpi_ui#16](https://github.com/JeffMcClintock/gmpi_ui/pull/16) (the fixes, either lands alone) + `tide/win/E66-fixed` (this row and entry).

## 2026-08-29 — windows — E64 root cause fixed Jeff's way: the wrapper's handle is registered, so the namespace defends itself (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"how about the obvious. register the root containers handle so everyone knows about it?"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** implemented Jeff's design for E64's root cause, and it turned out to be **TIDE-side only** — the `Id="1"` wrapper is minted by `TideApp::exportChunkXml` (`SynthEditSem/TideApp.cpp`), not by SynthEditLib, and SynthEdit's own exporter writes no such literal, so the collision never touched the commercial product. Branch `tide/win/E64-reserve-wrapper-handle`. GMPI#20 (the queue containment) stands unchanged as defence in depth.

### The design, and why it beats all four options I had listed

The wrapper's handle becomes `TideApp::kDspWrapperContainerHandle` (= 1, so every existing saved session and host chunk restores unchanged), and a `UniqueSnowflake dspWrapperReservation` member is **registered in the document's `uniqueIdDatabase`** at the top of `InitInstance` — before anything else in the document allocates. From there the namespace defends itself by mechanisms that already exist: the sequential parameter allocator's `while(find(key)) ++key` skips 1 like any other taken handle, and a latecomer claiming 1 (a hand-edited document) is renumbered by `Register`'s existing collision path. No reserved-base magic, no export-shape change, no new rule for anyone to remember — the reservation is a fact in the same map every allocator already consults.

Two loud checks, per Jeff's plastering-over ruling: `InitInstance` asserts if the reservation itself was beaten to the handle, and `importChunkXml` asserts if it did not survive a document rebuild (`DeleteContents` only unregisters objects in the document tree, so it does — verified, not assumed: the map has no bulk-clear on that path and `swap()` has no callers).

### Measured

- **Ordering verified in the artifact:** the standalone's pushed DSP doc now shows the wrapper still at `Id="1"` and the first host-control parameter at **`Handle="0"`, with nothing allocated 1** — the allocator skipped the reservation exactly as designed. No reservation-failure lines, no asserts.
- **The trigger path, end to end:** a hosted Debug render of `tests/hosts/v1-rack.rpp` — REAPER, `setActive` processor recreation, hc59's `ppc` and all — produced **no assert, no drain diagnostic**, a correct restore (both instances build 14,136), and **peak −6.3 dBFS / rms −17.0**, the reference figures. Before this fix the same path desynced the queue on the first parameter update.
- The E56 property survives: handles are still deterministic per load, just numbered around the reservation.

**Learned:**

- **"Register it so everyone knows" beats every clever alternative when a namespace already has an authority.** I had offered a reserved base, an export change, and a send-side filter; Jeff's version needs no new knowledge anywhere because the map IS the knowledge, and both existing allocators already consult it.
- **Find out whose literal it is before deciding whose fix it is.** Three sessions discussed this as a SynthEditLib/GMPI question; one grep found the `Id="1"` in TIDE's own ALLOWED file, which collapsed the gating question entirely.
- **A reservation is only a reservation if it is registered before the first allocation** — and the verification of that ordering is in the exported artifact (parameter handles 0, 2, …), not in the code review.
- **The heredoc backslash trap got me again**, one session after writing it into lessons: `\\n` collapsed and put a real newline inside a C string literal. The rule that sticks: escape-bearing code goes through a Write-tool file, never a heredoc — no exceptions for "just two lines".

**Not verified:** mac/linux builds (no platform code; CI will say); Ableton itself — REAPER exercises the same wrapper lifecycle, but the original reproduction machine is one insert-and-cable session away from closing this for good.

**Machine state.** TideSynth on `tide/win/E64-reserve-wrapper-handle` until this lands; `GMPI` parked on `tide/win/E64-que-selfheal` (open PR #20); all other repos clean on defaults. The installed Debug VST3 carries both fixes.

**Branch/PR:** `tide/win/E64-reserve-wrapper-handle` — TideApp.h/.cpp, this row and entry. Pairs with [GMPI#20](https://github.com/JeffMcClintock/GMPI/pull/20); either lands without the other, but together the collision is impossible AND any future misreader is contained loudly.

## 2026-08-29 — windows — E64 diagnosed to the byte: E56's own fix aimed a parameter at the DSP's root container, and the queue now survives it loudly (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"rebuilt, reproduced it, check the log"* ×3, *"do (b) for sure. Then explain why you changed the handles"*, *"ensure it fires an assertion when this happens"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** closed E64's open question with three measured links, built fix (b) as [GMPI#20](https://github.com/JeffMcClintock/GMPI/pull/20) (PR-GATED — proposed, not merged), and deliberately did NOT touch the handle allocation — that is Jeff's ruling to make, and the options are laid out below. Row **IN-REVIEW** on the queue half.

### The chain, each link measured rather than argued

1. **The desync line, from the live Ableton reproduction** (a file-logging probe, because a GUI host swallows stderr): `declaredLen=1 actuallyRead=0 readyBytes=1 partial=0 handle=1 fourcc=ppc.` — a 1-byte patch-parameter change whose target consumed nothing.
2. **Who owns handle 1**, from a registration probe on a plain launch: `handle 1 registered to class ug_container`. The DSP export has always wrapped the graph in a root `<Module Id="1" Type="Container">` — decoded from the pushed chunk itself, after the on-disk document showed no low handles at all. `ug_container` inherits `dsp_msg_target::OnUiMsg`, a silent no-op, so it "handles" the message and reads none of it.
3. **Who sends a ppc for handle 1**: the host-control parameter that [SynthEditLib#72](https://github.com/JeffMcClintock/SynthEditLib/pull/72) (E56) now deterministically hands handle **1** — that PR's own measurement is `hc6=0 hc59=1`, and `HC_PROCESSOR_OFFLINE` is a bool: the 1 byte. It changes only in a HOSTED lifecycle (processor recreate under `setActive`), which is why three standalone reproductions were clean and Ableton failed every time.

`ProcessMessage` drained an unconsumed payload only when the client returned false; a target that EXISTS but reads short left its bytes to be parsed as the next header — misaligned forever, assert in Debug, silent garbage in Release. Also measured in passing: `ppc for handle 0 -> target (none)` — hc6's updates are silently discarded on every launch, the same disease with a benign face, and `RegisterDspMsgHandle` keeps the FIRST owner on a duplicate (`map::insert`, collision assert commented out), so none of this was visible.

### The fix, and Jeff's correction to it

[GMPI#20](https://github.com/JeffMcClintock/GMPI/pull/20): `ProcessMessage` snapshots the FIFO read index around the client call (new wrap-safe `readIndex()`/`consumedSince()`, deliberately not `_DEBUG`-only) and drains any remainder to the declared length — a misbehaving client damages its own message and nothing after it.

**My first cut demoted the Debug assert to a bounded log, and Jeff rejected that in as many words:** *"ensure it fires an assertion when this happens. because plastering over the root cause and silently swallowing the bug is only going to cause pain later."* He is right, and the shipped shape is: **drain AND assert.** Release gets containment (one lost message, aligned stream); Debug stays loud, with the diagnostic numbers on stderr before the modal. Only the ordinary no-target discard is quiet, as it always was. Consequence stated plainly: **the Debug dialog keeps appearing until the handle collision itself is resolved — by design.**

### Verified

- **Unit A/B** (`C:\SE\_scratch\e64\quetest`, Release = the shipped behaviour): origin/main FAILS the E64 shape — the message behind the short read is lost and the queue never drains; the branch passes 9/9, including the no-target discard control and twenty exact reads across a buffer wrap. The fix's own diagnostic line reproduces the Ableton signature verbatim (`handle 1 msg ppc. consumed 0 of 1`).
- **TIDE VST3 + standalone** (Debug, Jeff's tree) rebuilt and smoke-clean; the fixed plugin is installed locally, so his next insert-and-cable session is the live confirmation (Release-shape: no corruption; Debug: assert until the collision is ruled on).
- **SynthEditCL 113/113** in the scratch Ninja tree with `GMPI_SDK_FOLDER_OVERRIDE` at the branch — GMPI is the bottom layer and SynthEdit is a consumer.

### The handle question, laid out for the ruling rather than decided

Jeff: *"explain why you changed the handles, we can't just mess with how they work."* The honest answer: **#72 did not change the design — it made a broken implementation do what its own comment always promised.** The sequential-ID intent predates it (*"Generate nice sequential IDs… Using the same ID every time ensures resulting DSP XML is consistant and comparable each run"*); the old loop iterated an `unordered_map` expecting sorted order, so in practice the FIRST host-control parameter got 0 and every later one collided and fell back to a **random** handle per load — which is exactly E56's document-never-round-trips bug. What nobody knew: the brokenness was also load-bearing. Random handles almost never landed on the export's root container Id 1; deterministic ones do, every time. And handle **0** was already colliding-and-lost before #72 — measured — so the namespace overlap is old; #72 widened it from a silent data loss into a stream corruption.

Options, all his call: **(i) revert #72** — restores E56's nondeterminism to keep the accidental safety; **(ii) start the sequential IDs at a reserved base** — keeps E56's byte-identical property, one line, but hard-codes knowledge of the export's Id 1; **(iii) stop exporting the root wrapper as Id 1** — cleanest namespace, riskiest change; **(iv) stop the editor queueing ppc for parameters with no DSP-side registration** — fixes the handle-0 loss too. None taken; #20 makes every one of them safe to take slowly.

**Learned:**

- **A fix can be correct by its own Accept and still be load-bearing for a bug it cannot see.** #72's determinism was the right fix for E56 and is what armed E64; the randomness it removed was accidentally keeping a parameter's messages away from a container that would eat them.
- **"Handled" and "consumed" are different claims, and the queue only ever checked the first.** The discard path keyed on the client's return value; nothing anywhere compared bytes consumed against bytes declared outside a `_DEBUG` block.
- **Containment and alarm are separate requirements — do not trade one for the other.** I shipped the drain and demoted the assert in the same edit; Jeff caught it immediately. The drain protects users, the assert protects the codebase, and the fix needed both.
- **Print the numbers before the modal.** The assert dialog ends the session; the fprintf above it is why the next reproduction costs a glance instead of an attach.
- **A four-char code is not a string.** `'ppc'` is three chars + NUL; printing its bytes as `%c` before the lengths truncated the one line the whole reproduction existed to produce.
- **`map::insert` on a duplicate key is a silent policy decision.** First-wins, no error — and the assert that would have said so was commented out. The registration probe found in one launch what three sessions of queue-side analysis could not.

**Not verified:** mac/linux builds of the changed TU (no platform code; CI on the PR will say); the live Ableton confirmation against the fixed binary (installed, awaiting Jeff's next session); which of options (i)–(iv) Jeff rules — nothing is built on any of them.

**Machine state.** `SynthEditLib` probe reverted, clean on `main`. `GMPI` parked on `tide/win/E64-que-selfheal` (open PR #20) — returned to `main` at session end per STEP 5 once Jeff has seen the diff, but left checked out for now since his local rebuild consumes it. TideSynth on `tide/win/E64-diagnosed` until this lands. The installed Debug VST3 carries the fix; Jeff's REAPER config untouched this session. Unit test kept at `C:\SE\_scratch\e64\quetest` with the round-1/round-2 probe logs.

**Branch/PR:** [GMPI#20](https://github.com/JeffMcClintock/GMPI/pull/20) (the fix) + `tide/win/E64-diagnosed` (this row and entry). Merging the TideSynth side alone changes no behaviour; GMPI#20 is the substance.

## 2026-08-29 — macos — E19's mac AU3 cell: the host CAN be isolated here, the locked screen is the real wall, and a second AUv3 will not register (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.3** (no `claude` CLI on this box's PATH; the app's `CFBundleShortVersionString`, which A13 records as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`)

**Did:** took **E19**'s **mac AU3 cell** — the one cell only this box can measure, and the last of E19's three format legs never attempted. **Could not measure it, and the three reasons are each measured rather than asserted.** Row back to **TODO** with a human-sized next step written on it. **E59 → DONE and archived**, flipped on its Accept re-measured here, not on its merges. Branch `tide/mac/E19-au3-mac-leg`. TideSynth only; no product code changed, no sibling repo committed to.

**Why E19 and not another row.** The `mac` NEXT cell is 2026-08-28 and predates a dozen rows closing (S1b, E38, E51, E47, E61, E62, E57, E58, E55, E56, E39, E53), so I re-walked in file order rather than trusting it: **S8** GATED (`SynthEditLib/CMakeLists.txt`), **E7** turns on Jeff's unruled *"where do the jacks live"* — STEP 2 forbids work that differs under an open answer — **E2** not takeable by its own row, **E60** taken by linux ([#550](https://github.com/JeffMcClintock/TideSynth/pull/550) + GMPI_Wrappers#32), **E63**/**E64** `win`, **X2** `linux`. E19 was the topmost eligible row and its stated blocker, E59, had merged.

### The good half: this box can isolate REAPER, and Windows cannot

Copy `REAPER.app` into a directory, `touch reaper.ini` beside it, and REAPER keeps its **entire** resource tree there. Verified in both directions afterwards, because performing a restore is not the same as checking one: the developer's `~/Library/Application Support/REAPER` compares **identical, mtimes and sizes included**, and `~/Library/Audio/Plug-Ins` plus `~/Applications` compare identical across **1,019 files**.

**That is the opposite of the Windows result and the contrast is the useful part.** Windows was measured twice as un-isolatable — `reaper.ini` beside `reaper.exe` does not engage portable mode, removing `reaper-install-rev.txt` does not either, and `SHGetKnownFolderPath` ignores the environment. So *"we cannot isolate the host"* is a **Windows** fact that had begun to read as a fleet one, and a mac run has no excuse for rendering against the developer's configuration the way the 2026-08-28 windows run had to.

### The trap that cost the most, and the control that named it in one command

A **fresh** portable config **hangs on a first-run modal**, and every symptom accuses the plug-in: `-renderproject` never returns, no output appears, the timeout looks exactly like TIDE wedging a render. I spent three attempts on the wrong suspects — REAPER's licence file, then E29's token byte-order, then the screen lock.

The repo's own control settles it, and it loads no plug-in at all:

```
python3 scripts/render-and-measure.py --control
```

Fresh config: **times out at 280 s**. Seed the portable directory with the developer's configured `reaper.ini` (plus the plug-in caches and `reaper-reginfo2.ini` to save a rescan): **rc=0 in 14 s, peak −6.0 / rms −9.0 dBFS**. One command separated "the host cannot start" from "the plug-in is broken", and I should have run it before the other three guesses rather than after.

### The wall: the screen is LOCKED, and the boundary is narrower than "no GUI"

A scheduled run on this box finds the login window up — `screencapture` showed it. The two paths behave differently, measured minutes apart in one session so neither result is about the machine being busy:

| path | locked session |
|---|---|
| `REAPER -renderproject <project>` | **works** — 3–4 s a fixture |
| full GUI launch driven by `Scripts/__startup.lua` | **hangs**, no script output at all |

So E19's **audio** clauses are reachable unattended on this box and its **animation, pixel-diff and int/bool/enum** clauses are not, because all three need the plug-in's editor on screen. That is a permanent property of a scheduled mac run, not a harness gap to close.

**One trap worth naming:** `screencapture` on a locked session returns an **all-black PNG of the full screen size**, not an error. A black frame reads as *"the window drew nothing"* when it means *"the display is off"* — which is E58's lesson (an unpainted region and a deliberately dark one are pixel-identical) turning up somewhere new. The tell was the byte size being **identical** to an earlier capture.

### Why the cell needs a human rather than a better harness

The registered AUv3 is Jeff's `~/Applications/TIDE-Rack-AUv3.app`, dated **2026-08-25** — pre-E59, and built without `TIDE_VCV_FUNDAMENTAL` or `RACK_ADAPTOR_TRACE`, so it carries neither the VCV LFO/Scope this cell needs as producers nor the counters that are its instrument. It cannot answer E19.

**A current build cannot be registered beside it.** Five ways, all failing, all with `pluginkit -m -i <id> -v` answering `(no matches)` while Jeff's stayed registered:

| attempt | result |
|---|---|
| launch the built `TIDE-Rack-AUv3.app` from the build tree | not registered |
| `pluginkit -a` on its appex | rc=0, still not registered |
| clone with distinct `CFBundleIdentifier` **and** distinct AU subtype `Dr19` | not registered |
| inside-out ad-hoc re-sign (`codesign -vvv --strict` passes on appex *and* app) | not registered |
| `lsregister -f` on the clone in `~/Applications` | not registered |

So measuring a current build's AU3 in a real host means **displacing the developer's registration**, and an unattended run must not: if the run died in between, his AUv3 would be left pointing at a build tree that gets deleted. I stopped there rather than doing it and hoping to restore.

**What a human needs to do is small**, and it is on the row: on an unlocked session, build `TIDE_Rack_AU3_assemble` with `-DTIDE_VCV_FUNDAMENTAL=ON -DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`, copy the app over the one in `~/Applications`, and **launch it once** — launch is what registers an AUv3; the CMake comment's measurement that a copy alone does not register still holds.

### The build side is proven, so nobody re-does it

`TIDE_Rack_AU3_assemble` is **green on macOS main**, 0 errors. The appex links VCV Fundamental (**76** references in its link edge, **38** module objects built) and carries `RACK_ADAPTOR_TRACE`'s strings and E59's `declined to publish the startup default`.

**One check that will mislead the next run, so it is written down:** `strings` for `VCV: Scope` returns **0** on a correctly-linked binary. Ids are composed at runtime as `"VCV: " + slug` (`RackAutoRegister.h:40`), so the literal never appears. I read that zero as "VCV did not link", and a second wrong reading agreed with it — I searched the appex's link edge for `VCV` in a **4,000-character** window when the edge is **12,201** characters. Both were my error; the linker had done its job the whole time.

### E59 confirmed on macOS main, through a real host

| fixture | measured |
|---|---|
| `tests/hosts/v1-rack.rpp` | **peak −6.3 dBFS, rms −17.0 dBFS — AUDIO PRESENT** |
| `tests/hosts/v1-rack-uncabled.rpp` | **−inf — SILENCE** (negative control) |

The first is the 2026-08-18 macOS reference *to the decimal* and the same figure the windows box measured after its fix, now reproduced from an isolated REAPER on a locked session. That is what E59 was flipped **DONE** on — the Accept re-measured, not the three merges.

**Learned:**

- **Run the harness's own no-plug-in control BEFORE theorising about the plug-in.** A fresh REAPER config and a broken plug-in produce an identical timeout; `--control` separates them in 14 seconds, and I reached for it fourth instead of first.
- **"We cannot isolate the host" can be true on one platform and false on another, and the fleet will generalise it.** Windows measured it twice as impossible; macOS does it with `touch reaper.ini`. A negative result about a machine deserves re-testing on each machine, not inheriting.
- **A locked screen is not "no GUI" — it is a line through the middle of the tooling.** Offline rendering works and editor-driving does not, so which clauses of an Accept are reachable depends on which side of that line they fall.
- **A black screenshot is a reading about the DISPLAY, not about the window.** Identical byte sizes across two captures were the tell; the image alone would have kept blaming the app.
- **A `strings` miss is only evidence once you know how the string is built.** Runtime-composed ids never appear as literals, so the absence of `VCV: Scope` proves nothing — and searching a truncated window of a 12 KB link line manufactured a second wrong answer that agreed with the first.
- **Two wrong readings that agree are not corroboration when they share a cause.** Both of mine came from asking a binary a question its build never promised to answer.
- **When the safe version of an experiment does not exist, stop rather than doing the unsafe one carefully.** Displacing a developer's AUv3 registration is recoverable right up until the run dies holding it.

**Not verified:** the AU3 leg itself — instantiation, the 60-second animation window, the int/bool/enum toggle and the pixel diff are all untouched, and `string` still has no producer; whether REAPER on macOS would even list the AUv3 once a current build is registered (its AU cache has never held a TIDE entry); and whether the first-run modal is the licence prompt or the audio-device prompt, which I did not isolate because seeding `reaper.ini` fixed both at once.

**Machine state.** All six repos were clean and on their default branches at the start; TideSynth is on this run's branch until STEP 5 returns it. **Jeff's `~/Library/Application Support/REAPER` is byte-identical to its pre-run snapshot** (mtimes and sizes), and his plug-in folders plus `~/Applications` are identical across 1,019 files. The test clone `~/Applications/TIDE-Rack-E19.app` was removed, deregistered with `pluginkit -r` and `lsregister -u`, and `pluginkit -mv` now lists only his own extension, unchanged. One `TIDE-Rack.appex` process my `auval -a` spawned was killed. Build tree `build-e19mac/` is gitignored (`SE_LOCAL_BUILD=OFF`, so its POST_BUILD could not replace his installed plug-ins — checked in `build.ninja` before building, and confirmed after). The portable REAPER copy, its seeded config and all renders are under the session scratchpad, outside every repo. No REAPER or TIDE process left running.

**Next:** **E19's mac AU3 cell wants one unlocked interactive session** — build, copy the app into `~/Applications`, launch once, then the linux box's instruments apply unchanged. **The audio half of E19 is already reachable unattended here** now the portable recipe exists, so a scheduled mac run can re-measure any fixture cheaply. **E60** is with linux on [#550](https://github.com/JeffMcClintock/TideSynth/pull/550).

**Branch/PR:** `tide/mac/E19-au3-mac-leg` — the E19 row, E59's flip and archive, the macOS section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry.

## 2026-08-28 — windows — E64 filed: the ui->dsp queue desyncs in a HOST but not in the standalone, and three negatives are the finding (interactive, Jeff directing)

**Prompt:** interactive, Jeff directing (*"hmm"* with a screenshot, then *"take it"*, then *"you can instrument it"*). As **tide-rack-bot** (both paths). Prompt sha b97bc00a5.

**Did:** took the assertion Jeff hit in Ableton, established which queue it is by attaching to the live process, built an instrumented Debug build that names the offending message, failed to reproduce it three different ways, and filed **E64** with the negatives as its content. **No fix. The row is the deliverable**, and the instrument is left in place for the one reproduction only a human can currently drive.

### First: it is not E59, and one command settled that

The binary that faulted was built at 17:39 and contains **zero** of E59's new strings (`syncState exporting` ×0, `declined to publish the startup default` ×0). E59 merged as [#547](https://github.com/JeffMcClintock/TideSynth/pull/547) after that, so the fault predates it. Worth doing before any analysis: the alternative is a session spent auditing your own change.

### The stack, from attaching to the live process rather than reasoning

Ableton was still up with the modal open — `MainWindowTitle` was literally *"Microsoft Visual C++ Runtime Library"*, which is how the PID was found without asking. `cdb -p <pid> -c "~*k; qd"` (**`qd`, not `q`** — `q` would have killed his DAW):

```
ucrtbased!wassert
TIDE_Rack!gmpi::hosting::interThreadQue::pollMessage+0x218
TIDE_Rack!SynthRuntime::ServiceDspRingBuffers+0x75
TIDE_Rack!SeAudioMaster::ServiceGuiQue+0x3f
TIDE_Rack!ug_patch_automator::HandleEvent+0x9d
TIDE_Rack!SynthEdit::subProcess+0x329
TIDE_Rack!wrapper::Processor_VST3::process+0x15a8
```

So it is **TIDE's own `queUiToDsp`** (parameter 3), on an Ableton `AudioCalc` thread — not the wrapper's queue and not the DSP->GUI direction #410 fixed. There were two candidate queues before this and no way to choose between them; the stack costs one command and removes the guess.

I went back for the locals and **lost that race** — the process was gone. Thread NUMBERS also change between attaches, so the second attempt selected a dialog-pump thread; select by thread ID (`~~[0x258c]s`), not by index.

### Three negatives, and they are the actual finding

An instrumented Debug build replaces the bare assert with a line naming `handle`, `msgId`, `declaredLen`, `actuallyRead`, `readyBytes`, `partial`. With it:

| attempt | result |
|---|---|
| hosted state-restore + `-renderproject` of `v1-rack.rpp` | **no desync** |
| STANDALONE, insert an Oscillator over the command channel — document really rebuilt, `18307 -> 23397 bytes` | **no desync** |
| 160 `--set-param` writes | no desync — but see below, this arm proves little |

**The `--set-param` arm is weak and I am labelling it rather than counting it:** `--info` reports `parameterCount: 4`, which is the plug-in's four GMPI parameters, not rack knobs. Rack edits never travel that way, so that burst never touched the path under test.

The other two are real. **`queUiToDsp`'s bytes come from `SynthRuntime_editor::takeUiToDspMessages`, which the standalone shares** — and the standalone, doing the same insert, is clean. What differs is how a changed blob parameter reaches the processor: the standalone calls `onParameterChanged` **directly** (`StandaloneHost.cpp:162`), while VST3 goes `sendNonNativeParameterToProcessor` -> `IMessage` -> `Processor_VST3::notify` -> `m_message_que_ui_to_dsp` -> pin, **asynchronously, through a last-writer-wins parameter**. That is what is left.

### What I ruled OUT, including my own first answer

**Overflow is not it.** My first hypothesis was `SynthEdit::onSetPins`' drop-on-full arm — it discards the whole blob when it will not fit, and a drop while the receiver is mid-partial-message would desync it permanently. Then I measured the capacities: **both queues are `AUDIO_MESSAGE_QUE_SIZE` = 5 MB**. An insert-plus-cable burst overflowing that is not plausible, so the story was wrong and I said so before building anything on it.

Two candidates survive, both unconfirmed and both written on the row: a blob overwritten before the processor consumed it, or `ServiceWaitersIncremental`'s **multi-part mode** (`startMultiPartSend`) splitting an oversize message across calls, where a lost part cannot be reassembled. Note `takeUiToDspMessages`' own comment asserts *"ServiceWaitersIncremental only ever Sends complete messages"* — that is **not unconditionally true**, and it is load-bearing.

### The instrument left behind, and the one thing that must be undone

The probe now appends to a **file** (`C:\SE\_scratch\e64\tide-e64.log`) as well as stderr, because a GUI host swallows stderr — the first version was stderr-only and would have told Jeff nothing in Ableton. The **assert is left intact**, so his experience is unchanged. His build tree is `SE_LOCAL_BUILD=ON`, so his next rebuild installs it where Ableton loads it; one reproduction then names the message.

**That probe is UNCOMMITTED in the `GMPI` working tree and must be reverted** — `git -C C:\SE\GMPI checkout -- Hosting/message_queues.cpp`. GMPI is PR-GATED; this is the E47 precedent (temporary probes, reverted, nothing shipped), not a change.

**Learned:**

- **When a modal is on screen, the process is a live specimen — attach before anyone dismisses it.** The window title of an assert dialog is the app's own title, so the PID is greppable without asking. `qd` detaches; `q` would have killed the DAW.
- **Thread numbers are per-attach; thread IDs are not.** My second attach selected a completely different thread under the same number and reported a dialog pump as the fault site.
- **A negative in one host is evidence about the TRANSPORT when the content path is shared.** The standalone and the plug-in build the same bytes from the same function; only the delivery differs, so "reproduces in one and not the other" points at delivery rather than at the message.
- **Measure the capacity before believing an overflow story.** Two queues at 5 MB killed my first hypothesis in one grep, after I had already written it down as the likely cause.
- **A probe that writes only to stderr is useless in a GUI host.** Ableton shows none of it; the file is what makes the next reproduction worth anything.
- **`--set-param` on a plug-in's GMPI parameters is not a rack edit.** `parameterCount: 4` should have told me before I sent 160 of them.
- **This tool's heredocs collapse backslashes**, so escape-heavy Python must go through a file, not `<<'PY'`. It cost three malformed edits today, one of which silently ate characters out of a documented path.

**Not verified:** the mechanism, entirely — two candidates are named and neither is confirmed; whether it reproduces in REAPER as well as Ableton (which would make it scriptable here); and whether Release silently mis-parses rather than merely skipping the check, which is the part that matters for a shipped build.

**Machine state.** `%APPDATA%\REAPER` was backed up, narrowed to the probe bundle, restored and verified **identical including mtimes**. Jeff's installed VST3 is untouched by me (he rebuilds it himself). Build tree `build-e59/` is gitignored and multi-config; the Debug bundle, fixtures and logs are under `C:\SE\_scratch\e64\`. No TIDE or REAPER process left running by me. **`C:\SE\GMPI` is deliberately DIRTY with the probe described above.**

**Next:** **Jeff rebuilds and repeats the gesture in Ableton**; the log file then names the message and the two candidate mechanisms collapse to one. After that the fix is likely in `Processor_VST3`'s blob transport rather than in TIDE.

**Branch/PR:** `tide/win/E64-uidsp-que-desync` — the E64 row and this entry. No product code; the probe is not committed.

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
