// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class TiDEPanelGui final : public PluginEditor
{
	Pin<std::string> pinText;
	Pin<std::string> pinTextColor;

public:
	TiDEPanelGui() = default;

	ReturnCode render(gmpi::drawing::api::IDeviceContext *drawingContext) override
	{
		Graphics g(drawingContext);

		auto backgroundBrush = g.createSolidColorBrush(Colors::WhiteSmoke);
		g.fillRectangle(bounds, backgroundBrush);

		auto textFormat = g.getFactory().createTextFormat();
		auto brush = g.createSolidColorBrush(Colors::Red);

		g.drawTextU(pinText.value.c_str(), textFormat, bounds, brush);

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = Register<TiDEPanelGui>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="SE TiDE:Panel" name="Panel" category="TiDE">
    <GUI graphicsApi="GmpiUi">
        <Pin name="Text"       datatype="string_utf8" default="Filter"/>
        <Pin name="Text Color" datatype="string_utf8" default="FFff0000"/>
    </GUI>
</Plugin>
)XML");
}
