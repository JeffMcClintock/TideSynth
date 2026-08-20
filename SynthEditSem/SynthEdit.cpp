/* Copyright (c) 2007-2023 SynthEdit Ltd

Permission to use, copy, modify, and /or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <cstdio>
#include <cstring>
#include "Processor.h"
#include "SynthRuntime.h"          // TideSynth S12 - the rack's sound engine
#include "IProcessorMessageQues.h"
#include "tinyxml/tinyxml.h"       // TideSynth E10 - validating the chunk before the engine parses it

using namespace gmpi;

class SynthEdit final : public Processor, public IShellServices, public IProcessorMessageQues
{
	// Pin order mirrors the <Audio> section above - pins self-register in
	// declaration order.
	MidiInPin pinMidi;
	AudioOutPin pinLeft;
	AudioOutPin pinRight;
	BlobInPin pinChunk; // parameterId 1: the document's DSP XML, pushed by the editor

	// TideSynth S12: the rack's actual sound engine. The same class the
	// SynthEdit VST3 target uses; the document arrives through setDocumentXml
	// instead of from an exporter-baked bundle resource. All of this lives in
	// TIDE, not in the (generic, shared) wrappers - Jeff's ruling.
	SynthRuntime rack;
	bool rackPrepared = false;

	// TideSynth V3 -- "is it even receiving MIDI?", which is the first question
	// to ask when a rack that should be played from the host stays silent, and
	// the one nothing could answer before. A rack module wired to MIDI has
	// FOUR places it can fail (host does not send; the wrapper declares no
	// event input; this method never runs; the rack drops it), and from the
	// outside all four look identical. These two one-shot lines separate the
	// last two from the first two.
	//
	// One print per instance per condition, on purpose: this is the audio
	// thread, so it must not log per event. It is a first-event report, not a
	// trace. Matches the `TIDE:` stderr convention TideApp.cpp:612 uses, which
	// is why the DAW must be launched from a shell to see it.
	bool loggedMidiSeen = false;
	bool loggedMidiDropped = false;
	bool loggedBadDocument = false;
	bool loggedUnpreparedSilence = false;

	// TideSynth: the sample rate the rack was last built for. prepareToPlay is
	// reached from exactly one place -- the chunk arriving in onSetPins -- so the
	// rack's rate is latched at DOCUMENT push time, and nothing in this class
	// handles a rate change on its own.
	//
	// That sounds like a detune bug and is not one, because THE OBJECT HOLDING
	// THIS MEMBER DOES NOT SURVIVE A RATE CHANGE. Every wrapper absorbs one by
	// destroying the IProcessor and constructing a fresh instance:
	// gmpi_processor::start_processor (GMPI/Hosting/processor_holder.cpp:48)
	// releases the old plugin (:55), creates a new one (:69), calls open() (:82),
	// and deliberately re-seeds the blob parameter from its retained bytes (:215)
	// -- so the chunk arrives again, onSetPins runs again, and the rack is built
	// at the NEW rate. VST3 hangs that off setActive(true), which the spec
	// mandates the host bracket a rate change with; AU off Initialize(), which it
	// refuses to skip because StreamFormatWritable returns false while
	// initialised; CLAP off activate().
	//
	// So the "rate CHANGED" branch below can never fire under those wrappers:
	// preparedSampleRate is re-zeroed with each new object. It is kept as a
	// tripwire for a host that changes rate on a SURVIVING instance -- which is
	// the one case nothing here would handle -- and the unsuffixed line remains
	// useful as a once-per-instance report of the rate the rack was built at.
	// Measured in REAPER 2026-08-18, driving the device 48000 -> 44100 -> 48000 on
	// a loaded project: eight build lines, none suffixed, audio unchanged in level
	// afterwards. Working: TideSynth docs/e9-sample-rate.md, row E9.
	int preparedSampleRate = 0;

	// IProcessorMessageQues - the queue pair SynthRuntime services. Nothing
	// drains the GUI-bound queue yet (parameters don't flow in the thin
	// slice), but SeAudioMaster's servicing path dereferences the peer, so
	// the queues must exist.
	gmpi::hosting::interThreadQue queDspToUi{ SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE };
	gmpi::hosting::interThreadQue queUiToDsp{ SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE };

public:
	SynthEdit()
	{
		rack.connectPeer(this);
	}

	// TideSynth E10: is this blob a document the engine can actually build?
	//
	// A host-supplied byte string reaches BuildDspGraph with no validation
	// anywhere in between, and BuildDspGraph trusts its shape completely: it
	// dereferences the <Document> element (SeAudioMaster.cpp:410) BEFORE the
	// `if (!pElem)` guard on the next line, walks into <DSP>'s first child
	// unchecked (:420, whose asserts compile out in Release), and reads the
	// <PatchManager> attributes at :428. A document that merely PARSES is not
	// enough for any of those.
	//
	// MEASURED, not theorised: a REAPER project whose saved chunk was the
	// well-formed but foreign `<Patch/>` segfaulted the render outright --
	// TiXmlNode::FirstChildElement <- BuildDspGraph <- prepareToPlay <-
	// onSetPins <- Processor_VST3::process, on a REAPER worker thread,
	// EXC_BAD_ACCESS at 0x28. Nothing exotic is needed to produce one: the
	// chunk arrives base64-decoded by gmpi::base64Decode, which silently skips
	// bytes outside its alphabet and returns whatever falls out, so a
	// truncated or hand-edited project file is sufficient.
	//
	// The real fix belongs in the engine and is filed GATED (E10). This is the
	// half TIDE owns: refuse the document rather than crash the host. Checked
	// against the same tinyxml1 the engine uses, so this cannot disagree with
	// it about what parses.
	static bool documentIsBuildable(const std::string& xml, const char*& whyNot)
	{
		TiXmlDocument doc;
		doc.Parse(xml.c_str());

		if (doc.Error())
		{
			whyNot = "does not parse as XML";
			return false;
		}

		auto* document = doc.FirstChildElement("Document");   // guards :410
		if (!document)
		{
			whyNot = "has no <Document> root";
			return false;
		}

		auto* dsp = document->FirstChildElement("DSP");       // guards Open()'s null main_container
		if (!dsp)
		{
			whyNot = "has no <DSP> section";
			return false;
		}

		auto* first = dsp->FirstChildElement();               // guards :420-:426
		if (!first || strcmp(first->Value(), "Module") != 0)
		{
			whyNot = "has no <Module> inside <DSP>";
			return false;
		}

		return true;
	}

	// TideSynth: install the silence writer at open(), NOT when a document
	// arrives.
	//
	// subProcess used to be installed only at the tail of onSetPins, and
	// onSetPins only runs when a pin is set. For TIDE's pin layout a fresh
	// instance with no chunk gets NO such event -- start_processor queues
	// PinStreamingStart only for audio INPUTS (TIDE declares none), skips
	// output pins, has no MIDI default, and `continue`s on the empty blob
	// (processor_holder.cpp:226) -- so subProcess was never installed and
	// the output buffers were never touched at all.
	//
	// MEASURED: a REAPER project carrying a 440 Hz item and a TIDE instance
	// whose saved chunk is empty rendered the item straight through, peak
	// -6.0 dBFS at 440.00 Hz, with `TIDE: host MIDI arrived BEFORE the rack
	// was prepared` in the log proving the processor was live and unprepared.
	// No wrapper clears output buffers (Processor_VST3.cpp:906-941 hands them
	// over as-is and its silenceFlags block is #if 0; AU2 and CLAP likewise),
	// so what the host hears is whatever was in the buffer. The comment in
	// subProcess claiming "silence, exactly as the pre-S12 stub behaved" was
	// aspirational: this is what makes it true.
	ReturnCode open(api::IUnknown* phost) override
	{
		const auto r = Processor::open(phost);

		setSleep(false);
		setSubProcess(&SynthEdit::subProcess);

		return r;
	}

	void onSetPins() override
	{
		if (pinChunk.isUpdated())
		{
			const auto& blob = pinChunk.getValue();
			if (blob.size() > 0)
			{
				const std::string xml((const char*)blob.data(), blob.size());

				const char* whyNot = "";
				if (!documentIsBuildable(xml, whyNot))
				{
					// One line per instance: this is the audio thread, and a
					// host that re-sends a bad chunk must not be able to make
					// us log per block.
					if (!loggedBadDocument)
					{
						loggedBadDocument = true;
						fprintf(stderr,
							"TIDE: REFUSED a %zu-byte chunk - it %s. The rack is unchanged"
							" and stays silent rather than crashing the host (E10).\n",
							blob.size(), whyNot);
					}
					return;
				}

				rack.setDocumentXml(xml);

				if (host)
				{
					// Build (or REbuild) the graph in place - setDocumentXml set
					// documentPending_, which forces prepareToPlay to
					// reinitialise even with unchanged sample rate. Synchronous
					// on the audio thread: fine for rack-sized documents, and
					// race-free for exactly that reason, but it parses XML and
					// builds the graph there.
					//
					// The faded swap can replace this later. It is reachable,
					// contrary to what this comment used to claim: resetting is
					// entered via ug_vst_out.h:65 -> SeAudioMaster::
					// onFadeOutComplete() -> OnFadeOutComplete()
					// (iseshelldsp.h:124), and ug_vst_out IS audioOutModule in a
					// plugin (SetupVstIO runs under !isEditor()). DoAsyncRestart
					// is likewise reached from dsp_patch_parameter.cpp:773 for any
					// host control with requiresAsyncRestart(), a set that
					// includes HC_PATCH_CABLES - every rack re-cabling. What is
					// true is only that nothing in TIDE calls it yet. Note it
					// could not absorb a rate change unaided: the resetting branch
					// rebuilds from SynthRuntime's member sampleRate
					// (SynthRuntime.cpp:388), which only prepareToPlay writes.
					const int hostRate = static_cast<int>(host->getSampleRate());
					if (hostRate != preparedSampleRate)
					{
						// Only on a CHANGE, so this cannot chatter on the audio
						// thread when a document is pushed repeatedly by editing.
						fprintf(stderr, "TIDE: rack built for %d Hz, block %d%s\n",
							hostRate, (int)host->getBlockSize(),
							preparedSampleRate ? " (rate CHANGED)" : "");
						preparedSampleRate = hostRate;
					}
					rack.prepareToPlay(
						this,
						static_cast<int32_t>(host->getSampleRate()),
						host->getBlockSize(),
						true);
					rackPrepared = true;
				}
			}
		}

		// The runtime must keep running through fades, rebuilds and tails -
		// never let the wrapper put this module to sleep.
		setSleep(false);
		setSubProcess(&SynthEdit::subProcess);
	}

	void onMidiMessage(int /*pin*/, std::span<const uint8_t> midiMessage) override
	{
		if (rackPrepared)
		{
			if (!loggedMidiSeen)
			{
				loggedMidiSeen = true;
				fprintf(stderr,
					"TIDE: host MIDI reaching the rack - first message %zu byte(s), status 0x%02x\n",
					midiMessage.size(),
					midiMessage.empty() ? 0u : static_cast<unsigned>(midiMessage[0]));
			}
			rack.MidiIn(getBlockPosition(), midiMessage.data(), static_cast<int>(midiMessage.size()));
		}
		else if (!loggedMidiDropped)
		{
			// Not a warning: the host may legitimately send MIDI before the
			// editor has pushed the document. It only matters if NO "reaching
			// the rack" line ever follows, which would mean every note was
			// dropped this way.
			loggedMidiDropped = true;
			fprintf(stderr,
				"TIDE: host MIDI arrived BEFORE the rack was prepared, dropped (waiting for the document)\n");
		}
	}

	void subProcess(int sampleFrames)
	{
		float* outputs[2] = { getBuffer(pinLeft), getBuffer(pinRight) };

		if (!rackPrepared)
		{
			// No document yet (or one that was refused): digital silence.
			// Reachable only because open() installs this function -- see there.
			if (!loggedUnpreparedSilence)
			{
				loggedUnpreparedSilence = true;
				fprintf(stderr, "TIDE: unprepared - writing silence to the host's output buffers\n");
			}
			for (auto* out : outputs)
				std::fill(out, out + sampleFrames, 0.0f);
			return;
		}

		int64_t silenceFlagsOut{};
		rack.process(sampleFrames, nullptr, outputs, 0, 2, 0, silenceFlagsOut);
	}

	// IShellServices. Empty bodies are honest: no controller-side reader
	// exists yet (S11 owns the return path), and TIDE has no
	// ignore-program-change or latency reporting in the thin slice.
	void onQueDataAvailable() override {}
	void flushPendingParameterUpdates() override {}
	void onSetParameter(int32_t, int32_t, RawView, int) override {}
	void EnableIgnoreProgramChange() override {}
	void latencyChanged(int) override {}

	// IProcessorMessageQues
	gmpi::hosting::IWriteableQue* MessageQueToGui() override { return &queDspToUi; }
	gmpi::hosting::interThreadQue* ControllerToProcessorQue() override { return &queUiToDsp; }
	void Service() override {}
};

// can't do normal register as would register as child plugin of itself.
//ReturnCode RegisterProcessorPrimary(const char* xml, CreatePluginPtr create)
//{
//	return Factory().RegisterPluginWithXml(
//		gmpi::api::PluginSubtype::Audio
//		, xml
//		, []() -> api::IUnknown* { return new SynthEdit(); }
//	);
//}

void* newSynthEditGui(); // see SynthEditGui.cpp
void* newSynthEditController(); // see SynthEditController.cpp

// custom factory for SynthEdit SEM
class FactorySpecial : public gmpi::api::IPluginFactory
{
public:
	// IMpPluginFactory methods
	ReturnCode createInstance(
		const char* uniqueId,
		gmpi::api::PluginSubtype subType,
		void** returnInterface) override
	{
		if (subType == gmpi::api::PluginSubtype::Audio)
		{
			*returnInterface = new SynthEdit();
			return gmpi::ReturnCode::Ok;
		}
		else if (subType == gmpi::api::PluginSubtype::Editor)
		{
			*returnInterface = newSynthEditGui();
			return gmpi::ReturnCode::Ok;
		}
		else if (subType == gmpi::api::PluginSubtype::Controller)
		{
			*returnInterface = newSynthEditController();
			return gmpi::ReturnCode::Ok;
		}

		*returnInterface = {};
		return gmpi::ReturnCode::NoSupport;
	}

	ReturnCode getPluginInformation(int32_t index, gmpi::api::IString* returnXml) override
	{
		if(index != 0)
		{
			return gmpi::ReturnCode::NoSupport;
		}

		// BACKLOG P5. This string IS the plug-in's host-visible identity -- it is
		// what getPluginInformation() hands the wrapper, and the wrapper parses
		// it with GMPI/Hosting/xml_spec_reader.cpp. The sibling file
		// SynthEditSem/SynthEdit.xml looks like the source of truth and is NOT:
		// it is only referenced by SynthEdit.rc, whose loader is behind #if 0.
		//
		//   name   -- the PRODUCT, "TIDE Rack" (PLAN's naming ruling).
		//   vendor -- the ORGANISATION, "TIDE Synth". VST3 draws exactly this
		//             vendor/product distinction, so the two fields get two
		//             different answers rather than one name twice. Omitting
		//             `vendor` defaults it to "GMPI" (xml_spec_reader.cpp:532-535),
		//             which is why hosts listed this as "SynthEdit (GMPI)".
		//
		// DO NOT RENAME id -- it is not cosmetic. The VST3 class UID is *hashed
		// from this string*: textIdtoUuid(plugin->id, ...) at
		// GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:244-245, feeding
		// info->cid. Change it and the plug-in's identity changes, so every host
		// project that already loaded TIDE fails to find it on reload. "SE
		// SynthEdit" is permanent, and users never see it.
		const static std::string xmlstr(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<PluginList>
    <Plugin id="SE SynthEdit" name="TIDE Rack" vendor="TIDE Synth" category="Experimental">
		<Parameters>
			<Parameter id="0" name="controllerPtr" ignorePatchChange="true" datatype="blob" persistant="false" private="true"/>
			<Parameter id="1" name="chunk"         ignorePatchChange="true" datatype="blob"/>
		</Parameters>
        <Audio>
            <Pin name="MIDI" datatype="midi"/>
            <Pin name="Left"  datatype="float" rate="audio" direction="out"/>
            <Pin name="Right" datatype="float" rate="audio" direction="out"/>
            <Pin name="chunk" datatype="blob" parameterId="1"/>
        </Audio>
        <GUI graphicsApi="GmpiGui">
			<Pin name="controllerPtr" datatype="blob" parameterId="0" private="true" />
		</GUI>
        <Controller/>
    </Plugin>
</PluginList>
)XML");

		returnXml->setData(xmlstr.data(), static_cast<int32_t>(xmlstr.size()));
		return gmpi::ReturnCode::Ok;
	}

//	ReturnCode RegisterPlugin(const char* uniqueId, gmpi::api::PluginSubtype subType, CreatePluginPtr create);
//	ReturnCode RegisterPluginWithXml(gmpi::api::PluginSubtype subType, const char* xml, CreatePluginPtr create);

	// IUnknown methods
	GMPI_QUERYINTERFACE_METHOD(IPluginFactory);
	GMPI_REFCOUNT_NO_DELETE
};

FactorySpecial& Factory()
{
	static FactorySpecial theFactory;
	return theFactory;
}
// This is the DLL's main entry point.  It returns the factory.
extern "C"

#ifdef _WIN32
__declspec (dllexport)
#else
#if defined (__GNUC__)
__attribute__((visibility("default")))
#endif
#endif

ReturnCode MP_GetFactory(void** returnInterface)
{
	// call queryInterface() to keep refcounting in sync
	return Factory().queryInterface(&gmpi::api::IUnknown::guid, returnInterface);
}

//namespace
//{
//	auto r = RegisterPrimary( // <SynthEdit>::withXml(
//R"XML(
//<?xml version="1.0" encoding="UTF-8"?>
//<PluginList>
//    <Plugin id="SE SynthEdit" name="SynthEdit" category="Experimental">
//		<Parameters>
//			<Parameter id="0" name="controllerPtr" ignorePatchChange="true" datatype="blob" persistant="false" private="true"/>
//			<Parameter id="1" name="chunk"         ignorePatchChange="true" datatype="blob"/>
//		</Parameters>
//        <Audio>
//            <Pin name="MIDI" datatype="midi"/>
//            <Pin name="Left"  datatype="float" rate="audio" direction="out"/>
//            <Pin name="Right" datatype="float" rate="audio" direction="out"/>
//        </Audio>
//        <GUI graphicsApi="GmpiGui">
//			<Pin name="controllerPtr" datatype="blob" parameterId="0" private="true" />
//		</GUI>
//        <Controller/>
//    </Plugin>
//</PluginList>
//)XML");
//}
