# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

**WHY THIS SECTION IS ABOVE THE ENTRIES AND MUST STAY THERE (A36, 2026-09-10).**
It used to sit *below* the oldest entry, and on 2026-09-01 a rotation swept it
into the archive along with the entries it was standing behind — because a
rotation removes the oldest entries and the oldest entries are at the bottom.
**The instruction that says "do this every run" stopped being read every run,
and the file grew from 53 KB to 229 KB across the fourteen entries that
followed, written by runs that could no longer see this rule.** Nothing caught it: `check-journal-prepend.py` deliberately does
not gate the header block, and the rotation was legal in every other respect.
Above the first dated entry a rotation cannot reach this — which is also where
the run prompt ("the template at the top of that file"), that script's own
docstring ("the header/template block above the first real entry") and A8's own
row ("the live file holding the template + current month") have all said it was.

**One more thing to check before you rotate**, learned the same day: rotation
opens a NEW archive file whenever it crosses a month, and
`scripts/extract-lessons.py` must be able to SEE that file or every lesson you
move into it drops out of [docs/lessons.md](docs/lessons.md) silently — the
exact failure A30 exists to prevent. It discovers `JOURNAL-YYYY-MM.md` by glob
now, so this should stay true on its own; `python3 scripts/extract-lessons.py
--check` before and after a rotation is the one command that proves it did.

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

**Archives:** [JOURNAL-2026-10.md](JOURNAL-2026-10.md), [JOURNAL-2026-09.md](JOURNAL-2026-09.md), [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

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

## 2026-11-16 — windows — E19: the win/STANDALONE venue is built and the cell fails on a real defect — no standalone probe could connect on this platform at all, and now that one can, rack drags turn out to do nothing (scheduled run)

**Prompt:** 18210767f · Opus 5, `claude-opus-5` · app Claude desktop **5.11492.5** (`appVersion` in `%LOCALAPPDATA%\Claude\Logs\main.log`; no `claude` on PATH) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E19**'s **win/STANDALONE** cell — the one its Accept names for this platform (`win: VST3+GMPI+STANDALONE`) and the one no run has ever reached. Built the transport that was missing, measured six clauses, and filed the defect three of them ran into. `FLEET-PAUSED` absent. STEP 0's fetch succeeded. `run-lock.py acquire` rc=0 with the documented windows `owner=self (NO agent ancestor matched -- this lock is age-capped only)` warning; released at STEP 5.

**STEP 1:** no open `platform:win` issue — the only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, E89 DONE) and #44 (the digest).

**STEP 1.5, and this is worth copying.** [#679](https://github.com/JeffMcClintock/TideSynth/pull/679) (`tide/win/E94-bool-fixture`, the 11-10 run's) was open with a **failing `linux` check**, which STEP 1.5 ranks above all backlog work. **It was not the PR.** The `verify` workflow's engine download — `curl -fL $ENGINE_BASE/SynthEdit-linux-x64.tar.gz` — returned **HTTP 403**, and the evidence that it was transient is three-fold: the *previous* `verify` run on the same branch **34 seconds earlier** was green, the scheduled runs on `main` on 11-09 and **today 03:10Z** are both green, and the URL serves **200, 16,390,790 bytes** from this box now. `gh run rerun --failed` → green, `mergeStateStatus: CLEAN`. **A red check on this repo is worth one minute of history before it is worth a commit.** #679 is now green with no review comments, so per STEP 1.5 it is waiting for Jeff and I left it alone.

**STEP 2:** E19 was `TODO`, `any`, and the only remaining branch was my own lane's finished `tide/win/E94-bool-fixture`. The other reachable rows are parked for the reasons the 10-26 and 11-10 windows cells list (A35/A37/A42/A43/A45 on open questions or on Jeff, S8 `NEEDS-SPEC`, E2 an umbrella, E84 a `.github/workflows/**` edit this credential cannot make, X2/E93 other platforms, E7/E44/E49 `DONE-PENDING-ACCEPT`). **The row's own text and its Accept disagree about this platform**, and I followed the Accept: the row's "still not measured" list says *win/CLAP*, but the Accept's format matrix puts CLAP under `linux` and gives `win` **VST3 + GMPI + STANDALONE**. Two previous win cells have pointed at win/CLAP on the strength of that list. **win/GMPI is now the last unmeasured win cell.**

### The blocker was never the clause — nothing could connect

**Every standalone probe in the repo opens an `AF_UNIX` socket** (`e38`, `e51`, `e62`, `e65`, `e82`), and **this platform's server publishes a named pipe**: `\\.\pipe\gmpi-standalone.<pid>`, built from `GetCurrentProcessId()` (`GMPI_Wrappers/wrapper/Standalone/mcp/IpcServerWin.h`, `kPipePrefix`). The 11-10 entry found this and called the port "a small job"; it was, and this is it. That also answers **E94's own *"check first"*** question, which asked whether the Windows standalone serves the AF_UNIX channel at all. It does not.

**The one protocol difference that matters, and it is not a detail.** The unix client frames a request by half-closing its write end and reads the reply to EOF. **A named pipe has no half-close** — closing the write end closes the connection and the replies with it. What replaces it is exact rather than a heuristic: `CommandProtocol::respondTo` runs one command line and returns **one** response line unconditionally, and `LineFramer` calls it once per line, so N commands produce exactly N lines and the client counts them. No idle timer and no guess about how long a verb may take.

**Two things I got wrong first, both worth the next run's attention:**

- **Testing whether a pipe exists by opening it CONSUMES an instance.** The server hands the connection to a client thread which reads zero bytes and disconnects. The real server re-arms before it serves so it survives that, but the self-test's stand-in did not — and the failure surfaced one call later as a `FileNotFoundError` from `talk()`, nowhere near its cause. `WaitNamedPipeW(name, 0)` asks the same question without connecting, and its errors separate the cases: **`ERROR_FILE_NOT_FOUND` (2) = not published; `ERROR_SEM_TIMEOUT` (121) / `ERROR_PIPE_BUSY` (231) = published, all instances busy.** Both busy answers count as published.
- **The pipe name is CONSTRUCTED from the pid we launched, never searched.** The unix socket sits in a private `GMPI_STANDALONE_IPC_DIR`, so a glob there can only find this probe's app. **The Windows pipe namespace is global**, so the same glob would also match a standalone the developer has open, and a probe that drove the wrong app would measure the wrong thing *while passing*.

**Verification artifact for the transport itself:** [tests/standalone_channel_selftest.py](tests/standalone_channel_selftest.py) — a stand-in server, no app and no plugin, **8 arms green including 3 MUST-FAIL controls** (an absent channel raises on the deadline; a dead app ends the wait in 0.00 s rather than waiting out 30 s; a server that answers once and goes quiet times out in 2.01 s rather than hanging, which is the arm the pipe needs and the socket got for free).

### PASS, FAIL, and the control that makes the difference a finding

`main` at `f6551b4d8`, target `TIDE_Rack_STANDALONE`, Release, `-DTIDE_VCV_FUNDAMENTAL=ON -DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`, `-DCMAKE_GENERATOR_INSTANCE=".../18/Community"`, **no `*_FOLDER_OVERRIDE`**, so all deps were fetched into the build tree and no sibling checkout of Jeff's was read. Configure rc=0, build rc=0, **0 `error C`/`error LNK` lines on the first pass** (Defender's cold-tree `.obj` trap did not fire). One command, run twice end to end: [tests/e19_win_standalone_probe.py](tests/e19_win_standalone_probe.py).

| clause | run 1 | run 2 |
|---|---|---|
| 1 channel published, `--info` answers | **PASS** `\\.\pipe\gmpi-standalone.<pid>`, **1100x626** | same |
| 2 browser `--double` click places a WT LFO the DSP constructs | **PASS** `RackProcessor: 'LFO2' constructed` | same |
| 3 both counters advancing after **60 s** | **PASS** 73 `light` + **326** `render` lines after the last `building rack`, **45** distinct light values | **PASS** 73 + **325**, **44** |
| 4 **POSITIVE CONTROL** — `--drag` rotates the FREQ knob | **PASS** **101 of 1,764 px** inside the knob's own box | **101 of 1,764** |
| 5 drag the panel by its title strip → the module moves | **FAIL** no `document changed`; panel x **525.3 → 525.3** | identical |
| 6 drag the WT output jack → Out L → a cable | **FAIL** **0 of 9** points swept ±4 DIP | identical |
| geometry, derived afresh each run | panel 525.3,288.7..600.7,562.7, zoom **0.714**, WT jack **586,530** | identical |

The only numbers that move are the free-running animation counters, which is what a free-running animation should do; every value the probe asserts on is identical across the two runs.

**Clause 4 is why 5 and 6 are a finding rather than a broken probe.** Pointer events reach the editor and a widget *inside the module panel* obeys them; the same gesture aimed at the rack does nothing at all. **Filed as E95.** Three causes are already excluded and the next run should not re-derive them: it is not the coordinates (the ±4 DIP sweep, over a jack centre derived from the app's own screenshot); it is not `--drag`'s single-dispatched-job design (a hand-rolled `--pointer-down` / 20× `--pointer-move` / `--pointer-up` fails identically); and it is not the channel (`CommandDispatcher::cmdDrag` is platform-neutral — `onPointerDown`, `onPointerMove`×steps, `onPointerUp` straight into the client). **Both gestures pass on macOS**, where `e82` cables these exact jacks.

**Learned — the trap that cost the most, and it is not DPI.**

- **Rack coordinates are not portable between platforms, and DPI is not the reason.** The window is 1100x626 everywhere and the channel takes DIPs, so DPI is already divided out — this box renders at 1.5× and the screenshot is 1650x939, which is exactly that. **What differs is the RACK's OWN ZOOM: 0.713 here against mac's 0.745.** Four per cent of a 380-unit panel is 11 DIP and a jack is ~7 DIP across, so `e82`'s `WT_JACK = "590,540"` against a measured centre of **586.3, 529.7** lands outside the jack — and the probe then fails at a clause with nothing to do with what it tests. [tests/rack_geometry.py](tests/rack_geometry.py) derives the panel and its jacks from a screenshot the probe already takes. **Do not add a per-platform coordinate table; that is the same bug with more rows.**
- **`Out L` was wrong too, by less:** measured **590.1, 224.1** against the assumed `592,220`.
- **A Git Bash heredoc eats a backslash.** `r"\\.\pipe\x"` written into a `<<'PY'` heredoc arrived as `\.\pipe\x`, and `CreateNamedPipeW` answered `ERROR_INVALID_NAME (123)` — which reads exactly like a wrong API call. Fifteen minutes went into a ctypes signature that was never the problem. **Write the script to a file.** This is [[windows-tmp-path-mismatch]]'s trap in a new place.
- **The app's stderr through a redirect is block-buffered**, so a trace line can surface seconds after the gesture that caused it; an early version of the cable test read a *stale* `first NONZERO OUTPUT` from the placement and reported a cable that had not been made. Gate on `document changed` and poll — the same lesson the 11-02 entry recorded for REAPER's stderr.

**Also measured, and it is independent agreement rather than a re-run:** `RackProcessor: 'LFO2' first NONZERO OUTPUT pin 0` reads **0.500131** in the Windows standalone at the default 1024 `Wave points`, against **0.500143** on the mac standalone (E82), **0.500142** on hosted linux VST3, and **0.500143** in a bare Windows VST3 host (E94). Four venues, three platforms, agreeing to the fourth decimal; the standalone's fifth place differs and I did not chase it.

**Not verified:** E82's menu clauses on this platform — the ported `e82_wtlfo_wave_points_probe.py` now opens the channel, places the module and reaches **arm 1b** here before stopping on E95, so the menu half is blocked behind the cable, not behind the transport. **The other four standalone probes are NOT ported** — the helper is there and each needs its own geometry check, which is cheap but unverified, so I did not claim it. **win/GMPI** has no venue. I did **not** build or run SynthEdit or SynthEditCL, so *"SynthEdit still builds"* is not my claim; `GMPI_Wrappers` was **read only** and no shared repo was written to. *"`main` builds on Windows"* is mine only for this configuration. I did not re-read `docs/lessons.md` (179 KB) or `BACKLOG.md` in full, only where they bear on this walk.

**Scheduling: this run fired IN ITS SLOT, the second in a row.** First wall-clock reading **20:10:58 local Monday** against the `20:06` slot — about 4 minutes, inside `jitterSeconds 396`. **The October clock error did not recur, checked with the 10-26 detector rather than assumed:** `Win32_OperatingSystem.LastBootUpTime` **2026-11-13 15:39:24** against the event-log boot entry **15:39:17** is **7 seconds** apart, not a timezone offset, and `Root Dispersion` is **9.08 s** against October's 46,808.97 s. The `Kernel-General` event 1s over the last five days are routine ~1.1 s syncs every ~9 h; the last was 15:17 local, ~4 h 53 m before the slot.

**The box was idle: `GetLastInputInfo` read 19,129 s = 5 h 18 m** (last input about 14:52 local). Three Visual Studio instances and Outlook were open and untouched. **No synthetic Win32 input was sent anywhere this run** — the standalone is driven over its own command channel, which injects into the plugin's editor rather than through `SendInput`, so the 11-02 stray-click hazard does not arise in this venue at all. That is a real advantage of the standalone over the REAPER leg on a developer's workstation.

**Jeff's working tree:** `C:\SE\TideSynth` was dirty on arrival — `RackModules/VCO.synthedit` with a real content change (`git diff --ignore-all-space` shows 444 lines, so not CRLF churn) plus seven untracked `RackModules/VCO.skin/*.svg`, the same state the 11-10 run found. **Untouched, per STEP 5's third kind.** All work was in a scratchpad `git worktree`, so his checkout never left `main` and was never modified. `GMPI_STANDALONE_CONFIG_DIR` was set on every launch and is honoured before the platform arms (`StandaloneSettings.cpp`), so `%APPDATA%\TIDE Rack\` was never read or written. No credential value appears anywhere.

**Next:** see the `win` cell. **For Jeff, three things:** (1) **E95** — rack drags do nothing in the Windows standalone while widget drags work; a fix may land in GATED `EditorLib`, which would be a ruling question rather than a build-break repair. (2) **E19's row text and its Accept disagree about which formats `win` owes** — the text says win/CLAP, the Accept says VST3+GMPI+STANDALONE, and two cells have now been pointed by the text. (3) **The bot token's documented expiry was 2026-11-07, nine days ago, and both identity paths still answer `tide-rack-bot`** — the journal has repeated that date for weeks; it is either renewed or stale, and I did not check it against the credential.

**Branch/PR:** `tide/win/E19-win-standalone-pipe`. It changes `tests/`, the E19 and new E95 rows, the `win` cell, the journal, the rotation and `docs/lessons.md`. `tests/**` is off the auto-merge allowlist, so it waits for Jeff by design.

## 2026-11-02 — windows — E19: the win/VST3 cell passes pixel diff and int/enum in REAPER, run twice with identical numbers — and a stray click reached the developer's Outlook before the input gate existed (scheduled run)

**Prompt:** 18210767f · Opus 5, `claude-opus-5` · app Claude desktop **4.4212.0.0** (from the running `claude.exe` under `WindowsApps`; CLI not on PATH) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** took **E19**, the win/VST3 cell, and measured its two owed clauses — the rack-canvas **pixel diff** and **int/bool/enum** — in REAPER on the `e83` fixture. `FLEET-PAUSED` absent. STEP 0's fetch succeeded. `run-lock.py acquire` rc=0 with the documented windows `owner=self (NO agent ancestor matched -- this lock is age-capped only)` warning.

**STEP 1:** no open `platform:win` issue; the only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, E89 DONE and archived) and #44 (the digest). **STEP 1.5:** no open PR on `tide/win/**` — in fact zero open PRs in the fleet. **STEP 2:** the `win` NEXT cell named no item but said *"if #671 has merged, E19 is free and it is your item"*; it merged on 10-29, so E19 was `TODO`, `any`, unclaimed, and the topmost eligible row. The other ten reachable rows are parked for the reasons the 10-26 windows cell lists; `git diff --stat f4e3a61 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows one line (the `mac` cell), so that walk stands. All four open `PROPOSED:` entries still say *"may proceed meanwhile: everything"*.

### The result: the cell passes, and two runs agree to the sixth place

Branch `tide/win/E19-win-vst3-pixel-menu`. REAPER **7.78**, `main` at `9818c48`, Release with `TIDE_VCV_FUNDAMENTAL=ON -DRACK_ADAPTOR_TRACE=1` and `-DSE_LOCAL_BUILD=OFF` (so all ten deps were fetched into the build tree and **no sibling checkout of Jeff's was read**), 0 build errors. Fixture `tests/fixtures/e83-vcv-scope-cabled.xml`.

| clause | run 1 | run 2 |
|---|---|---|
| Scope screen, 65 s apart | **2,272 of 22,000** | **2,287 of 22,000** |
| whole editor | 7,268 of 1,485,000 | 7,268 of 1,485,000 |
| module browser (in-frame ZERO control) | **0 of 147,900** | **0 of 147,900** |
| REAPER transport, same pair (LIVE-CAPTURE control) | 1,131 of 64,800 | 1,497 of 64,800 |
| WT LFO selected | 18-item TIDE menu, **`Wave points >` 32…16384, 1024 ticked** | same |
| empty rack, deselected | TIDE's own **7-item rack menu**, no `Wave points` | same |
| DSP obeys: A/B/A/B | **0.500143 / 0.500004 / 0.500143 / 0.500004** | identical |
| edit with NO pick | **0.500004** | **0.500004** |
| transport | `playstate=1`, pos 0 → 106.6, real time against `t` | same |

**The verification artifact is [tests/e19_win_vst3_probe.py](tests/e19_win_vst3_probe.py)** — the whole measurement in one hosted window, exit 0 twice — plus the new Windows screen driver [tests/e19-host-feedback/e19x_win.py](tests/e19-host-feedback/e19x_win.py). `fx_ident` named the staged bundle on every launch, which mattered more than usual: **Jeff had installed his own build of the same plug-in at 12:15 today**, and the two carry the same VST3 UID.

**Why the 2026-09-02 run got 0 of 760,950 and this one did not:** the fixture, exactly as that entry predicted. `e53` puts no VCV panel on the visible rack page; `e83` does.

**The menu is read as TEXT on this platform**, which is strictly better than the linux leg. A Windows context menu is an ordinary `#32768` window that answers `MN_GETHMENU`, so `GetMenuItemInfoW` yields every row and its `MF_CHECKED` directly — no screenshot, no row-height arithmetic. It also sharpens the control: both menus are identified as TIDE's own by the `About TIDE` item they carry, so *"no `Wave points` when deselected"* is known to be the plug-in answering rather than the host.

### For Jeff, first: a stray click reached your Outlook, and the mechanism is worth knowing

While shaking the harness out, **this probe's clicks and drags went into REAPER's track view and then into your open Outlook window — one of them into its folder pane, which changed the folder on display.** Nothing was deleted and no menu item was ever activated: every menu lookup after those clicks failed to find its item, and no `Delete` keystroke was ever sent. **That was luck, not design.**

The mechanism is trap 2 below: an unconditional second `Escape` closes REAPER's floating FX window, the editor HWND is destroyed, and `ClientToScreen` on a dead window **silently leaves the point at 0,0** — so every later click is delivered in RAW SCREEN COORDINATES to whatever happens to be underneath. Nothing errors; the input simply goes somewhere else.

The fix is `check_target()`: `WindowFromPoint` before every click and before **both ends** of every drag, raising `MisdirectedInput` rather than delivering. **Any new GUI probe on this box should use it.** The general rule, which this run paid for: *on a developer's workstation a missed click is a safety failure, not a flaky test* — synthetic input that misses its target does not fail, it lands in somebody else's program.

**Learned — four Windows-only traps, and what makes them expensive is that all four present as the PLUG-IN misbehaving:**

- **`MOUSEEVENTF_ABSOLUTE` without `MOUSEEVENTF_VIRTUALDESK` maps onto the PRIMARY monitor, not the virtual desktop.** A click meant for screen 892,603 was delivered at 1874,603 — `892 × 3840/5760` — which is REAPER's ruler, and up came REAPER's timeline menu. **The Y was correct throughout**, because the virtual desktop is the same height as the primary monitor here, so it reads as *the wrong menu* rather than as a miss. The driver now verifies with `GetCursorPos` and refuses to click if the cursor did not land where asked.
- **An unconditional `Escape` closes the floating FX window** — see above. Escape only while a `#32768` is actually on screen.
- **`GetAncestor(editor, GA_ROOT)` answers 0 here**, so the top-level to raise must come from the enumeration that found the editor.
- **`SetForegroundWindow` is refused to a process that does not own the foreground window.** It returns having raised nothing, REAPER's maximized main window stays over the 1672×995 float, and clicks land on the host. `AttachThreadInput` did not help either; **clicking the float's own caption is what works**, and the function now checks `GetForegroundWindow` instead of assuming.

Two more, smaller: **REAPER holds `measure.log` open**, so a relaunch whose `rm` failed leaves `measure.lua` dying on `io.open` and the host sitting on an empty project — kill REAPER and wait for the process to go before clearing logs. And **REAPER's stderr is block-buffered through the runner's redirect**, so a rebuild's trace line can surface many seconds after the edit; a fixed 3 s wait read every arm as `(none)` while the rebuilds were plainly happening in the log. Poll.

**The lane's scheduling, since five previous cells have asked about it: this run fired IN ITS SLOT.** The first wall-clock reading I took was **20:09:35 local Monday**, about 3 minutes after the `20:06:36` slot the 10-26 entry recorded, well inside `jitterSeconds 396` — where four of the five firings before 10-26 were catch-ups into Jeff's working hours. **And the 10-26 clock error did not recur, checked with that entry's own detector rather than assumed:** there were two `Kernel-General` event 1s today (09:33:33 and 18:39:41 local), but `Win32_OperatingSystem.LastBootUpTime` **08:42:21** against the event-log boot entry **08:42:13** is **8 seconds** apart rather than a timezone offset, and `Root Dispersion` is **9.07 s** against October's 46,808.97 s. The later sync was ~1 h 27 m before the slot. I did **not** query the scheduler's own `lastRunAt`/`nextRunAt`, so the in-slot claim rests on my local clock plus that skew check.

**The box was idle:** `GetLastInputInfo` read **11,268 s = 3 h 08 m** (last input about 17:02 local). That is the shortest idle any lane has acted on — mac took E82 at 34 h — and it is an evening slot, but three Visual Studio instances were open and untouched, and `GetLastInputInfo` is tick-based so the 10-26 clock lesson does not apply to it.

**Not verified:** the **bool** arm (no bool-bearing module in the `e83` fixture — the same gap linux reported), **win/CLAP**, and `string` still has no producer (the 2026-09-16 census). I did **not** build or run SynthEdit or SynthEditCL, so *"SynthEdit still builds"* is not my claim — no shared repo was touched, read or fetched, and nothing outside TideSynth was modified. *"`main` builds on Windows"* is mine only for this configuration (Release, `SE_LOCAL_BUILD=OFF`, VCV Fundamental on). I did not re-read `docs/lessons.md` (178 KB) or `BACKLOG.md` in full, only where they bear on this walk. **I did not audit Jeff's Outlook** beyond observing the window title, and I will not; the input log above is the complete account of what was sent.

**Jeff's working tree:** `C:\SE\TideSynth` was dirty on arrival with a real content change to `RackModules/VCO.synthedit` (parameter values — `git diff --ignore-all-space` shows content, so not CRLF churn), alongside his two commits `97fd0e2` *"renamed to VCO"* and `9818c48` *"added VCO sync"*. **Untouched, per STEP 5's third kind.** All work was in a scratchpad `git worktree`, so the checkout never left `main` and was never modified.

**Machine state:** `%APPDATA%\REAPER` was backed up before the first launch and restored afterwards; both `REAPER.ini` and `reaper-vstplugins64.ini` came back md5-identical. REAPER was never allowed to quit on its own, so it never rewrote the ini on the way out. No credential value appears anywhere. Box lock released as the last act.

**Next:** see the `win` cell — the cheapest remaining win debt is **win/CLAP**, whose drivers already exist. **For Jeff, three things:** (1) **the Outlook incident above**, and the input gate that now prevents it; (2) **E19's bool arm is owed on two platforms and needs a fixture, not a platform** — it may deserve its own row; (3) the bot token expires **2026-11-07**, five days from today — I did not re-check that date against the credential, and this is the closest any run has been to it.

**Branch/PR:** `tide/win/E19-win-vst3-pixel-menu` — [#678](https://github.com/JeffMcClintock/TideSynth/pull/678). It changes `tests/`, the harness README, the E19 row, the `win` cell, the journal, the rotation and `docs/lessons.md`. `tests/**` is off the auto-merge allowlist, so it waits for Jeff by design.

## 2026-10-29 — macos — no item: #671 merged and E19 is free again, but its only mac clause is AU3 in a real host and that still needs Jeff's AUv3 registration displaced (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **4.29.0** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. I re-pointed the `mac` NEXT cell and rotated one entry out. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`fd8fd5b..f4e3a61`). `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: there are no open `platform:mac` issues. STEP 1.5: there are no open PRs at all. Remote heads are `main` and Jeff's `gpu-backend`. STEP 2: [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) merged (`44e6949`), so E19 is `TODO`, `any`, and no longer claimed by an open PR. `git diff --stat fd8fd5b origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md` only: the `mac` and `linux` cells, the E19 row's new linux text, and E82 leaving for `BACKLOG-DONE.md`. `decisions.md` and `PLAN.md` are unchanged, so the rest of the 10-27 walk stands.

- **E19:** the row's Accept gives mac one cell: *"AU3 in a real host, not just auval"*. `pluginkit -m -v` still shows only the 2026-08-31 `com.tidesynth.tiderack.au3app.extension` 0.1.1 at `~/Applications/TIDE-Rack-AUv3.app`. Measuring a current build means registering a new appex over Jeff's, and every mac cell since 10-25 has left that for when he is present. The row's other open clauses (bool, linux/CLAP, win/VST3) are not mac's.
- **E93:** not built unasked. **The other `any` rows** stay parked for the reasons the 10-26 windows entry lists.

Since 10-28 Jeff also landed product work on `main` directly (the Oscillator prefab's waveform slider switch and its SVG, `SynthEditSem/CMakeLists.txt` staging it into the bundles). No row names it, so nothing here acts on it.

**Learned:**

- **A merged claim does not make a row reachable.** E19 stopped being taken today, but the mac cell was never blocked by the claim. It is blocked by the AUv3 registration, which is a separate thing.

**Not verified:** nothing was built or run, so "`main` builds on mac" after Jeff's `SynthEditSem/CMakeLists.txt` change is CI's claim, not mine. The box was idle (HID idle 213,409 s, about 59 h). I read `BACKLOG.md`, `docs/lessons.md` and `docs/decisions.md` only where they bear on this walk. The bot token expires **2026-11-07**, 9 days from today. I did not re-check that date against the credential.

**Rotation:** done, one entry. With this entry `JOURNAL.md` was over 60 KB. I moved the oldest entry (10-20 windows) to the end of [JOURNAL-2026-10.md](JOURNAL-2026-10.md), verbatim, which brought the file back under 60 KB (57,327 bytes) with nine entries. `extract-lessons.py --check` was green on the pristine worktree before any edit (1,736 lessons from 406 entries). I regenerated `docs/lessons.md` afterwards.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) E19's mac AU3 cell needs you present, or a word that a run may register a fresh AUv3 appex over the 08-31 one. (2) The bot token expires 2026-11-07, 9 days out.

**Branch/PR:** `tide/mac/noitem-2026-10-29`. It changes the journal, the rotation, the `mac` NEXT cell and `docs/lessons.md`.

## 2026-10-28 — macos — no item: nothing moved since the 10-27 walk except this lane's own cell, and #671 is now CONFLICTING (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **3.6556.2** · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. I re-pointed the `mac` NEXT cell. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`3b63f57..fd8fd5b`). `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: there are no open `platform:mac` issues. STEP 1.5: there is no open PR on `tide/mac/**`. STEP 2: the only commit since `3b63f57` is the 10-27 mac bookkeeping merge (#673), and `git diff --stat 3b63f57 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows one line, the `mac` cell. So the 10-27 walk stands row for row. E19 is taken by #671, E93 stays unbuilt unasked, and the `any` rows are parked for the reasons the 10-26 windows entry gives. Nothing is eligible.

**One new fact:** [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) (`tide/linux/E19-vst3-linux-recheck`) still passes its checks (13 pass, 2 skipping), but GitHub now reports it `mergeable: CONFLICTING`. The 10-26 and 10-27 bookkeeping merges both said they would conflict with it at the top of `JOURNAL.md`, and that has now happened. It is the linux lane's PR, so STEP 1.5 hands it to that lane, not this one. I did not touch it.

**Learned:**

- **Each no-item bookkeeping PR that merges ahead of a waiting PR adds to that PR's conflict.** Three lanes prepending to `JOURNAL.md` while #671 waits on Jeff is what turned it from green to `CONFLICTING`.

**Not verified:** nothing was built or run. The box was idle (HID idle 127,010 s, about 35 h), but `pluginkit -m -v` still shows only the 2026-08-31 `com.tidesynth.tiderack.au3app.extension` 0.1.1, so E19's mac AU3 cell is still out of reach even once #671 merges. I did not read `docs/lessons.md` or `BACKLOG.md` in full. The bot token expires **2026-11-07**, 10 days from today. I did not re-check that date.

**Rotation:** not needed. `JOURNAL.md` is 59,824 bytes with this entry, just under 60 KB. The next entry will have to rotate.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) #671 now needs a rebase by the linux lane before it can merge. (2) E19's mac AU3 cell needs you present. (3) The bot token expires 2026-11-07, 10 days out.

**Branch/PR:** `tide/mac/noitem-2026-10-28`. It changes the journal, the `mac` NEXT cell and `docs/lessons.md`. It will conflict with #671 at the top of `JOURNAL.md`.

## 2026-10-27 — macos — no item: E82 merged, E19 claimed by the linux lane on #671, and nothing else is reachable by `mac` (scheduled run)

**Prompt:** 1821076 · Opus 5.5, `claude-opus-5-5` · app Claude desktop **3.6556.2** (CLI version not readable, no `claude` on PATH) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item. I re-pointed the `mac` NEXT cell, which still said E82 was in review. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`09206a2..3b63f57`). `run-lock.py acquire` returned rc=0 (`owner=auto:ancestor-match`).

**Result:** STEP 1: there are no open `platform:mac` issues. STEP 1.5: there is no open PR on `tide/mac/**`. The only open PR is [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) (`tide/linux/E19-vst3-linux-recheck`), and it is green, so I left it alone. STEP 2: `git diff --stat 5a24534 origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows only `BACKLOG.md`, three lines: the `win` and `mac` NEXT cells and E82 moving `TODO`→`IN-REVIEW`. So the 10-26 walk stands row for row:

- **E82:** merged ([#669](https://github.com/JeffMcClintock/TideSynth/pull/669), `09206a242`). Its flip and archive are already on #671 (it edits the E82 row and `BACKLOG-DONE.md`). Doing them here would only add a conflict, so I left them to #671.
- **E19:** taken. #671 is an open PR from a different platform that names the id. Its mac AU3 cell needs Jeff's AUv3 registration displaced anyway. `pluginkit -m -v` still shows only the 2026-08-31 `com.tidesynth.tiderack.au3app.extension` 0.1.1. The box was idle (HID idle 40,616 s), but that does not unlock this.
- **E93:** not built unasked. Its Accept is either a WONTFIX ruling or a non-TIDE AU2 consumer, and neither is a run's call.
- **The other `any` rows** (A35, A37, A42, A43, A45, S8, E2, E84) are parked for the reasons the 10-26 windows entry lists. `decisions.md` and `PLAN.md` are unchanged.

**Learned:**

- **When a merged PR's bookkeeping flip already rides on another platform's open PR, leave it there.** Doing the flip twice only adds a conflict on the most contended rows.

**Not verified:** nothing was built or run, so "`main` builds on mac" is CI's claim, not mine. I read `BACKLOG.md`, `docs/lessons.md` and `docs/decisions.md` only where they bear on this walk, not in full. I did not verify #671's measurements. The bot token expires **2026-11-07**, 11 days from today. I did not re-check that date.

**Rotation:** not needed. `JOURNAL.md` is still under 60 KB with this entry.

**Machine state:** `~/Documents/GitHub/TideSynth` stayed on `main`, clean. The work was in a scratchpad `git worktree`, removed at the end. No other repo was touched, no GUI was used, and no credential value appears anywhere. I released the box lock as the last step.

**Next:** see the `mac` cell. **For Jeff:** (1) #671 is green and waiting on you. Merging it also flips E82 to DONE. (2) E19's mac AU3 cell needs you present. (3) The bot token expires 2026-11-07.

**Branch/PR:** `tide/mac/noitem-2026-10-27`. It changes the journal, the `mac` NEXT cell and `docs/lessons.md`. It will conflict with #671 at the top of `JOURNAL.md`.

## 2026-10-26 — windows — no item: this lane finally got the idle evening box it has been blocked on for five runs, and the queue had nothing left for it — E19 was claimed by the linux lane nine hours earlier. What delivered the idle box was a **13-hour clock error** (scheduled run)

**Prompt:** 18210767f · Opus 5, `claude-opus-5` · app Claude desktop **3.6556.2** (CLI version not readable — no `claude` on PATH and `~/.claude/versions` is empty on this box) · as **tide-rack-bot** (both paths: REST `tide-rack-bot`, GraphQL `tide-rack-bot 314850083`, matching the hard-coded `GIT_AUTHOR_EMAIL`) · transport assertion `git@github.com:`, as required · scheduled run

**Did:** no backlog item — nothing is eligible for `win`, and the walk below is derived this run rather than inherited. Nothing was built and no product code ran. `FLEET-PAUSED` is absent on `origin/main`. STEP 0's fetch succeeded (`20a60580f..09206a242`). `run-lock.py acquire` rc=0, pid 25272, `owner=self (NO agent ancestor matched -- this lock is age-capped only)` — the documented Windows path, printing the warning it is supposed to print.

**STEP 1:** no open `platform:win` issue, and on this platform that verifies nothing — `build.yml:523` (`matrix.platform != 'win'`) excludes windows from filing, as six previous cells record. The only open issues in the repo are [#583](https://github.com/JeffMcClintock/TideSynth/issues/583) (linux, triaged as E89, DONE and archived) and #44 (the digest). **STEP 1.5:** no open PR on `tide/win/**`. One PR is open in the fleet and it is not mine: [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) (`tide/linux/E19-vst3-linux-recheck`), every check green, `tests/**` so off the auto-merge allowlist and waiting for Jeff by design. Green with nothing unresolved is not mine to touch, so I left it alone.

### The finding: the box was unattended for the first time in this lane's history, and the queue was empty anyway

Five windows runs in a row (09-14, 09-16, 10-13, 10-20 and this one) have ended with no item, and every one of the first four gave the same cause: **E19 and E82 were the only two rows this lane could take, both need an idle box, and the box was never idle.** The 10-20 entry measured why — the Monday 20:06 local slot is missed and the catch-up fires at Claude app launch, which is by construction when the developer sits down.

**This run had the idle box.** `GetLastInputInfo` read **18,371 s — 5 h 06 m 11 s** at 22:59:04 local, so the last input was about **17:52:53**. One `devenv` and three `VirtualBoxVM` processes are up, both since early afternoon, and neither has been touched since. Nobody is at this machine.

**And there was nothing to do with it, because the other two boxes finished both rows first.** E82's Accept was met by the mac lane on 10-25 ([#669](https://github.com/JeffMcClintock/TideSynth/pull/669), merged 2026-10-25T22:55Z as `09206a242`), and E19 was claimed by the linux lane at **2026-10-26T00:20Z**, **9 h 36 m before this run started**. So the lane's blocker has changed shape rather than cleared: it is no longer *"the box is never idle"*, it is *"this lane is awake so rarely that the two rows reserved for an idle box were completed by the boxes that run daily."* That is a scheduling consequence and it belongs to **A45**, which is already filed and already waiting on Jeff; I filed nothing, per the 10-23 mac precedent of not re-filing A45's own evidence as a new row.

### What delivered the idle box: a 13-hour system clock error, corrected 2 m 21 s before this run fired

This run started **09:56:34Z = 22:56:34 Monday local**. Its slot was `nextRunAt 2026-10-26T07:06:36Z` = **20:06 Mon local** (recorded by the 10-20 entry; `nextRunAt` is now `2026-11-02T07:06:36.000Z`, the same seconds, which is what confirms the slot was not re-pointed). So the run is **2 h 49 m 58 s late**, and `jitterSeconds 396` — 6 m 36 s at most — cannot cover that.

**The mechanism is in the system event log, and it is not app launch this time.** `Microsoft-Windows-Kernel-General` **event 1** (system time change) fired twice at 22:54:13 local, and the material one says:

| field | value |
|---|---|
| `OldTime` | `2026-10-25T20:54:12.586Z` |
| `NewTime` | `2026-10-26T09:54:13.792Z` |
| `TimeDeltaInMs` | **46801206** = **13 h 00 m 01.2 s forward** |
| `ProcessName` | `svchost.exe` (w32time), `Reason 1` |
| `TimeZoneBias` | `-780` (UTC+13, `New Zealand Standard Time` + DST) |
| `RealTimeIsUniversal` | `false` |

`w32tm /query /status` agrees and dates it: **`Last Successful Sync Time: 26/10/2026 9:54:12 am`** — that stamp is in the *pre-jump* clock, i.e. real 22:54:12 — with `Leap Indicator: 3(not synchronized)` and `Root Dispersion: 46808.97s`, which is the size of the jump.

So **the clock came up about 13 h slow at boot and stayed that way for roughly nine hours.** Boot is `Kernel-General` event 12 stamped `00:47:29`, and `Win32_OperatingSystem.LastBootUpTime` — computed from the *corrected* clock — reads **13:47:29**: the same instant, 13 h apart, which is the cleanest proof of the skew. Why it came up slow, and why w32time's first successful sync took nine hours, are **not established** here. A `RealTimeIsUniversal=false` machine whose RTC holds UTC is the obvious candidate and the three running VirtualBox VMs are another; I measured neither, and changing a system clock setting is not a run's to do.

**The slot and the correction are the same event.** Under the slow clock, local time read 07:06 Monday at the real 20:06 slot, so the cron's `0 20 * * 1` had not come round; it would next have matched at real **09:06 Tuesday local** — another catch-up into the developer's working morning, exactly the 10-20 pattern. The 22:54:13 correction made the Monday 20:06 slot past-due instead, and the scheduler fired the catch-up **2 m 21 s later**. A clock error is what bought this lane its first evening run.

### Three instrument notes, because this box's timestamps are now known to lie

- **Tick-based readings survived the jump and wall-clock stamps did not.** `GetLastInputInfo`/`GetTickCount` measure since boot, so the 5 h 06 m idle above is unaffected. Every absolute stamp this box recorded between 13:47 and 22:54 real time — process `StartTime`, the event log, anything a run or the digest wrote — is **13 h behind**. Relative orderings inside that window are intact, because the offset is uniform.
- **The 10-20 entry's conclusions survive this, and it is worth saying why rather than leaving it hanging.** Its evidence was relative: app launch 09:10:39, the run 09:10:45 six seconds later, every other process 09:10:26–09:10:59, `GetLastInputInfo` 1 s. A uniform skew changes none of that. What a skew *can* corrupt is the one comparison that mixes sources — server-side `lastRunAt`/`nextRunAt` against a locally stamped event — which is precisely the comparison A45 rests on. Use the server-side pair on both sides where you can.
- **`quser` printed `IDLE TIME: none` for the console session while `GetLastInputInfo` said five hours.** Do not use `quser` for this question on this box.

### STEP 2, walked rather than inherited

`git diff --stat 20a60580f origin/main -- BACKLOG.md docs/decisions.md PLAN.md` shows `BACKLOG.md` only, four lines: the `win` and `mac` NEXT cells, A45 filed, and E82 moved `TODO`→`IN-REVIEW`. `decisions.md` and `PLAN.md` have not changed since the 10-20 walk. Eleven rows are `TODO` and reachable by `win` or `any`, and every one is parked:

- **E19** is **taken**. [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) is an open PR from a different platform naming the id, which STEP 2 settles in one sentence — and the collision is real rather than nominal: that PR rewrites the whole E19 row and the `linux` cell, so a second branch editing it would conflict in the row it is reporting on. **This is the row this run would otherwise have taken**, and it is the only one the idle box above would have unlocked.
- **E82** is `IN-REVIEW` on `origin/main` with [#669](https://github.com/JeffMcClintock/TideSynth/pull/669) merged, so STEP 4's mechanical flip reaches it — **but #671 already carries that flip and archive.** Doing it here would be a second edit to `BACKLOG.md`'s and `BACKLOG-DONE.md`'s most contended rows for no gain. Left to #671.
- **A35** says in terms it is *"PARKED ON ITS OWN TWO `PROPOSED:` ENTRIES … NOT AVAILABLE TO TAKE"*; both are still open.
- **A37** is parked on its own Accept, which asks for *"a stated ceiling"* and an archive convention that does not exist yet — and its own Size line says the convention question *"may want Jeff rather than a run"*. Under-specified by STEP 2's test, as the 10-13 and 10-20 runs read it.
- **A42** is ineligible by its own words: *"(b) without (a) is not identical under every answer"*.
- **A43** is the same shape and says *"a run should not pick"*.
- **A45** is this lane's own scheduling row and picks nothing deliberately; every option is about Jeff's hours and his box.
- **S8** carries `NEEDS-SPEC` first in its row.
- **E2** is an umbrella that says of itself it is not takeable.
- **E84** is a `.github/workflows/**` edit this credential deliberately cannot make.
- **X2** is `linux`, **E93** is `mac`.

All four open `PROPOSED:` entries in `docs/decisions.md` were read: they say *"May proceed meanwhile: everything"*, *"ALL ordinary work, without exception"*, *"everything except E72 itself"* (archived) and *"EVERYTHING, without exception"*. **None parks a row by its own clause**, so A35 and A37 are parked by their rows' own Accepts, as before. The three `DONE-PENDING-ACCEPT` rows (E7, E44, E49) await an acceptance judgement rather than a merge, so STEP 4's rule does not reach them.

**Default branch:** not built here — there was no item. CI's reading is green: `build` **success** at `09206a242`, the current `main` head, and #671's own `windows` job passed in 11 m 19 s against the same content.

**Learned:**

- **A workstation's clock can be hours wrong for most of a day, and then a routine NTP sync becomes a scheduler event.** The correction made a missed slot past-due and the catch-up fired 2 m 21 s later. Any conclusion of the form *"the task fired at X"* on this box should be checked against `Kernel-General` event 1 over the same window before it is believed.
- **`Win32_OperatingSystem.LastBootUpTime` and the boot entry in the event log disagreeing by exactly a timezone offset is the cheapest possible detector for this.** One is computed from the current clock and the other was stamped by the old one; the gap between them *is* the skew, with no NTP query needed.
- **Prefer tick-based instruments to wall-clock ones for "is anybody there".** `GetLastInputInfo` is immune to a clock jump by construction, and it was the only reading this run did not have to re-derive. `quser`'s `IDLE TIME` column is useless here — it said `none` against a measured five hours.
- **A row reserved for a condition one lane almost never meets will be finished by a lane that meets it often.** E19 and E82 were this lane's only two rows for five runs; by the time the idle box arrived, the mac box had closed E82 and the linux box had claimed E19 nine hours earlier. Reserving work for a rare condition is not the same as reserving it for the box that has the condition.
- **A full stop in STEP 2 can be the correct outcome of a *successful* run.** Every guard passed, the one structural blocker cleared for the first time, and the answer was still "nothing is eligible" — because availability and eligibility are different properties and only one of them improved.

**Not verified:** **nothing was built, compiled or run as product code**, so *"`main` builds on Windows"* is CI's claim and not mine, and I did not read the build logs — only the run conclusions. **No GUI was taken, no window created and no screenshot made**, even though the box was idle: the one row that wanted a window is taken by #671, and preparing a taken item's measurement is still working on a taken item. **The cause of the 13 h clock skew is not established** — RTC-as-UTC and the VirtualBox VMs are candidates I did not test, and I changed no system setting. **Why w32time's first successful sync took about nine hours is likewise unestablished.** I did not re-read `docs/lessons.md` in full (176 KB), only the parts bearing on this walk. **I did not verify #671's measurements**, only its state and its diff. The bot token expires **2026-11-07**, **12 days** from today; I did not re-check that date against the credential.

**Rotation:** done, and it took **two** entries rather than one. With this entry `JOURNAL.md` was 71,733 bytes; moving the oldest (10-14 windows, E89/box-lock) left 63,605, still over the 60 KB stop, so the next oldest (10-16 linux, E92) went too, leaving **52,936 bytes and six entries** — above the four-entry floor, which is what the rule stops on. Both were appended to [JOURNAL-2026-10.md](JOURNAL-2026-10.md) below what was there, verbatim and in date order (that file is ascending in practice, whatever the header's *"newest-first"* says — its tail was already the other 10-14 entry). `extract-lessons.py --check` was green on a pristine `origin/main` worktree before any edit (1,725 lessons from 402 entries) and is green again after the regeneration (1,730 from 403) — measured on a separate checkout, because running it after the prepend only tells you your own edit is unwritten. **#671 rotates the 10-14 entry too and stops there**, so our first archive hunk is identical and the second is mine alone; expect to re-resolve the tail of the archive as well as the top of `JOURNAL.md`.

**Machine state:** `C:\SE\TideSynth` **stayed on `main` and was never checked out** — all work was in one scratchpad `git worktree`, removed and pruned at the end. The tree was clean on arrival. No other repo was touched, read or fetched: `C:\SE\SE16`, `C:\SE\SynthEditLib`, `C:\SE\gmpi_ui`, `C:\SE\GMPI_Wrappers` and `C:\SE\GMPI` were not opened, because no item needed them — and the developer has a Visual Studio instance open, so not touching them was also the right call independent of the queue. No credential value appears anywhere. Box lock released as the last act.

**Next:** see the `win` cell. **For Jeff, four things:** (1) **This box's clock was 13 hours slow from boot at 13:47 until w32time corrected it at 22:54** — worth a look, because it will recur on every boot if the cause is the RTC/UTC setting, and while it lasts every timestamp this box writes into the fleet's own records is 13 h out. (2) **A45 is still the one row waiting on you**, and this run sharpens it rather than answering it: the idle box arrived and the queue was empty, because the two rows held for it were finished by the two boxes that run daily. (3) [#671](https://github.com/JeffMcClintock/TideSynth/pull/671) is green and waiting on you, and **E19's win/VST3 cell is the next win item the moment that PR merges** — its two owed clauses (the rack-canvas pixel diff and `int/bool/enum`) now have a producer and a recipe from #669's `WT LFO` probe. (4) The bot token expires **2026-11-07**, 12 days out.

**Branch/PR:** `tide/win/2026-10-26-queue-blocked`. This entry, the `win` cell, the rotation and the regenerated `docs/lessons.md` are on that branch. It is bookkeeping-only and should auto-merge. It will conflict with #671 at the top of `JOURNAL.md`, and at the tail of `JOURNAL-2026-10.md` because this rotation moved one entry more than #671's.

