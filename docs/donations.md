# The in-plugin donation affordance — design

BACKLOG **D1**. Produced 2026-08-16 on the macOS box. **Design note only —
nothing here is built, and nothing here is v0.1.**

[PLAN.md](../PLAN.md) "Price and funding" is the requirement:

> **Funding is by donation.** Both the plugin and tidesynth.com should make
> donating possible. Neither should nag: no splash screen, no countdown, no
> modal reminder, nothing that interrupts making sound. A donation route that a
> user has to go looking for is the intended outcome, not a failure of the
> design.

Three constraints narrow it before any design work starts — constraints 1 and 5
(one view, minimal dialogs) rule out a dialog or a second window, so this lives
in the breadcrumb bar or an about pane or nowhere; constraint 3 (sandbox-safe)
puts the obvious implementation, "open a URL in a browser", in doubt.

---

## The headline

**The row's own first question — can an AUv3 open a URL at all — splits by
platform, and that split is the entire design.**

| | can the plugin open a URL? | basis |
|---|---|---|
| **macOS AUv3** | **Yes**, on the evidence below | measured on this box |
| **iOS AUv3** | **No route that can be relied on** | measured (compile) + reported (runtime) |

And iOS is the platform PLAN says drives everything: *"iOS AUv3 is the
constraint that drives the design. If it runs there, it runs anywhere."*

**So the affordance must not depend on opening a URL.** Design it for the
platform that cannot, and let the platforms that can add the click as a
progressive enhancement. That is the recommendation in full; the rest of this
note is why, and what is still unknown.

---

## Method — what was actually measured here

Two independent things get confused in this area, so they were measured
separately: whether the API **compiles** in an app extension, and whether the
**sandbox** permits it at runtime.

### Measurement 1 — compile-time availability

`-fapplication-extension` is the flag Xcode sets for any app-extension target
(`APPLICATION_EXTENSION_API_ONLY = YES`). An AUv3 is an app extension, so this
is the real gate, not a proxy for it. Xcode 26.6, iPhoneOS SDK + MacOSX SDK.

| # | target | code under test | `-fapplication-extension` | clang exit |
|---|---|---|---|---|
| 1 | macOS | `[[NSWorkspace sharedWorkspace] openURL:]` + `activateFileViewerSelectingURLs:` | yes | **0** |
| 2 | iOS | `[[UIApplication sharedApplication] openURL:options:completionHandler:]` | yes | **1 — hard error** |
| 3 | iOS | *same file as #2* | **no** (control) | **0** |
| 4 | iOS | `[vc.extensionContext openURL:completionHandler:]` | yes | **0** |

Row 3 is the control that makes row 2 mean something: the identical source
compiles cleanly without the flag, so the flag is what rejects it, not a typo
or a missing SDK. The diagnostic is unambiguous:

```
error: 'sharedApplication' is unavailable: not available on iOS (App Extension)
       - Use view controller based solutions where appropriate instead.
UIApplication.h:87: note: property 'sharedApplication' is declared unavailable here
```

Two conclusions, both compile-time facts rather than opinions:

- **On iOS the UIKit route is closed before the program ever runs.** Not
  discouraged — it will not build.
- **On macOS nothing is closed.** `NSWorkspace` compiles fine in extension
  mode. In fact **no AppKit header carries a single `NS_EXTENSION_UNAVAILABLE`
  annotation** (`grep -rl` across the whole framework returns nothing, and
  `NSApplication.h` has zero) — AppKit simply does not use that mechanism the
  way UIKit does. So `openurl.mm` as it stands is buildable in a macOS appex.

### Measurement 2 — does the App Sandbox block `NSWorkspace`?

Compiling is not permission. An AUv3 is sandboxed, so the runtime question
stands on its own. No AUv3 is installed on this machine (`pluginkit -mv -p
com.apple.AudioUnit-UI` → *no matches*) and TIDE cannot currently build one
(**S10**: `SE_IOS_APP.xcodeproj` is dead, all four targets fail on 28 stale
`SE_DSP_CORE/` references). So this was measured with a purpose-built probe
instead: a minimal `.app`, ad-hoc signed **with `com.apple.security.app-sandbox`
and nothing else**, run against an identical unsandboxed build.

**Proof the sandbox was genuinely active**, which is the step that makes the
rest admissible — `NSHomeDirectory()` came back redirected:

```
unsandboxed: /Users/jeffmcclintock
sandboxed:   /Users/jeffmcclintock/Library/Containers/com.tidesynth.d1lsprobe/Data
```

Deliberately, the probe **opens nothing visible**. It registers a throwaway
headless URL scheme (`x-tide-donate-probe:`) handled by a background-only app
that writes a marker file and exits, so a real launch can be observed without
anything appearing on screen.

| test | unsandboxed | sandboxed |
|---|---|---|
| resolve the `https` handler (`URLForApplicationToOpenURL:`) | `/Applications/Google Chrome.app` | **identical** |
| `openURL:configuration:completionHandler:` on the probe scheme | `app=yes, err=none` | **identical** |
| handler actually launched **and received the URL** | yes | **yes** |

**A sandboxed process opens URLs exactly as an unsandboxed one does.** The App
Sandbox is not the obstacle on macOS.

**One wrinkle worth recording, because it cost an hour and looked like a
sandbox denial.** With the handler receiving URLs the *legacy* way — the
`kAEGetURL` Apple Event — the sandboxed run reported `openURL` **success** and
yet the marker never appeared: the app launched, the URL never arrived. That is
Apple Events being blocked (they need
`com.apple.security.automation.apple-events`), not URL opening being blocked.
Switching the handler to the modern `application:openURLs:` delivery — which is
what browsers actually use — made sandboxed and unsandboxed identical. **A
success return from `openURL` does not by itself prove the payload was
delivered**; anyone re-testing this should check the far end, not the return
value.

---

## What this means for each shipping format

Formats from PLAN "Target formats".

| Format | Process model | URL route | Verdict |
|---|---|---|---|
| Windows VST3 / GMPI | in-process in the host | `ShellExecuteW` | works |
| macOS VST3 / AU (v2) | in-process in the host | `NSWorkspace` | works — **no extension rules apply at all** |
| macOS AUv3 | `.appex`, sandboxed | `NSWorkspace` | compiles; sandbox permits it (see caveat) |
| iOS AUv3 | `.appex`, sandboxed | UIKit route **will not compile** | **assume closed** |
| Linux VST3 / CLAP | in-process in the host | `xdg-open` | works |

Note the macOS AU/VST3 row: those load *into the host process* and are not
extensions, so the whole question is moot for them. Only AUv3 is affected. It is
easy to over-generalise "macOS is restricted" from an iOS finding — most of what
TIDE ships on macOS is not.

---

## What is still unknown, stated precisely

Neither gap is closable on this box today, and the design below is chosen so
that **neither answer changes it**.

1. **The macOS appex sandbox profile may be stricter than plain
   `app-sandbox`.** Measurement 2 used the generic App Sandbox, because no AUv3
   exists here to test and none can be built (**S10**). An extension inherits a
   profile that is not documented as identical. *Closes when TIDE has a
   buildable macOS AUv3 target — i.e. after S10 is ruled and M2 is scoped.*
2. **iOS: whether `extensionContext.openURL` actually succeeds from an audio
   unit extension.** It **compiles** (row 4), so the gate is runtime, not
   build-time. Apple's own documentation could not be quoted here — the
   developer.apple.com pages are JavaScript-rendered and fetch to an empty
   body — but the consistent report from the AUv3 developer community is that
   `extensionContext.open(url)` is **disabled for AUv3 extensions**, with the
   permitted set being roughly Today widgets and iMessage apps. **Treat this as
   reported, not measured.** *Closes with a device or simulator test: an AUv3
   in a host, calling it and reading the completion handler's `success`.*

Because both fall the same way for the design, **do not block D1's successor on
either.** Build the copy-based affordance; add the click where a capability
check says it works.

---

## Prior art already in the tree

Two things worth knowing before designing this from scratch — neither is
mentioned in D1's row.

**A donation URL already exists, and it is already TIDE-branded:**

```cpp
// SynthEdit/SynthEditWayland/WaylandMainWindow.cpp:50
constexpr const char* kDonationUrl = "https://ko-fi.com/tiderack";
```

`ko-fi.com/tiderack` — named for TIDE Rack, not SynthEdit. It appears twice in
the Wayland build: a **"Buy me a coffee (online)"** Help-menu item
(`:276`) and, more usefully for us, **as plain text inside the About box**
(`:957`):

```cpp
+ L"\n\nBuy me a coffee: " + kDonationUrlW;
```

So the fallback recommended below — *show the URL, let the user take it* — is
not a hypothetical: SynthEdit already ships it beside the clickable version.
**What TIDE cannot copy is the container.** That About box is
`SeMessageBoxAsync(...)`, a modal dialog, which constraint 5 rules out. The
content transfers; the presentation does not.

**`browse_to` has no callers.** Grepping `SynthEdit`, `SynthEditLib`, `gmpi_ui`
and `GMPI_Wrappers` finds **zero** call sites for `gmpi::browse_to` — and
exactly one for `gmpi::open_url` outside the Wayland menu, `OpenWebPage()` at
`SynthEdit2/Application.cpp:389`. `browseto.mm` is compiled into EditorLib on
every Apple build and never called.

---

## Recommendation

**One design, five parts. Parts 1–3 are the whole thing on every platform;
parts 4–5 are enhancements that must never be load-bearing.**

1. **Nothing on the rack surface, and nothing at load time.** No button
   competing with modules, no first-run prompt, no badge. PLAN's "a user has to
   go looking for it" is a design instruction, and it is the part most likely to
   erode later — it should be written into the row that builds this, not just
   into this note.

2. **An About pane, reached from the breadcrumb bar.** The breadcrumb bar is the
   only persistent chrome constraint 1 allows, and a pane is what constraint 5
   requires instead of a dialog — the same mechanism already used for properties
   and the module browser. Donation lives in About; it does not get UI of its
   own.

3. **In that pane, the URL as selectable text plus a "Copy link" button.** This
   is the floor, and it works on every target including the most restricted one,
   with no capability check and no platform branching. **Measured
   extension-legal on both platforms** — `UIPasteboard.generalPasteboard` and
   `NSPasteboard.generalPasteboard` both compile clean under
   `-fapplication-extension` (exit 0), and `UIPasteboard.h` carries **zero**
   `NS_EXTENSION_UNAVAILABLE` annotations. A recommendation whose own fallback
   is unverified is worth very little, so this was checked rather than assumed.

4. **Where a URL can be opened, make the same line clickable.** One runtime
   capability check, not five per-platform code paths — on iOS, call
   `extensionContext.openURL` and let the completion handler's `success` decide
   whether the affordance is a link or plain text. That is the honest test and
   it costs nothing to get wrong. **Do not silently do nothing on failure**: a
   dead link is worse than visible text, which is precisely the failure mode
   part 3 exists to prevent.

5. **A QR code is a real option for iOS, and should be deferred, not
   dismissed.** It is the one route needing no platform cooperation at all, and
   an iPad user with a phone in reach can act on it — which copy-to-clipboard
   does not solve when the donation happens on a different device. It costs a QR
   encoder in the bundle. **Revisit only once unknown 2 is measured**; if the
   iOS route turns out open, this is dead weight.

**Do not use `browse_to` for any of this.** Revealing a file in Finder is a
different operation with a worse sandbox story, and it has no callers today —
reaching for it here would be its first.

---

## Follow-on items this note found

Both are **GATED** paths a scheduled run may not edit, so they are filed rather
than fixed, per STEP 5.

- **`EditorLib`'s Apple guard is not iOS-aware.**
  `SE16/EditorLib/CMakeLists.txt:161-166` adds `browseto.mm` and `openurl.mm`
  under plain `if(APPLE)` and links `-framework AppKit`. Both files
  `#import <AppKit/AppKit.h>`, and **AppKit does not exist on iOS** — so an iOS
  build of EditorLib fails to compile, before any sandbox question is reached.
  `if(APPLE)` needs to become an iOS-excluding condition, or the two files need
  to move behind a per-platform source list. **This lands on M2**, which is
  written as though the iOS target merely needs building. Filed as **D3**.
- **`browseto.mm` is compiled and never called** (zero call sites, above).
  Deleting it from the Apple source list removes an AppKit dependency and one
  sandbox-hostile API from every Apple build, TIDE's included, at no functional
  cost. Same file, same gate. Filed as **D4**.

---

## Verification artifact

Everything in Measurements 1 and 2 is reproducible from the probe sources
written for this note. They were **deliberately not committed** — they are
throwaway, and the repo is not where a one-off `.app` and a URL-scheme handler
belong. The method is fully specified above: four `clang -fsyntax-only`
invocations with and without `-fapplication-extension` (the control row is what
makes them evidence), and an ad-hoc-signed `.app` carrying only
`com.apple.security.app-sandbox`, verified active by the `NSHomeDirectory()`
container redirect before any result was read.

**Machine state:** the probe's LaunchServices registration was unregistered and
its bundle, container and marker files deleted; `x-tide-donate-probe:` no longer
resolves, and the `https` handler is unchanged. Nothing was installed
permanently.
