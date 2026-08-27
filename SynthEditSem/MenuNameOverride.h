#pragma once

// BACKLOG V7 — the hook that lets TIDE change a context menu it does not own,
// without touching the code that builds it.
//
// WHY A HOOK AND NOT AN EDIT. The items come straight from EditorLib's
// populateContextMenu, and EditorLib is shared with SynthEdit proper: deleting
// "Locked" there deletes it for every existing SynthEdit user. Jeff ruled on
// 2026-08-26 that it must not — *"The context menu names make sense in
// synthedit, but need revising in TIDE where they don't entirly convey the
// meaning so well."* So SynthEdit keeps its menu and TIDE filters a copy on the
// way out.
//
// WHAT CHANGED 2026-08-27. The V7 ruling was answered, and the answer was not
// the question. V7 asked what to RENAME four navigation items to; Jeff replied
// with a specification of what to REMOVE, per context, plus one item to add:
//
//   1. Right click on top-level rack view: remove "Arrange", "Skin", "Locked",
//      "Goto Structure...".
//   2. Clicking a rack module container at the top level: remove
//      "Delete (keep wires)". Add "Show Circuit" which opens the structure view
//      of the module prefab container (not the top-level container).
//   3. On the structure View: remove "Arrange", "Screen shot". Also in Release
//      build only, remove "Panel Edit", "Goto Parent".
//
// So the rename table is gone and this is a FILTER. Recorded in
// docs/decisions.md; the corrections that survived contact with the code are
// below, because each one would otherwise be rediscovered by reading the
// ruling and then failing to find the string.
//
// FOUR CORRECTIONS, ALL MEASURED AGAINST MfcDocPresenter.cpp.
//
//  * "Arrange" and "Skin" are SUBMENUS, not items — `menu.beginSubMenu("&Arrange")`
//    at :1112 and `"&Skin"` at :1134. They reach a sink as an ordinary addItem
//    carrying PopupMenuFlags::SubMenuBegin (ContextMenuHelper.h:28), so a filter
//    CAN drop one — but only by swallowing every item up to the matching
//    SubMenuEnd. Dropping the begin alone would splice the submenu's contents
//    into the parent menu and leave a stray end marker.
//
//  * The string is "Screenshot", one word, and it is ALREADY `#if defined(_DEBUG)`
//    (:1366) so a Release build never emits it. Filtered unconditionally anyway,
//    which is what the ruling asks and is a no-op in Release.
//
//  * "Panel Edit" and "Goto Parent" each exist TWICE, in MUTUALLY EXCLUSIVE
//    branches of `if (moduleHandle >= 0) … else …`: "Pa&nel Edit..." / "Goto
//    Parent Container" when a module is under the pointer, "Panel Edit..." /
//    "Goto Parent..." on the background. A user only ever sees one of each,
//    which is exactly what Jeff said when this was put to him as a choice — so
//    all four strings are listed and there is no choice to make. The ampersands
//    are MFC accelerators and are part of the string; matching is on the WHOLE
//    string, never a substring, or one entry would silently catch both variants
//    and hide the fact that there are two.
//
//  * "Delete (keep wires)" (:1106) is added BEFORE the view-type branch, so it
//    appears in the structure view too. The ruling removes it from the rack
//    only, so it is view-scoped rather than unconditional.
//
// WHAT THIS DELIBERATELY DOES NOT DO. It never renames anything any more, and
// it adds nothing. "Show Circuit" is added by SynthEditGui, not here, because it
// needs the module under the pointer and a filter only ever sees text.

#include <cstring>
#include <string>
#include <vector>

#include "helpers/NativeUi.h"
#include "RefCountMacros.h"

namespace tide
{

/// Which of EditorLib's two menus is being built. The ruling is per-context and
/// three of the ten rules below are not unconditional, so the sink has to be
/// told; it cannot infer this from the text alone.
///
/// Mirrors CF_PANEL_VIEW / CF_STRUCTURE_VIEW rather than naming the rack,
/// because that is the distinction EditorLib itself branches on
/// (`viewType == CF_PANEL_VIEW`). In TIDE the master's panel view IS the rack.
enum class MenuView
{
	Panel,      ///< the rack, or any container's panel view
	Structure,  ///< the wiring inside a container
};

/// One suppression rule.
struct MenuSuppression
{
	const char* text;         ///< exact match, ampersands included
	bool        panelView;    ///< drop it when the panel/rack menu is being built
	bool        structureView;///< drop it when the structure menu is being built
	bool        releaseOnly;  ///< drop it only in a Release build
};

/// The ruling of 2026-08-27, as a table. Adding a row is the whole of changing
/// what TIDE hides.
inline const std::vector<MenuSuppression>& menuSuppressions()
{
	static const std::vector<MenuSuppression> kRules{
		// text                      panel  structure  releaseOnly
		{ "&Arrange",                true,  true,      false },  // submenu
		{ "&Skin",                   true,  false,     false },  // submenu
		{ "Locked",                  true,  false,     false },
		{ "Goto Structure...",       true,  false,     false },
		{ "Delete (keep wires)",     true,  false,     false },
		{ "Screenshot",              true,  true,      false },
		{ "Pa&nel Edit...",          true,  true,      true  },
		{ "Panel Edit...",           true,  true,      true  },
		{ "Goto Parent Container",   true,  true,      true  },
		{ "Goto Parent...",          true,  true,      true  },
	};
	return kRules;
}

/// Should `text` be dropped from the menu `view` is building?
inline bool suppressMenuItem(const char* text, MenuView view)
{
	if (!text)
		return false;

	for (const auto& r : menuSuppressions())
	{
		if (!r.text || 0 != std::strcmp(text, r.text))
			continue;

		if (r.releaseOnly)
		{
#ifdef _DEBUG
			continue;   // a developer keeps the item
#endif
		}

		return view == MenuView::Panel ? r.panelView : r.structureView;
	}
	return false;
}

/// An IContextItemSink that forwards to another one, dropping what the ruling
/// says TIDE does not show.
///
/// TIDE hands this to `view->populateContextMenu` in place of the host's own
/// sink, so every item EditorLib adds passes through here on its way to the
/// menu. Nothing downstream knows, and nothing in EditorLib changes.
///
/// GMPI_REFCOUNT_NO_DELETE: it lives on the stack of the function that builds
/// one menu, and the sink it wraps outlives that. A refcount that could reach
/// zero would put a `delete` on a stack object the first time a callee released
/// its reference.
class FilteringContextItemSink final : public gmpi::api::IContextItemSink
{
	gmpi::api::IContextItemSink* inner_{};
	gmpi::api::IUnknown* innerUnknown_{};
	MenuView view_{ MenuView::Structure };

	/// >0 while inside a submenu whose BEGIN was suppressed. Counts nesting so a
	/// submenu inside a dropped submenu cannot end the swallow early.
	int swallowDepth_ = 0;

	/// A separator that has been seen but not yet forwarded. Held back because
	/// EditorLib groups its items with separators, and removing a whole group
	/// otherwise leaves the separator that introduced it — a menu that opens
	/// with a rule, or ends with one, or shows two in a row. Flushed only when a
	/// real item follows, which collapses runs and drops leading ones; a
	/// trailing one is simply never flushed.
	bool pendingSeparator_ = false;

	/// Nothing has been forwarded yet, so a separator now would be a leading one.
	bool emittedAnything_ = false;

public:
	/// `inner` receives the surviving items. `innerUnknown` is the object TIDE
	/// was originally given, and is what any interface OTHER than
	/// IContextItemSink is fetched from — see queryInterface.
	FilteringContextItemSink(gmpi::api::IContextItemSink* inner,
	                         gmpi::api::IUnknown* innerUnknown,
	                         MenuView view)
		: inner_(inner), innerUnknown_(innerUnknown), view_(view)
	{
	}

	gmpi::ReturnCode addItem(const char* text, int32_t id, int32_t flags,
	                         gmpi::api::IUnknown* callback) override
	{
		if (!inner_)
			return gmpi::ReturnCode::Fail;

		const bool isBegin     = 0 != (flags & (int32_t)gmpi::api::PopupMenuFlags::SubMenuBegin);
		const bool isEnd       = 0 != (flags & (int32_t)gmpi::api::PopupMenuFlags::SubMenuEnd);
		const bool isSeparator = 0 != (flags & (int32_t)gmpi::api::PopupMenuFlags::Separator);

		// --- inside a dropped submenu -------------------------------------
		if (swallowDepth_ > 0)
		{
			if (isBegin)
				++swallowDepth_;
			else if (isEnd)
				--swallowDepth_;

			// Reported as accepted. The caller is EditorLib building a menu it
			// believes in, and a Fail here would look to it like the sink had
			// broken rather than like TIDE having an opinion.
			return gmpi::ReturnCode::Ok;
		}

		// --- separators are deferred, never dropped outright ---------------
		if (isSeparator)
		{
			pendingSeparator_ = true;
			return gmpi::ReturnCode::Ok;
		}

		// --- the ruling ----------------------------------------------------
		if (suppressMenuItem(text, view_))
		{
			if (isBegin)
				swallowDepth_ = 1;
			return gmpi::ReturnCode::Ok;
		}

		if (pendingSeparator_)
		{
			pendingSeparator_ = false;
			if (emittedAnything_)
				inner_->addItem("", 0, (int32_t)gmpi::api::PopupMenuFlags::Separator, nullptr);
		}

		emittedAnything_ = true;
		return inner_->addItem(text, id, flags, callback);
	}

	// EVERY OTHER INTERFACE IS DELEGATED, and leaving that out would be a real
	// bug rather than a tidiness point: the object TIDE is handed also
	// implements IPopupMenu on some backends, and a caller that asks for it must
	// still get the genuine one. Only IContextItemSink is intercepted, because
	// only IContextItemSink carries the text.
	//
	// The consequence, stated because it is a real limit: a caller that reaches
	// the menu through IPopupMenu bypasses the filter. No current path does --
	// populateContextMenu is handed IContextItemSink -- but a future one could,
	// and it would fail silently.
	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		*returnInterface = {};

		if (*iid == gmpi::api::IContextItemSink::guid || *iid == gmpi::api::IUnknown::guid)
		{
			*returnInterface = static_cast<gmpi::api::IContextItemSink*>(this);
			addRef();
			return gmpi::ReturnCode::Ok;
		}

		if (innerUnknown_)
			return innerUnknown_->queryInterface(iid, returnInterface);

		return gmpi::ReturnCode::NoSupport;
	}

	GMPI_REFCOUNT_NO_DELETE;
};

} // namespace tide
