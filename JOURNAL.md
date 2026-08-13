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

## 2026-08-13 — macos — S6 (part 2 of 2)

**Prompt:** `dd93251` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Part 1** of this run is the A11 entry (halt at STEP 0.7, cleared by Jeff
mid-session) on branch `tide/mac/A11-step07-halt`, [#46](https://github.com/JeffMcClintock/TideSynth/pull/46).
This is the item the run went on to take once the assertions passed.

**Did:** Deleted `SE16/SE_IOS_APP/TIDE/Plugins/` — six `.sem` bundles, **26
tracked files, 4.4 MB**. Chose *remove* over the row's "or add a README"
alternative: constraint 7 rules out separately-loadable module bundles
entirely, so a README would preserve 4.4 MB of a contradiction and explain it
rather than fix it.

**Result — the deletion is right, and two of the row's premises were wrong.**

What the files are, measured: all six binaries `Mach-O 64-bit bundle x86_64`,
`platform 1` (macOS) or `LC_VERSION_MIN_MACOSX`, in macOS bundle layout
(`Contents/MacOS/`, `Contents/_CodeSignature/`). Added 2021-02-24→2021-03-03,
**untouched since**. Nothing there can load on arm64 iOS. That much the row had
right.

| the row said | measured |
|---|---|
| "dead **iOS** module artifact", installed by a Run Script to a macOS-only destination | consumer is **`SeAudioUnitMacOS`** — a **macOS** AUv3 app-extension. **No iOS target references the folder at all.** It was never wired into an iOS build; the destination is not a mistake, it is correct for that target |
| the Run Script is the wiring; deleting the folder is safe | the Run Script is only *half*. **Individual files inside the bundles are entries in that target's Resources build phase** — real `CpResource` inputs |

The 2021 commit messages agree with the correction: *"chore(ios) : macOS AUV3
runnning (fixed signing by signing TIDE sems)"*. This is macOS AUv3 scaffolding
that happens to live under an iOS-named folder, which is exactly why it reads as
an iOS module story.

**Verification artifact — and the A/B is not clean, so I am not calling it
clean.**

| `SeAudioUnit macOS`, same machine, same command | before | after |
|---|---|---|
| result | **BUILD FAILED**, RC=65 | **BUILD FAILED**, RC=65 |
| errors | 6 × missing `SE_DSP_CORE/*.cpp` compile inputs | 7 × `CpResource` "couldn't be opened" |

Nothing went from working to broken. But the **failure mode changed**, and the
compile errors stop surfacing afterwards only because `xcodebuild` stops
scheduling once the resource phase fails — they are still there underneath. My
first reading of the pbxproj said the bundles were in no build phase at all;
that was wrong, and the A/B is what caught it. Tracing the six `.sem` *folder*
ids was not enough — they are group entries whose *children* are the build
inputs.

**The standing rule is honoured, and structurally rather than by luck.** Fresh
Ninja configure into a scratch dir (this tree untouched), all four local
overrides, full build: **RC=0, 936/936**, producing `SynthEdit_VST3.vst3`,
`SynthEdit_GMPI.gmpi`, `SynthEditCL.app`, `TIDE.gmpi`, `TIDE_VST3.vst3`. And
**no `CMakeLists.txt` or `.cmake` in `SE16` references `SE_IOS_APP`**, so the
CMake build and that Xcode project are fully decoupled — the deletion could not
have reached them.

**Learned — the big one, and it is much larger than S6:**

**`SE_IOS_APP.xcodeproj` is dead, and it bears on M2.** All four targets fail,
each RC=65, on **28 references to `SE_DSP_CORE/`** — the pre-split name of the
DSP core directory, which no longer exists (it became `SynthEditLib`). The
pbxproj was last touched **2022-12-15**. PLAN calls iOS AUv3 "the constraint
that validates the whole design", and **M2 is written as though a working iOS
project exists to build on. It does not.** M2 is really "author an iOS target",
not "get the existing one green" — worth knowing before anyone estimates it.
Filed as **S10**, with the revive-or-retire decision named as Jeff's.

Two smaller ones:

- **`database.se.xml` is the same architecture constraint 7 forbids.**
  `SE_IOS_APP/TIDE/Resources/database.se.xml` is a 31-entry module database
  naming the six now-deleted bundles by `imbeddedFilename`, and it *is* wired
  into two Resources build phases. Left alone deliberately — outside S6's scope
  — but it should not be revived as-is. Folded into S10.
- **`gh pr edit` fails with the bot's token; `gh api ... -X PATCH` does not.**
  `gh pr edit` issues a GraphQL query touching `login`/`name`/`slug`, which
  needs `read:org`; the bot has `repo` only, by design. The REST route has no
  such requirement. This will bite any run that tries to amend a PR body —
  including a run following STEP 1.5's "push fixes to the SAME branch".

**Next:**

1. **Merge [SynthEdit#13](https://github.com/JeffMcClintock/SynthEdit/pull/13)
   and [#47](https://github.com/JeffMcClintock/TideSynth/pull/47).** Order does
   not matter for the build — #13 is the only code change and it cannot break
   anything the CMake build touches — but merging the docs alone would say a
   deletion landed that did not.
2. **Answer S10 before S9.** If the project is retired, S9 is moot and the right
   move is deleting the whole `.xcodeproj`.
3. **`mac` NEXT moved S6 → P6**, and P6 is genuinely this box's to close: it
   needs an **Xcode**-generator build to reproduce the `CodeSign` failure, which
   Ninja never emits. `SynthEditCL` builds clean under Ninja here (re-confirmed
   RC=0 during this run), so the codesign step is the whole remaining question.

**Tree hygiene:** `SE16` was clean at claim time and only the 26 deletions were
staged — re-checked with `git status` immediately before commit, per the C3
run's lesson about an idle index being harvested. No work of Jeff's was touched.

**Branch/PR:** `tide/mac/S6-dead-ios-modules` in both repos —
[SynthEdit#13](https://github.com/JeffMcClintock/SynthEdit/pull/13) (the
deletion) and [#47](https://github.com/JeffMcClintock/TideSynth/pull/47) (docs).

---

## 2026-08-13 — macos — A11, mac half — halted at STEP 0.7, then resolved in session (part 1 of 2)

**Prompt:** `dd93251` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Outcome, up front: A11 is DONE on all three boxes.** This run halted on STEP
0.7's second assertion; Jeff applied the missing `git config` line while the
session was still live; the assertion and the full acceptance test then passed
and the run continued to S6. **The resolution is at the bottom of this entry;
S6 is [part 2](#2026-08-13--macos--s6-part-2-of-2), its own entry.** The halt
record below is kept unedited, because the deadlock it documents is real and
survives the fix.

**Did:** Nothing. This run stopped at STEP 0.7's second assertion, as the prompt
requires, before selecting or claiming any backlog item. **S6 was not started**
and remains `TODO`. What follows is the halt record plus the read-only
diagnostics needed to make it actionable.

**Result — assertion 1 passed, assertion 2 printed nothing:**

| STEP 0.7 command | required | actual |
|---|---|---|
| `gh api user --jq .login` | `tide-rack-bot` | `tide-rack-bot` ✅ |
| `git config --global --get url."https://github.com/".insteadOf` | `git@github.com:` | **empty, exit 1** ❌ |

That is the A11 gap the linux run found on 2026-08-13, in the one box A11 still
lists as outstanding. `git config --global --get-regexp 'url\.'` returns nothing
at all — setup step 3 has never been applied here. Setup steps 1–2 *are* in
place: `credential.https://github.com.helper` is
`!/opt/homebrew/bin/gh auth git-credential` (Homebrew path, not `gh`).

**The exposure on this box is nil today — and that is a measurement, not an
assumption.** Exhaustive sweep, `find ~ -maxdepth 5 -name .git` (excluding
`Library/`, `node_modules/`, `build/`, `_deps/`, `.Trash/`), **28 repos**:

- **Zero SSH GitHub remotes.** Every GitHub repo is `https://` — including all
  nine fleet repos (`TideSynth`, `SynthEdit`, `SynthEditLib`, `gmpi_ui`,
  `GMPI_Wrappers`, `GMPI`, `GMPI_Adaptors`, `GMPI-plugins`, `gimpi_ui_tests`)
  and the eight `VST_SDK` submodules.
- Non-GitHub and therefore out of scope: `~/SynthEdit` (Azure DevOps),
  `~/Plugins` + four `~/myagent/_work/*/s` build-agent checkouts (Azure DevOps,
  remote named `SSG` not `origin`), `~/MacSIMD` (**no remotes at all**).

So this box matches Windows (22/22 already HTTPS), not linux (8 of 9 SSH). **The
macOS A2 evidence A11 called "void until checked" is not void** — `gmpi_ui#3`/`#4`
and `GMPI_Wrappers#1`/`#2` were pushed from HTTPS remotes here, so those pushes
did authenticate as the bot, not as Jeff via SSH.

**A11's read-only three-way proof, run in full** against the private
`https://github.com/JeffMcClintock/SynthEdit.git`. No push, no write:

| GH_TOKEN | `git ls-remote` | `gh api user` |
|---|---|---|
| bogus | **`fatal: Authentication failed`** — `remote: Invalid username or token.` | — |
| real bot token | succeeds (`b3c1efb07…` HEAD) | `tide-rack-bot` |
| unset | succeeds | `JeffMcClintock` |

Leg 1 is the load-bearing one: git genuinely consults `gh` for HTTPS GitHub URLs
on this box rather than a keychain entry or an SSH key. **So for every remote
that actually exists here, the credential wiring is correct and now proven.**

**What is missing is only the structural safeguard**, which is exactly what A11
says it is for: a future `git clone git@github.com:…` would silently re-open the
hole with no signal, because nothing rewrites it.

**Learned — the finding that matters, and it is new:**

**This box cannot heal itself, and every future scheduled mac run will halt
here.** STEP 0.7 fires before STEP 1 and STEP 2, so a run can never reach the
point of claiming A11 — the assertion that detects the problem also forbids
fixing it ("do not 'fix' it by carrying on as whoever you are"). A11's mac half
is therefore **not takeable by a scheduled run at all**; it needs Jeff or an
interactive session on this machine. Until then the macOS box is a no-op in the
fleet: it will burn a run every week producing this same entry. Linux and
Windows are unaffected — both applied step 3 on 2026-08-13.

That deadlock is not a flaw in the halt rule, which did its job. It is a gap in
A11's framing: the row is written as ordinary backlog work with plat `any`, and
one third of it structurally cannot be done that way.

**Second, smaller:** `ls-remote --get-url origin` echoes the literal string
`origin` when no such remote exists, rather than erroring. A sweep that greps for
`git@` will read those as clean; they need checking with `git remote -v` before
being called clean. Two repos here hit that, both benign.

**I deliberately did NOT apply the one-line fix**, though it is the whole
remedy and I had the evidence for its acceptance test in hand. STEP 0.7 says
stop and do nothing else, and a run that reasons its way past its own failed
safety assertion is the precise failure mode the rule exists to prevent. It is
Jeff's to run, on this box:

```
git config --global url."https://github.com/".insteadOf "git@github.com:"
```

Acceptance is already half-established above: after that command, assertion 2
prints `git@github.com:`, all 28 remotes still read `https://`, and the
three-way proof is recorded here. **A11 can then be flipped DONE across all
three boxes.**

**Also checked, and clear:** no `platform:mac` issues; the only open issue is
[#44 "Fleet watchdog digest"](https://github.com/JeffMcClintock/TideSynth/issues/44)
(author `app/github-actions`, unlabelled — A6's digest, informational). No open
PRs at all in TideSynth, so STEP 1.5 had nothing either. Tree was clean and on
`main`, in sync with `origin/main`; no dirt of Jeff's was touched.

**Journal rotation was skipped on purpose.** `JOURNAL.md` is 78 KB / 11 entries
and is over the 30 KB target, but rotation is STEP 4 work and this run never
reached STEP 4. The next win or linux run should do it.

**One caveat for Jeff before he runs the command**, carried over from the linux
entry: his interactive pushes then resolve through `gh`'s keyring token. If that
token lacks `workflow` scope, a commit touching `.github/workflows/**` is
rejected until `gh auth refresh -h github.com -s workflow` is run once. This did
not bite on Windows (its token already had the scope); **unverified here** — I
did not inspect Jeff's keyring scopes, since doing so is outside a halted run.

**Next:**

1. **Jeff: run the one `git config` line above on this box.** Until then macOS
   contributes nothing and S6 stays untouched.
2. **Rewrite A11's mac line** to say it needs an interactive session, not a
   scheduled run, and record the deadlock above so the next person does not
   re-file it as agent work. Consider whether STEP 0.7 should let a run apply
   *this specific* config repair — I think not, but it should be a decision
   rather than an accident.
3. **`mac`'s NEXT stays S6**, untouched and still eligible, for the first mac
   run after the fix lands.

### Resolution — same session, Jeff applied the fix

Jeff ran the one line on this box while the session was still open. Everything
above stands as written; this is what changed after it.

**STEP 0.7 re-run, both assertions:**

| command | required | actual |
|---|---|---|
| `gh api user --jq .login` | `tide-rack-bot` | `tide-rack-bot` ✅ |
| `git config --global --get url."https://github.com/".insteadOf` | `git@github.com:` | `git@github.com:` ✅ |

**A11's acceptance test, in full, on mac:**

- All **nine fleet repos** still resolve `https://` (`ls-remote --get-url origin`
  — note this applies `insteadOf` rewriting, so it is testing the post-fix path).
- **The safeguard itself demonstrably works**, which is the part the sweep alone
  cannot show: feeding git an explicit `git@github.com:JeffMcClintock/TideSynth.git`
  now resolves to `https://github.com/JeffMcClintock/TideSynth.git`. That is the
  future-SSH-clone hole closed, not merely absent.
- **Three-way proof re-run post-fix, unchanged:** bogus token → auth fails; real
  bot token → succeeds as `tide-rack-bot`; no token → succeeds as
  `JeffMcClintock`.

**So A11 is DONE — linux 2026-08-13, win 2026-08-13, mac 2026-08-13.** Row
flipped in this PR.

**The deadlock finding is not retired by this.** It was resolved by a human
happening to be at the keyboard, which is exactly the circumstance a *scheduled*
run does not have. Had this fired unattended at 03:00, the box would have sat
halted for a week and every subsequent mac run would have halted identically.
The general shape is worth keeping in view: **STEP 0.7 can put a box into a
state that only an interactive session can clear, and nothing in the fleet
notices or escalates.** A6's watchdog digest is the natural place to surface a
box that halted, and does not do so today. Filed as **A12**.

**Branch/PR:** `tide/mac/A11-step07-halt` — TideSynth only, docs only, no code.
The branch is named for the halt that produced it; **S6, the item this run went
on to take, is on its own branch** (see part 2). A later mac run should not treat
this branch as work-in-progress to resume.
---

## 2026-08-13 — windows — A11, win half (interactive session, Jeff directing)

**Did:** Checked this box against the SSH-remote gap the linux run found in
A2/A11: swept every local git repo under `C:\SE` (`find C:\SE -maxdepth 2
-name .git`, 22 repos — not just the fleet's usual 5) for its remote
protocol, then applied the global `url."https://github.com/".insteadOf
"git@github.com:"` rewrite and proved it against the private `SynthEdit`
repo the way A11's acceptance test specifies.

**Result:** All 22 repos on Windows were already HTTPS — nothing here was
ever actually exposed, unlike linux's 8-of-9. Applied the rewrite anyway,
since the acceptance test wants it as a structural safeguard, not just a
reaction to today's state. Three-part proof, all against
`https://github.com/JeffMcClintock/SynthEdit.git` (private): bogus
`GH_TOKEN` → `fatal: Authentication failed for
'https://github.com/JeffMcClintock/SynthEdit.git/'` (exit 128); real bot
token (from `~/.tide/agent-token`) → `git ls-remote` succeeds, `gh api user`
confirms `tide-rack-bot`; no `GH_TOKEN` → succeeds, confirms `JeffMcClintock`.
Also checked the caveat the linux fix flagged for Jeff's own workflow-file
access (`gh auth refresh -s workflow` needed once the rewrite lands) — did
not apply here, this box's `gh auth status` already shows `workflow` in
scope.

**Learned:** The fleet's "5 repos" framing (used everywhere A2 discusses
scope) undercounts what's actually on disk — Windows alone has 22 local git
repos under `C:\SE`, most unrelated to TIDE (SE15, SSG, Waves, and other
dormant product repos). The SSH-remote risk is about *any* repo the box's
git config touches, not just the ones the bot has a token for, so the sweep
has to be exhaustive (`find`, not "check the 5 I know about") the way linux's
was.

**Next:** mac remains outstanding — its A2 evidence is still authorship-only,
not authentication-verified. A11 stays TODO until mac's sweep and proof are
done too.

**Branch/PR:** none — committed directly to `main`, interactive session.

---

## 2026-08-13 — linux — A11 (new; A2 follow-up, interactive session, Jeff directing)

**Did:** Jeff asked for help finishing **A2** on this box. A2 had been flipped
**DONE** on all three boxes earlier the same day, but the row I started from was
the pre-flip one — my local `main` was 30+ commits stale, which is how I came at
it fresh. Steps 1–2 were already in place here since 2026-08-09 and I re-verified
rather than assumed them. Then checked the remotes, which nobody had, and found
the mechanism does not reach most of them. Fixed this box, corrected
[docs/a2-actor-separation.md](docs/a2-actor-separation.md), added setup **step 3**
and a second STEP 0.7 assertion to
[docs/weekly-run-prompt.md](docs/weekly-run-prompt.md), filed **A11** for the
remaining boxes.

**Result:** every command run, none assumed.

| Check | Result |
|---|---|
| `gh api user --jq .login` with `~/.tide/agent-token` | `tide-rack-bot` |
| token scopes | `repo` — no `workflow`, as intended |
| bot reads private `SynthEdit` | yes |
| `credential.https://github.com.helper` | both lines present |
| remotes on this box | **8 of 9 SSH**, only `TideSynth` HTTPS |
| `ssh -T git@github.com` | `Hi JeffMcClintock!` |

**Learned — three things, and the second is the one that matters.**

**1. A credential helper keyed to `credential.https://github.com.helper` is never
consulted for a `git@github.com:` URL.** So on any repo with an SSH remote, a run
authenticates with Jeff's key and pushes through his admin bypass. Fixed with a
global `url."https://github.com/".insteadOf "git@github.com:"`, not per-repo
`remote set-url`: a fresh clone defaults back to SSH and re-opens the hole with
no signal. Proved read-only in both directions, no push — against the private
`SynthEdit` over HTTPS, a bogus `GH_TOKEN` **fails** auth (that failure is the
proof git consults `gh` at all, rather than the SSH key or the keyring), the real
token succeeds, no token succeeds as Jeff. Cheaper than the Windows method, which
needed a real push to `main` to see `GH013`.

**2. Both guards A2 rests on are blind to it, including the one used to close
it.** STEP 0.7's `gh api user` answers `tide-rack-bot` because `gh`'s API path
reads `GH_TOKEN` and never touches git's transport. And the PR-authorship check
A2 was flipped DONE on answers `tide-rack-bot` because author and committer come
from the four `GIT_*` exports, which STEP 0.7 sets unconditionally. **Authorship
proves authorship, not authentication.** A push made as Jeff over SSH arrives
stamped bot, past an assertion that passes, into a log that reads correctly.
Linux looked clean for one accidental reason: `TideSynth` was the only repo its
runs touched and the only repo that was HTTPS. **The macOS evidence A2 cited —
`gmpi_ui#3`/`#4`, `GMPI_Wrappers#1`/`#2` — is drawn entirely from repos that were
SSH on this box**, and no one has looked at mac's or win's remotes. That is A11.

**3. A stale local `main` makes the bot's first push fail with an error about a
file you did not touch.** My first push was rejected with *"refusing to allow a
Personal Access Token to create or update workflow `.github/workflows/build.yml`
without `workflow` scope"* on a commit touching three `.md` files. Cause: the
branch was cut from a `main` 30+ commits behind, and `1157be3` had since changed
`build.yml`, so relative to `origin/main` the branch *reverted* two workflow files
— which needs the scope the bot deliberately does not have and never will.
**Diagnose with `git diff origin/main HEAD -- .github/`, not by reading your own
commit**, which shows nothing. `git fetch` + rebase clears it. This will recur on
any box whose `main` has drifted, and the message points at the wrong thing every
time.

**Next:** **A11 on mac and win** — one `git config` line each, then the two
assertions. Until then their A2 "verified" means *authorship verified,
authentication unknown*. Jeff needs `gh auth refresh -h github.com -s workflow`
once for his own interactive pushes, now that they resolve through `gh`'s keyring
token: `SE16` has nine workflow files. Note this is the same scope wall **C9(a)**
hit from the other direction.

**Prompt:** n/a — interactive session, not a scheduled run. Steps 1–2 were already
in place, so the work itself ran as Jeff until step 3 landed; the commit and push
below are the first exercise of the fixed path on this box.

**Branch/PR:** `tide/linux/A2-ssh-remote-gap` — TideSynth only. No other repo was
committed in; the box-level `git config` is not a repo change and lives nowhere
but this machine, which is exactly why A11 has to be done per box.

---

## 2026-08-13 — jeff — decision: rack mode is TIDE's default view (interactive session, not a scheduled run)

**Did:** Jeff described SynthEdit's new "rack mode" — the top-level Panel View
renders as a Eurorack case, modules and Containers drag-and-snap into rack
slots — and ruled that in TIDE this becomes the *only* top-level option, not
one of two. Unlocking a module/Container opens its own structure view to
rewire signal flow. Rewrote PLAN.md constraint 1 to match (was: structure
view only, "No panel view"; now: rack is default, structure view is the
unlock drill-down) and recorded the ruling in docs/decisions.md.

**Result:** PLAN.md constraint 1 and its "One sentence" summary rewritten;
decisions.md carries the ruling and its reasoning (closer to Cardinal, with
per-module signal-flow editing added on top). Not yet touched, and flagged
as open follow-ups rather than silently assumed: whether the v0.1 acceptance
test should now be rack-first (currently still says "shows a structure
view..."), and whether BACKLOG U1 needs rescoping around the rack as default.

**Learned:** The underlying SynthEdit feature already exists — `SE16`
`a056d3f5b chore(se) : experimental eurorack 'rack mode' for the panel
view`, from earlier this same day — so this ruling is catching up to code
already landing, not speculating ahead of it. Also: the 2026-08-09 Eurorack
section of PLAN.md already stated "opening a Container is optional" as the
product's differentiator; today's ruling is the concrete mechanism that
fulfils that, and constraint 1's literal wording ("No panel view") was the
one place still contradicting it.

**Next:** Decide the v0.1 acceptance-test wording and U1's scope before
either becomes stale in the same way constraint 1 just was.

**Branch/PR:** none — committed directly to `main`, interactive session.

---

## 2026-08-13 — windows — C3

**Prompt:** `e09e766` · claude-opus-5[1m] · app Claude Code (Agent SDK harness) · as `tide-rack-bot`

**Did:** Moved the document model into the public repo — carve-out stage C3.
27 files leave `SE16/SynthEdit2/` for the **root** of `SynthEditLib`: `DocOb`,
`CContainer`, `CUG`(+`_with_patches`), `Plug`, `Plug4`, `PlugIO4`,
`PlugDescriptionDecorator`, `Plug_decorator_{autoduplicate,namable,sdk2,vst}`,
`SynthEditDocBase`, `SynthEditDoc2`. `EditorLib/CMakeLists.txt` repointed
(27 entries, `${EDITOR_DIR}`/`${EDITOR2_DIR}` → `${SYNTHEDITLIB_DIR}`);
`SynthEdit2.vcxproj` + `.filters` repointed for `SynthEditDoc2`.
PRs: [SynthEditLib#5](https://github.com/JeffMcClintock/SynthEditLib/pull/5),
[SynthEdit#11](https://github.com/JeffMcClintock/SynthEdit/pull/11) — **they must
merge together.** Also flipped **P7 → DONE** (both its PRs merged) and filed
**C9** and **C10**.

**Result:** Release x64 on this box — `EditorLib.lib`, `SynthEditCL.exe`,
`TIDE_VST3.vst3` and `SynthEdit2.exe` all build; `ctest -C Release` **92/92
passed, 0 failed**. `SynthEdit2` built via P8's recipe
(`MSBuild SynthEditStore.sln -t:SynthEdit2 -p:Configuration=Release -p:Platform=x64`),
which is what exercises the `.vcxproj` edit. **Positive control**, because
"it still builds" after a move proves nothing on its own: renaming
`C:\SE\SynthEditLib\DocOb.cpp` aside makes the build fail with
`error C1083: Cannot open source file: 'C:\SE\SynthEditLib\DocOb.cpp'`, and
restoring it builds clean — so the build genuinely reads the new location.
**26 of 27 files byte-identical** to the originals (SHA-256 per file, line
endings normalised). mac / iOS / linux **unverified** — not buildable here.

**Default branch:** SE16 master was also built standalone after restoring the
checkout, not merely inferred from the branch build — EditorLib.lib and
SynthEdit2.exe both build clean at 7cb95f33b. So this stage did not break a
working master, and master was not already broken before it. No
platform-labelled issue was needed.

**Learned:**

- **C2's "nothing outside EditorLib compiles it" test caught exactly one file,
  and it was not obvious.** `SynthEditDoc2.cpp` is compiled by
  `SynthEdit2.vcxproj` as well as by EditorLib — by a path relative to
  `SynthEdit2/`, so the move would have broken the WinUI3 app while EditorLib
  and TIDE carried on building fine. The grep that finds this is over
  `*.vcxproj`/`*.filters`/`*.pbxproj`/`CMakeLists.txt`/`*.cmake`/`*.yml` for each
  candidate basename. **Run it at C4 and C5.** (Fix: the entries now read
  `..\..\SynthEditLib\`, matching how that project already references
  `..\..\SynthEditLib\modules\se_sdk3_hosting\BundleInfo.cpp`.)
- **The `#include "../` check needs a second step C2's note did not state.**
  Grepping is not enough — you have to test whether each target *exists* at that
  relative path. Eight hits across the moved set; seven
  (`../tinyXml2/tinyxml2.h`, `../se_sdk3_hosting/GmpiResourceManager.h`) point at
  directories that **do not exist** under `SE16/`, so they were always resolving
  through the search path and move harmlessly, exactly as C2 found for
  `checkpoint.h`. The eighth, `../se_build_number.h`, **does** exist — and that
  is the only one that matters. So: `test -e` each one; the harmless majority is
  noise and the single real hit is the whole finding.
- **`se_build_number.h` blocks C4 and C5 — filed as C9.** It is SynthEdit's
  product version, at the private repo's root, bumped by `[Build-Machine]`, and
  read by three release workflows at that path. C3 escaped by luck: its one
  includer, `SynthEditDocBase.cpp`, **never used the macros** (zero occurrences
  of `SE_MAJOR_VERSION`/`SE_MINOR_VERSION`/`SE_BUILD_NUMBER`), so the fix was
  deleting a dead line — that is the 27th file, the sole content change in the
  whole stage. The other four includers are live uses:
  `ModuleFactory_Editor.cpp` and `SkinMgr.cpp` (**C4**), `Application.cpp`
  (**C5**), `ExportAsPlugin.cpp` (stays private, fine). Moving the header needs
  a `.github/workflows/**` edit, which a scheduled run **cannot** do — the bot
  token deliberately lacks `workflow` scope. **This needs a decision before C4
  starts, or C4 decides it by accident.**
- **Root vs subfolder: chose root, deliberately, and filed the re-home as C10
  blocked on C6.** Root is already an include dir in all three build systems, so
  the move cost zero include-path edits — which is what keeps breakage on the
  riskiest stage unambiguously about the move. A subfolder now would smear one
  include-path change across C3/C4/C5 and several build systems, including the
  macOS/iOS ones this box cannot verify. After C6 the same change is **one line**
  in an `EditorLib/CMakeLists.txt` that by then lives in `SynthEditLib`.
- **The two repos normalise line endings differently, so blob comparison across
  them is worthless.** `SE16` has `.gitattributes` `* text=auto` (LF in the
  blob); `SynthEditLib` has no `.gitattributes` and `core.autocrlf=false`, so it
  stores **CRLF** — and C2's `checkpoint.cpp` is CRLF there too, so the new files
  match precedent. Compare with `tr -d '\r'` or every file reads as 100% changed.
  Related: `sed -i` on a CRLF file silently rewrites the whole file to LF; the
  tell is `diff` reporting `1,197c1,197`.
- **P3 partly moved out from under itself.** `CContainer.cpp` carried its
  `#include "afxres.h"` unaltered into the public repo, so the MFC requirement is
  public the moment SynthEditLib#5 merges. Row updated with the new path.

**Jeff was working in `SE16` throughout this run, and the two of us collided on
the index. Nothing was lost, and he resolved his half himself.** Worth reading
in full, because the failure mode is not obvious and it will recur.

`SE16` was clean at claim time. Partway through, `d4d0acac5 se_screenshot:
report contentRect, and optionally crop to it` (Jeff McClintock, 08:39 +1200)
appeared **on `tide/win/C3-document-model`** — his tooling committed to whatever
branch was checked out, which was this run's — and it swept **this run's 27
staged deletions** in from the index alongside its own 3 `SynthEditMcp` files.
This run did not revert or rewrite it: it preserved that commit on a local
branch, then `git reset --mixed origin/master` unwound the **index only**,
leaving every file on disk byte-for-byte (the three `SynthEditMcp` files were
SHA-256'd before and after, unchanged), which put his work back to uncommitted
changes exactly as the tree was found. Only this run's own 30 paths were then
staged, by name.

**He then sorted it out himself, while this run was writing PRs:** he created
**`jeff/mcp-screenshot-contentrect`** and committed his work cleanly there as
`d1b403000` — 3 files, no C3 deletions. Verified byte-identical (blob hashes) to
what the safety branch held, so that safety branch was pure redundancy and was
deleted; his own branch is the live copy. **His `png.ts` work is committed and
safe, and is *not* on `master`.**

**One thing he should know: `jeff/mcp-screenshot-contentrect` is based on
`ae4b434df`, this run's C3 commit — not on `master`.** So merging that branch as
it stands would drag C3 in with it. His commit is cleanly separable (a
cherry-pick onto `master` touches only the 3 `SynthEditMcp` files). He also
pushed `97497580a` and `e4216d0d9` to `master` during the run, so `origin/master`
moved twice more; local `master` is left 3 behind, as found — not fast-forwarded,
since that is his call.

**The transferable lesson:** a run whose staged index sits idle through a long
build can have that index harvested by someone else's commit. The deletions were
staged, then four target builds and a 92-test ctest ran — a wide window.
**Stage late, and re-check `git status` immediately before `git commit`**; that
check is the only reason this was caught rather than shipped inside someone
else's commit.

**Next:**

1. **Merge SynthEditLib#5 and SynthEdit#11 together.** Either alone breaks the
   build. Then flip C3 to DONE and C4 unblocks.
2. **Answer C9 before C4 starts.** Recommended option (c): give `SynthEditLib`
   its own version header, or pass the version in as a compile definition, and
   leave `se_build_number.h` where SynthEdit's workflows expect it. Option (b)
   (add SE16's root to the include path) fails at C7 by construction.
3. NEXT for win moved C3 → **P3**, since C4 is `BLOCKED(C3)` until the merge.
4. Still true from the 2026-08-12 windows run: `A3`/`A5`/`A6` can never be done
   by a scheduled run — all three edit `.github/workflows/`.

**Branch/PR:** `tide/win/C3-document-model` in all three repos —
[TideSynth PR](https://github.com/JeffMcClintock/TideSynth/pull/38),
[SynthEditLib#5](https://github.com/JeffMcClintock/SynthEditLib/pull/5),
[SynthEdit#11](https://github.com/JeffMcClintock/SynthEdit/pull/11).

---

## 2026-08-13 — macos — P7b

**Did:** Fixed **P7b** — `DrawingFrameCocoa::onRender` using `backBuffer` after
the re-entrant `drawingClient->render()` call without re-checking it. One guard
in `gmpi_ui/backends/DrawingFrameMac.mm`, plus a regression test that reproduces
the defect first: `gmpi_ui/tests/mac_render_reentrant_resize.mm`, built and run
by `tests/run_mac_render_test.sh`.

**Result — it is a real use-after-free, not a latent one, and the row named the
wrong line.**

P7 filed this as "latent, not demonstrated: it needs a client that resizes
during render and none currently does". Both halves were right, and neither
prevents a test: no *shipping* client does it, but a synthetic `IDrawingClient`
does it in five lines. Unfixed sources die on the first such paint.

The correction that matters, because it moves the fix:

| | the row said | measured |
|---|---|---|
| faulting call | `CGContextRestoreGState(backBuffer)` / `CGBitmapContextCreateImage(backBuffer)`, *after* the block | `context.popAxisAlignedClip()`, **inside** the block |
| why | both use `backBuffer` after `render()` | both read the **member**, which `onResize` sets to `nullptr` — they pass CoreGraphics a NULL, which is untidy, not a fault |
| the real one | — | `gmpi::cocoa::GraphicsContext` keeps its **own** copy in `cgContext_` from `setCGContext` time, and nothing nulls that copy |

So the guard has to sit immediately after `render()` returns, not after the
scope closes. A one-line fix placed where the row pointed would have changed
nothing and looked correct.

**Verification artifact — A/B, 3 runs each, same binary, same machine:**

| sources | result |
|---|---|
| unfixed (`git stash` of the guard only) | **SIGSEGV 3/3**, exit 139 |
| fixed | **exit 0, PASS 3/3** |

The unfixed crash report backtrace, which is what makes it the *right* crash and
not just a crash:

```
CGContextRestoreGState                                   (CoreGraphics)
gmpi::cocoa::GraphicsContext::popAxisAlignedClip()
DrawingFrameCocoa::onRender(NSView*, gmpi::drawing::Rect*)
-[GMPI_VIEW_VERSION_03 drawRect:]
```

`EXC_BAD_ACCESS (SIGSEGV)`, `KERN_INVALID_ADDRESS`. Liveness, copied from the
P7 harness's discipline rather than assumed: the renderer drew 2 distinct
colours *before* the re-entrant resize and 3 *after* it, so "survived" cannot
mean "never ran", and the frame provably recovers at the new size.

**And the existing harness still passes, which is the other half of the claim.**
A new test proving the new guard works says nothing about whether ordinary
painting still does. P7's `mac_editor_resize_host`, built standalone against the
VST3 SDK and run on the `TIDE_VST3` this build produced: **exit 0, 3/3**, editor
live, survived every oversized resize+paint, still drawing at the end. Visible
in its output as a by-product: P7a's clamp is live in this binary —
`onSize(0, 0, 16385, 600)` lands the child at **8192 x 600**.

**Learned:**

- **AddressSanitizer cannot see this, and an ASan-only run is a confident false
  PASS.** The freed read happens inside CoreGraphics; ASan checks loads the
  compiler instrumented plus the functions it intercepts, and a system framework
  is neither. I found this the honest way — my first harness was ASan-only and
  reported PASS on the unfixed sources. The positive control is what settled it:
  the same ASan binary flags a *hand-written* read of the same freed pointer
  immediately (`heap-use-after-free ... freed by _CFRelease`), so ASan was
  tracking the allocation and simply never sees the read. **Guard Malloc**
  (`DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib`) is the detector that
  works: the page is unmapped, so whoever touches it faults. The test script
  defaults to it and says all this at the top.
- **The measurement bug from the P7 entry recurred, in the same shape.** First
  version of the client drew one rect near the drawing origin and the liveness
  probe reported 1 distinct colour — because `onRender` flips to top-down while
  AppKit's `visibleRect` origin is the bottom-left, so the sampled tile landed
  where the rect was not. Fixed by drawing stripes over the whole arranged rect
  rather than moving the probe: the count then does not depend on where either
  the client or the sampler happens to look. **Anyone porting a paint probe to
  Cocoa should expect to hit this once.**
- **The whole test needs no CMake, no VST3 and no plugin.** `DrawingFrameMac.mm`
  plus `DrawingFrameCommon.cpp` compile and link standalone in one `clang++`
  line (`-fno-objc-arc`; the backend's Objective-C is manually reference
  counted, `MacColorDialog.h` calls `-retain`). That is a much cheaper harness
  than P7's, and the right shape whenever the defect is inside `gmpi_ui` itself.
  It follows the convention `gmpi_ui/tests/` already uses — a shell script that
  invokes the compiler, not a build system.
- **`GraphicsContext` caching `cgContext_` is a general hazard, not a P7b
  detail.** Any backend that hands a client a context object holding a raw
  device pointer has the same shape. I did not widen the fix to make
  `setCGContext(nullptr)` reachable from `onResize` — that touches the
  cross-platform class every GMPI plugin uses, and the item is one guard. Worth
  a row if anyone finds a second instance.

**Build health — verified, not assumed.** Fresh Ninja configure of `SynthEdit`
with all four local overrides into a scratch build dir (the tree itself was not
touched), `ninja` with no target: **RC=0** across `SynthEdit_VST3`,
`SynthEdit_GMPI`, `TIDE`, `TIDE_VST3` and `SynthEditCL`. So the standing
direction — leave SynthEdit, SynthEditCL and TIDE all building — is honoured and
checked. This corroborates the P7a run's finding that SynthEditCL *does* build on
macOS with the Ninja generator; **P6 is still not closed by that**, for P7a's
reason: P6's failure is a `CodeSign` step the Ninja generator never emits.

**STEP 1 / 1.5 — what I found before picking an item:**

- **No `platform:mac` issues; no open issues at all** in TideSynth.
- **P7a is complete and not mine to redo.** The NEXT block on `main` still points
  `mac` at P7a, but its two code PRs — [gmpi_ui#3](https://github.com/JeffMcClintock/gmpi_ui/pull/3)
  and [GMPI_Wrappers#2](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/2) —
  **merged on 2026-08-12**, and only its docs PR [#35](https://github.com/JeffMcClintock/TideSynth/pull/35)
  is open, with no reviews and no comments. That PR itself moves the pointer to
  P7b. Under STEP 1.5 a PR with nothing unresolved is waiting for merge, not for
  me, so I left it alone and took P7b — the item #35 nominates.
- **The red-checks rule is still unusable, exactly as the C8 entry reported.**
  #35's head and `main` fail identically on all three platforms; that is the
  documented pre-C7 failure, not a signal. **B1** remains the row that fixes it.
- **P7 is now flippable and I flipped it**, in place: both its linked PRs have
  merged ([GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1)
  2026-08-12, [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) 2026-08-10).
  I did **not** move the row to the Done section: [#36](https://github.com/JeffMcClintock/TideSynth/pull/36)
  is rotating landed rows into `BACKLOG-DONE.md` and a move here would collide
  with it for no gain.
- **The `docs/p7-resize-audit-mac-x11.md` correction is appended at the end of
  the file, not written into the follow-ups table**, deliberately: #35 is editing
  that table right now. Appending keeps both merges clean; the table's P7b line
  stays wrong until someone rebases, and the postscript says so in as many words.

**Expect conflicts, and here is how they resolve.** Three PRs are open against
`main` and all three edit `JOURNAL.md` and `BACKLOG.md`:

| PR | BACKLOG rows it touches | overlap with this one |
|---|---|---|
| [#34](https://github.com/JeffMcClintock/TideSynth/pull/34) E1 | E1 | none |
| [#35](https://github.com/JeffMcClintock/TideSynth/pull/35) P7a | NEXT block, P7a, P6 | **adjacent** — P7a is the line between my P7 and P7b edits |
| [#36](https://github.com/JeffMcClintock/TideSynth/pull/36) A8 | rotates 24 landed rows out | P7 flip may need re-applying after its rotation |

`JOURNAL.md` conflicts by construction — every entry inserts at the top. The
resolution is the one Jeff already used for #31/#32: keep both entries, newest
first. For BACKLOG, my three edits (P7 status, P7b row, nothing else) are
disjoint from every other PR's *content*; only their line adjacency conflicts.
**I did not touch the NEXT block** — #35 already moves `mac` to P7b, and once
both land P7b reads IN-REVIEW, which STEP 2 treats as ineligible and falls
through cleanly.

**Next:** `mac` has no obvious next row once P7b lands — P7c is `linux`, P7d is
`any` but is a scope question for Jeff (`GMPI-plugins` is GATED by default), and
the NEXT block will be pointing at an IN-REVIEW item. **Recommend Jeff sets the
`mac` pointer explicitly**, or answers P7d, which would make it takeable in
minutes. Independently: **B1** keeps costing every run real time, and **A6**'s
digest would have told me in one line what I spent this run's first twenty
minutes deriving from the API.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/P7b-render-backbuffer-guard` in both repos —
[gmpi_ui#4](https://github.com/JeffMcClintock/gmpi_ui/pull/4) (guard + test) and
[#37](https://github.com/JeffMcClintock/TideSynth/pull/37) (docs). **They must merge together in the sense that matters:** the gmpi_ui PR
carries the fix and the test, this one carries only docs, so either order builds
— but merging the docs alone would leave the backlog saying a fix landed that
did not.

---

## 2026-08-12 — windows — A8 (C3 not taken — see below)

**Did:** Rotated the journal, archived the landed backlog rows, distilled the
five largest live rows into `docs/`, and wrote a link checker to prove none of it
stranded a reference. **I did not take C3, which the NEXT block points win at** —
that first, since it is what affects the next run.

**Why C3 was not taken.** SE16 was dirty with **Jeff's live work** — an
experimental Eurorack `rackMode` across `SynthEdit2/MfcDocPresenter.{cpp,h}`,
`SynthEdit2/SynthEditDocBase.h` and three files in
`SynthEditLib/modules/se_sdk3_hosting/`; real content, not CRLF churn.
**`SynthEditDocBase.h` is one of the six files C3 must `git mv`**, and a move
either commits his uncommitted line or destroys it — STEP 5's third dirt
category forbids both. Full detail is now a **precondition on the C3 row**, and
on C4 by implication, since the `MfcDocPresenter` pair is C4's scope.

**It was transient, and it cleared during this run.** Jeff committed the work at
10:51 — SE16 `a056d3f5b`, SynthEditLib `5c6bc1b` — so by the end of the run both
trees were clean and **C3 is unblocked again**. I did not switch to it: A8 was
already claimed, built and in review, and STEP 2 says one item. Re-check rather
than trusting either state; this is a working machine and it moves under you.

**A3, the row above A8, was equally unavailable** — as are A5 and A6. All three
edit `.github/workflows/` and the bot's token deliberately lacks `workflow`
scope. The credential is enforcing the rule as designed, but it means **the
`any` NEXT pointer (A3) can never be satisfied by a scheduled run.**

**Result** — A8 asked for "under 30 KB after rotation, no broken links":

| | before | after |
|---|---|---|
| `JOURNAL.md` | 192,010 B / 37 entries | **25,209 B / 3 entries** (+ this one) |
| `BACKLOG.md` | 76,304 B / 76 rows | **45,003 B / 52 rows** |
| broken links | 4 | **0** of 164 relative links |

New: `JOURNAL-2026-08.md` (34 entries), `BACKLOG-DONE.md` (24 rows),
`scripts/check-links.py`, 5 `docs/<id>.md`.

**Verification artifacts, this session:**

- **Rotation is lossless.** Re-parsed both files, concatenated live + archive,
  compared to the pre-rotation file: **37 entries before, 37 after, order and
  headings identical, every body byte-identical** by SHA-256.
- **The five lifted rows are verbatim** — whitespace-normalised comparison
  against each new `docs/` file. Only line breaks are new.
- **`scripts/check-links.py`, with a positive control.** Clean: 164 links, 0
  broken, exit 0. One bad link appended: exit 1, naming `BACKLOG.md:174`.
- **C8 flipped to DONE on evidence:** `SynthEditLib#4` merged
  2026-08-11T22:23:32Z, `SynthEdit#10` 22:22:56Z, both confirmed via the API.
  **P7 stays IN-REVIEW** — `GMPI_Wrappers#1` is still open.

**Learned:**

- **Lifting a row from a root-level file into `docs/` silently breaks its
  relative links.** Three of the five (`A2`, `N1`, `S1b`) carried `](docs/…)`
  links, correct in `BACKLOG.md` and wrong one directory down — **verbatim is
  faithful, not safe.** That is the whole argument for the checker; it caught all
  three, plus a pre-existing break in `docs/distribution.md:6`.
- **The rotation rule needs a floor, and the floor has to win.** "Under 30 KB"
  and "the last four entries must be readable" genuinely conflict here: three of
  the four retained entries are 3.7–10.3 KB, so this file lands **just under
  30 KiB, above a decimal 30,000**. I kept the floor rather than archive a
  fourth entry: a size rule that starves the handoff is worse than a marginally
  large file. That precedence is now written into the rule above.
- **A grooming item conflicts with every open PR by construction**, so the only
  mitigation is choosing what *not* to touch. #34 edits `E1` and #35 edits
  `P7a`/`P6`, so I distilled neither (I had lifted `E1` and reverted it —
  #34 adds a near-identically-named `docs/e1-verification-harness.md`).
  **Merge #34 and #35 first**; the journal hunk then resolves as: my header,
  their entries, my kept entries.
- **The `## Blocked on Jeff` section held nothing blocked on Jeff** — all six
  rows were `RESOLVED`. A section titled "agents must not start these" holding
  only settled history is a small trap; archived.
- **STEP 0.5 requires an app version this box does not expose.** No
  `AppData\Local\AnthropicClaude`, and the only version string under
  `AppData\Local\Claude\Logs` is the Chrome native host's. The line below says
  "undetermined" rather than copying the last entry's number.

**Build health:** nothing built, no code changed — this run touched **TideSynth
only**. **SE16 and SynthEditLib were left exactly as found** (dirty when I read
them, then committed by Jeff, not by me); `gmpi_ui` and `GMPI_Wrappers` were
clean and untouched. All five checkouts end on their default branches.

**STEP 1 / 1.5:** no open issues in TideSynth at all, so no `platform:win`
issue, and no open `tide/win/**` PR. #34 (linux/E1) and #35 (mac/P7a) are not mine. Per
the C8 entry, CI red is uninformative here until C7.

**Next:** merge **#34** and **#35**, then this PR after a rebase. Then win takes
**C3**, which is clean and unblocked as of 10:51 — but check the tree again, and
expect `rackMode` to keep moving in the files C3 and C4 must relocate. If it is
dirty again, the fallbacks are **S1b** and **P3**, and P3 has the same
precondition (`MfcDocPresenter.cpp`). **A3/A5/A6 need a `workflow`-scoped
credential or Jeff.**

**Prompt:** `e09e766` · claude-opus-5[1m] · app version undetermined on this box
· as `tide-rack-bot`

**Branch/PR:** [#36](https://github.com/JeffMcClintock/TideSynth/pull/36) on
`tide/win/A8-journal-rotation` — TideSynth only, no other repo committed in.

---

## 2026-08-12 — macos — P7a

**Did:** Took **P7a** and did both halves. Bounded the macOS editor extent in
`gmpi_ui/backends/DrawingFrameMac.mm`, and made `checkSizeConstraint` in
`GMPI_Wrappers/wrapper/VST3/SEVSTGUIEditorMac.cpp` write the accepted size back.
Both files were on STEP 5's ALLOWED list, so nothing needed escalating this time.

**The numbers, since choosing them was the judgement call.** P7 filed this row
precisely because it would not pick one: CoreGraphics has no wall to copy, so any
bound is a product decision about how much an editor may reserve. I picked from
**displays**, not from a graphics API:

| constant | value | why |
|---|---|---|
| `maxEditorDimensionPoints` | **8192** points/axis | the widest single Mac display in logical points is a 6K Pro Display XDR in "more space" at **3840**, so this is a bit over twice the largest real case |
| `maxBackingBitmapBytes` | **384 MiB** | the bitmap is 8 bytes/px at *backing* resolution, so a full-screen editor on that same display at 2x reserves **~265 MiB**; 384 clears it with headroom and still bites on every rect P7 exercised |

Aspect ratio is preserved when the area budget bites. **Both numbers are Jeff's
to overrule** — they live in one place with the reasoning beside them for exactly
that reason, and the PR body says so.

Why not one bound instead of two: a per-axis limit alone does not bound memory
(8192² points at 2x is 8.6 TB of reservation), and an area budget alone lets an
absurdly-shaped 16385 x 600 through. Each catches what the other misses. And
Windows' 16384 was rejected on the merits — at that per-axis limit the audit's own
`16385 x 600` case still costs +315 MiB, so copying it would have "fixed" the row
while leaving the measured defect standing.

**Clamped in two places on purpose.** `resizeNativeView` is the wrapper's path.
`initBackingBitmap` is where the memory is actually reserved and is reachable
*without* the wrapper — a host can set the view's frame directly, and
`createNativeView`'s own comment says JUCE does exactly that. If the second site
bites, the bitmap is smaller than the view and the blit at the end of `onRender`
stretches it: a blurry editor at an absurd extent, which is the intended trade and
is commented as such.

**Result — verified A/B, same plugin, both Debug, same machine.** "Before" is
`GainGui_VST3` built from a throwaway worktree at unmodified `gmpi_ui` +
`GMPI_Wrappers`, so the only difference between the columns is this change.

| host asked | before: view adopted | after: view adopted |
|---|---|---|
| `2178 x 32672` | `2178 x 32672` | **`1829 x 6879`** |
| `0 x 0` | `0 x 0` | **`1 x 1`** |
| `16385 x 600` | `16385 x 600` | **`8192 x 600`** |
| `600 x 16385` | `600 x 16385` | **`600 x 8192`** |
| recover `200 x 200` | `200 x 200`, 48 colours | `200 x 200`, 48 colours |

| paint | before | after |
|---|---|---|
| `16385 x 600` | **+253.4 MiB** | **+128.3 MiB** |
| `2178 x 32672` | +36.6 MiB | +31.3 MiB |
| peak resident | 612.8 MiB | 362.5 MiB |

**The before column reproduced P7's `+253 MiB` figure to within 0.4 MiB.** That is
the positive control, and it is worth more than the after column: it says the
harness and this machine still measure what they measured two days ago, so the
delta is the change and not the weather.

`checkSizeConstraint(0, 0, 2178, 32672)`, the row's stated acceptance observable:

| plugin | before | after |
|---|---|---|
| `GainGui_VST3` (resizable) | `kResultTrue`, **UNCHANGED** | `kResultTrue`, **`1829 x 6879`** |
| `TIDE_VST3` (fixed size) | `kResultFalse`, **UNCHANGED** | `kResultTrue`, **`1829 x 600`** |

`mac_editor_resize_host` exits **0, 3/3** on GainGui and 1/1 on a TIDE_VST3 built
against the change, live before (liveness A+B, 19–65 distinct colours) and still
drawing after recovery — so "passed because the clamp made resize a no-op" is
excluded. **Negative control:** `checkSizeConstraint(0, 0, 640, 480)` still comes
back `kResultTrue` with the rect *unchanged*, so an in-bounds size is accepted
as-is rather than spuriously adjusted. I added that control because every rect the
harness tests by default is an absurd one, and a clamp that mangled legitimate
sizes would have passed the whole suite.

**Build health — better than the standing rule expects, and this is the run's
second finding.** Configured a fresh Ninja build of `SynthEdit` with **all four**
local overrides (banner confirmed) and ran `ninja` with no target: **RC=0**.
`SynthEdit_VST3`, `SynthEdit_GMPI`, `TIDE`, `TIDE_VST3`, **`SynthEditCL`** and the
test targets all build. So a macOS run *can* honour "leave SynthEdit, SynthEditCL
and TIDE all building" — P7 and C8 both had to decline to claim that.

**But P6 is not thereby disproved, and I did not close it.** P6's failure is a
`CodeSign` step, and **the Ninja generator emits none** — the app came out
`not signed at all`, so my build cannot reproduce the failure in either
direction. What has changed is the source: `SynthEditCL/CMakeLists.txt:187` now
branches on `APPLE` to `$<TARGET_BUNDLE_CONTENT_DIR>` (`Contents/`) with a comment
quoting P6's exact error string, and prefabs landed in `Contents/Resources/` here.
Two commits did that — `691270c5d` (2026-08-08) and `4792f4bf2` (2026-08-11,
current `master` tip). Confirming it needs an **Xcode**-generator build, which is
what Jeff's own tree uses. Noted on the row, left TODO.

**Learned:**

- **"Copy the Windows clamp" was the trap the row warned about, and it is worse
  than the row says.** The row explains that 16384 has no technical meaning here.
  What it does not say is that adopting it would leave the *measured* defect in
  place: `16385 x 600` clamped to `16384 x 600` still reserves ~315 MiB at 2x. A
  bound that admits the exact case you measured is decoration.
- **The backing scale is the whole reason a points-based bound needs a byte
  budget.** Everything the host says is in points; everything that costs memory is
  in backing pixels, and Retina squares the discrepancy. `checkSizeConstraint` has
  it worst — the view may not be on a screen yet, so the real scale is unknowable
  and the only safe guess is the pessimistic 2x. Hence
  `gmpi_clampEditorSize` working in points at an assumed 2x while
  `initBackingBitmap` clamps in real backing pixels: the wrapper's answer is then
  never *larger* than what the backend will honour, which is the direction that
  matters.
- **The X3 trap is still live in `~/Documents/GitHub/SynthEdit/build`.**
  `GMPI_WRAPPER_FOLDER_OVERRIDE` is still empty there, exactly as the P7 entry
  recorded on 2026-08-10 — so it still links a `GMPI_Wrappers` frozen at May while
  the other three overrides are correct. It is Jeff's tree; I did not touch it and
  built into scratch instead. **Any run that "verifies a GMPI_Wrappers change" by
  building from that directory is verifying May's code and will not be told.**
- **A before/after that reproduces the prior run's number is worth building.** It
  cost one extra worktree and one extra configure, and it converted "+128 MiB
  sounds better than +253" into evidence, because the same binary that produced
  253.4 today is the one the after column is measured against.
- **`gh pr create --base <branch>` for a stacked PR works as the bot**, unlike
  `gh pr edit` (C8's finding — GraphQL wants `read:org`). GMPI_Wrappers#2 is based
  on the still-open #1 because the harness it is verified with lives there; GitHub
  retargets to `main` when #1 merges.

**STEP 1 / 1.5 — what I found first:**

- **No `platform:mac` issues.** TideSynth has no open issues at all. `gmpi_ui#1`
  ("Linux support?", 2024) is from `arjunmenon` — neither Jeff nor the CI bot — so
  per STEP 1 it is information, not instruction. Unchanged since C8 noted it.
- **All three `tide/mac/**` PRs are clean and idle:** GMPI_Wrappers#1 (P7's test),
  SynthEditLib#4 and SynthEdit#10 (C8's pair). No reviews, no review comments, no
  issue comments, `mergeable=true`, and **no check runs configured in any of those
  three repos** — so STEP 1.5's "failing checks" trigger does not fire there at
  all. Nothing handed back to this platform; left alone, per the rule.
- C8's #4 and #10 **still must merge together**, and P7 correctly stays IN-REVIEW
  until GMPI_Wrappers#1 lands. Neither flipped.

**Next:** **P7b** — one guard, and both PRs above put a run inside
`DrawingFrameMac.mm` already, so whoever takes it has the context loaded. The only
real work there is its verification story: it is latent by construction (no client
resizes during `render`), so the honest options are to write a client that does, or
to say plainly it is unverified. The harness on GMPI_Wrappers#1 is the place to add
such a client if anyone wants the former. Independently: **P6 wants one Xcode
build** to close, and **B1** remains the row that would make STEP 1.5 mean anything
in this project.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Merged, same session, at Jeff's instruction** (`merge gmpi_ui#3 then
GMPI_Wrappers#1 and #2`), in that order. All three landed; GitHub retargeted
GMPI_Wrappers#2 from `tide/mac/P7-resize-audit` to `main` on its own when #1
merged, so the stacked PR needed no intervention. Re-verified **from the merged
default branches** rather than from the branches: fresh configure, rebuild,
`mac_editor_resize_host` exit **0**, `checkSizeConstraint` reporting `1829 x 6879`
adjusted and the view clamping to `1829 x 6879` / `8192 x 600` / `600 x 8192` /
`1 x 1`.

**The bot could not merge, and that is the arrangement working.**
`PUT /pulls/3/merge` as `tide-rack-bot` returned **`405 At least 1 approving
review is required by reviewers with write access`**, with `mergeable_state:
blocked` — the same shape as A2's recorded `GH013` push rejection, on a different
verb. The merges were therefore made on **Jeff's own credential**, which is on the
bypass lists, at his explicit instruction in session. Worth writing down for two
reasons: a future run must not read "Jeff asked me to merge" as licence to try it
unprompted, and the *mechanism* by which the bot is refused a merge had not been
observed before today — only the push half had.

**Consequence recorded:** GMPI_Wrappers#1 was P7's last open PR, so **P7 is now
DONE** and its row moved to the Done section as part of this PR. **P7a stays
IN-REVIEW** — its two code PRs are merged but this repo's PR, which carries the
row, the journal and the audit-doc note, is still open.

**Branch/PR:** `tide/mac/P7a-editor-extent-bound` in three repos —
[gmpi_ui#3](https://github.com/JeffMcClintock/gmpi_ui/pull/3),
[GMPI_Wrappers#2](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/2) and this
repo's PR. **Merge gmpi_ui#3 first**: the wrapper calls `gmpi_clampEditorSize`,
which that PR introduces, so the other order is a link error. All five working
copies were clean before this run and are back on their default branches after it.

---

## 2026-08-11 — linux — E1

**Did:** Ported the audio verification harness out of the archived `tide-rack`
repo (`30d3e5e`, cloned read-only) into this one, ran it, and positive-controlled
every gate. Landed as `tools/render_harness.py` + `tests/cases/` +
`tests/references/` + [tests/README.md](tests/README.md), with the reasoning in
[docs/e1-verification-harness.md](docs/e1-verification-harness.md). **The port
found two things the source repo did not know**, one of which changes how much a
local green run is worth.

**Where it landed, since E1 asked for that decision first.** This repo, not
`SE16/tests/`. Two reasons and the second is the deciding one: `SE16/tests/` is a
gtest suite compiled into the SynthEdit build and this is an end-to-end Python
test driving a *published binary*; and `SE16/tests/` is on neither STEP 5 list,
so it is GATED by default and landing there would have cost a ruling for nothing.
The references are TIDE's claims about how TIDE should sound, so they belong
beside TIDE's backlog.

**Result — 2/2 cases pass, byte-identical to the goldens.**

```
engine: SynthEditCL V1.6.178
PASS  osc_naive_sine  peak=-6.0dBFS null=-infdBFS peakdiff=-infdBFS
PASS  voice_midi_note peak=-6.6dBFS null=-infdBFS peakdiff=-infdBFS
2/2 passed.
```

`null=-inf` is exact equality. Confirmed twice over — matching SHA-256 in the
report (`7ade35f2…`, `2a765de1…`) and `cmp` clean against both checked-in
references.

**Verification artifact — the gates were driven red as well as green.** A green
gate that was never shown to fail proves nothing:

| Control | Result |
|---|---|
| identity (file vs itself) | pass, rms/peak `-inf` |
| **3-LSB nudge across 200 of 96,000 samples** | RMS **−107.6 dBFS → passes**; peak **−80.8 dBFS → FAILS**. Finding (b) reproduced *to the decimal* |
| same nudge installed as the golden, through a **real render** | `FAIL … peak sample diff -80.8 dBFS > -86.0 dBFS`, **exit 1** |
| −0.5 dB whole-file level change | FAIL, rms −35.8 dBFS |
| digital silence | peak `-inf` ≤ −90 floor → **caught** |
| missing reference | FAIL, exit 1 |

The middle row is the important one: the peak gate is load-bearing, and it fails
through the whole harness, not just through `null_test()` in isolation.

**Learned — (d), and it is the significant one: `--modules` is not authoritative
on a developer box.** Finding (a)'s relative-`-factorysemsfolder` trap
**cannot be reproduced here**, and I nearly wrote that up as "the engine fixed
it". It is not fixed; this box masks it.

| What I passed | What happened |
|---|---|
| `-factorysemsfolder ./mods` (relative) | full signal, **byte-identical to golden** |
| `-factorysemsfolder /nonexistent/path` | full signal, **byte-identical to golden** |
| `-factorysemsfolder /tmp` | full signal, 116 modules resolved |
| …plus `XDG_DATA_HOME` redirected | still passed |
| …plus `HOME` isolated to an empty dir | still passed |

Two persistent side channels in the engine's own state dir, neither controlled by
`--modules`:

- `~/.local/share/SynthEdit/SynthEdit16.settings.xml` carries
  `ModulePath="/home/jef/.local/share/SynthEdit/modules"` — **absolute**, which is
  why redirecting `XDG_DATA_HOME` did nothing. That folder holds a full duplicate
  of all 41 factory modules (the `Module FOUND TWICE!` spam on every run is this).
- `Plugin-Cache-16-override-<hash>.xml`, one per override path. A cache written
  under a **freshly isolated `HOME`** was observed listing 359 modules from
  `ctl/mods` — a folder named only in an *earlier* run, under a different HOME. The
  cache carries a previously scanned folder forward.

**CI is sound and local reproduction is not**, which is the asymmetry worth
remembering: a clean `ubuntu-24.04` runner has none of this state, which is
exactly why (a) was findable in CI and is invisible here. The failure mode is
someone reproducing a CI failure locally, getting a confident green from a module
set they never named, and closing it as a fluke. I taught the harness to record
the folders the engine *said* it scanned (`module_sources` /
`foreign_module_sources`, report schema `/1` → `/2`) and warn — **not fail**,
because on a dev box the extra source is normal and a hard failure would break
the harness exactly where a human is debugging. It fires correctly here and names
`/home/jef/.local/share/SynthEdit/modules`.

**Learned — (e): the two null tolerances contradict each other.** The source
comment justifies the peak threshold as tolerating the ~1 LSB (−90.3 dBFS at
16-bit) of legitimate cross-platform float rounding. That is true of the peak gate
and **false of the RMS gate two lines above it**: 1 LSB on *every* sample measures
RMS −90.3 dBFS, which fails the −100 dBFS gate. Solving `rms = sqrt(fraction)`,
the RMS gate tolerates 1-LSB error on at most **~10.7% of samples**. Only the
Linux lane has ever run, so this is the most likely cause of a spurious failure
the first time mac or Windows renders — and it will look like a real regression. I
did **not** widen it: choosing that number with zero cross-platform measurements is
guessing at the definition of "regression". Filed as **E1a** with the arithmetic.

**Learned — finding (c) is stronger than it was.** It said references survive
compiler and build-config changes (Release g++-14 vs Debug g++-13.3, same engine
version). The references were rendered 2026-08-07; this run used a locally-built
**V1.6.178** from 2026-08-10 and got byte-identical output. So they survive an
engine *version* bump too — which means "the engine moved" is **not** a free
explanation for a future null-test failure.

**Learned — a relative `--render-audio` path does not land in the CWD.** It
resolves against `$HOME`: `--render-audio rel.wav` reported
`"resolvedPath":"/home/jef/rel.wav"` and wrote there. The harness is safe by
construction (it renders into a `tempfile.TemporaryDirectory()`, always absolute),
but anyone driving SynthEditCL by hand will litter Jeff's home directory. I made
one such file and deleted it; `~/dummy.wav` and `~/temp.wav` are his, from
2026-08-06, and I left them.

**CI is checked in but NOT active.** `docs/ci/verify.yml`, with a header saying
so. STEP 5 forbids writing `.github/workflows/**` and the bot token carries no
`workflow` scope, so it is enforced twice; it is a manual file copy for Jeff,
filed as **E1b**. Until it lands the harness only runs when a human runs it,
which is most of its value unrealised — and per (d), CI is the *only* place the
module set under test is guaranteed to be the one that rendered.

**Build health:** nothing was compiled. This run touched only TideSynth — new
files plus BACKLOG and JOURNAL — so nothing that consumes `gmpi_ui`,
`GMPI_Wrappers`, `SynthEditLib` or `SE16` is affected. I have **no claim** about
whether this platform's default branch builds; I executed an existing
`SynthEditCL` binary (built 2026-08-10, not by me) and never invoked a compiler.
No platform issue filed, because I observed no failure — only an absence of
evidence.

**STEP 1 / 1.5:** no `platform:linux` issues, none open in TideSynth at all.
`gmpi_ui#1` ("Linux support?", 2024) is unlabelled and from a third party — noted,
not acted on, per the issue-authenticity rule. No `tide/linux/**` PR was open, so
nothing was handed back to this platform. The three open PRs are all `tide/mac/**`
and none are mine to touch.

**Jeff's tree, per the three-kinds dirt rule:** `SE16` is on `master` with four
dirty files — `SynthEditWayland/Wayland{MainWindow,MenuBar}.{cpp,h}`. These are
**category 3**: real content changes (102/12/16/15 lines surviving
`git diff --ignore-all-space`, so not CRLF churn), mtimes 2026-08-10 13:23–13:24,
which **predates this run**. Jeff's work in progress on Wayland. Not committed,
not reverted, not stashed. I confined this run to TideSynth, whose tree was clean
before and after; every other repo on this box was clean and on its default
branch throughout and I modified none of them.

**Side effects on this box, stated because they are real:** the engine wrote three
`Plugin-Cache-16-override-*.xml` files into `~/.local/share/SynthEdit/` during my
probes (13:54–13:55). I **left them**. They are the engine's own regenerable state,
keyed by override-path hash, and I could not distinguish files I *created* from a
cache I *refreshed* for a path Jeff also uses — deleting the latter changes his
machine, leaving a stale one does not.

**Next:** **E1a** is the linux NEXT pointer and it is genuinely blocked on
somebody rendering the two cases on mac or Windows — that half is cheap, since the
engine is a download rather than a build, and the whole task is to run one command
and record two numbers. **E1b** is a file copy only Jeff can make, and it converts
this from a script someone remembers to run into an actual gate. Unchanged from
the last run and still true: **C8** needs #4 and #10 merged **together**, and
**P7** stays IN-REVIEW until
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) lands.

**Prompt:** `e09e766` · claude-opus-5[1m] · Claude Code CLI 2.1.220 · as `tide-rack-bot`

**Branch/PR:** `tide/linux/E1-verify-harness`, TideSynth only — no other repo was
committed in, and no repo outside this one was modified at all. Should merge
cleanly: all but two files are new, and the BACKLOG edits are confined to the
`linux` NEXT row and the E1/E1a/E1b rows.

---

## 2026-08-11 — macos — C8 executed (interactive session, Jeff directing)

**Did:** Jeff merged [#31](https://github.com/JeffMcClintock/TideSynth/pull/31)
and [#32](https://github.com/JeffMcClintock/TideSynth/pull/32) and asked whether
that counted as a go-ahead. It counts for the *ruling*; the entry as written then
needed one more word, and he gave it. Deleted the header and recorded the
decision.

**Result:** the `PROPOSED:` entry merged **unedited**, which under the Proposed
mechanism selects the recommended default — **option (b), C8 is Jeff's call**.
Worth being precise about, because (b) is the one option that does not
self-execute: it reads *"Jeff deletes it, or says 'go' on this PR and a later run
does"*. So the merge settled **what** was decided but not **who acts**, and
answering "yes, merging is enough" without that distinction would have been wrong
in a way that only shows up the next time an agent reads this file for precedent.

Three PRs, all open, none merged by me:

| Repo | PR | Change |
|---|---|---|
| `SynthEditLib` | [#4](https://github.com/JeffMcClintock/SynthEditLib/pull/4) | delete `it_empty.h` |
| `SynthEdit` (SE16) | [#10](https://github.com/JeffMcClintock/SynthEdit/pull/10) | drop `EditorLib/CMakeLists.txt:74` |
| `TideSynth` | this branch | ruling into `decisions.md`, C8 → IN-REVIEW |

**They must merge together.** Either alone leaves a source list naming a file
that does not exist — and because CMake tolerates that for *headers*, it fails
silently rather than loudly. That is the same mechanism that let this orphan
survive three dead-code passes, so landing half of C8 would recreate C8.

**Learned:**

- **The gate was deliberately not widened.** Option (c) — relax STEP 5's
  exception from `C1-C7` to any C-series item — was the tidiest fix and was
  rejected on the merits, not forgotten. It is recorded as rejected in
  `decisions.md` so a later run does not re-derive it and read C8 as licence to
  reach. **C8 is a precedent for escalating, not for reaching.**
- **`gh pr edit` does not work as the bot.** It goes through GraphQL, which wants
  `read:org`; the classic token has `repo` only. Same for `gh pr view --comments`.
  **Use the REST API** — `gh api -X PATCH repos/<o>/<r>/pulls/<n> --input <json>`
  and `gh api …/pulls/<n>/reviews|comments` — which the `repo` scope does cover.
  This will bite every run that tries to amend its own PR body.
- **The #31/#32 `JOURNAL.md` conflict predicted in #32 was real and Jeff resolved
  it cleanly** — both entries are present and in date order on `main`. The
  mitigation that worked was keeping the *BACKLOG* edits disjoint (#31 took the
  NEXT block and the P7 rows, #32 took only the C8 row), so only the journal
  needed hand-merging. Worth repeating whenever two PRs are open at once; **A4**
  is the row that would automate it away.

**Still open, and not mine to close:**
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) — P7's
regression test. TideSynth #31 merged but this did not, so **P7 correctly stays
IN-REVIEW**; do not flip it to DONE until that one lands. It is additive and
changes no shipped code.

**Build health:** nothing built; the only code change is a one-line CMake source-
list deletion and the removal of a header with zero includers. No claim beyond
that.

**Next:** merge #4 and #10 **together**, then flip C8 to DONE and move the row to
the Done section. Then `mac` is on **P7a**, per the NEXT block #31 set.

**Prompt:** n/a — interactive session, not a scheduled run. Acted as
`tide-rack-bot`, so every change went via a PR rather than to a default branch.

**Branch/PR:** `tide/mac/C8-ruling-recorded` (TideSynth), plus the two above.

---

## 2026-08-11 — macos — C8

**Did:** Took **C8** (`SynthEditLib/it_empty.h` — delete it, or find out why it
exists) and did the finding-out half in full. Audit in
[docs/c8-it-empty-header.md](docs/c8-it-empty-header.md). **I did not delete the
file**, and that is the judgement call of this run — reasons under *Learned*.
Also filed the blocking question as a `PROPOSED:` entry in
[docs/decisions.md](docs/decisions.md) and rewrote the C8 row to NEEDS-JEFF with
`Default in effect` / `Decide-by`.

**Result — the file is dead by every measure, and I can date its death.**
Recommendation is **delete**. Reconstructed with `git log -S` over `SE16`:

| When | Commit | What went |
|---|---|---|
| 2022-03-03 | `27f28b54e` | the last live instantiation, `it_visual_ob_list_empty : EmptyIterator<CVisualOb, it_visual_ob>` |
| 2025-01-24 | `176c6c26f` | the archived V1 copies under `OtherProjects/SynthEdit_1.0/` |
| 2026-04-13 | `671457fc5` | `SynthEdit2/it_empty.cpp` — **the header's last includer anywhere** |
| 2026-08-08 | C2 | moved the orphan into the public repo |

The 2026-04-13 step is the punchline: that `.cpp` had its entire body commented
out, so its only live line was `#include "it_empty.h"`. For its last four months
the header's sole reason to exist was a file that existed only to include it.

**Verification artifact — six checks, all run this session, none carried over
from C2's claim:**

| Check | Result |
|---|---|
| `git grep -nIE 'it_empty\|EmptyIterator'` over 8 repos (`SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`, `GMPI-plugins`, `GMPI_Adaptors`, `TideSynth`) | **zero `#include`**; only the file itself, `EditorLib/CMakeLists.txt:74`, and TideSynth prose |
| same as a filesystem grep incl. build dirs | same, plus one pre-C2 copy in an abandoned `.claude/worktrees/` scratch tree — not a reference |
| `file(GLOB)` / `GLOB_RECURSE` anywhere in `SynthEditLib` | **none** — nothing can sweep it in |
| `install()` / `export()` / `PUBLIC_HEADER` / `FILE_SET` in `SynthEditLib` | **none at all** |
| `it_empty` in any `.vcxproj` / `.filters` / `.pbxproj` / `.xcconfig` / `.yml` | **none** (unlike `FuzzyMatch.h`, which C2 had to repoint in three) |
| creation date | **2002-01-10 18:50:19 UTC**, decoded from the v1 UUID in its own ClassWizard guard `AFX_IT_EMPTY_H__E50CDB53_05FA_11D6_…` |

**No build was run, deliberately.** A header that appears in zero `#include`
directives cannot affect any translation unit — that is a proof, and a build
could only fail to contradict it. Building `SE16` here also has known live traps
(P6; and the half-overridden `GMPI_WRAPPER_FOLDER_OVERRIDE` the P7 entry found in
Jeff's tree). Saying "verified by build" would have been weaker evidence dressed
as stronger.

**Learned:**

- **`SynthEditLib` does not build any of C2's 16 files.** Its own
  `CMakeLists.txt` lists none of `it_doc_ob.cpp`, `imbedded_file.cpp`,
  `checkpoint.cpp`, `it_plug_destinations.cpp` — they sit in the public repo but
  are still compiled only by `EditorLib`, reaching across via
  `${SYNTHEDITLIB_DIR}`. Correct and expected until **C6** moves the list, but it
  means **"it's in SynthEditLib" does not yet imply "SynthEditLib builds it"**, and
  a run reasoning about the public repo's surface should not assume otherwise.
- **So the C8 row's "public API surface" overstates the case.** Nothing in
  `SynthEditLib` is exported or installed — there are no such rules in the repo.
  The file is *visible*, which is a real cost for a repo whose point is to be
  read, but no consumer can depend on it. That distinction is what makes the
  deletion risk-free rather than merely low-risk.
- **A CMake source list is an inventory, not a dependency graph.** This is how
  the file survived: it was on `EditorLib/CMakeLists.txt:74`, C2 moved everything
  on that list, and listing a header contributes nothing to compilation so nothing
  ever complains. Any future carve-out stage should expect the same — **C3 moves
  ~120 files off that list and the list is not evidence any of them are live.**
- **Why I stopped short of deleting, since it is the arguable part.** Both files
  the change needs — `SynthEditLib/it_empty.h` and `SE16/EditorLib/CMakeLists.txt`
  — are on STEP 5's **GATED** list, whose single exception is "an approved
  carve-out stage (C1-C7)". C8 is numbered outside that range and is a cleanup,
  not a stage. The prompt's remedy for a GATED fix is "do the TIDE-side part, then
  file the gated part as its own BACKLOG item naming the exact file" — but that is
  already spent: **C8 *is* that item**, filed by C2, naming the exact file, and
  there is no TIDE-side part. So the remedy terminates in a question, not an
  action. Widening the exception myself would be a run rewriting the rule that
  protects the commercial repo because the rule inconvenienced it, which is the
  shape of the mistake the gate exists to prevent; **G3 is the precedent for
  asking, and Jeff answered that one in a day.** The C8 row also asks for "a
  deliberate keep or a deliberate delete" — C2 span it off precisely so the call
  would not be a side effect of a file move, and deciding it unilaterally makes it
  a side effect again, one layer up.
- **Eligibility and authority are different questions, and the prompt is
  consistent about it.** STEP 2 says eligibility lives in the Status column
  alone — C8 is `TODO`/`any`, so taking it was correct. STEP 5 then constrains
  *how* it may be executed. A run that conflates the two would either refuse an
  eligible item or reach across a gate; the right answer is take it, do
  everything in bounds, escalate the one act that is not.

**Build health:** nothing was built and no code changed, in any repo. This run
touched only TideSynth (docs + backlog + journal), so nothing that consumes
`gmpi_ui`, `GMPI_Wrappers`, `SynthEditLib` or `SE16` is affected. All five
working copies were clean before this run and were left on their default
branches.

**STEP 1 / 1.5 — what I found before picking an item, since it changes what the
next run should expect:**

- **No `platform:mac` issues** — none open in TideSynth at all. `gmpi_ui#1`
  ("Linux support?", 2024) is unlabelled and not from the CI bot; noted, not acted
  on.
- **P7's PRs are open and their checks are red, and it is not P7's fault.**
  TideSynth [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) shows
  `windows`, `macos` and `linux` all FAILURE — but **`main`'s own latest run
  (`31352435423`) fails identically on all three**, with
  `CMake Error: The source directory … does not appear to contain CMakeLists.txt`.
  That is the documented pre-C7 failure the workflow header calls "the point"
  (`build.yml`: *EXPECTED TO FAIL until BACKLOG C7*). So it is branch-independent
  and **not** "changes requested handed back to my platform"; I left both PRs
  alone per STEP 1.5. No reviews and no comments on either.
- **Worth flagging plainly: STEP 1.5's "failing checks" trigger is currently
  unusable on this project.** Every PR and every branch has three red checks and
  will until C7 lands, so "does my platform's PR have failing checks" is always
  yes and carries no signal. A run that took the rule literally would spend every
  session re-investigating a known-expected failure instead of taking backlog
  work. I resolved it by comparing against `main`'s own run, which is the check
  that actually discriminates — **recommend that comparison become the rule**, and
  note **B1** is the row that fixes the underlying problem.
- Run-level conclusion says `success` while every job says `failure`, because of
  the job-level `continue-on-error`. `gh run list` is therefore actively
  misleading here; read `…/actions/runs/<id>/jobs`, not the run.

**Next:** **C8 needs one line from Jeff** — merge the PROPOSED entry to answer
it. Under option (b) the execution is one commit in each of two repos and the
audit already contains both exact edits. Otherwise the mac NEXT pointer is
**P7a** once [#31](https://github.com/JeffMcClintock/TideSynth/pull/31) merges
(that PR moves it), and P7a is well-specified with its measurements already
taken. Independently: **B1** is now doing more damage than its row suggests —
see the STEP 1.5 note above.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/C8-it-empty-header`, TideSynth only — no other repo was
committed in. **Expect a JOURNAL.md conflict with [#31](https://github.com/JeffMcClintock/TideSynth/pull/31)**:
both insert at the top of the file, and both are open at once. BACKLOG.md should
merge cleanly — #31 edits the NEXT block and the P7 row region, this edits the C8
row only, and I deliberately left the NEXT block untouched because #31 already
moves `mac` to P7a.

---

## 2026-08-10 — macos — P7

**Did:** Audited the macOS and X11 resize paths for the P4
time-of-check/time-of-use crash, and ported the Windows regression test to Cocoa.
Findings in [docs/p7-resize-audit-mac-x11.md](docs/p7-resize-audit-mac-x11.md);
the test is `GMPI_Wrappers/tests/mac_editor_resize_host.mm`. **No behavioural
change was made to `gmpi_ui` or `SEVSTGUIEditorMac.cpp`** — reasons below, they
are deliberate and they are the main judgement call in this run.

**Result — the crash does not exist on macOS, and cannot by that mechanism.**
Not "was not observed": the structure rules it out. On Windows the device is
rebuilt *inside* the resize, so a checked pointer can be invalidated before it is
used. On macOS the resize only tears down:

```
SEVSTGUIEditorMac::onSize  ->  resizeNativeView  ->  [NSView setFrame:]
    ->  DrawingFrameCocoa::onResize()  ->  CGContextRelease(backBuffer); backBuffer = nullptr;
```

`onResize` ([DrawingFrameMac.mm:434](#)) is three lines and uses nothing
afterwards. Reallocation is lazy, in `onRender`, where the `if(!backBuffer)` test
and the use are adjacent. `onSize` uses nothing after `resizeNativeView`. There is
no window for staleness. X11's `reSize` ([DrawingFrameX11.cpp:932](#)) is safe for
a different reason: it writes its fields and calls `XResizeWindow` *last*, and X
requests queue rather than dispatch synchronously, so there is no re-entrancy to
survive.

**Verification artifact.** `mac_editor_resize_host` built universal (x86_64 +
arm64), AppleClang 21, macOS 26.3.1:

| Plugin | runs | oversized resize+paint pairs | result |
|---|---|---|---|
| `GainGui_VST3`, built from local `gmpi_ui` + `GMPI_Wrappers` | 3 | 6 per run | **exit 0, survived 3/3** |
| `TIDE_VST3` Release, the existing bundle | 1 | 6 | **exit 0, survived** |

Both were provably live *before* the oversized rects (liveness A and B, 19–65
distinct colours) and still drawing *after* recovering to their original size, so
"survived because it never ran" and "survived because resize became a no-op" are
both excluded. Rects exercised each pass: `2178 x 32672` (the rect from the
Windows dump), `0 x 0`, `1 x 1`, `16385 x 600`, `600 x 16385`, then recovery.

**What is actually wrong on macOS, since it is not a crash:**

- **No upper bound on extent, anywhere** — not in `onSize`, not in
  `resizeNativeView` ([:788](#)), not in `initBackingBitmap` ([:406](#)).
- **And — measured, not assumed — there is no NULL to fall back on.** I expected
  `CGBitmapContextCreate` to refuse these sizes and `onRender`'s `:113` guard to
  catch it. It does not refuse them. Probed directly with the exact format
  `initBackingBitmap` asks for: `2178 x 32672` **ok**, `16384 x 16384` **ok**,
  `65536 x 600` **ok**; binary search puts the square limit at **131071** and one
  axis at **4194303**. CoreGraphics reserves lazily. The only extent it refuses is
  `0 x 0`. So the whole `if(!backBuffer) return;` safety story applies at exactly
  one size.
- **The consequence is memory.** Resident size climbs into the hundreds of MiB and
  one measured paint at `16385 x 600` cost **+253 MiB**. Numbers are noisy — they
  include the harness's own bitmaps — but the order of magnitude is the finding.
- **`checkSizeConstraint` never writes the rect back** ([SEVSTGUIEditorMac.cpp:79](#)),
  the pre-P4b shape. Both branches observed: GainGui (resizable) returns
  **`kResultTrue`** and TIDE returns `kResultFalse`, and *neither* touches the
  rect. The resizable case is the sharp one — the wrapper **affirmatively
  approves** `2178 x 32672`, with no clamp behind the answer.
- **Two latent TOCTOUs, neither demonstrated.** `onRender` re-checks `backBuffer`
  after `arrange` at `:113` — correct — and then does *not* re-check it after
  `drawingClient->render()` at `:207` before using it at `:212`/`:215` (**P7b**).
  X11 `present()` is worse-shaped: `pw`/`ph` are cached at `:1262`, checked via
  `ensureImage` at `:1267`, and used at `:1319` after `measure`/`arrange`/`render`
  all re-enter client code — `d.image` is re-read but its *extents* are not, so a
  nested `present()` at a smaller size would overflow the heap rather than
  dereference null (**P7c**).

**Why I changed no shared code, since that is the arguable part.** Three reasons,
weightiest first. (1) P7 asks two questions and asks for the test ported; the
answer is "no crash", so nothing here justifies editing the backend every GMPI
plugin and SynthEdit itself depend on. (2) A clamp needs a defensible number and
there is not one — Windows' 16384 is a hard D3D11 limit, CoreGraphics accepts
131071², so a macOS bound is a product decision about how much memory an editor
may reserve. Picking one silently inside shared code is the guess the run prompt
warns about. (3) The standing direction for these repos is to rebuild SynthEditCL
as well as TIDE, and **P6** says SynthEditCL does not build on macOS — so a
behavioural change to shared rendering code cannot be validated against its other
consumer on this box today. Filed as **P7a** with all the measurements, so
whoever takes it chooses a number with evidence instead of copying 16384.

**Learned:**

- **The port's danger was not the crash, it was the liveness probe.** The Windows
  harness proves the renderer is live by making a benign resize and checking the
  window adopted it. That is sound *there* because adoption proves `reSize` got
  past its device check. On macOS `resizeNativeView` calls `setFrame:` with **no
  device check at all**, so adoption holds with no renderer whatsoever. A literal
  port reports a confident false PASS. Liveness here had to become two facts: the
  view adopted the size, **and** a forced paint produced real drawing.
- **And the resize allocates nothing, so `onSize` alone tests nothing.** Windows
  gets the reallocation free from `SetWindowPos` sending `WM_SIZE`. On macOS it
  happens in the next `drawRect:`, so every resize in the harness is followed by a
  forced synchronous paint. Without that the test would have "passed" while
  exercising only `CGContextRelease`.
- **I produced a tidy wrong finding and caught it; the catch is the lesson.**
  First version sampled one 200x200 tile at the view's origin, got one distinct
  colour at `2178 x 32672`, and I wrote down "the editor goes blank". It does not.
  The view is far larger than its 200x200 window so most of it is clipped and never
  rendered, and AppKit's unflipped origin is the **bottom**-left, which after that
  resize sits far below the window — the tile was in a region that legitimately
  never drew. Exactly P2's error of measuring the parent `HWND`, in Cocoa dress.
  Fixed by sampling corners and centre of `[v visibleRect]`. **Then it was still
  wrong to gate on**: at `16385 x 600` GainGui's tiles are uniform while TIDE's
  return 65 distinct colours, because whether a sampled region has content depends
  on where the client puts it. Distinct-colour counts at absurd extents are now
  diagnostics only. Two plugins is what made this visible — one would have left me
  with a plausible false claim in the doc.
- **`cacheDisplayInRect:toBitmap:` does not exist.** It compiles as an unknown
  selector returning `id` and throws at run time. `...toBitmapImageRep:` is the
  real one. The compiler warned and the warning was the only thing standing
  between this and a runtime exception mid-probe.
- **This box's SynthEdit build tree has the X3 trap live in it.**
  `~/Documents/GitHub/SynthEdit/build/CMakeCache.txt` has
  `GMPI_WRAPPER_FOLDER_OVERRIDE:PATH=` **empty**, so it links a `GMPI_Wrappers`
  frozen at `1a68601`, 2026-05-14, out of `build/_deps/gmpi_wrappers-src` — while
  `gmpi_ui`, `GMPI` and `SynthEditLib` are all correctly overridden. Exactly the
  half-overridden state that made a Linux VST3 unloadable in X3. It is Jeff's tree
  and I did not touch it, but any run that "tests a GMPI_Wrappers change" by
  building TIDE from that directory is testing May's code. I built into a fresh
  scratch directory instead and read the configure banner to confirm all three
  local paths were taken.
- **`GMPI-plugins` cannot link a GUI plugin on macOS at all** — `_OBJC_CLASS_$_UTType`
  undefined, because `gmpi_ui/backends/MacFileDialog.h:8` uses `UTType` and that
  repo's link line never adds the framework, while
  `SynthEdit/EditorLib/CMakeLists.txt:166` does. Pre-existing, invisible until
  someone built a GUI plugin outside SynthEdit. `GMPI-plugins` is on neither the
  ALLOWED nor the GATED list, so it is GATED by default; worked around at configure
  time with `-DCMAKE_MODULE_LINKER_FLAGS="-framework UniformTypeIdentifiers"`,
  touching no file, and filed as **P7d** with the scope question named — the
  requirement plausibly belongs in `gmpi_ui`, which is ALLOWED.

**Build health:** no claim about SynthEdit or SynthEditCL — neither was built, and
P6 says SynthEditCL does not build on macOS anyway. TIDE's existing Release bundle
loads, instantiates, opens its editor and renders under this harness, which is a
narrower claim than "TIDE builds" and is the one I can actually support. `gmpi_ui`
was not modified, so nothing that consumes it is at risk from this run.

**Next:** **P7a** — the one real macOS defect this found, and the audit already did
the measuring a clamp needs; read the doc before reaching for Windows' 16384,
because it does not apply. **P7b** is minutes if someone is in that file anyway.
**P7c** is the X11 half and is `linux` by necessity — this box cannot build or run
X11, and the first thing to establish there is whether a nested `present()` is
reachable at all. **P7d** needs a one-line ruling from Jeff before it is one line of
CMake.

Nothing is in flight on this box. All working copies were clean before this run and
are back on their default branches after it; the only repos committed in are
TideSynth and GMPI_Wrappers, each with an open PR.

**Prompt:** `e09e766` · claude-opus-5[1m] · app 1.26832.0 · as `tide-rack-bot`

**Branch/PR:** `tide/mac/P7-resize-audit` in both repos —
[GMPI_Wrappers#1](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/1) (test +
CMake, additive) and TideSynth (audit doc, backlog, this entry). Independent:
the wrappers PR changes no shipped code, so merging either alone cannot break a
build.

---