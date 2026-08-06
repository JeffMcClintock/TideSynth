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

## The prompt is copied, not shared

This is the sharpest edge in the whole arrangement, and it is invisible from
inside a run.

[weekly-run-prompt.md](weekly-run-prompt.md) is the **master copy**, but nothing
reads it at run time. Creating the scheduled task *copies* that text into the
machine's own task definition:

| Machine | Where its copy lives |
|---|---|
| Windows | the `tidesynth-weekly-windows` scheduled task |
| macOS | `~/.claude/scheduled-tasks/tidesynth-weekly-macos/SKILL.md` |
| Linux | the `tidesynth-weekly-linux` scheduled task |

From then on the two are unrelated files. **Editing the master copy changes
nothing on any machine.** A PR that fixes the prompt fixes a document; the
machines keep running whatever text they were installed with, possibly for
months.

The reason this bites harder here than it would elsewhere: a run has no memory
and no way to notice. It cannot compare its own instructions against the repo —
the instructions are all it has, and they look authoritative. So a stale machine
does not fail loudly; it quietly follows old rules while the repo says otherwise,
and the journal it writes gives no hint. Three machines can be running three
different rulebooks and every entry will read as if everything is fine.

**So: any change to `weekly-run-prompt.md` is only half a change until every
machine's task is reinstalled.** Treat the two as one job.

### Reinstalling a machine's prompt

On that machine, in Claude Code:

> Update my `tidesynth-weekly-<machine>` scheduled task to match
> `docs/weekly-run-prompt.md` in the TideSynth repo, substituting `{MACHINE}`,
> `{PLATFORM}` and `{REPO}` for this box. Leave the cron alone.

Then diff the result against the master copy to confirm the only differences are
the intended substitutions — the substitutions are easy to get right and the
surrounding edits easy to drop.

### Current state — 2026-08-06

| Machine | Prompt version | Needs reinstall? |
|---|---|---|
| Windows | reinstalled 2026-08-06, after the P2 run | no |
| macOS | PR #4 text — predates the constraint-count fix | **yes** |
| Linux | as originally installed | **yes** |

Linux still predates PR #4, so on that box agents refuse to edit
`SE16/SynthEditSem/` (STEP 5's old blanket ban) and claim backlog items without
pushing the DOING mark — which is what let Linux and macOS both take S1 on
2026-08-06.

macOS has the PR #4 text but was installed before the "six constraints" wording
was removed, so its agent is told to check against six of PLAN.md's seven. Less
dangerous than Linux's copy, still wrong.

Windows was reinstalled by hand after the P2 run. Note what that cost: the P2 run
itself executed under the old blanket ban. It was an observe-only item so nothing
was blocked, but S1a — the next `win` item that touches code — would have been
refused by its own instructions.

A future refinement worth considering: stamp a version or date into the prompt
text and have STEP 4 echo it into the journal entry. Staleness would then be
visible in the handoff instead of having to be remembered.

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
