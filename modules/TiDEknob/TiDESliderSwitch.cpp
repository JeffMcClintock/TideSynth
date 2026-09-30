// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "Processor.h"

using namespace gmpi;

struct TiDEswitch final : public Processor
{
	IntInPin pinpatchValue;
    EnumOutPin pinSignalOut;

    TiDEswitch() = default;

	void onSetPins() override
	{
		pinSignalOut = pinpatchValue;
	}
};

namespace
{
auto r = Register<TiDEswitch>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="SE TiDE:sliderswitch" name="switch" category="TiDE">
    <Parameters>
        <Parameter id="0" datatype="enum" name="patchValue"/>
    </Parameters>
    <Audio>
        <Pin name="patchValue" datatype="int" private="true" parameterId="0"/>
        <Pin name="Out" datatype="enum" direction="out" autoConfigureParameter="true"/>
    </Audio>
    <GUI>
        <Pin name="patchValue" datatype="float" private="true" parameterId="0" parameterField="Normalized"/>
        <Pin name="Hint In" datatype="string_utf8" parameterId="0" parameterField="Hint"/>
        <Pin name="Background Color" datatype="string_utf8" default="00000000"/>
        <Pin name="Stroke Color" datatype="string_utf8" default="FF000000"/>
    </GUI>
</Plugin>
)XML");
}
