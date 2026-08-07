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

## Decision: GitHub Pages

Decided by Jeff, 2026-08-07. Free, custom domain, free TLS, and the page lives
in this repo so there is no second copy to drift. The shared-host route further
down stays documented as the fallback.

### What is already done

`.github/workflows/pages.yml` is committed. On every push to `main` that touches
`website/`, it uploads `website/` as the Pages artifact and deploys it — no
build step, because the site is one static file. It can also be run by hand from
the Actions tab (`workflow_dispatch`).

Until Pages is enabled it fails at `configure-pages` with **"Get Pages site
failed"**. That error means "Pages is not enabled yet", not that the workflow is
broken.

### DONE — went live 2026-08-08

`https://tidesynth.com` serves `website/index.html` over a Let's Encrypt
certificate, `www` 301s to the apex, and HTTP 301s to HTTPS. The checklist below
is kept as the record of what was done, with the two places it was **wrong**
corrected inline. See the JOURNAL entry for 2026-08-08 for the full account.

Two corrections worth reading before you touch DNS for any other domain here:

1. **The Domainz DNS editor is at <https://clients.domainz.net.nz/~/dns>.** It is
   *not* reachable from the domain's own product page, which offers only
   Lock/Unlock, Update Nameservers, Update Registrant and EPP code. Its Settings
   tab has labels and delegate access, and the separate "tidesynth.com (Domain
   Manager)" product — filed under *Email & Office Tools* — is metadata only.
   Nothing in that navigation leads to the zone. Go to `/~/dns` directly.
2. **The portal's "Export Zone" button is broken** — it returns 500 Internal
   Server Error every time. There is no working export, so a zone backup has to
   be transcribed by hand. One is kept at
   [dns-zone-tidesynth.com.txt](dns-zone-tidesynth.com.txt). A failed Export
   also leaves a dead modal behind that makes the *next* dialog you open render
   as a 500 — reload the page and the editor works fine. Do not conclude from
   that second 500 that DNS editing is broken; it is not.

Also worth knowing: the zone table and its dialogs do **not** appear in the
accessibility tree, so this page cannot be driven by anything reading the DOM
that way — it has to be done by eye.

### Go-live checklist (Jeff — repo settings and registrar)

1. **Enable Pages:** repo → Settings → Pages → Source: **GitHub Actions**.
2. **Run the workflow once** (Actions → "Deploy website to GitHub Pages" → Run
   workflow). The site is now at `jeffmcclintock.github.io/TideSynth/`.
3. **Custom domain:** same Settings page → Custom domain: `tidesynth.com` →
   Save. **Do step 4 first.** GitHub verifies the domain against the
   authoritative nameservers when you save, so setting it before DNS has
   propagated can fail the check and need retrying. Also make sure
   [website/CNAME](../website/CNAME) exists — with the *GitHub Actions* source
   the published artifact defines the served domain, and without that file a
   later deploy can silently clear what you set here.
4. **DNS, at the registrar** — apex `A` records to GitHub Pages. The zone
   editor is at `/~/dns`, not on the domain product page (see corrections
   above). **Replace** the existing records rather than adding beside them: the
   apex `A` pointed at `202.124.241.178`
   (`redirector.servers.netregistry.net`) and `www` CNAMEd to a dead Azure
   static-website endpoint. **Leave the MX and the smtp/imap/pop/pop3/webmail
   CNAMEs alone — tidesynth.com has live email on `nsserver.net.nz`,** and
   nothing about the website touches it.

   ```
   tidesynth.com.      A     185.199.108.153
   tidesynth.com.      A     185.199.109.153
   tidesynth.com.      A     185.199.110.153
   tidesynth.com.      A     185.199.111.153
   www.tidesynth.com.  CNAME jeffmcclintock.github.io.
   ```

   (Optional IPv6: `AAAA` records `2606:50c0:8000::153` through
   `2606:50c0:8003::153`.)
5. **Enforce HTTPS:** back on the Pages settings page, tick **Enforce HTTPS**
   once the certificate has been issued — it appears automatically after DNS
   propagates. In practice the certificate arrived **four minutes** after the
   custom domain was saved, not the hour budgeted here. Note GitHub sets
   `https_enforced` back to *false* when you save a new custom domain, so this
   step is not optional tidying — without it the apex serves plain HTTP. The
   redirect then takes a few more minutes to reach the edge: expect
   `http://tidesynth.com` to keep answering `200` rather than `301` for a short
   while after the setting reads as enabled. That lag is GitHub-side, not a
   misconfiguration.
6. **Verify the domain** (Settings → Pages → Verified domains, at the *account*
   level: <https://github.com/settings/pages>). Optional but recommended — it
   stops anyone else claiming `tidesynth.com` on Pages if the DNS ever points
   away temporarily.

After step 4 propagates, `https://tidesynth.com` serves `website/index.html`,
and every merged change to `website/` is live within a minute.

One consequence of the custom domain: the Pages URL space is the domain root, so
absolute paths in the page (`/foo.png`) work as written. While it is still on
the `github.io/TideSynth/` URL (between steps 2 and 4), absolute paths would
break — the page currently has none, and keeping it that way avoids caring.

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
