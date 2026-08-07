# website/

The tidesynth.com holding page. BACKLOG **W1**.

One file, `index.html`. No build step, no dependencies, no JavaScript, and no
external requests of any kind — no fonts, no CDN, no analytics. W1 says "no
trackers"; having nothing to load is the cheapest way to keep that true and to
keep it verifiable by reading one file.

To preview it, open `index.html` in a browser. There is nothing to install.

## Before this goes live

One placeholder is marked `TODO(jeff)` in the HTML — the donation URL. The
platform has not been chosen, and it must stay a plain `<a href>`: hosted donate
*widgets* ship third-party script and cookies, which W1 forbids.

The repo went public on 2026-08-07, so the "Source" section now links it. That
was the other placeholder.

## What the page must not say

**"Open source."** Public is not open source. Neither `TideSynth` nor
`SynthEditLib` has a LICENSE file, so both are all-rights-reserved by default —
readable by anyone, legally usable by no one. PLAN.md is explicit that TIDE
cannot claim the phrase until **L1** picks a licence. The current wording says
"developed in the open" and states plainly that the licence is unsettled, which
is true today. If you edit that section, keep it true.

## Deploying

Not from here — W1 leaves deployment to Jeff. See
[../docs/hosting.md](../docs/hosting.md) for how synthedit.com is actually
served (Astro → FTP → Apache, *not* Netlify despite appearances), why
tidesynth.com wants its own document root rather than a subdirectory, and what
the free-hosting options are.
