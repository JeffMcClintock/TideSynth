// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"
#include <algorithm>
#include <cmath>
#include <string>

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

namespace
{
constexpr float kPi = 3.14159265358979323846f;
// Where the pointer line starts, as a fraction of the radius out from the center.
constexpr float kPointerInnerFraction = 0.25f;
}

// Knob look ported from VectorKnob_VCV (SynthEditLib/modules/SubControlsXp/VectorRingGui.cpp):
// a filled disc with a single radial pointer line, sharing VectorKnob's circular hit-test.
class TiDEsliderSwitchGui final : public PluginEditor, public gmpi::api::IDrawingLayer
{
 	Pin<float> pinpatchValue;
 	Pin<std::wstring> pinHint;
 	// Pins are auto-indexed in declaration order, so these must stay in the same
 	// order as the <GUI> pins in TiDEknob.cpp.
 	Pin<std::string> pinBackgroundColor;
 	Pin<std::string> pinStrokeColor;

 	void onSetpatchValue()
	{
		if (drawingHost)
			drawingHost->invalidateRect(nullptr);
	}

	// An unconnected colour pin arrives empty, which would decode as black —
	// fall back to the built-in look instead. colorFromHexString is gmpi_ui's
	// own (Drawing.h:654) and already implements SynthEdit's AARRGGBB
	// convention, alpha taken from the high byte only when present.
	static Color colorOrDefault(const std::string& hex, Color fallback)
	{
		return hex.empty() ? fallback : colorFromHexString(hex);
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
	TiDEsliderSwitchGui()
	{
		pinpatchValue.onUpdate = [this](PinBase*) { onSetpatchValue(); };
		pinBackgroundColor.onUpdate = [this](PinBase*) { onSetpatchValue(); };
		pinStrokeColor.onUpdate = [this](PinBase*) { onSetpatchValue(); };
	}

	ReturnCode hitTest(Point point, [[maybe_unused]] int32_t flags) override
	{
		return ReturnCode::Ok;
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

	ReturnCode onPointerUp([[maybe_unused]] Point point, [[maybe_unused]] int32_t flags) override
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

		auto switchRect = bounds;
		const auto isHorizontal = getWidth(bounds) > getHeight(bounds);
		if(isHorizontal)
		{
			switchRect.left += (std::max)(1.0f, getWidth(bounds) * 0.5f * pinpatchValue.value);
			switchRect.right = switchRect.left + getWidth(bounds) * 0.5f;
		}
		else
		{
			switchRect.top += (std::max)(1.0f, getHeight(bounds) * 0.5f * (1.0f - pinpatchValue.value));
			switchRect.bottom = switchRect.top + getHeight(bounds) * 0.5f;
		}

		auto brush = g.createSolidColorBrush(colorOrDefault(pinStrokeColor.value, Colors::White));
		g.fillRectangle(switchRect, brush);

		// shadow on grippy peaks
		brush.setColor({1.0f,1.0f,1.0f,0.2f});
		auto peakRect = switchRect;
		const auto dx = isHorizontal ? getWidth(bounds) / 8.f: 0.0f;
		const auto dy = isHorizontal ? 0.0f : getHeight(bounds) / 8.f;
		if(isHorizontal)
			peakRect.right = peakRect.left + dx * 0.5f;
		else
			peakRect.bottom = peakRect.top + dy * 0.5f;

		for(float peak = 0.0f; peak < 4.0f; peak++)
		{
			g.fillRectangle(peakRect, brush);

			peakRect.left += dx;
			peakRect.right += dx;
			peakRect.top += dy;
			peakRect.bottom += dy;
		}

		return ReturnCode::Ok;
	}
	// Layer 4 = editor guide (see IDrawingLayer in NativeUi.h): a design-time-only
	// overlay pass, so the debug outline still needs its own _DEBUG guard to stay
	// out of Release-configuration modules loaded into the same editor. Once a
	// plugin implements IDrawingLayer, render() is never called for layer 0 either
	// -- so all drawing, not just this guide, lives here now.
	ReturnCode renderLayer(gmpi::drawing::api::IDeviceContext* drawingContext, int32_t layer) override
	{
		if(layer == 1)
		{
			return render(drawingContext);
		}
		else if(layer == 4)
		{
			Graphics g(drawingContext);

			StrokeStyleProperties strokeStyleProperties{};
			strokeStyleProperties.lineCap = CapStyle::Round; // Flat caps don't draw dots on Windows.
			strokeStyleProperties.dashStyle = DashStyle::Dot;
			auto dottedStroke = g.getFactory().createStrokeStyle(strokeStyleProperties);

			g.drawRectangle(bounds, g.createSolidColorBrush(Colors::Orange), 1.0f, dottedStroke);

			return render(drawingContext);
		}

		return ReturnCode::NoSupport;
	}

	// support IDrawingLayer
	int32_t addRef() override
	{
		return PluginEditor::addRef();
	}

	int32_t release() override
	{
		return PluginEditor::release();
	}
	ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		*returnInterface = {};

		if((*iid) == gmpi::api::IDrawingLayer::guid)
		{
			*returnInterface = static_cast<gmpi::api::IDrawingLayer*>(this);
			PluginEditor::addRef();
			return ReturnCode::Ok;
		}

		return PluginEditor::queryInterface(iid, returnInterface);
	}
};

namespace
{
auto r = gmpi::Register<TiDEsliderSwitchGui>::withId("SE TiDE:sliderswitch");
}
