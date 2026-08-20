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

## 2026-08-20 — linux — A30: the lessons digest, and why the literal spec would have backfired

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Third item this session**, continued at Jeff's direction.

**Did:** built [docs/lessons.md](docs/lessons.md) — one line per lesson, every
run reads it, and **rotation cannot age it out** because
[scripts/extract-lessons.py](scripts/extract-lessons.py) generates it from
`JOURNAL.md` **and** `JOURNAL-2026-08.md`. Rotation gained a step 4 that
regenerates it, and the run prompt's reading list gained one line — that half is
the one A30 said wants Jeff's eye, and it is why this is a PR.

**Result — Accept met:** **599 lessons from 152 entries, 0 dropped**; the digest
is **56 KB against the 223 KB of Learned sections it distils, 4.1x smaller**.
`--check` fails when it is stale, so it cannot drift from the journals.

### The literal scope would have made things worse, and that is the finding

A30 says *"a standing, append-only digest that rotation copies each entry's
Learned bullets into"*. Copied verbatim that is **223 KB today — 24% of the
entire corpus** — added to what every run already reads. That is **worse than
the 192 KB that triggered A8 in the first place**, and it is the same trap A24
measured and rejected when it killed the 7-day journal floor.

So I measured before building, the way A24 did:

| approach | size | verdict |
|---|---|---|
| every Learned bullet, verbatim | 223 KB | worse than the problem |
| bold claim only, one line each | **56 KB** | shipped |

**What makes the cheap version work is the journal's own convention**: every
Learned bullet opens with a bold claim and then argues it, so the claim alone is
a real lesson rather than a title. That is a property of how this project already
writes, not something I imposed.

### The bug worth not repeating

The first cut read only markdown list items and reported **72 entries** — and
looked entirely complete while dropping **80 of the 152** entries that have
lessons. Three Learned shapes exist here and only one is a list:

```
1. **Learned:**  followed by "1." / "-" items          72 entries
2. **Learned:**  followed by one prose paragraph    }  the other 80
3. **Learned — headline.** **1. Claim...**          }
```

A second bug in the same pass: the no-bold fallback split on the first `.`, which
truncated inside inline code — `` `PKG_CONFIG_LIBDIR` pointed at a pruned copy of
the system ` `` was a real output line. This journal is full of `.pc` and
`foo.cpp:31`, so sentence-splitting has to ignore dots inside backticks.

**C15 — I reached the same finding as the macOS box, independently and hours later.** I checked C15 before starting (it was the topmost eligible row), found C16 had already closed it, and flipped it to DONE here. **[#202](https://github.com/JeffMcClintock/TideSynth/pull/202) landed the same conclusion first**, archived the row into `BACKLOG-DONE.md`, and filed **A31** for the underlying gap — *two ids, one job*, which A23's duplicate-id check cannot see by construction. **My duplicate edit is dropped from this branch**, per STEP 2's rule for a collision found after opening a PR; this branch is now a delta on top of theirs. The verification is not wasted, because it was done on merged `main` and agrees with theirs clause for clause: the only remaining includes resolve in the **public** `SynthEditLib`, `../SynthEdit2` survives only in comments, and the three `SynthEditApp` symbols appear only in comments. **Worth noting for A31:** two boxes spent a session each re-deriving one answer, and neither could see the other's row — the same shape as the collision itself.

**Learned:**

1. **A spec that says "copy X into a file every run reads" is a size decision in
   disguise, and it should be measured before it is implemented.** A30's own
   scope, followed literally, would have recreated A8 — the row that exists
   because this journal already grew past what runs could afford once.
2. **A generated index that silently covers half its input looks exactly like one
   that covers all of it.** The count came out plausible (72 entries, 306
   lessons) and nothing was obviously missing; only asking "of the entries that
   *have* lessons, how many are represented?" exposed it. Assert coverage against
   the source, not against your own output.
3. **Sentence-splitting on "." is wrong in any corpus that names files.** Dots
   inside backticks are the common case here, not the exception.
4. **This project's writing conventions are load-bearing infrastructure.** The
   digest is only affordable because Learned bullets already lead with a bold
   claim. A convention nobody enforced turned out to be the thing that made a
   mechanical distillation possible.

**Next:**

1. **The digest grows ~3.5 KB/day, ~100 KB/month** — 56 KB is affordable now and
   will not be by October. The lever is dropping the archive from the script's
   `SOURCES` once its lessons are spent, which halves it at a stroke. That is a
   judgement call and belongs to Jeff, not to a run.
2. **Nothing enforces that the digest is regenerated.** Rotation step 4 says to,
   and `--check` will catch a stale file, but no lint step runs it — adding one
   to `lint.yml` is a `.github/workflows/**` edit the token cannot push.
3. Unchanged: **the `apt-get` in `build.yml` is still the only thing between C7e
   and closed**, and CI filed a fifth issue (#195) for it while this ran.

**Branch/PR:** `tide/linux/A30-lessons-digest` — TideSynth only.

---

---

---

## 2026-08-20 — linux — S21 verified at runtime, and three things I got wrong

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Fifth item this session**, at Jeff's direction. It exists because he pushed back
on a claim I had repeated four times without rechecking.

### 1. The wayland claim was false, and I had the right data all along

I wrote, in the S21 entry and in two PR bodies, that `TIDE_STANDALONE`
*"cannot build on this box (wayland-client, xkbcommon, libdecor-0,
libpipewire-0.3 all absent)"*. Jeff: *"I have run synthedit wayland here, why no
wayland deps?"*

Measured:

| dep | this box |
|---|---|
| `wayland-client` | **1.22.0** |
| `xkbcommon` | **1.6.0** |
| `libdecor-0` | **0.2.2** |
| `wayland-scanner`, `wayland-protocols` | **present** |
| `libpipewire-0.3` | **missing — the only one** |

A normal configure says so in one line: `STANDALONE skipped -- missing:
libpipewire-0.3`. **That list of four was the CI runner's**, read out of the tree
I had deliberately crippled with a pruned `PKG_CONFIG_LIBDIR` to reproduce #190,
and then repeated as a fact about this machine.

**The failure was not a missing measurement.** My first probe this session
measured all three wayland modules as present. I overwrote correct data with a
number from a different experiment, and then used it to justify *not* verifying
two separate items — S21 shipped with "the plugin was never loaded", and I ruled
E14 not-takeable-here on the same premise.

### 2. So S21 is now verified by the reader, not by a directory listing

`sudo` needs a password no unattended run has, so rather than change the box:
`apt-get download libpipewire-0.3-dev libspa-0.2-dev`, `dpkg-deb -x` into a
scratch prefix, repoint `prefix=` in the two `.pc` files, point
`PKG_CONFIG_PATH` at them, and symlink `libpipewire-0.3.so` at the already
installed runtime `.so.0`. **TIDE_STANDALONE then builds and runs on Linux —
the first time it has.**

**Deterministic A/B, 3 runs each layout:**

```
post-fix : seeded=1  enriched=5  missing=0
pre-fix  : seeded=0  enriched=0  missing=6
```

With the fix: `TIDE: 6 rack prefab(s) seeded from the bundle`, and all five pin
XMLs enriched. Without: six `missing from bundle resources` lines, including
*"no Prefabs folder in bundle resources - the rack module browser will be
empty"*.

**And visually** — [before](docs/images/s21-prefabs-linux-before.png) /
[after](docs/images/s21-prefabs-linux-after.png). With the fix the browser's
**Prefabs** group lists Envelope, Filter, Midi, MidiCv, Oscillator, Output.
Without it **the Prefabs group is absent from the tree altogether**, and
`Sub-Controls` collapses from 27 classes to one (`Label`) because the XMLs never
load — the second half of S21, which I had only ever inferred.

### 3. A crash I reported before I had measured it

The first pre-fix run segfaulted, and I wrote *"the pre-fix layout segfaults"*.
**It does not.** 28 controlled runs — 8 per layout at 6s, 6 per layout at 12s,
both layouts — produced **zero** crashes. Two crashes were real, both on the
first run after relocating the resource directory with a 10s window, but the
correlation with the layout is unsupported. Filed as **S23** with what is and is
not known, unattached to S21. `gdb` is not installed here, so nothing names a
frame.

### 4. A merge commit of mine is authored as Jeff, and is already pushed

`72cc7c7` on `tide/linux/A30-lessons-digest` — I dropped the `GIT_*` exports on
the shell call that ran `git commit`, so the merge is stamped
`Jeff McClintock <jef@synthedit.com>`. `check-commit-authorship.py` reports it
and exits 0, by A26's design, because STEP 4 forbids rewriting anything already
pushed. **I have not rewritten it.** It is metadata rather than privilege — the
push authenticated as the bot, and authorship does not bypass a ruleset — but it
is exactly the misattribution A14 exists to prevent, and it is Jeff's call
whether to force-push a re-authored commit.

**Learned:**

1. **Data from a deliberately broken environment must be labelled at the moment
   it is written down.** The pruned-`PKG_CONFIG_LIBDIR` tree existed to answer
   "what does the CI runner lack?" — and its answer reads exactly like an answer
   to "what does this box lack?". Nothing in the log distinguishes them.
2. **A claim used to justify NOT doing work deserves more scrutiny than one used
   to justify doing it, not less.** "I can't verify this here" closed two items
   without review. It was wrong, and it was cheap to check.
3. **Report a crash with a rate, or don't report it as a consequence.** One
   observation became "the pre-fix layout segfaults" in the same message. 28 runs
   said otherwise.
4. **Export the identity in every shell that commits, not once per task.** Each
   Bash call is a fresh environment; the exports do not persist, and the failure
   is silent until the check prints it.
5. **`libpipewire-0.3-dev` is all that stands between this box and a working
   `TIDE_STANDALONE`**, and the download-and-extract route needs no root — so
   visual verification IS available on linux, which several rows assume it is
   not.

**Next:**

1. **E14 should be re-examined for this box.** I ruled it out because its
   authoritative check is *"place it in TIDE and look"*; that is now possible
   here, as the screenshots show.
2. **S23** needs `gdb` and a longer run window.
3. **Install `libpipewire-0.3-dev` properly** if the standalone is to be routine
   here — the scratch-prefix workaround is per-build and undocumented outside
   this entry and S23.
4. Unchanged: **the `apt-get` in `build.yml`** is still all that stands between
   C7e and closed; CI has now filed eight of those issues.

**Branch/PR:** `tide/linux/S21-runtime-verification` — TideSynth only.

---
## 2026-08-20 — macos — U2 was finished four days ago, and it was the last mac row

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Seventeenth item this session**, and taken with a concurrent agent running —
so: throwaway worktree only, claim pushed before any work, every change
committed as soon as it was coherent, `check-commit-authorship` and
`check-commit-completeness` on every commit. None of Jeff's trees were touched.

**Did:** closed **U2**, whose Accept had been met on 2026-08-16, and filed **A32**
for the gap that let it sit.

### The row had nothing in it

U2 asked for exactly one deliverable — *"a triage note naming root cause(s) and
the split; per-defect accepts land on the split rows."* Verified rather than
assumed:

- `docs/u2-triage-2026-08-16.md` — **163 lines**, exists
- **all five splits archived DONE with merged PRs**: U2a (gmpi_ui#7), U2b
  (gmpi_ui#8), U2c (SynthEdit#26), U2d (SynthEdit#27), U2e

**Four days stale — and it was the ONLY `mac`-marked row left in the queue.** A
run looking for mac work takes it and finds nothing to do. The failure is silent
and self-concealing: from outside, the queue looks like it has work in it.

### The obvious lint would be red on day one, and I measured that before proposing it

The rule writes itself: *flag any live row all of whose `X[a-z]` splits are
archived*. Run against the real tree it fires on **two** rows:

| row | verdict |
|---|---|
| **U2** | correct — nothing left in it |
| **E2** | **wrong** — a/b/c are done, but E2 is legitimately open; its remaining module stages are simply not filed yet |

It correctly does **not** fire on C7, whose splits include open ones.

**One real, one false: a 50% false-positive rate.** That is the same shape A23
and A27 were each nearly shipped with, and the same shape A24's proposed remedy
had. So A32 asks for an **advisory report** that never sets a non-zero exit,
not a gate.

**E2 is the interesting half of that false positive.** An umbrella with every
child done and more intended is, from the outside, indistinguishable from one
that is finished. That is a row-writing problem, not a lint problem, and A32 says
so.

**Learned:**

1. **Third stale-status row today** — C15 (duplicate), U2 (splits all landed).
   Different causes, one shared consequence: the queue advertises work that does
   not exist. A31 covers the first, A32 the second.
2. **Measuring a proposed lint against the live tree before writing it has now
   paid off four times today** (A23, A27, A24, A32). It is cheap, it is one
   command, and every time it changed the design.

**Next:**

1. **No `mac`-marked rows remain.** M1/M2/M3 and R3 are all BLOCKED; S9 is
   WONTFIX. Mac's queue is `any` rows or nothing.
2. **A31, A32** are the takeable process rows; **C10** wants `SynthEditLib`
   authority.
3. **The carve-out needs one `apt-get`** — [#189](https://github.com/JeffMcClintock/TideSynth/issues/189), Jeff's.

**Branch/PR:** `tide/mac/U2-close` — TideSynth only, backlog and journal.

## 2026-08-20 — macos — C15 was C16: two ids, one job, and a NEXT block pointing three runs at it

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Sixteenth item this session.** Repos synced first; the **linux box is awake
again** and holds A30 ([#198](https://github.com/JeffMcClintock/TideSynth/pull/198))
and S17 ([#200](https://github.com/JeffMcClintock/TideSynth/pull/200)), so
neither was taken here.

**Did:** took C15, found it already delivered, closed it, and filed the process
gap it exposed as **A31**.

### C15 and C16 are the same job under two ids

Every clause of C15's Accept, checked against `main` rather than assumed:

| C15 required | on `main` |
|---|---|
| `TideAppStubs.cpp` includes no private header | **0** hits |
| the three `SynthEditApp` symbols deleted | **0** |
| `SafeMessagebox` + `GetLicenseState` kept | **2** |
| `SynthEditSem`'s `../SynthEdit2` include gone | **0** |

**The windows box filed C15** while landing C14 (`fa75989`, [#177](https://github.com/JeffMcClintock/TideSynth/pull/177)).
**This box filed C16** for the same file, the same three symbols and the same
include-path deletion, hours later while landing C7b (`830c77c`) — from a branch
cut off the same `main`, where C15 was not yet visible.

**A23 solved the neighbouring problem and is blind to this one.** It detects
*one id, two rows*, which is what the two A17s were. This is *two ids, one job*,
and no id-based check can see it. Filed as **A31** with three candidate fixes and
a recommendation: the cheap habit — grep `BACKLOG.md` for the file you are about
to name — over a lint that can only catch rows citing a `path:line`.

**The cost was small only by luck.** C16 happened to land first, so C15 closed in
minutes. Filed the other way round, a run would have spent a session re-doing
finished work and discovered it at merge.

**One thing C15 got right and is worth keeping:** it predicted that C14 would
*not* close this half — *"narrowing a signature cannot remove a definition of
somebody else's member function"*. Correct, and C16 reached the same conclusion
independently by grepping.

### The NEXT block was pointing three rows at it

`check-next-block.py` — A27's own check — flagged the `win` and `any` cells, both
naming C15 as a take-target. **The windows box was about to take work that was
already done.** Without A27 that would have been a whole wasted run.

Re-pointed all three cells. And the *third* instance of a lesson I have now
written down twice and violated twice more in one sitting:

1. the superseded quote in `any` said "then **C15**" — flagged;
2. my replacement said "and then C15" — **flagged again**, because `then` before
   an ID is itself a take-verb;
3. deeper in the same cell, "if linux has it, take **C15**" — flagged a third
   time.

**Preserving NEXT-cell text verbatim is in direct tension with the check**, and
A22 already ruled which way that goes: superseded text loses its imperative. It
took three passes here because the cell is long and the phrases are buried.

**Learned:**

1. **The duplicate-work check that matters is not about ids.** A23 makes id
   collisions visible; nothing makes *job* collisions visible, and the branch
   model guarantees both. A31.
2. **Writing a rule down is not the same as being able to follow it.** I wrote
   A22's "superseded text must lose its imperative", then broke it twice in the
   same edit — once in text I had just written to explain the rule. The check
   caught all three; the habit caught none.

**Next:**

1. **A31** — the process fix, and the only thing this session leaves takeable
   that is not blocked.
2. **The carve-out is down to one workflow edit** ([#189](https://github.com/JeffMcClintock/TideSynth/issues/189)) — everything else in C7 has landed.
3. **U2** is the only `mac`-marked row left.

**Branch/PR:** `tide/mac/C15-duplicate-of-C16` — TideSynth only, backlog and journal.

## 2026-08-20 — linux — S21: the Linux bundle's resources were staged outside it

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Second item this session.** Jeff said "take next task" mid-run, which overrides
STEP 2's one-item rule; recording that here because otherwise the entry looks
like a run that helped itself to a second row.

**Did:** fixed the defect the previous entry filed. `SynthEditSem/CMakeLists.txt`
staged TIDE's bundle resources with `$<TARGET_FILE_DIR:tgt>/../Resources`,
commented *"the binary sits in Contents/<arch>/, so Resources is its sibling"* —
true on Windows, false on Linux, where `gmpi_plugin.cmake:853-861` links a bare
`.so` into the target directory and assembles the bundle around it afterwards.

**Linux needs TWO answers, not one**, and that is the part worth not
re-deriving. `BundleInfo::getBundleContentsFolder` (`BundleInfo.cpp:204`) walks
the loaded module's path for a `Contents` element and falls back to
`parent_path()` when there is none:

| format | loaded from | reader wants |
|---|---|---|
| VST3 | `TIDE_VST3.vst3/Contents/x86_64-linux/TIDE_VST3.so` | `…/Contents/Resources` |
| GMPI | a bare `TIDE.gmpi`, no `Contents` at all | `Resources` beside the binary |

So the old expression was wrong for both, by different amounts.

**The fix is a new `elseif(UNIX)` branch. The Windows expression is unchanged,
byte for byte, and that was the point** — S21 was found on Linux and Windows is
not verifiable from here, so it keeps its own branch rather than being retuned
by someone who cannot build it.

**Result:** fresh clone of the branch, full Release build — configure rc=0,
build rc=0, **zero error lines**. Accept clause met exactly as written:

```
TIDE_VST3.vst3/Contents/Resources/Prefabs/  6 .synthedit   + 5 XMLs beside it
build/SynthEditSem/Resources/Prefabs/       6 .synthedit   + 5 XMLs  (TIDE.gmpi)
build/Resources/                            NO LONGER EXISTS  (negative control)
```

**Verification artifact:** [tests/s21_bundle_resources_probe.py](tests/s21_bundle_resources_probe.py)
replicates the reader's own path algorithm and answers from its point of view,
so the check is not "a human looked at a directory listing". Run as an A/B with
a positive control:

```
pre-fix tree (main)  -> RESULT: FAIL   both formats, "EMPTY BROWSER"
post-fix tree        -> RESULT: PASS   both formats, prefabs=6 xml=5
```

The positive control is load-bearing here: a probe that only ever passes would
have looked identical and proved nothing.

**What this does NOT claim.** The plugin was never loaded. `TIDE_STANDALONE`
cannot build on this box (wayland-client, xkbcommon, libdecor-0, libpipewire-0.3
all absent) and there is no DAW here, so the claim is that the reader's computed
path now contains the files — not that a running TIDE listed six prefabs in its
browser. Someone on mac or windows can close that gap in seconds.

**Learned:**

1. **`$<TARGET_FILE_DIR>` is not inside the bundle on Linux**, unlike macOS where
   `$<TARGET_BUNDLE_CONTENT_DIR>` exists precisely because the linker writes into
   the bundle. Any "resources go next to the binary" reasoning has to ask which
   binary — the linker's output or the copy the bundle step made.
2. **A silent cross-repo disagreement needs a test written from ONE side.** The
   writer is in TideSynth and the reader is in SynthEditLib; each is internally
   consistent, which is why this survived. The probe deliberately encodes only
   the reader's algorithm.
3. **CI would not have caught this and still will not.** The matrix asserts
   compilation; nothing checks the staging step's output. A green Linux row says
   the platform builds, not that its module browser has anything in it.

**Next:**

1. **A mac or windows run should confirm its own platform still stages
   correctly** — neither expression changed for them, but the file did, and that
   is cheap to check with the same directory listing.
2. **The `SE_LOCAL_BUILD=TRUE` install copy is still wrong on Linux** and is left
   alone: `copy_plugin` copies to `~/.vst3` *before* these POST_BUILD steps run,
   the same ordering trap the APPLE block works around with `_tide_installed`.
   Not reproduced, because reproducing it means writing a plugin into Jeff's
   `~/.vst3`. Worth a row if anyone builds Linux with that flag.
3. Unchanged from the previous entry: **the `apt-get` in `build.yml` is still the
   only thing between C7e and closed**, and CI filed a fourth issue (#193) for
   the same cause while this ran.

**Branch/PR:** `tide/linux/S21-bundle-resources` — TideSynth only.

## 2026-08-20 — linux — #190: the Linux CI package set, measured

**Prompt:** 35e4ee6 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 · as **tide-rack-bot** (both paths)

**Did:** took no backlog row. STEP 1 outranked it — `build.yml`'s matrix ran for
the first time overnight and filed three `platform:linux` failures
([#189](https://github.com/JeffMcClintock/TideSynth/issues/189),
[#190](https://github.com/JeffMcClintock/TideSynth/issues/190),
[#191](https://github.com/JeffMcClintock/TideSynth/issues/191)), all one cause.
The macOS run that triggered them diagnosed it correctly and said in as many
words that *"the exact package set wants checking on a real ubuntu box rather
than guessed at from the probe names"*. This box is that ubuntu box.

### The CI failure, reproduced exactly

No containers on this machine, so the runner was mirrored at the layer the
failure actually lives in: a `PKG_CONFIG_LIBDIR` holding every `.pc` on this box
**minus** the seven the CI log reported as not found. Configuring a clean clone
under it reproduces the failure to the line:

```
CMake Error at FindPkgConfig.cmake (message):
  The following required packages were not found:
   - xext
Call Stack:  .../gmpi_wrappers-src/wrapper/VST3/CMakeLists.txt:257
```

Same error, same file:line, same package as
[run 32329948996](https://github.com/JeffMcClintock/TideSynth/actions/runs/32329948996).

### The chain, walked rather than read

`pkg_check_modules(... REQUIRED)` fails fast, so the log names one missing
module and hides the rest — the trap the mac entry flagged twice in one day.
Restoring one `.pc` at a time and re-configuring:

| step | rc | missing | probe |
|---|---|---|---|
| 1 | 1 | `xext` | `VST3/CMakeLists.txt:257` |
| 2 | 1 | `harfbuzz` | `:260` |
| 3 | 1 | `dbus-1` | `:264` |
| 4 | **0** | — | — |

**Three packages, not one.** Each `.pc`→Debian mapping was read with `dpkg -S`
rather than guessed: `libxext-dev`, `libharfbuzz-dev`, `libdbus-1-dev`. A fourth,
`libpng-dev`, passes today only because the runner image happens to ship it.

### Linux builds — the first time TIDE has ever been built here

Clean `git clone` of the public URL, 158 files, then CI's own two commands with
only the fixed package set visible:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release    rc=0
cmake --build build --config Release --parallel   rc=0, 0 error lines  (76s)
  -> TIDE.gmpi      7,378,536 bytes  ELF 64-bit LSB shared object, x86-64
  -> TIDE_VST3.so   8,494,704 bytes  ELF 64-bit LSB shared object, x86-64
  -> TIDE_VST3.vst3/Contents/x86_64-linux/TIDE_VST3.so
350 objects; grep -ci se16 over configure log / build log / CMakeCache.txt = 0
```

So all three platforms are proven and **the `apt-get` is the only thing left in
C7e**. It is still Jeff's: the token has no `workflow` scope, by design.

### Relaxing the X11 probe is measurably wrong, not merely inelegant

`GMPI_Wrappers` is ALLOWED, so I could have made the probe optional and turned
CI green without anyone. mac declined this on principle; here it is a number.
`ldd` shows all three libraries linked, and the undefined-symbol counts show they
are used: **5 `XShm*`**, **50 `hb_*`**, **27 `dbus_*`**. An optional probe does
not yield a Linux VST3 with a lesser editor — it yields one that fails to link.
The Wayland trio genuinely is optional: without it configure prints *"Wayland
support off … X11 editor only"* and *"STANDALONE skipped"*, then succeeds.

### A separate defect this build found — filed as S21, not fixed

Building on Linux for the first time exposed something CI would never have
caught, because CI stops at "did it compile": **TIDE's resources are staged
outside the Linux bundle.**

`SynthEditSem/CMakeLists.txt:339` uses `$<TARGET_FILE_DIR:tgt>/../Resources`,
commented *"the binary sits in Contents/<arch>/, so Resources is its sibling"*.
On Linux it does not: `gmpi_plugin.cmake:849-861` links a bare `.so` in the
target dir and copies it into the bundle **afterwards**. Measured — the XMLs and
`Prefabs/` land in `build/Resources/`, while `TIDE_VST3.vst3/Contents/` holds
**only** `x86_64-linux/`. The reader disagrees explicitly
(`BundleInfo.cpp:296-299`): Linux resources live at
`<name>.vst3/Contents/Resources/`. Both formats are wrong by different amounts —
the bare `.gmpi` has no `Contents` in its path at all, so it wants `Resources/`
beside the binary.

Consequence is already spelled out in the source: `seedPrefabsFromBundle` prints
*"no Prefabs folder in bundle resources - the rack module browser will be
empty"*, and the five pin-description XMLs never load, which is the linked-but-
pinless failure the CMake comment records from V3.

**Filed, not fixed** — STEP 3 scopes me to one item, the build is rc=0 so this is
not the build break, and the expression is shared with Windows, which I cannot
compile on.

**Learned:**

1. **A fail-fast dependency probe costs one CI round trip per missing package,
   and the cheap fix is to walk the chain locally.** Reading the list from source
   got 6 of 7 names right but could not say which were actually absent on the
   runner; restoring them one at a time answered both questions in one pass.
2. **"CI is green" would not have caught S21.** The matrix asserts compilation;
   the prefabs are a packaging step whose output nothing checks. A green Linux
   row would have said the platform works while its module browser was empty.
3. **The runner's package set is partly luck.** `libpng` is satisfied by the base
   image, not by anything this repo declares — so it is one image bump away from
   being the next `xext`, diagnosed one name at a time all over again.
4. `PKG_CONFIG_LIBDIR` pointed at a pruned copy of the system `.pc` files is an
   accurate, seconds-long stand-in for a differently-provisioned machine, and it
   isolates the variable better than a container would have.

**Next:**

1. **Jeff: one `apt-get` step and C7 closes**, unblocking C10 and R2–R6. The
   verified block is in [docs/ci/linux-build-deps.md](docs/ci/linux-build-deps.md),
   ready to paste. #189/#190/#191 stay open until a green run closes them.
2. **S21** is the next linux-takeable row, and it is small.
3. C7b, C16 and C7d were flipped IN-REVIEW→DONE here on their merged PRs. Their
   rows were not moved into `BACKLOG-DONE.md`; that archiving is still owed.

**Branch/PR:** `tide/linux/issue-190` — TideSynth only; docs, backlog, journal.

## 2026-08-20 — macos — C7e: the clean clone builds; the CI clause is one apt-get away

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Fifteenth item this session.** C7d merged first.

**Did:** ran C7's real proof, and it splits cleanly in two.

### The clean clone builds — proven by the literal test

Not a worktree, not an override: `git clone` of the public URL into an empty
directory with **no sibling repos and no SE16 anywhere on the path**.

```
git clone https://github.com/JeffMcClintock/TideSynth   158 files
cmake -B build -G Ninja                                 rc=0
cmake --build build                                     rc=0, zero error lines
  -> TIDE.gmpi, TIDE_VST3.vst3, TIDE_STANDALONE.app
lipo -archs TIDE.gmpi/Contents/MacOS/TIDE               x86_64 arm64
TIDE_STANDALONE                                         runs, enriches XML, seeds prefabs
grep SE16 in cc logs / CMakeCache / build.ninja         ZERO
```

**Universal, not a single-arch dev build** — worth stating, because that is the
difference between "it compiles" and "this is the artefact you would ship".

**A stranger can now clone this repo and build TIDE.** That is what C1–C7 were
for, and it is done.

### The CI clause is not met, and the reason has nothing to do with the carve-out

C7e is written as *"build.yml's three platforms **run rather than skip**, and
pass, on a PR."* They run now (C7d) and **windows passes**. **Linux fails on
missing system packages.**

`GMPI_Wrappers/wrapper/VST3/CMakeLists.txt:249-263` hard-requires six pkg-config
modules on Linux — `x11`, `xext`, `fontconfig`, `freetype2`, `harfbuzz`,
`libpng`, `dbus-1` — and the ubuntu runner has none.

**`pkg_check_modules(REQUIRED)` fails fast, so the log names only `xext`.**
Fixing that one moves the failure to the next probe — the same shape as
`CoreMidiDriver.h` this morning, where CI printed one missing header and a second
was waiting behind it. The full list and a ready-to-paste step are on
[#189](https://github.com/JeffMcClintock/TideSynth/issues/189), which `build.yml`
filed by itself.

**So C7e is NEEDS-JEFF, not blocked on a decision:** the remaining step is an
`apt-get` in `.github/workflows/build.yml`, and the fleet's token deliberately
lacks `workflow` scope.

**I did not reach for the alternative**, and it is worth saying why. `GMPI_Wrappers`
is on STEP 5's ALLOWED list, so I *could* have made the X11 probe optional the
way the Wayland one already is. That would turn CI green by removing a real
requirement — a Linux VST3 with no editor — which is papering over the gap, not
closing it. The dependency is genuine; installing it is the honest fix.

**Learned:**

1. **"CI is green" and "a stranger can build it" are different claims, and C7e
   asks for the first while carve-out.md calls the second the real proof.** Both
   were worth measuring separately; only one landed.
2. **A fail-fast probe reports one missing dependency and hides the rest.** Twice
   today. When a required-package check fails, read the *whole* list from the
   source rather than the one name in the log.

**Next:**

1. **One `apt-get` step and C7 closes**, unblocking C10 and the release track
   R2–R6. It is Jeff's to push.
2. The macOS matrix job was still running when this was written — windows green,
   linux red, macos unknown. It builds a universal binary from scratch, so it is
   slow.

**Branch/PR:** `tide/mac/C7e-clean-clone` — TideSynth only, backlog and journal.

## 2026-08-20 — macos — C7d: TideSynth builds on its own

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Fourteenth item this session.** C7b and C16 merged first.

**Did:** wrote TideSynth's root `CMakeLists.txt`. **The repo now configures and
builds TIDE without SE16.**

```
cmake -B build -G Ninja        rc=0
cmake --build build            rc=0, 460 edges, 456 objects
                               TIDE.gmpi, TIDE_VST3.vst3, TIDE_STANDALONE.app
TIDE_STANDALONE                runs, seeds 6 prefabs, builds the rack
grep for SE16 paths            ZERO, in both the configure and build logs
```

That is C7's whole point, reached: a stranger with this repo and a compiler gets
a plugin.

### The one thing the row did not predict, and it cost the first configure

TIDE's `FORMATS_LIST` is `GMPI VST3 STANDALONE` — no AU, no CLAP. But
**`GMPI_Wrappers` configures its AU2 and CLAP wrappers unconditionally**, and
CMake needs those SDK sources to exist even for a target nothing links:

```
CMake Error at build/_deps/gmpi_wrappers-src/wrapper/AU2/CMakeLists.txt:95:
  Cannot find source file: /include/AudioUnitSDK/AUBase.h
```

So the AudioUnit and CLAP fetches are in TIDE's root purely to satisfy
configure. Copied from SE16 verbatim, with a comment saying why — the temptation
next time will be to delete them as "TIDE doesn't ship AU".

**More generally: the SDK fetches and the CPM bootstrap are copied close to
verbatim rather than reworked.** They are fiddly, not TIDE-specific, and
divergence between the two roots is the likeliest way to break a build here
without breaking one there.

### What is deliberately absent

`se_vst3` / `se_gmpi` / `se_au` (SynthEdit's own plugin engine), `EditorScreenshot`
(dropped by U3 with the breadcrumb bar), `SynthEditCL`, `SynthEditWayland`,
`tests`, and the desktop apps. TIDE's subset is SynthEditLib + EditorLib +
SynthEditSem, and that is all.

### This turns the CI matrix ON, and that is the point

`build.yml`'s guard job keys on a root `CMakeLists.txt` existing — *"the moment
C7 adds a root CMakeLists.txt the matrix starts running again with nobody having
to remember to remove anything"*. So all three platforms begin building on
`tide/**` pushes and PRs, **with no `.github/workflows/**` edit**, which the
fleet's token could never have made.

**macOS is proven. Windows and Linux are unproven and may go red on first
contact.** That is the mechanism working rather than a regression, and
`build.yml` files a platform issue automatically on the push run.

**Learned:**

1. **A subproject you do not use can still block configure.** Nothing links AU2
   or CLAP, and both had to be fetched anyway. "Which formats do I ship" and
   "which SDKs must exist for CMake to generate" are different questions.
2. **Copying a fiddly block verbatim beats improving it.** The CPM bootstrap and
   SDK fetches are near-duplicates of SE16's on purpose; the failure mode of a
   tidied-up copy is a divergence nobody notices until one root builds and the
   other does not.

**Next:**

1. **C7e** — the clean-clone test, now genuinely runnable. After it C7 closes,
   and C10 plus the release track R2–R6 unblock.
2. Watch the first three-platform matrix run: win/linux may need work, and the
   issues will file themselves.

**Branch/PR:** `tide/mac/C7d-root-cmake` — TideSynth only.

## 2026-08-20 — macos — C16: the last private include was three dead symbols

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Thirteenth item this session**, stacked directly on C7b.

**Did:** deleted `#include "SynthEditApp.h"` from
`SynthEditSem/TideAppStubs.cpp` — the last include in TIDE's own source that
resolved only inside the private repo.

### The row expected C14's treatment. Measuring first showed there was nothing to narrow

C16 was filed assuming the two member definitions —
`SynthEditApp::isMoonbaseEnabled()` and `::licenseIsActive()` — needed the
complete type and therefore an interface, the way C14 handled
`ApplySynthEditConfig.cpp`.

**They are not referenced anywhere in `EditorLib` or `SynthEditLib`.** C11 had
already replaced that path with `ILicenseState`, and `GetLicenseState()` —
returning `nullptr`, in this very file — is what EditorLib actually calls. Every
surviving caller of those members is desktop-app code (`SynthEditMac/`,
`SynthEdit2/`) which links the real `SynthEditApp.cpp` and never this file.

**So the fix is a deletion, not an interface.** All three symbols go: the two
members and the `theApp` global.

**Why the row got it wrong is worth naming:** the file's own header comment still
described the pre-C11 world — *"three symbols that EditorLib references"* — and
I wrote C16 from that comment. **A stale comment set the expected difficulty of
the work**, exactly as it did for A27, where `check-next-block.py`'s docstring
claimed a behaviour the code never had.

One include replaced it: `ILicenseState.h`, which had been arriving
*transitively* through the private header and lives in the **public**
`SynthEditLib`. The build error that revealed it (`unknown type name
'ILicenseState'`) is the useful kind.

### Verification

| | |
|---|---|
| objects | **943 — identical to C7b's baseline**, so nothing stopped being built |
| artefacts | `TIDE.gmpi` + `TIDE_VST3.vst3` |
| ctest | **86/86** |
| runtime | `TIDE_STANDALONE` runs, seeds 6 prefabs |
| `dangling_private_includes.py` | **3 → 2**, `SynthEditApp.h` gone |

**The two survivors are not real.** Both are `tinyxml/tinyxml.h`, which resolves
in the **public** `SynthEditLib` — on TIDE's include path via `SYNTHEDITLIB_DIR`
— and is only reported because `--public` was pointed at TideSynth alone, while
TIDE's public surface is TideSynth *plus* SynthEditLib. **So TIDE's own source
has zero real dependencies on the private repo**, which is what C7d needs.

`SE16_SYNTHEDIT2_DIR`, which C7b added an hour earlier, is deleted from both
CMakeLists — it existed solely for this include.

**Learned:**

1. **A deletion is a legitimate answer to "narrow this to an interface", and it
   is cheaper to check for than to build toward.** Two greps over EditorLib and
   SynthEditLib decided it before any code was written.
2. **Stale comments do not just mislead about behaviour — they set the expected
   SIZE of the work.** C16's row inherited its difficulty estimate from a comment
   describing a world C11 had already ended. Second time today (see A27).

**Next:**

1. **C7d** — the root `CMakeLists.txt` TideSynth does not have. Now genuinely
   unblocked: nothing in TIDE's source reaches into SE16.
2. Then **C7e**, the clean-clone test, and C7 closes.

**Branch/PR:** `tide/mac/C16-tideappstubs` in TideSynth and SynthEdit, **stacked
on C7b's branches** and to be merged after them.

## 2026-08-20 — macos — C7b: TIDE's own source leaves the private repo

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Twelfth item this session.**

**Did:** `SynthEditSem/` (16 files) and `TideModules/` (11) now live in
**TideSynth**. SE16 consumes them through a `TIDESYNTH_FOLDER_OVERRIDE` +
`FetchContent` pair mirroring `SYNTHEDITLIB_FOLDER_OVERRIDE` line for line — one
pattern to learn, and anyone who already sets the SynthEditLib override knows
what to do with this one.

### Accept, met exactly as the row wrote it

| clause | result |
|---|---|
| `git ls-files` in SE16 shows zero `SynthEditSem/` / `TideModules/` | **0 and 0** |
| SE16 still produces `TIDE.gmpi` **and** `TIDE_VST3.vst3` | both |
| same object count | **943, identical** to the baseline taken in the same tree immediately before |
| ctest green | **86/86** |

Plus: `TIDE_STANDALONE` runs, seeds **6** prefabs from the moved `TideModules`,
and the bundle stages all six.

### Only ONE of the three `../` paths actually broke

The row predicted three. Both `../TideModules/prefabs` entries **still resolve**,
because the two folders moved *together* — `TideSynth/SynthEditSem/../TideModules`
is `TideSynth/TideModules`. Only `../SynthEdit2` broke, and it is now
`${SE16_SYNTHEDIT2_DIR}`, set by SE16's root before `add_subdirectory` and empty
when TideSynth builds standalone.

**`SOURCE_SUBDIR docs` in the FetchContent block is deliberate**, the same trick
the SynthEditLib block uses: it points at a folder with no `CMakeLists.txt` so
FetchContent does not add the fetched tree as a subproject. It also survives C7d
adding a root `CMakeLists.txt` to TideSynth, which would otherwise start being
configured twice.

### Taken on mac, not linux

The NEXT block nominated linux. Linux has not run since 2026-08-19 and left **no
branch and no claim**, the row is platform `any`, and it gates C7e → C7 → C10 →
R2–R6. STEP 2's collision test is a remote branch or open PR naming the id, and
there was neither. The `linux` NEXT row now says so and points at C7d.

### The residual, filed as C16

`SynthEditSem/TideAppStubs.cpp:31` still includes the private `SynthEditApp.h`.
**C14's twin**, and the row records the asymmetry that matters: two of the three
symbols are **member definitions** (`isMoonbaseEnabled`, `licenseIsActive`, both
`return false`) which need the complete type, but the third — `SynthEditApp*
theApp = nullptr` — needs **only a forward declaration**, because a pointer
definition does not require a complete type and a global's mangled name carries
no type in the Itanium ABI. So the symbol is unchanged either way.

Until C16 lands, a *standalone* TideSynth build still cannot compile this target.
That is expected and is C7d/C7e's business, not a regression: C7b's Accept never
claimed otherwise.

**Also this run:** flipped **C14, A22, A23, A24** to DONE on their merged PRs;
re-specced **E5** and closed **S20** on Jeff's answers.

**Learned:**

1. **Moving two folders together is cheaper than moving one.** Every relative
   path *between* them survives untouched. The row's "three `../` paths" became
   one purely because `TideModules` travelled with `SynthEditSem`.
2. **A "same object count" acceptance clause is worth more than it looks.** 943
   before and 943 after says no target silently stopped being built — which a
   green build and a green ctest would both have tolerated, since a dropped
   optional target breaks neither.

**Next:**

1. **C16** — the last private include; after it, C7d and then the clean-clone
   test. Mac is taking it.
2. **A12** is the only A-series row left and is `.github/workflows/**`.

**Branch/PR:** `tide/mac/C7b-tide-source` in TideSynth and SynthEdit — **two
repos, and they MUST merge together.** SE16's default (no override) fetches
TideSynth's `origin/main`; until the TideSynth half is on `main`, that fetch
returns a tree with no `SynthEditSem/` and SE16's configure fails.

## 2026-08-20 — macos — A24: the journal floor is one DATE, because seven days measures 651 KB

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Eleventh item this session.** A22 and A23 both merged.

**Did:** changed the rotation floor from four entries to **the later of four
entries or every entry sharing the most recent date**, raised the trim target
30 KB → 60 KB, and changed the run prompt from *"read at least the last four
entries"* to **"read all of it"**.

### A24's premise is right and its remedy is wrong, by a factor of twenty

**Premise, re-measured and now sharper than when filed:** `JOURNAL.md` held
**five entries all carrying the same date**. A run obeying *"read the last four
entries"* saw under one day. A24 measured "about half a day" on 08-18; it has
got worse.

**Remedy refuted.** A24 asked to *"retain everything from the last 7 days"*.
Counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it. **Seven days is 3.4× the 192 KB
that triggered A8 in the first place** — the cure twenty times more expensive
than the disease. Even *two* days is worse than the state A8 was created to fix.

**So the floor is one date.** It bounds the cost at roughly a day's work while
guaranteeing a run sees everything that happened most recently, which is the
failure A24 correctly identified. On a quiet week the four-entry floor still
binds and nothing changes — the rule only bites on days like this one.

### What it does not fix — filed as A30

Rotation carries an entry's **Learned** bullets into the archive with it, and
**no run reads the archive**. So a lesson is load-bearing for about a day and
then silently stops being read. A24's one-date bound is the most that is
affordable, so the window cannot be widened to solve this: the lessons have to
leave the rotating file, into a standing digest.

This is not hypothetical. Twice today a run re-derived something an earlier entry
had already recorded, and this session is the shortest possible distance from
those entries.

**Learned:**

1. **Measure the remedy, not just the problem.** A24 was filed with careful
   numbers for the *premise* and an unmeasured guess for the *fix*. The guess was
   off by 20×, and nothing in the row's own reasoning would have revealed it —
   only counting the bytes did.
2. **A24 nearly cited a taken ID.** I wrote "filed as A25" and A25 has been in
   `BACKLOG-DONE.md` since 08-18. Caught by checking rather than by lint —
   `check-id-refs` validates that a *referenced* ID exists, and A25 does exist.
   The duplicate check shipped this morning would have caught the *row*, had I
   written one. Worth knowing that "the ID exists" and "the ID is free" are
   different questions and only one of them is linted.

**Next:**

1. **A30** — the lessons digest. Wants Jeff's eye on the prompt half.
2. **A12** is the only A-series row left, and it is `.github/workflows/**`.
3. **C7b / C7d** are the carve-out's critical path; **C14** is IN-REVIEW with all
   three PRs merged and should flip to DONE.

**Branch/PR:** `tide/mac/A24-journal-floor` — TideSynth only.

## 2026-08-20 — macos — A23: duplicate-ID detection, and the three false alarms that shaped the rule

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Tenth item this session.** A22 is IN-REVIEW on
[#183](https://github.com/JeffMcClintock/TideSynth/pull/183), green and waiting.

**Did:** `scripts/check-id-refs.py` now fails when one ID owns more than one row.
It already parsed every row ID; the change is that it records **locations**
rather than a set, because a duplicate is only actionable if the report names
*both* lines — renumbering means editing one of them.

### The naive rule was red on day one, and that is the finding

A first cut flagged **three** IDs in the live tree. **All three were
legitimate**, and each taught the rule something:

| flagged | what it actually is |
|---|---|
| `~~P8~~` (BACKLOG.md) | a **superseded row kept beside its replacement**. `RE_ID_CELL` tolerates the tildes *on purpose*, so references to it still resolve |
| `~~G3~~` (BACKLOG-DONE.md) | the same shape |
| `S1` ×2 (BACKLOG-DONE.md) | **a genuine, deliberate duplicate** — the linux and macOS boxes both took S1 on 2026-08-06, before the cron stagger took effect, and the second row says so in its own text |

So the rule is narrower than "one ID, one row":

- **Superseded rows are excluded from the duplicate test only**, not from the
  known-ID set. Both properties are needed and they are not the same property.
- **Archive-only duplicates are not flagged at all.** The archive is history,
  *"archiving never rewrites a row"*, and flagging S1 would demand an edit the
  rules forbid — on every run, forever.
- **What IS flagged is any duplicate touching `BACKLOG.md`:** two live rows (the
  A17 collision A23 was filed for), or one row in each file — an archive move
  that *copied* instead of moving, which makes the row's status ambiguous.

### Verification

Two positive controls rather than the one A23 asked for, because the second
shape only became visible while writing the rule:

```
duplicate row in BACKLOG.md   -> rc=1, names BACKLOG.md:76 and :77
copied into the archive       -> rc=1, names BACKLOG.md:76 and BACKLOG-DONE.md:20
the real tree                 -> rc=0
```

`--selftest` is **25 cases, 0 failed** (was 20), on real file bodies rather than
regex snippets because every subtlety here is about *which file* a row is in.
**The selftest is itself proven able to fail:** flipping one case's expectation
gives `FAIL duplicate/clean`, rc=1.

**Noted, not fixed:** `lint.yml` invokes this script **without** `--selftest`, so
those 25 cases run only by hand — exactly like `check-next-block.py`. Making CI
run them is a `.github/workflows/**` edit, so it needs Jeff either way.

**Learned:**

1. **Run a new lint against real history before believing it.** Three false
   alarms, zero of them predictable from the row's description — and A23's own
   text said the check was "a few lines on data it has already collected", which
   was true and still nearly shipped a rule that failed `main`.
2. **Two properties that look like one.** "Is this a known ID?" and "does this ID
   own a row?" differ precisely on struck-through entries, and the existing
   regex's tolerance of `~~` was deliberate for the first question. Reusing a
   collector for a second question is where that kind of assumption breaks.

**Next:**

1. **A24** — the last A-series row: the journal's rotation floor is counted in
   entries, so cross-run memory shrinks as cadence rises.
2. **S20** and the CI-on-push question are Jeff's; **U3's click path** is still
   unverified.

**Branch/PR:** [#184](https://github.com/JeffMcClintock/TideSynth/pull/184), branch
`tide/mac/A23-duplicate-ids` — TideSynth only.

## 2026-08-20 — macos — A22: the row names the branch, not the PR; and SynthEdit's CI never runs on push

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Ninth item this session.** S19 is fully closed —
[SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66),
[#67](https://github.com/JeffMcClintock/SynthEdit/pull/67) and
[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28) all
merged, [#178](https://github.com/JeffMcClintock/TideSynth/issues/178) closed.

**Did:** STEP 4 now says the backlog row must name your **branch**, with the PR
link a best-effort extra.

### Why (a)+(d) rather than either option the row offered

The old wording — *"mark the item IN-REVIEW with links to every PR you opened"* —
**cannot be satisfied in the commit that makes the mark**, because the PR does
not exist until after it. So it guaranteed a follow-up.

A22 offered (a) *name the branch* and (d) *accept the follow-up PR*. Taking (d)
alone would legalise the second PR rather than remove the need for it — and #120
showed the second PR is not the real cost. Its follow-up landed on a branch whose
PR had already auto-merged, which is **a pushed branch with no PR: the one end
state STEP 5 forbids.** So the new text adds a precondition A22 did not ask for:

> check the PR is still open before pushing the follow-up, and if it has already
> merged, **drop the commit** — `gh pr view <n> --json state --jq .state`

Pushing nothing is always safe. A commit whose only content is a link is never
worth a second PR, and the branch name in the row is what makes the follow-up
**optional rather than load-bearing**.

**The evidence is use, not argument:** this session ran the
branch-then-follow-up shape about eight times across three repos — A27, A28, A21,
S19, U3 — and never needed a second PR.

### Found while confirming S19: the CI chain never runs on push — filed as S20

`SE16 Kickstart Build` is **`on: workflow_dispatch:` and nothing else**.
`cmake_win` triggers off *it*; `cmake_mac` triggers off `cmake_win`. **So a push
to `master` runs no build and no tests.** The last dispatch was 2026-08-19T09:34
and `master` has moved eight times since with **zero** runs.

That is half the reason S19's five failures survived a week: `continue-on-error`
hid them, and the chain fired rarely enough that few people ever saw a log.

**The obvious fix is wrong, which is why it is a row and not a patch.** That
chain is a *release* pipeline — it signs, notarizes, staples and **FTP-uploads to
synthedit.com**. `on: push` would publish on every commit. What it wants is a
separate build-and-test-only workflow, and that is `.github/workflows/**`, so it
needs Jeff either way.

**I did not dispatch it.** Triggering that chain is a release, not a test run —
so today's mac fixes are verified locally (86/86) and remain unconfirmed in CI
until Jeff next kicks one off.

**Learned:**

1. **A rule that cannot be obeyed in one step will be obeyed in two, and the
   second step is where the damage is.** Nobody was going to skip linking the PR;
   they were going to link it badly. The fix was not to demand less but to move
   the required content to something knowable *before* the push.
2. **"CI is green" means nothing until you know what triggers CI.** I spent the
   session treating cmake_mac as the arbiter of mac health. It runs when a human
   asks it to, and it had not been asked since before any of today's work.

**Next:**

1. **A23** — duplicate-id detection in `check-id-refs.py`, the best-specced row
   left, with a positive control already written into it. **A24** after it.
2. **S20** and the `continue-on-error` follow-through both want Jeff.
3. **U3's click path** is still the one unverified thing from today.

**Branch/PR:** [#183](https://github.com/JeffMcClintock/TideSynth/pull/183), branch
`tide/mac/A22-pr-link` — TideSynth only, docs and backlog.


## 2026-08-20 — macos — the mac test drift is FMA contraction, and my own diagnosis was wrong first

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Seventh and eighth items this session**, on Jeff's instruction. Also closes
**U3** (shipped as [SynthEdit#65](https://github.com/JeffMcClintock/SynthEdit/pull/65)).

**Did:** diagnosed the four `TestVoiceAllocation` failures that S19 papered over
this morning with raised gates, and reverted those gates because they turned out
to be unnecessary. [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66).

### The hypothesis I wrote into three documents was wrong

This morning's row, issue and PR all said the error shape — max −68 dB against an
average −150 dB — looked like **a one-sample timing difference at voice
transitions**. Measured against the reference `.wav`, every part of that is false:

| claim | measurement |
|---|---|
| a handful of samples | **81.9% of all samples**, continuous from 0.127 s |
| a one-sample shift | shift 0 = −68.73 dB, shift ±1 = **−22.69 dB** — zero wins by 46 dB |
| (unstated) a gain error | best scalar fit **0.999999192**, residual unchanged |

It was a plausible story fitted to one summary statistic, and it survived into
three places because nobody had opened the file. **The average/max ratio I
reasoned from was the cancellation utility's own metric, not something I had
computed.**

### The actual cause

**FMA contraction.** clang defaults `-ffp-contract` to *on*, fusing `a*b+c` into
one `fma`. arm64 always has FMA; x86-64 under MSVC or GCC does not emit it by
default — **which is exactly why Windows and Linux reproduce the references and
macOS does not.**

It is *not* the Apple fast-math subset that was already in `CMakeLists.txt`. That
was the obvious suspect and it was eliminated this morning: with
`-fassociative-math` and `-freciprocal-math` removed, the four residuals were
**bit-identical**. Only contraction moved them.

```
test                        contract=on   contract=off
Unterminated_Poly_Modules   -80.77 dB     -90.31 dB
Voice_Allocation_Mono_High  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Last  -68.73 dB     -90.31 dB
Voice_Allocation_Mono_Off   -73.41 dB     -90.31 dB
```

**−90.31 dB is exactly 1 LSB at 16 bits** (`20·log10(1/32768)`), i.e. bit-identical
within the file format. Full suite with the strict gates restored: **3 failures
with contraction on, 86/86 with it off.**

So there was never a voice-allocation defect, and **the four gates raised this
morning are reverted to 85/75/75/75.**

### Checked rather than assumed

[SynthEditLib#28](https://github.com/JeffMcClintock/SynthEditLib/pull/28)'s
soundfont scoping is **still load-bearing**: rebuilt with it reverted *and*
contraction off, `SoundfontOsc` still fails. Reassociation and contraction are
different mechanisms and neither fix makes the other redundant.

**Learned:**

1. **A hypothesis that explains the summary statistic is not a diagnosis.** Max
   ≫ average genuinely does suggest sparse differences — and the differences were
   dense. Ten minutes of `numpy` against the two files would have prevented three
   documents asserting it. **Open the artifact.**
2. **Eliminating the obvious suspect is worth more than confirming it.** This
   morning's A/B on `-fassociative-math` looked like a dead end — the figures were
   bit-identical, so the flags "weren't it". That negative result is what pointed
   at a *different* FP mechanism rather than a DSP bug, and it is why the second
   experiment was aimed correctly.
3. **`-ffp-contract` is invisible in a fast-math discussion.** It is not part of
   `-ffast-math`, is not mentioned by the flags this project already reasons
   about, and is on by default. On any arm64 target it is the first thing to check
   when a render differs from an x86-baked reference.

**Next:**

1. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178) can close once [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) merges** — except for the
   `continue-on-error` removal, which stays Jeff's.
2. **U3's click path is unverified** — one right-click on the rack background.
3. **A22, A23, A24** are the remaining A-series rows; **C7e** is unblocked from
   the `EditorScreenshot` direction now C7c is closed.

**Branch/PR:** `tide/mac/s19-fma-record` (this) + [SynthEdit#66](https://github.com/JeffMcClintock/SynthEdit/pull/66) (the code).

## 2026-08-20 — macos — C7c answered by removal, and the two questions that answer creates

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths)

**Sixth item this session**, on Jeff's instruction. Also closes **S19** — both its
PRs merged and macOS `ctest` is **100% of 86**; the `continue-on-error` removal
is still Jeff's and stays on [#178](https://github.com/JeffMcClintock/TideSynth/issues/178).

**Did:** recorded Jeff's C7c ruling and filed the work it implies as **U3**.

C7c asked whether `EditorScreenshot` should become public so a stranger's clone
can link it. **Jeff answered by deleting the need:** *"let's remove the
breadcrumb bar from TIDE, it's a bit redundant in a product where you seldom dig
deeper than 1 level in."*

That is the better answer than either option the row offered. TIDE's only uses of
`EditorScreenshot` are `SynthEditGui.cpp`'s `ContainerThumbnail.h` include and
the link line in `SynthEditSem/CMakeLists.txt`, and **both exist solely to draw
crumb thumbnails** — so the dependency leaves with the feature, nothing has to
come out of the commercial repo, and **C7e loses its last non-`SynthEditLib`
blocker.**

### Why the removal is U3 and NEEDS-JEFF rather than something I did

Reading the code before cutting found two things the ruling does not settle, and
either one guessed wrong ships a worse product than the bar:

1. **The crumbs are the only way back UP a level.**
   `breadcrumbBar->onNavigate` (`SynthEditGui.cpp:699`) is one of exactly **two**
   navigation entry points. The other, `seApp->onOpenContainerView` (`:703`),
   goes only *in*, or to the master via "Goto Structure…". I grepped
   `SynthEditLib`, `EditorLib` and `SynthEdit2`: **there is no existing
   go-to-parent affordance.** Remove the crumbs with no replacement and a user
   who opens a module is stranded in it.

2. **The About pane's only entry point is anchored to the crumb strip.** D6's own
   comment calls it *"the about pane and the only way in"*
   (`SynthEditGui.cpp:292-299`), a plain text affordance at the strip's right end.

U3 carries three options and recommends **(a) keep a thin strip with just
"◀ Back" and "About"** — the only one that changes no interaction the user
already has, while dropping exactly the part Jeff called redundant: the thumbnail
trail.

**`SE2::BreadcrumbBar` itself is not being deleted.** It is shared with the
WinUI3, Wayland, JUCE and Mac frontends; only TIDE stops using it.

**Learned:**

1. **"Remove the feature" can be the right answer to a licensing-boundary
   question, and it is not one an agent would have proposed.** C7c framed the
   choice as *which files move*; the cheapest answer was that none do. Worth
   remembering the next time a row's options list looks exhaustive.
2. **A one-line product decision can have load-bearing code underneath it.** The
   bar looked like a widget and is also the navigation model and the About
   pane's front door. Reading before cutting cost ten minutes.

**Next:**

1. **U3 wants Jeff's answer on Back and About**, then it is one session.
2. **C7e** is now unblocked from the `EditorScreenshot` direction; **C7b** and
   **C7d** are unchanged and still the linux box's.
3. **[#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — the workflow edit, and the unexplained `TestVoiceAllocation` residual.

**Branch/PR:** `tide/mac/C7c-drop-breadcrumb` — TideSynth only, no code change.

## 2026-08-20 — macos — a mac build break from the carve-out, and five test failures CI has been hiding for a week

**Prompt:** eba799e · Opus 5 (1M context), claude-opus-5[1m] · app: Claude desktop **1.32885.1** (Claude Code CLI version not resolvable on this box) · as **tide-rack-bot** (both paths)

**Fourth and fifth items this session**, on Jeff's instruction — he pointed at a
failing Actions run rather than the backlog. Repos synced first.

## 1. The build break: [SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63)

`cmake_mac` had been red on `master` since `6c7e90053` while `cmake_win` and
`cmake_linux` were green at the same sha.

**The first thing to get right was WHICH step failed.** The 5 test failures in
that log are a decoy: `ctest` is `continue-on-error: true`, so **step 6 is marked
success**. The run failed at **step 20, the Xcode build**:

```
SynthEditMac/SynthEditMac/MidiAutomationWindowController.mm:3:10:
  fatal error: '../../SynthEdit2/PatchParameter.h' file not found
```

Two headers left `SynthEdit2/` during the carve-out and the Xcode consumer was
never re-pointed:

| header | moved by | now at |
|---|---|---|
| `PatchParameter.h` | `9a53a4882` — C12f, the patch cluster | `SynthEditLib/` |
| `IMidiDriver.h` | `4f6f5b1ca` — C13, the three orphan headers | `SynthEditLib/` |

CMake follows them because SE16's `CMakeLists` hands EditorLib the include
directories; the Xcode project quotes the paths literally. **This is exactly the
hazard C10's row already names** — *"Non-CMake consumers still need checking by
hand: `SynthEdit2.vcxproj` and the SynthEditMac Xcode project"* — so the carve-out
stages should treat that line as a checklist item, not a footnote.

**I fixed both files although CI named only one.** Xcode stops at the first fatal
error. Reverting both edits and rebuilding — with the `.mm` already compiled —
gives `CoreMidiDriver.h:6:10: fatal error: '../../SynthEdit2/IMidiDriver.h' file
not found`. Fixing only what CI printed would have turned one red run into two.

**Verified by building, twice**: at the original tip, and again after merging the
current `master` (`61eaf744b`, C14 landed, which changes EditorLib's include
directories so it was worth re-checking rather than assuming). Both times:
`cmake --build` **1064/1064 rc=0**, then `xcodebuild -scheme SynthEdit -arch
arm64 -configuration Release` → ***\*\* BUILD SUCCEEDED\****.

### A dev-box trap found on the way to that link

Reaching the link first produced
`Undefined symbols: ApplyConfigPreInit(SynthEditApp&, ...)`. Cause:
`libEditorLib.a` is built from `build/_deps/syntheditlib-src` (FetchContent,
**`GIT_TAG origin/main`** — a live ref), while the Xcode project resolves
`ApplySynthEditConfig.h` from the **sibling** `../SynthEditLib` clone. The clone
was behind C14, so the library exported the new `CSynthEditAppBase&` signature
and the header still declared `SynthEditApp&`.

**CI cannot hit this**: its *"Symlink CMake-fetched deps for Xcode"* step points
`../SynthEditLib` at `build/_deps/syntheditlib-src`, so header and library are one
tree by construction. On a developer box they are two trees tracking a moving
ref. Fixed by fast-forwarding the clone to `86ab11c`. **This is S17's shape one
level up** and worth knowing before it costs someone an afternoon.

## 2. The five test failures — filed as S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)

**They are not a regression, and they are not one bug.**

`94% tests passed, 5 tests failed out of 86` appears in *every* mac run I
checked, including 2026-08-13, 08-14 and 08-18 — **all of which are marked
success**. `continue-on-error: true` is why nobody knew.

| platform | at `6c7e90053` |
|---|---|
| Windows | 92/92 |
| Linux | 86/86 |
| **macOS** | **5 of 86 fail** |

Reproduced locally with figures **identical to CI's to four decimal places**, so
deterministic rather than flaky.

**The A/B that splits them.** `SynthEdit/CMakeLists.txt:281` adds, on Apple only,
`-fno-math-errno -fno-trapping-math -fno-signed-zeros -fassociative-math
-freciprocal-math`. Windows gets `/fp:fast`; **Linux gets no fast-math at all**,
which is consistent with Linux being the platform that passes. Rebuilding the
whole tree with the two reassociating flags removed and nothing else changed:

| test | with | without |
|---|---|---|
| `TestSoundfont.SoundfontOsc` | FAIL | **PASS** |
| `Unterminated_Poly_Modules` | −80.7666 dB | **−80.7666 dB** |
| `Voice_Allocation_Mono_High` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Last` | −68.7254 dB | **−68.7254 dB** |
| `Voice_Allocation_Mono_Off` | −73.407 dB | **−73.407 dB** |

So `SoundfontOsc` is FP reassociation, and **the four `TestVoiceAllocation` cases
are something else entirely** — bit-identical output either way, i.e. a real
deterministic mac-vs-reference difference the flags do not touch.

**Hypothesis, and labelled as one:** max −68 dB against an average of −150 dB
means a handful of samples differ, not a level or timbre — a constant offset
would put the average near the max. That is the shape of a one-sample timing
difference at voice transitions, which fits tests whose whole subject is voice
allocation. Unconfirmed.

**Not fixed on purpose.** Bumping four tolerances would turn the suite green and
throw the finding away, and whether −68 dB is acceptable in this product is not
an agent's call. The reporting half — removing `continue-on-error` — is a
`.github/workflows/**` edit the token structurally cannot push.

**Learned:**

1. **A `continue-on-error` step turns a failing suite into a decoy twice over.**
   It hid five real failures for a week, *and* it put a wall of red test output
   at the top of a log whose actual failure was fourteen steps later. Read the
   per-step conclusions before reading the log.
2. **"Last green run" is not a baseline when a step can fail without failing the
   run.** My first instinct was to bisect `9674bbfc7..6c7e90053`; the tests were
   already failing at `9674bbfc7` and in every run before it. Checking the older
   *green* runs' logs cost one command and saved a bisect that would have found
   nothing.
3. **An A/B that changes one flag is worth more than a plausible story.** The
   mac-only fast-math subset explained all five failures beautifully. It explains
   exactly one.

**Next:**

1. **S19 / [#178](https://github.com/JeffMcClintock/TideSynth/issues/178)** — diagnose the four, rule on SoundfontOsc's tolerance, and
   Jeff removes `continue-on-error`.
2. **[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) is open and green** — mac `master` stays broken until it merges.
3. **A22, A23, A24** are the remaining A-series rows; A23 is the best-specced.

**Branch/PR:** `tide/mac/mac-ci-findings` (this bookkeeping) plus
[SynthEdit#63](https://github.com/JeffMcClintock/SynthEdit/pull/63) (the code).
Two repos; the SynthEdit half is the whole fix and this half is the record, so
neither blocks the other. Throwaway worktrees; every checkout left on its default
branch and clean.
