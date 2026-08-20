// Replaces SynthEdit2/SynthEditApp.cpp in the TIDE target.
//
// TIDE used to compile the whole of SynthEditApp.cpp purely to satisfy three
// symbols that EditorLib references. That dragged SynthEditApp::InitInstance()
// into the shipping plugin, and with it a LoadOrScanModuleData() call site, a
// detached MonitorFileSystem() watcher thread on the live-modules folder, the
// SynthEdit16.settings.xml read/write path, and the whole Moonbase licensing
// and activation-polling surface. None of it is reachable from TideApp — TIDE
// calls TideApp::InitInstance(), never SynthEditApp's — but "linked but not
// called" is exactly what BACKLOG S1b exists to stop, and it kept the plugin
// one stray call away from scanning the user's disk. See docs/module-enumeration.md.
//
// Behaviour is unchanged, not merely similar. SynthEditApp's constructor is
// what assigns the `theApp` global, and TideApp derives from CSynthEditAppBase,
// not SynthEditApp — so in a TIDE process `theApp` was always null and all three
// of these already did nothing:
//
//   SafeMessagebox        guarded on `if (theApp)`, so a no-op.
//   isMoonbaseEnabled()   `return false` — TIDE never defines SE_MOONBASE_SUPPORT.
//   licenseIsActive()     `return false`, same reason.
//
// The one TIDE-reachable caller of the latter two, MfcDocPresenter.cpp:1244,
// reads `theApp && theApp->isMoonbaseEnabled() && !theApp->licenseIsActive()`
// and was already dead on the first term.
//
// These are members of a class TIDE does not own. Defining them here rather
// than editing SynthEdit2/SynthEditApp.{h,cpp} is deliberate: those files are
// GATED behind the carve-out (C0). If SynthEditApp's declarations change, this
// file breaks at link time, which is the failure mode we want.

#include "SafeMessageBox.h"
#include "ILicenseState.h" // C16 -- came in transitively via SynthEditApp.h
                           // before; it lives in the PUBLIC SynthEditLib,
                           // which is the whole point of the change.

// C16, 2026-08-20. This file used to include SynthEdit2/SynthEditApp.h -- the
// LAST private include in TIDE's own source, and the one thing keeping a
// standalone TideSynth from configuring. It is gone, and so are the three
// symbols it existed for: the `theApp` global and stub definitions of
// SynthEditApp::isMoonbaseEnabled() / ::licenseIsActive().
//
// They were dead, and C11 is why. This file's header comment describes a
// pre-C11 world where EditorLib reached for SynthEditApp directly; C11 replaced
// that with ILicenseState, and GetLicenseState() below is what EditorLib
// actually calls now. Measured before deleting: neither `theApp` nor either
// member is referenced anywhere in EditorLib or SynthEditLib. Every surviving
// caller of those members is desktop-app code (SynthEditMac/, SynthEdit2/),
// which links the real SynthEditApp.cpp and never this file.
//
// If a future change makes EditorLib want `theApp` again, note it needs only a
// FORWARD DECLARATION here, not the header: a pointer definition does not
// require a complete type, and a global's mangled name carries no type in the
// Itanium ABI, so the symbol is identical either way.

void SafeMessagebox(
	void* /*hWnd*/,
	const wchar_t* /*lpText*/,
	const wchar_t* /*lpCaption*/,
	int /*uType*/
)
{
	// TIDE has no modal dialogs (PLAN constraint 5). The desktop implementation
	// forwards to theApp->SeMessageBox() only when theApp is set, so this was
	// already silent here. Callers are EditorLib's CopyFileWithReplace and the
	// module-scan paths in ModuleFactory_Editor -- none of which TIDE reaches.
	// BACKLOG S3 covers making those paths unreachable rather than merely quiet.
}

// BACKLOG C11. TIDE needs no licensing (Jeff, 2026-08-15): rather than
// implement ILicenseState via SynthEditApp's stubbed-out methods above,
// GetLicenseState() returns nullptr outright, so MfcDocPresenter.cpp's gate
// never even asks the question. theApp is always null in a TIDE process
// (see the file header comment) so this is `return theApp;` in every way
// that matters, but spelling it out avoids relying on that indirection to
// carry the "TIDE has nothing to gate" decision.
ILicenseState* GetLicenseState()
{
	return nullptr;
}
