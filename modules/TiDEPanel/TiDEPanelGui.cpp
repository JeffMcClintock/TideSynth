// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"
#include "TidePathTracer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string_view>
#include <thread>
#include <tuple>
#include "helpers/Timer.h"
// BACKLOG E15 - the caption's raised edge is NOT APPROVED (Jeff, 2026-08-19).
// Switched off rather than deleted: the mechanism is sound and the finding
// behind it is worth keeping, but the look is not signed off. Set to 1 to
// bring it back, then tune kEdgeOffsetDips / kEdgeBlurDips / kEdgeGain.
#define TIDE_PANEL_RAISED_EDGE 0

#if TIDE_PANEL_RAISED_EDGE
// The blur behind the caption's raised edge. This is the same filter
// gmpi_ui's `cachedBlur` uses (helpers/CachedBlur.h, as seen in
// Controls/BumpGui.cpp); cachedBlur itself composites a tinted bitmap onto a
// Graphics, whereas the highlight below needs the blurred MASK to subtract
// with, so it calls one level down.
#include "helpers/GinBlur.h"
#endif

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

// The rack-module faceplate. BACKLOG E15 — this replaces `SE Rectangle XP` as
// the background behind every rack-module prefab.
//
// The panel is PATH TRACED, not drawn, and it is described by pins rather than
// hard-coded: see docs/panel-design-language.md, which this module is the
// reference implementation of. It was developed as a separate `PanelTest`
// module and moved here once it worked; the Layout pin is what replaced the
// need for a scratch module to experiment in.
//
// WHAT IS A PIN AND WHAT IS NOT. Four pins describe the panel -- Rack Units,
// Layout, Material, Panel Color -- plus two for the caption. Those say WHAT is
// on the panel and how wide it is. Everything about how it LOOKS is a compile
// time constant below, because PLAN constraint 8 ships one appearance and
// forbids user skins: tune a constant here and every rack module moves
// together, which is the point. `SE Rectangle XP` exposed the look as pins;
// deliberately not carried over.
//
// TWO CACHED BITMAPS, both because the work behind them is far too slow to
// repeat per frame.
//
//   face    — the traced render. Seconds of path tracing, so it is produced on
//             a worker and cached process-wide; see FaceRenderer.
//   caption — the rotated legend. Rendering into a bounds-sized bitmap also
//             clips it for free.
//
// ---------------------------------------------------------------------------
// TRAPS THIS FILE HAS ALREADY FALLEN INTO. Read before adding geometry.
// ---------------------------------------------------------------------------
//
// Each of these cost real debugging time, and each has bitten more than once.
// They are documented at their sites too; this is the index.
//
//  1. A RAY THAT RUNS OUT OF STEPS IS REPORTED AS A MISS, so the pixel gets
//     alpha 0 and the HOST'S BACKGROUND shows through. Sphere tracing steps by
//     the distance to the nearest surface, so a ray heading down a hole
//     parallel to its wall crawls and never reaches the far side. This is what
//     put coloured rings around every hole. Every through-cut therefore TAPERS
//     behind the front face -- see taperedCut(). If you add a deep cut with
//     parallel walls, you will reintroduce it.
//
//  2. COINCIDENT SURFACES SPECKLE. Two faces at exactly the same place give
//     the tracer an undecidable zero, and it dithers between them. It has hit
//     the jack surround (flush with the pocket floor), the switch plate (sized
//     exactly to its pocket) and the jack bore (same radius as the panel
//     hole). Never make two surfaces exactly equal: overlap them, or leave a
//     deliberate gap, and say which in a comment.
//
//  3. A FLAT SURFACE FACING AN ORTHOGRAPHIC CAMERA REFLECTS ONE COLOUR. Every
//     pixel has the same normal and the same view direction, so it renders as
//     a flat patch no matter how good the material is. Anything that has to
//     read as hardware needs a surface that sweeps through angles -- the
//     brushing grooves, the panel chamfer, the knob bevel, the jack collar
//     being a torus rather than a washer. A "black chrome ring" is this bug.
//
//  4. A SIZE THAT IS DERIVED MUST BE DERIVED EVERYWHERE. When the vents
//     changed from a fixed size to one computed from panel width, three things
//     kept using the old assumption and broke one at a time: the keep-out
//     shape, the position, and the pocket-merge veto. If a component's
//     geometry depends on the panel, check its draw, its keep-out AND its
//     placement.
//
// HOW TO TELL A RENDER BUG FROM A TRANSPARENCY BUG, since they look alike and
// three plausible "fixes" for the ring above were all wrong (turning the sky
// off, neutralising the near-black materials, adding a full backing plate):
// put a LURID background behind the panel in the host. If the artefact takes
// that colour, it is alpha and no amount of lighting or material work will
// touch it. Jeff found it in one move this way, after a lot of measurement
// that did not.
namespace
{
// --- the faceplate's look. One set of values for the whole rack. ------------
constexpr float kCornerRadius = 4.0f;

// The face is PATH TRACED (modules/common) rather than drawn. A gradient plus
// grain can imitate a photograph of brushed aluminium; it cannot imitate the
// thing that actually reads as metal, which is that the surface is a picture of
// the room around it. Simulating it also means the chamfer highlight and the
// corner shading fall where the geometry puts them instead of being hand-placed.
//
// THE ONE THING THE TRACER CANNOT DO ON ITS OWN. A flat face seen head-on under
// an orthographic camera has the same normal at every pixel, so it reflects
// exactly one colour -- and statistical anisotropy (BrushMode) does not break
// that up, because the tangent is constant too. It renders as flat grey. The
// visible grain has to be GEOMETRY, so the grooves go into the distance field
// and the normal falls out of its gradient for free.
constexpr float kPanelThickness = 0.10f;   // world units; half-extent in Z
constexpr float kChamferDips = 1.5f;       // front-edge cut, in DIPs
constexpr float kRoughness = 0.30f;
constexpr float kAnisotropy = 0.85f;

// Grooves per DIP across the grain, and their peak GRADIENT. Slope rather than
// amplitude because the brushing you see is the normal tilt, and tilt is what
// slope means -- it stays honest when the groove count changes. Keeping it well
// under 1 also keeps the displaced field close enough to Lipschitz-1 to sphere
// trace.
//
// Tuned by eye at 48 DIPs wide, and the two brush directions do NOT want the
// same number. Vertical takes 110 grooves across the panel; horizontal wants
// about half that, because an anisotropic highlight is a smear ALONG the grain
// and this panel is eight times taller than it is wide -- a horizontal smear
// runs out of panel almost immediately and the fine grain collapses into what
// looks like noise, where the same density read as crisp brushing vertically.
constexpr float kGroovesPerDip = 55.0f / 48.0f;
constexpr float kGrooveSlope = 0.22f;

// Paths per pixel, and the ceiling on trace RESOLUTION.
//
// The ceiling is on device pixels per DIP, NOT on absolute width -- that
// distinction is the whole point. A flat width cap was the original guard
// against unbounded zoom, and it was silently wrong once panel width became a
// pin: a 6-unit panel is 288 DIPs across, so capping the trace at 96 px meant
// rendering one sixth of the detail and magnifying it six times. It looked
// exactly like it was: blurry.
//
// Capping the SCALE instead means a wider panel gets proportionally more
// pixels -- it is a bigger object, so it is more work, which is honest -- while
// zoom and DPI still cannot run away with it. Zooming past this is stretched,
// which is a uniform magnification of the right picture rather than a squashed
// one, so it degrades gently.
//
// 4x is a lot of pixels and the traced image is FLOAT rgba, 16 bytes each:
//
//     1 unit  @ 4x = 192 x 1520  =  4.5 MB
//     6 units @ 4x = 1152 x 1520 = 26.7 MB
//     16units @ 4x = 3072 x 1520 = 71.2 MB
//
// which is why the cache below is bounded by PIXELS rather than by a count of
// entries: eight of the last one would be half a gigabyte.
constexpr float kMaxTraceScale = 4.0f;

// How much traced imagery is kept alive for instant re-use. Deliberately in
// pixels: entry count is meaningless when one entry can be sixteen times
// another. About 48 MB of float at this figure.
constexpr size_t kTraceCachePixelBudget = 3u * 1024u * 1024u;

// Paths per pixel, tapered for large panels so a 6-unit module does not take
// six times as long as a 1-unit one. sqrt rather than linear: noise falls as
// the square root of sample count, so halving samples on a panel with four
// times the area keeps the visible grain about level.
constexpr int kSamplesPerPixel = 128;
constexpr int kMinSamplesPerPixel = 64;
constexpr uint32_t kReferenceTracePixels = 96u * 760u; // a 1-unit panel at 2x

// The progressive preview: how much smaller it is on a side, and how few paths
// it gets. Both are deliberately crude — it exists to be replaced within a
// second or two, and being stretched over the panel hides most of its noise.
constexpr uint32_t kPreviewDivisor = 6;
constexpr int kPreviewSamples = 48;

// How often the UI thread asks whether the worker has produced anything new.
constexpr int kPollMs = 100;

// What the panel is until an image exists. Roughly the traced panel's own mean
// tone, so the swap when the first image lands is a sharpening rather than a
// jump in brightness. Given as sRGB; colorFromHex decodes to linear.
constexpr uint32_t kPlaceholderGrey = 0x8E8E92u;

// --- the physical scale ------------------------------------------------------
//
// Everything here is authored in DIPs, which only means something once DIPs are
// pinned to millimetres. The anchor is VCV RACK'S, because matching it costs
// nothing and buys interchange: Rack works at 1 px = 1/75 inch, so
//
//     1 HP  = 5.08 mm = exactly 15 px      (RACK_GRID_WIDTH)
//     3U    = 128.5 mm = 380 px            (RACK_GRID_HEIGHT)
//
// (verified against rack.hpp, not recalled). Adopting the same number makes one
// DIP one Rack pixel, so a VCV panel layout ports across unchanged and both
// convert to real Eurorack millimetres exactly.
//
// A previous draft used a round 3 DIPs/mm. That is only 1.6% away and looks
// identical, but it puts 1 HP at 15.24 DIPs -- a panel width that can never be
// a whole number of DIPs, in a system where panel widths are counted in HP.
//
// This exists so parts can be specified as the REAL part: a 3 mm LED needs a
// 3.2 mm hole, not "about five DIPs, looks right".
constexpr float kDipsPerMm = 75.0f / 25.4f; // 2.9528, == VCV Rack's px/mm
constexpr float kMm = kDipsPerMm;           // for reading sizes as "8.0f * kMm"
constexpr float kHpDips = 5.08f * kDipsPerMm; // 15.0, exactly

// One rack unit of panel width. The Rack Units pin scales the panel in these.
//
// NOT YET one HP. At 48 DIPs this is 16.26 mm, or 3.2 HP -- no real module is
// that wide. Making the pin mean HP is one line here (kHpDips), but it also
// rescales every coordinate in every Layout string AND the module's own width
// in the host, so it is one deliberate edit rather than a side effect of
// fixing the scale. See docs/ui-design-language.md, "Sizes and the grid".
constexpr float kRackUnitDips = 48.0f;

// A 3U front panel, the only height a Eurorack module has. VCV calls the same
// number RACK_GRID_HEIGHT.
//
// THE PANEL IS NOT RESIZEABLE, and that is a statement about the object rather
// than a limitation: a rack module is 3U tall and a whole number of units wide,
// so a host that stretched one to fit a box would be drawing something that
// cannot exist. measure() therefore returns this size and ignores the space it
// is offered, which is also how the wrappers detect a fixed-size editor -- they
// measure twice, against nothing and against everything, and call an editor
// resizeable only when the two answers differ.
constexpr float kRackHeightDips = 380.0f;

// The widest panel the Rack Units pin will build. A ceiling is not politeness,
// it is a resource guard: cost and memory are both linear in panel area, and
// the pin is an integer a user can type into. At the 4x trace ceiling an
// 8-unit panel is 1536 x 1520 -- 2.3 Mpx, about 37 MB of float -- which fits
// inside the trace cache's budget. A three-digit number typed by accident
// would not, and would take the host down with it.
constexpr int kMaxRackUnits = 8;

// --- hardware, all sizes in DIPs ---------------------------------------------
// WHERE things go now comes from the Layout pin; only WHAT they look like is
// compiled in, per the one-look rule (PLAN constraint 8).

// Punched vent: staggered so the holes pack hexagonally, which is what a real
// punched sheet does — more open area between the same web thickness.
// Vents span the panel rather than being a fixed size: a grill that stayed
// 5 holes wide on a 3-unit panel would sit in the middle looking lost. Both
// kinds are laid out from the panel's own width, inset by this much at each
// side, and both can still be overridden per-instance from the Layout pin.
constexpr float kVentInsetDips = 6.0f;

constexpr float kVentPitchDips = 7.5f;
constexpr float kVentHoleRadiusDips = 1.9f;
constexpr int kVentColsAuto = 0; // "fill the width", resolved when tracing
constexpr int kVentRowsDefault = 4;

// Slotted vent: stadium slots (a rounded rect whose corner radius is half its
// thickness), sharing the faceplate's own 2D rounded-rect field.
constexpr float kSlotMinHalfLenDips = 4.0f; // a floor, for absurdly narrow panels
constexpr float kSlotHalfThickDips = 1.6f;
constexpr float kSlotPitchDips = 5.5f;
constexpr int kSlotRowsDefault = 3;

// Jack sockets: a plated collar set in a black moulded body, blind bore down
// the middle.
// Sized from a real PJ301M / Thonkiconn -- the standard Eurorack 3.5 mm socket
// -- rather than by eye. The eyeballed numbers were not merely small, they were
// IMPOSSIBLE: a 2.37 mm bore inside a 2.98 mm collar hole, on a socket that has
// to accept a 3.5 mm plug. Nothing about that could have been right.
constexpr float kJackBodyMm = 8.0f;        // the black moulded body
constexpr float kJackCollarOuterMm = 6.5f; // plated ring, outside
constexpr float kJackCollarInnerMm = 4.6f; // plated ring, the hole in it

constexpr float kJackSurroundDips = 0.5f * kJackBodyMm * kMm;
constexpr float kJackOuterDips = 0.5f * kJackCollarOuterMm * kMm;
constexpr float kJackInnerDips = 0.5f * kJackCollarInnerMm * kMm;

// THE BORE IS WIDER THAN THE COLLAR'S HOLE, and that is the whole point rather
// than a rounding choice. On the real part you look through the metal ring
// straight into a void; ours had a ring of moulded plastic showing inside the
// metal, because the bore was narrower than the hole above it. That annulus is
// also what produced the blue circle: it is a glossy dielectric facing the sky,
// so it did exactly what a glossy dielectric facing a bright blue source does.
//
// Opening the bore past the collar's inner edge removes the annulus and the
// reflection with it. The overhang is what a nut screwed down over a barrel
// actually looks like. Deliberately NOT equal to the collar radius: coincident
// vertical walls are the speckling trap the jack surround and the switch plate
// both fell into.
constexpr float kJackBoreOverhangDips = 0.4f;
constexpr float kJackBoreDips = kJackInnerDips + kJackBoreOverhangDips;

// The panel is PUNCHED AROUND THE WHOLE BODY, not just bored for the barrel.
// A jack is a part pushed through a panel cut-out, so a fine dark line runs
// round the outside of the bezel where the cut edge is -- visible on the real
// socket, and absent here while the panel was merely drilled for the bore and
// the bezel sat on top of solid metal.
//
// The gap has to be small: it is a see-through slot in the plate, and it is
// only dark rather than transparent because a backing sits behind it.
constexpr float kJackPanelGapDips = 0.6f;
constexpr float kJackPanelHoleDips = kJackSurroundDips + kJackPanelGapDips;

// The bezel stands proud of the PANEL FACE, as it does in the Behringer photo
// -- the moulded body is a part pushed through from behind and done up against
// the front, so its shoulder rides above the metal. Standing proud also keeps
// its face clear of every other surface, which matters to a sphere tracer: an
// earlier flush version was EXACTLY COPLANAR with the pocket floor and
// speckled the surround with radial dashes (coincident zeros are undecidable).
constexpr float kJackBezelProudDips = 1.2f;

// The bezel's outer rim is ROUNDED, not sharp. Same load-bearing job as the
// knob bevel and the collar torus: the curve is the only part of the bezel
// that sweeps through angles, so it is what catches the key light and draws
// the bright arc around the plastic that the photo shows.
constexpr float kJackBezelRoundDips = 0.9f;

// The collar is nickel-plated hardware, not a mirror. Some roughness is both
// more realistic and much quieter to render: a near-mirror finds the small
// bright sources mostly by luck, so it speckles.
constexpr float kJackMetalRoughness = 0.14f;

// How far the collar's ring sits above the bezel's face, as a fraction of its
// own tube radius. Above ~0.5 it starts to look like a washer balanced on the
// surface rather than a nut done up against it.
constexpr float kJackProudFrac = 0.25f;

// The automatic indent. Jacks are grouped by proximity — centres within
// kJackClusterDips share one pocket — and each group gets a pocket padded out
// from the group's extent. The pocket then SHRINKS away from any other
// component it would swallow, down to the minimum pad, because a pocket is a
// jack feature: a knob half-in half-out of a pocket is a drawing error on a
// real panel too.
// THE RULE: adjacent patch points share an indent, unless doing so would
// affect another widget. Expressed as a multiple of the bezel rather than as a
// bare number, because what counts as "the next jack along" is set by how big a
// jack is. 46 DIPs was too tight and split an ordinary row of three, which is
// the shape a panel of patch points actually takes.
constexpr float kJackClusterDips = 3.5f * (2.0f * kJackSurroundDips);
constexpr float kIndentPadDips = 5.0f;
constexpr float kIndentMinPadDips = 2.0f;
constexpr float kIndentCornerDips = 6.7f;
constexpr float kIndentDepth = 0.035f;   // world units, from the front face
constexpr float kIndentFillet = 0.02f;   // radius where the pocket meets the face

// LED punch-outs: a plain hole into a dark void. The lens and its light get
// drawn over the top in vector later, like the knob pointer.
//
// Sized as the REAL hole. Eurorack uses 3 mm LEDs, whose panel cut-out is
// 3.2 mm; the old 1.6 DIP radius was a 1.07 mm hole, a third of that, which is
// why the LEDs read as specks. Change kLedDiameterMm to 5.0f for 5 mm LEDs.
constexpr float kLedDiameterMm = 3.0f;
constexpr float kLedHoleClearanceMm = 0.2f; // a part has to fit through it
constexpr float kLedHoleRadiusDips = 0.5f * (kLedDiameterMm + kLedHoleClearanceMm) * kMm;

// The switch slot: a shallow pocket holding an UNBRUSHED aluminium plate with
// a square hole punched into a dark void. The moving part gets drawn over the
// top in vector later. Isotropic against the panel's grain on purpose: the
// two read as different pieces of metal.
constexpr float kSwitchHalfWidthDips = 7.7f;
constexpr float kSwitchHalfHeightDips = 13.4f;
constexpr float kSwitchCornerDips = 2.0f;
constexpr float kSwitchDepth = 0.030f;         // world units, into the plate
constexpr float kSwitchPlateRoughness = 0.38f; // duller than the panel, and unbrushed

// The inset plate is a shade SMALLER than the pocket it sits in. Sized to match
// exactly, its walls would be coincident with the pocket's and the tracer would
// speckle the seam -- the same coincident-surface trap the jack surround fell
// into. A real inset has a seam anyway.
constexpr float kSwitchPlateGapDips = 0.4f;

// The hole is a SLOT, not a square: the lever travels up and down inside it, so
// it needs the full travel cut away. CENTRED in the pocket, with equal margin
// above and below -- an offset hole reads as a mistake rather than as a throw.
constexpr float kSwitchHoleHalfWidthDips = 4.3f;
constexpr float kSwitchHoleMarginDips = 4.6f;  // plate left above AND below the slot
constexpr float kSwitchHoleHalfHeightDips = kSwitchHalfHeightDips - kSwitchHoleMarginDips;
constexpr float kSwitchHoleCornerDips = 0.35f; // a punch leaves a slight radius

// Knobs: plain black plastic cylinders with a bevelled rim, in the two sizes
// Behringer's modules use. No flutes and no indicator -- the pointer is drawn
// over the top in vector later, so baking one into the bitmap would fight it.
//
// The bevel is doing real work, not decoration: a flat cylinder top facing an
// orthographic camera is one flat colour, exactly like the unbrushed faceplate;
// the chamfer is the only part of the knob that sweeps through angles.
constexpr float kKnobBigRadiusDips = 10.6f;
constexpr float kKnobSmallRadiusDips = 6.2f;
// Heights measured off the Behringer 112 photo, and the two sizes are NOT the
// same proportion: the big knob is squat (height about 0.6 of its diameter)
// while the small one is nearly as tall as it is wide.
constexpr float kKnobBigHeightFrac = 1.2f;   // of the knob's own radius
constexpr float kKnobSmallHeightFrac = 1.7f; // ditto
constexpr float kKnobBevelFrac = 0.18f;
constexpr float kKnobRoughness = 0.25f;

// --- the panel material, per the Material pin ---------------------------------
constexpr float kFlatRoughness = 0.34f;   // flat aluminium: like the switch plate
constexpr float kPowderRoughness = 0.45f; // powder coat: a satin dielectric skin

// DIAGNOSTIC ONLY. Swings the camera off to one side so the hardware's HEIGHT
// becomes visible: head-on, a tall knob and a flat disc are the same picture and
// only the shading tells them apart. Leave at 0 -- the panel bitmap is wrong
// while it is on.
//
// Still an ORTHOGRAPHIC camera, just rotated. tide::render has no perspective
// mode, deliberately: a perspective knob only looks right from the exact spot
// the camera was pointed, and panel art has to tile. For judging height that
// costs nothing, since an axonometric view is what a technical drawing uses for
// precisely this reason.
#define TIDE_PANEL_DIAGNOSTIC_VIEW 0

// A mirrored ball floating in front of the panel, to SEE the environment the
// faceplate is reflecting. The studio is procedural and invisible to the camera
// (that is what keeps the panel's background transparent), so without something
// specular in the scene there is no way to look at it directly.
// OFF now the studio is settled -- it was a diagnostic, not part of the panel.
// Set to 1 to put it back when the lighting next needs looking at.
#define TIDE_PANEL_MIRROR_BALL 0
#if TIDE_PANEL_MIRROR_BALL
constexpr float kBallRadius = 0.42f;      // world units; the panel is 1.0 wide
constexpr float kBallCentreYFrac = 0.45f; // of halfH, up from the middle
#endif

// --- the room -----------------------------------------------------------
//
// Diffuse ALBEDOs, so all three stay below 1. A room is only ever seen in the
// metal, and its job is to give the reflection somewhere to be: a blue cast
// from above and a warm one from below is most of what separates "outdoors
// under a sky, standing on a wooden floor" from "inside a grey box".
constexpr tide::render::Vec3 kWallColour{ 0.55f, 0.55f, 0.58f };
constexpr tide::render::Vec3 kCeilingColour{ 0.33f, 0.55f, 0.86f }; // sky

// The ceiling also EMITS, as a broad soft skylight rather than a painted
// surface. Painted, it only ever showed what the room's own lamps bounced onto
// it, which was very little -- the blue barely reached the metal.
//
// It is a RECT LIGHT under the ceiling rather than an emissive ceiling slab.
// An emissive object is importance sampled through its bounding sphere, and a
// slab spanning the whole room has an enormous one, so nearly every shadow ray
// aimed at it would miss and the image would be filthy. A rectangle is sampled
// in closed form and every sample lands.
constexpr float kSkyHalfWidth = 12.0f;  // scaled by k, like everything else
constexpr float kSkyHalfDepth = 8.0f;
constexpr tide::render::Vec3 kSkyEmission{ 0.70f, 1.15f, 2.00f };
constexpr tide::render::Vec3 kFloorColour{ 0.30f, 0.19f, 0.11f };   // mid brown

// The key is a FOUR-PANE window rather than one rectangle. A single rect smears
// into one soft band on brushed metal; four with a gap between them put a
// mullion cross into every reflection, which is what reads as a window instead
// of a softbox.
constexpr float kKeyMullion = 0.16f; // half the gap between panes, world units at k = 1

// Equipment lights: the suggestion of racked gear either side of the panel.
// ONE per side — red to the left, green to the right — and bright enough to
// actually tint the metal. Six dim ones read as specks in a reflection and
// nothing at all on the panel; two strong ones read as gear in the room.
//
// Kept fairly LARGE for their brightness. A light this intense at a pinpoint
// radius is a firefly generator: it is reached mainly by chance, so one lucky
// sample lands a spike the clamp then has to cut. Radius over emission is the
// trade that keeps them clean.
constexpr int kEquipmentPerSide = 1;
constexpr float kEquipmentRadius = 0.16f;

// Where the gear sits. NEAR THE CAMERA AXIS, which is the whole trick: a flat,
// fairly smooth metal face viewed head-on reflects what is behind the CAMERA,
// so a light out to the side at sixty degrees never lands in the reflected cone
// no matter how bright it is. Pulling it in to about sixteen degrees off axis
// is what makes the panel pick up the colour; brightness alone did not.
constexpr float kEquipmentX = 1.2f;
constexpr float kEquipmentZ = 4.2f;
constexpr tide::render::Vec3 kEquipmentRed{ 26.0f, 1.2f, 0.8f };
constexpr tide::render::Vec3 kEquipmentGreen{ 1.2f, 24.0f, 4.0f };

#if TIDE_PANEL_RAISED_EDGE
// The caption's raised-paint highlight: a light edge along the TOP and LEFT of
// every glyph, as if lit from the top-left.
//
// Both values are in DIPs and scale to device pixels, so the edge holds its
// apparent weight at any zoom instead of thinning out on a HiDPI screen.
// kEdgeOffset is how far the occluding copy of the glyph slides down-right —
// it decides WHICH edges light. kEdgeBlur is the softness of the falloff — it
// decides how much the result looks lit rather than outlined.
// TEMPORARILY HEAVY so the effect can be judged; expect to halve both once the
// look is settled.
constexpr float kGlow = 1.0f;
constexpr float kEdgeOffsetDips = 0.25f;
constexpr float kEdgeBlurDips = 1.0f;
// Scales the peak of the difference-of-blurs. The difference never reaches
// full scale on its own -- two blurs of the same shape only diverge by so much
// -- so without a gain the edge is a faint smudge. This replaced an earlier
// falloff EXPONENT, which existed only to tame a saturating highlight; the
// difference does not saturate, so it needs lifting rather than taming.
constexpr float kEdgeGain = 2.5f;
#endif

// The caption's fill is a diagonal gradient rather than a flat colour: the pin
// colour exactly at the bottom-right, lifted toward white at the top-left, so
// the letterform agrees with the same light the raised edge implies. 0 = flat
// fill, 1 = white at the top-left corner. Linear, so a small number goes a
// long way from a dark fill — 0.10 is already a clear lift.
constexpr float kTextGradientLift = 0.10f;

// createTextFormat's default body height is 12; the caption is five times that.
// Inset from the bottom edge so it starts NEAR the bottom, not on it.
constexpr float kCaptionBodyHeight = 60.0f;
constexpr float kCaptionInset = 4.0f;
}

// --- the traced faceplate ---------------------------------------------------

namespace
{
using tide::render::Vec3;

// Value noise along one axis. The grooves have to be IRREGULAR: a sum of sines
// gives a ruled grating that reads as corduroy, because real brushing has no
// period. Hash-based noise has none either.
float hash11(float p)
{
	const float f = std::sin(p * 127.1f) * 43758.5453f;
	return f - std::floor(f);
}

float vnoise1(float x)
{
	const float i = std::floor(x);
	const float f = x - i;
	const float u = f * f * (3.0f - 2.0f * f); // smoothstep: C1, so the normal has no creases
	const float a = hash11(i);
	const float b = hash11(i + 1.0f);
	return a + (b - a) * u;
}

// Grain for grooves running along ONE axis: fast variation ACROSS the grain,
// and only a slow drift ALONG it. The drift is what stops the grooves being
// dead straight, which is the difference between brushed metal and a printed
// line screen. Callers pass (across, along), so swapping the two arguments is
// what turns vertical brushing into horizontal. Returns ~-1..1.
float brushGrain(float across, float along)
{
	float g = 0.0f;
	g += 0.60f * (vnoise1(across         + along * 0.03f)         * 2.0f - 1.0f);
	g += 0.30f * (vnoise1(across * 2.17f + along * 0.05f + 11.3f) * 2.0f - 1.0f);
	g += 0.10f * (vnoise1(across * 4.31f + along * 0.02f + 27.7f) * 2.0f - 1.0f);
	return g;
}

// A faceplate: a rounded rectangle in PLAN, extruded, with the front and back
// perimeter edges CHAMFERED.
//
// Not sdChamferBox, which cuts all twelve edges at 45 degrees — that chamfers
// the plan-view corners too, giving a panel with clipped corners instead of
// rounded ones. A real milled faceplate is rounded in plan and chamfered in
// section, so the two are built separately: a 2D rounded-rect field, extruded,
// then one 45-degree plane in the (radial, z) plane to cut the front edge.
// Signed distance to a rounded rectangle in the XY plane, centred on (cx, cy).
// Shared by the faceplate and the pocket milled into it, which is the whole
// reason it is a function: the two have to agree on what "rounded rectangle"
// means or the pocket's corners will not sit parallel to the panel's.
float sdRoundRect2D(float px, float py, float cx, float cy,
	float halfW, float halfH, float cornerR)
{
	using tide::render::maxf;
	using tide::render::minf;
	using tide::render::safeSqrt;

	const float ax = std::fabs(px - cx) - (halfW - cornerR);
	const float ay = std::fabs(py - cy) - (halfH - cornerR);
	const float qx = maxf(ax, 0.0f);
	const float qy = maxf(ay, 0.0f);
	return safeSqrt(qx * qx + qy * qy) + minf(maxf(ax, ay), 0.0f) - cornerR;
}

// Every through-cut WIDENS behind the front face, and this is not styling.
//
// A ray heading down a hole exactly parallel to its wall crawls: sphere tracing
// steps by the distance to the nearest surface, that distance stays ~0 all the
// way down, and the marcher hits kMaxMarchSteps (320) before it reaches the far
// side. A ray that runs out of steps is reported as hitting NOTHING -- so the
// pixel gets alpha 0 and the host's background shows through. Not a gap in the
// geometry; the plate is solid. The ray never arrives to be stopped by it.
//
// Our own safety factor makes it worse: the panel returns d * 0.55 to stay
// conservative about the groove displacement, which shortens every step by 45%.
// The failing sliver works out about a TENTH of a pixel wide -- invisible at
// 1:1 and a fat coloured ring the moment the panel view magnifies it.
//
// Tapering the cut makes the wall RECEDE from the ray, so the step size grows
// geometrically instead of staying pinned, and the far side arrives in about a
// hundred steps. It is also what a punched hole really looks like: the
// break-out side is wider than the punch side.
// HALF what it started at, and the number is measured rather than judged. The
// taper is not free: a cone is visible from straight above where a vertical
// wall is not, so every bit of slope shows as a bright bevel ring around the
// hole -- Jeff spotted it as "a bevel on the hole the jack peeks through".
// Bevel width is slope x the depth below the visible surface, so slope is the
// only lever on it.
//
// Armed with TIDE_PANEL_ALPHA_DEBUG, on a jack and an LED at 2 and 8 units:
//
//     slope 0        1652 leaked pixels at the jack, 353 at the LED
//     slope 0.0225   0
//     slope 0.045    0
//
// So a quarter is already enough and this is double it, for margin against
// scenes we have not tried. Do not tune it below 0.0225 without re-running
// that test -- and run the zero case too, or a broken test reads as a pass.
constexpr float kThroughCutTaper = 0.045f; // about 2.5 degrees

// `d2` is the cut's cross-section, `frontZ` the panel's front face. Widening
// only BEHIND the front face leaves the visible opening exactly as authored.
float taperedCut(float d2, float pz, float frontZ)
{
	return d2 - kThroughCutTaper * tide::render::maxf(0.0f, frontZ - pz);
}

// Tapering the CUT is only half the story: it does nothing for a ray hugging
// something that STANDS IN the cut.
//
// The jack is the case. Its barrel sits inside the panel's punched hole with a
// 0.6 DIP slot between them, so the slot has two walls -- the panel's, which
// taperedCut widens with depth, and the barrel's, which was straight up and
// down. A ray hugging the barrel keeps a constant tiny distance to it however
// much room opens up on the far side, runs out of steps, and is scored as a
// miss. Alpha 0, host background through the slot.
//
// It is worth saying how this one was actually pinned down, because eyeballing
// it got the wrong answer twice: the renderer was made to paint every
// non-opaque pixel magenta, and the leak then measured itself at 11.1-12.9 DIPs
// from the jack centre. The barrel wall is at 11.81 and the collar and bore are
// at 9.60 and 7.19, so the ring could only be this wall. Guessing from a
// screenshot had blamed the collar.
//
// So the shank NARROWS behind the panel face and the slot opens from both
// sides. Nothing visible changes: the taper starts at the panel face, below the
// bezel that covers it.
//
// CLAMPED, because the taper is a slope in world units while a component's
// radius shrinks as the panel gets wider -- kJackSurroundDips/dipsWide is eight
// times smaller on an 8-unit panel than on a 1-unit one, and an unclamped
// slope would taper the barrel away to nothing. Going vertical again after the
// clamp is safe: by then the ray has real clearance, so its steps are large.
constexpr float kShankTaperMaxFrac = 0.25f;

// `radial` is the distance from the shank's axis, `r` its radius.
float taperedShank(float radial, float r, float pz, float frontZ)
{
	const float shrink = tide::render::minf(
		kThroughCutTaper * tide::render::maxf(0.0f, frontZ - pz),
		kShankTaperMaxFrac * r);
	return radial - (r - shrink);
}

// Sweeps a 2D field along Z into a slab. Standard extrusion: the two distances
// combine as a right triangle outside the solid and as the nearer face inside.
float extrudeZ(float d2, float dz)
{
	using tide::render::maxf;
	using tide::render::minf;
	using tide::render::safeSqrt;

	const float a = maxf(d2, 0.0f);
	const float b = maxf(dz, 0.0f);
	return minf(maxf(d2, dz), 0.0f) + safeSqrt(a * a + b * b);
}

float sdFaceplate(const Vec3& p, float halfW, float halfH, float halfZ,
	float cornerR, float chamfer)
{
	using tide::render::maxf;

	const float d2 = sdRoundRect2D(p.x, p.y, 0.0f, 0.0f, halfW, halfH, cornerR);
	const float dz = std::fabs(p.z) - halfZ;
	const float d = extrudeZ(d2, dz);

	constexpr float invSqrt2 = 0.70710678f;
	return maxf(d, (d2 + std::fabs(p.z) - halfZ + chamfer) * invSqrt2);
}

// The room and its lights, built here rather than by tide::render::addStudio.
//
// LOCAL ON PURPOSE. addStudio paints the whole room ONE colour — a Material
// carries a single albedo, and the room is a single object, so it cannot be
// brown at the bottom and blue at the top by itself — and it gives the key one
// pane. Its look is also pinned by committed reference images for every demo
// scene, so changing it there would mean re-approving all of them. Building the
// environment here keeps those green while PanelTest experiments; if this look
// is adopted it can move into addStudio and the references regenerate with it.
//
// `k` scales the whole rig with the subject. Uniform scale leaves every
// source's solid angle unchanged, so the exposure holds and only framing moves.
void addPanelStudio(tide::render::Scene& scene, float k)
{
	using namespace tide::render;

	const Vec3 aim{ 0.0f, 0.0f, 0.0f };

	// Twice as wide, in X ONLY.
	//
	// Depth deliberately left alone. The lights are fixed in size and position,
	// so total flux is fixed: enlarge the room and the same light spreads over
	// more wall, the walls get dimmer, and the BOUNCE that fills the panel's
	// midtones drops with them. Doubling x and z together did exactly that --
	// four times the wall area, and the panel came out 35% darker, which is not
	// what "less cramped" is supposed to cost. The back wall is the one facing
	// the panel and contributes most of that bounce, so it stays put and only
	// the side walls retreat. Height is unchanged too: the sky and the floor are
	// what give the metal its blue-above / warm-below.
	const Vec3 half{ 20.0f * k, 16.0f * k, 10.0f * k };

	// The frame a rectangle standing at `position` and facing the subject sits
	// in. Split out from aimedRect because the window panes must SHARE one
	// frame: deriving it per pane aims each one separately at the subject, so
	// four panes meant to be coplanar fan out instead. At this rig's distances
	// that is about twenty degrees of splay -- plainly visible in a reflection.
	struct Frame { Vec3 right, up, forward; };
	const auto frameAt = [&](const Vec3& position)
	{
		const Vec3 forward = normalize(aim - position);
		const Vec3 reference = (std::fabs(forward.y) > 0.95f)
			? Vec3{ 0.0f, 0.0f, 1.0f } : Vec3{ 0.0f, 1.0f, 0.0f };
		const Vec3 right = normalize(cross(forward, reference));
		return Frame{ right, cross(right, forward), forward };
	};

	// A rectangle at `position` lying in an explicit frame.
	const auto framedRect = [&](const Vec3& position, const Frame& frame,
		float halfWidth, float halfHeight, const Vec3& emission)
	{
		Light l;
		l.shape = LightShape::Rect;
		l.position = position;

		l.halfU = frame.right * halfWidth;
		l.halfV = frame.up * halfHeight;
		// cross(halfU, halfV) must point AT the subject: the light is one-sided.
		if (dot(cross(l.halfU, l.halfV), frame.forward) < 0.0f)
			l.halfV = -l.halfV;

		l.emission = emission;
		l.twoSided = false;
		l.cameraVisible = false;
		return l;
	};

	// A single rectangle, aimed at the subject from where it stands. Aiming here
	// rather than at the call site because a softbox that is not pointed at the
	// subject contributes nothing but bounce.
	const auto aimedRect = [&](const Vec3& position, float halfWidth, float halfHeight,
		const Vec3& emission)
	{
		return framedRect(position, frameAt(position), halfWidth, halfHeight, emission);
	};

	// Walls.
	{
		Object room;
		room.material.kind = MaterialKind::Diffuse;
		room.material.colour = kWallColour;
		room.material.cameraVisible = false;
		room.unbounded = true;
		room.distance = [half](const Vec3& p) { return sdRoomInterior(p, half); };
		scene.add(std::move(room));
	}

	// Floor and ceiling: slabs laid against the inside of the room, since the
	// room itself can only be one colour. Each is thin and sits flush with the
	// face it covers, so it reads as that face being painted rather than as a
	// separate object floating in the room.
	const auto addSlab = [&](float insideY, const Vec3& colour)
	{
		const float t = 0.05f * k;
		const Vec3 centre{ 0.0f, insideY, 0.0f };

		Object slab;
		slab.material.kind = MaterialKind::Diffuse;
		slab.material.colour = colour;
		slab.material.cameraVisible = false;
		slab.unbounded = true;
		slab.distance = [half, centre, t](const Vec3& p)
		{
			return sdBox(p - centre, Vec3{ half.x, t, half.z });
		};
		scene.add(std::move(slab));
	};
	addSlab(-half.y + 0.05f * k, kFloorColour);
	addSlab(half.y - 0.05f * k, kCeilingColour);

	// KEY, as a four-pane window.
	{
		const Vec3 pos{ -5.2f * k, 4.4f * k, 4.8f * k };
		const float halfWidth = 3.8f * k;
		const float halfHeight = 6.5f * k;
		const float mullion = kKeyMullion * k;

		// Two panes plus two gaps span the window: 4*pane + 2*mullion = 2*half.
		const float paneW = (halfWidth - mullion) * 0.5f;
		const float paneH = (halfHeight - mullion) * 0.5f;

		// ONE frame, taken at the window's centre and reused by every pane, so
		// the four stay coplanar and square to each other -- a window, not four
		// softboxes pointed at the same spot.
		const Frame frame = frameAt(pos);

		for (int pane = 0; pane < 4; ++pane)
		{
			const float sx = (pane & 1) ? 1.0f : -1.0f;
			const float sy = (pane & 2) ? 1.0f : -1.0f;
			const Vec3 centre = pos
				+ frame.right * (sx * (paneW + mullion))
				+ frame.up * (sy * (paneH + mullion));

			scene.add(framedRect(centre, frame, paneW, paneH, Vec3{ 5.4f, 5.6f, 6.2f }));
		}
	}

	// SKY: a broad, dim, blue source directly overhead. Aimed at the subject
	// like the rest, which for something straight above resolves to pointing
	// straight down.
	scene.add(aimedRect({ 0.0f, 14.0f * k, 0.0f },
		kSkyHalfWidth * k, kSkyHalfDepth * k, kSkyEmission));

	// FILL: dimmer, warmer, opposite the key.
	scene.add(aimedRect({ 5.6f * k, -2.4f * k, 3.8f * k }, 3.2f * k, 5.5f * k,
		Vec3{ 1.45f, 1.30f, 1.10f }));

	// RIM: a narrow bright strip behind and above.
	scene.add(aimedRect({ 0.6f * k, 5.2f * k, -3.4f * k }, 2.0f * k, 0.5f * k,
		Vec3{ 6.5f, 6.6f, 7.0f }));

	// GLINT: small and intense, for the hard sparkle in the chamfer.
	{
		Light glint;
		glint.shape = LightShape::Sphere;
		glint.position = { 2.2f * k, 3.6f * k, 3.4f * k };
		glint.radius = 0.45f * k;
		glint.emission = { 26.0f, 26.0f, 27.0f };
		glint.cameraVisible = false;
		scene.add(glint);
	}

	// Equipment: a stack of coloured indicators down each side.
	for (int side = 0; side < 2; ++side)
	{
		const float sx = side ? 1.0f : -1.0f;
		for (int i = 0; i < kEquipmentPerSide; ++i)
		{
			// Spread up the panel, and staggered in depth so the two sides do
			// not mirror each other exactly. A lone light per side sits at
			// mid-height rather than at an end of that spread.
			const float t = (kEquipmentPerSide > 1)
				? (float)i / (float)(kEquipmentPerSide - 1) : 0.5f;

			Light led;
			led.shape = LightShape::Sphere;
			led.position = {
				sx * kEquipmentX * k,
				(-1.2f + 2.4f * t) * k,
				(kEquipmentZ + 0.4f * i) * k
			};
			led.radius = kEquipmentRadius * k;
			led.emission = ((i + side) & 1) ? kEquipmentGreen : kEquipmentRed;
			led.cameraVisible = false;
			scene.add(led);
		}
	}
}

// --- the layout specification -------------------------------------------------
//
// The Layout pin holds a tiny textual spec: one component per statement,
// statements separated by newlines OR semicolons — semicolons because XML
// attribute-value normalisation turns newlines into spaces, so a default
// carrying real newlines would arrive as one long line. '#' starts a comment.
//
//   knob [big|small] X Y
//   jack X Y
//   switch X Y
//   grill X Y [COLS ROWS]
//   slots X Y [ROWS]
//   led X Y
//
// X and Y are DIPs from the panel's TOP-LEFT, y downward — GUI coordinates, so
// the vector overlays drawn later (knob pointers, switch levers, LED lenses)
// can reuse the same numbers unchanged. The panel is RackUnits x 48 DIPs wide.
// They are SNAPPED to a half-DIP grid on the way in; see kWidgetGridDips.
//
// Unparseable statements are SKIPPED, never errors: this text is edited live
// in a pin, and a half-typed line must not blank the panel.

// EVERY COMPONENT CENTRE LANDS ON A HALF-DIP GRID, and that is an interface
// constraint, not a tidiness one.
//
// The painted panel is only half of a control. The other half is a vector
// widget sitting on top of it -- the knob's moving line, the LED's lens, the
// socket's hit target -- and those are separate host modules. SynthEdit stores
// a module's panelRect as integers: integer position, integer width, integer
// height. So a widget's centre is position + size/2, which can only ever be a
// multiple of 0.5 DIPs. Nothing the patch author does can change that.
//
// If a painted knob's centre is anywhere else, there is NO placement of the
// widget that lines up with it. The author cannot dial the error out; they can
// only get within a quarter DIP and stay wrong, which is exactly the "difficult
// to get it correct" that prompted this. So the painted side gives up a freedom
// it does not need: a quarter DIP of movement is invisible on a faceplate,
// while a quarter DIP of misregistration between a knob and the pointer drawn
// over it is not.
//
// Applied to every kind, including the vents that have no widget and do not
// need it. A rule with no exceptions cannot be applied to the wrong list later.
// (The auto-centred vents are unaffected in practice: the panel is a whole
// number of 48-DIP units, so its centre is already a whole DIP.)
constexpr float kWidgetGridDips = 0.5f;

float snapToWidgetGrid(float dips)
{
	return std::round(dips / kWidgetGridDips) * kWidgetGridDips;
}
struct PanelComponent
{
	enum class Kind : uint8_t { Knob, Jack, Switch, Grill, Slots, Led };
	Kind kind{};
	float x = 0.0f, y = 0.0f; // DIPs from panel top-left
	bool big = false;         // knobs only
	int cols = 0, rows = 0;   // grill / slots only
};

struct PanelSpec
{
	int units = 1;
	int material = 0;               // Material pin: 0 brushed, 1 flat, 2 powder
	Vec3 colour{ 0.5f, 0.5f, 0.5f };// powder coat albedo, LINEAR
	std::vector<PanelComponent> components;
};

// strtof via a bounded copy rather than std::from_chars<float>, which is still
// missing from some of the standard libraries this file has to build under.
bool parseFloatToken(std::string_view t, float& out)
{
	char buf[32];
	const size_t n = (std::min)(t.size(), sizeof(buf) - 1);
	std::memcpy(buf, t.data(), n);
	buf[n] = 0;
	char* end = nullptr;
	out = std::strtof(buf, &end);
	return end != buf;
}

std::vector<PanelComponent> parsePanelLayout(const std::string& text)
{
	std::vector<PanelComponent> out;

	size_t pos = 0;
	while (pos <= text.size())
	{
		size_t end = text.find_first_of(";\n\r", pos);
		if (end == std::string::npos)
			end = text.size();
		std::string_view line(text.data() + pos, end - pos);
		pos = end + 1;

		if (const auto hash = line.find('#'); hash != std::string_view::npos)
			line = line.substr(0, hash);

		// whitespace-split
		std::vector<std::string_view> tok;
		size_t i = 0;
		while (i < line.size())
		{
			while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
			size_t j = i;
			while (j < line.size() && line[j] != ' ' && line[j] != '\t') ++j;
			if (j > i)
				tok.push_back(line.substr(i, j - i));
			i = j;
		}
		if (tok.empty())
			continue;

		PanelComponent c{};
		size_t coordAt = 1;
		if (tok[0] == "knob")
		{
			c.kind = PanelComponent::Kind::Knob;
			// Size keyword optional; a bare "knob X Y" is a small one.
			if (tok.size() > 1 && (tok[1] == "big" || tok[1] == "small"))
			{
				c.big = (tok[1] == "big");
				coordAt = 2;
			}
		}
		else if (tok[0] == "jack")   c.kind = PanelComponent::Kind::Jack;
		else if (tok[0] == "switch") c.kind = PanelComponent::Kind::Switch;
		else if (tok[0] == "grill")  { c.kind = PanelComponent::Kind::Grill; c.cols = kVentColsAuto; c.rows = kVentRowsDefault; }
		else if (tok[0] == "slots")  { c.kind = PanelComponent::Kind::Slots; c.rows = kSlotRowsDefault; }
		else if (tok[0] == "led")    c.kind = PanelComponent::Kind::Led;
		else
			continue;

		if (tok.size() < coordAt + 2)
			continue;
		if (!parseFloatToken(tok[coordAt], c.x) || !parseFloatToken(tok[coordAt + 1], c.y))
			continue;

		// Snapped HERE, once, at the only point a centre enters the system.
		// Everything downstream -- the keep-out shapes, the jack pocket
		// grouping, the scene geometry -- reads c.x/c.y, so it all agrees by
		// construction. Snapping at the drawing end instead would put the
		// painted hole and the pocket that surrounds it on different grids.
		c.x = snapToWidgetGrid(c.x);
		c.y = snapToWidgetGrid(c.y);

		float v = 0.0f;
		if (c.kind == PanelComponent::Kind::Grill && tok.size() >= coordAt + 4)
		{
			if (parseFloatToken(tok[coordAt + 2], v)) c.cols = (std::max)(1, (int)v);
			if (parseFloatToken(tok[coordAt + 3], v)) c.rows = (std::max)(1, (int)v);
		}
		if (c.kind == PanelComponent::Kind::Slots && tok.size() >= coordAt + 3)
		{
			if (parseFloatToken(tok[coordAt + 2], v)) c.rows = (std::max)(1, (int)v);
		}

		out.push_back(c);
	}
	return out;
}

// How wide the vents come out on a panel of `dipsWide`. Both are derived
// rather than stored, so the same Layout string describes a 1-unit and a
// 6-unit module without editing.
int ventColsFor(float dipsWide, int specified)
{
	if (specified > 0)
		return specified;
	const float usable = dipsWide - 2.0f * kVentInsetDips;
	return (std::max)(1, (int)std::floor(usable / kVentPitchDips));
}

float slotHalfLenFor(float dipsWide)
{
	return (std::max)(kSlotMinHalfLenDips, 0.5f * dipsWide - kVentInsetDips);
}

// Whether this vent sizes itself to the panel. Slots always do; a grill does
// unless the Layout line gave it an explicit column count.
bool ventIsAutoWidth(const PanelComponent& c)
{
	return c.kind == PanelComponent::Kind::Slots
		|| (c.kind == PanelComponent::Kind::Grill && c.cols <= 0);
}

// Where a vent actually sits horizontally.
//
// An auto-width vent is as wide as the panel, so its centre IS the panel's
// centre -- honouring the X from the Layout line would hang a full-width bank
// off one edge, which is exactly what it did: a layout written for a 1-unit
// panel put X at 24, and on a 6-unit panel that pushed both vents so far left
// that they ran from the edge to about 60% across and looked like they had
// simply failed to reach. The X still counts for a grill with explicit
// columns, which is the case where the author has said how wide it should be.
float ventCentreXDips(const PanelComponent& c, float dipsWide)
{
	return ventIsAutoWidth(c) ? 0.5f * dipsWide : c.x;
}

// --- the automatic indent ------------------------------------------------------

struct DipRect { float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f; };

// The keep-out footprint of a non-jack component, as a circle. Used only to
// push pocket edges back, so generous-and-round beats tight-and-exact.
// The area a non-jack widget wants left alone, as a RECTANGLE.
//
// It was a circle of the half-diagonal, which is fine for a knob and badly
// wrong for anything long and thin. Once vents started spanning the panel, a
// 276-DIP-wide slot bank claimed a circle of radius 140 -- most of the lower
// panel -- and silently vetoed every pocket merge near it. A widget's keep-out
// has to be the shape of the widget.
//
// `has` is false for kinds that reserve nothing.
struct KeepOut { DipRect rect; bool has = false; };

KeepOut componentKeepOut(const PanelComponent& c, float dipsWide)
{
	// Vents use their EFFECTIVE centre, not the one in the Layout line -- a
	// keep-out in the wrong place is worse than none, since it vetoes pocket
	// merges over empty panel.
	const float cx = (c.kind == PanelComponent::Kind::Grill
		|| c.kind == PanelComponent::Kind::Slots)
		? ventCentreXDips(c, dipsWide) : c.x;

	const auto box = [&c, cx](float halfW, float halfH, float margin)
	{
		return KeepOut{ { cx - halfW - margin, c.y - halfH - margin,
			cx + halfW + margin, c.y + halfH + margin }, true };
	};

	switch (c.kind)
	{
	case PanelComponent::Kind::Knob:
	{
		const float r = c.big ? kKnobBigRadiusDips : kKnobSmallRadiusDips;
		return box(r, r, 1.5f);
	}
	case PanelComponent::Kind::Switch:
		return box(kSwitchHalfWidthDips, kSwitchHalfHeightDips, 1.5f);
	case PanelComponent::Kind::Grill:
		return box(0.5f * kVentPitchDips * (float)ventColsFor(dipsWide, c.cols),
			0.5f * kVentPitchDips * 0.866f * (float)c.rows, 2.0f);
	case PanelComponent::Kind::Slots:
		return box(slotHalfLenFor(dipsWide),
			0.5f * kSlotPitchDips * (float)c.rows, 2.0f);
	case PanelComponent::Kind::Led:
		return box(kLedHoleRadiusDips, kLedHoleRadiusDips, 2.0f);
	default:
		return {};
	}
}

// Shrink `rect` just enough to clear the circle, never past `minRect`, by
// moving the ONE edge that costs the least area. One edge rather than all
// four: the intruder sits on one side of the jacks, and that is the side the
// pocket should give up.
bool rectsOverlap(const DipRect& a, const DipRect& b)
{
	return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1;
}

// Shrink `rect` just enough to clear `keep`, never past `minRect`, by moving
// the ONE edge that costs the least area. One edge rather than all four: the
// intruder sits on one side of the jacks, and that is the side the pocket
// should give up.
void shrinkPocketClear(DipRect& rect, const DipRect& minRect, const DipRect& keep)
{
	if (!rectsOverlap(rect, keep))
		return;

	const float w = rect.x1 - rect.x0;
	const float h = rect.y1 - rect.y0;

	int bestEdge = -1;
	float bestPos = 0.0f, bestCost = 0.0f;
	const auto consider = [&](int edge, float pos, bool valid, float cost)
	{
		if (valid && cost >= 0.0f && (bestEdge < 0 || cost < bestCost))
		{
			bestEdge = edge;
			bestPos = pos;
			bestCost = cost;
		}
	};
	consider(0, keep.x0, keep.x0 >= minRect.x1, (rect.x1 - keep.x0) * h); // right edge in
	consider(1, keep.x1, keep.x1 <= minRect.x0, (keep.x1 - rect.x0) * h); // left edge in
	consider(2, keep.y0, keep.y0 >= minRect.y1, (rect.y1 - keep.y0) * w); // bottom edge up
	consider(3, keep.y1, keep.y1 <= minRect.y0, (keep.y1 - rect.y0) * w); // top edge down

	switch (bestEdge)
	{
	case 0: rect.x1 = bestPos; break;
	case 1: rect.x0 = bestPos; break;
	case 2: rect.y1 = bestPos; break;
	case 3: rect.y0 = bestPos; break;
	default: break; // boxed in on every side: leave it; the layout is the error
	}
}

// Jacks grouped by proximity (single linkage), one pocket per group, each
// pocket shrunk away from anything that is not a jack.
DipRect padRect(const DipRect& b, float pad)
{
	return { b.x0 - pad, b.y0 - pad, b.x1 + pad, b.y1 + pad };
}

// Pull `rect` back off every non-jack widget, never past `floorRect`, and
// report whether that succeeded. Shrinking FIRST and vetoing only if it fails
// is the difference between "these two jacks cannot share a pocket because a
// knob is near" and "they share a slightly tighter one" -- the pocket giving
// ground is not the other widget being affected.
bool clearOfOtherWidgets(DipRect& rect, const DipRect& floorRect,
	const std::vector<PanelComponent>& comps, float dipsWide)
{
	for (const auto& c : comps)
	{
		if (c.kind == PanelComponent::Kind::Jack)
			continue;
		const KeepOut k = componentKeepOut(c, dipsWide);
		if (k.has)
			shrinkPocketClear(rect, floorRect, k.rect);
	}

	for (const auto& c : comps)
	{
		if (c.kind == PanelComponent::Kind::Jack)
			continue;
		const KeepOut k = componentKeepOut(c, dipsWide);
		if (k.has && rectsOverlap(rect, k.rect))
			return false;
	}
	return true;
}

// One pocket per group of adjacent patch points.
//
// Agglomerative with a VETO rather than plain clustering: every jack starts in
// its own pocket, and two pockets merge only if the combined one can be made
// to clear every other widget. A single pass of "cluster then fix up" cannot
// express that -- it has already committed to the grouping by the time it
// discovers a knob in the way.
std::vector<DipRect> computeJackPockets(const std::vector<PanelComponent>& comps,
	float dipsWide)
{
	constexpr float pad = kJackSurroundDips + kIndentPadDips;
	constexpr float minPad = kJackSurroundDips + kIndentMinPadDips;

	std::vector<std::vector<size_t>> groups;
	for (size_t i = 0; i < comps.size(); ++i)
		if (comps[i].kind == PanelComponent::Kind::Jack)
			groups.push_back({ i });
	if (groups.empty())
		return {};

	const auto bboxOf = [&comps](const std::vector<size_t>& g)
	{
		DipRect b{ 1.0e9f, 1.0e9f, -1.0e9f, -1.0e9f };
		for (const size_t i : g)
		{
			b.x0 = (std::min)(b.x0, comps[i].x);
			b.y0 = (std::min)(b.y0, comps[i].y);
			b.x1 = (std::max)(b.x1, comps[i].x);
			b.y1 = (std::max)(b.y1, comps[i].y);
		}
		return b;
	};

	// Adjacent if ANY member of one is within reach of any member of the other,
	// so a row of jacks chains along rather than needing every pair to be close.
	const auto adjacent = [&comps](const std::vector<size_t>& ga,
		const std::vector<size_t>& gb)
	{
		for (const size_t a : ga)
			for (const size_t b : gb)
			{
				const float dx = comps[a].x - comps[b].x;
				const float dy = comps[a].y - comps[b].y;
				if (dx * dx + dy * dy <= kJackClusterDips * kJackClusterDips)
					return true;
			}
		return false;
	};

	for (bool merged = true; merged; )
	{
		merged = false;
		for (size_t a = 0; a < groups.size() && !merged; ++a)
		{
			for (size_t b = a + 1; b < groups.size() && !merged; ++b)
			{
				if (!adjacent(groups[a], groups[b]))
					continue;

				std::vector<size_t> combined = groups[a];
				combined.insert(combined.end(), groups[b].begin(), groups[b].end());

				const DipRect bbox = bboxOf(combined);
				DipRect candidate = padRect(bbox, pad);
				if (!clearOfOtherWidgets(candidate, padRect(bbox, minPad), comps, dipsWide))
					continue; // merging here WOULD affect another widget: leave them apart

				groups[a] = std::move(combined);
				groups.erase(groups.begin() + b);
				merged = true;
			}
		}
	}

	std::vector<DipRect> pockets;
	pockets.reserve(groups.size());
	for (const auto& g : groups)
	{
		const DipRect bbox = bboxOf(g);
		DipRect pocket = padRect(bbox, pad);
		clearOfOtherWidgets(pocket, padRect(bbox, minPad), comps, dipsWide);
		pockets.push_back(pocket);
	}
	return pockets;
}

// --- the cutter fields -----------------------------------------------------

// The grill's holes, as one infinite-in-Z cylinder per hole, to be SUBTRACTED
// from the plate so they go right through it.
//
// LIMITED REPETITION rather than a loop. Evaluating two dozen holes at every
// sphere-tracing step would dominate the render; folding the point into a
// single cell and CLAMPING the cell index costs the same as one hole and still
// yields a finite grid. Clamping is also why there are no half-holes: masking
// an infinite hole field with a rectangle would slice through whichever holes
// straddled the edge, which is exactly what a punched sheet never looks like.
float sdVentHoles(const Vec3& p, float centreX, float centreY, float pitchX,
	float pitchY, float radius, int cols, int rows)
{
	using tide::render::clampf;
	using tide::render::safeSqrt;

	const float yTop = centreY + 0.5f * pitchY * (float)rows;

	const float rowF = std::floor((yTop - p.y) / pitchY);
	const float row = clampf(rowF, 0.0f, (float)(rows - 1));
	const float cy = (yTop - p.y) - (row + 0.5f) * pitchY;

	// Alternate rows step half a pitch across and carry one hole fewer, which
	// is what turns a square grid into a staggered one.
	const bool odd = ((int)row & 1) != 0;
	const float offset = odd ? pitchX * 0.5f : 0.0f;
	const int rowCols = odd ? cols - 1 : cols;

	const float u = (p.x - centreX) + 0.5f * pitchX * (float)cols - offset;
	const float colF = std::floor(u / pitchX);
	const float col = clampf(colF, 0.0f, (float)(rowCols - 1));
	const float cx = u - (col + 0.5f) * pitchX;

	return safeSqrt(cx * cx + cy * cy) - radius;
}

// The slotted vent's cutter: a stack of horizontal stadiums, limited-repeated
// down y exactly as the punched grill is across x and y.
float sdSlotVents(const Vec3& p, float centreX, float centreY, float pitchY,
	float halfLen, float halfThick, int rows)
{
	using tide::render::clampf;

	const float yTop = centreY + 0.5f * pitchY * (float)rows;
	const float rowF = std::floor((yTop - p.y) / pitchY);
	const float row = clampf(rowF, 0.0f, (float)(rows - 1));
	const float cy = (yTop - p.y) - (row + 0.5f) * pitchY;

	// cornerR == halfThick is what turns the rectangle into a stadium.
	return sdRoundRect2D(p.x, cy, centreX, 0.0f, halfLen, halfThick, halfThick);
}

// The solid that gets SUBTRACTED to leave a pocket: a rounded rectangle in
// plan, running from the floor depth forward and out of the panel entirely.
//
// Open-ended forward on purpose. A closed box would need its front face placed
// above the panel surface and its rounding would then fillet that face too,
// making the "pocket" a dish; cutting with a half-space instead leaves a FLAT
// floor, and the only fillet is the one opSmoothSubtract puts where the pocket
// meets the face -- which is the edge you actually see.
float sdIndentTool(const Vec3& p, float centreX, float centreY, float halfW,
	float halfH, float cornerR, float zFloor)
{
	const float d2 = sdRoundRect2D(p.x, p.y, centreX, centreY, halfW, halfH, cornerR);
	return tide::render::maxf(d2, zFloor - p.z);
}

// --- the spec-driven trace ---------------------------------------------------

tide::render::Image traceFaceplate(uint32_t pixelWidth, uint32_t pixelHeight,
	const PanelSpec& spec, int samplesPerPixel)
{
	using namespace tide::render;

	// The spec decides the panel's width in DIPs; the pin, not the on-screen
	// size, because the module can be zoomed or resized without the hardware
	// changing physical size.
	const float dipsWide = kRackUnitDips * (float)(std::max)(1, spec.units);

	const float halfW = 0.5f;

	// From the SPEC, not from the pixel buffer's aspect. Those agree now that
	// the panel is fixed-size, and deriving it from the spec is what keeps them
	// agreeing: geometry that reads its proportions off whatever rectangle it
	// was handed will happily render a squashed panel and look like it worked.
	const float halfH = 0.5f * kRackHeightDips / dipsWide;
	const float halfZ = kPanelThickness;

	// DIP space -> world space. The panel is dipsWide DIPs and exactly 1.0
	// world units across. DIP y runs DOWN from the top-left (GUI space);
	// world y runs UP from the centre.
	const auto toWorldX = [dipsWide](float xDips) { return xDips / dipsWide - 0.5f; };
	const auto toWorldY = [dipsWide, halfH](float yDips) { return halfH - yDips / dipsWide; };

	Scene scene;

	// The rig is tuned around a 1:8 panel and scales uniformly with the subject.
	const float k = maxf(0.25f, halfH / 4.0f);
	addPanelStudio(scene, k);

	const float cornerR = kCornerRadius / dipsWide;
	const float chamfer = kChamferDips / dipsWide;
	const float grooves = kGroovesPerDip * dipsWide; // per world unit; world width is 1
	const float amp = kGrooveSlope / (grooves * 2.0f);

	// Everything cut out of or behind the plate, gathered as WORLD-space PODs
	// the panel's distance lambda can capture by value.
	struct PocketW { float cx, cy, hw, hh, cr; };
	struct GrillW { float cx, cy, pitchX, pitchY, r; int cols, rows; };
	struct SlotW { float cx, cy, pitchY, halfLen, halfThick; int rows; };
	struct HoleW { float x, y, r; };
	struct SwitchW { float cx, cy; }; // the slot is centred, so cy serves both

	std::vector<PocketW> pockets;
	std::vector<GrillW> grills;
	std::vector<SlotW> slotBanks;
	std::vector<HoleW> holes; // jack bores and LED punch-outs
	std::vector<SwitchW> switches;

	for (const DipRect& r : computeJackPockets(spec.components, dipsWide))
	{
		pockets.push_back({
			toWorldX(0.5f * (r.x0 + r.x1)),
			toWorldY(0.5f * (r.y0 + r.y1)),
			0.5f * (r.x1 - r.x0) / dipsWide,
			0.5f * (r.y1 - r.y0) / dipsWide,
			kIndentCornerDips / dipsWide });
	}

	// A dark interior behind every through-cut. Holes are drilled RIGHT
	// THROUGH, and the room is hidden from the camera so the panel keeps a
	// transparent background -- without a backing every hole would be a
	// see-through gap onto the host's canvas.
	const auto addBacking = [&scene](const Vec3& centre, const Vec3& half)
	{
		Object backing;
		backing.material = recipes::paint({ 0.012f, 0.012f, 0.012f });
		backing.boundsCentre = centre;
		backing.boundsRadius = length(half) + 0.01f;
		backing.distance = [centre, half](const Vec3& p) { return sdBox(p - centre, half); };
		scene.add(std::move(backing));
	};

	const float jackBoreR = kJackBoreDips / dipsWide;
	const float jackPanelHoleR = kJackPanelHoleDips / dipsWide;

	for (const auto& c : spec.components)
	{
		const float wx = toWorldX(c.x);
		const float wy = toWorldY(c.y);

		switch (c.kind)
		{
		case PanelComponent::Kind::Jack:
			// The PLATE takes the body-sized cut-out; the body's own blind bore
			// is a separate, smaller thing built with the jack below.
			holes.push_back({ wx, wy, jackPanelHoleR });
			addBacking({ wx, wy, -halfZ - 0.12f },
				{ jackPanelHoleR + 0.02f, jackPanelHoleR + 0.02f, 0.08f });
			break;

		case PanelComponent::Kind::Led:
			holes.push_back({ wx, wy, kLedHoleRadiusDips / dipsWide });
			addBacking({ wx, wy, -halfZ - 0.12f },
				{ (kLedHoleRadiusDips + 1.5f) / dipsWide, (kLedHoleRadiusDips + 1.5f) / dipsWide, 0.08f });
			break;

		case PanelComponent::Kind::Grill:
		{
			const float pitchX = kVentPitchDips / dipsWide;
			const float pitchY = pitchX * 0.86602540f; // hexagonal row spacing
			const int cols = ventColsFor(dipsWide, c.cols);
			const float vx = toWorldX(ventCentreXDips(c, dipsWide));
			grills.push_back({ vx, wy, pitchX, pitchY,
				kVentHoleRadiusDips / dipsWide, cols, c.rows });
			addBacking({ vx, wy, -halfZ - 0.12f },
				{ 0.5f * pitchX * (float)cols + 0.04f,
				  0.5f * pitchY * (float)c.rows + 0.04f, 0.08f });
			break;
		}

		case PanelComponent::Kind::Slots:
		{
			const float pitchY = kSlotPitchDips / dipsWide;
			const float halfLen = slotHalfLenFor(dipsWide) / dipsWide;
			const float vx = toWorldX(ventCentreXDips(c, dipsWide));
			slotBanks.push_back({ vx, wy, pitchY, halfLen,
				kSlotHalfThickDips / dipsWide, c.rows });
			addBacking({ vx, wy, -halfZ - 0.12f },
				{ halfLen + 0.04f, 0.5f * pitchY * (float)c.rows + 0.04f, 0.08f });
			break;
		}

		case PanelComponent::Kind::Switch:
		{
			switches.push_back({ wx, wy });
			addBacking({ wx, wy, -halfZ - 0.12f }, {
				kSwitchHoleHalfWidthDips / dipsWide + 0.04f,
				kSwitchHoleHalfHeightDips / dipsWide + 0.04f, 0.08f });
			break;
		}

		default:
			break;
		}
	}

	// --- the plate itself ----------------------------------------------------

	Material faceMaterial;
	bool grain = false;
	switch (spec.material)
	{
	case 1: // flat aluminium: isotropic, like the switch plate
		faceMaterial = recipes::polishedAluminium(kFlatRoughness);
		break;
	case 2: // powder-coated steel: a satin dielectric skin over the pin's colour
		faceMaterial = recipes::plastic(spec.colour, kPowderRoughness);
		break;
	default: // brushed HORIZONTALLY: material axis and groove axis are one
	         // decision in two places -- here and the brushGrain call below.
		faceMaterial = recipes::brushed(recipes::polishedAluminium(kRoughness),
			BrushMode::Fixed, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, kAnisotropy);
		grain = true;
		break;
	}

	const float swHalfW = kSwitchHalfWidthDips / dipsWide;
	const float swHalfH = kSwitchHalfHeightDips / dipsWide;
	const float swCorner = kSwitchCornerDips / dipsWide;
	const float swFloorZ = halfZ - kSwitchDepth;
	const float swHoleHalfW = kSwitchHoleHalfWidthDips / dipsWide;
	const float swHoleHalfH = kSwitchHoleHalfHeightDips / dipsWide;
	const float swHoleCorner = kSwitchHoleCornerDips / dipsWide;

	// ONE backing plate behind the WHOLE panel, rather than a box per hole.
	//
	// Per-hole boxes each covered their own hole and still left blue rims,
	// because a hole's edge is antialiased: the sub-pixel samples that go down
	// the hole have to land on something, and anything they miss is scored as
	// background -- which is transparent, so the host's canvas showed through
	// at partial alpha. A plate that spans everything cannot be missed.
	//
	// Inset by a DIP so it can never poke past the panel's own silhouette and
	// spoil the transparent surround, and given the panel's corner radius so
	// the inset holds at the corners too.
	{
		const float inset = 1.0f / dipsWide;
		const float backZ = -halfZ - 0.06f;

		Object backplate;
		backplate.material = recipes::paint({ 0.012f, 0.012f, 0.012f });
		backplate.boundsCentre = { 0.0f, 0.0f, backZ };
		backplate.boundsRadius = safeSqrt(halfW * halfW + halfH * halfH) + 0.1f;
		backplate.distance = [halfW, halfH, cornerR, inset, backZ](const Vec3& p)
		{
			const float d2 = sdRoundRect2D(p.x, p.y, 0.0f, 0.0f,
				halfW - inset, halfH - inset, cornerR);
			return extrudeZ(d2, std::fabs(p.z - backZ) - 0.05f);
		};
		scene.add(std::move(backplate));
	}

	Object panel;
	panel.material = faceMaterial;
	panel.boundsCentre = { 0.0f, 0.0f, 0.0f };
	panel.boundsRadius = safeSqrt(halfW * halfW + halfH * halfH + halfZ * halfZ) + 0.05f;
	panel.distance = [=](const Vec3& p)
	{
		float d = sdFaceplate(p, halfW, halfH, halfZ, cornerR, chamfer);

		for (const auto& gr : grills)
			d = opSubtract(d, taperedCut(sdVentHoles(p, gr.cx, gr.cy,
				gr.pitchX, gr.pitchY, gr.r, gr.cols, gr.rows), p.z, halfZ));

		for (const auto& sl : slotBanks)
			d = opSubtract(d, taperedCut(sdSlotVents(p, sl.cx, sl.cy, sl.pitchY,
				sl.halfLen, sl.halfThick, sl.rows), p.z, halfZ));

		for (const auto& sw : switches)
		{
			// The pocket, then the same square hole the plate gets -- the plate
			// alone is not enough, because behind it is the panel, and a hole
			// in one part only exposes the other. Same lesson as the jack bore.
			d = opSmoothSubtract(d, sdIndentTool(p, sw.cx, sw.cy, swHalfW, swHalfH,
				swCorner, swFloorZ), kIndentFillet);
			d = opSubtract(d, taperedCut(sdRoundRect2D(p.x, p.y, sw.cx, sw.cy,
				swHoleHalfW, swHoleHalfH, swHoleCorner), p.z, halfZ));
		}

		// POCKETS BEFORE HOLES, and the order is the whole difference between a
		// punched hole and a moulded dimple.
		//
		// opSmoothSubtract blends with whatever is already in the field, so
		// cutting the holes first let the pocket's fillet round their edges too:
		// a hole in a pocket floor came out with a bevel about 1.9 DIPs wide,
		// roughly two thirds of it fillet and the rest the through-cut taper.
		// Cutting the pocket first and then punching through it with a HARD
		// subtract leaves the hole's edge as sharp as the taper allows, which is
		// what a punched panel looks like -- you fillet the pressing, then you
		// punch it. The switch above has always done it in this order.
		for (const auto& po : pockets)
			d = opSmoothSubtract(d, sdIndentTool(p, po.cx, po.cy, po.hw, po.hh,
				po.cr, halfZ - kIndentDepth), kIndentFillet);

		// Jack bores and LED punch-outs: clean through the plate. What the eye
		// looks down is the dark thing behind -- the jack's blind barrel, the
		// LED's backing box -- so the hole goes dark without becoming a
		// see-through gap.
		// Radial cross-section rather than sdCylinder, so the same taper
		// applies: these are the deepest cuts and the worst offenders.
		for (const auto& h : holes)
		{
			const float dx = p.x - h.x;
			const float dy = p.y - h.y;
			const float radial = safeSqrt(dx * dx + dy * dy) - h.r;
			d = opSubtract(d, taperedCut(radial, p.z, halfZ));
		}

		if (grain)
		{
			// ACROSS the grain is y (horizontal brushing), so y is the fast
			// axis. Swapping these two arguments is the whole difference
			// between a horizontally and a vertically brushed panel.
			d -= amp * brushGrain(p.y * grooves, p.x * grooves);
		}

		// Displacing a field inflates its gradient, and sphere tracing needs
		// that gradient <= 1 or it steps straight through the surface. The
		// slope is bounded by kGrooveSlope, and opSmoothSubtract understates
		// distance near its fillet, so every step is shortened by this factor
		// to stay conservative.
		return d * 0.55f;
	};
	scene.add(std::move(panel));

	// --- the hardware --------------------------------------------------------

	for (const auto& c : spec.components)
	{
		const float wx = toWorldX(c.x);
		const float wy = toWorldY(c.y);

		switch (c.kind)
		{
		case PanelComponent::Kind::Jack:
		{
			// Two objects because it is two materials: CSG between objects is a
			// plain union, so the collar and the body simply overlap and each
			// keeps its own surface.
			const float rSurround = kJackSurroundDips / dipsWide;
			const float rOuter = kJackOuterDips / dipsWide;
			const float rInner = kJackInnerDips / dipsWide;

			// The bezel's face, which the collar sits on: above the PANEL
			// surface, not the pocket floor.
			const float bodyTop = halfZ + kJackBezelProudDips / dipsWide;
			const float bezelRound = kJackBezelRoundDips / dipsWide;

			// The collar: a TORUS, not a flat washer. A flat annulus facing an
			// orthographic camera reflects one direction -- a dim far wall --
			// and renders nearly black no matter how chrome it is. A round
			// tube sweeps through every angle, so some part of it is always
			// aimed at the key window: that is the bright ring on a real jack.
			{
				const float minor = 0.5f * (rOuter - rInner);
				const float major = 0.5f * (rOuter + rInner);
				const Vec3 centre{ wx, wy, bodyTop + kJackProudFrac * minor };

				Object collar;
				collar.material = recipes::chrome(kJackMetalRoughness);
				collar.boundsCentre = centre;
				collar.boundsRadius = major + minor + 0.01f;
				collar.distance = [centre, major, minor](const Vec3& p)
				{
					return sdTorus(p - centre, major, minor);
				};
				scene.add(std::move(collar));
			}

			// The body: ONE piece of black plastic running right under the
			// collar, so it shows both OUTSIDE the ring as a surround and
			// INSIDE it around the mouth -- a moulded body with a plated ring
			// set into it, which is how the real part is made.
			//
			// A BARREL, from just proud of the panel face to well BEHIND the
			// plate. Its front face is the black surround; its length is what
			// gives the bore somewhere deep to go, and its rounded rim is the
			// visible edge.
			{
				const float top = bodyTop;
				const float bottom = -halfZ - 0.10f;
				const float hz = 0.5f * (top - bottom);
				const Vec3 centre{ wx, wy, 0.5f * (top + bottom) };

				// A BLIND bore, but a DEEP one: deep enough that almost nothing
				// reaches the bottom is what makes the mouth read as an actual
				// hole rather than a dark disc.
				const float boreDepth = 1.85f * hz;
				const float rBore = jackBoreR;

				Object body;
				// Darker and matter than the knobs. It has to read as a VOID
				// down the middle, and a glossy wall at grazing incidence is a
				// near-perfect mirror -- which is how the sky got in there.
				body.material = recipes::plastic({ 0.015f, 0.015f, 0.015f }, 0.42f);
				body.boundsCentre = centre;
				body.boundsRadius = safeSqrt(rSurround * rSurround + hz * hz) + 0.01f;
				body.distance = [centre, rSurround, rBore, hz, boreDepth, bezelRound, halfZ](const Vec3& p)
				{
					const Vec3 q = p - centre;
					const float boreHalf = 0.5f * (boreDepth + 0.1f);
					const float boreZ = hz + 0.1f - boreHalf;
					float d = opSubtract(sdRoundCylinder(q, rSurround, hz, bezelRound),
						sdCylinder(q - Vec3{ 0.0f, 0.0f, boreZ }, rBore, boreHalf));

					// Narrow the shank behind the panel face, so the punched
					// slot around it is not bounded by a vertical wall. This is
					// what stops the slot leaking the background; see
					// taperedShank for why the cut's own taper is not enough.
					const float radial = safeSqrt(q.x * q.x + q.y * q.y);
					d = tide::render::maxf(d, taperedShank(radial, rSurround, p.z, halfZ));
					return d;
				};
				scene.add(std::move(body));
			}
			break;
		}

		case PanelComponent::Kind::Knob:
		{
			// Seated ON the panel face: centre offset by its own half height so
			// the base lands exactly on the front surface.
			const float r = (c.big ? kKnobBigRadiusDips : kKnobSmallRadiusDips) / dipsWide;
			const float hz = (c.big ? kKnobBigHeightFrac : kKnobSmallHeightFrac) * r;
			const float bevel = kKnobBevelFrac * r;
			const Vec3 centre{ wx, wy, halfZ + hz };

			Object knob;
			knob.material = recipes::plastic({ 0.023f, 0.023f, 0.023f }, kKnobRoughness);
			knob.boundsCentre = centre;
			knob.boundsRadius = safeSqrt(r * r + hz * hz) + 0.01f;
			knob.distance = [centre, r, hz, bevel](const Vec3& p)
			{
				return sdChamferCylinder(p - centre, r, hz, bevel);
			};
			scene.add(std::move(knob));
			break;
		}

		case PanelComponent::Kind::Switch:
		{
			// The inset plate: unbrushed, a shade smaller than its pocket (see
			// kSwitchPlateGapDips), buried well below the pocket floor so its
			// underside never becomes a surface anything can see.
			const float gap = kSwitchPlateGapDips / dipsWide;
			const float top = swFloorZ + 0.010f;
			const float bottom = swFloorZ - 0.05f;
			const float hz = 0.5f * (top - bottom);
			const float cz = 0.5f * (top + bottom);

			Object plate;
			plate.material = recipes::polishedAluminium(kSwitchPlateRoughness);
			plate.boundsCentre = { wx, wy, cz };
			plate.boundsRadius = safeSqrt(swHalfW * swHalfW + swHalfH * swHalfH + hz * hz) + 0.01f;
			plate.distance = [=](const Vec3& p)
			{
				const float rect = sdRoundRect2D(p.x, p.y, wx, wy,
					swHalfW - gap, swHalfH - gap, swCorner);
				const float d = extrudeZ(rect, std::fabs(p.z - cz) - hz);

				// The punched slot, straight through the plate.
				return opSubtract(d, taperedCut(sdRoundRect2D(p.x, p.y, wx, wy,
					swHoleHalfW, swHoleHalfH, swHoleCorner), p.z, swFloorZ));
			};
			scene.add(std::move(plate));
			break;
		}

		default:
			break;
		}
	}

#if TIDE_PANEL_MIRROR_BALL
	// Sits in FRONT of the panel so nothing clips it. Chrome rather than
	// aluminium: the point is to see the room, not the metal.
	{
		const Vec3 centre{ 0.0f, kBallCentreYFrac * halfH, halfZ + kBallRadius + 0.02f };

		Object ball;
		ball.material = recipes::chrome();
		ball.boundsCentre = centre;
		ball.boundsRadius = kBallRadius + 0.01f;
		ball.distance = [centre](const Vec3& p) { return sdSphere(p - centre, kBallRadius); };
		scene.add(std::move(ball));
	}
#endif

	Camera camera;
	camera.position = { 0.0f, 0.0f, 6.0f };
	camera.target = { 0.0f, 0.0f, 0.0f };
	camera.filmWidth = halfW * 2.0f;

#if TIDE_PANEL_DIAGNOSTIC_VIEW
	camera.position = normalize(Vec3{ 0.85f, 0.28f, 1.0f }) * 9.0f;
	camera.target = { 0.0f, 0.0f, 0.0f };
	camera.filmWidth = 1.45f; // the plate foreshortens, so leave margin
#endif

	Settings settings;
	settings.width = (int)pixelWidth;
	settings.height = (int)pixelHeight;
	settings.samplesPerPixel = samplesPerPixel;
	settings.maxBounces = 8;

	return render(scene, camera, settings);
}

// Samples for a trace of this many pixels: full quality at the size a 1-unit
// panel comes out, tapering by sqrt of the area ratio beyond that, never below
// kMinSamplesPerPixel.
int samplesFor(uint32_t width, uint32_t height)
{
	const double pixels = (double)width * (double)height;
	if (pixels <= (double)kReferenceTracePixels)
		return kSamplesPerPixel;

	const double taper = std::sqrt((double)kReferenceTracePixels / pixels);
	return (std::max)(kMinSamplesPerPixel,
		(int)std::lround((double)kSamplesPerPixel * taper));
}

// The face, and how it gets made.
//
// NOTHING HERE RUNS ON THE UI THREAD. An earlier version traced the preview
// synchronously "because it is only ~1/36th of the work", which is true and
// still wrong: during a resize drag every frame is a new pixel size, so the UI
// thread paid for a trace per frame AND spawned a full-size worker per frame,
// and a dozen multi-second workers fighting for cores janked everything else.
//
// So the UI thread only ever ASKS. If no image exists yet it draws flat grey
// (see kPlaceholderGrey) and tries again on the next tick.
struct FaceTrace
{
	// stage: 0 nothing, 1 preview usable, 2 full-size ready. Written by the
	// worker with release, read by the UI with acquire; the images behind it
	// are only touched by the worker before the store that publishes them.
	std::atomic<int> stage{ 0 };
	tide::render::Image preview;
	tide::render::Image full;
	uint32_t previewWidth = 0, previewHeight = 0;
	uint32_t fullWidth = 0, fullHeight = 0;
};

// ONE worker, with a "most recently wanted" slot rather than a queue.
//
// A queue would be wrong for this: during a drag the intermediate sizes are
// garbage the moment the next frame arrives, and a queue would faithfully
// render every one of them. The slot COALESCES -- overwriting it discards the
// sizes nobody is waiting for any more -- so a drag of any length costs at
// most the trace already in flight plus one more.
class FaceRenderer
{
public:
	struct Key
	{
		uint32_t width = 0, height = 0;
		uint64_t config = 0;
		bool operator<(const Key& o) const
		{
			return std::tie(width, height, config) < std::tie(o.width, o.height, o.config);
		}
		bool operator==(const Key& o) const
		{
			return width == o.width && height == o.height && config == o.config;
		}
	};

	struct Result
	{
		std::shared_ptr<FaceTrace> target;   // the trace for the size asked for
		std::shared_ptr<FaceTrace> fallback; // newest COMPLETE render of the
		                                     // same panel at ANY size, or null
	};

	// UI thread. Returns the trace for `key` -- possibly still empty -- and
	// makes it the thing the worker is working toward. Never blocks on a
	// render; the only lock held is around the bookkeeping.
	//
	// `fallback` is what makes a resize look like a resize rather than a
	// reload. It lives HERE rather than in the editor because the host may
	// rebuild the editor at any time (the headless screenshot harness builds a
	// fresh one per frame), and an image kept in a member would not survive
	// that. Kept per CONFIG, so it is only ever offered for a panel whose
	// layout, material and width in units are unchanged -- a stale render of a
	// DIFFERENT panel would be showing the wrong thing, where a stale render at
	// a different size is showing the right thing at the wrong resolution.
	Result request(const Key& key, const PanelSpec& spec)
	{
		std::unique_lock<std::mutex> lock(mutex);

		if (!started)
		{
			worker = std::thread([this] { run(); });
			started = true;
		}

		auto it = cache.find(key);
		if (it == cache.end())
		{
			evictLocked();
			it = cache.emplace(key, std::make_shared<FaceTrace>()).first;
			order.push_back(key);
		}

		Result result;
		result.target = it->second;
		if (lastUsable && lastUsableConfig == key.config && lastUsable != result.target)
			result.fallback = lastUsable;

		// Even a cached-but-incomplete trace is re-declared as wanted: it may
		// have been abandoned half-done when the size changed away and back.
		if (result.target->stage.load(std::memory_order_acquire) < 2)
		{
			wantedKey = key;
			wantedSpec = spec;
			haveWanted = true;
			lock.unlock();
			cv.notify_one();
		}
		return result;
	}

	~FaceRenderer()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			quit = true;
		}
		cv.notify_one();
		if (worker.joinable())
			worker.join();
	}

private:
	// Keep a few finished traces so flicking back to a previous size or layout
	// is instant. Only COMPLETE ones are evicted -- an in-flight trace is
	// still owned by the worker.
	static size_t traceCost(const FaceTrace& t)
	{
		return (size_t)t.fullWidth * t.fullHeight
			+ (size_t)t.previewWidth * t.previewHeight;
	}

	size_t cachedPixelsLocked() const
	{
		size_t total = 0;
		for (const auto& entry : cache)
			total += traceCost(*entry.second);
		return total;
	}

	// Evict oldest-first until the cache is inside its pixel budget. Only
	// COMPLETE traces go: an in-flight one is still owned by the worker, and
	// the one currently on screen is held by the editor's shared_ptr anyway, so
	// erasing it here frees nothing until it stops being used.
	void evictLocked()
	{
		for (size_t i = 0; i < order.size(); )
		{
			if (cachedPixelsLocked() <= kTraceCachePixelBudget)
				return;

			const auto old = cache.find(order[i]);
			if (old != cache.end()
				&& old->second->stage.load(std::memory_order_acquire) == 2
				&& old->second != lastUsable)
			{
				cache.erase(old);
				order.erase(order.begin() + i);
			}
			else
				++i;
		}
	}

	void run()
	{
		for (;;)
		{
			Key key;
			PanelSpec spec;
			std::shared_ptr<FaceTrace> trace;
			{
				std::unique_lock<std::mutex> lock(mutex);
				cv.wait(lock, [this] { return haveWanted || quit; });
				if (quit)
					return;

				key = wantedKey;
				spec = wantedSpec;
				haveWanted = false;

				const auto it = cache.find(key);
				if (it == cache.end())
					continue; // evicted while we waited; nothing to fill
				trace = it->second;
			}

			if (trace->stage.load(std::memory_order_relaxed) >= 2)
				continue;

			// PREVIEW first, at a fraction of the size. Cost is linear in
			// pixels, so 1/6 on a side is ~1/36th of the work: it lands in
			// well under a second and gives the panel the right material under
			// the right lights long before the full trace is done.
			if (trace->stage.load(std::memory_order_relaxed) < 1)
			{
				trace->previewWidth = (std::max)(1u, (key.width + kPreviewDivisor - 1) / kPreviewDivisor);
				trace->previewHeight = (std::max)(1u, (key.height + kPreviewDivisor - 1) / kPreviewDivisor);
				trace->preview = traceFaceplate(trace->previewWidth, trace->previewHeight,
					spec, kPreviewSamples);
				trace->stage.store(1, std::memory_order_release);

				// Available as a stand-in immediately. Only claim the slot if
				// nothing better holds it: a FULL render of this same panel at
				// some other size still looks better stretched than a preview
				// does, so it is not displaced by one.
				{
					std::lock_guard<std::mutex> lock(mutex);
					const bool betterHeld = lastUsable
						&& lastUsableConfig == key.config
						&& lastUsable->stage.load(std::memory_order_acquire) >= 2;
					if (!betterHeld)
					{
						lastUsable = trace;
						lastUsableConfig = key.config;
					}
				}
			}

			// A cancellation point, and the one that matters: if the panel has
			// been resized again while the preview was rendering, the seconds
			// the full trace would cost are seconds spent on a size nobody is
			// looking at. Drop it and go round for the newer one.
			if (wantedElsewhere(key))
				continue;

			trace->fullWidth = key.width;
			trace->fullHeight = key.height;
			trace->full = traceFaceplate(key.width, key.height, spec,
				samplesFor(key.width, key.height));
			trace->stage.store(2, std::memory_order_release);

			// Becomes the stand-in every later size of this same panel gets to
			// show while its own trace runs.
			{
				std::lock_guard<std::mutex> lock(mutex);
				lastUsable = trace;
				lastUsableConfig = key.config;

				// The trace that just finished is the one that made the cache
				// bigger, so this is the moment to check the budget -- waiting
				// for the next request would hold the peak until something
				// else happened to be asked for.
				evictLocked();
			}
		}
	}

	bool wantedElsewhere(const Key& key)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return (haveWanted && !(wantedKey == key)) || quit;
	}

	std::mutex mutex;
	std::condition_variable cv;
	std::map<Key, std::shared_ptr<FaceTrace>> cache;
	std::vector<Key> order;
	Key wantedKey{};
	PanelSpec wantedSpec;
	// The best image rendered so far for `lastUsableConfig`, offered as a
	// stand-in while a new size renders. Deliberately NOT "the last COMPLETE
	// trace", which was the bug: zooming in before the first full trace landed
	// found nothing to fall back to and dropped all the way to flat grey, even
	// though a perfectly good preview was already on screen. A low-fi image
	// beats no image.
	std::shared_ptr<FaceTrace> lastUsable;
	uint64_t lastUsableConfig = 0;
	bool haveWanted = false;
	bool quit = false;
	bool started = false;
	std::thread worker; // joined in the destructor: a detached thread still
	                    // tracing inside this DLL when the host unloads it is
	                    // a crash with no usable stack
};

FaceRenderer& faceRenderer()
{
	static FaceRenderer instance;
	return instance;
}
}

class TiDEPanelGui final : public PluginEditor, public gmpi::TimerClient
{
	Pin<std::string> pinText;
	Pin<std::string> pinTextColor;
	Pin<int32_t> pinRackUnits;
	Pin<std::string> pinLayout;
	Pin<int32_t> pinMaterial;
	Pin<std::string> pinPanelColor;

	Bitmap faceBitmap;
	Bitmap captionBitmap;
	SizeU bitmapSize{};      // DEVICE pixels, not DIPs — see getDeviceScale
	float bitmapScale = 0.0f;
	bool faceDirty = true;
	bool captionDirty = true;

	// The progressive face: a shared trace, which stage of it is currently in
	// `faceBitmap`, and that bitmap's own pixel size.
	std::shared_ptr<FaceTrace> faceTrace;
	SizeU faceSize{};
	// The previous full-size render, shown stretched while the current size is
	// still being traced.
	std::shared_ptr<FaceTrace> faceFallback;

	// Which of the three possible sources faceBitmap was built from, so it is
	// rebuilt when a better one appears and not otherwise. Higher is better.
	// Worst to best. A stale FULL outranks the current size's preview because
	// quality here is pixels, not freshness; a stale PREVIEW ranks below both,
	// but above drawing nothing at all.
	enum FaceSource { FaceNone = 0, FaceStalePreview, FacePreview,
		FaceStaleFull, FaceCurrentFull };
	int faceBitmapSrc = FaceNone;
	int faceTraceStage = 0; // how much of faceTrace has been consumed
	bool timerRunning = false;

	// CpuReadable is what makes lockPixels() work at all, and SRGBPixels is what
	// makes the result 8-bit rather than the default 64bpp half-float. Both are
	// needed: on DirectX the 8-bit face is gated behind CpuReadable, so
	// SRGBPixels alone still yields a target that cannot be locked
	// (gmpi_ui/GmpiApiDrawing.h:112-122).
	static constexpr int32_t kBitmapFlags = (int32_t)BitmapRenderTargetFlags::SRGBPixels
		| (int32_t)BitmapRenderTargetFlags::CpuReadable;

	void invalidate()
	{
		if (drawingHost)
			drawingHost->invalidateRect(nullptr);
	}

	void onCaptionChanged()
	{
		captionDirty = true;
		invalidate();
	}

	void onFaceChanged()
	{
		faceDirty = true;
		invalidate();
	}

	// The panel's one true size. Width follows the Rack Units pin; height is
	// always 3U.
	Size panelSizeDips() const
	{
		return { kRackUnitDips * (float)(std::clamp)(pinRackUnits.value, 1, kMaxRackUnits),
			kRackHeightDips };
	}

	void onUnitsChanged()
	{
		// The DESIRED size just changed, so the host has to be told to ask
		// again -- invalidateRect alone would repaint the old rectangle.
		if (drawingHost)
			drawingHost->invalidateMeasure();
		onFaceChanged();
	}

	// The face is fully described by four pins; this is that description as a
	// value, plus a hash of it to key the trace cache. A collision would need
	// two different pin states hashing identically AND sharing a pixel size.
	PanelSpec buildSpec() const
	{
		PanelSpec spec;
		spec.units = (std::clamp)(pinRackUnits.value, 1, kMaxRackUnits);
		spec.material = (std::clamp)(pinMaterial.value, 0, 2);

		// colorFromHexString decodes sRGB to LINEAR (Drawing.h,
		// SRGBPixelToLinear), which is exactly what the tracer wants an albedo
		// to be -- no second conversion here.
		const Color c = pinPanelColor.value.empty()
			? colorFromHex(0x2E3238u)
			: colorFromHexString(pinPanelColor.value);
		spec.colour = { c.r, c.g, c.b };

		spec.components = parsePanelLayout(pinLayout.value);
		return spec;
	}

	uint64_t specConfigHash() const
	{
		std::string all;
		all += pinLayout.value;         all += '\x1f';
		all += std::to_string(pinRackUnits.value); all += '\x1f';
		all += std::to_string(pinMaterial.value);  all += '\x1f';
		all += pinPanelColor.value;
		return (uint64_t)std::hash<std::string>{}(all);
	}

	// Magnitude of a transform's linear (scale) part.
	static float transformScale(const Matrix3x2& m)
	{
		const float s = std::sqrt(m._11 * m._11 + m._12 * m._12);
		return (s > 0.0f) ? s : 1.0f;
	}

	// PHYSICAL pixels per logical unit. `bounds` is in DIPs, so a bitmap sized
	// from it directly is generated at 1 pixel per DIP and then STRETCHED to the
	// display by drawBitmap — soft on any HiDPI screen and softer still at panel
	// zoom. The host's rasterization scale covers the display; the transform's
	// scale covers the zoom; the texture needs both.
	// Precedent: SynthEditLib/modules/Controls/Scope4Gui.cpp:353.
	float getDeviceScale(Graphics& g) const
	{
		const float dpiScale = drawingHost.get() ? drawingHost->getRasterizationScale() : 1.0f;
		return (std::max)(0.01f, dpiScale * transformScale(g.getTransform()));
	}

	// The hex-string -> Color conversion is gmpi_ui's own (Drawing.h:654). It
	// already implements SynthEdit's convention exactly: AARRGGBB, with alpha
	// taken from the high byte only when the string is longer than 6 digits.
	Color getTextColor() const
	{
		return pinTextColor.value.empty() ? Colors::Black : colorFromHexString(pinTextColor.value);
	}

	// Toward white, keeping the colour's own alpha — interpolateColor would drag
	// alpha to opaque along with the channels. A small amount goes a long way
	// from a dark fill, which is why kTextGradientLift is low.
	static Color liftedTowardWhite(Color c, float amount)
	{
		return Color{
			c.r + (1.0f - c.r) * amount,
			c.g + (1.0f - c.g) * amount,
			c.b + (1.0f - c.b) * amount,
			c.a };
	}

	static float cornerRadius(const Size& size)
	{
		return (std::clamp)(kCornerRadius, 0.0f, 0.5f * (std::min)(size.width, size.height));
	}


#if TIDE_PANEL_RAISED_EDGE
	// A white highlight along the TOP and LEFT inside edge of every glyph —
	// raised paint catching a light from the top-left.
	//
	// This is BumpGui.cpp's inner-highlight recipe applied to glyphs instead of
	// a rounded rect: take a copy of the shape, slide it AWAY from the light,
	// blur it, and light whatever the slid copy fails to cover. Because the copy
	// moves down-right, the band it vacates is the top-left inside edge, and
	// because it is blurred the band fades inward instead of stopping dead.
	//
	// The first version of this eroded the alpha by one pixel in the up/left
	// direction and lit the shortfall. That is a hard-edged rim, not lighting —
	// it aliases along every diagonal and reads as an outline. The blur is what
	// makes it look like a lit surface, and it is the SAME blur the rest of the
	// codebase uses (ginSingleChannel, via cachedBlur).
	//
	// Premultiplied white is simply "every channel equals alpha", so lighting a
	// pixel is a lerp of each channel toward its own alpha.
	static void addRaisedEdge(Bitmap& bitmap, float deviceScale)
	{
		if constexpr (kGlow <= 0.0f)
			return;

		auto pixels = bitmap.lockPixels(BitmapLockFlags::ReadWrite);
		if (!pixels)
			return;

		if (pixels.getBytesPerPixel() != 4 || !pixels.isInteger())
			return;

		uint8_t* const data = pixels.getAddress();
		const int32_t bytesPerRow = pixels.getBytesPerRow();
		const auto size = pixels.getSize();
		const int32_t w = (int32_t)size.width;
		const int32_t h = (int32_t)size.height;

		const int32_t offset = (std::max)(1, (int32_t)std::lround(kEdgeOffsetDips * deviceScale));
		const unsigned radius = (unsigned)(std::clamp)(
			(int32_t)std::lround(kEdgeBlurDips * deviceScale), 1, 254);

		// TWO blurred copies of the glyph coverage: one in place, one slid
		// down-right away from the light. The highlight is their DIFFERENCE.
		//
		// Lighting from the slid copy alone -- (255 - occluder) -- SATURATES:
		// every pixel the slid copy misses gets full white, so the band is a flat
		// plateau with a hard inner shoulder, and the blur only ever softens the
		// shoulder. That still reads as an eroded rim, which is why adding the
		// blur alone did not fix the look. A difference of two blurs has no
		// plateau: it peaks at the edge and falls away smoothly on both sides,
		// which is what actually looks lit.
		std::vector<uint8_t> base((size_t)w * h, 0);
		std::vector<uint8_t> occluder((size_t)w * h, 0);
		for (int32_t y = 0; y < h; ++y)
		{
			const uint8_t* src = data + (size_t)y * bytesPerRow + 3;
			uint8_t* dst = base.data() + (size_t)y * w;
			for (int32_t x = 0; x < w; ++x, ++dst, src += 4)
				*dst = *src;
		}
		for (int32_t y = offset; y < h; ++y)
		{
			const uint8_t* src = base.data() + (size_t)(y - offset) * w;
			uint8_t* dst = occluder.data() + (size_t)y * w + offset;
			for (int32_t x = offset; x < w; ++x)
				*dst++ = *src++;
		}

		ginSingleChannel(base.data(), (unsigned)w, (unsigned)h, radius, (unsigned)w);
		ginSingleChannel(occluder.data(), (unsigned)w, (unsigned)h, radius, (unsigned)w);

		for (int32_t y = 0; y < h; ++y)
		{
			uint8_t* row = data + (size_t)y * bytesPerRow;
			const uint8_t* occ = occluder.data() + (size_t)y * w;
			const uint8_t* bas = base.data() + (size_t)y * w;
			for (int32_t x = 0; x < w; ++x)
			{
				uint8_t* px = row + (size_t)x * 4;
				const int32_t a = px[3];
				if (a == 0)
					continue;

				// How much more the glyph covers here than its slid copy does.
				// Zero deep inside (both blurs saturated), zero outside (both zero),
				// zero on the bottom-right edge (the slid copy covers MORE there).
				const int32_t lit = (int32_t)bas[x] - (int32_t)occ[x];
				if (lit <= 0)
					continue;

				const float t = (std::min)(1.0f,
					kGlow * kEdgeGain * (float)lit * (1.0f / 255.0f));

				for (int c = 0; c < 3; ++c)
					px[c] = (uint8_t)(px[c] + t * (a - px[c]) + 0.5f);
			}
		}
	}
#endif

	// Wraps a traced image in a GMPI bitmap. The tracer's output is LINEAR,
	// PREMULTIPLIED and HDR; writePixels tone maps it, encodes to sRGB and
	// premultiplies to match what the compositor expects, so all this has to get
	// right is the channel order — a RUN-TIME property of the target, BGRA on
	// Windows and RGBA on macOS from the very same call.
	static Bitmap bitmapFromImage(Graphics& g, const tide::render::Image& image,
		uint32_t width, uint32_t height)
	{
		const SizeU pixels{ width, height };

		auto rt = g.getFactory().createCpuRenderTarget(pixels, kBitmapFlags);
		rt.beginDraw();
		rt.clear(Color{ 0.0f, 0.0f, 0.0f, 0.0f });
		rt.endDraw();

		auto bitmap = rt.getBitmap();
		{
			auto locked = bitmap.lockPixels(BitmapLockFlags::ReadWrite);
			if (locked && locked.getBytesPerPixel() == 4 && locked.isInteger())
			{
				const auto order = (locked.channelLayout() == 1)
					? tide::render::PixelOrder::Rgba
					: tide::render::PixelOrder::Bgra;
				tide::render::writePixels(image, locked.getAddress(),
					locked.getBytesPerRow(), order, /*premultiply*/ true);

#ifdef TIDE_PANEL_ALPHA_DEBUG
				// THE ALPHA-LEAK DIAGNOSTIC. Off by default; #define
				// TIDE_PANEL_ALPHA_DEBUG at the top of this file to arm it.
				//
				// Paints every pixel the tracer left even slightly transparent
				// as opaque magenta, so a leak MEASURES ITSELF: screenshot the
				// panel, find the magenta, convert its radius to DIPs and
				// compare against the geometry constants. Nothing in the scene
				// is magenta, and alpha is byte 3 in both channel orders (as it
				// happens, magenta is byte-identical in RGBA and BGRA too).
				//
				// Reach for this EARLY. Alpha leaks look like lighting bugs and
				// invite plausible stories about reflections; the ring around
				// the jacks was blamed on the sky, then on the collar, before
				// this measured it at 11.1-12.9 DIPs from the jack centre and
				// identified the barrel wall at 11.81 in one shot. Expect a
				// residue of a DIP or two around the panel's own outline --
				// that is the silhouette's antialiasing, and it belongs there.
				{
					auto* base = static_cast<uint8_t*>(locked.getAddress());
					const auto stride = locked.getBytesPerRow();
					for (uint32_t yy = 0; yy < height; ++yy)
					{
						uint8_t* row = base + static_cast<size_t>(yy) * stride;
						for (uint32_t xx = 0; xx < width; ++xx)
						{
							uint8_t* px = row + 4 * xx;
							if (px[3] < 250)
							{
								px[0] = 255; px[1] = 0; px[2] = 255; px[3] = 255;
							}
						}
					}
				}
#endif
			}
		}
		return bitmap;
	}

	// The caption runs UP the panel — rotated a quarter-turn anti-clockwise,
	// reading bottom to top, the way a Eurorack module is legended.
	//
	// THE SIGN IS THE THING TO GET RIGHT. makeRotation takes RADIANS and screen
	// y points DOWN, so a POSITIVE angle turns clockwise on screen. Anti-
	// clockwise is therefore -pi/2, not +pi/2.
	//
	// Rotating about the panel's centre means the drawing box in the rotated
	// frame is the panel TRANSPOSED — height by width — about that same centre.
	// In that frame the two alignments map to panel edges: Leading along the
	// baseline is the panel's BOTTOM, and Near across it is the panel's LEFT.
	// Deliberately NOT scaled to fit: a tall caption on a short module runs off
	// the top, and Jeff ruled that acceptable. It is clipped rather than
	// overflowing only because this renders into a bounds-sized bitmap — drawn
	// straight to the screen it would cover the neighbouring rack module.
	Bitmap renderCaption(Graphics& g, const Size& size, const SizeU& pixels, float deviceScale) const
	{
		const Point centre{ size.width * 0.5f, size.height * 0.5f };

		auto rt = g.getFactory().createCpuRenderTarget(pixels, kBitmapFlags);
		rt.beginDraw();
		rt.clear(Color{ 0.0f, 0.0f, 0.0f, 0.0f });
		{
			// Composes with the rotation below: a point is rotated in DIP space
			// first, then scaled to device pixels.
			TempTransform toDevice(rt, makeScale(deviceScale));

			auto textFormat = rt.getFactory().createTextFormat(
				kCaptionBodyHeight,
				{},
				FontWeight::Bold);
			textFormat.setWordWrapping(WordWrapping::NoWrap);
			textFormat.setTextAlignment(TextAlignment::Leading);        // start at the bottom
			textFormat.setParagraphAlignment(ParagraphAlignment::Near); // sit on the left edge

			// The panel, transposed about its own centre.
			const Rect rotatedBounds{
				centre.x - size.height * 0.5f + kCaptionInset,
				centre.y - size.width * 0.5f,
				centre.x + size.height * 0.5f,
				centre.y + size.width * 0.5f
			};

			constexpr float quarterTurnAntiClockwise = -1.57079632679489661923f;
			const Matrix3x2 rotation = makeRotation(quarterTurnAntiClockwise, centre);

			// A brush's coordinates live in the space in force when it is USED,
			// which here is the rotated one — so a gradient specified corner to
			// corner would run along the baseline and tilt with the text. Pull
			// the two SCREEN corners back through the inverse rotation instead,
			// and the gradient stays put while the glyphs turn.
			const Matrix3x2 toLocal = invert(rotation);
			const Point gradientFrom = transformPoint(toLocal, Point{ 0.0f, 0.0f });
			const Point gradientTo = transformPoint(toLocal, Point{ size.width, size.height });

			const auto fill = getTextColor();
			auto brush = rt.createLinearGradientBrush(
				gradientFrom, gradientTo,
				liftedTowardWhite(fill, kTextGradientLift), // top-left
				fill);                                      // bottom-right

			TempTransform rotate(rt, rotation);
			rt.drawTextU(pinText.value.c_str(), textFormat, rotatedBounds, brush);
		}
		rt.endDraw();

		auto bitmap = rt.getBitmap();
#if TIDE_PANEL_RAISED_EDGE
		addRaisedEdge(bitmap, deviceScale);
#endif
		return bitmap;
	}

	void updateBitmaps(Graphics& g, const Size& size, float deviceScale)
	{
		const SizeU pixels{
			(uint32_t)(std::max)(1.0f, std::ceil(size.width * deviceScale)),
			(uint32_t)(std::max)(1.0f, std::ceil(size.height * deviceScale))
		};

		// The device scale is part of the cache key: a window dragged to a
		// different-DPI monitor, or a panel zoom, changes it without changing
		// the logical size, and a stale bitmap would then be resampled.
		if (bitmapSize != pixels || bitmapScale != deviceScale)
		{
			faceDirty = true;
			captionDirty = true;
			bitmapSize = pixels;
			bitmapScale = deviceScale;
		}

		if (faceDirty)
		{
			// Trace at the panel's own size in device pixels, limited by
			// RESOLUTION rather than by width -- see kMaxTraceScale. Both
			// dimensions take the same factor, so the traced aspect is always
			// the panel's and nothing is ever squashed.
			const float traceScale = (std::min)(deviceScale, kMaxTraceScale);
			const uint32_t width = (std::max)(1u,
				(uint32_t)std::lround(size.width * traceScale));
			const uint32_t height = (std::max)(1u,
				(uint32_t)std::lround(size.height * traceScale));

			// ASKS for a render; never performs one. Returns immediately, with
			// an empty trace if nothing has been rendered at this size yet,
			// plus the previous full-size render to show in the meantime.
			const auto result = faceRenderer().request(
				{ width, height, specConfigHash() }, buildSpec());
			faceTrace = result.target;
			faceFallback = result.fallback;
			faceTraceStage = 0;
			faceDirty = false;
		}

		// Choose the best image available THIS frame, and rebuild the bitmap
		// only when that choice changes.
		//
		// The order is by pixels, not by freshness: the previous full render is
		// 96 x 768 real pixels and stretching it a little beats a correctly
		// sized preview at 16 x 128 stretched six times. So a stale full render
		// outranks a fresh preview, and the preview is only ever seen on a
		// panel that has never finished a render at all.
		faceTraceStage = faceTrace
			? faceTrace->stage.load(std::memory_order_acquire) : 0;

		const int fallbackStage = faceFallback
			? faceFallback->stage.load(std::memory_order_acquire) : 0;

		int source = FaceNone;
		if (faceTraceStage >= 2)         source = FaceCurrentFull;
		else if (fallbackStage >= 2)     source = FaceStaleFull;
		else if (faceTraceStage >= 1)    source = FacePreview;
		else if (fallbackStage >= 1)     source = FaceStalePreview;

		if (source != faceBitmapSrc)
		{
			const bool stale = (source == FaceStaleFull || source == FaceStalePreview);
			const FaceTrace* from = stale ? faceFallback.get() : faceTrace.get();
			switch (source)
			{
			case FaceCurrentFull:
			case FaceStaleFull:
				faceBitmap = bitmapFromImage(g, from->full, from->fullWidth, from->fullHeight);
				faceSize = { from->fullWidth, from->fullHeight };
				break;
			case FacePreview:
			case FaceStalePreview:
				faceBitmap = bitmapFromImage(g, from->preview, from->previewWidth, from->previewHeight);
				faceSize = { from->previewWidth, from->previewHeight };
				break;
			default:
				faceBitmap = {};
				break;
			}
			faceBitmapSrc = source;
		}

		// Only once the CURRENT size is fully rendered is the stand-in dead
		// weight. Released later than it used to be, on purpose: it now has to
		// survive being a preview that a later zoom may still want.
		if (source == FaceCurrentFull)
			faceFallback.reset();

		if (captionDirty || !captionBitmap)
		{
			captionBitmap = pinText.value.empty()
				? Bitmap{}
				: renderCaption(g, size, pixels, deviceScale);
			captionDirty = false;
		}
	}

public:
	TiDEPanelGui()
	{
		pinText.onUpdate      = [this](PinBase*) { onCaptionChanged(); };
		pinTextColor.onUpdate = [this](PinBase*) { onCaptionChanged(); };

		const auto faceChanged = [this](PinBase*) { onFaceChanged(); };
		pinRackUnits.onUpdate  = [this](PinBase*) { onUnitsChanged(); };
		pinLayout.onUpdate     = faceChanged;
		pinMaterial.onUpdate   = faceChanged;
		pinPanelColor.onUpdate = faceChanged;
	}

	~TiDEPanelGui()
	{
		stopTimer();
	}

	// IGNORES availableSize, deliberately -- see kRackHeightDips. Returning
	// something derived from the offer, even clamped, would make the editor
	// report itself resizeable and put us back to drawing a squashed panel.
	ReturnCode measure(const Size* /*availableSize*/, Size* returnDesiredSize) override
	{
		*returnDesiredSize = panelSizeDips();
		return ReturnCode::Ok;
	}

	// The panel draws its own size from the top-left of `bounds`, which may be
	// larger OR smaller than what the host allotted, so the clip area is the
	// union rather than either one.
	ReturnCode getClipArea(Rect* returnRect) override
	{
		const Size size = panelSizeDips();
		*returnRect = Rect{
			bounds.left, bounds.top,
			(std::max)(bounds.right,  bounds.left + size.width),
			(std::max)(bounds.bottom, bounds.top + size.height) };
		return ReturnCode::Ok;
	}

	// Polls the worker. It does not call back on purpose: invalidateRect belongs
	// to the UI thread, and a worker reaching into the editor would have to be
	// synchronised against the editor being destroyed underneath it. The worker
	// touches nothing but its own FaceTrace, which outlives both.
	bool onTimer() override
	{
		// Repaint whenever the worker has moved on a stage, and keep polling
		// until the full-size trace for the CURRENT size has been consumed.
		// Keyed on the trace rather than on the bitmap, because during a
		// resize the bitmap is already at stage 2 -- it is the OLD one -- and
		// stopping there would leave the stale image up for good.
		const int stage = faceTrace
			? faceTrace->stage.load(std::memory_order_acquire) : 0;

		if (stage != faceTraceStage)
			invalidate();

		if (faceTrace && stage >= 2 && faceTraceStage >= 2)
		{
			timerRunning = false;
			return false; // returning false unregisters this client
		}
		return true;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		// The panel's own size, NOT the rectangle the host happened to give us.
		// A host that honours measure() hands us exactly this; one that does
		// not gets the panel drawn at its true size in the corner of whatever
		// it offered, rather than distorted to fill it.
		const Size size = panelSizeDips();
		updateBitmaps(g, size, getDeviceScale(g));

		const Rect panelRect{
			bounds.left, bounds.top,
			bounds.left + size.width, bounds.top + size.height };

		if (faceBitmap)
		{
			// Source is the FACE bitmap's own pixel rect, which is NOT
			// bitmapSize: the preview is a fraction of it, and a capped full
			// trace is smaller too. Destination is always the DIP bounds, so
			// whichever is current gets stretched to fill the panel.
			const Rect faceSource{ 0.0f, 0.0f, (float)faceSize.width, (float)faceSize.height };
			g.drawBitmap(faceBitmap, panelRect, faceSource);
		}
		else
		{
			// Nothing has EVER been rendered -- the first frame of a fresh
			// panel, and only then, since a resize keeps the previous image up
			// rather than falling back here. Flat grey in the panel's own
			// silhouette: the cheapest thing that is still the right SHAPE.
			const float radius = cornerRadius(size);
			auto brush = g.createSolidColorBrush(colorFromHex(kPlaceholderGrey));
			g.fillRoundedRectangle(RoundedRect{ panelRect, radius, radius }, brush);
		}

		if (faceTraceStage < 2 && !timerRunning)
		{
			startTimer(kPollMs);
			timerRunning = true;
		}

		if (captionBitmap)
		{
			const Rect source{ 0.0f, 0.0f, (float)bitmapSize.width, (float)bitmapSize.height };
			g.drawBitmap(captionBitmap, panelRect, source);
		}

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = Register<TiDEPanelGui>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="SE TiDE:Panel" name="Panel" category="TiDE">
    <GUI graphicsApi="GmpiUi">
        <Pin name="Text"        datatype="string_utf8" default=""/>
        <Pin name="Text Color"  datatype="string_utf8" default="FF101010"/>
        <Pin name="Rack Units"  datatype="int" default="1"/>
        <Pin name="Layout"      datatype="string_utf8" default="grill 24 20; knob big 24 163; knob small 24 216; switch 24 247; led 14 272; led 34 272; jack 24 297; jack 24 337; slots 24 371"/>
        <Pin name="Material"    datatype="enum" default="0" metadata="Brushed Aluminium, Flat Aluminium, Powder-coated Steel"/>
        <Pin name="Panel Color" datatype="string_utf8" default="FF2E3238"/>
    </GUI>
</Plugin>
)XML");
}
