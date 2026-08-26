# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

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

## 2026-08-26 — macos — M9: the iOS AUv3 has been instantiated, and it made a sound (scheduled run)

**Prompt:** "i merged stuff, sync repos, continue."

M11 and GMPI#18 merged, which unblocked this. STEP 4 first: nine IN-REVIEW rows
whose PRs are all merged flipped to DONE, verified with `gh pr view` rather than
from the merge commits alone — M6, M8, M10, M11, E31, E36, V6, V7, E30. M11
going DONE is what took M9 off `BLOCKED(M11)`.

**And `main` is green again.** The break I reported at the end of the last run —
`14a8fd3` deleting `RackModules/MidiCv.synthedit` while `seedRootMidiCv`
inserted it by name — was fixed by V6 (#452) making the root assembly a default
DOCUMENT, plus #459 staging it. Re-measured on a correctly-staged bundle rather
than taken on trust: **7 rack prefab(s) seeded, default rack loaded, 24894 byte
document, "rack is populated"**.

**M9 ITSELF: THE FIRST TIME THE iOS AU HAS EVER BEEN INSTANTIATED.** It could be
installed, launched and REGISTERED — all three verified over the past days — and
never once opened, because there is no AUv3 host in the iOS simulator to open it
with. The ruling's answer is that the container app hosts its own extension,
which it has to ship anyway.

**Accept met, and it is a real cross-platform comparison rather than a
self-fulfilling one:**

```
macOS AU3  (tests/e9_au_rate_probe.mm)   440.0093 Hz     <- the control, run FIRST
iOS  AU3   (container app hosts its own) 440.2062 Hz     <- 0.81 cents apart
```

Same preset both times — `tests/hosts/v1-rack.rpp` through
`scripts/decode_rpp.py --preset-out`, which is also E2a's and M7's 440 Hz. The
iOS render: out-of-process, 18893-byte GMPIPRESET through `fullState`, 2.00 s at
48 kHz, **peak 0.4846 rms 0.1412**.

**Establishing the macOS control first was the single best decision here.** Had
I gone straight to iOS and got a number, I would not have known whether a
discrepancy was the platform, the preset, my analyser, or my host. With the
control in hand, one number on iOS settles it.

**THE RULING'S ONE UNVERIFIED CLAUSE IS NOW MEASURED, AND THE QUESTION IS MOOT.**
`docs/decisions.md` asked for "the full entitlement preconditions for in-process
loading on iOS" to be checked before implementing. They are not merely unmet:

```
error: 'kAudioComponentInstantiation_LoadInProcess' is unavailable:
       not available on iOS
```

The option does not exist — it will not compile, let alone need an entitlement.
iOS loads every AUv3 out of process, which is the ruling's own default and its
stated reason, so nothing is lost but an escape hatch that was never available.
`--gmpi-in-process` is still ACCEPTED and REPORTED rather than silently ignored.
The decision entry now carries the measured answer.

**A trap that cost me three minutes and will cost the next person more.**
`simctl launch --console` attaches to the app's stdio and does not return until
the app EXITS. This is a GUI app; it never does. The probe hung for its full
180-second timeout on a render that had **already succeeded in four seconds and
was sitting on disk the whole time** — the most misleading failure available,
because the artifact was there and the tool said nothing. Launch detached, poll
for the artifact, then read NSLog back out of the unified log with
`log show --start`. That is now what the probe does, with the reason in a
comment beside it.

**One design point worth keeping:** the host finds its OWN appex by reading
`PlugIns/*.appex`'s `Info.plist`, not by taking the first `aumu` in the
component registry. The registry holds every AUv3 on the device, so the lazy
version would silently host somebody else's plugin the moment a second one is
installed — and the measurement would look perfectly fine.

**Split:** `tests/m9_ios_au_host_probe.py` and the bookkeeping are TIDE's; the
host itself (`GMPI_Wrappers`) and its two link flags (`GMPI`) are PR-GATED and
are PROPOSED, not merged.

**Still not verified:** a real device — this is the simulator only — and any
third-party host, of which iOS has none to try.

## 2026-08-26 — macos — `main` is red twice from one prefab update; one half fixed, one is Jeff's (scheduled run)

**Prompt:** "i merged stuff, sync repos, continue."

Found while linting an unrelated branch: **`check-prefab-layout` failed on a
tree whose only changes were BACKLOG.md and JOURNAL.md.** That is the tell — a
lint that a docs-only diff cannot possibly break is failing on `main`, not on
you. Checked it out clean and it fails there too.

`322df0f update prefabs` breaks **two** gates:

1. **`check-prefab-layout`** — `AR_jef.synthedit` now has
   `SE Label handle 1167319384 at (4632,3800)..(4668,3806)` overhanging the
   panel `(3962,3794)..(4010,4178)`. Bisected: at `6d813b3`, the commit before,
   all seven prefabs pass.
2. **The M6 rack-content gate** — the same commit also deleted
   `Midi.synthedit` and `Output.synthedit`, so `RackModules/` holds **5** while
   `EXPECTED_PREFABS` still said 7: `FAIL 5 rack prefab(s) seeded, expected 7`.

Both run in CI, so every PR inherits them.

**I fixed the count and not the prefab, and the split is deliberate.** The count
is mechanical — the constant is a MIRROR of `RackModules/`, and the gate's own
failure text says as much. Nothing loads the deleted files by name (checked; the
`MidiCv.synthedit` hits in the tree are comments and a CMake path note), and the
default rack supplies both roles, so 5 is simply what is there. The label
overhang is prefab DATA, authored in SynthEdit, and re-authoring someone's
prefab is not a lint fix.

**THIS IS THE SECOND TIME IN ONE DAY, and that is the durable point.** The
earlier one was `14a8fd3` deleting `MidiCv.synthedit` while `seedRootMidiCv`
still inserted it by name. Same shape: a prefab file removed, a constant or a
call site left pointing at it, and a gate that only notices on a CLEAN stage —
because `copy_directory_if_different` merges and never deletes, so an
incremental build keeps serving the file that is no longer in the repo. My
local build said "7 rack prefab(s) seeded" until I deleted the staged folder
and forced a relink.

That staleness is worth more attention than either break: **a developer editing
prefabs cannot see this failure without a clean stage**, so it will keep
reaching CI. M11 fixed exactly that for the iOS path (`rm` before `cp`); the
macOS/Windows staging still merges. **Filed as E40**, with M11's generated
script as the worked example — written one day before this bit twice.

One detail for whoever takes it: deleting the staged `Prefabs/` folder is NOT
enough to re-measure. The POST_BUILD that restages only runs when the target
LINKS, so a rebuild after deleting the folder leaves it absent and the gate
then fails for a different reason. Delete the executable too.

## 2026-08-26 — macos — E30: the watchdog could not see the questions it exists to surface (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... Verify against
the row's ACCEPT, not a tool's exit line."

**The fix is four lines and the reasoning behind the bug is the interesting
part.** `check_proposed` skipped fenced code blocks before matching
`^PROPOSED:`, to avoid the escalation template's literal
`PROPOSED: <one-line question>` example. That is the right problem solved by the
wrong discriminator: **the template PRESCRIBES the fenced form**, so every real
entry is fenced too, and the skip swallowed all of them. The guard that actually
works was already sitting in the same `if` — `'<one-line question>' not in line`
— excluding the example BY CONTENT, which cannot be fooled by where the line
sits or by the template moving.

**Measured both sides, and the BEFORE is from the live issue rather than from
reasoning.** [#44](https://github.com/JeffMcClintock/TideSynth/issues/44) read
*"Open PROPOSED questions in docs/decisions.md: None."* while two were open —
V7's naming question and V4's rack-relevance predicate, the latter invisible
there since the day it was written. After: `--dry-run` prints both, and the
template's example is still excluded. `--selftest` still 4/4.

A6 built this digest as *"the single awaiting-Jeff surface"*, so a question it
cannot see is a question nobody is asked. That is what makes this a silent
failure of the thing's whole purpose rather than a formatting nit.

**I TRIPPED AN EDGE OF MY OWN AND IT BELONGS HERE.** Measuring the BEFORE, I ran
`python3 scripts/watchdog-digest.py` **without `--dry-run`** and it refreshed
issue #44 for real. Harmless in outcome — the digest exists to be refreshed and
what it posted was the true (buggy) state — but it was an outward-facing write I
did not intend to make. **The script posts by default and `--dry-run` is
opt-in**, which is the wrong way round for a tool a person is likely to run
first out of curiosity. Not changed here, because inverting a default is a
decision rather than a fix; noted on the row for whoever wants it.

**#44 is deliberately left stale.** Running the digest from this branch would
publish output generated by code that is not on `main`. The next scheduled run
corrects it once this merges.

## 2026-08-26 — macos — E26 closed by measurement: the chunk's own tag says the save refreshed it (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... Verify against
the row's ACCEPT, not a tool's exit line... If you cannot measure it, write
'unverified' and say exactly what is missing."

**No commit was needed. The defect cannot occur on today's `main`, and both
halves of the row's premise have been fixed since it was written.**

E26 asks *"whether `syncState()` ... is meant to be the hook — nothing in GMPI
or the wrappers calls it today"*. Both parts are now false:

- `StandaloneHost::syncPluginState()` **calls it**, immediately before capturing
  state, and its comment names this exact case: *"for one whose real state lives
  behind a chunk parameter (TIDE's document) it is the difference between saving
  the patch and saving a stale copy."*
- `SynthEditController::syncState()` **implements it**, re-exporting through
  `exportChunkXmlForSave()`. Its comment closes the row in one sentence: *"until
  this existed a knob tweaked after the last structural edit was NOT in the saved
  file."*

**BUT READING TWO COMMENTS IS NOT A MEASUREMENT, and this row deserved one.**

It turns out the artifact answers the question itself. `ChunkPrefix.h` puts a
four-byte tag on the front of every chunk recording WHY it was written:

```
TDb1  Build   a structural push from serviceDocumentSync
TDs1  Sync    the SAVE-TIME REFRESH, written only by syncState()
(none)Legacy  written before the tag existed
```

So "did the save re-export the chunk?" is answered by the **first four bytes of
any saved file**, with no instrumentation, no build flag and no staged failure.

**Eleven session files checked — the nine this run's driven standalones wrote,
plus Jeff's own live `session.xml` and its `session.previous.xml` backup — and
all eleven carry `TDs1`.** Not one `TDb1`, not one Legacy. Every save on this
box re-exports from the live document.

**A corollary worth enjoying:** those are the same four bytes that made
`e5_rack_footprint_probe.py` die with *"not well-formed (invalid token): line 1,
column 4"* until E36 taught it to skip them this afternoon. The prefix that was
an obstacle in one row is the evidence in this one.

**WHAT I TRIED FIRST AND WHY IT DID NOT WORK, because the next person will reach
for it too.** I went looking for a value edit to drive through the GUI: tweak
something, save, see it survive. Two dead ends:

1. A patch-cable **ADD** looks like a value edit and is not. It trips `dspDirty`
   through a `SuspendDSP` guard and pushes the document structurally — observed
   directly, `TIDE: DSP structure changed, pushing 18049 byte document`. That
   exercises the path E26 is *not* about. (E28's journal already recorded this
   as the reason cable-add "worked" before its fix; same trap, different row.)
2. A cable **REMOVE** does not trip it and is the right edit — but driving one
   is unreliable because of **E34**, which is precisely that the cable-end drag
   does not end.

The tag reads the answer off the artifact instead of trying to stage the
failure. When a mechanism records what it did, measure that, rather than
building a scenario to watch it.

## 2026-08-26 — macos — E25: the SIGSEGV is a missing null check, and the trigger is not the timer (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... If you cannot
measure it, write 'unverified' and say exactly what is missing. Never imply a
measurement you did not take."

That last sentence is what this entry is shaped by. **The cause is found; the
fix is GATED; the on-demand repro is still open.** E25 stays TODO.

**A SECOND CRASH REPORT EXISTS, and it is better evidence than the one the row
was filed on.** `TIDE-Rack-2026-08-26-021533.ips`. Worth knowing: the original
`...-2026-08-25-162615.ips` that E25 quotes is **already gone** — macOS rotates
`~/Library/Logs/DiagnosticReports` and only one TIDE report survives today. So
the stack is copied into the row rather than pointed at. Do the same next time;
a row that cites a file the OS deletes is a row that loses its evidence.

**THE TRIGGER IS SESSION RESTORE, NOT THE SYNC TIMER.** E25 reasons its way to
"a document STATE or an interaction, not the timer", which is half right and
stops one step short. The new stack names it:

```
start / main / runStandaloneApp / SessionState::restore /
StandaloneHost::restoreState / notifyControllerOfPreset /
SynthEditController::setParameter / TideApp::importChunkXml /
TideApp::exportChunkXml / CContainer::ExportXml / CPatchManager::ExportXml /
PatchParameter_base::ExportXml / CContainer::getIgnoreProgramChange /
CUG::GetPlug(int)
```

`importChunkXml` ends with `lastPushedShape = documentShape(exportChunkXml());`
(`TideApp.cpp:487`) — the export runs on a document rebuilt microseconds
earlier. `SynthEditGui::onTimer` is not on this stack at all. The row's own
"relaunched and let the timer tick for 100 s and it did not crash" was
therefore the right experiment against the wrong hypothesis.

**THE DEFECT, and it is one line.** `CContainer::getIgnoreProgramChange()`
(`CContainer.cpp:1654`):

```cpp
return GetPlug(PN_IGNORE_PC)->GetDefault().compare(L"1") == 0
       || (Container() && Container()->getIgnoreProgramChange());
```

`CUG::GetPlug(int)` (`CUG.cpp:1710`) **returns `nullptr` by design** when the
index is out of range, and its comment says exactly what happens next:

> callers that already null-check recover cleanly, and **the rest fault at a
> known point** instead of dereferencing whatever the out-of-range read
> produced.

`getIgnoreProgramChange` is one of "the rest". The report's
`KERN_INVALID_ADDRESS at 0x50` is a member access on a null `this`, which is
what `GetDefault()` on a null plug looks like.

**Why index 3 is the fragile one, and this is the part worth remembering:**
`PN_IGNORE_PC` is 3, and a container's pin table (`ug_container.cpp:198`)
declares exactly indices 0–3 with `Ignore Program Change` **LAST**. A container
whose `Plugs` vector is short by a single entry faults here and nowhere else —
so this accessor is the canary for an incomplete plug table, and it screams by
segfaulting.

**A second unguarded deref one frame up**, and fixing only the first would just
move the next report by a line: `PatchParameter_base::ignoreProgramChange()`
(`PatchParameter.cpp:1297`) is
`m_ignoreProgramChange || (module() && module()->Container()->getIgnoreProgramChange())`
— it guards `module()` for null, with a comment explaining why, and then
dereferences `module()->Container()` unguarded. A module directly under the
master container takes that one.

**WHAT DID NOT REPRODUCE — negative results, recorded as evidence rather than
omitted:**

1. Restoring the live `~/Library/Application Support/TiDE Rack/session.xml`
   into an isolated `HOME`: **clean**. (Its master container serialises
   `plugs=1`, which looked damning and is not — the table is topped up.)
2. A `TIDE_VCV_HETRICKCV=ON` build — 66 modules registered — with three ported
   modules inserted by double-click, one selected to open its properties pane
   (**this row's own stated lead**), saved and restored: **clean**.

So it is not "a HetrickCV module is present", and it is not time-alone. **The
one unknown left is which document state leaves a container's plug table short
of four** — a far smaller target than "a crash somewhere in serialisation", and
the next run should start there rather than re-deriving any of the above.

**The fix is GATED** (`SynthEditLib/EditorLib`), so it is filed, not written.
Both lines are in the row.

## 2026-08-26 — macos — V7: the override hook, and the gate that turned out not to be one (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... GATED =
SynthEditLib, SE16/EditorLib, SE16/SynthEdit2. Don't edit; file the gated half."

**THE ROW SAID THIS NEEDED A GATED FILE. IT DOES NOT, AND THAT IS THE WHOLE
FINDING.** V7 reasons that the context-menu items "come straight from
EditorLib's `populateContextMenu` and TIDE has no hook, so the hook must exist
before any string can differ" — which reads as: the hook goes where the items
are made, in `SynthEditLib/EditorLib`, which is GATED.

It does not have to. `SynthEditGui.cpp` already receives the host's sink and
hands it **straight through** to `view->populateContextMenu`. Put a TIDE-owned
`IContextItemSink` in that gap and every item EditorLib adds passes through
TIDE on its way to the menu. `IContextItemSink` is one method —
`addItem(text, id, flags, callback)` — so the wrapper is about thirty lines,
and **`SynthEditLib` is not touched at all**.

Worth generalising: "the string is made in a gated file" does not imply "the
change is in a gated file". Ask where the string is *delivered*.

**THE TABLE SHIPS EMPTY, AND THAT IS THE POINT.** V7 says in as many words:
*"Do not land the override carrying placeholder strings."* The four names turn
on what a TIDE user expects to read, which is a product decision still open as
a `PROPOSED:` question. The row's own suggested scheme (`Goto Panel` /
`Goto Structure`) is SynthEdit vocabulary — in TIDE a "panel" is the rack and a
"structure" is the inside of a module, which is the exact mismatch the ruling
names. Guessing would be worse than waiting.

**WHICH LEAVES A HOOK WHOSE ONLY OBSERVABLE BEHAVIOUR IS "CHANGES NOTHING".** A
hook nobody has watched work is not a hook — the same argument
`tests/rack-content/` makes for its negative controls. So
`tests/v7_menu_override_probe.cpp` drives the sink directly with a table of its
own: **16 checks, 0 failures.** A table entry renames; a near miss does NOT
(`Pa&nel Edit...` is a *different* EditorLib item from `Panel Edit...`, and a
substring rule would silently catch both while hiding that there are two); an
unlisted item passes through; `id`, `flags` and the callback pointer are
forwarded untouched; `queryInterface` returns the wrapper for
`IContextItemSink` and **delegates everything else**, which is what keeps a
host that also implements `IPopupMenu` working. And one check that stops the
probe becoming a rubber stamp: that the SHIPPED table is still empty, so this
passing can never mean TIDE has quietly started renaming things.

**A LATENT BUG FIXED ON THE WAY.** `sinkRef` was block-scoped, so the reference
`queryInterface` returns was released before the new wrapper could forward to
it. It survives today only because the hosts implementing this are
`GMPI_REFCOUNT_NO_DELETE` and `release()` is a no-op — a coincidence, not a
guarantee, and this file already carries a comment about the last time that
distinction mattered. Function-scoped now.

**NOT VERIFIED, and stated rather than implied: the menu was never re-checked
ON SCREEN.** The command channel has no right-click — `cmdPointer`'s only
option is `--double` — so a context menu cannot be raised headlessly. The
change is a pass-through with an empty table and the standalone builds and
runs, but that is an argument, not a measurement, and the difference is the
whole reason this journal exists.

Filed as **E38**, and it is the same shape as the gap `--double` closed: E31
recorded *"the command channel cannot drive an insert gesture"* and handed a
verification to a human; E35 added one flag and E36 was measurable end to end
in a script the next day. A `--right` modifier buys the same thing.
`CommandDispatcher.cpp` is **PR-GATED**, so E38 says propose, do not merge —
and says to check first whether `gmpi::api::PointerFlags` can even express a
secondary button, because if it cannot this is a GMPI change and a bigger row
than it looks.

## 2026-08-26 — linux — three boxes fixed one duplicate ID in two minutes, and a duplicate breaks two checks not one (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** STEP 1 — closed [#430](https://github.com/JeffMcClintock/TideSynth/issues/430)
by building `main` (492/492 rc=0, all four Linux artifacts). Then found the
duplicate `E34` and fixed it — **and so did two other boxes, inside two
minutes.** This entry keeps what survived the collision.

### The race

| PR | box | opened |
|---|---|---|
| [#447](https://github.com/JeffMcClintock/TideSynth/pull/447) | windows | **merged** — landed the fix |
| [#445](https://github.com/JeffMcClintock/TideSynth/pull/445) | macos | 02:33:48 |
| [#446](https://github.com/JeffMcClintock/TideSynth/pull/446) | linux (mine) | 02:34:04 |

**Sixteen seconds** between the mac PR and mine. All three renumbered the *same*
row — the insert-stacking pile → `E36` — and all three gave the same reason for
choosing that one over the cable row: `JOURNAL.md`'s citation of `E34` is
append-only and cannot be corrected, so the row with only backlog-side
references is the one that moves.

**Three independent runs converging on the identical repair is a good sign about
the rules and a bad sign about the coordination.** STEP 2's collision check
(`git ls-remote`, `gh pr list`) is run *before claiming a backlog item* — but
none of us was claiming an item. We each found a red lint while doing something
else and fixed it on the spot, which is exactly the path that check does not
cover.

My PR is rebuilt as a **delta on top of theirs**, per STEP 2's rule for a
collision discovered after the fact. Everything already on `main` — the
renumber, and the ID-correction note on `E31` — is dropped from it.

### What survived, and it is the part worth keeping

**A duplicate ID breaks TWO checks, and only one repair direction passes the
second one.** `check-id-refs.py` names the duplicate. `check-backlog-diff.py`
never mentions it — it parses rows into a **dict keyed by ID**, so a duplicate
silently *collapses* and the last occurrence wins. Its baseline for `E34` is
therefore the cable-drag row, and I measured both repairs against it:

| renumbered | `check-backlog-diff` |
|---|---:|
| cable-drag → E36 | **exit 1** — `E34: Item column differs` |
| insert-stacking → E36 | **exit 0** |

So the choice of which row to move was **forced by a dict's last-wins ordering**,
not by which row was newer or better-referenced. The other boxes reached the same
answer from the journal-is-append-only argument, which is a different and equally
valid route — but it happens to agree only because the two constraints point the
same way here. **They need not, and nothing checks that they do.**

### The third instance of a trap this journal already documents twice

My first run of the lint was:

```
python3 scripts/check-id-refs.py 2>&1 | tail -6; echo "rc=$?"
```

That reports **`tail`'s** exit status, and the duplicate block was above the
lines `tail` kept — so it printed a clean summary and `rc=0` while a duplicate
sat in front of it. **I spent a while investigating the check before checking my
own command.** The lessons *"check a lint's EXIT CODE, never grep its output"*
and *"`$?` after a pipeline is the LAST command's status"* are both already here.

### `__pycache__` was never ignored

Loading a check through `importlib` to inspect its internals leaves
`scripts/__pycache__/*.pyc`, and one got committed before I noticed. `.gitignore`
had no entry — that is what this PR adds, and it is the only file change.

**Verified:** `main` builds 492/492 rc=0 · `main`'s duplicate is gone (1 `E34`,
1 `E36`, `check-id-refs` exit 0) · both repair directions measured against
`check-backlog-diff` · no `.pyc` is tracked on `main`.

**Not verified:** neither E34 nor E36 was investigated as a defect; both are the
mac box's findings.

**Learned:**

- **A duplicate ID is not one broken check.** The one that reports it is not the
  one that constrains the fix; `check-backlog-diff` collapses duplicates by dict
  and then rejects the repair from the wrong side, silently.
- **Two correct-looking arguments can agree by luck.** "Renumber the row the
  append-only journal does not cite" and "renumber the row that is not the dict's
  last-wins baseline" gave the same answer here and are not the same rule.
- **`cmd | tail -n; echo $?` cannot report the command's status**, and I proved
  it for the third time in this project. Redirect to a file, echo `$?`, read the
  file.
- **STEP 2's collision check does not cover opportunistic fixes.** It guards
  claiming an item; three boxes fixed the same red lint in two minutes because
  none of us was claiming anything.
- **Losing a race is cheap if you rebase to the delta.** Everything on `main`
  came out of my branch in one command; what was left was genuinely mine.

**Machine left clean.** Scratch worktrees removed; the stray `scripts/__pycache__`
deleted from Jeff's checkout as well as from the branch. All six repos on their
default branches and clean.

**Branch/PR:** `tide/linux/E34-duplicate-id` — `.gitignore` and this entry.
**No backlog row and no product code change**, both dropped as already-landed.

## 2026-08-26 — macos — E36: inserts fill the row now, and the fix made a second bug visible (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... The standalone
is drivable: /tmp/gmpi-standalone.<uid>/gmpi-standalone.<pid> ... Use
`--pointer-down x,y --double` for a double-click (rapid clicks do NOT work).
Run under an isolated HOME; read geometry back from session.xml (base64 in
`<Param id="1">`, skip a 4-byte prefix) via tests/e5_rack_footprint_probe.py."

Every clause of that was load-bearing. E36 was verifiable at all only because
E35 landed the drivable channel yesterday; the row's own last line still says
"Verifying this needs a HUMAN or E35".

**THE FIX.** `SynthEditGui.cpp`'s insert has only ever been handed one point,
the view centre, so E31's snap made the pile tidy without making it a pile of
one. Now: walk the rack's top-level modules with `it_doc_ob`, take each
`getViewObRect(CF_PANEL_VIEW)`, and scan rows-first / columns-left-to-right at
`snapWidth` for the first rect the new footprint clears.

**Three decisions inside that are worth more than the search:**

1. **`AddPrefab`, not `AddModule`.** `AddModule` answers `-1` for any id
   beginning `*P=` — which in TIDE is EVERY rack module, they are all prefabs —
   so there was no handle to move. `AddPrefab` is implemented on top of it, so
   the insert is the identical call; it just also says what got created.
2. **Insert first, THEN move.** The free slot depends on the module's WIDTH and
   nothing knows that until the module exists. Known limit, left visible rather
   than papered over: the undo checkpoint `AddModule` takes internally records
   the original point, so a REDO re-inserts at the centre. The checkpoint is
   SynthEditLib's and GATED.
3. **The group moves together.** A prefab may hold several top-level modules;
   one offset for all of them keeps the layout its author chose. Scattering a
   prefab would be a worse bug than the pile.

**MEASURED, A/B, same script both sides, only `SynthEditGui.cpp` different:**

```
origin/main   7 violations -- SIX overlaps: 18432 sq between each pair of the
              three inserts (a full 48x384 module exactly on a 48x384 module),
              plus 8832 sq from each onto the seeded MIDI-CV
with E36      1 violation, and "ok  no overlaps among 5 placed module(s)";
              the three abut at 3732..3780, 3780..3828, 3828..3876
```

**THE PROBE STILL EXITS 1 AND THAT IS NOT THIS ROW.** The survivor is `MIDI In`
— an 8x14 panelRect, 0.533 HP, off-grid — **identical in the baseline run**, so
pre-existing. The run prompt warns about exactly this ("a probe can print
RESULT: FAIL for criteria the row never claimed"), and the only reason I can
say "pre-existing" rather than "probably pre-existing" is that the A/B was run.

**Tooling: the probe now reads a standalone session on its own.** It died with
`not well-formed (invalid token): line 1, column 4` on the four-byte `TDs1`
magic in front of the XML declaration — which is why the run prompt has to
explain those four bytes to every new session. It skips to the first `<` rather
than a fixed offset of 4, so a future `TDs2` needs no change.

**AND THEN THE INTERESTING PART: THE FIX IS CORRECT AND THE USER CANNOT SEE
IT.** Row 0 column 0 is the rack ORIGIN, (3732, 3732) — V5 records the same
number. The V3-seeded root MIDI-CV, the only thing on a fresh document, is at
(480, 464): **not on the rack at all**. So three correctly-placed modules went
3200 DIPs away and the window did not change.

I wrote a scroll-to-reveal for that. Then I measured it, and **it can never
fire**:

```
E36PROBE placed=3732,3732,3780,4116  visible=3734,3734,4234,4234  onScreen=1
```

`getVisibleRect(CF_PANEL_VIEW)` is computed from the CONTAINER's stored view
centre — the canvas centre (3984, 3984) with the no-view-open half-size of 250
— so by the document's own reckoning the module is on screen. The document is
not lying; the **live pane** is the thing out of step with it, which is exactly
what E33 says ("the document already stores them; TIDE throws them away on
every open").

**So the reveal was DELETED rather than shipped**, with the probe output kept
in the comment where it happened. Untestable code that looks like it handles a
case it cannot handle is worse than no code, and once E33 makes the live view
agree with the document the question changes shape anyway. Filed as **E37**,
with the split already done: (a) seed the root MIDI-CV at the rack origin —
cheap, standalone, and it may make (b) evaporate; (b) is E33's.

**Instrument, for the next run:** `--pointer-down x,y --double` then
`--pointer-up x,y` over the unix socket, batch terminated with `--ping <token>`
and read until the token echoes (that is what `mcp/src/session.ts` does).
Coordinates: the screenshot is CANVAS pixels, pointer input is DIPs, so divide
by `scale` from `--info`. Quit with SIGTERM — the quit-save is what writes
`session.xml`.

## 2026-08-26 — macos — M11: the iOS AUv3 has never actually been installed, and four bugs say why (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... Build trees go
stale silently. If you hit 'no member named X', suspect a pinned dependency
before your own code." The advice generalises: I hit a build break, suspected my
own configure, and it was neither — it was `main`.

**How this started.** M9 was the topmost takeable row (S1b and S8 are wholly
GATED, M8 was mine an hour earlier). M9 needs an iOS build, so I made one. It
did not build. Four times.

**FOUR BUGS, ONE ROOT CAUSE, AND ALL FOUR ARE INVISIBLE UNLESS THE TREE IS
FRESH.** That last clause is the whole reason they have survived: three of the
four are skipped or already-satisfied in an incremental tree, so every iOS build
anyone has done since they appeared has passed.

1. **`plist_util_host` links against the wrong platform.** It is compiled with
   `xcrun -sdk macosx`, but it runs inside an Xcode script phase, and Xcode
   exports `IPHONEOS_DEPLOYMENT_TARGET`. clang reads that and decides the target
   OS is iOS; `-sdk` only picks the sysroot. So: macOS sysroot, iPhone target,
   link dies on the first system dylib. One command line, run twice, is the
   entire proof — unset gives rc 0, `IPHONEOS_DEPLOYMENT_TARGET=18.0` gives
   `ld: building for 'iOS', but linking in dylib ... built for 'macOS'`.
   Invisible incrementally because the custom command has an `OUTPUT`.

2. **CMake tells Xcode the `.appex` is an application.** productType comes from
   the TARGET type, not from `BUNDLE_EXTENSION`, so Xcode adds an application's
   Validate phase and it rejects the thing it was just handed:
   `unknown application extension '.appex: expected '.app' or '.ipa'`.
   **`VALIDATE_PRODUCT=NO` does not disable it** — I passed it on the xcodebuild
   command line, confirmed it in the recorded invocation, and got the identical
   error. The phase belongs to the product type. Declaring
   `com.apple.product-type.app-extension` removes it at the source.

3. **The resource staging wrote into a directory named after an unexpanded
   variable — and this one shipped.** `$<TARGET_BUNDLE_CONTENT_DIR:...>` carries
   Xcode's `${EFFECTIVE_PLATFORM_NAME}`, and CMake writes it into the script
   phase with the **dollar escaped**, so Xcode does not resolve it and neither
   does the shell. `cmake -E copy` does not mind an odd path, so it cheerfully
   created a LITERAL directory:

   ```
   Release-iphonesimulator/TIDE-Rack.appex     Info.plist PkgInfo TIDE-Rack
   Release${EFFECTIVE_PLATFORM_NAME}/...       + all four XMLs + Prefabs/
   ```

   **That is the M5 defect, live on iOS, on a green build.** No control pins, an
   empty rack module browser, no MIDI jacks — precisely what M6's gate exists to
   catch and what M8 (this morning) taught the plugin to say out loud. Nothing
   caught it because no check can tell a copy into the wrong directory from a
   copy into the right one.

4. **`plist_util`'s Info.plist rewrite went to the same junk path**, so the
   appex kept CMake's stub: `CFBundlePackageType APPL` instead of `XPC!`, the
   un-prefixed bundle id M2 had already fixed once, and **no `NSExtension` key
   at all**. `simctl install` then fails with "Failed to create app extension
   placeholder", which names nothing useful.

**AND THAT CORRECTS THE RECORD ON M2 AND M10 — neither of which was wrong about
what it measured.** M2's `simctl install` rc=0 and M10's home-screen screenshot
are both true **of a container app with an empty `PlugIns/`**. An app with no
extension installs fine and shows its icon. M2's own "Not verified: the app was
installed, **not launched**, and the Audio Unit was never opened in an iOS host"
is the sentence that was doing the work, and it was right.

**A TRAP I FELL INTO AND THEN MEASURED MY WAY OUT OF.** When install first
failed *after* my fixes, the appex had gained resources — including a `Prefabs/`
subdirectory — and M2's journal says an appex with a SUBDIRECTORY fails to
install. That is a very plausible story and it is wrong. Bisected it the way M2
bisected the original, five variants, re-signed each:

```
everything                  rc=2      no XMLs, Prefabs kept    rc=2
no Prefabs/ dir             rc=2      empty Prefabs/ kept      rc=2
nothing but binary + plist  rc=2
```

All five. So it was never the content — it was bug 4, the plist. **The prior
journal entry supplied a ready-made explanation that fit the symptom and was not
the cause.** Cheap to check, expensive to assume.

**I also hit the `rc=$?`-after-a-pipe trap** — `out=$(... ) | tail` reporting
`tail`'s status — and printed "install rc=0" for an install that had returned 2.
[#446](https://github.com/JeffMcClintock/TideSynth/pull/446) documented that
exact trap the same evening; I read it and then did it anyway. Every rc in this
entry is captured before anything else runs.

**MEASURED, from clean, after all four:**

| check | result |
|---|---|
| clean `-G Xcode -DCMAKE_SYSTEM_NAME=iOS` build | `** BUILD SUCCEEDED **`, 0 errors |
| `Release${EFFECTIVE_PLATFORM_NAME}` | gone, nowhere in the tree |
| appex contents | 4 XMLs + `Prefabs/` (9) + binary |
| appex plist | `XPC!`, `...au3app.extension`, `NSExtension` present |
| `simctl install` / `launch` | **rc=0** / **rc=0** |
| `pluginkit -mAv` on the simulator | `com.tidesynth.tiderack.au3app.extension(0.1.1)` |

That last row has never happened. The extension had never been inside the app.

**Split:** item 3's TideSynth half is in this PR (`SynthEditSem/CMakeLists.txt`,
ALLOWED); items 1, 2 and 4 are GMPI and therefore GATED — proposed as
[GMPI#18](https://github.com/JeffMcClintock/GMPI/pull/18), **not merged**. **M9
is BLOCKED(M11)** until that lands: it cannot host an extension that will not
install.

**Also this run:** M8 shipped ([#444](https://github.com/JeffMcClintock/TideSynth/pull/444)),
and my duplicate-E34 fix was **closed as redundant** — three boxes fixed it
sixteen seconds apart and [#447](https://github.com/JeffMcClintock/TideSynth/pull/447)
won. A fourth write-up of one incident is noise.

## 2026-08-26 — macos — M8: the one silent way out of `seedPrefabsFromBundle()` now names itself (scheduled run)

**Prompt:** "Work continuously through the TideSynth backlog... Take the topmost
row that is TODO, platform mac or any, and not BLOCKED/NEEDS-JEFF." M8 was that
row once S1b and S8 were skipped as wholly GATED (both live entirely in
`SE16/EditorLib` / `SynthEditLib`, so there is no non-gated half to do).

**What it was.** `seedPrefabsFromBundle()` opened with
`if (resourceFolder.empty()) return;` — no message of any kind. Every OTHER
outcome in that function reports itself, so an unresolvable resource folder was
the single way to get an empty rack module browser in total silence. That is the
M5 shape exactly: M5 was a `BundleInfo` resolution failure that shipped for two
days behind a green `auval`.

**It is not a hypothetical branch, and reading BundleInfo says why.**
`getResource()` and `getResourceFolder()` are different lookups.
`BundleInfo.cpp:560` reaches the embedded Win32 resources FIRST and only falls
back to the folder, and `getResourceFolder()` returns `{}` unconditionally under
`GMPI_IS_PLATFORM_JUCE`. So the four module-description XMLs can enrich
perfectly while this one lookup comes back empty — which is what
`silent-empty-rack.log` has always modelled.

**MEASURED, NOT INFERRED — and the repro is worth keeping.** Copy the
standalone's Mach-O out of `TIDE-Rack.app/Contents/MacOS/` and run the bare
binary: `CreatePluginBundleRef()` fails and `getResourceFolder()` genuinely
returns empty. Controlled A/B, same tree, same command, same isolated `HOME`,
`SynthEditSem/TideApp.cpp` the only variable:

```
origin/main   ControlsXp/MidiPlayer2/Converters/VaFilters "missing from bundle
              resources", "MidiCv.synthedit did not insert a container"
              -- and nothing at all about the resource folder
with M8       the same five lines, plus
              TIDE: bundle resource folder did not resolve - the rack module
              browser will be empty
```

Both halves of the row's Accept are met. `check-rack-populated.py` now reports
it as `FAIL bundle resource folder did not resolve -- BundleInfo found no
Resources folder`, a named failure, not an absence.

**THE JUDGEMENT CALL, and it is the durable part of this entry.** The obvious
move once the branch has a message is to lean on the message. Do not. A needle
in `FATAL_LINES` can only catch a silence someone already thought of; the
positive assertion ("N rack prefab(s) seeded") catches the next one too, and
that asymmetry is the entire argument M6 was built on. So the absence check was
KEPT and sharpened instead of retired: it now splits into *explained* (a known
cause already fired — "Cause already named above: ...") and *unexplained* ("AND
NOTHING SAID WHY ... either a new way out of that function, or diagnostics that
never reached this channel"). Both are still failures. `silent-empty-rack.log`
is deliberately left as a pure absence and must keep failing — it is now the
only fixture that exercises the positive assertion alone, and that is a reason
to keep it, not a reason to update it.

**Verification.** Full Release build green (`-j8`, 100%). Healthy standalone
still passes: 9 rack prefab(s) seeded, `rack is populated`. All three negative
controls exit 1; `m5-empty-rack.log` still reports its documented 12 failures.

**Left for Jeff — a comment, not behaviour.** `.github/workflows/build.yml`
still reads *"seedPrefabsFromBundle() returns silently when the resource folder
is empty (BACKLOG M8)"*. That sentence is now false. The bot token has no
`workflow` scope, so it could not be corrected here.

## 2026-08-26 — windows — the standalone can be driven from a test run, and driving it found a cable-gesture bug (interactive, Jeff directing)

**Prompt:** hey, could you have driven tide via MCP to do that testing? / yes,
wire it up / get cable drags working. Instrument it if needed / file it

**Yes, it could have — and two sessions of hand-testing did not know.** The
app has published a command channel all along (`\.\pipe\gmpi-standalone.<pid>`,
printed at startup), and `GMPI_Wrappers/mcp/` is a BUILT MCP server for it:
`gmpi_drag`, `gmpi_pointer`, `gmpi_set_param`, `gmpi_screenshot`, `gmpi_note`,
`gmpi_render_audio`. Every knob and button test done by hand this week was
scriptable. Registered for this project now — see BACKLOG **E35**.

**Why it was missed, which is the reusable part:** `se_attach` failed, and I
concluded "MCP discovery is broken". It is not — `se_attach` belongs to the
SYNTHEDIT-EDITOR server and searches a different pipe prefix; TIDE's channel
belongs to a SECOND server that was not registered. Two servers, two
prefixes. The evidence was one `grep` apart: the failing tool's own error
names the prefix it searched, and `mcp/src/discover.ts` names the other.

**The coordinate trap, which cost three attempts.** The tool doc said
coordinates are "the same space a screenshot is in at scale 1" — true, and a
trap. This window runs at **scale 1.5**: 1100x626 DIPs, screenshot 1650x939.
Raw PNG coordinates put y=637 below the 626-DIP window, and the app does not
error — it takes the point and lands on whatever is nearest, so a drag aimed
at a jack silently grabbed a MODULE. **Divide screenshot pixels by `scale`
from `gmpi_info`.** The note now says so as an instruction (GMPI_Wrappers
bc3988b).

**Cable drags work, verified end to end.** LFO TRI (DIP 428,425) -> S&H ASR
IN (DIP 664,217): cable drawn, restart parked, `rack requested rebuild`
logged, `SHASR ins=0000101...` -> `1000101...`.

**And driving it immediately found a real bug — E34.** Dragging an EXISTING
cable end and releasing does not end the gesture; the cable clings to the
pointer until a second click, and the release is discarded. Jeff had seen it
by hand and named the two gestures TIDE means to accept: (1) click-release on
a patch point, move, click-release on the destination; (2) press, drag,
release on the destination. Gesture 2 works from a JACK and fails from an
existing cable END.

**Cause, located:** `ConnectorViewBase::onPointerUp`
(`se_sdk3_hosting/ConnectorView.cpp:355`) has two "keep dragging"
early-returns — the 6px `dragThreshold` (gesture 1, correct) and
`if (wasPickedUp) { wasPickedUp = false; return Unhandled; }` (:365), where
`wasPickedUp` is set unconditionally on left-press over a cable (:325). The
first release after picking up an end is therefore ALWAYS swallowed, however
far the pointer moved, so `EndCableDrag` never runs. The two guards overlap:
the threshold already covers the did-not-move case, so past 6px `wasPickedUp`
is not protecting gesture 1, it is breaking gesture 2.

**The consequence is worse than a stuck cable:** while the gesture hangs, the
view and the document disagree. Measured — drag an existing end to empty
space and release: the cable stops being drawn, no rebuild fires, and the
SAVED CHUNK still holds `fm="529566147" tm="1242924866" fp="6" tp="0"`.
Relaunch and it is back, connection bit set. A user reads that as "my edit
was lost".

**Left in place deliberately:** Jeff's live session still contains that
phantom cable, as a standing reproduction.

## 2026-08-26 — windows — E28: a patch cable now rebuilds the rack, from the document the DSP already holds (interactive, Jeff directing)

**Prompt:** yes, lets test the cable (then, on the first false pass: "removing
cable from Scope keeps drawing the waveform (fail)". And the ruling that
shaped the fix, from the day before: "If the user runs a patch-cable ... don't
resend the document, send only the text-parameter ... will trigger a rebuild
of the processor graph, but using the document it already should have. Not
yet another copy.")

**Two layers under the filed diagnosis.** E28 said the TRIGGER was uncalled.
In fact the whole chain above it was severed, and then the call itself was
swallowed:

1. Before #423, `PendingDspClients()` answered null in TIDE, so a cable's
   VALUE never left the editor at all — same null that ate every knob.
2. With values flowing, per-link tracing showed `DoAsyncRestart firing` and
   then silence: the call entered `TriggerRestart`'s fade machinery, whose
   completion path never runs when the HOST drives the blocks. The guard
   could not save itself — `synth_thread_running` is set unconditionally by
   `prepareToPlay`; the name lies, it means "generator open".

**The fix is Jeff's spec verbatim** (SynthEditLib #57): TIDE declares
`setHostDrivenRestart(true)`; DoAsyncRestart in that mode PARKS the request —
it arrives on the audio thread mid-process, and restarting there would tear
the graph out from under the running process() — and TIDE's subProcess
consumes it at the top of the next block, arming `documentPending_` with the
pending string EMPTY, so `prepareToPlay` rebuilds from the cached
`currentDspXml`. The document the DSP already holds; no copy travels.
`persistAcrossResets` carries the new cable list into the rebuilt graph.
Measured both directions: remove → `Scope ins=000`, add → `ins=100`.

**THE FALSE PASS, in detail, because it will fool someone again:** cable-ADD
appeared to work before any of this existed. Its code path happens to trip
`dspDirty` (a `SuspendDSP` RAII guard in the connection code), the next sync
tick's export happened to differ in shape, and the resulting document push
rebuilt the rack — carrying the cable in its patch-list. REMOVE trips no such
guard, which is how Jeff caught it: "removing cable from Scope keeps drawing
the waveform." A mechanism that works for add and not for remove is not a
mechanism; it is a coincidence with good aim.

**Collateral caught on the way:** the TiDE respelling (#434) made the
standalone REFUSE every session it had ever written — the identity check
compared names exactly, "TIDE Rack" != "TiDE Rack", and the user's patch was
quarantined on launch as if written by a foreign plugin. Case-insensitive now
(GMPI_Wrappers af7323f): names differing only by case are the same product.
A rename should never cost anyone their rack.

**Also observed, not yet explained:** the standalone's quiet-timer autosave
did not fire across ~50 minutes of edits in one instance (quit-save and
startup-forced saves work; the FREQ round-trip this morning proves the
syncState path). Not chased — the instance had lived through a
session-refusal and two binary swaps, so the next fresh look should start
clean before believing it.

## 2026-08-26 — windows — E27 resolved, and the attribution reverses: the 14KB "threshold" was the test fixture, not the plugin (interactive, Jeff directing)

**Prompt:** do E26 (whose VST3 half had become E27)

**The lead was tested and it was not the bug.** E27 blamed unchecked
`bytesRead` in the VST3 wrapper's state reads for racks over ~14.1KB restoring
blank in REAPER. Instrumenting `setState` showed something better: on a padded
fixture, **setState was never called at all**. REAPER's .RPP VST header line
embeds the chunk size at byte offset 32; the fixture generator (the scheduled
run's and mine alike) grew the chunk while copying the original header
verbatim, and REAPER refused the inconsistent block before the plugin ever
ran. Every row of the scheduled run's threshold table shares that artifact.

**With the header corrected, everything passes.** The 40,138-byte padded
document restores (`setState got 53605 bytes`), and its re-export is
byte-identical to the unpadded control's — 14,128 bytes, the ignorable
comment dropped. The real acceptance passed end to end: a 33,409-byte VCV
rack (LFO/Scope/LFO2/SHASR/Pulses, the live session document) loads in
REAPER, renders in the FX window with its FREQ light lit, REAPER saves it
itself (44,637-byte chunk under its own header), and the reload builds the
full document.

**The bytesRead fix stands** (GMPI_Wrappers 6174f90 + 3ea69d0): IBStream
genuinely permits short reads, both sites now loop, and a truly short stream
is reported as kResultFalse — "the restore failed" is honest where a silently
blank rack is not. It just was not THIS failure.

**Instrument that outlives the bug:** `TIDE: building rack from N byte
document` (SynthEdit.cpp) — E27's original sin was a blank restore printing
the same nothing as a good one. A real rack is tens of KB; the seeded blank
is ~13KB; one line now tells them apart in any host.

**For the next fixture author:** patching a .RPP's chunk means patching the
header line's size field at offset 32, or REAPER drops the block silently.
And the stale-binary trap claimed another hour here — the VST3 bundle copy
under Common Files did not refresh on an incremental build; the fresh DLL sat
in the build tree while REAPER loaded the old one. Verify the LOADED file's
timestamp before trusting a null result.

## 2026-08-26 — windows — syncState: the save-time refresh the chunk was always meant to have (interactive, Jeff directing)

**Prompt:** how are knobs state saved/restored? / have a look at the GMPI VST3
adaptor and contrast how it handles this: "C:\SE\GMPI_Adaptors" / implement it

**The chunk is stale BY DESIGN, and the design was already finished elsewhere.**
E26 (a knob tweak after the last structural edit is not in the saved file) is
not a hole in yesterday's work but the missing half of a pattern
GMPI_Adaptors' VST3Adaptor completed long ago: a GMPI plugin hosting a complex
inner world (there a third-party VST3, here the rack) keeps its chunk
parameter stale and refreshes it ON DEMAND via `IController::syncState()` —
"sync unsaved state from plugin to host" — invoked by the editor's
`preSaveState()` walk ("warn modules of imminent save"). The interface
declared it, SynthEdit's editor called it for hosted modules
(`CUG2::preSaveState`, UG2.cpp:114), the VST3Adaptor implemented it
(ControllerWrapper.cpp:172, with `inhibitFeedback` around the echo) — and no
wrapper in GMPI_Wrappers ever called it. Grep says zero sites.

**Wired end to end:**

- GMPI_Wrappers: `SessionState::saveNow` calls the plugin controller's
  `syncState()` synchronously before `captureState()`
  (`StandaloneHost::syncPluginState`), with edit-notification suppressed so a
  save cannot mark the session dirty and schedule the next save. VST3's
  controller `getState` makes the same call, with the ordering caveat written
  at the site: VST3 saves read the PROCESSOR's store and the refresh travels
  there asynchronously.
- TIDE: `SynthEditController::syncState()` runs the save-time export —
  including `preSaveState()`, which `exportChunkXml` deliberately skips and
  which is what will hand any future hosted-wrapper module ITS syncState —
  and refreshes parameter 1. `preSaveState` is called through the CDocOb
  base, where it is public; CContainer's override is protected.
- The chunk now carries a 4-byte tag (`SynthEditSem/ChunkPrefix.h`): `TDb1`
  build (structure changed, rebuild), `TDs1` sync (save refresh; a RUNNING
  rack must not rebuild — the standalone autosaves ~2s after every knob
  tweak — so a Sync chunk builds only a rack that does not exist yet, which
  is exactly restore-after-restart, made FRESHER by this since the wrapper's
  re-seed now carries current values). Untagged = legacy = build, so every
  pre-tag session.xml still loads. Classification is a fixed 4-byte compare
  on the audio thread, deliberately not a string comparison.

**Measured:** a live-only knob value (FREQ 3.64, present in no prior save)
survived quit-save and restore; the saved chunk carries the `TDs1` tag; zero
rack rebuilds at save time.

**The false trail, so nobody re-walks it:** the first end-to-end test showed
an UNTAGGED chunk and no rebuild-free save — because the writer was not the
new binary (a stale instance from before the rebuild wrote it), compounded by
the session-quarantine renaming session.xml/previous under the test. Prove
which binary wrote a file before debugging its bytes.

**Two sessions, one repo, live:** origin/main moved twice DURING this work —
Jeff's own #56 landed the identical BundleInfo fix minutes after I committed
mine locally (mine dropped, his stands), and the mac session grew BACKLOG's
E26 row with a further finding (VST3 wrapper state reads never check
`bytesRead` — a real restore-truncation bug my fix does NOT address) and
REAPER accept criteria this work does not yet satisfy. E26 is therefore NOT
marked done here; the standalone half is fixed, the VST3 half (bytesRead
loop + REAPER round-trip) remains open on that row.

**Unresolved observation, recorded honestly:** one Release instance exited
silently mid-`prepareToPlay` (during module construction, bare default
document, no assert, not reproduced since — Debug ran clean immediately
after). If a silent startup exit shows up again, start there.

## 2026-08-26 - macos - M6's CI wiring, with the gate proved both ways first

**Prompt:** interactive

**Did:** Jeff: *"take m6"*. M6's code half had merged
([#424](https://github.com/JeffMcClintock/TideSynth/pull/424)) leaving one thing
open, which the row named as Jeff's because `.github/workflows/**` is what the
bot token deliberately cannot push: **the CI step.** Did that half.

**Proved the gate discriminates BEFORE wiring it, on a real build:**

| subject | result |
|---|---|
| good standalone | **exit 0** - 4 XMLs enriched, **9 prefabs**, root MIDI-CV |
| same binary, `Prefabs/` + `ControlsXp.xml` deleted | **exit 1** - 11 assertions failed |

The negative case includes the one that matters most: *"no 'rack prefab(s)
seeded' line at all. This is the silent case"* - `seedPrefabsFromBundle()`
returning without a message, which is M8. **A grep-for-bad-lines gate would have
passed that build.** Asserting positives is the whole design.

**Extracted the step's bash out of the YAML and ran it verbatim.** A33 once
shipped a jq program that parsed as perfectly valid YAML and was broken shell;
YAML validity is not shell validity. Both branches exercised - populated -> 0,
missing binary -> 1 with an `::error::` annotation.

**macOS only, on purpose.** The script LAUNCHES the standalone and reads its
stderr, and GitHub's linux runner is headless, so a GUI binary there could fail
for a reason that has nothing to do with rack content. **A gate that cries wolf
is worse than no gate.** Widening wants someone to verify headless startup first.

**Not `continue-on-error`** - build.yml already carries that lesson in its own
comments: the `File a platform issue on failure` step was swallowed by exactly
that and *"had never once executed"* while every run reported success.

**Not verified:** that the step passes on the CI runner itself. It is proved on
this box against a real build; the runner is the next thing to watch.

## 2026-08-25 - macos - iOS runs, the bundle rule was wrong in three places, and M9 filed

**Prompt:** interactive

**Did:** Jeff: *"IOS is still wanted"*. Took the iOS AUv3 from never-launched to
installed, launched and registered - and found the fix I shipped this morning
was only a third of the problem.

**THREE functions located the bundle by walking for a `Contents` component**, not
one. [SynthEditLib#52](https://github.com/JeffMcClintock/SynthEditLib/pull/52)
fixed `CreatePluginBundleRef`; `getPluginPath` and `getBundleContentsFolder`
still had the original first-`Contents` bug.

**`Contents` cannot work for two independent reasons.** Nested bundles have TWO
(so the first is the host app - the M5/M7 empty-plugin bug), and **iOS has NONE**
- the layout is flat, `<host>.app/PlugIns/<plugin>.appex/<exe>`, payload in the
bundle root. So iOS resource lookup has NEVER worked; #52 merely changed it from
"wrong bundle" to "no bundle".

`bundleRootOf()` takes the deepest ancestor with a bundle suffix. Checked against
all seven artifacts TIDE builds - **old rule wrong on 3 of 7, new rule correct on
7 of 7**. Standalone, `.vst3` and `.gmpi` have exactly one `Contents`, so they
were never affected either way.
[SynthEditLib#55](https://github.com/JeffMcClintock/SynthEditLib/pull/55), merged.

**Verified:** macOS standalone still enriches 4 XMLs, seeds 9 prefabs and the
root MIDI-CV; macOS AUv3 still passes `auval` with 0 `retrievedValue` warnings;
iOS installs, launches, extension registers. **NOT verified:** that the iOS
extension then loads its resources - that needs the AU instantiated, and **the
simulator has no AUv3 host and no `auval`**.

**A FALSE NEGATIVE, AGAIN, AND THE SAME SHAPE AS THIS MORNING.** Asked whether
the code was still relevant, I grepped the scratchpad worktrees - which had been
REAPED - got nothing back, and briefly read that as "the function is gone". An
empty result from a missing input is not evidence. Caught only because the other
half of the same command also failed. **Check the input exists before believing
an empty result.**

**Filed M9: the iOS container app should host its own AUv3.** (M8 was taken by a scheduled run the same day - the grep-before-filing rule caught it.) An iOS AUv3 cannot
ship as a bare `.appex`, so the app exists regardless; making it host the
extension gives iOS a standalone AND the AU host we otherwise have no way to get.

**Jeff asked whether doing the same on macOS would mean less code overall.
Measured: no, and the intuition inverts.** `wrapper/Standalone` is 17,311 lines,
but 1209 is WASAPI, 990 PipeWire, ~2500 the MCP/IPC headless test surface - none
replaced by AU hosting. Only CoreAudio's 1046 could go. You would end up with two
standalone implementations instead of one. The real upside is coverage: an
AU-hosted standalone would have caught the M5 empty-AU bug immediately - at the
cost of the standalone-vs-AU divergence used as a diagnostic twice today.

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
