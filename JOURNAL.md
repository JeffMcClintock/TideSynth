# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-27 — macos — E32: the mac window position, and a ruling that deleted 109 lines of it (interactive session, Jeff directing)

**Prompt:** standing backlog-loop instruction in the session · Opus 5, `claude-opus-5` · Claude Code · commits authored `Jeff McClintock` per the interactive convention

**Did:** took **E32's last remaining half** — macOS window position. Built and
measured; branch `tide/mac/E32-window-position-mac`,
[GMPI_Wrappers#23](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/23),
**PR-gated so proposed, not merged**. Row is IN-REVIEW. Filed **E52** for a
separate break found doing it.

### The ruling, which is the durable part

Jeff, on being shown that AppKit was undoing the restored position:

> in the case that a platform has different conventions, lets stick to them. If
> Appkit want's to move the window full on-screen. That's no big deal. esp if
> users on that platform expect it anyway.

**E32's Accept and `PlatformShell::setWindowPosition`'s own comment both encode a
WINDOWS rule** — clamp so "enough of the caption is reachable", not "fully on
screen", because a partly-off window is deliberate. Win32 needs that, because it
leaves a window wherever it is put. **AppKit does not**: every frame about to be
displayed goes through `constrainFrameRect:toScreen:`, which pulls the window
fully on screen, and mac users expect that.

The first cut fought it with an `NSWindow` subclass overriding that one method.
It worked, it was measured, and it was **deleted**.

### Deleting it measured BETTER, not merely smaller

This is the part to carry into the next platform-difference argument. It took
**109 lines** out of the change, and the platform's answer beat the hand-rolled
one on the case that matters most:

| saved `9000,9000` | result |
|---|---|
| hand-rolled clamp | `0,30` — position thrown away, window parked top-left |
| AppKit unaided | `1140,520` — bottom-right corner, wholly visible |

**The clamp was not just redundant, it was worse.** It replaced the user's
position with a corner; AppKit kept as much of the intent as the screen allowed.
Measured by bypassing the clamp in a throwaway build rather than by reading the
docs — the two answers are indistinguishable on paper and four of the six test
cases agree.

### The measurement trap, which survives the rewrite

The overhang cases are the only ones that can see this class of bug at all.
Centred, exact, off-screen and clamped — the four anybody writes first — **all
pass whether AppKit is constraining or not**, because every one of them produces
a fully-on-screen window, which is what the constraint produces too. Only "user
deliberately hung the window off the edge" separates them, and that is the case
that looks least worth writing.

### What macOS now costs, recorded so it is not re-filed as a bug

**A deliberate overhang does not survive a round trip on macOS.** `x=1900` on a
2240-point display reopens at `1140`, flush right. The value is stored and
restored *exactly*; AppKit moves the window afterwards. Windows keeps the
overhang, macOS does not. **E32's Accept should be read as satisfied
per-platform, not uniformly** — which is the general shape of the ruling above.

### Points, not pixels

`PlatformShell` specifies *physical screen pixels*; the mac shell answers in
**points**, deliberately. The seam's own comment gives its reason for wanting
pixels — one unambiguous space spanning every monitor, because a value in DIPs
"would have to say which monitor's scale it meant". On macOS that property
belongs to points; backing pixels are per-display. A centred 1100-point window on
a 2240-point display saves `x=570`, not `1140`.

**Not `NSWindow` frame autosave**, which E32's row recommended: it writes to
`NSUserDefaults` on its own schedule, which would put the position in a different
file from the SIZE the portable half keeps in `standalone.conf`.

### How to measure a window position with no verb for it

`--info` reports size but not position, and the command channel has no move verb.
The way round both: prepare `standalone.conf`, launch, `SIGTERM` (which runs the
normal teardown), read the file back.

### E52: a supported build option that does not compile

`GMPI_STANDALONE_COMMAND_CHANNEL=OFF` is an `option(... ON)` with a comment
explaining what an OFF build is for, and **`StandaloneApp.cpp` does not compile in
it** — it calls `windowPosition`, `setWindowPosition` and `logicalSize`
unconditionally while `PlatformShell` declares all three inside the guard. Three
errors, and **one is E32's own already-merged size half**, so it has been broken
since that landed.

Measured against a **stashed** tree, so the result is `main`'s and not the
branch's — worth doing deliberately, because "my change broke it" and "my change
revealed it" look identical from a compiler.

**Not fixed in #23 on purpose.** Moving the seam out of the guard means moving
all three shells' overrides with it, and only the mac one can be built on this
box.

## 2026-08-27 — macos — E25: the crash report's faulting address disproves E25's own diagnosis, and moves the fix to a different file (interactive session, Jeff directing)

**Prompt:** standing backlog-loop instruction in the session, not `docs/weekly-run-prompt.md` · Opus 5, `claude-opus-5` · Claude Code · commits authored `Jeff McClintock` per the interactive convention

**Did:** took **E25**, the `EXC_BAD_ACCESS` in `CContainer::getIgnoreProgramChange()`.
Row stays **TODO** — its Accept is not met — but the diagnosis it carried is
wrong, and the one-line fix it recommended would not have stopped the crash.
Also flipped **E43** to DONE (both PRs verified merged) and filed **E46**.
Branch `tide/mac/E25-null-container-diagnosis`.

### The whole thing turns on one number nobody had used

The report says `KERN_INVALID_ADDRESS at **0x50**`. The previous entry read the
stack and concluded the container's plug table was short of `PN_IGNORE_PC` (3),
so `GetPlug(3)` returned nullptr and `->GetDefault()` faulted. **That story
faults at `0x0`, not `0x50`** — `GetDefault` is pure virtual (`Plug.h:39`), so
the null goes through `ldr x8, [x0]`, a vtable read at offset zero.

What faults at exactly `0x50` is the OTHER null. `otool -tV` on the shipped
binary:

```
__ZN3CUG7GetPlugEi:
    tbnz  w1, #0x1f, ...        <- guards the INDEX, not `this`
    ldp   x8, x9, [x0, #0x50]   <- Plugs (std::vector) — this+0x50
```

So `getIgnoreProgramChange` **entered with `this == nullptr`** faults at `0x50`.
Forcing exactly that under lldb reproduces the report's stack **frame for
frame**, `EXC_BAD_ACCESS (code=1, address=0x50)` included. Shipped as
`tests/e25_null_container_probe.py`; `--run` is that control, and the default
static mode needs only `otool`.

**And it explains the frame that ISN'T in the report.** There is no
`PatchParameter_base::ignoreProgramChange` frame, which is what made the
previous run read `ExportXml` as calling `getIgnoreProgramChange` directly. The
disassembly shows `ignoreProgramChange` is inlined into `ExportXml` and
**tail-calls** (`b`, not `bl`) `getIgnoreProgramChange` — a tail call owns no
frame. A missing frame was evidence, not the absence of it.

### Where the null comes from, and why the recommended fix could not have worked

```
__ZN19PatchParameter_base19ignoreProgramChangeEv:
    ldrb  w8, [x0, #0xd0]       <- m_ignoreProgramChange
    tbz   w8, #0x0, ...
    mov   w0, #0x1; ret         <- true short-circuits, module() never touched
    ldr   x8, [x0, #0x1c0]      <- module()
    cbz   x8, ...               <- module() IS null-checked
    ldr   x0, [x8, #0x38]       <- module()->Container()  — NOT checked
    b     __ZN10CContainer22getIgnoreProgramChangeEv
```

`PatchParameter.cpp:1297`. The load-bearing gated fix is that second guard, in
`PatchParameter.cpp` — **not** `CContainer.cpp:1654`, which is what the row told
Jeff to change. With `this` already null the fault happens inside `GetPlug`,
before any guard added to `getIgnoreProgramChange`'s body could run. The
`CContainer.cpp` guard is still worth having; it is a different bug.

### The remaining hunt is much smaller than "which document state"

`m_ignoreProgramChange` **defaults to `true`** (`PatchParameter.h:307`), so the
deref is unreachable for almost every parameter. It needs one that is **false**:
`HC_PATCH_CABLES` and `HC_PROGRAM_CATEGORY` set it false in code, and the
Properties pane's `Ignore Program Change` toggle sets it false on anything —
and that toggle was on screen when the crash happened.

Who has a null `Container()` is not a guess either; `DocOb.cpp:40` says it, in a
special case commented *"for 'Main' container"*. In a live document that is
`<master_container handle="1920872816" name="Main">`, and four parameters already
point at it — all four carrying `ignoreProgramChange="1"`.

**Three crafted documents did NOT reproduce it, recorded so nobody repeats
them:** `Module="1"` (that is the DSP-side id, not the editor handle — did not
resolve); `Module="999999999"` (did not resolve; became **E46**); and
`ignoreProgramChange="0"` on a master-container host control — which **does not
survive import**, because the host-control factory re-asserts its own default.
That last one is the useful negative: the flip has to happen *after* load, which
is what the Properties toggle does and what the next run should drive.

**Also gone: both `.ips` files.** macOS rotated them, and
`~/Library/Logs/DiagnosticReports` now holds no TIDE report at all — so the
Accept's *"crash-report count before and after"* has no *before* left and needs
re-stating by whoever takes the row.

### Two traps worth the lines, both cost real time here

- **`otool -p` and lldb's `breakpoint set -n` spell the same symbol
  differently.** otool wants the Mach-O `__ZN10CContainer22...`; lldb wants one
  fewer underscore. Feeding otool's spelling to lldb sets a breakpoint that
  never resolves, and lldb reports that as **silence**, not as an error — it
  reads exactly like "the condition never occurred". And `lstrip("_")` is the
  wrong fix: it eats both underscores and fails the same silent way. `[1:]`.
- **`breakpoint list` before `run` always says `no locations (pending)`**, so a
  check for that string is a false negative on a breakpoint that resolves fine
  at launch.
## 2026-08-27 — linux — X2: the tree has 1,982 unique warnings and fifteen of them are ours (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** Jeff asked for a linux-only task. **X1 and X2 are the only `platform: linux`
rows and both carry a bare `BLOCKED` with no `(id)`.** Checked the blocker rather
than assumed it, took **X2**, and annotated X1 without touching its status.

### The blocked rows were stale, and the check was two commands

Both sit under **After the carve-out**, and the carve-out is finished — **C7 is
DONE**. The journal's own lesson covers this: *"a `BLOCKED` row with no stated
blocker is a claim nobody has retested."*

**X1's Accept is already met.** *"VST3 + CLAP on Linux, GCC 13+"* — this box is
**GCC 13.3.0** and the full tree has built both artifacts repeatedly today.
**Annotated, not flipped:** a status change on a row this run did not take is the
drive-by edit that makes a queue untrustworthy.

### The census, which is the actual finding

`-Wall -Wextra` over the whole tree:

```
12,255 warning LINES
 1,982 UNIQUE warnings
```

**A 6x inflation from headers being re-included**, so the number this row's
wording implies is not the number to work from. By origin:

| origin | unique | |
|---|---:|---|
| **SynthEditLib** | **1,547** | 78%, GATED |
| fetched `_deps` | 134 | |
| gmpi_ui | 132 | |
| GMPI_Wrappers | 112 | |
| GMPI | 26 | PR-GATED |
| third-party | 16 | |
| **TideSynth (ours)** | **15** | |

**The row reads like a mountain and TIDE's share was fifteen lines.** That is
the whole value of measuring before planning: "zero-warning build" sounds like a
sweep and is actually an afternoon, *for the part we own*.

### One of the fifteen was a trap

Five variables in `TiDEPanelGui.cpp` looked plainly unused. They exist only to
feed `TIDE_LOG(...)`, which is `((void)0)` when `TIDE_PANEL_TRACE_LOG=0` — so
**deleting them would have compiled fine here and broken the diagnostic build.**
They are `[[maybe_unused]]` instead.

The nine unused parameters are on overrides, where the name documents the
interface, so they are `[[maybe_unused]]` rather than unnamed.

**`monotonicMs()` is deleted, not silenced.** Its own comment said it was *"for
the settle timer below"* — but that timer waits on a chrono duration and never
polls a clock, so it had **no callers at all**. A comment naming a consumer that
no longer consumes is exactly what makes dead code look live, so silencing it
would have preserved the lie.

### Verified both ways, which is the control

| build | result |
|---|---|
| `-Wall -Wextra`, logging **off** | rc=0, **0 TIDE-own warnings** (was 15) |
| `-Wall -Wextra -DTIDE_PANEL_TRACE_LOG=1` | rc=0, **0 TIDE-own warnings** |

The logging build is the one a deletion would have broken, so running it is what
makes `[[maybe_unused]]` a decision rather than a guess.

**Not verified:** Windows and macOS were not built, and their compilers warn
about different things — MSVC in particular. A tree-wide zero is **not** claimed
and is not achievable from here: 78% of what is left is behind the GATED line,
so it is a decision about `SynthEditLib`, not a task.

**Learned:**

- **Count unique warnings, not warning lines.** 12,255 against 1,982 is a 6x
  difference and it decides whether a row looks impossible or looks like an
  afternoon.
- **Bucket by repo before planning.** 78% of this tree's warnings are in a GATED
  repo; without that split, "zero-warning build" is an unownable goal rather
  than one with a doable part.
- **An unused variable can be a live one in another build configuration.** The
  five here feed a macro that compiles away; the fix that looks obvious is the
  one that breaks the build nobody runs by default.
- **A comment claiming a consumer is evidence, and it can be stale.**
  `monotonicMs()` said what it was for; that thing had been reimplemented and the
  function had zero callers. Silencing it would have kept a false statement in
  the tree.
- **Two of three "blocked" linux rows were not blocked.** The bare `BLOCKED` with
  no `(id)` is the tell, and re-checking cost two commands.

**Machine left clean.** Two scratch build trees and four dependency worktrees,
removed. All six repos on their default branches and clean.

**Branch/PR:** `tide/linux/X2-wall-wextra` — TideSynth only: six source files,
the X2 and X1 rows, and this entry.

## 2026-08-27 — macos — E43: the command channel now always answers, and the fix is a heartbeat rather than a list of verbs (scheduled run)

**Prompt:** b97bc00 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.1** (there is no `claude` CLI on this box's PATH, so this is the desktop app's `CFBundleShortVersionString`, which is the version A13 recorded as the discoverable one on a mac) · as **tide-rack-bot** (both paths)

**Did:** took **E43**. Its Accept is met. The product change is a
`GMPI_Wrappers` PR off branch `tide/mac/E43-bounded-command-deadline`; TideSynth
carries the row, two new rows and this entry. Per A22 the rows and this entry
name the **branch** — the PR number does not exist until after the commit that
would cite it, and predicting one is how a placeholder reached `main` once
already.

### Why E43 and not the four TODO rows above it

Written down because the next run will walk the same list. `S1b` and `S8` are
wholly GATED and were re-measured yesterday — there is no non-gated half left to
do. `E25`, `E34`, `E39` and `E42` each have their cause found and their fix in
`SynthEditLib`, filed not written. `E7` is a fact Jeff has ruled is not a
blocker; `E2` is an umbrella whose own row says its Accept cannot be stated.

That leaves **E38** and **E19**, and both were passed over for reasons I put on
their rows rather than only here:

- **E38's Accept is unsatisfiable by construction on macOS**, which its own
  entry measured yesterday: it asks that `--screenshot` show the context menu,
  and `cmdScreenshot` reads the app's own render buffer while a macOS popup is a
  separate window. STEP 2 says to name what is missing and move on, so the row
  now carries `NEEDS-SPEC: a readout that can observe a native popup menu`.
- **E19's mac cell wants AU3 in a real host**, and the row itself says the
  command channel is standalone-only so a rack cannot be built inside a hosted
  instance. That needs a human at the keyboard, not a scheduled run.

### What was wrong, measured before anything was changed

`--pointer-down 29,13` on the `File` menu, on a stock `origin/main` build:

```
--pointer-down 29,13    no answer in 25 s
--info, 2nd connection  no answer in 25 s      <- the channel, not the command
--info, 3rd connection  no answer in 20 s      <- and it never comes back
kill -TERM              STILL ALIVE            <- only kill -9 recovers
```

**The control is what makes those zeros mean "wedged" rather than "bad
coordinate":** `--pointer-down 500,400` on the rack canvas answers in **0.01 s**
on the same build, and a second connection right after it answers too. E38's
entry made exactly this mistake in the other direction yesterday — two zeros
that meant two different things — and it cost an hour there.

### The fix: a second deadline, and a heartbeat rather than a verb list

`MainThreadQueue::run` had one deadline and it is on the job **starting**. The
menu job *starts*, then opens an `NSMenu` whose nested modal run loop runs
inside the job, so the item is already `kRunning`, the compare-exchange fails
and the old code fell through to an unbounded `future.get()`. Because every
transport dispatches inline on its single listener thread, that one blocked
command took the whole channel with it.

So a started job is now on a clock too — `kProgressDeadline`, 20 s — and the
clock is a **heartbeat**, not a total: a job that is genuinely working calls
`MainThreadQueue::heartbeat()`, which is one line in `--render-audio`'s block
loop and nothing anywhere else.

**The row proposed "a deadline on finishing for the input verbs", and I did not
do that, deliberately.** A list of verbs that may block ages silently as verbs
are added — this repo has the lesson twice already, in A4's path allowlist and
in A20's. Inverted, an unheard-of new verb that blocks forever simply gets the
bounded answer, and only a verb that is *legitimately* slow has to opt out. That
is the safe direction to be wrong in.

**The two answers say different things, and that is load-bearing.** The
start-deadline line still says *"this command was NOT run"*. The new one says
*"this command STARTED and has not finished ... It was NOT cancelled and may
still complete"*. Reusing the first wording would be a lie about a running job,
and it is the one kind of lie a caller acts on — by retrying an edit that is
about to land anyway.

**A race I had to close on the way, and it would have fired at once.** The
waiter can reach the post-start wait in the sliver between `drain()`'s state CAS
and its own beat stamp. A zero beat there reads as "stalled since the epoch" and
answers immediately, so the beat is seeded at **enqueue** as well as stamped at
**start**. Worst case is then a deadline measured from the enqueue, which is
still bounded and still generous.

### Measured — A/B, one build tree, one header different

```
                              origin/main        with the fix
--pointer-down 29,13 (File)   no answer in 25s   20.01 s  "started":true
--info, 2nd connection        no answer in 25s   20.03 s  (opened at t+5s)
--info, 3rd connection        no answer in 20s    5.01 s
--pointer-down 500,400        0.01 s              0.01 s  (control, unchanged)
--render-audio 60 s           0.11 s              0.11 s  (control, unchanged)
```

The second row is the honest shape of what this buys: that connection was opened
five seconds into the stalled command, waited the first command out, and then
served its own 5 s start deadline. **So the worst case for any command is now
`kProgressDeadline + kStartDeadline` = 25 s**, because dispatch is still inline
on one listener thread. Bounded, and it explains itself, which is all the row
asked for.

### The rescue half is not measurable on the app, so it has a probe

`--render-audio` is the only verb that can legitimately outlive 20 s — and
**TIDE's default rack renders its 240-second maximum in 0.11 s on this box**, so
there is no way to watch the heartbeat save anything from the app side. A hook
nobody has watched work is not a hook (V7).

`GMPI_Wrappers/tests/main_thread_queue_deadline_probe.cpp` drives the real class
with no GMPI, no plugin, no window and no build system —
`c++ -std=c++17 -O1 -o /tmp/p tests/main_thread_queue_deadline_probe.cpp` — and
reports **13 checks, 0 failures** in 72 s.

**And it can fail.** Recompiled against `origin/main`'s `run()` — the same probe
source, that header patched only to expose the two constants and add a no-op
`heartbeat()`, leaving the unbounded `future.get()` exactly as it stands — it
reports **3 FAILURES**, and they are exactly the three clauses that describe the
fix: not reported as STARTED, not released at the deadline, and the job had
already finished by the time the caller got anything.

Its sleeps are sized off `MainThreadQueue`'s own published constants rather than
off the numbers 5 and 20, so changing a deadline cannot leave the probe quietly
measuring the wrong thing. It is opt-in
(`-DGMPI_WRAPPERS_BUILD_TESTS=ON`) and **not in CI**, because it has to sleep out
two real 20-second deadlines.

### Not verified, stated rather than implied

- **The menu is still not drivable.** This row bought a bounded *failure*, not a
  working gesture. Filed as **E44** rather than left on a row about to close.
- **Windows.** `IpcServerWin.h` and a different menu implementation. The change
  is in the shared, platform-free `MainThreadQueue.h` so it applies there too,
  but nobody has run the measurement.
- **A wedged app still ignores SIGTERM**, on both binaries. The handler posts to
  a main thread that is inside the modal loop. E43's own `kill -TERM` save
  workaround therefore does **not** apply to an app whose menu is open — worth
  knowing, because the row presents that workaround as general.

### STEP 4 bookkeeping, and two holes it exposed

Four IN-REVIEW rows had all their PRs merged, each state read from `gh pr view`
rather than inferred from a merge commit. **M9**, **E33** and **E40** went DONE
and were **moved to `BACKLOG-DONE.md`**, which is what STEP 4 actually says to
do. **E32** did not: every linked PR merged, but the row says in its own words
that the macOS position half is still open, so DONE would be false and IN-REVIEW
is false once nothing is open. It is back to **TODO**, re-scoped in the row to
exactly the mac half.

**Nothing had been archived since 2026-08-20** — six days and roughly sixty
merged PRs — and `BACKLOG.md` had reached **723 KB**. That is A8 recurring at
nine times the size that triggered A8, in the file every run on three machines
reads to find the handful of rows it can act on. `check-backlog-diff.py`
*permits* an archive move and never requires one, so a DONE row left in place is
silently legal. Filed as **E45**, with this run's three moves as the worked
example: one scripted pass, 13 KB.

**And a hole underneath that one.** The `E43` row on `origin/main` was missing
its closing `|`, so `check-backlog-diff.py`'s row regex never matched it —
**that row did not exist as far as that lint was concerned**, and a run could
have rewritten or deleted it against a clean report. Terminated here.

I nearly wrote that up as *two* rows, S24 and E43. S24 ends with `| ` and a
trailing space, which my hand-rolled detector rejected and the lint's `\|\s*$`
correctly accepts. Measured with the lint's own regex instead: **1 invisible row
before, 0 after.** Checking with the regex that matters rather than one that
looks equivalent is the reusable half.

### Build and tree state

Full `cmake --build` of TIDE on macOS from a fresh tree: **27 targets, rc=0** —
standalone, VST3, AU2, AU3 + appex, CLAP, GMPI and the AUv3 container app.
`SynthEditLib` was taken **fetched at `origin/main`** rather than from the local
checkout, deliberately: that working copy is parked on
`fix-patchmanager-dangling-properties-observer` with `EditorLib/PatchManager.cpp`
modified, which is Jeff's work in progress and not mine to build from, commit or
revert. Left exactly as found. `GMPI`, `GMPI_Wrappers` and `gmpi_ui` were each
parked on an already-merged agent branch with clean trees and were returned to
`main`.

**Still red on `main`, still needs Jeff, unchanged from yesterday:**
`check-prefab-layout` fails on `AR_jef.synthedit` (an SE Label overhangs the
panel, introduced by `322df0f`). It needs re-saving in SynthEdit; no run can fix
it.

**Learned:**

- **A deadline on "did it start" cannot bound "did it come back", and the two
  failures are indistinguishable from outside.** The escape hatch existed, its
  own comment said why it could not fire here, and nobody had read that sentence
  against this case.
- **Invert an allowlist when the unknown case is the dangerous one.** "Which
  verbs may block?" ages silently; "which verbs are allowed to take their time?"
  fails safe for every verb nobody has written yet.
- **A bounded error must not reuse a wording that was true of a different
  failure.** "NOT run" and "started, not finished" call for opposite actions
  from the caller.
- **When the app cannot exercise the path, the probe is the deliverable — and it
  is only worth anything once you have made it fail.** Three failures against
  the old logic is what makes thirteen passes against the new logic mean
  something.
- **Check with the regex the tool uses, not one that looks equivalent.** A
  trailing space is the difference between a row a lint cannot see and a row it
  reads fine.
- **STEP 4's archive move is a written rule that nothing enforces, and it
  stopped happening six days ago without anyone noticing.** A rule with no check
  behind it decays at exactly the rate the file grows.

**Next:** E44 (a `--menu <action>` verb, so `File > Save` is reachable at all)
is the row that makes every *"save and reload"* Accept in this backlog
executable by a scheduled run on macOS. E45's sweep is mechanical and large, and
should be its own PR separate from its check.

**Machine left clean.** All six repos on their default branches; the two scratch
build trees (`build-e43`, and the tests-only configure) are under gitignored
paths; every driven standalone ran under an isolated `HOME` in the session
scratchpad and all of them are killed. Nothing written to Jeff's config, and his
`SynthEditLib` working tree is untouched.

**Branch/PR:** `tide/mac/E43-bounded-command-deadline` in both repos. The
`GMPI_Wrappers` branch is the change — `mcp/MainThreadQueue.h`,
`mcp/CommandDispatcher.cpp`, `tests/main_thread_queue_deadline_probe.cpp`,
`tests/CMakeLists.txt`. TideSynth carries E43's row, E44, E45 and this entry.
**Merging TideSynth's side alone changes no behaviour**, and merging the
wrappers' side alone leaves the backlog saying the work is open.

## 2026-08-26 — macos — S1b re-measured: the scan is NOT dead code, and the cut is smaller than the row says (scheduled run)

**Prompt:** "compact. continue".

S1b's Accept is *"constraint 7 verifiable from the Release binary — none of
those symbols present"*, which is a command, so I ran it rather than reasoning
about it. It still fails. What is worth the handoff is the two conclusions that
moved. Full working is addendum **C1–C6** of
[docs/module-enumeration.md](docs/module-enumeration.md).

**The scan is reachable in TIDE, and B1/B5 say otherwise.** They read as "the
code is linked but nothing calls it; the job is to stop compiling it".
`CContainer::OnEditToPrefab` (`CContainer.cpp:1920`) rescans the prefab folder
after writing a prefab — `RefreshModuleData(false,false,true)` at `:1967` →
`ScanFolder(getDefaultPath("syntheditprefab"), ...)` at `Application.cpp:547`.
`ApplicationBase::LoadOrScanModuleData()` genuinely has no caller in TIDE, which
is what S1a removed and what B1 checked; it is simply not the only door. **A
symbol having no caller at the entry point you removed is not the same as the
symbol being unreachable** — that is the reusable half of this.

**So the cut is the BINARY LOADER, not "the scan half".** On the prefab
extension list, `ScanFolder` only pushes filenames onto `PrefabFileNames`; it
reaches `ScanBundle` only for a *directory* named `*.synthedit`. Everything that
dlopens hangs off the `.sem,.gmpi` arms or off the SEM cache, which exists to
avoid rescanning binaries. That is a much smaller and better-defined cut than
B5's "make ModuleFactory_Editor.cpp compile without its scan half", it keeps
Save-as-Prefab working, and it is exactly the third-party half Jeff parked on
2026-08-26.

**The linker cannot do it, and I nearly spent a build finding that out the
expensive way.** Nothing sets symbol visibility anywhere — not
`gmpi_plugin.cmake`, not TIDE's `CMakeLists.txt` — so the Release VST3 exports
**6,781** globals and every one is a dead-strip root; `-dead_strip` on its own
removes nothing. `-fvisibility=hidden` + `-dead_strip` would strip, and still
cannot satisfy this row, because `ScanFolder` is reachable and calls
`ScanBundle` unconditionally on one branch. **That is read off the call graph,
not a build I ran**, and the row says so in those words.

**One correction to B1's symbol list.** `nm -u` imports `_dladdr` as well as
`_dlopen`/`_dlsym`/`_dlclose`, and `_dladdr` must stay — `BundleInfo.cpp:69`
uses it to find the plugin's own bundle. An Accept that says "no `dl*`" sends
the next run after a symbol that cannot be removed.

**And it is not a size story.** The whole family is 20,736 B of `__text` across
17 symbols against a 5,490,472 B binary — 0.4%. B4 quoted before/after byte
counts for stage (a), which invites that framing. The case for (b) and (c) is
§7.1 and what an AUv3 reviewer sees.

**Sequencing note that had gone stale:** B5 recommended riding along with C4.
C0 resolved 2026-08-08 and C4 completed 2026-08-13; S1b did not ride along, so
thirteen days later there is nothing left to wait for.

The change itself is GATED (`SynthEditLib/EditorLib/**`), so this run filed the
gated half rather than making it. Row stays TODO.

## 2026-08-26 — macos — E34 reproduces on macOS, and its document half is provable without ever saving (scheduled run)

**Prompt:** "compact. continue".

E34 carried Windows-only evidence. It now carries macOS evidence too, and the
fix is still GATED (`SynthEditLib/.../ConnectorView.cpp:365`), so the row stays
TODO — only the verification moved.

**What reproduces.** Fixture rack with one cable, MIDI-CV `PITCH` -> Oscillator
`Pitch`. `--drag 283,307 660,468 --steps 20` picks up the EXISTING end at the
Oscillator jack and releases in empty rack space. Twice, on two fresh launches.
Afterwards the cable is drawn from `PITCH` to the release point and stays there:
the release was swallowed, exactly as the row says.

**What does NOT reproduce, and it matters for whoever writes the fix.** The
title says the cable *"clings to the pointer until a second click"*. On macOS it
does not cling — it FREEZES at the release point. `--hover 900,250`,
`--hover 1000,600` and a second click at `900,250` each produced a screenshot
**byte-identical** to the one taken right after the drag. So the second click
does not end the gesture here either; the rack is just left with a cable drawn
to nowhere until something else forces a rebuild.

**The control that makes those md5s mean anything.** Three identical screenshots
in a row is equally consistent with "nothing changed" and "the screenshot is
cached". Clicking a module-browser entry in the same hung state DID change the
image. Byte-identical is evidence only once you have shown the same call
producing a different answer.

**"No rebuild fires" is now a number, not a claim.** The pair
`DSP structure changed, pushing N byte document` + `building rack from N byte
document` appears exactly once, at startup — a control launch that received no
input at all logged the same pair. Both drags left the log at 12 lines. An
earlier reading of this nearly went wrong: the pair is in the log *after* the
drag, and only the no-input control shows it was always going to be.

**THE PART WORTH STEALING: you do not have to save to prove the document did
not change.** `kill -TERM` runs the normal teardown (`StandaloneApp.cpp:304`
installs the handler, the tick closes the window, `:447` calls
`session.saveNow` unconditionally). `SessionState::saveNow` re-captures the
patch — `syncPluginState()` then `captureState()` — and skips the write ONLY
when the bytes equal the baseline taken at launch (`SessionState.cpp:504`).
`session.xml` was not rewritten: same mtime, same md5, and no
`Session state: ...` status line. So the app looked, and there was nothing to
save. **A quiet session.xml is positive evidence, not a failed save** — provided
you check for the status line, which is the only other way that path stays
silent.

**E43 filed: one click on `File` wedges the command channel permanently.**
`--info` answers in 0.0 s; `--pointer-down 29,13` on `File` returns nothing in
25 s; a fresh connection sending `--info` afterwards also returns nothing in
25 s; the process sits alive and idle at 0.5% CPU. Two causes, both documented
in the code that has them:

  * `MainThreadQueue::run` has a 5 s busy escape hatch, and it cannot fire here
    because *"the deadline is on the job STARTING, never on it finishing"*
    (`MainThreadQueue.h:74`). `onPointerDown` over the menu bar opens a native
    `NSMenu` and the nested modal run loop runs INSIDE the job. The item is
    already `kRunning`, so `run()` falls through to an unbounded `future.get()`.
  * Dispatch is inline on the single listener thread — *"one plugin, one command
    at a time"* (`IpcServer.h:622`) — so the next connection is never accepted.

That is why the measurement is two 25 s waits and not one: the second one is the
whole channel, not the menu. Only `kill -9` recovers. Consequence: every Accept
phrased "save and reload" is unreachable by pointer on macOS, and a run that
tries it HANGS rather than fails, which reads as a crashed app.

**Also this run:** PR #479 (E40) had gone CONFLICTING behind #477 and #478;
rebased onto `origin/main`, keeping #477's E39 row and this branch's E40 row,
and putting E40's journal entry above E39's. `check-commit-authorship.py` then
blocked the push — the rebase turned an already-pushed Jeff-authored commit back
into an unpushed one, which flips that check from advisory to blocking by
design (A26). `gh api user` here is `JeffMcClintock`, not `tide-rack-bot`, so
the right answer was `--expect "Jeff McClintock"`, which the script's own usage
recommends, rather than re-authoring.

**Still red on `main`, still needs Jeff:** `check-prefab-layout` fails on
`AR_jef.synthedit` (SE Label overhangs the panel, introduced by `322df0f`). It
needs re-saving in SynthEdit; no run can fix it.

## 2026-08-26 — macos — E40: a deleted prefab kept shipping, and `rm` was only half the fix (scheduled run)

**Prompt:** "merge PRs in order" / "the continue looping over tasks".

E40 is my own row, filed after the second CI break in one day caused by the same
thing. It names the cause as `copy_directory_if_different` MERGING and never
deleting. **That is true and it is only half of it**, which I found by running
the row's own Accept instead of trusting my edit.

**The intermediate result is the whole lesson.** I added `rm -rf` before the
copy at both staging sites, rebuilt, deleted a prefab, rebuilt again without
clearing the tree — and the staged count stayed at **5**. Not because the delete
failed, but because **the step never ran**: it was a `POST_BUILD`, and a
POST_BUILD only fires when its target RELINKS. Editing `RackModules/` touches no
source, so nothing relinked and nothing re-staged. The merge was never reached.

Had I shipped after the edit and a green build, the row would have looked fixed
and the next prefab deletion would have broken CI exactly as before.

**So the per-target staging is now an always-run `add_custom_target(... ALL)`
that the format target depends on**, mirroring `${PROJECT_NAME}_stage_resources`
which already worked that way. It runs BEFORE the format target and makes its
own directory, so it needs no bundle to exist yet — and running early keeps it
out of the POST_BUILD ordering that `copy_plugin()` and the AU3 assemble step
already contend over, which is the other reason not to just add another
POST_BUILD.

**Both halves are needed and neither alone passes the Accept:** always-run makes
the step happen; `rm` before `copy_directory` makes it able to remove.

```
rm only, still POST_BUILD   staged 5   gate passes against a stale bundle
always-run + rm, 1 deleted  staged 4   FAIL 4 rack prefab(s) seeded, expected 5
prefab restored, rebuilt    staged 5   rack is populated
```

**Both sites changed** — the shared target (Windows/Linux/GMPI/CLAP) and the
per-target bundle arm (macOS). The iOS arm has done rm-before-cp since M11 and
is untouched; it needs its own generated shell script for
`${EFFECTIVE_PLATFORM_NAME}`, which is a different problem.

**Left alone deliberately:** the module XMLs are still individually
`copy_if_different`, so dropping one from `_tide_xmls` would strand the old
file. Same class — but that list is a CMake variable rather than a directory
scan, so removing one is a deliberate code edit, and the gate asserts the exact
four by name. Recorded rather than fixed speculatively.

## 2026-08-26 — macos — E39's prime suspect is wrong: the top strip is not a constant (scheduled run)

**Prompt:** "continue".

E39 reports the rack's top row as a short strip and names a prime suspect:
`kRackViewDips = 1008` is 2.625 rows of 384, and *"0.625 is close to the 0.68
measured — so the leftover is the prime suspect"*. That is a good hypothesis
and it makes a testable prediction: **the fraction should be the same every
time.**

**It is not.** Four screenshots off one build, each self-calibrated by measuring
its own rail pitch (which comes out at exactly 384 DIP in every one, so E5's row
height is not in question):

```
stored centre 3984 @ zoom 1.000   top strip 0.14 of a row
stored centre  940 @ zoom 0.745             0.27
stored centre 1353 @ zoom 1.000             0.29
stored centre 1349 @ zoom 0.381             2.16
(windows report)                            0.68
```

A canvas-height remainder is a property of the canvas and cannot vary with
scroll position. **So it is not the constant, and changing `kRackViewDips` to a
multiple of 384 would have produced no change and a wasted session.** That is
the whole value of this entry.

**What it actually is.** `TopView::renderRack` lays the case interior and its
rails out from the RACK ORIGIN every `rowHeight`, across whatever clip rect it
is handed — not from the top of the canvas. So the partial row at the top of the
window is just where the viewport sits relative to that grid, and **every freely
scrolled position shows one**. I checked the strip really is drawn as rack
rather than as background: its luminance is identical to the case interior
between rails, 27.7 in both. That is the row's *"no rails above it"* turned into
a number.

**This makes half the Accept unachievable as written.** *"Every rack row is a
full 384 DIP with rails above and below"* cannot hold while the view scrolls
freely — you would have to snap the viewport to row boundaries, which fights E33
(open where the document says) and would make panning feel notched. The
achievable half is the row's own alternative: **give the case a top** and stop
painting rack interior above row 0. That is in `renderRack`, which is GATED.

**A note on method, since I nearly measured the wrong thing twice.** My first
detector sampled a column band at canvas x 1700..2100 and found no rails at all
— that is outside the rack pane. My second used a hard-coded px-per-DIP from the
nominal zoom, which is wrong because `calcViewTransform` QUANTISES zoom so that
12 DIP maps to whole pixels. Deriving px-per-DIP from the measured rail pitch
instead makes each screenshot calibrate itself and removes both mistakes at
once. **When the thing you are measuring has a known period, use the period as
the ruler.**

## 2026-08-26 — macos — E29: the mac box is fine, and the obvious fix would break it (interactive, Jeff directing)

**Prompt:** "continue", then — on seeing a headless render stall —
*"blocked on a plugin-not-available dialog"* and *"drive it interactvly"*.

He was right on both counts, and driving it interactively is what turned a
plausible inference into a screenshot.

**THE QUESTION THE ROW LEFT OPEN IS ANSWERED: the mac box is NOT affected.**
REAPER **7.45** loads the committed raw-TUID token and renders `v1-rack.rpp` at
**peak -6.3 / rms -17.0 dBFS** — M7's and E2a's figure exactly. Negative control
first, as the harness itself instructs: `v1-rack-uncabled.rpp` -> `-inf`,
SILENCE. So the harness discriminates and the -6.3 means something.

**THE FINDING THAT CHANGES THE PLAN is the other direction.** Substituting the
Windows box's 7.78 token into the same fixture makes REAPER 7.45 refuse it:

```
Project Load Warning
The following effects were in the project file and are not available.
    Track 1: VST3i: TIDE Rack (TIDE Synth)
```

and the FX slot reads *"could not be loaded"*. **The two tokens are mutually
exclusive across these versions**, so this row's first repair option — *"a
re-save from a current REAPER"* — would fix Windows and break macOS. Ruled out
unless the fleet first agrees a single REAPER version. Worth knowing before
anyone tries: this mac is on 7.45 and REAPER is offering **7.79**, so upgrading
it casually would flip it to the Windows behaviour and break every committed
fixture here.

**IT FAILS AS A HANG, NOT AN ERROR — the part that would have bitten CI.** The
warning is modal, so `-renderproject` blocks on it forever. My first attempt sat
**over seven minutes** producing no render and no error, and I killed it by
hand. `render-and-measure.py` had no timeout at all. It does now: 300 s, kills
REAPER, and writes into the log exactly why and where to read about it. I fired
that path deliberately with a 25 s cap to prove it works (`rc=-9999`, REAPER
gone, message present) rather than trusting that I had written it correctly, and
re-ran the control and `v1-rack.rpp` to show nothing else moved.

**A METHOD NOTE WORTH KEEPING.** My second headless attempt reported SILENCE
rather than hanging, which nearly sent me down the wrong path — "the plugin
loads but makes no sound" is a completely different bug from "the plugin does
not load". Driving REAPER by hand settled it in one screenshot. **A headless
harness can only report what it can see, and a modal dialog is invisible to it;
the two failures it collapses into "no audio" are not the same failure.**

**Delivered:** the row's SECOND option verbatim — a note in
`tests/hosts/README.md` naming both tokens, which REAPER writes which, the
one-line `sed` for a LOCAL copy, how to discover what your own REAPER writes,
and the hang.

**CLOSED BY JEFF THE SAME HOUR — WONTFIX.** *"this product is not released. We can break DAW
sessions, they only exist only for our tests anyhow"*, then *"we simply don't care about broken
test sessions. don't waste time on it."* The escalation is WITHDRAWN rather than answered, and
that is the right call: the fixtures are instruments, not deliverables, and a one-line `sed`
unblocks any box that needs one. **The lesson for me is about proportion** — I had a NEEDS-JEFF
row with a default and a decide-by drafted for a question whose real answer was "this does not
matter". The measurement was worth ten minutes; the escalation machinery around it was not.
Two things survive and are worth keeping on their own merits: the README recipe, and the
harness timeout, which bounds a hang on ANY modal dialog rather than just this one.

**The Accept is half met and the row says so.** *"loads its plugin on all three
boxes"* is not true and no commit here can make it true without breaking macOS.
E29 is now NEEDS-JEFF with a default in effect (per-box local swap) and a
decide-by (before the next multi-box REAPER-rendered measurement), so an
unanswered question cannot quietly become the answer.

## 2026-08-26 — macos — the restored view lands 240 DIP off, and it is not the re-save (interactive, Jeff reporting)

**Prompt:** *"i re-saved defaultrack."* then, on seeing the result,
*"might depend on how big the window is a bit. but that looked wrong"*.

**He was right that it looked wrong, and right to be unsure why.** Window size
DOES change what fits — TIDE's rack pane is ~340 DIP narrower than the window
because the browser and properties strips eat the sides — so "it looked right
when I saved it" and "it looks wrong on a default window" can both be true. But
that is not what happened here.

**THE MEASUREMENT: three runs, two zooms, one constant.**

```
stored centre 1353 @ zoom 1.000  ->  applied 1593   (+240 DIP)
stored centre 1253 @ zoom 1.000  ->  applied 1493   (+240 DIP)
stored centre  940 @ zoom 0.745  ->  applied 1260   (+320 DIP)
```

240 x 1.0 and 320 x 0.745 are both **~478 WINDOW pixels**. A constant in window
space and not in document space is what rules out "the document is wrong" and
points at the transform. The applied centre is derived from where `TiDE Output`
(doc `1329..1377`) actually lands in the pane, so it is read off pixels rather
than inferred.

**THE CAUSE, one line.** `TopView::calcViewTransform` (ViewBase.cpp:1380):

```cpp
const Point canvasCenter{ (drawingBounds.right - drawingBounds.left) * 0.5f, ... };
```

That is half the pane's **SIZE** where it means its **MIDPOINT**. The transform
is then consumed in WINDOW space, where the pane begins at
`drawingBounds.left` — so the error is exactly `drawingBounds.left`, and
`SynthEditGui.cpp:463` sets `editorContentRect` window-rooted from
`editorStrip.left`: the two browser strips, **240 DIP**. The vertical axis
carries the same error of `drawingBounds.top`, ~11 DIP, small enough to have
gone unnoticed for as long as the horizontal one.

**Why it surfaced today.** The bug is old and was harmless while the centre was
meaningless: TIDE passed a hard-coded `kRackViewDips / 2` and nobody could tell
it was 240 out. **E33 made the stored centre load-bearing**, and the very next
person to save a framing they liked hit it. Filed as **E42**; the fix is
`(left + right) * 0.5f` and it is in GATED SynthEditLib, so it is filed, not
written.

**A detour worth recording, because it nearly became a wrong conclusion.** My
first pass at measuring this reported "no module visible at all" for two of the
three runs. That was my detector, not the app — it searched canvas columns
960..1620 while the module had been pushed to 520..615 by the very offset I was
trying to measure. **The bug moved the thing out of the window I was looking
in.** I only caught it by opening the screenshot and looking. An automated
readout that assumes where the answer will be cannot measure a displacement.

**E37 is now BLOCKED(E42)** and should probably close once E42 lands: Jeff's
re-save fixed the zoom, he deliberately did not move the modules, and the only
remaining complaint is the 240 DIP that belongs to E42. **He should not re-save
the file again until E42 is in** — the framing that looks right while saving
will keep reopening wrong.

## 2026-08-26 — macos — third-party modules are parked; the cluster goes with them (interactive, Jeff ruling)

**Prompt:** *"E24: 3rd-party module compatibility is not important at this
stage. We ship with only our own modules."*

Recorded in [docs/decisions.md](docs/decisions.md) and applied to every row it
reaches, so nothing is left looking takeable that is not.

**`BLOCKED`, not `WONTFIX`, and the distinction is the whole of the
bookkeeping.** *"At this stage"* is a sequencing call, not a rejection. WONTFIX
carries "do not re-file this", which would be wrong and would throw away real
work: E20's portability measurement, E22's licensing research, E23's render-path
correction and E24's screenshot all keep their value if this is revisited.

Parked: **E20, E21, E22, E23, E41**. Closed: **E24** — its PR merged and the
ruling makes its question moot as well as answered.

**THE PILOT PAID FOR ITSELF ON THE DAY IT LANDED, which is the pleasing part.**
E24 existed to stop E20-E22 proceeding on a guess about whether ported modules
render. It returned a number — **two of Crackle's four controls invisible**,
the knob and the switch drawing nothing — hours before the call was made. That
turned "should we ship third-party packs?" from a taste question into one with a
price attached. Whether or not it changed the answer, it made the answer cheap.

**A contradiction I made sure to leave behind rather than tidy away.** E23
states, from reading the code, that *"`drawKnobs()` draws rim, body AND
pointer"* and that *"a panel that is labels-only still gets grabbable
controls"*. E24 measured the opposite on a running rack. One of them is wrong.
Parking the rows would have buried that, so E23's row now carries the
contradiction explicitly and says whoever revives it starts by reconciling the
two, not by trusting either. A code reading and a screenshot disagreeing is
exactly the thing a future reader deserves to be warned about — and it is the
same failure mode E23 itself was filed to correct, in the other direction.

**What the ruling deliberately does NOT touch,** written down so a later run
does not over-apply it: the `TIDE_VCV_FUNDAMENTAL` / `TIDE_VCV_HETRICKCV` CMake
options stay (both default OFF, both harmless — deleting build capability is
more than the ruling asks); the adaptor repos are untouched; and **E2/E16's
curated set is unaffected**, because that is TIDE's own modules and is precisely
what the ruling says to concentrate on.

`docs/vcv-permissive-modules.md` gained a banner, because a research document
that reads as a plan is how a parked decision quietly restarts.

## 2026-08-26 — linux — E32's size half: the standalone reopens where it was, and the save had to move before closeWindow() (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** took **E32** scoped to the **size** half. Product change is
[GMPI_Wrappers#20](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/20) —
one file, +92 lines. **Position is not done**, deliberately, and the row is
IN-REVIEW rather than closed because of it.

### Why size here and position elsewhere

Size is portable, so it belongs in `StandaloneApp.cpp` where all three shells
get it at once. Position is not: **xdg-shell has no set-position**, so a Wayland
client can never place its own window. That is a property of the protocol rather
than a gap to close later, which is why the Linux half of this row was always
going to be smaller than the other two — and why doing the shared half from this
box is the useful contribution rather than a partial one.

`SessionState.h` already had the rule this follows: *"Window size and position
are the shell's business, not the plugin's, and are not kept here."* So the keys
go in `standalone.conf`, beside the device selection, and the patch stays clean.

### The trap the row did not name

The row lists three. A fourth turned up by running it: **the save has to happen
before `closeWindow()`.**

Afterwards the Wayland shell reports **0x0** — the frame is gone. Writing that
would persist a size the next launch rejects, so it would silently fall back to
the default **and look exactly like the feature had never worked**. The failure
is invisible in the code and obvious the moment you read the file.

### One bound, both directions

The sanity check is shared by the read and the write, so a size this build
refuses to restore is also one it refuses to save — the file cannot accumulate
values that are silently ignored forever. It only rejects nonsense; clamping to
the real minimum stays `setMinimumClientSize`'s job, which runs on **every**
path including the ones that never read a file. Duplicating that clamp here
would have been a second place to get it wrong.

### Verified — eight cases, headless weston

| case | result |
|---|---|
| first run, no config | **1100x626** (editor default) — unchanged |
| quit | conf gets `window.width=1100`, `height=626` |
| conf hand-set 900x500 | reopens **900x500** |
| conf zeros | 1100x626 |
| conf **width only** | 1100x626 — both or neither |
| conf `99999 / -7` | 1100x626 |
| unknown key alongside | **preserved** across the save |
| reopen 960x540, then quit | conf gets **960x540**, not the default |

**The last row is the one that matters.** Every other restore case proves the
read; only that one proves the *write* follows the live window rather than
re-writing the editor default — which is the failure mode that would have
shipped silently.

**Not verified:**

- **An interactive resize.** A real xdg-shell resize is a compositor gesture
  this harness cannot send, so the live-size path is demonstrated by restoring a
  non-default size and reading it back at quit. That is a proxy and I am calling
  it one.
- **Windows and macOS were not built**, and **both still need their position
  half.** The row is not complete and is marked IN-REVIEW, not DONE.

**Learned:**

- **"Save on shutdown" has an ordering, and the wrong one fails silently.**
  Reading the window after `closeWindow()` gives 0x0, which the next launch
  rejects — so the bug presents as "the feature does nothing" rather than as an
  error. Save before you tear down, and check the file rather than the code.
- **Share the validity bound between read and write, not the clamp.** One
  predicate means the file can never hold a value this build ignores; copying
  `setMinimumClientSize`'s job in would have been a second place to drift.
- **A protocol limit is a scope decision, not a TODO.** Wayland cannot position
  a window, so the linux slice of this row is *complete at size* — worth saying
  plainly so nobody files a follow-up to "finish" it here.
- **`git merge` into a worktree, then editing before resolving, corrupts the
  edit.** I ran a scripted row update while `BACKLOG.md` still had conflict
  markers. `git reset --hard origin/main` and redoing it took a minute; noticing
  took longer than it should have because the lint I ran first was on the *other*
  file.
- **Backticks in a `--body` argument are shell-interpreted.** Three lines of my
  first PR body ran as commands. `--body-file`, or `gh api -X PATCH` with JSON.

**Machine left clean.** Headless weston stopped, standalone stopped, scratch
`HOME`s throughout; nothing written to Jeff's config. All six repos on their
default branches and clean.

**Branch/PR:** `tide/linux/E32-window-size` in both repos —
[GMPI_Wrappers#20](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/20) is
the change; TideSynth carries the row and this entry. **Merging TideSynth's side
alone changes no behaviour.**

## 2026-08-26 — macos — E24: a HetrickCV module on TIDE's rack, looked at for the first time (scheduled run)

**Prompt:** "i merged stuff, sync repos, continue."

E24 has been carrying predictions from source and rendered SVGs since it was
filed, and says so: *"nothing here has been observed in TIDE"*. It is now
observed. **Crackle**, the row's own suggestion, inserted into a running rack.

**THE ANSWER IS NEITHER OPTION THE ROW OFFERED, and the third one is more
useful.** Not "the panel carries its components" and not "it renders bare":

```
Crackle declares 4 controls          TIDE draws
  createHCVKnob(28, 87)                NOTHING
  createInput<PJ301MPort>(33, 146)     drawn
  createParam<CKSS>(37, 220)           NOTHING
  createOutput<PJ301MPort>(33, 285)    drawn
```

The panel's own art is fine — title, the concentric motif, all four labels, the
HETRICK logo. **Two of the four controls are simply absent**, and the switch's
absence is conspicuous because the panel paints `Classic` and `Broken` with an
empty gap between them.

**How I got positions rather than impressions.** The module is at doc
`l=1352 t=280 r=1442 b=664`, 90x384 DIP over a 380-unit VCV panel, at zoom 1 and
scale 2, so `canvas_y = 255 + vcvY * 1.0105 * 1.955`. That puts the knob at
canvas 427, the input jack at 543, the switch at 689 and the output at 818 — and
each prediction is checkable against the screenshot. Reading a screenshot without
that arithmetic gets you "looks mostly fine", which is what the row already had.

**Why jacks and not knobs, which is the mechanism worth keeping.**
`RackEditor.h`'s warning is specifically about knob CAPS: it draws "only the
moving part — an indicator line per knob" and relies on the panel for the body.
HetrickCV's panel does not carry knob bodies, so the knob renders as nothing at
all — not even an indicator I can see. Jacks are unaffected because TIDE draws
those itself (E23's correction). **Switches are a THIRD case that neither E24 nor
E23 names:** `CKSS` is not a knob and not a jack, and nothing draws it.

**Verdict: (b), narrowed.** A Crackle on the rack is legible and half-usable, so
this is a cost rather than a wall, and the fix is the one the row anticipated —
`RackEditor` drawing TIDE's own `TiDEknob` at the reported position, plus a
switch case. TIDE ships the knob already.

**A separate bug fell out, filed as E41.** The inserted Crackle landed
overlapping `TiDE Output` and **off the 3-DIP snap grid** — so E36's
next-free-slot placement did not run for an adaptor-registered module. My first
instinct was "AddPrefab's diff returns empty for a non-prefab", which is
plausible and does NOT explain the off-grid position: the fallback is
`snapToGrid(getCenter())`, which cannot produce an off-grid result. So something
earlier in the path differs too, and the row says to instrument rather than
guess. I have written down the hypothesis and its own counter-evidence rather
than the hypothesis alone.

**One instrument note:** seeing the module at all required parking the view on
it, which required E33 (unmerged). The render finding does not depend on E33 —
but on `main` today you cannot look at an inserted module, which is worth
knowing before anyone tries to reproduce this.

## 2026-08-26 — macos — E38: the flag was the easy part, and it was not the problem (scheduled run)

**Prompt:** "i merged stuff, sync repos, continue."

**E38 stays TODO. The flag is written and proposed; its Accept is not met, and
the remaining work is a different shape than the row assumed.**

E38 reasoned by analogy: `--double` was one flag, it made an entire row (E36)
measurable in a script the next day, and a right-click looked "the same size".
It is not, and the analogy is exactly what made it look small.

**What is done.** `--right` sets `PointerFlags::SecondButton` — which does
exist, discharging the row's "check first whether the flag even exists"
(`gmpi_ui/helpers/NativeUi.h:94`, `0x20`). It REPLACES the primary rather than
joining it: a real right-click reports SecondButton alone, and a widget testing
for FirstButton would otherwise read one gesture as both a context menu and a
selection change.

**What is not, and why it cannot be a flag.**

- the menu is raised by `DrawingFrameCommon::doContextMenu(point, flags)`, on
  the **FRAME**;
- `cmdPointer` calls `context.inputClient->onPointerDown(...)` — the **INPUT
  CLIENT** — and never touches the frame;
- and `doContextMenu`'s own comment says macOS deliberately does not call it
  from the shared path (*"Doing it here too gave macOS two presses per
  right-click"*), so on macOS the menu comes from the Cocoa view's right-mouse
  handler, further still from the channel.

**THE MEASUREMENT, AND THE CONTROL IS THE WHOLE POINT.** A right-click that
changes nothing looks identical to a right-click that missed. So:

```
LEFT  click at (286,390)  ->  96543 pixels changed   (selects the module,
                                                      opens the properties pane)
RIGHT click at (286,390)  ->  {"right":true}, ZERO pixels changed
probe in populateEditorContextMenu -> ZERO hits
```

Without the left-click control the zero would have read as a bad coordinate,
and I would have spent the next hour hunting DIP conversions. I nearly did: my
first attempt clicked (700,400), got zero, and the left-click control there was
*also* zero — because it was empty canvas, where a left click legitimately does
nothing. Two zeros meaning two different things. Moving to a point where the
control was non-zero is what made the second zero mean something.

**And the Accept's instrument was never going to work either.** It asks that
`--screenshot` show the menu. `cmdScreenshot` reads `context.framePixels` — the
app's **own render buffer** — and a macOS popup is a separate window, so a
native menu could not appear in it at any point. Whoever re-attempts this wants
a probe in `populateContextMenu` or the menu model, not a picture.

**What is actually left:** a VERB, not a modifier, calling `doContextMenu` or
the platform equivalent, which means exposing the frame to the dispatcher.
Larger than the row assumed. V7's on-screen half stays unverified meanwhile,
which is honest and recorded on both rows.

## 2026-08-26 — macos — E33: TIDE was throwing away the view the document stored, and it cost an empty rack (scheduled run)

**Prompt:** "i merged stuff, sync repos, continue."

**I went to take E37 and found E33 underneath it.** E37 says the rack origin,
the content and the viewport are in three different places. Chasing which,
after V6, produced a much sharper answer: TIDE never applied the document's
stored pan and zoom, and V6 had just moved the default content somewhere that
made the omission fatal.

**THE OUT-OF-BOX EXPERIENCE ON `main` WAS BARE RAILS.** A fresh standalone drew
an empty rack. The rack was loaded the whole time —
`check-rack-populated.py` says `default rack loaded, 24894 byte document` and
the gate passes — and `DefaultRack.synthedit` even stores the view that would
show it, `PanelLocationCenter (1349, 284)`. TIDE discarded it on every open.

That is precisely the trap E33's own row records costing a windows run most of a
session, and the reason for its rule: **a claim of absence needs a trace or a
document dump, never a screenshot.** I nearly filed "the default rack does not
render" off a screenshot before checking the document.

**THE FIX IS THE ONE LINE THE ROW PROMISED, AND THE OLD LINE WAS IN THE WRONG
UNITS.** `viewOb->setCenter({ kRackViewDips / 2, kRackViewDips / 2 })`.
`kRackViewDips` is 1008 — the rack view's **size in DIPs** — and half of it, 504,
was being used as a **document coordinate**. The canvas is 7968 across,
`CContainer`'s default centre is (3984, 3984), and the rack origin is the panel
rect at (3732, 3732). So the viewport was parked **3400 DIPs from the rack**.
That is why this is not a like-for-like swap and why a blank document is
*better* off after it: (3984, 3984) is 252 DIPs from the rack origin.

Only the RESTORE was ever missing. The persist half already worked: `ViewBase`'s
pan and zoom handlers call `Presenter()->SetViewCenter`/`SetPanZoom`,
`MfcDocPresenter` writes them into the container, and they serialise.

**MEASURED THREE WAYS, because "the picture looks better" is not a measurement:**

1. **Same build, same document, only the stored view changed** — `(1349,284)@0.38`
   against `(3984,3984)@1.0`: **20.1% of the rack canvas differs.** The view
   demonstrably follows the document.
2. **Before vs after on the shipped default rack: 20.6% differs** — bare rails to
   a drawn module.
3. An injected `center=(400,3900) zoom=1.0` survives launch+quit **exactly**.

**A metric I threw away, recorded because it was tempting and wrong.** My first
readout counted "light pixels in the rack canvas", expecting more of them once a
module appeared. It went DOWN, 4.69% to 2.69% — because the rails are light too,
and the after-shot is zoomed to 38% so there is less rail on screen. The metric
measured rails, not modules. A pairwise diff between the two shots says what I
actually wanted to know.

**E33's own coupling rule, honoured.** The row says whoever moves second must
re-run the other's Accept, because `setCenter` existed to feed
`AddModule(id, getCenter())`. E36 has since replaced that with a next-free-slot
search, so the coupling is weaker than the row assumed — but I re-ran E36's
Accept anyway: **`ok  no overlaps among 6 placed module(s)`**, every insert
`on-grid  fits row`, only the same pre-existing off-grid `MIDI In`.

**WHAT THIS EXPOSES NEXT, and I did NOT fix it.** The shipped default rack puts
its two modules **1341 DIPs apart on rack row −9**, while the rack's panel rect
is **480 wide at row 0** — outside the rails and 2.8x wider than the window a DAW
would export. The file compensates with `PanelLocationZoom="0.38146973"`, which
is why the view is now legible but tiny, and which looks like an artifact of
wheel-zooming while authoring. That is a DATA change to V6's file, landed hours
earlier from another box, and re-authoring someone's shipped default is a
product decision rather than a bug fix. E37 is re-scoped to exactly that and
nothing else.

**Not verified, stated rather than implied:** a pan driven by a real user
gesture — the command channel has no wheel verb, the sibling of E38's missing
right-click — so the persist leg is verified by code path and document
round-trip, not by scrolling; and a DAW project, which the Accept also asks for.

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
