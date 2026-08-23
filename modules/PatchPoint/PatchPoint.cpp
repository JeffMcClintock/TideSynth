// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
#include "Processor.h"
#include "Extensions/PinConnection.h"

using namespace gmpi;

struct PatchPointOut : public Processor
{
	AudioInPin pinInput;
	AudioOutPin pinOutput;

	PatchPointOut() = default;

	void subProcess( int sampleFrames )
	{
		const auto* in = getBuffer(pinInput);
		auto* __restrict out = getBuffer(pinOutput);

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
    <Audio>
        <Pin name="Input" datatype="audio" linearInput="true" isMinimised="true"/>
        <Pin name="Output" datatype="audio" direction="out"/>
        <Pin name="Connected" datatype="bool" direction="out"/>
    </Audio>
    <GUI graphicsApi="GmpiUi"/>
    <PatchPoints>
        <PatchPoint pinId="0" center="10,10" radius="5"/>
    </PatchPoints>
</Plugin>
)XML");

auto r2 = Register<PatchPointOut>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
  <Plugin id="TiDE Patch Point Out" name="Patch Point-&gt;" category="TiDE">
   <Audio>
      <Pin name="Input" datatype="audio" linearInput="true"/>
      <Pin name="Output" datatype="float" direction="out" rate="audio" isMinimised="true" />
    </Audio>
    <GUI graphicsApi="GmpiUi"/>
    <PatchPoints>
		<PatchPoint pinId="1" center="10,10" radius="5" />
    </PatchPoints>
  </Plugin>
)XML");
}
