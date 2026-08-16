# The about pane — what it holds, and what it may never become

BACKLOG **D2**. Produced 2026-08-16 on the macOS box. **Placement spec only —
nothing here is built, and none of it is v0.1.**

Two items now need the same surface, and were about to get two answers to one
question:

- **D2** — the *"TIDE Synth — by SynthEdit Ltd"* credit, ruled by **R1(a)**.
- **D1** — the donation route, designed in [donations.md](donations.md).

This file is the single answer. It says where that surface is and what may go on
it; it does **not** repeat D1's evidence about opening URLs, which stands on its
own and is not re-litigated here.

---

## The ruling

**One about pane, reached from the breadcrumb bar, holding exactly four things:**

| # | Content | Source | Notes |
|---|---|---|---|
| 1 | Product and version — *TIDE Rack, version X.Y* | — | the line people quote in bug reports |
| 2 | **TIDE Synth — by SynthEdit Ltd** | **D2** / R1(a) | wording verbatim; see below |
| 3 | Donation URL as selectable text + **Copy link** | **D1** | clickable *only* where that works |
| 4 | Licence — ISC, and where the source lives | L1 | one line, not a licence dump |

Nothing else without a ruling. The pane is a place things get *put*, so it is
exactly the surface that accretes until it becomes the splash screen PLAN
forbids.

---

## Why a pane, and why the breadcrumb bar

Both are forced, not chosen:

- **Constraint 1** — one view, two depths (rack, and structure view inside a
  Container). No tabs, no dockable windows, no separate editor window. The
  breadcrumb bar is the only persistent chrome that exists at every depth.
- **Constraint 5** — properties and module browsing are *panes in whichever view
  is showing*, not modal dialogs. An about pane is the same mechanism, so this
  adds a consumer, not a concept.

**There is prior art to copy the content of and not the container.** SynthEdit's
Wayland build already shows the donation URL as About-box text:

```cpp
// SynthEdit/SynthEditWayland/WaylandMainWindow.cpp:953-959
void WaylandMainWindow::showAbout()
{
    const auto text = L"SynthEdit " + versionString()
                    + L"\n\u00A9 SynthEdit Ltd 1995-2026"
                    + L"\n\nBuy me a coffee: " + kDonationUrlW;
    app_.SeMessageBoxAsync(text.c_str(), L"About SynthEdit", MB_OK, {});
}
```

Note what that already gets right — version, company, donation, all as plain
text — and the one thing TIDE cannot take: `SeMessageBoxAsync` is a **modal
dialog**, which constraint 5 rules out. **The content transfers; the container
does not.**

---

## The credit — exact wording, and what not to write

**"TIDE Synth — by SynthEdit Ltd"**, verbatim from R1(a).

**It names the organisation, deliberately.** TIDE Rack is a product of TIDE
Synth; TIDE Synth is SynthEdit Ltd's. Writing *"TIDE Rack — by SynthEdit Ltd"*
drops the middle term, and that term is the one PLAN's naming ruling exists to
establish — the same reason `tidesynth.com` leads with the product and then
states the relationship once.

**Why the credit exists at all, since "subtle branding" reads like marketing and
is not.** R1(a) decided TIDE ships under the **existing `SynthEdit Limited`
signing identity**, because a second Azure certificate profile is not affordable.
So the user meets that company name **in a Gatekeeper or UAC prompt whether or
not TIDE ever mentions it** — and an unexpected company name in an OS security
dialog is a reason to cancel an install. The credit's job is that they meet it
first, somewhere calmer, on TIDE's terms. That is a *trust* function, and it is
why the credit belongs on a surface the user can find rather than one that finds
them.

It follows that **the credit must never be the plug-in name**. The host-visible
name is *TIDE Rack* and the vendor string is *TIDE Synth* — see **P5**, which
owns those two fields. This pane is a third thing and does not change them.

---

## Rules that do not bend

1. **Nothing appears unprompted.** No splash, no first-run pane, no badge, no
   dot on the breadcrumb bar. PLAN: *"a donation route that a user has to go
   looking for is the intended outcome, not a failure of the design."* The
   credit inherits that restraint even though it is not an ask.
2. **Never a dialog, never a second window** — constraints 1 and 5.
3. **Nothing on this pane may block making sound.** It is reachable while the
   rack plays and dismissable without touching audio.
4. **No image assets for the credit or the donation.** No logo, no hosted donate
   button, no QR bitmap shipped "just in case". This is the plugin-side twin of
   the website's settled rule, and the reason is the same: an embedded donate
   widget is third-party script and cookies. The website's version of this rule
   is in [../website/README.md](../website/README.md).
5. **The donation line degrades to text, never to nothing.** Per
   [donations.md](donations.md): where the URL cannot be opened, the pane shows
   it and offers **Copy link** — measured extension-legal on both platforms. A
   link that silently does nothing is worse than text.
6. **Adding a fifth item needs a ruling**, recorded in
   [decisions.md](decisions.md). This is the whole reason the table above is a
   fixed list.

---

## One thing that is Jeff's, found while checking this

**The Ko-fi page does not identify itself as TIDE Rack's.** Opening
<https://ko-fi.com/tiderack> in a browser gives a page titled *"Buy Jef a
Coffee"*, display name **"Jef"**, bio *"I'm a dude in New Zealand"*. Nothing on
it says TIDE Rack, TIDE Synth, or SynthEdit.

That matters to **this** item specifically, because D2's entire justification is
making the money-and-identity trail legible on TIDE's own terms. A user who
clicks *"Donate to TIDE Rack on Ko-fi"* — the website's exact link text — and
lands on a stranger's tip jar has hit the same surprise the credit exists to
prevent, one hop later.

**It is not a repo change and not an agent's to make**: it is Ko-fi account
settings — page title, display name, and a line of bio. Filed as **D5**, and
noted here because whoever implements this pane will link the same URL.

**Also verified, so nobody re-checks it:** `ko-fi.com/TideRack` (the website's
capitalisation) and `ko-fi.com/tiderack` (the Wayland code's) reach the **same
page** — Ko-fi canonicalises to lowercase. The inconsistency is harmless and
neither spelling is a dead link. `curl` cannot show this: Ko-fi returns **403 to
any user-agent it does not like, including for handles that do not exist**, so a
status-code check proves nothing here and a real browser is required.

---

## What is not decided

- **What the pane looks like.** This is placement and content, not visual
  design. Whoever builds it should expect **U1**'s audit to matter, since the
  rack-mode pivot changed what the breadcrumb bar is.
- **The installer credit field**, R1(a)'s third option. Deferred rather than
  rejected: there is no installer, and the whole release series (**R2**–**R6**)
  is blocked on there being something to ship. Revisit with the installer, not
  before.
- **Whether the pane carries build metadata** beyond the version string.
  Probably yes for bug reports, but it is a fifth item and rule 6 applies.
