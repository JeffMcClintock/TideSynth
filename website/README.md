# website/

The tidesynth.com holding page. BACKLOG **W1**.

One file, `index.html`. No build step, no dependencies, no JavaScript, and no
external requests of any kind — no fonts, no CDN, no analytics. W1 says "no
trackers"; having nothing to load is the cheapest way to keep that true and to
keep it verifiable by reading one file.

To preview it, open `index.html` in a browser. There is nothing to install.

## The one remaining placeholder — the donation link

The site is live (**H1**). This is the last `TODO(jeff)` left in the HTML.

**Platform decided 2026-08-08: Ko-fi.** What is missing is the *handle* — the
account does not exist yet, so `ko-fi.com/<handle>` does not resolve and the page
carries plain text with nothing to click.

Going live, in order:

1. **Claim the Ko-fi handle** at <https://ko-fi.com> — this is the step that
   makes the URL real, and it is the only part nobody else can do. Payout
   (PayPal or Stripe) can be connected afterwards; the page URL works as soon as
   the handle exists.
2. **Two edits in `index.html`, both inside that one comment block:** put the
   sentence above it back to "there **is** a link below", and swap the
   placeholder paragraph for the commented-out `<a href>` beside it, replacing
   `HANDLE`. Nothing else on the page moves.
3. **Add `.github/FUNDING.yml`** with `ko_fi: HANDLE` — see below.

**Do not guess the handle.** It cannot be verified from a sandboxed agent
session (ko-fi.com is not reachable from one), so it has to be pasted in by
whoever claims it.

**Why Ko-fi**, recorded so it is not re-argued: this page's audience is DAW
users, not developers, and GitHub Sponsors requires the *donor* to have a GitHub
account. Ko-fi's URL exists the moment the handle is claimed; one-off tipping
matches "if it turns out to be useful"; and `ko_fi:` in `FUNDING.yml` still earns
the repo Sponsor button, so both surfaces come without GitHub Sponsors
enrolment. Liberapay is recurring-first, the wrong shape for a no-nag ask.
PayPal is the fallback if opening a new account is unwelcome — SynthEdit already
has one.

**Ko-fi will offer you a widget or a button image. Take neither** — copy the
plain profile URL. See the first rule below.

Three rules that do not bend:

- **A plain `<a href>`, never an embedded widget.** Hosted donate buttons and
  button images ship third-party script and cookies; the footer promises
  neither. An outbound `<a href>` loads nothing, so the page keeps making zero
  external requests whichever platform wins. What the *destination* does on its
  own site is not this page's business and does not change that — it is the
  same arrangement as the two `github.com` links already on the page. Settled,
  not open.
- **No stand-in URL.** `<a href="#donate-url-tbd">` was tried; it rendered as a
  real link that went nowhere and read as a broken page (commit `453721e`).
- **No nagging.** [PLAN.md](../PLAN.md): a donation route the user has to go
  looking for is the intended outcome, not a failure of the design.

`.github/FUNDING.yml` does not exist yet, deliberately. One line — `ko_fi: HANDLE`
— puts a Sponsor button on the repo page: a second, free surface, and not part of
this page. Add it at step 3 above, **once the handle really exists**. A
`FUNDING.yml` pointing at an unclaimed handle renders a button that 404s, which
is the same dead-link failure as `#donate-url-tbd`, relocated to the repo page.

The in-plugin side is **not** this — that is BACKLOG **D1**, design-note only.

## The "open source" wording

The page says TIDE is open source under the ISC licence. That became true on
2026-08-07, when the repo went public and **L1** was resolved (ISC, matching
GMPI and gmpi_ui) with LICENSE files landed in both `TideSynth` and
`SynthEditLib`. Before that day the phrase would have been false — public with
no licence is all-rights-reserved — which is why the git history of this file
is careful about it.

One honest limit to keep in mind when editing: until the carve-out (C1–C7)
completes, the full plugin cannot yet be *built* from public code alone —
`EditorLib` is still private. The page claims open source, which is true of the
licence and the repos; it deliberately does not claim "build it yourself
today". Keep that distinction if you rewrite the section.

## Deploying

**GitHub Pages**, decided 2026-08-07. `.github/workflows/pages.yml` deploys
`website/` on every push to `main` that touches it — one static file, no build
step. The one-time enable-Pages and DNS steps are Jeff's; the checklist is in
[../docs/hosting.md](../docs/hosting.md), which also records how synthedit.com
is actually served (Astro → FTP → Apache, *not* Netlify despite appearances)
and why the shared host remains the documented fallback.
