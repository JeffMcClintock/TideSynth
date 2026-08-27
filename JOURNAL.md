# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-27 — macos — E34: one steer found the fix, and a human ran the test the harness cannot (interactive session, Jeff directing)

**Prompt:** standing backlog-loop instruction in the session · Opus 5, `claude-opus-5` · Claude Code · commits authored `Jeff McClintock` per the interactive convention

**Did:** took **E34** — the swallowed release when dragging an existing cable
end. Fix proposed as [SynthEditLib#60](https://github.com/JeffMcClintock/SynthEditLib/pull/60)
(GATED, so proposed not merged), **manually tested by Jeff and PASSED**. Row is
IN-REVIEW.

### The steer was the whole fix

Jeff, mid-investigation: *"note that the other type of cable (structure view)
gets this correct already"*.

`ConnectorViewBase::onPointerUp` is **shared** by both cable types, so the
difference had to be elsewhere — and it is. `wasPickedUp` is set only in
`PatchCableView::onPointerDown`; `ConnectorView2` never sets it. That is exactly
why the structure view has always worked, and it turns a guess into a reference
implementation sitting in the same file.

**It also settled narrow-vs-delete**, which the row had left open. The flag looks
like it protects click-then-move-then-click. It cannot: that gesture's second
click is a pointer-**DOWN**, and both subclasses end the drag from the identical
`imCaptured() → EndCableDrag` branch there. The 6px threshold covers
did-not-move. So past 6px the flag could only ever fire for press-drag-release —
the gesture it was breaking. Deleted, member and all.

**The general lesson: when two sibling classes disagree and one is right, the fix
is usually to delete what the broken one does extra, not to add to it.**

### The harness could not run the test, and that is a finding

**The command channel has no keyboard input and no scroll verb at all** — pointer
and MIDI only, verb list verified against `CommandDispatcher.cpp` on 2026-08-27.
Consequences hit immediately:

- an inserted module lands at `X=3732` on a 1100-DIP-wide view,
- there is no scroll verb to reach it, and no way to type into the properties
  X/Y fields to move it back,
- dragging jack-to-jack on the one visible module only selects it.

So **a cabled two-module rack cannot be built headlessly**, and E34's own claim
that *"repro is now scriptable — `--drag` from a connected jack to empty space"*
**is not true as written**: it presupposes a *connected* jack, and there is no
headless way to get one. That sentence misled this run; the row now says so.

### What to do about that, per Jeff

> If the MCP is insufficient to do your job, propose a fix/feature

Recorded because it changes what a blocked run should produce. A missing verb is
a **defect in our own code, in an ALLOWED gate** — not a constraint to work
around. The precedent is already in this journal: E31 handed a whole verification
to a human for want of an insert gesture, E35 added one flag, and E36 was then
measurable end to end in a script. The deliverable for a channel-blocked row is
**two PRs, not one** — the work, plus the proposed verb.

### And when a human is the right instrument anyway

Jeff ran the drag himself and it passed. Worth saying plainly: for this row that
is a **better** result than a scripted one. The bug was reported by a human
noticing a gesture felt wrong, and the fix has to be judged the same way. A
scripted A/B would have proved the document changed; it would not have proved the
gesture feels right.
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
## 2026-08-27 — windows — E19's VST3 cell is still not measured, and what stopped it is three defects upstream of REAPER (scheduled run)

**Prompt:** b97bc00a5 · Opus 5 (1M context), `claude-opus-5[1m]` · app Claude desktop **1.37937.1** · as **tide-rack-bot** (both paths)

**Did:** took **E19**, scoped to the windows VST3 cell. **Its Accept is not met and
the cell is not measured.** What the session produced instead is three filed
defects — **E48**, **E49**, **E50** — a `PROPOSED:` entry in
[docs/decisions.md](docs/decisions.md) that Jeff asked for in session, its parked
row **E51**, and STEP 4 bookkeeping on **X2**. TideSynth carries all of it;
nothing else was committed in any repo.

### Why E19, and why the row's own blockers were no longer the blockers

The NEXT block's `win` cell says nothing is takeable, but it is dated 2026-08-25
and predates E32–E50. Walking the rows in file order: **S1b** and **S8** are
wholly GATED, **E38** carries `NEEDS-SPEC`, and **E19** is the next one that is
TODO, `any`, unblocked, and states its Accept as observables. It is also this
box's own platform.

E19 recorded its VST3 and GMPI cells as blocked on **E27** and **E29**. Both have
since closed — E27 DONE (the ~14.1 KB "threshold" turned out to be the fixture
generator, not the plugin) and E29 WONTFIX with a per-box `sed` workaround — so
the cell looked reachable. **The wall turned out to be somewhere else entirely:
I could not get a prepared rack to reload in the STANDALONE, never mind inside a
host.**

### What actually happened, in the order it happened

**1. The instrumented build is fine.** `C:\SE\_scratch\e19` rebuilt against
today's `origin/main` — `TIDE_Rack_STANDALONE` and `TIDE_Rack_VST3`, 0 errors —
and the standalone runs, opens its command channel, and drives normally.

**2. Building the rack by script works, and two things about it are worth
keeping.** A **double-click** on a module-browser entry inserts; a single click
only selects. `--drag` on an inserted rack module **did not move it**, twice —
once via `--drag ... --steps 15` and once as explicit `--pointer-down` /
four `--pointer-move`s / `--pointer-up`, with no `DSP structure changed` either
time. So rack layout has to come from the document, not from a scripted gesture.
The Scope landed on top of the LFO, which is **E41**.

**3. Reusing yesterday's saved rack SEGFAULTS the app. 4/4.** `e19-fixture-doc.xml`
is 38,658 bytes, written by the 2026-08-26 windows run, whose journal entry
records it restoring correctly in the standalone that day. It now dies during
rack construction, at the same point every time:

```
Pulses constructed / SHASR constructed / Scope constructed   <- last line, always
LFO, LFO2, VCA x3, Compare                                   <- never reached
```

**The wrapper is not the variable.** Today's builds write a 4-byte `TDs1` header
in front of the document inside `<Param id="1">`; I reproduced the crash both
with that header and without it, so the reader skips it and it is not the cause.
Filed as **E49**.

**4. E46 is ruled out for that crash, by its own stated condition.** E46 —
filed by the mac box hours earlier, and it asks in its own words not to be quoted
as a measured crash — is a `map::end()` deref reached when a document names a
module handle the document does not contain. I parsed the crashing document for
every handle-bearing attribute (`Module`, `module`, `tiedtomod`, `fMod`, `tMod`,
case-insensitive) against every `<Module Id=>` / `<module handle=>` it defines:
**25 handles defined, 13 referenced, zero dangling.** The condition is absent.
A measured crash and a plausible nearby defect are not the same finding, and
E46's own row is the reason to check rather than assume.

**5. A rack saved TODAY reloads — but raises a modal dialog and loses part of
the document.** Five prefabs inserted by double-click, quit with `taskkill /IM`
(the graceful path, which saves), relaunch:

```
session.xml 66,294 B, document 49,607 B
relaunch:  stops after "default rack loaded, 25257 byte document"
           alive, Responding=True, CPU 0.09 s over 30 s, no pipe name, no window
           MainWindowTitle == "Connectors lost while loading"
Jeff dismissed it:  restore completes
next line:  "DSP structure changed, pushing 46030 byte document"
```

**49,607 in, 46,030 out — 3,577 bytes gone, and the dialog names them as
connectors.** Filed as **E48**. This is PLAN's v0.1 clause *"the patch survives
save-and-reload"* failing; v0.1 was measured on a 14 KB document and this is
49.6 KB.

**The headless half of it is worse than the loss.** A native modal runs a nested
message loop on the main thread, so while it is up there is no code of ours on
the stack. E43 bounded the same mechanism one layer up last night, but E43's
deadline needs a connection to answer on and **this dialog is raised before the
command channel opens** — there is nothing to bound and nothing to answer. The
only handle a caller has is the Win32 `MainWindowTitle`, which is not an
interface and has no equivalent on the other two platforms.

Jeff asked, in session, whether dialogs should be reported over the channel and
told me to file it as a `PROPOSED:` decision rather than a bug. It is in
[docs/decisions.md](docs/decisions.md), with **E51** as the parked row. The part
worth repeating here: all three options (report / report-and-answer / never
raise) need the same prerequisite, **one app-owned function that every prompt
goes through**, and that prerequisite is buildable today because it commits to
none of them.

**6. And then the control I was trying to establish fell over, which is the
finding I least expected.** I wanted a third-party-free rack so E48 could not be
dismissed as a VCV problem. **I never got one.** On a launch with an EMPTY config
folder, from a default rack whose file contains no VCV module of any kind:

```
DefaultRack.synthedit  25,257 B at bed03b0a0,  grep -c Compare == 0
                       (repo copy and staged bundle copy, same bytes)

app:  "default rack loaded, 25257 byte document"
      RackEditor: 'Compare' model=yes art=yes(res/Compare.svg)
      "building rack from 18183 byte document"
      RackProcessor: 'Compare' constructed / processing (block 96)
                     first NONZERO OUTPUT pin 5 (1.000000)
```

It is in the DSP graph and running. **The control that makes it mean something:**
the same binary launched with a session file present builds a **13,464**-byte
rack and constructs **no** Compare at all — same executable, same bundle, same
default-rack file. So it is the default-rack path specifically, not a property of
the build. Filed as **E50** with the hypothesis stated as a hypothesis: if a
module resolves to the wrong class once the VCV pack is registered — the **S46**
shape — then mismatched pins would drop connectors (E48) and a larger document
could construct something inconsistent and fault (E49). **One cause, three
symptoms, untested.** The test is one build with `TIDE_VCV_FUNDAMENTAL=OFF` and
one number to compare; I did not run it and the row says so.

### A correction I made to my own row before pushing it

E48's first draft said the loss happened *"on TIDE's OWN modules, with no
third-party module involved"*. **That was wrong** — the rogue Compare was in the
rack the whole time, and I only found it afterwards while trying to build the
control. The claim was the most load-bearing sentence in the row and would have
sent whoever took it looking in the wrong repo. Corrected in place, with the
correction left visible rather than tidied away. **The general form: a "control"
you assembled but never verified is not a control**, and I asserted its
third-party-freeness from the source file rather than from the running app,
which is exactly the direction E19's own lessons warn about.

### What I did NOT do, stated rather than implied

- **The VST3 cell is not measured.** REAPER was never launched this session.
  Everything above is the standalone.
- **E50 is not tested**, only observed with a control.
- **Nothing was built for macOS or Linux**, and none of these three defects has
  been checked on either.

### I said there was no debugger on this box. There is, and it gave me the crash.

**This is the run's own worst mistake and it nearly shipped in a row.** I searched
for `cdb.exe` with

```
Get-ChildItem 'C:\Program Files\WindowsApps' -Filter cdb.exe -Recurse -ErrorAction SilentlyContinue
```

got nothing back, and wrote *"there is no `cdb.exe` on this box"* into E49.
**`WindowsApps` denies directory LISTING**, so `-ErrorAction SilentlyContinue`
swallowed an access-denied and handed me an empty result that I read as absence.
My own memory note had the exact path all along, and probing it directly answers
`True`:

```
C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2603.20001.0_x64__8wekyb3d8bbwe\amd64\cdb.exe
```

**This is the same failure the archive already records** — *"An empty result from
a missing input is not evidence. Check the input exists before believing an empty
result."* (2026-08-25, macos). I hit it in the same shape, from the same kind of
suppressed error, and I only caught it because I went to correct the memory note
and re-read what it actually said.

**With the debugger, E49 stops being a characterisation and becomes a diagnosis:**

```
ExceptionAddress: TIDE_Rack!ug_patch_param_setter::ConnectParameter+0x5e
ExceptionCode:    c0000005 (Access violation)
Parameter[0]: 0            <- a read
Parameter[1]: 0x64         <- the address read: offset 0x64 from NULL
faulting insn: mov ecx,dword ptr [rbp+64h]
next insn:     call TIDE_Rack!HostControlisPolyphonic
```

That pair of instructions is
`HostControlisPolyphonic(parameter->getHostControlId())` with **`parameter`
null**, and the null is made two lines earlier in
`SynthEditLib/ug_patch_param_setter.cpp:172`:

```cpp
auto parameter = parent_container->get_patch_manager()->GetParameter(moduleHandle, moduleParameterId);
ConnectParameter(parameter, plug);      // nothing checks it
```

The only guards on the receiving overload are `assert`s and TIDE ships `-DNDEBUG`.
**Called from `ug_base::HookUpParameters` (`ug_base.cpp:745`) on the WASAPI render
thread** — `AudioDriverWasapi::renderThread` → `processAudio` → `prepareToPlay` →
`BuildDspGraph` → `BuildModules` → `Setup` — so the DSP graph is built on the
audio thread and this kills the render thread rather than surfacing as a load
error.

**And it sharpens the E46 relationship rather than settling it.** It is E46's
SHAPE — an assert-only guard on a lookup that can miss — at a different site in a
different file, and it is the measured crash E46 says it lacks. But E46's own data
condition really is absent here (25 handles defined, 13 referenced, zero
dangling), and **`GetParameter` takes a `(moduleHandle, parameterId)` PAIR while my
document test only checked module handles** — so the miss comes from the parameter
id on a module that does exist. That is narrower and more findable than E46's case,
and I would not have known it without the faulting address.

### Bookkeeping

**X2** was IN-REVIEW with [#484](https://github.com/JeffMcClintock/TideSynth/pull/484)
merged and nothing open, and its own words say *"ACCEPT MET FOR TIDE'S OWN CODE;
NOT MET TREE-WIDE"* — so DONE would be false. Back to **TODO**, per the **E32**
precedent, with the remaining work split in the row: a tree-wide zero is a
decision about GATED `SynthEditLib`, not a task, while the row's own
*"NOT VERIFIED"* half is Windows and macOS and **is** takeable. **E43** was
already DONE by the time I looked; both its PRs are merged.

**An ID COLLISION, caught before either side merged.** I filed my dialog row as
`E47` at 11:20; the mac box filed a different `E47` (*the properties pane can
be left pointing at a freed module*) at 11:28, on branch
`tide/mac/E47-properties-dangling-module`. Mine was first by eight minutes and I
renumbered it anyway, to **E51** — theirs is already cited from an open
[SynthEditLib#59](https://github.com/JeffMcClintock/SynthEditLib/pull/59) and from
its own branch name, so moving it costs the fleet more than moving mine costs me.
**A23's duplicate check cannot see this**: both rows are legal on their own branch
and only collide once they meet on `main`. I found it because a push was rejected
and I read the fetch output instead of retrying — the new branch was in the same
three lines. **Worth a habit: after a rejected push, read what the fetch brought
back before pushing again.**

### Machine state

`main` is **green** — `build` and `verify` both `success` on `c106b6641`, and no
open `platform:*` issue on any platform.

**All six repos are on their default branches and clean.** `GMPI_Wrappers` was
one commit behind and was fast-forwarded to pick up E43's `#22`. Jeff's
`DefaultRack.synthedit` was modified in the working tree when this run started —
STEP 5's third kind of dirt, left untouched — and he committed it himself
mid-session as `bed03b0a0`, so the tree is clean now.

`%APPDATA%\TIDE Rack\` **cannot be redirected by environment** — Windows resolves
it with `SHGetKnownFolderPath(FOLDERID_RoamingAppData)`, which ignores `%APPDATA%`
— so the folder was copied to the session scratchpad before the first launch and
restored byte-for-byte afterwards, verified by md5. No TIDE process is left
running. The build tree is `C:\SE\_scratch\e19`, outside every repo.

**Learned:**

- **A deadline needs a connection to answer on.** E43 bounded a wedged command;
  a dialog raised during startup happens before the channel exists, so the same
  mechanism produces a failure nothing can report. The fix for that one is not a
  longer deadline, it is not raising the dialog.
- **`Responding=True` with 0.09 s of CPU is the modal-dialog signature**, and it
  is indistinguishable from a slow build unless you go looking for a window title.
  Worth checking before concluding "hung".
- **Rule a nearby defect out by ITS stated condition, not by impression.** E46
  and my crash look alike and are not the same; parsing the document for dangling
  handles took one command and turned "probably E46" into "measurably not E46".
- **A control has to be verified, not assembled.** I built a "third-party-free"
  rack out of TIDE prefabs and it had a VCV module in it from the first frame.
  The source file said one thing and the running app said another.
- **The command channel cannot lay out a rack.** Double-click inserts; drag does
  not move. Anything about position has to be authored in the document.
- **`-ErrorAction SilentlyContinue` turns "I was not allowed to look" into "it is
  not there."** `WindowsApps` denies directory listing; the suppressed
  access-denied cost me a wrong sentence in a filed row, and the debugger it said
  was missing is what turned that row from a description into a diagnosis.
  **Suppress errors only when you already know which error you are suppressing.**
- **Check a lint by its EXIT CODE, not by reading the first lines of its output.**
  I piped `check-id-refs.py` through `head -2`, saw the advisory, called it green,
  and CI failed on a stale reference sitting four lines below the cut.

**Next:** **E50** first, and it is cheap — one `TIDE_VCV_FUNDAMENTAL=OFF` build
and one document size. It is now the only unexplained one of the three: E49 has
its faulting line, E48's dialog names its own symptom, and both are consistent
with a parameter that no longer resolves. If E50 is the cause then all three go
together and E19's VST3 cell becomes reachable, because the whole obstacle today
was that no prepared rack reloads reliably. **E49's guard is one line and is
GATED**, so it is filed rather than written whichever way E50 goes.

**Branch/PR:** `tide/win/E19-vst3-feedback-leg` — TideSynth only. E19's row back
to TODO, E51/E48/E49/E50 filed, X2 flipped, the `PROPOSED:` entry, and this
entry. **No product code changed in any repo**, so there is nothing here that can
break a build.
## 2026-08-27 — macos — Recovered two days of uncommitted work out of a working tree, and filed the hole it left (interactive session, Jeff directing)

**Prompt:** standing backlog-loop instruction in the session · Opus 5, `claude-opus-5` · Claude Code · commits authored `Jeff McClintock` per the interactive convention

**Did:** `SynthEditLib` had been parked dirty for two days and every scheduled
run is told to leave it alone *for that reason*. Recovered the work, landed it
as [SynthEditLib#59](https://github.com/JeffMcClintock/SynthEditLib/pull/59),
and filed **E47** for the half it does not fix. Also merged
[#58](https://github.com/JeffMcClintock/SynthEditLib/pull/58) and
[TideSynth#483](https://github.com/JeffMcClintock/TideSynth/pull/483).

### The failure mode worth naming: a branch that promises a fix and holds nothing

`fix-patchmanager-dangling-properties-observer` had **zero commits** — 10 behind
`origin/main`, 0 ahead. All 33 lines of the fix were an **uncommitted edit in one
file**, untouched since 2026-08-25. No stash, no commit, no PR, not on main.

The dirt was load-bearing in the worst way: it was the only copy of the work
**and** the thing blocking the repo from syncing, and the standing instruction to
leave a dirty repo alone guaranteed nobody would look. **A dirty tree that
survives more than a day is not a WIP, it is an outage** — check whether the
branch actually holds anything before respecting it.

**The recovery, and the order matters:** worktree off current `origin/main`, copy
the file in, commit, push, and only THEN discard the working-tree edit — after
`git rev-parse origin/<branch>:<file>` proved the blob was on the remote. Base
blob was identical at both the stale HEAD and current main, so no rebase and no
conflict; the committed blob was byte-identical to the working file, so nothing
was reformatted in transit. This repo is CRLF — a text-mode round trip would have
rewritten every line and buried 33 real lines in a 4,000-line diff.

### E47, and why chasing a comment beat trusting it

#59's comment says the LEAF-module case is unfixed. Checking it, **two obvious
paths are already covered**: `CDocOb::OnDelete()` and
`CSynthEditDocBase::DeleteContents()` both open with
`NotifyFast(OM_SHOW_PROPERTIES, nullptr)`, which clears `currentModule`. So the
single-module delete and every document reload are fine.

What is left is one conditional: `~CContainer` runs `DeleteAll()` — which frees
children with a bare `delete d` and never calls `OnDelete()` — and only then
`if (m_patch_manager) delete m_patch_manager`. The pane's own `OM_DELETE` handler
clears `layoutContainer` and **not** `currentModule`. **A container with no patch
manager never reaches the line that closes the window.** Read off the call graph,
not observed faulting, and E47 says so.

### Worktrees are a place work goes to die quietly

Four were registered against `SynthEditLib`, two from a session days old, living
in `/private/tmp` — which macOS sweeps. One had an **uncommitted** `BundleInfo.cpp`
that looked like lost work and was byte-identical to `origin/main`: already-landed
#56. **Verify by blob before removing, and by blob before panicking.**
`git diff --name-only origin/main..HEAD` is the wrong instrument — it lists
everything main has moved past, so a consumed branch looks full of unique work.
Compare `git rev-parse <commit>:<file>` against `origin/main:<file>` instead.

### Merging into a moving main

`TideSynth#483` was CLEAN, then conflicted before the merge landed — X2 (#484)
had gone in touching the same `BACKLOG.md` and `JOURNAL.md`. Rebased, kept both
journal entries with the base one byte-for-byte intact, confirmed E46 had not
collided, re-ran every lint **against the new base**. `SynthEditLib` has no CI at
all, so its only real check is TIDE compiling against it: built `TIDE_Rack` with
all three changes applied and confirmed it **links**, which `-fsyntax-only` cannot
tell you. `main` green on all three platforms afterwards.

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
