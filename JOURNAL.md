# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-09-01 — linux — E78: CLAP had E74's defect, and fixing it uncovered two more (interactive continuation, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths) · interactive continuation of the scheduled run below, Jeff directing (*"fix E78 too while you have the harness up"*)

**Did:** fixed **E78** — one line, the CLAP twin of E74 — and measured it against a probe I had to write, because the harness that was up could not host a CLAP GUI at all. **The fix works and one clause of E78's own Accept is still unmet.** Two new rows: **E79** (a shipping defect) and **E80** (E78's unmet half). Branches unchanged: the fix rides `tide/linux/E74-linux-timer-pump` ([GMPI_Wrappers#38](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/38)) because it needs [gmpi_ui#17](https://github.com/JeffMcClintock/gmpi_ui/pull/17)'s `pump()` exactly as E74 does; the probe rides [#569](https://github.com/JeffMcClintock/TideSynth/pull/569).

### "While you have the harness up" turned out not to hold, and that is most of this entry

REAPER 7.43 **cannot host TIDE's CLAP GUI on this box.** `TrackFX_Show` — float (3) and FX-chain (1), both tried — kills REAPER from inside its own GTK:

```
Gdk-CRITICAL: gdk_screen_get_root_window: assertion 'GDK_IS_SCREEN (screen)' failed
Gdk-CRITICAL: gdk_window_get_display:     assertion 'GDK_IS_WINDOW (window)' failed
Gdk-CRITICAL: gdk_x11_display_get_xdisplay: assertion 'GDK_IS_DISPLAY (display)' failed
Gdk-CRITICAL: gdk_x11_window_get_xid:     assertion 'GDK_IS_X11_WINDOW (window)' failed
```

Those are REAPER's own GDK failing to produce a window, so it never reaches `guiSetParent` and the plug-in is never asked for anything. **The VST3 editor floats fine in the same REAPER, same weston, same session**, which is what makes this the host and not us. The staged driver is what established it — `measure-clap.lua` logs *"about to TrackFX_Show mode N — if the log stops here, that call killed the host"*, and it does.

### So the instrument is a CLAP host of our own, and it is the reusable part

[tests/e78_clap_gui_probe.c](tests/e78_clap_gui_probe.c): dlopen, `clap_entry`, `state->load`, then the full GUI dance — `is_api_supported` / `create` / `set_parent` / `show` against a **real X11 window** — with `process()` on a **real second thread** and the main thread servicing the two host extensions the Linux editor requires and refuses to work without: `clap_host_timer_support` and `clap_host_posix_fd_support`.

That extension pair is the whole point. `guiIsApiSupported` returns false unless the host offers both, so a probe without them measures nothing, and `Editor_CLAP.cpp:269` registers the 16 ms timer in `guiSetParent` — which is where every question in this entry ends up.

### The A/B, one change apart

The first attempt was contaminated and worth recording: I had returned every repo to its default branch in the previous run's STEP 5, so the AFTER binary got built with TideSynth on `main` and **silently lacked the two counters** the whole measurement depends on. The tell was `feedback send` lines present in BEFORE and absent in AFTER — not a plausible outcome of a timer fix. Rebuilt both arms from the same trees; the BEFORE binary came back **byte-identical** (`419821d7…`), which is what says the second pair is a clean pair.

30 s, five-module prepared rack, `state->load` 51,690 of 51,690 both arms:

| | BEFORE | AFTER |
|---|---|---|
| processor shipped | 1,600 | 1,600 |
| editor received | **0** | **1,600** — send #N against received #N |
| `RackEditor: light` | **frozen #2, value 0.000** | **#3300, value 0.754**, varying |
| host timer ticks | 1,782 | 1,604 |

**The tick count is the control and it is the line to read first.** The host timer was firing ~1,700 times in *both* arms. So this was never a missing tick; it was the plug-in not using a tick it was already being handed. Without that number the result would read as "the probe started working", which is a different claim.

### What is still broken, and it is two separate things

**E79 — a hosted Linux CLAP with NO editor open never receives its document.** The timer is registered in `guiSetParent` and unregistered in `guiDestroy`, so with no window there is no UI-thread tick at all, and `Controller_CLAP::onTimer` is the only thing that carries the controller's document to the processor. Measured in REAPER, transport rolling 8 s with the editor deliberately never shown: `controller #1 restore of a 43199 byte document -> imported`, then `unprepared - writing silence`, and **no `building rack` line ever**. **A user who loads a project and presses play without opening the window gets silence.** E78's fix cannot reach it — it is a lifetime question, not a pump question.

**E80 — floats now traverse the CLAP channel and a 65,548-byte blob does not.** Lights run to #3300; `display-state update` stays frozen at `#1 arrived (0 bytes)`, where VST3 reaches #2160 and the standalone #1400 on the same tree. Located by TIDE's own send counter, which is *inside* the plug-in and upstream of any wrapper: the largest payload it ever packs on CLAP is **200 bytes**, against **65,673** repeatedly on VST3. Identical at block sizes 128, 512 and 2048, and `display-state capture #700` says the DSP captured it every time — so the blob never enters `queDspToUi`, and it is not pacing.

**The honest confound, which is why E80 is TODO and not a diagnosis:** the only host that has ever driven a TIDE CLAP GUI is our own probe, and no DAW has arbitrated because REAPER cannot. Step one there is a second opinion, not a fix.

**Learned:**

- **"While you have the harness up" is an assumption to test, not a saving.** The VST3 harness could not host a CLAP GUI at all, and finding that out cost more than the fix did. The staged driver — log the intent, then make the risky call — is what turned a silent host death into one line of evidence.
- **A probe that supplies the host extensions is not optional, it IS the measurement.** `guiIsApiSupported` refuses X11 unless the host offers timer *and* posix-fd support, so a simpler probe would have been told "no" and proved nothing. The thing the plug-in demands from a host is the thing worth implementing.
- **Count what the host did, not only what the plug-in did.** 1,782 timer ticks in the failing arm is the single number that makes this a plug-in defect rather than a harness improvement, and it cost one counter.
- **Returning every repo to its default branch is STEP 5 working, and it will silently un-build your next measurement.** The AFTER binary lost its instrumentation because TideSynth was back on `main`; nothing failed, the log was just quieter. Check the branch of *every* repo the artifact is built from before an A/B, not just the one you are editing.
- **A byte-identical rebuild is the cheapest possible proof that an A/B is clean.** The BEFORE binary re-linked to the same sha256, so the only difference in the second pair is the one line.
- **When one datatype crosses and another does not, stop looking at the transport.** Lights and display state ride the same pins, the same queue and the same wrapper; floats arriving and blobs not is a statement about the blob path, and it turned a vague "CLAP is still broken" into E80's one sentence.
- **`gdk_*: assertion failed` from a DAW is the DAW's, and chasing it is chasing someone else's bug.** Worth ten minutes to establish and no more; the way out was to stop using that host, not to fix it.

**Not verified:** **E80's cause**, entirely — the probe is our own host and nothing has arbitrated it; **E79**, which is filed from a measurement of the defect and carries no fix; whether the REAPER CLAP-GUI crash affects other CLAPs or only ours (no second CLAP was tried); the **Wayland** VST3 editor's copy of E74's fix, unchanged from the previous entry; **Windows and macOS**, where none of this was built or run and where the native timers mean the pump is inert by construction.

**Machine state.** TideSynth, gmpi_ui and GMPI_Wrappers are on their E74/E78 branches until STEP 5 returns them; `SE16`, `SynthEditLib` and `GMPI` were not touched at all this continuation. REAPER still ran only against the scratch `HOME`; `~/.vst3`, `~/.clap` and `~/.config/REAPER` are untouched and `~/.config/REAPER` still does not exist. The CLAP was installed as the documented semi-bundle **inside the scratch home** (`$SCRATCH/home/.clap/TIDE-Rack/`), never the developer's. `build-e19/` is gitignored and now carries the CLAP fix. Weston, REAPER and every probe were stopped with `scripts/kill-named.sh`.

**Next:** **E79 is the biggest thing on this lane** — a shipping defect a user meets by pressing play. **E80 wants a second opinion before a fix**, and the cheapest one is a second CLAP host with a GUI. Then **E75** and **E76**. And **E79's question is worth asking of VST3 too**: it restored fine here, but by a different route, so "who carries the document when no window is open" has only been answered for one wrapper.

**Branch/PR:** `tide/linux/E74-linux-timer-pump` ([GMPI_Wrappers#38](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/38)) — the CLAP one-liner, alongside E74's; `tide/linux/E74-editor-processor-rebind` ([#569](https://github.com/JeffMcClintock/TideSynth/pull/569)) — the probe, `frame_clap_chunk.py`, `prepare-clap.lua`, `measure-clap.lua`, `measure.lua`'s `E19_PROJ` fix, E78/E79/E80 and this entry.

## 2026-09-01 — linux — E74: the editor was never bound to ANY processor, and nothing pumps GMPI's timers in a hosted Linux plug-in (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required

**Did:** took **E74**, the `linux` NEXT cell's pick and the whole of E19's remaining linux VST3 FAIL. **Fixed and measured.** Branches `tide/linux/E74-editor-processor-rebind` (TideSynth), `tide/linux/E74-linux-timer-pump` (gmpi_ui and GMPI_Wrappers, which must merge together). STEP 1 and STEP 1.5 were both genuinely empty: no `platform:linux` issue, and **no open PR and no `tide/*` branch in any of the six repos** — the state the 2026-09-01 macos entry recorded, still true at the start of this run.

### The row's own diagnosis was wrong, and how it was wrong is the useful part

E74 said *"a hosted VST3 recreates the DSP instance and the editor's rack feedback pins stay bound to the retired one."* **The editor was never bound to any instance.** The recreation is real, still happens after the fix, and has nothing to do with it.

What separated the two was instrumenting **both ends of the blob parameter**, which nothing had done — every counter in the row belongs to the *inner* rack, on one side or the other of a channel nobody had measured. Two counters, both behind `RACK_ADAPTOR_TRACE`, on the same first-few-then-every-hundredth cadence the adaptor already uses:

| where | line |
|---|---|
| `SynthEditSem/SynthEdit.cpp`, `drainRackFeedback()` | `TIDE: instance #N feedback send #M (B bytes, H held back)` |
| `SynthEditSem/TideApp.cpp`, `receiveRackFeedback()` | `TIDE: editor received feedback #M (B bytes)` |

One 45 s run answered it:

| | hosted VST3 | STANDALONE (control) |
|---|---|---|
| processor shipped | **2,700** updates, `0 held back` | 2,900 |
| editor received | **0** | **2,900** — one-for-one, send #N ↔ received #N |

**Zero, not "some" — and zero from instance #3 as well as #4**, which is what kills the row's story. It also disposes of the previous run's reading of the ordering: `no RackEditor line ever again` after instance #4 is true and means only that the editor never got a line from #3 either.

### The cause, and it is bigger than E74

**`gmpi::TimerManager` has no native timer source on Linux, and nothing in a hosted plug-in pumps it.**

`gmpi_ui/helpers/Timer.cpp`, `Timer::start()`: `SetTimer` under `_WIN32`, `CFRunLoopTimerCreate` under `__APPLE__`, and under `#if !defined(_WIN32) && !defined(__APPLE__)` it merely marks itself running and relies on somebody calling `TimerManager::pump(elapsedMs)`. The header says so in as many words — *"the host application must call this periodically on the UI thread"*. The only caller in any repo is `GMPI_Wrappers/wrapper/Standalone/StandaloneApp.cpp:468`. **A plug-in has no application loop of its own.**

So **no `gmpi::TimerClient` in a hosted Linux plug-in has ever run.** `Controller_VST3` is one (`Controller_VST3.h:95`, `startTimer(timerPeriodMs)` at `Controller_VST3.cpp:383`), and `Controller_VST3::onTimer` (`:788`) is **the only caller** of `gmpiController.message_que_dsp_to_ui.pollMessage()`. That is the entire DSP→GUI channel. It also flushes `queueToDsp_`, so the **GUI→DSP direction was going nowhere either**.

Everything upstream was working perfectly and said so: `gmpi_processor::setPin` stored the blob and queued the waiter, `Processor_VST3::process` serviced it into `m_message_que_dsp_to_ui` (`:1017`), and `CommunicationProc` shipped it to the controller as a `BinaryMessage`. The controller received it and pushed it into a queue **that nothing ever polled**.

### The fix

`TimerManager::pump()` — a no-argument overload that measures its own elapsed time from `steady_clock` — called from `SEVSTGUIEditorLinux::onTimer()` and its Wayland twin, which are the host `IRunLoop` ticks registered at `kTimerIntervalMs = 16` and are the only UI-thread tick a hosted plug-in gets here.

**Self-timed rather than `pump(16)`, and the reason is not tidiness.** There is one run-loop tick **per open editor** and this manager is a process-wide singleton, so two instances of the same plug-in would each pump 16 ms every 16 ms and run **every** timer client in the process at twice its period — a defect that would only show up with two windows open. Self-timed, the second caller observes that no time has passed. The first call only starts the clock, so nobody is handed the process's whole uptime.

### Verification — E74's Accept, literally

Same harness, same tree, one change apart. REAPER 7.43 on headless weston, prepared 43,195-byte five-module rack, transport rolling the whole 75 s (`playstate=1`, position 0 → 74.931).

| | before | after |
|---|---|---|
| feedback shipped / received | 2,700 / **0** | 4,600 / **4,600** |
| `Scope display-state capture` | #2100 | #2100 |
| `display-state update … arrived` | **frozen at #1, 0 bytes** | **#2160, 65,548 bytes** |
| `light … update` | **frozen at #2, value 0.000** | **#9100, value 0.516, varying** |

The row's Accept asks for both counters *still advancing 60 s after the last `building rack` line*. Measured by line position, not inferred: that line is 117 of 662, and **214 `RackEditor: display-state update` lines follow it**, running `#30 → #2160`, with `light 1 update #200` → `light 0 update #9100`.

**Standalone control across the same A/B, byte for byte the same numbers before and after** — capture #1300, `display-state update` #1400/#1370, `light 1 update #5800 value 0.137` — which is what says the change did nothing to the path that already worked.

**Builds:** TIDE `build-e19` (Release, `TIDE_VCV_FUNDAMENTAL=ON`, `-DRACK_ADAPTOR_TRACE=1`, `SE_LOCAL_BUILD=OFF`) rc=0 on every step. **`SynthEditCL` builds `314/314`, 0 errors** against the changed `gmpi_ui` and `GMPI_Wrappers` — the shared-repo rule discharged by a build rather than by scope, from a scratch tree (`-DFETCHCONTENT_SOURCE_DIR_GMPI_UI=` / `_GMPI_WRAPPERS=` pointing at the working copies), so the developer's own `SE16/build/` was not touched.

### E78: CLAP has the identical hole, filed rather than fixed

`Controller_CLAP` is the same shape — a `gmpi::TimerClient` (`Controller_CLAP.h:9`) started at 15 ms (`Controller_CLAP.cpp:20`) whose `onTimer()` (`:28`) is the only caller of `pollMessage()` **and** of `pendingQueueClients.ServiceWaitersIncremental(&message_que_ui_to_dsp, …)`. The CLAP host tick that exists and does not pump is `Processor_CLAP::onTimer(clap_id)` at `Editor_CLAP.cpp:1062`. **One line, and it is not fixed here because no CLAP GUI host was driven this run** — and `tests/e60_clap_state_probe.cpp` cannot do it, because it drives the C ABI with no editor and therefore has no run-loop tick to pump.

**Learned:**

- **A filed row is one run's reading, and STEP 1's "re-verify before acting" deserves to apply to BACKLOG rows too.** E74 named a mechanism, a place to look and a scope, and all three were wrong. What made it worth taking anyway was its *Accept*, which was right and which the fix satisfies.
- **Instrument both ends of a channel before believing either end.** Every counter this project had was inside the inner rack; the outer blob parameter that carries them between processes had none, and it was the whole defect. Two `fprintf`s and one run.
- **"Both sides are correct and the middle is missing" looks exactly like "the wrong side is attached".** The DSP was shipping, the wrapper was serialising, the controller was receiving, the GUI was listening — and nothing polled the one queue in between. Reading either side alone confirms the other side is at fault.
- **A platform with no native timer is a whole class of dead code, not one dead feature.** Nothing `gmpi::TimerClient` does has ever run in a hosted Linux plug-in. Before blaming a Linux plug-in symptom on the plug-in, ask whether the thing that should have ticked is a `TimerClient`.
- **A process-wide singleton pumped from a per-window callback is a bug waiting for a second window.** `pump(16)` from the editor would have passed every test here and run every timer at 2× with two instances open. Measuring elapsed time inside the singleton costs eight lines and removes the question.
- **`scripts/kill-named.sh` exists; `pkill -f 'REAPER/reaper'` killed this shell with exit 144.** Fourth time in the fleet, and the script was written after the third — the lesson is not "remember the trap", it is "the tool is in `scripts/`".
- **Writing a source file back with Python's text mode strips CRLF and produces an 800-line diff of nothing.** `git diff --ignore-all-space --stat` against `git diff --stat` is the one-command tell; patch in binary mode with `\r\n` in the search strings.
- **A NEXT cell is a table row, so replacing its opening text and keeping the tail silently makes a four-column row in a three-column table.** `check-next-block.py` still said OK. Count the pipes.

**Not verified:** the **Wayland** VST3 editor's copy of the fix — it compiles in this configuration (`SEVSTGUIEditorWayland.cpp.o` is in the build) but the measurement ran on Xwayland, so `SEVSTGUIEditorLinux` is the one that was exercised; **E78**, entirely, which is inspection plus E74's A/B and no CLAP measurement; **Windows and macOS**, where this changes nothing by construction (both have a native timer and neither Linux editor is compiled) but where nothing was built or run; whether the restored GUI→DSP direction changes anything a user would notice, which was not this row's question; E19's **pixel-diff** and **int/bool/enum** clauses, still blocked on E75's fixture as the 2026-08-31 entry recorded.

**Machine state.** All six repos were clean and on their default branches at the start; TideSynth (2 commits) and SE16 (2) were fast-forwarded to their remotes and the other four were already current. **`SE16` was NOT committed to** — it was used read-only for the SynthEditCL build, out of a scratch tree. TideSynth, gmpi_ui and GMPI_Wrappers are each on this run's branch until STEP 5 returns them. REAPER 7.43 was downloaded fresh into the session scratchpad and ran only against a scratch `HOME`; `~/.vst3`, `~/.clap` and `~/.config/REAPER` were never written and `~/.config/REAPER` still does not exist. The standalone ran under a scratch `XDG_CONFIG_HOME`, so `~/.config/TiDE Rack/` is untouched. `build-e19/` is a gitignored scratch tree; Jeff's `build/` trees in `SE16` and `SE` were not touched. Headless weston, REAPER and the standalone were all stopped with `scripts/kill-named.sh` — 0 of each left running.

**Next:** **E75** is cheap and unblocks two more of E19's clauses; **E76** is one wrapper but its Accept turns on a ruling. **E78 before E19's linux CLAP cell, not after** — the cell cannot be measured through a channel that is dead for the same reason E74 was. And **the timer finding is worth a look on the other two boxes**: nothing here is Linux-specific except the absence of a native timer, so the question *"which `gmpi::TimerClient` never ran"* is only closed on Windows and macOS because `SetTimer` and `CFRunLoopTimer` happen to exist.

**Branch/PR:** `tide/linux/E74-editor-processor-rebind` (TideSynth) — the two trace counters, E74's row, E78, the refreshed `linux` NEXT cell, and this entry; `tide/linux/E74-linux-timer-pump` (gmpi_ui, GMPI_Wrappers) — the fix itself, in two repos that must merge together.

## 2026-09-01 — macos — E73 DONE, and the fleet has no open PRs and no agent branches for the first time (state update, interactive)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** · as **tide-rack-bot** (both paths) · interactive continuation of the scheduled run below, Jeff directing (*"sync all tide related repos, any PRs outstanding?"*, *"merge 565"*, *"delete 559"*, *"flip E73 to DONE"*)

**Did:** bookkeeping and cleanup, no code and no measurement this entry claims credit for. **E73 → DONE and archived** after [#565](https://github.com/JeffMcClintock/TideSynth/pull/565) merged; the stale `tide/mac/E65-panel-draft-render` branch deleted; all six repos synced.

### What landed

[#565](https://github.com/JeffMcClintock/TideSynth/pull/565) squash-merged as [`b824422`](https://github.com/JeffMcClintock/TideSynth/commit/b824422) at 21:59Z, 6 checks green, branch auto-deleted. Verified by `gh pr view 565 --json state` rather than inferred from the push — the 2026-08-18 A4/#120 trap is exactly this shape read the other way round.

It had been open a day and a half, and **the only thing holding it was the CONFLICTING state the scheduled run cleared that morning.** Nothing was ever wrong with the work.

### The sync and the sweep

Five of six repos were already current; **TideSynth's own `main` was 9 commits behind** (`ab251e0` → `14c3aaa`), which is worth noting because this box had been sitting on the merge-base all along and nothing said so. Fast-forward only; nothing stashed, nothing discarded.

**Open PRs across TideSynth, SynthEditLib, SynthEdit, GMPI, GMPI_Wrappers and gmpi_ui: zero.** Open issues: one, `#44`, the CI watchdog digest, which is not work. **This is the first time the fleet has had no open PR and no `tide/*` branch at all.**

`tide/mac/E65-panel-draft-render` (PR [#559](https://github.com/JeffMcClintock/TideSynth/pull/559), CLOSED not merged) was deleted at Jeff's instruction. **Checked rather than assumed before deleting**, which is the 2026-08-28 lesson about this exact branch: `git cherry` showed all three commits unmerged, but the two things anyone would want are accounted for — the probe and its fixture (`tests/e65_panel_preview_probe.py`, `tests/fixtures/e65-prefix-7panel.log`) are **on `main`**, salvaged as Jeff asked on 2026-08-31, and the remaining `TiDEPanelGui.cpp` scheduler fix is the one `b4bd4f49f` superseded and which must not be merged over it. **The commits survive at `refs/pull/559/head`** — verified to point at the same `fc9bbcd` *before* the delete, so this is reversible.

**Learned:**

- **Verify a merge by asking about the PR, not by reading your own push.** One `gh pr view --json state` separates "I pushed" from "it landed", and this project has a precedent in each direction — #120 merged out from under a follow-up, and rows have claimed DONE on unmerged PRs.
- **`git cherry` says what is unmerged, not what is lost.** All three E65 commits were unmerged and the branch was still safe to delete, because the parts worth keeping had arrived on `main` by a different route. The two questions are different and only the second one matters.
- **Establish the recovery ref before the destructive command, not after.** `refs/pull/559/head` survives branch deletion and pins the same tip — checking that first turned "delete this" from irreversible into reversible, and cost one `ls-remote`.
- **A local default branch can be silently stale on a box that has been doing work all along.** This one was 9 commits behind while the run pushed and merged perfectly happily, because every operation that mattered used `origin/main` explicitly. Worth a `git status -sb` at the end of a run rather than trusting "I was on main".

**Not verified:** nothing new — this entry measures nothing. E73's evidence is the 2026-08-31 interactive entry's, and the 599/599 build it carries is the 2026-09-01 scheduled entry's.

**Machine state.** All six repos on their default branches and clean; TideSynth on this flip's branch until its PR merges. No sibling repo was committed to. The developer's installed plug-ins are untouched (VST3 still sha256 `f3b09c3c…`, Aug 28 17:45:54; CLAP still Aug 22). Nothing running. `build-e73merge/` from the earlier run remains as a gitignored scratch tree — it is a warm 599-target Release/arm64 build, so any further mac measurement is minutes rather than an hour.

**Next:** **the queue's shape is now four rows on one blocker and three on rulings.** **E71, E77, E19's mac AU3 cell and E75 all want a single unlocked interactive session with a GUI host.** **E72, E76 and S8 want rulings, not sessions.** **E2 wants a product decision — which modules the first curated set contains — and it is what blocks E3 and E4.** Takeable without any of that: **E63** (win) is a real shipping defect, a Windows release package missing `DefaultRack.synthedit` and two pin XMLs; **E74** (linux) is the whole of E19's remaining linux FAIL with its harness already in the tree; **X2** (linux) is bookkeeping.

**Branch/PR:** `tide/mac/E73-done` — E73's flip and archive row, the refreshed `mac` NEXT cell, and this entry.

## 2026-09-01 — macos — STEP 1.5: #565 had gone CONFLICTING, and BACKLOG.md merged cleanly into two different E74s (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** (no `claude` CLI on this box's PATH; A13 records the app's `CFBundleShortVersionString` as the discoverable one on a mac) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required

**Did:** **STEP 1.5, and it was the whole run.** This platform's only open PR, [#565](https://github.com/JeffMcClintock/TideSynth/pull/565) (E73), had gone **CONFLICTING** while it sat overnight. Resolved and pushed; it is **MERGEABLE** again. The resolution turned up a **duplicate `E74`** that git had merged without a conflict, and the mac row is renumbered **E77**. Then re-walked the mac/any queue in file order and found nothing takeable — with the reason measured rather than inherited. No product code changed.

### A CONFLICTING PR is not "green and waiting for merge" — third time in five days

STEP 1.5 lists failing checks, requested changes and unresolved comments. #565 had **none** of the three — all 15 checks green, zero reviews, zero comments — and could not merge anyway: `mergeable: CONFLICTING`, `mergeStateStatus: DIRTY`. Under STEP 1.5's literal words *"green with nothing unresolved… leave it alone"* this PR reads as Jeff's problem, and it was not.

The 2026-08-28 macos entry first wrote that *"a conflict is not on STEP 1.5's list of three, and should be"*; the linux box hit it on 2026-08-31 with #550 and it was that run's entire first half. This is the third occurrence and the second on this box. **`mergeStateStatus` is one extra field on a `gh pr view` STEP 1.5 already makes you run** — that is the whole fix, and the rule text still does not name it.

### The resolutions, by ownership rather than by side

Three files conflicted; a fourth was the problem precisely because it did not.

| file | resolution |
|---|---|
| `JOURNAL.md` | main's copy **whole**, then this branch's one unique entry re-placed in run order |
| `JOURNAL-2026-08.md` | main's copy **verbatim** — a strict superset (416 entries vs the branch's 414) |
| `docs/lessons.md` | **regenerated**, not merged — `extract-lessons.py --write` |
| `BACKLOG.md` | **no conflict reported, and that was the defect** — see below |

The journal resolution is the linux box's recipe from 2026-08-31 and it held exactly. The check that makes it safe is set arithmetic, not reading: of the branch's 420 entries, **exactly one** — its own E73 entry — was absent from *both* of main's two files. Main had rotated two further entries out while #565 sat, so taking main whole is what picks that up. Re-placed above `macos — E19's mac AU3 cell` and below the three linux entries, which is run order: #565 opened 01:00Z, the linux entries landed 04:42–05:06Z.

`docs/lessons.md` came back as main's file plus E73's six-bullet block, 129,135 → 129,719 bytes. A generated file is a function of the other resolutions, so it is never hand-merged.

### The part worth carrying: a clean merge produced two different E74s

`BACKLOG.md` **auto-merged with no conflict** and left **two rows numbered E74** at lines 114 and 116:

- **mac's** (this branch) — E59's refusal not firing in a hosted AUv3: `startup default is 17959 bytes (syncState will not publish this document)` followed immediately by `syncState exporting 17959 byte document`.
- **linux's** (on `main` via [#566](https://github.com/JeffMcClintock/TideSynth/pull/566)) — a hosted VST3 recreating the DSP instance while the editor's rack feedback pins stay bound to the retired one.

Different findings, different platforms, filed hours apart from branches cut off the same `main`, landed at different points in the file. **This is the C15/C16 shape A23 exists for, and it is the second occurrence in two days** — E72 was renumbered from E70 on 2026-08-31 for the identical reason, 33 minutes apart that time.

**The mac row moved, to E77.** The tiebreak is not recency: main's E74 has **landed** and is cited by the `linux` NEXT cell, the E19 row and two journal entries, while this row existed only on an unmerged branch. Renumber the one with the fewest references; never rewrite what has landed — the same rule E72 recorded as *"archiving never rewrites a row"*. The E73 journal entry still calls it E74 and is **left as written**, because the journal is the record; E77's row carries the bridging note.

**What did not catch it, and what did.** `check-id-refs.py` is A23's duplicate check and it passes on the *merged* file — I found this by grepping the row ids by hand after the merge, not from a lint. The STEP 3 rule that would have prevented it is the pre-filing grep against freshly-fetched `origin/main`, and it could not have: **the mac E74 was filed at ~01:00Z and the linux E74 at ~04:58Z**, so at filing time neither existed for the other to find. Two runs on two boxes filing into the same numeric namespace within four hours is a race no per-run check closes.

### Verification

| check | result |
|---|---|
| TIDE, every target, merged tree | **599/599, 0 errors** — standalone, VST3, AU, AUv3 appex, AU3 app, CLAP |
| `check-id-refs` / `check-next-block` / `check-backlog-archived` / `check-links` | rc=0 |
| `check-journal-prepend` | `1 new entry prepended`, prepend-only OK |
| `check-backlog-diff` | `E73: TODO -> IN-REVIEW`, `1 new row(s): E77`, status/date cells and new rows only |
| `check-prompt-provenance` | OK (the E73 entry is interactive, exempt) |
| `check-commit-authorship --repo .` | 4 commits, all `tide-rack-bot`, rc=0 |
| PR state after push | `MERGEABLE` |

Fresh Release/arm64 Ninja, `SE_LOCAL_BUILD=OFF`, `TIDE_VCV_FUNDAMENTAL=OFF` — the shipped configuration, and OFF is also what stops POST_BUILD replacing the developer's installed plug-ins. **No SynthEditCL build and none is owed:** this branch touches `SynthEditSem/` only, which is ALLOWED and is not shared with SynthEdit; the rule is discharged by scope, not by a build. The 599 does incidentally say that `SynthEditLib` at `dcdfa6b` and `GMPI_Wrappers` at `017bb22` — both fast-forwarded from `origin/main` this run — compile into TIDE on macOS, which nobody had measured.

### The queue below STEP 1.5, walked in file order

**The screen is LOCKED** — `ioreg -n Root -d1 -a` reports `CGSSessionScreenIsLocked true`, which is the cheap check the 2026-08-31 entry landed. That single fact rules out four of the ten TODO mac/any rows, so it is worth stating before the walk rather than after.

**S8** GATED (`SynthEditLib/UgDatabase.cpp` + its CMake gating, which its own row says needs a ruling it does not ask for). **E19**'s mac AU3 cell wants a human at an unlocked screen. **E7** turns on Jeff's unruled *"where do the jacks live"*, which STEP 2 forbids working under. **E2** is not takeable by its own row. **E72** GATED (`SynthEditLib/EditorLib/`, and not a build break, so STEP 5's exception does not reach it) and wants a ruling. **E77** — this run's own renumbering — needs a prepared rack restored into a **hosted AUv3**, i.e. a GUI host. **E71** needs an AUv3 host for the same reason. **E74** (linux's) needs a hosted VST3 with the editor on screen. **E75** needs a rack *rendered* to prove two modules are visible on the default view, and its own row says it *"may be two questions"* with the prior one unanswered. **E76** is linux-specific despite its `any` platform, and its Accept turns on a ruling about whether a measurement script may edit its caller's environment.

**One IN-REVIEW row, and no flip is owed:** E73's own, whose PR is #565 and is still open.

**Learned:**

- **`mergeStateStatus` belongs in STEP 1.5's list and still is not in it.** Three occurrences in five days across two boxes, each one the entire first half of a run. The three conditions STEP 1.5 names are all things a *reviewer* did; a conflict is a thing that happened *to* the PR while it waited, and nothing in the step looks for it.
- **A clean merge is not evidence of a clean result, and id collisions are exactly where that bites.** `BACKLOG.md` reported no conflict and produced two E74s. Git merged correctly — the rows are in different hunks — and the file is wrong. Grep the ids after a bookkeeping merge; the lint passes on the merged file.
- **Two boxes can file the same id four hours apart and no per-run check can prevent it.** STEP 3's pre-filing grep is against `origin/main` at filing time, and at filing time the other row did not exist. This is now twice in two days, so it is a property of a three-box fleet with one numeric namespace, not bad luck.
- **When two rows collide, renumber by reference count, not by filing time.** The one that has landed is cited by NEXT cells, other rows and journal entries; the unmerged one is cited by its own branch. E72 reached the same answer via *"archiving never rewrites a row"*, which is the same rule wearing a different hat.
- **Read the exit code of the thing you ran, not of the pipeline that reported on it.** My build task came back `failed, exit code 1` — the build printed `BUILD_RC=0` at `[599/599]` and the 1 was a trailing `grep -c 'error:'` finding zero matches. The repo already has this lesson as *"check a lint by its exit code, not by the tail of its output"*; this is its mirror image and it cost a second look.
- **A locked screen is a queue fact, not a footnote.** Four of ten TODO rows here are unreachable for one reason, and putting `CGSSessionScreenIsLocked` at the top of the walk makes the blocked-queue finding one line instead of four paragraphs.

**Not verified:** anything about E73's own behaviour — its measurements are the 2026-08-31 interactive entry's and this run re-measured none of them; that the merged branch still produces a trace log, since `TIDE_TRACE_LOG` is OFF in the shipped configuration I built and switching it ON was not this run's question; linux and Windows builds of the merged branch; whether E77 reproduces anywhere, which needs the hosted AUv3 restore nobody has run.

**Machine state.** All six repos were clean and on their default branches at the start (`SE16` is not on this box). `SynthEditLib` (2 commits) and `GMPI_Wrappers` (1) were fast-forwarded to `origin/main`; **neither was committed to**, and no sibling repo was. TideSynth is on `tide/mac/E73-trace-to-file` until STEP 5 returns it to `main`. **The developer's installed plug-ins were never touched** — every build ran `SE_LOCAL_BUILD=OFF`, and `~/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3` and `CLAP/TIDE-Rack.clap` are unchanged. No AUv3 was registered, installed or displaced by this run; no REAPER, standalone or appex was launched, and none is running. `build-e73merge/` is a gitignored scratch build tree. The screen was locked throughout and no GUI was attempted.

**Next:** **#565 is mergeable and wants Jeff** — it is green, and the conflict that held it is resolved. **The mac lane is genuinely blocked for an unattended run**, and the binding constraint is one thing rather than nine: **E71, E77, E19's mac cell and E75 all want a single unlocked interactive session with a GUI host**, which is now the fourth row waiting on it and the strongest argument yet for scheduling one. **E72 wants a ruling, not a session.** And **A23 wants a fleet-level answer to id allocation** — twice in two days is a pattern, and per-run greps cannot close it.

**Branch/PR:** `tide/mac/E73-trace-to-file`, [#565](https://github.com/JeffMcClintock/TideSynth/pull/565) — the merge commit, the E74→E77 renumbering and its bridging note, the refreshed `mac` NEXT cell, and this entry.

## 2026-08-31 — linux — X1 closed by Jeff's ruling: the blocker was never written down (state update, interactive)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths) · interactive continuation, Jeff directing (*"what is x1 about?"*, then *"mark it DONE"*)

**Did:** **X1 → DONE and archived.** Bookkeeping only; no code, no measurement this entry is claiming credit for.

### The row, in full, from the repo's first commit

```
| X1 | BLOCKED | linux | VST3 + CLAP on Linux, GCC 13+. See the Linux toolchain memory for WSL specifics. |
```

That is `b2b1466`, *"Scaffold TIDE Synth coordination repo"*, and it is the entire original specification. It carried a **bare `BLOCKED` with no `(id)`**, under **After the carve-out** — so the implicit blocker was **C7**, C7 went DONE, and nothing anywhere said so.

### Why it took a human, and both refusals were correct

Its Accept has been met since at least 2026-08-27, measured rather than assumed on both occasions: GCC 13.3.0, 492/492 rc=0 then; **553/553, 0 errors** today on a fresh `TIDE_VCV_FUNDAMENTAL=ON` tree, producing both artifacts — **and both were driven, not merely linked.** The VST3 was hosted in REAPER 7.43 under headless weston with the transport rolling 75 s ([#566](https://github.com/JeffMcClintock/TideSynth/pull/566)); the CLAP went through `clap_plugin_state` load/save via `tests/e60_clap_state_probe.cpp` ([#550](https://github.com/JeffMcClintock/TideSynth/pull/550)).

Three linux runs in a row noticed and none flipped it. STEP 2: *"NEVER start a BLOCKED item, even if you think the blocker is stale... say so in the journal and stop."* The 2026-08-27 run added a second reason of its own — it was claiming X2, and *"a status change on a row it did not take is exactly the kind of drive-by edit that makes a queue untrustworthy."* Both are the rules working, and together they made the deadlock structural: **the only actor permitted to break it was Jeff.**

**Learned:**

- **A bare `BLOCKED` is unfalsifiable by construction, and the queue has no way to notice.** `BLOCKED(<id>)` can be re-checked by any run in one command; `BLOCKED` can only be re-checked by the person who wrote it, and after a while not even by them. Prefer the parameterised form, and a row whose blocker cannot be named probably wants `NEEDS-JEFF` — which at least says *who* is owed.
- **Two individually correct rules can compose into a deadlock that neither one describes.** "Never start a BLOCKED row" and "never edit a row you did not take" are both right and both worth keeping; their intersection is a row no agent may ever touch. Worth knowing that the fleet can manufacture these, because nothing in the process detects one.
- **Ask what the row is FOR before proposing a status.** The answer here was one line from the repo's first commit, and reading it is what turned "the blocker looks stale" into "the blocker was never written down" — a different claim, and the one that got a ruling.

**Not verified:** nothing new — this entry measures nothing. The build and host evidence it cites belongs to the two entries below it.

**Machine state.** All six repos on their default branches, clean; nothing running.

**Next:** **E74** remains the top of the linux lane, and **the linux CLAP cell of E19 is newly measurable** now the 32 KB cliff is off `main`.

**Branch/PR:** `tide/linux/X1-done` — the flip, its archive row, the `linux` NEXT cell, and this entry.

## 2026-08-31 — linux — the merges, and E60's fix measured after it had already landed (interactive continuation, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths) · interactive continuation of the scheduled run below, Jeff directing (*"resolve conflicts and merge"*)

**Did:** merged this box's three open PRs, resolved the one conflict the entry below predicted, and **flipped E60 to DONE**. Scope was deliberately my own PRs: [#565](https://github.com/JeffMcClintock/TideSynth/pull/565) is the mac box's E73 work and was left alone.

### Both E60 PRs had already auto-merged, within a minute of becoming eligible

[#550](https://github.com/JeffMcClintock/TideSynth/pull/550) merged at **04:42:02Z** and [GMPI_Wrappers#32](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/32) at **04:41:43Z** — both while I was still building #32 to check it. #550 had been CONFLICTING for three days; resolving it made it eligible and the docs-only allowlist took it, and #32 went with it.

**So nothing gated #32 on a build, and that is worth saying rather than presenting what follows as if it were a gate.** It is a product-code change in a repo with no CI. The measurement is post-hoc, and it passes.

`tests/e60_clap_state_probe.cpp` — which #550 itself had just landed — against two CLAP binaries differing by exactly #32, same commit of everything else:

| build | 51,690-byte `e53-vcv-rack-segv.xml` | 18,893-byte preset |
|---|---|---|
| `main` without #32 | **FAIL** — `load` false, **32,512 of 51,690** consumed, save falls back to the 86-byte default | PASS, 18,662 back |
| with #32 | **PASS** — 51,690 of 51,690, saves **51,630** | — |

**32,512 is the old `maxSize - chunkSize - 1` cliff to the byte.** The small preset passing on the *same pre-fix binary* is the positive control that stops the FAIL reading as a broken probe. The BEFORE binary was free: it was the copy installed into the scratch `HOME` an hour earlier, before the rebuild.

**Consumers:** TIDE **553/553** then **36/36**, 0 errors on linux. SynthEdit consumes only `se_gmpi/vst3` from GMPI_Wrappers and this change is confined to `wrapper/CLAP/`, so the SynthEditCL rule is discharged by scope, not by a build.

### The predicted conflict, and a near-miss resolving it

#566 went CONFLICTING the moment #550 landed, on exactly the one line the entry below said it would — the `linux` NEXT cell — plus `docs/lessons.md`, which is generated and was regenerated rather than merged.

**The near-miss is the part worth writing down.** My first archive attempt put a markdown TABLE inside E60's row, i.e. newlines inside a table cell, and `check-backlog-diff.py` correctly refused: a row that is no longer one line cannot be matched verbatim against its source. Reaching for `git checkout ORIG_HEAD -- BACKLOG.md` to start over then **silently reverted #550's own E60 row**, because ORIG_HEAD is the pre-merge branch tip and that row only exists on main. Caught by grepping for the row rather than by any lint. `git checkout --merge -- <file>` re-creates the conflict markers and is the right way back — and note it writes `<<<<<<< ours` / `>>>>>>> theirs`, not `HEAD` / `origin/main`, so a resolver script that pattern-matches the marker text silently matches nothing.

**Learned:**

- **A PR you resolved may merge before you finish checking it.** Auto-merge fires on eligibility, not on your intent, and a docs-only allowlist can pull a sibling repo's code PR along in the same minute. If a build is meant to gate a merge, it has to happen before the resolution, not after.
- **Say "post-hoc" out loud when verification arrives after the merge.** The numbers are just as true and mean something different; a row that presents them as a gate is lying about its own process.
- **Keep the superseded binary — it is the A/B for free.** The pre-fix CLAP was sitting in a scratch install directory from an earlier step, so the control cost one command instead of a second build tree.
- **A markdown table cannot go inside a table cell, and the archive lint is what catches it.** The row stops being one line and no longer matches its source verbatim, which is exactly the property the lint exists to protect.
- **`git checkout <ref> -- <file>` during a merge is not "undo".** It resolves the path to that ref's content, discarding the *other* side's changes outside the conflict hunk — here, another PR's row. `git checkout --merge -- <file>` is the undo.
- **Conflict marker text depends on how the conflict was produced.** `--merge` writes `ours`/`theirs` where the original merge wrote `HEAD`/`origin/main`; my resolver script matched neither and raised `NoneType has no attribute 'group'` rather than doing something wrong quietly, which is the only reason this is a footnote.

**Not verified:** #32 in a real CLAP host — the probe is deliberately the C ABI with no DAW, and the linux CLAP cell of E19 is now measurable and unmeasured; whether #32's larger loads behave on Windows or macOS.

**Machine state.** `GMPI_Wrappers` was briefly on a `verify-32` branch for the A/B build and is back on `main`, fast-forwarded, clean; the branch is deleted. All six repos on their default branches, clean. Nothing running. `build-e19/` is gitignored and now carries #32.

**Next:** **E74** is still the top of the linux lane. **The linux CLAP cell of E19 is newly measurable** now that the cliff is gone, and the harness in [tests/e19-host-feedback/](tests/e19-host-feedback/) mints its own project. **X1 still wants Jeff** — its `BLOCKED` mark has been stale since 2026-08-27 and no run may start it.

**Branch/PR:** `tide/linux/E19-vst3-linux-cell`, [#566](https://github.com/JeffMcClintock/TideSynth/pull/566) — the merge commit, E60's flip to DONE, the refreshed `linux` NEXT cell, and this entry.

## 2026-08-31 — linux — STEP 1.5 unblocked #550, then E19's linux VST3 cell: the DSP now runs the right rack, and the editor is bound to the wrong processor (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude Code **2.1.220** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required

**Did:** two things, in the order the prompt puts them. **STEP 1.5:** this platform's only open PR, [#550](https://github.com/JeffMcClintock/TideSynth/pull/550), had gone **CONFLICTING** while it sat for three days; resolved and pushed, it is `MERGEABLE` again. **STEP 2:** took **E19**'s linux VST3 cell, whose own text said *"do not re-take this cell until E59 closes"* and E59 closed on 2026-08-29. Branch `tide/linux/E19-vst3-linux-cell`. **Cell back to TODO; E74, E75 and E76 filed.** No product code changed on either branch.

### STEP 1.5 first, because a conflicted PR is not "waiting for merge"

STEP 1.5 lists failing checks, requested changes and unresolved comments. #550 had none of those — all 13 checks green, no reviews — and could not merge anyway. The 2026-08-28 macos entry already recorded that *"a conflict is not on STEP 1.5's list of three, and should be"*; this is the second time it has been the whole first half of a run.

All three conflicts were in the fleet's bookkeeping files, and the resolution is by date and ownership rather than by side:

- **`JOURNAL.md` — main's copy verbatim.** Main rotated every 2026-08-28 entry into the archive while #550 sat open, so the branch's own E60 entry was the only thing missing. It moved into `JOURNAL-2026-08.md` between the two 08-28 windows entries it sat between on the branch. Checked rather than assumed: of the branch's 35 entries, **exactly one** was absent from both main's `JOURNAL.md` and the archive.
- **`docs/lessons.md` — regenerated**, not hand-merged. `scripts/extract-lessons.py` reads both journal files, so the correct content is a function of the other two resolutions.
- **`BACKLOG.md`** — mac NEXT cell from main (2026-08-31, three days newer), linux NEXT cell from the branch (same day, "later"); E59 stays archived as main has it; E60 takes the branch's IN-REVIEW row.

`GMPI_Wrappers`[#32](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/32), E60's other half, is `MERGEABLE`/`CLEAN` with nothing unresolved, so STEP 1.5 says leave it — **and it still matters**: `Processor_CLAP::stateLoad` on `main` still has the `maxSize = 4096 * 8` cliff, unchanged by #36 and #37, which touched the same function.

### Then E19, and the first half of the result is that two of this row's claims are now false

**The DSP runs the PREPARED rack.** `TIDE: instance #3 building rack from 43191 byte document`, twice, and the 17,957-byte default never appears after the restore. On 2026-08-28 the same path gave `29101` then `17957`, which is the observation E59 was filed for.

**And `tests/hosts/v1-rack.rpp` makes sound on Linux for the first time.** PLAN cites that fixture for *the patch plays after reload*; it was **peak −inf, digital silence** on this box on 2026-08-28.

| fixture | linux, 2026-08-31 |
|---|---|
| `--control` (no plug-in at all) | −6.0 / −9.0 dBFS — the chain detects audio |
| `v1-rack.rpp` | **−6.3 / −17.0 dBFS**, 2 patch cables — the macOS reference to the decimal |
| `v1-rack-uncabled.rpp` | **−inf**, 0 patch cables — the negative control |
| `v3-midi-pitch.rpp` | −6.2 / −21.1 dBFS |
| `v3-midi-gate.rpp` | −6.3 / −21.2 dBFS |

### The FAIL that is left, and its mechanism is ordering rather than any single line

75 s, transport rolling the whole time (`playstate=1`, position 0 → 74.919 — the control that separates "the plug-in is frozen" from "nothing is being processed").

| | hosted VST3, REAPER 7.43 | STANDALONE (control) |
|---|---|---|
| `Scope display-state capture` | **#2100**, 65,548 bytes, still climbing | #1800 |
| `display-state update … arrived` | **frozen at #1, 0 bytes** | **#1820, 65,548 bytes** |
| `light … update` | **frozen at #2, value 0.000** | **#18100**, values varying |

Same build, same document, same box, same compositor. The log order says why:

```
TIDE: instance #3 building rack from 43191 byte document
RackProcessor: 'Scope' display-state capture #0 (65548 bytes)
RackEditor: light 0 update #0 value 0.000          <- the editors' initial defaults
RackEditor: display-state update #1 arrived (0 bytes)
TIDE: instance #4 building rack from 43191 byte document    <- a SECOND processor
RackProcessor: 'Scope' display-state capture #0 … #2100      <- and it runs alone
                                                   (no RackEditor line ever again)
```

**The standalone builds twice as well** — `Legacy chunk`, then `Build chunk, rack already prepared` — **but as `instance #1` both times.** So a double build is not the defect; the changing instance is, and the editor's feedback pins are left attached to a processor the host has retired. Filed as **E74**. It is not E59 (the document is right, both times) and not the ui→dsp direction E64 fixed.

### The number I will not let anyone quote, and the control that disarmed it

The hosted pixel diff is **0 of 690,800**, which is exactly what this row's Accept calls a FAIL. **It is not evidence, because the standalone control over the same interval gives byte-identical screenshots** — while its counters are at #18100.

The reason is the fixture. All five VCV editors construct with panel art in **both** arms (`RackEditor: 'Scope' model=yes art=yes(res/Scope.svg) art-size=195x380`), and none of them is on the visible rack page; vertical and horizontal scrolling did not reach them. The negative control that makes this the fixture's layout rather than a rendering fault: the **DEFAULT** rack in the same build draws its `Out` panel on the rails. So E19's pixel-diff and int/bool/enum clauses are unmeasured and want a fixture with a visible Scope — **E75**.

### Two traps that each cost a wrong provisional conclusion

**`render-and-measure.py` segfaults REAPER on Linux from a scheduled run's shell** — rc **−11**, the documented inherited-`WAYLAND_DISPLAY` crash — and the downstream symptom is an `EOFError` in Python's `wave` module on a zero-length render. I read that as the committed fixture's macOS token being rejected, wrote it down, and it was wrong: with `env -u WAYLAND_DISPLAY … GDK_BACKEND=x11` the same file renders −6.3/−17.0. **E29's divergence is real for what a host WRITES and did not stop a fixture being READ here.** Filed as **E76**.

**The standalone's config folder is `TiDE Rack`, lower-case `i`**, and `tests/fixtures/e53-vcv-rack-segv.README.md` said `TIDE Rack`. Following it loads the DEFAULT rack and says nothing — measured as `building rack from 17961 byte document` against `38658` once the file moved one directory. Corrected at its source.

### The harness is in the tree this time

[tests/e19-host-feedback/](tests/e19-host-feedback/) — the 2026-08-28 run built the REAPER-on-weston recipe and left its drivers in a session scratch that did not survive, which is this repo's own lesson arriving for the second time. The piece worth having beyond E19 is `frame_chunk.py`: the `vst_chunk` framing **measured off a default instance** (140 base64 chars, 105 bytes, `int32 len+4 | int32 1 | int32 len | XML | 8 zero bytes` — no 44-byte header, no trailer), and a mint route that adds the plug-in **by name** and then sets the parm, so REAPER writes its own token. It wrote `1013510754{506C7567696E474D50492050A2A07287}` unprompted, and **E29 cannot be got wrong by construction** that way.

**Build:** `TIDE_VCV_FUNDAMENTAL=ON`, `-DRACK_ADAPTOR_TRACE=1`, Release, `SE_LOCAL_BUILD=OFF` — **553/553, 0 errors**, all four artifacts, against `main` in all five sibling repos. Verified to contain what this run needed before believing any of it: `display-state update #` and E59's `declined to publish the startup default` are each present once in the standalone, the VST3 `.so` and the CLAP.

**Learned:**

- **A CONFLICTING PR is not "green and waiting for merge", and STEP 1.5's list of three does not name it.** Second run in four days where that was the entire first half. `mergeStateStatus` costs one field on a `gh pr view` that STEP 1.5 already makes you run.
- **Resolve a rotated `JOURNAL.md` by taking main whole and re-placing your own entry in the archive.** The merge conflict looks like a text problem and is a bookkeeping one; the check that makes it safe is set arithmetic — which of the branch's entries are in neither of main's two files — and it printed exactly one.
- **A generated file is not merged, it is regenerated.** `docs/lessons.md` conflicted in two places and `extract-lessons.py --write` settled both, because its content is a function of the files the other resolutions produced.
- **A frozen readout and an unattached listener look identical, and only the ORDER of the log separates them.** Every counter in this run was correct about what it could see. The finding is in which line comes after which, and no single line carries it.
- **When a control gives the same "failing" number as the experiment, the number is not about the experiment.** A 0-pixel diff was E19's own FAIL condition; the standalone's byte-identical screenshots turned it into a statement about the fixture. Run the control even when — especially when — the result already looks like the answer you expected.
- **A crash can present as a corrupt output file two layers away.** REAPER's rc −11 reached me as `EOFError` inside `wave.py`, and I had already written down "the token is rejected on linux" before reading the render log. The log was two lines from the exception.
- **Read a verb's usage before reporting that it ignores its arguments.** `--scroll 500,300 0,-5` reported `delta 120, horiz false` three times and I was one sentence from filing a harness gap; the real syntax is `--scroll <x,y> [--notches N] [--delta N] [--horiz]` and it works.
- **A folder name that differs by one letter's case fails silently and looks like a broken fixture.** `TIDE Rack` vs `TiDE Rack`: the app loads its default, logs nothing unusual, and the fixture sits one directory away. The document's own `standalonePlugin` attribute is still the OTHER spelling, and both are correct in their own place.
- **A NEXT cell has to live on the branch its targets live on, and two lints enforce that.** The `linux` cell also belongs to #550's diff, so I tried to update it there — and `check-next-block.py` and `check-id-refs.py` both refused, because E74/E75/E76 and `tests/e19-host-feedback/` exist only on #566. They were right, and it settles the question the 2026-08-31 macos entry raised as a judgement call: the cell goes where its targets are, the conflict is one line, and the cell says in its own text which side to take.
- **Two of these lints passed on a NEXT table I had just destroyed.** My first edit ate the `linux` row's Take column entirely; `check-next-block.py` and `check-backlog-diff.py` both said OK, and the only tell was the row COUNT dropping from 4 to 3 in the lint's own summary line. Read the count, not the verdict.

**Not verified:** the linux **CLAP** cell — E60 owns it and its fix is [GMPI_Wrappers#32](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/32), unmerged, so `main` still carries the 32 KB `stateLoad` cliff; whether **E74** reproduces on Windows or macOS (nothing about a processor recreation is platform-specific, but neither box has read these counters in a host); the E74 **fix**, entirely — the row names where to look and does not guess; E19's **int/bool/enum**, **pixel-diff** and **string** clauses, all three for reasons recorded above; whether the e53 fixture's modules are reachable by any view gesture at all, which is half of E75.

**Machine state.** All six repos were clean and on their default branches at the start; the five siblings were fast-forwarded to `origin/main` (GMPI 1 commit, gmpi_ui 1, GMPI_Wrappers 6, SynthEditLib 9, SE16 2) and **none was committed to**. TideSynth is on this run's branch until STEP 5 returns it. **REAPER 7.43 was downloaded fresh** — the 2026-08-28 copy lived in that session's scratch and is gone — and ran only against a scratch `HOME`, so `~/.vst3`, `~/.clap` and `~/.config/REAPER` were never written; `~/.config/REAPER` still does not exist, and `~/.vst3` and `~/.clap` compare identical to the pre-run listing, TIDE absent from both. The standalone ran under a scratch `XDG_CONFIG_HOME`; `~/.config/TiDE Rack/` is untouched. `build-e19/` is a scratch build tree and is gitignored; Jeff's `build/` was not touched. `decode_rpp.py` wrote `tests/hosts/v1-rack.rpp.block0.param1.xml` and `v1-rack-uncabled.rpp.block0.param1.xml` as side effects; both removed. Headless weston, REAPER and the standalone all stopped via `scripts/kill-named.sh` — 0 of each left running.

**Next:** **E74 is the whole of E19's linux VST3 cell now**, and its harness needs no authoring — it mints its own project. **E75 is cheap and unblocks two more of E19's clauses.** **#550 and GMPI_Wrappers#32 want Jeff's merge**; until #32 lands the linux CLAP cell cannot be measured at all. And **the same 553-target build is sitting in `build-e19/`**, so any further linux measurement is minutes rather than an hour.

**Branch/PR:** `tide/linux/E19-vst3-linux-cell`, [#566](https://github.com/JeffMcClintock/TideSynth/pull/566) — the harness in [tests/e19-host-feedback/](tests/e19-host-feedback/), the E19 row, E74/E75/E76, the linux sections of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), the audio table in [tests/hosts/README.md](tests/hosts/README.md), the folder-name correction in [tests/fixtures/e53-vcv-rack-segv.README.md](tests/fixtures/e53-vcv-rack-segv.README.md), and this entry. Plus the merge commit on `tide/linux/E60-clap-state-trace` ([#550](https://github.com/JeffMcClintock/TideSynth/pull/550)), which is the STEP 1.5 half.

## 2026-08-31 — macos — E73: a hosted plug-in gets its trace back, and one freopen beats sixteen edits it was not allowed to make (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** · as **tide-rack-bot** (both paths) · interactive, Jeff directing (*"do E73. i allowed the dialog."*)

**Did:** fixed **E73**, the blocker E19's mac AU3 cell hit hours earlier — **and not the way E73's own row proposed**. Branch `tide/mac/E73-trace-to-file`. The first log it produced found **E74**.

### The row said sixteen edits; the rules said none of them were mine

E73 proposed converting each `fprintf(stderr, …)` into a file write, the way E65 did for the panel. Following that would have stopped immediately: **sixteen of the sites are in `SynthEdit_Rack_Adaptor`, which is on NEITHER of STEP 5's lists and is therefore GATED by default**, and more are in `SynthEditLib`, GATED outright. The allowed-side part would have been a fraction of the instrument.

**One `freopen` on the process's stderr, from TIDE's own ALLOWED code, is both legal and better.** It captures every writer in the process — the rack adaptor's, SynthEditLib's, TIDE's, and sites nobody has written yet — and it edits no gated repo.

The deeper reason it is better: **the defect was never in those calls.** They are correct. The process they run in has no stderr worth writing to. Fixing the stream fixes the class; fixing the calls fixes a list.

### Where the file goes, which is the part that took thinking

`$TIDE_TRACE_LOG_PATH` if set, else `$TMPDIR/TideTrace.log`.

The default is the one that matters. **An AUv3 appex is sandboxed, so its `TMPDIR` is its own container** — `~/Library/Containers/<extension-bundle-id>/Data/tmp` — **writable from inside and readable from outside.** That is what makes the log collectable at all: a host launches an extension *through the system*, so environment variables set for the host never reach it, and no harness can point the extension anywhere. The env var is still honoured because it works for the standalone and the VST3, where it does propagate.

Armed at compile time (`-DTIDE_TRACE_LOG=ON`, OFF by default), for E65's reason plus one more: an environment variable cannot arm it in the one configuration that needs it most.

### Measured, in two halves because one would not have been enough

**The hosted half.** REAPER 7.45 hosting the AUv3 wrote a **19-line `TideTrace.log`** into the container, while `grep -icE 'TIDE:|RackProcessor'` on **REAPER's own stderr stayed at 0**. The pair is the proof — the same lines that reached nobody this morning are now in a file, and they are still not on the host's stderr.

**The cross-repo half**, which is the one that justifies the whole design choice. A standalone run with an explicit path captured:

```
Logging dialogs to stderr, and keeping them for --dialogs.
```

That line is written by **`SynthEditLib/EditorLib/SynthEditAppBase.h`** — a GATED repo this change does not touch. So the capture is demonstrably stream-level rather than TideSynth-specific, which is exactly the property that made `freopen` the right call. Also measured: **0** lines escaped to the process's own stdout or stderr.

**Not observed, and worth saying plainly rather than implying:** a literal `RackProcessor: … display-state capture #N` line. Neither run instantiated a VCV module — the standalone's command channel has **no add-module verb** (its verbs are pointer, menu, midi, param, screenshot; placement is a mouse drag through the browser), and the hosted AUv3 had the default rack. The adaptor writes with the same `fprintf(stderr, …)` as the SynthEditLib line that *was* captured, so it rides the same mechanism — but nobody has watched one yet, and that is E19's prepared-rack problem, not this row's.

### The first log it produced found a defect — filed as E74

Two adjacent lines from the container, a fresh hosted instance:

```
TIDE: controller #1 startup default is 17959 bytes (syncState will not publish this document)
TIDE: controller #1 syncState exporting  17959 byte document (host asked for state)
```

**Same size, and it published anyway.** E59's guard is byte equality against the recorded startup default, so the bytes differ by something the length does not show. E59's own comment predicts this direction — *"if the two ever differ spuriously this publishes"* — and calls it the cheap way to be wrong.

It was harmless **here**, and the reason is worth stating so nobody over-reads it: nothing was restored, and a fresh instance *should* build the default rack. But the published bytes are retained by the processor holder and re-seeded into the next processor it starts, which is the whole of E59, and the log shows that step happening (`instance #2 building rack from 17959 byte document (Sync chunk, rack not yet prepared)`). **With a restore in the picture, that is E59's failure** — and a restore into a hosted AUv3 is precisely what nobody has run on this platform.

**Learned:**

- **When a row prescribes an edit you are not allowed to make, the constraint is a design hint, not an obstacle.** Being unable to touch the rack adaptor forced the question *"whose stderr is this?"*, and the answer was a better fix than the one the row asked for — one file, no gated repos, and it covers writers that do not exist yet.
- **Fix the stream, not the call sites, when the call sites are all correct.** Sixteen `fprintf`s were not the defect; the process's stderr going nowhere was.
- **A sandbox container is a FEATURE for a harness, once you notice it is readable from outside.** The appex cannot be handed an environment, so a configurable path is useless there — but its own `TMPDIR` is a fixed, discoverable location, and defaulting to it is what makes the log collectable.
- **`freopen` that fails CLOSES the stream.** A bad path would not merely fail to help, it would destroy the stderr the standalone still depends on. Probe with `fopen` first and only redirect when that succeeds — one extra call, and the failure mode it removes is silent.
- **Prove a stream-level capture with a line you do not own.** "It must catch everything, because it is the same stream" is an argument; a line written by a GATED repo appearing in the file is a measurement, and it was free.
- **Take the diagnostic build back off the machine.** A build with the redirect armed writes a file on every instantiation — PLAN constraint 4 — so the installed AUv3 was returned to a normal build and checked with `strings`.

**Not verified:** a `RackProcessor:` line specifically, per above; whether the log survives a host that sandboxes the extension more tightly than REAPER does (Logic and Live untried); Windows and Linux, where the code compiles by inspection only — the `_WIN32` branch of the path logic is untested on this box.

**Machine state.** `~/Applications/TIDE-Rack-AUv3.app` is a **normal** build again — the trace one was installed only for the measurement and removed; `strings … 'trace log opened'` on the installed appex is **0**, and the container tmp was emptied. The AUv3 remains registered (UUID `2F335B9F…`). The developer's REAPER config, installed VST3 and installed CLAP are untouched as before; every build ran `SE_LOCAL_BUILD=OFF`. Build trees `build-e73/` and `build-e19au3/` are gitignored; the evidence log is copied into the session scratchpad. No REAPER, appex or standalone process left running. **Two containers exist for TIDE extensions**, `…au3app.extension` and a leftover `…e19test.extension` from the 2026-08-29 clone experiment; the second is inert and was left alone.

**Next:** **E74** is a diff of two equal-length documents and E73's log is the instrument. **E19's remaining clauses** now need only the prepared-rack half, since the trace half is solved. **E72** still wants a ruling.

**Branch/PR:** `tide/mac/E73-trace-to-file` — `SynthEditSem/TraceLog.h`, the two call sites, the `TIDE_TRACE_LOG` option, the E73/E74 rows, the doc section, and this entry.

## 2026-08-31 — macos — E19's mac AU3 cell: a DAW has now hosted TIDE's AUv3, and the half that is still unmeasured has a structural cause (interactive, Jeff directing)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.40609.0** · as **tide-rack-bot** (both paths) · interactive continuation of the same session, Jeff directing (*"merge your PRs. sync all tide related repos"*, then *"then take the next task"*)

**Did:** took **E19**'s mac AU3 cell — the topmost eligible row, and the one this box alone can measure — after both of its blockers lifted in the same minute: the screen was unlocked, and Jeff was present to authorise the one step an unattended run must not take. **A DAW has now hosted TIDE's AUv3, the first time on any box.** Branch `tide/mac/E19-au3-registered`. No product code changed.

### The registration wall came down exactly where 2026-08-29 said it would

That run measured five ways to register a current build **beside** the developer's — launching the built app, `pluginkit -a`, a clone with a distinct `CFBundleIdentifier` *and* subtype, an inside-out ad-hoc re-sign, `lsregister -f` — and all five left `pluginkit -m -i <id> -v` answering `(no matches)`. It was right, and it was right to stop: displacement was the only route and an unattended run must not take it.

**A `ditto` backup taken first is what makes it safe**, and it answers that run's stated objection directly — the risk was dying mid-way and leaving his registration pointing at a build tree that later gets deleted; a 5 MB copy makes that one command to undo.

| | before | after |
|---|---|---|
| `pluginkit -mv` UUID | `DBE224FD…` | **`793D00A0…`** |
| its date | 2026-08-25 | **2026-08-31** |

Read it by UUID and date, not by presence: the stale registration is present too and differs in nothing else.

### Then Apple's validator, before any DAW

```
auval -a          ->  aumu Drck Dsyh  -  TiDE Synth:TiDE Rack
auval -v aumu Drck Dsyh   ->  AU VALIDATION SUCCEEDED   rc=0
```

**The first Apple-validated AU result this project has.** M2 and E9 both record that TIDE's AU evidence was *our own probe, never a DAW*; `auval` is neither ours nor a DAW, and it is stricter than the first and cheaper than the second.

### REAPER hosts it — and two traps cost a launch each

REAPER 7.45 scanned the registered extension into its AU cache as `TiDE Synth:TiDE Rack` (his own cache had **never** held a TIDE entry — 0 matches, checked before starting), instantiated it as **`AUi: TiDE Rack (TiDE Synth)`**, floated the editor, and rolled the transport **43 s at `playstate=1`** with the position advancing to 43.14 — so `process()` ran and nothing wedged.

- **A seeded portable config reloads the developer's last project**, whose missing plug-ins raise a modal, and the modal blocks `Scripts/__startup.lua` from ever running. The symptom is a startup script that writes **no log at all**, which reads as "my script is wrong" — I spent a launch there. `loadlastproj=0` plus an explicit empty `.rpp`.
- **The AU cache must be deleted from the PORTABLE copy** to force a rescan; seeded from his, it has no TIDE entry, so REAPER never looks.

Useful by-product: the blocking modal is where REAPER's own naming convention is printed — `AUi: <name> (<manufacturer>)`. Take the spelling from REAPER rather than guessing it.

### The screenshot settles what a symbol check could not

The floated editor **drew**, and its module browser lists `LFO`, `LFO2`, `Scope`, `SEQ3`, `SHASR`, `Quantizer`, `RandomValues` and the rest under a **`Rack-VCV Fundamental`** heading, with the five prefabs above them.

That is a picture of VCV Fundamental linked and **enumerated inside the hosted extension**. The 2026-08-29 run reached for `strings … "VCV: Scope"`, got 0, read it as "VCV did not link", and then confirmed its own error with a second bad reading — the ids are composed at runtime so the literal never appears. No symbol check could have answered this; one screenshot did.

### The wall a human does NOT remove, and it is the reason the rest is unmeasured

**An audio-unit extension runs out-of-process, so everything this project traces to `stderr` is invisible when the plug-in is hosted.** `RACK_ADAPTOR_TRACE`'s counters and TIDE's own `syncState`/`building rack from` lines are all `fprintf(stderr, …)`. Measured, not assumed: the strings are in the appex binary, the plug-in loads and runs under the host, and grepping REAPER's stderr for `TIDE:` or `RackProcessor` returns **nothing**.

So the linux box's whole instrument set is unavailable here, and E19's animation, int/bool/enum and pixel-diff clauses cannot be read on macOS AU3 however long anybody watches. **Filed as E73**, whose fix already exists one layer up: E65's `TIDE_PANEL_LOG_PATH` + `-DTIDE_PANEL_TRACE_LOG`, which routes a trace to a file and defaults into `TMPDIR` so it survives the sandbox.

### One measurement that belongs to V2, recorded in passing

REAPER sees **3** parameters on the instance: `Bypass`, `Wet`, `Delta` — all REAPER's own AU wrapper params. **None of TIDE's parameters are visible to the host**, so there is nothing for a DAW to automate today. That is V2's problem and this is a datum for it, not a new row.

**Learned:**

- **"Needs a human" is a claim with an expiry, and it expired the minute one showed up.** Two of E19's blockers were properties of an *unattended* run — a locked screen and a registration nobody may displace — not of the platform. The row had said so since 2026-08-29; what changed was availability, and a run should check that before re-inheriting a blocker.
- **Take the backup and the objection disappears with it.** The 2026-08-29 refusal was reasoned from irreversibility ("if the run died in between"). A `ditto` first converts the whole argument into a one-command undo — the blocker was recoverability, not permission.
- **`auval` before any DAW.** It is Apple's, stricter than our probes, needs no host config, and had never been run against this plug-in. A DAW failure after `auval` passes means something about the DAW; before it, you do not know what it means.
- **A no-output startup script is more often a modal than a bug.** REAPER wrote nothing at all, and the cause was a dialog about a *different* project's missing plug-ins. Screenshot before debugging the script.
- **When a symbol check is ambiguous and the thing is on screen, screenshot it.** Third time this project has been misled by `strings` on runtime-composed ids; the picture cost one command and is unarguable.
- **Out-of-process changes what an instrument IS, not just where it prints.** Every counter this fleet added for the linux box is a `stderr` write, and that design choice silently excludes the AUv3 target entirely. Worth knowing before adding the next one.

**Not verified:** E19's animation window, int/bool/enum toggle and pixel diff — blocked on E73 and on getting a PREPARED rack into a hosted AUv3, which is the same shape as E60's CLAP blocker; audio out of the hosted AU (the default rack with no MIDI is silence, so the test would have proved nothing); whether the same holds in Logic or Live, neither of which was opened.

**Machine state.** **One deliberate change to the developer's machine, and it is the point of the exercise:** `~/Applications/TIDE-Rack-AUv3.app` is now the current build (Release/arm64, `TIDE_VCV_FUNDAMENTAL=ON`, `RACK_ADAPTOR_TRACE=1`, 395/395 0 errors) and is the registered AUv3. **The 2026-08-26 app it replaced is backed up** in the session scratchpad; restoring it is `rm -rf` + `ditto` + one `open -g`. Everything else was isolated and verified afterwards: his `~/Library/Application Support/REAPER` has **0 files** modified in the last two hours across 2052, and his installed `VST3/TIDE-Rack.vst3` (Aug 28) and `CLAP/TIDE-Rack.clap` (Aug 22) are untouched — every build ran `SE_LOCAL_BUILD=OFF`. The portable REAPER, its config and all captures are in the scratchpad. No REAPER, appex or TIDE process left running by this run; a `e38_context_menu_probe.py` and a standalone TIDE belonging to Jeff's own live session were running throughout and were left alone. **A macOS permission dialog is on his screen** — *"Claude is requesting to bypass the system private window picker"*, raised by `screencapture`; I did not answer it, because system security settings are his, and screen capture worked without it.

**Next:** **E73 unblocks three of E19's clauses** and is one session. **E19's remaining mac clauses also want a prepared rack in a hosted AUv3** — worth solving once, since E60 needs the same thing for CLAP. **E72** wants a ruling, not a session. And the AUv3 is registered *now*, so any further AU3 measurement is cheap until somebody rebuilds over it.

**Branch/PR:** `tide/mac/E19-au3-registered` — the E19 row, E73, the macOS AUv3 section of [docs/ci/headless-gui-verification.md](docs/ci/headless-gui-verification.md), and this entry.

