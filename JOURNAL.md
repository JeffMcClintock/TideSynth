# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## 2026-08-24 — macos — One token, two vocabularies: the crash, and what S45 really found (interactive)

**Prompt:** so the one remaining issue is Linux getting the datatype wrong? Shall we give it a task?

Chasing why S35 segfaulted SynthEditCL ended in a root cause that explains three
separate symptoms across two machines. Writing it down properly because the next
instance will look like a fourth unrelated bug.

**`"string"` means opposite things in the two SDKs.** SDK3 xml (`conversion.cpp`)
maps `"string"` to `DT_TEXT` — the WIDE type — and `"string_utf8"` to
`DT_STRING_UTF8`. GMPI xml (`GMPI/Hosting/xml_spec_reader.cpp`) maps `"string"`
to `PinDatatype::String`, which is utf-8, and has no entry for `"string_utf8"`
at all. The numeric enums agree (1 and 12 in both); only the words disagree.
Nothing arbitrates, so whichever table happens to read a file decides.

**Symptom 1 — the crash.** `moduleXmlIn()` returned whichever `.xml` the
directory iterator yielded first from a bundle's Resources. TIDE keeps its
child modules' xml there (`Converters.xml`, `VaFilters.xml`, ...), which are
SDK3 files sitting inside a `.gmpi`, so they were read with the GMPI table and
every `std::wstring` pin was recorded as utf-8. `SE TextToText8` arrived as
Text8->Text8, converted nothing, and `ug_base::connect` inserted converter after
converter until the stack died. Fixed in SynthEditLib#40: a `.gmpi` descriptor
must be `plugin.gmpi.xml` or `<bundle-name>.xml`; `.sem` untouched. Of the 64
bundles installed here, 62 already obey that and the only 2 that don't are the
two at fault.

**Symptom 2 — silent misses.** An unrecognised datatype token doesn't error:
`lookup()` returns an optional and the guarded assignment is skipped, leaving
`pinInfo::datatype` — which has no default initialiser — indeterminate. GMPI#15
accepts `"string_utf8"` as a synonym at Jeff's direction, which covers the
common port; the silent-miss path itself is still there.

**Symptom 3 — S45's captions, and where that fix pointed.** The linux box traced
tofu captions to raw `wchar_t` on the wire and fixed it by moving `LabelGui` to
`Pin<std::wstring>`. The investigation was genuinely good — it measured the
codepoints (28 for 7 characters, 3 NULs each) and eliminated fonts with a
control. But `SE Label` is a GMPI module (`graphicsApi="GmpiUi"`, GMPI
`Register<>::withXml`), so its `datatype="string"` means utf-8 and
`Pin<std::string>` was already right. Jeff checked the module independently and
agrees it is self-consistent. On macOS the cache records that pin as
`string_utf8` and captions render fine, so the change would have corrupted macOS
the mirror way. SynthEditLib#39 is closed; `main` still has `Pin<std::string>`.

**What that leaves is the real defect, now S46:** the same GMPI module is
registered as DT_STRING_UTF8 on macOS and DT_TEXT on linux. Both platforms take
the same path — `ViewBase.cpp:792` uses `pinInfo.GetDatatype()`, the registered
type, not a re-parse — so the divergence is in registration, not in reading a
default. I checked that specifically rather than assume it, because S45's stack
made a re-parse look plausible.

The first measurement is one line: linux's cache record for `id="SE Label"`, its
`datatype` and the `file=` it came from. If it says `string` where macOS says
`string_utf8`, that is the bug located.

**A correction I had to make mid-investigation.** I claimed the duplicates were
irrelevant after an isolated-`HOME` run still failed. That control was worthless:
a fake `HOME` independently breaks SynthEditCL (exit 134 on pre-S35 too). The
comparison that held up was real-`HOME`, where pre-S35 exits 0 and S35 segfaults.

**Not verified:** everything here is macOS. The linux half is S45's measurement,
which I have not reproduced, and Windows is untouched by either.

## 2026-08-24 — linux — S45: the captions were UTF-32 in a UTF-8 string, and S23 closed (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** diagnosed and fixed **S45** — the rack-caption tofu split out of S25
this morning. Product change is
[SynthEditLib#39](https://github.com/JeffMcClintock/SynthEditLib/pull/39).
Also closed **S23** at Jeff's instruction (*"close S23"*).

**It was never a font problem.** It is a datatype mismatch, and the fix is two
pin declarations.

### The chain, from a backtrace rather than from reading

Four eliminations were already on the row and all of them held — which is what
made the remaining space small enough to instrument:

```
MfcDocPresenter::RefreshView   MfcDocPresenter.cpp:483
ViewBase::Refresh              ViewBase.cpp:2103
ViewBase::ConnectModules       ViewBase.cpp:679     <- setting GUI pin defaults
PluginEditorBase::setPin       GmpiPluginEditor.h:106
Pin<std::string>::setFromHost  GmpiPluginEditor.h:49
LabelGui                       LabelGui.cpp:136
```

`LabelGui` declares `<Pin name="Text" datatype="string"/>`. `conversion.cpp:996`
maps **`"string"` → `DT_TEXT`**, and `RawConversions.cpp:238` encodes `DT_TEXT`
as **raw `wchar_t` bytes**:

```cpp
result.resize( sizeof(wchar_t) * s.size() );
memcpy( &result[0], s.data(), result.size() );
```

The C++ pin was `Pin<std::string>`, and `valueFromData<std::string>` is a
**verbatim byte copy**. So `"MIDI-CV"` arrived as `'M',0,0,0,'I',0,0,0,…` and
every NUL drew `.notdef`.

### The measurement that made it undeniable

Logging the shaped codepoints inside the text engine:

| caption | codepoints | real chars | NULs |
|---|---:|---:|---:|
| `MIDI-CV` | 28 | 7 | 21 |
| `PITCH` | 20 | 5 | 15 |
| `GATE` | 16 | 4 | 12 |
| `TRIG` | 16 | 4 | 12 |
| `VEL` | 12 | 3 | 9 |

**Exactly three NULs after every character** — the signature of a 4-byte
`wchar_t` string read as bytes.

### Why the notdef probe said zero while boxes were on screen

My first probe logged only the `else if (fallbackFor)` branch, which fires when
the primary face does not cover a codepoint *and* a fallback is available. Here
`fallbackFor` was null, so control took the final `else` and the primary drew
`.notdef` silently. **A probe on one branch of a four-branch decision reported
"no misses" while the screen was full of them** — and I nearly read that as
evidence the text engine was innocent, when it only meant my instrument was in
the wrong arm.

### The fix, and the decision I deliberately did not take

Declare the pins `Pin<std::wstring>` — matching what the host actually sends,
since `valueFromData<std::wstring>` already divides by `sizeof(wchar_t)` — and
convert to UTF-8 once, at the three points that hand text to the drawing API.

**The wire format is unchanged on purpose.** `datatype="string_utf8"` exists in
the same table and would also work, and is arguably tidier. It also changes the
datatype of a pin existing patches may connect to, and this codebase
auto-inserts converters on mixed-datatype connections — v0.1's own findings
record a silent-drop bug of exactly that shape. That is Jeff's call, stated on
the PR rather than made by me.

**Scope checked, not assumed:** `LabelGui` is the **only** `graphicsApi="GmpiUi"`
module in the tree and holds the only two pins with this mismatch.

### This is probably not Linux-only

`wchar_t` is 4 bytes on Linux and macOS, 2 on Windows — so Windows gets **one**
NUL per character from the same code path, not none. Nobody has looked. The mac
S25 measurement rendered an *Oscillator* prefab correctly, and that prefab has no
Label, so it never exercised this path. **Do not read "seen on linux" as
"Linux-only".**

**Verified:** captions render **MIDI-CV / PITCH / GATE / VEL / TRIG** in real
glyphs; module browser unchanged in the same window; zero plug-in diagnostics;
build 312/312 rc=0 with the temporary text-engine probes reverted first
(`grep -c TIDEDIAG` = 0 in `gmpi_ui`).

**Not verified:**

- **`SynthEditCL` was not built.** The change is confined to one module's pin
  declarations and its three uses, and it compiles as part of TIDE, but the
  commercial consumer was not built here.
- **Windows and macOS** were not built.
- **Whether any shipped patch connects a `DT_TEXT` source to a Label's Text
  pin** — the reason the wire format was left alone, and unmeasured.

**Learned:**

- **A probe on one branch of a multi-branch decision can report a clean bill of
  health for a failure happening in a sibling branch.** Zero notdefs with boxes
  on screen was the single most misleading measurement of the session, and it
  was my instrument, not the system.
- **Log the actual string, early.** Two builds were spent on font resolution;
  one `fprintf` of the shaped codepoints answered it outright, and the hex made
  the diagnosis unmistakable.
- **`backtrace()` + `addr2line -f -C` turns "who called this" into one build.**
  RelWithDebInfo carries enough symbols; the raw `+0x…` offsets resolve fine.
- **"Seen on platform X" and "X-only" are different claims**, and a `sizeof`
  difference is exactly the kind of thing that makes the second one false while
  looking true.
- **Eliminations are worth their cost when they shrink the search space enough
  to instrument.** Four negatives from the morning meant the afternoon had one
  place left to look.

**Machine left clean.** All temporary diagnostics reverted before the
verification build — `grep -c TIDEDIAG` is 0 across `gmpi_ui`, and that worktree
is back to `origin/main`. Headless weston stopped, standalone stopped, scratch
`HOME`s only; **Jeff's `~/.config/TIDE Rack` was not written to** (timestamps
still 2026-08-20). All six repos on their default branches and clean.

**Branch/PR:** `tide/linux/S45-caption-tofu` — TideSynth, the row plus this entry
and one screenshot. Product change is
[SynthEditLib#39](https://github.com/JeffMcClintock/SynthEditLib/pull/39);
**merging TideSynth's side alone changes no behaviour**, it only records.

## 2026-08-24 — linux — S25's Accept passes; the tofu that survives is a different bug (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** ran S25's own Accept on current `main` instead of building toward its
diagnosis. **It passes.** The row is closed by measurement, and what is left of
the symptom is split out as **S45**.

No product code changed.

### The measurement

`main` `44be26e` — which already carries Jeff's *"new tide-specific patch-points
(don't draw anything)"* — `TIDE_Rack_STANDALONE`, RelWithDebInfo, headless
weston, driven through the `gmpi-standalone` command channel. **Isolated `HOME`,
with `~/.config/TIDE Rack/session.xml` confirmed absent at launch** — that is the
trap the 2026-08-20 entry says contaminated three earlier observations, and it is
still the first thing to check.

Insertion is **arm-then-click**, not a drag (2026-08-20). Click `Prefabs`, click
`Oscillator`, click the rack.

| | result |
|---|---|
| freshly inserted `Oscillator` | `TIDE Oscillator`, X 4264 Y 3832 **W 20 H 66** — two bare jacks |
| after kill + relaunch (restored) | **identical** — same jacks, same pixels, same position |
| only difference in the crop | the **blue selection border**, which a fresh insert has and a restore does not |

**That is the mac result.** So insert ≡ restore now holds on both platforms, and
the cross-platform divergence this row existed for is gone.

**The stronger control:** the entire 1100×626 window, before and after a restart,
is **byte-identical** — SHA-256 `97b33b66…` both times, `ImageChops.difference`
bbox `None`. Not "looks the same".

### What did not go away, and why it cannot be this row

The seeded root MIDI-CV facade's pin captions still draw as missing-glyph boxes.
**They do it in both paths** — which is exactly why it is not S25, whose entire
subject is a *difference* between the two paths. Filed as **S45**.

Four things are eliminated on the way, and they are the value of the session:

1. **Not a missing font stack.** The module browser, in the **same window, same
   process**, renders `Fixed Values (Volts)`, `Volts to Float2` and the rest
   perfectly. Whatever is broken is one text path, not the text system.
2. **Not fontconfig and not a missing font file.** `strace` shows four real fonts
   opened — `DejaVuSans.ttf`, `LiberationSans-Regular.ttf`, `NotoSans-Regular.ttf`,
   `UbuntuSans[wdth,wght].ttf` — **zero `ENOENT` on any `.ttf`/`.otf`**, and
   `/etc/fonts/conf.avail/*` read. **This refutes the row's own leading suspect**,
   *"the linux font/resource-binding suspect"*, in the sense the row meant it.
3. **Not the source strings.** `Resources/Prefabs/MidiCv.synthedit` has **zero
   non-ASCII bytes**.
4. **Not insert-vs-restore**, per the byte-identical window above.

The surviving lead, recorded on S45 as a lead and **not** as a diagnosis:
`ModuleViewStruct.cpp:1155` asks for `createTextFormat(9.0f, {}, …)` — an **empty
font family** — while the browser side (`EditorLib/CUG.cpp:3410`) calls
`createTextFormat()` with none. Whether those resolve differently is unmeasured.

### A trap I checked and cleared, so nobody spends the session on it

`gmpi_ui/helpers/FontProvider.h` gates its fontconfig matcher behind
`__has_include(<fontconfig/fontconfig.h>)` — the exact silent-compile-out shape
S43(ii) documented, and it looked like the answer. **It is not:** the header is at
`/usr/include/fontconfig/fontconfig.h`, a default system include path, so it
compiles in whether or not CMake adds anything. Written down because the pattern
is genuinely dangerous elsewhere and will attract the next reader too.

**Verified:** build 312/312 rc=0, 0 errors; Accept run as written; byte-identical
window across restart; `strace` font census; the prefab byte scan.

**Not verified:**

- **The diagnosis of S45.** Eliminations only. I did not instrument the font
  resolution, which is the next step and is written on the row.
- **Jeff's "nothing at all on insert"** is still unreproduced and still not
  assumed to be either bug — though it is worth noting the freshly-inserted
  Oscillator draws **two small jacks and no faceplate**, which on a full rack
  could easily read as "nothing appeared".
- **Windows.** Unmeasured for either symptom.

**Learned:**

- **Run a row's Accept before building toward its diagnosis.** S25 handed me a
  suspect list and a next step; the Accept took twenty minutes and made the whole
  list moot. Sixth re-point in a row where the cell aimed at work already done.
- **A symptom that appears in BOTH arms of a comparison cannot be evidence about
  the comparison.** The tofu was real every time anyone looked, which is why it
  kept being attached to a row about insert-vs-restore; the thing that separates
  them is asking whether the *other* arm has it too.
- **The same window rendering some text correctly is the cheapest possible
  elimination**, and it was visible in the very first screenshot. It kills "fonts
  are broken on this box" before any tooling.
- **`__has_include` guards deserve a check, not a conclusion.** This one looked
  exactly like S43(ii)'s and is inert here because the header sits in a default
  system path.
- **`pgrep -f <pattern>` self-kill bit me twice in one session**, both times on a
  pattern containing my own build path. The journal has warned about this three
  times. `pgrep -x <exename>` does not have the problem.

**Machine left clean.** Headless weston stopped, the standalone stopped, all work
in scratch worktrees and a scratch `HOME`; **nothing was written to Jeff's
`~/.config`** and nothing installed. All six repos on their default branches and
clean.

**Branch/PR:** `tide/linux/S25-fresh-insert-tofu` — TideSynth only: the S25 and
S37 rows, the new S45 row, the linux NEXT cell, three screenshots and this entry.

## 2026-08-24 — linux — S37: CLAP has no bundle on Linux, so TIDE ships a semi-bundle instead (interactive, Jeff directing)

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

Jeff, on the shared-resources design: *"we can't have some Resources folder in
the CLAP directory that 100 plugins write into. That's amateur-hour. That's
nonsense."* Then, after the research: *"on Linux CLAP we ship a 'semi-bundle',
simple, easy. do it."*

**Zero code change in `SynthEditLib`.** This is `scripts/package-linux.sh` plus
two docs.

### He also caught that this row's framing was wrong

S37 says *"a second GMPI CLAP installed alongside"*, which reads as a `.gmpi`
module sharing a directory with a `.clap`. **That cannot happen** — different
formats, different scan roots. The only real case is **two GMPI-based CLAPs both
in `~/.clap`**, and a non-GMPI CLAP cannot trigger it at all: it never reads
`Resources/`, and its uninstaller ships no `ControlsXp.xml` to delete. The row
now says so.

### Option (a) is impossible, and that is measured rather than argued

`clap/entry.h`: *"plugin_path is the path to the DSO (Linux, Windows), or the
bundle (macOS)."* "Bundle" appears **twice in the entire CLAP repo** — that line
and the search rule.

REAPER 7.43, headless weston, isolated `HOME`, reading
`reaper-clap-linux-x86_64.ini`. Two plugins per scan so nothing could be deduped:

| layout | discovered |
|---|---|
| `~/.clap/SawDemo_CLAP.clap` (flat control) | **yes** |
| `~/.clap/nested/FreqAnalyser_CLAP.clap` | **yes** |
| `~/.clap/SawBundle.clap/SawDemo_CLAP.clap` | **yes** |
| `~/.clap/FreqBundle.clap/Contents/x86_64-linux/*.so` — **true bundle** | **NO** |

### The answer was in the same header the row already quoted

*"Each directory should be **recursively** searched."* So the plug-in lives in
its own subfolder, `getBundleContentsFolder()` walks for a `Contents` element,
finds none, returns `parent_path()` — and that is now `~/.clap/TIDE-Rack/`.

**That is why option (b) is not needed.** (b) moves the fallback for *every*
non-bundled GMPI consumer — the Linux `.gmpi`, the standalone, Windows — a
compatibility break for already-installed products, to fix a collision that today
has exactly one participant. The packaging change costs nobody anything.

### Verified

- **REAPER lists `TIDE Synth: TIDE Rack`** from `~/.clap/TIDE-Rack/TIDE-Rack.clap`
  — **one** entry, not two.
- **`strace` of the installed layout: 6 reads under `TIDE-Rack/Resources/`, 0
  under the shared `~/.clap/Resources`** — *while that shared folder still exists
  and still holds TIDE-named files.* That is the strong form: the collision target
  is present and ignored, not merely absent.
- Diagnostics clean: `enriched 26 of 70`, **`6 rack prefab(s) seeded`**,
  `root MIDI-CV seeded`.
- **Negative control:** same binary, no `Resources` beside it → all four
  `missing from bundle resources` plus `no Prefabs folder`. The instrument fires.
- **Upgrade path:** `install.sh` run against a simulated old install — new
  semi-bundle in, stale loose `TIDE-Rack.clap` removed, planted
  `OtherPluginData.xml` **survives**.

### The uninstall decision, which is the one judgement call here

`install.sh` **does not delete `~/.clap/Resources`**, and that is deliberate. By
the time we run, those files may be another product's — deleting a shared folder
by name is *precisely* the harm S37 documents. It removes only
`~/.clap/TIDE-Rack.clap`, which is unambiguously ours, and prints a note that the
shared folder is being left behind.

Removing the stale `.clap` is not optional: without it a host lists TIDE Rack
twice, and the old copy still reads the shared folder.

**Learned:**

- **"Can we use a bundle?" and "can we get a private directory?" are different
  questions, and only the second one mattered.** The spec says no to the first
  and yes to the second in adjacent paragraphs.
- **A negative control that leaves the hazard in place beats one that removes
  it.** Straceing against an install where the shared `Resources` is still
  present, still populated, and still ignored proves isolation; an empty parent
  directory would have proved nothing.
- **The expensive option was expensive because of who else it touched, not
  because of its size.** (b) is a few lines in `BundleInfo`; its cost is every
  other GMPI consumer. Ask which products a shared-code change moves before
  costing it.
- **A row's own wording can smuggle in a false premise that survives three
  runs.** "A second GMPI CLAP" was written by someone who knew what they meant;
  every later reader, including me, inherited the wrong picture until Jeff asked
  the plain question.
- **`weston --backend=headless --xwayland` plus an isolated `HOME` makes a real
  commercial DAW a scriptable instrument on this box.** REAPER's
  `reaper-clap-linux-x86_64.ini` is a readable scan report.

**Not verified:**

- **Only REAPER.** Ardour 8.4 here has no CLAP support at all; Bitwig, Qtractor
  and Carla are unmeasured. The spec mandates recursive search and REAPER honours
  it, but *"hosts should"* is not *"all hosts do"*.
- **Windows.** Same fallback and the same `%COMMONPROGRAMFILES%\CLAP` sharing,
  but TIDE ships no Windows CLAP today.
- **The VST3 is untouched** and did not need touching — its resources are inside
  its own bundle already.

**Machine left clean.** REAPER 7.43 downloaded to the session scratchpad and run
only against scratch `HOME`s — **Jeff's `~/.clap` and `~/.config/REAPER` were
never written to**, confirmed after. Headless weston stopped. Worktrees removed;
the `gmpi_ui/.claude/worktrees/` one belongs to another session and was left
alone.

**Branch/PR:** `tide/linux/S37-clap-semi-bundle` — TideSynth only:
`scripts/package-linux.sh`, `docs/distribution.md`, the S37 row and this entry.

## 2026-08-24 — macos — Merged the queue, then closed #291 on a green main (interactive)

**Prompt:** resolve and merge PRs / then take next task

Jeff directed merging, which overrides the standing "never merge your own PR".
Four landed: SynthEditLib#37 (S5), TideSynth#349 (S44, the Windows box's),
#350 (E18), and #353 (S5 record, which had already gone in).

**Order mattered and was not obvious.** #350's CI was red on `render-linux` and
`render-windows`, and the tempting read is "my PR broke CI". It did not:
`build.yml` had failed on `main` for five consecutive runs, and #350 touches two
docs and a script CI never runs. The fix for that red was sitting in #349 — the
Windows box's reference-set split. Merging #349 first turned the render jobs
green, and #350 then went in clean. Diagnosing before merging is what avoided
merging a red PR and calling it pre-existing.

**I shipped a broken commit and CI caught it.** Rebasing #350 the second time, I
resolved the BACKLOG conflict with a script that finds the FIRST
`<<<<<<<`/`=======`/`>>>>>>>` triple. There were TWO. `rebase --continue`
committed the file with live conflict markers in it, and I force-pushed. CI's
`lint` failed with `2 DUPLICATE ID(s): E9, E10`.

Worse, my own local lint run had said rc=0 on that same file. The reason is
worth keeping: I wrote

    printf "  %-24s rc=%s\n" "$(echo $c|cut -d' ' -f1)" "$?"

and `$?` there is the exit status of the `cut` in the command substitution, not
of the linter. Every rc I printed in that loop was `cut`'s. Same shape as the
earlier `git push | sed` bug — a status read through an intervening command.
**Run linters on their own line, or capture the status before anything else
runs.** After fixing that, the checks genuinely passed and so did CI.

Then STEP 1: issue **#291** (Jeff's, `platform:linux`) asked for three things —
decide the metric, re-bake references off the Mac, and actually run the test.
S44 and the render job satisfy all three, so I closed it on measured evidence
rather than on the fact that the PRs merged:

| platform | worst scene | changed | worst Δ |
|---|---|---|---|
| linux | — | 0.000% | 2 |
| windows | `shapes` | 0.083% | 10 |
| macos | `knob` | 0.023% | 17 |

Limits `0.800%` and `40`, 30 scene checks, run 32665903962. The issue's
*"`shapes` has 1% of margin, borderline flaky"* was a cross-ISA artefact — with
per-platform sets it is 0.083% against 0.800%. The tightest margin left is
macOS `knob` worst-delta **17 against 40**, recorded because it is the honest
counterpart to the comfortable changed-fraction numbers.

`main` is green on all three render platforms for the first time.

Rows flipped: S5, E18, S44 → DONE. Open PRs across all six repos: **zero**.

## 2026-08-24 — macos — E18: the probe stops excusing the case E10 fixed (interactive)

**Prompt:** merged. sync. continue.

E9, E10 and S35 all merged, so I flipped the three IN-REVIEW rows to DONE and
re-pointed the mac NEXT cell (it still announced E9 as IN-REVIEW; that is the
fourth time this cell has gone stale in two days).

E10 landing made `scripts/measure-chunk-robustness.py` wrong in a way that would
have quietly stayed wrong. Its `skeleton` case — `<Module>` with no
`<PatchManager>`, the one shape TIDE's own guard cannot see — was expectation
`known-limit`: reported, never failed, because the engine fix was GATED. The
probe even printed `NO LONGER CRASHES -- has E10 landed?`, which is the script
asking to be updated. Left alone it would have gone on passing whether or not
E10 held.

The case is now expectation `survive`: no `TIDE: REFUSED` line is expected (TIDE
genuinely cannot catch that shape), but the host must not die, and a crash is a
hard FAIL naming `SeAudioMaster::BuildDspGraph`. E10 now has a regression test
instead of an excuse.

Measured both directions on this box, same probe, minutes apart — that is the
whole point of the change, so both halves were run:

| binary | skeleton | REAPER crash reports | new logic |
|---|---|---|---|
| pre-E10 (Aug 23 release build) | `rc=-11` SIGSEGV | 2 → 3 | exit 1, FAIL |
| SynthEditLib main, E10 merged | `rc=0` rendered | 2 → 2 | exit 0, PASS |

The crash-report count is the independent evidence: `rc` alone is the probe's
own reading of its own run, and I wanted something outside the probe to agree.
The E10 build was configured with all four local overrides confirmed by their
`Using local ...` lines, so the SynthEditLib under test was the merged one.

The docstring's `*** THIS TOOL CRASHES REAPER ONCE, ON PURPOSE, EVERY RUN ***`
banner was true when written and is now false. Anyone reading it would expect a
crash and not investigate one. Rewritten to say the opposite, with the two
measurements recorded inline.

Jeff's installed plug-in was backed up before the swap and restored afterwards,
byte-identical — what is in `~/Library/Audio/Plug-Ins/VST3` is the Aug 23
release build he had, not my Debug build. The two REAPER crash reports the runs
produced were left in place rather than deleted: they are his data, and the
pre-E10 one is real evidence.

**Not verified:** Windows and Linux. The probe drives REAPER and has only ever
been run on macOS, so the new failure path is unexercised there.

## 2026-08-24 — macos — S5: the folder-info null deref, measured then guarded (interactive)

**Prompt:** sync all repos. next task

S5 was filed against `SE16/SynthEdit2/Application.cpp:167`. That path is dead:
the file moved repos, and neither `m_folder_settings` nor
`refreshFolderLocations` appears anywhere in SE16 today. It lives in
`SynthEditLib/EditorLib/Application.cpp` — EditorLib, which is GATED, and this
session is interactive, which satisfies the gate. Anyone taking this row from
its stated path would have concluded the bug was gone.

I nearly did. Reading `getFolderInfo` it looks safe — it loops over
`m_folder_settings`, and an empty vector just means zero iterations, then it
creates an entry and returns `.back()`. I wrote that down as "no UB". It was
wrong, and the reason is worth recording: the fall-through line is 300-odd
characters long, my grep truncated at 124, and the two `m_folder_settings[0]`
subscripts are in the tail. Printing the whole line is what found it.

So the row is right. An unrecognised extension inherits entry 0, `"All Files"`,
which exists only once `refreshFolderLocations()` has run — and
`TideApp::InitInstance` replaces `CSynthEditAppBase::InitInstance` wholesale
(S1a, the module-scan removal) and never calls it. In TIDE the vector is empty
for the life of the app.

Measured rather than argued. A harness linking the real `libEditorLib.a`,
subclassing `ApplicationBase`, deliberately not calling
`refreshFolderLocations()` — TIDE's exact state:

| | result |
|---|---|
| before | `getFolderInfo(L"sem")` → SIGSEGV, exit 139 |
| lldb | `EXC_BAD_ACCESS (code=1, address=0x0)` in `unique_ptr::operator->`, `this=0x0` |
| after | valid `folder_info`, description `"sem Files"`, exit 0 |

The second half matters as much: a populated entry 0 must still be inherited, or
the guard would have quietly broken full SynthEdit. Same harness, entry 0 seeded
— identical output before and after. `EditorLib` also builds clean against the
patched tree, all four overrides confirmed by their `Using local ...` lines.

**Not the fix:** calling `refreshFolderLocations()` from the `ApplicationBase`
constructor. It looks like the deeper repair — populate the vector once and no
caller can trip — but `getSettingString` is `virtual`, so a constructor call
dispatches to the base and seeds the wrong folders. The guard is the one
subscript in the repo; `.back()` on the next line is safe by construction.

**Left open deliberately:** `refreshFolderLocations()` still never runs in TIDE.
Its folders stay empty, so `ShortenFilename` returns absolute paths where it
would otherwise return relative ones — documents are less portable. That is a
behaviour question, not undefined behaviour, and it belongs to whoever decides
what TIDE's folder defaults should be, so it is on the row rather than changed
under cover of a crash fix.

**Not verified:** Windows and Linux. The harness was built and run on macOS.

Harness kept out of the repo on purpose — it needs three link stubs for UI
dialogs EditorLib references, which is scaffolding, not a test the project
should carry. SynthEditLib has no test target to put it in.

## 2026-08-24 — linux — the A4 auto-merge trap, hit a third time, and the branch DELETION is the new half

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**A correction to the entry directly below**, not a second item. That entry ends
*"Machine left clean"* and names no trap, because [#351](https://github.com/JeffMcClintock/TideSynth/pull/351)
merged after it was written. A separate entry rather than an edit, for the reason
#121 established: a log you edit is not a log.

**Nothing is wrong on `main`.** The rows and the entry landed in `9e5cb27`, whose
own subject carries `(#351)`. This is about how the run ENDED.

### What happened

STEP 4's A22 dance is: name the branch, push, open the PR, then push one more
commit adding the PR number — *"Check the PR is still open before you push that
follow-up, and if it has already merged, DROP it."* I did check. It said `OPEN`.
I made the commit. Then I ran the re-check and the push **in the same command
block**, chained unconditionally:

```
gh pr view 351 --json state --jq .state   ->  MERGED
git push                                  ->  * [new branch]
```

**The check fired correctly and I pushed anyway**, because I had already decided
to push when I wrote the block. STEP 4's rule is not "check", it is "check, and
branch on the answer" — a check whose result cannot stop the next command is
decoration.

### The new half: auto-delete means the follow-up RE-CREATES the branch

#120/#121 landed a follow-up on a branch whose PR had merged. Here the branch was
**already deleted** by merge auto-delete, so `git push` did not update anything —
`* [new branch]` — it **brought the branch back from the dead**, with a commit
nobody had asked to review.

That is strictly worse than #120/#121 and it does not look worse: the push output
is a cheerful `[new branch]` line identical to a first push. **The tell is that a
follow-up push should never say `[new branch]`.** If it does, the PR closed and
auto-delete ran.

### Fixed, by deleting rather than by a second PR

Deleted `origin/tide/linux/issue-156`. STEP 4 says pushing nothing is always safe
here and *"a commit whose only content is a link is not worth a second PR"* — and
A22's whole point is that **the branch name in the row is what makes the follow-up
optional**. Both rows name the branch, `9e5cb27`'s subject names the PR, so the
deleted commit carried nothing that is not already on `main`.

Checked before deleting that it was mine and that its parent was on `main`. The
three remaining remote branches belong to other boxes and were left alone.

**Verified:** `9e5cb27` is on `origin/main` and contains both row updates and the
entry; `git ls-remote --heads` shows no `tide/linux/**` branch; PR #351 `MERGED`;
issue #156 `CLOSED`.

**Not verified:** whether auto-delete is repo policy or was configured per-PR —
I observed the effect, not the setting.

**Learned:**

- **A `gh pr view` state check is worth nothing in the same unconditional command
  block as the push it is meant to gate.** Run it, read it, then decide. This is
  the third time this fleet has met the A4 auto-merge race and the first time the
  check was actually present and still did not help.
- **A follow-up push that reports `[new branch]` has re-created a deleted branch,
  not updated one.** With auto-delete on, the A22 follow-up window closes by
  removing the branch, so the failure mode is resurrection rather than a stranded
  commit — and the output looks like success.
- **The A22 follow-up is optional by design, so "drop it" is cheap.** The instinct
  to salvage the commit with a second PR is the expensive branch, and STEP 4
  already ruled it out; deleting is the one-command answer.
- **A merge that happens between writing an entry and pushing it makes that entry
  wrong about its own ending.** Cheaper to prepend a correction than to leave
  *"machine left clean"* as the last word on a run that left a stray branch.

**Machine left clean**, now genuinely: no `tide/linux/**` branch on the remote,
all scratch worktrees removed, all six repos on their default branches with clean
trees, nothing built in any of Jeff's checkouts, nothing installed.

**Branch/PR:** `tide/linux/issue-156-followup` — TideSynth, this entry only.

## 2026-08-24 — linux — STEP 1: #156 verified green on Linux, and the 44 failures reproduce on demand

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude Code **2.1.220** · as **tide-rack-bot** (both paths)

**Did:** took no backlog item. **STEP 1 outranked STEP 2** — there are three open
`platform:linux` issues, and one of them was actionable. Verified
[#156](https://github.com/JeffMcClintock/TideSynth/issues/156) on this platform
and closed it. **Zero product code changed**; this repo gets the two row updates,
a re-pointed NEXT cell and this entry.

### Why #156 and not the other two

STEP 1 says a platform issue outranks backlog work, so the first job was working
out which of the three is mine. Written down because the next Linux run will meet
the same three:

| issue | author | verdict |
|---|---|---|
| [#291](https://github.com/JeffMcClintock/TideSynth/issues/291) render references re-baked on the wrong box | Jeff | **not mine** — the fix is [#349](https://github.com/JeffMcClintock/TideSynth/pull/349), open and green from the windows box. Taking it would be the duplicate-work collision STEP 2 exists to prevent. |
| [#88](https://github.com/JeffMcClintock/TideSynth/issues/88) `SynthEditJuce` misses `Dialogs_editor2.cpp` | bot | **not takeable** — `SE16/SynthEditJuce/` is on neither STEP 5 list, so GATED by default. Not a build break either: the target is deprecated and reachable from no build on any box, so A17's exception does not stretch to it. |
| [#156](https://github.com/JeffMcClintock/TideSynth/issues/156) ctest 44/67 on Linux from macOS-hardcoded paths | bot | **this one.** Its stated cause was fixed by **S16** + **S42**, and *both rows say in bold "NOT VERIFIED: Windows and Linux"*. The verification is the work. |

STEP 1's bot-issue rule — *"re-verify the finding on your own platform before
acting on it"* — is the whole item here rather than a preamble to it.

### The measurement

Fresh scratch worktree of `SE16` at `origin/master` **`63ce2bb8e`** (the merge of
[SynthEdit#74](https://github.com/JeffMcClintock/SynthEdit/pull/74)), overrides
pointed at scratch worktrees of `SynthEditLib` `fb55275`, `TideSynth` `1364801`,
`gmpi_ui` `6aa8871`, `GMPI` `83b9de7`. Ninja, Release, GCC.

**The checkout path is the point.** It is
`.../scratchpad/wref/SE16` — not `~/SE/SE16`, not any developer's checkout, and
not the path baked into the fixtures. S16's Accept is *"from a checkout at any
path"*, and a build in the usual place cannot test that clause at all.

Configure **rc=0**. `cmake --build --target dsp_tests` is **327/327, rc=0, 0
errors** — and pulling in `SynthEditCL` and `cancellation` on its own is S16's
`add_dependencies` working, which the row says was necessary but not sufficient.

**Both defaults reached the binary, checked with `strings` rather than assumed:**

```
SE_BUILD_FOLDER_DEFAULT    -> .../scratchpad/build-se16
SE_UNITTEST_FOLDER_DEFAULT -> .../scratchpad/wref/SE16/UnitTest
/Users/jeffmcclintock       -> 0 occurrences
```

**ctest with `SE_BUILD_FOLDER` and `SE_CANCELLATION_FOLDER` deliberately unset:**

```
100% tests passed out of 73
```

**rc=0, stable across two consecutive runs.** Zero `not found` lines, zero
`32512`, and **zero references to the dead `/Users/jeffmcclintock/` checkout** —
against the 536 S42 measured mid-arc on macOS.

### The negative control, which is what makes the green mean anything

A suite that cannot fail reports 100% for the same reason a fixed one does. So,
same binary, same run, one variable:

| `SE_BUILD_FOLDER` | result |
|---|---|
| unset (the CMake default) | **73/73 pass, rc=0** |
| `/Users/jeffmcclintock/SynthEdit/build/` | **44 failed of 73, rc=8** |

**44 is the exact number in #156's title.** The issue's headline failure
reproduces on demand and disappears on demand, and the variable is the one thing
S16 changed.

One difference from #156's era, recorded because the next person will grep for
the old signature and not find it: the failure no longer surfaces as `system()`
returning `32512`. S16 added an explicit existence check, so
`Basics.Cancellation_Utility_Exists` and `TestUI.CancellationUtilExists` fail
first and name the missing helper. Better diagnostic, different string.

### The one red I had to chase, and it was not a defect

The first ctest run was **63/64 with `ui_tests_NOT_BUILT` failing**, which looks
exactly like a broken target. It is `gtest_discover_tests`' placeholder for
`ui_tests` — a `gmpi_ui` target registered at configure time that I had never
built, because I built `--target dsp_tests` and nothing else. Building `ui_tests`
(7/7, rc=0) turned 64 tests into 74 and the suite green.

Worth separating the two numbers, because they are both quoted in this project
and they are not the same suite: **`dsp_tests` is 63 of 63 here, which is exactly
the 63 the macOS run measured** — same size, both platforms, which is stronger
agreement than a pass rate. The 73 is `dsp_tests` + `ui_tests`; one further test
is `Disabled`. **#156's own "67" is a third number** from a differently-configured
tree, so do not read a mismatch against it as a regression.

`synth_ui_tests` was **not** generated: `GMPI_UI_TESTS_FOLDER` defaults to
`C:/SE/gimpi_ui_tests`, and this box's clone is at `~/SE/gimpi_ui_tests`. Not
part of this issue; noted so nobody reads its absence as breakage.

### Both default branches build on Linux — no platform issue to file

STEP 3 asks every run that builds anything to say whether its platform's default
branch also builds, and this run built two trees, so both answers are first-hand
rather than inherited from CI:

| tree | result |
|---|---|
| `SE16` `origin/master` `63ce2bb8e` | `dsp_tests` chain **327/327** + `ui_tests` **7/7**, rc=0, 0 errors |
| `TideSynth` `origin/main` `1364801` | **483/483, rc=0, 0 errors**, all four Linux artifacts |

The TIDE artifacts, from an unmodified `main`: `TIDE-Rack` (standalone),
`TIDE-Rack.clap`, `TIDE-Rack.gmpi`, and the `TIDE-Rack.vst3` bundle carrying
`TIDE-Rack.so`. **So there is no `platform:linux` build break to file**, and the
three open Linux issues are all about tests and packaging rather than compilation.

**Verified:** configure rc=0; `dsp_tests` 327/327 rc=0; `ui_tests` 7/7 rc=0;
ctest 73/73 rc=0 twice; `strings` control on the binary; the 44-failure negative
control; TIDE's own default branch built (below).

**Not verified:**

- **Windows.** Both S16 and S42 still say "NOT VERIFIED" for it and this run does
  not change that. The rows now say Linux is done and Windows is not.
- **`synth_ui_tests`**, for the folder-default reason above.
- **`SE16`'s full tree.** I built the `dsp_tests` and `ui_tests` chains, not all
  of `SE16`, so this says nothing about `SynthEditWayland` or #88.

**Learned:**

- **A "NOT VERIFIED on your platform" line in a DONE row is a work item, and
  nothing points at it.** S16 and S42 both carried one for a day. The thing that
  surfaced it was not the backlog — it was an open `platform:{PLATFORM}` issue
  describing the same defect, which STEP 1 forces every run to read first.
- **The negative control was one environment variable and it is the whole
  entry.** Everything else here is "the tests pass", which is what a suite with
  its fixtures missing also reports once it stops being able to fail. Reproducing
  #156's own 44 is what turns that into a measurement.
- **`gtest_discover_tests` registers a `<target>_NOT_BUILT` placeholder**, so a
  partial build produces a red test that names a target rather than a defect.
  Building one target and running the whole suite will always look like this.
- **Three quoted pass counts for one suite — 63, 67, 73 — and all three are
  correct.** They differ by which targets the tree generated. Quote the target
  with the number or the next run reads a configuration difference as a
  regression.
- **A scratch worktree is not just tidiness here, it is the test.** "From a
  checkout at any path" is unfalsifiable from the developer's own checkout.

**Next:**

1. **Windows is the remaining half of S16/S42.** Same recipe, one command, and
   the negative control transfers unchanged.
2. **#88's `SynthEditJuce` half needs an owner who can say "by inspection"** —
   it is one line, in a GATED path, in a target no box builds.
3. **S23 remains this box's take-target** and needs no ruling; the linux NEXT
   cell is re-pointed at it and says why S43(ii) and S37 are no longer options.

**Machine left clean.** Five throwaway worktrees under the session scratchpad,
one per repo, plus two scratch build trees. **Nothing was built in any of Jeff's
checkouts** and `~/SE/build` was not touched. No compositor was started, nothing
was installed, and no plug-in was copied anywhere. All six repos were clean and on
their default branches at the start and are back on them at the end.

**Branch/PR:** `tide/linux/issue-156` — TideSynth only: the S16 and S42 rows, the
linux NEXT cell, and this entry. **No product code change in any repo.**

## 2026-08-24 — macos — S35: the scanner searches both plug-in domains now (interactive)

**Prompt:** 5146a61 · claude-opus-5 · app unknown · as tide-rack-bot (both)

Fixed in [SynthEditLib#36](https://github.com/JeffMcClintock/SynthEditLib/pull/36),
option (a) — the one the row calls the right fix.

**Confirmed the mismatch on this box before touching anything:** `ModulePath` is
`/Library/Audio/Plug-Ins/GMPI` with 7 modules; `~/Library/Audio/Plug-Ins/GMPI`
has 9; none of the 9 were scanned.

New `getUserPluginsFolder()` returns EMPTY on Windows and Linux — Windows has no
per-user plug-ins location, and Linux already keeps everything under the
per-user data dir, so a second scan would be the same folder. Empty means no
second scan, so neither platform changes behaviour. The user path is DERIVED
from `ModulePath` rather than hardcoded, so someone who has repointed it keeps
one scan instead of silently gaining a folder.

**The verification is where this got interesting.** The row's own evidence metric
is "user-domain paths in `Plugin-Cache-16-override-*.xml`" — and after the fix it
was still **zero**. It would have been easy to read that as the fix not working,
and equally easy to ship it claiming success without looking.

What it actually is: those caches store module metadata without absolute paths,
and the modules were already known. Running `SynthEditCL -rescan` and reading the
output settled it — the scanner now prints both scan lines, and of the nine
user-domain modules **eight are duplicates of factory SEMs**, reported as
*"Module FOUND TWICE!"*, which is correct behaviour. The ninth is
**`TIDE-Rack.gmpi`**, not a duplicate, now visible where it was not.

A locally built plug-in module is the entire case this row exists for, so that
one file is the result — not the cache metric the row happened to reach for
when it was filed.

**Not verified:** Windows and Linux. Both take the empty-string path and get no
second scan, but only macOS was run.

**Note:** I ran `-rescan` against Jeff's real settings with a scratch build,
which rewrites his module cache. Compared before and after: identical content,
so nothing of his changed.

## 2026-08-24 — macos — E9: the AU absorbs a rate change, and the pitch is the proof

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** took **E9** — the mac NEXT cell named S42, which is DONE, so it fell to
the topmost eligible row and the `any` cell pointed here anyway. E9's only
remaining clause is *"AU remains genuinely unmeasured"*, which defers to **R3a**,
which the row calls `BLOCKED(M1)` with *"TIDE builds no AU"*. **Both halves went
stale two days ago:** M1 and R3a are DONE, and `SynthEditSem/CMakeLists.txt:163`
is `GMPI VST3 CLAP AU3 STANDALONE`. So the AU path became measurable and nobody
had noticed.

### The result

`tests/e9_au_rate_probe.mm` — a real AUv3 host handshake, then the
allocate/render/deallocate bracket at **48000 → 44100 → 48000**, which is what a
DAW does on a device rate change. **25 checks, all passing, byte-identical
across three consecutive runs.**

| rate | measured pitch | peak |
|---|---:|---:|
| 48000 | **440.0093 Hz** | −6.29 dBFS |
| 44100 | **440.0093 Hz** | −6.33 dBFS |
| 48000 again | **440.0093 Hz** | −6.29 dBFS |

**−0.000 cents**, against a stale-rate prediction of **404.2586 Hz** — 1.47
semitones flat, the "sounds wrong rather than broken" case the row's original
text calls the worst kind.

### Why this asks a harder question than the VST3 and CLAP halves did

The CLAP probe says so of itself: *"Deliberately NOT a null test: it asserts the
handshake completes and the plugin reports the rate it was given."* That is a
fair test of the mechanism and it is **not this row's Accept**, which is
*"changing the host's sample rate on a loaded project **re-tunes correctly**"*.
A handshake that completes at 44100 says nothing about tuning.

So the probe loads the **actual rack from `tests/hosts/v1-rack.rpp`** — the
fixture documented at 440.0 Hz / −6.3 dBFS — through `setFullState`, and
measures the output pitch. Getting the document there needed no new format
work: the AU3 wrapper carries TIDE's state verbatim as the `GMPIPRESET` key
(`AU3_Wrapper.mm:509,522`), which is the same outer `<Preset>` element the
`.rpp` already holds, so `scripts/decode_rpp.py` grew `--preset-out` and a VST3
fixture drives the AU unmodified.

### The reading is self-validating, which is why 440 twice is a strong result rather than a suspicious one

I did distrust it — identical to four decimal places at two different sample
rates looks like something that is not changing. It is the opposite:

**A plugin that ignored `setFormat`, one pinned to a fixed rate, and a rack that
kept a stale rate all emit samples on the 48 kHz grid for the 44.1 kHz leg — and
all three then measure 404.26 Hz, not 440.** Reading 440 Hz at 44100 is only
possible if the bus rate really changed *and* the rack rebuilt for it. The
failure modes collapse onto the same number, and it is not the number I got.

**Three controls, because a null result is worth what its controls are worth:**

1. `--selftest` measures synthetic tones including the predicted stale-rate
   404.25 Hz. The two hypotheses come out **35.75 Hz apart**, and digital
   silence correctly yields no frequency at all — without that last case,
   "it reported 440" and "it reports 440 for anything" look identical.
2. Each leg's own audio re-read at the *other* rate gives **478.9217** and
   **404.2586 Hz**, matching the predicted ratios to 4 dp. So the analyser
   demonstrably tracks its rate argument.
3. The legs rendered **96000 vs 88200 samples** — not the same buffer.

**The mechanism is the same one this row established for VST3 and CLAP:**
`AU3_Wrapper.mm:577` reads the rate off the output bus and `:600` calls
`plugin.start_processor(...)`, so `processor_holder.cpp` releases the old
processor, creates a fresh one, `open()`s it, and re-seeds the blob from
retained bytes. Instance replacement, on all three wrappers.

### The claim I printed that was false, and the instrument that caught it

The probe reported **"loaded in-process"** for three runs. It was printing the
option I had *requested*, not what happened. Two instruments settled it:
`NSStringFromClass` gives **`AUAudioUnit_XH`**, a proxy, and — dispositive —
walking `_dyld_image_count()` shows the **appex binary is not in this process's
address space at all**.

So the AU is hosted out-of-process, TIDE's own `TIDE: rack built for N Hz`
diagnostic cannot reach the probe, and the absence of that line means nothing.
I had spent time hunting for it in stderr and in the unified log before asking
whether it *could* be there. The audio is the whole of the evidence, and it is
the better evidence anyway.

### The build failure that was mine

The first build came back **rc=2** — codesign failing on a missing
`TIDE-Rack.appex`, with 0 compiler errors — which looks exactly like a
`platform:mac` break worth filing. It was not. The log showed the `TIDE_Rack_AU3`
target compiled and linked **twice**, with the progress counter going backwards
from 100% to 96%: an earlier backgrounded build I believed had been killed was
still alive and building into the same directory as the new one. Two `make`
processes, one build tree, racing over the appex.

A clean single rebuild is **rc=0, 0 errors, all five artifacts**. **So macOS
`main` builds and there is no platform issue to file** — and I nearly filed the
fleet's own #314-class race report against a bug I had caused.

**Verified:** 25/25 probe checks; three byte-identical runs; `--selftest` 6/6;
clean `main` build rc=0 with all five macOS artifacts.

**Not verified:**

- **No real DAW.** `e9_au_rate_probe` is ours. What Logic or GarageBand does on
  an actual device rate change is unmeasured — though the AU API bracket the
  probe drives is the one those hosts use.
- **iOS AUv3 was not exercised at all.** This is the macOS AU only.
- **The plug-in's own diagnostic**, for the out-of-process reason above.
- **Windows and Linux** build nothing AU-shaped, so nothing there was touched.
- **The analyser carries a ~0.2 Hz systematic bias on a decaying tone** (visible
  in `--selftest`: 439.80 for a true 440). It is a threshold artefact, it is
  common-mode across rates, and the verdict is a *ratio* between legs, so it
  cancels. Stated because the absolute figure 440.0093 should not be read as a
  tuning measurement of the rack.

**Learned:**

- **A probe that prints the option it requested is not reporting a measurement.**
  "Loaded in-process" was wrong for three runs and would have gone into this
  entry as fact. The class of the returned object, and better the loaded-image
  list, are the things that actually answer it.
- **When every failure mode collapses onto the same wrong number, the right
  number is strong evidence.** Working out what an ignored `setFormat`, a
  fixed-rate plugin and a stale rack would each measure — all 404.26 — is what
  turned a suspicious-looking 440-at-both-rates into a result.
- **A backgrounded build you think you killed is still building.** The
  double-linked target and a progress counter running backwards are the tell,
  and the failure it produced was a perfect imitation of a real parallel-staging
  race this fleet has already fixed twice (#314, S21).
- **Check whether a row's blocker is still real before believing the row is
  closed.** E9 had one clause left, that clause pointed at R3a, and R3a had been
  DONE for two days. Nothing re-reads a deferral.
- **A fixture saved for one format can drive another without being re-authored**
  when both wrappers carry the same preset XML — one `--preset-out` flag beat
  authoring an AU-specific fixture by hand.
- **`pluginkit -a <appex>` registers an AUv3 straight out of a build tree**, no
  copy into `/Applications` and no first launch, which makes an AU measurable in
  seconds. It is a *developer* shortcut and **does not revise M1's install
  story** for a shipped pkg — and it must be undone with `pluginkit -r`, or it
  leaves a live registration pointing at a deleted tree.

**Next:**

1. **E9 is IN-REVIEW, not DONE** — a later run flips it when the PR merges.
   Its Accept is now met on all three wrappers TIDE builds.
2. **A real AU host is the honest next test**, and it needs a human: Logic or
   GarageBand, change the device rate on a loaded project.
3. **The mac box has run out of ungated scheduled work.** There are no
   `platform: mac` TODO rows at all, and the `any` queue's takeable set emptied
   this week. Both NEXT cells now say so. The real mac-only work left is
   verification that wants a keyboard — starting with **M2's** own record that
   the iOS app was installed but never launched and its Audio Unit never opened
   in an iOS host.

### Found while cleaning up: two commits of S27 stranded with no PR

Checking this box for leftover worktrees at the end turned up one belonging to
another session, on `tide/mac/S27-render-ci` — and that branch is **two commits
ahead of `origin/main` while its PR [#331](https://github.com/JeffMcClintock/TideSynth/pull/331)
is MERGED.** The follow-ups landed on a branch whose PR had already closed: **the
trap STEP 4 documents from #120/#121, hit again a week later.**

**It is not tidy-up — it is the answer S27 was waiting for.** The stranded commit
is *"two reference sets — macos and windows-linux — selected per platform"*: 24
files, +207 lines, the reference PNGs split per platform. S27's own history frames
the open question as *"per-platform references or pinning the math"* and measured
that only **two** sets are needed, because Linux and Windows agree to three
decimals. Someone built exactly that and nobody was ever asked to review it —
and **S27 is marked DONE**, so nothing would have looked again.

Filed as **S44**. Not fixed here: it is another session's branch, the standing
rule is not to delete other sessions' branches, and STEP 4 forbids rewriting a
pushed commit — so the only correct move is a PR someone chooses to open.

**Machine left clean.** All work in a throwaway worktree under the session
scratchpad; nothing was built in `~/Documents/GitHub/TideSynth`. The AUv3 was
registered from the build tree with `pluginkit -a` and **deregistered with
`pluginkit -r` afterwards**; nothing was copied to `/Applications` or
`~/Applications` and no plug-in was installed into `~/Library/Audio/Plug-Ins`.
All six repos were clean and on their default branches at the start and are back
on them at the end. **One worktree on this box is NOT mine and was left alone:**
another session's, under `/private/tmp/claude-501/…-GitHub/a3974193…/scratchpad/wref`,
registered against `tide/mac/S27-render-ci` — see S44.

**Branch/PR:** `tide/mac/E9-au-rate-verify` — [#347](https://github.com/JeffMcClintock/TideSynth/pull/347). TideSynth: one new test probe, one
flag on an existing script, the backlog and this entry. No product code change.

## 2026-08-24 — windows — S44: the stranded reference split, landed and verified on the platform that could not check it

**Prompt:** 5146a61 · Opus 5 (1M context), `claude-opus-5[1m]` · app: Claude desktop **1.34493.1** (Claude Code 2.1.237) · as **tide-rack-bot** (both paths)

**Did:** took **S44**. Both NEXT cells that could point here — `win` and `any` —
named it as the single ungated row left on the board, filed by the mac box
eighteen hours earlier while it cleaned up after E9. STEP 1 clear (no open
`platform:win` issues), STEP 1.5 clear (no open PR from `tide/win/**`).

### The row's premise, and the thing it could not know

`origin/tide/mac/S27-render-ci` carries the per-platform reference split whose
PR [#331](https://github.com/JeffMcClintock/TideSynth/pull/331) had already
merged when the follow-ups were pushed onto it. Its own commit message ends:

> Not verified: Windows and Linux have not yet run against `windows-linux/`.
> That is the next CI run, and the 0.083% figure predicts both pass.

**This is one of those two platforms.** The claim was measurable here and nobody
had measured it, so that came before deciding what to do with the branch.

### The measurement

`tide_render_regression` built from `main` at `7b34d8155`, MSVC 14.51 x64,
Release, in a scratch tree.

| references | result |
|---|---|
| **`windows-linux/` from the stranded branch** | **10 of 10 match, rc=0** — three consecutive runs identical |
| `main`'s current flat `tests/references/` (the macOS-arm64 bake from `246399a`) | **5 of 10 FAIL** |

The failing five, against limits of 0.800% and delta 40:

    knob      35.359%  worst delta 142 at (24,39)
    materials 34.847%  worst delta  63 at (117,37)
    shapes    67.014%  worst delta  46 at (126,40)
    glass     54.562%  worst delta  53 at (38,27)
    glow      61.528%  worst delta  62 at (82,12)

All five `-fast` variants pass at 0.000% on both sets, which is the stranded
commit's own claim that Fast is bit-identical everywhere.

**The prediction is confirmed to the digit, not merely in direction.** The
stranded commit measured Windows-vs-Linux as *"0.083% of pixels, worst delta 10
— glass, glow, knob and materials are 0.000%, only `shapes` moves"*. This box,
against images baked on an **ubuntu** runner, reads glass/glow/knob/materials at
**0.000%** and `shapes` at **0.083%, worst delta 10**. Same scene, same figure,
same delta. And the 35–67% macOS gap it quotes reproduces here as 34.8–67.0%.

**So `main`'s render job is red on Windows today** — the Windows half of
[#291](https://github.com/JeffMcClintock/TideSynth/issues/291), which was
labelled `platform:linux` and is not only Linux's.

### The defect the stranded commit had, which is why this is not a straight cherry-pick

It put the platform choice in `build.yml`'s render matrix as a `refs:` column
and updated **only that caller**. There are three:

    .github/workflows/build.yml:611       "$exe" tests/references …
    modules/common/CMakeLists.txt:119     add_test(… "${CMAKE_CURRENT_SOURCE_DIR}/tests/references" …)
    modules/common/README.md:277          tide_render_preview --references modules/common/tests/references

After the split `tests/references` holds no PNGs at all — only two
subdirectories — so `ctest` would have gone red comparing against an empty
directory, and a developer following the README would have re-baked into it.

**Selection now lives in `tide_render_regression` itself.** Hand it the root and
it descends into `macos` or `windows-linux` for the platform it was built for;
hand it a set and it uses that, which is what keeps `--references .../macos`
working for re-approving an intended look change. One change fixes all three
callers, and **`.github/workflows/build.yml` needs no edit at all** — which is
also what puts this inside what a scheduled run may push, since the bot token
deliberately lacks `workflow` scope. **The stranded commit's shape was
unlandable from a scheduled run on any box**, and that is not a small detail:
it is why the branch sat.

### Why not simply open a PR from the branch, which is what the row asks for first

It does not merge. `origin/tide/mac/S27-render-ci` conflicts with `main` in
three files — `.github/workflows/build.yml`, `BACKLOG.md`, and `JOURNAL.md`,
which has rotated since. Resolving it means committing to `build.yml`, and no
scheduled run on any box can push that. The PR would have been unmergeable by
construction and unfixable by the fleet that opened it.

So the substance lands instead, with the expensive part carried over verbatim:
**all twenty PNGs are byte-identical to their sources**, hashed against
`origin/main` (the ten `macos/`) and `origin/tide/mac/S27-render-ci` (the ten
`windows-linux/`). Nothing was re-baked here. The `windows-linux` images came
off a real ubuntu runner, and reconstructing them on this box would have
silently replaced a Linux bake with a Windows one — the two agree to 0.083%,
which is close enough that the substitution would not have shown up in any test
and far enough that it would have been the wrong thing to ship.

**The branch is deliberately left alone.** It is another session's, the standing
rule is not to delete other sessions' branches, and its commits are pushed so no
rewrite is permitted. It is superseded and wants a human to delete it.

**Verified:**

- 10/10 against `windows-linux/`, rc=0, three consecutive runs byte-identical.
- The **exact absolute argument `add_test()` passes** resolves to `…/windows-linux` and passes 10/10.
- The set named directly (what the README documents) — same.
- **Negative control:** `tests/references/macos` named directly → **5 of 10 FAIL**. The resolver does not quietly fall through to the set that would pass, which is the failure a "look for the right directory" fallback most easily hides.
- Twenty reference PNGs hashed against their two sources; all twenty identical.
- Clean rebuild, no warnings.

### CI closed all three of the gaps this entry was going to list as unverified, and the prediction held

The PR's own run — [#349](https://github.com/JeffMcClintock/TideSynth/pull/349),
all three render jobs **pass**, `main`'s `build.yml` line unchanged:

| job | resolved set | result |
|---|---|---|
| **render-windows** | `tests/references/windows-linux` | 10/10 — `shapes` **0.083%, delta 10**, everything else 0.000% |
| **render-linux** | `tests/references/windows-linux` | 10/10 — **0.000% on all ten**, worst delta 2 |
| **render-macos** | `tests/references/macos` | 10/10 — `knob` 0.023% delta 17, everything else 0.000% |

**Three things fall out of that table, and none of them were guaranteed.**

**The Linux prediction was right for the stated reason.** I wrote before the run
that Linux should read 0.000% rather than Windows' 0.083%, because the images
came off an ubuntu runner and Linux is comparing against its own bake. It reads
**0.000% on all ten**. The 0.083% is specifically the Windows-vs-Linux gap, not
noise in the set.

**Two different Windows machines agree exactly.** This box (MSVC 14.51, local)
and `windows-latest` both read `shapes` at **0.083%, worst delta 10** and
everything else at 0.000%. Same figure to three decimals on unrelated hardware,
which is the reproducibility the whole image-test design claims and rarely gets
to demonstrate across machines.

**`macos` is picked and passes**, so the `#if defined(__APPLE__)` arm is measured
rather than reasoned — and it is not a trivial pass: `knob` moves 0.023% at delta
17 on `macos-latest` against references baked on a Mac, so that set has real
runner-to-runner variation and still lands well inside the limits.

**And the workflow was never touched.** Three platforms resolved three sets from
one unchanged command line, which is the whole claim of putting the selection in
the binary.

**Not verified:**

- **`ctest` end-to-end.** `modules/common` alone registers `add_test` without ever calling `enable_testing()` — that lives in `modules/CMakeLists.txt:58`, one level up — so `ctest` in a standalone `modules/common` build reports that no tests were found. Pre-existing, unrelated to this change, and not worth a row: the parent build is the one that runs it. I verified the argument instead of the harness.

**Learned:**

- **A "not verified" line in a commit message is an assignment, and the box it is addressed to may never read it.** This one named Windows and Linux explicitly, sat for a day, and was found only because a mac run tripped over the branch while tidying. The verification cost twenty minutes once someone looked.
- **Count the callers before moving a path.** The split moved a directory and updated one of three consumers. Nothing catches that — `ctest` is not in the workflow that was edited, and the workflow is not in the build that runs `ctest`. Grepping the moved path across the tree is one command and it is the whole check.
- **A resolver needs its wrong branch tested, not its right one.** "Root resolves to `windows-linux` and passes" is also what a resolver that ignores its argument entirely would print. Pointing it at `macos` and watching five scenes fail is what separates those.
- **Byte-identity to a source is worth asserting mechanically.** Twenty images that "look right" and twenty images hashed against the two commits they came from are different claims, and only the second survives someone asking where a picture came from six weeks later.
- **A branch can be stranded because of what it contains, not because someone forgot.** This one holds a `.github/workflows/**` edit, so no scheduled run could ever have rebased or merged it. Reading the credential's limits explains a stall that otherwise looks like carelessness.

**STEP 4 bookkeeping, all on verified PR state rather than memory:**

- **E9** IN-REVIEW → DONE ([#347](https://github.com/JeffMcClintock/TideSynth/pull/347) merged).
- **A34** IN-REVIEW → DONE ([#338](https://github.com/JeffMcClintock/TideSynth/pull/338) merged).
- **S41** IN-REVIEW → DONE ([#327](https://github.com/JeffMcClintock/TideSynth/pull/327) and [#315](https://github.com/JeffMcClintock/TideSynth/pull/315) merged).
- **E10 was deliberately NOT flipped.** Its TideSynth PR [#346](https://github.com/JeffMcClintock/TideSynth/pull/346) merged but [SynthEditLib#35](https://github.com/JeffMcClintock/SynthEditLib/pull/35) is still open, and IN-REVIEW means *every* linked PR.
- The `win` cell's own instruction — check S22's PR state — was followed: [#344](https://github.com/JeffMcClintock/TideSynth/pull/344) merged and the row already read DONE.

**Next:**

1. **[#291](https://github.com/JeffMcClintock/TideSynth/issues/291) closes when this merges** — its remedy is exactly this PR, and all three render jobs are green on the branch. The issue is `platform:linux`-labelled and was never only Linux's; Windows fails five of the same ten scenes. Commented there with the numbers rather than closing it, since it is Jeff's issue and the fix has not landed yet.
2. **Any future intended look change now has to be re-approved in BOTH sets**, on a machine of each family, or the platform that was not re-baked goes red. The README says so; nothing enforces it, and that is the obvious next defect this arrangement can produce.
3. **`origin/tide/mac/S27-render-ci` wants deleting by a human** once this merges, along with the mac-box worktree registered against it.
4. **The `any` queue now has no ungated row at all.** Both cells say so; the next scheduled run on any box should expect to find nothing takeable and stop rather than invent work.

**Machine left clean.** All work in a throwaway worktree and build tree under the
session scratchpad; nothing was built in `C:\SE\TideSynth`. **Two pre-existing
things on this box were left alone, both predating this run:** `C:\SE\TideSynth`
has a modified `tools/tidepanel-screenshot.synthedit` — real content, not CRLF
churn (`git diff --ignore-all-space` shows the `PanelLocationZoom` and
`panelRect` values changing), so it is the developer's work in progress; and a
registered worktree at `C:\SE\wt345` on `tide/linux/S37-clap-collision`, clean,
whose PR [#345](https://github.com/JeffMcClintock/TideSynth/pull/345) has merged
and whose branch is gone from origin. Neither is this run's. `SE16`,
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were clean and on their default
branches at the start and were never touched.

**Branch/PR:** `tide/win/S44-s27-reference-split` — [#349](https://github.com/JeffMcClintock/TideSynth/pull/349), TideSynth only.

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
