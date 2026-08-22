// BACKLOG S24 -- the mac half of the module-arm cross cursor.
//
// TIDE's cross-cursor affordance shortcuts straight to the OS because GMPI
// exposes no setCursor on IInputHost (SynthEditGui.cpp's own comment). On
// Windows that shortcut is ::SetCursor(IDC_CROSS), re-asserted per pointer
// move; this file is the exact analogue for AppKit. Same design, deliberately:
// a per-platform one-liner beats inventing a cursor API in shared code for
// one affordance (the Wayland app solves it with its toplevel's own setCursor,
// which gmpi_ui's mac frame does not have).
//
// NSCursor `set` persists until something else sets a cursor, so unlike Win32
// there is nothing fighting us between events -- but re-asserting per move is
// still correct: AppKit's cursor-rect machinery (window edges, text fields)
// resets the cursor on its own schedule, exactly like WM_SETCURSOR.
//
// Main thread only, which pointer events and OnNotify already are.

#ifdef __APPLE__

#include <TargetConditionals.h>

// __APPLE__ IS DEFINED ON iOS TOO, and AppKit is macOS-only, so the real
// implementation needs the narrower test. iOS gets an empty one rather than
// being excluded from the build: SynthEditGui.cpp calls this unconditionally
// on pointer move, and making the CALLER platform-aware would spread the
// condition across shared code for an affordance that simply does not exist
// on a touch screen. There is no cursor to show.
#if TARGET_OS_OSX

#import <AppKit/AppKit.h>

void tideShowCrossCursor(bool show)
{
    if (show)
        [[NSCursor crosshairCursor] set];
    else
        [[NSCursor arrowCursor] set];
}

#else

void tideShowCrossCursor(bool /*show*/)
{
}

#endif // TARGET_OS_OSX

#endif
