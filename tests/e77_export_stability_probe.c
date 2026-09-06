/* BACKLOG E77 -- what differs between two equal-length exports of the same
 * document, and is the difference reachable without a GUI host?
 *
 * WHY THIS EXISTS
 * ---------------
 * E59 made syncState() REFUSE to publish a document that is byte-equal to the
 * one the controller was born holding, because publishing the starter rack
 * before the host has restored anything poisons the bytes the processor holder
 * re-seeds into every later processor. The guard is a byte comparison of two
 * exportChunkXmlForSave() results, and its own comment states the assumption it
 * rests on: "Two exports within one process are stable".
 *
 * E73's first hosted-AUv3 log contradicted that, in two adjacent lines:
 *
 *     TIDE: controller #1 startup default is 17959 bytes (syncState will not
 *           publish this document)
 *     TIDE: controller #1 syncState exporting 17959 byte document (host asked
 *           for state)
 *
 * Same size, published anyway -- so the two exports differed somewhere the byte
 * COUNT does not show. E77 asks what, and the row's candidates are a timestamp,
 * a handle, or a pointer-valued parameter.
 *
 * The row also says the answer needs "a prepared rack restored into a hosted
 * AUv3", i.e. a GUI host, and on this box that means an unlocked screen and a
 * human. THAT IS TRUE OF THE ROW'S ACCEPT AND NOT OF ITS FIRST QUESTION. Two of
 * the three candidates -- a timestamp, and anything that drifts while a process
 * runs -- are properties of exportChunkXmlForSave() alone, and a bare CLAP host
 * exercises exactly the same controller.
 *
 * WHAT IT MEASURES, AND WHY THE SIZE OF THE SAVE IS THE WHOLE ANSWER
 * -----------------------------------------------------------------
 * Processor_CLAP::stateSave calls the paired controller's syncState() and then
 * serialises the controller's parameters. So the guard's decision is visible in
 * the SAVED BYTES with no log to read and no stderr to capture:
 *
 *     refusal fired  -> chunk parameter 1 stays empty -> save is ~86 bytes
 *     refusal failed -> the 17,9xx-byte startup default lands in parameter 1
 *                       -> save is ~18 KB
 *
 * That is E77 reproducing, or not, as one integer. A `TIDE: syncState exporting
 * ...` line on stderr says the same thing and is worth capturing too, but this
 * probe does not depend on being able to read it -- which matters, because the
 * configuration that redirects the plug-in's stderr to a file (TIDE_TRACE_LOG)
 * is OFF in every shipped build.
 *
 * THE VARIABLE, AND WHY IT IS TIME
 * --------------------------------
 * `--delay-sec N` sleeps between the controller's construction (which captures
 * startupDefaultChunk) and each save. A back-to-back pair is the control: if
 * the two exports are identical at 0 s and differ at 30 s, the difference is
 * time-carrying content -- the row's "timestamp" candidate -- and that is a
 * one-line finding rather than a session. If they are identical at every delay,
 * time is EXCLUDED, which is worth as much: it leaves only things that need
 * something to have happened in between, and an editor is the obvious one.
 *
 * `--saves K` takes K of them, so a drift that needs several exports to show up
 * is not missed by taking exactly two.
 *
 * WHAT IT DELIBERATELY DOES NOT ANSWER
 * ------------------------------------
 * Anything that requires an editor, a restore, or a host that behaves like a
 * DAW. A null result here does not clear the guard -- it narrows where the
 * difference can be hiding, and hands the AUv3 half a smaller question.
 *
 * BUILD
 *   cc -std=c11 -I <build>/_deps/clap-src/include \
 *      tests/e77_export_stability_probe.c -o probe
 *
 * RUN
 *   ./probe <build>/SynthEditSem/TIDE-Rack.clap /tmp/e77 --delay-sec 0
 *   ./probe <build>/SynthEditSem/TIDE-Rack.clap /tmp/e77 --delay-sec 30 --saves 3
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <clap/clap.h>

static int failures = 0;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

/* The smallest host a plugin will accept -- deliberately offering no
 * extensions, so anything the wrapper hard-requires fails loudly here. */
static const void *host_get_extension(const clap_host_t *h, const char *id)
{
    (void)h; (void)id;
    return NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }

static clap_host_t host = {
    .clap_version = CLAP_VERSION_INIT,
    .host_data = NULL,
    .name = "tide-e77-probe",
    .vendor = "TIDE Synth",
    .url = "",
    .version = "1.0",
    .get_extension = host_get_extension,
    .request_restart = host_noop,
    .request_process = host_noop,
    .request_callback = host_noop,
};

typedef struct { char *buf; size_t len, cap; } sink_t;

static int64_t sink_write(const clap_ostream_t *s, const void *buffer, uint64_t size)
{
    sink_t *sk = (sink_t *)s->ctx;
    if (sk->len + size + 1 > sk->cap) {
        size_t want = (sk->len + size + 1) * 2;
        char *n = realloc(sk->buf, want);
        if (!n) return -1;
        sk->buf = n; sk->cap = want;
    }
    memcpy(sk->buf + sk->len, buffer, (size_t)size);
    sk->len += (size_t)size;
    sk->buf[sk->len] = 0;
    return (int64_t)size;
}

/* The wrapper appends a NUL to the stream (Processor_CLAP::stateSave writes
 * length+1), so trim it -- an XML file with a trailing NUL is not one. */
static size_t trimmed(const sink_t *sk)
{
    size_t n = sk->len;
    while (n && sk->buf[n - 1] == '\0') --n;
    return n;
}

static void write_out(const char *prefix, int index, const sink_t *sk)
{
    char path[2048];
    snprintf(path, sizeof path, "%s.save%d.xml", prefix, index);
    FILE *f = fopen(path, "wb");
    if (!f) { printf("      (could not write %s)\n", path); return; }
    fwrite(sk->buf, 1, trimmed(sk), f);
    fclose(f);
    printf("      %zu bytes -> %s\n", trimmed(sk), path);
}

/* The threshold that separates the two outcomes, and it is nowhere near either
 * of them: a refusal leaves parameter 1 empty and the save is under a hundred
 * bytes, while publishing the startup default puts ~18 KB into it. Anything
 * between the two is a THIRD outcome nobody has seen, and is reported as such
 * rather than bucketed into whichever side it is nearer. */
#define PUBLISHED_FLOOR 1000

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
                "usage: %s <path-to.clap> <out-prefix> [--delay-sec N] [--saves K]\n",
                argv[0]);
        return 2;
    }
    const char *bundle = argv[1];
    const char *prefix = argv[2];
    unsigned delaySec = 0;
    int saves = 2;

    for (int i = 3; i < argc; ++i) {
        if (0 == strcmp(argv[i], "--delay-sec") && i + 1 < argc)
            delaySec = (unsigned)strtoul(argv[++i], NULL, 10);
        else if (0 == strcmp(argv[i], "--saves") && i + 1 < argc)
            saves = (int)strtol(argv[++i], NULL, 10);
        else { fprintf(stderr, "unknown argument: %s\n", argv[i]); return 2; }
    }
    if (saves < 1) saves = 1;

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

    const clap_plugin_factory_t *factory = entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    check("the plugin factory is available", factory != NULL);
    if (!factory) return 1;

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
    check("plugin 0 has a descriptor", desc != NULL);
    if (!desc) return 1;

    /* create_plugin is where the controller is built and initialize()d, and
     * therefore where startupDefaultChunk is captured. Every delay below is
     * measured from HERE, which is the clock the guard's comparison rides on. */
    const clap_plugin_t *plug = factory->create_plugin(factory, &host, desc->id);
    check("create_plugin returns an instance", plug != NULL);
    if (!plug) return 1;
    check("plugin->init succeeds", plug->init(plug));

    const clap_plugin_state_t *state =
        (const clap_plugin_state_t *)plug->get_extension(plug, CLAP_EXT_STATE);
    check("the plugin offers clap.state", state != NULL);
    if (!state) return 1;

    printf("      delay %u s between saves, %d save(s)\n", delaySec, saves);

    int published = 0;
    int inconclusive = 0;
    size_t first = 0;

    for (int i = 0; i < saves; ++i) {
        if (delaySec) sleep(delaySec);

        sink_t sk = {0};
        clap_ostream_t os = { .ctx = &sk, .write = sink_write };
        char label[128];
        snprintf(label, sizeof label, "state->save #%d", i);
        check(label, state->save(plug, &os));

        const size_t n = trimmed(&sk);
        if (0 == i) first = n;

        printf("      save#%d: %zu bytes  %s\n", i, n,
               n >= PUBLISHED_FLOOR
                   ? "PUBLISHED -- the E59 refusal did NOT fire (E77 reproduces)"
                   : "refused -- the two exports were byte-equal");
        if (n >= PUBLISHED_FLOOR) published = 1;
        if (n != first) inconclusive = 1;

        write_out(prefix, i, &sk);
        free(sk.buf);
    }

    /* The finding, stated as the thing a reader wants and not as an exit code.
     * A refusal at every delay is a NEGATIVE result and it is the useful half
     * of this probe: it excludes time-carrying content from the document, which
     * is one of the three candidates E77's row names. */
    printf("\n");
    if (published)
        printf("E77 REPRODUCES headlessly at delay=%u s: syncState published the "
               "startup default.\n", delaySec);
    else
        printf("E77 does NOT reproduce headlessly at delay=%u s: %d export(s) "
               "byte-equal to the startup default.\n", delaySec, saves);
    if (inconclusive)
        printf("NOTE: the saves are not all the same size -- read the written "
               "files before concluding anything.\n");

    plug->destroy(plug);
    entry->deinit();

    printf("\ne77_export_stability_probe: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}
