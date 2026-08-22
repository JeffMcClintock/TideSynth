// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#pragma once
#include "TidePathTracer.h"
#include <string>
#include <vector>

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
	// Analytic: same field sdPlane(p, {0,1,0}, height) described, intersected
	// exactly instead of marched — see AnalyticForm in TidePathTracer.h.
	floor.analytic = AnalyticForm::Plane;
	floor.planeNormal = { 0.0f, 1.0f, 0.0f };
	floor.planeOffset = height;
	return floor;
}

// Stand a shape up: exchange the SDF's Z (its natural axis, per the library's
// Z-up convention) with world Y, so a cylinder modelled along Z rests upright
// on a Y-up floor. Defined ONCE because the two plausible spellings differ in
// chirality — {x, z, y} is the self-inverse mirror-exchange used throughout,
// while {x, -z, y} is a true rotation — and a scene mixing them flips
// asymmetric shapes with nothing to say which was meant.
constexpr Vec3 upright(const Vec3& p) { return { p.x, p.z, p.y }; }

// A three-quarter orthographic view. Front-on is what a panel bitmap needs, but
// it is a poor way to JUDGE a material: a flat-on ortho camera sees one normal
// per flat face and therefore one reflected colour, so everything looks
// painted. Turning the camera puts a range of angles on every object and lets
// the grazing behaviour show. `offset` is where the camera sits relative to the
// target; a scene passes its own when the default framing crops it.
inline void setThreeQuarterView(Camera& camera, float filmWidth,
	Vec3 target = { 0.0f, 0.0f, 0.0f }, Vec3 offset = { 3.2f, 2.4f, 5.0f })
{
	camera.position = target + offset;
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
		// WORN, because a dark anodised finish is where surface imperfection
		// shows most and where its absence is most obvious — an even black
		// face is the most synthetic-looking thing this renderer can produce.
		// It is also the pin: nothing else in the suite exercises the
		// imperfection field, which is off by default everywhere.
		{ Shape::ChamferBox, recipes::worn(recipes::anodisedBlack(), 0.45f) },
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
		// Sized against the WORST reach, which is the round box: its corner is
		// sqrt(3)*0.36 + 0.10 = 0.7235 from centre. The first pass used 0.72 —
		// visually fine, silently wrong: a bound the object pokes through stops
		// being a lower bound, and grazing rays step clean over the corner tips.
		o.boundsRadius = 0.74f;

		// The knurled knob's grain must run around ITS OWN axis — which after
		// upright() is world Y through the cell centre. The recipe's defaults
		// (world Z through the origin) put the lathe somewhere left of the
		// table: the grain came out running along the knob's axis, brushing it
		// like a broom instead of turning it.
		//
		// Lightly worn as well: a knob is the one part on a panel that gets
		// touched, so a pristine one is the least plausible object in the
		// scene. Gentler than the anodised box's 0.45 — aluminium wears by
		// polishing rather than by scuffing.
		if (cell.shape == Shape::Knurled)
			o.material = recipes::worn(
				recipes::brushedAluminium(Vec3{ 0.0f, 1.0f, 0.0f }, centre), 0.25f);

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
			{
				// Turned 28 degrees so the 3/4 camera sees the chamfer facets
				// catch light on two faces at once — and, deliberately, through
				// the library's Transform, which until this had no caller: the
				// committed reference now pins makeTransform's rotation maths.
				static const Transform turn =
					makeTransform(Vec3{ 0.0f, 0.0f, 0.0f }, Vec3{ 0.0f, 1.0f, 0.0f }, 28.0f);
				return turn.toWorldDistance(
					sdChamferBox(turn.toLocal(p), Vec3{ 0.46f, 0.46f, 0.46f }, 0.14f));
			}

			case Shape::Torus:
				// upright() would stand it like a coin; the gold ring instead
				// lies FLAT on the table, which for a torus means the same
				// exchange — its natural axis is Z and the table's is Y.
				return sdTorus(upright(p), 0.36f, 0.16f);

			case Shape::Knurled:
			{
				// A knurled control knob: chamfered cylinder, grooves stamped
				// around the rim by folding the domain into one sector.
				const Vec3 q = upright(p);
				float d = sdChamferCylinder(q, 0.46f, 0.34f, 0.07f);
				const Vec3 r = opPolarRepeat(q, 44);
				const float groove = sdCylinder(Vec3{ r.x - 0.46f, r.y, 0.0f }, 0.014f, 1.0f);
				const float band = sdCylinder(q, 2.0f, 0.26f);
				return opSubtract(d, opIntersect(groove, band));
			}

			case Shape::Cone:
				return sdCappedCone(upright(p), 0.50f, 0.16f, 0.38f);

			case Shape::Capsule:
				return sdRoundCylinder(upright(p), 0.34f, 0.48f, 0.28f);

			case Shape::Dome:
			default:
				// A hemisphere: a sphere cut by a plane, which is what most
				// indicator lenses and button caps actually are.
				return opIntersect(sdSphere(p, 0.52f), sdPlane(p, Vec3{ 0.0f, -1.0f, 0.0f }, 0.14f));
			}
		};
		scene.add(std::move(o));
	}

	setThreeQuarterView(camera, 7.4f, Vec3{ 0.0f, 0.0f, -0.6f }, Vec3{ 2.6f, 3.4f, 5.0f });
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

	// A standing ring, facing the camera: curved in two directions, so it
	// throws a folded caustic rather than a single spot, and its own body shows
	// the tint deepening where the ray path through the glass is longest.
	//
	// No upright() here — sdTorus's natural axis is Z, which IS the camera
	// axis, so the bare SDF already stands the ring on edge like a coin. (The
	// first version swizzled it anyway, out of habit: that laid the ring flat
	// AND left it levitating — the centre height below was chosen for a
	// standing ring's reach of major+minor, not a flat one's of minor.)
	// centre.y - (0.40 + 0.21) = -0.62, exactly the floor plane: it rests.
	Object ring;
	ring.material = recipes::tintedGlass({ 0.16f, 0.52f, 0.92f }, 1.1f);
	ring.boundsCentre = { 0.82f, -0.01f, 0.0f };
	ring.boundsRadius = 0.68f;
	ring.distance = [](const Vec3& p)
	{
		return sdTorus(p - Vec3{ 0.82f, -0.01f, 0.0f }, 0.40f, 0.21f);
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

	// The room stays, all the studio lights go — zeroing an emission makes
	// addStudio not add that light at all. Anything visible here is lit by the
	// cubes.
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

// The scene registry: ONE row per scene, carrying everything the tools need.
//
// One table rather than parallel registries (a name list, a build-if-chain, an
// aspect switch) because parallel registries drift: the first version decided
// each scene's aspect by string-comparing its NAME in a separate function, so
// adding a second square scene meant remembering an edit two files away — and
// forgetting it would have rendered the scene cropped, committed the cropped
// image as the reference, and then PASSED forever after. A scene that is not a
// full row here does not exist, which is the point.
struct SceneDef
{
	const char* name;
	bool square; // knob bitmaps are square; showcase frames are 16:9
	Scene (*build)(Camera&);
};

inline constexpr SceneDef kScenes[] = {
	{ "knob",      true,  &makeKnobScene },
	{ "materials", false, &makeMaterialsScene },
	{ "shapes",    false, &makeShapesScene },
	{ "glass",     false, &makeGlassScene },
	{ "glow",      false, &makeGlowScene },
};
inline constexpr int kSceneCount = (int)(sizeof(kScenes) / sizeof(kScenes[0]));

// The size the committed reference images are rendered at.
//
// Small and cheap on purpose: the regression test runs on every build, and its
// job is to notice that the LOOK changed, which a 160-pixel frame does just as
// well as a 640-pixel one. It is also below the quality ladder's taper
// threshold, so `Quality::Standard` hands out its full base sample count here —
// which keeps the references readable rather than a sandstorm, and a noisy
// reference makes a human reviewing a diff unable to tell a change from grain.
//
// There is deliberately no kReferenceSamples any more: the sample count comes
// from the ladder now, and a second constant beside it could only ever drift
// out of agreement with the thing it was meant to describe.
inline constexpr int kReferenceWidth = 160;

// Null for an unknown name, so the CLI can print usage instead of a stack trace.
inline const SceneDef* findScene(const std::string& name)
{
	for (const SceneDef& def : kScenes)
		if (name == def.name)
			return &def;

	return nullptr;
}

inline int sceneHeight(const SceneDef& def, int width)
{
	return def.square ? width : (width * 9 / 16);
}

// The two modes each get their own committed reference, and they check
// different things. The FULL one pins the look — materials, lighting, caustics.
// The FAST one pins the GEOMETRY, and pins it harder: with no light transport
// there is no Monte Carlo noise for a change to hide in, so a moved chamfer or
// a bounds clip shows up cleanly instead of at the edge of the sampling floor.
// It costs about 40 ms for the whole set.
inline constexpr RenderMode kReferenceModes[] = { RenderMode::Full, RenderMode::Fast };

inline std::string referenceName(const SceneDef& def, RenderMode mode)
{
	return std::string(def.name) + (mode == RenderMode::Fast ? "-fast" : "") + ".png";
}

// The rung each reference mode pins. Draft IS RenderMode::Fast, and Standard is
// the faceplate quality the product actually ships, so the two references per
// scene are exactly the two rungs a user ever sees.
inline constexpr Quality referenceQuality(RenderMode mode)
{
	return (mode == RenderMode::Fast) ? Quality::Draft : Quality::Standard;
}

// Built in ONE place so the generator and the test cannot disagree about what a
// reference is — a drifting sample count or mode would fail every scene at once
// with deltas that read as a renderer regression.
//
// Routed THROUGH the quality ladder rather than assembling a Settings by hand.
// Hand-assembling pinned only the fields it happened to mention: the ladder's
// own numbers — sample count, bounce budget, clamp, roulette depth, filter —
// went unchecked, so `Quality::Standard` could have been retuned to anything
// overnight and every reference would still have matched. Now the committed
// images ARE the shipped rungs, and changing a rung is a change you have to
// look at.
inline Settings referenceSettings(const SceneDef& def, RenderMode mode)
{
	return qualitySettings(referenceQuality(mode), kReferenceWidth,
		sceneHeight(def, kReferenceWidth));
}

// Render a scene and encode it to straight (NON-premultiplied) 8-bit RGBA —
// the one pixel pipeline behind both the committed references and the
// regression comparison. It lives here, used by both tools, because the two
// must be bit-identical: a premultiply flag or tone-map argument that drifted
// between them would fail every scene at once with deltas that read as a
// renderer regression instead of an encoding mismatch.
inline Image renderSceneRgba8(const SceneDef& def, const Settings& settings,
	std::vector<uint8_t>& rgba8)
{
	Camera camera;
	Scene scene = def.build(camera);

	// Fully qualified: inside tide::demo, plain `render` is ambiguous between
	// the tide::render NAMESPACE and the function it contains.
	Image image = tide::render::render(scene, camera, settings);

	rgba8.assign((size_t)image.width * (size_t)image.height * 4, 0);
	writePixels(image, rgba8.data(), image.width * 4, PixelOrder::Rgba, false);
	return image;
}
}
