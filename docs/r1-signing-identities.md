# R1 — signing identities and accounts

Status **NEEDS-JEFF**, platform **—**. Lifted verbatim out of the
[BACKLOG.md](../BACKLOG.md) row by **A8**, 2026-08-12, when that file had
reached 76 KB and every run on three machines was reading all of it. The row
now carries the decision-shaped summary and points here; this file is the
detail. Wording below is unchanged from the row — only the line breaks are
new.

---

**Signing identities and accounts.** **Mostly answered 2026-08-08 by Jeff — (b)
and (d) are done, (a) is answered-by-default and wants a deliberate yes, (c) is
untouched.** (a) May TIDE sign under the existing Azure Trusted Signing account
(`SynthEditTrustedSigning`, profile `SynthEditCertificateProfile`,
`SE16/SynthEdit_store_win.yml:199-210`)? **Still open, but currently answered
by default: the credentials as configured ship TIDE under the SynthEdit
identity on both platforms** — see the branding note below. A second Azure
certificate profile is cheap if the answer changes. (b) **Confirmed** — Apple
Developer ID is `Developer ID Application: SynthEdit Limited (36SNPLRFK3)`, and
notarization runs on the app-specific-password route (`APPLE_ID` /
`APPLE_ID_PASSWORD` / `APPLE_TEAM_ID`), matching
`SynthEdit_cmake_mac.yml:223-244` so the recipe ports without rewriting
`notarytool`. An App Store Connect API key would be scoped and revocable where
the app-specific password is tied to Jeff's Apple ID and grants far more than
notarization — worth switching before anything consumes it, not urgent while
nothing does. (c) App Store listing for the iOS AUv3 (needed by M2, not v0.1) —
**not started.** (d) **Done early, ahead of C7** — 8 Actions secrets and 4
variables now exist on the public repo: secrets `AZURE_TENANT_ID`,
`AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`, `APPLE_CERT_P12_BASE64`,
`APPLE_CERT_PASSWORD`, `APPLE_ID`, `APPLE_ID_PASSWORD`, `APPLE_TEAM_ID`;
variables `AZURE_CODESIGN_ENDPOINT`, `AZURE_CODESIGN_ACCOUNT`,
`AZURE_CODESIGN_PROFILE`, `APPLE_SIGNING_IDENTITY`. The non-secret half is
deliberately in *variables* — those values are on every binary TIDE ships, and
hiding them only makes failures harder to read. Nothing consumes any of them
yet. **Two follow-ups, filed as R7.**
