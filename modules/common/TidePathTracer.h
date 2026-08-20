// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#pragma once
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

// A small Monte-Carlo path tracer for generating the CACHED bitmaps behind
// TIDE's knobs and faceplates. Two files, no dependencies beyond the standard
// library, ISC like the rest of the repo.
//
// WHY A PATH TRACER AT ALL. The look TIDE wants — brushed aluminium that turns
// as the knob turns, glass with a coloured shadow under it, plastic that is
// plastic rather than a grey gradient — is lighting, not drawing. Faking it
// with gradients means hand-tuning every highlight for every shape and getting
// it wrong the moment the shape changes. Simulating it means describing the
// material once and letting the geometry decide where the highlights land.
//
// WHY THAT IS AFFORDABLE. Every output is CACHED — rendered once into a bitmap
// and blitted from then on (see modules/TiDEPanel/TiDEPanelGui.cpp, which
// already caches procedurally generated faces exactly this way). So this code
// optimises for correctness and for looking good, never for speed.
//
// WHY SIGNED DISTANCE FIELDS RATHER THAN TRIANGLES. Panel hardware is chamfers,
// fillets, knurling and countersinks. Those are one parameter each in an SDF
// and a mesh-generation problem otherwise. It also means no mesh format, no
// mesh loader and no BVH — which is most of what makes small renderers stop
// being small.
//
// LAYOUT
//   TidePathTracer.h    this file: scene description, SDF authoring helpers
//   TidePathTracer.cpp  the integrator, the BSDFs, the sampler
//
// COMPILER NOTE. modules/CMakeLists.txt builds everything with MSVC /fp:fast
// and clang -ffast-math. Under those flags the compiler may assume no NaN and
// no infinity, so std::isnan and std::isfinite are NOT reliable and nothing
// here is allowed to depend on them. Guards are explicit range and denominator
// tests instead. This is called out again at each site that would otherwise be
// written the usual way.
namespace tide::render
{

// ---------------------------------------------------------------------------
// Vector maths
// ---------------------------------------------------------------------------

struct Vec3
{
	float x = 0.0f, y = 0.0f, z = 0.0f;

	constexpr Vec3() = default;
	constexpr Vec3(float v) : x(v), y(v), z(v) {}
	constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	constexpr Vec3 operator-() const { return { -x, -y, -z }; }
	constexpr Vec3 operator+(const Vec3& b) const { return { x + b.x, y + b.y, z + b.z }; }
	constexpr Vec3 operator-(const Vec3& b) const { return { x - b.x, y - b.y, z - b.z }; }
	// Componentwise, NOT a dot product. Colours are Vec3 and colour maths is
	// componentwise, so this is the multiplication that gets used most.
	constexpr Vec3 operator*(const Vec3& b) const { return { x * b.x, y * b.y, z * b.z }; }
	constexpr Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
	constexpr Vec3 operator/(float s) const { return { x / s, y / s, z / s }; }

	Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
	Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
	Vec3& operator*=(const Vec3& b) { x *= b.x; y *= b.y; z *= b.z; return *this; }
	Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

	constexpr float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
	constexpr float maxComponent() const { return x > y ? (x > z ? x : z) : (y > z ? y : z); }
	constexpr bool isBlack() const { return x <= 0.0f && y <= 0.0f && z <= 0.0f; }
};

constexpr Vec3 operator*(float s, const Vec3& v) { return v * s; }
constexpr float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
constexpr Vec3 cross(const Vec3& a, const Vec3& b)
{
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }
constexpr float lengthSquared(const Vec3& v) { return dot(v, v); }

// Returns the Z axis on a zero-length input rather than a NaN. Under /fp:fast a
// 0/0 here would propagate silently into the whole image.
inline Vec3 normalize(const Vec3& v)
{
	const float lenSq = dot(v, v);
	if (lenSq <= 0.0f)
		return { 0.0f, 0.0f, 1.0f };

	return v * (1.0f / std::sqrt(lenSq));
}

constexpr Vec3 minv(const Vec3& a, const Vec3& b)
{
	return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z };
}
constexpr Vec3 maxv(const Vec3& a, const Vec3& b)
{
	return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z };
}
inline Vec3 absv(const Vec3& v) { return { std::fabs(v.x), std::fabs(v.y), std::fabs(v.z) }; }
inline Vec3 expv(const Vec3& v) { return { std::exp(v.x), std::exp(v.y), std::exp(v.z) }; }
constexpr Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }
constexpr float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
constexpr float maxf(float a, float b) { return a > b ? a : b; }
constexpr float minf(float a, float b) { return a < b ? a : b; }
// Every sqrt in this renderer goes through here. Round-off routinely drives
// quantities like (1 - cos^2) a hair below zero, and /fp:fast turns the
// resulting NaN into undefined behaviour rather than a visible black pixel.
inline float safeSqrt(float v) { return std::sqrt(maxf(0.0f, v)); }

constexpr float kPi = 3.14159265358979323846f;
constexpr float kInvPi = 0.31830988618379067154f;

// ---------------------------------------------------------------------------
// Materials
// ---------------------------------------------------------------------------

enum class MaterialKind : uint8_t
{
	Diffuse,     // matte paint, paper, the room's walls
	Plastic,     // a dielectric gloss coat over a coloured diffuse base
	Metal,       // rough conductor; anisotropic when `anisotropy` is non-zero
	Glass,       // rough dielectric with Beer-Lambert absorption inside
	Emissive,    // glows AND is importance sampled via its bounding sphere — see Light below
};

// How the shading tangent is built, which is what decides which WAY a brushed
// metal is brushed. This is the whole difference between a knob that looks
// machined and one that looks like a grey ball.
enum class BrushMode : uint8_t
{
	None,        // isotropic; `anisotropy` is ignored
	Concentric,  // rings around `brushAxis` — a lathe-turned knob top
	Radial,      // spokes out from `brushAxis` — a sunburst finish
	Fixed,       // one world-space direction — a linear-brushed faceplate
};

struct Material
{
	MaterialKind kind = MaterialKind::Diffuse;

	// Diffuse/Plastic: albedo. Metal: normal-incidence reflectance (F0).
	// Glass: the colour transmitted after travelling `absorbDistance` through
	// it — an artist-facing control that is converted to an absorption
	// coefficient internally.
	Vec3 colour{ 0.8f, 0.8f, 0.8f };

	// Perceptual roughness in [0,1]; alpha = roughness^2 internally. 0 is a
	// perfect mirror / perfectly clear glass.
	float roughness = 0.25f;

	// 0 = isotropic, approaching 1 = strongly directional. Only meaningful when
	// `brush` is not None.
	//
	// KNOWN LIMITATION: single-scattering GGX loses energy as roughness and
	// anisotropy grow (no Kulla-Conty multiple-scattering compensation — a 2D
	// table is more machinery than this renderer wants). Measured: ~5% gone by
	// alpha 0.45, up to ~47% at roughness 0.45 with anisotropy 1.0, tracking
	// the LARGER of the two anisotropic alphas. The shipped recipes stay near
	// alpha 0.1 where the loss is a percent or two; push both sliders high and
	// the metal will read darker than its colour says it should.
	float anisotropy = 0.0f;
	BrushMode brush = BrushMode::None;
	Vec3 brushAxis{ 0.0f, 0.0f, 1.0f };
	Vec3 brushOrigin{ 0.0f, 0.0f, 0.0f };

	// Metal only: the reflectance at grazing incidence. Real metals tint
	// towards white at the edge; leaving this at white is the usual choice and
	// tinting it is how you tell brass from gold.
	Vec3 edgeTint{ 1.0f, 1.0f, 1.0f };

	// Glass and Plastic: index of refraction. 1.5 is window glass and is also
	// the right default for a plastic clear-coat (F0 = 0.04).
	float ior = 1.5f;

	// Glass only: the distance over which `colour` is the surviving fraction.
	// Zero disables absorption and gives water-clear glass. Making this small
	// relative to the object deepens the colour — this is what produces a
	// COLOURED SHADOW rather than a grey one.
	float absorbDistance = 0.0f;

	// Emissive only. Radiance, not power: a bigger light at the same emission
	// is a brighter scene.
	Vec3 emission{ 0.0f, 0.0f, 0.0f };

	// Hides an object from PRIMARY rays only. The enclosing room is built with
	// this false, so it lights the subject and appears in its reflections while
	// leaving the bitmap's background transparent. See Scene::room below.
	bool cameraVisible = true;
};

// ---------------------------------------------------------------------------
// Material recipes
//
// Named materials that already look right, so a scene can say what a thing is
// made of instead of specifying a BSDF. Reach for these first; drop to raw
// Material only when a recipe is close but not quite.
//
// The metal colours are measured normal-incidence reflectances in LINEAR sRGB.
// They are not free parameters and are not "the colour of the metal" in a
// paint sense — a metal's colour IS its Fresnel curve, and these values are what
// make copper read as copper rather than as orange plastic. Change the
// roughness to change the finish; leave the colour alone.
// ---------------------------------------------------------------------------

namespace recipes
{

// --- metals ----------------------------------------------------------------

inline Material metal(Vec3 reflectance, float roughness)
{
	Material m;
	m.kind = MaterialKind::Metal;
	m.colour = reflectance;
	m.roughness = roughness;
	return m;
}

// Turn any metal recipe into a BRUSHED one. `mode` picks the grain: Concentric
// for a lathe-turned knob top, Fixed for a linear-brushed faceplate.
//
// The strength defaults high because subtle anisotropy just reads as "slightly
// blurry" — the effect only becomes legible as brushing once the highlight is
// clearly stretched.
inline Material brushed(Material m, BrushMode mode, Vec3 axis = { 0.0f, 0.0f, 1.0f },
	Vec3 origin = { 0.0f, 0.0f, 0.0f }, float strength = 0.85f)
{
	m.anisotropy = strength;
	m.brush = mode;
	m.brushAxis = axis;
	m.brushOrigin = origin;
	return m;
}

inline Material polishedAluminium(float roughness = 0.06f)
{
	return metal({ 0.912f, 0.914f, 0.920f }, roughness);
}

// The default TIDE knob material: turned aluminium, grain running around the
// axis. Every light in the room smears into a ring, which is exactly what a
// real lathe-turned knob does.
inline Material brushedAluminium(Vec3 axis = { 0.0f, 0.0f, 1.0f },
	Vec3 origin = { 0.0f, 0.0f, 0.0f }, float roughness = 0.22f)
{
	return brushed(metal({ 0.912f, 0.914f, 0.920f }, roughness),
		BrushMode::Concentric, axis, origin);
}

inline Material chrome(float roughness = 0.03f)
{
	return metal({ 0.550f, 0.556f, 0.554f }, roughness);
}

inline Material stainlessSteel(float roughness = 0.20f)
{
	return metal({ 0.562f, 0.565f, 0.578f }, roughness);
}

inline Material brass(float roughness = 0.18f)
{
	return metal({ 0.910f, 0.778f, 0.423f }, roughness);
}

inline Material copper(float roughness = 0.14f)
{
	return metal({ 0.955f, 0.638f, 0.538f }, roughness);
}

inline Material gold(float roughness = 0.10f)
{
	return metal({ 1.000f, 0.766f, 0.336f }, roughness);
}

inline Material titanium(float roughness = 0.28f)
{
	return metal({ 0.542f, 0.497f, 0.449f }, roughness);
}

// Black anodising is a dark, slightly rough metal rather than black paint: it
// keeps the metal's grazing brightness, which is what stops it looking like
// plastic.
inline Material anodisedBlack(float roughness = 0.30f)
{
	Material m = metal({ 0.115f, 0.118f, 0.125f }, roughness);
	m.edgeTint = { 0.85f, 0.86f, 0.90f };
	return m;
}

// --- plastics --------------------------------------------------------------

inline Material plastic(Vec3 colour, float roughness)
{
	Material m;
	m.kind = MaterialKind::Plastic;
	m.colour = colour;
	m.roughness = roughness;
	m.ior = 1.5f;
	return m;
}

// Injection-moulded, straight out of the tool. The default for knob caps and
// button tops.
inline Material glossyPlastic(Vec3 colour) { return plastic(colour, 0.10f); }

// Bead-blasted or lightly textured: the finish most synth hardware actually
// has, because a full gloss shows every fingerprint.
inline Material satinPlastic(Vec3 colour) { return plastic(colour, 0.28f); }

inline Material mattePlastic(Vec3 colour) { return plastic(colour, 0.55f); }

// Soft-touch rubber: rough enough that the coat highlight is a broad sheen
// rather than a spot.
inline Material rubber(Vec3 colour) { return plastic(colour, 0.75f); }

// --- glass -----------------------------------------------------------------

inline Material glass(Vec3 transmitted, float absorbDistance, float roughness = 0.0f)
{
	Material m;
	m.kind = MaterialKind::Glass;
	m.colour = transmitted;
	m.absorbDistance = absorbDistance;
	m.roughness = roughness;
	m.ior = 1.52f;
	return m;
}

inline Material clearGlass() { return glass({ 1.0f, 1.0f, 1.0f }, 0.0f, 0.0f); }

// `depth` is the distance over which the tint reaches full strength, so it
// should be somewhere near the thickness of the object. Setting it much SMALLER
// than the object deepens the colour and darkens the shadow; much larger and
// the glass looks merely tinted.
inline Material tintedGlass(Vec3 tint, float depth = 1.0f)
{
	return glass(tint, depth, 0.0f);
}

// Acid-etched / sandblasted. Roughness on glass costs a lot of samples to
// converge, so keep it low unless the render can afford the time.
inline Material frostedGlass(Vec3 tint = { 1.0f, 1.0f, 1.0f }, float roughness = 0.18f)
{
	return glass(tint, 0.0f, roughness);
}

// --- everything else -------------------------------------------------------

// Flat paint. Panel silkscreen, paper, the inside of a room.
inline Material paint(Vec3 colour)
{
	Material m;
	m.kind = MaterialKind::Diffuse;
	m.colour = colour;
	return m;
}

// A glowing surface: an LED, a backlit legend, a glowing cube.
//
// `strength` is radiance, so it is unbounded and values well above 1 are normal
// — an LED that reads as ON against a lit panel needs to be several times
// brighter than the panel, not slightly brighter.
//
// An emissive OBJECT lights the room properly as long as it has a finite
// bounding sphere set, because the renderer aims shadow rays at that sphere.
// Leave Object::boundsRadius at its default and it will still glow, but it will
// only light things by chance and will be noisy.
inline Material glow(Vec3 colour, float strength = 6.0f)
{
	Material m;
	m.kind = MaterialKind::Emissive;
	m.emission = colour * strength;
	m.colour = colour;
	return m;
}

}

// ---------------------------------------------------------------------------
// Signed distance field authoring helpers
//
// Distance is NEGATIVE inside, POSITIVE outside, and — for the ones marked
// exact — equal to the true Euclidean distance to the surface. Sphere tracing
// only requires a LOWER BOUND on that distance, so the inexact ones are still
// safe; they just cost extra steps.
//
// These are the standard closed forms popularised by Inigo Quilez, whose
// published SDF code is MIT licensed and therefore compatible with ISC. They
// have been rewritten against this file's Vec3 rather than copied verbatim.
// ---------------------------------------------------------------------------

inline float sdSphere(const Vec3& p, float radius)
{
	return length(p) - radius;
}

// Exact. `halfExtents` is half the side length on each axis, so a 2x2x2 cube is
// halfExtents = 1.
inline float sdBox(const Vec3& p, const Vec3& halfExtents)
{
	const Vec3 q = absv(p) - halfExtents;
	// Outside distance is the length of the positive part; inside distance is
	// the largest (least negative) component. Summing them is correct because
	// only one of the two is ever non-zero.
	const float outside = length(maxv(q, Vec3{ 0.0f }));
	const float inside = minf(maxf(q.x, maxf(q.y, q.z)), 0.0f);
	return outside + inside;
}

// A box with its edges filleted. `radius` is subtracted from the extents, so
// the overall size is unchanged — which is what you want when rounding a
// faceplate that has to stay the same size.
inline float sdRoundBox(const Vec3& p, const Vec3& halfExtents, float radius)
{
	return sdBox(p, halfExtents - Vec3{ radius }) - radius;
}

// A box with its edges CUT at 45 degrees rather than rounded. Visually very
// different from a fillet under a hard light: a chamfer produces one crisp
// highlight line per edge, a fillet produces a smeared band. Panel hardware is
// usually chamfered.
inline float sdChamferBox(const Vec3& p, const Vec3& halfExtents, float chamfer)
{
	const float box = sdBox(p, halfExtents);

	// A chamfer is a cut along the EDGES, so it takes one 45-degree plane per
	// edge direction: the XY-edge plane has normal (1,1,0)/sqrt(2), and |p|
	// folds the four parallel edges into one test. Three such planes cover all
	// twelve edges, and where they meet they trim the corners by themselves.
	//
	// The first version of this used the single corner plane
	// (|x|+|y|+|z| - sum + c)/sqrt(3) instead — which cuts only the EIGHT
	// CORNERS and leaves every edge knife sharp: at an edge midpoint the third
	// coordinate is ~0, so that plane sits far below the surface and the max()
	// always returns the uncut box. A "chamfered" faceplate rendered with no
	// chamfer highlight anywhere, which is the one job this function has.
	const Vec3 a = absv(p);
	constexpr float invSqrt2 = 0.70710678f;
	const float exy = (a.x + a.y - (halfExtents.x + halfExtents.y) + chamfer) * invSqrt2;
	const float eyz = (a.y + a.z - (halfExtents.y + halfExtents.z) + chamfer) * invSqrt2;
	const float ezx = (a.z + a.x - (halfExtents.z + halfExtents.x) + chamfer) * invSqrt2;

	return maxf(maxf(box, exy), maxf(eyz, ezx));
}

// Exact. A cylinder along the Z axis — Z-up matches the orthographic front view
// this renderer is built for, so a knob body needs no rotation.
inline float sdCylinder(const Vec3& p, float radius, float halfHeight)
{
	const float d = length(Vec3{ p.x, p.y, 0.0f }) - radius;
	const float h = std::fabs(p.z) - halfHeight;
	const float outside = length(maxv(Vec3{ d, h, 0.0f }, Vec3{ 0.0f }));
	const float inside = minf(maxf(d, h), 0.0f);
	return outside + inside;
}

// A cylinder whose top and bottom rims are filleted by `radius`. The workhorse
// for knob bodies: the fillet is what catches the light and gives the rim its
// bright edge.
inline float sdRoundCylinder(const Vec3& p, float radius, float halfHeight, float round)
{
	return sdCylinder(p, radius - round, halfHeight - round) - round;
}

// A cylinder whose rims are chamfered instead. `chamfer` is the 45-degree cut
// measured along each axis.
inline float sdChamferCylinder(const Vec3& p, float radius, float halfHeight, float chamfer)
{
	const float body = sdCylinder(p, radius, halfHeight);
	const float rho = length(Vec3{ p.x, p.y, 0.0f });
	const float cut = (rho + std::fabs(p.z) - (radius + halfHeight) + chamfer) * 0.70710678f;
	return maxf(body, cut);
}

// Exact. A torus in the XY plane, `major` from the axis to the tube centre and
// `minor` the tube radius.
inline float sdTorus(const Vec3& p, float major, float minor)
{
	const float q = length(Vec3{ p.x, p.y, 0.0f }) - major;
	return length(Vec3{ q, p.z, 0.0f }) - minor;
}

// A capped cone along Z. Bounded, not exact.
inline float sdCappedCone(const Vec3& p, float radiusBottom, float radiusTop, float halfHeight)
{
	const float rho = length(Vec3{ p.x, p.y, 0.0f });
	const Vec3 k1{ radiusTop, halfHeight, 0.0f };
	const Vec3 k2{ radiusTop - radiusBottom, 2.0f * halfHeight, 0.0f };
	const Vec3 q{ rho, p.z, 0.0f };
	const Vec3 ca{ q.x - minf(q.x, (q.y < 0.0f) ? radiusBottom : radiusTop),
		std::fabs(q.y) - halfHeight, 0.0f };
	const float k2LenSq = maxf(dot(k2, k2), 1e-20f);
	const Vec3 cb = q - k1 + k2 * clampf(dot(k1 - q, k2) / k2LenSq, 0.0f, 1.0f);
	const float s = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f;
	return s * safeSqrt(minf(dot(ca, ca), dot(cb, cb)));
}

// An infinite plane through the origin. `normal` must be unit length.
inline float sdPlane(const Vec3& p, const Vec3& normal, float offset)
{
	return dot(p, normal) + offset;
}

// A hollow ROOM: the distance from a point inside to the nearest wall, positive
// throughout the interior and zero at the walls.
//
// Note the sign, which is the opposite of every other function here and is the
// whole point. The others describe SOLIDS, so they are negative inside. A room
// is the empty space itself, so a ray flying around inside it must see POSITIVE
// distance-to-wall or the tracer would believe it had started buried in
// something and refuse to move.
//
// Implemented as the exact complement of the solid box, so it is a true
// Lipschitz-1 field EVERYWHERE — inside the room it equals the min distance to
// a wall, and outside the box it goes correctly negative (that region is the
// room's "solid"). An earlier version computed min(halfExtents - |p|) directly,
// which is identical inside but overstates how deep an outside point is buried;
// the complement form costs the same and has no invalid region, so a camera
// accidentally placed outside a visible room fails loudly at the wall instead
// of undefined-ly.
inline float sdRoomInterior(const Vec3& p, const Vec3& halfExtents)
{
	return -sdBox(p, halfExtents);
}

// --- combination -----------------------------------------------------------

inline float opUnion(float a, float b) { return minf(a, b); }
inline float opIntersect(float a, float b) { return maxf(a, b); }
// Subtracts `b` from `a`.
inline float opSubtract(float a, float b) { return maxf(a, -b); }

// Polynomial smooth minimum: a fillet of radius ~k where the two shapes meet.
// Beware — this UNDERSTATES the distance near the joint, so it is not
// Lipschitz-1 there and an aggressive k can let the sphere tracer step through
// thin features. Keep k small relative to the shapes being joined.
inline float opSmoothUnion(float a, float b, float k)
{
	if (k <= 0.0f)
		return minf(a, b);

	const float h = clampf(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
	return b + (a - b) * h - k * h * (1.0f - h);
}

inline float opSmoothSubtract(float a, float b, float k)
{
	if (k <= 0.0f)
		return maxf(a, -b);

	const float h = clampf(0.5f - 0.5f * (a + b) / k, 0.0f, 1.0f);
	return a + (-b - a) * h + k * h * (1.0f - h);
}

// --- domain operations -----------------------------------------------------

// Rotates `p` about the Z axis so that the region can be modelled once and
// repeated `count` times around the axis. This is how knurling and detents get
// made: build ONE groove near +X and stamp it around the rim.
//
// The seam problem: naive polar repetition leaves a discontinuity where the
// last sector meets the first, which shows up as a visible crack. Folding the
// angle into a half-sector about the sector centre (rather than wrapping it)
// makes the field symmetric across every boundary and removes the seam.
inline Vec3 opPolarRepeat(const Vec3& p, int count)
{
	if (count < 1)
		return p;

	const float sector = 2.0f * kPi / (float)count;
	float angle = std::atan2(p.y, p.x);
	const float rho = length(Vec3{ p.x, p.y, 0.0f });

	// Fold to the nearest sector centre; the remainder is in [-sector/2, +sector/2].
	angle -= sector * std::round(angle / sector);

	return { rho * std::cos(angle), rho * std::sin(angle), p.z };
}

// --- rigid transform -------------------------------------------------------

// An orthonormal frame plus a translation and a UNIFORM scale.
//
// Uniform only, deliberately. Sphere tracing needs the field to be Lipschitz-1
// and a non-uniform scale multiplies distances differently per axis, so the
// returned value stops bounding the true distance and the tracer overshoots
// through surfaces. Model the shape at its real proportions instead.
struct Transform
{
	Vec3 origin{ 0.0f, 0.0f, 0.0f };
	Vec3 axisX{ 1.0f, 0.0f, 0.0f };
	Vec3 axisY{ 0.0f, 1.0f, 0.0f };
	Vec3 axisZ{ 0.0f, 0.0f, 1.0f };
	float scale = 1.0f;

	// World point -> local point.
	Vec3 toLocal(const Vec3& p) const
	{
		const Vec3 d = p - origin;
		return Vec3{ dot(d, axisX), dot(d, axisY), dot(d, axisZ) } * (1.0f / scale);
	}

	// The matching correction for a distance evaluated in local space.
	float toWorldDistance(float localDistance) const { return localDistance * scale; }
};

// Rotation about an arbitrary unit axis, as a frame. Degrees, because scene
// code reads better that way.
Transform makeTransform(const Vec3& origin, const Vec3& axis, float degrees, float scale = 1.0f);

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

// One object is one distance function plus one material. CSG BETWEEN objects is
// a plain union, which is what you want anyway once the parts have different
// materials; CSG WITHIN an object is done inside its own lambda with the op*
// helpers above.
//
// std::function costs an indirect call per sphere-tracing step. That is the
// single biggest cost in this renderer and it is accepted knowingly: it buys a
// scene description that is ordinary C++, so a knob is a function rather than a
// data format with a parser. `bounds` is what keeps it affordable — see below.
struct Object
{
	std::function<float(const Vec3&)> distance;
	Material material;

	// A bounding SPHERE in world space that must fully contain the object.
	// Rays that miss it skip the object's distance function entirely, which is
	// what stops an eight-object scene from costing eight evaluations at every
	// one of a hundred marching steps.
	//
	// Getting this WRONG is the one way to produce silently clipped geometry,
	// so it defaults to effectively infinite: correct but slow until set.
	Vec3 boundsCentre{ 0.0f, 0.0f, 0.0f };
	float boundsRadius = 1.0e9f;

	// Set for the room, which must not have a bounding sphere applied at all.
	bool unbounded = false;
};

// ---------------------------------------------------------------------------
// Lights
//
// Lights are ANALYTIC — a rectangle or a sphere — and are deliberately not
// Objects. The reason is next-event estimation: at every bounce the integrator
// wants to pick a point ON a light and connect to it, and there is no general
// way to pick a uniform point on an arbitrary signed distance field. Restricting
// the primary emitters to two shapes that can be sampled in closed form is what
// turns a noisy renderer into a quiet one, and it costs nothing in practice
// because a window is a rectangle and a lamp is a sphere.
//
// An SDF object with MaterialKind::Emissive is ALSO importance sampled, through
// the one closed-form proxy every object already carries: its bounding sphere.
// The integrator aims shadow rays at the cone that sphere subtends and keeps
// only the ones that genuinely land on the object — unbiased, at the cost of
// the cone samples that miss. So a glowing cube really lights the room, and how
// WELL it does depends directly on how snug its boundsRadius is: a padded bound
// inflates the cone and most of its samples miss. A light that is a plain
// rectangle or sphere should still be a Light — its samples never miss.
// ---------------------------------------------------------------------------

enum class LightShape : uint8_t
{
	Rect,    // a window or a softbox; emits from one face only by default
	Sphere,  // a lamp; emits in all directions
};

struct Light
{
	LightShape shape = LightShape::Rect;

	Vec3 position{ 0.0f, 0.0f, 0.0f };

	// Rect: the two half-extent vectors spanning the rectangle. Their cross
	// product is the emitting normal, so they also orient it. Sphere: unused.
	Vec3 halfU{ 1.0f, 0.0f, 0.0f };
	Vec3 halfV{ 0.0f, 1.0f, 0.0f };

	float radius = 0.25f; // Sphere only

	// Radiance, not power. Doubling the size of a rect at fixed emission
	// doubles the light in the room, which is how real windows behave.
	Vec3 emission{ 1.0f, 1.0f, 1.0f };

	// Rect only. One-sided by default so a window set into a wall does not also
	// illuminate the far side of that wall.
	bool twoSided = false;

	// Lights are hidden from primary rays for the same reason the room is: the
	// subject needs a transparent background. Turn it on to see the softbox.
	bool cameraVisible = false;
};

struct Scene
{
	std::vector<Object> objects;
	std::vector<Light> lights;

	// Radiance returned by rays that leave the scene entirely. With a closed
	// room this is never seen directly and only matters if the room is removed.
	Vec3 background{ 0.0f, 0.0f, 0.0f };

	void add(Object o) { objects.push_back(std::move(o)); }
	void add(const Light& l) { lights.push_back(l); }
};

// --- the studio ------------------------------------------------------------

// The enclosing environment. Metal and glass are almost entirely a picture OF
// THEIR SURROUNDINGS, so with nothing around them they render as flat grey no
// matter how good the BSDF is. This builds that surrounding procedurally — no
// bitmap, no HDRI file.
//
// The orthographic problem this exists to solve: a front-on orthographic camera
// looking at a FLAT metal face gives every pixel the same view direction and
// the same normal, so the reflection is one constant colour across the whole
// face. Curvature, chamfers and brushed anisotropy are what break that up, and
// large rectangular sources are what make the brushing legible — an anisotropic
// highlight is a SMEAR OF THE SOURCE, so a point source smears into a thin line
// and a window smears into a broad, readable sheen.
struct Studio
{
	// Half-extents of the room the subject sits in.
	Vec3 roomHalfExtents{ 6.0f, 6.0f, 6.0f };
	Vec3 wallColour{ 0.55f, 0.55f, 0.58f };
	Vec3 floorColour{ 0.22f, 0.22f, 0.24f };

	// Every light is a rectangle AIMED AT `aim`, described by where it is and
	// how big it is. Aiming is automatic because a softbox that is not pointed
	// at the subject is just a wall.
	Vec3 aim{ 0.0f, 0.0f, 0.0f };

	// KEY: large, soft, slightly cool, high and to the left. Large is the
	// operative word — an anisotropic highlight is a SMEAR of the source along
	// the brushing direction, so a big rectangle smears into a broad readable
	// sheen and a point smears into a hairline that reads as a scratch.
	Vec3 keyPosition{ -3.6f, 3.0f, 3.4f };
	float keyHalfWidth = 2.6f;
	float keyHalfHeight = 2.6f;
	Vec3 keyEmission{ 5.4f, 5.6f, 6.2f };

	// FILL: dimmer, warmer, opposite the key. Its job is to keep the shadow
	// side from going to black, and its warmth against the key's coolness is
	// most of what makes a grey object look like it is somewhere real.
	Vec3 fillPosition{ 4.0f, -1.0f, 2.6f };
	float fillHalfWidth = 2.2f;
	float fillHalfHeight = 2.2f;
	Vec3 fillEmission{ 1.45f, 1.30f, 1.10f };

	// RIM: a narrow, bright strip behind and above, to put a bright line along
	// the top chamfer and lift the subject off the background.
	Vec3 rimPosition{ 0.6f, 3.4f, -2.6f };
	float rimHalfWidth = 2.0f;
	float rimHalfHeight = 0.35f;
	Vec3 rimEmission{ 6.5f, 6.6f, 7.0f };

	// GLINT: a small intense lamp for the hard sparkle in fillets and chamfers.
	// Small AND intense rather than large and dim: a glint is a mirror image of
	// the source, so shrinking it is what makes it a spark instead of a patch.
	//
	// Sized and dimmed against FIREFLIES. The obvious setting — tiny and
	// blazing — gives the crispest sparkle and also the worst noise: glass and
	// polished metal reach it only by specular paths, which carry no MIS
	// weight, so a single lucky sample lands a huge spike that the clamp then
	// has to cut. Radius 0.35 at emission 26 keeps the sparkle and makes the
	// lamp big enough to be found reliably.
	Vec3 glintPosition{ 1.9f, 3.2f, 3.0f };
	float glintRadius = 0.35f;
	Vec3 glintEmission{ 26.0f, 26.0f, 27.0f };

	bool enableRoom = true;
	bool enableRim = true;
	bool enableGlint = true;
};

// Appends the room and its lights to `scene`. Everything it adds is
// cameraVisible = false, so the subject keeps a transparent background while
// still being lit by, and reflecting, all of it.
void addStudio(Scene& scene, const Studio& studio = {});

// ---------------------------------------------------------------------------
// Camera and settings
// ---------------------------------------------------------------------------

// Orthographic, because panel artwork has to tile and align: a perspective
// knob only looks right at the exact spot the camera was pointed, and every
// other instance on the panel looks wrong.
struct Camera
{
	Vec3 position{ 0.0f, 0.0f, 6.0f };
	Vec3 target{ 0.0f, 0.0f, 0.0f };
	Vec3 up{ 0.0f, 1.0f, 0.0f };

	// The width of the visible film in WORLD units. Height follows from the
	// pixel aspect ratio, so square pixels stay square.
	float filmWidth = 2.4f;

	// How far back the ray origin is pushed before marching. An orthographic
	// ray has no natural origin and must start outside everything it could hit,
	// including the room.
	float nearPullback = 20.0f;
};

struct Settings
{
	int width = 256;
	int height = 256;

	// Paths per pixel. 32 is a usable preview, 512 is clean, and a glass object
	// with a coloured caustic wants more — the caustic arrives only by chance
	// through BSDF sampling and is the last thing in the image to converge.
	int samplesPerPixel = 256;

	// Path length. 8 is enough for a knob; glass needs more because every
	// surface of the object costs two (in and out).
	int maxBounces = 12;

	// There is deliberately no exposure here. Exposure is an ENCODE-time knob —
	// it belongs to writePixels, where it actually acts — and an early version
	// that also carried the field here shipped the classic trap: render()
	// ignored it, so setting it did nothing unless the caller separately passed
	// the same number to writePixels. One owner, no lie.

	// Per-sample radiance clamp, for firefly suppression. This BIASES the
	// result — it removes energy and will slightly dim genuine caustics — but
	// unclamped Monte Carlo puts single blazing pixels in an otherwise clean
	// image, and one bad pixel is more objectionable in a 64-pixel knob than a
	// percent of missing energy. Raise it if caustics look weak; 0 disables.
	float clampRadiance = 12.0f;

	uint32_t seed = 0x9E3779B9u;

	// 0 means "use the hardware concurrency". Tiles are independent and each
	// gets its own RNG stream seeded from its coordinates, so the image is
	// DETERMINISTIC regardless of thread count — a render is reproducible and
	// diffable, which matters when the output is committed as a cached asset.
	int threads = 0;
};

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

// LINEAR, PREMULTIPLIED, high dynamic range RGBA. Not tone mapped and not
// encoded — those are lossy, and different consumers want them applied
// differently, so the render stays neutral and `writePixels` produces whatever
// a given target needs from the same buffer.
//
// Premultiplied because that is simply what a Monte Carlo average IS. A pixel
// on the silhouette fires some rays that hit the subject and some that sail
// past into the transparent background; averaging over ALL of them — counting
// the misses as zero — yields colour-times-coverage, which is the definition of
// premultiplied. Dividing it back out here would only add a division by a
// possibly-zero alpha for the benefit of code that mostly wants to multiply it
// straight back in.
struct Image
{
	int width = 0;
	int height = 0;
	std::vector<float> rgba; // width * height * 4, row-major, top row first

	Vec3 pixel(int x, int y) const
	{
		const size_t i = ((size_t)y * (size_t)width + (size_t)x) * 4;
		return { rgba[i], rgba[i + 1], rgba[i + 2] };
	}
	float alpha(int x, int y) const
	{
		return rgba[((size_t)y * (size_t)width + (size_t)x) * 4 + 3];
	}
};

Image render(const Scene& scene, const Camera& camera, const Settings& settings);

// Byte order of the destination. GMPI hands back BGRA on Windows and RGBA on
// macOS from the same call, so this is a run-time property of the target
// bitmap, not a compile-time one (TiDEPanelGui.cpp reads it from
// pixels.channelLayout()).
enum class PixelOrder : uint8_t { Rgba, Bgra };

// Tone maps, encodes to sRGB, and writes 8-bit pixels.
//
// `premultiply` multiplies the ENCODED (sRGB) values by alpha rather than the
// linear ones. That is not what physics would say, but it is what Direct2D and
// CoreGraphics mean by a premultiplied sRGB surface, and matching the
// compositor matters more than matching the physics — premultiplying in linear
// and then encoding leaves a visible dark fringe around every antialiased edge.
// Pass false for PNG, whose format stores unassociated alpha.
// `whitePoint` is the linear value that tone maps to 1.0. Raising it keeps more
// highlight detail at the cost of a duller midtone; setting it very high is
// effectively "no tone mapping, just clip", which is what you want if the
// bitmap has to colour-match a flat UI palette exactly.
void writePixels(const Image& image, uint8_t* dst, int32_t bytesPerRow,
	PixelOrder order, bool premultiply, float exposure = 1.0f, float whitePoint = 4.0f);

}
