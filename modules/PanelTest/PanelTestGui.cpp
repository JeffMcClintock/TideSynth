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
// ONLY TWO PINS, on purpose. The appearance is TIDE's, not the patch author's:
// PLAN constraint 8 ships one look and forbids user skins, so the faceplate's
// colours, corner radius, grain and glow are compile-time constants below
// rather than pins. Tune them here and every rack module moves together, which
// is the point. `SE Rectangle XP` exposed all of these as pins; deliberately
// not carried over.
//
// TWO CACHED BITMAPS, and both exist for the same reason: the effects need
// per-pixel access, which means a CPU-readable offscreen target.
//
//   face    — gradient plus material grain. Regenerating it per frame would be
//             both slow and WRONG: fresh random numbers every redraw would make
//             the surface crawl.
//   caption — the rotated legend and its inner glow. Rendering into a
//             bounds-sized bitmap also clips it for free.
//
// Per the design language (docs/ui-design-language.md), panels are flat in the
// sense that matters — no bevel, no drop shadow, no glow on the panel itself.
// The gradient and grain are material, not decoration, and both are subtle
// enough to be felt rather than seen.
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

// Paths per pixel, and the cap on how big a face is ever traced.
//
// The cap is what bounds the worst case. Tracing is SECONDS, and cost is linear
// in pixels, so without a ceiling a high-DPI or zoomed-in panel would quietly
// become a ten-second stall. Above the cap the face is traced at the cap and
// stretched, which costs a little sharpness and nothing else.
constexpr int kSamplesPerPixel = 128;
constexpr uint32_t kMaxTracedWidth = 96;

// The progressive preview: how much smaller it is on a side, and how few paths
// it gets. Both are deliberately crude — it exists to be replaced within a
// second or two, and being stretched over the panel hides most of its noise.
constexpr uint32_t kPreviewDivisor = 6;
constexpr int kPreviewSamples = 48;

// How often the UI thread asks whether the worker has finished.
constexpr int kPollMs = 100;

// One rack unit of panel width. The Rack Units pin scales the panel in these,
// and every layout coordinate and hardware size below is authored in the same
// DIPs, so a "10.6 DIP knob" is the same physical knob on any width of module.
constexpr float kRackUnitDips = 48.0f;

// --- hardware, all sizes in DIPs ---------------------------------------------
// WHERE things go now comes from the Layout pin; only WHAT they look like is
// compiled in, per the one-look rule (PLAN constraint 8).

// Punched vent: staggered so the holes pack hexagonally, which is what a real
// punched sheet does — more open area between the same web thickness.
constexpr float kVentPitchDips = 7.5f;
constexpr float kVentHoleRadiusDips = 1.9f;
constexpr int kVentColsDefault = 5;
constexpr int kVentRowsDefault = 4;

// Slotted vent: stadium slots (a rounded rect whose corner radius is half its
// thickness), sharing the faceplate's own 2D rounded-rect field.
constexpr float kSlotHalfLenDips = 13.0f;
constexpr float kSlotHalfThickDips = 1.6f;
constexpr float kSlotPitchDips = 5.5f;
constexpr int kSlotRowsDefault = 3;

// Jack sockets: a plated collar set in a black moulded body, blind bore down
// the middle.
constexpr float kJackSurroundDips = 10.5f; // black plastic body, OUTSIDE the collar
constexpr float kJackOuterDips = 7.0f;   // collar outer radius
constexpr float kJackInnerDips = 4.4f;   // collar inner radius
constexpr float kJackBoreDips = 3.5f;    // the socket mouth

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
constexpr float kJackClusterDips = 46.0f;
constexpr float kIndentPadDips = 5.0f;
constexpr float kIndentMinPadDips = 2.0f;
constexpr float kIndentCornerDips = 6.7f;
constexpr float kIndentDepth = 0.035f;   // world units, from the front face
constexpr float kIndentFillet = 0.02f;   // radius where the pocket meets the face

// LED punch-outs: a plain small hole into a dark void. The lens and its light
// get drawn over the top in vector later, like the knob pointer.
constexpr float kLedHoleRadiusDips = 1.6f;

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

constexpr float kSwitchHoleHalfDips = 4.3f;
constexpr float kSwitchHoleCornerDips = 0.35f; // a punch leaves a slight radius
constexpr float kSwitchHoleOffsetDips = 4.5f;  // above the pocket's centre

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
#define PANELTEST_DIAGNOSTIC_VIEW 0

// A mirrored ball floating in front of the panel, to SEE the environment the
// faceplate is reflecting. The studio is procedural and invisible to the camera
// (that is what keeps the panel's background transparent), so without something
// specular in the scene there is no way to look at it directly.
// OFF now the studio is settled -- it was a diagnostic, not part of the panel.
// Set to 1 to put it back when the lighting next needs looking at.
#define PANELTEST_MIRROR_BALL 0
#if PANELTEST_MIRROR_BALL
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
//
// Unparseable statements are SKIPPED, never errors: this text is edited live
// in a pin, and a half-typed line must not blank the panel.
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
		else if (tok[0] == "grill")  { c.kind = PanelComponent::Kind::Grill; c.cols = kVentColsDefault; c.rows = kVentRowsDefault; }
		else if (tok[0] == "slots")  { c.kind = PanelComponent::Kind::Slots; c.rows = kSlotRowsDefault; }
		else if (tok[0] == "led")    c.kind = PanelComponent::Kind::Led;
		else
			continue;

		if (tok.size() < coordAt + 2)
			continue;
		if (!parseFloatToken(tok[coordAt], c.x) || !parseFloatToken(tok[coordAt + 1], c.y))
			continue;

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

// --- the automatic indent ------------------------------------------------------

struct DipRect { float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f; };

// The keep-out footprint of a non-jack component, as a circle. Used only to
// push pocket edges back, so generous-and-round beats tight-and-exact.
float componentKeepOut(const PanelComponent& c)
{
	using tide::render::safeSqrt;
	switch (c.kind)
	{
	case PanelComponent::Kind::Knob:
		return (c.big ? kKnobBigRadiusDips : kKnobSmallRadiusDips) + 1.5f;
	case PanelComponent::Kind::Switch:
		return safeSqrt(kSwitchHalfWidthDips * kSwitchHalfWidthDips
			+ kSwitchHalfHeightDips * kSwitchHalfHeightDips) + 1.5f;
	case PanelComponent::Kind::Grill:
	{
		const float hw = 0.5f * kVentPitchDips * (float)c.cols;
		const float hh = 0.5f * kVentPitchDips * 0.866f * (float)c.rows;
		return safeSqrt(hw * hw + hh * hh) + 2.0f;
	}
	case PanelComponent::Kind::Slots:
	{
		const float hh = 0.5f * kSlotPitchDips * (float)c.rows;
		return safeSqrt(kSlotHalfLenDips * kSlotHalfLenDips + hh * hh) + 2.0f;
	}
	case PanelComponent::Kind::Led:
		return kLedHoleRadiusDips + 2.0f;
	default:
		return 0.0f;
	}
}

// Shrink `rect` just enough to clear the circle, never past `minRect`, by
// moving the ONE edge that costs the least area. One edge rather than all
// four: the intruder sits on one side of the jacks, and that is the side the
// pocket should give up.
void shrinkPocketClear(DipRect& rect, const DipRect& minRect, float cx, float cy, float r)
{
	using tide::render::clampf;

	const float px = clampf(cx, rect.x0, rect.x1);
	const float py = clampf(cy, rect.y0, rect.y1);
	const float dx = cx - px;
	const float dy = cy - py;
	if (dx * dx + dy * dy >= r * r)
		return; // already clear

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
	consider(0, cx - r, cx - r >= minRect.x1, (rect.x1 - (cx - r)) * h); // pull right edge in
	consider(1, cx + r, cx + r <= minRect.x0, ((cx + r) - rect.x0) * h); // pull left edge in
	consider(2, cy - r, cy - r >= minRect.y1, (rect.y1 - (cy - r)) * w); // pull bottom edge up
	consider(3, cy + r, cy + r <= minRect.y0, ((cy + r) - rect.y0) * w); // pull top edge down

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
std::vector<DipRect> computeJackPockets(const std::vector<PanelComponent>& comps)
{
	std::vector<size_t> jacks;
	for (size_t i = 0; i < comps.size(); ++i)
		if (comps[i].kind == PanelComponent::Kind::Jack)
			jacks.push_back(i);
	if (jacks.empty())
		return {};

	// Label propagation until settled. A handful of jacks; O(n^2) is nothing.
	std::vector<int> label(jacks.size());
	for (size_t i = 0; i < label.size(); ++i)
		label[i] = (int)i;

	for (bool changed = true; changed; )
	{
		changed = false;
		for (size_t a = 0; a < jacks.size(); ++a)
		{
			for (size_t b = a + 1; b < jacks.size(); ++b)
			{
				const auto& ca = comps[jacks[a]];
				const auto& cb = comps[jacks[b]];
				const float dx = ca.x - cb.x;
				const float dy = ca.y - cb.y;
				if (dx * dx + dy * dy <= kJackClusterDips * kJackClusterDips
					&& label[a] != label[b])
				{
					const int merged = (std::min)(label[a], label[b]);
					label[a] = label[b] = merged;
					changed = true;
				}
			}
		}
	}

	std::vector<DipRect> pockets;
	for (size_t seed = 0; seed < jacks.size(); ++seed)
	{
		if (label[seed] != (int)seed)
			continue; // not a cluster representative

		DipRect bbox{ 1.0e9f, 1.0e9f, -1.0e9f, -1.0e9f };
		for (size_t m = 0; m < jacks.size(); ++m)
		{
			if (label[m] != (int)seed)
				continue;
			const auto& c = comps[jacks[m]];
			bbox.x0 = (std::min)(bbox.x0, c.x);
			bbox.y0 = (std::min)(bbox.y0, c.y);
			bbox.x1 = (std::max)(bbox.x1, c.x);
			bbox.y1 = (std::max)(bbox.y1, c.y);
		}

		const float pad = kJackSurroundDips + kIndentPadDips;
		const float minPad = kJackSurroundDips + kIndentMinPadDips;
		DipRect pocket{ bbox.x0 - pad, bbox.y0 - pad, bbox.x1 + pad, bbox.y1 + pad };
		const DipRect minRect{ bbox.x0 - minPad, bbox.y0 - minPad, bbox.x1 + minPad, bbox.y1 + minPad };

		for (const auto& c : comps)
		{
			if (c.kind == PanelComponent::Kind::Jack)
				continue;
			const float r = componentKeepOut(c);
			if (r > 0.0f)
				shrinkPocketClear(pocket, minRect, c.x, c.y, r);
		}

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
	const float halfH = 0.5f * (float)pixelHeight / (float)pixelWidth;
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
	struct SwitchW { float cx, cy, holeY; };

	std::vector<PocketW> pockets;
	std::vector<GrillW> grills;
	std::vector<SlotW> slotBanks;
	std::vector<HoleW> holes; // jack bores and LED punch-outs
	std::vector<SwitchW> switches;

	for (const DipRect& r : computeJackPockets(spec.components))
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
		backing.material = recipes::paint({ 0.020f, 0.020f, 0.024f });
		backing.boundsCentre = centre;
		backing.boundsRadius = length(half) + 0.01f;
		backing.distance = [centre, half](const Vec3& p) { return sdBox(p - centre, half); };
		scene.add(std::move(backing));
	};

	const float jackBoreR = kJackBoreDips / dipsWide;

	for (const auto& c : spec.components)
	{
		const float wx = toWorldX(c.x);
		const float wy = toWorldY(c.y);

		switch (c.kind)
		{
		case PanelComponent::Kind::Jack:
			holes.push_back({ wx, wy, jackBoreR });
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
			grills.push_back({ wx, wy, pitchX, pitchY,
				kVentHoleRadiusDips / dipsWide, c.cols, c.rows });
			addBacking({ wx, wy, -halfZ - 0.12f },
				{ 0.5f * pitchX * (float)c.cols + 0.04f,
				  0.5f * pitchY * (float)c.rows + 0.04f, 0.08f });
			break;
		}

		case PanelComponent::Kind::Slots:
		{
			const float pitchY = kSlotPitchDips / dipsWide;
			const float halfLen = kSlotHalfLenDips / dipsWide;
			slotBanks.push_back({ wx, wy, pitchY, halfLen,
				kSlotHalfThickDips / dipsWide, c.rows });
			addBacking({ wx, wy, -halfZ - 0.12f },
				{ halfLen + 0.04f, 0.5f * pitchY * (float)c.rows + 0.04f, 0.08f });
			break;
		}

		case PanelComponent::Kind::Switch:
		{
			const float holeY = toWorldY(c.y - kSwitchHoleOffsetDips);
			switches.push_back({ wx, wy, holeY });
			const float hh = kSwitchHoleHalfDips / dipsWide;
			addBacking({ wx, holeY, -halfZ - 0.12f }, { hh + 0.04f, hh + 0.04f, 0.08f });
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
	const float swHoleHalf = kSwitchHoleHalfDips / dipsWide;
	const float swHoleCorner = kSwitchHoleCornerDips / dipsWide;

	Object panel;
	panel.material = faceMaterial;
	panel.boundsCentre = { 0.0f, 0.0f, 0.0f };
	panel.boundsRadius = safeSqrt(halfW * halfW + halfH * halfH + halfZ * halfZ) + 0.05f;
	panel.distance = [=](const Vec3& p)
	{
		float d = sdFaceplate(p, halfW, halfH, halfZ, cornerR, chamfer);

		for (const auto& gr : grills)
			d = opSubtract(d, sdVentHoles(p, gr.cx, gr.cy, gr.pitchX, gr.pitchY,
				gr.r, gr.cols, gr.rows));

		for (const auto& sl : slotBanks)
			d = opSubtract(d, sdSlotVents(p, sl.cx, sl.cy, sl.pitchY,
				sl.halfLen, sl.halfThick, sl.rows));

		for (const auto& sw : switches)
		{
			// The pocket, then the same square hole the plate gets -- the plate
			// alone is not enough, because behind it is the panel, and a hole
			// in one part only exposes the other. Same lesson as the jack bore.
			d = opSmoothSubtract(d, sdIndentTool(p, sw.cx, sw.cy, swHalfW, swHalfH,
				swCorner, swFloorZ), kIndentFillet);
			d = opSubtract(d, sdRoundRect2D(p.x, p.y, sw.cx, sw.holeY,
				swHoleHalf, swHoleHalf, swHoleCorner));
		}

		// Jack bores and LED punch-outs: clean through the plate. What the eye
		// looks down is the dark thing behind -- the jack's blind barrel, the
		// LED's backing box -- so the hole goes dark without becoming a
		// see-through gap.
		for (const auto& h : holes)
			d = opSubtract(d, sdCylinder(p - Vec3{ h.x, h.y, 0.0f }, h.r, 1.0f));

		for (const auto& po : pockets)
			d = opSmoothSubtract(d, sdIndentTool(p, po.cx, po.cy, po.hw, po.hh,
				po.cr, halfZ - kIndentDepth), kIndentFillet);

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
				body.material = recipes::satinPlastic({ 0.020f, 0.020f, 0.022f });
				body.boundsCentre = centre;
				body.boundsRadius = safeSqrt(rSurround * rSurround + hz * hz) + 0.01f;
				body.distance = [centre, rSurround, rBore, hz, boreDepth, bezelRound](const Vec3& p)
				{
					const Vec3 q = p - centre;
					const float boreHalf = 0.5f * (boreDepth + 0.1f);
					const float boreZ = hz + 0.1f - boreHalf;
					return opSubtract(sdRoundCylinder(q, rSurround, hz, bezelRound),
						sdCylinder(q - Vec3{ 0.0f, 0.0f, boreZ }, rBore, boreHalf));
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
			knob.material = recipes::plastic({ 0.022f, 0.022f, 0.025f }, kKnobRoughness);
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
			const float holeY = toWorldY(c.y - kSwitchHoleOffsetDips);
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

				// The punched hole, straight through the plate.
				return opSubtract(d, sdRoundRect2D(p.x, p.y, wx, holeY,
					swHoleHalf, swHoleHalf, swHoleCorner));
			};
			scene.add(std::move(plate));
			break;
		}

		default:
			break;
		}
	}

#if PANELTEST_MIRROR_BALL
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

#if PANELTEST_DIAGNOSTIC_VIEW
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

// One trace per (pixel size, panel configuration), shared by every instance
// and every redraw.
//
// PROGRESSIVE, because a full trace is seconds and a redraw cannot wait. The
// PREVIEW is traced synchronously at a fraction of the size — cost is linear in
// pixels, so at 1/6 on a side it is ~1/36th of the work and does not show — and
// the full-size trace runs on a worker. Until that lands the preview is
// stretched over the panel: soft, but already the right material under the
// right lights, which a flat grey placeholder is not.
struct FaceTrace
{
	tide::render::Image preview;              // ready when the object is
	tide::render::Image full;                 // written by the worker only
	std::atomic<bool> fullReady{ false };     // release/acquire handoff for `full`
	uint32_t previewWidth = 0, previewHeight = 0;
	uint32_t fullWidth = 0, fullHeight = 0;
};

// Workers are JOINED before their thread objects are destroyed. A detached
// thread still running inside this DLL when the host unloads it is a crash
// with no usable stack, and a trace takes long enough to make that a real race
// rather than a theoretical one. Finished workers are also reaped as new work
// arrives, so live-editing the Layout pin does not accumulate dead threads.
struct FaceTraceCache
{
	struct Key
	{
		uint32_t width = 0, height = 0;
		uint64_t config = 0;
		bool operator<(const Key& o) const
		{
			return std::tie(width, height, config) < std::tie(o.width, o.height, o.config);
		}
	};

	std::mutex mutex;
	std::map<Key, std::shared_ptr<FaceTrace>> traces;
	std::vector<Key> insertOrder; // for eviction, oldest first
	std::vector<std::pair<std::thread, std::shared_ptr<FaceTrace>>> workers;

	~FaceTraceCache()
	{
		for (auto& worker : workers)
		{
			if (worker.first.joinable())
				worker.first.join();
		}
	}
};

std::shared_ptr<FaceTrace> faceTraceFor(uint32_t width, uint32_t height,
	const PanelSpec& spec, uint64_t configHash)
{
	static FaceTraceCache cache;

	std::lock_guard<std::mutex> lock(cache.mutex);

	// Reap finished workers: their trace is complete, so join returns at once.
	for (size_t i = 0; i < cache.workers.size(); )
	{
		if (cache.workers[i].second->fullReady.load(std::memory_order_acquire))
		{
			cache.workers[i].first.join();
			cache.workers.erase(cache.workers.begin() + i);
		}
		else
			++i;
	}

	const FaceTraceCache::Key key{ width, height, configHash };
	const auto it = cache.traces.find(key);
	if (it != cache.traces.end())
		return it->second;

	// Evict the oldest COMPLETE traces beyond a small budget. The images are a
	// couple of megabytes each and the Layout pin is edited live, so an
	// unbounded cache would quietly grow by one render per edit. In-flight
	// traces are exempt: their worker still owns a reference anyway.
	constexpr size_t kMaxCached = 8;
	for (size_t i = 0; i < cache.insertOrder.size() && cache.traces.size() >= kMaxCached; )
	{
		const auto old = cache.traces.find(cache.insertOrder[i]);
		if (old != cache.traces.end()
			&& old->second->fullReady.load(std::memory_order_acquire))
		{
			cache.traces.erase(old);
			cache.insertOrder.erase(cache.insertOrder.begin() + i);
		}
		else
			++i;
	}

	auto trace = std::make_shared<FaceTrace>();

	trace->previewWidth = (std::max)(1u, (width + kPreviewDivisor - 1) / kPreviewDivisor);
	trace->previewHeight = (std::max)(1u, (height + kPreviewDivisor - 1) / kPreviewDivisor);
	trace->preview = traceFaceplate(trace->previewWidth, trace->previewHeight,
		spec, kPreviewSamples);

	trace->fullWidth = width;
	trace->fullHeight = height;
	std::thread worker([trace, width, height, spec]
	{
		trace->full = traceFaceplate(width, height, spec, kSamplesPerPixel);
		trace->fullReady.store(true, std::memory_order_release);
	});
	cache.workers.emplace_back(std::move(worker), trace);

	cache.traces.emplace(key, trace);
	cache.insertOrder.push_back(key);
	return trace;
}
}

class PanelTestGui final : public PluginEditor, public gmpi::TimerClient
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

	// The progressive face: a shared trace, which of its two images is currently
	// in `faceBitmap`, and that bitmap's own pixel size.
	std::shared_ptr<FaceTrace> faceTrace;
	SizeU faceSize{};
	bool faceIsFull = false;
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

	// The face is fully described by four pins; this is that description as a
	// value, plus a hash of it to key the trace cache. A collision would need
	// two different pin states hashing identically AND sharing a pixel size.
	PanelSpec buildSpec() const
	{
		PanelSpec spec;
		spec.units = (std::clamp)(pinRackUnits.value, 1, 16);
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
			// Cap the traced size. Cost is linear in pixels and a full trace is
			// seconds, so past the cap the face is traced at the cap and
			// stretched — which costs some sharpness, where not capping would
			// cost a stall that grows without limit as the panel is zoomed.
			const uint32_t width = (std::min)(pixels.width, kMaxTracedWidth);
			const uint32_t height = (std::max)(1u, (uint32_t)std::lround(
				(double)pixels.height * (double)width / (double)pixels.width));

			faceTrace = faceTraceFor(width, height, buildSpec(), specConfigHash());
			faceBitmap = {};
			faceIsFull = false;
			faceDirty = false;
		}

		// Take the full-size trace the moment it lands; show the preview until
		// then. Both go through the same blit, stretched to `bounds`.
		if (faceTrace)
		{
			if (faceTrace->fullReady.load(std::memory_order_acquire) && !faceIsFull)
			{
				faceBitmap = bitmapFromImage(g, faceTrace->full,
					faceTrace->fullWidth, faceTrace->fullHeight);
				faceSize = { faceTrace->fullWidth, faceTrace->fullHeight };
				faceIsFull = true;
			}
			else if (!faceBitmap)
			{
				faceBitmap = bitmapFromImage(g, faceTrace->preview,
					faceTrace->previewWidth, faceTrace->previewHeight);
				faceSize = { faceTrace->previewWidth, faceTrace->previewHeight };
			}
		}

		if (captionDirty || !captionBitmap)
		{
			captionBitmap = pinText.value.empty()
				? Bitmap{}
				: renderCaption(g, size, pixels, deviceScale);
			captionDirty = false;
		}
	}

public:
	PanelTestGui()
	{
		pinText.onUpdate      = [this](PinBase*) { onCaptionChanged(); };
		pinTextColor.onUpdate = [this](PinBase*) { onCaptionChanged(); };

		const auto faceChanged = [this](PinBase*) { onFaceChanged(); };
		pinRackUnits.onUpdate  = faceChanged;
		pinLayout.onUpdate     = faceChanged;
		pinMaterial.onUpdate   = faceChanged;
		pinPanelColor.onUpdate = faceChanged;
	}

	~PanelTestGui()
	{
		stopTimer();
	}

	// Polls the worker. It does not call back on purpose: invalidateRect belongs
	// to the UI thread, and a worker reaching into the editor would have to be
	// synchronised against the editor being destroyed underneath it. The worker
	// touches nothing but its own FaceTrace, which outlives both.
	bool onTimer() override
	{
		if (!faceTrace || faceTrace->fullReady.load(std::memory_order_acquire))
		{
			timerRunning = false;
			invalidate();
			return false; // returning false unregisters this client
		}
		return true;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		const Size size{ getWidth(bounds), getHeight(bounds) };
		updateBitmaps(g, size, getDeviceScale(g));

		if (faceBitmap)
		{
			// Source is the FACE bitmap's own pixel rect, which is NOT
			// bitmapSize: the preview is a fraction of it, and a capped full
			// trace is smaller too. Destination is always the DIP bounds, so
			// whichever is current gets stretched to fill the panel.
			const Rect faceSource{ 0.0f, 0.0f, (float)faceSize.width, (float)faceSize.height };
			g.drawBitmap(faceBitmap, bounds, faceSource);
		}

		if (!faceIsFull && !timerRunning)
		{
			startTimer(kPollMs);
			timerRunning = true;
		}

		if (captionBitmap)
		{
			const Rect source{ 0.0f, 0.0f, (float)bitmapSize.width, (float)bitmapSize.height };
			g.drawBitmap(captionBitmap, bounds, source);
		}

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = Register<PanelTestGui>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="SE TiDE:PanelTest" name="Panel Test" category="TiDE">
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
