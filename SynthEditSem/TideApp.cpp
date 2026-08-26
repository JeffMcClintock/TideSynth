#include <cassert>
#include <string_view>                  // S11 - importChunkXml
#include <fstream>                      // V6 - loadDefaultDocument
#include "tinyxml/tinyxml.h"            // S12 - exportChunkXml
#include "SerializationHelper_XML.h"    // S12 - SAT_VST3 / EXP_PLUGIN
#include "DocOb.h"                      // S12 - CDocOb::exportFlags
#include <cstdio>
#include "TideApp.h"
#include <cstdio>
#include "SynthEditDoc2.h"
#include "SynthEditDocBase.h"
#include "Module_Info3_internal.h" // merge-only ControlsXp.xml enrichment — U2e
#include "modules/tinyXml2/tinyxml2.h"
#include "conversion.h"
#include "BundleInfo.h"
#include "se_filesystem.h"           // se_fs — E2a prefab enumeration
#include "it_doc_ob.h"               // V3 — walking the master container
#include <set>                       // V3 — the handle snapshot
#include <algorithm>                  // std::replace — E2a
#include <system_error>               // std::error_code — E2a
#include "ContainerViewStruct.h"
#include "ContainerView.h" // SE2::ContainerViewPanel — U1a
#include "Dialogs_editor.h"
#include "MfcDocPresenter.h"
#include "GmpiResourceManager.h" // resourceFolders seeding — U2e
#include "SkinMgr.h"
#include "notify.h" // Notifiable / Notifier — required transitively by ModuleBrowser.h
#include "ModuleBrowser.h"
#include "PropertiesBrowser.h"
#include <cstdarg>                   // tideDiag's varargs -- BACKLOG M6
#if defined(__APPLE__)
#include <os/log.h>                  // the one diagnostic channel an appex can reach -- BACKLOG M6
#endif

#if TIDE_VCV_FUNDAMENTAL || TIDE_VCV_HETRICKCV
#include "RackFactory.h" // rack_adaptor::registerDeferredModules — the ported Rack modules
#endif

// TIDE ships none of SynthEdit's editor dialogs (PLAN constraint 5), and two of
// the three below have live filesystem writes behind them, which constraint 3
// forbids outright. These definitions have to exist regardless, because
// SynthEditLib calls them unconditionally -- CUG.cpp:2635 (Connect...) and
// CUG_with_patches.cpp:164 (patch manager). So the only thing left to choose is
// how they fail.
//
// BACKLOG S3: `assert(false)` alone was not a choice, it was an accident.
// NDEBUG compiles it out, so every shipping build reached one of these and
// returned as though the dialog had been shown and cancelled -- the user picks
// a menu item and nothing happens, with nothing anywhere saying why. Report on
// every build instead, release included.
//
// Why a stderr breadcrumb and not something louder: each louder option breaks a
// rule TIDE already holds.
//
//   abort()/std::terminate()  kills the host DAW. Strictly worse than the
//                             silent no-op it would replace, and the exact
//                             failure P4 was fixed to stop.
//   a message box             is a modal dialog (constraint 5) and needs a
//                             parent window TIDE may not have under AUv3.
//                             TideAppStubs.cpp already stubs SafeMessagebox
//                             to nothing for that reason.
//   a log file                is a write outside the plugin bundle
//                             (constraints 3 and 4) -- the very thing the
//                             sandbox audit filed these under.
//
// stderr is what is left, and it is already this project's answer to exactly
// this question: SynthEditLib/modules/se_sdk3_hosting/ModuleView.cpp:684 reports
// an unregistered module type the same way, with a comment saying "Loud in
// Release on purpose (TideSynth BACKLOG U2d) ... stderr, not a dialog". It also
// costs nothing when nobody is listening. The assert stays so a debug build
// still stops at the call site.
//
// This closes S3's TIDE-side half only. The menu entries that reach these are
// built in SynthEditLib/MfcDocPresenter.cpp, which is GATED -- see BACKLOG S3g.
static void tideRemovedDialog(const char* what)
{
	std::fprintf(stderr,
		"TIDE: '%s' was invoked, but TIDE ships no such dialog (PLAN constraint 5, BACKLOG S3).\n"
		"      Nothing was done. The menu entry that reached this is SynthEdit's, not TIDE's.\n",
		what);
	std::fflush(stderr);
	assert(false);
}

// BACKLOG M6 -- the same breadcrumbs, mirrored somewhere a SANDBOXED host can
// actually read them.
//
// Every diagnostic below goes to stderr, which is the right answer for the
// standalone and worth nothing for an AUv3: an app extension's stderr reaches
// neither the terminal nor the unified log, and its /tmp is not /tmp (both
// measured in M5). So the plugin could report "no Prefabs folder in bundle
// resources - the rack module browser will be empty" on every single
// instantiation and nothing outside the process would ever see it. That is
// precisely how an EMPTY rack -- no control pins, no prefabs, no MIDI jacks --
// shipped from 2026-08-23 to 2026-08-25 while `auval` returned exit 0 and
// AU VALIDATION SUCCEEDED on it. `auval` validates the AU *interface* and
// never asks whether the plugin contains anything.
//
// os_log is the one channel that crosses that boundary, and it is a better
// answer than the log file the block above rules out, for the same reasons:
// it is the platform's own facility rather than a file the plugin creates, so
// it writes nothing to the user's disk and needs no sandbox exception.
// Constraints 3 and 4 are untouched. It also costs nothing when nobody is
// subscribed, which is every ordinary run.
//
// MIRRORED, NOT REPLACED. stderr stays the primary channel, so the standalone,
// the CI logs and every existing habit are unchanged; this only adds a second
// copy on Apple platforms. scripts/check-rack-populated.py subscribes to the
// subsystem below and asserts the rack came up populated.
namespace
{
	// Keep in step with scripts/check-rack-populated.py, which greps the
	// unified log for exactly this string.
	constexpr const char* kTideDiagSubsystem = "com.tidesynth.tiderack";
}

static void tideDiag(const char* format, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, format);
	const int written = std::vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (written < 0)
		return;

	std::fputs(buffer, stderr);
	std::fflush(stderr);

#if defined(__APPLE__)
	static os_log_t tideLog = os_log_create(kTideDiagSubsystem, "startup");

	// Trim the trailing newline: stderr wants it, the unified log adds its own
	// record boundary and would otherwise show a blank line per entry.
	const size_t length = (written < (int)sizeof(buffer)) ? (size_t)written : sizeof(buffer) - 1;
	if (length > 0 && buffer[length - 1] == '\n')
		buffer[length - 1] = '\0';

	// %{public}s because os_log REDACTS dynamic strings by default -- a
	// diagnostic that logs as "<private>" is not a diagnostic. Nothing here is
	// user data: these are resource names, counts and pin indices.
	os_log(tideLog, "%{public}s", buffer);
#endif
}

void doDialogConnectUg(class CUG* cug)
{
	tideRemovedDialog("Connect...");
}

void doDialogPatchManager(CUG_with_patches* ug)
{
	tideRemovedDialog("Patch Manager");
}

// BACKLOG S3g. TIDE has none of the module-editor dialogs, so EditorLib must
// not offer the menu entries that reach them. The stubs above stay as the
// backstop for any path that still gets there: "Copy Patch" may become
// useful in TIDE later, and "Connect..." is a SynthEdit-only future.
bool AppHasModuleEditorDialogs()
{
	return false;
}

// doDialogBuildCodeSkeleton is deliberately NOT defined here any more. It was
// declared by no header and called by nothing -- in this repo, in SynthEditLib,
// or anywhere else -- so the stub was dead weight rather than a guard. The live
// "Build Code Skeleton..." path does not pass through it at all: it runs
// MfcDocPresenter.cpp:1276 -> POPUP_MENU_DEBUG_CODE -> CUG.cpp:2034's
// VO_Notify(OM_SHOW_CODE_SKELETON_DIALOG), whose only handler is the WinUI3
// app's MainWindow.xaml.cpp:762. TIDE registers no handler, so the notification
// is dropped and CUG::BuildSkeletonCode -- the create_directory/copy_file code
// the sandbox audit's finding A6 names -- is unreachable here, though still
// linked. Making that menu entry absent is the GATED half, BACKLOG S3g.

ISeApp* CreateApp()
{
	return new TideApp();
}

TideApp::TideApp()
	: moduleDragAndDrop(this)
{
}

SE2::TopView* TideApp::OpenView(gmpi::api::IUnknown* host)
{
	return OpenViewForContainer(host, nullptr);
}

// U1b — double-click on a Container in the structure view lands here:
// MfcDocPresenter (PresenterCommand::Open) -> CContainer::OnMenuCommand ->
// Document()->OpenView -> this override. The base implementation calls
// m_app_user_interface->OpenView, which TIDE never sets (null deref); route
// to the GUI's navigation callback instead. The GUI rebuilds the editor view
// for the target container on its next UI tick and the breadcrumb follows
// via onViewOpened.
void TideApp::OpenView(CContainer* p_object, int view_flag)
{
	if (onOpenContainerView)
		onOpenContainerView(p_object, view_flag);
}

// BACKLOG V5. The rack canvas, and it is square.
//
// Was 7968, which is 20.75 rows tall and 525 HP wide -- about 11x taller and
// 7.7x wider than VCV Rack shows by default, which is why it looked big.
//
// A TIDE DIP is very nearly a VCV pixel, so the two are directly comparable:
// E5 ruled row 384 and standard width 48, and Eurorack's 3U = 128.5 mm with
// 1 HP = 5.08 mm puts TIDE at 15.2 DIP per HP against VCV's RACK_GRID_WIDTH of
// 15, and 384 per row against RACK_GRID_HEIGHT 380 -- a ratio of 1.011.
// VCV's default window is 1024x720 (settings.cpp), i.e. 68.3 HP by 1.89 rows.
//
// 1008 is the nearest value to that 1024 which keeps the grid divisibility the
// old comment recorded (60-DIP grid plus two 24-DIP borders, so N = 60k + 48):
// 66.4 HP wide by 2.62 rows, which brackets VCV rather than dwarfing it.
// 1248 would be the alternative if the target were a physical 84 HP case.
static constexpr int kRackViewDips = 1008; // 60*16 + 48

SE2::TopView* TideApp::OpenViewForContainer(gmpi::api::IUnknown* host, CContainer* container, int requested_view_flag)
{

	// BACKLOG U1b (second half) — constraint 1's two depths, routed per
	// container. The MASTER container is the rack: panel view, the product's
	// face (U1a's pivot, workable since U2d/U2e made panel modules and
	// controls draw). Any OTHER container opens as its STRUCTURE view —
	// double-clicking a Container is the unlock gesture for now; if unlock
	// grows a confirm/lock UI later it changes the gesture, not this
	// routing. The struct-view interim default (2026-08-16, while panel
	// controls were blocked) ended here.
	auto* targetContainer = container ? container : Document()->MasterContainer;
	const bool isRackLevel = (targetContainer == Document()->MasterContainer);
	const int view_flag = requested_view_flag
		? requested_view_flag
		: (isRackLevel ? CF_PANEL_VIEW : CF_STRUCTURE_VIEW);

	// BACKLOG V4. The module browser is app-level and cannot see which view is
	// active, so tell it. In the rack it offers rack modules only -- the prefabs
	// plus anything categorised "Rack..."; drilling into the structure view
	// offers everything, which is where the TiDE parts (Panel, knob, patch
	// points) belong, since those build a rack module rather than being one.
	// Keyed off view_flag, not isRackLevel: "Goto Structure..." on the master
	// shows the rack's own structure, so isRackLevel is still true there.
	setBrowserScope(view_flag == CF_PANEL_VIEW ? ModuleScope::RackOnly : ModuleScope::Everything);

	const gmpi::drawing::Size viewSize{ static_cast<float>(kRackViewDips), static_cast<float>(kRackViewDips) };
	SE2::TopView* viewOb{};
	if (view_flag == CF_PANEL_VIEW)
		viewOb = new SE2::ContainerViewPanel(viewSize);
	else
		viewOb = new SE2::ContainerViewStruct(viewSize);
	view = viewOb;

	// Attach the host *before* setDocument — Init triggers an immediate
	// Refresh/measure pass on children, and ModuleViewStruct::measure needs
	// drawingHost for getFactory().
	if (host)
		viewOb->setHost(host);

	// TopView::onMouseWheel uses the presence of these callbacks as a flag
	// for "do I have scrollbars to coordinate with?". Without them set, it
	// short-circuits to ViewBase::onMouseWheel and Ctrl-wheel zoom /
	// shift-wheel horizontal scroll / plain wheel vertical scroll never
	// fire. TIDE doesn't show real scrollbars, so the lambdas are no-ops —
	// they just satisfy the guard so the view's own wheel logic runs. (If
	// we ever add visible scrollbars to the editor strip, populate them
	// from the spec here.)
	viewOb->hscrollBar = [](const SE2::scrollBarSpec&) {};
	viewOb->vscrollBar = [](const SE2::scrollBarSpec&) {};

	auto presenter = new MfcDocPresenter(targetContainer, view_flag);

#ifdef _DEBUG
	if (auto mod = CreateDocObject(L"VCA"); mod)
	{
		// position the debug module in the view we now render.
		mod->offsetViewObRect(view_flag, 24, 0);
		Document()->MasterContainer->addToContainer(mod);
	}
#endif

	viewOb->setDocument(presenter);

	// BACKLOG U2c — TopView::centerPos defaults to {0,0}, which parks the
	// viewport on the canvas's top-left corner and sends every
	// insert-at-centre (AddModule(id, getCenter())) to that same corner:
	// the "§6 offset / dead strip" on both platforms, and Windows'
	// "every module lands on the same point". Centre on the canvas midpoint
	// until U1c defines what "home" means for a rack. After setDocument so
	// the first arrange has run and calcViewTransform sees real bounds.
	viewOb->setCenter({ kRackViewDips / 2.0f, kRackViewDips / 2.0f });

	// U1b — keep the breadcrumb trail current for every open, including
	// navigation re-opens.
	if (onViewOpened)
		onViewOpened(targetContainer, view_flag);

	return viewOb; // refcount = 1; caller (SynthEditGui) takes ownership
}

// The saved chunk. Two sections under one <Document>, because the two consumers
// need different serialisations of the same rack:
//
//   <DSP>    - S12. The exporter's "DSP STRUCTURE XML" block (ExportAsPlugin.cpp)
//              run at runtime against the live document. This is what the
//              processor's blob pin feeds to SynthRuntime::setDocumentXml, and
//              SeAudioMaster::BuildDspGraph navigates Document->DSP explicitly,
//              so it neither sees nor minds the sibling below.
//   <Editor> - S11. The editor's OWN document format, which is a different
//              serialisation entirely: <Modules> (capital) with no positions on
//              the DSP side, <modules> under <master_container> with the full
//              object model on this side. The DSP text cannot rebuild the
//              editor's document - it has no layout in it at all - so restoring
//              the rack needs this section, and saving only <DSP> is why the
//              rack could never have come back however well the blob survived.
//
// No Scramble - the text goes straight to the in-process runtime and into host
// state, not into a shipped bundle.
std::string TideApp::exportChunkXml()
{
	// NOT EXP_PLUGIN: that flag makes doExport() drop every module marked
	// excludeFromVst - which includes Sound Out and the Diagnostic modules -
	// because a BAKED plugin export replaces them. TIDE self-hosts the
	// document with editor semantics, so nothing may be pruned.
	CDocOb::exportFlags = 0;
	if (auto* containerInfo = ModuleFactory()->GetById(L"Container"))
		containerInfo->ClearSerialiseFlag(); // needs serialise for json GUI but not for XML DSP (per the exporter)

	TiXmlDocument doc2;
	doc2.LinkEndChild(new TiXmlDeclaration("1.0", "", ""));

	auto* documentElement = new TiXmlElement("Document");
	doc2.LinkEndChild(documentElement);

	auto* dspElement = new TiXmlElement("DSP");
	documentElement->LinkEndChild(dspElement);

	// SAT_SYNTHEDIT_DSP, not SAT_VST3: the editor's own runtime format. With
	// any other target, ExportXml_Pt2 on a top-level container serialises ONLY
	// its first child container and disregards loose modules by design (the
	// baked-plugin path replaces them). TIDE's flat rack needs every module.
	Document()->MasterContainer->ExportXml(dspElement, SAT_SYNTHEDIT_DSP);

	// SetupVstIO expects the exporter's shape: a main container whose child is
	// the "synth" container (it takes the patch manager from the child and
	// bridges the child's audio/MIDI plugs to the synthetic VST IO modules).
	// TIDE's document is flat - the rack IS the master container - so wrap the
	// exported master in a synthetic outer container to restore that shape.
	{
		TiXmlElement* masterE = dspElement->FirstChildElement("Module");
		if (masterE)
		{
			auto* masterCopy = masterE->Clone();

			auto* outer = new TiXmlElement("Module");
			outer->SetAttribute("Id", "1");
			outer->SetAttribute("Type", "Container");
			outer->LinkEndChild(new TiXmlElement("Plugs"));
			auto* mods = new TiXmlElement("Modules");
			outer->LinkEndChild(mods);
			mods->LinkEndChild(masterCopy);
			outer->LinkEndChild(new TiXmlElement("Lines"));

			// The patch manager belongs to the MAIN container (BuildPatchManager
			// runs on it unconditionally); the inner container then inherits it,
			// which is ordinary SE containment. Move it up.
			if (auto* innerE = masterCopy->ToElement())
			{
				if (auto* pm = innerE->FirstChildElement("PatchManager"))
				{
					outer->LinkEndChild(pm->Clone());
					innerE->RemoveChild(pm);
				}
			}

			dspElement->Clear();
			dspElement->LinkEndChild(outer);
		}
	}

	// <Editor> - the half the editor can actually read back. Mirrors
	// CSynthEditDocBase::ExportXmlProject, minus two things it does that a
	// continuous sync must not:
	//
	//  - preSaveState() / RemoveOrphanedHostControls() MUTATE the document, and
	//    this runs on every serviceDocumentSync tick, not on a File>Save.
	//  - ExportModuleInfo records what is known about UNAVAILABLE modules so a
	//    document referencing a missing one still round-trips. TIDE's module set
	//    is fixed and compiled in (PLAN constraint 7), so every module in a TIDE
	//    document is available by construction and there is nothing to record.
	//
	// The window layout is skipped for the same reason: constraint 1 leaves TIDE
	// no window arrangement to preserve.
	if (auto* master = Document()->MasterContainer; master)
	{
		// The editor serialisers are tinyxml2 while the DSP block above is
		// tinyxml1, and the two libraries share no node types. Build the section
		// in tinyxml2, print it, and re-parse it with tinyxml1 so it can be
		// linked in. A serialise/re-parse per tick is not free, but it keeps one
		// chunk with one root rather than two parameters to keep in step.
		tinyxml2::XMLDocument editorDoc;
		auto* editorRoot = editorDoc.NewElement("Editor");
		editorDoc.LinkEndChild(editorRoot);

		Document()->ExportXml(editorRoot, SAT_SYNTHEDIT_DOCUMENT);

		ModuleFactory()->ClearSerialiseFlags();

		auto* masterContainerE = editorDoc.NewElement("master_container");
		editorRoot->LinkEndChild(masterContainerE);
		master->Export(masterContainerE, SAT_SYNTHEDIT_DOCUMENT);

		tinyxml2::XMLPrinter p;
		editorDoc.Print(&p);

		TiXmlDocument bridge;
		bridge.Parse(p.CStr());
		if (!bridge.Error())
		{
			if (auto* parsed = bridge.RootElement(); parsed)
				documentElement->LinkEndChild(parsed->Clone());
		}
	}

	TiXmlPrinter printer;
	printer.SetIndent("  ");
	doc2.Accept(&printer);

	return printer.CStr();
}

// S11 - the other direction. Rebuilds the editor's document from a chunk the
// DAW restored, replacing whatever InitInstance's blank one left standing.
//
// Returns false and changes nothing when it cannot do the job, which is the
// whole point: an unusable, truncated or foreign document must leave an empty
// rack, never take the host down (S11 step 1). The caller is on the MAIN
// thread, inside the wrapper's state-restore guard.
bool TideApp::importChunkXml(std::string_view xml)
{
	if (xml.empty() || !Document())
		return false;

	tinyxml2::XMLDocument doc;
	if (tinyxml2::XML_SUCCESS != doc.Parse(xml.data(), xml.size()))
		return false;

	auto* documentE = doc.FirstChildElement("Document");
	if (!documentE)
		return false;

	// A saved CHUNK nests the editor tree in <Editor>; a document SAVED BY
	// SYNTHEDIT (what V6's DefaultRack.synthedit is) puts <master_container>
	// directly under <Document>. Accept both, and pick by structure rather
	// than by which file it came from -- the importer wants the element that
	// HOLDS the master container, and that is the only difference.
	//
	// The DSP-only case this used to reject still is: a chunk saved before
	// <Editor> existed has neither an <Editor> nor a <master_container> child,
	// so it fails the second test exactly as it used to fail the first.
	auto* editorE = documentE->FirstChildElement("Editor");
	if (!editorE && documentE->FirstChildElement("master_container"))
		editorE = documentE;
	if (!editorE)
		return false; // a DSP-only chunk: saved before <Editor> existed

	// Replacing the document tears the master container down and frees every
	// object in it. TopView holds those, and SynthEditGui owns the TopView - so
	// doing this under a live view would leave the editor drawing freed memory.
	// The host restores state between initialize() and createView(), so the
	// normal path has no view yet; refuse rather than gamble on the exception.
	if (view)
		return false;

	Document()->DeleteContents();

	const int fileFormatVersion = Document()->ImportXml(editorE, SAT_SYNTHEDIT_DOCUMENT);
	CDocOb::m_loading_version = fileFormatVersion;
	Document()->SetLoadingVersion(fileFormatVersion);

	Document()->ImportModules(editorE, SAT_SYNTHEDIT_DOCUMENT);

	if (!Document()->MasterContainer)
		return false; // import produced nothing usable; caller leaves the rack empty

	// TIDE *is* the rack (PLAN constraint 1 / U1c), and rackMode is a document
	// field, so a freshly imported document needs it set exactly as
	// InitInstance sets it on a blank one.
	Document()->rackMode = true;

	// Baseline the sync against what was JUST IMPORTED rather than forcing a
	// push: the wrapper re-seeds the chunk parameter into the processor when
	// it starts (SynthEdit.cpp:70's comment walks the mechanism), so the
	// processor builds this same document on its own and a GUI push here was
	// a guaranteed duplicate rebuild. If the round-trip ever produces a
	// different document, the next 500ms tick pushes the difference anyway.
	lastPushedShape = documentShape(exportChunkXml());

	return true;
}

// The document reduced to its SHAPE: modules and wiring, with every
// <patch-list> removed. That is where parameter VALUES live -- knobs,
// buttons, and the patch-cable list host control alike -- and none of them
// may cost the processor a restart. See serviceDocumentSync.
std::string TideApp::documentShape(const std::string& doc)
{
	std::string out;
	out.reserve(doc.size());

	size_t at = 0;
	for (;;)
	{
		const auto open = doc.find("<patch-list>", at);
		if (open == std::string::npos)
		{
			out.append(doc, at, std::string::npos);
			break;
		}

		out.append(doc, at, open - at);

		const auto close = doc.find("</patch-list>", open);
		if (close == std::string::npos)
			break; // malformed; the prefix is enough to compare on

		at = close + 13; // strlen("</patch-list>")
	}

	return out;
}

void TideApp::serviceDocumentSync()
{
	if (!onPushChunk || !Document() || !Document()->MasterContainer)
		return;

	// DEBOUNCE, exactly as the SynthEdit app does it (Jeff's steer).
	//
	// Editing sets a flag; the timer serialises once. Delete twenty modules
	// and invalidateDsp() fires twenty times, costing twenty bools, and this
	// tick builds ONE document. CSynthEditAppBase::OnTimer does the same with
	// the same flag (SynthEditAppBase.cpp:891) before its own soft restart --
	// TIDE differs only in where the document goes afterwards, because its
	// processor is a separate object rather than an in-process engine.
	//
	// dspDirty is the RIGHT signal and it is already maintained for us: an
	// RAII SuspendDSP guard sets it at 23 sites - adding and deleting modules,
	// re-cabling, container surgery - and TideApp inherits both the flag and
	// invalidateDsp() from CSynthEditAppBase. Nothing about a knob turn
	// touches it, which is the whole point: values are messages now.
	//
	// WHY NOT just export and compare, which is what this used to do: it
	// serialised the WHOLE document twice a second forever to ask whether
	// anything had changed, and the answer was almost always no. Measured
	// 2026-08-25 on a three-module rack: 32.5KB and 1.87ms per export, 40
	// exports in 20 idle seconds, 39 of them discarded. One more module took
	// it to 38.2KB and 2.4ms, so it grows with the rack and is paid forever.
	// Jeff, on the numbers, on a rack he rightly called tiny: 'holy fuck that
	// was expensive.'
	if (!dspDirty)
		return;

	dspDirty = false;

	auto xml = exportChunkXml();

	// THE DOCUMENT GOES TO THE PROCESSOR ONLY WHEN ITS SHAPE CHANGES.
	//
	// Jeff's ruling, 2026-08-25, and the three cases it separates:
	//
	//   a module or prefab is added or deleted  the graph is genuinely
	//                                           different: send the document,
	//                                           the processor restarts, and
	//                                           that cost is unavoidable.
	//   a patch cable is moved                  send NOTHING here. The cable
	//                                           list is the HC_PATCH_CABLES
	//                                           host control, its value rides
	//                                           the message queue like any
	//                                           other parameter, and the DSP
	//                                           side turns it into a graph
	//                                           rebuild from the document it
	//                                           ALREADY HAS (requiresAsyncRestart
	//                                           -> DoAsyncRestart). Shipping a
	//                                           second copy of the document to
	//                                           say "a wire moved" is waste.
	//   a knob or button moves                  send NOTHING here either; the
	//                                           value is a message.
	//
	// So the comparison ignores every <patch-list>: that is where parameter
	// VALUES live, the cable list included. What remains is modules and their
	// wiring - the shape.
	//
	// This decision belongs HERE, on the GUI thread at 500ms, and emphatically
	// not in the processor. It was briefly done there and that was a mistake:
	// onSetPins runs on the AUDIO THREAD, where building and comparing a 12KB
	// string per arriving chunk is exactly the sort of work that has no
	// business next to a real-time deadline.

	auto shape = documentShape(xml);
	if (shape == lastPushedShape)
		return;

	lastPushedShape = std::move(shape);
	// Rare by construction now - only a structural edit gets here - and worth
	// saying out loud, because it is the expensive one.
	std::fprintf(stderr, "TIDE: DSP structure changed, pushing %zu byte document\n", xml.size());
	onPushChunk(xml.data(), xml.size());
}

// The chunk's return half. Bytes in, nothing decoded here: synthRuntime owns
// the queue and hands each framed message to CSynthEditAppBase::
// onQueMessageReady, which routes it by handle to the PatchParameter that
// sent it -- the same path an in-editor processor would have taken. That is
// the whole point of forwarding the framing intact rather than translating it.
//
// What this makes work, and did not work at all before: every DSP->GUI
// parameter update in TIDE. Output parameters, meters, Scope captures, and a
// module's lights (the VCV Fundamental ports' LEDs are what exposed it -- the
// processor queued 1054 light updates in 12 s and the editor received none,
// measured 2026-08-25).
void TideApp::receiveRackFeedback(const unsigned char* data, int size)
{
	synthRuntime.receiveDspMessages(data, size);
}

std::string TideApp::exportChunkXmlForSave()
{
	if (Document() && Document()->MasterContainer)
	{
		// Through the CDocOb base, where preSaveState is public - CContainer
		// re-declares its override protected, and TIDE is not on its friend
		// list. The base declaration is the interface the save path uses.
		static_cast<CDocOb*>(Document()->MasterContainer)->preSaveState();

		// Catch-all cull of orphaned host controls, exactly as the app's own
		// save does. If it removes anything the document genuinely changed,
		// and the next sync tick will push a Build - correct, and rare.
		Document()->MasterContainer->RemoveOrphanedHostControls();
	}

	return exportChunkXml();
}

bool TideApp::takeDspMessages(std::vector<unsigned char>& out)
{
	return synthRuntime.takeUiToDspMessages(out);
}

// See the header. Unconditional, unlike the base: "is the editor's own
// processor running" is the wrong question in TIDE, where the answer is always
// no and the processor is elsewhere.
gmpi::hosting::QueuedUsers* TideApp::PendingDspClients()
{
	return &synthRuntime.pendingProcessorQueueClients;
}

bool TideApp::setQuiet(bool newValue)
{
	const bool previous = quiet;
	quiet = newValue;
	return previous;
}

ModuleBrowser* TideApp::OpenModuleBrowser(gmpi::api::IUnknown* host)
{
	auto* mb = new ModuleBrowser(this); // 'this' is CSynthEditAppBase
	if (host)
		mb->setHost(host);
	mb->Init(); // registers viewModel as observer of the app
	return mb;  // refcount = 1; caller (SynthEditGui) takes ownership
}

PropertiesBrowser* TideApp::OpenPropertiesBrowser(gmpi::api::IUnknown* host)
{
	auto* pb = new PropertiesBrowser(this);
	if (host)
		pb->setHost(host);
	pb->Init();
	return pb;
}

void TideApp::OnCloseView(SE2::TopView*)
{
	view = {};
}

void TideApp::CloseAllViews()
{
	view = {};
}

bool TideApp::InitInstance()
{
	// No module scan, no module cache. Every module TIDE ships is statically
	// registered before this runs -- CModuleFactory's constructor force-links
	// the built-ins (SynthEditLib/UgDatabase.cpp, initialise_synthedit_modules)
	// -- and the module browser reads that in-memory list, never the disk.
	// The scan that used to run here (semFolder + LoadOrScanModuleData) exists
	// to find *third-party* modules, which PLAN.md constraint 7 rules out, and
	// its cache file was a write outside the plugin's sandbox, which
	// constraint 4 rules out. Verified empirically 2026-08-07: with these
	// lines gone and no cache file present, the browser is fully populated.
	// See TideSynth docs/module-enumeration.md (stage 1) and BACKLOG S1a.

	CommandManager()->m_enable_undo = false;

	// BACKLOG U2e — the base CSynthEditAppBase::InitInstance seeds
	// GmpiResourceManager's resourceFolders (skin images among them); this
	// override replaced the base wholesale for S1a and dropped that seeding,
	// so every skin-image URI resolved against an EMPTY map:
	// BitmapWidget::Load failed, the classic controls' widget layer never
	// built, and every later widgets[] access crashed (U2e's three stacks).
	// Seed the one type TIDE's panel controls actually read. Reading the
	// user skins folder matches the editor's current behaviour; moving TIDE
	// to bundle-internal resources (sandbox-safe, constraint 4) is U2e/U1c
	// follow-up, not this line's job.
	GmpiResourceManager::Instance()->resourceFolders[GmpiResourceType::Image] =
		SkinMgr::Instance()->SkinFolder();

	// BACKLOG U2e — the classic controls' pin DESCRIPTIONS. Full SynthEdit gets
	// these from the module scan; TIDE's scan is gone by design (S1a), so the
	// statically-registered classes had EMPTY gui_plugs: ViewBase::ConnectModules
	// then has no pin list to send defaults through, the SDK3 setPin+notifyPin
	// pass never happens, the pin-update handlers never fire, and the classic
	// controls never build their widgets (U2e's guarded crash trio).
	//
	// A MODULE NEEDS BOTH HALVES IN TIDE, and this is the half that is easy to
	// forget because the other one is invisible from here:
	//
	//   .cpp  the class and its registration -- listed explicitly in
	//         SynthEditSem/CMakeLists.txt's source list (constraint 7's fixed set)
	//   .xml  the pin descriptions -- merged here
	//
	// With only the XML you get an "insertable phantom": a browser entry with no
	// class behind it. With only the .cpp you get a class with no pins, which
	// takes the whole enclosing container's widget layer down with it -- a blank
	// rack rather than one missing module. E2a hit the second failure while
	// giving the prefabs a faceplate.
	//
	// SubControlsXp.xml left the list with E15: the faceplate is SE TiDE:Panel
	// now, which registers withXml and needs no merged pin descriptions.
	// automatically -- the GetById() guard below only enriches classes that are
	// actually registered, which is exactly what keeps this safe to point at a
	// file describing far more modules than TIDE links.
	// MidiPlayer2.xml joined for BACKLOG V3/E7 -- SE MIDItoGate2, the monophonic
	// stand-in for MIDI-CV 2 while a polyphonic source cannot drive other rack
	// modules through a patch cable. Same enrichment-only deal as the other two.
	// VaFilters.xml joined for BACKLOG E2c -- SE SV Filter4 ("StateVar Filter"),
	// one of TIDE's two go-to MVP modules. Same enrichment-only deal: that file
	// describes Korg/Moog/RMS and more, and the GetById() guard below skips every
	// one whose .cpp is not in the source list.
	//
	// ITS SIBLING NEEDS NOTHING HERE. SE Oscillator4 ("Oscillator HD") registers
	// with Register<>::withXml(R"XML(...)") inline in Oscillator.cpp, so its pins
	// arrive with the class and there is no file to merge.
	//
	// THIS LIST AND THE ONE IN CMakeLists.txt (_tide_xmls) MUST MOVE TOGETHER.
	// CMake stages the file into Contents/Resources; this loop is what READS it.
	// Adding it in only one place fails silently in whichever direction you
	// missed -- staged but never read, or read but never staged (the second says
	// so on stderr, the first says nothing at all).
	// ONCE PER PROCESS, not once per instance. This merge writes into the
	// PROCESS-GLOBAL module database (Module_Info), and a standalone app made
	// that distinction invisible: InitInstance ran exactly once, so nothing here
	// needed a guard. A plug-in host creates SEVERAL instances in ONE process --
	// an AUv3 extension process is shared across instantiations -- so the second
	// instance re-scanned every module and tripped RegisterParameters's
	// assert("Already scanned parameters"), aborting the extension process.
	// auval could not open the AU at all; it can now.
#if TIDE_VCV_FUNDAMENTAL || TIDE_VCV_HETRICKCV
	// The ported Rack modules — VCV Fundamental (GPL) and/or HetrickCV (CC0),
	// each behind its own root-CMakeLists option, both OFF by default. Their createModel() lines QUEUED registrations during
	// static init; the XML generator cannot run there, because a module's
	// RACK_DISPLAY_STATE is declared after upstream's .cpp registers. By now
	// static init is long over, so flushing builds every module's XML and
	// registers it through SynthEditLib's gmpi::RegisterPluginWithXml — the
	// same ModuleFactory() path as every statically-registered module above.
	// Both halves arrive together (the XML carries the pins), so nothing
	// below in the enrichment loop needs to know these exist. Idempotent
	// inside the adaptor, for the same several-instances-one-process reason
	// as s_xmlMerged.
	//
	// ONE flush drains the whole queue, whichever packs are linked, so the
	// count is reported against the set that is actually in this binary
	// rather than per pack -- the adaptor's queue does not record which repo
	// a registration came from, and inventing a split here would be a guess.
	{
		const int registered = rack_adaptor::registerDeferredModules();
#if TIDE_VCV_FUNDAMENTAL && TIDE_VCV_HETRICKCV
		const char* which = "VCV Fundamental + HetrickCV";
#elif TIDE_VCV_FUNDAMENTAL
		const char* which = "VCV Fundamental";
#else
		const char* which = "HetrickCV";
#endif
		tideDiag("TIDE: %s — %d module(s) registered\n", which, registered);
	}
#endif

	static bool s_xmlMerged = false;
	if (!s_xmlMerged)
	{
		s_xmlMerged = true;

		for (const auto* resourceName : { "ControlsXp.xml", "MidiPlayer2.xml", "Converters.xml", "VaFilters.xml" })
		{
			const auto xml = BundleInfo::instance()->getResource(resourceName);
			if (xml.empty())
			{
				tideDiag("TIDE: %s missing from bundle resources - those controls will have no pins\n", resourceName);
				continue;
			}

			tinyxml2::XMLDocument doc;
			doc.Parse(xml.c_str());
			if (doc.Error())
				continue;

			tinyxml2::XMLNode* root = doc.FirstChildElement("PluginList");
			if (!root)
				root = &doc;
			// The count is REPORTED, not merely computed, and that is the point.
			// "Enriched" means the class was registered AND its pins arrived; the
			// difference between the two numbers is the file's entries TIDE does not
			// link, which is expected and harmless. What is NOT harmless is a zero:
			// it means every class this file describes is missing from the source
			// list, i.e. the .cpp half was forgotten -- the exact half of the
			// both-halves rule above that nothing else here can see. That failure
			// used to be completely silent and cost a debugging cycle on V3.
			int enriched = 0, described = 0;
			for (auto e = root->FirstChildElement("Plugin"); e; e = e->NextSiblingElement("Plugin"))
			{
				++described;
				const auto id = JmUnicodeConversions::Utf8ToWstring(e->Attribute("id"));
				if (auto* mi3 = dynamic_cast<Module_Info3_internal*>(CModuleFactory::Instance()->GetById(id)); mi3)
				{
					mi3->ScanXml(e);
					++enriched;
				}
			}
			tideDiag("TIDE: %s enriched %d of %d described class(es)%s\n",
				resourceName, enriched, described,
				enriched == 0 ? " -- ZERO: is the .cpp in CMakeLists' source list?" : "");
		}
	}

	// BACKLOG E2a — the rack prefabs (oscillator, envelope, output). Stage 4 of
	// docs/module-enumeration.md §6, and the last stage that was never built.
	seedPrefabsFromBundle();

	// create a blank document
	createNewDocument();
	Document()->OnNewDocument(); // creates an empty main container

	// BACKLOG U1c — TIDE *is* the rack (PLAN constraint 1), so rack mode is
	// the product's normal state, not an opt-in. Everything behind this flag
	// already exists in the shared code and simply had no way to switch on in
	// a TIDE build: MfcDocPresenter::getRackLayout returns enabled only when
	// the document says rackMode AND the view is the top-level panel view (so
	// sub-panels and every structure view keep ordinary layout, which is what
	// U1b's second depth wants); ViewBase::snapToGrid then snaps drags and
	// resizes to one HP across and one rack row down instead of the square
	// snap; and TopView::renderRack draws the case interior with its rails and
	// per-HP mounting holes. In full SynthEdit the same flag is a per-project
	// setting toggled from a context menu that only exists in _DEBUG builds —
	// so a Release TIDE could not reach it at all.
	Document()->rackMode = true;

	// BACKLOG V3/V6 — every fresh document gets a wired MIDI-CV at the ROOT,
	// and since V6 it arrives as DATA rather than as C++.
	loadDefaultDocument();

	assert(Document()->MasterContainer);
	return true;
}

TideApp::~TideApp()
{
	if (document_)
		document_->DeleteContents();
}

std::string TideApp::getVendor4charCode()
{
	return "TIDE";
}


// ---------------------------------------------------------------------------
// BACKLOG E2a — rack prefabs, shipped inside the bundle.
//
// Prefabs are DATA, not code, which is the whole reason this is allowed while
// the module scan S1a deleted is not: constraint 3 forbids loading executable
// code from outside the bundle, and constraint 4 forbids writing outside the
// sandbox. Enumerating a read-only folder INSIDE our own bundle is neither.
// See TideSynth docs/module-enumeration.md §4.
// ---------------------------------------------------------------------------

namespace
{
	// The bundle sub-folder CMake stages RackModules/*.synthedit into. Capital
	// P to match SynthEditCL's staging target, which the comment there flags as
	// matching the code's expectation only by accident on case-insensitive
	// filesystems -- so spell it the same way rather than relying on that.
	const wchar_t* const kPrefabFolder = L"Prefabs";
}

void TideApp::seedPrefabsFromBundle()
{
	const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
	if (resourceFolder.empty())
		return;

	const se_fs::path prefabs = se_fs::path(resourceFolder) / kPrefabFolder;

	std::error_code ec;
	if (!se_fs::is_directory(prefabs, ec))
	{
		// Not fatal: a build with no prefabs staged is a working plugin with an
		// empty Prefabs group, and saying so beats an assert in a DAW. S13's
		// ruling -- an absent data file is not an assert -- applies here too.
		tideDiag("TIDE: no Prefabs folder in bundle resources - the rack module browser will be empty\n");
		return;
	}

	// PrefabFileNames holds paths RELATIVE to the prefab root: the browser
	// splits them into group + name on the separator (ModuleFactory_Editor.cpp
	// :2353), and LoadPrefab re-resolves them through ResolveFilename below.
	// Storing absolute paths here would put the whole path in the menu.
	size_t found = 0;
	for (se_fs::recursive_directory_iterator it(prefabs, ec), end; it != end && !ec; it.increment(ec))
	{
		if (!it->is_regular_file(ec))
			continue;

		const auto ext = it->path().extension().wstring();
		if (ext != L".synthedit" && ext != L".syntheditprefab")
			continue;

		auto relative = se_fs::relative(it->path(), prefabs, ec).wstring();
		if (ec)
			continue;

		// The browser splits on both separators but LoadPrefab concatenates, so
		// normalise here rather than relying on the platform's preference.
		std::replace(relative.begin(), relative.end(), L'\\', L'/');

		ModuleFactory()->PrefabFileNames.push_back(relative);
		++found;
	}

	if (found == 0)
		tideDiag("TIDE: Prefabs folder present but empty - check the POST_BUILD staging step\n");
	else
		tideDiag("TIDE: %zu rack prefab(s) seeded from the bundle\n", found);
}

std::wstring TideApp::ResolveFilename(const std::wstring& name, const std::wstring& extension)
{
	const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
	if (!resourceFolder.empty())
	{
		const se_fs::path candidate = se_fs::path(resourceFolder) / kPrefabFolder / name;

		std::error_code ec;
		if (se_fs::exists(candidate, ec))
			return candidate.wstring();
	}

	// Anything that is not one of ours falls through to the base, which is what
	// resolves samples and the like.
	return CSynthEditAppBase::ResolveFilename(name, extension);
}

// ---------------------------------------------------------------------------
// BACKLOG V6 — the root assembly is DATA now, not code.
//
// This replaces seedRootMidiCv(), which built the same graph from two
// AddModule calls, an AddPrefab and six AddConnectors. Jeff's observation is
// what closed it: "You say 'it can't be a prefab' then you tell me you build a
// container with a bunch of stuff in it, and insert it. That's the same as a
// prefab isn't it." It was — and AddPrefab's own comment, two lines below the
// workaround, already said a prefab may hold several top-level modules.
//
// WHAT THE DELETION BUYS. The old code hard-coded `facadePin = 7 + jack index`
// as an explicit contract with build-prefabs.py, and said in a comment that if
// the jack list ever changed this had to change with it. A stored document has
// no such coupling: the connections are data, so a jack added in SynthEdit
// travels with them.
//
// V3's reasoning survives the move and is worth keeping here, because the file
// this loads is the only thing now enforcing it: `SE MIDI to CV 2` is
// polyphonicSource/cloned, so whatever container holds it becomes a voice
// container, and polyphony cannot escape a container (E7). It must sit at the
// ROOT, with the rack module the user cables from being a FACADE of jacks fed
// inward. A document that nests it is silently wrong in a way that still looks
// right in the editor.
//
// IT LIVES OUTSIDE Prefabs/ ON PURPOSE. seedPrefabsFromBundle() scans
// Resources/Prefabs recursively into the module browser, so a default document
// shipped there would be user-insertable — and a SECOND root `SE MIDI to CV 2`
// breaks voice allocation (Jeff, 2026-08-26: "two midi-cvs break it"). Keeping
// this file a SIBLING of Prefabs/ rather than a member makes "there is exactly
// one" a property of where it sits, not of anybody remembering.
//
// An absent or unreadable file leaves the blank document standing, exactly as
// seedPrefabsFromBundle() leaves an empty browser: S13's ruling is that a
// missing data file is a diagnostic, not an assert.
// ---------------------------------------------------------------------------
bool TideApp::loadDefaultDocument()
{
	const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
	if (resourceFolder.empty())
		return false;

	const se_fs::path file = se_fs::path(resourceFolder) / L"DefaultRack.synthedit";

	std::error_code ec;
	if (!se_fs::is_regular_file(file, ec))
	{
		tideDiag("TIDE: no DefaultRack.synthedit in bundle resources - starting with an empty rack\n");
		return false;
	}

	std::ifstream in(file, std::ios::binary);
	if (!in)
	{
		tideDiag("TIDE: DefaultRack.synthedit present but unreadable - starting with an empty rack\n");
		return false;
	}

	const std::string xml{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };

	// The SAME importer a DAW-restored chunk uses, deliberately: a default
	// document IS a saved document, so there is only one path to keep correct.
	if (!importChunkXml(xml))
	{
		tideDiag("TIDE: DefaultRack.synthedit did not import - starting with an empty rack\n");
		return false;
	}

	tideDiag("TIDE: default rack loaded, %zu byte document\n", xml.size());
	return true;
}

// BACKLOG S7. TIDE ships its own look and must write NOTHING outside its own
// container (PLAN constraints 4 and 8). Answering false keeps SkinMgr out of
// <home>/SynthEdit Projects/ entirely -- no folder, no skins copy, and no
// .resource_version stamp to fight SynthEdit over.
bool AppUsesUserSkinsFolder()
{
	return false;
}
