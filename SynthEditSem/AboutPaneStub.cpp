// Non-Apple fallback for the about pane's clipboard write (TideSynth BACKLOG
// D6). Returning false is not a failure mode — it is the honest answer, and the
// pane then omits "Copy link" entirely rather than offering a button that
// silently does nothing (design doc rule 5: the donation line degrades to text,
// never to nothing).
//
// Windows (OpenClipboard/SetClipboardData) and Linux (the X11/Wayland clipboard
// helpers already in gmpi_ui/backends) are both reachable; neither is wired
// because neither box could verify it this session.

#include <string>

namespace tide
{

bool clipboardAvailable()
{
	return false;
}

bool setClipboardText(const std::string&)
{
	return false;
}

} // namespace tide
