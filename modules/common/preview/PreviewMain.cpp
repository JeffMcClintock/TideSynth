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
//   tide_render_preview <scene> [--size N] [--spp N] [--out F]
//   tide_render_preview --references <dir>
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
	std::printf("usage: tide_render_preview <scene> [--size N] [--spp N] [--out F]\n");
	std::printf("       tide_render_preview --references <dir>\n");
	std::printf("scenes:");
	for (const tide::demo::SceneDef& def : tide::demo::kScenes)
		std::printf(" %s", def.name);
	std::printf("\n");
	return 2;
}

bool renderToPng(const tide::demo::SceneDef& def, const Settings& settings,
	const std::string& outPath)
{
	const auto start = std::chrono::steady_clock::now();

	// The SAME pipeline the regression test compares with — see DemoScenes.h.
	std::vector<uint8_t> pixels;
	const Image image = tide::demo::renderSceneRgba8(def, settings, pixels);

	const auto elapsed = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - start).count();

	if (!tide::png::write(outPath, pixels.data(), image.width, image.height))
	{
		std::printf("could not write %s\n", outPath.c_str());
		return false;
	}

	std::printf("%-10s %4dx%-4d %5d spp  %6.2fs  -> %s\n", def.name,
		image.width, image.height, settings.samplesPerPixel, elapsed, outPath.c_str());
	return true;
}

// Regenerates the whole committed reference set at the size and sample count
// the regression test uses. Run this after DELIBERATELY changing a look, then
// LOOK AT the new images before committing them — an approved reference is only
// as good as the eye that approved it.
int writeReferences(const std::string& directory)
{
	Settings settings;
	settings.width = tide::demo::kReferenceWidth;
	settings.samplesPerPixel = tide::demo::kReferenceSamples;

	for (const tide::demo::SceneDef& def : tide::demo::kScenes)
	{
		settings.height = tide::demo::sceneHeight(def, settings.width);
		if (!renderToPng(def, settings, directory + "/" + def.name + ".png"))
			return 1;
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

	Settings settings;
	settings.width = 384;
	settings.samplesPerPixel = 128;

	std::string outPath = first + ".png";

	for (int i = 2; i < argc; ++i)
	{
		const bool hasValue = (i + 1) < argc;
		if (!std::strcmp(argv[i], "--size") && hasValue)
			settings.width = std::atoi(argv[++i]);
		else if (!std::strcmp(argv[i], "--spp") && hasValue)
			settings.samplesPerPixel = std::atoi(argv[++i]);
		else if (!std::strcmp(argv[i], "--out") && hasValue)
			outPath = argv[++i];
		else
			return usage();
	}

	if (settings.width < 1 || settings.samplesPerPixel < 1)
		return usage();

	settings.height = tide::demo::sceneHeight(*def, settings.width);

	return renderToPng(*def, settings, outPath) ? 0 : 1;
}
