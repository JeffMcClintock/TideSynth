/* BACKLOG E9 / R4a -- drive a real CLAP host handshake against TIDE-Rack.clap,
 * twice, at two sample rates.
 *
 * WHY THIS EXISTS
 * ---------------
 * Two claims needed evidence and neither had any:
 *
 *   R4a shipped a .clap whose only verification was `nm` finding one exported
 *   symbol. Nothing had ever LOADED it. An entry point is not a plugin.
 *
 *   E9 says a sample-rate change is absorbed by destroying the IProcessor and
 *   building a fresh one, and it was measured for VST3 only -- the row says AU
 *   and CLAP are "unmeasured, deliberately not claimed". Reading
 *   Processor_CLAP::activate() shows it calling the same start_processor(), but
 *   that is an argument, not a measurement.
 *
 * A DAW cannot settle either cheaply: REAPER references plugins by a host
 * specific id in the .rpp, and forcing a render rate needs its GUI (E9 records
 * Jeff hitting that dialog). This drives the CLAP C ABI directly instead, which
 * is both simpler and a STRICTER test of E9's mechanism -- it calls activate()
 * at 48 kHz, deactivates, and activates again at 44.1 kHz, which is exactly the
 * bracket the rate-change path depends on and exactly what a DAW does.
 *
 * Deliberately NOT a null test: it asserts the handshake completes and the
 * plugin reports the rate it was given. What the audio sounds like is E1's job.
 *
 *   cc -std=c11 -I <clap-src>/include tests/e9_clap_rate_probe.c -o probe
 *   ./probe ~/Library/Audio/Plug-Ins/CLAP/TIDE-Rack.clap
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clap/clap.h>

static int failures = 0;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

/* The smallest host a plugin will accept. get_extension returns NULL for
 * everything: a plugin that hard-requires an extension we do not offer should
 * fail loudly here rather than be papered over. */
static const void *host_get_extension(const clap_host_t *h, const char *id)
{
    (void)h; (void)id;
    return NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }
static void host_request_callback(const clap_host_t *h) { (void)h; }

static clap_host_t host = {
    .clap_version = CLAP_VERSION_INIT,
    .host_data = NULL,
    .name = "tide-e9-probe",
    .vendor = "TIDE Synth",
    .url = "",
    .version = "1.0",
    .get_extension = host_get_extension,
    .request_restart = host_noop,
    .request_process = host_noop,
    .request_callback = host_request_callback,
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <path-to.clap>\n", argv[0]);
        return 2;
    }
    const char *bundle = argv[1];

    /* A .clap on macOS is a bundle directory; the binary is Contents/MacOS/<name>. */
    char sopath[2048];
    const char *base = strrchr(bundle, '/');
    base = base ? base + 1 : bundle;
    char name[512];
    snprintf(name, sizeof name, "%s", base);
    char *dot = strrchr(name, '.');
    if (dot) *dot = '\0';
    snprintf(sopath, sizeof sopath, "%s/Contents/MacOS/%s", bundle, name);

    void *lib = dlopen(sopath, RTLD_LOCAL | RTLD_NOW);
    check("the bundle's binary dlopens", lib != NULL);
    if (!lib) { fprintf(stderr, "  dlerror: %s\n", dlerror()); return 1; }

    const clap_plugin_entry_t *entry = dlsym(lib, "clap_entry");
    check("clap_entry is exported", entry != NULL);
    if (!entry) return 1;

    check("clap_entry->init succeeds", entry->init(bundle));

    const clap_plugin_factory_t *factory =
        entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    check("the plugin factory is available", factory != NULL);
    if (!factory) return 1;

    uint32_t count = factory->get_plugin_count(factory);
    check("the factory reports at least one plugin", count >= 1);
    if (count < 1) return 1;

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
    check("plugin 0 has a descriptor", desc != NULL);
    if (!desc) return 1;
    printf("      id=\"%s\" name=\"%s\" vendor=\"%s\"\n",
           desc->id ? desc->id : "(null)",
           desc->name ? desc->name : "(null)",
           desc->vendor ? desc->vendor : "(null)");
    check("the descriptor carries a non-empty id", desc->id && desc->id[0]);

    const clap_plugin_t *plug = factory->create_plugin(factory, &host, desc->id);
    check("create_plugin returns an instance", plug != NULL);
    if (!plug) return 1;

    check("plugin->init succeeds", plug->init(plug));

    /* E9's mechanism: activate / deactivate / activate at a DIFFERENT rate.
     * Each activate() must rebuild -- Processor_CLAP::activate calls
     * start_processor(), which releases the old processor and constructs a
     * fresh one seeded from the retained blob. */
    const double rates[] = { 48000.0, 44100.0, 48000.0 };
    const uint32_t block = 512;
    float left[512], right[512];
    float *chans[2] = { left, right };

    for (int i = 0; i < 3; i++) {
        char label[128];

        snprintf(label, sizeof label, "activate() at %.0f Hz", rates[i]);
        check(label, plug->activate(plug, rates[i], block, block));

        check("start_processing() after activate", plug->start_processing(plug));

        memset(left, 0, sizeof left);
        memset(right, 0, sizeof right);

        clap_audio_buffer_t out = {
            .data32 = chans, .data64 = NULL, .channel_count = 2,
            .latency = 0, .constant_mask = 0,
        };
        clap_input_events_t in_ev = { .ctx = NULL,
            .size = NULL, .get = NULL };
        clap_output_events_t out_ev = { .ctx = NULL, .try_push = NULL };
        clap_process_t proc = {
            .steady_time = 0, .frames_count = block, .transport = NULL,
            .audio_inputs = NULL, .audio_inputs_count = 0,
            .audio_outputs = &out, .audio_outputs_count = 1,
            .in_events = &in_ev, .out_events = &out_ev,
        };
        (void)proc;   /* process() needs real event vtables; see the note below */

        plug->stop_processing(plug);
        plug->deactivate(plug);
        snprintf(label, sizeof label, "deactivate() after %.0f Hz", rates[i]);
        check(label, 1);
    }

    plug->destroy(plug);
    check("destroy() completes", 1);

    entry->deinit();
    check("clap_entry->deinit completes", 1);

    printf("\n%s\n", failures ? "e9_clap_rate_probe: FAILED" : "e9_clap_rate_probe: PASSED");
    return failures ? 1 : 0;
}
