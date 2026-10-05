#pragma once

// TIDE requires a same-process controller -- ruled by Jeff 2026-10-06, BACKLOG
// E86. The module factory is populated only from TideApp::InitInstance, which
// the CONTROLLER reaches via initialize(), so a processor created without one
// builds a rack of zero modules and reports success. This flag is what lets the
// processor say so instead.
//
// Header-only on purpose: an inline function's local static is one object across
// translation units, so neither file has to link against the other. If TideApp
// is not in the binary at all the flag simply stays false, which is the correct
// answer rather than a link error.

namespace tide
{

inline bool& controllerInitialisedFlag()
{
	static bool initialised = false;
	return initialised;
}

// True once a TideApp has run InitInstance in this process.
inline bool controllerInitialised()
{
	return controllerInitialisedFlag();
}

} // namespace tide
