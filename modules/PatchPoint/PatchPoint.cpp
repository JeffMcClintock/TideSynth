// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include <cmath>
#include "Processor.h"
#include "Extensions/PinConnection.h"

using namespace gmpi;

struct PatchPointOut : public Processor
{
	AudioInPin pinInput;
	AudioOutPin pinOutput;
	FloatOutPin pinLevel; // to the GUI's LED, via a private parameter

	// The signed sample of largest magnitude since the last LED update, sent ~30 times a second.
	float peak = 0.0f;
	int samplesUntilLevel = 0;
	int levelInterval = 1;

	PatchPointOut() = default;

	void onGraphStart() override
	{
		levelInterval = (std::max)(1, static_cast<int>(host->getSampleRate() / 30.0f));
		Processor::onGraphStart();
	}

	void subProcess( int sampleFrames )
	{
		const auto* in = getBuffer(pinInput);
		auto* __restrict out = getBuffer(pinOutput);

		for (int i = 0; i < sampleFrames; ++i)
		{
			if (std::fabs(in[i]) > std::fabs(peak))
				peak = in[i];
		}
		samplesUntilLevel -= sampleFrames;
		if (samplesUntilLevel <= 0)
		{
			pinLevel.setValue(peak, getBlockPosition()); // subProcess must give the sample position
			peak = 0.0f;
			samplesUntilLevel = levelInterval;
		}

		// auto-vectorized copy.
		while(sampleFrames > 3)
		{
			out[0] = in[0];
			out[1] = in[1];
			out[2] = in[2];
			out[3] = in[3];

			out += 4;
			in += 4;
			sampleFrames -= 4;
		}

		while(sampleFrames > 0)
		{
			*out++ = *in++;
			--sampleFrames;
		}
	}

	void onSetPins() override
	{
		// Set state of output audio pins.
		pinOutput.setStreaming(pinInput.isStreaming());

		// A steady input stops subProcess sending levels, so send the steady value itself.
		if (!pinInput.isStreaming())
			pinLevel = pinInput.getValue();

		// Set processing method.
		setSubProcess(&PatchPointOut::subProcess);
	}
};

struct PatchPointIn final : public PatchPointOut
{
	BoolOutPin pinConnected;

	PatchPointIn() = default;

	void onGraphStart() override
	{
		// Is anything actually patched into the jack?
		//
		// pinInput.isStreaming() cannot answer this: it reports whether the audio
		// is time-varying, so a patched input holding a steady CV reads false -
		// backwards for the case that matters. Only the graph knows, hence the
		// IPinConnection extension. A host that lacks it reports every pin
		// connected, which is what modules assumed before it existed.
		//
		// Asked once, not per block: SynthEdit rebuilds the DSP graph when the
		// user patches a cable, so this cannot change while the processor lives.
		const synthedit::PinConnections pins(host.get(), pinInput.getIndex() + 1);
		pinConnected = pins.isConnected(pinInput.getIndex());

		// Set BEFORE the base class runs, so the initial update it sends on every
		// output pin already carries the answer.
		PatchPointOut::onGraphStart();
	}
};

namespace
{
auto r = Register<PatchPointIn>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="TiDE Patch Point In" name="Patch Point&lt;-" category="TiDE">
    <Parameters>
        <Parameter id="0" name="Level" datatype="float" private="true" ignorePatchChange="true" persistant="false"/>
    </Parameters>
    <Audio>
        <Pin name="Input" datatype="audio" linearInput="true" isMinimised="true"/>
        <Pin name="Output" datatype="audio" direction="out"/>
        <Pin name="Connected" datatype="bool" direction="out"/>
        <Pin name="Level" datatype="float" direction="out" parameterId="0" private="true"/>
    </Audio>
    <GUI graphicsApi="GmpiUi">
        <Pin name="Title" datatype="string_utf8" default=""/>
        <Pin name="Level" datatype="float" parameterId="0" private="true"/>
    </GUI>
    <PatchPoints>
        <PatchPoint pinId="0" center="15,25" radius="5"/>
    </PatchPoints>
</Plugin>
)XML");

auto r2 = Register<PatchPointOut>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
  <Plugin id="TiDE Patch Point Out" name="Patch Point-&gt;" category="TiDE">
    <Parameters>
        <Parameter id="0" name="Level" datatype="float" private="true" ignorePatchChange="true" persistant="false"/>
    </Parameters>
   <Audio>
      <Pin name="Input" datatype="audio" linearInput="true"/>
      <Pin name="Output" datatype="float" direction="out" rate="audio" isMinimised="true" />
      <Pin name="Level" datatype="float" direction="out" parameterId="0" private="true"/>
    </Audio>
    <GUI graphicsApi="GmpiUi">
        <Pin name="Title" datatype="string_utf8" default=""/>
        <Pin name="Level" datatype="float" parameterId="0" private="true"/>
    </GUI>
    <PatchPoints>
		<PatchPoint pinId="1" center="15,25" radius="5" />
    </PatchPoints>
  </Plugin>
)XML");
}
