// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "DemoScenes.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

// The NUMERIC half of the renderer's test suite: four properties that the
// header and README claim in words, checked as numbers.
//
// WHY THIS EXISTS ALONGSIDE tests/RenderRegression.cpp, WHICH PINS IMAGES. The
// image test is the right guard for anything an eye can judge — a broken BSDF,
// an inverted normal, a lost light — and it stays the primary one. It is the
// wrong guard for these four, for two separate reasons:
//
//   * Two of them (progressive composition, Fast-vs-Full coverage) are claims
//     that two RENDERS AGREE. An image test compares one render against a
//     committed PNG, so it cannot express "these two must match" at all; and
//     after the 8-bit encode a 2e-7 disagreement and a 2e-3 disagreement both
//     quantise to zero, which is exactly the resolution the claims live at.
//
//   * Two of them (bloom's symmetry and its alpha, the quality ladder's
//     ordering) have no committed picture to compare against, because the
//     sizes and settings they need are not the ones the references are
//     rendered at — and adding references for them would mean ten more PNGs to
//     re-approve by eye every time a look changes.
//
// So: pictures for what an eye judges, numbers for what an eye cannot see.
// Neither replaces the other, and both run under ctest.
//
// WHY NO TEST FRAMEWORK. Same rule as the rest of modules/common — the renderer
// depends on nothing but the C++20 standard library, and a test binary that
// FetchContent-ed gtest would make the one dependency-free corner of this repo
// stop being one. A plain main() that prints a line per check and returns
// non-zero is enough; RenderRegression.cpp is written the same way.
//
//   tide_render_unit          (no arguments — it renders everything it needs)
//
// ON THE NUMBERS IN THE COMMENTS. Each check quotes what it MEASURED when it
// was written, as an anchor. The assertions are set well away from those
// anchors, for the same reason RenderRegression.cpp's tolerances are: a
// different compiler or optimisation level reorders floating point and moves
// the last bit or two. A check that trips here means the PROPERTY broke, not
// that a rounding moved. (`tide_render_strict_fp()` in CMakeLists.txt keeps
// this target off the project's global /fp:fast, so the drift that remains is
// between toolchains rather than within one.)

namespace
{
using namespace tide::render;

int gChecks = 0;

// printf-style into a std::string, so a check can carry its measured number on
// the same line as its verdict. std::format would read better and is C++20, but
// libstdc++ only shipped it in GCC 13 and this file has to compile wherever the
// renderer does; snprintf is everywhere.
std::string text(const char* format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	std::vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	return buffer;
}

// One assertion. It prints the measured value on EVERY line, pass or fail,
// because a column of bare "ok"s cannot tell you whether a tolerance was
// comfortable or was one ulp from tripping — and these numbers are the record
// of what the renderer actually does, which is half of why the file exists.
//
// Returns a failure COUNT rather than a bool so callers can sum them the way
// RenderRegression.cpp's runScene() does.
int check(bool passed, const char* what, const std::string& measured)
{
	++gChecks;
	std::printf("%s %-34s %s\n", passed ? "ok  " : "FAIL", what, measured.c_str());
	return passed ? 0 : 1;
}

// Rec.709 luma — the same weights applyBloom() uses to decide how much coverage
// a halo carries. Any consistent weighting would do for a RATIO test; matching
// the one already in the renderer keeps a single definition of "how bright is
// this" in the codebase.
//
// Accumulated in double. Fourteen thousand float adds of values around one lose
// enough of the tail to be visible in the fourth digit, which is the digit the
// quality-ladder check reports.
double meanLuminance(const Image& image)
{
	const size_t count = (size_t)image.width * (size_t)image.height;
	if (count == 0)
		return 0.0;

	double sum = 0.0;
	for (size_t i = 0; i < count; ++i)
	{
		sum += 0.2126 * (double)image.rgba[i * 4 + 0]
			+ 0.7152 * (double)image.rgba[i * 4 + 1]
			+ 0.0722 * (double)image.rgba[i * 4 + 2];
	}

	return sum / (double)count;
}

// Mean coverage over the whole frame, background included. The background is
// alpha 0, so this is literally "what fraction of the frame the subject covers"
// — which is the quantity the Fast-vs-Full check is about.
double meanAlpha(const Image& image)
{
	const size_t count = (size_t)image.width * (size_t)image.height;
	if (count == 0)
		return 0.0;

	double sum = 0.0;
	for (size_t i = 0; i < count; ++i)
		sum += (double)image.rgba[i * 4 + 3];

	return sum / (double)count;
}

// The demo registry's `glass` scene and the camera it wants.
//
// Fetched through findScene() rather than by calling makeGlassScene() directly,
// so that a scene renamed or dropped from the registry fails HERE — loudly, at
// run time, with a message — instead of leaving this file quietly testing a
// scene the tools no longer offer.
bool buildGlassScene(Scene& scene, Camera& camera)
{
	const tide::demo::SceneDef* def = tide::demo::findScene("glass");
	if (def == nullptr)
	{
		std::printf("FAIL %-34s scene \"glass\" is not in tide::demo::kScenes\n", "scene lookup");
		return false;
	}

	scene = def->build(camera);
	return true;
}

// ---------------------------------------------------------------------------
// 1. Progressive accumulation composes exactly
// ---------------------------------------------------------------------------
//
// renderProgressive() renders a RANGE of samples into an existing image, so a
// consumer can put something on screen after 32 paths and keep refining in
// place. The contract that makes that safe is stated in TidePathTracer.h: every
// sample is seeded from its pixel coordinate and its sample INDEX, never from a
// shared counter, so [0,32) + [32,96) + [96,128) must land on the same estimate
// as [0,128) rendered in one call.
//
// This is the guard on that. Anything that made a sample's value depend on the
// BATCH it arrived in — a per-render RNG stream, a stratification grid anchored
// to the batch instead of to settings.samplesPerPixel, an accumulator that
// stored a sum where the next batch expected an average — would leave every
// single-call render in the image test untouched and silently break every
// progressive consumer. There is no picture that catches it.
//
// WHY THE TOLERANCE IS NOT ZERO. The two runs trace bit-identical paths; what
// differs is the arithmetic that averages them. render() divides one
// 128-sample sum once. The progressive path multiplies a running average back
// up by 32, adds the next batch, divides by 96, and so on — the same value
// through a different sequence of counts, and therefore a different sequence of
// roundings. Measured worst per-component delta: 2.38e-07, about an ulp at
// these magnitudes. The 1e-4 limit is four hundred times that and still far
// tighter than a single misplaced sample could hide in — one path out of 128
// landing in the wrong batch moves its pixel by a percent of its own radiance,
// which is several orders of magnitude the other way.
//
// The `glass` scene at the reference size, because it is the noisiest thing in
// the registry — its caustics arrive only by chance through BSDF sampling — so
// any dependence on batch boundaries has the most room to show itself.
int testProgressiveAccumulation()
{
	Scene scene;
	Camera camera;
	if (!buildGlassScene(scene, camera))
		return 1;

	Settings settings;
	settings.width = 160;
	settings.height = 90;
	settings.samplesPerPixel = 128;

	// Bloom stays off (it is off by default). render() applies bloom only to a
	// COMPLETE image while renderProgressive() never applies it at all, so a
	// non-zero bloomStrength here would compare a bloomed image against an
	// unbloomed one and fail for a reason that has nothing to do with seeding.
	const Image whole = render(scene, camera, settings);

	// Deliberately uneven batches. Equal thirds would still pass if the
	// implementation happened to key its stratification off the batch SIZE;
	// 32 + 64 + 32 does not let that slip through. The batches also straddle
	// the stratification grid's boundary — sqrt(128) = 11, so 121 of the 128
	// samples are stratified and the last 7 are plain jitter — which is the
	// awkward part of the range and exactly where a batch-anchored grid would
	// diverge.
	Image progressive;
	renderProgressive(scene, camera, settings, progressive, 0, 32);
	renderProgressive(scene, camera, settings, progressive, 32, 96);
	renderProgressive(scene, camera, settings, progressive, 96, 128);

	int failures = 0;
	failures += check(progressive.width == whole.width && progressive.height == whole.height,
		"progressive image is the same size",
		text("%dx%d against %dx%d", progressive.width, progressive.height,
			whole.width, whole.height));

	if (failures != 0)
		return failures;

	// Every component of every pixel, alpha included: alpha accumulates through
	// the same running average as radiance and would break the same way.
	float worst = 0.0f;
	int worstX = 0;
	int worstY = 0;
	for (int y = 0; y < whole.height; ++y)
	{
		for (int x = 0; x < whole.width; ++x)
		{
			const size_t i = ((size_t)y * (size_t)whole.width + (size_t)x) * 4;
			for (int c = 0; c < 4; ++c)
			{
				const float delta = std::fabs(progressive.rgba[i + c] - whole.rgba[i + c]);
				if (delta > worst)
				{
					worst = delta;
					worstX = x;
					worstY = y;
				}
			}
		}
	}

	constexpr float kMaxDelta = 1.0e-4f;
	failures += check(worst <= kMaxDelta,
		"progressive batches compose",
		text("worst delta %.3e at (%d,%d), limit %.0e", (double)worst, worstX, worstY,
			(double)kMaxDelta));

	return failures;
}

// ---------------------------------------------------------------------------
// 2. Bloom is symmetric and carries its own alpha
// ---------------------------------------------------------------------------
//
// applyBloom() is a LENS artefact, not light transport: everything above
// `threshold` bleeds a Gaussian-ish halo (three box blurs, per the central
// limit theorem) and the halo raises ALPHA along with it.
//
// That second half is the load-bearing one and is the easy one to lose. Without
// it, writePixels' un-premultiply divides by an alpha of zero everywhere the
// halo spills past the silhouette, and a composited glow ends dead at the
// object's edge. The symptom reads as "bloom looks a bit weak", not as a bug,
// which is why it wants a test rather than an eye.
//
// WHY A SYNTHETIC IMAGE RATHER THAN A DEMO SCENE. The check needs a pixel that
// is bright AND within blur range of a transparent one. At the reference size
// no scene in the registry has that — `glow`'s cubes sit well inside a lit
// frame, several halo widths from any transparency. Rendering one big enough to
// arrange it would turn this into a test of the SCENE (of where its cubes
// happen to sit) rather than of the post-pass, and would cost seconds to
// establish something a hand-built image states exactly in microseconds.
//
// 33x9 is chosen so the halo geometry is known rather than incidental: the halo
// width is 2% of the long side floored at sigma 1, so 33 gives sigma 1 and box
// radius 1, hence a three-pixel reach in each direction. Wide enough for a
// reading two pixels out; narrow enough that the frame edge never enters the
// blur window, so the edge renormalisation is out of the picture and cannot be
// what makes the symmetry check pass.
int testBloom()
{
	Image image;
	image.width = 33;
	image.height = 9;
	image.rgba.assign((size_t)image.width * (size_t)image.height * 4, 0.0f);

	// One bright, fully covered pixel dead centre; everything else transparent
	// black. rgb 10 against a threshold of 0.5 puts 9.5 into the bright pass —
	// a source, not a lit surface.
	constexpr int kCentreX = 16;
	constexpr int kCentreY = 4;
	const size_t centre = ((size_t)kCentreY * (size_t)image.width + (size_t)kCentreX) * 4;
	image.rgba[centre + 0] = 10.0f;
	image.rgba[centre + 1] = 10.0f;
	image.rgba[centre + 2] = 10.0f;
	image.rgba[centre + 3] = 1.0f;

	applyBloom(image, 0.10f, 0.5f);

	// Two pixels out: inside the halo's three-pixel reach, and outside the
	// source pixel itself, so what is read here arrived entirely by blurring.
	const Vec3 halo = image.pixel(kCentreX + 2, kCentreY);
	const float haloAlpha = image.alpha(kCentreX + 2, kCentreY);
	const float haloMin = minf(halo.x, minf(halo.y, halo.z));

	// Loose limits on purpose. The exact halo value depends on the box radius,
	// which depends on the frame size through a lround — pinning it would make
	// this a test of that rounding rather than of the property. What is being
	// asserted is that the halo REACHES here at all and that it brought its
	// coverage with it. Measured: rgb 0.0274, alpha 0.0274 (equal because
	// applyBloom derives the alpha it adds from the luma of what it added, and
	// the luma of a neutral colour is that colour).
	constexpr float kMinHalo = 0.005f;

	int failures = 0;
	failures += check(haloMin > kMinHalo,
		"bloom halo reaches beyond the source",
		text("rgb %.4f %.4f %.4f, floor %.3f", (double)halo.x, (double)halo.y,
			(double)halo.z, (double)kMinHalo));

	failures += check(haloAlpha > kMinHalo,
		"bloom halo carries its own alpha",
		text("alpha %.4f, floor %.3f", (double)haloAlpha, (double)kMinHalo));

	// Mirror symmetry is what catches an off-by-one in the sliding window.
	// boxBlurPass primes a running sum for position 0 and then slides it — drop
	// `lo`, admit `hi + 1` — and every plausible slip in there (priming one
	// short, dropping before writing, an inclusive bound that should be
	// exclusive) SHIFTS the halo by a pixel rather than destroying it. A
	// one-pixel shift of a soft glow is invisible in any picture and exact
	// here: the input is mirror-symmetric about x = 16, so the output must be
	// too.
	//
	// It is not mirror-symmetric to the LAST BIT, and the tolerance is not
	// zero, because the blur is a RUNNING SUM that sweeps each line left to
	// right — admitting one sample and dropping another at every step. The
	// rounding it has accumulated by the time it reaches x = 14 is not the
	// rounding it has accumulated by x = 18, so the two halves of a symmetric
	// halo come out a few ulps apart. Measured worst mismatch: 1.5e-09. The
	// 1e-6 limit is some six hundred times that, and still four orders of
	// magnitude below the halo value it is comparing — so a shift of even one
	// pixel is nowhere near able to slip under it.
	float worstMirror = 0.0f;
	int worstX = 0;
	int worstY = 0;
	for (int y = 0; y < image.height; ++y)
	{
		for (int d = 1; d <= kCentreX; ++d)
		{
			const size_t left = ((size_t)y * (size_t)image.width + (size_t)(kCentreX - d)) * 4;
			const size_t right = ((size_t)y * (size_t)image.width + (size_t)(kCentreX + d)) * 4;
			for (int c = 0; c < 4; ++c)
			{
				const float delta = std::fabs(image.rgba[left + c] - image.rgba[right + c]);
				if (delta > worstMirror)
				{
					worstMirror = delta;
					worstX = kCentreX + d;
					worstY = y;
				}
			}
		}
	}

	constexpr float kMaxMirror = 1.0e-6f;
	failures += check(worstMirror <= kMaxMirror,
		"bloom halo is mirror symmetric",
		text("worst mismatch %.3e at (%d,%d), limit %.0e", (double)worstMirror,
			worstX, worstY, (double)kMaxMirror));

	return failures;
}

// ---------------------------------------------------------------------------
// 3. The quality ladder is ordered
// ---------------------------------------------------------------------------
//
// qualitySettings() exists so a consumer can say HOW GOOD rather than HOW,
// which only means anything if the rungs are genuinely in order.
//
// The rung that can silently invert is Ultra. Standard clamps per-sample
// radiance at 12 to kill fireflies, and that clamp BIASES the image downwards —
// it deletes energy, and dims genuine caustics along with the outliers. Ultra
// sets clampRadiance to 0 precisely to get that energy back, paying for the
// resulting variance in samples. So if Ultra ever renders DIMMER than Standard
// on the same scene, something in the high rung is losing light and the
// "better" setting is quietly the worse one.
//
// THE TWO OVERRIDES are what make this a test of TRANSPORT rather than of
// anything else:
//
//   * samplesPerPixel drops from Ultra's 4096 to 256. The comparison converges
//     long before 4096 and the rung's real count would put minutes into every
//     ctest run. What is under test is the clamp, not the sample budget.
//
//   * bloomStrength drops from 0.06 to 0. Bloom ADDS energy to the frame, so
//     leaving it on would let this check pass on the post-pass alone even if
//     transport had lost light — it would assert the opposite of what it is
//     for.
//
// Measured mean luminance on `glass` at 160x90: 0.8827 standard, 0.9169 ultra —
// Ultra about 3.9% brighter, which is the clamped caustic energy coming back.
// The assertion allows Ultra to come in 2% BELOW Standard, because at 256 paths
// both numbers are still Monte Carlo estimates and the two rungs differ in
// reconstruction filter and roulette depth as well as in the clamp. The failure
// it is looking for is a rung that lost a light, not a percent of noise.
int testQualityLadder()
{
	const Settings draft = qualitySettings(Quality::Draft, 160, 90);
	const Settings standard = qualitySettings(Quality::Standard, 160, 90);
	const Settings high = qualitySettings(Quality::High, 160, 90);
	Settings ultra = qualitySettings(Quality::Ultra, 160, 90);

	int failures = 0;

	// The rungs' own arithmetic first: it costs nothing, it pins the promises
	// the README makes about them in a table, and it localises a failure — if
	// these trip, the luminance comparison below is measuring a ladder that is
	// already wrong and its number means nothing.
	failures += check(draft.mode == RenderMode::Fast,
		"draft renders in fast mode",
		text("mode %s", draft.mode == RenderMode::Fast ? "Fast" : "Full"));

	failures += check(standard.mode == RenderMode::Full && high.mode == RenderMode::Full
		&& ultra.mode == RenderMode::Full,
		"standard/high/ultra path trace",
		text("%s/%s/%s",
			standard.mode == RenderMode::Full ? "Full" : "Fast",
			high.mode == RenderMode::Full ? "Full" : "Fast",
			ultra.mode == RenderMode::Full ? "Full" : "Fast"));

	// Draft is left out of this ordering ON PURPOSE. It is a Fast-mode rung, so
	// samplesPerPixel is ignored entirely and sits at the Settings default of
	// 256 — HIGHER than Standard's 128. Including it would assert on a field
	// that rung does not read, and would have to be "fixed" by giving Draft a
	// meaningless sample count.
	failures += check(standard.samplesPerPixel <= high.samplesPerPixel
		&& high.samplesPerPixel <= ultra.samplesPerPixel,
		"sample counts are non-decreasing",
		text("%d <= %d <= %d", standard.samplesPerPixel, high.samplesPerPixel,
			ultra.samplesPerPixel));

	failures += check(ultra.clampRadiance == 0.0f,
		"ultra has no radiance clamp",
		text("standard %.1f, ultra %.1f", (double)standard.clampRadiance,
			(double)ultra.clampRadiance));

	// See the header comment: transport only, so the answer cannot come from
	// the post-pass, and 256 paths so the answer arrives this decade.
	ultra.samplesPerPixel = 256;
	ultra.bloomStrength = 0.0f;

	Scene scene;
	Camera camera;
	if (!buildGlassScene(scene, camera))
		return failures + 1;

	const double standardLuma = meanLuminance(render(scene, camera, standard));
	const double ultraLuma = meanLuminance(render(scene, camera, ultra));

	constexpr double kFloorRatio = 0.98;
	failures += check(ultraLuma >= standardLuma * kFloorRatio,
		"ultra is not dimmer than standard",
		text("standard %.4f, ultra %.4f (%.1f%%), floor %.0f%%",
			standardLuma, ultraLuma,
			(standardLuma > 0.0) ? (ultraLuma / standardLuma * 100.0) : 0.0,
			kFloorRatio * 100.0));

	return failures;
}

// ---------------------------------------------------------------------------
// 4. Fast mode renders the geometry, it does not approximate it
// ---------------------------------------------------------------------------
//
// RenderMode::Fast drops all light transport for one primary ray and a
// closed-form shade — a few hundred times faster. The claim that makes it
// useful, and that both the header and the README make in as many words, is
// that it uses the SAME marcher and the SAME primary ray, so the silhouette and
// the alpha channel come out the same as a full render's. If that quietly
// stopped being true, Fast mode would still produce a pretty picture and would
// no longer tell you anything about the shape you were trying to fix — which is
// its entire job.
//
// ALPHA IS THE RIGHT MEASURABLE. It is coverage and nothing else. Colour
// differs enormously between the modes by design, so comparing colour would
// measure the design; coverage must not differ at all beyond the sampling
// difference (Fast lays a fixed 2x2 sub-pixel grid, Full draws 24 filtered
// jittered positions), and that difference only touches edge pixels.
//
// A BORED PLATE RATHER THAN A DEMO SCENE, because the bore is where the two
// modes could most plausibly disagree. A hole driven straight through a plate
// makes rays entering near its wall run nearly parallel to that wall: their
// step size stays tiny, they exhaust the march step budget, and a ray out of
// steps is reported as a MISS — alpha 0, the background leaking through as a
// ring around the hole. That is a march-BUDGET phenomenon rather than a shading
// one, which makes it exactly the kind of thing that would start to differ if
// the two modes ever stopped sharing a marcher.
//
// Four plates: thin and deep, straight bore and tapered — the two axes that
// decide how hard the bore is to march. Covering all four means the check holds
// across that range rather than at one convenient point.
//
// IT DOES NOT ASSERT THAT THE LEAK IS ABSENT, in either mode. Whether a given
// bore leaks is a property of the GEOMETRY, not of the renderer; the README
// documents it as a modelling gotcha with the taper as its cure. Asserting it
// away here would be claiming a hazard had been designed out that has not been.
// What is asserted is that the two modes AGREE about it, leak and all.
//
// As it happens none of these four leaks measurably at the sizes above — the
// straight bores land at mean alpha 0.7585 full / 0.7588 fast, within 0.0006 of
// the 0.7582 that the plate's area minus the bore's says they should be. That
// is worth knowing and worth NOT asserting: it makes the four cases a spread of
// marching difficulty rather than a leak reproduction, and if a future change
// to the step budget started leaking here, this check should fail only if the
// two modes then disagreed about it.
//
// Measured deltas in mean alpha: 0.0002 to 0.0003 across all four plates, which
// is the sub-pixel sampling difference at the edges and nothing else. The 0.005
// limit is more than an order of magnitude clear of that, and far below what a
// silhouette that genuinely differed between the modes would move.

// One plate: a rounded slab with a hole through it, in the studio.
//
// `taperedBore` picks between the two bores. Both are built around the same
// nominal 0.22 aperture — the cone is exactly 0.22 at the plate's mid-plane —
// and the cone then flares BEHIND the visible face and pinches in front of it,
// which is the shape the README prescribes for the leak: widening the hole
// where the camera cannot see it costs nothing and is what a punched hole looks
// like anyway.
//
// `reach` is three times the half-depth so the bore comfortably outruns the
// plate in both directions. A bore that stopped flush with the faces would put
// two coincident surfaces at the rim — the other documented hazard — and the
// speckle that produces is a different bug from the one this scene is for.
Scene makeBoredPlate(float halfDepth, bool taperedBore)
{
	Scene scene;
	addStudio(scene);

	const float reach = halfDepth * 3.0f;

	Object plate;
	plate.material = recipes::anodisedBlack();
	plate.boundsCentre = { 0.0f, 0.0f, 0.0f };

	// The slab's true half-diagonal is sqrt(0.85^2 + 0.85^2 + halfDepth^2),
	// about 1.21 thin and 1.33 deep, so this contains it with room to spare at
	// both depths. Too large is merely slow; too small silently clips, and a
	// clipped plate would make BOTH modes wrong together and the check pass.
	plate.boundsRadius = 1.2f + halfDepth;

	plate.distance = [halfDepth, reach, taperedBore](const Vec3& p)
	{
		const float bore = taperedBore
			? sdCappedCone(p, 0.22f + reach * 0.20f, 0.22f - reach * 0.20f, reach)
			: sdCylinder(p, 0.22f, reach);

		return opSubtract(sdRoundBox(p, Vec3{ 0.85f, 0.85f, halfDepth }, 0.03f), bore);
	};
	scene.add(std::move(plate));

	return scene;
}

int testFastCoverage()
{
	struct Plate
	{
		const char* name;
		float halfDepth;
		bool taperedBore;
	};

	const Plate plates[] = {
		{ "thin straight", 0.10f, false },
		{ "thin tapered",  0.10f, true  },
		{ "deep straight", 0.55f, false },
		{ "deep tapered",  0.55f, true  },
	};

	int failures = 0;
	for (const Plate& plate : plates)
	{
		const Scene scene = makeBoredPlate(plate.halfDepth, plate.taperedBore);

		// Front on and orthographic: the framing a panel bitmap actually uses,
		// and the one where a bore is a circle rather than an ellipse, so a
		// leak shows as a clean ring instead of being confused with foreshortening.
		Camera camera;
		camera.position = { 0.0f, 0.0f, 6.0f };
		camera.target = { 0.0f, 0.0f, 0.0f };
		camera.filmWidth = 1.9f;

		// 200x200 so the bore is around 46 pixels across — big enough that a
		// ring of leaked pixels is a measurable fraction of the frame rather
		// than a rounding. 24 paths because this measures COVERAGE, which is
		// converged almost immediately; the radiance it also computes is thrown
		// away.
		Settings settings;
		settings.width = 200;
		settings.height = 200;
		settings.samplesPerPixel = 24;

		settings.mode = RenderMode::Full;
		const double full = meanAlpha(render(scene, camera, settings));

		settings.mode = RenderMode::Fast;
		const double fast = meanAlpha(render(scene, camera, settings));

		const double delta = std::fabs(full - fast);

		constexpr double kMaxDelta = 0.005;
		failures += check(delta < kMaxDelta,
			text("fast matches full: %s", plate.name).c_str(),
			text("full %.4f, fast %.4f, delta %.4f, limit %.3f",
				full, fast, delta, kMaxDelta));
	}

	return failures;
}
}

int main()
{
	std::printf("tide_render_unit\n\n");

	int failures = 0;
	failures += testProgressiveAccumulation();
	failures += testBloom();
	failures += testQualityLadder();
	failures += testFastCoverage();

	if (failures == 0)
	{
		std::printf("\nall %d checks passed\n", gChecks);
		return 0;
	}

	// The numbers above are the diagnosis: each failing line already carries
	// what was measured and what the limit was, which is the first thing anyone
	// investigating would want and is why check() prints it unconditionally.
	std::printf("\n%d of %d checks failed\n", failures, gChecks);
	return 1;
}
