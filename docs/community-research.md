# Community research routine

BACKLOG **A9**. Built 2026-08-16 on the macOS box.
[scripts/community-research.py](../scripts/community-research.py).

Reads five public sources, drops what TIDE's constraints already exclude,
dedups, and prints **PROPOSED** rows for triage. **It decides nothing.**

```bash
python3 scripts/community-research.py            # all sources, top 12
python3 scripts/community-research.py --source watch --top 0   # the iOS watch, in full
python3 scripts/community-research.py --selftest # offline, no network
python3 scripts/community-research.py --dry-run  # what it would fetch, and as whom
```

---

## The guardrails are structural, not policy

A9 lists them as hard rules, so they are enforced by construction rather than by
remembering:

| Rule | How it is enforced |
|---|---|
| read-only; never post, vote or DM | `_get()` is the only network call and can only issue GET. **There is no write path to disable** — adding one means adding the capability first, which is where someone should say no. |
| courtesy rates | every request waits `COURTESY_SECONDS` (1.5s) since the previous one, process-wide |
| identify ourselves | User-Agent names the project and links to the repo, so an operator who dislikes the traffic knows who to tell |
| https only | a plain-`http` URL raises; pinned by a selftest case |
| provenance on every item | each row prints its source key and its URL |
| never decides direction | the script prints; **it cannot edit `BACKLOG.md`** |

The bot token, if present, is attached **only** to `api.github.com` requests and
only to raise the rate limit — every endpoint used works without it.

---

## Sources, and the two that are deliberately absent

| Key | Source | Notes |
|---|---|---|
| `forum` | VCV Community (Discourse `latest.json`) | 30 newest topics |
| `cardinal` | `DISTRHO/Cardinal` issues | public REST API |
| `surge` | `surge-synthesizer/surge` issues | public REST API |
| `library` | VCV Library `manifests-cache.json` | ecosystem census |
| `watch` | VCV Community **search**, for the standing hypothesis | see below |

**Not automated, on purpose:**

- **Audiobus / Loopy Pro forums** — no usable API. A9 makes these a *quarterly
  human skim*. This is where the iOS AUv3 audience actually lives, so it is a
  **known gap, not an oversight**.
- **Reddit / KVR** — demoted by A9: low yield and ToS-hostile to bulk fetch.

---

## The `watch` source exists because passive scanning cannot work

A9's standing hypothesis is that **no open-source modular exists on iOS AUv3**,
and the routine should watch for anyone moving into that gap.

**Reading the newest topics cannot do that, and this was measured rather than
assumed:** across **48 real items** (30 forum topics + 18 Cardinal issues) from
a live run, the hypothesis filter matched **zero**. An iPad thread appears on
that forum roughly once a year. A passive watch would have looked like a working
watch that had simply found nothing — the most dangerous shape of failure this
project keeps running into.

Searching finds them on the first call: *"VCV Rack for iPad - 2025?"*, *"VCV
Rack on iOS/Android devices?"*, *"How are you connecting/using VCV with an
iPad?"*.

**Two corrections that made it usable**, both from live output:

- **The match must be in the topic TITLE.** Discourse search matches post
  bodies, so the top hits were the forum's megathreads — *"What are you
  listening to?"* (6,135 replies) and *"Member Introductions"* — which merely
  contain "iPad" somewhere across thousands of posts. Requiring the title cut 54
  hits to **17**, all on-topic.
- **Watch items rank by recency, not engagement.** The hypothesis is about
  someone moving into the gap *now*, so a busy 2018 thread is worth less than a
  quiet 2026 one.

---

## The auto-reject filter is PLAN, transcribed

A9 asks for the product philosophy as an explicit auto-reject filter,
Cardinal-style. [PLAN.md](../PLAN.md) already carries it, so the rules are a
transcription rather than a new opinion, and **each cites the constraint it comes
from** — a rejection can be argued with by arguing with PLAN, not with a regex.

Covered: plugin export (commercial boundary), audio/MIDI device selection and
ASIO (2), standalone app (2), file browsing and sample loading (3), module
scanning (7), user skins (8), tabs and dockable windows (1), modal dialogs (5).

**A hypothesis match always beats a reject rule.** An item saying "iOS AUv3
build with a module scan folder" is flagged, not dropped — the constraint answer
is known, but *someone building on iOS* is the signal.

### The constraint rules read titles only, and that is a bug fix

The first live run auto-rejected a real Surge XT crash report — *"Surge XT CLAP
crashes REAPER on load (SIGSEGV in JUCE repaint)"* — because the reporter's
diagnostics happened to say *"The standalone app also works fine"*. An
incidental mention in a bug report is not a feature request, and **throwing away
crash reports is the worst thing this filter could do**.

So constraint rules match the **title**; the hypothesis flag still reads the
body, because its failure mode is the opposite and far cheaper — a false
positive merely flags an item for a human. Both behaviours are pinned by
selftest cases built from **the real incident**, not an invented example.

---

## Dedup, and why there is a rejection memory

Three gates, in order: the
[rejection memory](community-research-rejected.md) (human judgement), the
backlog (does it already cite this URL), then the constraint filter.

Without the memory, every run re-proposes the same rejected ideas and trains its
reader to skim — the failure A4's row calls "training the rubber-stamp reflex on
trivia". **Verified by A/B**: with `surge#7782` listed, a run reports
`1 already rejected before · 24 proposed`; with the file removed, the same run
reports `0 · 25` and the item reappears.

---

## Known limitations — read before trusting the output

1. **Ranking is engagement, which is a proxy for "worth a glance", not for
   relevance to TIDE.** The output still contains other projects' housekeeping
   ("Do a windows arm64ec build", "Release checklist for Surge XT 1.4"). The
   routine filters what TIDE has *ruled out*; it does not judge what TIDE needs,
   and it should not pretend to. **Triage is a human step, by design.** The next
   real improvement is a relevance signal, and it wants thought rather than more
   regexes.
2. **`--top 12` by default, and the withheld count is always printed.** No
   silent caps: a routine that quietly truncates reads as "this is everything".
3. **The library census is a census, not a feed.** `manifests-cache.json` holds
   only timestamps; per-plugin tags and descriptions live in 552 separate
   manifests, and fetching those every run is not courteous. A full tag census
   wants a shallow clone as an occasional job. **Measured: 553 plugins, 4,958
   modules** — worth recording because
   [process-review-2026-08-09.md](process-review-2026-08-09.md) describes the
   ecosystem as "8,000+ modules", which is roughly 1.6× the real figure.
4. **No state between runs except the rejection memory.** A run does not know
   what the last run proposed, so an untriaged item reappears. That is
   deliberate for now — the alternative is a seen-list that silently hides
   things nobody ever read.
5. **Not wired into CI, and it should not be.** It makes outbound network calls
   to third parties; a dev/agent tool like
   [dangling_private_includes.py](../scripts/dangling_private_includes.py). The
   `--selftest` is offline and safe to run anywhere.

---

## Triage

Everything it prints is a **proposal**. For each item: file it as a `PROPOSED:`
row per [decisions.md](decisions.md) if it is a genuine question, add it to the
backlog if it is obvious work, or list its key in
[the rejection memory](community-research-rejected.md) with a one-line reason.

Doing nothing is also fine — but then it will appear again next run, which is
the intended pressure.
