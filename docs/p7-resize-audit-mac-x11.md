# P7 — the macOS and X11 resize paths, audited

BACKLOG **P7**. The Windows editor-resize crash (**P4**, diagnosed in
[p4-resize-crash.md](p4-resize-crash.md), reproduced 3/3 by **P4c**) left two
sibling paths unexamined: `DrawingFrameX11.cpp`'s `reSize(int,int)` and
`DrawingFrameMac.mm`'s `onResize()`. This is that audit, plus the ported
regression test.

Everything below was read out of the source or measured on this machine. Where
something is inferred rather than observed it says so.

## Verdict, up front

**macOS does not reproduce the crash, and cannot reproduce it by that
mechanism.** The Cocoa resize path is structurally immune to the
time-of-check/time-of-use defect for a plain reason: it has no check to go stale.
Confirmed by measurement, 3/3, on two different plugins.

**Two real but lesser defects were found on macOS**, neither of which crashes:
an unbounded extent that costs hundreds of megabytes, and a `checkSizeConstraint`
that never writes back — the same contract defect P4b fixed on Windows, still
unfixed here.

**X11's `reSize` is also structurally safe, but `present()` is not.** It caches
both an image pointer and the extents it was sized for across two re-entrant
client calls. That is the P4 shape, in a function P7 did not name. It is latent,
not proven reachable, and it belongs to the Linux box — this machine cannot build
or run X11.

Nothing was changed in `gmpi_ui`. See [What was deliberately not changed](#what-was-deliberately-not-changed).

## The two questions, answered

P7 asks two questions of each backend.

### Q1 — does anything cache a device/surface liveness check across a call that can re-enter?

| Path | Answer |
|---|---|
| macOS `DrawingFrameCocoa::onResize` ([:434](#)) | **No.** Three lines: release the bitmap if present, null the pointer. Nothing is used afterwards, so there is nothing to be stale. |
| macOS `SEVSTGUIEditorMac::onSize` ([:53](#)) | **No.** It calls `resizeNativeView` and returns. The Windows sibling's fault was the code *after* `SetWindowPos`; here there is none. |
| macOS `DrawingFrameCocoa::onRender` ([:100](#)) | **Yes — latent.** Detail below. |
| X11 `X11DrawingFrame::reSize` ([:932](#)) | **No.** It writes its fields, then calls `XResizeWindow` last. Nothing is read after, and X requests are queued rather than dispatched synchronously, so there is no re-entrancy to survive in the first place. |
| X11 `X11DrawingFrame::Impl::present` ([:1252](#)) | **Yes — latent, and worse-shaped.** Detail below. |

The macOS resize path is immune for a structural reason worth stating plainly,
because it is the reason the crash cannot be ported: **on Windows the device is
rebuilt inside the resize; on macOS it is only torn down.**
`onResize` releases the backing bitmap and stops. Reallocation happens later and
lazily, in `onRender`, from a `if(!backBuffer)` test in the same function that
then uses it. There is no window in which a checked pointer can be invalidated by
a re-entrant call, because the check and the use are adjacent.

**macOS `onRender`, the latent one.** The pattern P4 warns about does appear here:

```
:103   if(!backBuffer) { initBackingBitmap(); ... }
:110       drawingClient->arrange(&finalRect);     <-- re-entrant into client code
:113   if(!backBuffer) return;                    <-- re-checked. correct.
...
:207   drawingClient->render(...)                 <-- re-entrant into client code
:212   CGContextRestoreGState(backBuffer);        <-- NOT re-checked
:215   CGImageRef backImage = CGBitmapContextCreateImage(backBuffer);
```

The author got it right at `:113` — the check after `arrange` is exactly the
discipline `reSize` lacked — and then did not repeat it after `render`. A client
that resized its own view from inside `render` would have its backing bitmap
released at `:437` and `:212` would operate on a freed `CGContextRef`.

**Not demonstrated.** It needs a client that resizes during render, and no
current client does. Recorded because the same reasoning is what made P4 real,
and because the fix is one line in a file that already shows it knows the rule.

**X11 `present()`, the worse-shaped one.** It caches *extents* as well as a
pointer:

```
:1262  const int pw = d.width;  const int ph = d.height;
:1267  if (!d.ensureImage(pw, ph)) return;        <-- the check
:1290      d.client->measure(&avail, &desired);   <-- re-entrant
:1293      d.client->arrange(&all);               <-- re-entrant
:1303  d.client->render(...)                      <-- re-entrant
:1319  ... reinterpret_cast<uint8_t*>(d.image->data), d.image->bytes_per_line, pw, ph ...
```

`d.image` is re-read from the struct at `:1319`, so a replaced image is picked up
— but `pw`/`ph` are not. If a client callback triggered a nested `present()` at a
different size, `ensureImage` would free and reallocate the image, and the outer
call would then describe the **new, possibly smaller** buffer using the **old**
extents and hand that to `encodeDirtyRect`. That is a heap overflow, not a null
dereference.

**Also not demonstrated**, and it needs something the audit could not establish
from the source: a client callback that pumps the X event loop synchronously.
`processEvents` is driven by the host, and the frame's own `invalidateRect` only
marks a dirty region, so on current evidence no nested `present()` occurs. Filed
rather than fixed — see [Follow-ups](#follow-ups-filed).

### Q2 — is there any upper bound on the requested extent?

**Neither backend has one. Anywhere.**

| Path | Bound |
|---|---|
| macOS `SEVSTGUIEditorMac::onSize` ([:53](#)) | none — forwards the host's rect verbatim, exactly as the Windows sibling did before P4a |
| macOS `resizeNativeView` ([:788](#)) | none — assigns width/height into an `NSRect` and calls `setFrame:` |
| macOS `initBackingBitmap` ([:406](#)) | none — sizes the bitmap from the view's backing bounds |
| X11 `X11DrawingFrame::reSize` ([:932](#)) | **lower only** — `max(1, …)` on both axes, so degenerate sizes are handled and over-limit ones are not |
| X11 `ensureImage` ([:229](#)) | **lower only** — rejects `w <= 0 \|\| h <= 0` |

The Windows fix (P4a) clamps to 16384, the Direct3D 11 maximum texture
dimension. **That number has no meaning on either sibling backend**, and the
macOS consequence is not the one the Windows story would lead you to predict.

**Measured, not assumed.** `CGBitmapContextCreate` was probed directly with the
exact format `initBackingBitmap` requests (16-bit integer components, linear
sRGB, premultiplied last), and with the 32-bit float fallback:

| Requested | 16-bit | 32-bit float |
|---|---|---|
| 2178 × 32672 — the Windows crash rect | ok | ok |
| 16385 × 600 | ok | ok |
| 16384 × 16384 — the D3D11 square limit | ok | ok |
| 65536 × 600 | ok | ok |
| 200 × 200 — control | ok | ok |
| 0 × 0 | **FAIL** | **FAIL** |

Binary search: the largest working square is **131071 × 131071**, and the largest
single dimension with the other at 64 is **4194303**. CoreGraphics allocates
lazily, so these succeed as address-space reservations.

So on macOS **there is no NULL to fall back on**, which removes the whole
`if(!backBuffer) return;` safety story at large sizes. `backBuffer` is a valid
context and everything proceeds. The cost is memory: resident size climbing into
the hundreds of megabytes, and a single measured paint at 16385 × 600 costing
**+253 MiB**. The only extent CoreGraphics actually refuses is the degenerate
`0 × 0`, which `onRender` already handles at `:113`.

`checkSizeConstraint` ([SEVSTGUIEditorMac.cpp:79](#)) is the other half, and it
is the pre-P4b Windows code unchanged: it measures, returns `kResultTrue` on an
exact match and `kResultFalse` otherwise, and **never writes the rect back**.
Both branches were observed:

| Plugin | `checkSizeConstraint(0,0,2178,32672)` | rect afterwards |
|---|---|---|
| GainGui (resizable) | `kResultTrue` | 2178 × 32672, **unchanged** |
| TIDE_VST3 | `kResultFalse` | 2178 × 32672, **unchanged** |

The resizable case is the sharper one: the wrapper **affirmatively approves**
2178 × 32672. A polite host asks whether it may use that size, is told yes, and
proceeds — and there is no clamp behind the answer. P4b's reasoning ("a host that
ignores the return value then calls onSize with a number the view never agreed
to") applies here in a stronger form, because on macOS the view *did* agree.

## The regression test

`GMPI_Wrappers/tests/mac_editor_resize_host.mm`, beside the Windows one, built
with `-DGMPI_WRAPPERS_BUILD_TESTS=ON`. Same minimal-dependency approach: the
`pluginterfaces` headers plus system frameworks, no SDK hosting sources.

```
mac_editor_resize_host <plugin.vst3> [right] [bottom]
```

Exit `0` survived and was provably live, `1` setup failure, `3` inconclusive.

Two things a port of the Windows harness must get right, and both were got wrong
first here before being corrected.

### The resize allocates nothing, so onSize alone tests nothing

On Windows the crash is *inside* `onSize`, because `SetWindowPos` dispatches
`WM_SIZE` synchronously. On macOS `onSize` → `resizeNativeView` → `setFrame:` →
`onResize()` only *releases* the bitmap; the allocation at the new size happens
in the next `drawRect:`. **A harness that calls `onSize` and prints "SURVIVED"
has proved that releasing a bitmap does not crash.**

So every resize here is followed by a forced synchronous paint —
`cacheDisplayInRect:toBitmapImageRep:` — which is the only thing that drives
`onRender` → `initBackingBitmap` at the new extent. That is Windows' free gift
from `SetWindowPos`, paid for explicitly.

*(The selector is `cacheDisplayInRect:toBitmapImageRep:`. `...toBitmap:` does not
exist, compiles anyway as an unknown selector returning `id`, and throws at run
time. The compiler warns; the warning is worth reading.)*

### The Windows liveness probe does not transfer

The Windows harness proves the renderer is live by making a benign resize and
checking the window adopted it — sound there, because adoption proves `reSize`
got past its device check. **That reasoning is invalid on macOS**, where
`resizeNativeView` calls `setFrame:` unconditionally with no device check at all.
Adoption would be true with no renderer whatsoever, so a literal port reports a
false PASS.

Liveness here is therefore two independent facts, both required:

- **A** — the plugin's own `NSView` adopted the probed size (the resize path ran)
- **B** — a forced paint produced more than one distinct pixel value (the
  renderer ran and the bitmap exists — the macOS analogue of "the D2D device
  exists")

### The measurement bug this harness had, twice

Worth recording because both mistakes look like findings.

**First**, the probe sampled a single 200 × 200 rect at the view's origin. At
2178 × 32672 that reported one distinct colour, which reads as "the editor went
blank" — a tidy, wrong conclusion. The view is far larger than its 200 × 200
window, so almost all of it is clipped and never rendered; and AppKit's unflipped
origin is the *bottom*-left, which after such a resize is far below the window.
The tile was in a region that legitimately never drew. This is the same shape as
P2's Windows error of measuring the parent `HWND`: the wrong rectangle, not a
broken renderer. The probe now samples corners and centre of `[v visibleRect]`.

**Second**, even corrected, a low count at absurd extents is not evidence of
anything. Whether a sampled region contains content depends on where the client
puts it. In one run, at 16385 × 600, GainGui's tiles came back uniform while
TIDE's returned **65 distinct colours**. Distinct-colour counts at the oversized
extents are therefore printed as diagnostics and **do not gate the result**; only
the liveness probe before and the recovery probe after do.

### Results

Built and run on macOS 26.3.1, AppleClang 21, universal (x86_64 + arm64), against
`GainGui_VST3` built from local `gmpi_ui` + `GMPI_Wrappers`, and against the
existing `TIDE_VST3` Release bundle.

| Plugin | runs | oversized resize+paint pairs | result |
|---|---|---|---|
| GainGui_VST3 | 3 | 6 per run | **exit 0, survived 3/3** |
| TIDE_VST3 (Release) | 1 | 6 | **exit 0, survived** |

Both were provably live first (A and B held: 19–65 distinct colours), survived
every one of `2178 × 32672`, `0 × 0`, `1 × 1`, `16385 × 600`, `600 × 16385`, and
still drew afterwards on recovery to their original size — so the clamp-turned-
resize-into-a-no-op failure mode is excluded too.

`0 × 0` is worth calling out as the one extent CoreGraphics refuses: the bitmap
comes back NULL and `onRender`'s `:113` guard returns without drawing. That is
the safety net working, and it is the only size where it exists.

## What was deliberately not changed

**No behavioural change was made to `gmpi_ui` or to
`SEVSTGUIEditorMac.cpp`.** Only the new test file and its CMake entry were added.
Three reasons, in order of weight:

1. **P7 is an audit, and the audit's answer is "no crash".** The item asks two
   questions and asks for the test ported. There is no crash on this platform to
   stop, so nothing here justifies editing the rendering backend that every GMPI
   plugin and SynthEdit itself depend on.
2. **A clamp needs a defensible number and there isn't one yet.** Windows' 16384
   comes from a hard Direct3D limit. CoreGraphics has no comparable wall — it
   accepts 131071² — so any macOS bound would be a product decision about how
   much memory an editor may reserve, not a technical ceiling. Picking one
   silently, inside shared code, is the kind of guess the run prompt warns about.
3. **The standing direction for these repos cannot be honoured on this box
   today.** Changes to `gmpi_ui`/`GMPI_Wrappers` are supposed to be validated by
   rebuilding SynthEditCL as well as TIDE, and **P6** records that SynthEditCL
   does not build on macOS. A behavioural change to shared rendering code that
   cannot be checked against its other consumer should not be made here.

The `checkSizeConstraint` writeback is the one change with a clear precedent
(P4b) and a small diff. It is still not made here, for reason 2: the writeback is
only meaningful with a bound to write back *to*, and for the resizable client the
current code returns `kResultTrue` — the branch P4b's fix never reaches. Filed
with its evidence instead.

## Follow-ups filed

| id | what |
|---|---|
| **P7a** | macOS: bound the extent and make `checkSizeConstraint` write back. Carries the measured CG limits and the memory figures, so whoever takes it is choosing a number with evidence rather than copying 16384. **Done 2026-08-12** — 8192 points per axis and a 384 MiB bitmap budget, both set from displays rather than from a graphics API, in [gmpi_ui#3](https://github.com/JeffMcClintock/gmpi_ui/pull/3) + [GMPI_Wrappers#2](https://github.com/JeffMcClintock/GMPI_Wrappers/pull/2). The A/B reproduced this document's `+253 MiB` figure to within 0.4 MiB before halving it. |
| **P7b** | macOS: re-check `backBuffer` after `drawingClient->render()` in `onRender` ([:207](#) → [:212](#)). One line, and the same function already does it correctly at `:113`. |
| **P7c** | X11: `present()` caches `pw`/`ph` across `measure`/`arrange`/`render`. `linux` — this box cannot build or run X11. Includes what the audit could not settle: whether any client callback can pump the X loop and cause a nested `present()`. |
| **P7d** | `GMPI-plugins` cannot link a GUI plugin on macOS — see below. |

### The GMPI-plugins link break, found on the way

Building any GUI plugin there fails at link:

```
Undefined symbols for architecture x86_64:
  "_OBJC_CLASS_$_UTType", referenced from:
       in libVST3_Wrapper.a[x86_64][17](DrawingFrameMac.mm.o)
```

`gmpi_ui/backends/MacFileDialog.h:8` imports `<UniformTypeIdentifiers/…>` and
uses `UTType`; `GMPI-plugins`' link line never adds the framework.
`SynthEdit/EditorLib/CMakeLists.txt:166` does add it, which is why SynthEdit's own
macOS build is unaffected and this has stayed invisible.

Pre-existing, and **not fixed here**: `GMPI-plugins` is on neither the ALLOWED nor
the GATED list in the run prompt, so it is GATED by default. This audit worked
around it at configure time, touching no file:

```bash
-DCMAKE_MODULE_LINKER_FLAGS="-framework UniformTypeIdentifiers"
```

Filed as **P7d** with the path named, per the default-GATED rule. Note the fix
plausibly belongs in `gmpi_ui` (declaring its own framework requirement) rather
than in each consumer, which is a scope question for Jeff and the reason this is a
row rather than a commit.

## Reproducing this

```bash
cmake -S GMPI-plugins -B build-p7 -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DGMPI_SDK_FOLDER_OVERRIDE=$HOME/Documents/GitHub/GMPI \
  -DGMPI_UI_FOLDER_OVERRIDE=$HOME/Documents/GitHub/gmpi_ui \
  -DGMPI_WRAPPER_FOLDER_OVERRIDE=$HOME/Documents/GitHub/GMPI_Wrappers \
  -DGMPI_WRAPPERS_BUILD_TESTS=ON \
  -DCMAKE_MODULE_LINKER_FLAGS="-framework UniformTypeIdentifiers"

ninja -C build-p7 mac_editor_resize_host GainGui_VST3

./build-p7/GMPI_Wrappers/wrapper/gmpi_wrapper_tests/mac_editor_resize_host \
    build-p7/plugins/GainGui/GainGui_VST3.vst3
```

**Read the configure banner.** All three of "Using local GMPI folder", "Using
local GMPI-UI folder" and "Using local GMPI WRAPPERS folder" must appear. An
override that is silently absent is [building.md](building.md)'s X3 trap, and it
bit this audit — see the journal entry for 2026-08-10.

---

## Postscript — P7b done, 2026-08-13, and one correction to the table above

P7b is fixed and, unlike when this document filed it, **demonstrated**. Two
things above are wrong and are corrected here rather than in place, because the
follow-ups table is being edited by the P7a PR at the same time.

**1. The line this audit named is not the line that faults.** The P7b row above
says "re-check `backBuffer` after `drawingClient->render()` (`:207` → `:212`)",
meaning the fault would land on `CGContextRestoreGState(backBuffer)` or
`CGBitmapContextCreateImage(backBuffer)`. It does not. Both of those read the
*member*, and `onResize` sets the member to `nullptr` — so they hand
CoreGraphics a NULL, which is untidy but not a use-after-free.

The line that actually faults is one earlier: `context.popAxisAlignedClip()`.
`gmpi::cocoa::GraphicsContext` keeps its **own** copy of the pointer in
`cgContext_`, taken at `setCGContext` time, and nothing nulls that copy. So it
calls `CGContextRestoreGState` on freed memory. Measured backtrace, from the
crash report of the unfixed build:

```
CGContextRestoreGState                                   (CoreGraphics)
gmpi::cocoa::GraphicsContext::popAxisAlignedClip()
DrawingFrameCocoa::onRender(NSView*, gmpi::drawing::Rect*)
-[GMPI_VIEW_VERSION_03 drawRect:]
```

`EXC_BAD_ACCESS (SIGSEGV)`, `KERN_INVALID_ADDRESS`. The consequence for the fix
is concrete: the guard has to go **inside** the braces, immediately after
`render()` returns. A guard placed after the block closes — the obvious reading
of "`:207` → `:212`" — would sit one line too late and fix nothing.

**2. "Latent, not demonstrated" no longer holds.** It was true that no shipping
client resizes itself during render, and that is still true — but a synthetic
one reaches the path in a few lines, and this is now a regression test:
`gmpi_ui/tests/mac_render_reentrant_resize.mm`, driven by
`tests/run_mac_render_test.sh`. Unfixed sources die 3/3; fixed sources pass 3/3.

**AddressSanitizer cannot see this class of defect, which is worth knowing
before the next audit reaches for it.** The freed read happens *inside*
CoreGraphics, and ASan only checks loads the compiler instrumented plus the
functions it intercepts — a system framework is neither. Measured, with a
positive control: an ASan build of the test reports a clean PASS on the unfixed
sources, while the same binary flags a hand-written read of the same freed
pointer instantly. **Guard Malloc** (`DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib`)
is the detector that works, because it unmaps the page and the fault is taken
by whoever touches it, instrumented or not.

**P7c inherits the correction.** The X11 row's reasoning is about stale
extents, not a stale pointer, so it stands as written — but the general lesson
does transfer: when auditing one of these, check what the *callee* cached, not
only what the caller re-reads.
