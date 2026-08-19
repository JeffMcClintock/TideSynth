// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"
#include <algorithm>
#include <cmath>

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

namespace
{
constexpr float kPi = 3.14159265358979323846f;
}

// Knob look ported from VectorKnob_VCV (SynthEditLib/modules/SubControlsXp/VectorRingGui.cpp):
// a filled disc with a single radial pointer line, sharing VectorKnob's circular hit-test.
class TiDEknobGui final : public PluginEditor
{
 	Pin<float> pinpatchValue;
 	Pin<std::wstring> pinHint;

 	void onSetpatchValue()
	{
		if (drawingHost)
			drawingHost->invalidateRect(nullptr);
	}

	void calcDimensions(Point& center, float& radius, float& thickness)
	{
		center = getCenter(bounds);
		radius = (std::min)(getWidth(bounds), getHeight(bounds)) * 0.4f;
		thickness = radius * 0.2f;
	}

	bool hasCapture() const
	{
		bool captured = false;
		if (inputHost.get())
			inputHost->getCapture(captured);
		return captured;
	}

	Point pointPrevious{};

public:
	TiDEknobGui()
	{
		pinpatchValue.onUpdate = [this](PinBase*) { onSetpatchValue(); };
	}

	ReturnCode hitTest(Point point, int32_t flags) override
	{
		Point center;
		float radius;
		float thickness;
		calcDimensions(center, radius, thickness);

		const float dx = point.x - center.x;
		const float dy = point.y - center.y;
		const float outerRadius = radius + thickness * 0.5f;

		return dx * dx + dy * dy <= outerRadius * outerRadius ? ReturnCode::Ok : ReturnCode::Fail;
	}

	ReturnCode onPointerDown(Point point, int32_t flags) override
	{
		// Let host handle right-clicks (e.g. show its own context menu).
		if ((flags & static_cast<int32_t>(gmpi::api::PointerFlags::FirstButton)) == 0)
			return ReturnCode::Ok;

		pointPrevious = point;

		if (inputHost.get())
			inputHost->setCapture();

		return ReturnCode::Ok;
	}

	ReturnCode onPointerMove(Point point, int32_t flags) override
	{
		if (!hasCapture())
			return ReturnCode::Unhandled;

		const float coarseness = (flags & static_cast<int32_t>(gmpi::api::PointerFlags::KeyControl)) != 0 ? 0.001f : 0.005f;

		// Respond to both axes: drag up or right increases, down or left decreases.
		float newValue = pinpatchValue.value + coarseness * ((point.x - pointPrevious.x) - (point.y - pointPrevious.y));
		newValue = std::clamp(newValue, 0.0f, 1.0f);

		pointPrevious = point;

		pinpatchValue = newValue;

		return ReturnCode::Ok;
	}

	ReturnCode onPointerUp(Point point, int32_t flags) override
	{
		if (!hasCapture())
			return ReturnCode::Unhandled;

		if (inputHost.get())
			inputHost->releaseCapture();

		return ReturnCode::Ok;
	}

	ReturnCode onMouseWheel(Point point, int32_t flags, int32_t delta) override
	{
		// ignore horizontal scrolling
		if ((flags & static_cast<int32_t>(gmpi::api::PointerFlags::ScrollHoriz)) != 0)
			return ReturnCode::Unhandled;

		if (hitTest(point, flags) != ReturnCode::Ok)
			return ReturnCode::Unhandled;

		const float scale = (flags & static_cast<int32_t>(gmpi::api::PointerFlags::KeyControl)) != 0 ? 1.0f / 12000.0f : 1.0f / 1200.0f;

		float newValue = std::clamp(pinpatchValue.value + delta * scale, 0.0f, 1.0f);

		pinpatchValue = newValue;

		return ReturnCode::Ok;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		Point center;
		float radius;
		float thickness;
		calcDimensions(center, radius, thickness);

		auto brushForeground = g.createSolidColorBrush(Colors::White);
		auto brushBackground = g.createSolidColorBrush(Colors::Gray);

		const float startAngleRadians = 35.0f * kPi / 180.0f; // gap between "straight down" and each end of the arc.
		const float quarterTurnClockwise = kPi * 0.5f;

		const float normalized = std::clamp(pinpatchValue.value, 0.0f, 1.0f);
		const float sweepAngle = normalized * (kPi * 2.0f - startAngleRadians * 2.0f);
		const float angle = quarterTurnClockwise + startAngleRadians + sweepAngle;
		const Point movingPoint{ center.x + radius * cosf(angle), center.y + radius * sinf(angle) };

		auto strokeStyle = g.getFactory().createStrokeStyle(CapStyle::Round);

		// Background circle.
		g.fillCircle(center, radius + thickness * 0.5f, brushBackground);

		// Pointer line.
		g.drawLine(center, movingPoint, brushForeground, thickness, strokeStyle);

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = gmpi::Register<TiDEknobGui>::withId("SE TiDE:knob");
}
