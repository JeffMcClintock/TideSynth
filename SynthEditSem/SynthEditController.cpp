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

#include <cstring>
#include <vector>
#include <memory>
#include <string_view>
#include "RefCountMacros.h"
#include "Common.h"
#include "GmpiSdkCommon.h"
#include "GmpiApiEditor.h"
#include "TideAppWrapper.h"
#include "SynthEditDocBase.h"
#include "TideApp.h"
#include "ChunkPrefix.h"

using namespace gmpi;

class SynthEditController final : public gmpi::api::IController
{
	std::unique_ptr<ISeApp> seApp;
	TideApp* tideApp{}; // same object as seApp; typed, so setParameter needn't downcast
	gmpi::shared_ptr<gmpi::api::IControllerHost> host;
	int32_t handle = 0;

public:
	SynthEditController()
	{
	}
	~SynthEditController()
	{
		//_RPT0(0, "SynthEditController destructor\n");
//		seApp->CloseAllViews();
	}
#if 0
	ReturnCode initialize() override
	{
		const auto r = PluginEditor::initialize();

		auto client = seApp->Initial izeSem1();
		attach Client(client);
		seApp->Initial izeSem2(client);

		return ReturnCode::Ok;
	}
#endif
	// IController
	ReturnCode initialize(gmpi::api::IUnknown* phost, int32_t phandle) override
	{
		handle = phandle;
		phost->queryInterface(&gmpi::api::IControllerHost::guid, host.put_void());

		auto app = new TideApp();
		seApp.reset(app);
		tideApp = app;

		app->InitInstance();

		// Publish the seApp pointer via parameter 0 so editor instances can
		// pick it up later through gmpi_controller_holder::initUi.
		constexpr int32_t controllerPtrParamId = 0;
		const int voiceId = 0;
		auto me = seApp.get();

		if (host)
			host->setParameter(controllerPtrParamId, Field::Value, voiceId, sizeof(me), (const uint8_t*) &me);

		// S12 - the GUI's periodic serviceDocumentSync() lands here: push the
		// document's DSP XML through the chunk parameter (id 1). The wrapper
		// carries it to the processor's blob pin like any other parameter -
		// wrappers stay generic; everything TIDE-specific is on this side.
		app->onPushChunk = [this](const void* data, size_t size)
		{
			constexpr int32_t chunkParamId = 1;
			if (!host)
				return;

			// Tagged Build: the shape changed, and the processor must rebuild.
			// See ChunkPrefix.h for the whole story.
			std::vector<uint8_t> tagged(tideChunk::tagSize + size);
			memcpy(tagged.data(), tideChunk::tagBuild, tideChunk::tagSize);
			memcpy(tagged.data() + tideChunk::tagSize, data, size);
			host->setParameter(chunkParamId, Field::Value, 0, static_cast<int32_t>(tagged.size()), tagged.data());
		};

		// Parameter EDITS take this route instead of the chunk: whole `ppc`
		// messages on parameter 3, which the processor pushes into the queue
		// its rack already polls. Same carriage as the chunk, wildly cheaper
		// consequence - a value arrives as a value, and nothing is rebuilt.
		app->onPushDspMessages = [this](const void* data, size_t size)
		{
			constexpr int32_t dspMessagesParamId = 3;
			if (host)
				host->setParameter(dspMessagesParamId, Field::Value, 0, static_cast<int32_t>(size), (const uint8_t*)data);
		};

		return ReturnCode::Ok;
	}
	
	// "Sync unsaved state from plugin to host" - and until this existed a
	// knob tweaked after the last structural edit was NOT in the saved file.
	//
	// The chunk parameter is stale BY DESIGN between saves: keeping it
	// continuously fresh cost a whole-document serialisation twice a second
	// (removed 2026-08-25), and values now travel as messages. So the host
	// calls here immediately before it serialises state, and the chunk is
	// refreshed on demand - the exact pattern GMPI_Adaptors' VST3Adaptor
	// established (ControllerWrapper::syncState, triggered by the editor's
	// preSaveState walk).
	//
	// Tagged Sync, not Build: this refresh reaches the running processor too
	// (the wrapper ships every changed blob), and a rebuild per autosave
	// would be an audio glitch per knob tweak. A Sync chunk only builds a
	// rack that does not exist yet - the restore-after-restart re-seed.
	ReturnCode syncState() override
	{
		constexpr int32_t chunkParamId = 1;

		if (!tideApp || !host)
			return ReturnCode::Ok; // nothing loaded; nothing to sync

		const auto xml = tideApp->exportChunkXmlForSave();
		if (xml.empty())
			return ReturnCode::Ok;

		std::vector<uint8_t> tagged(tideChunk::tagSize + xml.size());
		memcpy(tagged.data(), tideChunk::tagSync, tideChunk::tagSize);
		memcpy(tagged.data() + tideChunk::tagSize, xml.data(), xml.size());
		host->setParameter(chunkParamId, Field::Value, 0, static_cast<int32_t>(tagged.size()), tagged.data());

		return ReturnCode::Ok;
	}

	// IParameterObserver — the inbound half of the chunk, and the reason this
	// is no longer a stub. The DAW has restored the plug-in's state; the
	// wrapper applies it to the controller's parameters and then hands each one
	// here, so parameter 1 arrives carrying the document the user saved.
	//
	// Note this is the CONTROLLER's route, not an editor pin route. Parameter 1
	// deliberately has no <GUI> pin: every controller->editor delivery iterates
	// guiPins and lands on an IEditor, which exists only while the plug-in
	// window is open. The document has to be restored whether or not the user
	// ever opens the window, so it belongs here, where TideApp lives.
	ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const uint8_t* data) override
	{
		constexpr int32_t chunkParamId = 1;

		if (parameterHandle != chunkParamId || fieldId != Field::Value)
			return ReturnCode::NoSupport;

		if (!tideApp || !data || size <= 0)
			return ReturnCode::NoSupport;

		// Either tag may come back from a save - Build if the last thing
		// before saving was structural, Sync if it was syncState's refresh -
		// and a pre-tag session.xml has none. The document is the same shape
		// underneath all three.
		const auto kind = tideChunk::classify(data, static_cast<size_t>(size));
		const auto* doc = tideChunk::payload(data, kind);
		const auto docSize = tideChunk::payloadSize(static_cast<size_t>(size), kind);

		// A chunk we cannot use leaves the blank document standing - an empty
		// rack, which is the fail-safe outcome S11 requires. It must never
		// escape as an exception: we are on the host's main thread during
		// project load.
		const bool imported = tideApp->importChunkXml(
			std::string_view(reinterpret_cast<const char*>(doc), docSize));

		return imported ? ReturnCode::Ok : ReturnCode::Fail;
	}

	ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		GMPI_QUERYINTERFACE(gmpi::api::IController);
		GMPI_QUERYINTERFACE(gmpi::api::IParameterObserver);
		return ReturnCode::NoSupport;
	}
	GMPI_REFCOUNT;
};

void* newSynthEditController() // see FactorySpecial
{
	return (void*) new SynthEditController();
}

//namespace
//{
//	auto r = gmpi::Register<SynthEditController>::withId("TIDE Synth: TIDE Rack");
//}
