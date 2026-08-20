// Clipboard write for the about pane's "Copy link" (TideSynth BACKLOG D6).
//
// D1 measured both Apple pasteboards extension-legal — UIPasteboard and
// NSPasteboard compile clean under -fapplication-extension, and UIPasteboard.h
// carries zero NS_EXTENSION_UNAVAILABLE annotations — which is why "URL as text
// plus Copy link" is the donation route's floor: it works even where opening a
// URL does not.
//
// Platform split follows the same rule as gmpi_ui's browseto/openurl helpers
// (BACKLOG D3): choose the framework here on TARGET_OS_OSX rather than making
// each consumer's CMake do it, so the TU compiles on every Apple target.

#include <string>
#include <TargetConditionals.h>

#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

namespace tide
{

bool clipboardAvailable()
{
    return true; // both Apple pasteboards are always present
}

bool setClipboardText(const std::string& utf8)
{
    @autoreleasepool {
        NSString* text = [NSString stringWithUTF8String:utf8.c_str()];
        if (!text)
            return false;

#if TARGET_OS_OSX
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        [pb clearContents];
        return [pb setString:text forType:NSPasteboardTypeString] == YES;
#else
        [UIPasteboard generalPasteboard].string = text;
        return true;
#endif
    }
}

} // namespace tide
