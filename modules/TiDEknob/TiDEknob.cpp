// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "Processor.h"

using namespace gmpi;

struct TiDEknob final : public Processor
{
	FloatInPin pinpatchValue;
	IntInPin pinAppearance;
    FloatOutPin pinSignalOut;

    TiDEknob() = default;

	void onSetPins() override
	{
		pinSignalOut = pinpatchValue;
	}
};

namespace
{
auto r = Register<TiDEknob>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="SE TiDE:knob" name="knob" category="TiDE">
    <Parameters>
        <Parameter id="0" datatype="float" name="patchValue"/>
    </Parameters>
    <Audio>
        <Pin name="patchValue" datatype="float" private="true" parameterId="0"/>
        <Pin name="Appearance" datatype="enum" isMinimised="true" metadata="knob"/>
        <Pin name="Signal Out" datatype="float" direction="out" autoConfigureParameter="true"/>
    </Audio>
    <GUI>
        <Pin name="patchValue" datatype="float" private="true" parameterId="0"/>
        <Pin name="Hint In" datatype="string_utf8" parameterId="0" parameterField="Hint"/>
        <Pin name="Background Color" datatype="string_utf8" default="00000000"/>
        <Pin name="Stroke Color" datatype="string_utf8" default="FFF9692C"/>
    </GUI>
</Plugin>
)XML");
}
