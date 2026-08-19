// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#pragma once
#include "TidePathTracer.h"
#include <string>

// The demo scenes, shared by two consumers that must stay in step:
//
//   preview/PreviewMain.cpp   renders them large and pretty, to look at
//   tests/RenderRegression.cpp renders them small and compares to references
//
// They live in one header precisely so the regression test cannot drift away
// from the pictures. If a scene is edited the test fails, the reference is
// re-approved by eye, and the two move together — which is the whole point of
// pinning images rather than numbers.
//
// Header-only and inline: these are scene DESCRIPTIONS, a few hundred lines of
// literals, and giving them a translation unit and a build rule would be more
// ceremony than the content deserves.
namespace tide::demo
{
using namespace tide::render;

// A camera-VISIBLE floor. The studio's own room is invisible so that panel
// artwork comes out with a transparent background; the demo scenes add this
// back explicitly, because a shadow needs something to fall on.
inline Object makeFloor(float height, Vec3 colour = { 0.66f, 0.66f, 0.68f })
{
	Object floor;
	floor.material = recipes::paint(colour);
	floor.material.cameraVisible = true;
	floor.unbounded = true;
	floor.distance = [height](const Vec3& p) { return sdPlane(p, Vec3{ 0.0f, 1.0f, 0.0f }, height); };
	return floor;
}

// A three-quarter orthographic view. Front-on is what a panel bitmap needs, but
// it is a poor way to JUDGE a material: a flat-on ortho camera sees one normal
// per flat face and therefore one reflected colour, so everything looks
// painted. Turning the camera puts a range of angles on every object and lets
// the grazing behaviour show.
inline void setThreeQuarterView(Camera& camera, float filmWidth, Vec3 target = { 0.0f, 0.0f, 0.0f })
{
	camera.position = target + Vec3{ 3.2f, 2.4f, 5.0f };
	camera.target = target;
	camera.up = { 0.0f, 1.0f, 0.0f };
	camera.filmWidth = filmWidth;
}

// --- scenes ----------------------------------------------------------------

// The TIDE knob, seen the way a panel actually shows it: straight on.
//
// Everything about the shape here exists to defeat the flat-orthographic
// problem. Seen head-on a flat metal disc reflects exactly one colour, so the
// top is gently DOMED, the rim is CHAMFERED, and the grain is CONCENTRIC, which
// smears every light in the room into a ring.
inline Scene makeKnobScene(Camera& camera)
{
	Scene scene;
	addStudio(scene);

	constexpr float kRadius = 0.50f;
	constexpr float kHalfHeight = 0.20f;
	constexpr float kChamfer = 0.05f;

	Object body;
	body.material = recipes::brushedAluminium();
	body.boundsCentre = { 0.0f, 0.0f, 0.0f };
	body.boundsRadius = kRadius + 0.06f;
	body.distance = [](const Vec3& p)
	{
		const float shell = sdChamferCylinder(p, kRadius, kHalfHeight, kChamfer);

		// A sphere of radius 3 centred just below the top face crowns it by
		// about 4%. Far too slight to read as a shape, and entirely visible as
		// a highlight that sweeps rather than sits.
		const float dome = sdSphere(p - Vec3{ 0.0f, 0.0f, kHalfHeight - 3.0f }, 3.0f);
		float d = opIntersect(shell, dome);

		// The pointer: a groove milled across the top face. It reads because
		// its two walls catch the light differently.
		const float pointer = sdBox(p - Vec3{ 0.0f, kRadius * 0.55f, kHalfHeight },
			Vec3{ 0.020f, kRadius * 0.42f, 0.035f });
		return opSubtract(d, pointer);
	};
	scene.add(std::move(body));

	// A dark plastic cap over the centre, where concentric grain degenerates:
	// on the axis there is no defined tangent direction, so the sheen would
	// collapse into a smudge. Covering it is cheaper than special-casing it.
	Object cap;
	cap.material = recipes::glossyPlastic({ 0.055f, 0.058f, 0.065f });
	cap.boundsCentre = { 0.0f, 0.0f, kHalfHeight };
	cap.boundsRadius = 0.15f;
	cap.distance = [](const Vec3& p)
	{
		return sdRoundCylinder(p - Vec3{ 0.0f, 0.0f, kHalfHeight - 0.005f },
			0.115f, 0.028f, 0.012f);
	};
	scene.add(std::move(cap));

	camera.position = { 0.0f, 0.0f, 8.0f };
	camera.target = { 0.0f, 0.0f, 0.0f };
	camera.up = { 0.0f, 1.0f, 0.0f };
	camera.filmWidth = 1.18f;
	return scene;
}

// Four spheres in a row: the same shape in four finishes, so the MATERIAL is
// the only variable.
inline Scene makeMaterialsScene(Camera& camera)
{
	Scene scene;
	addStudio(scene);
	scene.add(makeFloor(0.55f));

	const Material entries[] = {
		recipes::polishedAluminium(),
		recipes::brushed(recipes::polishedAluminium(0.30f), BrushMode::Fixed, { 1.0f, 0.0f, 0.0f }),
		recipes::glossyPlastic({ 0.62f, 0.10f, 0.09f }),
		recipes::tintedGlass({ 0.32f, 0.78f, 0.45f }, 1.4f),
	};

	float x = -1.65f;
	for (const Material& material : entries)
	{
		Object o;
		o.material = material;
		o.boundsCentre = { x, 0.0f, 0.0f };
		o.boundsRadius = 0.56f;
		const float cx = x;
		o.distance = [cx](const Vec3& p) { return sdSphere(p - Vec3{ cx, 0.0f, 0.0f }, 0.55f); };
		scene.add(std::move(o));
		x += 1.10f;
	}

	camera.position = { 0.0f, 0.35f, 9.0f };
	camera.target = { 0.0f, 0.0f, 0.0f };
	camera.filmWidth = 4.6f;
	return scene;
}

// The full spread: eight shapes, eight materials, one per cell. This is the
// picture to look at when deciding what a new part should be made of.
inline Scene makeShapesScene(Camera& camera)
{
	Scene scene;
	addStudio(scene);
	scene.add(makeFloor(0.62f, { 0.60f, 0.60f, 0.63f }));

	enum class Shape { Sphere, RoundBox, ChamferBox, Torus, Knurled, Cone, Capsule, Dome };

	struct Cell { Shape shape; Material material; };

	const Cell cells[] = {
		{ Shape::Knurled,    recipes::brushedAluminium() },
		{ Shape::ChamferBox, recipes::anodisedBlack() },
		{ Shape::Torus,      recipes::gold() },
		{ Shape::Sphere,     recipes::copper() },
		{ Shape::RoundBox,   recipes::glossyPlastic({ 0.10f, 0.28f, 0.62f }) },
		{ Shape::Dome,       recipes::chrome() },
		{ Shape::Cone,       recipes::brass() },
		{ Shape::Capsule,    recipes::tintedGlass({ 0.85f, 0.32f, 0.18f }, 1.1f) },
	};

	// Two rows of four, spread across the floor.
	int index = 0;
	for (const Cell& cell : cells)
	{
		const int column = index % 4;
		const int row = index / 4;
		++index;

		const Vec3 centre{ (float)(column - 1.5f) * 1.32f, 0.0f, (float)(row) * -1.35f };

		Object o;
		o.material = cell.material;
		o.boundsCentre = centre;
		o.boundsRadius = 0.72f;

		const Shape shape = cell.shape;
		o.distance = [centre, shape](const Vec3& world)
		{
			const Vec3 p = world - centre;
			switch (shape)
			{
			case Shape::Sphere:
				return sdSphere(p, 0.50f);

			case Shape::RoundBox:
				return sdRoundBox(p, Vec3{ 0.46f, 0.46f, 0.46f }, 0.10f);

			case Shape::ChamferBox:
				return sdChamferBox(p, Vec3{ 0.46f, 0.46f, 0.46f }, 0.14f);

			case Shape::Torus:
				// Laid flat: the SDF's tube lies in XY, so a quarter turn about
				// X stands it up the way a ring sits on a table.
				return sdTorus(Vec3{ p.x, p.z, p.y }, 0.36f, 0.16f);

			case Shape::Knurled:
			{
				// A knurled control knob: chamfered cylinder, grooves stamped
				// around the rim by folding the domain into one sector.
				const Vec3 q{ p.x, p.z, p.y }; // stand the cylinder upright
				float d = sdChamferCylinder(q, 0.46f, 0.34f, 0.07f);
				const Vec3 r = opPolarRepeat(q, 44);
				const float groove = sdCylinder(Vec3{ r.x - 0.46f, r.y, 0.0f }, 0.014f, 1.0f);
				const float band = sdCylinder(q, 2.0f, 0.26f);
				return opSubtract(d, opIntersect(groove, band));
			}

			case Shape::Cone:
				return sdCappedCone(Vec3{ p.x, p.z, p.y }, 0.50f, 0.16f, 0.38f);

			case Shape::Capsule:
				return sdRoundCylinder(Vec3{ p.x, p.z, p.y }, 0.34f, 0.48f, 0.28f);

			case Shape::Dome:
			default:
				// A hemisphere: a sphere cut by a plane, which is what most
				// indicator lenses and button caps actually are.
				return opIntersect(sdSphere(p, 0.52f), sdPlane(p, Vec3{ 0.0f, -1.0f, 0.0f }, 0.14f));
			}
		};
		scene.add(std::move(o));
	}

	setThreeQuarterView(camera, 7.4f, Vec3{ 0.0f, 0.0f, -0.6f });
	camera.position = Vec3{ 0.0f, 0.0f, -0.6f } + Vec3{ 2.6f, 3.4f, 5.0f };
	return scene;
}

// Coloured glass over a pale floor, lit by one big window.
//
// THE SHAPES ARE CURVED ON PURPOSE. A flat slab refracts every ray by the same
// amount, so seen head-on it is a uniform tinted rectangle — technically
// correct and visually indistinguishable from frosted plastic. Curvature is
// what makes glass look like glass: it bends the background by varying amounts,
// and it FOCUSES transmitted light into a bright caustic core instead of
// spreading it evenly.
//
// ONE light, not the full studio. A caustic is an image of the source, so four
// sources give four overlapping caustics and the result reads as a smudge.
inline Scene makeGlassScene(Camera& camera)
{
	Scene scene;

	Studio studio;
	studio.enableRim = false;
	studio.enableGlint = false;
	studio.keyPosition = { -1.6f, 4.6f, 1.8f };
	studio.keyHalfWidth = 1.1f;
	studio.keyHalfHeight = 1.1f;
	studio.keyEmission = { 34.0f, 34.5f, 36.0f };
	studio.fillPosition = { 4.0f, 0.5f, 3.0f };
	studio.fillEmission = { 0.55f, 0.52f, 0.48f };
	addStudio(scene, studio);

	scene.add(makeFloor(0.62f, { 0.80f, 0.80f, 0.82f }));

	// A sphere is the strongest caustic generator there is — it is a lens, so
	// it collects a wide beam and concentrates it to a point.
	Object ball;
	ball.material = recipes::tintedGlass({ 0.92f, 0.42f, 0.10f }, 1.6f);
	ball.boundsCentre = { -0.78f, 0.0f, 0.20f };
	ball.boundsRadius = 0.66f;
	ball.distance = [](const Vec3& p)
	{
		return sdSphere(p - Vec3{ -0.78f, 0.0f, 0.20f }, 0.62f);
	};
	scene.add(std::move(ball));

	// A standing ring: curved in two directions, so it throws a folded caustic
	// rather than a single spot, and its own body shows the tint deepening
	// where the ray path through the glass is longest.
	Object ring;
	ring.material = recipes::tintedGlass({ 0.16f, 0.52f, 0.92f }, 1.1f);
	ring.boundsCentre = { 0.82f, -0.02f, 0.0f };
	ring.boundsRadius = 0.68f;
	ring.distance = [](const Vec3& p)
	{
		const Vec3 q = p - Vec3{ 0.82f, -0.02f, 0.0f };
		return sdTorus(Vec3{ q.x, q.z, q.y }, 0.40f, 0.21f);
	};
	scene.add(std::move(ring));

	camera.position = { 0.3f, 2.0f, 6.0f };
	camera.target = { 0.0f, -0.18f, 0.1f };
	camera.filmWidth = 3.6f;
	return scene;
}

// Glowing cubes in a dark room. The point of this scene is that the cubes are
// not just bright pixels — they are the ONLY light source, so everything else
// in the frame is lit by them, and the metal picks up their colours.
inline Scene makeGlowScene(Camera& camera)
{
	Scene scene;

	// The room stays, all the studio lights go. Anything visible here is lit by
	// the cubes.
	Studio studio;
	studio.enableRim = false;
	studio.enableGlint = false;
	studio.wallColour = { 0.18f, 0.18f, 0.21f };
	studio.keyEmission = { 0.0f, 0.0f, 0.0f };
	studio.fillEmission = { 0.0f, 0.0f, 0.0f };
	addStudio(scene, studio);

	scene.add(makeFloor(0.62f, { 0.30f, 0.30f, 0.33f }));

	// Strength is radiance and the tone curve rolls off towards `whitePoint`,
	// so a cube much above ~8 saturates all three channels and reads as a WHITE
	// box that happens to throw coloured light. Around 5 the cube keeps its own
	// hue and still clearly lights the room.
	struct Cube { Vec3 centre; Vec3 colour; float strength; };
	const Cube cubes[] = {
		{ { -1.35f, -0.30f, 0.10f }, { 1.00f, 0.30f, 0.13f }, 5.0f },
		{ {  0.00f, -0.30f, -0.5f }, { 0.18f, 1.00f, 0.50f }, 5.0f },
		{ {  1.35f, -0.30f, 0.10f }, { 0.26f, 0.48f, 1.00f }, 5.0f },
	};

	for (const Cube& cube : cubes)
	{
		Object o;
		o.material = recipes::glow(cube.colour, cube.strength);
		o.boundsCentre = cube.centre;
		// The bounding sphere is what shadow rays are aimed at, so it must be
		// snug: an over-large bound still gives the right answer but wastes
		// most of its samples on empty space around the cube.
		o.boundsRadius = 0.52f;
		const Vec3 centre = cube.centre;
		o.distance = [centre](const Vec3& p)
		{
			return sdRoundBox(p - centre, Vec3{ 0.28f, 0.28f, 0.28f }, 0.035f);
		};
		scene.add(std::move(o));
	}

	// Something reflective to catch all three colours at once.
	Object ball;
	ball.material = recipes::polishedAluminium(0.12f);
	ball.boundsCentre = { 0.0f, 0.32f, 0.75f };
	ball.boundsRadius = 0.46f;
	ball.distance = [](const Vec3& p)
	{
		return sdSphere(p - Vec3{ 0.0f, 0.32f, 0.75f }, 0.42f);
	};
	scene.add(std::move(ball));

	Object bar;
	bar.material = recipes::brushed(recipes::polishedAluminium(0.26f),
		BrushMode::Fixed, { 1.0f, 0.0f, 0.0f });
	bar.boundsCentre = { 0.0f, -0.05f, -1.5f };
	bar.boundsRadius = 2.4f;
	bar.distance = [](const Vec3& p)
	{
		return sdRoundBox(p - Vec3{ 0.0f, -0.05f, -1.5f }, Vec3{ 2.2f, 0.55f, 0.10f }, 0.05f);
	};
	scene.add(std::move(bar));

	camera.position = { 0.9f, 1.3f, 6.0f };
	camera.target = { 0.0f, -0.15f, 0.0f };
	camera.filmWidth = 4.6f;
	return scene;
}

// Every scene the preview tool and the regression test know about, in a fixed
// order so a reference set can be regenerated wholesale.
inline const char* const kSceneNames[] = { "knob", "materials", "shapes", "glass", "glow" };
inline constexpr int kSceneCount = 5;

// The size and sample count the committed reference images are rendered at.
//
// Small and cheap on purpose: the regression test runs on every build, and its
// job is to notice that the LOOK changed, which a 160-pixel frame does just as
// well as a 640-pixel one. The sample count is high enough that the image is
// readable rather than a sandstorm — a noisy reference makes a human reviewing
// a diff unable to tell a real change from grain.
inline constexpr int kReferenceWidth = 160;
inline constexpr int kReferenceSamples = 128;

// Returns false for an unknown name rather than throwing, so the CLI can print
// a usage message instead of a stack trace.
inline bool buildScene(const std::string& name, Scene& scene, Camera& camera)
{
	if (name == "knob")
		scene = makeKnobScene(camera);
	else if (name == "materials")
		scene = makeMaterialsScene(camera);
	else if (name == "shapes")
		scene = makeShapesScene(camera);
	else if (name == "glass")
		scene = makeGlassScene(camera);
	else if (name == "glow")
		scene = makeGlowScene(camera);
	else
		return false;

	return true;
}

// The knob is square because a knob bitmap is; the showcase scenes are 16:9.
inline int sceneHeightFor(const std::string& name, int width)
{
	return (name == "knob") ? width : (width * 9 / 16);
}
}
