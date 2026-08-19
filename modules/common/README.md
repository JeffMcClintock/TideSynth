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
on, the way [PanelTestGui.cpp](../PanelTest/PanelTestGui.cpp) already caches its
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
auto pixels = bitmap.lockPixels(BitmapLockFlags::WriteOnly);
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

## Preview tool

```bash
tide_render_preview shapes --size 640 --spp 640 --out shapes.png
```

Scenes: `knob`, `materials`, `shapes`, `glass`, `glow`.

## Regression test

`ctest -R render_regression` renders every demo scene at 160px / 128spp and
compares it against a committed reference PNG. The whole set takes a few
seconds.

It pins **images** rather than numbers on purpose. Almost everything this
renderer can get wrong is invisible to a unit test and obvious to an eye: the
anisotropy convention was inverted for a while and every pdf still integrated to
one; the bounding sphere was being hit as if it were geometry and every BSDF was
still correct. Both were caught by looking.

It is not flaky. The RNG is seeded from the pixel coordinate and sample index,
never from a shared counter, so output does not depend on thread scheduling —
and `tide_render_strict_fp` opts every target out of the global `/fp:fast` so
the optimiser cannot move the images either. The tolerance that remains is for
a different compiler reordering floating point, and is far below what any real
regression produces.

When a change to a look is **intended**:

```bash
tide_render_preview --references modules/common/tests/references
```

Then *look at the new images* before committing them, and commit them alongside
the change that caused them. An approved reference is only as good as the eye
that approved it.
