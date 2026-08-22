# Distribution — installers and downloads

The plan for getting TIDE from a build tree onto a user's machine, on every
platform, and for making the downloads reachable from tidesynth.com. Written
2026-08-07, at Jeff's direction, well before there is anything to ship — v0.1
([PLAN.md](../PLAN.md)) is the gate, and every R-item in BACKLOG is blocked on
**V1** for exactly that reason. The point of writing it now is that the plan
changes *how the release CI is shaped*, and that the accounts and identities it
needs (R1) have lead time.

## What already exists — reuse, do not reinvent

SynthEdit ships today, and its infrastructure was located by reading the
private `SE16` repo (paths relative to `SE16/`):

| Thing | Where | What it proves |
|---|---|---|
| Inno Setup installers | `SynthEdit2/installer/SynthEdit2.iss`, `SynthEditCL/installer/SynthEditCL.iss` | Windows installer tech is Inno Setup, with working precedent to copy from |
| **Azure Trusted Signing** | `SynthEdit_store_win.yml:205-207` — endpoint `eus.codesigning.azure.net`, account `SynthEditTrustedSigning`, profile `SynthEditCertificateProfile` | A Windows code-signing identity **exists and is paid for**. No new cert purchase needed if TIDE may sign under it (R1) |
| Apple signing + DMG | `SynthEdit_cmake_mac.yml:185-199` — `create_dmg.sh`, `codesign --timestamp` with `$(APPLE_CERTIFICATE_SIGNING_IDENTITY)`, comment "required for notarization" | An Apple Developer identity and a notarization-shaped pipeline exist |
| CI host | the `*.yml` files at the `SE16` root are **Azure Pipelines**, in the private repo | TIDE's release automation will be **GitHub Actions in the public repo** — the *secrets* (Azure signing creds, Apple cert, notary credentials) must be re-created as GitHub Actions secrets; the recipes port, the credentials do not follow automatically |

## Naming — decided 2026-08-08, settle nothing else against it

Three forms. Which one you use depends on who reads it:

| | Form | Where |
|---|---|---|
| **Display** | `TIDE Rack` (space) | the plug-in name a DAW shows, installer titles, the website — anything a person reads |
| **Shipped files** | `TIDE-Rack` (dash) | binaries, bundles, release assets — anything with a path or a URL |
| **CMake targets** | `TIDE_Rack` (underscore) | internal only, never shipped — see below |

**No spaces in any filename, ever.** That is the rule the other two serve. A
space becomes `%20` in every `releases/latest/download/` permalink, forever, and
R6's whole design is that those permalinks never change — so **a space in a
shipped filename is a bug you cannot fix later.** It also spares
`TrackFX_AddByName(tr, "TIDE-Rack.vst3")` any quoting gymnastics and lets
`install.sh` glob cleanly.

**Dashes rather than underscores for shipped files**, decided 2026-08-08 after
first choosing underscores. Both defend equally against spaces, so the choice is
style — but these filenames become *visible link text* on a no-JS page (R6), and
underscores get swallowed by link underlining in most browsers, where a dash
never disappears. It is also the near-universal release-asset convention
(`surge-xt-win64.exe`, `ardour-8.6.0.tar.gz`). **Do not mix the two:** the
briefly-considered `TIDE_Rack-macOS.pkg` implies `_` means "inside the name" and
`-` means "field boundary", a distinction nothing in the toolchain consumes.

**CMake targets keep the underscore, and that is deliberate rather than an
oversight.** `SE16/SynthEditSem/CMakeLists.txt:90` builds them as
`${PROJECT_NAME}_${kind}`, so a dashed project name would produce
`TIDE-Rack_VST3` — the mixed form again, in the one place you cannot avoid it.
Targets are never shipped, so leave them `TIDE_Rack` / `TIDE_Rack_VST3` and set
`OUTPUT_NAME` to the dashed form. Note `:45` also passes `PROJECT_NAME` into
`gmpi_plugin.cmake`, which likely feeds the bundle name and possibly an
Info.plist identifier — check where that lands before assuming `OUTPUT_NAME`
alone is enough (BACKLOG **N1**(a)).

The organisation is **TIDE Synth**; it does not appear in any artifact name.
Repo, domain and GitHub org keep their existing names ([PLAN.md](../PLAN.md)
naming section, BACKLOG **N1**).

## Per-platform artifacts

| Platform | Artifact (constant name) | Contents & install destination | Signing |
|---|---|---|---|
| Windows | `TIDE-Rack-Windows.exe` (Inno Setup) + `TIDE-Rack-Windows.zip` | `TIDE-Rack.vst3` → `C:\Program Files\Common Files\VST3\` | Azure Trusted Signing (installer **and** the .vst3 inside it) |
| macOS | `TIDE-Rack-macOS.pkg` | **AUv3** → `/Applications/TIDE-Rack-AUv3.app` (the extension rides inside it; **the app must be LAUNCHED ONCE** before the AU appears — measured 2026-08-23, see the note below), VST3 → `/Library/Audio/Plug-Ins/VST3/` | Developer ID + **notarize + staple** — an unnotarized pkg is effectively unopenable on modern macOS |
| iOS | — none on the website — | AUv3 ships inside a container app — the SAME wrapper macOS now uses — **App Store only**; the website links the App Store page as a plain text link | App Store pipeline (M2/M3 territory) |
| Linux | `TIDE-Rack-Linux.tar.gz` | `TIDE-Rack.vst3/` → `~/.vst3/`, CLAP → `~/.clap/`, plus a short `install.sh` that copies them | none — no signing convention on Linux |

**The macOS AUv3 needs one launch.** Copying `TIDE-Rack-AUv3.app` into
`/Applications` is NOT enough for the Audio Unit to appear in a host: measured
2026-08-23, `pluginkit` reports "(no matches)" 30 seconds after a copy to either
`/Applications` or `~/Applications`, and reports the extension within 12 seconds
of the app being opened. Whatever tells the user how to install this has to tell
them to open it once. The pkg puts the app in place; it cannot open it for them.

Notes:

- **Asset names carry no version.** The version lives in the release tag and in
  each binary's own version resource. This is what lets a static, script-free
  website link "the latest installer" — see below.
- **The `.gmpi` artifact is not shipped to end users** for now. `TIDE-Rack.gmpi` is
  the GMPI-format build; until there is a host story for it, the VST3 (and AU
  on mac, CLAP on Linux) are the user-facing deliverables. Revisit when GMPI
  hosting matures — one line in the release workflow either way.
- **macOS AUv3** ships inside an app, not a pkg — it arrives with the iOS/App
  Store work (M2), not with the website download. The pkg carries AU + VST3.

## Release flow

**Trigger:** push a tag `v*` (e.g. `v0.1.0`) on `main`.

**Pipeline (GitHub Actions, `release.yml` — future):** one job per platform
builds, signs, and uploads to a single GitHub Release, plus a `SHA256SUMS.txt`
covering every asset. GitHub Releases cost nothing, version themselves, and
keep bandwidth off the website host entirely.

**The C7 dependency, and the interim path.** CI cannot build TIDE until the
carve-out completes — `EditorLib` is private, so a public-repo runner has
nothing to link against. That does **not** block releasing:

> **Interim:** each platform's box builds and signs locally — exactly what the
> weekly runs already do — and uploads with
> `gh release upload v0.1.0 <files>`. Same tag, same release page, same
> permalinks. The automation is a convenience that arrives with C7; the
> *distribution design* works from the first v0.1 build.

Do not sign artifacts inside pull-request workflows, ever — the release
workflow runs on tag push only, so fork PRs never see the signing secrets.

## The website side

A Downloads section on the holding page, added **only when there is something
to download** — the "Status: in development, nothing to download yet" card is
honest today and gets replaced, not contradicted.

GitHub's `releases/latest/download/<asset>` permalink always redirects to the
newest release's asset of that name. With constant asset names, the download
links are **static `<a href>`s that never need updating**:

```html
<a href="https://github.com/JeffMcClintock/TideSynth/releases/latest/download/TIDE-Rack-Windows.exe">Windows</a>
<a href="https://github.com/JeffMcClintock/TideSynth/releases/latest/download/TIDE-Rack-macOS.pkg">macOS</a>
<a href="https://github.com/JeffMcClintock/TideSynth/releases/latest/download/TIDE-Rack-Linux.tar.gz">Linux</a>
```

No JavaScript, no version-number maintenance, no third-party requests from the
page itself (the request happens only when a link is clicked) — the page keeps
its "no cookies, no scripts, no tracking" footer honestly. iOS gets a plain
text link to the App Store page; Apple's badge images live on Apple's CDN and
would be the page's first external resource, so no badge.

## Identities and accounts — R1, the only item with lead time

1. **Windows:** may TIDE binaries sign under `SynthEditTrustedSigning`? The
   certificate subject becomes the publisher name users see in UAC prompts —
   if that says "SynthEdit Ltd" (or similar) on a TIDE installer, decide
   whether that is acceptable or whether TIDE needs its own certificate
   profile. Same Azure account either way; a second profile is cheap.
2. **macOS:** confirm the existing Developer ID identity may sign TIDE, and
   that notarization credentials (`notarytool` app-specific password or App
   Store Connect API key) can be issued for GitHub Actions.
3. **iOS:** an App Store listing for TIDE — new bundle id, free app. Needed by
   M2, not by v0.1.
4. **GitHub:** when C7 lands, re-create the above as Actions secrets in the
   public repo.

None of this costs new money on the likely path — the Azure account and Apple
membership exist. The decision content is naming and publisher identity, which
is Jeff's, like L1 and G1.
