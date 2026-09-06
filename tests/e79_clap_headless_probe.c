/* BACKLOG E79 -- does a hosted CLAP that never opens its editor receive its
 * document, or does it play silence?
 *
 * E79 was filed from Linux, where the answer is "silence": the Linux CLAP
 * registers its host timer in guiSetParent (GMPI_Wrappers/wrapper/CLAP/
 * Editor_CLAP.cpp:269) and that timer is the only UI-thread tick a hosted
 * plug-in gets on that platform, so with no window nothing services the
 * controller->processor queue and the restored document never reaches the DSP.
 *
 * THE REASON THIS PROBE EXISTS RATHER THAN A SECOND REAPER SESSION. E79's own
 * row says to check the other platforms before assuming it is CLAP-only, and
 * the two platforms differ in a way that is visible in the source rather than
 * only in a measurement:
 *
 *   - Controller_CLAP's tick is NOT the editor's. It is a gmpi::TimerClient
 *     started in the constructor (Controller_CLAP.cpp:19) and stopped in the
 *     destructor -- its lifetime is the CONTROLLER's.
 *   - On macOS that TimerClient is backed by a CFRunLoopTimer added to
 *     CFRunLoopGetCurrent() in kCFRunLoopCommonModes (gmpi_ui helpers/
 *     Timer.cpp:136-138). So it fires whenever the thread that constructed the
 *     plug-in runs its run loop -- which every Cocoa host's main thread does,
 *     editor or no editor.
 *   - The host-timer registration E79 blames is inside `#if defined(__linux__)`.
 *
 * So the macOS prediction is "the document arrives with no editor", and the
 * thing that carries it is the host's RUN LOOP, not the host's window.
 *
 * WHICH IS ALSO THE TRAP, and it is why this probe has two arms. A bare CLAP
 * host that never runs a CFRunLoop would reproduce E79's symptom perfectly on a
 * perfectly healthy macOS build -- because the probe starved the timer, not
 * because the plug-in did. A one-armed probe here would have produced a
 * confident false confirmation.
 *
 *   --runloop     (default) spin the main run loop between blocks, the way a
 *                 Cocoa host's main thread does. This is the EXPERIMENT.
 *   --no-runloop  never spin it. This is the POSITIVE CONTROL: it starves the
 *                 same timer Linux has no source for, so it should show E79's
 *                 symptom on a healthy build. A PASS in the first arm only
 *                 means something if this arm FAILS.
 *
 * The editor is never created: gui.create/gui.set_parent are not called and
 * clap.gui is never even queried. That is the whole point of the measurement.
 *
 * Build (macOS):
 *   cc -std=c11 -I build-e79/_deps/clap-src/include \
 *      tests/e79_clap_headless_probe.c -framework CoreFoundation -o /tmp/e79probe
 *
 * Run:
 *   /tmp/e79probe build-e79/SynthEditSem/TIDE-Rack.clap <preset.xml>
 *
 * The preset is the OUTER <Preset> element, which is what
 * `scripts/decode_rpp.py --preset-out` writes out of a REAPER project. The
 * inner <Document> alone is NOT restorable -- the wrapper looks for the
 * <Preset> wrapper.
 */

#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clap/clap.h>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

static int failures = 0;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

/* The smallest host a plugin will accept -- deliberately offering no
 * extensions, so anything the wrapper hard-requires fails loudly here. Same
 * stub as tests/e69_clap_state_probe.c, and deliberately so: if TIDE's CLAP
 * ever starts hard-requiring a host extension, both probes should notice. */
static const void *host_get_extension(const clap_host_t *h, const char *id)
{
    (void)h; (void)id;
    return NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }

static clap_host_t host = {
    .clap_version = CLAP_VERSION_INIT,
    .host_data = NULL,
    .name = "tide-e79-probe",
    .vendor = "TIDE Synth",
    .url = "",
    .version = "1.0",
    .get_extension = host_get_extension,
    .request_restart = host_noop,
    .request_process = host_noop,
    .request_callback = host_noop,
};

typedef struct { const char *buf; size_t len, pos; } source_t;

static int64_t source_read(const clap_istream_t *s, void *buffer, uint64_t size)
{
    source_t *sr = (source_t *)s->ctx;
    size_t left = sr->len - sr->pos;
    size_t n = size < left ? (size_t)size : left;
    memcpy(buffer, sr->buf + sr->pos, n);
    sr->pos += n;
    return (int64_t)n;
}

static char *slurp(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, (size_t)n, f);
    fclose(f);
    b[rd] = 0;
    *out_len = rd;
    return b;
}

/* Let the host's main thread run, the way a Cocoa app's does between audio
 * callbacks. This is the ONLY thing separating the two arms. */
static void spin_runloop(double seconds)
{
#if defined(__APPLE__)
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, seconds, false);
#else
    (void)seconds;
#endif
}

/* ---- events: a single note-on, so the rack's ADSR actually opens ---------- */

typedef struct {
    clap_event_note_t note;
    int  deliver;         /* 1 while the note has not been handed over yet */
} events_t;

static uint32_t in_size(const struct clap_input_events *list)
{
    events_t *e = (events_t *)list->ctx;
    return e->deliver ? 1u : 0u;
}

static const clap_event_header_t *in_get(const struct clap_input_events *list, uint32_t index)
{
    events_t *e = (events_t *)list->ctx;
    if (!e->deliver || index != 0) return NULL;
    return &e->note.header;
}

static bool out_try_push(const struct clap_output_events *list, const clap_event_header_t *ev)
{
    (void)list; (void)ev;
    return true;
}

int main(int argc, char **argv)
{
    const char *bundle = NULL, *presetPath = NULL;
    int useRunloop = 1, blocks = 400, loadPreset = 1;
    const uint32_t blockSize = 512;
    const double sampleRate = 44100.0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--runloop"))         useRunloop = 1;
        else if (!strcmp(argv[i], "--no-runloop")) useRunloop = 0;
        else if (!strcmp(argv[i], "--no-preset"))  loadPreset = 0;
        else if (!strcmp(argv[i], "--blocks") && i + 1 < argc) blocks = atoi(argv[++i]);
        else if (!bundle)     bundle = argv[i];
        else if (!presetPath) presetPath = argv[i];
    }
    if (!bundle || !presetPath) {
        fprintf(stderr,
            "usage: %s <path-to.clap> <preset.xml> [--runloop|--no-runloop] [--blocks N]\n",
            argv[0]);
        return 2;
    }

    printf("e79_clap_headless_probe: arm = %s%s, %d blocks of %u at %.0f Hz, editor NEVER created\n\n",
           useRunloop ? "--runloop (a host main thread runs)"
                      : "--no-runloop (starves the controller's timer)",
           loadPreset ? "" : " + --no-preset (NEGATIVE CONTROL: no document is ever restored)",
           blocks, blockSize, sampleRate);

    /* A .clap on macOS is a bundle directory; the binary is Contents/MacOS/<name>. */
    char sopath[2048];
    const char *base = strrchr(bundle, '/');
    base = base ? base + 1 : bundle;
    char name[512];
    snprintf(name, sizeof name, "%s", base);
    char *dot = strrchr(name, '.');
    if (dot) *dot = '\0';
#if defined(__APPLE__)
    snprintf(sopath, sizeof sopath, "%s/Contents/MacOS/%s", bundle, name);
#else
    snprintf(sopath, sizeof sopath, "%s", bundle);
#endif

    void *lib = dlopen(sopath, RTLD_LOCAL | RTLD_NOW);
    check("the bundle's binary dlopens", lib != NULL);
    if (!lib) { fprintf(stderr, "  dlerror: %s\n", dlerror()); return 1; }

    const clap_plugin_entry_t *entry = dlsym(lib, "clap_entry");
    check("clap_entry is exported", entry != NULL);
    if (!entry) return 1;
    check("clap_entry->init succeeds", entry->init(bundle));

    const clap_plugin_factory_t *factory = entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    check("the plugin factory is offered", factory != NULL);
    if (!factory) return 1;

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
    check("descriptor 0 exists", desc != NULL);
    if (!desc) return 1;

    const clap_plugin_t *plug = factory->create_plugin(factory, &host, desc->id);
    check("create_plugin returns an instance", plug != NULL);
    if (!plug) return 1;
    check("plugin->init succeeds", plug->init(plug));

    const clap_plugin_state_t *state =
        (const clap_plugin_state_t *)plug->get_extension(plug, CLAP_EXT_STATE);
    check("the plugin offers clap.state", state != NULL);
    if (!state) return 1;

    char *preset = NULL;
    if (loadPreset) {
        size_t presetLen = 0;
        preset = slurp(presetPath, &presetLen);
        check("the preset file reads", preset != NULL);
        if (!preset) return 1;
        printf("      loading a %zu byte preset\n", presetLen);

        source_t src = { preset, presetLen, 0 };
        clap_istream_t is = { .ctx = &src, .read = source_read };
        check("state->load accepts the preset", state->load(plug, &is));
    } else {
        printf("      NO state->load at all (negative control: the DEFAULT rack)\n");
    }

    /* THE HANDOVER WINDOW. A real host restores state and then goes back to its
     * event loop before the first audio callback. That gap is where
     * Controller_CLAP::onTimer ships the document to the processor -- on a
     * platform where anything ticks it. The control arm skips it. */
    if (useRunloop) {
        printf("      spinning the run loop for 0.5 s (the host's restore->play gap)\n");
        spin_runloop(0.5);
    } else {
        printf("      NOT spinning the run loop (control arm)\n");
    }

    check("activate succeeds", plug->activate(plug, sampleRate, 1, blockSize));
    check("start_processing succeeds", plug->start_processing(plug));

    /* --- buffers ---------------------------------------------------------- */
    float *L = calloc(blockSize, sizeof(float));
    float *R = calloc(blockSize, sizeof(float));
    float *chans[2] = { L, R };
    clap_audio_buffer_t outBus = {
        .data32 = chans, .data64 = NULL, .channel_count = 2,
        .latency = 0, .constant_mask = 0,
    };

    events_t ev;
    memset(&ev, 0, sizeof ev);
    ev.note.header.size     = sizeof(clap_event_note_t);
    ev.note.header.time     = 0;
    ev.note.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.note.header.type     = CLAP_EVENT_NOTE_ON;
    ev.note.header.flags    = 0;
    ev.note.note_id = -1;
    ev.note.port_index = 0;
    ev.note.channel = 0;
    ev.note.key = 60;          /* middle C, the note V1's pitch clause used */
    ev.note.velocity = 0.8;
    ev.deliver = 0;

    clap_input_events_t  inEv  = { .ctx = &ev, .size = in_size, .get = in_get };
    clap_output_events_t outEv = { .ctx = NULL, .try_push = out_try_push };

    double peak = 0.0, sumsq = 0.0;
    uint64_t frames = 0;
    int processFailures = 0;

    for (int b = 0; b < blocks; ++b) {
        /* Deliver the note-on once, after a couple of blocks, so the first
         * blocks also serve as a "before the note" reading. */
        ev.deliver = (b == 2) ? 1 : 0;

        memset(L, 0, blockSize * sizeof(float));
        memset(R, 0, blockSize * sizeof(float));

        clap_process_t p;
        memset(&p, 0, sizeof p);
        p.steady_time         = (int64_t)frames;
        p.frames_count        = blockSize;
        p.transport           = NULL;
        p.audio_inputs        = NULL;
        p.audio_inputs_count  = 0;
        p.audio_outputs       = &outBus;
        p.audio_outputs_count = 1;
        p.in_events           = &inEv;
        p.out_events          = &outEv;

        clap_process_status st = plug->process(plug, &p);
        if (st == CLAP_PROCESS_ERROR) ++processFailures;

        for (uint32_t i = 0; i < blockSize; ++i) {
            double a = fabs((double)L[i]), b2 = fabs((double)R[i]);
            if (a > peak) peak = a;
            if (b2 > peak) peak = b2;
            sumsq += (double)L[i] * L[i] + (double)R[i] * R[i];
        }
        frames += blockSize;

        /* A host's main thread keeps running while audio streams. Give it the
         * same slice each block; 1 ms over 400 blocks is ~0.4 s of run loop,
         * comfortably more than the controller's 15 ms tick needs. */
        if (useRunloop) spin_runloop(0.001);
    }

    check("no block returned CLAP_PROCESS_ERROR", processFailures == 0);

    double rms = sqrt(sumsq / (double)(frames * 2));
    double peakDb = peak > 0.0 ? 20.0 * log10(peak) : -INFINITY;
    double rmsDb  = rms  > 0.0 ? 20.0 * log10(rms)  : -INFINITY;

    printf("\n      %llu frames, peak %.6f (%.1f dBFS), rms %.1f dBFS\n",
           (unsigned long long)frames, peak, peakDb, rmsDb);

    /* The claim under test. E79's Accept asks for audio rather than -inf.
     *
     * With --no-preset the claim INVERTS. That arm restores nothing, so the
     * instance is running whatever rack it seeded itself with, and the number
     * it produces is the yardstick the experiment's number has to be different
     * from. Without it, "-6.3 dBFS with no editor" would be equally consistent
     * with "the restore worked" and with "the restore did nothing and the
     * default rack happens to make a sound". */
    if (loadPreset)
        check("the RESTORED rack produced audio with no editor ever created", peak > 0.0);
    else
        printf("      (negative control: this is the DEFAULT rack's output, for comparison)\n");

    plug->stop_processing(plug);
    plug->deactivate(plug);
    plug->destroy(plug);
    entry->deinit();
    free(L); free(R); free(preset);

    printf("\n%s\n", failures ? "e79_clap_headless_probe: FAILED"
                              : "e79_clap_headless_probe: PASSED");
    return failures ? 1 : 0;
}
