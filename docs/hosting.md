# Hosting tidesynth.com

Nothing about hosting was written down anywhere before this. All of the
synthedit.com facts below were reconstructed from
`JeffMcClintock/synthedit-website` on 2026-08-07 — they are read out of that
repo's config, not from the server itself, so treat the server-side details as
"what the deploy config believes" until someone logs in and looks.

## What synthedit.com actually is

Not Netlify. `netlify.toml` sits in that repo's root and is **vestigial and
misleading** — the real deploy is GitHub Actions → FTP
(`.github/workflows/deploy.yml`):

```yaml
server:     ftp.synthedit.com
username:   synth
server-dir: /domains/synthedit.com/public_html/_site/
```

So: an Astro static build, FTP'd to a traditional Apache shared host. The
`/domains/<domain>/public_html/` path is the **DirectAdmin** convention (cPanel
uses a different layout), and the key point is that it is **already
per-domain** — the account is structured for more than one.

That root also does real work. `server/root.htaccess` maps the Astro build onto
the domain root but falls through to the old SilverStripe CMS for `/purchase/`,
`/members/`, `/downloads/` and friends. It is hand-maintained and explicitly
**not** deployed by CI, so it drifts silently.

## Recommendation

**Use GitHub Pages** now that the repo is public — it is free, the page already
lives in this repo, and it keeps the site beside the source. The shared-host
route below stays documented because it also works, costs nothing extra, and is
the fallback if Pages ever proves limiting.

The rest of this section describes that fallback.

## Fallback: additional domain on the synthedit.com host

Add tidesynth.com to the same hosting account as an additional domain. It gets
`/domains/tidesynth.com/public_html/`, an independent document root. Two sites,
one account, no interaction.

The deploy is then the existing workflow with one line changed:

```yaml
server-dir: /domains/tidesynth.com/public_html/
```

**Do not put TIDE at `synthedit.com/tide/`.** It would land inside the
SilverStripe fallthrough rules in that hand-maintained `.htaccess` — the one
piece of the setup that is already fragile — and it blurs a boundary worth
keeping sharp, since TIDE is free and SynthEdit is not.

Costs nothing extra: the server is already paid for.

### What only Jeff can check

1. **Does the plan allow additional domains?** Entry-level shared plans are
   sometimes single-domain. Control panel → Domain Setup.
2. **DNS** — tidesynth.com's nameservers or A record must point at the host.
3. **TLS** — a certificate for the new domain; usually one click for Let's
   Encrypt in DirectAdmin.
4. **FTP credentials** — the `synth` user can probably reach `/domains/` and
   therefore both trees. A separate user scoped to the TIDE domain is tidier and
   keeps the GitHub secret for one site from having write access to the other.

## Free hosting, if the shared host does not work out

TIDE is free and (eventually) open source, so every major static host's free
tier applies. Ranked for this specific page.

| Option | Custom domain + TLS | Catch |
|---|---|---|
| **GitHub Pages** | yes, free | Static only. Repo is already public, and the page already lives in it |
| **Cloudflare Pages** | yes, free | 500 builds/month. Cloudflare terminates TLS, so they see the traffic |
| **Netlify** | yes, free | 100 GB/month, 300 build-min/month. Config already exists in the sibling repo |
| **GitLab / Codeberg Pages** | yes, free | Codeberg is a non-profit and a good ideological fit; smaller, slower |

**GitHub Pages became the obvious choice on 2026-08-07**, when Jeff made
`TideSynth` public. Pages from a private repo needs a paid plan; from a public
one it is free, and it serves a custom domain with a free certificate. The page
already lives in this repo, so there is nothing to move and no second place for
it to drift out of sync with — point Pages at `website/` (or a `gh-pages`
branch), add a `CNAME`, and set the DNS.

That makes it the lowest-effort option **and** the one that keeps the site next
to the source, which is worth something for a project whose whole pitch is being
developed in the open.

Two caveats specific to Pages: it is static-only (fine — this page is one HTML
file), and the repo being public means the site's whole history is public too,
including anything ill-advised committed to `website/`.

Two notes before picking a free host:

- **A free static host is not obviously better than what already exists.** The
  shared host is paid for, already automated, and already understood. The real
  argument for moving is resilience and not coupling a free project's uptime to
  the commercial site's hosting bill — not cost.
- **Watch the "no trackers" rule.** W1 forbids trackers, and none of these
  inject client-side script by default. But Cloudflare and Netlify both offer
  one-click analytics, and Vercel's free Hobby tier is **non-commercial only**,
  which a donation link makes at best ambiguous. Vercel is left off the table
  above for that reason.

## Not decided here

Which of these to use. The holding page (`website/`) is deliberately a single
self-contained HTML file with no build step, so it can be dropped onto **any** of
them — including the existing FTP host — without committing to a toolchain.
Deployment is Jeff's, per W1.
