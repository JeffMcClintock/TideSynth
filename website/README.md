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
