# Competitive review — modular synth patchers, 2026-08-18

Produced 2026-08-18 (windows, interactive) at Jeff's request. **The first
competitive review this project has had** — nothing in the tree covered it
before, which is worth recording, since PLAN.md has cited VCV Rack, Cardinal and
Reaktor Blocks as design references since 2026-08-09 without any of them being
measured.

Companion: [module-set.md](module-set.md) — the MVP and nice-to-have module
lists. Conventions extracted from Blocks: [blocks-connection-scheme.md](blocks-connection-scheme.md).
**These documents decide nothing**; they are input to E2, E16 and E17.

**Every price, version and format claim carries a source.** Anything not
confirmed from a primary source is marked **UNVERIFIED**. Prices are as
advertised on 2026-08-18 and will drift.

---

## 1. The one-paragraph summary

**TIDE Rack's position is unoccupied, and it is narrower than it looks.** VCV
Rack — the category leader — is free as an application and **$149** the moment
you want it inside a DAW, which is the exact combination TIDE proposes to give
away. But "free modular plugin" is not empty ground: Cherry Audio gives away 22
modules, plugdata is free and open on five platforms *including iOS*, and
Audulus went free in December 2025. The defensible claim is narrower and
sharper: **an open-source, free, Eurorack-style rack that is a plugin first and
survives an iOS AUv3 sandbox.** Nobody occupies that. And the nearest
neighbour — Reaktor Blocks — belongs to a company that entered insolvency in
January 2026 and changed hands in May (§6).

---

## 2. The landscape

### 2a. Literal Eurorack simulators

| Product | Status | Price | OS | Plugin formats | Modules | Licence |
|---|---|---|---|---|---|---|
| **VCV Rack 2** (2.6.6, 2025-11-04) | active | **Free** (standalone only) / **Pro $149**; VCV+ **$29/mo**, or $19/mo annual | Win 7+, macOS 10.9+, Linux | **Pro only:** VST2, VST3, AU, CLAP. **No AAX.** **Free cannot run in a DAW at all** | library **4,396–4,958** depending on how you count (see note) | GPLv3+ **with a non-commercial plugin exception** |
| **Cardinal** (26.02, 2026-02-28) | active | Free | *"FreeBSD, Linux, macOS, Windows and the Web"* — **no iOS**; also ARM, RISC-V, headless | AU, CLAP, LV2, VST2, VST3, standalone. **No AAX** | ~1,382 from 84 brands, all compiled in. *"does not load external modules"* | GPLv3+, **fully viral** |
| **miRack** (4.55) | active | **$14.99** + IAP (AUv3 hosting $4.99, controller $4.99) | iOS 14+, iPadOS, macOS 11+, visionOS | **AUv3**, VST3/AU on Mac | 800+, curated and compiled in | closed |
| **Softube Modular** | **⚠ feature-frozen — no new modules since Oct 2020** | **€89/$89**; add-ons **$25–$199** (Buchla 296e $149) | Win 10/11, macOS | VST2, VST3, AU, **AAX**. **No standalone** | 7 Doepfer + 50+ stock | closed, **iLok required**, 3 activations |
| **Voltage Modular** | **⚠ near-dormant — last substantive release 2.9.2, June 2024** | **Nucleus free**; Ignite $50; Core $99 | Win 7+, macOS 11+ | VST2, VST3, AU, **AAX**, standalone | 22 (Nucleus) / 90+ (Core) + store | closed, no iLok, 4 activations |
| **Reason Rack 14** | **active, and newly strategic** | **$199**, or **$11.99/mo / $99/yr** | Win, macOS | VST3, AU, **AAX**. Standalone needs the Reason DAW | 90+ devices + Rack Extensions | closed, no iLok, 3 machines |

**On the VCV module count:** running
[scripts/community-research.py](../scripts/community-research.py) against the
library manifest cache on 2026-08-18 gives **553 plugins / 4,958 modules**; the
library website reports **4,396**. The difference is almost certainly
unpublished or deprecated entries in the manifest. Either figure supports the
point being made; cite the measurement, not the website.

**Two incumbents are coasting, and that is the softest ground in this market.**
Softube Modular has shipped **no new module since October 2020** — the only
changes are bug fixes inherited from Softube's shared plugin framework — and
still has no MIDI learn, no CC control, ~4-voice polyphony and a fixed
non-resizable window. Cherry Audio's own news feed carried **zero Voltage
Modular items through July 2026** while the company shipped vintage-synth
recreation after vintage-synth recreation.

### 2b. Adjacent patchers

| Product | Price | OS | Plugin formats | Modules | Licence |
|---|---|---|---|---|---|
| **Reaktor 6** (6.5.0, **2023-04-13**) | **$199** (loyalty from $99), perpetual | Win 10/11, macOS 13–15. **No Linux, no iOS** | VST2, VST3, AU, AAX, standalone. **No CLAP** | 70+ instruments; User Library 4,000+ | closed |
| ├ **Reaktor Player** | **Free** | as above | as above | **times out after 30 min with unlicensed content** | closed |
| ├ **Blocks Base** | **Free** (also in Komplete Start) | — | — | **24 modules, 35 preset racks** | closed |
| └ **Blocks Primes** | **$99 / €99** | — | — | **23 or 26 modules — NI's own pages conflict; flag** | closed |
| **Bitwig The Grid** | **Studio edition only, $399.** Essentials $99 and Producer $199 **do not include it** | Win, macOS 10.15+, **Linux** | it is a DAW, not a plugin | **~245 modules across 16 categories**, 390+ presets | closed |
| **Max 9** (9.1.5, 2026-07-28) | perpetual **$807** / $243 yr / $26.99 mo | Win, macOS 11+ | Max is a host; M4L needs Live Suite | — | closed |
| └ **RNBO** | perpetual **$605** / $203 yr / $21 mo — **requires an active Max licence** | — | **exports** VST3, AU, Max external, Raspberry Pi, WebAssembly, C++ | — | closed |
| **plugdata** (0.9.3-2, 2026-03-06) | **Free** | *"macOS Windows Linux iOS BSD"* | *"Standalone VST3 AU CLAP LV2"* + **iOS AUv3** | ships ELSE + Cyclone libraries | **GPL-3.0** |
| **Audulus 4** (4.7, 2025-12-11) | **Free**; IAP module packs $1.99–$14.99; Pro sub $4.99/mo or $29.99/yr | iOS 18+, macOS 14.6+, visionOS. **No Windows/Linux** (dropped after 3.1) | AUv3 | 53 primitive nodes | closed |
| **Drambo** (2.55, Aug 2026) | **$24.99** + IAP (Visual $14.99, Waves/DSP/Formants $4.99 each) | iOS 15.6+, macOS 12.5+ (Apple Silicon), visionOS | AUv3 instrument/effect/MIDI + standalone; **hosts other AUv3 plugins** | **140+ modules** | closed |

Sources: <https://vcvrack.com/Rack> · <https://cardinal.kx.studio/about> ·
<https://www.softube.com/modular> · <https://store.cherryaudio.com/bundles/voltage-modular-nucleus> ·
<https://www.reasonstudios.com/reason-rack> · <https://www.bitwig.com/buy/> ·
<https://www.cycling74.com/shop> · <https://plugdata.org/> ·
<https://apps.apple.com/us/app/plugdata/id6473122351> ·
<https://apps.apple.com/us/app/mirack/id1468259834> · <https://www.beepstreet.com/ios/drambo>.
VCV library census measured on 2026-08-18 by running
[scripts/community-research.py](../scripts/community-research.py) `--source library`.

---

## 3. How patch parameters reach the host — the V2 question, answered by precedent

**This is the most directly actionable section in the review.** BACKLOG **V2**
asks how a patch's parameters become DAW-automatable without the user building a
panel first. The field splits cleanly, and the split is a design lesson.

| Product | Mechanism | Host params | Panel required? |
|---|---|---|---|
| **VCV Rack Pro** | **1024 pre-declared slots**, auto-bound as modules are added; unused ones show as `Unassigned #nnn` | 1024 | **No** |
| **Reason Rack** | **a pre-declared pool advertised at >2000 slots**, bound per rack device as devices are added | >2000 advertised, **~256 practically automatable** (**UNVERIFIED**, community-sourced) | **No** (but Combinator strongly advised) |
| **RNBO** | a `[param name]` object in the patch **auto-exposes** a host parameter. Attributes carry `@min @max @displayname @enum @unit @order`. The default plugin UI is **auto-generated** from the param objects | unbounded | **No** |
| **plugdata** | a `[param name]` object, or the Automation Parameters sidebar | `param1`–`param64` (**UNVERIFIED**, read from source) | **No** |
| **Bitwig The Grid** | Grid module parameters **are** native Bitwig parameters — automatable, modulatable, mappable, controller-API-accessible | native | **No** |
| **Cardinal** | the user must patch a **Host Parameters** module into the signal path, which converts 24 host params to CV | **24, static** | **effectively yes** |
| **Reaktor** | a panel control, plus a per-control **Automation ID** and an instrument **Base ID**. **Multi-Picture controls cannot be automated at all**; IDs are auto-generated but not auto-maintained, so deleting controls leaves holes | — | **Yes** |
| **Max for Live** | a `live.*` UI object in Parameter Mode with visibility "Automated and Stored" | — | **Yes** |

### The engineering lesson, and it is a specific one

**The pre-declared pool is the only design that delivers "it just works, no panel
needed" — and it is what both serious DAW-integrated racks chose.** VST3 and AU
both want a stable parameter list per instance, indexed positionally; a modular
rack has no fixed parameter set because the user creates it at runtime. VCV and
Reason both answer by declaring a large flat pool up front and binding patch
parameters into slots as modules are added.

**Both then made the same mistake, and TIDE can simply not make it.** The
binding is *positional*: it follows insertion order and a recycled free list. So

- deleting a module **orphans** the DAW's automation lane, and
- **Rack may reuse that slot for a newly created module's parameter, silently
  driving the wrong thing.**

Reason has the identical failure mode, plus a lazy-naming bug: parameter names
reach the host so late that the documented Ableton workaround is *to drag the
device a few pixels inside the plugin* to make the names populate. Reason's own
answer is to tell everyone to wrap devices in a **Combinator** and automate its
macros instead — which is to say Reason deferred the problem rather than solving
it, and shipped a custom-panel builder to cover the gap.

**So the ruling for TIDE's V2 is concrete:**

1. **Pre-declare a pool** so automation works with zero user setup.
2. **Bind slots to a persistent, content-addressed identity — module UUID plus
   parameter ID — never to insertion order or a recycled free list.** This is
   the documented failure in *both* incumbents and it is cheap to avoid at
   design time and near-impossible to fix later.
3. **Push parameter names to the host eagerly**, not lazily. Lazy naming is the
   single most-reported Reason Rack integration bug.

TIDE is unusually well placed to do (2), because S11's design already makes the
whole document ride in the plugin state with stable module ids.

**TIDE should follow RNBO/plugdata/Grid, not Reaktor/M4L** — which is
consistent with design note 4 in [design-notes.md](design-notes.md), and is the
concrete shape V2 has been missing. Note V2's stated premise ("without a panel
view") is stale since the 2026-08-13 rack pivot: the rack *is* a panel view. The
declarative principle survives the pivot; the wording does not.

### The gap this opens, and it is TIDE's

The declarative camp pays a price: **RNBO's and plugdata's auto-generated panels
are generic and ugly**, and a real UI means writing C++/JUCE. The panel-first
camp gets a designed surface but taxes the user to build it.

**Nobody currently offers automatic host parameters *and* a good default panel
*and* an optional custom one.** For TIDE that combination is nearly free: the
rack panel *is* the default UI (constraint 1 since the 2026-08-13 pivot), and
each module Container already has an authored face. TIDE can be declarative
*and* well-presented, which is an unoccupied position rather than a compromise.

**One licensing trap worth knowing**, since RNBO is PLAN.md's stated
inspiration: RNBO's exported VST3 binaries are marked **personal use only**;
distributing them requires either a Steinberg VST3 licence agreement or
open-sourcing under GPLv3. TIDE has no equivalent problem — it ships one plugin
under ISC rather than generating plugins for others — but it is a reminder that
"export a plugin" carries obligations TIDE deliberately avoids by embedding
patches instead.

---

## 4. What each one gets right and wrong

### VCV Rack — the category definition

**Right:** the rack *is* the interface and the whole patch is visible; any
output patches to any input; QWERTY-as-MIDI means someone with no hardware makes
sound in seconds; an enormous free ecosystem. PLAN.md already takes all of this.

**Wrong, for TIDE's purposes:** the free/Pro split puts the plugin — the only
form TIDE ships — behind **$149**. Being an application first means the user
solves audio routing themselves. And the open ecosystem produces **no consistent
visual language at all** (§5).

### Cardinal — the precedent that already made TIDE's decision

The existence proof for constraint 7: ~1,400 modules compiled into one binary,
and in its own words it *"does not load external modules and does not connect to
the official Rack library/store."* Also proof that the rack layout survives
being embedded in a plugin.

**The trap TIDE must not fall into:** Cardinal is GPLv3+ **and does not ship on
iOS** — its platform list stops at the web.

### Reaktor Blocks — the closest thing to TIDE's product shape, and the one in trouble

**The most instructive competitor**, because Blocks' *product* shape is nearly
identical to TIDE's:

- **Blocks Base is free** — 24 modules, 35 preset racks — and runs in the free
  Reaktor Player. Curated, pre-wired, ready to play. Exactly TIDE's "the closed
  surface is complete on its own".
- **The free Player is more restricted than it first appears.** You *can*
  re-patch cables on an existing rack's front panel. You **cannot** add or
  remove blocks, cannot open or edit the Structure, cannot build custom blocks,
  and cannot load User Library content — and the Player **times out after 30
  minutes** with unlicensed content. Full Reaktor is **$199**.
- **The unified connection scheme is the hidden engineering**, and it is
  confirmed from two independent sources: all ports are **audio-rate signals in
  the range -1..+1**, with meaning by port role, so anything feeds anything. See
  [blocks-connection-scheme.md](blocks-connection-scheme.md) — **a solved answer
  to E2's open question, worth copying deliberately.**

**Wrong:**
- **Blocks is architecturally monophonic** — signals between Reaktor Instruments
  are always mono, so polyphony needs workarounds (a 4-voice SPLIT, or
  third-party poly blocks). For a modern synth that is a serious limitation.
- **No HiDPI/Retina support**, still unresolved and a standing complaint.
- The structure view is white-cable spaghetti (§5), and Reaktor is a heavyweight
  host to carry.
- **Reaktor 6.5.0 shipped 2023-04-13 and nothing since.**

### Bitwig The Grid — the best ideas, the worst availability

Technically the strongest modulation model in the survey: ~245 modules, all
signals audio-rate *and stereo*, Poly Grid 4× oversampled, parameters natively
automatable, and "pre-cords" giving per-parameter modulation before any cable is
drawn. **But it is locked to the $399 Studio edition and is not a plugin** — it
is a feature of one DAW, with no way to export a Grid patch. An inspiration, not
a rival.

Its own weak points are instructive: **no custom UI for Grid patches** (every
patch looks like a node graph — the opposite of a curated rack), the recorder
module does not persist across save/reload, and hosting a VST inside the Grid is
clumsy.

### Drambo — the iOS design that questions cables entirely

140+ modules, $24.99, AUv3, and it **hosts other AUv3 plugins**. Most
interesting for TIDE: it is **cable-less** — modules auto-connect and signal
flows strictly left-to-right, with modules nestable inside modules. That is a
legitimate answer to the small-screen problem (§5) and worth knowing about even
though TIDE has already committed to a cabled rack.

### plugdata — the open-source precedent nobody in this repo has noticed

Free, GPL-3.0, VST3/AU/CLAP/LV2 on desktop, **and on the iOS App Store with
AUv3 instrument and effect plugins**. A Pd dataflow patcher rather than a rack,
so not a direct competitor — but it has already solved the iOS AUv3 packaging,
distribution and App Store review problems TIDE will hit. **It belongs on the
community-research watch list** (BACKLOG **A28**).

---

## 5. Visual design language

**Method:** official screenshots were downloaded and **viewed directly**, not
described from memory. Five UIs were examined first-hand — Reaktor Blocks, VCV
Rack, Bitwig The Grid, Cherry Audio Voltage Modular, Audulus — plus TIDE's own
current UI from [images/p2-tide-editor-release.png](images/p2-tide-editor-release.png).

### The spectrum, as observed

| Product | Position | Panel | Controls | Cables | Density |
|---|---|---|---|---|---|
| **VCV Rack** | photoreal skeuomorphic | simulated aluminium, **corner screws**, brushed texture, rendered 3.5mm jacks, glowing LEDs | chunky 3D knobs, cast shadows | **thick, saturated, drooping, with specular highlights and shadows onto panels** — they occlude labels | ~6 modules / 1400px |
| **Voltage Modular** | stylised hardware | flat colour rectangles **with screws**; colour = function (green sources, blue amps, red envelopes) | small 3D knobs, lit jack rings | thin, desaturated — **but so many the panels become unreadable** | ~20 modules / 1400px |
| **Reaktor Blocks** | **flat panel, dimensional control** | flat near-white card, thin header, name in small caps. **No screws, no metal, no texture** | **grey cylinder, top-lit gradient, faint sheen, soft low-opacity contact shadow** | none in panel view; white lines in a separate structure view | ~19 blocks / 1920px |
| **The Grid** | flat, diagrammatic | dark card, 1px border, small radius, near-zero shadow | **flat circle with a coloured value arc** | thin beziers routed **behind** nodes; **modulation also shown at the destination** | ~15 nodes / 1600px |
| **Audulus** | pure abstract vector | pure black ground, barely-lighter rounded cards | **a single thin cyan arc** — no cap, no fill | very thin, low visual weight | moderate |

### What Blocks actually does — the "tasteful realism", made implementable

Jeff's description is precise. In Blocks the realism is **not in the panel**; it
is **entirely in the controls**:

- panel: flat, matte, near-white (~`#f0f0ee`), no texture, no bevel, no screws
- knob: grey cylinder, **subtle top-lit vertical gradient**, faint specular
  sheen, small dark pointer
- **the shadow is a soft, low-opacity, barely-offset contact shadow** — ambient
  occlusion, not a dramatic cast shadow. It reads as *the control resting on the
  panel*, not floating above it
- two-tier typography: oversized low-contrast display words used as graphics
  (`LFO`, `OSC`, `8 STEPS`) plus tiny ALL-CAPS functional labels
- roughly two accent hues (cyan primary, coral secondary) over neutral panels
- live inline dataviz — the envelope draws its own curve, the LFO its shape

### The most useful negative result

**Voltage Modular proves that thin cables do not save a dense rack.** Push
module density up and the cables win: in the reference screenshot the panels
underneath are effectively unreadable. This is the strongest argument in the
survey for hideable cables, for the Grid's show-modulation-at-the-destination
model, or for both. Drambo's answer — no cables at all — is the extreme version.

### TIDE today, for the record

Flat mid-grey chrome with a **light grey document canvas** inside it; SynthEdit's
uncollapsible two-column browser taking ~360 of 1672px; **no elevation, no accent
colour, no branding**; large dead regions; and third-party category names scanned
from the developer's own machine visible in the tree. It is a developer tool's
UI. The distance from here to any row in the table above is the real design work.

### Recommendation — filed as BACKLOG E17, for Jeff to rule on

> **A second proposal exists, and it is exploratory.**
> [ui-design-language.md](ui-design-language.md) — found untracked in the
> working tree while committing this review — sketches a
> Marathon/Designers-Republic language. Jeff's characterisation, 2026-08-18:
> *"we're spit-balling with the marathon stuff."* It is worked out in more
> detail on palette, geometry and type than this section attempts, and its
> no-bitmap drawability argument is genuinely strong — but **detail is not
> validation, and the two documents are peers, not a baseline and a comment.**
> They conflict on one specific point: it bans drop shadows, bevels and
> decorative gradients outright, and the recommendation below puts a soft
> contact shadow on controls because that is the thing Jeff actually named
> liking in Blocks. **E17 is where the two get reconciled.**

**Adopt Blocks' material treatment, the Grid's colour discipline, and a true
rack layout.**

1. **Flat panels, dimensional controls.** Matte panel; knobs and sliders carry a
   top-lit gradient and a soft contact shadow. This single decision produces
   Jeff's stated preference.
2. **Colour encodes signal type, not vendor** — the Grid's discipline, not
   Blocks' product-line colours, since TIDE has no product lines. With no manual
   and a closed panel, hue is the cheapest teaching device available.
3. **Cables thin, behind panels, and hideable**, with modulation *also* shown on
   the destination control. Learned from Voltage's failure.
4. **Inline dataviz wherever a parameter has a shape** — envelopes, LFOs,
   filters, waveshapers. Highest information-per-pixel in the survey.
5. **Vector, not bitmap**, stroke-led like Audulus where detail must survive.
   TIDE must be legible in a small plugin window *and* on iOS. **VCV's
   ~6-modules-per-1400px density is the trap.** Note Reaktor's missing HiDPI
   support as the cautionary tale for bitmap-era decisions.
6. **Anchor to the existing brand.** The website already defines `#1c6e8c` light
   / `#6ec2dc` dark on `#101216` — tide-coloured, and close to Blocks' cyan.

**The production argument decides this even if taste does not.** Photoreal
panels are per-module illustration work, must all be drawn by us (no borrowed
hardware art, no photographed faceplates), and cannot be held consistent across
30–60 modules by a very small team. Flat panels plus a **shared control library**
makes a new module a layout, not an illustration. It also ages best and scales
down.

---

## 6. The strategic context nobody in this repo has recorded

**Two of the six most relevant companies changed hands in the first half of
2026, in opposite directions.**

### Reason Studios was acquired by LANDR, and Reason Rack was cut loose as a product

- **LANDR acquired Reason Studios on 2026-01-06.**
- On **2026-01-21/22** Reason restructured its pricing to sell **Reason Rack as
  a standalone product** — $199 perpetual, $11.99/mo, $99/yr — rather than only
  as part of the DAW.
- LANDR's stated strategy is to get the Rack plugin running inside *every* major
  DAW.

**This is the finding that should most affect TIDE's planning.** The closest
existing precedent to "a rack that lives in your DAW" was, until January, a
side-feature of a declining DAW. It now has a well-funded owner actively pushing
it at exactly the position TIDE is aiming for. TIDE is free and open where Reason
Rack is $199 and closed, so they are not the same product — but the category is
about to get marketing money spent on it, which cuts both ways: more attention
on the idea, and a bigger incumbent in it.

### Native Instruments — Reaktor's owner — went through insolvency and was acquired by inMusic

- Preliminary insolvency filed end of January 2026, formal insolvency by March,
  triggered by debt from the Francisco Partners investor structure — roughly
  **£250M of debt against ~£25M annual revenue**.
- **inMusic Brands** (Akai Professional, Moog, Alesis, Denon DJ) signed the
  definitive agreement on **7–8 May 2026** at Superbooth; iZotope, Plugin
  Alliance and Brainworx came with it.
- inMusic subsequently **shut NI's UK office**, with CEO and CTO departing.
- **Reaktor 6.5.0 dates from 2023-04-13.** An NI staff comment suggesting a new
  Reaktor platform was publicly **retracted** — *"there are no plans for a
  Reaktor"* — so Reaktor 7 is speculation. **UNVERIFIED** as to intent either
  way; what is verified is the silence since 2023.

Sources: <https://www.musicbusinessworldwide.com/native-instruments-acquired-by-inmusic-owner-of-akai-and-moog-three-months-after-entering-insolvency/> ·
<https://www.gearnews.com/inmusic-native-instruments/> ·
<https://www.musicradar.com/music-tech/the-tools-you-rely-on-today-will-keep-working-and-the-tools-you-will-rely-on-tomorrow-are-actively-being-built-inmusic-confirms-native-instruments-acquisition-bringing-it-under-the-same-ownership-as-moog-akai-pro-and-many-more>

**Why this matters to TIDE, stated carefully:** the product TIDE most resembles,
and the one Jeff names as a favourite, is maintained by a company that has just
been through insolvency and an ownership change, and its last release was three
years ago. That is an *opportunity signal* — Blocks users may be looking for
something with a future — but it is not a reason to hurry, and inMusic has
publicly committed that existing tools keep working. **Do not put this in
marketing.** It is context for prioritisation, not a talking point.

---

## 7. The correction this review forces

`docs/community-research.md` records A9's standing hypothesis: **"no open-source
modular exists on iOS AUv3."**

**In that strict form it is false.** plugdata is open source (GPL-3.0), free, on
the iOS App Store, and *"ships with AUv3 effect and instrument plugins"*
(<https://apps.apple.com/us/app/plugdata/id6473122351>).

The narrower claim survives and is the one to reason from: **no open-source
*Eurorack-style rack* on iOS AUv3.** miRack, Drambo and Audulus hold that space
and are all closed-source.

A second belief needs softening. It is tempting to conclude GPLv3 *structurally*
bars VCV and Cardinal from the App Store. The tension is real — GPLv3's anti-DRM
terms against Apple's distribution model, and GPL apps have been pulled before —
but **it is enforceable only by copyright holders, and Apple does not audit
licences.** plugdata ships under GPL-3.0 precisely because its author can consent
for their own code. VCV's obstacle is **contributor consent plus the $149 Pro
business**, not law.

**So ISC is a real advantage — fewer questions, no consent problem — but not a
moat. Do not build the strategy on "they legally cannot follow us."**

Filed as BACKLOG **A28**.

---

## 8. What this means for TIDE — the honest read

**Real advantages:**

1. **Free *and* a plugin.** VCV charges $149 for that combination. Nobody else
   gives it away as a Eurorack-style rack.
2. **iOS AUv3.** No open-source *rack* is there; the three that are, are paid and
   closed. ISC makes the route easier than GPLv3 does.
3. **A curated, consistent module set.** VCV's 4,958 modules are its strength
   *and* the reason it has no coherent design language. A fixed set is the only
   way to get consistency, and constraint 7 already commits to it.
4. **Openable Containers.** Reaktor charges $199 to look inside a Block, and the
   free Player cannot open the Structure at all. TIDE gives that away.
5. **Polyphony done properly** would beat Blocks outright, which is
   architecturally monophonic between Instruments.
6. **Two incumbents are coasting.** Softube Modular (iLok, ~4 voices, no MIDI
   learn, frozen since 2020) and Voltage Modular (dormant since mid-2024,
   Java engine) both have installed bases with unaddressed pain.
7. **Patch topology lives in the saved state.** Reaktor **cannot save structure
   changes as presets — only parameter values**, which cuts against the whole
   modular idiom. TIDE's S11 design (the document rides in the plugin state) is
   strictly better, and it is a differentiator rather than merely a fix.
8. **HiDPI.** Reaktor has none and Reason Rack renders tiny in Cubase. A
   vector-first UI wins this by default (§5).

**Real risks, stated plainly:**

1. **Module count is the product, and TIDE has none yet.** Cardinal ships ~1,400;
   the Grid ~245; Drambo 140+; Blocks Base gives away 24; Voltage gives away 22.
   TIDE's realistic first set is ~20 ([module-set.md](module-set.md)) — competitive
   with the *free* tiers and nowhere near the paid ones. **Curation and polish
   have to carry the product**, exactly as PLAN.md says.
2. **"Free" is table stakes on iOS, not a differentiator.** Audulus is free with
   paid unlocks; plugdata is free. The differentiator is *open-source rack*.
3. **Host automation (V2) is not optional**, and §3 shows the field has already
   settled on the declarative answer. A rack inside a DAW that its DAW cannot
   automate fails its own premise.
4. **The AUv3 memory ceiling (~300 MB 32-bit / ~360 MB 64-bit per instance) is
   unrecorded in PLAN.md** ([Apple Developer Forums](https://developer.apple.com/forums/thread/47396))
   and constrains the compiled-in module set, embedded wavetables and rack size
   on *every* platform via constraint 9.
5. **HiDPI and scaling are a decided-early problem.** Reaktor's missing Retina
   support is a decade-old decision it still cannot undo.
6. **The category is about to get money spent on it.** LANDR bought Reason
   Studios in January 2026 and immediately repositioned Reason Rack as a
   standalone plugin aimed at every DAW (§6). TIDE is not competing on price
   with a $199 closed product, but it is no longer competing in a quiet corner.
7. **Automation slot identity is a design decision with no second chance.**
   Both VCV and Reason ship a documented bug where deleting a module silently
   repoints an existing automation lane (§3). Getting this right costs nothing
   now and cannot be retrofitted once patches exist in the wild.

---

## 9. Sources

Primary pages fetched 2026-08-18: <https://vcvrack.com/Rack> ·
<https://cardinal.kx.studio/about> · <https://www.softube.com/modular> ·
<https://plugdata.org/> · <https://apps.apple.com/us/app/plugdata/id6473122351> ·
<https://apps.apple.com/us/app/mirack/id1468259834> ·
<https://community.vcvrack.com/t/vcv-rack-for-ipad-2025/22514> ·
<https://developer.apple.com/forums/thread/47396> · <https://www.bitwig.com/buy/> ·
<https://www.cycling74.com/shop>

Reaktor Blocks connection scheme extracted from the **REAKTOR Blocks manual,
software version 1.1 (12/2015)**, section 4. VCV library census measured locally
via [scripts/community-research.py](../scripts/community-research.py).
