#pragma once

// BACKLOG V7 — the hook that lets TIDE rename a context-menu item it does not
// own, without touching the code that creates it.
//
// WHY A HOOK AND NOT A RENAME. The items come straight from EditorLib's
// populateContextMenu, and EditorLib is shared with SynthEdit proper: renaming
// "Panel Edit..." there changes the menu for every existing SynthEdit user.
// Jeff ruled on 2026-08-26 that it must not — *"The context menu names make
// sense in synthedit, but need revising in TIDE where they don't entirly convey
// the meaning so well."* So SynthEdit keeps its names and TIDE overrides them,
// which requires somewhere to put the override. There was nowhere.
//
// WHY THE TABLE SHIPS EMPTY, and this is deliberate rather than unfinished.
// The same ruling left the four replacement strings open — they turn on what a
// TIDE user expects to read, which is a product decision, and it is filed as a
// `PROPOSED:` question in docs/decisions.md. V7 says it outright: *"Do not land
// the override carrying placeholder strings."* The row's own suggested scheme
// (Goto Panel / Goto Structure / ...) is SynthEdit vocabulary and is exactly the
// mismatch the ruling names, so guessing here would be worse than waiting.
//
// Filling in kOverrides is therefore a one-line change per name, once the
// question is answered, and nothing else moves.

#include <cstring>
#include <string>
#include <vector>

#include "helpers/NativeUi.h"
#include "RefCountMacros.h"

namespace tide
{

/// One rename: the text EditorLib emits, and what TIDE shows instead.
struct MenuNameOverride
{
	const char* from;
	const char* to;
};

/// EMPTY ON PURPOSE — see the header comment. Add entries here when the
/// `PROPOSED:` question in docs/decisions.md is answered.
///
/// Matching is on the WHOLE string, not a prefix or a substring: "Panel
/// Edit..." and "Pa&nel Edit..." are different items in EditorLib (the second
/// is the on-a-container variant and still carries an MFC accelerator), and a
/// substring rule would silently catch both with one entry and hide the fact
/// that there are two. If both should change, say so with two entries.
inline const std::vector<MenuNameOverride>& menuNameOverrides()
{
	static const std::vector<MenuNameOverride> kOverrides{};
	return kOverrides;
}

/// Look one up. Returns `text` unchanged when nothing matches, so the caller
/// never has to branch.
inline const char* overrideMenuName(const char* text)
{
	if (!text)
		return text;

	for (const auto& o : menuNameOverrides())
	{
		if (o.from && 0 == std::strcmp(text, o.from))
			return o.to ? o.to : text;
	}
	return text;
}

/// An IContextItemSink that forwards to another one, renaming as it goes.
///
/// TIDE hands this to `view->populateContextMenu` in place of the host's own
/// sink, so every item EditorLib adds passes through here on its way to the
/// menu. Nothing downstream knows, and nothing in EditorLib changes.
///
/// GMPI_REFCOUNT_NO_DELETE: it lives on the stack of the function that builds
/// one menu, and the sink it wraps outlives that. A refcount that could reach
/// zero would put a `delete` on a stack object the first time a callee released
/// its reference.
class RenamingContextItemSink final : public gmpi::api::IContextItemSink
{
	gmpi::api::IContextItemSink* inner_{};
	gmpi::api::IUnknown* innerUnknown_{};

public:
	/// `inner` receives the renamed items. `innerUnknown` is the object TIDE was
	/// originally given, and is what any interface OTHER than IContextItemSink
	/// is fetched from — see queryInterface.
	RenamingContextItemSink(gmpi::api::IContextItemSink* inner,
	                        gmpi::api::IUnknown* innerUnknown)
		: inner_(inner), innerUnknown_(innerUnknown)
	{
	}

	gmpi::ReturnCode addItem(const char* text, int32_t id, int32_t flags,
	                         gmpi::api::IUnknown* callback) override
	{
		if (!inner_)
			return gmpi::ReturnCode::Fail;

		return inner_->addItem(overrideMenuName(text), id, flags, callback);
	}

	// EVERY OTHER INTERFACE IS DELEGATED, and leaving that out would be a real
	// bug rather than a tidiness point: the object TIDE is handed also
	// implements IPopupMenu on some backends, and a caller that asks for it must
	// still get the genuine one. Only IContextItemSink is intercepted, because
	// only IContextItemSink carries the text.
	//
	// The consequence, stated because it is a real limit: a caller that reaches
	// the menu through IPopupMenu bypasses the rename. No current path does --
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
