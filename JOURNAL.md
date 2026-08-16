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

## 2026-08-16 — macos — U1a

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **U1a** — `TideApp::OpenView` now constructs `SE2::ContainerViewPanel`
with `CF_PANEL_VIEW`. **TIDE opens the rack/panel view.** Sixth item this
session, at Jeff's direction.

**Result — the symbol A/B, which is bar (a) and is the whole verifiable claim:**

| | before | after |
|---|---|---|
| `ContainerViewPanel` | **0** | **13** |
| `ContainerViewStruct` | 15 | **0** |
| `ModuleViewPanel` | 25 | 25 |
| bogus name (control) | 0 | 0 |

Release build **BUILD SUCCEEDED, 0 errors**, universal x86_64 + arm64, and P5's
identity strings verified un-regressed in the same binary.

**Learned — the interesting number is the one that went to zero, not the one
that went to thirteen.** `ContainerViewStruct` is now at **0 symbols**: nothing
constructs it, so **the structure view is currently unreachable**. Constraint 1
asks for *two* depths — rack by default, structure view on unlock — so this
change makes the rack default *and* removes the other depth. That is a real
regression against constraint 1 taken as a whole, not a tidy half-step, and I
have written it into **U1b**'s row: that item is now "breadcrumb bar **and** an
unlock path that constructs the structure view", which is more than it was
scoped as an hour ago. **Nobody would have noticed this from the diff** — it
only shows up because the audit had established the before-numbers.

**Learned — it was four files, and typing the interface to the base is what
makes it the last time.** `ISeApp` was typed to the concrete
`ContainerViewStruct`, threaded through `TideApp.h`, `TideAppWrapper.h` and
`SynthEditGui.cpp`. It is now `SE2::TopView`. Every member `SynthEditGui.cpp`
calls on the view — `arrange`, `Presenter`, `getCenter`, `DragNewModule`, the
scrollbar callbacks — is a base member, checked before the change rather than
discovered by the compiler. **The one thing the compiler did catch** was a
forward declaration: `TideAppWrapper.h` declared `class ContainerViewStruct;`
rather than including anything, so the first build failed with 17 errors that
all cascaded from `no type named 'TopView' in namespace 'SE2'`. One line.

**What is NOT done, and it is bar (b) of this row's own Accept.** *"Draws
something sane in a host"* — **the plug-in has not been loaded in a DAW.** The
class links and the binary builds; whether the rack renders, renders blank, or
crashes is unknown. U1a's row warned that a crash here would be information
rather than failure; that warning is still unspent, because nobody has looked.

**Next — and the most useful next action is not a backlog item.** **Load the
rebuilt `TIDE.gmpi` in a DAW.** One observation closes U1a's bar (b), closes
P5's outstanding "REAPER shows TIDE Rack" check, and unblocks U1b and U1c, which
should *stay* blocked until then — taking either now means building on a view
nobody has seen render. The mac NEXT row therefore points at **P10** (minutes,
ALLOWED, deletes the dead `SynthEdit.xml` that nearly caused a no-op PR during
P5) rather than at more rack work.

**STEP 1 / 1.5:** no `platform:mac` issues. [SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)
is this run's own and is the only open PR.

**Side effects on this box:** `SynthEdit/build/` rebuilt again (Release,
target `TIDE`). Committed in two repos: `SynthEdit` (the change) and `TideSynth`
(this entry and the backlog). `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were
read only and left clean.

**Branch/PR:** [SynthEdit#25](https://github.com/JeffMcClintock/SynthEdit/pull/25)
— **that one carries the change**; the TideSynth PR is bookkeeping and they need
not merge together.

---

## 2026-08-16 — macos — U1

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **U1** — the fresh audit its own row demanded before anything else,
[docs/u1-rack-mode-audit.md](docs/u1-rack-mode-audit.md), and split the build
work into **U1a/U1b/U1c**. Fifth item this session, at Jeff's direction; a
scheduled run still takes exactly one. Also flipped **P5** to DONE after
watching both its PRs merge.

**The headline, and it changes what rack mode costs: it is not started, and it
is also not a from-scratch build.**

`TideApp::OpenView` still constructs `ContainerViewStruct` with
`CF_STRUCTURE_VIEW` (`TideApp.cpp:63`) — no branch, no setting, no second path.
The 2026-08-13 pivot changed PLAN and has not yet changed a line of TIDE. **But
a top-level panel renderer already exists in the public repo TIDE links:**
`SE2::ContainerViewPanel : public TopView`,
`SynthEditLib/modules/se_sdk3_hosting/ContainerView.h:15` — same base class as
the structure view, with `CF_PANEL_VIEW` a first-class type through
`MfcDocPresenter` and `CUG`, not a stub.

**Learned — "the class exists" and "the class ships" are different claims, and
only one was true. Measured with both controls.**

| binary | `ContainerViewPanel` | `ContainerViewStruct` | `ModuleViewPanel` | bogus name |
|---|---|---|---|---|
| shipped `TIDE.gmpi` | **0** | 15 | 25 | 0 |
| `ContainerView.o` in `libSynthEditLib.a` | **13** | — | — | — |

`ContainerView.cpp` is on `SynthEditLib/CMakeLists.txt:535`, so it compiles;
nothing in TIDE references `ContainerViewPanel`, so **the linker never extracts
that archive member**. Exactly the static-library behaviour C12e documented for
`Dialogs_editor2.obj`. Without the positive and negative controls the zero would
have been indistinguishable from a bad `nm` invocation, which is the whole
reason both are in the table.

**The single most useful number is `ModuleViewPanel` = 25.** The *per-module*
panel renderer already ships in TIDE; only the *top-level container* one does
not. That asymmetry is what turns rack mode from "write a renderer" into "wire
up the one that exists", and it is why U1a is scoped as one line in one ALLOWED
file rather than a project.

**Learned — two of the P2-era findings survive the pivot, and the visual ones
could not be re-measured.** No breadcrumb bar; properties and module browsers
constructible (`TideApp.cpp:79,88`) but unplaced — both still true. The canvas
offset and the dead strip down the right, from
[state-of-the-prototype.md](docs/state-of-the-prototype.md) §6, **need a running
host and this audit did not run one.** Said so in the doc rather than repeating
them as if re-checked. That is the honest limit of a source-and-symbols audit.

**Split into three, in dependency order, and deliberately not folded together:**
**U1a** switches the view (one ALLOWED file, two acceptance bars — links, then
draws); **U1b** the breadcrumb bar, which [about-pane.md](docs/about-pane.md)
now depends on since it is the only chrome the about pane can hang from; **U1c**
rack styling and snapping, **the only part that is genuinely unwritten**. U1a's
result decides whether U1c is styling or a build, so costing U1c now would be
guessing — its row says so instead of carrying a number.

**U1a's row carries a warning worth repeating here:** nothing has ever linked
`ContainerViewPanel` in TIDE, so a crash or a blank canvas on first render is
**information, not the row failing**. File it and keep going.

**STEP 1 / 1.5:** no `platform:mac` issues; no open PRs at the start of this
item — P5's two had just merged.

**Next:** **U1a**, and it is mac-shaped for the same reason D1 was: its second
acceptance bar is "draws something sane in a host", which this box can do.
**P10** is the cheap fallback.

**Side effects on this box:** none — this item read source and ran `nm` on
binaries already built earlier in the session. TideSynth was the only repo
committed in; `SynthEdit`, `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were
read only and left clean on their default branches.

**Branch/PR:** the TideSynth PR carrying this entry, the audit and the split.

---

## 2026-08-16 — macos — P5

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **P5** — the plug-in now calls itself **TIDE Rack**, vendor **TIDE
Synth**. Fourth item this session, at Jeff's direction; a scheduled run still
takes exactly one. Filed **P10**.

**The item contained a trap that would have produced a no-op PR, and finding it
is the main thing to hand on.** The obvious target is
`SynthEditSem/SynthEdit.xml` — it is 12 lines, it is named after the plug-in, and
it holds exactly the `id`/`name` attributes P5 is about. **It is not what
ships.** The live identity is an embedded XML **string literal** in
`SynthEditSem/SynthEdit.cpp`'s `getPluginInformation()`, which is what the
wrapper actually parses. `SynthEdit.xml` is referenced only by `SynthEdit.rc`
(`IDR_GMPXML1 GMPXML`), and the only loader for that resource is inside
**`#if 0`**, whose non-Windows branch literally reads *"not needed for built-in
XML #error implement this for mac"*. I had already edited the wrong file before
tracing far enough to notice. Filed as **P10**; both are updated in step
meanwhile so they cannot drift.

**The `id` question the previous entry flagged is now answered, by reading the
code rather than by reasoning about VST3 in general.** The class UID is
**hashed from the id string** — `textIdtoUuid(plugin->id, ...)` at
`GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:244-245`, feeding
`info->cid`. So renaming `SE SynthEdit` *would* change the plug-in's identity and
orphan every host project that had loaded TIDE. It stays. Users never see it,
and the caution was justified.

**The vendor half explains the original symptom exactly.** Omitting the `vendor`
attribute defaults `vendorName` to `"GMPI"` —
`GMPI/Hosting/xml_spec_reader.cpp:532-535` — which is precisely why REAPER
listed this as *"SynthEdit (GMPI)"*. So P5 was two fields, not one, and the
second one was invisible until the default was read.

**Result — A/B on the built binary, which is the artifact.**

| | `<Plugin …>` in the binary | `"TIDE Rack"` | `name="SynthEdit"` |
|---|---|---|---|
| before (Aug 8 build) | `id="SE SynthEdit" name="SynthEdit"` | **0** | present |
| after (this build) | `id="SE SynthEdit" name="TIDE Rack" vendor="TIDE Synth"` | 2 | **0** |

`id="SE SynthEdit"` still present in both. Release build of target `TIDE`:
**BUILD SUCCEEDED, 0 errors**, universal **x86_64 + arm64**.

**Learned — the mac build tree was stale in a way that looks like your own
breakage, and the fix is one command.** The first build failed with
`Build input file cannot be found: SynthEdit2/plug4.cpp`. That file has not
existed since the carve-out moved it; `EditorLib/CMakeLists.txt:120` correctly
says `${SYNTHEDITLIB_DIR}/plug4.cpp`. The **generated Xcode project** was stale
— Xcode's `ZERO_CHECK` did not regenerate it. `cmake .` in `build/` fixed it
(0 references to the old path afterwards, 4 to the right one) and the build then
succeeded. **Any mac run touching this tree after a carve-out stage should
expect this and re-run `cmake .` before believing a build failure is theirs.**

**Learned — `getVendor4charCode()` is unaffected, checked rather than assumed.**
`SanitizeVendor4charCode(code, vendorName)` regenerates the four-character code
*from the vendor name* only when the code is not 4 alphanumeric characters with
a capital. `TideApp::getVendor4charCode()` returns `"TIDE"`, which passes, so
changing `vendorName` from "GMPI" to "TIDE Synth" does **not** reach it. That
mattered because the four-char code is plug-in identity too, in the AU/preset
sense, and silently changing it would have been the same class of bug as
renaming `id`.

**What is NOT verified, and it is the row's own acceptance evidence.** P5's
original finding was REAPER listing `VST3i: SynthEdit (GMPI)` and
`TrackFX_AddByName(tr, "TIDE_VST3", ...)` returning -1. **The rebuilt plug-in
has not been loaded in a host.** The binary carries the right strings and that
is strong, but "REAPER shows TIDE Rack" is unconfirmed. Marked as such in the
row rather than claimed.

**STEP 1 / 1.5:** no `platform:mac` issues; no open PRs at the start of this
item — the earlier stack (#69/#70/#71/#72/#73) had all merged.

**Next:** **U1** for the mac box — the rack-mode UX audit, which
[docs/about-pane.md](docs/about-pane.md) now depends on, and which a macOS box
can actually drive. **P10** is the cheap fallback. Whoever loads TIDE in a host
should close P5's last gap while they are there.

**Side effects on this box:** `SynthEdit/build/` was **regenerated and rebuilt**
(`cmake .` + Release build of `TIDE`) — that tree was already stale and is now
correct, but it is Jeff's build tree and the rebuild took a few minutes of CPU.
`SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were read only and left clean.
Committed in **two** repos this time: `SynthEdit` (the code) and `TideSynth`
(this entry and the backlog).

**Branch/PR:** [SynthEdit#24](https://github.com/JeffMcClintock/SynthEdit/pull/24)
+ TideSynth PR — **the SynthEdit one carries the actual fix**; the TideSynth one
is bookkeeping and they do not have to merge together.

---

## 2026-08-16 — macos — A9

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **A9** — the community research routine,
[scripts/community-research.py](scripts/community-research.py), with
[docs/community-research.md](docs/community-research.md) and a human-editable
[rejection memory](docs/community-research-rejected.md). **Third item this
session, at Jeff's direction** ("keep working"); a scheduled run still takes
exactly one. Also flipped **D5** to DONE — Jeff updated the Ko-fi page mid-session
and it is verified below.

**The guardrails are structural rather than policy**, which is the part worth
keeping. A9 lists them as hard rules, so `_get()` is the only network call in
the file and can only issue GET: **there is no write path to disable.** Posting,
voting or DMing would require adding the capability first, which is the point at
which a human says no. Courtesy rate 1.5s process-wide, identifying User-Agent,
https-only, and the script **prints** — it cannot edit `BACKLOG.md`.

**Result — three things the first LIVE run found that no selftest would have.**
This is the entry's real content, because all three looked fine in isolation.

1. **It auto-rejected a real Surge XT crash report.** *"Surge XT CLAP crashes
   REAPER on load (SIGSEGV in JUCE repaint)"* was dropped under "constraint 2 —
   the DAW owns I/O", because somewhere in the reporter's diagnostics was the
   phrase *"The standalone app also works fine"*. An incidental mention in a bug
   report is not a feature request, and **discarding crash reports is the worst
   thing this filter could do.** Constraint rules now read the **title** only;
   the hypothesis flag still reads the body, because its failure mode is the
   opposite and much cheaper. Both pinned by selftest cases built from **the
   real incident**, not an invented example.
2. **Output was sorted ascending by date**, burying 2026 items under 2024 ones.
   Worth naming because **it looked like a fetch bug and was not** — I checked
   the raw API response and it returns newest-first; the defect was entirely in
   my display sort. Ranking is now hypothesis-first, then engagement, then
   recency, all descending.
3. **A passive scan cannot serve the standing hypothesis at all.** Across **48
   real items** (30 forum topics + 18 Cardinal issues) the hypothesis filter
   matched **zero** — an iPad thread appears on that forum roughly once a year.
   So a "watch" built on reading the newest topics would have looked like a
   working watch that had simply found nothing, which is the exact failure shape
   this project keeps hitting.

**Learned — the fix for (3) is that the watch has to SEARCH, and searching needs
one more correction than it looks like.** A `watch` source now queries Discourse
search directly and finds the signal on the first call: *"VCV Rack for iPad -
2025?"*, *"VCV Rack on iOS/Android devices?"*, *"How are you connecting/using
VCV with an iPad?"*. But Discourse search matches **post bodies**, so the top
hits were the forum's megathreads — *"What are you listening to?"* (6,135
replies) and *"Member Introductions"* — which merely contain "iPad" somewhere
across thousands of posts. **Requiring the match in the topic TITLE cut 54 hits
to 17, all on-topic**, and watch items rank by recency rather than engagement,
because the hypothesis is about someone moving into the gap *now*.

**Verified:** selftest **17/17** (offline); a live run across all five sources;
and the rejection memory proven by **A/B** rather than by reading the code —
with `surge#7782` listed a run reports `1 already rejected before · 24
proposed`, and with the file removed the same run reports `0 · 25` and the item
reappears.

**Measured in passing, and it corrects a doc:** the VCV ecosystem is **553
plugins and 4,958 modules**.
[docs/process-review-2026-08-09.md](docs/process-review-2026-08-09.md) describes
it as *"the 8,000+ module ecosystem"* — roughly 1.6× over. Not edited there, since
that document is a dated record of a review; the correction lives in A9's row and
in the new doc.

**The limitation I did not paper over.** Ranking is engagement, which is a proxy
for *worth a glance*, not for relevance to TIDE — so other projects' housekeeping
("Do a windows arm64ec build", "Release checklist for Surge XT 1.4") still
reaches the output. **The routine filters what TIDE has ruled out; it does not
judge what TIDE needs, and it should not pretend to.** Triage stays human, which
is what A9's PROPOSED-only design asks for anyway. A relevance signal is the next
real improvement and wants thought rather than more regexes. Stated at the top of
the doc's limitations section, not buried.

**D5 — DONE, and verified rather than taken on trust.** Jeff updated the Ko-fi
page during the session. It now renders as **"Jef [TIDE Rack]"** with the title
*"Buy Jef [TIDE Rack] a Coffee"*; an hour earlier it was plain *"Jef"* with
nothing naming the product. Checked in a real browser, which is the only way —
this session established that Ko-fi 403s unfamiliar user-agents **even for
handles that do not exist**, so `curl` cannot answer the question. The website's
*"Donate to TIDE Rack on Ko-fi"* link now lands somewhere that agrees with its
own link text. Flipped on the D2 branch, because D5 is defined there and does not
exist on `main` yet.

**STEP 1 / 1.5:** no `platform:mac` issues. [#69](https://github.com/JeffMcClintock/TideSynth/pull/69)
is open and is this session's own — `lint` green, and its three red build checks
are the pre-existing **B1** condition (all three die at Configure because
TideSynth has no top-level `CMakeLists.txt`), so it is waiting for merge rather
than for work. This branch is **stacked on it**, as the 61→64 stack was.

**A concurrent run exists, and I found it late — say so plainly, per the prompt.**
[#70](https://github.com/JeffMcClintock/TideSynth/pull/70)
(`tide/win/C11-S10-rulings`, also `tide-rack-bot`, opened 00:42Z) was created
*after* I opened #69, and I only noticed it because it took the PR number I had
predicted. It touches `BACKLOG.md`, `JOURNAL.md` and `JOURNAL-2026-08.md` — the
same three coordination files every run edits — so **conflicts with this stack
are expected, and merge order matters.** It is a different item set (C11, S10,
S9, M2, A16), not a duplicate claim, so nothing was wasted.

**It does supersede one thing I wrote above and in the NEXT row**: #70 rules
**S9 → WONTFIX** and **S10 → IN-REVIEW** (retire, not revive), and rescopes
**M2**. My screening said the remaining `mac`-only rows were "GATED or Jeff's",
naming S9 and S10 — that conclusion still holds (neither is available work), but
the *reason* for S9/S10 changes once #70 lands. **I did not rebase this stack
onto #70.** The prompt says to make your branch a delta on top of theirs when you
collide, and that is written for colliding on the same *item*; here the overlap
is only the shared coordination files, which is the ordinary condition for every
run. Rebasing three stacked branches onto a fourth unmerged one would make all of
them depend on #70 merging first, for no gain. Flagged on the PR instead so
whoever merges sequences it.

**Next:** **P5** for the mac box. Its scope got cleaner this session without
anyone editing it: [docs/about-pane.md](docs/about-pane.md) now says the about
pane is a *third* surface that does **not** change the host-visible plug-in name
or the vendor string, so P5 owns exactly two fields. **Whoever next runs the
research routine should read its limitations section first** — the output is a
proposal list, and treating it as a to-do list is the way this becomes noise.

**Side effects on this box:** none outside the scratchpad. TideSynth was the only
repo committed in; `SynthEdit` was read only, and `SynthEditLib`, `gmpi_ui` and
`GMPI_Wrappers` were untouched. The routine made read-only GET requests to
community.vcvrack.com, api.github.com and raw.githubusercontent.com; **nothing
was posted, voted on, or logged into.**

**Learned — do not predict your own PR number, even to avoid a placeholder.** The previous entry's fix for the auto-merge race was to finish STEP 4 *before* opening the PR, which means writing the number before it exists. I wrote #70; GitHub issued **#71**. Predicting is the same defect as a placeholder wearing a plausible disguise — and worse, because a wrong-but-real number links to someone else's PR rather than looking obviously unfinished. **The rule that actually works: push STEP 4 first, open the PR, then correct the number in a follow-up commit on the same branch.** Safe whenever the PR cannot auto-merge before you get there, which is any PR touching `scripts/` or `website/`.

**Branch/PR:** [TideSynth#71](https://github.com/JeffMcClintock/TideSynth/pull/71),
stacked on [#69](https://github.com/JeffMcClintock/TideSynth/pull/69) and
retargeting to `main` as its parent merges.

---

## 2026-08-16 — macos — D2

**Prompt:** `b3e9876` · claude-opus-5[1m] · Claude Code 2.1.229 · as `tide-rack-bot`

**Did:** **D2** — placed the *"TIDE Synth — by SynthEdit Ltd"* credit. **Second
item this session, at Jeff's direction** ("do the next task"); a scheduled run
still takes exactly one. Both halves the row named: the website footer, which is
real code, and the plugin-side placement, which is a spec only.

**The design decision worth recording is that D1 and D2 were about to get two
different answers to one question.** Both need "a subtle surface that is not a
dialog", and D1's note had already argued one into existence. So this run wrote
[docs/about-pane.md](docs/about-pane.md) as the *single* answer — **one about
pane off the breadcrumb bar holding exactly four things** (version, credit,
donation, licence). The fixed list is the point: an about pane is where things
get *put*, so it is precisely the surface that accretes until it becomes the
splash screen PLAN forbids. Adding a fifth item now needs a ruling.

**Result — website half.**

| check | result |
|---|---|
| footer wording | **"TIDE Synth — by SynthEdit Ltd"** — R1(a) verbatim |
| tag balance | OK |
| subresource tags (`script`/`img`/`link`/`iframe`/…) | **zero** — the page's "no external requests" promise is intact |
| `https://www.synthedit.com/` | **200, no redirect** |
| live outbound links | 4, all real |

The credit is a plain `<a href>`, which is the same settled arrangement as the
existing ko-fi and github links — an outbound href loads nothing, so linking
does not cost the page its zero-request property.

**Learned — a raw grep of `website/index.html` finds `#donate-url-tbd`, and it
is a false alarm both README and page comments will make you doubt.** That
string is the placeholder the W1 history says was removed; it survives **only
inside an HTML comment** describing the old mistake. Stripping comments before
counting shows **0** occurrences in live markup. Anyone auditing that file
should strip comments first — the file is more comment than markup by volume,
deliberately, so raw greps mislead in both directions.

**Learned — `curl` cannot tell you whether a Ko-fi handle exists.** Ko-fi
returns **403 to any user-agent it dislikes, including for handles that plainly
do not exist** — checked with a deliberately nonsense handle as a control, which
also returned 403. So a status-code check proves nothing, and the website
README's standing rule ("confirm the URL resolves before committing it") needs a
real browser to satisfy. Done that way here.

**Two things found by actually opening it, neither of which a code reading would
have surfaced:**

- **The Ko-fi page does not identify itself as TIDE Rack's.**
  <https://ko-fi.com/tiderack> renders as *"Buy Jef a Coffee"*, display name
  **"Jef"**, bio *"I'm a dude in New Zealand"* — nothing naming TIDE Rack, TIDE
  Synth or SynthEdit. **This defeats D2's own justification one hop later**: the
  website's link text is *"Donate to TIDE Rack on Ko-fi"*, so a user meets
  exactly the unexplained-identity surprise the credit exists to prevent. Filed
  as **D5**, `NEEDS-JEFF` — it is account settings and needs the password.
- **`ko-fi.com/TideRack` (website) and `ko-fi.com/tiderack` (Wayland code,
  `WaylandMainWindow.cpp:50`) reach the same page** — Ko-fi canonicalises to
  lowercase. The inconsistency is harmless; recorded so nobody "fixes" one of
  them and re-checks this.

**Prior art reused rather than reinvented:** `WaylandMainWindow::showAbout()`
(`:953-959`) already puts version, company and donation URL in one place as
plain text. TIDE takes the **content** and not the **container** — that one is
`SeMessageBoxAsync`, a modal dialog, which constraint 5 rules out. The spec says
so explicitly, because the temptation on implementation day will be to copy the
function.

**Sequencing — I applied last run's lesson and it worked.** The previous entry
learned that A4's auto-merge can merge a PR mid-STEP-4, and the fix was to
finish STEP 4 and push it **before** opening the PR. Done that way here: this
entry, the backlog and the D5 row were all committed and pushed first, so the PR
was complete the moment it existed. **No placeholder was ever pushed** — the
`__D2PR__` substitution happened before the first push, not after it.

**This PR will not auto-merge, and that is correct.** It touches `website/**`,
which A4's allowlist deliberately excludes because **a merge there IS a
production deploy of tidesynth.com**. So it waits for Jeff, unlike the last two.

**STEP 1 / 1.5:** re-checked at the start of this item — no `platform:mac`
issues, no open PRs in any of the five repos (both of this session's earlier PRs
had already auto-merged).

**Next:** **A9** for the mac box — the D-series is now exhausted for a scheduled
run (D1/D2 IN-REVIEW; D3/D4 GATED in `SE16/EditorLib/CMakeLists.txt`; D5 needs
Jeff's Ko-fi password). A9 is `any`, unblocked, PROPOSED-output-only, and should
be budgeted as a design session first. **D5 is small and worth doing before
v0.1 links that page from inside the plugin as well as from the website.**

**Side effects on this box:** none outside the scratchpad. TideSynth was the
only repo committed in; `SynthEdit` was read (`WaylandMainWindow.cpp`) and not
written, and `SynthEditLib`, `gmpi_ui` and `GMPI_Wrappers` were untouched — all
four confirmed clean and on their default branches. Two public pages were
fetched read-only (ko-fi.com, synthedit.com); nothing was posted, and no account
was logged into.

**Branch/PR:** [TideSynth#69](https://github.com/JeffMcClintock/TideSynth/pull/69).
