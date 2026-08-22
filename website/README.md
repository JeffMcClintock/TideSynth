# website/

The tidesynth.com holding page. BACKLOG **W1**.

`index.html`, plus the two encodes of the hero image it serves. No build step,
no dependencies, no JavaScript, and no external requests of any kind — no
fonts, no CDN, no analytics. W1 says "no trackers"; having nothing third-party
to load is the cheapest way to keep that true and to keep it verifiable by
reading one file. The images do not weaken that — they are same-origin files
Pages serves next to the page, so a loaded page makes exactly two requests and
both go to tidesynth.com.

To preview it, open `index.html` in a browser. There is nothing to install.

## Naming — read before editing copy

Per [PLAN.md](../PLAN.md), decided 2026-08-08: the **product** is **TiDE Rack**;
the **organisation** is **TiDE Synth**, which may ship more than one plugin.
`tidesynth.com` is the organisation's address and is not changing.

So the page **leads with the product** — that is what a visitor came for — and
states the relationship exactly once, early: *"TiDE Rack is the first plugin
from TiDE Synth — hence the address."* A reader who typed `tidesynth.com` and
landed on something called TiDE Rack needs that resolved in the first screen,
not in a footer.

If a second plugin ever ships, this page becomes a list and the organisation
moves to the top. It is not that yet, and restructuring early would make it a
page about nothing.

Two things not to "fix":

- **The repo links say `TideSynth` and should.** The repository is the
  organisation's and keeps its name — the same answer the carve-out gave for
  `SynthEditLib`. See BACKLOG **N1**.
- **There is no bare "TiDE" left in the visible copy**, deliberately. It is
  ambiguous now that it prefixes both names. Write "TiDE Rack" or "TiDE Synth".
- **The lower-case `i` is not a typo — do not "correct" it to `TIDE`.** Decided
  2026-08-22: the name is set `TiDE`, on purpose, to be a little quirky. It is
  orthography only and does not reopen **N1**; the names themselves are
  unchanged. It binds prose, not identifiers — `TIDE_Rack.vst3`, the
  `TideSynth` repo and the `TideRack` Ko-fi handle all keep their own spelling,
  as do verbatim quotations of anything written before that date. This page's
  copy was converted the day it was ruled; most of the rest of the tree was
  not, so all-caps `TIDE` elsewhere is expected rather than stale. Ruling and
  scope: [../docs/decisions.md](../docs/decisions.md).

## The hero image — done

**Live since 2026-08-22.** `hero.avif` (19 KB), with `hero.jpg` (71 KB) as the
fallback, encoded from `../docs/images/hero-master.png`. The masters stay in
`docs/`; only the encodes ship.

**What it is, and why it earns the space.** The wordmark reads downwards — T,
i, D, E — standing over water, and its reflection reads **EDiT**. The two
halves share the word `synth` at the waterline, so the picture says *TiDE
synth* and *synth EDiT* at once. That is the same relationship the naming
section above needs three paragraphs to establish, and the picture lands it
before anyone reads a word.

**The artwork is shaped for the slot, and that is why it fits.** The original
render is 1024×1536 — a 2:3 portrait, far too tall to sit above a paragraph of
text without shoving the page off the screen. What ships is 1024×1061, and it
was **not uniformly squashed**: measured against the original, the top 795 rows
are byte-identical, and only the reflection *below the waterline* is
compressed, to 36% of its height. The wordmark is untouched, and the
foreshortening falls on the one element where it reads as correct rather than
as distortion — a reflection on a water plane receding from the viewer does
exactly that. Both files are kept: `hero-master.png` is what ships,
`hero-original-tall.png` is the 2:3 render it came from.

**The band it sits in** takes the artwork's own shape (`aspect-ratio:
1024/1061`) and is capped at `80vh`. Measured in a browser at 700×1000,
390×844 and 812×375, the artwork fills it exactly in all three — zero
letterbox in either axis, painted ratio 0.9651 against a source ratio of
0.9651. The `80vh` cap is there for the short, wide viewport — a phone held
sideways — where rather than let a full-width picture shove the page below the
fold, the aspect ratio wins and the band shrinks *proportionally*, to 290×300
at 812×375. So the picture is never letterboxed and never distorted, which
makes `object-fit: contain` and the `#00030a` background belt and braces rather
than load-bearing. Keep them anyway: `#00030a` is the artwork's own corner
colour, measured rather than guessed (the four corners run `rgb(0,3,7)` to
`rgb(1,3,9)`), so if some future viewport does make the band show, its edges
dissolve into the picture instead of framing it.

**`object-fit: cover` would destroy it.** cover crops, and there is nothing
here to crop: the wordmark runs from the top of the T down through the last
letter of its own reflection. Lose either end and the idea is gone.

**Two things measured rather than assumed:**

- **10-bit AVIF, not 8-bit.** Most of this image sits below `rgb(0,10,20)`,
  where 8-bit AVIF has too few levels to model the gradient and visibly
  blotches in the sky. Boosting both encodes 4× against the master shows it
  plainly. The recipe is `libaom, crf 20, yuv444p10le`; re-encode from the PNG
  master, never from a previous encode.
- **The page still makes zero external requests.** Loaded, it fetches
  `index.html` and `hero.avif` and nothing else — `hero.jpg` is only fetched by
  browsers without AVIF, which since Edge 121 is a small remainder. Never move
  either file to a CDN or an image host; that would trade the one property this
  page is built around for a few kilobytes.

**The rejected alternative**, so it is not re-proposed: a second render of the
same idea set in a neon Tokyo skyline, kept as `hero-alt-tokyo.png`. It was
dropped because the reflection — the entire point — competes with the
surrounding neon and stops reading at a glance, and because 415 of its 1536
rows are empty black.

**One judgement call, flagged rather than hidden.** The artwork carries the
*organisation's* name, and this page leads with the *product*. It is placed
above the `h1` anyway, so a visitor gets org mark → `TiDE Rack` → the sentence
tying the two together, which answers "why did tidesynth.com hand me something
called TiDE Rack" faster than the prose did alone. If that reads wrong, move
the block below the tagline; nothing else depends on its position.

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

## The SynthEdit Ltd credit — done

**Live in the footer: "TiDE Synth — by SynthEdit Ltd"**, linking to
<https://www.synthedit.com/>. BACKLOG **D2**, implementing the **R1(a)** ruling
of 2026-08-13.

**Why it exists**, so it is not read as marketing: TiDE ships **signed under the
existing `SynthEdit Limited` identity**, because a second certificate profile is
not affordable. A user therefore meets that company name in a Gatekeeper or UAC
prompt whether or not this page mentions it — and an unexpected company name in
an OS security dialog is a reason to cancel an install. The credit's job is that
they meet it *here first*, on TiDE's own terms.

Three things not to "fix":

- **The wording is R1(a)'s and names the *organisation*.** R1(a) wrote it "TIDE
  Synth" in August 2026, before the spelling was settled; the 2026-08-22 ruling
  restyled the four letters and changed not one word of the line. Not "TiDE
  Rack by SynthEdit Ltd" — that drops the middle term this whole site exists to
  explain (TiDE Rack is a product of TiDE Synth; TiDE Synth is SynthEdit Ltd's).
  See the naming section above.
- **Footer, not masthead.** R1(a) says "placed subtly". A banner would be
  advertising; a footer line is provenance.
- **It is a plain `<a href>`**, like every other link here, so the page still
  makes zero external requests. Same settled rule as the donation link.

The in-plugin half of D2 is a placement spec only, not built:
[../docs/about-pane.md](../docs/about-pane.md).

## The "open source" wording

The page says TiDE is open source under the ISC licence. That became true on
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
