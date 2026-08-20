/* Copyright (c) 2007-2025 SynthEdit Ltd

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

#include "helpers/GmpiPluginEditor.h"
#include "TideAppWrapper.h"
#include "ContainerViewStruct.h"
#include "notify.h" // Notifiable — required transitively by ModuleBrowser.h
#include "Notify_msg.h" // OM_DRAG_NEW_MODULE, OM_SHOW_PROPERTIES
#include "ModuleBrowser.h"
#include "PropertiesBrowser.h"
#include "CContainer.h" // U3 — Container() for "Goto Parent"; used to arrive
                       // transitively via BreadcrumbBar.h, now gone
#include "AboutPane.h" // D6 — the about pane, now opened from the context menu
#include "helpers/ContextMenuHelper.h" // U3 — Goto Parent / Goto Rack / About
#include "helpers/Timer.h" // gmpi::TimerClient — deferred navigation (U1b)
#if defined(__linux__)
#include "modules/se_sdk3/TimerManager.h" // S26 — the se_sdk timer pump below
#endif
#include "helpers/NativeUi.h" // gmpi::api::IDialogHost
#include "helpers/unicode_conversion.h" // JmUnicodeConversions::Utf8ToWstring
#include "RefCountMacros.h"
#include <algorithm>
#include <chrono> // S26 — elapsed time for the se_sdk timer pump
#ifdef _WIN32
#include <windows.h> // SetCursor / LoadCursor / IDC_CROSS — drag-affordance cursor
#endif
#ifdef __APPLE__
// S24 — the AppKit analogue of the Win32 shortcut above; TideCursorMac.mm.
void tideShowCrossCursor(bool show);
#endif

// ---------------------------------------------------------------------------
// PaneHostWrapper — translates IDialogHost rect args from form-local-from-zero
// to plugin-local before forwarding to the outer host.
//
// Why: the side panes use origin-rooted arrange (so PropertiesBrowser's inner
// ScrollPortal doesn't double-translate; see comment in arrange()). That means
// the form thinks it lives at form-local (0,0)-(W,H). When a child (e.g. the
// Response Curve dropdown, or a click-to-edit text field) asks the host for a
// popup or text edit at form-local coords, the host puts it there in the OUTER
// plugin window — i.e. at plugin-local (~30, ~120), well to the left of where
// the user clicked. Symptom: the dropdown popup appeared over the editor
// pane instead of next to the dropdown.
//
// This wrapper sits between each side pane and the outer host. Pass-through
// for IDrawingHost and IInputHost (those don't take screen-positional args).
// IDialogHost: createTextEdit / createPopupMenu / createKeyListener each take
// a rect — translate it by the pane's strip offset before forwarding.
// createFileDialog / createStockDialog don't take rects, pass through.
//
// Offset is mutable: SynthEditGui calls setOffset whenever arrange recomputes
// the strips (e.g. window resize), so the wrapper stays in sync without
// re-doing setHost.
namespace
{
class PaneHostWrapper final : public gmpi::api::IUnknown
{
	// Inner dialog host that translates rect args. Holds the parent's
	// IDialogHost (refcounted) and a pointer back to the wrapper so it
	// reads the latest offset.
	struct TranslatedDialogHost final : public gmpi::api::IDialogHost
	{
		PaneHostWrapper*                         outer{}; // set in PaneHostWrapper's ctor
		gmpi::shared_ptr<gmpi::api::IDialogHost> inner;
		const gmpi::drawing::Point*              offsetPtr; // owned by the wrapper

		gmpi::drawing::Rect translate(const gmpi::drawing::Rect* r) const
		{
			return {
				r->left   + offsetPtr->x,
				r->top    + offsetPtr->y,
				r->right  + offsetPtr->x,
				r->bottom + offsetPtr->y };
		}

		gmpi::ReturnCode createTextEdit(const gmpi::drawing::Rect* r, gmpi::api::IUnknown** ret) override
		{
			if (!inner) return gmpi::ReturnCode::NoSupport;
			const auto t = translate(r);
			return inner->createTextEdit(&t, ret);
		}
		gmpi::ReturnCode createPopupMenu(const gmpi::drawing::Rect* r, gmpi::api::IUnknown** ret) override
		{
			if (!inner) return gmpi::ReturnCode::NoSupport;
			const auto t = translate(r);
			return inner->createPopupMenu(&t, ret);
		}
		gmpi::ReturnCode createKeyListener(const gmpi::drawing::Rect* r, gmpi::api::IUnknown** ret) override
		{
			if (!inner) return gmpi::ReturnCode::NoSupport;
			const auto t = translate(r);
			return inner->createKeyListener(&t, ret);
		}
		gmpi::ReturnCode createFileDialog(int32_t type, gmpi::api::IUnknown** ret) override
		{
			return inner ? inner->createFileDialog(type, ret) : gmpi::ReturnCode::NoSupport;
		}
		gmpi::ReturnCode createStockDialog(int32_t type, const char* title, const char* text, gmpi::api::IUnknown** ret) override
		{
			return inner ? inner->createStockDialog(type, title, text, ret) : gmpi::ReturnCode::NoSupport;
		}
		gmpi::ReturnCode createColorDialog(gmpi::drawing::Color initialColor, gmpi::api::IUnknown** ret) override
		{
			return inner ? inner->createColorDialog(initialColor, ret) : gmpi::ReturnCode::NoSupport;
		}

		// Lifetime delegates to the outer wrapper (COM-aggregation style): a
		// holder of this tear-off (e.g. a Form's env.dialogHost) keeps the
		// whole PaneHostWrapper alive. A no-op refcount here let holders
		// outlive the wrapper and release into freed memory (UAF crash when
		// the wrappers were destroyed before the browsers on document load).
		gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
		{
			return outer->queryInterface(iid, returnInterface);
		}
		int32_t addRef() override  { return outer->addRef(); }
		int32_t release() override { return outer->release(); }
	};

	// Wraps IDrawingHost so invalidate rects get translated into plugin-local
	// before the parent host receives them. Without this, a parameter change
	// (dropdown selection) updated the model but the form's invalidate
	// landed on the LEFT strip — the right pane stayed stale until something
	// else triggered a full repaint.
	struct TranslatedDrawingHost final : public gmpi::api::IDrawingHost
	{
		PaneHostWrapper*                          outer{}; // set in PaneHostWrapper's ctor
		gmpi::shared_ptr<gmpi::api::IDrawingHost> inner;
		const gmpi::drawing::Point*               offsetPtr;

		gmpi::ReturnCode getDrawingFactory(gmpi::api::IUnknown** returnFactory) override
		{
			return inner ? inner->getDrawingFactory(returnFactory) : gmpi::ReturnCode::NoSupport;
		}
		void invalidateRect(const gmpi::drawing::Rect* invalidRect) override
		{
			if (!inner) return;
			if (invalidRect)
			{
				const gmpi::drawing::Rect t{
					invalidRect->left   + offsetPtr->x,
					invalidRect->top    + offsetPtr->y,
					invalidRect->right  + offsetPtr->x,
					invalidRect->bottom + offsetPtr->y };
				inner->invalidateRect(&t);
			}
			else
			{
				inner->invalidateRect(nullptr); // null = invalidate everything; no translation needed
			}
		}
		void invalidateMeasure() override
		{
			if (inner) inner->invalidateMeasure();
		}
		float getRasterizationScale() override
		{
			return inner ? inner->getRasterizationScale() : 1.0f;
		}

		// Lifetime delegates to the outer wrapper — see TranslatedDialogHost.
		gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
		{
			return outer->queryInterface(iid, returnInterface);
		}
		int32_t addRef() override  { return outer->addRef(); }
		int32_t release() override { return outer->release(); }
	};

	gmpi::shared_ptr<gmpi::api::IUnknown> parent;
	gmpi::drawing::Point                  offset{};
	TranslatedDialogHost                  dialogWrapper;
	TranslatedDrawingHost                 drawingWrapper;

public:
	PaneHostWrapper()
	{
		dialogWrapper.outer  = this;
		drawingWrapper.outer = this;
	}

	void initialize(gmpi::api::IUnknown* p)
	{
		parent = {};
		if (p)
		{
			p->addRef();
			parent.attach(p);
			gmpi::shared_ptr<gmpi::api::IDialogHost> innerDialog;
			p->queryInterface(&gmpi::api::IDialogHost::guid, innerDialog.put_void());
			dialogWrapper.inner     = innerDialog;
			dialogWrapper.offsetPtr = &offset;

			gmpi::shared_ptr<gmpi::api::IDrawingHost> innerDrawing;
			p->queryInterface(&gmpi::api::IDrawingHost::guid, innerDrawing.put_void());
			drawingWrapper.inner     = innerDrawing;
			drawingWrapper.offsetPtr = &offset;
		}
		else
		{
			dialogWrapper.inner  = {};
			drawingWrapper.inner = {};
		}
	}

	void setOffset(gmpi::drawing::Point off) { offset = off; }

	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		if (*iid == gmpi::api::IDialogHost::guid)
		{
			*returnInterface = static_cast<gmpi::api::IDialogHost*>(&dialogWrapper);
			dialogWrapper.addRef();
			return gmpi::ReturnCode::Ok;
		}
		if (*iid == gmpi::api::IDrawingHost::guid)
		{
			*returnInterface = static_cast<gmpi::api::IDrawingHost*>(&drawingWrapper);
			drawingWrapper.addRef();
			return gmpi::ReturnCode::Ok;
		}
		// IInputHost and any other interfaces — pass through to the parent host.
		return parent ? parent->queryInterface(iid, returnInterface) : gmpi::ReturnCode::NoSupport;
	}

	GMPI_REFCOUNT;
};
} // namespace

using namespace gmpi;
using namespace gmpi::editor;

// Three-pane in-plugin editor mirroring the full SynthEdit GUI:
//   [ Module Browser ][ Editor (ContainerViewStruct) ][ Properties Browser ]
//
// Layout: each pane gets a vertical strip of the plugin window. Side-pane
// widths are fixed; the editor takes whatever's left in the middle.
//
// Render: each pane is bracketed by pushAxisAlignedClip + a translate
// transform on the device context (matching the screenshot Phase 2 work
// in EditorScreenshot/ScreenshotRenderer.cpp). Without the clip, a pane's
// `g.clear()` wipes its neighbours; without the transform, a pane that
// draws in its own (0,0)-origin coords lands at bitmap-(0,0).
//
// Input: pointer events arrive in plugin-local coords. We dispatch to the
// pane the pointer is in, translating the point into the pane's own local
// coords (the same convention render() uses). Pointer-down captures into
// the pane (via capturedPane) so a drag that leaves the strip still routes
// to the same pane until pointer-up.
//
// Selection -> Properties: the editor's existing ObjectSelected path
// already fires OM_SHOW_PROPERTIES through the app's notification chain,
// and PropertiesBrowser::viewModel observes that — so clicking a module
// in the editor pane updates the right pane automatically.
//
// Drag-from-browser: clicking a module in the browser fires
// OM_DRAG_NEW_MODULE on the app. The GUI app's MainWindow observes this
// and forwards to the active view's DragNewModule(); we mirror that here
// — SynthEditGui itself is a Notifiable, so the click ends up in
// view->DragNewModule(id), and the next click on the editor strip drops
// the module via ViewBase's existing onPointerDown drag-receive path.
#if defined(__linux__)
// S26 — pump the se_sdk timers, which otherwise NEVER FIRE on Linux.
//
// MfcDocPresenter is its own se_sdk::TimerClient: setView() does StartTimer(50),
// and each tick services viewDirty -> RefreshView() -> re-export the container
// to JSON and rebuild the view. That is the "refresh" half of the two
// mechanisms — reconstructing the view from the document, not repainting the
// pixels — and it is how an inserted module becomes visible.
//
// On Windows/macOS the se_sdk TimerManager rides native timers. On Linux it
// has no source at all: TimerManager.h:94 says the host "must call
// [Pump(elapsedMs)] periodically", and SynthEditWayland's main loop does
// exactly that (Main.cpp:190). TIDE is a plugin with no main loop of its own,
// so nothing pumped it — measured 2026-08-20 by Jeff, with real hardware:
// click-placing a module inserted it into the DOCUMENT (the properties pane
// proved that) but the VIEW never rebuilt until switching views forced a
// setView(), whose "OnTimer(); // intial refresh" is the one refresh that does
// not need a timer.
//
// This rides a gmpi_ui TimerClient because that system IS serviced here —
// StandaloneApp.cpp:332 pumps it on Linux, and SynthEditGui's own heartbeat
// below already depends on it. 30ms sits under the presenter's 50ms interval;
// Pump() takes real elapsed time, so longer-interval clients (scopes) still
// fire on their own schedule. Not folded into SynthEditGui's heartbeat because
// that ticks at 500ms for S12's document sync, which serialises the whole
// document per tick — too heavy to run at refresh rate.
struct SeSdkTimerPump final : public gmpi::TimerClient
{
	std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
	SeSdkTimerPump() { startTimer(30); }
	~SeSdkTimerPump() { stopTimer(); }
	bool onTimer() override
	{
		const auto now = std::chrono::steady_clock::now();
		const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
		last = now;
		// Global-scope qualifier: the se_sdk class is ambiguous with
		// gmpi::TimerManager here, which is exactly the collision its header
		// warns about ("prevent ambiguity with GMPI-UI TimerClient").
		::TimerManager::Instance()->Pump(static_cast<int>(ms));
		return true;
	}
};
#endif

class SynthEditGui final : public PluginEditor, public Notifiable, public gmpi::TimerClient
{
	ISeApp* seApp{};
	Pin<Blob> controllerPtr;
	gmpi::shared_ptr<gmpi::api::IUnknown> hostUnknown;
	gmpi::shared_ptr<SE2::TopView> view;
#if defined(__linux__)
	// S26 — lives and dies with the editor, like the presenter timers it drives.
	SeSdkTimerPump seSdkTimerPump;
#endif

	// Per-pane host wrappers — translate IDialogHost rects into plugin-local
	// coords so popups / text-edits land next to the control that requested
	// them rather than at form-local-from-zero on the outer host. Held by
	// shared_ptr because Form::setHost stores it via gmpi::shared_ptr.
	// Declared BEFORE the browsers so they're destroyed after them: each
	// browser's Form holds refs into its wrapper and releases them on
	// destruction.
	gmpi::shared_ptr<PaneHostWrapper> browserHostWrapper;
	gmpi::shared_ptr<PaneHostWrapper> propertiesHostWrapper;

	gmpi::shared_ptr<ModuleBrowser>            moduleBrowser;
	gmpi::shared_ptr<PropertiesBrowser>        propertiesBrowser;

	// U3 — the breadcrumb bar is gone (C7c: "a bit redundant in a product where
	// you seldom dig deeper than 1 level in"). What it uniquely provided —
	// navigating back UP — is now "Goto Parent" / "Goto Rack" on the context
	// menu, which needs to know where we are. Seeded by onViewOpened.
	// SE2::BreadcrumbBar itself is untouched; the WinUI3, Wayland, JUCE and Mac
	// frontends still use it.
	CContainer* currentContainer{};

	// D6 — the about pane. Its text affordance used to sit at the right end of
	// the crumb strip and was "the only way in"; with the strip gone it opens
	// from the context menu instead. The pane itself is unchanged: not a dialog,
	// no modal loop, dismissed by a click away from it.
	tide::AboutPane            aboutPane;

	// Deferred navigation, drained on the shared UI timer: requests originate
	// inside pointer dispatch (a crumb click, or a double-click inside the
	// view), and rebuilding the view stack from within its own dispatch would
	// destroy the object the event is being dispatched through. One-shot.
	CContainer* pendingNavTarget{};
	int         pendingNavFlag{}; // 0 = route by depth; CF_* honours a menu request
	bool        navPending{};

	// True between OM_DRAG_NEW_MODULE start (browser click) and OM_DRAG_NEW_MODULE
	// end (drop on view, ESC, or a second browser click). onPointerMove uses
	// this to keep the cross cursor displayed via Win32 SetCursor.
	bool dragInProgress = false;

	// Scroll state for the editor pane. The view writes these via the
	// hscrollBar / vscrollBar callbacks we wire in notifyPin; render reads
	// them to draw the scrollbar thumbs; onPointer* uses them to map drag
	// pixels back to scroll values.
	SE2::scrollBarSpec editorHSpec{};
	SE2::scrollBarSpec editorVSpec{};

	// Scrollbar geometry, recomputed in recomputeStrips. Both bars are
	// kScrollBarWidth pixels thick; vertical sits along the editor strip's
	// right edge, horizontal along the bottom. They overlap a small dead
	// corner at the bottom-right.
	static constexpr float kScrollBarWidth = 12.0f;
	gmpi::drawing::Rect editorContentRect{}; // editor strip minus scrollbar tracks
	gmpi::drawing::Rect editorVScrollRect{};
	gmpi::drawing::Rect editorHScrollRect{};

	// Tracks an in-progress scrollbar drag.
	enum class ScrollDrag { None, EditorV, EditorH };
	ScrollDrag scrollDrag       = ScrollDrag::None;
	float      scrollDragStartM = 0.0f; // pixel coord at drag start
	double     scrollDragStartV = 0.0;  // scroll value at drag start

	// Pane that captured the most recent pointer-down. Subsequent moves /
	// up events route here regardless of where the pointer drifts. nullptr
	// means no capture (pointer-up just received, or never went down).
	enum class Pane { None, Browser, Editor, Properties };
	Pane capturedPane = Pane::None;

	// Side-pane widths in logical pixels. Picked to match what feels
	// right in a typical 800-1200 wide plugin window — the screenshot
	// version uses 400/320 (Module Browser/Properties) but those are
	// generous for documentation; in-plugin we want the editor to be
	// the dominant pane.
	static constexpr float kBrowserW    = 240.0f;
	static constexpr float kPropertiesW = 280.0f;
	static constexpr float kMinEditorW  = 200.0f;

	// Strip rectangles, recomputed in arrange().
	gmpi::drawing::Rect browserStrip{};
	gmpi::drawing::Rect editorStrip{};
	gmpi::drawing::Rect propertiesStrip{};
	bool                showSidePanes = false; // false if window too narrow for all three

	// Decide whether the bounds afford all three panes, and partition.
	void recomputeStrips(const gmpi::drawing::Rect& r)
	{
		// U3 — no breadcrumb strip any more; the panes fill the whole bounds.
		const gmpi::drawing::Rect body{ r.left, r.top, r.right, r.bottom };

		const float totalW = body.right - body.left;
		showSidePanes = (totalW >= kBrowserW + kMinEditorW + kPropertiesW);

		if (showSidePanes)
		{
			browserStrip    = { body.left,                 body.top, body.left + kBrowserW,     body.bottom };
			editorStrip     = { body.left + kBrowserW,     body.top, body.right - kPropertiesW, body.bottom };
			propertiesStrip = { body.right - kPropertiesW, body.top, body.right,                body.bottom };
		}
		else
		{
			browserStrip    = {};
			editorStrip     = body; // editor takes everything when too narrow
			propertiesStrip = {};
		}

		// Subdivide the editor strip into content area + scrollbar tracks.
		// The view's drawingBounds gets the content rect, so its viewTransform
		// centres on a smaller canvas; the scrollbars sit alongside.
		editorContentRect = {
			editorStrip.left,
			editorStrip.top,
			editorStrip.right  - kScrollBarWidth,
			editorStrip.bottom - kScrollBarWidth };
		editorVScrollRect = {
			editorStrip.right - kScrollBarWidth,
			editorStrip.top,
			editorStrip.right,
			editorStrip.bottom - kScrollBarWidth };
		editorHScrollRect = {
			editorStrip.left,
			editorStrip.bottom - kScrollBarWidth,
			editorStrip.right - kScrollBarWidth,
			editorStrip.bottom };
	}

	// Translate a plugin-local point into a pane's own local coords (where
	// the pane's top-left is (0,0)). Render uses the same translate via the
	// device-context transform, so input round-trips correctly.
	static gmpi::drawing::Point toLocal(gmpi::drawing::Point p, const gmpi::drawing::Rect& strip)
	{
		return { p.x - strip.left, p.y - strip.top };
	}

	// NOTE: side panes use origin-rooted arrange + a render-time translate.
	// We tried "strip-rooted everywhere" to make form-local == plugin-local
	// (which would simplify popup positioning), but PropertiesBrowser::Body
	// uses a ScrollPortal, and the Portal applies translate(bounds.left, …)
	// to its children — meaning Body's strip-rooted Grid coords get
	// double-translated by the Portal. Net effect: pane renders blank.
	// So we keep origin-rooted bounds (where the Portal's translate is
	// identity) and accept that popups land at form-local coords on the
	// outer host. A future fix wraps IDialogHost so it translates back
	// to plugin-local before forwarding.

	Pane paneAt(gmpi::drawing::Point p) const
	{
		if (!showSidePanes) return Pane::Editor;
		if (p.x < browserStrip.right)         return Pane::Browser;
		if (p.x >= propertiesStrip.left)      return Pane::Properties;
		return Pane::Editor;
	}

	// Forwards a render call to a Form-based pane (ModuleBrowser /
	// PropertiesBrowser), translating the device context so the pane draws
	// at its strip's top-left rather than bitmap (0,0). Side panes use
	// origin-rooted arrange (see comment above) so without this translate
	// they'd land at plugin-local (0,0). Editor pane is special-cased:
	// ContainerViewStruct's viewTransform already accounts for arrange-rect
	// offset, so it doesn't need an outer translate.
	//
	// COMPOSE with the host's existing transform rather than REPLACE it.
	// The host has set its own transform to position our plugin inside
	// the canvas (module-on-panel offset, DPI scaling, etc.); replacing
	// that loses parent context and the form draws nowhere visible.
	// Row-vector convention: `offset * parent` applies offset first, then
	// parent.
	template <class FormPane>
	void renderFormPane(
		gmpi::drawing::api::IDeviceContext* ctx,
		const gmpi::drawing::Rect&          strip,
		FormPane*                           pane)
	{
		if (!pane || strip.right <= strip.left)
			return;

		gmpi::drawing::Graphics g(ctx);
		g.pushAxisAlignedClip(strip);

		gmpi::drawing::Matrix3x2 saved{};
		ctx->getTransform(&saved);

		const auto offsetT  = gmpi::drawing::makeTranslation({ strip.left, strip.top });
		const auto composed = offsetT * saved;
		ctx->setTransform(&composed);

		pane->render(ctx);

		ctx->setTransform(&saved);

		g.popAxisAlignedClip();
	}

	// Replace the no-op scrollbar callbacks (set by TideApp::OpenView just to
	// satisfy TopView::onMouseWheel's hasScrollbars guard) with real ones that
	// capture the spec for our drawn scrollbars. Extracted so navigation
	// re-opens (U1b) rewire identically.
	void wireViewScrollbars()
	{
		if (!view)
			return;
		view->hscrollBar = [this](const SE2::scrollBarSpec& spec)
		{
			editorHSpec = spec;
			if (drawingHost) drawingHost->invalidateRect(nullptr);
		};
		view->vscrollBar = [this](const SE2::scrollBarSpec& spec)
		{
			editorVSpec = spec;
			if (drawingHost) drawingHost->invalidateRect(nullptr);
		};
	}

	// U1b — see the pendingNav members above for why navigation is deferred.
	void requestNavigate(CContainer* target, int view_flag = 0)
	{
		if (!target)
			return;
		pendingNavTarget = target;
		pendingNavFlag = view_flag;
		navPending = true;
		startTimer(30);
	}

	bool onTimer() override
	{
		// S12 - the same timer now doubles as the document-sync heartbeat:
		// while the editor is open, give the app a chance to push DSP XML.
		// (Edits can only happen while an editor is open, so a timer that
		// lives and dies with the GUI covers every mutation.)
		if (seApp)
			seApp->serviceDocumentSync();

		if (!navPending)
			return true; // keep ticking for sync; stop only if we never started the periodic timer
		navPending = false;
		auto* target = pendingNavTarget;
		const int flag = pendingNavFlag;
		pendingNavTarget = nullptr;
		pendingNavFlag = 0;

		if (seApp && target)
		{
			if (view)
				seApp->OnCloseView(view.get());
			view.attach(seApp->OpenViewForContainer(hostUnknown.get(), target, flag));
			wireViewScrollbars();
			if (view && bounds.right > bounds.left)
			{
				view->arrange(&editorContentRect);
				view->updateScrollBars();
			}
			if (drawingHost)
				drawingHost->invalidateRect(nullptr);
		}
		return true; // S12 - the timer is now periodic (document sync)
	}

public:
	~SynthEditGui()
	{
		stopTimer(); // U1b — no ticks into a dying object
		if (seApp)
		{
			seApp->onOpenContainerView = nullptr; // U1b — both capture `this`
			seApp->onViewOpened = nullptr;
			seApp->UnRegisterObserver(this);
			if (view)
				seApp->OnCloseView(view.get());
		}
		// moduleBrowser / propertiesBrowser self-destruct via shared_ptr;
		// their dtors unregister from the app's observer list.
	}

	// Notifiable: forward app-level notifications to whichever pane needs them.
	void OnNotify(Notifier* /*sender*/, int lHint, void* pHint) override
	{
		if (lHint == OM_DRAG_NEW_MODULE && view)
		{
			// pHint is the module-id c-str when starting a drag, nullptr when
			// ending. ViewBase::DragNewModule handles both cases (null clears
			// the drag-receive flag).
			view->DragNewModule(static_cast<const char*>(pHint));
			// Track for the cross-cursor effect in onPointerMove.
			dragInProgress = (pHint != nullptr);
#ifdef __APPLE__
			// S24: change at the browser click itself, and restore the arrow
			// the moment the arm ends — the per-move re-assert below only
			// runs while armed, so it can never un-set the cross.
			tideShowCrossCursor(dragInProgress);
#endif
		}
		else if (lHint == OM_INSERT_MODULE_AT_VIEW_CENTER && view && pHint)
		{
			// Double-click in the Module Browser fires InsertAtViewCenter on
			// the drag-and-drop manager → OM_INSERT_MODULE_AT_VIEW_CENTER
			// notification with pHint = module-id c-str. Mirror the GUI app's
			// MainWindow handler: ask the presenter to insert at the view's
			// current centre. The notification reaches us via Notifiable
			// (registered in notifyPin); without this branch the user's
			// double-click is silently ignored.
			const auto moduleId = JmUnicodeConversions::Utf8ToWstring(
				static_cast<const char*>(pHint));
			if (auto* p = view->Presenter())
				p->AddModule(moduleId.c_str(), view->getCenter());
		}
		else if (lHint == OM_SHOW_PROPERTIES)
		{
			// (lHint and pHint not used here; the relevant viewModel observer
			// inside PropertiesBrowser already updates currentModule.)
			(void)pHint;
			// PropertiesBrowser::OnModelWillChange already calls Form::redraw()
			// which invokes host->invalidateRect(nullptr) — but that's the
			// host PropertiesBrowser was setHost'd with, which may not be the
			// outer plugin host's drawing surface (depending on how the parent
			// wires its hosts). Belt-and-suspenders: invalidate from our own
			// PluginEditor::drawingHost, which is unambiguously the outer host.
			// Without this, clicking a module in the editor selected it (the
			// notification fires; viewModel updates internally) but the right
			// strip didn't visually refresh.
			if (drawingHost)
				drawingHost->invalidateRect(nullptr);
		}
	}

	ReturnCode setHost(gmpi::api::IUnknown* phost) override
	{
		PluginEditor::setHost(phost);
		hostUnknown = phost;

		// Side panes get host wrappers that translate IDialogHost coords
		// (popups / text-edits / key listeners). Editor view doesn't need
		// the wrapping — ContainerViewStruct's input is local-origin via
		// its arrange rect already.
		if (!browserHostWrapper)    { browserHostWrapper.attach   (new PaneHostWrapper()); }
		if (!propertiesHostWrapper) { propertiesHostWrapper.attach(new PaneHostWrapper()); }
		browserHostWrapper   ->initialize(phost);
		propertiesHostWrapper->initialize(phost);
		// EVERY wrapper, not just the two created here. initialize() is also the
		// SEVERING path: setHost(nullptr) makes it drop the host ref, and
		// StandaloneHost's destructor calls exactly that ("sever the editor
		// before the controller goes"). This list once omitted the breadcrumb
		// bar's wrapper (created later, in notifyPin) -- so on shutdown it was
		// the one wrapper still holding the host, and released it after the host
		// had gone. The standalone's pane host is GMPI_REFCOUNT_NO_DELETE
		// (AppLayout.h), i.e. the ref is non-owning and cannot keep it alive, so
		// that release was a virtual call through a dangling pointer:
		// EXC_BAD_ACCESS in PaneHostWrapper::release() <- ~SynthEditGui <-
		// ~StandaloneHost, with a pointer-authentication-failure signature.
		// MEASURED, and stated precisely because it is a partial win: before this
		// line, seven consecutive shutdowns crashed in PaneHostWrapper::release
		// with garbage/PAC-failure addresses. After it, that crash is gone --
		// three consecutive shutdowns instead crash in
		// ~PluginEditor <- ~SubView <- ~ModuleView <- ~ViewBase, dereferencing
		// null+0x10. That is a SECOND, independent teardown bug that was queued
		// behind this one, filed separately. So this fixes what it claims to fix
		// and the app still cannot quit cleanly.
		if (view)              view             ->setHost(phost);
		if (moduleBrowser)     moduleBrowser    ->setHost(static_cast<gmpi::api::IUnknown*>(browserHostWrapper.get()));
		if (propertiesBrowser) propertiesBrowser->setHost(static_cast<gmpi::api::IUnknown*>(propertiesHostWrapper.get()));
		return ReturnCode::Ok;
	}

	ReturnCode notifyPin(int32_t pinId, int32_t voice) override
	{
		if (pinId == 0 && controllerPtr.value.size() == sizeof(seApp))
		{
			seApp = *reinterpret_cast<ISeApp**>(controllerPtr.value.data());

			if (seApp)
			{
				// Editor first (matches existing behaviour), then panes. Side
				// panes use the per-pane host wrapper so IDialogHost rect
				// args get translated into plugin-local coords on the way
				// to the outer host (see PaneHostWrapper above).
				if (!browserHostWrapper)    { browserHostWrapper.attach   (new PaneHostWrapper()); browserHostWrapper   ->initialize(hostUnknown.get()); }
				if (!propertiesHostWrapper) { propertiesHostWrapper.attach(new PaneHostWrapper()); propertiesHostWrapper->initialize(hostUnknown.get()); }

				// The menu command's flag is honoured ("Goto Structure..." on the
				// master is the rack's unlock); U3's context-menu items pass 0.
				seApp->onOpenContainerView = [this](CContainer* c, int flag) { requestNavigate(c, flag); };
				startTimer(500); // S12 - periodic document-sync heartbeat (also services deferred navigation)
				// U3 — remember where we are so the context menu can offer
				// "Goto Parent" / "Goto Rack". This is all that survives of the
				// breadcrumb bar's bookkeeping.
				seApp->onViewOpened = [this](CContainer* c, int /*flag*/)
				{
					currentContainer = c;
				};

				view.attach(seApp->OpenView(hostUnknown.get()));
				moduleBrowser    .attach(seApp->OpenModuleBrowser    (static_cast<gmpi::api::IUnknown*>(browserHostWrapper.get())));
				propertiesBrowser.attach(seApp->OpenPropertiesBrowser(static_cast<gmpi::api::IUnknown*>(propertiesHostWrapper.get())));

				// Replace the no-op scrollbar callbacks (set by TideApp::OpenView
				// just to satisfy TopView::onMouseWheel's hasScrollbars guard)
				// with real ones that capture the spec for our drawn scrollbars
				// and request a repaint so the thumbs visibly track scroll.
				wireViewScrollbars();

				// Forward OM_DRAG_NEW_MODULE → view->DragNewModule (see class
				// comment). Has to happen after the panes exist; the manager
				// fires the notification from the browser's pointer handler,
				// which only runs after Init below.
				seApp->RegisterObserver(this);

				// Bring everything up to date if we've already been arranged.
				// Origin-rooted rects for side panes — see arrange() above.
				if (bounds.right > bounds.left)
				{
					recomputeStrips(bounds);
					if (view)
					{
						view->arrange(&editorContentRect);
						view->updateScrollBars(); // populate initial scrollbar spec
					}
					if (moduleBrowser)
					{
						const gmpi::drawing::Rect browserLocal{
							0.0f, 0.0f,
							browserStrip.right - browserStrip.left,
							browserStrip.bottom - browserStrip.top };
						moduleBrowser->arrange(&browserLocal);
					}
					if (propertiesBrowser)
					{
						const gmpi::drawing::Rect propsLocal{
							0.0f, 0.0f,
							propertiesStrip.right - propertiesStrip.left,
							propertiesStrip.bottom - propertiesStrip.top };
						propertiesBrowser->arrange(&propsLocal);
					}
					if (browserHostWrapper)
						browserHostWrapper->setOffset({ browserStrip.left, browserStrip.top });
					if (propertiesHostWrapper)
						propertiesHostWrapper->setOffset({ propertiesStrip.left, propertiesStrip.top });
				}
			}
		}

		return PluginEditor::notifyPin(pinId, voice);
	}

	ReturnCode measure(const gmpi::drawing::Size* availableSize, gmpi::drawing::Size* returnDesiredSize) override
	{
		// Pick a sensible plugin window size. We're resizable (return whatever
		// the host offers), but cap to a reasonable max so VST3 hosts that
		// probe with availableSize = {99999, 99999} don't try to hand us a
		// 99999x99999 window. Default width grew vs the single-pane version
		// to give the editor breathing room alongside both side panes.
		constexpr float defaultWidth  = 1100.0f;
		constexpr float defaultHeight = 600.0f;
		constexpr float maxWidth      = 2400.0f;
		constexpr float maxHeight     = 1800.0f;

		// BACKLOG E2a — a host may PROBE with zero available space, meaning
		// "what size do you actually want?" rather than "here is your window".
		// The clamp below answers that probe with zero, and a host that takes
		// the answer literally then falls back to its own default: the GMPI
		// standalone measures at {0,0}, reads the reply as our natural size,
		// treats zero as "no opinion" and opens a 400x400 window
		// (GMPI_Wrappers wrapper/Standalone/StandaloneHost.cpp:165-205). At
		// that width recomputeStrips sets showSidePanes = false, so BOTH the
		// module browser and the properties pane silently vanish -- which is
		// what made TIDE look like it had no module browser at all.
		// Answer a probe with the size we actually want.
		if (availableSize->width <= 0.0f || availableSize->height <= 0.0f)
		{
			returnDesiredSize->width  = defaultWidth;
			returnDesiredSize->height = defaultHeight;
			return ReturnCode::Ok;
		}

		returnDesiredSize->width  = (std::min)(availableSize->width,  maxWidth);
		returnDesiredSize->height = (std::min)(availableSize->height, maxHeight);

		if (availableSize->width  >= maxWidth)  returnDesiredSize->width  = defaultWidth;
		if (availableSize->height >= maxHeight) returnDesiredSize->height = defaultHeight;

		return ReturnCode::Ok;
	}

	ReturnCode arrange(const gmpi::drawing::Rect* finalRect) override
	{
		PluginEditor::arrange(finalRect);
		recomputeStrips(*finalRect);

		// Editor: ContainerViewStruct's calcViewTransform centres on the
		// drawingBounds size only, so passing the content rect directly is
		// fine — its render-side composes with the host's parent transform.
		// Use the content rect (excludes scrollbar tracks) so the editor's
		// surround colour stops at the scrollbars and module-on-canvas
		// centring uses the scrollable viewport size.
		if (view)
		{
			view->arrange(&editorContentRect);
			// Populate the initial scrollbar spec. arrange() doesn't fire
			// the spec callback by itself — the editor view only re-runs
			// updateScrollBars when something actively pans/zooms/resizes
			// the camera. Without this our editorH/V Spec stays default-
			// constructed zero, drawScrollBar's range==0 check trips, and
			// the thumbs aren't drawn until the first user scroll.
			view->updateScrollBars();
		}

		// Side panes (Form-based with internal ScrollPortal): Body lays
		// children out using `bounds` as absolute coords, then the
		// ScrollPortal translates by `bounds.left` again — so any non-zero
		// bounds.left double-offsets and content drifts off-screen. Pass
		// origin-rooted rects (so the Portal's translate is a no-op);
		// renderFormPane's outer translate is what actually positions
		// the pane on the plugin window. The PaneHostWrapper offset
		// (set below) handles popup-positioning back to plugin-local.
		if (moduleBrowser)
		{
			const gmpi::drawing::Rect browserLocal{
				0.0f, 0.0f,
				browserStrip.right - browserStrip.left,
				browserStrip.bottom - browserStrip.top };
			moduleBrowser->arrange(&browserLocal);
		}
		if (propertiesBrowser)
		{
			const gmpi::drawing::Rect propsLocal{
				0.0f, 0.0f,
				propertiesStrip.right - propertiesStrip.left,
				propertiesStrip.bottom - propertiesStrip.top };
			propertiesBrowser->arrange(&propsLocal);
		}

		if (browserHostWrapper)
			browserHostWrapper->setOffset({ browserStrip.left, browserStrip.top });
		if (propertiesHostWrapper)
			propertiesHostWrapper->setOffset({ propertiesStrip.left, propertiesStrip.top });

		return ReturnCode::Ok;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		// Outermost clip — keeps the entire editor inside its host bounds
		// when nested inside a wrapper ModuleView (matches the original
		// single-pane behaviour).
		gmpi::drawing::Graphics g(drawingContext);
		gmpi::drawing::ClipDrawingToBounds clip(g, bounds);

		// Editor first; ContainerViewStruct uses its arrange-rect offset
		// directly so it doesn't need an outer translate. Clip to the
		// CONTENT rect (excludes scrollbar tracks) so the editor's surround
		// colour stops where the scrollbars start.
		if (view)
		{
			gmpi::drawing::Graphics gEditor(drawingContext);
			gEditor.pushAxisAlignedClip(editorContentRect);
			view->render(drawingContext);
			gEditor.popAxisAlignedClip();

			drawEditorScrollBars(drawingContext);
		}

		if (showSidePanes)
		{
			renderFormPane(drawingContext, browserStrip,    moduleBrowser.get());
			renderFormPane(drawingContext, propertiesStrip, propertiesBrowser.get());
		}

		// D6 — the pane itself, last so it sits above everything, and over the
		// whole editor rather than inside one strip. Not a dialog: no modal
		// loop, nothing blocking, dismissed by a click away from it.
		if (aboutPane.isOpen)
		{
			gmpi::drawing::Graphics g(drawingContext);
			aboutPane.render(g, bounds);
		}

		return ReturnCode::Ok;
	}

	// Draw a single scrollbar's track + thumb. Returns the thumb rect so
	// onPointer* can hit-test against it without recomputing. Track is a
	// flat dark rectangle, thumb is a lighter rounded rectangle inset
	// from the track edges — matches the ModuleBrowser ScrollPortal style.
	gmpi::drawing::Rect drawScrollBar(
		gmpi::drawing::api::IDeviceContext* ctx,
		const gmpi::drawing::Rect&          barRect,
		const SE2::scrollBarSpec&           spec,
		bool                                vertical) const
	{
		gmpi::drawing::Graphics g(ctx);
		// Track background — flat rect, no rounding (matches ScrollPortal).
		auto trackBrush = g.createSolidColorBrush(gmpi::drawing::colorFromHex(0x282828u));
		g.fillRectangle(barRect, trackBrush);

		// No content overflow → no thumb (the inert track is enough hint).
		const double range  = spec.Maximum - spec.Minimum;
		const double total  = range + spec.ViewportSize;
		if (range <= 0.0 || total <= 0.0 || spec.ViewportSize <= 0.0)
			return {};

		const float barLen = vertical
			? (barRect.bottom - barRect.top)
			: (barRect.right  - barRect.left);
		const float thumbLen = std::max(20.0f,
			static_cast<float>(spec.ViewportSize / total) * barLen);
		const float usable   = std::max(0.0f, barLen - thumbLen);
		const float thumbOff = usable
			* static_cast<float>((spec.Value - spec.Minimum) / range);

		gmpi::drawing::Rect thumbRect = barRect;
		if (vertical)
		{
			thumbRect.top    = barRect.top + thumbOff;
			thumbRect.bottom = thumbRect.top + thumbLen;
			// Inset 4px on the cross axis so the thumb floats in the track.
			thumbRect.left  += 4.0f;
			thumbRect.right -= 4.0f;
		}
		else
		{
			thumbRect.left  = barRect.left + thumbOff;
			thumbRect.right = thumbRect.left + thumbLen;
			thumbRect.top    += 4.0f;
			thumbRect.bottom -= 4.0f;
		}

		auto thumbBrush = g.createSolidColorBrush(gmpi::drawing::colorFromHex(0x707070u));
		g.fillRoundedRectangle({ thumbRect, 4.0f, 4.0f }, thumbBrush);
		return thumbRect;
	}

	void drawEditorScrollBars(gmpi::drawing::api::IDeviceContext* ctx)
	{
		drawScrollBar(ctx, editorVScrollRect, editorVSpec, /*vertical*/ true);
		drawScrollBar(ctx, editorHScrollRect, editorHSpec, /*vertical*/ false);

		// Dead corner where the two scrollbars would meet. Fill it so the
		// editor's grid doesn't peek through.
		const gmpi::drawing::Rect deadCorner{
			editorStrip.right  - kScrollBarWidth,
			editorStrip.bottom - kScrollBarWidth,
			editorStrip.right,
			editorStrip.bottom };
		gmpi::drawing::Graphics g(ctx);
		auto brush = g.createSolidColorBrush(gmpi::drawing::colorFromHex(0x282828u));
		g.fillRectangle(deadCorner, brush);
	}

	// Map a mouse position on the scrollbar to a new scroll Value.
	// `delta` is the pixel distance the user has dragged from drag-start.
	double scrollValueFromDrag(const SE2::scrollBarSpec& spec, const gmpi::drawing::Rect& barRect, bool vertical, float delta) const
	{
		const double range  = spec.Maximum - spec.Minimum;
		const double total  = range + spec.ViewportSize;
		if (range <= 0.0 || total <= 0.0) return spec.Minimum;

		const float barLen = vertical
			? (barRect.bottom - barRect.top)
			: (barRect.right  - barRect.left);
		const float thumbLen = std::max(20.0f,
			static_cast<float>(spec.ViewportSize / total) * barLen);
		const float usable   = std::max(1.0f, barLen - thumbLen);

		const double valueDelta = static_cast<double>(delta) * (range / usable);
		double newValue = scrollDragStartV + valueDelta;
		if (newValue < spec.Minimum) newValue = spec.Minimum;
		if (newValue > spec.Maximum) newValue = spec.Maximum;
		return newValue;
	}

	bool pointInRect(gmpi::drawing::Point p, const gmpi::drawing::Rect& r) const
	{
		return p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom;
	}

	ReturnCode getClipArea(gmpi::drawing::Rect* returnRect) override
	{
		*returnRect = bounds;
		return ReturnCode::Ok;
	}

	// ---- input dispatch ----
	// Side panes use origin-rooted bounds so input points need to be
	// translated from plugin-local into form-local-from-zero before
	// forwarding. Editor pane: ContainerViewStruct handles its own
	// coord transform internally, so points pass through verbatim.

	ReturnCode hitTest(gmpi::drawing::Point point, int32_t flags) override
	{
		switch (paneAt(point))
		{
		case Pane::Browser:
			return moduleBrowser     ? moduleBrowser    ->hitTest(toLocal(point, browserStrip),    flags) : ReturnCode::Ok;
		case Pane::Properties:
			return propertiesBrowser ? propertiesBrowser->hitTest(toLocal(point, propertiesStrip), flags) : ReturnCode::Ok;
		case Pane::Editor:
		default:
			return view ? view->hitTest(point, flags) : ReturnCode::Ok;
		}
	}

	ReturnCode onPointerDown(gmpi::drawing::Point point, int32_t flags) override
	{
		// D6 — while the about pane is open it gets first refusal, so the click
		// that dismisses it never also lands on the rack behind it. The pane is
		// still not modal: it consumes this one click and nothing else.
		if (aboutPane.isOpen)
		{
			aboutPane.onPointerDown(point);
			if (drawingHost) drawingHost->invalidateRect(nullptr);
			return ReturnCode::Ok;
		}

		// Scrollbar hits take precedence over pane routing — a click on the
		// thumb shouldn't also fall through to the editor canvas.
		if (view && pointInRect(point, editorVScrollRect))
		{
			scrollDrag       = ScrollDrag::EditorV;
			scrollDragStartM = point.y;
			scrollDragStartV = editorVSpec.Value;
			return ReturnCode::Ok;
		}
		if (view && pointInRect(point, editorHScrollRect))
		{
			scrollDrag       = ScrollDrag::EditorH;
			scrollDragStartM = point.x;
			scrollDragStartV = editorHSpec.Value;
			return ReturnCode::Ok;
		}

		const auto pane = paneAt(point);
		capturedPane = pane;
		switch (pane)
		{
		case Pane::Browser:
			return moduleBrowser     ? moduleBrowser    ->onPointerDown(toLocal(point, browserStrip),    flags) : ReturnCode::Unhandled;
		case Pane::Properties:
			return propertiesBrowser ? propertiesBrowser->onPointerDown(toLocal(point, propertiesStrip), flags) : ReturnCode::Unhandled;
		case Pane::Editor:
		default:
			return view ? view->onPointerDown(point, flags) : ReturnCode::Unhandled;
		}
	}

	ReturnCode onPointerMove(gmpi::drawing::Point point, int32_t flags) override
	{
		// Cross cursor while drag-from-browser is in progress. GMPI doesn't
		// expose setCursor on IInputHost, so we go straight to the OS here
		// (Win32 / AppKit; linux still has no path — S24 notes why).
		// Re-set every move because the host's WM_SETCURSOR handler will
		// otherwise revert to the default arrow between events. Crude, but
		// the simplest thing that works for our one-off drag affordance.
#ifdef _WIN32
		if (dragInProgress)
			::SetCursor(::LoadCursor(nullptr, IDC_CROSS));
#endif
#ifdef __APPLE__
		// S24: same re-assert for AppKit — cursor rects (window edges, text
		// fields) reset the cursor on their own schedule, like WM_SETCURSOR.
		if (dragInProgress)
			tideShowCrossCursor(true);
#endif

		// Scrollbar drag wins over pane routing while it's active.
		// onHScroll/onVScroll deliberately skip updateScrollBars (the GUI app
		// calls them from its WinUI scrollbar handler, which already reflects
		// the user's drag visually). For our hand-drawn scrollbar the spec
		// callback IS the thumb's source of truth, so call updateScrollBars
		// explicitly afterwards — that fires our lambda, which stores the
		// new spec and triggers a repaint with the thumb at the right place.
		if (scrollDrag == ScrollDrag::EditorV && view)
		{
			const float delta = point.y - scrollDragStartM;
			view->onVScroll(scrollValueFromDrag(editorVSpec, editorVScrollRect, /*vertical*/ true, delta));
			view->updateScrollBars();
			return ReturnCode::Ok;
		}
		if (scrollDrag == ScrollDrag::EditorH && view)
		{
			const float delta = point.x - scrollDragStartM;
			view->onHScroll(scrollValueFromDrag(editorHSpec, editorHScrollRect, /*vertical*/ false, delta));
			view->updateScrollBars();
			return ReturnCode::Ok;
		}

		// During a drag, route to the captured pane so a click that started
		// in (say) the browser stays with the browser even if the user drags
		// over the editor.
		const auto pane = (capturedPane != Pane::None) ? capturedPane : paneAt(point);
		switch (pane)
		{
		case Pane::Browser:
			return moduleBrowser     ? moduleBrowser    ->onPointerMove(toLocal(point, browserStrip),    flags) : ReturnCode::Unhandled;
		case Pane::Properties:
			return propertiesBrowser ? propertiesBrowser->onPointerMove(toLocal(point, propertiesStrip), flags) : ReturnCode::Unhandled;
		case Pane::Editor:
		default:
			return view ? view->onPointerMove(point, flags) : ReturnCode::Unhandled;
		}
	}

	ReturnCode onPointerUp(gmpi::drawing::Point point, int32_t flags) override
	{
		// End any active scrollbar drag — let go of the thumb and stop
		// intercepting pointer events.
		if (scrollDrag != ScrollDrag::None)
		{
			scrollDrag = ScrollDrag::None;
			return ReturnCode::Ok;
		}

		const auto pane = (capturedPane != Pane::None) ? capturedPane : paneAt(point);
		const auto rc = [&]() {
			switch (pane)
			{
			case Pane::Browser:
				return moduleBrowser     ? moduleBrowser    ->onPointerUp(toLocal(point, browserStrip),    flags) : ReturnCode::Unhandled;
			case Pane::Properties:
				return propertiesBrowser ? propertiesBrowser->onPointerUp(toLocal(point, propertiesStrip), flags) : ReturnCode::Unhandled;
			case Pane::Editor:
			default:
				return view ? view->onPointerUp(point, flags) : ReturnCode::Unhandled;
			}
		}();
		capturedPane = Pane::None;
		return rc;
	}

	ReturnCode onMouseWheel(gmpi::drawing::Point point, int32_t flags, int32_t delta) override
	{
		switch (paneAt(point))
		{
		case Pane::Browser:
			return moduleBrowser     ? moduleBrowser    ->onMouseWheel(toLocal(point, browserStrip),    flags, delta) : ReturnCode::Unhandled;
		case Pane::Properties:
			return propertiesBrowser ? propertiesBrowser->onMouseWheel(toLocal(point, propertiesStrip), flags, delta) : ReturnCode::Unhandled;
		case Pane::Editor:
		default:
			return view ? view->onMouseWheel(point, flags, delta) : ReturnCode::Unhandled;
		}
	}

	ReturnCode onKeyPress(wchar_t c) override
	{
		// No focus model yet — keys go to the editor (matches single-pane
		// behaviour and the GUI app's keyboard-to-canvas convention).
		return view ? view->onKeyPress(c) : ReturnCode::Unhandled;
	}

	ReturnCode setHover(bool isMouseOverMe) override
	{
		// Forward to all panes; if the mouse left the plugin entirely, all
		// three need to clear their hover state.
		if (view)              view->setHover(isMouseOverMe);
		if (moduleBrowser)     moduleBrowser->setHover(isMouseOverMe);
		if (propertiesBrowser) propertiesBrowser->setHover(isMouseOverMe);
		return ReturnCode::Ok;
	}

	ReturnCode populateContextMenu(gmpi::drawing::Point point, gmpi::api::IUnknown* contextMenuItemsSink) override
	{
		switch (paneAt(point))
		{
		case Pane::Browser:
			return moduleBrowser     ? moduleBrowser    ->populateContextMenu(toLocal(point, browserStrip),    contextMenuItemsSink) : ReturnCode::Unhandled;
		case Pane::Properties:
			return propertiesBrowser ? propertiesBrowser->populateContextMenu(toLocal(point, propertiesStrip), contextMenuItemsSink) : ReturnCode::Unhandled;
		case Pane::Editor:
		default:
			return populateEditorContextMenu(point, contextMenuItemsSink);
		}
	}

	// U3 — with the breadcrumb bar gone, the About pane and a jump-to-top live
	// here. Ruled 2026-08-20: "lets just rely on a context-menu 'goto parent' or
	// 'goto rack' for now ... same with 'about'."
	//
	// "Goto Parent..." is DELIBERATELY NOT ADDED HERE: the editor's own
	// background menu already has it (MfcDocPresenter.cpp:1353), greyed at the
	// top level via `container->Container()`. Measured rather than assumed --
	// libEditorLib.a carries the string and TIDE links it, so adding our own
	// would have put two identical items in the menu. So the ruling's "goto
	// parent" half was already satisfied and only "goto rack" is new.
	//
	// TIDE's items go FIRST and the view's follow, so they keep a stable
	// position no matter what the view offers for whatever was right-clicked.
	ReturnCode populateEditorContextMenu(gmpi::drawing::Point point, gmpi::api::IUnknown* contextMenuItemsSink)
	{
		bool addedOurs = false;

		gmpi::api::IContextItemSink* sinkRaw{};
		if (contextMenuItemsSink && contextMenuItemsSink->queryInterface(
				&gmpi::api::IContextItemSink::guid, reinterpret_cast<void**>(&sinkRaw)) == ReturnCode::Ok && sinkRaw)
		{
			// queryInterface returns a reference; attach so it is released.
			gmpi::shared_ptr<gmpi::api::IContextItemSink> sinkRef;
			sinkRef.attach(sinkRaw);

			ContextMenuHelper menu(sinkRaw);

			CContainer* rack = currentContainer;
			while (rack && rack->Container())
				rack = rack->Container();

			// GREYED at the rack, not omitted. The first cut omitted it, and
			// Jeff reported it simply missing -- correctly, because the rack is
			// where you right-click most and an item that is absent there reads
			// as absent everywhere. The editor's own "Goto Parent..." greys for
			// the same reason (MfcDocPresenter.cpp:1353,
			// `container->Container() ? 0 : GRAYED`), so this now matches the
			// item it sits beside. Greying also keeps it visible when
			// currentContainer is somehow null, which makes that failure
			// diagnosable instead of silent.
			const bool atRack = (!rack || rack == currentContainer);
			menu.addItem("Goto Rack", 0,
				[this, rack](int32_t) { if (rack) requestNavigate(rack); },
				atRack ? int32_t(gmpi::api::PopupMenuFlags::Grayed) : 0);
			addedOurs = true;

			menu.addItem("About TIDE...", [this](int32_t)
				{
					aboutPane.toggle();
					if (drawingHost) drawingHost->invalidateRect(nullptr);
				});
			addedOurs = true;

			menu.addSeparator();
		}

		const auto viewResult = view ? view->populateContextMenu(point, contextMenuItemsSink) : ReturnCode::Unhandled;

		// If the view had nothing to offer, the menu is still ours to show.
		return (viewResult == ReturnCode::Ok || addedOurs) ? ReturnCode::Ok : ReturnCode::Unhandled;
	}
};

void* newSynthEditGui() // see FactorySpecial
{
	return (void*) new SynthEditGui();
}
