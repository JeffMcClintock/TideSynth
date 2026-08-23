# TidePathTracer

A small Monte-Carlo path tracer that generates the cached bitmaps behind TIDE's
knobs and faceplates. Two source files, no dependencies beyond the C++20
standard library, ISC licensed like the rest of the repo.

It exists because the look TIDE wants — brushed aluminium that turns as the knob
turns, glass with a coloured shadow under it, plastic that is plastic rather
than a grey gradient — is *lighting*, not drawing. Faking it with gradients
means hand-tuning every highlight for every shape, and getting it wrong the
moment the shape changes.

Every output is **cached**: rendered once into a bitmap and blitted from then
on, the way [TiDEPanelGui.cpp](../TiDEPanel/TiDEPanelGui.cpp) already caches its
procedurally generated faces. So the code optimises for correctness and for
looking good, never for speed.

## Layout

| File | What it is |
| --- | --- |
| `TidePathTracer.h` | Scene description, material recipes, SDF authoring helpers |
| `TidePathTracer.cpp` | The integrator, the BSDFs, the sampler |
| `DemoScenes.h` | The demo scenes, shared by the preview tool and the test |
| `PngIo.h` | Minimal PNG read/write — **dev tooling only**, never shipped |
| `preview/` | The PNG preview tool |
| `tests/` | The image regression test and its committed references |

`tide_render` depends on nothing — not GMPI, not the SynthEdit SDK. That is
deliberate: it is meant to end up in `gmpi_ui/helpers` so every GMPI module can
use it, not just TIDE's.

## Using it

Link the library and include the header:

```cmake
target_link_libraries(MyModule PRIVATE tide_render)
```

An object is one distance function plus one material. CSG *between* objects is a
plain union; CSG *within* an object happens inside its own lambda.

```cpp
using namespace tide::render;

Scene scene;
addStudio(scene);              // room + key/fill/rim windows + a glint lamp

Object knob;
knob.material = recipes::brushedAluminium();
knob.boundsCentre = { 0, 0, 0 };
knob.boundsRadius = 0.56f;     // must CONTAIN the object — see the warning below
knob.distance = [](const Vec3& p)
{
    return sdChamferCylinder(p, 0.50f, 0.20f, 0.05f);
};
scene.add(std::move(knob));

Camera camera;                 // orthographic, front on
camera.filmWidth = 1.18f;

Settings settings;
settings.width = settings.height = 256;
settings.samplesPerPixel = 512;

const Image image = render(scene, camera, settings);
```

Then write it into a GMPI bitmap. The image is **linear, premultiplied, HDR**;
`writePixels` tone maps, encodes to sRGB and premultiplies to match whatever the
target wants:

```cpp
auto pixels = bitmap.lockPixels(BitmapLockFlags::ReadWrite);
const auto order = (pixels.channelLayout() == 1) ? PixelOrder::Rgba : PixelOrder::Bgra;
writePixels(image, pixels.getAddress(), pixels.getBytesPerRow(), order, /*premultiply*/ true);
```

### Material recipes

Say what a thing is made of rather than specifying a BSDF:

```
metals     polishedAluminium  brushedAluminium  chrome  stainlessSteel
           brass  copper  gold  titanium  anodisedBlack
plastics   glossyPlastic  satinPlastic  mattePlastic  rubber
glass      clearGlass  tintedGlass  frostedGlass
other      paint  glow
```

`recipes::brushed(material, mode, axis, origin)` turns any metal into a brushed
one. `BrushMode::Concentric` is a lathe-turned knob top; `Fixed` is a
linear-brushed faceplate.

The metal colours are measured normal-incidence reflectances in linear sRGB.
They are not free parameters — a metal's colour *is* its Fresnel curve, and
those values are what make copper read as copper rather than as orange plastic.
Change the roughness to change the finish; leave the colour alone.

### Glowing things

`recipes::glow(colour, strength)` makes an emissive object. It lights the room
properly — shadow rays are aimed at its bounding sphere — **as long as
`boundsRadius` is set and snug**. Left at the default it still glows, but only
lights things by chance and will be noisy.

## Gotchas

- **A hole bored straight through a plate will leak the background.** Rays
  that enter near the wall run parallel to it, so their step size stays tiny,
  they exhaust `kMaxMarchSteps`, and a ray out of steps is reported as a
  *miss* — which means alpha 0, so the host's background shows through as a
  ring around the hole. The fix is geometric: **taper the bore** so the wall
  recedes from the ray. Widening it behind the visible face costs nothing and
  is what a punched hole looks like anyway. Raising the step count instead
  makes every ray in the scene pay for it.
- **Never place two surfaces at exactly the same position.** Coincident faces
  give the tracer an undecidable zero and it dithers between them, which shows
  up as speckle along the seam. Overlap them or leave a deliberate gap.
- **`boundsRadius` must contain the object.** It is used as a conservative
  distance bound, so an object that pokes outside its bounds gets silently
  clipped. Too large is merely slow; too small is wrong.
- **Uniform scale only.** Sphere tracing needs the field to be Lipschitz-1, and
  non-uniform scale breaks that — the tracer steps through surfaces.
- **The studio is invisible to the camera** so panel artwork gets a transparent
  background, while still lighting the subject and appearing in its reflections.
  Demo scenes add their own visible floor when they want to show a shadow.
- **Glass gets alpha = 1 and shows the studio behind it.** Compositing glass
  over a panel means putting the panel in the scene, not behind the bitmap.
- **A flat surface seen head-on under an orthographic camera reflects exactly
  one colour.** Curvature, chamfers and brushed anisotropy are what break that
  up. This is why the knob is domed by 4% and chamfered.

## The quality ladder

`qualitySettings(Quality, width, height)` returns a complete `Settings` for one
of four rungs, so a consumer says how good rather than how:

| | what it is | knob-sized bitmap |
| --- | --- | --- |
| `Draft` | `RenderMode::Fast`: geometry only | ~3 ms |
| `Standard` | the shipped faceplate budget (128 paths, tapered, 12 bounces) | ~0.2 s |
| `High` | 4× the paths, softer clamp, roulette held back | ~1 s |
| `Ultra` | marketing: no taper, **no radiance clamp**, roulette off, Gaussian filter, bloom | minutes |

`Standard` and `High` taper their sample count by √(area) past a one-unit
panel (`samplesFor()`), which keeps big panels affordable in the product;
`Ultra` deliberately does not — a 4K frame gets the full count everywhere.
TiDEPanel's numbers moved here verbatim, so `Quality::Standard` *is* the
shipped look and every consumer of the renderer ages together.

The knobs the ladder turns are all public on `Settings` for hand-tuning:
`filter` (Box / Tent / Gaussian reconstruction, via filter importance sampling
so determinism survives), `rouletteDepth`, `clampRadiance`, and
`bloomStrength`/`bloomThreshold` (a post-pass lens halo that carries its own
alpha, so composited glows survive premultiplied output).

Renders can also be **progressive**: `renderProgressive()` accumulates sample
ranges into an existing image, and because every sample is seeded by pixel and
index, `[0,64)` then `[64,128)` is bit-identical to `[0,128)` in one call
(verified to 2e-7) — show the first batch, refine in place, pay nothing for
the interruptions.

For grading, `writePixels16` + `tide::png::write16` emit 16-bit-per-channel
PNG (`--png16` on the preview tool): same tone map and exact sRGB encode, with
the 8 extra bits that survive a curve without banding.

## Surface imperfection

`recipes::worn(material, amount)` is the single biggest thing between a
correct render and a photographic one. Real hardware is never uniform — a
physically perfect surface reads as CG long before it reads as metal, and no
amount of extra light transport fixes it, because the problem is not the light.

It adds roughness that wanders across the part and a suggestion of scuffing
along the grain, both procedural, so there is no map to author and it holds at
any resolution. Off by default everywhere, and ignored by `Fast` along with the
rest of the material model.

## Depth of field

`Camera::aperture` and `Camera::focusDistance`. Defocus does **not** require
perspective: the camera stays orthographic and the ray origins spread across
the aperture aimed at a common focus point, which is a telecentric lens — a
real thing, and a staple of product photography. Receding objects blur without
changing size. `aperture = 0` (the default) is exactly the sharp camera it
always was, bit for bit.

## Fast mode

`Settings::mode = RenderMode::Fast` (or `--fast` on the preview tool) replaces
the path tracer with one primary ray and a closed-form shade: every surface
opaque, lit from a single fixed direction. Measured on the knob scene at 448px:
**36 ms against 18.5 s — about 500× faster, at the same resolution.**

That last part is the point. Low-resolution previews are the wrong trade,
because the thing you iterate on is *geometry* — chamfer widths, grooves, bores,
seams — and those are sub-pixel exactly when you shrink the image, while the
Monte Carlo noise that hides them is worst exactly when you cut samples.
Dropping the *lighting* instead keeps every pixel and removes every sample.

**It renders the geometry, it does not approximate it.** Fast mode calls the
same marcher with the same primary ray, so the silhouette, the alpha channel and
every geometric artefact come out the same. Measured on bored plates, mean
coverage agrees with the full render to within 0.03%, and the deep-straight-bore
leak below shows up in both at the same magnitude — including the taper
recovering it.

What it drops: reflection, shadow, transmission, caustics, roughness,
anisotropy. Brushed and polished aluminium look identical. Anything about
*light* means going back to `Full`.

## Preview tool

```bash
tide_render_preview shapes --size 640 --spp 640 --out shapes.png
tide_render_preview shapes --size 640 --fast --out quick.png
tide_render_preview glass --size 1600 --quality ultra --png16 --out hero.png
```

Scenes: `knob`, `materials`, `shapes`, `glass`, `glow`.

## Tests

`ctest` runs two suites, and they catch different things.

`render_unit` is the **numeric** half: four properties the header claims in
words, checked as numbers — progressive batches compose exactly, bloom carries
its own alpha, the quality rungs are ordered, Fast and Full agree on coverage.
Fourteen checks, no reference data.

It earns its place because those failures are *invisible to the image test*.
Anchoring the stratification grid to the batch size instead of the planned
total, for example, leaves every single-call render untouched and silently
breaks every progressive consumer — tried as a negative control, and
`render_regression` passed while `render_unit` failed by 6400x its tolerance.

## Regression test

`ctest -R render_regression` renders every demo scene at 160px in **both**
modes and compares each against a committed reference PNG — `<scene>.png` and
`<scene>-fast.png`. Ten references, a few seconds; the fast half costs about
40 ms of that.

The two halves check different things. The full reference pins the **look** —
materials, lighting, caustics. The fast reference pins the **geometry**, and
pins it harder: with no light transport there is no noise for a change to hide
in, so a moved chamfer shows up cleanly instead of at the edge of the sampling
floor. A failure in `fast` alone means the shape moved; a failure in `full`
alone means the lighting or a material did.

It pins **images** rather than numbers on purpose. Almost everything this
renderer can get wrong is invisible to a unit test and obvious to an eye: the
anisotropy convention was inverted for a while and every pdf still integrated to
one; the bounding sphere was being hit as if it were geometry and every BSDF was
still correct. Both were caught by looking.

It is not flaky. The RNG is seeded from the pixel coordinate and sample index,
never from a shared counter, so output does not depend on thread scheduling —
and `tide_render_strict_fp` opts every target out of the global `/fp:fast` so
the optimiser cannot move the images either. Fast mode goes further and uses a
fixed sub-pixel grid, so it involves no RNG at all. The tolerance that remains
is for a different compiler reordering floating point, and is far below what any
real regression produces.

Keeping it that way needs one discipline: **consecutive RNG draws must be
separate statements**, never two arguments to the same call. C++ leaves argument
evaluation order unspecified, so `f(rng.next(), rng.next())` lets the compiler
decide which draw is which and the references stop being reproducible across
toolchains. This is not hypothetical — it was written that way once, and the
image test caught it the same minute.

### Two reference sets, one per platform family

`tests/references/` holds `macos/` and `windows-linux/`, and
`tide_render_regression` picks its own — hand it the root and it descends into
the set for the platform it was built for. Nothing calling it needs to know
which; CI, `ctest` and a shell all pass the same path.

Two sets and not three is a measurement, made in S27 and re-measured on Windows
in S44: Windows and Linux renders of the same commit differ from each other by
**0.083% of pixels, worst delta 10** — inside the limits with an order of
magnitude to spare — while both differ from macOS by **35–67%** of pixels at
deltas of 46–142. macOS is the outlier; Windows and Linux share a set.

When a change to a look is **intended**:

```bash
tide_render_preview --references modules/common/tests/references/macos
```

The preview tool *writes* references, so unlike the test it takes the set
explicitly — substitute `windows-linux` on Windows or Linux. **A look change has
to be re-approved in both sets**, on a machine of each family, or the platform
you did not re-bake goes red on the next CI run.

Then *look at the new images* before committing them, and commit them alongside
the change that caused them. An approved reference is only as good as the eye
that approved it.
