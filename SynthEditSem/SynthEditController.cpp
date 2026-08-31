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

#include <atomic>     // TideSynth E59 - the controller sequence number
#include <cstring>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <iostream>   // the command-line trace in applyCommandLineConfig
#include "TraceLog.h"   // TideSynth E73 - a hosted plug-in has no stderr
#include "RefCountMacros.h"
#include "EditorLib/ApplySynthEditConfig.h"
#if defined(GMPI_STANDALONE) && GMPI_STANDALONE
#include "StandaloneApp.h"
#endif
#include "Common.h"
#include "GmpiSdkCommon.h"
#include "GmpiApiEditor.h"
#include "TideAppWrapper.h"
#include "SynthEditDocBase.h"
#include "TideApp.h"
#include "ChunkPrefix.h"

using namespace gmpi;

namespace
{

// -quiet and friends, but ONLY when this build is the standalone executable.
//
// WHY THE GUARD IS THE WHOLE POINT. This runs inside the plugin, which in every
// other configuration is loaded into a DAW -- and a DAW's command line belongs
// to the DAW. Reading it there would let REAPER's own flags decide how TIDE
// behaves, silently and very hard to trace. Under the standalone the command
// line IS ours, so the question is answerable.
//
// GMPI_STANDALONE rather than GMPI_STANDALONE_COMMAND_CHANNEL: the latter is
// standalone-only today, but it answers "can a harness drive this build", which
// is a different question and can be switched off on its own. See the comment
// where GMPI_STANDALONE is defined.
//
// The argv comes from the wrapper's main(), which stashes it -- rather than from
// __argv / _NSGetArgv / proc/self/cmdline, which need a different incantation
// per platform AND work just as well inside a DAW, which is the one thing that
// must not happen.
void applyCommandLineConfig(CSynthEditAppBase& app)
{
#if defined(GMPI_STANDALONE) && GMPI_STANDALONE
	const int argc = gmpi::standalone::standaloneArgc();
	char** const argv = gmpi::standalone::standaloneArgv();

	// SAY WHAT ARRIVED. This whole function failing open is invisible: every
	// flag simply does nothing, and the only symptom is behaviour that looks
	// like the default. Measured 2026-08-27 (TIDE BACKLOG E48/E51) -- -quiet
	// was inert on Windows for as long as the standalone existed, because the
	// Win32 and Wayland shells called runStandaloneApp(shell) and its argc/argv
	// parameters are DEFAULTED, so nothing failed and nothing was said.
	//
	// stderr and unconditional: this runs once per launch, before the command
	// channel exists, and it is the one line that distinguishes "the flag did
	// not arrive" from "the flag arrived and did nothing".
	std::cerr << "TIDE: command line: argc=" << argc
	          << (argv ? "" : ", argv=NULL");
	for (int i = 1; argv && i < argc; ++i)
		std::cerr << ' ' << (argv[i] ? argv[i] : "(null)");
	std::cerr << std::endl;

	if (argc <= 1 || !argv)
		return; // nothing but the program name: leave every default alone

	// BEFORE InitInstance, which ApplySynthEditConfig.h requires and explains:
	// --rescan is implemented here as a cache clear that InitInstance then
	// regenerates from, and it is too late once the module set is loaded.
	// argv[0] IS INCLUDED, deliberately: ParseSynthEditArgs starts its loop at
	// index 1 because it expects a whole argv. Passing a pre-stripped vector puts
	// the first real flag at index 0, where it is silently skipped -- which cost
	// an hour here, because every other symptom (argc correct, argv non-null, the
	// define present) says the wiring works.
	std::vector<std::string_view> args;
	args.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; ++i)
		args.emplace_back(argv[i]);

	const auto cfg = ParseSynthEditArgs(args);
	std::cerr << "TIDE: parsed quiet=" << (cfg.quiet ? 1 : 0)
	          << " rescan=" << (cfg.rescanModules ? 1 : 0) << std::endl;
	ApplyConfigPreInit(app, cfg);
#else
	(void)app;
#endif
}


// Let the command channel read the prompts quiet mode diverted.
//
// The converting layer between two types that must not know each other: the
// wrapper is built against plugins that have never heard of EditorLib, and
// EditorLib has never heard of the standalone. This file is the one place that
// sees both, so the copy happens here.
//
// SAME GUARD AS THE FLAG ITSELF. Inside a DAW there is no standalone to install
// into, and the wrapper's symbols are not even linked.
void installDialogDrain(CSynthEditAppBase& app)
{
#if defined(GMPI_STANDALONE) && GMPI_STANDALONE
	// `app` outlives the channel: the controller owns it and the channel is
	// stopped during the same teardown, so capturing the address is safe.
	gmpi::standalone::setDialogDrain([&app]()
	{
		std::vector<gmpi::standalone::DivertedDialog> out;
		for (auto& p : app.takeDivertedPrompts())
			out.push_back({ p.title, p.text, p.flags, p.answered });

		return out;
	});
#else
	(void)app;
#endif
}

} // namespace

class SynthEditController final : public gmpi::api::IController
{
	std::unique_ptr<ISeApp> seApp;
	TideApp* tideApp{}; // same object as seApp; typed, so setParameter needn't downcast
	gmpi::shared_ptr<gmpi::api::IControllerHost> host;
	int32_t handle = 0;

	// E59: which controller, and in what ORDER did its two state calls happen.
	//
	// The chunk parameter has exactly two writers on this side -- onPushChunk
	// (Build) and syncState (Sync) -- and exactly one reader, setParameter.
	// From outside, a hosted session shows a restored rack in the editor and
	// the DEFAULT one in the DSP, and every explanation of that is a statement
	// about the ORDER of those three calls. Nothing printed any of them.
	//
	// A counter rather than `this`: a host may create and destroy controllers
	// during a project load, and a reused address reads as one object.
	static int nextControllerSeq()
	{
		static std::atomic<int> counter{ 0 };
		return ++counter;
	}
	const int controllerSeq = nextControllerSeq();

	// E59: the document this instance was BORN holding, captured once, before
	// the host has had any chance to restore anything.
	//
	// It exists to answer one question in syncState(): "is what I am about to
	// publish anything the user has ever seen, or is it just the bundle's
	// starter rack?" Publishing the latter is never useful and is actively
	// harmful -- see the comment there.
	//
	// Captured through exportChunkXmlForSave(), not exportChunkXml(), so that
	// both sides of the comparison are produced by the identical function. A
	// mismatch of PRODUCERS would make the comparison fail spuriously, and this
	// comparison must only ever fail towards publishing.
	std::string startupDefaultChunk;

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
		// E73 -- before the first trace line below. See SynthEdit.cpp's copy;
		// whichever of the two the host constructs first opens the file, and
		// in an AUv3 both live in the same out-of-process appex.
		tide::trace::redirectStderrOnce();

		handle = phandle;
		phost->queryInterface(&gmpi::api::IControllerHost::guid, host.put_void());

		auto app = new TideApp();
		seApp.reset(app);
		tideApp = app;

		// E59: the first line of the ordering trace. See controllerSeq.
		std::cerr << "TIDE: controller #" << controllerSeq
		          << " initialized (TideApp fresh - holds the DEFAULT rack until"
		             " setParameter restores one)" << std::endl;

		applyCommandLineConfig(*app);
		installDialogDrain(*app);


		app->InitInstance();

		// E59: capture the starter rack now. InitInstance has just loaded
		// DefaultRack.synthedit and nothing else can have touched the document
		// yet, so this is exactly "the rack a user has not chosen".
		startupDefaultChunk = app->exportChunkXmlForSave();
		std::cerr << "TIDE: controller #" << controllerSeq
		          << " startup default is " << startupDefaultChunk.size()
		          << " bytes (syncState will not publish this document)" << std::endl;

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

		// E59 -- THE FIX, AND IT IS A REFUSAL RATHER THAN A CORRECTION.
		//
		// MEASURED 2026-08-28 on Windows, REAPER 7.78, rendering
		// tests/hosts/v1-rack.rpp. The host asks the controller for state
		// BEFORE it restores any (Controller_VST3::getState, whose own comment
		// says "The host is saving" -- true of a save, and NOT true of the call
		// a host makes while instantiating). At that moment TideApp holds
		// nothing but the starter rack InitInstance loaded, so this function
		// published it:
		//
		//   TIDE: controller #1 syncState exporting 17959 byte document
		//   TIDE: controller #1 restore of a 14136 byte document -> imported
		//   TIDE: instance #3 building rack from 14136 byte document (Legacy...)
		//   TIDE: instance #5 building rack from 17959 byte document (Sync...)
		//
		// The two 17,959s are the same document, and that is the whole bug: the
		// bytes this function writes to the chunk parameter are RETAINED by the
		// processor holder and re-seeded into every processor it starts later
		// (gmpi_processor::start_processor, processor_holder.cpp:215). So a
		// pre-restore refresh does not merely go stale -- it becomes the
		// document the NEXT processor instance is born running, while the
		// editor goes on showing the restored one. The rack is on screen and
		// the DSP is playing something else; v1-rack.rpp renders digital
		// silence on a rack whose two patch cables are intact.
		//
		// It also refutes the assumption importChunkXml states in as many
		// words -- "the wrapper re-seeds the chunk parameter into the processor
		// when it starts, so the processor builds this same document on its
		// own". It does re-seed; the bytes were just not this document's.
		//
		// THE REFUSAL. Before any restore, the only thing this controller can
		// possibly hold is the starter rack, so "the export equals the startup
		// default" is exactly "I have nothing to say yet". Refusing to publish
		// it leaves the chunk parameter holding whatever the host restored,
		// which is the correct document, and costs nothing when there was
		// nothing to restore: a fresh instance the user never touched reloads
		// its starter rack from the bundle either way.
		//
		// A byte comparison, and the direction it fails in is the design. If the
		// two ever differ spuriously this publishes -- today's behaviour -- so a
		// false negative costs a bug that already exists, while a false positive
		// (suppressing a real save) is what would lose a user's work. Two
		// exports within one process are stable; E56's handle churn is per-LOAD,
		// not per-export.
		//
		// A knob tweak is NOT suppressed, which is what this function was added
		// for: it changes the exported bytes even though it changes no
		// structure, so the comparison fails and the refresh goes out.
		if (xml == startupDefaultChunk)
		{
			std::cerr << "TIDE: controller #" << controllerSeq
			          << " syncState declined to publish the startup default ("
			          << xml.size() << " bytes) - nothing has been restored or"
			             " edited yet (E59)" << std::endl;
			return ReturnCode::Ok;
		}

		// E59: THE line this whole trace exists for.
		//
		// This runs when the HOST asks for state (Controller_VST3::getState).
		// A host that asks BEFORE it restores -- to snapshot the "before" for
		// its own undo, which is ordinary host behaviour -- gets whatever
		// TideApp holds at that moment, and on a fresh controller that is the
		// DEFAULT rack. Those bytes then go to the chunk parameter, are
		// retained by the processor holder, and are re-seeded into the next
		// processor instance it starts. The editor is never wrong; the DSP is.
		//
		// So the size printed here, compared against the restore below and
		// against the instance lines from the processor, is what separates
		// "the editor pushed over the restore" from "a save-time refresh of a
		// not-yet-restored document poisoned the retained bytes".
		std::cerr << "TIDE: controller #" << controllerSeq
		          << " syncState exporting " << xml.size()
		          << " byte document (host asked for state)" << std::endl;

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
	ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, [[maybe_unused]] int32_t voice, int32_t size, const uint8_t* data) override
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

		// E59: the restore, with its size and its verdict. Paired with the
		// syncState line above, the ORDER of the two is the finding.
		std::cerr << "TIDE: controller #" << controllerSeq
		          << " restore of a " << docSize << " byte document -> "
		          << (imported ? "imported" : "REJECTED") << std::endl;

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
