#include "SynthEditAppBase.h"
#include "ModuleDragAndDropManager.h"
#include "TideAppWrapper.h"

// Inherits CSynthEditAppBase rather than the lighter ApplicationBase so that
// the editor-side helpers — ModuleBrowser, PropertiesBrowser,
// MfcDocPresenter, the OM_SHOW_PROPERTIES notification chain — see the same
// app type they get inside the full SynthEdit GUI. This pulls in the
// interThreadQueUser plumbing too; we don't drive any of it from TIDE, but
// the static-vtable cost is negligible for an in-plugin editor.
class TideApp : public CSynthEditAppBase, public ISeApp
{
	SE2::TopView* view{};
	std::string lastPushedDspXml; // S12 — dedupe for serviceDocumentSync

public:
	// `moduleDragAndDrop` mirrors SynthEditApp's setup so that ModuleBrowser
	// and MfcDocPresenter find a real drag-and-drop manager (they consult
	// `app->getModuleDragAndDropManager()` and silently no-op on nullptr).
	// Without this, clicking a module in the browser had no effect; with
	// it, the click fires OM_DRAG_NEW_MODULE and the next view click drops
	// the module.
	ModuleDragAndDropManager moduleDragAndDrop;

	TideApp();
	~TideApp();
	bool InitInstance() override;

	// ISeApp
	SE2::TopView* OpenView(gmpi::api::IUnknown* host) override;
	SE2::TopView* OpenViewForContainer(gmpi::api::IUnknown* host, CContainer* container, int view_flag = 0) override;
	void OpenView(CContainer* p_object, int view_flag) override; // CSynthEditAppBase — double-click enter (U1b)
	bool setQuiet(bool) override; // U1b follow-up — quiet the module factory during thumbnail renders
	void serviceDocumentSync() override; // S12 — push the document's chunk to the processor
	void receiveRackFeedback(const unsigned char* data, int size) override; // the return half
	std::string exportChunkXml();        // S12/S11 — the saved chunk: <DSP> + <Editor>
	bool importChunkXml(std::string_view xml); // S11 — rebuild the document from a saved chunk
	void OnCloseView(SE2::TopView*) override;
	void CloseAllViews() override;
	ModuleBrowser*     OpenModuleBrowser    (gmpi::api::IUnknown* host) override;
	PropertiesBrowser* OpenPropertiesBrowser(gmpi::api::IUnknown* host) override;

	// ApplicationBase — only override what differs from defaults
	ModuleDragAndDropManager* getModuleDragAndDropManager() override { return &moduleDragAndDrop; }
	std::string               getVendor4charCode()              override;

	// BACKLOG U1c — TIDE *is* the rack, so InitInstance/importChunkXml pin
	// Document()->rackMode true and the panel's "Rack Mode" toggle has nothing
	// to offer but a way to turn the product off. Hide it.
	bool                      rackModeIsFixed()                 override { return true; }

	// BACKLOG E2a — a prefab drop arrives as "*P=<relative path>" and
	// CContainer::LoadPrefab resolves it through this. The inherited
	// implementation answers with the user's "SynthEdit Projects/Prefabs"
	// folder, which TIDE has no business reading (constraint 4) and which on
	// an end user's machine does not exist. Resolve into the bundle instead.
	std::wstring ResolveFilename(const std::wstring& name, const std::wstring& extension) override;

private:
	// E2a — populate ModuleFactory()->PrefabFileNames from the bundle. In full
	// SynthEdit the module scan does this; S1a deleted the scan by design, so
	// nothing did, and the browser's Prefabs group was silent.
	void seedPrefabsFromBundle();
	void seedRootMidiCv();          // V3 -- the MIDI-CV every fresh document gets

	// ISeApp Notifier passthrough — disambiguate against the inherited
	// Notifier::RegisterObserver / UnRegisterObserver via explicit `using`.
	void RegisterObserver  (Notifiable* observer) override { Notifier::RegisterObserver  (observer); }
	void UnRegisterObserver(Notifiable* observer) override { Notifier::UnRegisterObserver(observer); }
};
