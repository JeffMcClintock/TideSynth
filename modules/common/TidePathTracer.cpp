// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "TidePathTracer.h"
#include <algorithm>
#include <atomic>
#include <thread>

// The integrator, the BSDFs and the sampler. See TidePathTracer.h for what this
// is and why it exists.
//
// Everything here is written against published formulae rather than copied from
// an existing renderer, so that the whole thing can stay ISC. The sources are
// named at each site: Heitz's visible-normal sampling, Smith's height-correlated
// masking term, and the Trowbridge-Reitz (GGX) distribution are all from papers,
// and a formula is not a copyrightable expression.
//
// READ THIS FIRST if you are changing the maths. There are two conventions used
// throughout and mixing them up accounts for most rough-surface bugs:
//
//   * `wo` is the OUTGOING direction, pointing away from the surface towards
//     where the ray came from (the camera, for the first bounce). `wi` is the
//     INCOMING direction being sampled. Both point AWAY from the surface.
//   * Fresnel is evaluated against the MICROFACET normal `wh`, never against the
//     geometric normal. On a rough surface those differ, and using the geometric
//     normal produces a metal that is too dark at the edges and glass with no
//     grazing sheen.
namespace tide::render
{
namespace
{

// ---------------------------------------------------------------------------
// Tuning constants
// ---------------------------------------------------------------------------

// The distance below which a sphere-traced ray is considered to have arrived.
// Scene units are "about one" for a knob, so this is roughly a thirtieth of a
// pixel at the resolutions these bitmaps are rendered at.
constexpr float kSurfaceEpsilon = 1.0e-4f;

// How far a secondary ray is pushed off the surface before it starts marching.
// An SDF needs a bigger nudge than a triangle mesh does: a mesh has an exact
// plane to sit on, whereas here the surface is wherever the field happens to
// cross zero, and starting a march at a point whose distance is already ~0
// leaves the tracer taking zero-length steps forever.
constexpr float kRayOffset = 6.0e-4f;

// A ray that exhausts this is reported as a MISS, and that is a trap worth
// knowing before you meet it: a miss on a primary ray means alpha 0, so the
// pixel comes out TRANSPARENT and whatever is behind the bitmap shows through.
// It reads as a hole in the model rather than as a tracing failure.
//
// It bites when a ray travels nearly PARALLEL to a surface, because then the
// distance to that surface -- which is the step size -- stays tiny for the
// whole journey. A hole bored straight through a plate is the classic case:
// rays entering near the wall crawl and never reach the far side. TIDE's panel
// hit this on every LED and jack hole (modules/TiDEPanel, see taperedCut) and
// the fix was geometric, not numeric -- taper the bore so the wall recedes
// from the ray and the step size can grow.
//
// Raising this number is the wrong first move: it makes every ray in the scene
// pay for a case that geometry can remove.
constexpr int kMaxMarchSteps = 320;
constexpr float kMaxTraceDistance = 400.0f;

// Below this roughness a surface is treated as perfectly smooth: sampled as a
// mirror, excluded from next-event estimation, and given an MIS weight of one.
// Rough-surface maths degenerates as alpha approaches zero (the distribution
// becomes a delta and its pdf becomes unbounded), so there has to be a cutoff
// somewhere, and doing it explicitly is better than watching the variance
// explode.
constexpr float kSmoothThreshold = 2.0e-3f;

// ---------------------------------------------------------------------------
// Random numbers
//
// PCG32 — a 64-bit LCG whose output is permuted before being truncated to 32
// bits. Implemented from the algorithm as published by Melissa O'Neill (the
// reference implementation is Apache-2.0 / MIT, and the construction itself is
// described in her paper). Chosen over std::mt19937 because it seeds cheaply,
// which matters when every pixel gets its own independent stream.
// ---------------------------------------------------------------------------

struct Rng
{
	uint64_t state = 0x853c49e6748fea9bULL;
	uint64_t inc = 0xda3e39cb94b95bdbULL;

	uint32_t nextBits()
	{
		const uint64_t old = state;
		state = old * 6364136223846793005ULL + inc;
		const uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
		const uint32_t rot = (uint32_t)(old >> 59u);
		return (xorshifted >> rot) | (xorshifted << ((0u - rot) & 31u));
	}

	// Half open [0,1). The 24-bit mantissa is filled exactly, so the result can
	// never round up to 1.0f — which it can if you divide by 0xFFFFFFFF.
	float next() { return (float)(nextBits() >> 8) * (1.0f / 16777216.0f); }
};

// Seeds a stream from a pixel and a sample index. Every pixel is independent of
// thread scheduling, which is what makes a render reproducible.
Rng seedRng(uint32_t x, uint32_t y, uint32_t sample, uint32_t seed)
{
	// SplitMix64 finaliser: cheap, and good enough avalanche that neighbouring
	// pixels do not produce visibly correlated noise.
	uint64_t z = ((uint64_t)y << 32 | x) + 0x9E3779B97F4A7C15ULL * (sample + 1u) + seed;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	z ^= (z >> 31);

	Rng rng;
	rng.state = z;
	rng.inc = (z << 1) | 1u; // must be odd or the LCG loses half its period
	rng.nextBits();
	return rng;
}

// ---------------------------------------------------------------------------
// Shading frames and sampling
// ---------------------------------------------------------------------------

// An orthonormal basis. `toLocal` puts the surface normal on +Z, which is the
// space every BSDF below works in.
struct Frame
{
	Vec3 x{ 1.0f, 0.0f, 0.0f };
	Vec3 y{ 0.0f, 1.0f, 0.0f };
	Vec3 z{ 0.0f, 0.0f, 1.0f };

	Vec3 toLocal(const Vec3& v) const { return { dot(v, x), dot(v, y), dot(v, z) }; }
	Vec3 toWorld(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
};

// Branchless orthonormal basis from a unit normal (Duff et al., JCGT 2017).
// The copysign trick is what avoids the singularity that the naive
// "cross with whichever axis is least aligned" construction has.
Frame makeFrame(const Vec3& n)
{
	Frame f;
	f.z = n;
	const float sign = (n.z >= 0.0f) ? 1.0f : -1.0f;
	const float a = -1.0f / (sign + n.z);
	const float b = n.x * n.y * a;
	f.x = { 1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x };
	f.y = { b, sign + n.y * n.y * a, -n.y };
	return f;
}

// As above, but with the tangent forced to a preferred direction. This is what
// makes brushed metal directional: the anisotropic highlight lies along the
// tangent, so choosing the tangent chooses the brushing.
//
// `preferred` is projected onto the tangent plane and renormalised. If it is
// parallel to the normal there is no valid tangent — that is the centre of a
// concentrically brushed knob — and the caller gets an arbitrary but stable
// frame instead of a NaN.
Frame makeFrameWithTangent(const Vec3& n, const Vec3& preferred)
{
	const Vec3 projected = preferred - n * dot(preferred, n);
	const float lenSq = dot(projected, projected);
	if (lenSq < 1.0e-8f)
		return makeFrame(n);

	Frame f;
	f.z = n;
	f.x = projected * (1.0f / std::sqrt(lenSq));
	f.y = cross(n, f.x);
	return f;
}

// Concentric mapping of a unit square to a unit disc (Shirley & Chiu). Better
// than the polar mapping because it preserves the stratification of the input,
// so stratified samples stay stratified after the transform.
void sampleConcentricDisc(float u1, float u2, float& dx, float& dy)
{
	const float a = 2.0f * u1 - 1.0f;
	const float b = 2.0f * u2 - 1.0f;
	if (a == 0.0f && b == 0.0f)
	{
		dx = dy = 0.0f;
		return;
	}

	float r, theta;
	if (std::fabs(a) > std::fabs(b))
	{
		r = a;
		theta = (kPi * 0.25f) * (b / a);
	}
	else
	{
		r = b;
		theta = (kPi * 0.5f) - (kPi * 0.25f) * (a / b);
	}

	dx = r * std::cos(theta);
	dy = r * std::sin(theta);
}

// Cosine-weighted hemisphere about +Z. pdf = cosTheta / pi.
Vec3 sampleCosineHemisphere(float u1, float u2)
{
	float dx, dy;
	sampleConcentricDisc(u1, u2, dx, dy);
	return { dx, dy, safeSqrt(1.0f - dx * dx - dy * dy) };
}

// ---------------------------------------------------------------------------
// Fresnel
// ---------------------------------------------------------------------------

// Exact unpolarised Fresnel for a dielectric.
//
// `cosThetaI` is measured against the microfacet normal and may be negative,
// which means the ray is leaving the denser medium; the relative index is
// inverted in that case. `eta` is always given as (inside / outside).
// Returns 1 on total internal reflection.
float fresnelDielectric(float cosThetaI, float eta)
{
	cosThetaI = clampf(cosThetaI, -1.0f, 1.0f);
	if (cosThetaI < 0.0f)
	{
		eta = 1.0f / eta;
		cosThetaI = -cosThetaI;
	}

	// cos_t^2 = ((eta - 1)(eta + 1) + cos_i^2) / eta^2.
	//
	// Algebraically identical to the familiar 1 - (1 - cos_i^2)/eta^2, but with
	// no subtraction of nearly equal numbers. The textbook form computes
	// 1 - cos_i^2 at grazing incidence, where cos_i^2 underflows against the 1
	// and the difference keeps only a handful of bits — an absolute error of
	// about 2.4e-4 in cos_t, which lands exactly where the Fresnel curve is
	// steepest and where a knob's rim highlight lives. This form is also exact
	// when eta == 1, and the sign of its numerator IS the TIR test.
	const float cos2ThetaTNumerator = (eta - 1.0f) * (eta + 1.0f) + cosThetaI * cosThetaI;
	if (cos2ThetaTNumerator <= 0.0f)
		return 1.0f; // total internal reflection

	const float cosThetaT = safeSqrt(cos2ThetaTNumerator) / eta;
	const float rParallel = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
	const float rPerp = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
	return clampf(0.5f * (rParallel * rParallel + rPerp * rPerp), 0.0f, 1.0f);
}

// Exact unpolarised Fresnel for a conductor, one channel, from a complex index
// of refraction (n - ik).
float fresnelConductorExact(float cosTheta, float n, float k)
{
	const float c = clampf(cosTheta, 0.0f, 1.0f);
	const float c2 = c * c;
	const float s2 = maxf(0.0f, 1.0f - c2);

	const float n2 = n * n;
	const float k2 = k * k;

	const float t0 = n2 - k2 - s2;
	// The discriminant is a sum of squares and cannot really be negative; the
	// guard is against round-off, not against the maths.
	const float a2b2 = safeSqrt(t0 * t0 + 4.0f * n2 * k2);

	// a^2 = (a2b2 + t0) / 2, but only computed that way when t0 >= 0.
	//
	// When t0 is negative — which is the whole grazing-angle region, and every
	// angle at all for a metal whose k exceeds n — a2b2 converges on |t0| and
	// the sum cancels away every significant digit. Multiplying by the
	// conjugate gives a^2 = 2 n^2 k^2 / (a2b2 - t0), where the denominator is a
	// sum of positives and nothing cancels.
	const float a = (t0 >= 0.0f)
		? safeSqrt(0.5f * (a2b2 + t0))
		: safeSqrt(2.0f * n2 * k2 / maxf(a2b2 - t0, 1.0e-20f));

	const float t1 = a2b2 + c2;
	const float t2 = 2.0f * a * c;
	const float rPerp = (t1 - t2) / maxf(t1 + t2, 1.0e-12f);

	const float t3 = c2 * a2b2 + s2 * s2;
	const float t4 = t2 * s2;
	const float rParallel = rPerp * (t3 - t4) / maxf(t3 + t4, 1.0e-12f);

	return clampf(0.5f * (rPerp + rParallel), 0.0f, 1.0f);
}

// The complex IOR a metal is actually evaluated with, derived once per material
// from the two colours an artist can reason about.
struct ConductorIor
{
	Vec3 n{ 1.0f, 1.0f, 1.0f };
	Vec3 k{ 1.0f, 1.0f, 1.0f };
};

// Gulbrandsen's artist-friendly inversion (JCGT 2014): given the reflectance at
// normal incidence `r` and the reflectance at grazing `g`, recover the (n, k)
// that reproduce them, then use the EXACT curve in between.
//
// Why not just Schlick. Schlick with a tint is the usual shortcut and it is
// wrong for aluminium by up to 0.12 absolute in red around 80 degrees — and
// wrong by DIFFERENT amounts per channel, so it shifts hue at grazing angles
// rather than just brightness. On a knob, grazing angles are the entire chamfer
// and the entire rim, which is exactly where the eye looks. This renderer has no
// performance pressure whatsoever, so paying for the exact curve is free.
ConductorIor makeConductorIor(const Vec3& reflectivity, const Vec3& edgeTint)
{
	ConductorIor ior;
	for (int c = 0; c < 3; ++c)
	{
		// Clamped below 1 because r = 1 makes nMax infinite, and above 0 to keep
		// the logarithm-free inversion well conditioned.
		const float r = clampf(reflectivity[c], 0.0f, 0.9999f);
		const float g = clampf(edgeTint[c], 0.0f, 1.0f);

		const float sqrtR = safeSqrt(r);
		const float nMin = (1.0f - r) / (1.0f + r);
		const float nMax = (1.0f + sqrtR) / maxf(1.0f - sqrtR, 1.0e-6f);
		const float n = g * nMin + (1.0f - g) * nMax;

		// k^2 follows from requiring the exact normal-incidence reflectance to
		// come out as r. It goes negative for the dielectric-like corner of the
		// parameter space, where the honest answer is k = 0.
		const float k2 = ((n + 1.0f) * (n + 1.0f) * r - (n - 1.0f) * (n - 1.0f))
			/ maxf(1.0f - r, 1.0e-6f);

		(&ior.n.x)[c] = n;
		(&ior.k.x)[c] = safeSqrt(k2);
	}
	return ior;
}

Vec3 fresnelConductor(float cosTheta, const ConductorIor& ior)
{
	return {
		fresnelConductorExact(cosTheta, ior.n.x, ior.k.x),
		fresnelConductorExact(cosTheta, ior.n.y, ior.k.y),
		fresnelConductorExact(cosTheta, ior.n.z, ior.k.z),
	};
}

// ---------------------------------------------------------------------------
// Anisotropic GGX (Trowbridge-Reitz)
//
// All of these work in the local frame where the normal is +Z, the tangent is
// +X and the bitangent is +Y. `ax` is the roughness ALONG THE TANGENT and `ay`
// along the bitangent, so brushing along X means ax > ay: the surface varies
// slowly along the grooves and quickly across them.
// ---------------------------------------------------------------------------

struct Ggx
{
	float ax = 0.1f;
	float ay = 0.1f;

	bool isSmooth() const { return ax < kSmoothThreshold && ay < kSmoothThreshold; }
};

// Perceptual roughness to GGX alpha, and scalar anisotropy to the two alphas.
//
// alpha = roughness^2 is the Disney convention and is used consistently
// everywhere in this file. It exists because alpha itself is perceptually
// bunched up at the low end — squaring gives a slider where the visible change
// per unit of travel is roughly constant.
//
// The 0.9 cap on anisotropy stops `aspect` reaching zero, which would make one
// alpha zero and the distribution a delta in that direction.
Ggx makeGgx(float roughness, float anisotropy)
{
	const float alpha = maxf(roughness, 0.0f) * maxf(roughness, 0.0f);
	const float aspect = safeSqrt(1.0f - 0.9f * clampf(anisotropy, -1.0f, 1.0f));

	Ggx g;
	// NOTE THE ORIENTATION, which is the reverse of the Disney/Blender
	// convention and is deliberate.
	//
	// There, `anisotropy` is an abstract slider and X is whatever the UV
	// happened to give. Here X is the BRUSH DIRECTION, and a brush cuts grooves
	// ALONG itself. Along a groove the surface is smooth, so the normal barely
	// varies and alpha_x must be SMALL; across the grooves it is corrugated, so
	// alpha_y must be LARGE.
	//
	// The consequence is worth stating because it is counter-intuitive and it is
	// how you tell the code is right by looking: since the lobe spreads along
	// whichever axis has the larger alpha, the highlight elongates ACROSS the
	// grooves, not along them. Concentric grain on a knob therefore gives a
	// RADIAL streak that sweeps as the knob turns — the familiar turned-aluminium
	// look — and not a set of concentric rings. Rings are what the inverted
	// convention produces, and they read as a pressed CD rather than machined
	// metal.
	g.ax = alpha * aspect;
	g.ay = alpha / maxf(aspect, 1.0e-4f);

	// Floor each alpha at HALF the smooth threshold, for two reasons.
	//
	// Consistency: isSmooth() asks whether BOTH alphas are tiny, so a strongly
	// anisotropic low-roughness metal can land with ax far below the threshold
	// while ay sits just above it — a near-delta sliver of a lobe. Nudging the
	// roughness across that boundary used to flip the material discontinuously
	// from a plain mirror to a razor-thin streak; with the floor, the lobe on
	// the rough side of the line is merely tight, and the transition is
	// invisible.
	//
	// Stability: D and the VNDF pdf divide by each alpha, and a 1e-4 alpha
	// pushes them toward the edge of float32 for no visual gain — at these
	// widths the smear is fractions of a degree, indistinguishable from a
	// mirror anyway. (Which is also why an ultra-smooth BRUSHED metal renders
	// as a mirror rather than brushed: below the threshold the anisotropy is
	// genuinely subvisible, and dropping it is the honest rendering.)
	if (!g.isSmooth())
	{
		g.ax = maxf(g.ax, kSmoothThreshold * 0.5f);
		g.ay = maxf(g.ay, kSmoothThreshold * 0.5f);
	}
	return g;
}

// The microfacet distribution D(wh).
float ggxD(const Ggx& g, const Vec3& wh)
{
	if (wh.z <= 0.0f)
		return 0.0f;

	const float hx = wh.x / g.ax;
	const float hy = wh.y / g.ay;
	const float d = hx * hx + hy * hy + wh.z * wh.z;
	if (d <= 0.0f)
		return 0.0f;

	return 1.0f / (kPi * g.ax * g.ay * d * d);
}

// Smith's Lambda: the ratio of masked to visible microfacet area.
float ggxLambda(const Ggx& g, const Vec3& w)
{
	const float cos2 = w.z * w.z;
	if (cos2 <= 0.0f)
		return 0.0f;

	const float sin2 = maxf(0.0f, 1.0f - cos2);
	if (sin2 <= 0.0f)
		return 0.0f;

	// The alpha seen along this azimuth, weighted by the direction's projection
	// onto the tangent and bitangent.
	const float ax2 = (w.x * g.ax) * (w.x * g.ax);
	const float ay2 = (w.y * g.ay) * (w.y * g.ay);
	const float alpha2Tan2 = (ax2 + ay2) / cos2;

	// The textbook form is 0.5 * (sqrt(1 + t) - 1). For a smooth surface viewed
	// near head-on, t is tiny, sqrt(1 + t) rounds to exactly 1.0f, and the
	// subtraction cancels to zero in visible steps right across the highlight —
	// the brightest, most scrutinised part of the image. Multiplying through by
	// the conjugate gives the algebraically identical t / (2 (sqrt(1+t) + 1)),
	// which never cancels.
	const float root = safeSqrt(1.0f + alpha2Tan2);
	return alpha2Tan2 / (2.0f * (root + 1.0f));
}

float ggxG1(const Ggx& g, const Vec3& w) { return 1.0f / (1.0f + ggxLambda(g, w)); }

// Height-correlated Smith masking-shadowing. Correlated rather than separable
// because the separable form double-counts the correlation between what is
// visible from `wo` and what is lit from `wi`, and is noticeably too dark at
// grazing angles — exactly where a knob's rim highlight lives.
float ggxG2(const Ggx& g, const Vec3& wo, const Vec3& wi)
{
	return 1.0f / (1.0f + ggxLambda(g, wo) + ggxLambda(g, wi));
}

// Sample a microfacet normal from the DISTRIBUTION OF VISIBLE NORMALS
// (Heitz, "Sampling the GGX Distribution of Visible Normals", JCGT 2018).
//
// Sampling D directly, as older code does, generates a great many microfacets
// that face away from the viewer and are then discarded — which is both slower
// and much noisier at grazing angles. Sampling the visible distribution instead
// generates only microfacets that can actually be seen.
//
// The construction: warp the view direction into the hemisphere-configuration
// where the distribution is a uniform disc, sample the disc, and warp back.
Vec3 ggxSampleVndf(const Ggx& g, const Vec3& wo, float u1, float u2)
{
	// Transform the view direction into the hemisphere configuration.
	Vec3 vh = normalize(Vec3{ g.ax * wo.x, g.ay * wo.y, wo.z });

	// An orthonormal basis in the disc's plane.
	const float lenSq = vh.x * vh.x + vh.y * vh.y;
	const Vec3 t1 = (lenSq > 0.0f)
		? Vec3{ -vh.y, vh.x, 0.0f } * (1.0f / std::sqrt(lenSq))
		: Vec3{ 1.0f, 0.0f, 0.0f };
	const Vec3 t2 = cross(vh, t1);

	// A uniform point on the disc, with the lower half squashed to account for
	// the projection of the hemisphere onto the disc.
	float px, py;
	sampleConcentricDisc(u1, u2, px, py);
	const float s = 0.5f * (1.0f + vh.z);
	py = (1.0f - s) * safeSqrt(1.0f - px * px) + s * py;

	// Lift back onto the hemisphere and undo the warp.
	const Vec3 nh = t1 * px + t2 * py + vh * safeSqrt(1.0f - px * px - py * py);
	return normalize(Vec3{ g.ax * nh.x, g.ay * nh.y, maxf(nh.z, 0.0f) });
}

// The density of the above, per unit solid angle of the HALF VECTOR.
float ggxVndfPdf(const Ggx& g, const Vec3& wo, const Vec3& wh)
{
	if (wo.z <= 0.0f)
		return 0.0f;

	const float dotOh = dot(wo, wh);
	if (dotOh <= 0.0f)
		return 0.0f;

	return ggxG1(g, wo) * dotOh * ggxD(g, wh) / wo.z;
}

// ---------------------------------------------------------------------------
// BSDFs
// ---------------------------------------------------------------------------

struct BsdfSample
{
	Vec3 direction{ 0.0f, 0.0f, 1.0f }; // local
	Vec3 weight{ 0.0f, 0.0f, 0.0f };    // f * cos / pdf, already divided
	float pdf = 0.0f;
	bool specular = false;              // skip NEE and MIS at this vertex
	bool transmitted = false;           // crossed the surface; medium changes
};

// The value and density of a BSDF for a KNOWN pair of directions. Needed by
// next-event estimation, which picks `wi` itself and then has to ask the
// surface how likely that was.
struct BsdfEval
{
	Vec3 value{ 0.0f, 0.0f, 0.0f }; // f * |cos wi|
	float pdf = 0.0f;
};

// The specular reflectance of a dielectric interface at normal incidence.
// ior 1.5 gives the familiar 0.04.
float dielectricF0(float ior)
{
	const float r = (ior - 1.0f) / (ior + 1.0f);
	return r * r;
}

// Fraction of the diffuse lobe that survives the coat, used to keep a plastic
// from reflecting more than it receives. This is the cheap approximation —
// scale the base by one minus the coat's normal-incidence reflectance — rather
// than a full directional albedo integral. It slightly over-darkens at grazing
// angles and is invisible at the roughnesses plastics actually have.
float plasticDiffuseScale(float ior) { return 1.0f - dielectricF0(ior); }

// --- diffuse ---------------------------------------------------------------

// Reflect `wo` about the microfacet normal `wh`. One definition on purpose:
// three BSDFs need it, and the classic transposition bug (wo - wh*2*dot) in a
// single hand-copied site produces below-horizon directions that the wi.z
// guards silently reject — one material quietly darker, no error anywhere.
Vec3 reflectAbout(const Vec3& wo, const Vec3& wh, float dotOh)
{
	return wh * (2.0f * dotOh) - wo;
}

BsdfEval evalDiffuse(const Material& m, const Vec3& wi)
{
	BsdfEval e;
	if (wi.z <= 0.0f)
		return e;

	e.value = m.colour * (kInvPi * wi.z);
	e.pdf = wi.z * kInvPi;
	return e;
}

BsdfSample sampleDiffuse(const Material& m, float u1, float u2)
{
	BsdfSample s;
	s.direction = sampleCosineHemisphere(u1, u2);
	// f * cos / pdf = (albedo/pi * cos) / (cos/pi) = albedo. The cosine cancels
	// exactly, which is the entire reason for sampling cosine-weighted.
	s.weight = m.colour;
	s.pdf = s.direction.z * kInvPi;
	return s;
}

// ---------------------------------------------------------------------------
// Multiple-scattering energy compensation (Kulla-Conty)
//
// Single-scattering GGX loses light, and loses more of it the rougher the
// surface gets. The model traces one bounce off the microsurface and discards
// whatever is shadowed; in reality that light bounces again between the
// microfacets and most of it eventually escapes. The missing fraction is
// exactly 1 - E(mu), where E is the BRDF's directional albedo.
//
// Measured on this implementation before compensation: about 5% gone by alpha
// 0.45, and up to 47% at roughness 0.45 with anisotropy 1.0 - the loss tracks
// the LARGER of the two anisotropic alphas, because that is the axis doing the
// shadowing. A 92%-reflective aluminium rendering at half brightness stops
// reading as metal and starts reading as grey plastic.
//
// Kulla and Conty's fix (SIGGRAPH 2017 course notes) adds a second lobe
// carrying precisely the missing energy, shaped so the pair integrates to one:
//
//     f_ms(wo, wi) = (1 - E(mu_o)) (1 - E(mu_i)) / (pi (1 - E_avg))
//
// scaled by a colour term that accounts for the light being re-absorbed on
// every extra bounce.
//
// WHY A TABLE AND NOT AN ANALYTIC FIT. Published fits exist, but they are fits
// to the isotropic Smith-GGX with a particular masking term, and this renderer
// uses the height-correlated form. Integrating our own BRDF gives a table that
// is right for the BRDF we actually have, and it costs one 32x32 sweep once per
// process.
//
// WHY 2D AND NOT 3D. A fully anisotropic table would need an azimuth axis as
// well. Instead the effective alpha is max(ax, ay) - the measurement above says
// the loss tracks the larger alpha, so the isotropic table indexed by it
// captures the bulk. It slightly OVER-compensates in the middle azimuths, and
// over-compensating a 47% deficit is a far smaller error than leaving it.
// ---------------------------------------------------------------------------

// Directional albedo E(mu, alpha): the fraction of light leaving the surface
// for a view at cos(theta) = mu, with Fresnel forced to 1. Sampled with the
// same VNDF routine the BSDF uses, so the table measures THIS implementation
// including its below-horizon rejection.
constexpr int kAlbedoSize = 32;

struct AlbedoTable
{
	// [alpha][mu], both axes uniformly sampled over (0, 1].
	float e[kAlbedoSize][kAlbedoSize] = {};
	// Cosine-weighted average of E over the hemisphere, per alpha.
	float average[kAlbedoSize] = {};
};

// Built once, deterministically: a fixed seed and a fixed sample count, so the
// table is bit-identical on every run and the committed reference images stay
// reproducible. A Monte Carlo table with a wandering seed would quietly make
// every render irreproducible.
AlbedoTable buildAlbedoTable()
{
	AlbedoTable table;
	constexpr int kSamples = 512;

	for (int ai = 0; ai < kAlbedoSize; ++ai)
	{
		const float alpha = ((float)ai + 0.5f) / (float)kAlbedoSize;

		Ggx g;
		g.ax = alpha;
		g.ay = alpha;

		double weightedSum = 0.0;
		double weightTotal = 0.0;

		for (int mi = 0; mi < kAlbedoSize; ++mi)
		{
			const float mu = ((float)mi + 0.5f) / (float)kAlbedoSize;
			const Vec3 wo{ safeSqrt(1.0f - mu * mu), 0.0f, mu };

			Rng rng = seedRng((uint32_t)ai, (uint32_t)mi, 0u, 0x9E3779B9u);

			double sum = 0.0;
			for (int i = 0; i < kSamples; ++i)
			{
				const float u1 = rng.next();
				const float u2 = rng.next();

				const Vec3 wh = ggxSampleVndf(g, wo, u1, u2);
				const float dotOh = dot(wo, wh);
				if (dotOh <= 0.0f)
					continue;

				const Vec3 wi = reflectAbout(wo, wh, dotOh);
				if (wi.z <= 0.0f)
					continue; // below the horizon: this is the lost energy

				// The VNDF weight with Fresnel = 1 is exactly G2/G1(wo).
				sum += (double)(ggxG2(g, wo, wi) / maxf(ggxG1(g, wo), 1.0e-6f));
			}

			const float e = (float)(sum / (double)kSamples);
			table.e[ai][mi] = clampf(e, 0.0f, 1.0f);

			// E_avg is the cosine-weighted mean: 2 * integral E(mu) mu dmu.
			weightedSum += (double)table.e[ai][mi] * (double)mu;
			weightTotal += (double)mu;
		}

		table.average[ai] = (weightTotal > 0.0)
			? clampf((float)(weightedSum / weightTotal), 0.0f, 1.0f) : 1.0f;
	}

	return table;
}

const AlbedoTable& albedoTable()
{
	// Function-local static: C++ guarantees the initialisation is thread-safe
	// and happens once, which matters because render() calls this from every
	// worker thread at the same moment.
	static const AlbedoTable table = buildAlbedoTable();
	return table;
}

float albedoLookup(float alpha, float mu)
{
	const AlbedoTable& t = albedoTable();

	const float a = clampf(alpha * (float)kAlbedoSize - 0.5f, 0.0f, (float)(kAlbedoSize - 1));
	const float m = clampf(mu * (float)kAlbedoSize - 0.5f, 0.0f, (float)(kAlbedoSize - 1));

	const int a0 = (int)a, m0 = (int)m;
	const int a1 = std::min(a0 + 1, kAlbedoSize - 1);
	const int m1 = std::min(m0 + 1, kAlbedoSize - 1);
	const float af = a - (float)a0, mf = m - (float)m0;

	const float e00 = t.e[a0][m0], e01 = t.e[a0][m1];
	const float e10 = t.e[a1][m0], e11 = t.e[a1][m1];

	return (e00 + (e01 - e00) * mf) * (1.0f - af)
		+ (e10 + (e11 - e10) * mf) * af;
}

float albedoAverage(float alpha)
{
	const AlbedoTable& t = albedoTable();
	const float a = clampf(alpha * (float)kAlbedoSize - 0.5f, 0.0f, (float)(kAlbedoSize - 1));
	const int a0 = (int)a;
	const int a1 = std::min(a0 + 1, kAlbedoSize - 1);
	return t.average[a0] + (t.average[a1] - t.average[a0]) * (a - (float)a0);
}

// The multiple-scattering lobe's value, times cos(wi), for a conductor.
//
// `fAvg` is the Fresnel averaged over the hemisphere: light making N extra
// bounces is attenuated by F^N, and summing that geometric series gives the
// colour of the compensation term. Without it the added energy would be white
// and gold would desaturate as it roughened.
Vec3 multiScatterMetal(const Ggx& g, const Vec3& fAvg, const Vec3& wo, const Vec3& wi)
{
	// The effective isotropic alpha standing in for the anisotropic pair.
	//
	// RMS, not max(ax, ay) and not the geometric mean. Measured against the
	// white furnace at anisotropy 0.85: the geometric mean is exactly the
	// isotropic alpha and so under-compensates (it does not see the anisotropy
	// at all), while max() over-compensates badly — it read +18% at roughness
	// 0.6, turning a 34% deficit into an 18% surplus. RMS lands between them
	// and tracks the measured loss closely across the range.
	const float alpha = safeSqrt((g.ax * g.ax + g.ay * g.ay) * 0.5f);

	const float eo = albedoLookup(alpha, wo.z);
	const float ei = albedoLookup(alpha, wi.z);
	const float eAvg = albedoAverage(alpha);

	const float denom = 1.0f - eAvg;
	if (denom <= 1.0e-4f)
		return {}; // nothing measurable was lost

	// f_ms * cos(wi); the 1/pi and the cosine combine to the usual form.
	const float lobe = (1.0f - eo) * (1.0f - ei) * wi.z * kInvPi / denom;

	// Kulla-Conty's colour term: F_avg^2 E_avg / (1 - F_avg (1 - E_avg)),
	// the sum of the series over all the extra bounces.
	const Vec3 numerator = fAvg * fAvg * eAvg;
	const Vec3 series{
		1.0f - fAvg.x * denom,
		1.0f - fAvg.y * denom,
		1.0f - fAvg.z * denom,
	};

	return Vec3{
		numerator.x / maxf(series.x, 1.0e-4f),
		numerator.y / maxf(series.y, 1.0e-4f),
		numerator.z / maxf(series.z, 1.0e-4f),
	} * lobe;
}

// Hemispherical average Fresnel for a conductor.
//
// Schlick's curve integrates over the cosine-weighted hemisphere in closed
// form to (20 F0 + 1) / 21, so this recovers F0 from the complex IOR exactly
// and then uses that. Schlick is not good enough for the PRIMARY reflection —
// that is why fresnelConductorExact exists — but this term only decides the
// tint of light that has already bounced several times off the microsurface,
// where the angular detail has long since been averaged away.
Vec3 averageFresnel(const ConductorIor& ior)
{
	Vec3 result;
	for (int c = 0; c < 3; ++c)
	{
		const float n = (&ior.n.x)[c];
		const float k = (&ior.k.x)[c];

		const float nPlus = n + 1.0f;
		const float nMinus = n - 1.0f;
		const float f0 = clampf((nMinus * nMinus + k * k)
			/ maxf(nPlus * nPlus + k * k, 1.0e-6f), 0.0f, 1.0f);

		(&result.x)[c] = (20.0f * f0 + 1.0f) / 21.0f;
	}
	return result;
}

// --- metal -----------------------------------------------------------------

BsdfEval evalMetal(const Ggx& g, const ConductorIor& ior, const Vec3& wo, const Vec3& wi)
{
	BsdfEval e;
	if (wo.z <= 0.0f || wi.z <= 0.0f || g.isSmooth())
		return e;

	const Vec3 wh = normalize(wo + wi);
	const float d = ggxD(g, wh);
	if (d <= 0.0f)
		return e;

	const Vec3 f = fresnelConductor(dot(wo, wh), ior);
	const float g2 = ggxG2(g, wo, wi);

	// f_r = D * G2 * F / (4 cos_o cos_i), and the estimator wants f_r * cos_i,
	// so one cosine cancels here.
	e.value = f * (d * g2 / (4.0f * wo.z));

	// Plus the light single scattering threw away. Added to the VALUE only:
	// the pdf stays the single-scattering one, which is legal because the
	// extra lobe is broad and the sampled directions still cover it — the
	// estimator divides by the density it actually sampled from either way.
	e.value += multiScatterMetal(g, averageFresnel(ior), wo, wi);

	e.pdf = ggxVndfPdf(g, wo, wh) / (4.0f * dot(wo, wh));
	return e;
}

BsdfSample sampleMetal(const Ggx& g, const ConductorIor& ior, const Vec3& wo, float u1, float u2)
{
	BsdfSample s;
	if (wo.z <= 0.0f)
		return s;

	if (g.isSmooth())
	{
		s.direction = { -wo.x, -wo.y, wo.z };
		s.weight = fresnelConductor(wo.z, ior);
		s.pdf = 1.0f;
		s.specular = true;
		return s;
	}

	const Vec3 wh = ggxSampleVndf(g, wo, u1, u2);
	const float dotOh = dot(wo, wh);
	if (dotOh <= 0.0f)
		return s;

	const Vec3 wi = reflectAbout(wo, wh, dotOh);
	if (wi.z <= 0.0f)
		return s;

	s.direction = wi;
	// The whole estimator collapses to F * G2/G1(wo): D and the geometry factors
	// cancel against the visible-normal pdf. Deriving that once is worth it —
	// evaluating the full quotient numerically loses precision at low roughness.
	s.weight = fresnelConductor(dotOh, ior)
		* (ggxG2(g, wo, wi) / maxf(ggxG1(g, wo), 1.0e-6f));
	s.pdf = ggxVndfPdf(g, wo, wh) / (4.0f * dotOh);

	// The multiple-scattering lobe is carried on the same sample rather than
	// being given its own: it is broad and smooth, so the single-scattering
	// direction is a perfectly good place to evaluate it, and one lobe means
	// one pdf and no mixture bookkeeping to get wrong.
	if (s.pdf > 0.0f)
		s.weight += multiScatterMetal(g, averageFresnel(ior), wo, wi) / s.pdf;

	return s;
}

// --- plastic: a dielectric coat over a diffuse base -------------------------

// Schlick's approximation for the plastic COAT (and only the coat: metals use
// the exact conductor curve, glass the exact dielectric one). Defined once so
// evalPlastic and samplePlastic cannot drift apart — the sampled weight is
// value/pdf against evalPlastic, and two hand-copied Fresnels that disagree by
// one edit is a silent estimator bias, not a visible error.
float schlickDielectric(float f0, float cosTheta)
{
	const float c = clampf(1.0f - cosTheta, 0.0f, 1.0f);
	const float c2 = c * c;
	return f0 + (1.0f - f0) * (c2 * c2 * c);
}

// The probability of choosing the specular lobe. Weighted by the coat's
// reflectance so a shinier coat is sampled more often, and clamped away from 0
// and 1 so neither lobe can ever become unsamplable.
float plasticSpecularProbability(float ior)
{
	return clampf(dielectricF0(ior) * 6.0f, 0.10f, 0.75f);
}

BsdfEval evalPlastic(const Material& m, const Ggx& g, const Vec3& wo, const Vec3& wi)
{
	BsdfEval e;
	if (wo.z <= 0.0f || wi.z <= 0.0f)
		return e;

	const float pSpec = plasticSpecularProbability(m.ior);
	const float f0 = dielectricF0(m.ior);

	// Diffuse half.
	const Vec3 diffuse = m.colour * (kInvPi * wi.z * plasticDiffuseScale(m.ior));
	const float pdfDiffuse = wi.z * kInvPi;

	// Specular half.
	Vec3 specular{ 0.0f };
	float pdfSpecular = 0.0f;
	if (!g.isSmooth())
	{
		const Vec3 wh = normalize(wo + wi);
		const float d = ggxD(g, wh);
		if (d > 0.0f)
		{
			const float fr = schlickDielectric(f0, dot(wo, wh));
			specular = Vec3{ d * ggxG2(g, wo, wi) * fr / (4.0f * wo.z) };
			pdfSpecular = ggxVndfPdf(g, wo, wh) / (4.0f * dot(wo, wh));
		}
	}

	e.value = diffuse + specular;
	// The MIXTURE pdf, not the chosen lobe's pdf. Using only the lobe that was
	// sampled makes the estimator biased and is the classic multi-lobe bug.
	e.pdf = pSpec * pdfSpecular + (1.0f - pSpec) * pdfDiffuse;
	return e;
}

BsdfSample samplePlastic(const Material& m, const Ggx& g, const Vec3& wo,
	float uLobe, float u1, float u2)
{
	BsdfSample s;
	if (wo.z <= 0.0f)
		return s;

	const float pSpec = plasticSpecularProbability(m.ior);

	Vec3 wi;
	if (uLobe < pSpec)
	{
		if (g.isSmooth())
		{
			// A smooth coat is a mirror, and this vertex IS flagged specular:
			// evalPlastic drops the delta lobe entirely, so mirror transport is
			// reachable only through this branch and must keep MIS weight one.
			// Down-weighting it against a light-sample pdf would mix a discrete
			// probability with a solid-angle density and darken every coat
			// highlight. (The diffuse base still gets NEE as usual — the two
			// lobes' estimators are disjoint here, not double counted.)
			wi = { -wo.x, -wo.y, wo.z };
			s.direction = wi;
			s.weight = Vec3{ schlickDielectric(dielectricF0(m.ior), wo.z) / pSpec };
			s.pdf = pSpec;
			s.specular = true;
			return s;
		}

		const Vec3 wh = ggxSampleVndf(g, wo, u1, u2);
		const float dotOh = dot(wo, wh);
		if (dotOh <= 0.0f)
			return s;

		wi = reflectAbout(wo, wh, dotOh);
	}
	else
	{
		wi = sampleCosineHemisphere(u1, u2);
	}

	if (wi.z <= 0.0f)
		return s;

	// Re-evaluate both lobes for the chosen direction. This is deliberately not
	// the "return the chosen lobe's weight" shortcut: with two overlapping lobes
	// the correct weight is the SUM of the values over the MIXTURE pdf.
	const BsdfEval e = evalPlastic(m, g, wo, wi);
	if (e.pdf <= 0.0f)
		return s;

	s.direction = wi;
	s.weight = e.value / e.pdf;
	s.pdf = e.pdf;
	return s;
}

// --- glass: rough dielectric with absorption --------------------------------

// Refract `wo` about `wh` with relative index `eta` = (transmitted / incident).
// Returns false on total internal reflection.
bool refractAbout(const Vec3& wo, const Vec3& wh, float eta, Vec3& wt)
{
	const float cosThetaI = dot(wo, wh);
	const float sin2ThetaI = maxf(0.0f, 1.0f - cosThetaI * cosThetaI);
	const float sin2ThetaT = sin2ThetaI / (eta * eta);
	if (sin2ThetaT >= 1.0f)
		return false;

	const float cosThetaT = safeSqrt(1.0f - sin2ThetaT);
	wt = wh * (cosThetaI / eta - cosThetaT) - wo * (1.0f / eta);
	return true;
}

BsdfSample sampleGlass(const Material& m, const Ggx& g, const Vec3& wo,
	bool entering, float uChoice, float u1, float u2)
{
	BsdfSample s;

	// eta is always transmitted-over-incident, so it inverts on the way out.
	const float eta = entering ? m.ior : (1.0f / m.ior);

	// The half vector, and the cosine Fresnel is measured against. `wo.z > 0`
	// always holds here because the caller flips the frame when the ray is
	// inside, so "entering" is carried as a flag rather than as a sign.
	const bool smooth = g.isSmooth();
	const Vec3 wh = smooth ? Vec3{ 0.0f, 0.0f, 1.0f } : ggxSampleVndf(g, wo, u1, u2);

	const float dotOh = dot(wo, wh);
	if (dotOh <= 0.0f)
		return s;

	const float fr = fresnelDielectric(dotOh, eta);

	Vec3 wt;
	const bool canRefract = refractAbout(wo, wh, eta, wt);

	// Choose a lobe in proportion to Fresnel. Because the choice probability
	// equals the weight, F cancels out of the estimator entirely — which is why
	// glass is so much quieter than it looks like it should be.
	const bool reflectIt = !canRefract || (uChoice < fr);

	if (reflectIt)
	{
		const Vec3 wi = reflectAbout(wo, wh, dotOh);
		if (wi.z <= 0.0f)
			return s;

		s.direction = wi;
		s.transmitted = false;
		if (smooth)
		{
			s.weight = Vec3{ 1.0f };
			s.pdf = 1.0f;
			s.specular = true;
		}
		else
		{
			s.weight = Vec3{ ggxG2(g, wo, wi) / maxf(ggxG1(g, wo), 1.0e-6f) };
			s.pdf = (ggxVndfPdf(g, wo, wh) / (4.0f * dotOh)) * fr;
		}
		return s;
	}

	if (wt.z >= 0.0f)
		return s; // refraction must end up on the far side

	s.direction = wt;
	s.transmitted = true;

	// The eta^2 non-symmetry factor. Radiance is compressed when it enters a
	// denser medium, and a path traced from the CAMERA carries importance
	// rather than radiance, so the correction appears as a division here.
	//
	// For a CLOSED object the entry and exit factors cancel exactly, so leaving
	// this out looks identical for a knob's glass cap. It is included because it
	// is free and because a single interface — a window pane modelled as one
	// surface — is visibly wrong without it.
	const float etaScale = 1.0f / (eta * eta);

	if (smooth)
	{
		s.weight = Vec3{ etaScale };
		s.pdf = 1.0f;
		s.specular = true;
	}
	else
	{
		s.weight = Vec3{ etaScale * ggxG2(g, wo, wt) / maxf(ggxG1(g, wo), 1.0e-6f) };

		// The refraction Jacobian: the half vector for transmission is
		// -(eta_i*wi + eta_o*wo) normalised, and mapping its density to the
		// density of `wt` costs this factor. Written out rather than folded in
		// so it can be checked against the reference derivation.
		const float denom = dotOh + eta * dot(wt, wh);
		const float jacobian = (denom * denom > 0.0f)
			? std::fabs(eta * eta * dot(wt, wh)) / (denom * denom)
			: 0.0f;
		s.pdf = ggxVndfPdf(g, wo, wh) * jacobian * (1.0f - fr);
	}
	return s;
}

// Beer-Lambert. `colour` is the fraction surviving `distance` world units, so
// the coefficient is its negative log. Guarded away from zero because a fully
// absorbing channel would be an infinite coefficient.
Vec3 absorptionCoefficient(const Vec3& colour, float distance)
{
	if (distance <= 0.0f)
		return { 0.0f, 0.0f, 0.0f };

	const float inv = 1.0f / distance;
	return {
		-std::log(clampf(colour.x, 1.0e-4f, 1.0f)) * inv,
		-std::log(clampf(colour.y, 1.0e-4f, 1.0f)) * inv,
		-std::log(clampf(colour.z, 1.0e-4f, 1.0f)) * inv,
	};
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

struct Hit
{
	bool valid = false;
	float t = 0.0f;
	Vec3 position{};
	Vec3 normal{};          // always faces AGAINST the incoming ray
	bool backface = false;  // the ray was travelling inside the object
	const Material* material = nullptr;
	const Light* light = nullptr;

	// Which scene entry was struck, so downstream code can identify it without
	// comparing pointers: the medium tracker needs the OBJECT to negate while
	// inside it, and the MIS bookkeeping needs to find the matching emitter.
	int objectIndex = -1;
	int lightIndex = -1;
};

// A conservative lower bound on the distance to one object.
//
// The bounding sphere is not just an early-out: when the point is outside the
// sphere, the distance TO THE SPHERE is itself a valid lower bound on the
// distance to anything inside it, so it can be returned directly and the
// object's own (expensive, indirect-called) distance function skipped entirely.
//
// THE SLACK IS LOAD BEARING. Returning the bound whenever it is merely positive
// looks right and is badly wrong: as a ray approaches the bounding sphere the
// bound falls to zero, the tracer sees a distance below its hit epsilon, and it
// stops — ON THE BOUNDING SPHERE. Every object then renders as its own bounding
// sphere, which is subtle for a sphere and total for anything else. Handing back
// the true field well before the bound can be mistaken for a surface is what
// makes the optimisation invisible.
constexpr float kBoundsSlack = 0.01f;

float objectDistance(const Object& o, const Vec3& p)
{
	if (!o.unbounded)
	{
		const float toBounds = length(p - o.boundsCentre) - o.boundsRadius;
		if (toBounds > kBoundsSlack)
			return toBounds;
	}

	return o.distance(p);
}

// The whole scene as one field, plus which object was nearest.
//
// `insideIndex` names the ONE object the ray is currently travelling inside
// (the glass it refracted into), or -1 for none. That object's distance is
// negated — turning "how deep am I" into "how far to my exit" — while every
// OTHER object keeps its ordinary sign.
//
// Negating per object rather than negating the combined minimum matters. The
// first version negated the min of the whole field, which reads plausibly and
// is wrong whenever anything else sits inside the glass: the min was always the
// glass's own (most negative) distance, so the march stepped clean through any
// embedded solid — an LED potted in a glass dome simply never rendered. With
// the negation applied to the medium alone, the field's minimum is once again a
// true lower bound on the distance to the nearest SURFACE, whichever object
// owns it.
float sceneDistance(const Scene& scene, const Vec3& p, bool primary, int insideIndex, int& nearest)
{
	float best = kMaxTraceDistance;
	nearest = -1;

	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		const Object& o = scene.objects[i];
		if (primary && !o.material.cameraVisible)
			continue; // marched straight through, as if it were not there

		float d = objectDistance(o, p);
		if ((int)i == insideIndex)
			d = -d;

		if (d < best)
		{
			best = d;
			nearest = (int)i;
		}
	}

	return best;
}

// Gradient of the field by the tetrahedron trick: four taps at the corners of a
// regular tetrahedron rather than the six of a central difference. The 6-tap
// version is marginally more accurate and 50% more expensive, and since the
// field is only ever C0 at CSG seams anyway, the extra accuracy buys nothing
// visible.
// Always taken on the PLAIN field (insideIndex = -1), even when the march that
// found the surface ran with a negated medium. The surface is the same surface
// from either side, and the plain field's gradient is its OUTWARD normal — which
// is exactly what the backface test downstream expects. Tapping the negated
// field instead would hand back an inward normal at the medium's boundary, the
// backface test would read every glass exit as an entry, and eta would invert.
Vec3 sceneNormal(const Scene& scene, const Vec3& p, bool primary)
{
	constexpr float h = 1.5f * kSurfaceEpsilon;
	int ignored = 0;

	const Vec3 k0{ 1.0f, -1.0f, -1.0f };
	const Vec3 k1{ -1.0f, -1.0f, 1.0f };
	const Vec3 k2{ -1.0f, 1.0f, -1.0f };
	const Vec3 k3{ 1.0f, 1.0f, 1.0f };

	const Vec3 n =
		k0 * sceneDistance(scene, p + k0 * h, primary, -1, ignored) +
		k1 * sceneDistance(scene, p + k1 * h, primary, -1, ignored) +
		k2 * sceneDistance(scene, p + k2 * h, primary, -1, ignored) +
		k3 * sceneDistance(scene, p + k3 * h, primary, -1, ignored);

	return normalize(n);
}

// Sphere trace the SDF objects.
//
// `insideIndex` is the object the ray starts inside (>= 0 for the interior
// segment of every refracted path — what makes glass possible at all), or -1.
// `wantNormal` skips the normal estimate — four extra full-scene evaluations —
// for callers that only need to know WHETHER something was hit: shadow rays and
// the emissive-object confirmation probe, which between them are most of the
// rays in a lit scene.
bool marchObjects(const Scene& scene, const Vec3& origin, const Vec3& direction,
	float maxDistance, bool primary, int insideIndex, bool wantNormal, Hit& hit)
{
	float t = kRayOffset;
	int nearest = -1;

	for (int step = 0; step < kMaxMarchSteps && t < maxDistance; ++step)
	{
		const Vec3 p = origin + direction * t;
		const float d = sceneDistance(scene, p, primary, insideIndex, nearest);

		// Mostly CONSTANT, unlike the usual advice.
		//
		// A perspective renderer grows its hit epsilon with distance because a
		// pixel's ray cone widens, so far-away detail is not worth resolving.
		// An ORTHOGRAPHIC camera has no ray cone at all — every ray is parallel
		// and a pixel covers the same world distance at any depth — so growing
		// the epsilon there just blurs distant geometry for nothing. The small
		// term that remains is float32 precision loss along t, not a cone.
		const float epsilon = kSurfaceEpsilon * (1.0f + t * 0.004f);
		if (d < epsilon)
		{
			if (nearest < 0)
				return false;

			hit.valid = true;
			hit.t = t;
			hit.position = p;
			hit.material = &scene.objects[nearest].material;
			hit.objectIndex = nearest;

			// The RAW outward gradient, deliberately NOT oriented against the
			// ray here. Orienting it would destroy the only evidence of which
			// side was hit: a normal already turned to face the ray always
			// reports dot(n, d) < 0, so the back-face test downstream can never
			// fire. Glass then treats every EXIT as an entry, uses 1.52 instead
			// of 1/1.52, and refracts and totally-internally-reflects at all the
			// wrong angles — which reads as glass that is too dark and muddy
			// rather than as anything obviously broken.
			if (wantNormal)
				hit.normal = sceneNormal(scene, p, primary);
			return true;
		}

		// The step is the distance itself — that is the sphere-tracing
		// guarantee. maxf keeps a degenerate zero distance from stalling the
		// loop entirely; without it a ray that lands exactly on a CSG seam
		// spins for all kMaxMarchSteps and then reports a miss.
		t += maxf(d, kSurfaceEpsilon * 0.5f);
	}

	return false;
}

// --- analytic lights -------------------------------------------------------

Vec3 rectNormal(const Light& l) { return normalize(cross(l.halfU, l.halfV)); }

bool intersectRectLight(const Light& l, const Vec3& origin, const Vec3& direction,
	float maxDistance, float& tOut)
{
	const Vec3 n = rectNormal(l);
	const float denom = dot(direction, n);
	if (denom * denom < 1.0e-12f)
		return false; // parallel to the plane

	const float t = dot(l.position - origin, n) / denom;
	if (t <= kRayOffset || t >= maxDistance)
		return false;

	if (!l.twoSided && denom > 0.0f)
		return false; // hitting the unlit back of a one-sided window

	// Project into the rectangle's own axes. Dividing by the SQUARED length
	// converts the projection into the [-1,1] parameterisation directly.
	const Vec3 d = origin + direction * t - l.position;
	const float uu = dot(d, l.halfU) / maxf(dot(l.halfU, l.halfU), 1.0e-12f);
	const float vv = dot(d, l.halfV) / maxf(dot(l.halfV, l.halfV), 1.0e-12f);
	if (uu < -1.0f || uu > 1.0f || vv < -1.0f || vv > 1.0f)
		return false;

	tOut = t;
	return true;
}

bool intersectSphereLight(const Light& l, const Vec3& origin, const Vec3& direction,
	float maxDistance, float& tOut)
{
	const Vec3 oc = origin - l.position;
	const float b = dot(oc, direction);
	const float c = dot(oc, oc) - l.radius * l.radius;
	const float disc = b * b - c;
	if (disc <= 0.0f)
		return false;

	const float sq = std::sqrt(disc);
	float t = -b - sq;
	if (t <= kRayOffset)
		t = -b + sq;
	if (t <= kRayOffset || t >= maxDistance)
		return false;

	tOut = t;
	return true;
}

// The nearest hit against geometry AND lights.
Hit intersect(const Scene& scene, const Vec3& origin, const Vec3& direction,
	float maxDistance, bool primary, int insideIndex, bool wantNormal = true)
{
	Hit hit;
	marchObjects(scene, origin, direction, maxDistance, primary, insideIndex, wantNormal, hit);

	float closest = hit.valid ? hit.t : maxDistance;

	for (size_t i = 0; i < scene.lights.size(); ++i)
	{
		const Light& l = scene.lights[i];
		if (primary && !l.cameraVisible)
			continue;

		// A light with nothing to emit does not exist. Without this, a studio
		// preset that zeroes a lamp leaves an INVISIBLE BLACK CARD hanging in
		// the scene — camera-invisible, radiance zero, yet still opaque — that
		// silently absorbs every bounce ray unlucky enough to cross it.
		if (l.emission.isBlack())
			continue;

		float t = 0.0f;
		const bool got = (l.shape == LightShape::Rect)
			? intersectRectLight(l, origin, direction, closest, t)
			: intersectSphereLight(l, origin, direction, closest, t);

		if (got)
		{
			closest = t;
			hit.valid = true;
			hit.t = t;
			hit.position = origin + direction * t;
			hit.material = nullptr;
			hit.objectIndex = -1;
			hit.light = &l;
			hit.lightIndex = (int)i;
			hit.normal = (l.shape == LightShape::Rect)
				? rectNormal(l)
				: normalize(hit.position - l.position);
		}
	}

	if (hit.valid && hit.light == nullptr)
	{
		// Orient the normal against the ray and remember which side was hit.
		// For glass this is what distinguishes entering from leaving.
		hit.backface = dot(hit.normal, direction) > 0.0f;
		if (hit.backface)
			hit.normal = -hit.normal;
	}

	return hit;
}

// Anything at all between two points? Lights are not blockers.
//
// Glass IS a blocker here, deliberately. A shadow ray cannot refract — bending
// it would no longer connect the two points it was built to connect — so
// next-event estimation must treat glass as opaque. The light that really does
// pass through the glass arrives instead via BSDF sampling, and THAT is what
// paints the coloured caustic under a piece of coloured glass. Trying to "fix"
// this by letting shadow rays pass tinted through glass produces a flat coloured
// patch with no bright core, which reads as a decal rather than as light.
bool occluded(const Scene& scene, const Vec3& origin, const Vec3& direction, float distance)
{
	Hit hit;
	// wantNormal = false: a shadow ray only asks WHETHER, never WHERE, so the
	// four-tap gradient at the blocking surface would be pure waste — and
	// blocked shadow rays are a large share of all rays in a lit scene.
	return marchObjects(scene, origin, direction, distance - kRayOffset * 4.0f,
		false, -1, false, hit);
}

// ---------------------------------------------------------------------------
// Emitters
//
// Two things can emit: an analytic Light, and an Object whose material is
// Emissive. They are unified here so the integrator has ONE list to importance
// sample and one pdf to weight against.
//
// An Emissive object is sampled through its BOUNDING SPHERE. That sphere is
// already required for the tracing optimisation, so a glowing cube costs
// nothing extra to aim at: sample the cone the bounds subtend, fire a ray, and
// keep the contribution only if the ray actually lands on that object.
// Directions inside the cone that miss the cube contribute zero, which wastes a
// few samples on the corners and is completely unbiased — the pdf is the cone's
// either way. This is what makes a glowing cube LIGHT the room rather than
// merely glow in it.
// ---------------------------------------------------------------------------

struct LightSample
{
	Vec3 direction{};   // world, from the shading point towards the emitter
	float distance = 0.0f;
	Vec3 radiance{};    // emitted radiance towards the shading point
	float pdf = 0.0f;   // per unit SOLID ANGLE at the shading point
};

// The cone a sphere subtends from a point.
//
// `oneMinusCos` is carried separately and is NOT computed as 1 - cosThetaMax.
// For a small or distant lamp, rSq/distSq is tiny, cosThetaMax rounds to
// exactly 1.0f, and the subtraction yields exactly 0 — making the solid angle
// zero and the pdf a division by zero. The algebraically identical
// x / (1 + sqrt(1 - x)) never cancels, and is exact across the whole range.
struct SphereCone
{
	bool valid = false;
	float cosThetaMax = 1.0f;
	float oneMinusCos = 0.0f;
	float distance = 0.0f;
};

SphereCone sphereCone(const Vec3& centre, float radius, const Vec3& from)
{
	SphereCone cone;

	const Vec3 toCentre = centre - from;
	const float distSq = dot(toCentre, toCentre);
	const float rSq = radius * radius;
	if (distSq <= rSq)
		return cone; // the shading point is inside the lamp

	const float x = rSq / distSq;
	cone.distance = std::sqrt(distSq);
	cone.cosThetaMax = safeSqrt(1.0f - x);
	cone.oneMinusCos = x / (1.0f + cone.cosThetaMax);
	cone.valid = cone.oneMinusCos > 0.0f;
	return cone;
}

// Uniform direction within the cone, about +Z.
//
// Works in the ONE-MINUS-COSINE domain end to end, which is why it takes the
// SphereCone rather than a bare cosThetaMax. Recomputing 1 - cosThetaMax here
// by subtraction would re-introduce the exact cancellation the struct exists to
// avoid: for a small distant emitter cosThetaMax rounds to 1.0f, the sampled
// spread collapses onto the axis, and the pdf — built from the exact
// oneMinusCos — keeps describing the finite cone. Every probe then hits the
// target instead of the correct cone fraction and its light is overestimated by
// the cone-to-object solid-angle ratio. sin is taken from the versine identity
// sin^2 = omc * (2 - omc), which stays exact where cos - 1 does not.
Vec3 sampleCone(const SphereCone& cone, float u1, float u2)
{
	const float omc = u1 * cone.oneMinusCos;
	const float cosTheta = 1.0f - omc;
	const float sinTheta = safeSqrt(omc * (2.0f - omc));
	const float phi = 2.0f * kPi * u2;
	return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
}

// The matching density, per unit solid angle. Sample and pdf MUST come from
// this one pair — the formula appearing once is what guarantees MIS stays
// consistent across the four places that need it.
float conePdf(const SphereCone& cone)
{
	return cone.valid ? (1.0f / (2.0f * kPi * cone.oneMinusCos)) : 0.0f;
}

// A cone-sampled direction towards a sphere at `centre`, in world space.
Vec3 sampleSphereConeDirection(const SphereCone& cone, const Vec3& centre,
	const Vec3& from, float u1, float u2)
{
	const Frame frame = makeFrame(normalize(centre - from));
	return frame.toWorld(sampleCone(cone, u1, u2));
}

struct Emitter
{
	const Light* light = nullptr; // analytic; null for an emissive object
	int lightIndex = -1;          // index into Scene::lights, or -1
	int objectIndex = -1;         // index into Scene::objects, or -1
	Vec3 proxyCentre{};           // bounding sphere, for objectIndex only
	float proxyRadius = 0.0f;
};

std::vector<Emitter> buildEmitters(const Scene& scene)
{
	std::vector<Emitter> emitters;

	for (size_t i = 0; i < scene.lights.size(); ++i)
	{
		const Light& l = scene.lights[i];

		// The same rule intersect() applies: a black light does not exist. Left
		// in the list it would soak up its share of every NEE pick — each one a
		// full shadow march for a guaranteed zero — which in a scene that zeroes
		// two of five lights is 40% of all light samples doing nothing.
		if (l.emission.isBlack())
			continue;

		Emitter e;
		e.light = &l;
		e.lightIndex = (int)i;
		emitters.push_back(e);
	}

	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		const Object& o = scene.objects[i];
		if (o.material.kind != MaterialKind::Emissive || o.material.emission.isBlack())
			continue;

		// An UNBOUNDED emissive object cannot be aimed at — there is no finite
		// cone to sample. It still glows and still lights things by chance; it
		// simply is not importance sampled. Skipping it here is what keeps the
		// pdf bookkeeping honest.
		if (o.unbounded || o.boundsRadius <= 0.0f || o.boundsRadius >= 1.0e8f)
			continue;

		Emitter e;
		e.objectIndex = (int)i;
		e.proxyCentre = o.boundsCentre;
		e.proxyRadius = o.boundsRadius;
		emitters.push_back(e);
	}

	return emitters;
}

LightSample sampleEmitter(const Emitter& emitter, const Vec3& from, float u1, float u2)
{
	LightSample s;

	if (emitter.objectIndex >= 0)
	{
		const SphereCone cone = sphereCone(emitter.proxyCentre, emitter.proxyRadius, from);
		if (!cone.valid)
			return s;

		s.direction = sampleSphereConeDirection(cone, emitter.proxyCentre, from, u1, u2);
		// The farthest any point of the object can be; the caller's probe stops
		// there and confirms what was actually hit.
		s.distance = cone.distance + emitter.proxyRadius;
		s.pdf = conePdf(cone);
		s.radiance = Vec3{ 1.0f }; // replaced by the caller from the real hit
		return s;
	}

	const Light& l = *emitter.light;

	if (l.shape == LightShape::Rect)
	{
		// Uniform over the rectangle's area.
		const Vec3 point = l.position + l.halfU * (2.0f * u1 - 1.0f) + l.halfV * (2.0f * u2 - 1.0f);
		const Vec3 delta = point - from;
		const float distSq = dot(delta, delta);
		if (distSq <= 0.0f)
			return s;

		s.distance = std::sqrt(distSq);
		s.direction = delta * (1.0f / s.distance);

		float cosLight = -dot(s.direction, rectNormal(l));
		if (!l.twoSided && cosLight <= 0.0f)
			return s; // shading point is behind a one-sided window

		cosLight = std::fabs(cosLight);
		if (cosLight <= 1.0e-6f)
			return s; // edge on: the solid angle is zero

		// Area to solid angle: pdf_A * r^2 / cos. A rectangle spanned by two
		// HALF extents has area 4|u x v|.
		const float area = 4.0f * length(cross(l.halfU, l.halfV));
		if (area <= 0.0f)
			return s;

		s.pdf = distSq / (cosLight * area);
		s.radiance = l.emission;
		return s;
	}

	// Sphere, sampled over the cone it subtends rather than over its surface.
	// Uniform-area sampling would put half its samples on the far side, all of
	// which are occluded by the lamp itself — pure wasted variance.
	const SphereCone cone = sphereCone(l.position, l.radius, from);
	if (!cone.valid)
		return s;

	s.direction = sampleSphereConeDirection(cone, l.position, from, u1, u2);

	// Stop the shadow ray at the lamp's surface, not at its centre.
	float t = 0.0f;
	if (!intersectSphereLight(l, from, s.direction, 1.0e9f, t))
		t = cone.distance - l.radius;

	s.distance = t;
	s.pdf = conePdf(cone);
	s.radiance = l.emission;
	return s;
}

// The density the sampler WOULD have produced for a given direction. Needed by
// MIS when a BSDF-sampled ray happens to land on an emitter.
float emitterPdf(const Emitter& emitter, const Vec3& from, const Vec3& direction, float hitDistance)
{
	if (emitter.objectIndex >= 0)
		return conePdf(sphereCone(emitter.proxyCentre, emitter.proxyRadius, from));

	const Light& l = *emitter.light;

	if (l.shape == LightShape::Rect)
	{
		const float cosLight = std::fabs(dot(direction, rectNormal(l)));
		if (cosLight <= 1.0e-6f)
			return 0.0f;

		const float area = 4.0f * length(cross(l.halfU, l.halfV));
		if (area <= 0.0f)
			return 0.0f;

		return (hitDistance * hitDistance) / (cosLight * area);
	}

	return conePdf(sphereCone(l.position, l.radius, from));
}

// The power heuristic with beta = 2 (Veach). Squaring pushes the weight harder
// towards whichever strategy had the higher density, which is what suppresses
// the fireflies the balance heuristic leaves behind.
float misWeight(float pdfA, float pdfB)
{
	if (pdfA <= 0.0f)
		return 0.0f;
	if (pdfB <= 0.0f)
		return 1.0f;

	// Written as 1/(1 + (b/a)^2) rather than a^2/(a^2 + b^2).
	//
	// The textbook form squares the densities directly, and densities here are
	// not small numbers: a tight GGX lobe reaches 1e6 and a small distant lamp
	// 1e8. Squaring 1e20 overflows float32 (max 3.4e38) and the ratio becomes
	// inf/inf. Dividing FIRST keeps the intermediate near 1 in every case that
	// matters, and the one remaining extreme is caught explicitly.
	const float ratio = pdfB / pdfA;
	if (ratio > 1.0e18f)
		return 0.0f;

	return 1.0f / (1.0f + ratio * ratio);
}

// ---------------------------------------------------------------------------
// Procedural imperfection
//
// See Imperfection in the header for why this exists. The short version: a
// surface with one roughness everywhere reads as CG no matter how correct the
// light transport is.
// ---------------------------------------------------------------------------

// Integer hash, deliberately NOT the usual sin(dot(p, k)) * 43758.5453 trick.
// That idiom is a portability trap here: its result depends on the platform's
// sin implementation and on whether the compiler contracts the dot product, so
// the committed reference images would stop being reproducible across
// toolchains — the exact property this renderer spends a CMake function
// defending. Integer arithmetic is bit-identical everywhere.
//
// The mixer is Chris Wellons' `lowbias32`, found by a search over multiply-xorshift
// triples for minimum avalanche bias; it is public-domain and small enough to
// verify by reading.
uint32_t hashMix(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7feb352du;
	x ^= x >> 15;
	x *= 0x846ca68bu;
	x ^= x >> 16;
	return x;
}

// A deterministic value in [0,1) for an integer lattice point.
float latticeValue(int ix, int iy, int iz)
{
	uint32_t h = hashMix((uint32_t)(ix * 73856093) ^ (uint32_t)(iy * 19349663)
		^ (uint32_t)(iz * 83492791));
	h = hashMix(h);
	return (float)(h >> 8) * (1.0f / 16777216.0f);
}

// Trilinear value noise with a smoothstep fade.
//
// VALUE noise rather than gradient (Perlin) noise: gradient noise is smoother
// and better for organic shapes, and this wants neither. Surface wear is
// blotchy, and value noise is a third of the code with no visible penalty at
// the frequencies used here.
float valueNoise(const Vec3& p)
{
	const float fx = std::floor(p.x), fy = std::floor(p.y), fz = std::floor(p.z);
	const int ix = (int)fx, iy = (int)fy, iz = (int)fz;

	const float tx = p.x - fx, ty = p.y - fy, tz = p.z - fz;

	// Smoothstep, so the lattice does not show as a grid of creases.
	const float ux = tx * tx * (3.0f - 2.0f * tx);
	const float uy = ty * ty * (3.0f - 2.0f * ty);
	const float uz = tz * tz * (3.0f - 2.0f * tz);

	const auto mix = [](float a, float b, float t) { return a + (b - a) * t; };

	const float x00 = mix(latticeValue(ix, iy, iz), latticeValue(ix + 1, iy, iz), ux);
	const float x10 = mix(latticeValue(ix, iy + 1, iz), latticeValue(ix + 1, iy + 1, iz), ux);
	const float x01 = mix(latticeValue(ix, iy, iz + 1), latticeValue(ix + 1, iy, iz + 1), ux);
	const float x11 = mix(latticeValue(ix, iy + 1, iz + 1), latticeValue(ix + 1, iy + 1, iz + 1), ux);

	return mix(mix(x00, x10, uy), mix(x01, x11, uy), uz);
}

// Fractal sum. Returns roughly [0,1), centred near 0.5.
float fbm(const Vec3& p, int octaves)
{
	float sum = 0.0f;
	float amplitude = 0.5f;
	float total = 0.0f;
	Vec3 q = p;

	for (int i = 0; i < octaves; ++i)
	{
		sum += amplitude * valueNoise(q);
		total += amplitude;
		amplitude *= 0.5f;
		q = q * 2.03f; // not exactly 2, so octaves do not align their lattices
	}

	return (total > 0.0f) ? (sum / total) : 0.5f;
}

// The direction surface features run along: the brush direction where there is
// one, an arbitrary stable tangent otherwise. Shared with shadingFrame below so
// the scratches cannot end up running across the grain they are meant to follow.
Vec3 preferredTangent(const Material& m, const Vec3& normal, const Vec3& position)
{
	switch (m.brush)
	{
	case BrushMode::Fixed:
		return m.brushAxis;

	case BrushMode::Concentric:
	case BrushMode::Radial:
	{
		const Vec3 radial = position - m.brushOrigin;
		const Vec3 axis = normalize(m.brushAxis);
		return (m.brush == BrushMode::Concentric)
			? cross(axis, radial)
			: radial - axis * dot(radial, axis);
	}

	default:
		return makeFrame(normal).x;
	}
}

// Perturbs `roughness` and `normal` in place. Both effects come from the same
// noise field, one octave apart, so a scuffed patch is also a rougher patch —
// which is what wear actually does to a surface.
void applyImperfection(const Material& m, const Vec3& position, Vec3& normal, float& roughness)
{
	const Imperfection& imp = m.imperfection;
	if (imp.roughness <= 0.0f && imp.scratch <= 0.0f)
		return;

	const int octaves = std::clamp(imp.octaves, 1, 6);
	const float frequency = maxf(imp.frequency, 1.0e-3f);

	// Build a frame whose X runs along the grain, then SQUASH the sampling
	// position across it. Squashing the domain across the grain is what
	// stretches the features along it — the noise is isotropic; the anisotropy
	// is entirely in how it is sampled.
	const Frame grain = makeFrameWithTangent(normal, preferredTangent(m, normal, position));
	const float streak = maxf(imp.streak, 1.0f);

	const Vec3 local{
		dot(position, grain.x) / streak,
		dot(position, grain.y),
		dot(position, grain.z),
	};
	const Vec3 sample = local * frequency;

	if (imp.roughness > 0.0f)
	{
		// Centred on 1 so the material's nominal roughness stays its average
		// rather than becoming its floor.
		const float wander = (fbm(sample, octaves) - 0.5f) * 2.0f;
		roughness = maxf(roughness * (1.0f + imp.roughness * wander), 1.0e-3f);
	}

	if (imp.scratch > 0.0f)
	{
		// Central differences on the noise give a gradient; tilting the normal
		// along it is the standard bump-mapping move. Sampled at twice the
		// frequency, so the scratches are finer than the roughness blotching
		// they sit inside.
		const Vec3 fine = sample * 2.0f;
		const float h = 0.5f;
		const float gx = fbm(fine + Vec3{ h, 0.0f, 0.0f }, octaves)
			- fbm(fine - Vec3{ h, 0.0f, 0.0f }, octaves);
		const float gy = fbm(fine + Vec3{ 0.0f, h, 0.0f }, octaves)
			- fbm(fine - Vec3{ 0.0f, h, 0.0f }, octaves);

		// Clamped hard. A shading normal that departs too far from the
		// geometric one gets its path dropped by the wo.z test in the
		// integrator, which shows up as black speckle rather than as scratches.
		const float amount = clampf(imp.scratch, 0.0f, 1.0f) * 0.25f;
		const Vec3 tilted = normal
			+ grain.x * (gx * amount)
			+ grain.y * (gy * amount);

		const Vec3 renormalised = normalize(tilted);
		if (dot(renormalised, normal) > 0.2f)
			normal = renormalised;
	}
}

// ---------------------------------------------------------------------------
// Material dispatch
// ---------------------------------------------------------------------------

// Builds the shading frame, honouring the material's brushing direction.
//
// `anisoScale` (out, in [0,1]) fades the anisotropy to isotropic where the
// brush direction degenerates. Concentric and radial grain are undefined ON
// the brush axis — at the centre of a lathe-turned knob top the tangent
// direction decorrelates pixel to pixel and the sheen collapses into a smudge
// that shifts with resolution. Real turned metal does the same thing for the
// same reason (the groove curvature exceeds the highlight width), and it looks
// like a small matte dot; the smoothstep here reproduces that instead of the
// artefact. The fade key is sin of the angle between the axis and the radial
// arm — a pure direction ratio, so it is scale- and resolution-independent.
Frame shadingFrame(const Material& m, const Vec3& normal, const Vec3& position,
	float& anisoScale)
{
	anisoScale = 1.0f;

	if (m.brush == BrushMode::None || m.anisotropy == 0.0f)
		return makeFrame(normal);

	switch (m.brush)
	{
	case BrushMode::Fixed:
		return makeFrameWithTangent(normal, preferredTangent(m, normal, position));

	case BrushMode::Concentric:
	case BrushMode::Radial:
	{
		const Vec3 radial = position - m.brushOrigin;
		const Vec3 axis = normalize(m.brushAxis);

		const float radialLenSq = dot(radial, radial);

		// |axis x radial| / |radial| = sin(angle between them): 0 on the axis,
		// 1 on the equator. Fully anisotropic above 0.15, matte at the centre.
		const float sinAngle = (radialLenSq > 1.0e-12f)
			? length(cross(axis, radial)) / std::sqrt(radialLenSq) : 0.0f;
		const float t = clampf(sinAngle / 0.15f, 0.0f, 1.0f);
		anisoScale = t * t * (3.0f - 2.0f * t);

		if (anisoScale <= 0.0f)
			return makeFrame(normal);

		// Concentric: the tangent runs AROUND the axis — the way the tool
		// travelled on a lathe, so the highlight forms a ring. Radial: along
		// the spoke instead, a sunburst finish. Shared with the imperfection
		// field so scratches follow the same grain.
		return makeFrameWithTangent(normal, preferredTangent(m, normal, position));
	}

	default:
		return makeFrame(normal);
	}
}

BsdfEval evalBsdf(const Material& m, const Ggx& g, const ConductorIor& ior,
	const Vec3& wo, const Vec3& wi)
{
	switch (m.kind)
	{
	case MaterialKind::Diffuse: return evalDiffuse(m, wi);
	case MaterialKind::Plastic: return evalPlastic(m, g, wo, wi);
	case MaterialKind::Metal:   return evalMetal(g, ior, wo, wi);
	default:                    return {}; // glass and emitters get no NEE
	}
}

bool materialIsSpecularOnly(const Material& m, const Ggx& g)
{
	if (m.kind == MaterialKind::Glass)
		return true; // never connected to a light, regardless of roughness
	if (m.kind == MaterialKind::Metal && g.isSmooth())
		return true;

	return false;
}

// ---------------------------------------------------------------------------
// The integrator
// ---------------------------------------------------------------------------

struct PathResult
{
	Vec3 radiance{};
	float alpha = 0.0f;
};

// Constants derivable from a Material alone, computed ONCE PER RENDER instead
// of at every path vertex. The Gulbrandsen inversion behind ConductorIor and
// the logs behind the absorption coefficient are pure functions of material
// fields, and a knob render was re-deriving the same conductor tens of
// millions of times per frame.
struct ObjectDerived
{
	ConductorIor ior{};
	Vec3 sigmaA{ 0.0f, 0.0f, 0.0f };
};

std::vector<ObjectDerived> buildDerived(const Scene& scene)
{
	std::vector<ObjectDerived> derived(scene.objects.size());
	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		const Material& m = scene.objects[i].material;
		if (m.kind == MaterialKind::Metal)
			derived[i].ior = makeConductorIor(m.colour, m.edgeTint);
		if (m.kind == MaterialKind::Glass && m.absorbDistance > 0.0f)
			derived[i].sigmaA = absorptionCoefficient(m.colour, m.absorbDistance);
	}
	return derived;
}

PathResult tracePath(const Scene& scene, const Settings& settings,
	const std::vector<Emitter>& emitters, const std::vector<ObjectDerived>& derived,
	Vec3 origin, Vec3 direction, Rng& rng)
{
	PathResult result;

	Vec3 throughput{ 1.0f, 1.0f, 1.0f };

	// Whether the previous vertex left emission collection to BSDF sampling
	// alone, and how likely the direction we arrived along was. Together these
	// decide the MIS weight applied to any emission found at the next vertex.
	bool previousWasSpecular = true;
	float previousBsdfPdf = 1.0f;

	// Volume state: WHICH object the ray is inside, or -1 for vacuum. An index
	// rather than a material pointer because the marcher needs to know which
	// single object's field to negate — negating the whole scene's would step
	// clean over anything embedded in the glass.
	int mediumIndex = -1;

	const float emitterCount = (float)emitters.size();
	const float selectionPdf = (emitterCount > 0.0f) ? (1.0f / emitterCount) : 0.0f;

	for (int bounce = 0; bounce < settings.maxBounces; ++bounce)
	{
		const bool primary = (bounce == 0);

		const Hit hit = intersect(scene, origin, direction, kMaxTraceDistance, primary, mediumIndex);

		if (!hit.valid)
		{
			// Escaped. Primary rays that escape are the transparent background
			// and must not pick up the environment colour, or the bitmap would
			// composite as a grey square.
			if (!primary)
				result.radiance += throughput * scene.background;
			break;
		}

		// Beer-Lambert over the segment just travelled, applied BEFORE the
		// vertex is shaded so the light arriving here is already tinted.
		if (mediumIndex >= 0 && !derived[mediumIndex].sigmaA.isBlack())
			throughput *= expv(-(derived[mediumIndex].sigmaA * hit.t));

		// Anything a camera ray touches counts as coverage. Glass included: it
		// is "there" even though you can see through it.
		if (primary)
			result.alpha = 1.0f;

		// --- emission ------------------------------------------------------
		//
		// Emission is added HERE and only here, whether the emitter was reached
		// by a BSDF sample or found by accident. Adding it again where the
		// shadow ray "arrives" at a light would double every lit surface.
		const bool isAnalyticLight = (hit.light != nullptr);
		const bool isEmissiveObject =
			(hit.material != nullptr && hit.material->kind == MaterialKind::Emissive);

		if (isAnalyticLight || isEmissiveObject)
		{
			Vec3 emission{};
			bool emits = false;

			if (isAnalyticLight)
			{
				const bool front = dot(direction, hit.normal) < 0.0f;
				emits = hit.light->twoSided || front || hit.light->shape == LightShape::Sphere;
				emission = hit.light->emission;
			}
			else
			{
				emits = true;
				emission = hit.material->emission;
			}

			if (emits)
			{
				float weight = 1.0f;

				// Only down-weight if this emitter could ALSO have been found by
				// next-event estimation from the previous vertex. A specular
				// bounce cannot be, so it keeps full weight.
				if (!previousWasSpecular && selectionPdf > 0.0f)
				{
					for (const Emitter& e : emitters)
					{
						const bool match = isAnalyticLight
							? (e.lightIndex == hit.lightIndex)
							: (e.objectIndex == hit.objectIndex);
						if (!match)
							continue;

						const float pdfLight =
							emitterPdf(e, origin, direction, hit.t) * selectionPdf;
						weight = misWeight(previousBsdfPdf, pdfLight);
						break;
					}
				}

				result.radiance += throughput * emission * weight;
			}
			break; // emitters are opaque
		}

		const Material& m = *hit.material;

		// --- build the local frame -----------------------------------------
		//
		// Imperfection is applied FIRST, because it moves the shading normal
		// and the frame has to be built around where the normal ended up.
		Vec3 shadingNormal = hit.normal;
		float roughness = m.roughness;
		applyImperfection(m, hit.position, shadingNormal, roughness);

		float anisoScale = 1.0f;
		const Frame frame = shadingFrame(m, shadingNormal, hit.position, anisoScale);
		const Vec3 wo = frame.toLocal(-direction);
		if (wo.z <= 0.0f)
			break; // shading normal disagrees with the geometry; drop the path

		const Ggx ggx = makeGgx(roughness, m.anisotropy * anisoScale);
		const ConductorIor& ior = derived[hit.objectIndex].ior;

		// Whether next-event estimation runs at THIS vertex. Computed once and
		// reused for the MIS flag below, because the two must agree: the MIS
		// down-weight on emission found at the NEXT vertex exists solely to
		// avoid double counting against a light-sample estimator that ran HERE.
		const bool neeEligible = !materialIsSpecularOnly(m, ggx) && selectionPdf > 0.0f;

		// --- next event estimation -----------------------------------------
		if (neeEligible)
		{
			const int index = std::min((int)(rng.next() * emitterCount), (int)emitters.size() - 1);
			const Emitter& emitter = emitters[index];

			LightSample ls = sampleEmitter(emitter, hit.position, rng.next(), rng.next());
			if (ls.pdf > 0.0f)
			{
				const Vec3 wi = frame.toLocal(ls.direction);
				if (wi.z > 0.0f)
				{
					const BsdfEval e = evalBsdf(m, ggx, ior, wo, wi);
					if (e.pdf > 0.0f && !e.value.isBlack())
					{
						// hit.normal, not the bumped shading normal: the offset
					// exists to clear the actual surface, and a tilted normal
					// would push the origin along the surface instead of off it.
					const Vec3 shadowOrigin = hit.position + hit.normal * kRayOffset;
						bool visible = false;

						if (emitter.objectIndex >= 0)
						{
							// An emissive OBJECT needs a full trace rather than
							// a yes/no occlusion test: the cone aims at the
							// bounding sphere, so the ray has to be confirmed to
							// have landed on the glowing thing rather than
							// slipped past a corner or hit something in front.
							// Bounded at ls.distance — the far side of the
							// bounds — because nothing beyond it can be the
							// target; cone samples that miss used to keep
							// marching to the far room wall before failing.
							// wantNormal false: only the identity matters here.
							const Hit probe = intersect(scene, shadowOrigin, ls.direction,
								ls.distance + kRayOffset * 4.0f, false, -1, false);
							if (probe.valid && probe.objectIndex == emitter.objectIndex)
							{
								visible = true;
								ls.radiance = scene.objects[emitter.objectIndex].material.emission;
							}
						}
						else
						{
							visible = !occluded(scene, shadowOrigin, ls.direction, ls.distance);
						}

						if (visible && !ls.radiance.isBlack())
						{
							const float pdfLight = ls.pdf * selectionPdf;
							const float weight = misWeight(pdfLight, e.pdf);
							result.radiance +=
								throughput * e.value * ls.radiance * (weight / pdfLight);
						}
					}
				}
			}
		}

		// --- BSDF sampling -------------------------------------------------
		BsdfSample bs;
		switch (m.kind)
		{
		case MaterialKind::Diffuse:
			bs = sampleDiffuse(m, rng.next(), rng.next());
			break;
		case MaterialKind::Plastic:
			bs = samplePlastic(m, ggx, wo, rng.next(), rng.next(), rng.next());
			break;
		case MaterialKind::Metal:
			bs = sampleMetal(ggx, ior, wo, rng.next(), rng.next());
			break;
		case MaterialKind::Glass:
			// `hit.backface` is true when the ray was already inside the glass,
			// so it is exactly the "leaving" test the BSDF needs.
			bs = sampleGlass(m, ggx, wo, !hit.backface, rng.next(), rng.next(), rng.next());
			break;
		default:
			break;
		}

		if (bs.pdf <= 0.0f || bs.weight.isBlack())
			break;

		throughput *= bs.weight;

		const Vec3 nextDirection = frame.toWorld(bs.direction);

		// Refraction has to restart the march on the far side of the surface,
		// so the offset flips to follow it through.
		const float offsetSign = bs.transmitted ? -1.0f : 1.0f;
		origin = hit.position + hit.normal * (kRayOffset * offsetSign);
		direction = nextDirection;

		if (bs.transmitted)
			mediumIndex = (mediumIndex == hit.objectIndex) ? -1 : hit.objectIndex;

		// NOT plain bs.specular. Rough glass samples are honest about having a
		// finite pdf (specular = false), but glass NEVER gets NEE — so emission
		// its rays find has no light-sampling twin to balance against, and
		// down-weighting it would only delete energy: a lamp seen through
		// frosted glass rendered 5-50x too dark exactly this way. The rule the
		// MIS weight actually encodes is COULD-NEE-ALSO-HAVE-FOUND-IT, and that
		// is neeEligible, not smoothness.
		previousWasSpecular = bs.specular || !neeEligible;
		previousBsdfPdf = bs.pdf;

		// --- Russian roulette ----------------------------------------------
		// Only after settings.rouletteDepth bounces: killing paths early saves
		// little and adds noise exactly where the image is dimmest — glass
		// interiors, bore shadows. A depth at or past maxBounces disables it,
		// which is what the Ultra rung does.
		if (bounce >= settings.rouletteDepth)
		{
			const float survive = clampf(throughput.maxComponent(), 0.0f, 0.95f);
			if (rng.next() >= survive)
				break;

			throughput *= (1.0f / maxf(survive, 1.0e-4f));
		}
	}

	// Firefly clamp. Deliberately on the WHOLE path's radiance rather than per
	// vertex, so a legitimately bright but consistent result survives and only
	// the rare lucky path is cut back.
	//
	// std::isfinite is not used to catch the pathological cases, because
	// /fp:fast permits the compiler to assume it always returns true. A plain
	// magnitude comparison is what actually holds under those flags.
	if (settings.clampRadiance > 0.0f)
	{
		const float peak = result.radiance.maxComponent();
		if (peak > settings.clampRadiance)
			result.radiance *= (settings.clampRadiance / peak);
	}

	return result;
}

// ---------------------------------------------------------------------------
// Fast preview shading
//
// No light transport whatsoever: one primary ray, then a closed-form shade from
// the surface normal. This is the classic "solid"/workbench shading every
// modelling tool carries next to its renderer, and it is here for the same
// reason — geometry iteration wants resolution and silhouette, not photons.
//
// Deliberately NOT physically based, and deliberately not a cheap path tracer:
// there is no sampling, no pdf, no estimator, so there is nothing here that can
// be subtly biased. The worst a mistake in this file can do is look wrong.
// ---------------------------------------------------------------------------

// Direction TO the key light, = normalize({-3.6, 3.0, 3.4}), which is exactly
// where addStudio puts the studio key. Matching it matters: the preview and the
// full render then agree about which side of a chamfer is the lit one, so a
// bevel tuned in preview does not flip when the real lights arrive.
constexpr Vec3 kFastLightDirection{ -0.6218f, 0.5182f, 0.5872f };

// Wrapped rather than clamped diffuse: `ambient + key * (0.5 + 0.5 cos)` never
// reaches zero, so a surface facing away keeps its form instead of going to a
// black silhouette. Losing the unlit side is precisely losing half the geometry
// you opened the preview to look at.
constexpr float kFastAmbient = 0.18f;
constexpr float kFastKey = 0.90f;

// A fixed Blinn-Phong pop with no Fresnel and no roughness. Its whole job is to
// put a highlight ON EDGES: a chamfer or fillet sweeps through the mirror angle
// somewhere along its width, so the band appears roughly where the real render
// will put it — which is what makes an edge treatment judgeable at all.
constexpr float kFastSpecular = 0.30f;
constexpr float kFastSpecularPower = 32.0f;

// A grazing-angle lift, and the term that makes this mode useful on the shape
// it exists for.
//
// Seen head-on under an orthographic camera, a flat face has exactly ONE normal
// and therefore exactly one shade. The full render escapes that because a metal
// face is a picture of the room; the preview has no room, so without this a
// knob previews as a plain disc with a dot in the middle. This is zero on a
// surface square to the camera and rises wherever one turns away, so it draws
// precisely the features that TURNING is made of: chamfers, fillets, groove
// walls, the falloff of a crown. It is a curvature cue, not a Fresnel term —
// there is no physics claimed here.
constexpr float kFastRim = 0.45f;

Vec3 shadeFast(const Hit& hit, const Vec3& rayDirection)
{
	// Emitters keep their emission, so an LED still reads as ON and a visible
	// softbox still reads as a light rather than as a white wall.
	if (hit.light)
		return hit.light->emission;

	if (!hit.material)
		return {};

	const Material& m = *hit.material;
	if (m.kind == MaterialKind::Emissive)
		return m.emission;

	// `colour` means something different per material — albedo for diffuse and
	// plastic, normal-incidence reflectance for metal, transmitted tint for
	// glass — but all four are "roughly what this thing looks like", which is
	// all a geometry pass needs to keep the parts tellable apart. Glass shades
	// as an opaque solid of its own tint, on purpose: a preview that showed
	// through it would hide the shape being previewed.
	const Vec3 normal = hit.normal; // already oriented against the ray
	const Vec3 view = -rayDirection;

	const float wrap = clampf(0.5f + 0.5f * dot(normal, kFastLightDirection), 0.0f, 1.0f);

	// Multiplied into the albedo rather than added on top, so a turning surface
	// keeps its own colour instead of washing towards white. Brass that goes
	// pale at every chamfer stops looking like brass.
	const float rim = kFastRim * (1.0f - clampf(dot(normal, view), 0.0f, 1.0f));

	const Vec3 half = normalize(kFastLightDirection + view);
	const float specular = kFastSpecular
		* std::pow(maxf(dot(normal, half), 0.0f), kFastSpecularPower);

	return m.colour * (kFastAmbient + kFastKey * wrap + rim) + Vec3{ specular };
}

// ---------------------------------------------------------------------------
// Pixel filtering and stratification
// ---------------------------------------------------------------------------

struct FilterOffset
{
	float x = 0.5f;
	float y = 0.5f;
};

// Warp a uniform (u1, u2) into a sub-pixel offset drawn from the filter's own
// distribution (filter importance sampling — see PixelFilter in the header for
// why this and not splatting). Offsets are relative to the pixel's top-left
// corner; Tent and Gaussian range outside [0,1), which is the point — that is
// the sample reaching into what a splatting filter would have borrowed from
// the neighbours.
FilterOffset filterOffset(PixelFilter filter, float u1, float u2)
{
	switch (filter)
	{
	default:
	case PixelFilter::Box:
		return { u1, u2 };

	case PixelFilter::Tent:
	{
		// Inverse CDF of the triangle on [-1, 1], one axis at a time (the 2D
		// tent is separable).
		const auto invTent = [](float u)
		{
			return (u < 0.5f) ? (safeSqrt(2.0f * u) - 1.0f)
				: (1.0f - safeSqrt(2.0f - 2.0f * u));
		};
		return { 0.5f + invTent(u1), 0.5f + invTent(u2) };
	}

	case PixelFilter::Gaussian:
	{
		// Box-Muller, sigma 0.45px, CLAMPED at 1.25px rather than rejected:
		// rejection would consume a variable number of RNG draws and shift
		// every draw the path makes afterwards, and the sliver of mass parked
		// on the clamp ring is far below anything visible.
		constexpr float kSigma = 0.45f;
		constexpr float kMaxRadius = 1.25f;

		// 1 - u1 is in (0, 1], so the log is finite.
		const float radius = minf(kMaxRadius,
			kSigma * safeSqrt(-2.0f * std::log(maxf(1.0f - u1, 1.0e-7f))));
		const float phi = 2.0f * kPi * u2;
		return { 0.5f + radius * std::cos(phi), 0.5f + radius * std::sin(phi) };
	}
	}
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

struct CameraBasis
{
	Vec3 right{}, up{}, forward{}, origin{};
	float halfWidth = 1.0f, halfHeight = 1.0f;
};

CameraBasis makeCameraBasis(const Camera& camera, const Settings& settings)
{
	CameraBasis b;
	b.forward = normalize(camera.target - camera.position);
	b.right = normalize(cross(b.forward, camera.up));
	b.up = cross(b.right, b.forward);

	b.halfWidth = camera.filmWidth * 0.5f;
	const float aspect = (settings.width > 0)
		? (float)settings.height / (float)settings.width : 1.0f;
	b.halfHeight = b.halfWidth * aspect;

	// An orthographic ray has no natural origin: every ray is parallel, so the
	// starting point has to be pushed back far enough to clear everything the
	// camera could see. Not doing this puts the film plane inside the subject
	// and slices it in half.
	b.origin = camera.position - b.forward * camera.nearPullback;
	return b;
}

}

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

Transform makeTransform(const Vec3& origin, const Vec3& axis, float degrees, float scale)
{
	const Vec3 a = normalize(axis);
	const float radians = degrees * (kPi / 180.0f);
	const float c = std::cos(radians);
	const float s = std::sin(radians);
	const float ic = 1.0f - c;

	Transform t;
	t.origin = origin;
	t.scale = (scale > 0.0f) ? scale : 1.0f;

	// Rodrigues' rotation, written out as the three basis vectors.
	t.axisX = { c + a.x * a.x * ic, a.x * a.y * ic + a.z * s, a.x * a.z * ic - a.y * s };
	t.axisY = { a.y * a.x * ic - a.z * s, c + a.y * a.y * ic, a.y * a.z * ic + a.x * s };
	t.axisZ = { a.z * a.x * ic + a.y * s, a.z * a.y * ic - a.x * s, c + a.z * a.z * ic };
	return t;
}

void addStudio(Scene& scene, const Studio& studio)
{
	// A rectangle placed at `position` and aimed at `aim`, sized in its own
	// plane. Aiming is done here rather than by the caller because a softbox
	// that is not pointed at the subject contributes nothing but bounce.
	const auto aimedRect = [&](const Vec3& position, float halfWidth, float halfHeight,
		const Vec3& emission)
	{
		Light l;
		l.shape = LightShape::Rect;
		l.position = position;

		const Vec3 forward = normalize(studio.aim - position);
		// Any up vector not parallel to `forward`; world up unless that is what
		// we are pointing along, in which case fall back to Z.
		const Vec3 reference = (std::fabs(forward.y) > 0.95f)
			? Vec3{ 0.0f, 0.0f, 1.0f } : Vec3{ 0.0f, 1.0f, 0.0f };

		const Vec3 right = normalize(cross(forward, reference));
		const Vec3 up = cross(right, forward);

		l.halfU = right * halfWidth;
		l.halfV = up * halfHeight;
		// cross(halfU, halfV) must point AT the subject, since the light is
		// one-sided and emits along its normal.
		if (dot(cross(l.halfU, l.halfV), forward) < 0.0f)
			l.halfV = -l.halfV;

		l.emission = emission;
		l.twoSided = false;
		l.cameraVisible = false;
		return l;
	};

	if (studio.enableRoom)
	{
		// The room. Invisible to the camera, so it lights and reflects without
		// ever appearing as a background.
		Object room;
		room.material.kind = MaterialKind::Diffuse;
		room.material.colour = studio.wallColour;
		room.material.cameraVisible = false;
		room.unbounded = true;

		const Vec3 half = studio.roomHalfExtents;
		room.distance = [half](const Vec3& p) { return sdRoomInterior(p, half); };
		scene.add(std::move(room));
	}

	// A light set to zero is a light turned OFF, so it is not added at all. The
	// renderer would also cope (black lights are filtered from emitters and
	// intersection), but not adding it keeps the scene honest: presets dim a
	// light to almost-nothing to keep it, and zero it to remove it.
	if (!studio.keyEmission.isBlack())
		scene.add(aimedRect(studio.keyPosition, studio.keyHalfWidth, studio.keyHalfHeight,
			studio.keyEmission));
	if (!studio.fillEmission.isBlack())
		scene.add(aimedRect(studio.fillPosition, studio.fillHalfWidth, studio.fillHalfHeight,
			studio.fillEmission));

	if (studio.enableRim)
		scene.add(aimedRect(studio.rimPosition, studio.rimHalfWidth, studio.rimHalfHeight,
			studio.rimEmission));

	if (studio.enableGlint)
	{
		Light glint;
		glint.shape = LightShape::Sphere;
		glint.position = studio.glintPosition;
		glint.radius = studio.glintRadius;
		glint.emission = studio.glintEmission;
		glint.cameraVisible = false;
		scene.add(glint);
	}
}

void renderProgressive(const Scene& scene, const Camera& camera, const Settings& settings,
	Image& image, int sampleBegin, int sampleEnd)
{
	// The first batch owns the allocation; later batches must arrive with the
	// image exactly as the previous batch left it — see the header contract.
	if (sampleBegin <= 0)
	{
		sampleBegin = 0;
		image.width = (settings.width > 1) ? settings.width : 1;
		image.height = (settings.height > 1) ? settings.height : 1;
		image.rgba.assign((size_t)image.width * (size_t)image.height * 4, 0.0f);
	}

	const CameraBasis basis = makeCameraBasis(camera, settings);

	// Built once, not per path: it holds POINTERS into the scene, so the scene
	// must not be touched while a render is in flight.
	const std::vector<Emitter> emitters = buildEmitters(scene);
	const std::vector<ObjectDerived> derived = buildDerived(scene);

	const bool fast = (settings.mode == RenderMode::Fast);

	// Fast mode is exact, so "progressive" means nothing to it: the first batch
	// renders the whole image and later batches would only redo it.
	if (fast && sampleBegin > 0)
		return;

	if (!fast && sampleEnd <= sampleBegin)
		return;

	// Fast mode's samples are a fixed sub-pixel GRID, not jittered draws: with
	// no light transport the shade is exact, so jitter would add noise to an
	// image that has none. It also makes the preview bit-deterministic without
	// the RNG being involved at all.
	const int fastGrid = std::clamp(settings.fastAntiAlias, 1, 8);
	const int samplesPerPixel = fast ? (fastGrid * fastGrid) : sampleEnd;
	const float invSpp = 1.0f / (float)samplesPerPixel;

	// Stratification is anchored to the PLANNED total from settings, not to
	// this batch, so batches land in the same strata they would in a single
	// call — that identity is the whole progressive contract. Samples past the
	// grid (spp not a perfect square, or a caller running long) fall back to
	// plain jitter, which is correct, just unstratified.
	const int strataK = (int)std::sqrt((float)std::max(settings.samplesPerPixel, 1));
	const int strataCount = strataK * strataK;

	// Tiles rather than scanlines: a tile is a compact region of the image, so
	// the threads working on it touch nearby geometry and the bounding-sphere
	// rejections behave consistently. Scanline scheduling gives one thread the
	// empty background and another the whole subject.
	constexpr int kTile = 16;
	const int tilesX = (image.width + kTile - 1) / kTile;
	const int tilesY = (image.height + kTile - 1) / kTile;
	const int tileCount = tilesX * tilesY;

	int threadCount = settings.threads;
	if (threadCount <= 0)
		threadCount = (int)std::thread::hardware_concurrency();
	threadCount = std::clamp(threadCount, 1, 64);

	std::atomic<int> nextTile{ 0 };

	// Pixel plus a sub-pixel offset in [0,1) -> the ray's starting point.
	// THE orthographic camera: every ray shares `basis.forward` and they differ
	// only in where they start.
	const auto rayOrigin = [&](int x, int y, float offsetX, float offsetY)
	{
		const float px = ((float)x + offsetX) / (float)image.width;
		const float py = ((float)y + offsetY) / (float)image.height;

		// Film coordinates: +1 right, +1 up. Image rows run top to bottom,
		// hence the flip on py.
		const float fx = (px * 2.0f - 1.0f) * basis.halfWidth;
		const float fy = (1.0f - py * 2.0f) * basis.halfHeight;

		return basis.origin + basis.right * fx + basis.up * fy;
	};

	const auto worker = [&]()
	{
		for (;;)
		{
			const int tile = nextTile.fetch_add(1);
			if (tile >= tileCount)
				return;

			const int x0 = (tile % tilesX) * kTile;
			const int y0 = (tile / tilesX) * kTile;
			const int x1 = std::min(x0 + kTile, image.width);
			const int y1 = std::min(y0 + kTile, image.height);

			for (int y = y0; y < y1; ++y)
			{
				for (int x = x0; x < x1; ++x)
				{
					Vec3 sum{ 0.0f };
					float alphaSum = 0.0f;

					// Resume from the running average: multiplying it back up
					// by sampleBegin recovers the sum this pixel had, so the
					// batch is a straight continuation of the same estimator.
					const size_t i = ((size_t)y * (size_t)image.width + (size_t)x) * 4;
					if (!fast && sampleBegin > 0)
					{
						sum = Vec3{ image.rgba[i + 0], image.rgba[i + 1], image.rgba[i + 2] }
							* (float)sampleBegin;
						alphaSum = image.rgba[i + 3] * (float)sampleBegin;
					}

					if (fast)
					{
						for (int sy = 0; sy < fastGrid; ++sy)
						{
							for (int sx = 0; sx < fastGrid; ++sx)
							{
								// Cell CENTRES, so the grid is symmetric about
								// the pixel and an edge lands the same way from
								// either side.
								const Vec3 origin = rayOrigin(x, y,
									((float)sx + 0.5f) / (float)fastGrid,
									((float)sy + 0.5f) / (float)fastGrid);

								// primary = true and mediumIndex = -1: the same
								// call the full render makes for its camera ray,
								// so camera-invisible geometry stays invisible
								// and the alpha channel comes out identical.
								const Hit hit = intersect(scene, origin, basis.forward,
									kMaxTraceDistance, true, -1);
								if (!hit.valid)
									continue;

								sum += shadeFast(hit, basis.forward);
								alphaSum += 1.0f;
							}
						}
					}
					else
					{
						for (int s = sampleBegin; s < sampleEnd; ++s)
						{
							// Seeded from the PIXEL, not from a shared counter,
							// so the image does not depend on how the tiles were
							// scheduled across threads.
							Rng rng = seedRng((uint32_t)x, (uint32_t)y, (uint32_t)s, settings.seed);

							// The two draws are SEPARATE STATEMENTS on purpose.
							// Passing them straight into a call as two arguments
							// reads better and is a determinism bug: C++ leaves
							// argument evaluation order unspecified, so which
							// draw becomes x and which becomes y is the
							// compiler's choice, and the committed reference
							// images stop being reproducible across toolchains.
							// (Written that way once; the image regression
							// caught it immediately.)
							float u1 = rng.next();
							float u2 = rng.next();

							// Stratify, then warp through the reconstruction
							// filter — so the strata survive the warp and land
							// as evenly spread FILTERED positions.
							if (s < strataCount)
							{
								u1 = ((float)(s % strataK) + u1) / (float)strataK;
								u2 = ((float)(s / strataK) + u2) / (float)strataK;
							}

							const FilterOffset f = filterOffset(settings.filter, u1, u2);
							const Vec3 origin = rayOrigin(x, y, f.x, f.y);

							const PathResult r = tracePath(scene, settings, emitters, derived,
								origin, basis.forward, rng);
							sum += r.radiance;
							alphaSum += r.alpha;
						}
					}

					image.rgba[i + 0] = sum.x * invSpp;
					image.rgba[i + 1] = sum.y * invSpp;
					image.rgba[i + 2] = sum.z * invSpp;
					image.rgba[i + 3] = alphaSum * invSpp;
				}
			}
		}
	};

	if (threadCount == 1)
	{
		worker();
	}
	else
	{
		std::vector<std::thread> threads;
		threads.reserve((size_t)threadCount);
		for (int i = 0; i < threadCount; ++i)
			threads.emplace_back(worker);
		for (std::thread& t : threads)
			t.join();
	}
}

Image render(const Scene& scene, const Camera& camera, const Settings& settings)
{
	Image image;
	renderProgressive(scene, camera, settings, image,
		0, std::max(settings.samplesPerPixel, 1));

	if (settings.bloomStrength > 0.0f)
		applyBloom(image, settings.bloomStrength, settings.bloomThreshold);

	return image;
}

// Three successive box blurs of equal radius converge on a Gaussian (central
// limit theorem doing the work), each a running-sum sweep — O(pixels) per pass
// regardless of radius, where a true Gaussian kernel at hero sizes would be a
// hundred taps per pixel. Edges renormalise by the true window size instead of
// borrowing zeros, so the halo does not dim against the frame border.
namespace
{
void boxBlurPass(std::vector<float>& src, std::vector<float>& dst,
	int width, int height, int radius, bool horizontal)
{
	const int lineCount = horizontal ? height : width;
	const int lineLength = horizontal ? width : height;
	const size_t strideAlong = horizontal ? 3 : (size_t)width * 3;
	const size_t strideAcross = horizontal ? (size_t)width * 3 : 3;

	for (int line = 0; line < lineCount; ++line)
	{
		const size_t base = (size_t)line * strideAcross;
		for (int c = 0; c < 3; ++c)
		{
			float sum = 0.0f;
			// Prime the window for position 0: [0, radius].
			for (int j = 0; j <= std::min(radius, lineLength - 1); ++j)
				sum += src[base + (size_t)j * strideAlong + c];

			for (int pos = 0; pos < lineLength; ++pos)
			{
				const int lo = std::max(0, pos - radius);
				const int hi = std::min(lineLength - 1, pos + radius);
				dst[base + (size_t)pos * strideAlong + c] = sum / (float)(hi - lo + 1);

				// Slide: drop `lo`, admit `hi + 1`.
				if (pos + radius + 1 <= lineLength - 1)
					sum += src[base + (size_t)(pos + radius + 1) * strideAlong + c];
				if (pos - radius >= 0)
					sum -= src[base + (size_t)(pos - radius) * strideAlong + c];
			}
		}
	}
}
}

void applyBloom(Image& image, float strength, float threshold)
{
	if (strength <= 0.0f || image.width <= 0 || image.height <= 0)
		return;

	const int w = image.width;
	const int h = image.height;
	const size_t count = (size_t)w * (size_t)h;

	// Bright pass, straight off the premultiplied HDR: only what exceeds the
	// threshold blooms, so lit surfaces stay put and sources and glints bleed.
	std::vector<float> bright(count * 3);
	for (size_t i = 0; i < count; ++i)
		for (int c = 0; c < 3; ++c)
			bright[i * 3 + c] = maxf(image.rgba[i * 4 + c] - threshold, 0.0f);

	// Halo width scales with the frame (2% of the long side), so the look
	// survives a resolution change instead of tightening into an outline.
	const float sigma = clampf(0.02f * (float)std::max(w, h), 1.0f, 48.0f);
	const int radius = std::max(1, (int)std::lround((std::sqrt(4.0f * sigma * sigma / 3.0f + 1.0f) - 1.0f) * 0.5f));

	std::vector<float> scratch(count * 3);
	for (int pass = 0; pass < 3; ++pass)
	{
		boxBlurPass(bright, scratch, w, h, radius, true);
		boxBlurPass(scratch, bright, w, h, radius, false);
	}

	for (size_t i = 0; i < count; ++i)
	{
		const float r = strength * bright[i * 3 + 0];
		const float g = strength * bright[i * 3 + 1];
		const float b = strength * bright[i * 3 + 2];

		image.rgba[i * 4 + 0] += r;
		image.rgba[i * 4 + 1] += g;
		image.rgba[i * 4 + 2] += b;

		// The halo carries its own coverage. Without this, writePixels'
		// unpremultiply zeroes every bloomed pixel outside the silhouette and
		// the glow ends dead at the object's edge.
		const float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
		image.rgba[i * 4 + 3] = minf(image.rgba[i * 4 + 3] + luma, 1.0f);
	}
}

int samplesFor(int width, int height, int base, int floor, int referencePixels)
{
	const double pixels = (double)std::max(width, 1) * (double)std::max(height, 1);
	if (pixels <= (double)referencePixels)
		return base;

	// Taper by the square root of the area ratio: cost grows with WIDTH rather
	// than area, which is what keeps a wide panel affordable in the product.
	const double taper = std::sqrt((double)referencePixels / pixels);
	return std::max(floor, (int)std::lround((double)base * taper));
}

Settings qualitySettings(Quality quality, int width, int height)
{
	Settings s;
	s.width = width;
	s.height = height;

	switch (quality)
	{
	case Quality::Draft:
		s.mode = RenderMode::Fast;
		s.fastAntiAlias = 2;
		break;

	case Quality::Standard:
		// The shipped faceplate budget: 128 paths tapering past a one-unit
		// panel at 2x, floor 64.
		//
		// The bounce cap is 12 rather than the 8 TiDEPanel used, because 8 was
		// a saving that never existed. Measured on the glass scene, 8 bounces
		// loses 1.37% of the converged energy where 12 loses 0.18% — and
		// measured on render time, raising it costs +0.8% (materials) to +2.1%
		// (glass), with the metal knob inside noise. Russian roulette at depth
		// 3 was already ending dim paths long before the cap could bind, so
		// the cap was only ever truncating the BRIGHT paths that matter, and
		// only on transmissive geometry. Faceplates never noticed either way;
		// glass did.
		s.samplesPerPixel = samplesFor(width, height, 128, 64);
		s.maxBounces = 12;
		s.clampRadiance = 12.0f;
		s.rouletteDepth = 3;
		s.filter = PixelFilter::Tent;
		break;

	case Quality::High:
		// 4x the paths, roulette held back to bounce 8, the clamp loosened to
		// only catch true outliers. About a second for a knob-sized bitmap —
		// the background-refine quality, not the interactive one.
		s.samplesPerPixel = samplesFor(width, height, 512, 128);
		s.maxBounces = 12;
		s.clampRadiance = 48.0f;
		s.rouletteDepth = 8;
		s.filter = PixelFilter::Tent;
		break;

	case Quality::Ultra:
		// Marketing: no taper — a 4K frame gets the full count everywhere —
		// no clamp (every caustic keeps its energy, paid for in samples),
		// roulette effectively off, the Gaussian filter, bloom on. Minutes to
		// hours, and nothing in the product should ask for it implicitly.
		s.samplesPerPixel = 4096;
		s.maxBounces = 24;
		s.clampRadiance = 0.0f;
		s.rouletteDepth = 24;
		s.filter = PixelFilter::Gaussian;
		s.bloomStrength = 0.06f;
		break;
	}

	return s;
}

// One implementation for both bit depths: everything up to the very last line
// — unpremultiply, tone map, exact sRGB encode, re-premultiply — is identical,
// and duplicating it is how the 8- and 16-bit paths would drift apart. Only
// the quantum differs: 255 or 65535.
template <typename Word, int kMaxCode>
void writePixelsImpl(const Image& image, Word* dst, int32_t bytesPerRow,
	PixelOrder order, bool premultiply, float exposure, float whitePoint)
{
	if (!dst || image.width <= 0 || image.height <= 0)
		return;

	// Word offsets of R, G and B within each 4-word pixel. Alpha is last in
	// both layouts, so only R and B ever swap.
	const int rIndex = (order == PixelOrder::Rgba) ? 0 : 2;
	const int bIndex = (order == PixelOrder::Rgba) ? 2 : 0;

	// Extended Reinhard: rolls the highlights off towards `whitePoint` while
	// leaving the midtones almost linear, so a colour picked from the UI palette
	// still comes out close to the value that was asked for. Applied per channel
	// rather than to luminance, which desaturates very bright highlights — the
	// right behaviour for a specular glint, which really is white.
	const float w2 = maxf(whitePoint, 1.0e-3f) * maxf(whitePoint, 1.0e-3f);

	const auto toneMap = [w2](float v)
	{
		v = maxf(v, 0.0f);
		return v * (1.0f + v / w2) / (1.0f + v);
	};

	// The exact piecewise sRGB transfer function, not the 2.2 power
	// approximation. The two diverge most in the deep shadows, which for a dark
	// panel is precisely where the render spends its time.
	const auto encodeSrgb = [](float v)
	{
		v = clampf(v, 0.0f, 1.0f);
		return (v <= 0.0031308f) ? (v * 12.92f)
			: (1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f);
	};

	for (int y = 0; y < image.height; ++y)
	{
		// bytesPerRow stays BYTES for both widths, matching what a locked
		// bitmap reports.
		Word* row = (Word*)((uint8_t*)dst + (size_t)y * (size_t)bytesPerRow);
		for (int x = 0; x < image.width; ++x)
		{
			const size_t i = ((size_t)y * (size_t)image.width + (size_t)x) * 4;
			const float alpha = clampf(image.rgba[i + 3], 0.0f, 1.0f);

			// UNPREMULTIPLY before tone mapping, always — even when the output
			// is going to be premultiplied again two lines later.
			//
			// Tone mapping is non-linear, so T(a*C) != a*T(C). Feeding it a
			// half-covered edge pixel whose colour has already been scaled by
			// 0.5 lands that pixel on a completely different part of the curve
			// from its fully-covered neighbour, and the silhouette picks up a
			// dark or milky fringe that no amount of extra sampling removes.
			const float unpremultiply = (alpha > 1.0e-4f) ? (1.0f / alpha) : 0.0f;

			float channel[3] = {
				image.rgba[i + 0] * unpremultiply * exposure,
				image.rgba[i + 1] * unpremultiply * exposure,
				image.rgba[i + 2] * unpremultiply * exposure,
			};

			for (float& c : channel)
				c = encodeSrgb(toneMap(c));

			// Premultiplication happens AFTER the sRGB encode, on purpose.
			//
			// Physically the right place is linear space, and doing it there is
			// what a compositing textbook says. But Direct2D and CoreGraphics
			// both define a premultiplied sRGB surface as holding
			// encode(colour) * alpha, and they un-premultiply by dividing the
			// ENCODED value. Premultiplying in linear and then encoding
			// disagrees with that by exactly the nonlinearity of the curve,
			// which shows up as a dark halo around every antialiased edge.
			// Matching the compositor wins over matching the textbook.
			const float scale = premultiply ? alpha : 1.0f;

			constexpr float kMax = (float)kMaxCode;
			Word* px = row + (size_t)x * 4;
			px[rIndex] = (Word)(clampf(channel[0] * scale, 0.0f, 1.0f) * kMax + 0.5f);
			px[1] = (Word)(clampf(channel[1] * scale, 0.0f, 1.0f) * kMax + 0.5f);
			px[bIndex] = (Word)(clampf(channel[2] * scale, 0.0f, 1.0f) * kMax + 0.5f);
			px[3] = (Word)(alpha * kMax + 0.5f);
		}
	}
}

void writePixels(const Image& image, uint8_t* dst, int32_t bytesPerRow,
	PixelOrder order, bool premultiply, float exposure, float whitePoint)
{
	writePixelsImpl<uint8_t, 255>(image, dst, bytesPerRow, order, premultiply,
		exposure, whitePoint);
}

void writePixels16(const Image& image, uint16_t* dst, int32_t bytesPerRow,
	PixelOrder order, bool premultiply, float exposure, float whitePoint)
{
	writePixelsImpl<uint16_t, 65535>(image, dst, bytesPerRow, order, premultiply,
		exposure, whitePoint);
}

}
