# Journal

Append-only. Newest at the top. One entry per run.

**This file is the handoff.** Each weekly run starts with no memory of any
previous run — what is written here is the only thing the next run knows. An
entry that says "made progress on the view" is worthless. An entry that says
"the structure view fails to measure because drawingHost is null until setHost
runs; fixed by reordering, see commit abc123" is the whole point.

## Rotation — do this as part of STEP 4, every run

Every run on three machines reads this file in full, so its size is a cost paid
forever. It hit **192 KB across 37 entries in six days** before the first
rotation (**A8**, 2026-08-12). Nothing is ever deleted or rewritten — old
entries just move to a per-month archive.

**The rule, applied after you append your own entry:**

1. Move the oldest entries out, in order, into `JOURNAL-<YYYY>-<MM>.md` for the
   month each entry belongs to, appending **below** what is already there so the
   archive stays newest-first. Copy the template from
   [JOURNAL-2026-08.md](JOURNAL-2026-08.md) if that month has no file yet.
2. Stop when this file is **under 60 KB**, or when the floor is reached —
   whichever comes first. **The floor is the LATER of: the four most recent
   entries, or every entry carrying the most recent date.** The floor always
   wins; a busy day pushing this file over 60 KB is correct, not a rotation
   failure.
3. Never edit an entry while archiving it. The archive is the record.

**Why a date and not a duration (A24, 2026-08-20).** A24 asked for a time-based
floor — *"retain everything from the last 7 days"* — and measuring what that
costs is what killed it. Entries per day, counted across both files:

| window | entries | bytes |
|---|---|---|
| last 1 date | 9 | 63 KB |
| last 2 dates | 25 | 164 KB |
| last 3 dates | 51 | 301 KB |
| **last 7 dates** | **112** | **651 KB** |

Every run on three machines reads all of it, so 7 days is **3.4× the 192 KB that
triggered A8 in the first place** — the remedy would have been twenty times more
expensive than the problem. Even two days is worse than the state A8 was created
to fix.

So the floor is **one date**, which bounds the cost at roughly a day's work while
guaranteeing a run can always see everything that happened most recently — the
failure A24 correctly identified, where a 4-entry floor at ten entries a day
bought under half a day. On a quiet week the four-entry floor still binds and
nothing changes.

**What this does NOT fix, filed as A30:** the durable lessons still age out.
Rotation moves an entry's *"Learned"* bullets into the archive with it, and no
run reads the archive. The cheap answer is a standing digest that never rotates;
the expensive one is reading 651 KB.

A month splits across both files as it ages — recent entries here, older ones in
the archive. That is why step 1 says "the month each entry belongs to".

**Archives:** [JOURNAL-2026-08.md](JOURNAL-2026-08.md).

Template:

```
## YYYY-MM-DD — <machine> — <BACKLOG id>

**Did:** what actually changed.
**Result:** built / tested / failed, with the real output.
### Correction: Ardour IS a host here, and it settles the question

**Jeff asked "don't we have Ardour host?" — yes, and that makes three separate
claims of mine wrong.** I wrote in the row, both PR bodies and the issue that
closing this needed REAPER on a win/mac box. **Ardour 8.4 is installed on this
box**, `ardour-vst3-scanner` answers precisely this question, and **my own memory
note from 2026-08-19 records using it**, including the
`LD_LIBRARY_PATH=/usr/lib/ardour8` quirk it needs.

```
BROKEN (main):  VST3 not a valid bundle:
                  '.../TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE_Rack_VST3.so'
FIXED  (both):  [Info]: Found Plugin: TIDE Rack
                  uid=506C7567696E474D504920501951ED43 category="Instrument|Synth"
                  n_outputs=2 n_midi_inputs=1
```

Ardour derives the payload name from the bundle name — exactly the rule GMPI's
own comment states — so **the Linux VST3 is unloadable today, not merely oddly
named**, and the fix is host-verified on the platform that has the bug. The
scanned UID also matches the one in all five `.rpp` fixtures.

**The lesson is not "use Ardour".** It is that I asserted an environment limit
three times without testing it, while holding a note that contradicted it.
"Not verifiable here" is a claim about the machine, and it deserves one command
before it goes into a row, two PR bodies and an issue.

Ardour's cache entry from the scan pointed into a scratch tree and was removed;
Jeff's other nine cached plugins were left alone.


**Learned:** anything the next run would otherwise rediscover the hard way.

0. **"Not verifiable on this box" is a measurable claim, and I shipped it three
   times unmeasured.** Ardour was installed the whole time and my own memory note
   named the command. Check the machine before writing a limit into a row.
**Next:** what should happen next, and why.
**Branch/PR:** link.
```

---

## 2026-08-22 — macos — v0.1.0: Windows and Linux shipped, macOS wanted a certificate nobody had sent

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** cut `v0.1.0` and watched the pipeline run for the first time. **Windows
and Linux succeeded, signing included.** macOS failed at `Package (macOS)`:

```
productbuild: error: Could not find appropriate signing identity for
              "Developer ID Installer: SynthEdit Limited (36SNPLRFK3)"
```

### The correction, and it is mine

Earlier today I wrote on R5 that the missing credential was *"now
provisioned"*, because `APPLE_INSTALLER_SIGNING_IDENTITY` had appeared in the
`release` environment between one check and the next.

**The variable was provisioned. The certificate was not.** The workflow logs the
keychain after import, and it held exactly one identity — `E112A74081E6…`, the
Developer ID **Application** cert. `APPLE_CERT_P12_BASE64` carries only that
one, so `productbuild` had nothing to sign the pkg with.

Naming an identity is not the same as shipping its private key, and I treated a
variable appearing as evidence that the credential behind it existed. It is not
even weak evidence — the two are stored in different places, by different
mechanisms, for different reasons.

**The logging is what caught it in seconds.** I put `security find-identity -v
-p codesigning` at the end of the import step "so the job says what it can
actually sign with". That line turned a one-word error into a diagnosis.

### Two risks this run retired

**The ambiguity hazard is dead.** I flagged that the mac box holds two valid,
identically-named Developer ID Application certs, and that if
`APPLE_CERT_P12_BASE64` carried both, `codesign` would fail as *"ambiguous"*. It
carries one. `codesign` signed cleanly, and the risk is now closed by
measurement rather than left open as a caveat.

**R3a is confirmed in CI, not just locally.** Everything up to `productbuild`
worked on macOS: the AU built, `codesign` reported the component *"valid on
disk"* and *"satisfies its Designated Requirement"*, and `pkgbuild` added
**both** payloads. The change I landed an hour before the tag did what it
claimed on a machine that had never seen it.

### What this cost, and what it did not

The failed leg cost about an hour of macOS build time and no artifacts —
`publish` is `needs: build`, so it skipped rather than publishing a partial
release. **No half-finished release was created, and no tag needs deleting.**
That is `fail-fast: false` plus a gated publish doing exactly their job.

**Learned:**

- **A configuration variable naming a credential is not the credential.** They
  live in different stores. Seeing `APPLE_INSTALLER_SIGNING_IDENTITY` appear
  told me its *name* was known, and I wrote "provisioned" — which is a claim
  about the private key, and I had checked nothing about the private key.
- **Log what the job can actually do, not what it was configured to do.**
  Printing the keychain's identities after import turned this from "signing
  failed" into "the keychain has one cert and it is the wrong kind" with no
  extra round trip.
- **A release that fails before `publish` costs time and nothing else.**
  `needs: build` meant no partial release, no orphaned assets and no tag to
  delete. Worth keeping in mind against the temptation to publish per-platform
  as each finishes.
- **Two platforms passing is real evidence.** Azure Trusted Signing is now
  proven end to end on a real tag, which no amount of structural assertion could
  have established.

**Next:** **Jeff exports a `.p12` containing BOTH identities** — both are on the
mac box (`security find-identity -v` lists `D55D4DDE…` "Developer ID Installer",
valid to 2027-02-01) — base64s it and updates `APPLE_CERT_P12_BASE64`. **No
workflow change is needed**: `security import` handles a multi-identity P12 and
the import step already passes `-T /usr/bin/productbuild`. Then re-run the
failed job. **Notarization is still unverified** — the run never reached
`notarytool`, so Apple has never seen a submission and **R6** is blocked on the
same question it was this morning.

**Branch/PR:** `tide/mac/R5-installer-cert-finding` — TideSynth, backlog and journal only.

---

## 2026-08-22 — macos — ccache went into build.yml and not release.yml, and the numbers are in

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** Jeff asked whether the macOS build was still slow and whether a
self-hosted runner was needed. Both halves of the answer turned out to be
measurements I had promised and not yet taken.

### The gap: I fixed CI and not releases

S30 added ccache to `build.yml`. `release.yml` has **zero** ccache references —
I wrote them as separate items and never went back. So every release paid the
full ~60 minutes on macOS, which is the one place the wait is actually felt,
because a person is standing there waiting on a tag.

Now fixed, with the cache key prefix **deliberately matching `build.yml`'s**: a
tag run can read caches created on the default branch, so a release starts warm
from whatever `main` last compiled rather than from nothing.

### The measurement I owed

macOS `Build` step, either side of the ccache merge:

```
02:36   71.5 min    before
02:36   60.1 min    before
03:26    0.3 min    after
04:21    0.2 min    after
```

**I was about to report that as "60 minutes to 15 seconds" and stopped.** The
one hit-rate sample I pulled read **66.5% (1068 hits / 537 misses)** — and 537
C++ compiles do not finish in twelve seconds. Correlating properly showed I had
taken the duration from one run and the statistics from another.

So what is solid is narrower than the headline: **both post-ccache macOS builds
finished in well under a minute against 60+ before.** The exact speedup is not
established, and both post-ccache runs were docs-and-backlog merges whose C++ was
largely unchanged — a run that genuinely recompiles will be slower than 0.2 min.

That is still decisive for the question Jeff asked. **No self-hosted runner is
needed:** that option was sized against a 60-minute build, and the build is no
longer 60 minutes.

### A choice worth naming rather than sliding past

This caches the build of a **signed, shipped artifact**. ccache keys on
preprocessed source, compiler and flags, so a hit is a byte-identical object —
the same assumption an incremental local build makes every day. It is a
deliberate trade, not an oversight, and the comment at the point of use says how
to force a cold build if it is ever in doubt.

**Learned:**

- **Two workflows that build the same thing need the same fixes.** I treated
  "CI is slow" and "releases are slow" as one problem and fixed one file. The
  release path is the one with a human waiting on it.
- **Correlate a duration and its statistics to the same run before quoting a
  ratio.** 66.5% hits alongside a twelve-second build is a contradiction, and the
  contradiction was mine — two different runs. The narrower claim survives; the
  headline number did not.
- **Say what a cache key prefix couples.** `release.yml` sharing `build.yml`'s
  prefix is what makes a release start warm, and it silently stops working if
  either is changed alone.
- **An option sized against an old measurement expires with it.** The
  self-hosted runner was the right answer to a 60-minute build. It is not the
  right answer to this one, and nothing about the runner changed.

**Next:** the v0.1.0 rerun is in flight and will **not** benefit from this — it
started before this branch exists. The first release that does is the next tag.
**Notarization is still the unproven step**; nothing here touches it.

**Branch/PR:** `tide/mac/S30-ccache-in-release` — TideSynth.

---

## 2026-08-22 — macos — R3a: the AU goes into the pkg, before the first tag

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** Jeff asked for a `v0.1.0` tag. Checking the preconditions first turned
up that `package-macos.sh` still carried this, in its header:

> *"WHAT IS DELIBERATELY NOT HERE: the AU … TIDE does not build one … BACKLOG M1
> is BLOCKED."*

**M1 landed earlier today.** So a `v0.1.0` cut right then would have published a
macOS pkg missing a payload `distribution.md` promises — behind a
`releases/latest/download` permalink R6 undertakes never to change. Jeff chose
to land R3a first.

### What the packager does now

Stages `TIDE-Rack.component` into `/Library/Audio/Plug-Ins/Components` beside the
VST3, signs both in one loop, and **refuses to build without it**. The AU is
required, not optional: a pkg that silently omits half its stated payload is
worse than one that fails to build.

**Verified by expanding the pkg**, not by trusting the build:

```
/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3
/Library/Audio/Plug-Ins/Components/TIDE-Rack.component
    CFBundleExecutable   TIDE-Rack     (== the binary)
    CFBundleIdentifier   com.tidesynth.tiderack.au
    manufacturer/subtype Dsyh / Drck
```

### The guard that exists because this cost a day

The script now asserts `CFBundleExecutable` names a binary that is actually
present. That is the M3/GMPI#8 failure — a plist naming the wrong executable
**builds and installs fine**, and macOS simply declines to register the
component. The moment the artefact is sealed into a pkg is the last place that
check is cheap.

Both guards negative-controlled rather than assumed:

```
component removed          -> error: no TIDE-Rack.component
CFBundleExecutable wrong   -> error: ... Contents/MacOS/TIDE-Rack_AU is missing
restored                   -> AU executable check passes, pkg written
```

**Learned:**

- **Check the preconditions of a release before cutting the tag, not after.**
  The stale header was three lines of comment and would have become a published
  artifact with a permanent URL. Releases are the one place "we'll fix it in the
  next one" costs a version number.
- **A comment that was true when written is a liability the moment its subject
  changes.** "M1 is BLOCKED" was accurate for weeks and wrong for six hours, and
  six hours was enough to nearly ship on it.
- **Put the check where the artifact is sealed.** The AU executable-name
  mismatch is invisible at build time and invisible at install time; it only
  shows up when a host declines to load. Packaging is the last cheap moment.
- **Sign every bundle in the payload, not the first one.** An unsigned component
  inside a signed pkg passes a casual check and fails Gatekeeper.

**Next:** **cut `v0.1.0`.** The pipeline has never been executed: the tag will
pause at the `release` environment for Jeff's approval, then build, sign,
notarize and publish. **NOT verified here: installing the pkg** — this box
produces an unsigned, unnotarized one and the script says so. Whether macOS
accepts the installed AU end-to-end is what the release run answers, and **R6**
depends on it.

**Branch/PR:** `tide/mac/R3a-au-in-pkg` — TideSynth.

---

## 2026-08-22 — macos — E1c's deciding render: my hypothesis is refuted, and the row is still open

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff asking what is next

**Did:** ran the macOS half of the experiment the linux box built. It was
waiting on this platform specifically, `SynthEditCL` was already built here, and
the outcome was **pre-committed** on the row so it could not be rationalised
afterwards.

### The answer, and it is not the one I predicted

```
osc_naive_pitched   null=-73.5 dBFS  peakdiff=-68.7   (pitch PINNED to 5 V)
osc_naive_sine      null=-73.5 dBFS  peakdiff=-68.7   (pitch UNDRIVEN)
```

Identical. **Driving the pitch input changes nothing.**

By the row's own pre-commitment that is the *"module is the variable after all"*
branch. **The undriven-pitch-input hypothesis was mine**, argued in this journal
two entries ago from the fact that `osc_naive_sine` was the only case with an
unconnected pitch pin. It is dead, and it took one render to kill — which is
exactly what the linux box built the case for.

Worth noting the design that made it decisive: both cases render at *exactly*
440.0 Hz, verified by zero-crossing count before either was trusted. Had the
frequencies differed, a different phase increment would have confounded the
residual and the experiment would have proved nothing.

### But the row is NOT closed, and saying it was would be the real error

`voice_midi_note` uses the **same** `SE Oscillator (naive)` and lands at
**−123.1 dBFS / −90.3 peakdiff (1 LSB)** against these two at −73.5 / −68.7
(12 LSB). If the module were the variable, all three would agree. They do not.

So I checked the two obvious confounds rather than reporting the binary and
stopping:

- **level** — `voice_midi_note` peaks at **−6.6 dBFS** against the oscillators'
  −6.0. Its smaller residual is not a quieter signal.
- **RMS averaging over an enveloped render** — its **peakdiff** is 1 LSB, so the
  worst single sample is genuinely better, not merely averaged down by a decay.

Both dead. What actually differs is the **pitch value**: `voice_midi_note` plays
MIDI note 64 (≈329.63 Hz) while both `osc_naive_*` cases sit at 440.0 Hz. And
E1a already tied this residual to *"a frequency offset of 0.15 ppm = ~2.5 ULP at
single precision"* — a property of **the specific pitch value's phase
increment**, not of the module and not of whether the input is driven.

The next single-variable case writes itself: the naive oscillator with pitch
pinned to note 64's value instead of 5 V.

### What I did not change

**The gates.** The new case's own `tolerance_reason` says to re-tighten *or leave
alone on the evidence*, and the evidence says leave them: at −73.5 dBFS it is
genuinely drift-class. **`prefab_oscillator` and `prefab_filter` are untouched
and unresolved** — they use the CORE `Oscillator` at 5 V and measure −131.1 /
−121.4, so they remain rounding-class cases carrying drift-class gates. That is
the part of E1c with a real cost, and this run did not address it.

**Learned:**

- **A pre-committed binary outcome is worth the effort of setting up.** Writing
  both interpretations down *before* the render meant the −73.5 could not be
  read as anything other than "my hypothesis is wrong". I would not trust myself
  to be that clean about it afterwards.
- **Answering the experiment's question is not the same as answering the row's.**
  The case settled "is it the driven input" definitively and left "why does
  `voice_midi_note` differ" exactly where it was. Reporting the binary and
  closing would have buried a contradiction the same data contains.
- **Check the confounds on the case you are ARGUING FROM, not just the one you
  ran.** `voice_midi_note` was my counter-example, so its level and its peakdiff
  were the things that had to be eliminated — two renders, and both candidate
  explanations died.
- **A residual can be a property of the VALUE, not the code.** 440 Hz and 329.63
  Hz have different phase increments and therefore different rounding behaviour
  in single precision. "Which module" and "which input" were both the wrong axis.

**Next:** **one more case** — naive oscillator, pitch pinned to MIDI note 64's
value — settles whether the discriminator is the pitch value. It can run on
either platform against a reference seeded on the other. **E1c stays TODO**:
`prefab_oscillator` and `prefab_filter` still carry drift-class gates for a
mechanism their scripts do not exhibit, and that is the row's actual cost.

**Branch/PR:** `tide/mac/E1c-mac-half` — TideSynth.

---

## 2026-08-22 — macos — R5: the release workflow, and the credentials were already there

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** wrote `.github/workflows/release.yml` — tag `v*` → build, sign, notarize,
publish one Release with constant-name assets and `SHA256SUMS.txt`. **Jeff must
push it**; the fleet token carries `repo` scope only, checked again rather than
remembered.

### The credentials already existed, and nothing said so

I went looking for what R5 would need Jeff to provision. Repo-level secrets are
genuinely empty — `{"total_count":0,"secrets":[]}`, readable and zero. But the
repo has **environments**, and `release` is fully stocked:

```
secrets    APPLE_CERT_P12_BASE64  APPLE_CERT_PASSWORD
           APPLE_ID  APPLE_ID_PASSWORD  APPLE_TEAM_ID
           AZURE_CLIENT_ID  AZURE_CLIENT_SECRET  AZURE_TENANT_ID
variables  APPLE_SIGNING_IDENTITY
           AZURE_CODESIGN_ACCOUNT  AZURE_CODESIGN_ENDPOINT  AZURE_CODESIGN_PROFILE
```

R1 recorded the *identities*; nothing recorded that they are already wired into
a GitHub environment. I nearly wrote a workflow that asked Jeff to add them.

**And it carries `required_reviewers: JeffMcClintock`.** Any job naming that
environment pauses for approval before a secret is decrypted. That is a stronger
guarantee than this row's own "never on PRs" clause and it cost nothing to
adopt — so the build job names the environment, and the eight signing secrets
are unreachable without a human.

### One credential was missing — and Jeff provisioned it the same session

`productbuild --sign` needs a **Developer ID *Installer*** certificate. That is a
different certificate from the *Application* one that signs bundles, and
`package-macos.sh:95` reads `APPLE_INSTALLER_SIGNING_IDENTITY` — which does not
exist in the environment.

Without it the pkg is payload-signed but **not installer-signed**, which
`notarytool` refuses to notarize and macOS refuses to install. The workflow fails
the job with a named error rather than publishing that quietly.

**It exists now.** Jeff asked where to find it; `security find-identity -v` on the
mac box has exactly one — `Developer ID Installer: SynthEdit Limited
(36SNPLRFK3)`, valid to 2027-02-01 — and the `release` environment already
carried a matching `APPLE_INSTALLER_SIGNING_IDENTITY` by the time I looked again.
**Note the policy flag matters:** `security find-identity -v -p codesigning` does
NOT list Installer certificates, because signing a `.pkg` is a different policy
from signing code. Searching with the codesigning filter would have concluded the
cert was absent when it was sitting right there.

**And a hazard the same command turned up:** the box holds **two valid,
identically-named** `Developer ID Application: SynthEdit Limited (36SNPLRFK3)`
certificates — `E112A740…` from 2026-03-26 and `CEFB950D…` from 2026-08-08.
`codesign` matches on the common name, so if `APPLE_CERT_P12_BASE64` carries
both, signing fails with *"ambiguous (matches N identities)"*. I cannot inspect
the P12, so this is a **named risk, not a diagnosis** — documented at the point
of use, with the fix (use the SHA-1 hash; a hash cannot be ambiguous).

### A design error I made and caught before shipping it

My first draft signed the Windows artifacts *after* packaging, with one
`files-folder: dist, filter: exe` step. That is wrong, and wrong in the way that
looks right: the packager **copies** the built DLL into the bundle, the zip and
the installer, so signing afterwards leaves three unsigned copies inside signed
containers. `distribution.md` says the installer *and the .vst3 inside it*.

Windows now signs in two passes — DLL, then package, then installer — with a
`Get-AuthenticodeSignature` check that the installer's signature is `Valid`
rather than merely present.

### What could be verified, and what could not

A release workflow cannot be run without cutting a real tag, so I asserted its
structure instead of its behaviour. Ten checks, all passing:

```
trigger is push-tags only            top-level permissions are read-only
no pull_request / _target trigger    build gated on the release environment
build has no write permission        publish is the only writer
publish touches no signing secrets   matrix does not fail-fast
windows: sign -> package -> sign     every asset verified before upload
```

Every third-party action was checked against its registry rather than guessed —
and that caught one: I wrote `azure/trusted-signing-action@v0`, which **does not
exist**. It is at v2. I also read its `action.yml` to confirm the eleven input
names I used are real.

**Not verified, and it cannot be:** that any of this runs. No tag has been cut,
nothing has been notarized, and Apple's verdict on a real submission is the open
question **R6** depends on.

**Learned:**

- **Check the ENVIRONMENTS before concluding a repo has no secrets.**
  `actions/secrets` returning `total_count: 0` is not the whole picture;
  `actions/environments` and then `environments/<name>/secrets` is. I was one
  API call from writing a workflow that asked for credentials that already
  existed.
- **A protected environment is a better gate than a trigger condition.**
  "Only runs on tags" is a property of the workflow file, which anyone who can
  push a tag inherits. `required_reviewers` is a property of the credential, and
  it holds even if the trigger is wrong.
- **Sign before you package, not after.** Any packager that copies a payload
  into a container makes post-hoc signing produce something that passes a casual
  signature check and ships unsigned code inside.
- **Two Apple certificates, not one.** *Developer ID Application* signs bundles;
  *Developer ID Installer* signs `.pkg`. One variable named `APPLE_SIGNING_IDENTITY`
  reads like it covers both and does not.
- **`security find-identity -v -p codesigning` hides Installer certificates.**
  Signing a `.pkg` is a different policy from signing code, so the codesigning
  filter omits exactly the cert you are looking for. Use bare
  `security find-identity -v` before concluding a certificate is missing.
- **Identically-named certificates are a live hazard, not a tidiness issue.**
  Two valid certs sharing a common name make `codesign -s "<name>"` ambiguous.
  The SHA-1 hash is accepted wherever a name is, and cannot collide.
- **Look up every action version.** `@v0` was a plausible-looking guess for a
  Microsoft action and it does not exist. The registry answers in one call, and
  a wrong version is a failure that only appears at release time.
- **When behaviour cannot be tested, assert structure.** Ten parsed-YAML
  assertions are not a substitute for running it, but they are the difference
  between "it looks right" and "the secrets are provably behind a gate".

**Next:** **Jeff pushes the branch**, then adds a **Developer ID Installer**
certificate plus an `APPLE_INSTALLER_SIGNING_IDENTITY` variable to the `release`
environment — until then the mac leg fails by design. **R6** wants a real tag;
nothing here has been executed. **R3a** is still the next mac item.

**Branch/PR:** `tide/mac/R5-release-workflow` — authored, unpushed.

---

## 2026-08-22 — macos — S30's two fixes, and a design that could not have worked

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** the two changes Jeff picked — stop cancelling macOS, and cache the
build. Both in `.github/workflows/build.yml`, so **Jeff pushes**.

### The design I tried first, and why it is impossible

The obvious fix is per-platform cancellation: keep cancelling the fast
platforms, exempt the slow one.

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}-${{ matrix.name }}
  cancel-in-progress: ${{ matrix.name != 'macos' }}
```

**Job-level `concurrency` may only use the `github`, `inputs` and `vars`
contexts. `matrix` is not among them.** I checked GitHub's workflow-syntax
reference rather than trusting my recollection, and it is explicit. That YAML
would have parsed, and then behaved in some way I had not designed.

So the change is the blunt one: `cancel-in-progress: false` for everything.
Which turns out to be defensible on its own numbers — cancelling was destroying
**25% of windows and linux runs too**, each costing 5–10 minutes to redo.

**And it does not pile up runs**, which was my worry: GitHub keeps at most **one
pending** run per group, a third arrival superseding the second. So a group holds
one running plus one queued, and the newest push always gets its turn. The cost
is latency on a rapidly-pushed branch, not an unbounded queue.

### ccache, and the two settings that are load-bearing

`hash_dir=false`. Dependency sources are re-fetched each run into a path
containing the build directory, so absolute paths differ between runs and would
defeat every hash. Hashing content instead of paths is what makes a cross-run hit
possible **at all** — without it the cache would be installed, populated, and
never hit, which looks like "ccache does not help here".

`CMAKE_OBJCXX_COMPILER_LAUNCHER` alongside C and CXX. The mac build compiles
`.mm` sources; leaving OBJCXX unset would exempt exactly the Cocoa layer — the
mac-specific half of a mac-specific problem.

And `restore-keys` matters more than the exact key: TIDE's own sources change
every run, so an exact-key hit is rare and the normal case is a **prefix** hit
that is warm for the dependencies.

### I still have not measured the payoff, and said so in the workflow

The 56% figure is a share of compile **count**, not wall-clock, and dependency
objects are not necessarily the slow ones. Rather than quote it as a saving, the
workflow prints `ccache -s` on **every** run including failed ones — a cache that
is not being hit is the thing worth seeing, and a red run is when you most want
to know. The next few runs measure the answer.

**Learned:**

- **`matrix` is not available to job-level `concurrency`** — only `github`,
  `inputs`, `vars`. Per-platform cancellation inside a matrix is not expressible;
  it needs separate jobs or a blunt workflow-level setting.
- **Check the context list before writing an expression that reads naturally.**
  `${{ matrix.name != 'macos' }}` looks obviously fine and is not, and the
  failure would have been silent rather than a parse error.
- **A compiler cache that hashes absolute paths never hits across CI runs.**
  `hash_dir=false` is the difference between a working cache and one that is
  populated and useless — and the useless version looks like evidence that
  caching does not help.
- **Print the cache statistics unconditionally.** `if: always()` on `ccache -s`
  means a failed build still tells you whether the cache is working, which is
  exactly when the question comes up.
- **When you cannot measure the benefit, ship the measurement.** Saying "56% of
  compiles" as though it were "56% faster" would have been a guess wearing a
  number's clothes.

**Next:** **Jeff pushes the branch.** Then the first two or three runs answer
what ccache is actually worth, and **the concurrency choice should be revisited**
— if ccache brings macOS near 5–10 minutes, cancelling becomes cheap again and
the latency cost of not cancelling stops being worth paying.

**Branch/PR:** `tide/mac/S30-concurrency-and-ccache` — authored, unpushed.

---

---

## 2026-08-22 — macos — S30 re-measured: the queue is gone, the build is the problem

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** Jeff asked what he needs to do for S30. Before answering I re-measured
it, because the row's numbers are from 2026-08-21 and a great deal has merged
since. **The row's diagnosis no longer holds.**

### What the row says, and what is true today

S30 says: starvation, caused by a **1h+ queue** for hosted macOS runners meeting
`cancel-in-progress`. Measured over the last 25 build runs, per job
`created_at → started_at → completed_at`:

| | queue median | build median |
|---|---|---|
| macos | **0.1 min** (max 41.6) | **59.5 min** |
| linux | 0.0 min | 5.2 min |
| windows | 0.0 min | 10.0 min |

**The queue is gone.** What remains is a macOS *build* that takes an hour.

And it is not doing more work. The same run compiled **495** objects on macOS
and **496** on Windows — so macOS is roughly **5× slower per unit of work**,
not busier. A step breakdown puts 54.3 of those minutes in `Build` and 0.8 in
`Configure`, so it is compilation, not setup.

That is what feeds the cancellations: a 60-minute job offers a 60-minute window
for a newer push to kill it. macOS is cancelled **17 of 28** jobs (61%) against
**14 of 56** (25%) for windows and linux.

### The good news the row does not have

Completion is **32%** (9/28), not the **5%** recorded yesterday — most likely
the arm64-only change plus S29 removing duplicate runs. Still 2.4× worse than
the other platforms, but the row understates the current state by a lot.

### Why this changes the answer

The row's options were chosen against a queue, so they are now mis-weighted:

- **self-hosted runner** — still works, and is now the *definitive* fix rather
  than merely a queue bypass; the same build takes minutes on Jeff's own Mac
- **drop macOS from the per-branch matrix** — still hides it rather than fixing it
- **cache the build** — *new, and cheapest*. 56% of the macOS compiles come from
  fetched dependencies that change rarely, so `ccache` attacks most of the hour
  for a few lines of YAML
- **stop cancelling macOS specifically** — *also new*. The completion rate is a
  `concurrency` policy choice **independent of duration**, and separating the two
  levers was not visible while the diagnosis was "the queue"

**Learned:**

- **Re-measure a row before recommending against it, especially a performance
  one.** S30's mechanism was correct when filed and wrong a day later. Answering
  "what should I do" from the row's own numbers would have sent Jeff after a
  queue that no longer exists.
- **A cancellation rate is a symptom of DURATION, not only of policy.** The
  longer a job runs the wider its window to be cancelled, so "why is it always
  cancelled" and "why does it take an hour" can be the same question — and the
  fix for one may be the fix for both.
- **Compare work done, not just time taken.** macOS at 54 min next to Windows at
  10 could have meant macOS builds more. Counting compiles — 495 vs 496 — turns
  "slower" into "5× slower at the same job", which is a different problem with
  different fixes.
- **A stale diagnosis is worse than no diagnosis**, because it stops the next
  person measuring. This row had a mechanism, numbers and options, all internally
  consistent and all describing yesterday.

**Next:** the decision is **Jeff's** — self-hosted runner (definitive, most
setup), `ccache` in `build.yml` (cheapest, needs his push, unmeasured payoff), or
a `concurrency` change that stops cancelling macOS (fixes the completion rate
without touching the hour). **NOT MEASURED:** how much `ccache` would actually
remove — 56% is a share of compile *count*, not wall-clock, and that is one
cached run away from being known.

**Branch/PR:** `tide/mac/S30-remeasure` — TideSynth, backlog and journal only.

---

## 2026-08-22 — macos — AU is on, and four rows closed on one build

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** added `AU` to `FORMATS_LIST` — one word, and the last step of a chain
that took four separate fixes across three repos. TIDE now builds **five**
formats from plain `main`:

```
TIDE-Rack.gmpi  .vst3  .clap  .component  .app     rc=0, 0 errors
```

That closes **M1**, **M3**, **R8** and **R9**, and unblocks **R3a**.

### One build, three rows verified at once

The AU is the only artifact where all three identity decisions are visible
together, so enabling it tested all of them in a single measurement:

```
CFBundleExecutable  TIDE-Rack   == binary TIDE-Rack     (M3 — GMPI#8)
CFBundleIdentifier  com.tidesynth.tiderack.au           (R8 — GMPI#9)
manufacturer/subtype  Dsyh / Drck                       (R9)
auval               AU VALIDATION SUCCEEDED, rc=0
```

`Drck` is the code R9 *predicted* before the rename went in. Seeing it arrive on
a real artifact, validated by Apple's own tool, is what turns that prediction
into a fact.

**And this time nothing was hand-patched.** Both earlier AU measurements edited
the installed bundle to get there; this is the build's own output.

### Regression check, because "it builds" is not "nothing broke"

Adding a format to a shared list is exactly the kind of change that can disturb
its neighbours. So the other two were re-measured rather than assumed:

```
v1-rack.rpp        -6.3 dBFS / -17.0 rms   (documented figure)
v3-midi-pitch.rpp  -6.2 dBFS / -21.1 rms   (documented figure)
CLAP host probe    PASSED
```

### The four blockers, recorded where the switch is

`FORMATS_LIST` now carries a comment naming all four, because "AU is just one
word" is true today and was wildly false a day ago:

1. a header shadowing caused by **TIDE's own** root `include_directories()`
   putting SynthEditLib's `modules/shared` ahead of GMPI_Wrappers' for every
   target — including `plist_util`, which is AU-only (S17's class)
2. AudioUnitSDK needing C++23, where the exception must sit **inside** the
   format loop or a later `set_property` silently overwrites it
3. two strong "here to satisfy linker" fallbacks colliding at link time
4. `plist_util` **deriving** `CFBundleExecutable` — issue #271's class in a
   third site, and the one that made a successful build produce a component
   macOS silently refused

### A warning I did not bury

`auval` passes with one warning: *"Can Initialize Unit to un-supported num
channels: InputChan:0, OutputChan:1"* — the unit accepted a mono-out
configuration it does not advertise. **Filed as S39** rather than left in M3's
tail, because a closed row is where a live finding goes to be invisible (A32,
and I have made that exact mistake before with `setBlob` on E6).

I have not measured whether it matters. The row says so.

**Learned:**

- **Enable the thing that exercises the most decisions at once.** The AU was the
  only artifact carrying M3's, R8's and R9's changes simultaneously, so one
  `auval` run verified three rows. Choosing *which* verification to run is worth
  as much as running it.
- **A prediction confirmed on a real artifact is worth more than the same value
  read off a build.** `Drck` was predicted from a reimplementation of
  `to4charId` before the rename; Apple's validator agreeing is independent
  confirmation of the whole chain.
- **"It builds" is not "nothing broke".** Adding a format to a shared list needs
  the other formats re-measured, not assumed — two renders and a probe.
- **Put the history at the switch.** `set(FORMATS_LIST ... AU ...)` now carries
  all four blockers in a comment, because the one-word diff makes it look like it
  always could have been one word.
- **File the leftover warning as a row, immediately.** M3 closing is exactly when
  its unexamined warning would have stopped being visible.

**Next:** **R3a** — the AU half of the macOS `.pkg`; `scripts/package-macos.sh`
stages `TIDE-Rack.component` into `/Library/Audio/Plug-Ins/Components` beside the
payloads it already carries. The artifact exists, registers and validates.
**AUv3 (`AU3`) is still unbuilt** — M1 named it and only the AU2 path shipped;
worth its own row before anyone claims AU support is complete. **Still Jeff's:**
**R5** (a `.github/workflows/**` file) and **S30** (macOS CI).

**Branch/PR:** `tide/mac/M1-enable-au` — TideSynth.

---

## 2026-08-22 — macos — R8: every bundle now has an identifier TIDE owns, and codesign stops inventing one

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** R8, the other half of R9's decision. Scheme follows the ruling that TIDE
owns its identity: reverse-DNS on `tidesynth.com`, per format.

```
TIDE-Rack.gmpi        com.tidesynth.tiderack.gmpi
TIDE-Rack.vst3        com.tidesynth.tiderack.vst3
TIDE-Rack.clap        com.tidesynth.tiderack.clap
TIDE-Rack.component   com.tidesynth.tiderack.au
TIDE-Rack.app         com.tidesynth.tiderack
```

Per format on purpose: all of them can be installed side by side, macOS treats
the identifier as the unit of identity, and sharing one across them is the same
class of collision R9 removed from the plug-in id. The standalone takes the bare
form because it is the application, not one of the plug-in formats.

### The proof is one line

```
before:  Identifier=TIDE-Rack-a330dda6894e6d221c12682d5774c181b4845f8b
after:   Identifier=com.tidesynth.tiderack.vst3
```

`codesign` was inventing an identifier from the executable name plus a hash. It
**succeeded** either way, which is exactly why this sat unnoticed — but that
string is what Gatekeeper, notarization tickets and any future update logic key
on.

### One symptom, two causes, two fixes

**The plug-in formats had the key present and EMPTY.** CMake's stock
`MacOSXBundleInfo.plist.in` substitutes `MACOSX_BUNDLE_GUI_IDENTIFIER`, and
nothing set it for a plug-in target — GMPI sets it for STANDALONE only
(`gmpi_plugin.cmake:775`). So TIDE sets it per target in its own file, beside
the `OUTPUT_NAME` it already sets. No upstream change needed.

**The AU had the key ABSENT.** Its plist comes from `plist_util`, not CMake, and
the emitting block was `#if 0`'d out over a *"bundle ID not matching"* warning.
[GMPI#9](https://github.com/JeffMcClintock/GMPI/pull/9) passes
`--bundle-id "$<TARGET_PROPERTY:...,MACOSX_BUNDLE_GUI_IDENTIFIER>"`, so the
plug-in declares it once and both plist paths pick it up.

Reading "present and empty" as the same problem as "absent" would have sent me
to one fix for two mechanisms — the plug-in formats never needed `plist_util`,
and the AU could not be fixed from TIDE's file at all.

### The claim I had to test rather than assert

"The GMPI change is a no-op for plugins that set nothing" is the kind of sentence
that sounds obviously true and hides an empty-argument bug. So:

```
plist_util … --bundle-id ""              -> 0 CFBundleIdentifier keys
plist_util … --bundle-id com.example.x   -> 1 CFBundleIdentifier key
```

Empty passes through the generator expression, `plist_util` omits the key, and
an existing caller's plist is unchanged.

### S17's guard earned its keep, twice

Configuring with `-DGMPI_SDK_FOLDER_OVERRIDE=` into a tree that already had a
fetched `gmpi` failed with *"'gmpi' has a local override AND a fetched copy; the
build would silently compile one of them while the log names the other."*

Exactly right, and exactly the failure it was written for. It cost me two
configures to learn that an override needs a **fresh build directory** — the
cache remembers, so a tree that has ever been configured one way poisons the
other. Worth knowing before blaming the override.

**Learned:**

- **"Present and empty" and "absent" are different bugs.** They present
  identically — `PlistBuddy -c Print` returns nothing for both — and they have
  different causes and different fixes. Check which one you have before choosing
  where to fix it.
- **`codesign` succeeding is not evidence of a correct identity.** It invents an
  identifier from a hash when the plist has none, so signing works and the
  artifact is still wrong in the field that matters downstream.
- **An override needs a fresh build directory.** `FETCHCONTENT_SOURCE_DIR_*` and
  `*_FOLDER_OVERRIDE` are cached, so a tree configured without one keeps the
  fetched copy and S17 refuses — correctly. Do not debug the override; delete
  the tree.
- **Test the no-op claim on a pass-through argument.** An empty string surviving
  a generator expression into a CLI parser is exactly where a "harmless default"
  quietly becomes a usage error.
- **Keep the enabling change separate from the identity change.** `AU` stays out
  of `FORMATS_LIST` here so a bisect can tell "we turned on AU" from "we set
  identifiers" — even though the AU path is now proven twice over.

**Next:** **`com.tidesynth.tiderack.au` is latent** until `AU` joins
`FORMATS_LIST`, which is M1's last step and now fully proven — the AU registers
and `auval` passes with the new identifier and R9's new `Drck` subtype.
**Notarization is still unverified**: this removes the synthesised identifier
**R5** would have met, but no submission has been made.

**Branch/PR:** `tide/mac/R8-bundle-identifiers` — TideSynth, plus
[GMPI#9](https://github.com/JeffMcClintock/GMPI/pull/9) for the AU half.

---

## 2026-08-22 — macos — R9: TIDE owns its identity, and the id was a fossil of the old product

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · interactive, Jeff directing

**Did:** Jeff ruled R9. The id was `SE SynthEdit` because TIDE was originally
going to be *"SynthEdit in the DAW"*; the product pivoted to TIDE Rack and the
id never followed. **None of it has ever shipped**, so backward compatibility —
the entire basis of the `DO NOT RENAME` comment — does not apply.

`id="SE SynthEdit"` → `id="TIDE Synth: TIDE Rack"`. One string.

### The comment was right, and its premise was gone

`SynthEdit.cpp` carried an explicit **DO NOT RENAME**, reasoning that the VST3
GUID is hashed from the string so *"every host project that already loaded TIDE
fails to find it on reload"*. That is exactly correct. It just assumed such
projects exist.

Worth recording that it never mentioned **uniqueness** — that `SE SynthEdit` is
SynthEdit's own shell id (`SynthEdit/synthedit_as_sem.synthedit:56`), so TIDE's
identity was never TIDE's to keep. The comment reasoned about migration and
visibility; the gap was ownership.

### The colon was the missing half

`plist_util` splits `Vendor: Product` to derive the AU's four-character codes.
Probing the installed CLAPs showed the convention plainly:

```
FreqAnalyser_CLAP   id="GMPI: Freq Analyser"
SawDemo_CLAP        id="GMPI: GmpiSawDemo"
TIDE-Rack           id="SE SynthEdit"          <- no colon, so no vendor half
```

That is *why* the AU subtype was `Syhd` — SynthEdit's, not TIDE's. The rename
fixes the AU codes as a side effect of getting the shape right.

### Predicted first, then measured

The temptation is to change the string and read off whatever appears. Instead I
reimplemented djb2 and `to4charId` in Python and **checked the model against the
OLD values** — it reproduced the shipped subtype `Syhd` and the GUID tail
`1951ED43` exactly. Only then did I trust its predictions:

| | before | predicted | built |
|---|---|---|---|
| CLAP / GMPI id | `SE SynthEdit` | `TIDE Synth: TIDE Rack` | ✓ |
| AU manufacturer | `Dsyh` | `Dsyh` | ✓ |
| AU subtype | `Syhd` | `Drck` | ✓ |
| VST3 GUID tail | `1951ED43` | `A2A07287` | ✓ (via fixtures) |

One thing the model got **wrong**, and I did not paper over it: I also predicted
REAPER's leading decimal (`1386065673`) from `getVst2Id64`, and got
`1734972482`. Mismatch — so that number is not what I assumed. Rather than guess,
I changed only the GUID and let the render decide. It works, so REAPER keys on
the GUID and that decimal is something else. **Still unexplained, and labelled as
such** rather than quietly dropped.

### The negative control is the proof

Rendering the *un-updated* `v1-rack.rpp` against the renamed plugin:

```
peak= -inf dBFS  rms= -inf dBFS  -> SILENCE
```

REAPER cannot find it. That is the identity change working. Then all five
fixtures re-pointed and re-rendered, each matching its documented figure —
including `v1-rack-uncabled`, whose silence is its job.

**That control has a cost I did not think about**: it pops a modal "VST not
found" in REAPER, on the machine Jeff is sitting at. It did.

### A live tool was carrying the old GUID

`scripts/measure-chunk-robustness.py` (E10's repro harness) embeds the project
XML including the GUID. It would have gone silently stale — measuring a plugin
REAPER could no longer find, and reporting that as robustness. Updated, re-run,
PASS.

Running it also **crashes REAPER once, by design** — the `skeleton` case is E10's
KNOWN LIMIT and the engine fix is GATED. Jeff saw the crash and reasonably asked.
The tool now says so in its header, in capitals, so nobody else has to wonder.

**Learned:**

- **Validate a derivation model against the CURRENT value before trusting its
  prediction.** Reproducing `Syhd` and `1951ED43` is what made `Drck` and
  `A2A07287` claims rather than hopes — and it is what exposed that my VST2-id
  model was wrong while the GUID model was right.
- **When part of a model fails, say so and route around it.** The decimal did not
  match; changing only the GUID and letting the render decide beat guessing, and
  the unexplained part is now written down instead of forgotten.
- **A "DO NOT RENAME" comment is an argument, not a law.** Read what it assumes.
  This one assumed shipped projects; there were none. It was still worth having —
  it named the exact mechanism, which is why the change took an hour instead of a
  day.
- **Grep for the identifier in TOOLS, not just in source.** The old GUID was
  embedded in a Python harness that would have kept "passing" against a plugin
  that no longer existed.
- **A negative control that makes a GUI app prompt is not free when someone is at
  the keyboard.** Worth saying out loud before running it, not after.
- **If a tool crashes something on purpose, put that in its header in capitals.**
  Expected-crash-as-designed is indistinguishable from a regression to whoever
  happens to be watching.

**Next:** **R8** is the other half of this decision — bundle identifiers, sharing
the naming scheme this ruling just set. **AU is still not in `FORMATS_LIST`**;
M1/M3's PRs merged, so that is now one word plus a verification. **The window
closes at 1.0** — the GUID is a pure function of this string, and the comment now
says to treat it as frozen from the first release.

**Branch/PR:** `tide/mac/R9-tide-owns-its-id` — TideSynth.

---

## 2026-08-22 — macos — M1 and M3 fixed properly, and the override my own notes warned about

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** finished what the previous item diagnosed. M1's link failure and M3's
registration failure both have fixes in review, and this time the verification
ran against a **plain build** rather than a hand-patched bundle.

- [GMPI_Wrappers#11](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/11) — `--exe-name` / `--bundle-id`, and two weak fallbacks
- [GMPI#8](https://github.com/JeffMcClintock/GMPI/pull/8) — pass `$<TARGET_FILE_BASE_NAME:...>` to `plist_util`

### The order is a hard constraint, not a preference

`plist_util` rejects unknown arguments with `usage()` and exit 2, and it runs in
a POST_BUILD step. So merging the GMPI half first **breaks every AU build**.
#11 first, then #8. Stated at the top of both PRs, because a reviewer taking
them in PR-number order would get it backwards.

### `#if 0` was hiding R8's AU half

The AU2 emitter had `CFBundleIdentifier` commented out with *"build warning
about bundle ID not matching"*. Someone silenced a warning and the cost was **no
identifier at all** — which is why `codesign` invents `TIDE-Rack-a330dda6…`.
`--bundle-id` is opt-in, so the default output is unchanged and nobody gets the
old warning back unless they ask for it.

### The override my own memory warned me about, and I still checked wrong

TIDE has "Using local …" overrides for the VST3 SDK, SynthEditLib, GMPI and
gmpi_ui — **four**, and none for GMPI_Wrappers, which is fetched by
`SynthEditSem` with plain `FetchContent_Declare`. So the override is
`-DFETCHCONTENT_SOURCE_DIR_GMPI_WRAPPERS=…`.

I set it, then checked whether it took by looking in
`build/_deps/gmpi_wrappers-src` — which **does not exist when the override
works**, because FetchContent uses the given directory instead. I read that
absence as "override ignored" and nearly went hunting for a bug that was not
there. The real check is the cache and the dependency report:

```
GMPI_Wrappers <- .../wt-gw [fetched]
```

**Small correction worth noting:** S17's provenance report labels a
`FETCHCONTENT_SOURCE_DIR_*` override as `[fetched]`. It is accurate about the
path, which is the load-bearing half, but the label is wrong.

### Backwards compatibility, measured rather than asserted

The tempting claim is "the flags are optional, so nothing changes". I ran the
tool three ways instead:

```
default        CFBundleExecutable = TIDE-Rack_AU    CFBundleIdentifier absent
--exe-name     CFBundleExecutable = TIDE-Rack       CFBundleIdentifier absent
--bundle-id    ...                                  CFBundleIdentifier present

diff(default, --exe-name)  ->  CFBundleExecutable, and nothing else
```

### And the end-to-end result

From a plain build with both branches in place and no hand-editing: rc=0, zero
errors, `CFBundleExecutable` matches the binary, `auval -a` lists
`aumu Syhd Dsyh - TIDE Synth:TIDE Rack`, and **`AU VALIDATION SUCCEEDED`**.

**Learned:**

- **FetchContent's source override makes `_deps/<name>-src` ABSENT, not
  populated.** Checking for the directory to confirm an override is backwards.
  Check `CMakeCache.txt` or the dependency report instead.
- **A two-PR fix can have an ORDER, not just a pairing.** When the consumer
  passes a flag the provider must already understand, the provider merges first
  or the build breaks. Say so at the top of both, since PR numbers imply the
  opposite order here.
- **`#if 0` around a correctness feature is a bug with a comment.** The AU's
  missing bundle identifier was a silenced warning, and the silence cost more
  than the warning did.
- **"The flags are optional so nothing changes" is a claim, and a cheap one to
  test.** Three runs and a diff turned it into evidence.
- **Verify from a plain build before claiming a fix.** The previous item proved
  the AU *could* validate by hand-patching the installed bundle; that is a
  different claim from "the build produces something that validates", and only
  the second one is worth anything to a user.

**Next:** **merge order is #11 then #8.** After both, the TIDE-side remainder is
one word — `AU` into `FORMATS_LIST` — and **R3a unblocks**. **AUv3 (`AU3`) is
still unbuilt**; M1 names it and only the AU2 path was exercised. This box's
NEXT cell now points at **S38** (the Objective-C class collisions, reproducible
on any mac with two GMPI plugins installed) since M1/M3 are waiting on other
repos.

**Branch/PR:** `tide/mac/M1-M3-in-review` — TideSynth bookkeeping only; the code
is in the two PRs above.

---

## 2026-08-22 — linux — S7: TIDE does write to the user's home, and does not spew skins — the guard is an accident

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** took **S7** and did the runtime verification it asks for before any fix.
The row's prediction is **half right**, and the half that is wrong is wrong in a
way that makes the finding more interesting rather than less.

### Confirmed: a launch writes outside the plugin's container

Launching `TIDE-Rack` creates, from launch alone and with no user action:

```
<home>/SynthEdit Projects/
<home>/SynthEdit Projects/skins/
<home>/SynthEdit Projects/.resource_version     (contents: "0")
```

That is **PLAN constraint 4** (self-contained — nothing written outside the
plugin's own container) and **constraint 8** (no skin folder, nothing
skin-related written to the user's disk), both violated at startup.

### Refuted: it does not spew the skin set

`skins/` is created and stays **empty**. The copy is guarded by
`if (exists(srcRoot))` with `srcRoot = GetHomeDir()/Resources/skins`
(`SkinMgr.cpp:76-80`), and **TIDE's bundle ships no `Resources/skins`** — its
`Resources/` holds `Prefabs/` and four pin XMLs, nothing else.

**So the payload is absent, not blocked, and that is the actual finding.**
`create_directories(destRoot)` runs unconditionally inside `shouldCopy`, and
`shouldCopy` is `versionChanged || !exists(destRoot/"default")`. For TIDE
`SE_APP_BUILD_NUMBER` is **0**, the stored version reads back **0**, and
`default/` is never created — so **`shouldCopy` is true on every launch,
forever**. The day someone adds a `skins/` to TIDE's resources, it starts
copying them into every user's home silently. Nothing guards this; TIDE simply
has nothing to copy yet.

### The part that nearly went wrong

`BundleInfo::getUserDocumentFolder()` resolves the home through
**`getpwuid(getuid())->pw_dir`, deliberately ignoring `$HOME`**
(`BundleInfo.cpp:343-351` — so a sandboxed macOS app sees the real home rather
than its container). **`HOME=<scratch>` therefore does not sandbox this test at
all**, and the obvious version of this experiment would have written into Jeff's
real home while looking careful.

Redirected instead with a 30-line `LD_PRELOAD` shim over `getpwuid`/`getpwuid_r`,
**validated against a probe before being trusted** (`pw_dir=/home/jef` without,
`pw_dir=<scratch>` with). Jeff's `~/SynthEdit Projects/` is byte-identical before
and after — 713 entries, `find -printf '%T@ %p'` diff clean. The shim is
committed as `tools/fakehome_shim.c` with its reasoning, because the next runtime
test of "what does this write" needs it too.

### Incidental, and worth separating from the above

`SynthEditCL` wrote `.resource_version` = **186** into the *real*
`~/SynthEdit Projects/` at 09:46 today — during this run's own **E1c** renders.
So the machinery is live for SynthEdit itself, not merely theoretical. It is
unrelated to the TIDE question and predates the S7 test; I noticed it only
because I snapshotted the folder first.

Also: the row cites `SE16/SynthEdit2/SkinMgr.cpp:27-30`. The carve-out moved that
file; it is `SynthEditLib/EditorLib/SkinMgr.cpp` now. Still GATED.

**Learned:**

1. **`HOME=` is not a sandbox when the code uses `getpwuid`.** Two libraries in
   this stack deliberately prefer it, for a good macOS reason. Check which one a
   path comes from *before* running a write test, not after.
2. **Validate a test harness against a probe before trusting its result.** A
   silently-not-working `LD_PRELOAD` would have produced "TIDE writes nothing" —
   a clean, wrong, reassuring answer.
3. **"It does not do the bad thing" and "it cannot do the bad thing" are
   different findings**, and only the second is a guard. Here the directory
   creation is unconditional and only the payload is missing.
4. **Snapshot the thing you are about to test before you test it.** The
   `.resource_version` = 186 write was my own harness from an earlier item, and
   without a baseline I would have attributed it to TIDE.

**Next:**

1. **The fix is GATED** (`SynthEditLib/EditorLib/SkinMgr.cpp`) and this is not a
   build break, so A17 does not reach it. **Try the TIDE-side route first**: TIDE
   never needs a user skin folder, so the goal is that `SkinMgr`'s constructor is
   never reached, or is pointed at the bundle.
2. **Accept is now stateable and cheap**: a TIDE launch creates nothing under the
   user's home, demonstrated with the committed shim.
3. **S2's sandbox audit overlaps** — whoever takes either should re-read the
   other, as the row already says.

**Machine left clean.** Everything ran under the shim in a scratch home; Jeff's
`~/SynthEdit Projects/` verified byte-identical. weston and the standalone both
stopped by pid (S31). TideSynth back on `main`.

**Branch/PR:** `tide/linux/S7-skin-writes` — TideSynth only, row + journal + the shim. No product code change.

---

---

---

## 2026-08-22 — macos — M1 and M3 were never blocked by the carve-out, and the AU passes auval

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** Jeff asked what mac-only work remains. The mac-boxed rows are `M1`,
`M2`, `M3`, `R8` and `R3a` — and three of them were **one-line `BLOCKED` rows
with no stated blocker**:

```
| M1 | BLOCKED | mac | AU + AUv3 targets building on macOS. |
| M3 | BLOCKED | mac | `auval` clean. |
```

Their only blocker is the section heading above them, **"After the carve-out"**,
and C7 has been DONE since 2026-08-21. That is the same shape as R2–R6, which
this fleet found unblocked three days after the fact.

So I built it. **`auval`: `AU VALIDATION SUCCEEDED`.**

### Three blockers, each hidden behind the one before it

**1 — header shadowing, and the offending path is ours.** `plist_util` is an
AU-only helper that compiles GMPI's `dynamic_linking.cpp`, which wants
`wrapper::JmUnicodeConversions`. Three headers are called
`unicode_conversion.h`:

| repo | namespace |
|---|---|
| GMPI_Wrappers `wrapper/common` | `wrapper::JmUnicodeConversions` ← wanted |
| SynthEditLib `modules/shared` | `JmUnicodeConversions` (global) ← found |
| gmpi_ui `helpers` | `gmpi::unicode` (unrelated) |

`plist_util` *does* ask for the right directory — as a **target-scoped**
`PRIVATE` dir, while our ROOT `CMakeLists.txt` adds SynthEditLib's with
**directory-scoped** `include_directories()`, which CMake places first. Measured
order on the failing compile line: SynthEditLib at **position 3**, GMPI_Wrappers
at **position 9**. This is S17's class, and it is TIDE's path leaking into a
target TIDE does not define.

**2 — AudioUnitSDK needs C++23.** `std::expected`. Verified rather than assumed:
a three-line program compiles at `-std=c++23` and fails at `-std=c++20`.

**The obvious fix silently does nothing**, and that cost me a build. Setting
`CXX_STANDARD 23` on the AU target earlier in `SynthEditSem/CMakeLists.txt`
prints its configure message, sets the property — and the compile still says
`-std=gnu++20`, because a later `set_property(... CXX_STANDARD 20)` inside the
format loop overwrites it. I only found that by reading the failing compile line
for `-std`, not by trusting the property. The exception now lives *inside* that
loop.

**3 — two duplicate symbols.** `AU2_Wrapper.cpp` defines
`initialise_synthedit_extra_modules(bool)` — its own comment says *"here to
satisfy linker"* — and `CreatePluginBundleRef()`. TIDE already has both from
EditorLib and SynthEditLib. Marking both `__attribute__((weak))` links it: the
fallback survives for plugins that need it, a real definition wins. Tested
against the fetched copy; `GMPI_Wrappers` is ALLOWED, so it is a normal PR.

Then it built. **And the component did not register.**

### The registration failure is issue #271's class, in a third place

`auval -a` did not list it. `auval -v` said *"Cannot get Component's Name
strings"* and *"didn't find the component"*. I signed it — no change. I added a
`CFBundleIdentifier` — no change.

A working control on the same machine settled it in one command:

```
Poly Synth2.component   CFBundleExecutable = SeAu           binary = SeAu        <- match
TIDE-Rack.component     CFBundleExecutable = TIDE-Rack_AU   binary = TIDE-Rack   <- MISMATCH
```

`plist_util.cpp:587` derives the name as `pluginPath.stem() + "_AU"` — correct
only while a plugin does **not** set `OUTPUT_NAME`. TIDE sets it. **That is
exactly issue #271**: one half of a pair follows `OUTPUT_NAME`, the other follows
the target name, and the platform silently declines the result.

Corrected by hand, re-signed: `aumu Syhd Dsyh - TIDE Synth:TIDE Rack`, then
`AU VALIDATION SUCCEEDED` — 19 passes, one warning. The `auval` log also shows
E9's shipped fix working in a real host: *"TIDE: unprepared - writing silence to
the host's output buffers"*.

### Two more identity leaks, and a new row

The AU plist has **no** `CFBundleIdentifier` key at all — not empty like the
other three formats, absent — so `codesign` again invents one (**R8**). And the
AU's `manufacturer`/`subtype` are `Dsyh`/`Syhd`, which are not TIDE's: **R9**'s
leak again, in the four-character-code namespace where uniqueness is what
registration is keyed on.

Registering it also printed **seven** Objective-C class collisions with two
unrelated GMPI plugins — `GMPI_VIEW_MAKER_VERSION_02` and friends, *"may cause
spurious casting failures and mysterious crashes"*. Filed as **S38**
(written as S37 when this entry was drafted; the linux box filed a different S37
the same day and landed first, so this row renumbered on merge). The `_02`
/ `_03` suffixes show someone already met this and versioned the names, which
fixes it between versions and never between two plugins.

### Why AU is NOT in FORMATS_LIST on this branch

Enabling it today ships a component **no host registers**. The two fixes land as
inert enablers, and the AU branch raises `FATAL_ERROR` rather than skipping —
N1a lost a session to a silent `if(TARGET ...)`.

**Learned:**

- **A `BLOCKED` row with no stated blocker is a claim nobody has retested.**
  Three of them here inherited it from a section heading whose subject closed a
  day earlier. Re-derive the blocker before believing it — this is the second
  time this week (R2–R6 was the first).
- **A CMake property can be set, announced, and overwritten one loop later.**
  `set_target_properties` is not a commitment. Verify against the compile line
  (`-std=`), not against the configure output.
- **A working control on the same machine beats any amount of reading.**
  Comparing TIDE's AU plist to a plugin that *does* register found the mismatch
  in one command, after signing and identifier theories had both failed.
- **`OUTPUT_NAME` breaks every hand-derived sibling name, not just the one you
  fixed.** Linux VST3 bundle (#271), `copy_plugin()`, and now the AU's
  `CFBundleExecutable`. The pattern is a *derived* name sitting next to a
  generator-expression name. Grep for the derivation, not for the symptom.
- **Fix the first error and expect the count to go UP.** One error became 20
  became 1 became a link failure became a registration failure. Fail-fast means
  the first message is a position report, not a scope estimate.
- **Objective-C class names are process-global.** Any GMPI plugin sharing them
  collides with any other in the same host, and versioned suffixes do not help
  between two plugins of the same version.

**Next:** **M1 needs the `GMPI_Wrappers` weak-symbol PR** (ALLOWED, small).
**M3 needs `plist_util` to be told the executable name rather than deriving it**
— an argument in `plist_util.cpp` (ALLOWED) plus its invocation in
`gmpi_plugin.cmake` (**PR-GATED**), which must land together, same as
GMPI#6/#274. Then AU can go into `FORMATS_LIST` and **R3a unblocks**. **AUv3
(`AU3`) is still unbuilt** — M1 names it and only `AU` was tested.

**Branch/PR:** `tide/mac/M1-au-targets` — TideSynth. **Based on
`tide/mac/E9-clap-host-verify` (#283)** for the R9 reference.

---

---

## 2026-08-22 — macos — loading the CLAP for the first time found that TIDE ships SynthEdit's identity

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **E9** for its CLAP question, wrote a real CLAP host to answer it,
and the handshake printed something far more important than the answer.

### The row's caveat was stale within hours

E9 says AU and CLAP are *"unmeasured, deliberately not claimed, because TIDE
builds neither (`SynthEditSem/CMakeLists.txt:59` is `GMPI VST3 STANDALONE`)"*.
**R4a merged earlier today and made that false** — the list is
`GMPI VST3 CLAP STANDALONE`, and it is not at that line any more either.

And R4a's own verification was thin, by my own admission on its row: `nm` found
one exported symbol. **An entry point is not a plugin.** Nothing had ever loaded
the thing.

### Why a probe instead of REAPER

A DAW cannot settle this cheaply. REAPER references plugins in `.rpp` by a
host-specific id, and forcing a render rate needs its GUI — E9 itself records
Jeff hitting that dialog. So `tests/e9_clap_rate_probe.c` drives the CLAP C ABI
directly: `dlopen` → `clap_entry` → factory → descriptor → `create_plugin` →
`init`, then

```
activate(48000) → deactivate → activate(44100) → deactivate → activate(48000)
```

which is exactly the bracket a DAW uses for a rate change, and a **stricter**
exercise of it than REAPER gave. 21 checks, all passing.

That confirms E9's mechanism transfers: `Processor_CLAP::activate()` calls
`plugin.start_processor(...)` with the new rate, and `processor_holder.cpp` `:55`
releases the old processor, `:69` creates a fresh one, `:82` calls `open()`,
`:215` re-seeds the blob. CLAP absorbs a rate change by instance replacement,
exactly as VST3 does.

### And then the descriptor printed this

```
id="SE SynthEdit"  name="TIDE Rack"  vendor="TIDE Synth"
```

`name` and `vendor` were rebranded. **`id` — the only field that functions as
identity — was not.** `SynthEditSem/SynthEdit.cpp` declares it, and the
commented-out original two lines below still reads
`<Plugin id="SE SynthEdit" name="SynthEdit" ...>`, which is where it came from.

That is not cosmetic, and it does not stop at CLAP.
`GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:200` builds the **VST3 class
GUID** from the ASCII `"PluginGMPI "`, a `P`/`C` role byte, and
**`hashString(id)`** — a djb2 hash of that string and nothing else.

So I computed it against the artifact rather than trusting the reading:

```
hashString("SE SynthEdit") = 0x43ED5119   little-endian -> 1951ED43

committed fixture tests/hosts/v1-rack.rpp:
  1386065673{506C7567696E474D504920 50 1951ED43}
             "PluginGMPI "          "P"  ^^^^^^^^ exact match
```

**TIDE Rack's shipped VST3 GUID is a hash of SynthEdit's generic shell id.** The
CLAP id is that string verbatim (`Factory_CLAP.cpp:25`), and GMPI uses it
directly. Any other GMPI plugin declaring `SE SynthEdit` gets the same GUID and
the same CLAP id; a host with both installed cannot tell them apart, and a saved
project can reload the wrong one. Filed as **R9**.

**The timing is the point.** The GUID is a pure function of the id, so fixing the
id *changes the GUID*, and every project saved with TIDE Rack stops finding the
plugin. That cost is near zero today and permanent after 1.0 — and R2, R3 and R4
are building installers for these exact artifacts right now.

**Learned:**

- **Load the artifact in a real host before believing it works.** R4a's evidence
  was `nm` finding `clap_entry`, which I wrote up as verified. One handshake
  found an identity bug that no amount of building would have shown.
- **A stale caveat is most dangerous when it is your own and hours old.** E9 said
  CLAP was unmeasurable *because TIDE builds no CLAP*. I merged the change that
  made it buildable earlier in the same session and did not go back.
- **Write the host when the DAW is the expensive part.** ~170 lines of C against
  the CLAP C ABI drove the rate-change bracket more precisely than REAPER could,
  headlessly, with no GUI and no project file.
- **When a rename touches `name` and `vendor`, check `id`.** Display fields are
  what a person sees and notices; the identifier is what the software uses and
  nobody looks at. This one survived the whole N1a rename.
- **Follow an identifier to what is DERIVED from it.** The CLAP id looked like a
  CLAP-only problem until `textIdtoUuid` turned out to hash it into the VST3
  GUID. Grep for what consumes an identity string before scoping the blast
  radius.
- **Verify a hash claim by computing it.** Reading "the GUID is a hash of the id"
  is an argument; matching `1951ED43` in a committed fixture is proof, and it
  took four lines of Python.

**Next:** **R9 is Jeff's** and wants deciding before the first release rather than
after — it pairs with **R8** (the empty `CFBundleIdentifier`), since both need one
naming scheme. **E9 is left TODO deliberately**: its Accept is measured for VST3
and now CLAP, but AU is genuinely unmeasured and belongs to **R3a**, which is
`BLOCKED(M1)`.

**Branch/PR:** `tide/mac/E9-clap-host-verify` — TideSynth.

---

---

## 2026-08-22 — linux — E1c: the deciding case, and the control that makes it decide anything

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** resolved R4's conflict first (STEP 1.5), then took **E1c** and built the
one experiment macOS had narrowed it to. The Linux half is done; one macOS or
Windows render finishes it.

### The case

`tests/cases/osc_naive_pitched.json` — the **same** `SE Oscillator (naive)` as
`osc_naive_sine`, same duration, rate and source pin, differing only in that
`Pitch` is driven from a patch point pinned to 5 V rather than free-running on
the module default.

### The control nobody had run, and it is the whole reason this experiment works

**Both cases render at exactly 440.0 Hz** — zero-crossing count over the 2.0 s
render, 95,999 frames at 48 kHz, identical for both. Pinning 5 V happens to
reproduce the module's own default note.

That is not a detail, it is the experiment's validity. Had the two rendered at
different frequencies, any cross-platform residual would have been confounded by
a different phase increment, and a `-123 dBFS` result would have proved nothing
about the *driven-ness* of the input. I checked it because "same module, one pin
changed" is only single-variable if the pin change does not move the note, and
nothing in the row said whether it did.

### Also confirmed while the harness was up

`osc_naive_sine` renders **bit-identical** to its stored reference here —
`null=-inf dBFS`. Its provenance record says `recorded: reconstructed`, inferred
by macOS from journal archaeology; this corroborates the same conclusion by
measurement. I did **not** rewrite that file — regenerating it would upgrade the
label but discard the `evidence` field explaining how it was reconstructed, and
it is another box's work.

The new reference is seeded with first-hand provenance: `recorded: measured`,
`SynthEditCL V1.6.186`, `x86_64`, sha256 `7ade35f2…`.

### What is left, and why not here

The residual is a **macOS-vs-Linux** quantity, so one platform cannot produce it.
A second box renders `osc_naive_pitched` against this reference, and the outcome
is binary and pre-committed in the row so it cannot be rationalised after the
fact: **-123 dBFS** means the discriminator is the undriven pitch input (and
`prefab_oscillator`/`prefab_filter` carry gates for a mechanism they do not
exhibit); **-73 dBFS** means the module is the variable and `osc_naive_sine`'s
stated reason stands.

The new case ships with **provisional** drift-class gates copied from
`osc_naive_sine`, and its `tolerance_reason` says so — gating it as if the answer
were known would beg the question it exists to settle.

**Learned:**

1. **A "single-variable" experiment is a claim, and it is cheap to check.**
   Pinning the pitch input could easily have changed the note; if it had, the
   whole comparison would have been worthless and would have *looked* fine.
   One zero-crossing count per render settled it.
2. **The audio harness runs on Linux** — `tools/render_harness.py --cli
   <SynthEditCL> --modules <folder>`, using the existing
   `~/SE/build/SynthEditCL/SynthEditCL`. Nothing in the rows said so, and it
   needs no REAPER.
3. **The harness warns when the engine scanned module folders outside
   `--modules`** (`~/.local/share/SynthEdit/modules` on this box), and says the
   run therefore does not prove which module set rendered. True here; it is a
   developer box. Worth reading rather than skipping on a result that matters.
4. **Do not regenerate another box's provenance record to improve its label.**
   `--update-refs` would have stamped `measured` over `reconstructed` and thrown
   away the reasoning that made the reconstruction credible.

**Next:**

1. **One macOS or Windows render** of `osc_naive_pitched` against this reference
   closes E1c. Everything needed is in the row and the case's `tolerance_reason`.
2. If it lands in the rounding class, **`prefab_oscillator` and `prefab_filter`
   want their gates revisited** — they are currently drift-class for a mechanism
   their scripts would not exhibit.

**Machine left clean.** Renders went to a scratch `--out`; the only tracked
additions are the new case and its reference. TideSynth back on `main`.

**Branch/PR:** `tide/linux/E1c-pitch-pinned` — TideSynth only.

---

## 2026-08-22 — linux — R4: the tarball, and the CLAP's resources have nowhere to live

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** synced all five repos, confirmed **#271 is genuinely fixed on `main`**,
then took **R4** — the Linux tarball — and filed the one thing it cannot solve.

### #271 closed, checked from `main` rather than from my own branches

Both halves merged (GMPI#6 + #274), and the split-brain risk was the whole point
of that pairing, so I re-checked from a **clean configure and build of `main`**.
TIDE tracks GMPI by `GIT_TAG origin/main`, so a fresh configure fetches the fix:

```
TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so
TIDE-Rack.vst3/Contents/Resources/…            (no stray TIDE_Rack_VST3.vst3)
[Info]: Found Plugin: TIDE Rack   uid=506C7567696E474D504920501951ED43
```

One bundle, and Ardour loads it. Closed.

### R4: the tarball

`scripts/package-linux.sh` → `TIDE-Rack-Linux.tar.gz`, 5.7 MB, 32 entries, no
spaces and no underscored shipped names (docs/distribution.md's rule, which
exists because a space is `%20` in a permalink R6 promises never to change).

**Verified by installing it, not by listing it:** untar into a scratch `HOME`,
run `install.sh`, and Ardour's scanner finds the *installed* plugin at
`~/.vst3/TIDE-Rack.vst3` with all six prefabs present. `install.sh` writes
nothing outside `HOME`, needs no root, honours `VST3_DIR`/`CLAP_DIR` and is
re-runnable.

### The finding: a Linux CLAP has nowhere to put its data

A Linux CLAP is a **bare shared object**, not a bundle directory — `gmpi_plugin.cmake`
says so in as many words. So it carries no resources, and
`BundleInfo::getBundleContentsFolder()` walks the module path for a `Contents`
element and **falls back to `parent_path()`**. The lookup therefore lands beside
the `.so`: **`~/.clap/Resources`, shared with every other CLAP installed the same
way.**

Confirmed it actually needs them rather than assuming: `TIDE-Rack.clap` contains
the same `no Prefabs folder in bundle resources` and `%s missing from bundle
resources` strings the VST3 does, and ships none itself — 7.6 MB against the
VST3 bundle's 8.7 MB, the difference being the 160 KB `Resources`.

**R4 ships it anyway**, because the alternative is an empty rack module browser —
the S21 failure — and the `README.txt` documents the folder and offers a
VST3-only install. The design problem is **S37**, with three options costed.

**And the CLAP is packaged but never loaded**, because there is no CLAP host on
this box: `clap-validator` and `clap-info` absent, and Ardour 8.4 has **no** CLAP
support (`strings libardour.so.3` finds no `clap_entry`). All three checked, not
assumed — which is the habit yesterday's Ardour correction was supposed to teach
me.

**Learned:**

1. **Verify a two-repo fix from the shared branch, not from the branch that made
   it.** Both halves merging is not the same as both halves reaching a consumer;
   TIDE fetches GMPI by moving tag, so only a clean configure proves it.
2. **"Where does this format keep its data?" is a packaging question with a
   different answer per format.** The VST3 is self-contained and the CLAP is not,
   on the same platform, in the same build — and only the CLAP leaks into a
   shared directory.
3. **A bare `.so` plugin format has no namespace**, so any resource convention
   built on `parent_path()` is shared-by-construction. Worth knowing before
   choosing that convention for a fourth format.
4. **Check for a validator before promising verification.** I could verify the
   VST3 half completely and the CLAP half not at all, and the honest package is
   one that says which is which.

**Next:**

1. **S37** wants a CLAP host on some box before it can be measured at all; that
   may be its real first step.
2. **R4's tarball is not uploaded anywhere** — R6 owns the release plumbing, and
   this row only produces the artifact.
3. **The audio half of v0.1 still has not been run against the renamed
   artifacts** — `render-and-measure.py` is REAPER-specific.

**Machine left clean.** All builds and installs ran in scratch trees and a fake
`HOME`; `~/.vst3`, `~/.clap` and the developer's build tree were not written to.
Ardour cache entries from the scans pointed into scratch trees and were removed,
leaving his nine own entries. All five repos synced and on their default branches.

**Branch/PR:** `tide/linux/R4-linux-tarball` — TideSynth only.
## 2026-08-22 — macos — STEP 4: six PRs merged in one go, and every NEXT cell went stale at once

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** bookkeeping, after Jeff merged the whole queue —
[#272](https://github.com/JeffMcClintock/TideSynth/pull/272),
[#275](https://github.com/JeffMcClintock/TideSynth/pull/275),
[#276](https://github.com/JeffMcClintock/TideSynth/pull/276),
[#278](https://github.com/JeffMcClintock/TideSynth/pull/278),
[#279](https://github.com/JeffMcClintock/TideSynth/pull/279),
[#280](https://github.com/JeffMcClintock/TideSynth/pull/280), plus
[GMPI#6](https://github.com/JeffMcClintock/GMPI/pull/6) and
[#274](https://github.com/JeffMcClintock/TideSynth/pull/274) together. Flipped
**R2, R3, R4a, S31** to DONE with PR state verified rather than assumed, and
re-pointed **all four** NEXT cells.

### Every cell in the NEXT block was stale simultaneously

That is the thing worth recording. The block is per-platform, so a single run
normally invalidates one cell. Six items landing at once invalidated all four:

| cell | pointed at | why it was dead |
|---|---|---|
| `win` | R2 | merged an hour earlier |
| `mac` | N1a | DONE since 2026-08-22, two rows ago |
| `linux` | S23 | that box already measured it as not reproducing |
| `any` | C7b, C15 | both archived DONE |

**And `check-next-block.py` passed on every one of them.** It reports "2
take-target(s) checked across 4 NEXT rows" — its trigger set is deliberately
narrow (A20/A27), matching imperative take-phrases and a leading bolded ID. The
`any` cell names C7b and C15 as *history* rather than as "take this", which is
exactly the shape it is designed not to flag, and which is nonetheless useless to
the run that reads it.

So the check is doing its documented job and the block still rotted. A cell can
be *correct* and *worthless* at the same time.

### A mistake worth its own line

Rewriting the cells, I wrapped each in `**` — but they already began with a
bolded take-target, so the result was `****P11**`. That breaks A27's rule
directly: the check keys on the ID a cell *begins* with when bolded, and a
four-asterisk prefix is not that. It passed anyway, because the ID was live and
the regex still matched. Caught by looking at the rendered table rather than at
the exit code.

I also nearly left the **Why** column untouched while replacing every **Take**
cell, which would have paired new instructions with a year-old justification —
`win`'s Why still said *"Both default branches are GREEN, measured 2026-08-20"*.

### What the four cells say now

- **`win` → P11's Windows half.** Its own row, **P3**, is GATED — both files are
  on the GATED list. The mac half turned out to be a *different mechanism*, so
  the cell says explicitly not to assume the Windows one matches.
- **`mac` → E9.** `SynthEditSem/SynthEdit.cpp`, TIDE's own, spec already settled.
  R8 is mac's own row and is blocked on a naming decision from Jeff.
- **`linux` → R4**, whose blockers all cleared today, plus two side-tasks that
  **only Linux can do**: run `sh tests/s31_kill_named_test.sh` (S31's bug does not
  exist on BSD), and render one E1c case (`verify.yml` is ubuntu-only).
- **`any` → E9**, with the takeable set spelled out, because most of the 26-row
  `any` queue is GATED and no run should re-derive that.

**Learned:**

- **A batch merge invalidates the WHOLE NEXT block, not one cell.** The block is
  written as if one run changes one platform's cell. After a multi-PR merge,
  re-read all four — three of the four here named work that had finished.
- **`check-next-block.py` cannot see a cell that cites a dead row as history.**
  Its narrow trigger set is the right trade (A20 argues it), but it means green
  lint is not evidence the block is useful. Read the cells.
- **Do not wrap a NEXT cell in `**` — it already starts bolded.** `****ID**`
  breaks the leading-bolded-ID convention A27 depends on and still passes lint.
- **When you replace the Take column, replace the Why column.** New instructions
  under old reasoning is worse than either alone, because the reasoning is what a
  run uses to decide whether the instruction still applies.
- **Flip a row on verified PR state, not on memory of having pushed it.**
  `gh pr view --json state,mergedAt` for each of the four took one command and is
  the difference between bookkeeping and guessing.
- **Not every leftover deserves a row.** S31's outstanding evidence is one command
  on another platform; that belongs in the `linux` NEXT cell with "reopen S31 if
  it fails", not in a new ID.

**Next:** the queue is genuinely empty of mac-takeable work that does not need
Jeff. **E9** is the mac cell and the largest live thing this box can both change
and verify. **Waiting on Jeff:** **R5** (a `.github/workflows/**` file — this
token has `repo` scope only, verified), **R8** (a naming decision, then a
PR-GATED GMPI change), **S27**'s cross-platform reference decision, and **S30**,
where **no macOS job completed on any of the eight PRs merged today**.

**Branch/PR:** `tide/mac/step4-bookkeeping` — TideSynth, backlog and journal only.

---

## 2026-08-22 — windows — R2: the Windows installer, and the payload it must carry is not the file the build emits

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** took **R2**, the Windows installer. It is the first `win`-marked
takeable row this platform has had — the release track unblocked yesterday
([#270](https://github.com/JeffMcClintock/TideSynth/pull/270)) and no windows
box had run since. Shipped `installer/windows/TIDE-Rack.iss` +
`scripts/package-windows.ps1`, which produce **`TIDE-Rack-Windows.exe`
(3.0 MB)** and **`TIDE-Rack-Windows.zip` (1.4 MB)**, the two assets
[docs/distribution.md](docs/distribution.md) names.

### Two things had to be fixed before anything could be packaged

**1. `cmake -S . -B build` does not build on this box, and the error names the
wrong thing.** CMake picks the Visual Studio **BuildTools** instance, which has
no MFC, and the build dies:

```
syntheditlib-src\EditorLib\MfcDocPresenter.cpp(4,10): error C1083:
  Cannot open include file: 'afxres.h': No such file or directory
syntheditlib-src\EditorLib\CContainer.cpp(8,10): error C1083: ...
```

Those are the two files **P3** exists to de-MFC, so the failure reads as a known
problem in the code rather than an unknown one in the environment.
`18\Community` has `atlmfc`; `18\BuildTools` does not. Fixed with
`-DCMAKE_GENERATOR_INSTANCE="C:/Program Files/Microsoft Visual Studio/18/Community"`
— a flag `docs/building.md` records **only for the old SE16 path**, not for
C7d's root `CMakeLists.txt`, which is the one a stranger now uses. With it:
configure rc=0, build rc=0, **zero `error C` / `error LNK` lines**, producing
`TIDE-Rack.gmpi`, `TIDE-Rack.vst3` and `TIDE-Rack.exe`. Built with **no
`*_FOLDER_OVERRIDE`s**, the way CI does — all eight dependencies fetched fresh,
so nothing here is contaminated by the concurrent session's dirty trees.

Incidentally this is the first Windows confirmation of **N1a's PDB fix**: the
three PDBs are `TIDE_Rack.pdb` / `TIDE_Rack_VST3.pdb` /
`TIDE_Rack_STANDALONE.pdb`, distinct, and the parallel link that produced
`LNK1201` does not recur.

**2. The plug-in the build produces does not work.** That is **S36**, filed
below, and it is the more important of the two.

### The decision that is actually R2's: the payload is a VST3 BUNDLE

`gmpi_plugin.cmake`'s VST3 block gives Windows a bare `SUFFIX ".vst3"` — macOS
gets a real bundle, Linux gets one assembled by a POST_BUILD copy, Windows gets
neither. A bare DLL is a legal VST3 and hosts load it. But TIDE has data it
cannot work without — four pin XMLs and six rack prefabs — and **a bare DLL has
nowhere to keep it**. Its only sibling directory is
`C:\Program Files\Common Files\VST3`, shared with every other vendor's
plug-ins, where a folder called `Prefabs` and a file called `Converters.xml`
have no business being.

So the asset ships:

```
TIDE-Rack.vst3\Contents\x86_64-win\TIDE-Rack.vst3
TIDE-Rack.vst3\Contents\Resources\{ControlsXp,Converters,MidiPlayer2,VaFilters}.xml
TIDE-Rack.vst3\Contents\Resources\Prefabs\*.synthedit
```

and the runtime already reads exactly that: `BundleInfo.cpp:670-684` sets
`pluginIsBundle` from *"a path element with an extension, followed by
`Contents`"*, and `getResourceFolder()` (`:266-271`) then returns
`<bundle>\Contents\Resources\`.

### Verified in three conditions, and the third is the shipped asset itself

Every one is the **same binary**, differing only in where it sits:

| # | condition | what the plug-in printed |
|---|---|---|
| **a** | flat DLL, exactly as the build leaves it | all four XMLs `missing from bundle resources`; `no Prefabs folder in bundle resources - the rack module browser will be empty` |
| **b** | flat, with the staged resources copied **beside** the binary | `ControlsXp.xml enriched 2 of 18` (+3 more); **`6 rack prefab(s) seeded from the bundle`** |
| **c** | **unzipped from `TIDE-Rack-Windows.zip`**, run from inside the bundle | the same four `enriched` lines; **`6 rack prefab(s) seeded from the bundle`**; `rack built for 48000 Hz` |

**(b) proves the runtime rule, (c) proves the shipped asset obeys it, and (a) is
the control that makes either mean anything.** (c) is run by dropping the
STANDALONE `TIDE-Rack.exe` *inside* the shipped bundle beside the DLL: its
module path is then the same shape the host sees, so the same `BundleInfo`
branch runs, and unlike the VST3 it prints to a stderr I can read. That
substitution is the one thing about (c) worth distrusting, and it is why (b)
exists.

### The installer is proven, not merely compiled

Windows has no counterpart to macOS's `installer -target <sandbox volume>`: the
destination is a fixed machine path under Program Files, so a real run needs
elevation and this session has none (`IsInRole(Administrator)` = **False**).

So `-SelfTest` compiles **the same `.iss` a second time** with two `#define`s
overridden — `Vst3Dir` at a scratch folder, `PrivilegesLevel` at `lowest` — and
runs that copy silently. **11 files installed, every one SHA-256 identical to
the staged payload; none missing, none differing, none extra; and the
uninstaller removed the bundle whole.** The two overrides are the *only*
difference between the two compilations: same `[Files]`, same
`[UninstallDelete]`, same `[Code]`. They are compile-time defines rather than
runtime hooks precisely so the shipped installer cannot be talked into using
them.

Inno Setup was not on this box. Installed **user-scope**
(`winget install --id JRSoftware.InnoSetup --scope user`) so it landed in
`%LOCALAPPDATA%\Programs`, needing no administrator and no change to
`Program Files`.

### Signing: not done, not claimed, and the block is unverified

It runs only when `AZURE_TENANT_ID` / `AZURE_CLIENT_ID` / `AZURE_CLIENT_SECRET`
are set, under the `SynthEdit Limited` identity R1(a) settled, with endpoint /
account / profile defaults taken from `SE16/SynthEdit_store_win.yml:199-211`.
**It has never run.** `ArtifactSigning@1` is an Azure Pipelines task with no
local or GitHub Actions equivalent, so the script drives `signtool.exe` with the
Azure Code Signing dlib instead — the documented stand-in, whose dlib is not
installed here. It **throws** rather than silently skipping if credentials are
present without `TRUSTED_SIGNING_DLIB`, so it cannot claim a signature that did
not happen. **R5 owns the secret store and should treat that block as a starting
point to test, not as working code.**

### What did NOT work, so nobody repeats it

**A portable REAPER did not run unattended.** Copying
`C:\Program Files\REAPER (x64)` to a scratch folder with a `reaper.ini` beside
`reaper.exe` and calling `-renderproject` produced **zero bytes of output and no
wav in 240 s** — it stalls on a first-run modal. It was also **not isolated**:
Jeff's `%APPDATA%\REAPER\REAPER.ini` and `reaper-fxtags.ini` carry that
attempt's timestamp. Nothing of his was deleted or edited by hand, but the run
touched them, and this entry is where that is recorded rather than left to be
noticed. The scratch copy is gone. Separately, **`render-and-measure.py` is
mac-only as written** — `REAPER = "/Applications/REAPER.app/Contents/MacOS/REAPER"`,
hardcoded at line 51 — so the five audio fixtures cannot be measured from this
box without changing that script, which was outside R2 and is not changed here.

**Learned:**

1. **A packaging script's real job is deciding what the shipped layout IS, not
   copying a build tree into a zip.** Half of R2 turned out to be the discovery
   that the artifact the build emits cannot be installed correctly, and no
   amount of installer scripting would have surfaced that — running the thing
   did.
2. **`afxres.h` names a missing header and means a wrong Visual Studio
   instance.** Two VS 18 instances on one box, only one carrying MFC, and CMake
   picks by its own rule. The two failing files are exactly the two P3 exists to
   de-MFC, which is what makes the error read as a code problem.
3. **Windows has no sandboxed installer run, so the way to prove one is to
   compile it twice.** Two `#define`s and a SHA-256 tree comparison is a stronger
   claim than a real elevated install would have been anyway, because it
   compares against the payload rather than against expectations.
4. **The app version STEP 0.5 asks for IS discoverable on this box**, contrary to
   the standing lesson from A13 (2026-08-14): `appVersion: '1.34493.1'` sits in
   `%LOCALAPPDATA%\Claude\Logs\main.log`. One grep.
5. **A "portable" REAPER on Windows is neither portable nor unattended.** It
   stalled on a modal and still wrote to the user's roaming profile. If a windows
   run needs a host, the honest options are Jeff's own REAPER — saying what was
   touched — or nothing.

**Next:**

1. **S36 is the row to take next, and its (a) is minutes** —
   `SynthEditSem/CMakeLists.txt` is TIDE's own file. Its (b) is the better fix
   and is PR-GATED in GMPI; it would also bear on
   [#271](https://github.com/JeffMcClintock/TideSynth/issues/271). **Any Windows
   user of a developer build has an empty module browser today.**
2. **R5 must test the signing block before relying on it** — see above. Until
   then `TIDE-Rack-Windows.exe` draws a SmartScreen warning and a UAC prompt
   naming an unknown publisher, and the script says so on every run.
3. **`docs/building.md` should carry `CMAKE_GENERATOR_INSTANCE` for the root
   `CMakeLists.txt` path**, not only for the SE16 one. Not done here — it is
   N1b's neighbourhood and this run had one item.
4. **This platform's queue is empty again once R2 merges.** P3 is still the only
   other `win` row and is still GATED; the NEXT cell now says so without naming a
   dead row.

**Machine left clean.** TideSynth's checkout was never switched — all work
happened in a **separate worktree** at `C:\SE\_r2`, because a concurrent session
is live in `C:\SE\TideSynth`: `modules/common/TidePathTracer.cpp` was dirty with
**227 lines of real content** (Kulla-Conty multiple-scattering compensation;
`git diff --ignore-all-space` non-empty) and local `main` carried **two unpushed
commits**, both `tide_render`. Neither was committed, reverted nor stashed, and
both have since landed on `origin/main` as somebody else's work. The worktree is
removed. Nothing was written to `C:\Program Files`; Inno Setup went to
`%LOCALAPPDATA%\Programs`. `%APPDATA%\TIDE Rack\` did not exist before this run,
was created by the standalone launches, and was removed. `%APPDATA%\REAPER` is
as noted above. No other repo was touched — this item is entirely inside
TideSynth.

**`C:\SE\TideSynth` ends this run parked on `review/renderer`, and that is NOT a dead agent run — do not reset it.** The concurrent session created `review/renderer-base` and `review/renderer` at 08:52–08:53 and committed to them **as Jeff McClintock** (`10247d3ba`, *"The renderer's arc since its last review, as one commit"*); the reflog shows the checkout moving `main` → `review/renderer-base` → `main` → `review/renderer` while this run was pushing. It was on `main` when this run started and this run never switched it. Putting it back would yank a live session off its own branch, so STEP 5's "return every working copy to its default branch" is deliberately not applied here — the third dirt category's reasoning (never touch work in progress that is not yours) governs the branch as much as the files.

**Filed as S36, not S35.** The macos box filed a different **S35** (P11's mac half)
hours after this row was written and landed first, so mine was renumbered on
rebase -- **A23's duplicate-id hazard, live**, caught by reading the rebased file
rather than by the lint, which sees one id per row and cannot see two rows
racing from different branches.

**Branch/PR:** `tide/win/R2-windows-installer` — TideSynth only. Two new files,
plus the R2 and S36 rows.

---

---

## 2026-08-22 — macos — S27: four suspects eliminated, and the reference box turns out to be x86_64

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **S27** — `tide_render`'s image references failing on mac — and
eliminated four candidate causes by measurement. No fix; the row wants a
decision, and the decision is better made knowing what it is not.

### The experiment only a Mac can run

S27's hypothesis is *"libm transcendental divergence ... or an x86-vs-arm64
divergence — labelled a hypothesis, not a diagnosis"*. Those two are separable on
exactly one kind of machine: an Apple Silicon Mac, which builds and runs **both**
architectures against **one** OS, one libm, one compiler.

`modules/common` is a self-contained CMake project with no GMPI or SDK
dependency, so this cost two configures.

**arm64 render vs x86_64 render, same machine:**

```
ok  knob        0.168%   worst delta 9        ok  knob (fast)        0.000%  delta 1
ok  materials   0.125%   worst delta 16       ok  materials (fast)   0.000%  delta 1
ok  shapes      0.396%   worst delta 14       ok  shapes (fast)      0.000%  delta 1
ok  glass       0.146%   worst delta 10       ok  glass (fast)       0.000%  delta 1
ok  glow        0.000%   worst delta 1        ok  glow (fast)        0.000%  delta 1
```

**All ten pass**, inside the existing 0.400% / delta-40 limits. Against the
committed references, both architectures fail at **35–67%**, worst delta 46–142.

Two orders of magnitude apart. **ISA is not what moves the image.**

### Three more suspects, three more eliminations

**Renderer drift.** Eight commits touched the tracer after `37d65d5`, and the
current tip `4128291` (*telecentric depth of field*) landed without re-baking —
so "the references are just stale" was the obvious reading. Built at `25e0bf6`,
the last commit that *did* update them: **identical figures to three decimals**
(35.301 / 35.007 / 66.917 / 54.465 / 61.535), the same numbers `main` gives. The
references never matched a mac build at any commit.

**The RNG.** A hand-rolled PCG over `uint32`/`uint64` shifts
(`TidePathTracer.cpp:84`). No `std::uniform_real_distribution`, no
`std::mt19937` — the usual cross-stdlib trap is absent, so the sample sequence is
bit-identical everywhere.

**Inherent nondeterminism.** The five `(fast)` variants pass at **0.000%** on both
architectures, and the code explains itself: fast mode uses *"a fixed sub-pixel
GRID, not jittered draws ... bit-deterministic without the RNG being involved at
all"* (`:2539`).

### What survives, with an argument instead of a guess

The divergence is confined to the transcendental-heavy Monte Carlo path.
`shadeFast()` contains **1** transcendental call; the tracer overall contains
**19**. `sqrt` is excluded from that count — IEEE-754 requires it correctly
rounded, so it cannot diverge. Few transcendentals → bit-stable. Many → 35–67%.

### And the reference box is x86_64, but not a Mac

The x86_64 build reproduces all five fast references **bit-exactly, worst delta
0**, while arm64 is off by one — yet x86_64-on-macOS still fails the full scenes
identically to arm64. So the references came from an **x86_64 machine running a
different libm**: Windows or Linux.

### Two corrections to the row, and one new problem

The row says *"5 of 5 scenes fail"*. It is **5 of 10 checks** — every fast variant
passes, and that asymmetry is the most useful fact available.

I also misread the exit status once: piping the tool into `tail` and echoing `$?`
reports **`tail`'s** status, so the test looked like it passed while printing five
failures. It exits **1**, correctly.

**New, latent:** `shapes` cross-ISA is **0.396% against a 0.400% limit** — one
percent of margin. Even with correct references that scene is borderline flaky.

**Learned:**

- **An Apple Silicon Mac separates ISA from OS/libm in a way no other box can** —
  two architectures, one operating system. When a cross-platform difference is
  suspected, that is the cheapest possible discriminator, and it costs one extra
  `-DCMAKE_OSX_ARCHITECTURES`.
- **`$?` after a pipeline is the LAST command's status.** `tool | tail` then
  `echo $?` reports `tail`. Use `${PIPESTATUS[0]}`, or don't pipe when the status
  is what you came for — I briefly recorded a failing test as passing.
- **Check whether a hand-rolled RNG is actually the portable kind before blaming
  it.** `std::uniform_real_distribution` differs between libc++ and libstdc++ and
  is the classic cause; a PCG over integer shifts is not, and ruling it out took
  one grep.
- **`sqrt` is not a cross-platform divergence source.** IEEE-754 requires it
  correctly rounded. `pow`/`exp`/`log`/`sin`/`cos` are not required to be, and are
  where libm implementations actually differ — so count those separately.
- **A passing subset is a control, not noise.** The fast-mode scenes passing at
  0.000% is what turns "the renderer is nondeterministic" into "the renderer is
  deterministic except in the path that calls transcendentals".
- **Rebuild at the commit that produced the artifact before assuming drift.** It
  cost one build to kill the most plausible explanation, and believing it would
  have sent someone to re-bake references that were never right.

**Next:** the decision is still **Jeff's** — per-platform references, a tolerance
derived from measured cross-platform residual, or pinning the math — but it can
now be made knowing ISA and the RNG are irrelevant, that a bit-stable subset
already exists to build on, and that `shapes` needs headroom regardless.
**Whoever owns it should also decide where the test runs**, since TideSynth's root
force-disables `TIDE_RENDER_PREVIEW` and no CI has ever executed it.

**Branch/PR:** `tide/mac/S27-isa-vs-libm` — TideSynth, backlog and journal only.

---

## 2026-08-22 — macos — E1c: the hypothesis was already refuted by a table in this repo

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **E1c**, could not run its harness here, and found that the
experiment it asks for is unnecessary — the hypothesis it names is contradicted
by numbers already written down. Then fixed the thing that made establishing
that take an afternoon.

### The row asks the wrong question

E1c says to test whether *"the core oscillator's phase increment is
cross-platform stable where the naive one's is not"*. E1a's own table settles it:

| case | oscillator | pitch input | residual | E1a's class |
|---|---|---|---|---|
| `osc_naive_sine` | `SE Oscillator (naive)` | **not connected** | **−73.5 dBFS** | 12 LSB, *"not rounding"* |
| `voice_midi_note` | `SE Oscillator (naive)` | keyboard | −123.1 dBFS | 1 LSB, *"pure rounding"* |
| `prefab_oscillator` | `Oscillator` (core) | patch point, 5 V | −131.1 dBFS | rounding |
| `prefab_filter` | `Oscillator` (core) | patch point, 5 V | −121.4 dBFS | rounding |

The top two are **the same module**, measured macOS-vs-Linux-goldens in one run,
identical on three independent engines — and they are **50 dB apart**. The module
cannot be the variable.

What co-varies instead is visible in the scripts: `osc_naive_sine` is the only
case whose **pitch input is connected to nothing**, so it free-runs on the module
default. Every case that drives pitch — from a keyboard or from a pinned patch
point, naive oscillator or core — is rounding class.

So `prefab_oscillator` and `prefab_filter` are rounding-class cases carrying
phase-drift-class gates, and their `tolerance_reason` cites a mechanism their own
scripts do not exhibit. And the settling experiment is now **one** case, not two:
the naive oscillator *with* pitch pinned. Rounding class confirms the pitch
reading; −73 dB class restores the module reading.

### The part that cost the time, and the part I fixed

Every number above needed its platform pair established before it meant
anything, and **nothing in `tests/references/` recorded that.** It took a journal
entry from nine days earlier plus a sentence buried in a case description
(*"REFERENCE SEEDED ON macOS, 2026-08-18 — unlike the other two, which came from
Linux"*) to work out which WAV came from where.

The four were comparable at all only because one run happened to produce them —
luck, not method. A null-test residual means *"rounding, ignore it"* if both
sides ran on one platform and *"cross-platform drift, size your gates for it"* if
they did not, and those are opposite conclusions from the same number.

So `--update-refs` now writes `tests/references/<case>.provenance.json` recording
system, release, machine, engine build and the reference hash. Six existing
references backfilled, honestly graded: three `reconstructed` with the evidence
quoted, and **three `unknown`** — `prefab_envelope`, `prefab_filter`,
`prefab_midi` — where nobody wrote it down and I could not establish it.

`prefab_filter` is the interesting one. E1c calls its −121.4 figure a Linux
verify *"against macOS-seeded references"*, which implies Darwin — but the
reference file was added on 2026-08-20 and the measurement is dated 2026-08-19.
Those do not line up, so recording Darwin would have been inventing a fact that
merely sounded right. It is `unknown`.

### Verification

`--selftest` needs no engine, which is the whole point given the harness needs
`SynthEditCL` and a Linux box. Six new cases, and both negative controls bite:
hardcode the platform → the platform check fails; write the sidecar under the
wrong name → the path check fails. Restored, the suite passes.

### What I did NOT do

**The gates are unchanged.** E1c's Accept requires a positive control —
tightened gates passing on both platforms while failing a deliberate regression —
and `verify.yml` is `ubuntu-24.04` only and needs `SynthEditCL` from the private
repo. **Changing a gate without that control is exactly what created this row.**

**Learned:**

- **Before designing an experiment, check whether the repo already ran it.** E1c
  named a hypothesis two existing measurements refute. The table was in
  `JOURNAL-2026-08.md` and the module names were in the case files; nothing
  needed to be rendered.
- **When two cases differ by 50 dB, list every way they differ before believing
  the first explanation.** "Naive vs core oscillator" was the obvious reading and
  it was wrong — the same module appears on both sides. The undriven pitch input
  was the only variable that actually tracked the split.
- **A measurement without its provenance is not a measurement.** A null-test
  residual supports opposite conclusions depending on whether the two sides ran
  on the same platform. Record the platform pair *with the artifact*, at the
  moment it is produced — reconstructing it later is archaeology and sometimes
  impossible.
- **Grade backfilled facts explicitly.** `measured` / `reconstructed` / `unknown`
  keeps a later reader from treating a plausible inference as a record. Marking
  `prefab_filter` unknown was more useful than recording the Darwin the row
  implies, because the dates do not support it.
- **A harness that needs an engine should still have a mode that does not.**
  `--selftest` is why this change is verified at all from a box that cannot run
  the real suite.

**Next:** the one-case experiment (naive oscillator, pitch pinned to 5 V) settles
the mechanism and wants a **Linux** box, since that is where `verify.yml` runs.
Then the gates can be justified rather than inherited. **Re-seeding
`prefab_envelope`, `prefab_filter` or `prefab_midi` fixes its `unknown` record as
a side effect** — worth doing on whichever platform is going to own them.

**Branch/PR:** `tide/mac/E1c-reference-provenance` — TideSynth.

---

## 2026-08-22 — macos — S31: the trap only exists on Linux, and that is why writing it down four times did not work

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **S31** — `pkill -f <pattern>` killing the shell that runs it —
and shipped `scripts/kill-named.sh` with a 7-case suite. The useful finding is
not the script. It is *why* the lesson had failed to stick through two journals,
one doc and three repeats.

### The negative control refused to reproduce

First move was to reproduce the bug, since a fix for a bug you have not seen is
a guess. Backgrounded a probe, then ran `pkill -f $PAT` from a shell whose own
command line contained `$PAT` — the exact shape of all three recorded hits.

The shell survived. Exit 0.

That is either a broken test or a wrong premise, and the way to tell is to ask
whether the signal was even sent. Trapping `TERM` in the calling shell answers
it:

```
  trap TERM; kill -TERM $$      -> ">>> got TERM", rc=143     (it CAN die)
  trap TERM; pkill -f $PAT      -> ">>> finished normally"    (nothing arrived)
```

Not ignored — **never delivered**. Then `man pkill`:

> **-a** Include process ancestors in the match list. By default, the current
> pgrep or pkill process **and all of its ancestors are excluded**.

**BSD `pkill` excludes ancestors by default. GNU procps excludes only itself.**
So the trap is Linux-only. All three recorded hits were on the linux box. A mac
or windows run cannot reproduce it however carefully it tries — *"it worked when
I tested it"* was true, and useless.

That is the actual reason four retellings failed: two of the three boxes reading
the lesson could never see the behaviour it described.

### The platform split forced the test design

If cases only check behaviour — *does it kill the target, does the shell live* —
then on macOS **they pass whether or not the filter works**, because the OS is
already doing the filtering. I did not reason my way to that; I broke the
ancestor walk on purpose (`p=1` before the loop, which is precisely the Linux
bug) and re-ran:

```
  PASS  kills the named process
  PASS  the calling shell survives (rc=0)      <- the bug is LIVE and invisible
  FAIL  ancestor list contains the calling shell
  FAIL  ancestor list contains the grandparent
```

So the suite asserts the ancestor list **directly**, through a
`--print-ancestors` mode, instead of inferring it from behaviour the OS would
mask. Second control, `is_ancestor` forced true: case 1 fails — it spares
everything and kills nothing. Both breaks are caught; the restored script is 7/7.

### What I did not verify

**The suite has never run on Linux or Windows** — which is where the bug lives.
It is POSIX `sh` with a `ps`-based ancestor walk and no `/proc` dependency, but
that is an argument, not a measurement. One `sh tests/s31_kill_named_test.sh` on
the linux box closes the row.

**Learned:**

- **When a negative control refuses to reproduce a documented bug, that is a
  result, not a broken harness.** Chasing "why didn't it fire" turned a
  three-line script into the actual explanation for why the lesson never stuck.
- **`pkill -f` self-kill is a Linux-only trap.** BSD (macOS) excludes the caller
  and all ancestors by default; GNU procps excludes only the caller. Check
  `man pkill` for `-a` before assuming a `pkill` behaviour is portable.
- **A lesson that two of three boxes cannot reproduce will not stick by being
  written down again.** The fix is a mechanism that behaves identically
  everywhere, or the platform caveat stated up front so the boxes that cannot
  see it know they are not the audience.
- **Test what the OS might be doing for you, directly.** If a platform makes
  your safeguard redundant, your behavioural tests pass with the safeguard
  removed — so they are not testing it. Assert the internal state instead, and
  prove it by breaking the code and watching the right case fail.
- **Ask whether the signal was delivered, not whether the process died.**
  Trapping the signal separates "not sent" from "sent and ignored", which are
  different bugs with different fixes.
- **Silence expected noise in test output.** Every passing case printed
  `Terminated: 15` from job control; starting probes in a detached subshell
  removes it. Output that always appears is output nobody reads, so a real
  failure hides in it.

**Next:** **the linux box should run `sh tests/s31_kill_named_test.sh` once** —
that is the only outstanding evidence, and it is the platform the bug is real on.
Nothing else blocks the row.

**Branch/PR:** `tide/mac/S31-kill-by-pid` — TideSynth.

---

## 2026-08-22 — macos — R4a: CLAP was in nobody's build, and my own Linux fix was a half-fix

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **R4**, split it, shipped the half that is not Linux-bound as
**R4a**, and spent most of the item repairing a GMPI fix I had already opened —
which is the part worth reading.

### R4's Accept named a format nothing built

R4 wants a Linux tarball carrying *"the VST3 and CLAP bundles"*.
[docs/distribution.md](docs/distribution.md) lists CLAP as shipped on all three
platforms, and R2 and R3 say the same thing. But `FORMATS_LIST` in
[SynthEditSem/CMakeLists.txt](SynthEditSem/CMakeLists.txt) read
`GMPI VST3 STANDALONE`, so **no platform was building a CLAP at all.** Three
installer rows had been waiting on a one-word change nobody had noticed was
missing, because a missing format produces no error — only an absent file.

It cost one word. GMPI already had the wrapper wired, and the format picked up
N1a's `OUTPUT_NAME` for free.

**Measured on the artefact, not the build log** — the build succeeding says
nothing about whether the thing it made is loadable:

```
TIDE-Rack.clap/Contents/MacOS/TIDE-Rack        arm64
  nm -gU | grep clap_entry   ->  _clap_entry        (exactly 1 entry point)
  Contents/Resources/Prefabs/{Oscillator,Envelope,MidiCv,Filter}.synthedit
configure rc=0, build rc=0, 0 CMake errors, 0 compiler errors
```

### A suspect eliminated is still progress

The CLAP bundle's `CFBundleIdentifier` is **present and empty**
(`<string></string>`) — I first wrote "missing", because `PlistBuddy -c Print`
returns nothing either way, and the distinction turned out to name the cause.
It looked like a CLAP-specific plist gap until I checked the other formats as a
control: `TIDE-Rack.vst3` and `TIDE-Rack.gmpi` are identical, and only the
`.app` has a real one. **So CLAP is eliminated — this is pre-existing and
belongs to the signing track, not here.**

**One cause explains both symptoms.** The plugin formats fall through to CMake's
stock `MacOSXBundleInfo.plist.in`, and `MACOSX_BUNDLE_GUI_IDENTIFIER` is set
only on the STANDALONE branch (`GMPI/gmpi_plugin.cmake:775`), so the
substitution yields an empty string — and that same stock template is why all
four bundles also declare `CFBundlePackageType=APPL` rather than `BNDL`. The fix
is one property per format, not a new plist. Filed as **R8** rather than fixed
in passing.

What `codesign` actually does with it is worth recording, because I tested it
instead of assuming: it **succeeds**, inventing
`TIDE-Rack-555549444341029c5cd537c59001e63d20c200d3` from the executable name
and a hash. So it is not a signing blocker, but that string is what Gatekeeper
and notarization tickets key on. R5 is where it bites.

### The part I got wrong: I fixed the lines my grep returned

Earlier this session I opened [GMPI#6](https://github.com/JeffMcClintock/GMPI/pull/6)
for the Linux VST3 bundle-name mismatch N1a introduced. The linux box had found
the same defect independently, **measured on Linux** rather than reasoned from
the CMake, and filed it as [#271](https://github.com/JeffMcClintock/TideSynth/issues/271).
It closed its own [GMPI#7](https://github.com/JeffMcClintock/GMPI/pull/7) under
the first-filed rule and left me a review. All three of its points were correct
and all three were mine to fix.

**1. It was a half-fix that would have made things worse.**
`SynthEditSem/CMakeLists.txt` stages the VST3's `Resources/` into a path spelled
to match GMPI's expression character-for-character. Merging GMPI#6 alone gives
Linux *two* bundles:

```
TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so    <- loadable, NO resources
TIDE_Rack_VST3.vst3/Contents/Resources/Prefabs/…     <- resources, never loaded
```

A loadable plugin with no prefabs is the exact failure **S21** exists to prevent.
[#274](https://github.com/JeffMcClintock/TideSynth/pull/274) is the companion and
the two must land together.

**2. I missed a second site.** `copy_plugin()` ships the same mismatched pair into
`~/.vst3`. I had grepped for uses of the `vst3_bundle` *variable* and declared the
class closed; that site spells `${TARGET_NAME}.vst3` directly, so my search could
not have found it. **The grep was a proxy for the class and I mistook it for the
class.**

**3. My edit rewrote 1,274 lines.** `gmpi_plugin.cmake` is pure CRLF. I edited it
with Python `open(p, 'w')`, whose text mode wrote LF, so a three-line change
arrived as 1,288 insertions / 1,274 deletions — unreviewable, and `git blame`
destroyed. Redone in binary mode: **10 insertions / 4 deletions**, matching the
linux box's own diff exactly.

### One I deliberately did not fix

`${SUB_PROJECT_NAME}.appex` at `gmpi_plugin.cmake:1057`/`:1067`/`:1076`/`:1095`
is structurally identical — `$<TARGET_BUNDLE_DIR:...>` copied to a target-named
destination. I flagged it in the PR body and left it: an appex resolves through
its `Info.plist` rather than by name-globbing, so the consequence is probably
cosmetic, and TIDE builds no AU3, so **neither box can test it either way.** A
speculative edit in a PR-GATED repo is worse than a note.

**Learned:**

- **A format missing from a build list produces no error, only an absent file.**
  CLAP was named in `docs/distribution.md` and in three rows' Accept clauses
  while being in nobody's `FORMATS_LIST`. Documents that describe an artifact
  are not evidence the artifact is built; `ls` the build tree.
- **Grepping for a variable name closes the uses of that variable, not the
  defect class.** `copy_plugin()` had the identical bug and spelled the path
  literally. Before claiming a class is fixed, search for the *shape* of the
  defect — a hand-built path next to a generator expression — not the identifier
  that happened to appear in the first instance.
- **Never edit a CRLF file with Python text mode.** `open(p, 'w')` silently
  normalises every line ending in the file. `open(p, 'rb')` / `'wb'` with
  explicit `\r\n` keeps the diff to the lines actually changed. Check with
  `d.count(b'\r\n')` against `d.count(b'\n')` before and after.
- **Checking a control turns a bug report into an elimination.** The missing
  `CFBundleIdentifier` looked like a CLAP defect until the other three bundles
  were checked and had it too. One extra command moved it from R4a's blocker to
  R8's finding.
- **A duplicate found from two boxes is not waste** — the second box's review is
  what caught two of the three defects in the first box's fix.
- **Check a lint's EXIT CODE, never grep its output.** I ran
  `check-id-refs.py 2>&1 | grep -viE "advisory|umbrella|E2|…"` to skip a known
  advisory, and the filter swallowed a real `SHARED LOCATION` failure — CI
  caught it on #275 instead. A check that distinguishes advisory from fatal
  *in its return code* is telling you something a grep cannot. There is now a
  helper that runs all seven the way `lint.yml` does and reports rc.
- **Invoke a lint exactly as CI does or the local run means nothing.**
  `check-prompt-provenance.py` and `check-journal-prepend.py` take **file
  paths** to the base copies, not git refs. Passed refs, they fail on branches
  CI has already marked green — a false alarm that trains you to ignore the
  tool. `grep -nE "run: python3 scripts/" .github/workflows/lint.yml` is the
  source of truth.
- **Filing a row out of another row duplicates its citations.** Splitting R8
  out of R4a, and S35 out of P11, each collided on a `file:line` a live row
  already cited. Decide which row *owns* each line: the measurement row keeps
  the evidence, the fix row keeps the line a patch would touch. R8's real
  subject was `:775` (`MACOSX_BUNDLE_GUI_IDENTIFIER`) rather than N1's `:774`
  (`MACOSX_BUNDLE_BUNDLE_NAME`) — the collision was also a sloppy citation.
- **`gh pr edit` needs `read:org` and the agent token has only `repo`.** Use
  `gh api -X PATCH repos/OWNER/REPO/pulls/N --input file.json` to set a body.
  `gh pr comment` works fine.

**Next:** **R4 stays on the `linux` box** — the tarball and `install.sh` cannot
be built or verified here — and it should not be packaged until GMPI#6 and #274
both land, or the tarball ships a bundle no host loads. **R4a** is in review.
**R8** is R5's to hit and needs a naming decision from Jeff (publisher is
*SynthEdit Limited* per R1). GMPI#6 is PR-GATED and complete; it needs Jeff, and
it must merge with #274.

**Branch/PR:** `tide/mac/R4-clap-and-tarball` — TideSynth. Upstream:
[GMPI#6](https://github.com/JeffMcClintock/GMPI/pull/6) (rebuilt, PR-GATED).

---

## 2026-08-22 — linux — #271: fixing the bundle name alone would have emptied the bundle

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** STEP 1 outranked the backlog — [#271](https://github.com/JeffMcClintock/TideSynth/issues/271),
the packaging break I filed earlier today, is my platform's product break. Fixed
it as two PRs that must land together: [GMPI#7](https://github.com/JeffMcClintock/GMPI/pull/7)
(PR-GATED, raised as a proposal, not merged) and
[#274](https://github.com/JeffMcClintock/TideSynth/pull/274).

Also resolved #273's conflict first, per STEP 1.5, and corrected an
overstatement in my own issue on the way.

### The one-line fix was a trap

`gmpi_plugin.cmake` builds the Linux VST3 bundle directory from the **target**
name while the `.so` inside takes `OUTPUT_NAME`. Obvious fix: make the directory
follow `OUTPUT_NAME` too. It builds, and it produces **two** bundles:

```
TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so    <- loadable, NO resources
TIDE_Rack_VST3.vst3/Contents/Resources/Prefabs/…     <- resources, never loaded
```

`SynthEditSem/CMakeLists.txt` stages `Resources/` into a path it spells out to
match GMPI's expression **character for character** — and its comment says so, in
as many words. So the GMPI-only fix leaves the loadable bundle with no prefabs
and no pin XMLs: **exactly the failure S21 was filed to fix.** I only saw it
because I built and listed the tree instead of trusting a green rc=0.

With both halves: one bundle, `TIDE-Rack.vst3/Contents/x86_64-linux/TIDE-Rack.so`,
Resources and all six prefabs intact — and `TIDE-Rack.vst3` is the name the five
`tests/hosts/*.rpp` fixtures already expect after N1a.

### Blast radius, probed rather than argued

The PR-GATED rules want to know what else this touches. With `OUTPUT_NAME` unset,
`$<TARGET_FILE_BASE_NAME:t>` **is** the target name — measured with a two-target
CMake probe:

```
plain:   target=plain    base=plain
renamed: target=renamed  base=Some-Name
```

The only other in-tree `gmpi_plugin()` consumer, `SE16/se_gmpi/vst3`, does not set
`OUTPUT_NAME`, so its output is unchanged. That is the argument for a GMPI change
being safe, and it is checkable rather than rhetorical.

### I overstated my own issue, and corrected it

I had written that `~/.vst3/TIDE_Rack_VST3.vst3/…` "is what a Linux user ends up
with". Reasoned from the code, not observed. The whole `copy_plugin()` block is
gated on **`SE_LOCAL_BUILD`** (`gmpi_plugin.cmake:1139`), which this build sets
`FALSE`; the generated `build.make` has no `~/.vst3` reference and TIDE is
correctly absent from that folder. Local developer builds do propagate it;
standalone and CI builds never run the copy. Corrected on the issue.

Corroboration for the convention itself, since I was asserting one: every other
VST3 installed here keeps bundle name == payload name — `Gain_VST3.vst3` →
`Gain_VST3.so`, `Container.vst3` → `Container.so`, `FinalCheckSynth.vst3` →
`FinalCheckSynth.so`.

**Learned:**

1. **When two files are documented as mirroring each other, changing one is a
   half-fix by construction.** The comment in `SynthEditSem/CMakeLists.txt` named
   the GMPI line it copies. Reading the *other* side of a documented pairing
   before editing either is the cheap move.
2. **A build that succeeds can still package nothing.** rc=0 with an empty
   loadable bundle is a worse outcome than a compile error, and only `find` on
   the output tree distinguishes them.
3. **`GMPI_SDK_FOLDER_OVERRIDE` makes a PR-GATED change testable** without
   touching the developer's tree: clone GMPI to scratch, point a scratch build
   at it, and the whole proposal is verifiable before it is proposed.
4. **Write to a CRLF file with Python and you get a 1,280-line diff.** Caught it
   on the first `git diff --stat` — read the byte mode and re-encode. STEP 5
   warns about CRLF churn for stashes; it applies to your own edits too.
5. **Prove a no-op instead of claiming one.** A five-line CMake probe turned "this
   should not affect other consumers" into a printed before/after.

**Next:**

1. **The two PRs must merge together** — GMPI#7 first or simultaneously; either
   alone leaves the bundle split or the names mismatched. Both bodies say so.
2. **#271 stays OPEN** — nothing here loaded the plugin in a host. It is a layout
   check against the rule GMPI's own comment states. Closing it wants the v0.1
   harness against the fixed bundle, which needs REAPER (win or mac).
3. **#273 (N1b)** is conflict-free again and waiting on review.

**Machine left clean.** All builds ran in scratch trees against a scratch GMPI
clone; Jeff's `~/TideSynth/build` and his `~/.vst3` were not written to. TideSynth
back on `main` after this branch.

**Branch/PR:** `tide/linux/issue-271` → [#274](https://github.com/JeffMcClintock/TideSynth/pull/274), with [GMPI#7](https://github.com/JeffMcClintock/GMPI/pull/7).
## 2026-08-22 — macos — P11's mac half had the right symptom and the wrong mechanism

**Prompt:** e214f06 · Fable 5 (claude-fable-5) · app: Claude desktop **1.32885.1** · as **tide-rack-bot** (both paths) · LOOP mode, Jeff present

**Did:** took **P11**, re-measured its mac half, and corrected it. The row said
the mac build has *no* module-database install step. It has one. It installs
into a folder the scanner never reads — which produces the identical symptom and
sends you somewhere completely different to fix it.

### The row set the wrong expected difficulty

> *"the mac build has NO counterpart to the win post-build module-DB copy at all"*

Read that and the job is "add an install step". The job is not that.

| | path | who |
|---|---|---|
| `SE_LOCAL_BUILD` installs to | `~/Library/Audio/Plug-Ins/GMPI` | `GMPI/gmpi_plugin.cmake:1225` |
| the scanner reads | `/Library/Audio/Plug-Ins/GMPI` | hard-coded |

`getPlatformPluginsFolder()` returns the string literal `"/Library/Audio/Plug-Ins/"`
(`SynthEditLib/modules/se_sdk3_hosting/BundleInfo.cpp:152-164`); `"GMPI"` is
appended in `SynthEdit/SynthEdit2/SynthEditApp.cpp:155-164`; that one path is
everything `RefreshModuleData` scans. **There is no
`NSSearchPathForDirectoriesInDomains` anywhere in the scan path** — so unlike
VST3 and AU, which search user *and* system by convention, the user domain is
never consulted. Filed as **S35**.

### Two independent measurements, because one would not have settled it

I started from the cache, not the code. `~/Library/Application Support/SynthEdit/`
holds the `Plugin-Cache-16-override-*.xml` files the scan writes, and the newest
recorded exactly one TIDE bundle: the **stale, system-domain, pre-rename**
`/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi`.

**That alone proves nothing** — that cache was written at 08:09 and the current
`TIDE-Rack.gmpi` was installed at 21:57, so age explains it. The question that
does settle it is *"does the scanner **ever** record a user-domain path?"*:

```
all Plugin-Cache-16-override-*.xml, every date:
  602  /Library/Audio/Plug-Ins/GMPI
    0  ~/Library/Audio/Plug-Ins/...        <- any user-domain path, of any kind
```

Zero, ever. Then the code confirmed the mechanism the cache implied.

### TIDE is not affected — and that is the useful half

TIDE does **no** module scan. S1a removed it; the browser reads a force-linked
in-memory list (`SynthEditSem/TideApp.cpp:434-442`). So this never touches TIDE's
own runtime. It bites **SynthEdit the editor** consuming TIDE as a third-party
module, which is the configuration P11's Windows symptom was found in.

### One thing N1a made permanent

Any `/Library/Audio/Plug-Ins/GMPI/TIDE.gmpi` predating the rename is now an
orphan: the build emits `TIDE-Rack.gmpi`, so nothing will ever update the old
name again, and the scanner keeps serving whatever was last copied there. Two
were sitting on this box — system-domain 2026-08-16, user-domain 2026-05-07.

### An assumption of mine, caught by testing it

I wrote that the hand-copy needs `sudo`, because the folder is system-wide.
`[ -w /Library/Audio/Plug-Ins/GMPI ]` says otherwise **on this machine** — it is
owned by the developer, presumably from an installer or an old `chmod`. On a
fresh machine it is root-owned, which is why SynthEdit's CI runs `sudo mkdir -p`
and `sudo chmod 777` on it
(`SynthEdit/.github/workflows/Export_Tests_mac.yml:34-35`). The doc now says
both and tells you to check, instead of asserting either.

**Learned:**

- **A stale row is most expensive when its symptom is right and its mechanism is
  wrong.** "No install step" and "install step pointing at the wrong domain"
  look identical from the outside and lead to opposite work. When a row's
  mechanism claim is older than a few weeks, re-derive it before costing the
  job — the symptom surviving is not evidence the explanation did.
- **"The cache doesn't list X" is not evidence X is ignored** — it may just
  predate X. The question that settles it is whether the artifact *ever* records
  that class of thing, across every copy you have. One `grep` over all cache
  files was worth more than reading the newest one carefully.
- **`SE_LOCAL_BUILD` on macOS does not do what its name implies.** It installs,
  and the install is invisible to the scanner. Anyone debugging "my rebuilt
  module didn't take effect" on mac is looking at this.
- **Check `[ -w ]` before telling someone to use `sudo`.** Folder ownership under
  `/Library` is not uniform across machines; asserting it wastes the reader's
  time in whichever direction you got it wrong.
- **The shared-citation lint (A31) earns its keep on rows you split.** Filing S35
  out of P11 duplicated two `file:line` citations across both. The right fix was
  not to delete one at random but to decide which row *owns* each line: P11 owns
  the evidence, S35 owns the line a fix would change.

**Next:** **S35** is the real fix and it is **GATED** —
`SynthEditLib/EditorLib/Application.cpp` needs Jeff. Its first task is not the
extra `ScanFolder` call but the question that call raises: whether a bundle
present in **both** domains produces duplicate module IDs. Both copies existed on
this box, so that is testable rather than theoretical. **P11 itself stays open**
for its Windows half and the misleading diagnostic, neither of which this touched.
**Windows and Linux are unexamined** — `getPlatformPluginsFolder()` branches per
platform and I measured only mac.

**Branch/PR:** `tide/mac/P11-mac-module-visibility` — TideSynth, docs and backlog only.

---

## 2026-08-22 — linux — N1b: the rename's live docs, and a Linux-only gap N1a could not have seen

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app 2.1.220 (Claude Code) · as **tide-rack-bot** (both paths)

**Did:** took **N1b** (unblocked once N1a's merged PR let me flip it DONE), and
found a packaging regression on the way to the ground truth I needed.

### The finding is worth more than the item: #271

Building `main` here — the **first Linux build since N1a landed** — is green
(rc=0, 0 errors), but the VST3 bundle is not:

```
TIDE_Rack_VST3.vst3/Contents/x86_64-linux/TIDE-Rack.so
^^^^^^^^^^^^^^^                           ^^^^^^^^^
```

The directory kept the **target** name; the payload took `OUTPUT_NAME`.
`gmpi_plugin.cmake`'s own comment says a host scanning `~/.vst3` looks for
`<name>.vst3/Contents/<arch>-linux/<name>.so` — the two must match, and before
N1a they did (`TIDE_VST3.vst3/…/TIDE_VST3.so`).

**Why mac and Windows verification could not catch it:** they never build that
path by hand. macOS gets it from `BUNDLE_EXTENSION` and Windows from `SUFFIX`,
and both resolve through `OUTPUT_NAME` for free. Linux is the only platform
where the bundle path is spelled out (`:853`), and it is the one platform N1a
could not be checked on. **The same shape as N1a's own commit title** — *"a
rename that skipped work silently"* — happening once more, one platform along.

It reaches the user, not just the build tree: `copy_plugin()`'s Linux VST3 branch
copies by target name on both sides, so `~/.vst3` gets the same mismatch. The
`.gmpi`/`.clap` branch uses `$<TARGET_FILE:…>` and is already correct, which
narrows the fix to one hand-spelled path in two places. Filed as
[#271](https://github.com/JeffMcClintock/TideSynth/issues/271) with the suggested
one-liner; **not fixed** — `gmpi_plugin.cmake` is in GMPI, which is PR-GATED, and
I could not find a TIDE-side fix because TIDE does not control that variable.

### N1b itself: the triage corrected N1's own bucketing

I wrote N1's cost model yesterday and put `docs/state-of-the-prototype.md` in
"live reference docs" **on the strength of its filename**. Reading it says
otherwise — *"Observation only. … Everything below was seen"*, dated 2026-08-06.
Its REAPER Lua transcript, its crash-report text and its P5 finding about the FX
browser would all be **falsified** by a rename. Same for `p4-resize-crash.md`,
whose PDB names sit inside a measured before/after byte table.

So both are **annotated, not rewritten** — against what the N1b row expected of
me. I have said so in the row rather than doing it silently, because it is a
judgement Jeff may want to overrule.

Genuinely live, and updated:

| doc | what was stale |
|---|---|
| `docs/building.md` | the build command, the two-target trap, the macOS copy line |
| `docs/ci/linux-build-deps.md` | build-and-run instructions |
| **`docs/ci/headless-gui-verification.md`** | **wrong within a day of my writing it** |
| `docs/n1-tide-rack-rename.md` | its status line still said TODO |

The headless doc is the one worth noting: I wrote it yesterday and it told the
next run to launch `./TIDE_STANDALONE`, which no longer exists. I corrected it
and **re-ran the recipe end to end** rather than assuming — the renamed binary
comes up under headless weston, prints its command channel, and `pgrep -x
TIDE-Rack` matches, so the shutdown line still works too.

Rather than stamp eleven banners on eleven dated records, `docs/building.md`
gains one **Current target and artifact names** table, so there is a single
authoritative place to check and the records stay untouched.

**Learned:**

1. **Classify a doc by reading its opening, not its filename.** "state-of-the-
   prototype" sounds like current state; it is a dated observation report. My own
   cost model got this wrong 24 hours earlier, and only reading fixed it.
2. **A doc you wrote yesterday is not exempt from going stale.** The headless
   recipe was obsolete within a day, by someone else's merge. Grep your own
   output when a rename lands.
3. **The first build on a platform after a cross-platform rename is a real
   test.** Nothing failed, exit code 0 throughout — the defect is a *name*, and
   only comparing two names caught it.
4. **When two things must agree, check them against each other, not against
   spec.** Both halves of the bundle path were individually defensible; only
   putting them side by side showed the mismatch.
5. **"Verified on two platforms" is not "verified".** N1a was checked on mac and
   Windows and was correct on both, by two different mechanisms — neither of
   which is the mechanism Linux uses.

**Next:**

1. **#271** — one-line fix in GMPI (`$<TARGET_FILE_BASE_NAME:…>` for the bundle
   dir, in both the assembly and the copy). PR-GATED: happy to raise the GMPI PR
   on request, but not unilaterally.
2. **The v0.1 fixtures now name `TIDE-Rack.vst3`** while Linux emits
   `TIDE_Rack_VST3.vst3`. Undetectable here (no REAPER); it resolves itself when
   #271 lands.
3. **N1b's annotate-don't-rewrite call** is Jeff's to overrule if he wanted those
   two files edited.

**Machine left clean.** TideSynth back on `main` after the PR; weston and the
standalone both stopped by pid (**S31**), scratch `XDG_CONFIG_HOME` throughout so
`~/.config/TIDE Rack/` is untouched. The gmpi_ui working tree is now clean — its
2026-08-19 edit went out as gmpi_ui#10 earlier today.

**Branch/PR:** `tide/linux/N1b-live-docs` — TideSynth only. No code change.
## 2026-08-22 — macos — N1a: OUTPUT_NAME renamed three things, and only one of them had an extension

**Prompt:** 5146a61 · Opus 5 (1M context), claude-opus-5[1m] · app Claude desktop **1.34493.1** · as **tide-rack-bot** (both paths)

**Did:** STEP 1.5, not a backlog item. [#268](https://github.com/JeffMcClintock/TideSynth/pull/268) is this platform's own
open PR and its `windows` check was red, which outranks new work. Fixed the
cause, on the same branch, and re-ran N1a's full Accept against a binary built
from the tree being pushed.

### The failure, and why two of three platforms said nothing

```
LINK : fatal error LNK1201: error writing to program database
  'D:\a\TideSynth\TideSynth\build\SynthEditSem\Release\TIDE-Rack.pdb';
  check for insufficient disk space, invalid path, or insufficient privilege
  [...\TIDE_Rack_VST3.vcxproj]
   Creating library ...\Release\TIDE-Rack.lib and object ...\Release\TIDE-Rack.exp
   TIDE_Rack.vcxproj -> ...\Release\TIDE-Rack.gmpi
```

Read those three lines together and the message is a lie about its own cause.
`TIDE_Rack.vcxproj` is writing `TIDE-Rack.lib` at the same moment
`TIDE_Rack_VST3.vcxproj` fails to write `TIDE-Rack.pdb`. It is not disk space.
**`OUTPUT_NAME "TIDE-Rack"` renames three artifacts per target, not one** — the
module, the linker PDB, and the import library — and all three format targets
(`TIDE_Rack`, `TIDE_Rack_VST3`, `TIDE_Rack_STANDALONE`) build into the SAME
directory on Windows. Only the module is disambiguated, by its extension
(`.gmpi` / `.vst3` / `.exe`). The PDB and the `.lib` are not, so three targets
raced for one `TIDE-Rack.pdb` under MSBuild's parallel link.

**macOS and Linux were green on the identical commit**, and that is the whole
trap: neither emits a PDB, neither emits an import library for a MODULE, and on
macOS each artifact is a bundle so even the paths differ. A rename verified
end-to-end on this box could not have shown it here.

Fixed by keying the two side-channel names off the TARGET name, which is unique
by construction (`${PROJECT_NAME}_${kind}`):

```cmake
set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "TIDE-Rack"
    PDB_NAME "${SUB_PROJECT_NAME}"
    COMPILE_PDB_NAME "${SUB_PROJECT_NAME}"
    ARCHIVE_OUTPUT_NAME "${SUB_PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_NAME "TIDE Rack")
```

That also restores the pre-rename convention rather than inventing one: the
PDBs were `TIDE.pdb` / `TIDE_VST3.pdb`, i.e. target-named, which is what
`docs/state-of-the-prototype.md:106` and `docs/p4-resize-crash.md` still record.
Those two are live reference docs and belong to **N1b**, so they are untouched.

### The macOS half is proven inert, by construction rather than by re-rendering

Configured the branch twice — once with the fix, once with it stashed — and
diffed the ENTIRE generated build system under `build/SynthEditSem`, with the
build-directory name normalised away. **Three files differ, and every difference
is a PDB filename:**

| target | before | after |
|---|---|---|
| `TIDE_Rack` | `TIDE-Rack.gmpi/Contents/MacOS/TIDE-Rack.pdb` | `…/TIDE_Rack.pdb` |
| `TIDE_Rack_VST3` | `TIDE-Rack.vst3/Contents/MacOS/TIDE-Rack.pdb` | `…/TIDE_Rack_VST3.pdb` |
| `TIDE_Rack_STANDALONE` | `TIDE-Rack.pdb` | `TIDE_Rack_STANDALONE.pdb` |

All three `link.txt` files are byte-identical. **The baseline column is also the
proof of the diagnosis** — three targets, one PDB name — obtained without a
Windows machine.

### N1a's Accept, re-run against this tree's own binary

Configure rc=0, build rc=0, zero `error` lines. `TIDE-Rack.gmpi`,
`TIDE-Rack.vst3` and `TIDE-Rack.app` all emitted; targets still `TIDE_Rack*`;
`lipo -archs` = `arm64`, as ruled 2026-08-21.

The build does **not** copy the VST3 into `~/Library/Audio/Plug-Ins`, so
rendering straight away would have measured the previous session's bundle. It
was installed deliberately and the identity checked rather than assumed —
installed and built binaries both
`009060be0f4852280bd89e4cabfc3277df0f3040f36d1e15af9232950c5fe816`. Jeff's stale
`TIDE_VST3.vst3` (16 Aug, same plugin ID) was parked for the duration, so
`TIDE-Rack.vst3` was the only candidate, and **restored afterwards**.

| fixture | this run | 2026-08-21 |
|---|---|---|
| `--control` | PASS, −6.0 / −9.0 | PASS |
| `v1-rack` | −6.3 / −17.0, 2 cables | −6.3 / −17.0 |
| `v1-rack-midi` | −6.3 / −17.0, 4 cables | −6.3 / −17.0 |
| `v3-midi-pitch` | −6.2 / −21.1, 4 cables | −6.2 / −21.1 |
| `v3-midi-gate` | −6.3 / −21.2, 3 cables | −6.3 / −21.2 |
| `v1-rack-uncabled` | **silence**, 0 cables | silence |

### The Windows link, which this box cannot compile — verified by CI, then waited for

There is no MSVC here, and STEP 3 forbids fixing a platform blind. This is not
that: the break is this platform's own PR, STEP 1.5 makes it mine, and **the
PR's own `windows` job is the verification** — which is why the fix went to the
same branch, and why the run stayed up for it rather than declaring victory on
a mechanism.

[Run 32492249466](https://github.com/JeffMcClintock/TideSynth/actions/runs/32492249466), on `276e150`:

| job | before (`ea6a1e1`) | after (`276e150`) |
|---|---|---|
| `guard` | success | success |
| `linux` | success | success |
| **`windows`** | **failure — `LNK1201`** | **success** |
| `macos` | success | queued (S30) |

**`windows` went red → green on a one-block change, with `linux` green on both
ends as the control.** That is the A/B this platform could not run locally.

`macos` is still queued and **carries no information about this change** — it
was green on the previous head, the change sets Windows-only properties, and the
local build here was rc=0 with the artifacts checked. S30 (the mac runner
completes ~5% of runs) is why it is still sitting there, and waiting longer would
be waiting on a 5%-likely event to confirm something already measured — the same
call the 2026-08-21 C7e entry made, for the same reason.

**Learned:**

1. **`OUTPUT_NAME` is three renames, and only the one with an extension is
   collision-proof.** Targets sharing an output directory can carry the same
   `OUTPUT_NAME` safely only for the artifact whose suffix differs; `PDB_NAME`
   and `ARCHIVE_OUTPUT_NAME` have no suffix to save them. Any future format
   added to `FORMATS_LIST` inherits this for free now, because both derive from
   the target name.
2. **`LNK1201` names disk space, privilege and path, and means none of them.**
   The line above it in the log — a sibling project writing the same base name —
   is the actual evidence, and it is easy to skim past as ordinary progress.
3. **Configure twice and diff the generated build system.** It answered "can
   this change affect macOS?" exactly, in about a minute, and produced the
   diagnosis of the Windows failure as a by-product. Cheaper and stronger than
   re-rendering, which could only have shown that nothing broke.
4. **A build that does not install is a measurement trap.** `cmake --build`
   leaves the previous bundle in the plugin folder, so a render "after the
   change" silently measures the artifact from before it. Hashing the installed
   binary against the built one is one command and converts a plausible result
   into a proof — the same trap the 2026-08-21 entry hit from the other side,
   with a same-ID bundle rather than a stale one.

**Next:**

1. **[#268](https://github.com/JeffMcClintock/TideSynth/pull/268) is green where it matters and wants only a merge** — `lint`,
   `guard`, `windows` and `linux` all pass on `276e150`; `macos` is queued
   behind S30 and cannot say anything about a Windows-only property. No
   `platform:win` issue was needed.
2. **N1b unblocks when N1a merges** — and it now has two more references to
   carry, both PDB names: `docs/state-of-the-prototype.md:106` and
   `docs/p4-resize-crash.md:461-462`. Noted on its row.
3. **Two stale `platform:mac` issues were closed** — see below; they were never
   this platform's break.

**Also this run, STEP 1 bookkeeping.** [#264](https://github.com/JeffMcClintock/TideSynth/issues/264) and [#260](https://github.com/JeffMcClintock/TideSynth/issues/260) were the only open
`platform:mac` issues and **both were re-verified before being touched**, per
STEP 1's rule about not fixing what you cannot observe. Neither is a macOS
break: each names a branch that no longer exists (both merged), and in each run
**all three platforms failed the Build step together**, which is `main`'s state
at that hour rather than anything about this platform. `main` went green on all
three at `5ef0bf29`, and macOS passes on #268's current head. Closed with that
evidence, and the linux box's own lesson is what made the check cheap — *"a
branch's CI platform issue can be reporting `main`'s break."*

**Machine left clean.** TideSynth returned to `main`, tree clean; every other
repo on this box (`SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`,
`GMPI`) was clean at the start of the run, untouched during it, and clean at the
end — this item is one file in TideSynth. Builds went to the scratchpad, not to
Jeff's trees. `~/Library/Audio/Plug-Ins/VST3/` ends the run as it began, except
that `TIDE-Rack.vst3` is now this branch's build rather than yesterday's;
`TIDE_VST3.vst3` is back and byte-untouched, and deleting it remains Jeff's
call, exactly as the previous entry left it.

**Branch/PR:** `tide/mac/N1a-rename` — [#268](https://github.com/JeffMcClintock/TideSynth/pull/268), TideSynth only. One CMake block.

---

