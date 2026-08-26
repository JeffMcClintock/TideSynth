# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

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

## 2026-08-26 — windows — E19: the standalone leg passes, and a controlled A/B says the VST3 silently drops any rack over ~14 KB

**Prompt:** b97bc00a5 · claude-opus-5[1m] · app version not readable on this box (no `claude` on PATH, no version file under `%LOCALAPPDATA%`) · as `tide-rack-bot` (both)

**Took E19** — nothing else was eligible. The NEXT block's three cells all still
say "nothing takeable", but they predate the E19–E26 rows that 2026-08-25's
interactive sessions filed, and **E19 is the topmost TODO that is `any`,
unblocked, and states its Accept as a command.** M6 was skipped: open PR
[#424](https://github.com/JeffMcClintock/TideSynth/pull/424) is `tide/mac/**`,
so that item is the mac box's.

**Result in one line: the WINDOWS STANDALONE leg PASSES; the VST3 and GMPI legs
do not, and the reason the VST3 one does not is worth more than the leg was.**

### The instrument this row did not know it had

E19 names `RACK_ADAPTOR_TRACE` as its instrument. The thing that actually made
the run possible is **the standalone's command channel** — a named pipe
`\\.\pipe\gmpi-standalone.<pid>`, printed at startup as `command channel: ...`,
compiled in by default (`GMPI_Wrappers/wrapper/Standalone/mcp/`). It takes
`--screenshot`, `--pointer-down/-up/-move`, `--drag`, `--hover`, `--set-param`,
`--list-params`, `--note-on`, `--render-audio`, one command per line, JSON back.

**This overturns the 2026-08-25 dead end.** That entry recorded that injected
mouse input (`SetCursorPos` + `mouse_event`, `PostMessage WM_LBUTTONDOWN`) never
reaches TIDE's window and concluded "every click measurement here came from Jeff
clicking. Budget for that." **It does not have to be.** The channel does not
inject OS input at all — it calls `onPointerDown/Move/Up` on the editor's own
input client, so it lands where OS injection could not. **I built the entire
LFO→Scope rack this row asks for, by script, unattended.** Coordinates are
logical DIPs from the window's top-left; a screenshot is the same space scaled
by `scale` (1.5 here), so pixel/1.5 = DIP, and `--info` reports the geometry.

Minimal client, since there is no CLI for it (PowerShell, because a named pipe
wants .NET):

```powershell
$c = New-Object System.IO.Pipes.NamedPipeClientStream(".", "gmpi-standalone.<pid>", "InOut")
$c.Connect(5000); $w = New-Object System.IO.StreamWriter($c); $w.AutoFlush = $true
$r = New-Object System.IO.StreamReader($c)
$w.WriteLine("--screenshot C:/tmp/shot.png"); $r.ReadLine()
```

**It is standalone-only**, and that is exactly why the other two legs failed —
see below.

### STANDALONE — PASS

Build: `cmake -B C:/SE/_scratch/e19 -DTIDE_VCV_FUNDAMENTAL=ON -DCMAKE_CXX_FLAGS=-DRACK_ADAPTOR_TRACE=1`
plus the four `*_FOLDER_OVERRIDE`s and `CMAKE_GENERATOR_INSTANCE`;
`TIDE_Rack TIDE_Rack_VST3 TIDE_Rack_STANDALONE` all built, 0 errors. (`-D` not
`/D` in `CMAKE_CXX_FLAGS` — MSYS mangles a leading slash into
`C:/Program Files/Git/D...` and the configure dies in a try-compile.)

Rack: VCV LFO with its FREQ light, VCV Scope cabled LFO **SIN → IN 1**, verified
at the pins rather than by eye — `RackProcessor: 'LFO' connections ins=00000
outs=1000` and `'Scope' connections ins=100 outs=00`, i.e. output 0 and input 0,
which is what SIN and X are given the adaptor's inputs-then-outputs pin order.

Measured over **63 s** (08:57:27 → 08:58:30), past the documented freeze horizon:

| | at T0 | at T+63 s |
|---|---|---|
| display-state applies (`codec=yes`) | #22,100 | #38,160 |
| light updates | #20,300 | #27,800 |
| display-state frame size | 65,548 B | 65,548 B |

Still advancing at the end of the window, 52 distinct checksums across the last
200 applies, and **5,157 pixels differ** between screenshots 63 s apart — the
busiest 100×100 tile is at (2100,1000), which is the Scope's display. The
freeze-after-a-handful signature did not appear. **float and blob both live.**

### VST3 — the plugin runs, and then silently loses your rack

The plugin loads in REAPER 7.78, its editor draws inside the FX window, the
adaptor traces reach a shell-redirected stderr, and `TIDE: rack feedback
reaching the editor - first 37 byte(s)` fires. Then the saved rack does not come
back — `root MIDI-CV seeded` and a ~13.3 KB blank document.

**The A/B that pins it, and it is a clean one.** Take `tests/hosts/v1-rack.rpp`'s
own preset — REAPER's bytes, not mine — and change **nothing but the size** by
splicing an ignorable XML comment into `<Document>`:

| document | base64 `val` | restores? | pushed to DSP |
|---|---|---|---|
| 14,136 B (unpadded) | 18,848 | **yes** | shape 4,787 B |
| 14,647 B (+500) | 19,532 | **no** | 13,371 B = blank |
| 16,147 B (+2,000) | 21,532 | **no** | 13,340 B = blank |
| 24,147 B (+10,000) | 32,196 | **no** | 13,363 B = blank |
| 40,147 B (+26,000) | 53,532 | **no** | 13,338 B = blank |

Same rack, same modules, same everything. **The threshold is between 14,136 and
14,647 document bytes**, and above it the patch is dropped **silently** — no
error, no log line, just a blank rack and a freshly seeded MIDI-CV.

**What this costs in practice:** Jeff's own five-module VCV rack is a 31,648-byte
document and the one I built is 38,658 — both far over. **Both restore correctly
in the standalone from the identical `<Preset>` bytes.** So this is a per-format
defect, which is precisely what E19 exists to find, and it puts PLAN's v0.1
clause *"patch survives save-and-reload of the host project"* in doubt: that
clause was measured on a 14 KB document, which is under the line.

**Ruled out, each by its own control** — do not re-derive these:

- **Not my chunk framing.** I reconstructed REAPER's VST3 chunk wrapper
  (`<u32 len+4><u32 1><u32 len>` + preset + 8 zero bytes, base64 in 128-char
  lines). Re-encoding v1's *own* preset with my encoder restores fine (shape
  4,787 vs the original's 4,820).
- **Not preset size.** v1's preset padded to 51,910 B **outside** the `val`
  attribute restores. Only growth of the base64 blob itself breaks it.
- **Not the preset's shape.** The standalone's `session.xml` carries an XML
  prolog and `standalonePlugin=` / `standalonePluginVersion=` attributes that
  REAPER's does not; stripping both changes nothing.
- **Not module registration or ordering.** `TIDE: VCV Fundamental — 39 module(s)
  registered` is line 1 of the VST3 log, before any restore, exactly as in the
  standalone.
- **Not content.** The failing and passing documents differ by an XML comment.

**Prime suspect, named but NOT instrumented, so treat it as a lead:**
`Controller_VST3::setComponentState` (`GMPI_Wrappers/wrapper/VST3/Controller_VST3.cpp:456`)
and `Processor_VST3::setState` (`Processor_VST3.cpp:1045`) both do
`state->read(chunk.data(), chunkSize, &bytesRead)` and **never look at
`bytesRead`**. A short read on a larger chunk would leave the tail unwritten, the
XML unparseable, and the failure exactly this silent. It fits the size behaviour,
and it fits the standalone working, since the standalone never goes through an
`IBStream`. Filed as **E27**; reading `bytesRead` in a loop is a small,
well-scoped change and this run deliberately did not make it (STEP 3: one item).

### The other VST3 finding: the committed host fixtures no longer load at all

Every `tests/hosts/*.rpp` in the repo names the plugin as
`1386065673{506C7567696E474D50492050A2A07287}`. REAPER 7.78 on this box writes
and expects `1558955188{67756C506E694D4750492050A2A07287}`. **Same UID, opposite
byte order** — the TUID is literally `"PluginGMPI     "` with byte 11 = `'P'` and
a 4-byte id hash (`GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:200`), so
the fixtures hold the raw TUID and REAPER now writes the COM little-endian
rendering of it. The plugin's identity has **not** changed.

The symptom is *"Project Load Warning — the following effects were in the project
file and are not available"* and a track with no plugin. It reproduces with the
untouched `v1-rack.rpp` against Jeff's own installed build, so it is nothing to
do with anything I built. Substituting the token makes every fixture load. Filed
as **E29**. This does not touch the mac box's `render-and-measure.py` results,
because that script hardcodes `/Applications/REAPER.app/...` and has never run on
Windows — but the fixtures are shared, so **the mac box should check whether its
REAPER agrees with the committed token.**

### A patch cable does not rebuild the DSP graph

Found on the way, reproduced on two independent racks. Draw a cable in the
editor: it renders, it persists, it is written into `HC_PATCH_CABLES` — and **no
rebuild happens.** No `RackProcessor: ... constructed`, no new `connections`
line, the Scope's input stays `ins=000`, its display-state checksum stays frozen
at one value.

Add any unrelated module and the rebuild that follows picks the cable up
correctly — `LFO ... outs=1000`, `Scope ... ins=100`. So the cable is recorded
and the *trigger* is missing, which is the useful half of the diagnosis.

**E9 already said this by inspection and this is its first measurement:**
*"`DoAsyncRestart` is reached from `dsp_patch_parameter.cpp:773` for any host
control with `requiresAsyncRestart()`, a set that includes `HC_PATCH_CABLES` —
nothing in TIDE calls it yet."* Filed as **E28**.

### GMPI — not attempted, and why

Hosting `TIDE-Rack.gmpi` needs SynthEdit, and the rack this row wants would have
to be built inside that host. **There is no way to do that unattended**: the
command channel is standalone-only and OS-level input injection does not reach
TIDE's window. The same wall stopped the VST3 leg from being measured with a
*real* rack. Stated plainly so the next run does not spend a session finding it:
**E19's Accept is only automatable in the standalone today**, and the cheapest
way to change that is either a command channel in the plugin editor or one
human-built fixture per format, saved once and committed.

### Three times I believed an empty picture, and was wrong twice

Worth more than the findings, because it cost most of the session:

- **The rack view scrolls, and an empty-looking rack is usually a scrolled one.**
  The standalone opened on a region with nothing in it while four modules sat at
  `panelRect` x 597–1113. I concluded "the session did not restore" and started
  hand-authoring a replacement.
- **I read a session file at the wrong moment and concluded nothing persisted.**
  It was a stale read; state persists fine. I nearly wrote that up as a defect.
- **The third time it was real** — but only because a trace, not a picture, said
  so.

The rule this yields: **on this UI, a claim of absence needs a trace or a
document dump, never a screenshot.** Every finding above rests on one.

### Verification artifacts

`RackProcessor: connections ins=/outs=` bitmaps for pin-level cable proof;
`display-state update #N` / `apply #N ... sum=` counters and `light N update #M
value V` across a 63 s window; a 5,157-pixel screenshot diff with the busiest
tile on the Scope; the five-row padding table above, each row a separate REAPER
launch reading `TIDE: DSP structure changed, pushing N byte document`; a
`LoadLibraryW` + `GetProcAddress` check proving the VST3 exports
`GetPluginFactory` (it was never a broken binary).

### Machine left clean

Everything I touched is back: the installed `Common Files\VST3\TIDE-Rack.vst3`
binary (I swapped my trace build in and put Jeff's 16,401,408-byte original
back), `reaper-vstplugins64.ini` (restored from backup after a forced rescan),
and `%APPDATA%\TIDE Rack\session.xml` + `session.previous.xml` (restored
byte-for-byte from copies taken before the first launch). No TIDE or REAPER
process left running. Build tree is `C:\SE\_scratch\e19`, outside every repo. All
seven working copies are on their default branches and clean.

**Note for whoever wonders:** `C:\SE\VCV_Fundamental_gmpi` carries one unpushed
commit of Jeff's, `93a27f9`, docs only (`DEVNOTES.md`, `README.md`). Left alone —
STEP 5's third kind of dirt.

**Branch/PR:** `tide/win/E19-feedback-format-matrix` — this entry, E19's status,
and rows E27/E28/E29.

## 2026-08-26 — macos — M6: auval passes an empty rack and this does not; the appex finally has a voice (scheduled run)

**Prompt:** b97bc00 · claude-opus-5[1m] · app 1.34493.1 · as tide-rack-bot (both)

**Did:** took **M6**, the only eligible ungated TODO. Built the content gate,
and discharged its Accept **on the real artifact rather than a fixture**.

### The headline, measured twice on this box today

| subject | `auval` | `check-rack-populated.py` |
|---|---|---|
| healthy AUv3 (my build) | exit 0, SUCCEEDED | **exit 0**, six assertions ok |
| AUv3 with `Prefabs/` + `ControlsXp.xml` removed | **exit 0, `AU VALIDATION SUCCEEDED`** | **exit 1, 8 failures named** |

The second row IS M6. A rack with no prefabs, no MIDI jacks and a missing pin
XML validates clean. I removed the two resources from the installed appex,
re-signed ad-hoc, re-registered with `pluginkit -a`, and `auval` never blinked.

### The work was the CAPTURE, not the assertions

M6's row said "a gate can be as small as: fail if any negative line appears".
The assertions are indeed small. **They were also unreachable**: an appex's
stderr goes nowhere an outside process can read, which is why M5 resorted to
`freopen`-ing it into the sandbox container and deleted the hack before
redeploying. A gate cannot be a hack you remove.

M5's own note contains the answer without drawing the conclusion — *"`log
stream` shows os_log only"*. **os_log is the channel that crosses the
boundary.** `TideApp.cpp` now mirrors its ten startup diagnostics through a
`tideDiag()` shim. Chosen over the log file deliberately: TideApp.cpp's
existing comment block rules a log file out as *"a write outside the plugin
bundle (constraints 3 and 4)"*, and os_log is the platform's own facility
rather than a file the plugin creates — nothing on the user's disk, no sandbox
exception. stderr stays primary, so the standalone and CI logs are unchanged.

**The A/B that proves the mirror is load-bearing**, same command both times:

- against the build already registered on this box (`main`, no mirror):
  **captured nothing**, 6 assertions failed.
- against the os_log build: `2/18, 2/7, 26/70, 2/7`, **9 prefabs**, root
  MIDI-CV — identical to the standalone.

That first result is worth reading carefully: the gate reported failure on a
plugin that is probably *fine*. **That is correct behaviour, not a false
positive** — an unobservable plugin must not be called healthy — but it means
this gate only means something on a build carrying the mirror.

### The design decision I would defend hardest

**Assert the POSITIVE lines, do not grep for the bad ones.**
`seedPrefabsFromBundle()` opens with `if (resourceFolder.empty()) return;` —
**no message at all.** So an unresolved resource folder gives an empty module
browser in total silence, and the cheap grep-for-bad-lines gate passes it. That
is the same shape as M5's `BundleInfo` defect. Requiring
`N rack prefab(s) seeded` fails it, because the line is simply absent.
`tests/rack-content/silent-empty-rack.log` is the standing control for exactly
this, and the silent `return` itself is filed as **M8**.

### What I did NOT do

- **No CI wiring.** `.github/workflows/**` is what the bot token deliberately
  cannot push. The script runs by hand today; the job is one step and is
  Jeff's. **So M6 is not fully closed by this PR** and the row says so.
- **No audio.** Same standing gap as every entry this week — the gate proves
  the rack is POPULATED, not that it sounds.
- **Windows and Linux.** The `--standalone` arm is portable and unexercised
  there; the `--au3` arm is macOS-only by definition.
- **`tideRemovedDialog` is not mirrored.** Equally invisible under AUv3 and
  arguably deserves the same treatment — left alone as out of scope.

**Verified:** cold configure + Release build rc=0, **0 error lines**, all
siblings `[fetched]` from their own `main` (not the local overrides — see the
dirt note below); the four-way A/B above; both fixture negative controls; the
live emptied-AUv3 negative control; the standalone arm re-run after an include
move.

**Learned:**

- **`auval` passing is compatible with the plugin containing nothing**, and now
  there is a command that says so out loud rather than a paragraph in a row.
- **An absent line is a failure mode a grep cannot see.** The cheap version of
  this gate would have shipped and felt like coverage.
- **The AU3 appex is assembled by its own always-run target**, `TIDE_Rack_AU3_assemble`,
  not by building `TIDE_Rack_AU3` and `TIDE_Rack_AU3App`. I built both of those
  and got an app with an empty `Contents/PlugIns` — which installs fine and
  provides nothing, the exact failure `package-macos.sh` has a guard for.
- **`pluginkit -a` registers the extension; a GUI launch is not required.**
  building.md says registration needs an `open`. It does not — and `open` from a
  `/tmp` path does not register at all, which cost me a cycle.
- **The appex is reused across instantiations**: two `9 rack prefab(s) seeded`
  lines per `auval` run, one XML merge. `s_xmlMerged` working, visible for free.

**Machine left exactly as found, and checked rather than assumed.** Testing the
AU3 meant displacing the registered build in `~/Applications`. Jeff's original
was `ditto`'d aside first and restored after: **same path, 19/19 files,
`sha256 14f7a89f…` identical to the backup**, re-registered, `auval` SUCCEEDED
on it. The `pluginkit` UUID and timestamp differ because re-registering mints a
fresh record — that is registration identity, not content. No TIDE process left
running. All build output in scratch; nothing installed from my build.

**Jeff's working tree was dirty when I arrived and I did not touch it** (STEP
5's third kind). `TideSynth`: `SynthEditSem/TideApp.cpp` and
`SynthEditController.cpp`, both carrying `// TEMP-DIAG ... Revert before
landing` scaffolding from 2026-08-25 17:47. `SynthEditLib`: parked on branch
`fix-patchmanager-dangling-properties-observer` with `EditorLib/PatchManager.cpp`
modified. **This is why I worked in a scratch worktree and built with fetched
siblings rather than the local overrides** — his in-progress SynthEditLib is not
in my build, and my TideApp.cpp change is not in his tree.

**Also:** flipped **M5** IN-REVIEW → DONE ([#405](https://github.com/JeffMcClintock/TideSynth/pull/405)
merged, state queried not remembered). `main` is green on all three platforms;
zero open `platform:*` issues; no open PRs before mine.

**Branch/PR:** `tide/mac/M6-rack-content-gate`.

## 2026-08-25 — windows — a button press sends a VALUE: the missing half of S12, and the document export that cost 2ms twice a second (interactive, Jeff directing)

**Prompt:** i'm interested if the data transfer from the scope to it's gui is
optimal... but is the wrapper itself efficient? (then, on the buttons: "are we
clear that pushing a button should not rebuild the DSP (an expensive
operation) merely send the value?", "we have teh queue mechanism for sending
values", "9 rebuilds!!!!! try for one", and on the export cost: "holy fuck
that was expensive... this is a *tiny* rack")

**THE HEADLINE: nothing the editor did to a parameter ever reached the DSP.**
`PatchParameter_base::UpdateDspValue` asks the application for the queue to
post an edit to, and `CSynthEditAppBase::PendingDspClients()` answers null
unless the EDITOR'S OWN runtime is running a processor. TIDE's never is - its
processor is a separate object, and under AUv3 a separate process - so every
knob turn and every button click was discarded at that null check as
"processor not running". One null, and it is why the VCV buttons appeared
dead all day. `TideApp::PendingDspClients()` now answers with its real queue.

**The missing half of S12.** The bytes then need CARRYING, which nothing did.
The editor was already serialising edits into `m_message_que_ui_to_dsp` - in
SynthEdit proper the DSP shares the process and polls it directly. Now
`SynthRuntime_editor::takeUiToDspMessages` hands whole framed messages to the
GUI heartbeat, the controller ships them on blob parameter 3, and the
processor pushes them into the rack's own ui->dsp queue, which
`SynthRuntime::ServiceDspRingBuffers` already polls. Verified on the wire:
handle 994049736, id `ppc`, payload `00 00 80 3f` = 1.0, then `00 00 00 00`.

**A REBUILT ENGINE KEEPS ITS LIVE PARAMETER VALUES, AND THAT IS A FEATURE.**
Recorded because I called it a bug and Jeff corrected me: "You add a module
(resets processor), you tweak its settings, you add a second module (resets
processor). There is no way this should wipe the first module's settings."
The DSP owns live state; the document is a snapshot that may be stale. Which
is exactly why a value could never arrive by document push - the rebuild
rightly refuses to clobber - and why the value path had to exist.

**So the document goes to the processor only when its SHAPE changes.** A
module added or deleted: send it, the rack restarts, unavoidable. A patch
cable moved: send NOTHING - the cable list is the HC_PATCH_CABLES host
control, its value rides the same message queue, and the DSP turns it into a
graph rebuild from the document it already has (`requiresAsyncRestart` ->
`DoAsyncRestart`, with `persistAcrossResets` keeping the list). A knob or
button: nothing either. Startup went from 18 rack builds to 1.

**THE DECISION BELONGS TO THE EDITOR, NOT THE PROCESSOR.** It was briefly the
processor's - comparing the arriving chunk's structure in `onSetPins` - and
that is a mistake worth remembering: `onSetPins` runs on the AUDIO THREAD.
Jeff: "the Processor has important real-time stuff to do, not comparing huge
strings."

**The export was costing 2ms twice a second, forever.** `serviceDocumentSync`
serialised the whole document on every tick to ask whether anything had
changed. Measured on a three-module rack: 32.5KB, 1.87ms, 40 exports in 20
idle seconds, 39 discarded; one more module took it to 38.2KB and 2.4ms, so
it grows with the rack. Fixed by debouncing on `dspDirty` - the flag an RAII
`SuspendDSP` guard already sets at 23 sites, which `CSynthEditAppBase::OnTimer`
already consumes before its own soft restart. Jeff's steer was to look at what
the app already does, and it was right there. Idle exports: 60 per 30s -> 2,
both at startup, steady state zero. The LFO's LED visibly doubled its frame
rate.

**Two transport defects fixed on the way, both of which killed DSP->GUI
feedback outright:**

- **I broke it myself** and it is the sharpest lesson here. Keeping churning
  values out of the exported document by DROPPING the `<patch-list>` element
  silently killed feedback for every volatile parameter - the DSP builds its
  patch memory from those entries, and a parameter without one never
  transmits to the GUI at all. A/B: element absent, the editor got 26 light
  updates then nothing ever again; element present, 123 and counting. The
  element must stay; only its TEXT is blanked.
- **The controller's blob arm deduped a STREAM.** `setParameterBlob` skipped
  notifying editors when bytes matched the previous message - right for a
  value, wrong for TIDE's feedback channel. The rack settled into a repeating
  12-byte watchdog, every batch compared equal, every batch was dropped, and
  the editor received nothing at all. Same rule the processor side learned
  first: a blob parameter is a stream, not a value.

**A latched light is sent once and then never again**, so if the editor is not
listening at that instant it is wrong forever - OFFSET's lamp was lit or dark
purely on startup timing. The adaptor now re-asserts every light once a
second, which makes the channel self-healing instead of exactly-once.

**Dead end, so nobody repeats it:** injected mouse input (SetCursorPos +
mouse_event, and PostMessage WM_LBUTTONDOWN) never reaches TIDE's window, even
with the foreground check passing and WindowFromPoint naming the right child.
Every click measurement here came from Jeff clicking. Budget for that.

**Still open:** parameter 1 is the persistent chunk, and now that neither
value edits nor cable moves push it, it refreshes only on a structural change
- so a knob tweak before a save can be lost. `syncState()` is declared on
`IEditor` but no wrapper calls it. See BACKLOG E26.

## 2026-08-25 — macos — The port is pushed and fetches; panels were 100 wide; and a new crash signature (interactive, Jeff directing)

**Prompt:** merge PRs / then lets run the standalone with the new modules / oh. and do the two packs get their own category? / generally good. PhasorToLFO looks too narrow? / did that / (was a crash recently)

**Did:** finished E20 end to end — the port is on GitHub, the fetch path works,
and the modules render correctly after a real bug was found by **looking at
them**. Also diagnosed a crash Jeff mentioned in passing, filed as **E25**.

### The port is live and the fetch path is proven

`HetrickCV_gmpi` exists, is public, and holds the two commits. Getting there
took three attempts and the failures are worth recording: `gh repo create
--source --push` reported *"Unable to add remote 'origin'"* because the clone I
built already had one, so the repo was created **empty** and stayed that way;
the bot then could not push because **public is not writable**; a collaborator
invite fixed it.

**The `FetchContent` path had never run** — everything until now used
`HETRICKCV_FOLDER_OVERRIDE`. It works: `Fetching HetrickCV_gmpi from github`,
`HetrickCV_static: ... 66 module object libraries`, `[fetched]`.

### Two packs, two categories — confirmed by looking

Jeff asked whether each pack gets its own category. **Yes**, and the browser
shows it: HetrickCV's modules, then a `Rack-VCV Fundamental` group header, then
Fundamental's. `Rack/HetrickCV` and `Rack/VCV Fundamental`, both starting
`Rack` as `ModuleScope::RackOnly` requires.

### "PhasorToLFO looks too narrow?" — it was, and so was every other one

**Every HetrickCV panel was rendering 100 DIP wide regardless of its real
width.** An SVG may declare `width`/`height` as PERCENTAGES and carry its real
size only in `viewBox`; every HetrickCV panel is authored
`width="100%" height="100%" viewBox="0 0 180 380"`. `panelMetrics` read only
the attributes, and **`"100%"` parses as `100`**.

**Why it presented as a width bug rather than a parse bug**, which is the part
worth keeping: the caller floors a sub-rack-size panel to rack height, so the
bogus `100` HEIGHT was silently corrected to 380 and only the width survived to
be seen. The `panel measured 100x100 - flooring to rack size` line was firing
**once per module** and reading as routine.

| | before | after |
|---|---|---|
| `PhasorToLFO` (12 HP) | `100x100` -> `100x384` | **`180x380` -> `180x384`** |
| `ASR` (6 HP) | `100x100` -> `100x384` | **`90x380` -> `90x384`** |
| flooring warnings | 1 per module | **0** |

Fixed in [Adaptor#5](https://github.com/JeffMcClintock/SynthEdit_Rack_Adaptor/pull/5).
**The control matters because this is shared code:** Fundamental authors real
numbers, so its panels take the unchanged path — exercised across both panel
styles plus both degenerate cases (no attributes; relative with NO viewBox,
which must NOT fall through to 0x0). Fundamental still registers 39, zero
flooring warnings.

**This is the bug that only looking finds.** Three entries ago I rendered panel
SVGs and reasoned about them at length; none of that could have shown it,
because the SVG is correct and the READER was wrong.

### The crash: diagnosed, not reproduced

`TIDE-Rack-2026-08-25-162615.ips`, `EXC_BAD_ACCESS`/`SIGSEGV`:

```
CContainer::getIgnoreProgramChange()   <- faults
PatchParameter_base::ExportXml
CPatchManager::ExportXml
CContainer::ExportXml
TideApp::exportChunkXml
TideApp::serviceDocumentSync
SynthEditGui::onTimer
```

**It is a new signature, measured against the other ten reports on this box
today:** every one of those is `EXC_CRASH` in `LoadPrefab`,
`RegisterParameters` or `ImportChildren` — different exception class, different
site. This is the only `EXC_BAD_ACCESS` and the only `getIgnoreProgramChange`.

**It did NOT reproduce**: relaunching the same build and letting the sync timer
tick for 100 s was clean. `exportChunkXml` runs on every tick, so the trigger is
document state or interaction, not elapsed time. Filed as **E25** with the lead:
the properties pane was showing `Ignore Program Change` for a ported module's
parameters when it happened, and that is the accessor that faults.

**Verified:** the fetch path from a clean build dir; the before/after panel
metrics via `RACK_ADAPTOR_TRACE`; the parser control across four panel shapes;
39 Fundamental modules still registering; the crash-signature comparison across
all 11 reports.

**Not verified:**

- **Still no audio.** Registration, rendering and layout are all proven now;
  nothing has been heard.
- **The crash is not reproducible on demand**, so nothing is fixed.
- **The 13 excluded modules**, Windows and Linux.

**Learned:**

- **`"100%"` parses as `100`**, and a percentage in an SVG's width is a real
  authoring convention, not a malformed file.
- **A floor on one axis hides a parse failure on both.** The height was wrong
  too and got silently corrected, which is exactly why this looked like a width
  problem for as long as nobody read the trace.
- **Rendering an asset and reading it are different tests.** I rendered these
  panels three entries ago and learned nothing about this, because the asset was
  never the problem.
- **`gh repo create --source` will not add a remote that already exists**, and
  it reports that as a warning while still creating an empty repo.
- **Public does not mean writable.** Two different access failures in one
  session with the same 403 shape.

**Next:**

1. **[Adaptor#5](https://github.com/JeffMcClintock/SynthEdit_Rack_Adaptor/pull/5)**
   wants merging — without it every ported pack with percentage-sized panels
   renders wrong.
2. **E25** needs a reproduction before anything else.
3. **Make one make a sound.** It is the only thing in E20-E25 still untouched.

**Machine left clean.** Scratch build trees and worktrees; no TIDE process left
running (checked); nothing installed. `HetrickCV_gmpi` and the adaptor clone are
deliberate additions.

**Branch/PR:** `tide/mac/E25-sync-export-crash` — the E25 row and this entry.

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
