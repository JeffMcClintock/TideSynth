// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "DemoScenes.h"
#include "PngIo.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Renders every demo scene small and compares it to a committed reference PNG.
//
// WHY IMAGES RATHER THAN NUMBERS. Almost everything this renderer can get wrong
// is invisible to a unit test and obvious to an eye. The anisotropy convention
// was inverted for a while and every pdf still integrated to one; the bounding
// sphere was being hit as if it were geometry and every BSDF was still correct.
// Both were caught by looking. Pinning the pictures is what turns "I looked at
// it once" into something a build can keep checking.
//
// WHY THIS IS NOT FLAKY. The renderer seeds its RNG from the PIXEL COORDINATE
// and the sample index, never from a shared counter, so the output does not
// depend on how tiles were scheduled across threads. Same source, same machine,
// same image — every time.
//
// The tolerance below exists for a different reason: a DIFFERENT compiler or
// optimisation level reorders floating point and shifts the last bit or two.
// The thresholds are set to swallow that and nothing else. A real regression —
// a broken BSDF, an inverted normal, a lost light — moves thousands of pixels
// by tens of levels and trips both limits at once.
//
//   tide_render_regression <referenceDir> [failureDir]
//
// On failure the rendered image is written to `failureDir` (default: the
// working directory) as <scene>-actual.png so the two can be compared by eye.

namespace
{

// A pixel must differ by more than this, on any channel, before it counts as
// changed at all. Two levels is roughly the noise floor of float reassociation.
constexpr int kChannelTolerance = 2;

// ...and no more than this fraction of pixels may be changed. A handful of
// pixels along a silhouette can legitimately flip when the maths is reordered;
// a percent of the frame cannot.
constexpr double kMaxChangedFraction = 0.004;

// Regardless of how few pixels moved, none of them may move this far. This is
// what catches a small but catastrophic change — a highlight that vanished, a
// caustic that moved.
constexpr int kMaxChannelDelta = 40;

struct Comparison
{
	bool matched = false;
	double changedFraction = 0.0;
	int worstDelta = 0;
	int worstX = 0;
	int worstY = 0;
};

Comparison compare(const std::vector<uint8_t>& actual, const std::vector<uint8_t>& reference,
	int width, int height)
{
	Comparison c;

	size_t changed = 0;
	const size_t pixelCount = (size_t)width * (size_t)height;

	for (size_t i = 0; i < pixelCount; ++i)
	{
		int worst = 0;
		for (int channel = 0; channel < 4; ++channel)
		{
			const int delta = std::abs(
				(int)actual[i * 4 + channel] - (int)reference[i * 4 + channel]);
			if (delta > worst)
				worst = delta;
		}

		if (worst > kChannelTolerance)
			++changed;

		if (worst > c.worstDelta)
		{
			c.worstDelta = worst;
			c.worstX = (int)(i % (size_t)width);
			c.worstY = (int)(i / (size_t)width);
		}
	}

	c.changedFraction = (pixelCount > 0) ? (double)changed / (double)pixelCount : 0.0;
	c.matched = (c.changedFraction <= kMaxChangedFraction) && (c.worstDelta <= kMaxChannelDelta);
	return c;
}

int runScene(const tide::demo::SceneDef& def, const std::string& referenceDir,
	const std::string& failureDir)
{
	using namespace tide::render;

	const std::string name = def.name;

	Settings settings;
	settings.width = tide::demo::kReferenceWidth;
	settings.height = tide::demo::sceneHeight(def, settings.width);
	settings.samplesPerPixel = tide::demo::kReferenceSamples;

	// The SAME pipeline the reference generator writes with — see DemoScenes.h.
	std::vector<uint8_t> actual;
	const Image image = tide::demo::renderSceneRgba8(def, settings, actual);

	const std::string referencePath = referenceDir + "/" + name + ".png";

	std::vector<uint8_t> reference;
	int refWidth = 0, refHeight = 0;
	std::string error;
	if (!tide::png::read(referencePath, reference, refWidth, refHeight, error))
	{
		std::printf("FAIL %-10s %s\n", name.c_str(), error.c_str());
		std::printf("     regenerate with: tide_render_preview --references %s\n",
			referenceDir.c_str());
		return 1;
	}

	if (refWidth != image.width || refHeight != image.height)
	{
		std::printf("FAIL %-10s reference is %dx%d, rendered %dx%d\n",
			name.c_str(), refWidth, refHeight, image.width, image.height);
		return 1;
	}

	const Comparison c = compare(actual, reference, image.width, image.height);
	if (c.matched)
	{
		std::printf("ok   %-10s %.3f%% pixels moved, worst delta %d\n",
			name.c_str(), c.changedFraction * 100.0, c.worstDelta);
		return 0;
	}

	const std::string actualPath = failureDir + "/" + name + "-actual.png";
	tide::png::write(actualPath, actual.data(), image.width, image.height);

	std::printf("FAIL %-10s %.3f%% of pixels moved (limit %.3f%%), worst delta %d at (%d,%d)"
		" (limit %d)\n",
		name.c_str(), c.changedFraction * 100.0, kMaxChangedFraction * 100.0,
		c.worstDelta, c.worstX, c.worstY, kMaxChannelDelta);
	std::printf("     wrote %s — compare it against %s\n",
		actualPath.c_str(), referencePath.c_str());
	return 1;
}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::printf("usage: tide_render_regression <referenceDir> [failureDir]\n");
		return 2;
	}

	const std::string referenceDir = argv[1];
	const std::string failureDir = (argc > 2) ? argv[2] : ".";

	int failures = 0;
	for (const tide::demo::SceneDef& def : tide::demo::kScenes)
		failures += runScene(def, referenceDir, failureDir);

	if (failures == 0)
	{
		std::printf("\nall %d scenes match\n", tide::demo::kSceneCount);
		return 0;
	}

	std::printf("\n%d of %d scenes changed.\n", failures, tide::demo::kSceneCount);
	std::printf("If the change was INTENDED, look at the -actual images, then run\n");
	std::printf("  tide_render_preview --references %s\n", referenceDir.c_str());
	std::printf("and commit the new references with the change that caused them.\n");
	return 1;
}
