/* v7_menu_override_probe — does TIDE's context-menu filter do what the ruling says?
 *
 * BACKLOG V7. TIDE cannot edit the menu it shows: the items come from
 * EditorLib's populateContextMenu, which is shared with SynthEdit proper, and
 * Jeff ruled 2026-08-26 that SynthEdit's menu must not change. So TIDE wraps
 * the sink and filters a copy on the way out.
 *
 * WHY A PROBE AND NOT A SCREENSHOT, and this is not a preference. The command
 * channel cannot raise a context menu at all (E38: `--right` sets the flag but
 * the menu is raised by the FRAME, which the dispatcher never touches), and on
 * macOS trying to open one from a command WEDGES the app (E43: a native NSMenu
 * runs a nested modal loop inside the job). `--screenshot` could not see it
 * either -- it reads the app's own render buffer and a macOS popup is a
 * separate window. So the far side of the wrapper is the only place this
 * behaviour is observable at all, and that is what this drives.
 *
 * A hook nobody has watched work is not a hook. Every rule the ruling states
 * has a case below, and so does every way the filter could be wrong in a
 * direction that still looks fine: a near-miss string, a submenu spliced open
 * instead of removed, a separator left behind by the group it introduced.
 *
 * BUILD (no CMake, no GMPI, no plugin -- one header and the standard library)
 *   clang++ -std=c++20 tests/v7_menu_override_probe.cpp \
 *     -I SynthEditSem -I build/_deps/gmpi_ui-src -I build/_deps/gmpi-src/Core \
 *     -o v7_menu_override_probe
 *
 * RUN
 *   ./v7_menu_override_probe        # exit 0 = the filter matches the ruling
 */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "MenuNameOverride.h"

namespace
{

int failures = 0;

void check(const char* what, bool ok)
{
    std::printf("  %-4s %s\n", ok ? "ok" : "FAIL", what);
    if (!ok)
        ++failures;
}

/// Records what reached it, so the test can assert on the far side of the
/// wrapper rather than on the wrapper's own opinion of what it did.
struct RecordingSink : gmpi::api::IContextItemSink
{
    struct Item
    {
        std::string text;
        int32_t id;
        int32_t flags;
        gmpi::api::IUnknown* callback;
    };
    std::vector<Item> items;

    // A GUID this object claims that IContextItemSink is not, so the
    // delegation half of queryInterface has something real to find.
    inline static const gmpi::api::Guid probeGuid =
    { 0x11111111, 0x2222, 0x3333, { 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb } };

    gmpi::ReturnCode addItem(const char* text, int32_t id, int32_t flags,
                             gmpi::api::IUnknown* callback) override
    {
        items.push_back({ text ? text : "", id, flags, callback });
        return gmpi::ReturnCode::Ok;
    }

    gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
    {
        *returnInterface = {};
        if (*iid == gmpi::api::IContextItemSink::guid || *iid == gmpi::api::IUnknown::guid
            || *iid == probeGuid)
        {
            *returnInterface = static_cast<gmpi::api::IContextItemSink*>(this);
            return gmpi::ReturnCode::Ok;
        }
        return gmpi::ReturnCode::NoSupport;
    }

    int32_t addRef() override  { return 1; }
    int32_t release() override { return 1; }
};

constexpr int32_t kBegin = (int32_t)gmpi::api::PopupMenuFlags::SubMenuBegin;
constexpr int32_t kEnd   = (int32_t)gmpi::api::PopupMenuFlags::SubMenuEnd;
constexpr int32_t kSep   = (int32_t)gmpi::api::PopupMenuFlags::Separator;

/// Was `text` forwarded?
bool has(const RecordingSink& r, const char* text)
{
    for (const auto& i : r.items)
        if (i.text == text)
            return true;
    return false;
}

std::string joined(const RecordingSink& r)
{
    std::string s;
    for (const auto& i : r.items)
    {
        if (!s.empty()) s += " | ";
        s += i.flags & kSep ? "<sep>" : i.text;
    }
    return s;
}

} // namespace

int main()
{
    // ---------------------------------------------------------------------
    // 1. The rack menu, replayed exactly as MfcDocPresenter builds it.
    //
    // The order and the flags are copied from the real call sites, because a
    // filter that works on a tidied-up sequence and not on the real one is
    // worth nothing. Cut/Copy/Paste/Delete (:1075-1097), "Delete (keep wires)"
    // (:1106, added BEFORE the view-type branch), the &Arrange submenu
    // (:1112-1127), then the panel-view block: &Skin (:1134-1196), "Locked"
    // (:1199), a separator and "Goto Structure..." (:1226).
    // ---------------------------------------------------------------------
    {
        std::printf("rack (panel) menu\n");
        RecordingSink rec;
        tide::FilteringContextItemSink f(&rec, &rec, tide::MenuView::Panel);

        f.addItem("Cut", 0, 0, nullptr);
        f.addItem("Copy", 0, 0, nullptr);
        f.addItem("Paste", 0, 0, nullptr);
        f.addItem("Delete", 0, 0, nullptr);
        f.addItem("Delete (keep wires)", 0, 0, nullptr);
        f.addItem("&Arrange", 0, kBegin, nullptr);
        f.addItem("Move to Front", 0, 0, nullptr);
        f.addItem("Move to Back", 0, 0, nullptr);
        f.addItem("", 0, kEnd, nullptr);
        f.addItem("&Skin", 0, kBegin, nullptr);
        f.addItem("Bright", 0, 0, nullptr);
        f.addItem("Open Global Skins...", 0, 0, nullptr);
        f.addItem("", 0, kEnd, nullptr);
        f.addItem("Locked", 0, 0, nullptr);
        f.addItem("", 0, kSep, nullptr);
        f.addItem("Goto Structure...", 0, 0, nullptr);

        std::printf("       -> %s\n", joined(rec).c_str());

        check("Cut/Copy/Paste/Delete survive",
              has(rec, "Cut") && has(rec, "Copy") && has(rec, "Paste") && has(rec, "Delete"));
        check("\"Delete (keep wires)\" is gone",   !has(rec, "Delete (keep wires)"));
        check("\"Locked\" is gone",                !has(rec, "Locked"));
        check("\"Goto Structure...\" is gone",     !has(rec, "Goto Structure..."));
        check("the &Arrange submenu is gone",      !has(rec, "&Arrange"));
        check("the &Skin submenu is gone",         !has(rec, "&Skin"));

        // The submenu CONTENTS are the half a naive filter gets wrong: dropping
        // only the begin marker splices "Move to Front" into the parent menu,
        // where it looks like a deliberate top-level item.
        check("...and so are its contents (Move to Front/Back)",
              !has(rec, "Move to Front") && !has(rec, "Move to Back"));
        check("...and so are the skins inside it",
              !has(rec, "Bright") && !has(rec, "Open Global Skins..."));

        // A stray end marker would close a submenu nobody opened.
        int ends = 0;
        for (const auto& i : rec.items) if (i.flags & kEnd) ++ends;
        check("no orphaned SubMenuEnd is forwarded", ends == 0);

        // The separator introduced "Goto Structure...", which is gone.
        check("the separator its group left behind is gone too",
              rec.items.empty() || !(rec.items.back().flags & kSep));
    }

    // ---------------------------------------------------------------------
    // 2. The structure menu. Same filter, different verdict on two items --
    //    which is the whole reason the sink is told which menu it is filtering.
    // ---------------------------------------------------------------------
    {
        std::printf("structure menu\n");
        RecordingSink rec;
        tide::FilteringContextItemSink f(&rec, &rec, tide::MenuView::Structure);

        f.addItem("Cut", 0, 0, nullptr);
        f.addItem("Delete (keep wires)", 0, 0, nullptr);
        f.addItem("&Arrange", 0, kBegin, nullptr);
        f.addItem("Move to Front", 0, 0, nullptr);
        f.addItem("", 0, kEnd, nullptr);
        f.addItem("", 0, kSep, nullptr);
        f.addItem("Panel Edit...", 0, 0, nullptr);
        f.addItem("Goto Parent...", 0, 0, nullptr);
        f.addItem("Screenshot", 0, 0, nullptr);

        std::printf("       -> %s\n", joined(rec).c_str());

        check("\"Delete (keep wires)\" SURVIVES here, unlike at the rack",
              has(rec, "Delete (keep wires)"));
        check("the &Arrange submenu is gone here too", !has(rec, "&Arrange"));
        check("\"Screenshot\" is gone",               !has(rec, "Screenshot"));

        // The one pair of rules that differ by BUILD rather than by view. Both
        // arms are asserted, so a Debug run is a real test and not a skip.
#ifdef _DEBUG
        check("Debug KEEPS \"Panel Edit...\"",  has(rec, "Panel Edit..."));
        check("Debug KEEPS \"Goto Parent...\"", has(rec, "Goto Parent..."));
#else
        check("Release drops \"Panel Edit...\"",  !has(rec, "Panel Edit..."));
        check("Release drops \"Goto Parent...\"", !has(rec, "Goto Parent..."));
#endif
    }

    // ---------------------------------------------------------------------
    // 3. Exact match, never a substring.
    //
    // "Pa&nel Edit..." and "Panel Edit..." are DIFFERENT EditorLib items in
    // mutually exclusive branches -- the ampersand is an MFC accelerator, not
    // noise. Both are listed in the table on purpose. A substring rule would
    // catch both with one entry, look correct, and hide that there are two.
    // ---------------------------------------------------------------------
    {
        std::printf("matching\n");
        RecordingSink rec;
        tide::FilteringContextItemSink f(&rec, &rec, tide::MenuView::Structure);

        f.addItem("Pa&nel Edit...", 0, 0, nullptr);
        f.addItem("Goto Parent Container", 0, 0, nullptr);
        f.addItem("Locked Groove", 0, 0, nullptr);      // near miss, must survive
        f.addItem("Arrange", 0, 0, nullptr);            // no ampersand: not the submenu
        f.addItem("Show Circuit", 0, 0, nullptr);       // TIDE's own, never filtered

        std::printf("       -> %s\n", joined(rec).c_str());

#ifdef _DEBUG
        check("Debug KEEPS the on-a-module variants",
              has(rec, "Pa&nel Edit...") && has(rec, "Goto Parent Container"));
#else
        check("Release drops the on-a-module variants too, not just the background ones",
              !has(rec, "Pa&nel Edit...") && !has(rec, "Goto Parent Container"));
#endif
        check("\"Locked Groove\" is NOT caught by the \"Locked\" rule", has(rec, "Locked Groove"));
        check("bare \"Arrange\" is NOT caught by the \"&Arrange\" rule", has(rec, "Arrange"));
        check("\"Show Circuit\" passes through untouched", has(rec, "Show Circuit"));
    }

    // ---------------------------------------------------------------------
    // 4. Separators: collapsed, never leading, never trailing.
    //
    // Removing a group leaves the separator that introduced it. Three
    // consecutive rules at the top of a menu is the visible symptom, and it is
    // the kind of thing that ships because nobody writes it down as a bug.
    // ---------------------------------------------------------------------
    {
        std::printf("separators\n");
        RecordingSink rec;
        tide::FilteringContextItemSink f(&rec, &rec, tide::MenuView::Panel);

        f.addItem("", 0, kSep, nullptr);                 // leading
        f.addItem("Locked", 0, 0, nullptr);              // dropped
        f.addItem("", 0, kSep, nullptr);
        f.addItem("", 0, kSep, nullptr);                 // doubled
        f.addItem("Cut", 0, 0, nullptr);
        f.addItem("", 0, kSep, nullptr);
        f.addItem("Goto Structure...", 0, 0, nullptr);   // dropped
        f.addItem("", 0, kSep, nullptr);                 // trailing

        std::printf("       -> %s\n", joined(rec).c_str());

        check("exactly one item survives, and no separator with it",
              rec.items.size() == 1 && rec.items[0].text == "Cut");
    }

    // ---------------------------------------------------------------------
    // 5. Everything else is forwarded VERBATIM, and the wrapper delegates.
    // ---------------------------------------------------------------------
    {
        std::printf("passthrough\n");
        RecordingSink rec;
        tide::FilteringContextItemSink f(&rec, &rec, tide::MenuView::Structure);

        auto* cb = reinterpret_cast<gmpi::api::IUnknown*>(&rec);
        f.addItem("Help...", 4242, (int32_t)gmpi::api::PopupMenuFlags::Grayed, cb);

        check("text, id, flags and callback all arrive unchanged",
              rec.items.size() == 1
              && rec.items[0].text == "Help..."
              && rec.items[0].id == 4242
              && rec.items[0].flags == (int32_t)gmpi::api::PopupMenuFlags::Grayed
              && rec.items[0].callback == cb);

        void* out = nullptr;
        check("queryInterface(IContextItemSink) returns the WRAPPER",
              f.queryInterface(&gmpi::api::IContextItemSink::guid, &out) == gmpi::ReturnCode::Ok
              && out == static_cast<gmpi::api::IContextItemSink*>(&f));

        out = nullptr;
        check("every other interface is delegated to the real sink",
              f.queryInterface(&RecordingSink::probeGuid, &out) == gmpi::ReturnCode::Ok
              && out == static_cast<gmpi::api::IContextItemSink*>(&rec));
    }

    // ---------------------------------------------------------------------
    // 6. The table is the ruling, and nothing has quietly been added to it.
    //
    // This one exists so that a green run can never mean "TIDE has started
    // hiding something nobody agreed to". Ten rules, and the count is the
    // ruling's own: 4 from item 1 (Arrange, Skin, Locked, Goto Structure),
    // 1 from item 2 (Delete (keep wires)), 2 from item 3 (Arrange again --
    // already counted -- and Screenshot), and 4 Release-only strings covering
    // item 3's two names in both their branches.
    // ---------------------------------------------------------------------
    {
        std::printf("the table itself\n");
        check("the shipped table holds exactly the ten rules of the ruling",
              tide::menuSuppressions().size() == 10);
    }

    std::printf("\n%s  %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
