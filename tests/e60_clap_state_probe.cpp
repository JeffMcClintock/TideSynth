// BACKLOG E60 -- does TIDE's CLAP wrapper round-trip a prepared rack through
// clap_plugin_state, independently of any DAW?
//
// E60 measured, through REAPER 7.43, that a prepared rack cannot be got into a
// hosted CLAP instance while the identical construction works for VST3, and
// left the deciding question open in as many words: "whether the state is
// dropped by REAPER 7.43's CLAP implementation or by TIDE's own
// clap_plugin_state.save/load -- nobody has looked at the wrapper side".
//
// This is the wrapper side, with no DAW in the picture. It dlopens the .clap,
// instantiates the plug-in against a minimal host, calls state->load() with a
// preset read from a file, then state->save(), and reports the byte counts and
// both return values. A DAW cannot be blamed for what happens here, and neither
// can this harness be blamed for what a DAW does -- which is the whole point of
// splitting the measurement in two.
//
// Build (needs only the CLAP headers, which are header-only):
//   c++ -std=c++17 -I<clap-src>/include tests/e60_clap_state_probe.cpp -ldl \
//       -o e60_clap_state_probe
//
// Run:
//   ./e60_clap_state_probe <path/to/TIDE-Rack.clap> <preset.xml>
//
// Exit codes are deliberately distinct, because "the harness could not set up"
// and "the plug-in refused the state" must not look alike -- the E62 lesson:
//   0  round trip OK          the saved bytes come back
//   1  the plug-in refused    load() or save() returned false, or the sizes differ
//   2  harness/setup failure   could not dlopen, no factory, no state extension

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <dlfcn.h>

#include <clap/clap.h>

namespace
{

std::string readFile(const char* path)
{
    std::string out;
    FILE* f = fopen(path, "rb");
    if (!f)
        return out;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        out.append(buf, n);
    fclose(f);
    return out;
}

// ---- the input stream we hand to state->load() -------------------------------
// A real host reads from a file and returns short counts freely, so this returns
// at most 4096 bytes per call rather than the whole buffer -- a wrapper that
// only works when the first read delivers everything would pass a
// return-it-all stub and fail in a DAW.
struct InStream
{
    clap_istream_t api{};
    const std::string* data{};
    size_t pos{};
};

int64_t inRead(const clap_istream_t* s, void* buffer, uint64_t size)
{
    auto* self = static_cast<InStream*>(s->ctx);
    const size_t remaining = self->data->size() - self->pos;
    size_t n = size < remaining ? (size_t)size : remaining;
    if (n > 4096)
        n = 4096;
    memcpy(buffer, self->data->data() + self->pos, n);
    self->pos += n;
    return (int64_t)n;
}

// ---- the output stream we hand to state->save() ------------------------------
struct OutStream
{
    clap_ostream_t api{};
    std::string data;
};

int64_t outWrite(const clap_ostream_t* s, const void* buffer, uint64_t size)
{
    auto* self = static_cast<OutStream*>(s->ctx);
    self->data.append(static_cast<const char*>(buffer), (size_t)size);
    return (int64_t)size;
}

// ---- the minimal host --------------------------------------------------------
const void* hostGetExtension(const clap_host_t*, const char*) { return nullptr; }
void hostRequestRestart(const clap_host_t*) {}
void hostRequestProcess(const clap_host_t*) {}
void hostRequestCallback(const clap_host_t*) {}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <TIDE-Rack.clap> <preset.xml>\n", argv[0]);
        return 2;
    }
    const char* clapPath = argv[1];
    const char* presetPath = argv[2];

    const std::string preset = readFile(presetPath);
    if (preset.empty())
    {
        fprintf(stderr, "SETUP: could not read preset '%s'\n", presetPath);
        return 2;
    }
    printf("preset file      %s\n", presetPath);
    printf("preset bytes     %zu\n", preset.size());

    void* lib = dlopen(clapPath, RTLD_NOW | RTLD_LOCAL);
    if (!lib)
    {
        fprintf(stderr, "SETUP: dlopen failed: %s\n", dlerror());
        return 2;
    }

    auto* entry = static_cast<const clap_plugin_entry_t*>(dlsym(lib, "clap_entry"));
    if (!entry)
    {
        fprintf(stderr, "SETUP: no clap_entry symbol\n");
        return 2;
    }
    if (!entry->init(clapPath))
    {
        fprintf(stderr, "SETUP: clap_entry->init failed\n");
        return 2;
    }

    auto* factory = static_cast<const clap_plugin_factory_t*>(
        entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (!factory || factory->get_plugin_count(factory) == 0)
    {
        fprintf(stderr, "SETUP: no plugin factory / no plugins\n");
        return 2;
    }

    const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, 0);
    printf("plugin           %s (%s)\n", desc->name ? desc->name : "?",
           desc->id ? desc->id : "?");

    clap_host_t host{};
    host.clap_version = CLAP_VERSION;
    host.host_data = nullptr;
    host.name = "e60_clap_state_probe";
    host.vendor = "TIDE";
    host.url = "";
    host.version = "1";
    host.get_extension = hostGetExtension;
    host.request_restart = hostRequestRestart;
    host.request_process = hostRequestProcess;
    host.request_callback = hostRequestCallback;

    const clap_plugin_t* plugin = factory->create_plugin(factory, &host, desc->id);
    if (!plugin)
    {
        fprintf(stderr, "SETUP: create_plugin returned null\n");
        return 2;
    }
    if (!plugin->init(plugin))
    {
        fprintf(stderr, "SETUP: plugin->init failed\n");
        return 2;
    }

    auto* state = static_cast<const clap_plugin_state_t*>(
        plugin->get_extension(plugin, CLAP_EXT_STATE));
    if (!state)
    {
        fprintf(stderr, "SETUP: plugin does not implement %s\n", CLAP_EXT_STATE);
        return 2;
    }

    // The default state first. It is the negative control: without it, a saved
    // size that happens to look plausible cannot be told from the plug-in
    // reporting whatever it booted with -- the E52 empty-config lesson.
    OutStream before;
    before.api.ctx = &before;
    before.api.write = outWrite;
    const bool savedBefore = state->save(plugin, &before.api);
    printf("save(default)    %s, %zu bytes\n", savedBefore ? "true" : "false",
           before.data.size());

    InStream in;
    in.api.ctx = &in;
    in.api.read = inRead;
    in.data = &preset;
    const bool loaded = state->load(plugin, &in.api);
    printf("load(preset)     %s, %zu of %zu bytes consumed\n", loaded ? "true" : "false",
           in.pos, preset.size());

    OutStream after;
    after.api.ctx = &after;
    after.api.write = outWrite;
    const bool savedAfter = state->save(plugin, &after.api);
    printf("save(after load) %s, %zu bytes\n", savedAfter ? "true" : "false",
           after.data.size());

    plugin->destroy(plugin);
    entry->deinit();

    // The verdict. "It saved something" is not the test; the test is that what
    // comes back is the rack that went in, and that it differs from the default.
    int rc = 0;
    if (!loaded)
    {
        printf("VERDICT          FAIL -- state->load() returned false\n");
        rc = 1;
    }
    else if (!savedAfter)
    {
        printf("VERDICT          FAIL -- state->save() returned false after a good load\n");
        rc = 1;
    }
    else if (after.data.size() == before.data.size() && after.data == before.data)
    {
        printf("VERDICT          FAIL -- load() reported success but the saved state is "
               "byte-identical to the default; the preset was dropped\n");
        rc = 1;
    }
    else
    {
        printf("VERDICT          PASS -- the loaded rack survives a save (%zu bytes, "
               "default was %zu)\n",
               after.data.size(), before.data.size());
    }
    return rc;
}
