// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "DemoScenes.h"
#include "PngIo.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

// A developer tool, not part of any plugin. It renders one of the demo scenes
// to a PNG so a material or a light can be judged without a plugin host, a DAW
// and a rebuild of the whole rack in the loop.
//
//   tide_render_preview <scene> [--size N] [--quality Q | --spp N | --fast]
//                               [--png16] [--out F]
//   tide_render_preview --references <dir>
//
// --quality picks a rung of the ladder (draft, standard, high, ultra) and sets
// every knob at once — see Quality in TidePathTracer.h. --fast is shorthand
// for draft; --spp overrides just the sample count. --png16 writes 16 bits per
// channel for grading (ultra's natural companion).
//
// The scenes themselves live in DemoScenes.h, shared with the regression test.
//
// Exit codes: 0 success, 1 render/write failure, 2 usage error. Distinct on
// purpose — a script that regenerates references needs to tell "you typed the
// scene name wrong" apart from "the output directory is missing".
using namespace tide::render;

namespace
{

int usage()
{
	std::printf("usage: tide_render_preview <scene> [--size N] [--quality Q | --spp N | --fast]\n");
	std::printf("                           [--png16] [--out F]\n");
	std::printf("       tide_render_preview --references <dir>\n");
	std::printf("quality: draft standard high ultra\n");
	std::printf("scenes:");
	for (const tide::demo::SceneDef& def : tide::demo::kScenes)
		std::printf(" %s", def.name);
	std::printf("\n");
	return 2;
}

bool renderToPng(const tide::demo::SceneDef& def, const Settings& settings,
	const std::string& outPath, bool png16 = false)
{
	const auto start = std::chrono::steady_clock::now();

	// The SAME pipeline the regression test compares with — see DemoScenes.h.
	// The 16-bit path re-encodes from the same HDR Image; the throwaway 8-bit
	// encode it also did costs microseconds against the render it follows.
	std::vector<uint8_t> pixels;
	const Image image = tide::demo::renderSceneRgba8(def, settings, pixels);

	const auto elapsed = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - start).count();

	bool wrote = false;
	if (png16)
	{
		std::vector<uint16_t> wide((size_t)image.width * (size_t)image.height * 4);
		writePixels16(image, wide.data(), image.width * 4 * (int32_t)sizeof(uint16_t),
			PixelOrder::Rgba, false);
		wrote = tide::png::write16(outPath, wide.data(), image.width, image.height);
	}
	else
	{
		wrote = tide::png::write(outPath, pixels.data(), image.width, image.height);
	}

	if (!wrote)
	{
		std::printf("could not write %s\n", outPath.c_str());
		return false;
	}

	// Report what actually ran: "spp" is meaningless in fast mode, and printing
	// it anyway is how a preview gets mistaken for a converged render.
	if (settings.mode == RenderMode::Fast)
		std::printf("%-10s %4dx%-4d  fast %dx  %6.3fs  -> %s\n", def.name,
			image.width, image.height, settings.fastAntiAlias, elapsed, outPath.c_str());
	else
		std::printf("%-10s %4dx%-4d %5d spp  %6.2fs  -> %s%s\n", def.name,
			image.width, image.height, settings.samplesPerPixel, elapsed,
			outPath.c_str(), png16 ? "  (16-bit)" : "");
	return true;
}

// Regenerates the whole committed reference set at the size and sample count
// the regression test uses. Run this after DELIBERATELY changing a look, then
// LOOK AT the new images before committing them — an approved reference is only
// as good as the eye that approved it.
int writeReferences(const std::string& directory)
{
	for (const tide::demo::SceneDef& def : tide::demo::kScenes)
	{
		for (const RenderMode mode : tide::demo::kReferenceModes)
		{
			const Settings settings = tide::demo::referenceSettings(def, mode);
			if (!renderToPng(def, settings,
				directory + "/" + tide::demo::referenceName(def, mode)))
				return 1;
		}
	}

	std::printf("\nreferences written to %s — LOOK AT THEM before committing.\n",
		directory.c_str());
	return 0;
}
}

int main(int argc, char** argv)
{
	const std::string first = (argc > 1) ? argv[1] : "knob";
	if (first == "-h" || first == "--help")
		return usage();

	if (first == "--references")
		return (argc > 2) ? writeReferences(argv[2]) : usage();

	const tide::demo::SceneDef* def = tide::demo::findScene(first);
	if (!def)
		return usage();

	int width = 384;
	int sppOverride = 0;
	bool fast = false;
	bool png16 = false;
	bool haveQuality = false;
	Quality quality = Quality::Standard;

	std::string outPath = first + ".png";

	for (int i = 2; i < argc; ++i)
	{
		const bool hasValue = (i + 1) < argc;
		if (!std::strcmp(argv[i], "--size") && hasValue)
			width = std::atoi(argv[++i]);
		else if (!std::strcmp(argv[i], "--spp") && hasValue)
			sppOverride = std::atoi(argv[++i]);
		else if (!std::strcmp(argv[i], "--fast"))
			fast = true;
		else if (!std::strcmp(argv[i], "--png16"))
			png16 = true;
		else if (!std::strcmp(argv[i], "--quality") && hasValue)
		{
			const std::string name = argv[++i];
			haveQuality = true;
			if (name == "draft") quality = Quality::Draft;
			else if (name == "standard") quality = Quality::Standard;
			else if (name == "high") quality = Quality::High;
			else if (name == "ultra") quality = Quality::Ultra;
			else return usage();
		}
		else if (!std::strcmp(argv[i], "--out") && hasValue)
			outPath = argv[++i];
		else
			return usage();
	}

	if (width < 1)
		return usage();

	const int height = tide::demo::sceneHeight(*def, width);

	// The ladder sets everything from one word; the loose flags then override
	// their one field each, so `--quality ultra --spp 512` is a quick ultra.
	// Without --quality the tool keeps its historical defaults (plain Settings,
	// 128 paths) so old command lines keep producing what they always did.
	Settings settings;
	if (haveQuality)
	{
		settings = qualitySettings(quality, width, height);
	}
	else
	{
		settings.width = width;
		settings.height = height;
		settings.samplesPerPixel = 128;
	}

	if (fast)
		settings.mode = RenderMode::Fast;
	if (sppOverride > 0)
		settings.samplesPerPixel = sppOverride;

	return renderToPng(*def, settings, outPath, png16) ? 0 : 1;
}
