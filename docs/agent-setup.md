# Three machines, one backlog

## The coordination model

The three machines **cannot talk to each other**. There is no shared memory, no
message passing, no way for the Mac to ask the Windows box a question. Every
weekly run starts with an empty head.

Everything they share goes through the GitHub repo:

```
                    ┌─────────────────────┐
                    │   GitHub: TideSynth │
                    │                     │
                    │  BACKLOG.md   ←── the queue
                    │  JOURNAL.md   ←── the memory
                    │  Issues       ←── cross-platform requests
                    │  PRs          ←── all work, never merged by agents
                    └─────────────────────┘
                       ↑        ↑        ↑
                  ┌────┘        │        └────┐
              Windows          macOS         Linux
              Fri 02:00       Sat 02:00     Sun 02:00
              features        AU/AUv3       VST3/CLAP
              VST3/GMPI       iOS sandbox   GCC warnings
```

**The runs are staggered on purpose.** Windows goes first and pushes a branch;
the Mac wakes up a day later and sees it; Linux a day after that. A change
therefore gets three platforms' scrutiny within a weekend, without any machine
waiting on another. If they all ran Friday at 02:00 they would collide on the
same backlog item and produce three conflicting branches.

## Platform roles

Each machine only takes backlog items marked for its platform, or `any`.

| Machine | Cron | Owns |
|---|---|---|
| Windows | `0 2 * * 5` (Fri 02:00) | Feature work, VST3 + GMPI, the carve-out stages (the SynthEdit repo lives here) |
| macOS | `0 2 * * 6` (Sat 02:00) | AU, AUv3, **iOS sandbox validation**, `auval`, notarization prep |
| Linux | `0 2 * * 0` (Sun 02:00) | VST3, CLAP, GCC warnings, portability |

macOS carries the most important role: iOS AUv3 is the constraint the whole
design is built around (PLAN.md constraint 3). If it does not run there, the
design is wrong, and only the Mac can find that out.

## How a build failure gets fixed

This is the mechanism the whole arrangement exists for.

1. Windows pushes a branch. CI runs the cross-platform matrix.
2. The macOS job fails to compile.
3. CI opens (or an agent opens) a GitHub issue labelled `platform:mac`,
   containing the **full compiler output**, the branch name, and the commit sha.
4. Saturday 02:00: the Mac's run reads open `platform:mac` issues *before*
   touching the backlog. Fixing a broken build outranks new feature work.
5. The Mac fixes it, pushes to the same branch, comments on the issue with what
   was wrong, closes it, and writes a JOURNAL entry.
6. Sunday: Linux does the same for its own platform.

The rule that makes this work: **an agent never fixes another platform's build
blind.** A Windows agent guessing at an Apple framework error produces plausible
nonsense. It should file the issue and let the machine that can actually compile
it do the work.

## Setting up the macOS and Linux boxes

On each machine, clone the repo and create the scheduled task. The task prompt
is identical apart from the machine name, cron, and platform role — the prompt
below is the Windows one; substitute accordingly.

```bash
git clone <repo-url> ~/TideSynth
```

Then in Claude Code on that machine, ask it to create a weekly scheduled task
using [the prompt in this repo](weekly-run-prompt.md), with:

- macOS: cron `0 2 * * 6`, machine name `macos`, platform role `mac`
- Linux: cron `0 2 * * 0`, machine name `linux`, platform role `linux`

All three machines are now set up, so in practice this section is about
*re*-installing — which is a routine need, not a one-off. Read the next section
before assuming a prompt change has reached anywhere.

## The prompt is fetched, not copied — since 2026-08-09

This used to be the sharpest edge in the whole arrangement, and it was invisible
from inside a run. It is fixed by indirection.

Each machine's scheduled task now holds a short **bootstrap** — machine name,
platform role, repo path, and an instruction to fetch
[weekly-run-prompt.md](weekly-run-prompt.md) from `origin/main` and follow it.
The real instructions are read fresh every run, so **editing that file and
merging it reaches all three machines on their next run**, with no reinstall.

Two details that make it hold:

- The run reads the prompt from `origin/main`, via
  `git show origin/main:docs/weekly-run-prompt.md`, **not from the working
  tree** — which may be dirty or parked on a branch an earlier run left behind.
- STEP 4 records the prompt's blob sha
  (`git rev-parse --short origin/main:docs/weekly-run-prompt.md`) in the journal
  entry. It changes only when the prompt changes, so which rules actually ran is
  now visible in the handoff instead of having to be remembered. A box still on
  a frozen copy silently omits that line, which is the tell.

The cost is blast radius: a bad edit now reaches every machine at once rather
than one. That is the better trade, because prompt changes go through a PR that
Jeff merges, while staleness went through nothing and announced itself to nobody.

**The bootstrap cannot install itself.** A machine that reads nothing remote
cannot be told to start reading something remote, so each box needed one last
manual install — **G4** for macOS, **G5** for Linux. **Both landed 2026-08-09,
and Windows converted itself the same day. All three boxes now fetch.** The old
model below is history, kept because the failure it caused is worth remembering,
not because any machine still runs it.

### The old model, and why it had to go

Creating a scheduled task *copies* the prompt text into the machine's own task
definition:

| Machine | Where its copy lives |
|---|---|
| Windows | `C:\Users\jef\.claude\scheduled-tasks\tide-synth-weekly-windows\SKILL.md` |
| macOS | `~/.claude/scheduled-tasks/tidesynth-weekly-macos/SKILL.md` |
| Linux | the `tidesynth-weekly-linux` scheduled task |

**Note the Windows task is named `tide-synth-weekly-windows`, with a hyphen**,
not `tidesynth-` as this table said until 2026-08-09. Verify the real name with
the scheduled-task list before a reinstall; naming a task that does not exist
creates a second one and leaves the original firing.

From then on the two were unrelated files, and **editing the prompt changed
nothing on any machine.** A PR that fixed the prompt fixed a document; the
machines kept running whatever text they were installed with, for months.

The reason that bit harder here than it would elsewhere: a run has no memory and
no way to notice. It cannot compare its own instructions against the repo — the
instructions are all it has, and they look authoritative. So a stale machine did
not fail loudly; it quietly followed old rules while the repo said otherwise, and
the journal it wrote gave no hint. Three machines could be running three
different rulebooks with every entry reading as if everything were fine, and for
most of this project's life that is exactly what was happening.

Two rounds of hand-reinstalling proved the discipline does not hold. The rule was
"treat a prompt change and three reinstalls as one job", and it failed twice
inside four days — G2 reached only macOS, G3 reached nobody, and the state table
in this document confidently said Windows was current while it was not. The fix
had to remove the copying, not add more diligence to it.

### Installing the bootstrap on a machine

On that machine, in Claude Code:

> Replace my `tide-synth-weekly-<machine>` scheduled task with the block under
> "The bootstrap" in `docs/weekly-run-prompt.md` in the TideSynth repo,
> substituting `{MACHINE}`, `{PLATFORM}` and `{REPO}` for this box. Leave the
> cron alone.

Check the task's real name first — Windows' is `tide-synth-weekly-windows`, with
a hyphen, and naming one that does not exist creates a second task while leaving
the original firing. Then read the result back and check three things:

- **No numbered STEPs beyond STEP 0.** If there are, the full prompt was
  installed instead of the bootstrap and the box is frozen again. Read, do not
  grep: the bootstrap mentions "STEP 4" in prose ("Put that sha in your JOURNAL
  entry, as STEP 4 requires"), so `grep 'STEP 4'` matches a *correct* install.
  What disqualifies it is a **heading** — `STEP 1 —`, `STEP 2 —` and so on.
- **The three values are substituted** everywhere except the one sentence that
  tells the run to substitute them into the *fetched* prompt. That sentence
  keeps its literal `{MACHINE}`, `{PLATFORM}`, `{REPO}`; a check that demands
  zero placeholders will wrongly fail.
- **STEP 0's commands run verbatim.** Paste all three into a shell from an
  unrelated working directory, with the tree parked on a branch. That is the
  only part that can actually be wrong on a given box — a bad repo path, or `~`
  not expanding — and it costs seconds to prove rather than discovering it at
  02:00 next Sunday.

Both false alarms were found by G5 and hit again by G4; the shell check is what
caught nothing on either box, which is the outcome you want from it.

### Current state — 2026-08-09

| Machine | Prompt version | Needs install? |
|---|---|---|
| Windows | **bootstrap** — fetches the prompt every run | no, and never again |
| macOS | **bootstrap** — converted 2026-08-09 by the G4 run | no, and never again |
| Linux | **bootstrap** — converted 2026-08-09 by the G5 run | no, and never again |

**This table is finished.** It exists now as a record that the conversion
completed, not as state anyone maintains — which was the point, because every
version of it before this one was wrong within three days of being written. The
prompt version a run actually executed is in that run's JOURNAL entry, as a blob
sha, put there by the run itself.

The task names are not consistent and each box's is worth checking before
touching it: `tide-synth-weekly-windows` (hyphen after `tide`),
`tidesynth-weekly-macos`, `tidesynth-weekly-linux`. Their files are under
`~/.claude/scheduled-tasks/<name>/SKILL.md` on macOS and Linux, and the Windows
equivalent under `C:\Users\jef\`. All three confirmed against the live task list
during G4 and G5.

What the two frozen copies were actually missing, recorded because it is the
measure of how much silent drift the copy model produced. **macOS** (installed
2026-08-06 14:51, the PR #4 generation) was missing G3 — so it did not know
`gmpi_ui` and `GMPI_Wrappers` are ALLOWED, and a run picking up P7-shaped work
would have refused its own item, P7 being a `mac` row whose fix lives entirely in
those two repos. It was also missing the CRLF-churn guidance, STEP 4's "in EVERY
repo you committed in" and its prompt-sha line, all of STEP 5's two-end-states
rule including which repo is `master`, and it still said "six constraints"
against PLAN's eight. **Linux** was the same generation and missing the same
things — see the G5 entry, which corrected an earlier claim here that it also
predated PR #4. It did not.

Windows was previously reinstalled by hand after the P2 run. Note what that cost:
the P2 run itself executed under the old blanket ban. It was an observe-only item
so nothing was blocked, but S1a — the next `win` item that touches code — would
have been refused by its own instructions. The 2026-08-09 reinstall closed a
second such gap: Windows had been stale on G3 since 2026-08-07 while this table
said it was current, because the table is updated by hand and the person updating
it is not the person the staleness affects.

That second gap is what settled the argument for fetching over copying: the
copy-and-remember model had now failed twice, and the second failure happened
while a table in this document asserted it had not.

The refinement this section used to file as "worth considering" is now done and
cost nothing once the prompt was fetched rather than copied: **STEP 4 records the
prompt's blob sha in every journal entry.** Which rules a run executed is visible
in its handoff. A box still on a frozen copy has no STEP 0, so it omits that line
entirely — absence is the signal, and it is the one piece of evidence that was
missing every previous time this went wrong.

### Caveats worth knowing before relying on this

- **Scheduled tasks only run while the Claude app is open.** If the machine is
  asleep or the app is closed at 02:00, the task runs at next launch instead.
  For a weekly cadence that is usually fine, but it means "it did not run" is a
  normal occurrence, not a bug.
- **One run is one context window.** A run cannot work for hours. This is why
  backlog items are scoped small and why JOURNAL.md discipline matters more
  than anything else here.
- **Agents never merge.** Every run opens a PR and stops. Merging is Jeff's.
- **Agents never push to `main`** and never create public repos or deploy the
  website.

## If it produces junk

Kill it. Nothing here is load-bearing for SynthEdit — the carve-out stages are
the only items that touch the commercial repo, and those are gated on explicit
approval (BACKLOG C0). Delete the scheduled tasks and the branches; the SE16
tree is untouched by everything else.
