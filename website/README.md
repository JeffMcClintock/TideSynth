# website/

The tidesynth.com holding page. BACKLOG **W1**.

One file, `index.html`. No build step, no dependencies, no JavaScript, and no
external requests of any kind — no fonts, no CDN, no analytics. W1 says "no
trackers"; having nothing to load is the cheapest way to keep that true and to
keep it verifiable by reading one file.

To preview it, open `index.html` in a browser. There is nothing to install.

## Naming — read before editing copy

Per [PLAN.md](../PLAN.md), decided 2026-08-08: the **product** is **TIDE Rack**;
the **organisation** is **TIDE Synth**, which may ship more than one plugin.
`tidesynth.com` is the organisation's address and is not changing.

So the page **leads with the product** — that is what a visitor came for — and
states the relationship exactly once, early: *"TIDE Rack is the first plugin
from TIDE Synth — hence the address."* A reader who typed `tidesynth.com` and
landed on something called TIDE Rack needs that resolved in the first screen,
not in a footer.

If a second plugin ever ships, this page becomes a list and the organisation
moves to the top. It is not that yet, and restructuring early would make it a
page about nothing.

Two things not to "fix":

- **The repo links say `TideSynth` and should.** The repository is the
  organisation's and keeps its name — the same answer the carve-out gave for
  `SynthEditLib`. See BACKLOG **N1**.
- **There is no bare "TIDE" left in the visible copy**, deliberately. It is
  ambiguous now that it prefixes both names. Write "TIDE Rack" or "TIDE Synth".

## The donation link — done

**Live since 2026-08-08: <https://ko-fi.com/TideRack>**, a plain `<a href>` in
the "It runs on donations" paragraph, plus `.github/FUNDING.yml` (`ko_fi: TideRack`)
for the repo Sponsor button. That was the last `TODO(jeff)` on the page; there
are no placeholders left in the HTML.

**Why Ko-fi**, recorded so it is not re-argued: this page's audience is DAW
users, not developers, and GitHub Sponsors requires the *donor* to have a GitHub
account. Ko-fi's URL exists the moment the handle is claimed; one-off tipping
matches "if it turns out to be useful"; and `ko_fi:` in `FUNDING.yml` still earns
the repo Sponsor button, so both surfaces come without GitHub Sponsors
enrolment. Liberapay is recurring-first, the wrong shape for a no-nag ask.
PayPal is the fallback if opening a new account is unwelcome — SynthEdit already
has one.

**Ko-fi offers a widget and a button image. Neither is used, and neither may
be** — only the plain profile URL. See the first rule below.

If the link ever changes, confirm the new URL resolves *before* committing it.
This one was checked against the live site first, after two earlier rounds where
it could not be: `<a href="#donate-url-tbd">` shipped as a link that went
nowhere (`453721e`), and the handle did not exist yet at the time of the commit
that chose the platform.

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

`.github/FUNDING.yml` carries the same handle and puts a Sponsor button on the
repo page — a second, free surface, and not part of this page. It was added only
after `ko-fi.com/TideRack` was confirmed to resolve: a `FUNDING.yml` naming an
unclaimed handle renders a button that 404s, the same dead-link failure as
`#donate-url-tbd`, relocated to the repo page.

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
