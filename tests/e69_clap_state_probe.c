/* BACKLOG E69 -- does the CLAP wrapper's stateSave read the CONTROLLER's
 * freshly-synced store, or echo the processor's?
 *
 * WHY THIS EXISTS
 * ---------------
 * E68 fixed the VST3 save by making `Processor_VST3::getState` PULL a freshly
 * minted preset from the paired controller instead of echoing the processor's
 * store, which is only as fresh as the last async message the audio thread
 * applied. GMPI_Wrappers#36 then did the same for CLAP and AU3 -- and shipped
 * saying "CLAP compiled and installed on Windows; AU3 is mac-only, CI will
 * say." CI says a thing compiles. It does not say a save is fresh.
 *
 * A DAW is the obvious instrument and the wrong one here: driving REAPER's GUI
 * to load-edit-save needs an unlocked screen and the developer's desktop, and
 * REAPER's CLAP state route is separately broken (BACKLOG E60). The CLAP C ABI
 * needs neither. `Processor_CLAP`'s constructor creates and initialises the
 * plug-in's own controller unconditionally (Processor_CLAP.cpp:88-99, TIDE
 * BACKLOG S43(ii)) -- no host GUI extension involved -- so a five-call bare
 * host exercises exactly the branch #36 added:
 *
 *     if (pluginController) { pluginController->syncState();
 *                             st = controller.gmpiController.getPresetXml(); }
 *     else                  { st = plugin.getPresetUnsafe(); }
 *
 * WHAT IT MEASURES, and why each save is worth taking
 * --------------------------------------------------
 *   save #0, on a FRESH instance -- nothing loaded, nothing edited. TIDE's
 *   controller holds the default rack from construction ("TideApp fresh -
 *   holds the DEFAULT rack until setParameter restores one"); the processor's
 *   store does not. So the two stores DISAGREE at this moment, which is what
 *   makes an otherwise boring save a discriminator.
 *
 *   save #1, after state->load(preset) -- the round trip a host performs. The
 *   loaded bytes are the control: an echo writes back what it was handed, a
 *   fresh mint does not, and E68 measured exactly that shape on VST3
 *   (14,136 in -> 14,494 out).
 *
 * Deliberately NOT an audio test, and not a claim about cables reaching the
 * DSP: it reports the bytes a host would write to its project file. What is in
 * them is the caller's to check -- scripts/dump_preset.py counts the cables.
 *
 *   cc -std=c11 -I <clap-src>/include tests/e69_clap_state_probe.c -o probe
 *   ./probe <TIDE-Rack.clap> <out-prefix> [preset.xml]
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
    .name = "tide-e69-probe",
    .vendor = "TIDE Synth",
    .url = "",
    .version = "1.0",
    .get_extension = host_get_extension,
    .request_restart = host_noop,
    .request_process = host_noop,
    .request_callback = host_noop,
};

/* --- the two streams. A growable sink and a fixed source, nothing more. --- */

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

/* Writes what the host would have written to its project file. The wrapper
 * appends a NUL to the stream (Processor_CLAP::stateSave writes length+1), so
 * trim it -- an XML file with a trailing NUL is not one. */
static void write_out(const char *prefix, const char *suffix, const sink_t *sk)
{
    char path[2048];
    snprintf(path, sizeof path, "%s%s", prefix, suffix);
    size_t n = sk->len;
    while (n && sk->buf[n - 1] == '\0') --n;
    FILE *f = fopen(path, "wb");
    if (!f) { printf("      (could not write %s)\n", path); return; }
    fwrite(sk->buf, 1, n, f);
    fclose(f);
    printf("      %zu bytes -> %s\n", n, path);
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

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <path-to.clap> <out-prefix> [preset.xml]\n", argv[0]);
        return 2;
    }
    const char *bundle = argv[1];
    const char *prefix = argv[2];
    const char *presetPath = argc > 3 ? argv[3] : NULL;

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

    const clap_plugin_t *plug = factory->create_plugin(factory, &host, desc->id);
    check("create_plugin returns an instance", plug != NULL);
    if (!plug) return 1;
    check("plugin->init succeeds", plug->init(plug));

    const clap_plugin_state_t *state =
        (const clap_plugin_state_t *)plug->get_extension(plug, CLAP_EXT_STATE);
    check("the plugin offers clap.state", state != NULL);
    if (!state) return 1;

    /* --- save #0: a fresh instance, nothing loaded and nothing edited --- */
    sink_t s0 = {0};
    clap_ostream_t os0 = { .ctx = &s0, .write = sink_write };
    check("state->save on a fresh instance", state->save(plug, &os0));
    printf("      save#0 (fresh instance): %zu bytes\n", s0.len);
    write_out(prefix, ".fresh.xml", &s0);

    /* --- save #1: after loading a prepared preset --- */
    if (presetPath) {
        size_t plen = 0;
        char *pbuf = slurp(presetPath, &plen);
        check("the preset file reads", pbuf != NULL);
        if (pbuf) {
            printf("      preset in: %zu bytes (%s)\n", plen, presetPath);
            source_t src = { .buf = pbuf, .len = plen, .pos = 0 };
            clap_istream_t is = { .ctx = &src, .read = source_read };
            check("state->load accepts the preset", state->load(plug, &is));

            sink_t s1 = {0};
            clap_ostream_t os1 = { .ctx = &s1, .write = sink_write };
            check("state->save after load", state->save(plug, &os1));

            size_t out = s1.len;
            while (out && s1.buf[out - 1] == '\0') --out;
            printf("      save#1 (after load): %zu bytes\n", out);
            write_out(prefix, ".afterload.xml", &s1);

            /* The discriminator E68 used, restated: an echo writes back what it
             * was handed, so a DIFFERENT size proves the save did not echo.
             *
             * The converse does not hold, and it bit here: the mint is a fixed
             * point, so feeding a previously-minted document back in produces
             * the identical bytes and this line reads "same size" on a save
             * that minted. Only the difference is evidence; sameness is a
             * prompt to diff the files, which is why they are written out. */
            printf("      IN=%zu OUT=%zu  %s\n", plen, out,
                   out == plen ? "SAME SIZE -- inconclusive; diff the files"
                               : "DIFFERENT SIZE -- the save did not echo its input");
            free(pbuf);
        }
    }

    plug->destroy(plug);
    entry->deinit();

    printf("\n%s\n", failures ? "e69_clap_state_probe: FAILED" : "e69_clap_state_probe: PASSED");
    return failures ? 1 : 0;
}
