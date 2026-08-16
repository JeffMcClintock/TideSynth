# Community research — rejection memory

BACKLOG **A9**. Read by
[scripts/community-research.py](../scripts/community-research.py) on every run.

**What this is for.** Without it, every run re-proposes the same items and the
routine trains its reader to skim past its output — the same failure A4's row
names as "training the rubber-stamp reflex on trivia". A key listed here is
never proposed again.

**This file records human judgement, not the routine's.** The script's own
auto-reject filter is separate: it encodes PLAN's design constraints and lives
in the script, where each rule cites the constraint it comes from. This file is
for things a *person* looked at and decided were not TIDE's problem.

## Format

One bullet per rejected item — the `source#id` key exactly as the routine
prints it, then a one-line reason:

```
- `surge-synthesizer/surge#7782` — another project's release admin, no product signal
```

The key is parsed; everything after it is for humans. To un-reject something,
delete its line.

## Rejected

- `surge-synthesizer/surge#7782` — another project's release checklist; project
  admin, not product signal. Seeded 2026-08-16 by the A9 run to exercise the
  mechanism end-to-end, and chosen because it is unambiguously not a product
  question — the routine must never look like it is deciding direction.
