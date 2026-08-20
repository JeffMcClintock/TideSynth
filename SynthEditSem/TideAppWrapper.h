#pragma once

namespace SE2
{
	// U1a: the interface is typed to the base view, not to either concrete
	// one, so switching the top-level rendering (structure <-> panel/rack)
	// does not re-thread a type through this header, TideApp.h and
	// SynthEditGui.cpp each time.
	class TopView;
}
class ModuleBrowser;
class PropertiesBrowser;
class Notifiable;
namespace gmpi { namespace api { struct IUnknown; } }

#include <functional>

// firewall off SE SDK3 from GMPI SDK
struct ISeApp
{
	virtual ~ISeApp() {}

	// Builds the inner editor view, attaches `host` (so the view's drawing/input
	// hosts are valid before the first Refresh/measure pass), then wires up
	// the presenter. Returns an addRef'd ContainerViewStruct; caller takes ownership.
	virtual SE2::TopView* OpenView(gmpi::api::IUnknown* host) = 0;

	// U1b — container-parameterized open for breadcrumb navigation.
	// container == nullptr means the document's master container.
	// view_flag: 0 routes by depth (master = rack/panel, any other container =
	// structure); an explicit CF_* honours the caller's request — that is how
	// "Goto Structure..." unlocks the master container's own structure view.
	virtual SE2::TopView* OpenViewForContainer(gmpi::api::IUnknown* host, class CContainer* container, int view_flag = 0) = 0;

	// U1b — set by the GUI. TideApp routes CSynthEditAppBase::OpenView(CContainer*)
	// here (fired when the user double-clicks a Container in the structure
	// view) instead of the editor-app m_app_user_interface path TIDE never
	// sets — which made that double-click a null deref before this existed.
	std::function<void(class CContainer*, int /*view_flag*/)> onOpenContainerView;

	// U1b — fired at the end of every OpenView/OpenViewForContainer with the
	// container actually opened; the GUI keeps the breadcrumb trail current.
	std::function<void(class CContainer*, int /*view_flag*/)> onViewOpened;

	// U1b follow-up — scoped suppression of the app's "duplicate module"
	// dialogs around the offscreen thumbnail render, which walks the module
	// factory. Returns the PREVIOUS value so the caller can restore it rather
	// than assuming it was false. A two-method accessor rather than exposing
	// the application object, because firewalling SE SDK3 off from the GMPI
	// side is this interface's whole job.
	virtual bool setQuiet(bool) = 0;

	// S12 - called periodically by the GUI while the editor is open. Exports
	// the document's DSP XML and, when it differs from the last push, hands it
	// to onPushChunk (installed by the controller, which owns the parameter
	// host). Diffing whole-XML beats a modified-flag: it needs no cooperation
	// from every mutation site, and a missed tick self-heals on the next one.
	virtual void serviceDocumentSync() = 0;
	std::function<void(const void* data, size_t size)> onPushChunk;

	virtual void OnCloseView(SE2::TopView*) = 0;
	virtual void CloseAllViews() = 0;

	// Side panes for the in-plugin three-pane editor. Each constructs a
	// gmpi::ui::Form-derived browser, attaches `host`, calls Init() to
	// register observers and build initial visuals, and returns it
	// addRef'd. Caller takes ownership (gmpi::shared_ptr::attach pattern).
	// The Properties Browser tracks the currently-selected module via the
	// app's OM_SHOW_PROPERTIES notification chain; selecting a module in
	// the editor pane updates it automatically.
	virtual ModuleBrowser*     OpenModuleBrowser    (gmpi::api::IUnknown* host) = 0;
	virtual PropertiesBrowser* OpenPropertiesBrowser(gmpi::api::IUnknown* host) = 0;

	// Notifier passthrough — lets the plugin editor observe app-level
	// events (OM_DRAG_NEW_MODULE in particular: when the user clicks a
	// module in the browser, ModuleDragAndDropManager fires that, and
	// the editor needs to forward it to view->DragNewModule so the next
	// view click drops the module). Mirrors the Notifier API on the
	// underlying CSynthEditAppBase without dragging Notifier through
	// the firewall.
	virtual void RegisterObserver  (Notifiable* observer) = 0;
	virtual void UnRegisterObserver(Notifiable* observer) = 0;
};

ISeApp* CreateApp();
