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
#include <vector>
#include "Processor.h"
#include "SynthRuntime.h"          // TideSynth S12 - the rack's sound engine
#include "IProcessorMessageQues.h"
#include "tinyxml/tinyxml.h"       // TideSynth E10 - validating the chunk before the engine parses it
#include "ChunkPrefix.h"           // why did this chunk arrive - Build, Sync, or Legacy

using namespace gmpi;

class SynthEdit final : public Processor, public IShellServices, public IProcessorMessageQues
{
	// Pin order mirrors the <Audio> section above - pins self-register in
	// declaration order.
	MidiInPin pinMidi;
	AudioOutPin pinLeft;
	AudioOutPin pinRight;
	BlobInPin pinChunk; // parameterId 1: the document's DSP XML, pushed by the editor

	// parameterId 2: the RETURN path -- the inner rack's DSP->GUI messages,
	// forwarded verbatim to the editor. See drainRackFeedback().
	BlobOutPin pinFeedback;

	// parameterId 3: the OUTBOUND path -- the editor's parameter edits, as
	// whole `ppc` messages, fed straight into the queue the rack already
	// polls (SynthRuntime::ServiceDspRingBuffers). This is what makes turning
	// a knob cost a message instead of a full graph rebuild.
	BlobInPin pinDspMessages;

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

	// IProcessorMessageQues - the queue pair SynthRuntime services.
	//
	// queDspToUi IS NOW DRAINED (drainRackFeedback), and it is a
	// lock_free_fifo rather than an interThreadQue for one reason: siphon2()
	// hands out a pointer to the queued bytes so they can be forwarded
	// WITHOUT re-framing them. That is exactly what the VST3 wrapper does
	// with its own DSP->controller queue (Processor_VST3::CommunicationProc),
	// and forwarding the framing intact is what lets the editor side reuse
	// the identical reader. Both types satisfy MessageQueToGui()'s
	// IWriteableQue return, so SynthRuntime is unaffected by the swap.
	gmpi::hosting::lock_free_fifo queDspToUi{ SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE };
	gmpi::hosting::interThreadQue queUiToDsp{ SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE };

	// One line the first time rack feedback actually reaches the editor -- the
	// question "is the return path alive?" had no answer for the whole thin
	// slice, and a silent success is as hard to read as a silent failure.
	bool loggedFeedback = false;
	bool loggedDspMessageOverflow = false;


	// drainRackFeedback's whole-message reassembly. Holds at most a partial
	// message tail between blocks; see the function for why partial bytes
	// must never ride a pin update.
	std::vector<uint8_t> feedbackScratch;

public:
	SynthEdit()
	{
		rack.connectPeer(this);

		// The host drives every block here; a TriggerRestart fade can never
		// complete. Restart requests (a re-cabling, a polyphony change) are
		// parked instead and consumed at the top of subProcess.
		rack.setHostDrivenRestart(true);
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
		// Parameter edits from the editor: hand the bytes to the rack's own
		// ui->dsp queue and let SynthRuntime::ServiceDspRingBuffers poll them
		// out, exactly as it does when the editor and DSP share a process.
		// The framing is untouched, so the reader on the far side is the one
		// SynthEdit already uses.
		if (pinDspMessages.isUpdated())
		{
			const auto& blob = pinDspMessages.getValue();
			if (!blob.empty())
			{
				if (blob.size() <= (size_t)queUiToDsp.freeSpace())
				{
					queUiToDsp.pushString(static_cast<int>(blob.size()), blob.data());
					queUiToDsp.Send();
				}
				else if (!loggedDspMessageOverflow)
				{
					// Audio thread: one line per instance, never per block.
					loggedDspMessageOverflow = true;
					fprintf(stderr,
						"TIDE: dropped a %zu-byte parameter update - the rack's ui->dsp queue was full.\n",
						blob.size());
				}
			}
		}

		if (pinChunk.isUpdated())
		{
			const auto& blob = pinChunk.getValue();
			if (blob.size() > 0)
			{
				// The 4-byte tag says why the bytes came (ChunkPrefix.h). A
				// Sync chunk is a save-time refresh of the persistent value -
				// the standalone autosaves moments after every knob tweak -
				// and a running rack must not pay a rebuild for it. It only
				// builds a rack that does not exist yet: the wrapper re-seeds
				// this parameter into a FRESH processor after a restart, and
				// there the very same bytes are the restore.
				const auto kind = tideChunk::classify(blob.data(), blob.size());
				if (kind == tideChunk::Kind::Sync && rackPrepared)
				{
					// Refresh only; the running rack already holds these
					// values live. Fall through to the tail below - the
					// sleep/subProcess re-assertions must run on every call.
				}
				else
				{
				const std::string xml(
					(const char*)tideChunk::payload(blob.data(), kind),
					tideChunk::payloadSize(blob.size(), kind));

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

				// One line per build, size included. E27 was a chunk silently
				// truncated by the host, and NOTHING said what arrived - a
				// blank restore printed the same nothing as a good one. The
				// size is the discriminator a log reader needs: a real rack is
				// tens of KB, the seeded blank is ~13KB.
				fprintf(stderr, "TIDE: building rack from %zu byte document\n", xml.size());

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
				} // else (not a Sync refresh)
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

		// The rack asked for a rebuild (DoAsyncRestart in plugin mode - a
		// host control like the patch-cable list changed). Consume it at a
		// block BOUNDARY, never mid-process, and rebuild from the document
		// the rack already holds; persistAcrossResets carries the new cable
		// list into the rebuilt graph. Same synchronous audio-thread cost as
		// a chunk arrival, and far rarer.
		if (rackPrepared && host && rack.takePluginRestartRequest())
		{
			fprintf(stderr, "TIDE: rack requested rebuild (host control changed)\n");
			rack.prepareToPlay(
				this,
				static_cast<int32_t>(host->getSampleRate()),
				host->getBlockSize(),
				true);
		}

		int64_t silenceFlagsOut{};
		rack.process(sampleFrames, nullptr, outputs, 0, 2, 0, silenceFlagsOut);

		drainRackFeedback();
	}

	// THE DSP -> GUI RETURN PATH, and until this existed nothing in TIDE had
	// one -- for any module, not just the VCV ports whose dead LEDs exposed it.
	//
	// rack.process() above ends in SeAudioMaster::PostProcess ->
	// ServiceDspWaiters2, which serialises every pending parameter update
	// (output parameters, meters, Scope captures -- SynthEditLib's "ppc"
	// messages) into MessageQueToGui(), i.e. queDspToUi. Nothing read it, so
	// every one of those sat there forever: measured 2026-08-25, the VCV LFO's
	// light queued 1054 updates in 12 s and the editor received none.
	//
	// WHY A BLOB PARAMETER rather than reaching for the editor directly: the
	// processor and the editor are separate objects, and under AUv3 separate
	// PROCESSES, so nothing may be handed across as a pointer. A blob output
	// parameter is GMPI's own supported channel for exactly this and every
	// wrapper already implements it -- gmpi_processor::setPin queues the
	// parameter, the wrapper ships it to the controller, and
	// gmpi_controller_holder delivers it to each editor's matching GUI pin as
	// a "ppc3" message. It is the chunk parameter's route, in reverse.
	//
	// THE BYTES ARE FORWARDED VERBATIM, framing included, but ONLY IN WHOLE
	// MESSAGES -- and the second half is what the first version of this
	// function got wrong, in a way that WEDGED the editor after a handful of
	// updates.
	//
	// A GMPI parameter is a VALUE, not a queue: gmpi_processor::setPin stores
	// the blob and the wrapper ships whatever is CURRENT when it services the
	// parameter -- last writer wins. Under light traffic every update ships
	// and the distinction is invisible, which is exactly why the lights
	// worked and the first Scope frames worked. Under 64KB-per-frame display
	// state, updates outrun the servicing, an intermediate blob is
	// overwritten unsent -- and if blobs carry ARBITRARY BYTE RUNS of the
	// stream (the old code forwarded each contiguous fifo run as its own
	// update), losing one tears a message in half. The editor-side queue then
	// reads a length field from the middle of someone else's payload and
	// waits forever for a message that size: every later byte feeds the
	// phantom, nothing ever parses again, the display freezes for good.
	// Measured 2026-08-25: apply-checksums streaming on a fresh instance,
	// frozen minutes later with arrivals still counting -- Jeff's "handful of
	// updates, then frozen".
	//
	// So each pin update is SELF-CONTAINED: whole messages only, split found
	// by walking the (handle, id, length) headers the queue writes. A dropped
	// update then loses those messages and nothing else -- for feedback
	// values the next update supersedes them anyway -- and the receiver can
	// never desynchronise. The scratch buffer holds at most one partial
	// message tail between blocks (a message split across the fifo's wrap or
	// a half-serialised multipart); it is not a second queue.
	void drainRackFeedback()
	{
		// Everything out of the fifo (two contiguous runs when it wraps).
		int readyBytes = queDspToUi.readyBytes();
		while (readyBytes > 0)
		{
			void* data{};
			const auto contiguous = queDspToUi.siphon2(&data);
			if (contiguous <= 0)
				break;

			const auto* p = static_cast<const uint8_t*>(data);
			feedbackScratch.insert(feedbackScratch.end(), p, p + contiguous);
			queDspToUi.siphon2_advance(contiguous);
			readyBytes -= contiguous;
		}

		// How many WHOLE messages sit at the front of the scratch?
		constexpr size_t headerSize = 3 * sizeof(int32_t); // handle, id, length
		size_t whole = 0;
		while (feedbackScratch.size() - whole >= headerSize)
		{
			int32_t messageLength{};
			memcpy(&messageLength, feedbackScratch.data() + whole + 2 * sizeof(int32_t), sizeof(messageLength));

			// A corrupt length would re-create the exact wedge this function
			// exists to prevent; the queue is 5MB, so nothing legitimate is
			// bigger. Drop the lot and resynchronise on fresh messages.
			if (messageLength < 0 || messageLength > SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE)
			{
				fprintf(stderr, "TIDE: rack feedback stream corrupt (length %d) - resetting\n", messageLength);
				feedbackScratch.clear();
				return;
			}

			const size_t total = headerSize + static_cast<size_t>(messageLength);
			if (feedbackScratch.size() - whole < total)
				break; // partial tail: keep for next block

			whole += total;
		}

		if (whole == 0)
			return;

		pinFeedback.setRaw({ feedbackScratch.data(), whole });
		pinFeedback.sendPinUpdate(getBlockPosition());
		feedbackScratch.erase(feedbackScratch.begin(), feedbackScratch.begin() + whole);

		if (!loggedFeedback)
		{
			loggedFeedback = true;
			fprintf(stderr, "TIDE: rack feedback reaching the editor - first %zu byte(s)\n", whole);
		}
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
		//   name   -- the PRODUCT, "TiDE Rack" (PLAN's naming ruling).
		//   vendor -- the ORGANISATION, "TiDE Synth". VST3 draws exactly this
		//             vendor/product distinction, so the two fields get two
		//             different answers rather than one name twice. Omitting
		//             `vendor` defaults it to "GMPI" (xml_spec_reader.cpp:532-535),
		//             which is why hosts listed this as "SynthEdit (GMPI)".
		//
		//   id     -- TIDE'S OWN IDENTITY, and the single string every format
		//             derives its identity from. Renamed 2026-08-22 (BACKLOG R9);
		//             it was "SE SynthEdit", a fossil of the original "SynthEdit
		//             in the DAW" framing, from before the pivot to TIDE Rack.
		//
		// WHAT THIS ONE STRING CONTROLS -- all four formats, which is why it is
		// worth getting right once:
		//
		//   VST3  class GUID = djb2 hash of this string, textIdtoUuid() at
		//         GMPI_Wrappers/wrapper/VST3/MyVstPluginFactory.cpp:200
		//   CLAP  clap_descriptor.id, verbatim (Factory_CLAP.cpp:25)
		//   GMPI  the id, verbatim
		//   AU    manufacturer and subtype, via to4charId() on the two halves
		//         either side of the colon (plist_util.cpp:262-271)
		//
		// THE COLON IS LOAD-BEARING. "Vendor: Product" is the convention the other
		// GMPI plugins follow ("GMPI: Freq Analyser", "GMPI: GmpiSawDemo"), and
		// plist_util splits on it for the AU's four-character codes. Without one
		// there is no vendor half -- which is how the old id produced the subtype
		// "Syhd", SynthEdit's rather than TIDE's.
		//
		// RENAMING IT IS A BREAKING CHANGE, and that is affordable exactly once.
		// The GUID is a pure function of this string, so a host project saved with
		// an older build stops finding the plug-in. Jeff ruled 2026-08-22: none of
		// this has ever shipped, so there is nothing to be compatible with. After
		// 1.0 that stops being true -- treat this string as frozen from then on.
		const static std::string xmlstr(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<PluginList>
    <Plugin id="TIDE Synth: TIDE Rack" name="TiDE Rack" vendor="TiDE Synth" category="Experimental" version="0.1.1">
		<Parameters>
			<Parameter id="0" name="controllerPtr" ignorePatchChange="true" datatype="blob" persistant="false" private="true"/>
			<Parameter id="1" name="chunk"         ignorePatchChange="true" datatype="blob"/>
			<Parameter id="2" name="feedback"      ignorePatchChange="true" datatype="blob" persistant="false" private="true"/>
			<Parameter id="3" name="dspMessages"   ignorePatchChange="true" datatype="blob" persistant="false" private="true"/>
		</Parameters>
        <Audio>
            <Pin name="MIDI" datatype="midi"/>
            <Pin name="Left"  datatype="float" rate="audio" direction="out"/>
            <Pin name="Right" datatype="float" rate="audio" direction="out"/>
            <Pin name="chunk" datatype="blob" parameterId="1"/>
            <Pin name="feedback" datatype="blob" direction="out" parameterId="2"/>
            <Pin name="dspMessages" datatype="blob" parameterId="3"/>
        </Audio>
        <GUI graphicsApi="GmpiGui">
			<Pin name="controllerPtr" datatype="blob" parameterId="0" private="true" />
			<Pin name="feedback" datatype="blob" parameterId="2" private="true" />
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
