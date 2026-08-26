// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class PatchPointGui final : public PluginEditor, public gmpi::api::IDrawingLayer
{
	// Radius of the clickable disc, and of the debug outline.
	static constexpr float radius = 9.0f;
	static constexpr Point center{ 10.0f, 10.0f };

public:
	PatchPointGui() = default;

	// Layer 4 = editor guide (see IDrawingLayer in NativeUi.h): a design-time-only
	// overlay pass, so the debug outline still needs its own _DEBUG guard to stay
	// out of Release-configuration modules loaded into the same editor. Once a
	// plugin implements IDrawingLayer, render() is never called for layer 0 either
	// -- so all drawing, not just this guide, lives here now.
	ReturnCode renderLayer(gmpi::drawing::api::IDeviceContext* drawingContext, int32_t layer) override
	{
		if (layer == 4)
		{
			Graphics g(drawingContext);

			StrokeStyleProperties strokeStyleProperties{};
			strokeStyleProperties.lineCap = CapStyle::Round; // Flat caps don't draw dots on Windows.
			strokeStyleProperties.dashStyle = DashStyle::Dot;
			auto dottedStroke = g.getFactory().createStrokeStyle(strokeStyleProperties);

			g.drawEllipse({ center, radius + 0.5f, radius + 0.5f }, g.createSolidColorBrush(Colors::Orange), 1.0f, dottedStroke);

			return ReturnCode::Ok;
		}
		return ReturnCode::NoSupport;
	}

	// Fixed size: return the same constant regardless of availableSize. That is how
	// an editor declares it does not resize (see PluginEditor::measure).
	ReturnCode measure([[maybe_unused]] const Size* availableSize, Size* returnDesiredSize) override
	{
		*returnDesiredSize = Size{ 20.0f, 20.0f };
		return ReturnCode::Ok;
	}

	// Ok = hit, Unhandled/Fail = miss.
	// The base class defaults to Ok so the user can select by clicking; here we
	// narrow the hit area to the disc so the corners of the 20x20 box fall through.
	// point will always be within the bounding rect.
	ReturnCode hitTest(Point point, [[maybe_unused]] int32_t flags) override
	{
		const float dx = point.x - center.x;
		const float dy = point.y - center.y;

		return dx * dx + dy * dy <= radius * radius ? ReturnCode::Ok : ReturnCode::Fail;
	}

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

		if ((*iid) == gmpi::api::IDrawingLayer::guid)
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
	// One editor serves both patch-point plugins. These ids must match the
	// <Plugin id="..."> attributes in PatchPoint.cpp exactly, or the host finds
	// no editor for the plugin. The factory keys on {subtype, id}, so registering
	// the same class twice under two ids is fine -- SDK3 needed a second subclass
	// here only because its macro keyed on the type.
	auto rIn = gmpi::Register<PatchPointGui>::withId("TiDE Patch Point In");
	auto rOut = gmpi::Register<PatchPointGui>::withId("TiDE Patch Point Out");
}
