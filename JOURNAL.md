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
**Learned:** anything the next run would otherwise rediscover the hard way.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---

## 2026-08-21 — linux — S23: the session file is innocent, and the kernel had both crashes logged

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** ran the one experiment the previous run left written but unrun, got a clean
result — and then found the thing that actually answers S23 sitting in
`journalctl`, which no run had read. Also filed **S32**, because `gnome-shell`
segfaulted a fourth time and took Jeff's desktop down mid-run.

### The experiment: the suspect file loads fine

`~/.config/TIDE Rack/session.previous.xml` (17,866 bytes, md5
`010ed62b3baa7bbdda95ee935108d6e7`) copied to `<scratch>/TIDE Rack/session.xml`,
no `session.loading` beside it, launched as
`XDG_CONFIG_HOME=<scratch> gdb -batch -ex run -ex bt --args ./TIDE_STANDALONE`.

**Ran ~3 minutes, no signal.** Three things say the restore *succeeded* rather
than merely failing to crash:

| check | result |
|---|---|
| quarantine fired? | **no** — the scratch dir has no `session.previous.xml` afterwards, so `parametersAreReadable` passed and `host_.restoreState()` returned true |
| did the app write its own state back? | **yes** — scratch `session.xml` 17,866 → **17,346** bytes, new mtime; the debounced save only runs in a healthy session |
| does the rack draw? | **yes** — screenshot over the command channel, modules and patch points visible |

**Jeff's own config was byte-identical before and after** — both md5s unchanged.
`XDG_CONFIG_HOME` isolation does what `StandaloneSettings.cpp:49` promises.

### The kernel had both original crashes, with an offset

```
Aug 20 17:32:58 TIDE_STANDALONE[23881]: segfault at fffffffffffffff8 ip 000062312e4f7627 ... in TIDE_STANDALONE[3b4627,62312e1cb000+404000]
Aug 20 17:33:27 TIDE_STANDALONE[23924]: segfault at fffffffffffffff8 ip 000055bae7342627 ... in TIDE_STANDALONE[3b4627,55bae7016000+404000]
```

**Same module offset `3b4627` both times, under different ASLR bases.** So this
is **one deterministic code site**, not a random smear — which is the single most
useful fact anyone has produced about S23, and it was free.

`0xfffffffffffffff8` is **-8**, and `error 5` decodes as a **user-mode READ of a
mapped page**. That is the signature of reading 8 bytes *before* a null-ish base
— `back()` on an empty container, `*(--it)` at `begin()`, a length stored ahead
of a null data pointer — **not** a plain null `->field`.

**And the 29-second gap proves the session file is innocent, independently of the
replay.** Crash 1 at 17:32:58 leaves the breadcrumb, so the *next* launch
quarantines the file — `session.previous.xml` is stamped **17:33**, exactly
between the two — and comes up at defaults. **That defaults run crashed anyway at
17:33:27, at the same offset.** So the second crash was not loading the
quarantined file at all.

Two hypotheses die and one hedge resolves:

- **"the bad session file is the crash input"** — dead, from both directions.
- **"the 28 clean runs were runs of an app that had thrown the file away"** — the
  mechanism is real, but it is not why S23 stopped reproducing, because the
  crash recurred *after* the quarantine.
- **`session.previous.xml` IS a quarantine artifact**, not the `File > Revert to
  Plugin Defaults` false positive the row hedged about — its 17:33 stamp sits
  between the two crashes. (`keepCurrentAside` is reachable from exactly one
  place, `StandaloneApp.cpp:353`, armed at `:180`.)

**Not reproduced since:** zero `TIDE_STANDALONE` segfaults in `journalctl` after
2026-08-20 17:33 — across the previous run's 28 controlled runs and ~1000 driven
insertions, and this run's replay.

### A lead I am labelling unreliable, on purpose

`addr2line` on **today's** binary at `0x3b4627` gives `CContainer::OnEditContain()`
at `SynthEditLib/EditorLib/CContainer.cpp:2338`, and `:2330` there does use a
`dynamic_cast` result with no null check. Tempting — `OnEditContain` is the
unlock-a-container path, which is exactly the rack gesture.

**It is probably a coincidence and should not be spent on.** It is a *different
binary* from the one that faulted, and a null `->Plugs.size()` would fault at a
small **positive** offset, not at **-8**. The signature does not match. Recorded
only so the next run does not re-derive it.

### S32: the compositor, not the app

`gnome-shell` **segfaulted at 18:28:59** — `code=dumped, status=11/SEGV` — and
took the graphical session to the login screen. From the apport core:

```
wl_display_flush_clients -> wl_client_destroy -> libmutter-14
  -> g_signal_handler_disconnect -> g_type_check_instance   SIGSEGV
```

A **use-after-free in mutter while destroying a disconnecting Wayland client**.
Four occurrences in two days (`Aug 20 17:35:18`, `Aug 21 16:44:21`, `16:48:06`,
`18:28:59`). **TIDE_STANDALONE did not crash** — no crash report for it; my own
`timeout` terminated it.

Stated carefully in both directions, because this journal has got it wrong twice:
the crash landed within seconds of TIDE's SIGTERM and TIDE's `gmpi-wl` memfd was
mapped in the compositor — **but** `wl_display_flush_clients` iterates every
client and the core does not name which one, `/memfd:… (deleted)` is how every
memfd appears, the 16:48 crash happened with TIDE not running, and TIDE's own
backend handles a lost connection cleanly (`DrawingFrameWayland.h:3856-3868`)
rather than faulting. **So "the compositor crash IS S23's 139" does not follow.**

**Learned:**

1. **`journalctl` keeps a kernel record of every segfault, with a module-relative
   offset, and nobody in this fleet had looked.** Two runs spent hours on stress
   loops and controlled-run counts for a crash the kernel had already located
   twice. **Before trying to reproduce a crash, ask what the machine already
   wrote down about it.**
2. **The same module offset under two different ASLR bases means one
   deterministic site.** The absolute `ip` differs and looks like noise; the
   bracketed offset is the invariant, and it is the number worth keeping.
3. **A fault at `0xfffffffffffffff8` is -8, and -8 is a signature, not an
   address.** It says "read just before a base", which rules out the whole class
   of plain null-`->field` explanations — including the one `addr2line` handed me.
4. **A timeline can refute a hypothesis that a reproduction attempt cannot.** The
   29-second gap between the two crashes, read against when the quarantine must
   have fired, kills the session-file theory on its own — no build, no run.
5. **Resolving an address in a rebuilt binary is not evidence.** It produced a
   plausible, thematically perfect function name that the fault signature then
   contradicted. The check that made it safe was asking whether the *symptom*
   matched, not whether the *story* did.
6. **A "GNOME Shell crashed" line and a login prompt are not the same event.** At
   19:11 I read `Xwayland terminated` plus a shell restart as a fifth crash; it
   was Jeff logging back in, and the greeter exiting normally. `loginctl` and the
   `session opened/closed` lines told the truth in one command.

**Next:**

1. **S23 is one step from closed:** rebuild the tree as it stood on 2026-08-20
   ~17:30 and `addr2line 0x3b4627` in *that* binary. `-g` does not change codegen
   at `-O3`, so the offset should survive. **No more stress runs** — stress has
   never reproduced it and this box cannot afford them.
2. **S32 before any further GUI work here:** option (b), a nested or headless
   compositor, is the one a run can adopt without touching Jeff's machine.
3. **S31** is still open — the `pkill -f` trap.

**Did not rebuild.** The item needed no build: the binary in `build/SynthEditSem/`
was built at 16:27 today, after `main`'s last code commit (`da0bb37`, 16:01), and
the only thing landed since is `c5c29ee`, docs-only. The previous run measured
`main` building here rc=0 with 0 error lines at that same code state. Jeff had
just logged back in and a full rebuild is CPU he was using.

**Jeff's working tree and config left as found:** `~/TideSynth` was clean at
start and is clean apart from this branch's two files; `~/.config/TIDE Rack/`
is byte-identical (md5s above). All replay work happened in the session
scratchpad.

**Branch/PR:** `tide/linux/S23-session-replay` — TideSynth only, row and journal. No code change.

---

## 2026-08-21 — linux — S23 did not reproduce, but a mechanism explains why it never would

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (claude-desktop 1.32885.1) · as **tide-rack-bot** (both paths)

**Did:** STEP 1 first — verified and closed
[#257](https://github.com/JeffMcClintock/TideSynth/issues/257) — then took **S23**
and stopped it early, deliberately, without a backtrace. The row goes back to
`TODO` with the blocker cleared, one hypothesis that explains the whole observed
pattern, and the exact experiment left to run.

### STEP 1: #257 was `main`'s break, not the branch's

CI filed it against `tide/mac/E11-blob-lifetime`, so it reads as that branch's
fault. It was not. `TiDEPanelGui.cpp` referred to `tide::render::Quality` before
`modules/common/TidePathTracer.h` declared it, and the counts per commit say
where that lived:

| commit | uses in `TiDEPanelGui.cpp` | `Quality` in `common/TidePathTracer.h` |
|---|---|---|
| `fd1e106` | 2 | **0** |
| `348e91d` | 2 | **0** |
| `f760589` | 2 | **0** |
| `da0bb37` | 2 | **2** |

So **`main` itself was broken from 01:36 to 04:02**, and both other `build.yml`
runs in that window failed identically on **windows and linux**
([32436905221](https://github.com/JeffMcClintock/TideSynth/actions/runs/32436905221),
[32439088554](https://github.com/JeffMcClintock/TideSynth/actions/runs/32439088554)).
No `platform:win` twin exists only because that step is `matrix.platform != 'win'`.
Jeff's `da0bb37` added the declaration.

Verified by building here rather than by reading CI — `cmake -S . -B build
-DCMAKE_BUILD_TYPE=Release` rc=0, `cmake --build build --config Release
--parallel` **rc=0, 0 error lines, 3m36s**, the failing TU compiled into all
three targets, and `TIDE.gmpi` / `TIDE_VST3.so` / `TIDE_VST3.vst3` /
`TIDE_STANDALONE` all linked. Issue closed with the working; no PR, because
there was nothing left to fix.

### S23's stated blocker is gone, and so is one of its hypotheses

- **`gdb` is installed** — 15.1 (Ubuntu 15.1-1ubuntu1~24.04.1). The row's
  *"`gdb` is not installed, so nothing here names a frame"* is stale.
- **`libpipewire-0.3-dev` is 1.0.5-1ubuntu3.3 system-wide and the runtime
  `libpipewire-0.3.so.0.1005.0` is the same version.** So the tempting theory —
  that the two crashes came from S21's `apt-get download` + `dpkg-deb -x`
  workaround compiling against headers that did not match the linked runtime —
  **is dead**: the archive version and the installed version are the same.
  Worth killing explicitly, because it is the first thing the workaround
  suggests.

### It did not reproduce, and here is the rate

Release build with `-g` added at the same `-O3` (so codegen is the shipped one,
not `RelWithDebInfo`'s `-O2`):

| what | result |
|---|---|
| 4 native runs, 25s window, SIGTERM at the end | **rc=0 every time** — a clean SIGTERM shutdown exits 0, so any 139 really is a segfault |
| 1 run under `gdb`, driven ~13 min over the command channel: ~1000 arm-then-click prefab insertions plus a full-window screenshot after each | **no signal** |

The only `SIGNAL-STOP` in the gdb log is my own `kill -9`. **28 clean runs
were the prior evidence; this adds roughly 1000 driven interactions to it.**

### The mechanism that explains "seen twice, never again" — and it needs no nondeterminism at all

`SessionState` (`GMPI_Wrappers/wrapper/Standalone/SessionState.cpp`) writes a
**breadcrumb** `session.loading` before reading the patch and removes it after
(`:395-419`). A launch that finds one concludes the previous run died inside the
read, **quarantines `session.xml` to `session.previous.xml`**, and comes up at
defaults (`:325-328`). The header says why in as many words: an assert inside
the parse is *"the failure a return code cannot catch"*, and without this
*"the app would die on every launch from now on"*.

**So a crash in session restore is self-limiting to exactly one occurrence per
bad file** — which is the entire shape of what S23 recorded: two crashes, both
on a *first* run, then 28 controlled runs finding nothing. The 28 runs were not
evidence of rarity. They were runs of an app that had already thrown the
offending file away.

**And the file is still on disk.** `~/.config/TIDE Rack/session.previous.xml`,
**17,866 bytes, dated 2026-08-20 17:33** — the same box and the same day as the
two crashes. That is the prime suspect, sitting where the quarantine put it.

**Not proven.** `keepCurrentAside` (`:255`) rotates to the same filename for
ordinary reasons, and the header names a false positive of its own — a second
instance launched while the first is still in `restore()` sees the crumb and
quarantines a good file. Either could have produced this one.

### Why I stopped instead of running the experiment that decides it

The experiment is one command and needs nothing from Jeff:
`StandaloneSettings.cpp:49` honours **`XDG_CONFIG_HOME`**, so the suspect file
can be replayed in a scratch config dir under `gdb` **without touching Jeff's
own config at all**. The harness is written (`replay.sh`, in this run's
scratchpad, reproduced in the row).

I did not run it, because **the desktop session on this box crashed twice while
I was working**, and this is Jeff's machine. Stated with the control, because
the control is what matters: the first crash (~16:45) came during the heavy
insertion stress; **the second (~16:49) came when `TIDE_STANDALONE` was not
running at all** — I had killed it five minutes earlier and the relaunch was
never made. So **the correlation with TIDE is unsupported**, and the second
event is the negative control that breaks it. This box is a **VirtualBox** VM
whose log carries `libEGL warning: Ensure your X server supports DRI3` and
repeated `Invalid sequence for VSYNC frame info`; `gnome-shell` restarted three
times in four minutes. Neither event was a reboot — `journalctl --list-boots`
shows one boot across the whole session.

I am recording that as an observation about the box, **not** as a finding about
the product, precisely because S21's own entry made the opposite mistake with
these same crashes and had to retract it.

**Jeff's working tree was left as found**: `session.xml` was overwritten by my
stress run (5.8 MB of inserted junk), and the 13,406-byte 2026-08-20 original
was restored from a backup taken before the first launch — **md5 verified equal**.
The stress file is kept as evidence in the scratchpad, not in the repo.

**Learned:**

1. **A self-healing mechanism upstream of a bug will make that bug look
   intermittent, and a controlled-run count cannot see it.** 28 clean runs read
   as "rare"; they were 28 runs of an app that had already quarantined the input.
   **Before sizing a crash by its rate, ask what the first crash changed.**
2. **`XDG_CONFIG_HOME` is honoured by the standalone**, so any session-state
   experiment can run fully isolated from the developer's own config. This is
   the technique that makes replaying a suspect patch safe on a shared box.
3. **A branch's CI platform issue can be reporting `main`'s break.** The title
   names the branch, so the natural reading is wrong. One `git show` of the
   failing TU against `main` decides who owns it — and here the answer was
   nobody on this branch.
4. **A clean SIGTERM shutdown exits 0**, so in this harness a 139 is unambiguous.
   Worth one run to establish before counting crashes.
5. **`pkill -f <pattern>` matched my own shell and killed it (exit 144)** — the
   third time this fleet has recorded it and the second in two runs on this box.
   Two journal bullets have not stopped it. Filed as **S31**, because a lesson
   this durable wants a wrapper, not another bullet.
6. **Report a crash with its control, not just its correlation.** I had a
   plausible story — heavy path-traced rendering in a software-GL VM takes the
   compositor down — and the second crash, with nothing running, refuted it. It
   would have been an easy and confident thing to write down.

**Next:**

1. **The one experiment that decides S23**, and it is cheap: replay
   `~/.config/TIDE Rack/session.previous.xml` as `session.xml` under
   `XDG_CONFIG_HOME=<scratch>` with no `session.loading` present, under `gdb`.
   A segfault names the frame and closes the row; a clean run eliminates the
   only hypothesis that explains the observed pattern. **Do this before any
   further stress runs** — it is seconds, and the stress is hours.
2. **Do it when the box is calm.** The desktop was restarting itself today; that
   is worth knowing before blaming anything it hosts.
3. **S31** — the `pkill -f` trap.

**Branch/PR:** `tide/linux/S23-standalone-segfault` — TideSynth only, row and journal. No code change.

---

## 2026-08-21 — macos — E11's hazard is unreachable, and the reason is a stub nobody had noticed

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** answered E11 by measurement — **the dangling-pointer hazard cannot
happen** — and found the real defect underneath it, which belongs to **E6**
rather than to a new row.

### E11 asked the wrong question, and the right one is cheaper

The row wanted the drain ordering established: does anything write parameter 1
between `start_processor` (host main thread) and the first `process()` (audio
thread)? **The ordering does not matter, because the pointer is never taken.**

Exhaustive rather than sampled — `value_` appears exactly four times in
`processor_holder`:

| site | what it does |
|---|---|
| `.h:87` | double default, at construction |
| `.h:91` | `std::vector<uint8_t>{}` — an EMPTY blob, at construction |
| `.cpp:224` | read (the seed) |
| `.cpp:337` | read (`sendParameterToProcessor`) |

**No write after construction**, because `setBlob` (`.h:135`) is a **stub**: its
one assignment is commented out at `:137` and it unconditionally returns `true`.
So a blob parameter's vector is empty for the object's whole life, and both
sites that could publish a raw pointer decline to — the seed hits
`if (!v || v->empty()) continue;`, and `sendParameterToProcessor` takes the
inline branch because `v.size() > 8` is false at size 0. `e.oversizeData_` is
never assigned. Nothing can dangle.

**E11 is WONTFIX with a trigger written into it:** the moment `setBlob` is
implemented, both sites go live and the row's analysis applies exactly as
written.

### The stub is the real defect, and two callers already trust it

`onQueMessageReady` (`.cpp:397`) reads the payload into a local vector, calls
`setBlob`, gets `true`, and then calls `sendParameterToProcessor` — **which
reads the still-empty vector and sends a zero-length blob event, discarding the
bytes it just read.** `setPin` (`.cpp:652`) marks the controller waiting on a
parameter whose stored value is empty. And the seed's own comment says it exists
so a processor created at any time starts with the parameter's *current* bytes —
which cannot happen while nothing ever writes them.

**Not verified at runtime, and I am saying so rather than implying otherwise:**
TIDE's patch persistence goes through the preset chunk (S12), not the
blob-parameter round trip, so TIDE working is not evidence this path works.

### A31's habit stopped this becoming a duplicate

Before filing a new row I grepped the backlog for `processor_holder` — the habit
this fleet added yesterday — and got **three hits: E9, E11, E6**. **E6 already
owns this ground**, saying a blob-capable prime *"needs a non-scalar setter in
GMPI's processor_holder — only setParameterNormalizedFromDaw exists"*. That
premise is slightly wrong in an actionable way: the setter **exists and is
empty**, which is a sharper and more fixable statement than "there is none". So
the finding went onto E6 and no S31 was filed.

**Learned:**

1. **A row that asks "is this ordering safe?" can be answered by showing the
   code never reaches the ordering at all.** Four greps beat the runtime
   experiment the row proposed, and the answer is stronger — not "safe today"
   but "unreachable by construction".
2. **A stub that returns `true` is worse than one that returns `false`.** Both
   callers here branch on the result, so an honest `false` would have surfaced
   this years earlier; instead the bytes are read, discarded, and reported as
   stored.
3. **The grep-before-filing habit paid for itself the day after it shipped**
   (A31). Three rows already cited this file; one of them already owned the
   finding.

**Next:**

1. **E6** carries the sharpened root cause. Implementing `setBlob` is PR-GATED
   and wants Jeff — it is a hosting-layer change with every GMPI plug-in
   downstream.
2. Whether anything shipping depends on the blob-parameter path is unmeasured;
   the preset chunk is what TIDE actually uses.

**Branch/PR:** `tide/mac/E11-blob-lifetime` — TideSynth only, rows and journal. No GMPI change.

---

## 2026-08-21 — macos — arm64-only, and the FORCE that made the obvious change a no-op

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** made TIDE Rack arm64-only on macOS. Jeff: *"lets change macOS to
ARM-only, for faster building. Any straggler who wants intel can build it
themselves."*

### The one-line version of this change does nothing, silently

TIDE sets `CMAKE_OSX_ARCHITECTURES` in its own root (`:32`) and in
`modules/CMakeLists.txt:5`. Changing those to `arm64` looks like the whole job.
It is not: **`SynthEditLib/CMakeLists.txt:10` and
`GMPI_Wrappers/CMakeLists.txt:12` both do `set(... CACHE STRING ... FORCE)`**,
and a FORCEd cache set overrides the parent scope *and* the command line.

Proven rather than reasoned about, before touching anything:

```
cmake -DCMAKE_OSX_ARCHITECTURES=arm64 <TideSynth>
  CMakeCache.txt:  CMAKE_OSX_ARCHITECTURES:STRING=x86_64;arm64
```

So the flag a person would reach for first is discarded without a word. Both
shared lines are now guarded with `if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)` —
universal stays the default for anyone who does not choose, and a consumer that
has chosen keeps its choice.

### The measurement, and the negative control that matters more

| | |
|---|---|
| universal, cold | **160 s** |
| arm64, cold | **79 s** — 2.03x |
| `lipo -archs` on TIDE / TIDE_VST3 / TIDE_STANDALONE | **arm64** (control binary: `x86_64 arm64`) |
| **SynthEdit configure — the negative control** | **still `x86_64;arm64`** |

**SynthEdit is untouched and still ships universal**, which is not luck: SE16's
root sets the variable before adding those subdirectories, so the guard skips
and the value is identical. That control is the whole reason this was safe to
do in shared repos — without it, "TIDE got faster" and "the commercial product
quietly lost Intel" look the same from here.

**Roughly half the build is the shared libraries** (SynthEditLib 181 + EditorLib
57 objects against SynthEditSem's 219), which is why a per-target
`OSX_ARCHITECTURES` property on TIDE's own targets — the change that would have
needed no shared edit at all — was rejected: it would have bought about half
the speedup, because the libraries underneath would still compile twice.

**Stated rather than buried:** a released TIDE Rack will not run on an Intel Mac
and **nothing will tell such a user why** — the plugin just fails to load.
Recorded on **R3**, which owns the packaging where a minimum-hardware note would
belong.

**Learned:**

1. **`set(... CACHE ... FORCE)` in a dependency silently outranks the consumer
   AND the command line.** The tell was cheap — one configure, one `grep` of
   `CMakeCache.txt` — and without it this change would have been committed,
   reviewed and merged while doing nothing at all.
2. **When a change must reach shared code, the negative control is the
   deliverable.** Proving SynthEdit still configures universal is what
   distinguishes this from a change that quietly dropped Intel support for the
   commercial product too.

**Next:**

1. Three PRs that **must merge together** — TIDE's arm64 line is inert until
   both FORCEs are guarded.
2. Whether SynthEdit itself goes arm64 is now a one-line change and a separate
   decision.

**Branch/PR:** `tide/mac/macos-arm64` + [SynthEditLib#31](https://github.com/JeffMcClintock/SynthEditLib/pull/31) + [GMPI_Wrappers#10](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/10).
## 2026-08-21 — macos — Linux CI is green, and the macOS job that would confirm it cannot say anything

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** prepared C7e's and C7's status on the evidence that exists, rather than
on the evidence the clause asks for — and said which is which.

### The apt-get landed and Linux went green

Jeff pushed [#250](https://github.com/JeffMcClintock/TideSynth/pull/250). The
step that has failed every Linux run since the matrix first fired now reads:

```
3. Install Linux deps: success
4. Configure:          success     <- red for two days
5. Build:              success
6. File a platform issue on failure: skipped
```

That last line is the mechanism confirming it rather than me reading a log.
**Nothing was hiding behind the fail-fast probe** — the measured four-package
set was complete, which was the standing risk.

### The macOS job on that PR is incapable of telling us anything

Checked rather than assumed, because it is the difference between waiting and
finishing: **#250 changes exactly one file**, and its only non-comment change is
inside a step gated `if: matrix.platform == 'linux'`. The macOS job's definition
is byte-identical to `main`'s. Its outcome carries **zero information about this
change** — so waiting for it is waiting for a 5%-likely event to confirm a step
that did not change.

### So C7e is IN-REVIEW, not DONE, and the reason is written down

The clause is *"three platforms run rather than skip, and pass, on a PR"*.
Windows and Linux pass on #250; macOS is QUEUED. **All three platforms are
proven — but not in one run**, and that is a CI-capacity fact (S30: macOS
completes ~5% of runs here) rather than anything about TIDE. The two runs that
did complete macOS recently both **passed**, and both were runs where **linux
failed and macOS succeeded**.

Flipping to DONE on that composite would be reading the clause loosely on my own
authority, so it is IN-REVIEW with the argument attached and the call left to
Jeff.

**Learned:**

1. **Ask what a pending check could possibly prove before waiting on it.** One
   look at the diff showed the macOS job was gated out of every line that
   changed. That reframed an hour of waiting as a decision to make.
2. **"All three platforms pass" and "all three passed in one run" are different
   claims, and only one of them is what an Accept clause usually means.** Saying
   which one you have is the whole job when the weaker one is all that CI
   capacity allows.

**Next:**

1. Jeff's call on whether the composite satisfies C7e; C7 moves with it.
2. On closing, **C10 and R2-R6 unblock** — the release track has waited on this.
3. **S30** (macOS starvation) and **S29** (duplicate runs) are the CI-capacity
   rows this turned up; both want a workflow edit.

**Branch/PR:** `tide/mac/C7-close` — TideSynth only, status and evidence.

---

## 2026-08-21 — macos — STEP 4 bookkeeping: seven rows flipped on merged PRs

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** flipped **A31, A32, C10, E5, E15, S24, E6** from IN-REVIEW to DONE.
Every PR behind them merged today, and each state was **queried rather than
assumed** — thirteen PRs across four repos, all `MERGED`.

This is STEP 4's own instruction (*"If you see an IN-REVIEW row whose PRs have
all merged, flip it as part of your STEP 4"*), done in one pass because the
whole mac chain landed at once rather than one row at a time.

**Learned:**

1. **A day that merges thirteen PRs leaves the backlog lying by seven rows.**
   IN-REVIEW is accurate for about as long as it takes the PR to merge, and
   nothing flips it automatically — so a summary taken off the status column
   mid-merge-run understates what shipped.

**Next:** the queue's remaining TODOs are the S-series GATED rows, the linux
platform issues, and E2 — which now has a ruled nine-module list and no open
questions.

**Branch/PR:** `tide/mac/flip-merged-rows-0821` — TideSynth only, bookkeeping.

---

## 2026-08-21 — macos — E16 ruled Tier 1, and four conventions came with it

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** recorded Jeff's E16 ruling — **TIDE ships Tier 1** — plus four
conventions and seven corrections he gave alongside it, and verified the four
claims that were checkable rather than transcribing them.

### The ruling is a sequencing argument, not a size preference

*"we need a testable, installable MVP first"* — Tier 1 is what lets installers,
the website and the release track move, while the full set is developed in
parallel by Jeff. The risk being managed is stated plainly: *"I wouldn't want
to spend a lot of time on a full set only to find we got the design language
wrong or something that requires a lot of redesign."* Every authored panel is
hostage to E17 until an installable build has been tested.

**The doc's own argument for Tier 2 was rejected as false.** It said that below
Tier 2 a user *"can make a sound but not music"*; Jeff: *"Tier 1 absolutely can
make music. Plenty of real hardware products ship with only this type of
functionality."* Corrected in place.

### Four claims checked against the tree, because each changes what is blocked

| claim | verified |
|---|---|
| Oscillator HD is the oscillator TIDE ships | **already compiled in** by E2c (`SynthEditSem/CMakeLists.txt:299`) |
| glide is already in MIDI-CV2 | **yes** — `CVoiceList.cpp` drives constant-rate glide via `HC_GLIDE_START_PITCH` into `MidiToCv2`'s `pitchInterpolator` |
| pitch 0.5 = 440 Hz, 0.6 = 880 Hz | **exactly** — `ug_oscillator2.cpp:375` is `440.0 * pow(2.0, volts - 5.0)`, and `ULookup.h:9` agrees |
| volts are display-only | consistent — `CVoiceList.cpp:980` records the "SE volts (0-10 V)" convention on the MIDI-CV host controls |

**The first of those unblocks the MVP.** S8 has been carried as the oscillator's
blocker since 2026-08-18, and its measurement is still true — but it is about
`OscillatorNaive`, and TIDE ships `SE Oscillator4`. The row stays open for the
packaging fault it found; it no longer gates E2.

### The set is trimmed by three, each for a mechanism

**Sample & Hold** out (*"not critical"*). **Slew/Glide** out — it is in MIDI-CV2,
verified above. **Mixer** out, ruled minutes later once the question was put
plainly: *"let's leave mixer out of MVP to reduce the critical path."* TIDE's
cables fan in with automatic summing, so a mixer is convenience rather than
capability.

**So the MVP is NINE modules** — Oscillator HD, Envelope, Output, I/O modules,
Filter, VCA, LFO, Noise, Attenuverter+Offset — and **nothing in the set is
open**. Worth noting that the first pass of this entry recorded the Mixer as
undecided rather than guessing which way *"quite useful though i guess"* fell;
asking cost one line and got a ruling that also names its reason.

### I flagged a critical-path risk that does not exist, and Jeff corrected it inside the hour

Polyphony is ruled to be SynthEdit's existing model — modules always
monophonic, the runtime clones them per voice on the DSP graph, voice count set
on the MIDI-CV rack module, *"essentially free"*. The I/O modules are ruled to
be rack modules, mandatory but movable. **I read those two together as putting
MIDI-CV back inside a Container** — E7's measured failure — and wrote it up as
newly on the MVP's critical path.

**It is not, and the error is worth keeping.** Jeff: *"conceptually they are
rack modules, but in reality we place the MIDI-CV2 at the root level and route
it into the Container (which represents it on the GUI as patch-points). This is
already solved. MIDI-CV as rack module [is] how the end-user thinks of it, not
how it is implemented."*

So root-placement-plus-facade **is the architecture**, and PLAN.md's word for it
— *"v0.1 side-steps this"* — is what made it read as a temporary evasion of an
open limitation. All four places that carried my inference are corrected:
`decisions.md`, `module-set.md` §9.3, E7's row, and PLAN.md itself. **S28 turned out to be wrong too, and is closed** — see below.

### S28: I filed a row without asking why two modules differed

I had sharpened S28 to *"the root implementation modules should not render on
the rack canvas"*. Jeff: *"if they have no GUI class (which MIDI-CV2 does not),
they already don't render on the rack canvas (only on the structure-view).
That's already implemented and working."*

**Verified, and it splits the two modules the row had lumped together:**

| module | GUI class | saved rect | rendered? |
|---|---|---|---|
| `SE MIDI to CV 2` | **none** — no `*Gui*` registration in SynthEditLib | **0,0,0,0** | no — the probe already skipped it as *"zero rect"* |
| `MIDI In` | **yes** — `modules_internal/MidiInGui.cpp:123` | 8x14 | yes, because it is meant to |

So the self-exclusion the row asked for **already exists by mechanism**, and the
one module that does render has a GUI class and is *supposed* to. Under the same
day's I/O-modules ruling it is a rack module that has not been authored to rack
proportions yet — **E2's work on item #4, not a defect**. S28 is closed WONTFIX,
and the probe's docstring now carries the discriminator so the next reader does
not re-derive it or, worse, add a name-based exception list.

**Learned:**

1. **A correction that removes a blocker is worth checking hardest, not
   least.** "We ship Oscillator HD" reads like a preference; it retired a
   three-day-old blocker, and one grep proved the module was already in the
   binary.
2. **"Rack module" was a statement about the USER'S MODEL and I read it as one
   about implementation** — then reasoned confidently from it to a critical-path
   risk that does not exist. The tell I missed was available: V3 had already
   built root-placement-plus-facade and it works. **A vocabulary that describes
   the product can look exactly like one that describes the code**, and PLAN.md
   calling the arrangement a *"side-step"* of a *"still open"* limitation is
   what made the wrong reading the natural one.
3. **Corrections are cheapest when the work is still unmerged.** These touched
   four documents, a probe, a row and this entry; none had landed, so the
   record shows the rulings rather than the rulings plus retractions.
4. **Two wrong calls this session died to one habit: reasoning from a
   measurement without asking what produced it.** S28 came from a probe line
   listing two root modules, one measuring and one not — and I never asked why
   they differed. One grep for a GUI class answers it. The probe was right both
   times; the interpretation was not.

**Next:**

1. **E2 is unblocked** and now has a ruled list to author from.
2. **E2 has everything it needs**: the Mixer is ruled out, and E7's container
   boundary turned out to be already solved. Nothing in the set is open.
3. The release track (R2-R6) has an MVP to package.

**Branch/PR:** `tide/mac/E16-tier1-ruled` — TideSynth only, rulings and docs.

---

## 2026-08-21 — macos — E5: the rack grid ruled, and the snap is gcd(12, 15)

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** implemented Jeff's ruling on the rack grid. `hpWidth` stays **15**
(the visual HP), a new **`snapWidth = 3`** is the placement pitch, the row is
**384**, TIDE's standard module is **48** wide, and a panel is the **whole
row**.

### Jeff proposed 384 / snap 12 / width 48, and the measurement moved one of the three

Snapping on 12 is the arm that would have cost VCV compatibility, which is the
opposite of what it looks like — every VCV module is a multiple of 15 by
construction, so today's 15-snap fits them all exactly:

| snap | exact for | worst gap |
|---|---|---|
| 15 (today) | 13 of 13 common VCV widths | 0 |
| **12** | **5 of 13** | **9 DIPs (3.0 mm)** |
| 6 | 10 of 13 | 3 DIPs |
| **3 = gcd(12,15)** | **13 of 13** | **0** |

12 and 15 first agree at 60 DIPs (4 HP), so a 12-snap misaligns every VCV
module that is not a multiple of 4 HP — worst on the 1–2 HP utilities there are
many of. **3 is the coarsest snap that satisfies both worlds**, and it keeps
every TIDE dimension a multiple of 12 anyway, because 384 and 48 both are.
Jeff took it.

### `hpWidth` was doing two jobs, and this would have wrecked the second

`ViewBase::renderRack` derives the rail hole pitch *and* the hole radius from
`hpWidth` — *"one threaded mounting hole per HP"*, `holeRadius = hpWidth *
0.13`. Setting it to 3 would have given the rails a hole every 3 DIPs at
**0.39 DIP radius**: fine sandpaper instead of Eurorack rails, and nothing
would have failed to build. Six consumers, all in one file; the split sends
`snapToGrid` to `snapWidth` and leaves the four rendering sites on `hpWidth`.

### The rail allowance never existed — Jeff's correction, and it is the bigger one

*"useable interior is the entire rack module surface (we don't draw mounting
hardware)."* Confirmed in the render order rather than taken on trust:
`ContainerView.cpp:71` calls `renderRack` inside the **background fill**, so
modules draw over the rails exactly as a real panel covers the rails it is
screwed to. Rails show only in empty slots.

So `build-prefabs.py`'s `380 - 2*15 = 350` subtracted an allowance that does
not exist — and that assumption had propagated: **#239's probe encoded it too**
and reported TIDE's own prefab as `TALLER THAN ROW`, a verdict measured against
a constant that does not govern. Both corrected. **A wrong model in a
verification tool is worse than no tool**, because its output looks like
evidence.

### Verified

- probe rewritten to the ruled model: **12 selftest cases, 0 failed**, and the
  new cases are the ruling's own claims — a 380 VCV panel and a 384 TIDE panel
  both pass, 400 fails, 48 / 30 / 36 all land on the 3-grid, 100 does not.
- all four targets build rc=0 against the modified `SynthEditLib`.
- MidiCv regenerated at 384 and **re-measured on a running rack**, not just
  rebuilt: `TIDE MIDI-CV  w=96 h=384  6.400 HP  on-grid  fits row`, no
  overlaps — the two clauses this row could never satisfy, satisfied — and the
  rails visibly pass *behind* the panel
  ([docs/images/e5-rack-ruled-grid-macos.png](docs/images/e5-rack-ruled-grid-macos.png)).

**The one violation left is not a rack module, and I did not silence it.**
`MIDI In` 8x14 is TIDE's seeded root plumbing (`TideApp.cpp:727` — V3's
polyphony workaround, a root MIDI-CV feeding the rack module as a facade).
Teaching the probe that name would make it lie about what it measures, so the
measurement stands and the question — should root plumbing carry a panel rect
at all? — is filed as **S28**.

**The GATED half is its own PR** ([SynthEditLib#30](https://github.com/JeffMcClintock/SynthEditLib/pull/30)) — two constants and one line, shared with SynthEdit's
own rack mode, so it is reviewable on its own.

**Learned:**

1. **A constant that serves both a layout rule and a drawing rule will be
   changed for one and silently break the other.** `hpWidth` read as "the HP",
   and it was also the hole pitch. The tell was reading every consumer before
   editing the definition, which took one grep.
2. **The most expensive thing in this item was an assumption inside a probe.**
   `350` was arithmetic on a model nobody had checked against the renderer, and
   it had already produced a confident false verdict about TIDE's own shipped
   prefab.

**Next:**

1. **#239 is superseded by this branch** — it carried the same work stacked
   behind E16's unresolved ruling, and its probe held the wrong rail model.
2. E5's second clause (no overlaps) is now measurable on the ruled grid.

**Branch/PR:** `tide/mac/E5-rack-grid` (TideSynth) + [SynthEditLib#30](https://github.com/JeffMcClintock/SynthEditLib/pull/30) — merge together.

---

## 2026-08-20 — macos — E6's honest tell: renders that ignored your state now say so

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · eighth item

**Did:** the wrappers-side arm of E6 — `render-audio`'s result JSON now
carries `parametersUnprimed`, the count of non-scalar parameters the prime
loop skipped. One counter, one field
([GMPI_Wrappers#9](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/9)).

**Verified live**, TIDE built with the branch as its wrappers override:
`{peak: 0, parametersPrimed: 0, parametersUnprimed: 2, silent: true}` — the
before-state is the row's own 2026-08-18 finding (no such field, `peak: 0`
indistinguishable from a silent patch).

**Scope honesty:** this is the row's "or, failing that" arm. The full Accept
— a standalone render matching the live app's audio — needs a blob-capable
prime, and that is a non-scalar setter in GMPI's `processor_holder`
(PR-GATED) plus a real design question about blob event payload lifetime
through the queued PinSet path. Proposing that half-baked into the most
curated repo at midnight is exactly what the PR-GATED bar exists to slow
down; it stays filed.

**Learned:**

1. **A row that names its own fallback scope can be half-shipped honestly** —
   the field ships value now (every E2a-class measurement stops being fooled)
   while the row keeps the full Accept visible instead of being closed on the
   cheap arm.

**Next:**

1. The GMPI blob-prime, for whoever takes the design question to Jeff.
2. E1c is the last small takeable row in this section.

**Branch/PR:** `tide/mac/E6-unprimed-report` in GMPI_Wrappers
([#9](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/9)) + TideSynth (bookkeeping).

---

## 2026-08-20 — macos — S25 does not reproduce on mac, and the negative result is the deliverable

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · seventh item

**Did:** ran S25's own comparison on this box and returned the row to TODO
with the platform scoped: **insert ≡ restore on mac**.

Same protocol as the linux finding (fresh isolated HOME so the rack is
genuinely empty — the session-restore trap is real on mac too, at
`Library/Application Support/TIDE Rack/session.xml`, not `.config`): the
Oscillator prefab click-placed from the browser renders as its two bare
jacks with correct glyphs — no tofu, no grey panel — and the same instance
restored after kill/relaunch renders identically. Screenshots committed
beside the row.

**What the negative result eliminates:** any platform-independent divergence
between the freshly-inserted and restored paths. The E2a `PanelWndPosition`
suspect behaves the same in both (the 20×66 child-union size is the
documented measure mechanic, not a divergence). **What survives:** linux
font/resource binding — tofu is missing glyphs, and the linux fresh-insert
path failing to bind what its restore path binds fits everything measured.
That diagnosis needs the linux box; this one cannot observe the path.

**Learned:**

1. **A cross-platform row can be closed on one platform and open on another,
   and saying which is the whole value of a cheap reproduction.** Twenty
   minutes here spared the linux box the half of the suspect list that
   platform-independence just killed.

**Next:**

1. **linux box:** diff the font/resource binding between the two paths (the
   S21 probe discipline — write it from the reader's side).
2. Jeff's "nothing at all on insert" remains its own unreproduced report.

**Branch/PR:** `tide/mac/S25-fresh-insert-tofu` — TideSynth only, row + evidence.

---

## 2026-08-20 — macos — S24: the cross cursor was already there on Windows, and mac got the same shortcut

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · sixth item

**Did:** gave the module-arm state a cross cursor on mac.

### The row's premise had moved again, in a useful direction

S24 asks for a port of `SetCursorHandler` registration the way
`WaylandMainWindow.cpp:72` does it. Measuring first found TIDE **already
ships the affordance on Windows** — an inline `::SetCursor(IDC_CROSS)` in
`SynthEditGui.cpp` (`dragInProgress` set at OM_DRAG_NEW_MODULE, re-asserted
every pointer move, with its own comment: "GMPI doesn't expose setCursor on
IInputHost, so we go straight to Win32"). So the smallest correct change is
the AppKit analogue of TIDE's own mechanism, not new handler plumbing:
`TideCursorMac.mm` — `[[NSCursor crosshairCursor] set]` — called at
arm/disarm and re-asserted per move (AppKit cursor rects reset the cursor on
their own schedule, exactly like WM_SETCURSOR). One `.mm`, a Darwin-only
source entry, two `#ifdef __APPLE__` forks beside the `_WIN32` ones.

### Verified to the machine's limit, then handed to the instrument that found it

- builds rc=0; `tideShowCrossCursor` linked in the standalone (nm = 1).
- the full path driven live over the command channel: browser click **armed**
  (entry highlighted), rack click **placed a List Entry** at the click point
  with fully populated properties — the arm/disarm/re-assert calls all on
  that path, no crash. (The placed combo box also *draws* here — S25's tofu
  did not manifest for this control in this build.)
- **the pointer bitmap is the one thing a screenshot cannot carry** — the
  screenshot API excludes the cursor. Jeff reported the stuck cursor, so per
  S26's lesson the verification of the pixels goes to his mouse; the PR asks.

**Linux deliberately not attempted:** the Wayland APP works because
`WaylandToplevel` has its own `setCursor`; gmpi_ui's frames expose none, so a
linux TIDE cursor needs either a gmpi_ui frame API (a design question, not a
port) or a linux shortcut of its own. Left for the linux box rather than
guessed at from here.

**Learned:**

1. **Third time today a row's central premise had moved before it was taken**
   (P7d delivered elsewhere, E15's pin ruling unlanded, now S24's "defined
   and never called" — TIDE grew a Win32 path someone added without touching
   the row). Measuring the premise first is now the cheapest step of every
   item.

**Next:**

1. **Jeff, with a real mouse:** browser-click a module — the pointer should
   turn crosshair until the rack click lands (mac).
2. **S25** (tofu) — did not reproduce for List Entry here; wants re-measuring
   against the E15 panel stack before anyone chases it.

**Branch/PR:** `tide/mac/S24-cross-cursor` — TideSynth only.

---

## 2026-08-20 — macos — E15: the rack's faceplate is TIDE's own panel, and two breaks the swap flushed out

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · fifth item

**Did:** swapped MIDI-CV's faceplate from `SE Rectangle XP` to `SE TiDE:Panel`
and retired the Rectangle from TIDE entirely — both halves of the
hardcoded-twice pair (`SynthEditSem/CMakeLists.txt` staging AND `TideApp.cpp`'s
XML merge loop), per E2c's warning that missing either side fails silently.

**Taken with `BLOCKED(E14)` in the status cell:** E14 is DONE, and BLOCKED(id)
is defined as blocked-until-id-is-DONE, so the block had expired by its own
definition. Recorded because the letter of "never start a BLOCKED item" and
the definition point different ways for an expired blocker; the row now says
which reading was used.

### Every Accept clause, measured

| clause | evidence |
|---|---|
| MIDI-CV rebuilt on `SE TiDE:Panel` | regenerated via SynthEditCL; `assert_all_modules_linked` clean, its note naming the id |
| drawn in TIDE with legible captions | [docs/images/e15-midicv-tidepanel.png](docs/images/e15-midicv-tidepanel.png) — heading + PITCH/GATE/VEL/TRIG black on the light plate |
| Rectangle gone, build green | build rc=0; `__GLOBAL__sub_I_RectangleGui.cpp` **0** and `__GLOBAL__sub_I_TiDEPanelGui.cpp` **1** in BOTH `TIDE.gmpi` and `TIDE_VST3` |

### The two breaks the swap flushed out

1. **The first regeneration drew captions overhanging the plate.** The panel
   sizes ITSELF — RackUnits × 48 DIPs (`kRackUnitDips`) — and
   `SubView::measure` unions the children, so the authored 105-DIP slot
   produced a 48-wide plate under 58-wide labels. Fixed with `Rack Units 2`
   (96 DIPs; the one pin the swap sets — per-instance geometry, where the
   2026-08-19 ruling made *appearance* compile-time) and the faceplate grid
   re-authored to 96. The residual 3.2-HP-per-U vs 15-DIP-HP mismatch is
   E17's own open note, deliberately not settled here.
2. **`TiDEPanel.gmpi` had never been built on mac, and could not be.**
   `helpers/Timer.cpp`'s mac path is CFRunLoopTimer; the loadable-bundle link
   line carries no frameworks, so the authoring bundle failed with undefined
   `_kCFRunLoopCommonModes`. One APPLE-guarded CoreFoundation line. (TIDE's
   static build never sees it — the wrappers already link CF; same class as
   P7d, one repo over.)

### The authoring pipeline on mac, recorded so nobody re-derives it

SynthEditCL's factory scan root is **`SynthEditCL.app/Contents/PlugIns/`**
(the rescan prints it), NOT `build/modules/` — the TiDE `.gmpi` bundles must
be copied there, then `SynthEditCL -rescan`, then `build-prefabs.py --secl`.
The other five prefabs regenerate byte-identical except handle churn (the V3
lesson), so they were reverted; only `MidiCv.synthedit` is committed. And
`TIDE_STANDALONE` restores its session even under an isolated `HOME`
(`Library/Application Support/TIDE Rack/session.xml` on mac, not `.config`) —
two identical screenshots cost a relaunch before the session file was found.

**Flagged for Jeff, not acted on:** the row records a ruling that the panel
"ships exactly two pins", but `main`'s registration XML declares SIX (Text,
Text Color, Rack Units, Layout, Material, Panel Color). Rack Units earns its
keep as instance geometry; whether Layout/Material/Panel Color go compile-time
is the unimplemented half of that ruling.

**Skipped on the way here, with reasons:** **N1** defers itself ("after C7,
not during"; C7 waits on Jeff's apt-get, and a rename would double-conflict
with the open C10 PRs). **P11**'s remaining half is the diagnostic at
`DocOb.cpp:540` — GATED, not a build break; option (c) already landed in
docs/building.md. **S16/S22/S18** are GATED (S18 is also a licensing question
first). **E9**'s Accept needs a within-instance rate change no measurement has
ever produced — the wrappers replace the instance instead. **E5** waits on
rack styling (NEEDS-JEFF by its own text).

**Learned:**

1. **A module that measures itself is a different contract from a rectangle
   that takes orders, and the prefab grid only worked because the old
   faceplate obeyed it.** The swap surfaced the collision immediately —
   authored geometry vs self-sizing panels — and E17's units note is where the
   real reconciliation lives.
2. **"It builds in the product" says nothing about the authoring path.** The
   panel shipped statically into TIDE on three platforms while its loadable
   authoring bundle had never linked on mac. The first prefab regeneration
   needing it found that in one step.

**Next:**

1. **S24/S25** are the remaining takeable S-rows (cursor feedback; the tofu
   render — S25 may interact with this swap and wants re-measuring on top of
   it).
2. **E2's next children** now have both the panel and the pipeline; E16/E17's
   open notes gate the module list and sizing.

**Branch/PR:** `tide/mac/E15-panel-swap` — TideSynth only.

---

## 2026-08-20 — macos — P7d was already fixed, from a third direction, and its parked question is moot

**Prompt:** f7ae1a4 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · scheduled run, Jeff present · fourth item

**Did:** ran P7d's own Accept before building anything — `ninja GainGui_VST3`
on a fresh `GMPI-plugins` configure, no extra flags — and it **passes**:
rc=0, bundle produced, `otool -l` shows the UniformTypeIdentifiers load
command with the UTType reference resolved. Closed the row as DONE.

The fix is GMPI_Wrappers `3838493` — `gmpi_weak_frameworks()` in
`wrapper/cmake/GmpiFrameworks.cmake`, included by **all five** wrappers, in
the `-weak_framework` form `MacFileDialog.h`'s own comment specifies. Its
comment even names this row's incident: *"GMPI-plugins macOS CI, July–August
2026"*.

**The row had parked itself waiting on a scope ruling** (may a run touch
`GMPI-plugins`, or should gmpi_ui declare the framework?) — and the landed
answer is a third place neither option named: the wrappers, which are what
actually compile `DrawingFrameMac.mm`. No `GMPI-plugins` edit, no per-consumer
link lines, ALLOWED path. The ruling request is moot.

**Learned:**

1. **A row that parks on a question can be closed by running its Accept —
   check that before re-raising the question.** The ruling P7d wanted was
   never given and never needed; the fix arrived while the row waited. Fourth
   already-delivered row in two days (C15, U2, E14's naming note, now P7d) —
   running the Accept first is cheaper than any of the work the row proposes.

**Next:**

1. Queue by file order continues: S-series GATED rows are skipped (not build
   breaks), **N1 defers itself until C7 closes** ("do it after C7, not
   during" — C7 waits on Jeff's apt-get, and a rename now would double-conflict
   with the open C10 PRs), **A12/B1** are workflow edits the token cannot push.

**Branch/PR:** `tide/mac/P7d-uttype-framework` — TideSynth only, bookkeeping.

---

