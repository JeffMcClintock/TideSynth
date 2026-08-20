#pragma once
// TIDE Rack's about pane — TideSynth BACKLOG D6, built to the spec in
// TideSynth docs/about-pane.md (produced by D2, which merged D1's donation
// route and D2's credit into ONE surface rather than two).
//
// The spec's rules are load-bearing, so they are restated here next to the
// code that has to keep them:
//
//   1. Nothing appears unprompted — no splash, no first-run pane, no badge or
//      dot on the breadcrumb bar. The user goes looking for this.
//   2. Never a dialog and never a second window (constraints 1 and 5). It is a
//      pane drawn inside the editor, dismissed by clicking away from it.
//   3. Nothing here may block making sound: no modal loop, no audio touched.
//   4. No image assets — no logo, no hosted donate button, no QR bitmap.
//   5. The donation line degrades to text and never to nothing: the URL is
//      always shown as selectable-looking text, with Copy link offered wherever
//      the clipboard is reachable. A link that silently does nothing is worse
//      than plain text.
//   6. EXACTLY FOUR ITEMS. Adding a fifth needs a ruling recorded in
//      TideSynth docs/decisions.md — an about pane is where things get *put*,
//      so it is precisely the surface that accretes until it becomes the splash
//      screen PLAN forbids.
//
// Deliberately NOT copied: SynthEdit's own WaylandMainWindow::showAbout(),
// which has the right content but wraps it in SeMessageBoxAsync — a modal
// dialog, which rule 2 rules out. The content transfers; the container does not.

#include <array>
#include <string>
#include <string_view>

#include "Drawing.h"
#include "helpers/openurl.h"
#include "experimental/theme.h"

namespace tide
{

// Platform clipboard write. Returns false where there is no clipboard we can
// reach, and the pane then omits the Copy link button rather than offering one
// that silently does nothing (rule 5). Implemented for Apple in
// AboutPaneClipboard.mm; other platforms get the stub below until someone
// needs them.
bool setClipboardText(const std::string& utf8);

// True where setClipboardText can actually do something. Kept separate so the
// pane can decide whether to OFFER Copy link without writing to the clipboard
// to find out.
bool clipboardAvailable();

// ---- the four items, and nothing else (rule 6) ----

// 1. Product and version. TIDE has no release yet — the whole R2..R6 series is
// blocked on there being something to ship — so this is the honest placeholder
// and those rows own the real number when they land.
inline constexpr std::string_view kProductLine = "TIDE Rack — version 0.1 (unreleased)";

// 2. The credit, verbatim from R1(a) via the design doc. It names the
// ORGANISATION deliberately: TIDE Rack is a product of TIDE Synth, and TIDE
// Synth is SynthEdit Ltd's. Dropping the middle term is the one edit that
// defeats the purpose. This is also NOT the plug-in name or vendor string —
// those are P5's two fields and this pane is a third thing.
inline constexpr std::string_view kCreditLine = "TIDE Synth — by SynthEdit Ltd";

// 3. Donation. Lowercase spelling: Ko-fi canonicalises to lowercase and both
// spellings reach the same page (verified in a browser — Ko-fi 403s curl).
inline constexpr std::string_view kDonationUrl = "https://ko-fi.com/tiderack";

// 4. Licence and where the source lives — one line, not a licence dump. ISC
// was chosen by Jeff (BACKLOG L1); the LICENSE file is in the TideSynth repo.
inline constexpr std::string_view kLicenceLine = "ISC licence — github.com/JeffMcClintock/TideSynth";

struct AboutPane
{
    static constexpr float kWidth      = 440.0f;
    static constexpr float kHeight     = 168.0f;
    static constexpr float kPad        = 18.0f;
    static constexpr float kLineHeight = 26.0f;
    static constexpr float kFontHeight = 13.0f;
    static constexpr float kRadius     = 6.0f;

    bool isOpen = false;

    // Rects recomputed every render, in plugin-local coords, so hit-testing
    // always matches what was actually drawn.
    gmpi::drawing::Rect paneRect{};
    gmpi::drawing::Rect urlRect{};
    gmpi::drawing::Rect copyRect{};
    gmpi::drawing::Rect closeRect{};
    bool copyOffered = clipboardAvailable(); // false => button omitted, never a dead one
    bool copiedRecently = false; // swaps the button's label to "Copied" as feedback

    gmpi::drawing::TextFormat textFormat;
    gmpi::drawing::TextFormat textFormatBold;

    void toggle() { isOpen = !isOpen; copiedRecently = false; }
    void close()  { isOpen = false; copiedRecently = false; }

    // Centre the pane over the area it is drawn into.
    void layout(const gmpi::drawing::Rect& area)
    {
        const float cx = 0.5f * (area.left + area.right);
        const float cy = 0.5f * (area.top + area.bottom);
        paneRect = {
            cx - 0.5f * kWidth, cy - 0.5f * kHeight,
            cx + 0.5f * kWidth, cy + 0.5f * kHeight };
    }

    void render(gmpi::drawing::Graphics& g, const gmpi::drawing::Rect& area)
    {
        if (!isOpen)
            return;

        layout(area);

        const auto& theme = gmpi::ui::currentTheme();

        if (!textFormat)
        {
            const std::array<std::string_view, 3> families{ "Selawik", "Segoe UI", "Arial" };
            textFormat = g.getFactory().createTextFormat(kFontHeight, families);
            textFormat.setWordWrapping(gmpi::drawing::WordWrapping::NoWrap);
            textFormat.setParagraphAlignment(gmpi::drawing::ParagraphAlignment::Center);

            textFormatBold = g.getFactory().createTextFormat(
                kFontHeight + 1.0f, families, gmpi::drawing::FontWeight::Bold);
            textFormatBold.setWordWrapping(gmpi::drawing::WordWrapping::NoWrap);
            textFormatBold.setParagraphAlignment(gmpi::drawing::ParagraphAlignment::Center);
        }

        // A soft scrim over the editor: it signals "this is transient" and
        // gives the pane contrast over both the dark rack case and the light
        // structure view. It is NOT a modal — the click that lands on it
        // dismisses the pane (see onPointerDown).
        {
            auto scrim = g.createSolidColorBrush(gmpi::drawing::Color{ 0.0f, 0.0f, 0.0f, 0.35f });
            g.fillRectangle(area, scrim);
        }

        auto panelBrush  = g.createSolidColorBrush(theme.controlBackground);
        auto borderBrush = g.createSolidColorBrush(theme.controlText);
        auto textBrush   = g.createSolidColorBrush(theme.controlText);

        gmpi::drawing::Color dim = theme.controlText;
        dim.a *= 0.65f;
        auto dimBrush = g.createSolidColorBrush(dim);

        // Link colour: the theme has no dedicated accent, so lift the text
        // colour toward blue rather than inventing a palette entry.
        gmpi::drawing::Color linkColor{ 0.35f, 0.60f, 0.95f, 1.0f };
        auto linkBrush = g.createSolidColorBrush(linkColor);

        const gmpi::drawing::RoundedRect panel{ paneRect, kRadius, kRadius };
        g.fillRoundedRectangle(panel, panelBrush);
        g.drawRoundedRectangle(panel, dimBrush, 1.0f);

        float y = paneRect.top + kPad;
        const float x0 = paneRect.left + kPad;
        const float x1 = paneRect.right - kPad;

        auto lineRect = [&](float h) {
            const gmpi::drawing::Rect r{ x0, y, x1, y + h };
            y += h;
            return r;
        };

        // 1. Product and version — the line people quote in bug reports.
        g.drawTextU(kProductLine, textFormatBold, lineRect(kLineHeight), textBrush);

        // 2. The credit.
        g.drawTextU(kCreditLine, textFormat, lineRect(kLineHeight), dimBrush);

        y += 6.0f;

        // 3. Donation: the URL as text, clickable where a browser can be
        // opened, with Copy link beside it.
        urlRect = { x0, y, x0 + 210.0f, y + kLineHeight };
        g.drawTextU(kDonationUrl, textFormat, urlRect, linkBrush);
        {
            // Underline, drawn rather than styled: it is the affordance that
            // says "this opens something".
            const float uy = urlRect.bottom - 6.0f;
            g.drawLine({ urlRect.left, uy }, { urlRect.left + 148.0f, uy }, linkBrush, 1.0f);
        }

        if (copyOffered)
        {
            copyRect = { urlRect.right + 8.0f, y + 2.0f, urlRect.right + 96.0f, y + kLineHeight - 2.0f };
            const gmpi::drawing::RoundedRect btn{ copyRect, 3.0f, 3.0f };
            g.drawRoundedRectangle(btn, dimBrush, 1.0f);

            auto centred = textFormat;
            centred.setTextAlignment(gmpi::drawing::TextAlignment::Center);
            g.drawTextU(copiedRecently ? "Copied" : "Copy link", centred, copyRect, dimBrush);
        }
        else
        {
            copyRect = {};
        }
        y += kLineHeight + 6.0f;

        // 4. Licence and source.
        g.drawTextU(kLicenceLine, textFormat, lineRect(kLineHeight), dimBrush);

        // Dismiss affordance. Clicking anywhere outside the pane also closes
        // it; this is for the user who looks for an X.
        closeRect = { paneRect.right - 26.0f, paneRect.top + 8.0f, paneRect.right - 8.0f, paneRect.top + 26.0f };
        {
            auto x_ = closeRect;
            g.drawLine({ x_.left + 4.0f, x_.top + 4.0f }, { x_.right - 4.0f, x_.bottom - 4.0f }, dimBrush, 1.4f);
            g.drawLine({ x_.right - 4.0f, x_.top + 4.0f }, { x_.left + 4.0f, x_.bottom - 4.0f }, dimBrush, 1.4f);
        }
    }

    // Returns true if the pane consumed the click. The pane is not modal, but
    // while it is open it gets first refusal so a click meant for "dismiss"
    // never also lands on the rack behind it.
    bool onPointerDown(gmpi::drawing::Point p)
    {
        if (!isOpen)
            return false;

        auto hit = [&](const gmpi::drawing::Rect& r) {
            return p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom;
        };

        if (hit(closeRect))
        {
            close();
            return true;
        }

        if (copyOffered && hit(copyRect))
        {
            copiedRecently = setClipboardText(std::string(kDonationUrl));
            return true;
        }

        if (hit(urlRect))
        {
            // D1's ruling: open_url, never browse_to — revealing a file in
            // Finder is a different operation with a worse sandbox story.
            gmpi::open_url(std::string(kDonationUrl));
            return true;
        }

        if (hit(paneRect))
            return true; // swallow clicks on the pane body

        close(); // click outside dismisses
        return true;
    }
};

} // namespace tide
