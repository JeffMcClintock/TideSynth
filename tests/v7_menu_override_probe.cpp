/* BACKLOG V7 -- prove the context-menu override HOOK works, while it renames
 * nothing.
 *
 * WHY THIS EXISTS AT ALL, and it is the awkward part of V7.
 *
 * V7's ruling splits the work in two. The mechanism is takeable now; the four
 * replacement strings are not -- they turn on what a TIDE user expects to read,
 * which is Jeff's call, and the row says in as many words: "Do not land the
 * override carrying placeholder strings." So `menuNameOverrides()` ships EMPTY.
 *
 * That leaves a hook whose entire observable behaviour is "changes nothing",
 * and a hook nobody has watched work is not a hook -- the same argument
 * tests/rack-content/ makes for its negative controls. A screenshot cannot
 * distinguish "the override mechanism is correct and the table is empty" from
 * "the override mechanism is never called". This can: it drives the sink
 * directly with a table of its own and asserts on what comes out the far side.
 *
 * WHAT IT ASSERTS
 *   1. an entry in the table renames, on the WHOLE string
 *   2. an item not in the table passes through UNCHANGED
 *   3. a near-miss does NOT match -- "Pa&nel Edit..." is a different item from
 *      "Panel Edit..." in EditorLib, and a substring rule would catch both
 *   4. id, flags and the callback pointer are forwarded untouched -- the sink
 *      is a text filter and must not become anything else
 *   5. queryInterface hands back the wrapper for IContextItemSink and
 *      DELEGATES everything else, which is what keeps a host that also
 *      implements IPopupMenu working
 *   6. the SHIPPED table is empty, so this probe passing never means TIDE has
 *      quietly started renaming things
 *
 * BUILD
 *   clang++ -std=c++20 tests/v7_menu_override_probe.cpp \
 *     -I SynthEditSem -I build/_deps/gmpi_ui-src -I build/_deps/gmpi-src/Core \
 *     -o v7_menu_override_probe
 *
 * RUN
 *   ./v7_menu_override_probe        # exit 0 = the hook is wired correctly
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

/// The wrapper's rename is a free function on the shipped table, so a test
/// table needs its own filter. Same rule -- whole-string compare -- written out
/// here rather than parameterised into the header: the header's job is to hold
/// the ONE table TIDE ships, and a seam for injecting another would be a seam
/// for shipping the wrong one.
struct TestSink : gmpi::api::IContextItemSink
{
    gmpi::api::IContextItemSink* inner;
    const std::vector<tide::MenuNameOverride>& table;

    TestSink(gmpi::api::IContextItemSink* i, const std::vector<tide::MenuNameOverride>& t)
        : inner(i), table(t) {}

    gmpi::ReturnCode addItem(const char* text, int32_t id, int32_t flags,
                             gmpi::api::IUnknown* callback) override
    {
        const char* out = text;
        for (const auto& o : table)
            if (o.from && text && 0 == std::strcmp(text, o.from))
                out = o.to;
        return inner->addItem(out, id, flags, callback);
    }
    gmpi::ReturnCode queryInterface(const gmpi::api::Guid*, void** r) override
    { *r = {}; return gmpi::ReturnCode::NoSupport; }
    int32_t addRef() override  { return 1; }
    int32_t release() override { return 1; }
};

} // namespace

int main()
{
    std::printf("V7 -- context-menu name override\n\n");

    // --- the SHIPPED table renames nothing -------------------------------
    check("the shipped override table is EMPTY (V7: no placeholder strings)",
          tide::menuNameOverrides().empty());
    check("overrideMenuName() is therefore identity on a real EditorLib item",
          0 == std::strcmp("Panel Edit...", tide::overrideMenuName("Panel Edit...")));
    check("overrideMenuName(nullptr) is nullptr, not a crash",
          tide::overrideMenuName(nullptr) == nullptr);

    // --- the wrapper forwards faithfully ---------------------------------
    {
        RecordingSink rec;
        tide::RenamingContextItemSink wrapper(&rec, &rec);

        int callbackTarget = 0;
        auto* callback = reinterpret_cast<gmpi::api::IUnknown*>(&callbackTarget);

        wrapper.addItem("Panel Edit...", 7, 42, callback);

        check("one item reached the inner sink", rec.items.size() == 1);
        if (rec.items.size() == 1)
        {
            check("text passes through unchanged while the table is empty",
                  rec.items[0].text == "Panel Edit...");
            check("id is forwarded untouched",       rec.items[0].id == 7);
            check("flags are forwarded untouched",   rec.items[0].flags == 42);
            check("callback pointer is forwarded untouched",
                  rec.items[0].callback == callback);
        }
    }

    // --- and it DOES rename when the table says so ------------------------
    {
        const std::vector<tide::MenuNameOverride> table{
            { "Panel Edit...", "PROBE-RENAMED" },
        };

        RecordingSink rec;
        TestSink filter(&rec, table);

        filter.addItem("Panel Edit...", 0, 0, nullptr);   // in the table
        filter.addItem("Pa&nel Edit...", 0, 0, nullptr);  // NEAR MISS, different item
        filter.addItem("Goto Structure...", 0, 0, nullptr); // not in the table

        check("three items reached the inner sink", rec.items.size() == 3);
        if (rec.items.size() == 3)
        {
            check("a table entry renames",
                  rec.items[0].text == "PROBE-RENAMED");
            check("a NEAR MISS is left alone -- whole-string match, not substring",
                  rec.items[1].text == "Pa&nel Edit...");
            check("an unlisted item passes through",
                  rec.items[2].text == "Goto Structure...");
        }
    }

    // --- queryInterface: intercept one, delegate the rest -----------------
    {
        RecordingSink rec;
        tide::RenamingContextItemSink wrapper(&rec, &rec);

        void* out = nullptr;
        const auto rcSink = wrapper.queryInterface(&gmpi::api::IContextItemSink::guid, &out);
        check("queryInterface(IContextItemSink) succeeds",
              rcSink == gmpi::ReturnCode::Ok);
        check("...and returns the WRAPPER, so items are still renamed",
              out == static_cast<void*>(static_cast<gmpi::api::IContextItemSink*>(&wrapper)));

        out = nullptr;
        const auto rcOther = wrapper.queryInterface(&RecordingSink::probeGuid, &out);
        check("queryInterface(some other interface) is DELEGATED, not refused",
              rcOther == gmpi::ReturnCode::Ok);
        check("...and returns the INNER object, not the wrapper",
              out == static_cast<void*>(static_cast<gmpi::api::IContextItemSink*>(&rec)));
    }

    std::printf("\n%s: %d check(s) failed\n",
                failures ? "V7 OVERRIDE HOOK BROKEN" : "V7 override hook OK", failures);
    return failures ? 1 : 0;
}
