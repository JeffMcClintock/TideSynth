// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class PatchPointGui final : public PluginEditor
{
	// Radius of the clickable disc, and of the debug outline.
	static constexpr float radius = 9.0f;
	static constexpr Point center{ 10.0f, 10.0f };

public:
	PatchPointGui() = default;

	ReturnCode render(gmpi::drawing::api::IDeviceContext *drawingContext) override
	{
#ifdef _DEBUG
		Graphics g(drawingContext);

		g.drawCircle(center, radius + 0.5f, g.createSolidColorBrush(Colors::Gray));
#endif
		return ReturnCode::Ok;
	}

	// Fixed size: return the same constant regardless of availableSize. That is how
	// an editor declares it does not resize (see PluginEditor::measure).
	ReturnCode measure(const Size* availableSize, Size* returnDesiredSize) override
	{
		*returnDesiredSize = Size{ 20.0f, 20.0f };
		return ReturnCode::Ok;
	}

	// Ok = hit, Unhandled/Fail = miss.
	// The base class defaults to Ok so the user can select by clicking; here we
	// narrow the hit area to the disc so the corners of the 20x20 box fall through.
	// point will always be within the bounding rect.
	ReturnCode hitTest(Point point, int32_t flags) override
	{
		const float dx = point.x - center.x;
		const float dy = point.y - center.y;

		return dx * dx + dy * dy <= radius * radius ? ReturnCode::Ok : ReturnCode::Fail;
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
