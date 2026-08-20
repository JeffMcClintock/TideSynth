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

#import <AppKit/AppKit.h>

void tideShowCrossCursor(bool show)
{
    if (show)
        [[NSCursor crosshairCursor] set];
    else
        [[NSCursor arrowCursor] set];
}

#endif
